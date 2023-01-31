#include "quickjs.h"
#include "udf.h"
#include <malloc.h>
#include <stddef.h>
#include <string.h>
#include "msgpack.h"

static JSRuntime *rt;
static JSContext *ctx;
static JSValue global;
static JSValue msgpack;
static JSValue pack;
static JSValue unpack;

void js_std_dump_error(JSContext *ctx);
void js_std_add_helpers(JSContext *ctx, int argc, char **argv);
JSModuleDef *js_init_module_std(JSContext *ctx, const char *module_name);
JSModuleDef *js_init_module_os(JSContext *ctx, const char *module_name);

static int initialize()
//
// Initialize the Python interpreter
//
// Returns
// -------
// int : 0 for success, -1 for error
//
{
    int rc = 0;
    JSValue val1 = 0;
    JSValue val2 = 0;
    JSValue val3 = 0;

    if (rt) goto exit;

    rt = JS_NewRuntime();
    if (!rt)
    {
        fprintf(stderr, "could not allocate JS runtime\n");
        goto error;
    }

    ctx = JS_NewContext(rt);
    if (!ctx) 
    {
        fprintf(stderr, "could not allocate JS context\n");
        goto error;
    }

    js_std_add_helpers(ctx, -1, NULL);
    js_init_module_std(ctx, "std");
    js_init_module_os(ctx, "os");

    global = JS_GetGlobalObject(ctx);

    const char *str = "import * as std from 'std';\n"
                "import * as os from 'os';\n"
                "globalThis.std = std;\n"
                "globalThis.os = os;\n";

    val1 = JS_Eval(ctx, str, strlen(str), "<input>", JS_EVAL_TYPE_MODULE);
    if (JS_IsException(val1)) goto error;

    val2 = JS_Eval(ctx, MSGPACK_TXT, strlen(MSGPACK_TXT), "<input>", JS_EVAL_TYPE_MODULE);
    if (JS_IsException(val2)) goto error;

    msgpack = JS_GetPropertyStr(ctx, global, "msgpack");
    if (JS_IsException(msgpack)) goto error;

    pack = JS_GetPropertyStr(ctx, msgpack, "serialize");
    if (JS_IsException(pack)) goto error;

    unpack = JS_GetPropertyStr(ctx, msgpack, "deserialize");
    if (JS_IsException(unpack)) goto error;

    str = "std.loadScript('/app/main.js');\n";

    val3 = JS_Eval(ctx, str, strlen(str), "<input>", JS_EVAL_TYPE_MODULE);
    if (JS_IsException(val1)) goto error;

exit:
    JS_FreeValue(ctx, val1);
    JS_FreeValue(ctx, val2);
    JS_FreeValue(ctx, val3);
    return rc;

error:
    js_std_dump_error(ctx);
    rc = -1;
    goto exit;
}

int udf_exec(udf_string_t *code)
//
// Execute arbitrary code
//
// Parameters
// ----------
// code : string
//     The code to execute
//
// Returns
// -------
// int : 0 for success, -1 for error
//
{
    int rc = 0;
    JSValue val = 0;

    if (initialize()) goto error;

    val = JS_Eval(ctx, code->ptr, code->len, "<input>", JS_EVAL_TYPE_MODULE);
    if (JS_IsException(val)) goto error;

exit:
    JS_FreeValue(ctx, val);
    return rc;

error:
    js_std_dump_error(ctx);
    rc = -1;
    goto exit;
}

JSValue load_function(char *path, uint64_t path_l)
//
// Search a library path for a specified function
//
// Parameters
// ----------
// path : string
//     The path to the function
// path_l : int
//     The length of the string in `path`
//
// Returns
// -------
// JSValue containing function reference
//
{
    char *func = NULL;
    uint64_t func_l = 0;
    char *obj = path;
    char *end = path + path_l;
    JSValue js_func = 0;
    JSValue js_obj = global;
    JSValue js_obj2 = 0;

    for (uint64_t i = 0; i < path_l; i++)
    {
        if (path[i] == '.')
        {
            path[i] = '\0';
            js_obj2 = JS_GetPropertyStr(ctx, js_obj, obj);
            path[i] = '.';
            obj = path + i + 1;
            JS_FreeValue(ctx, js_obj);
            js_obj = js_obj2;
            js_obj2 = 0;
            if (JS_IsException(js_obj)) goto error;
        }
    }

    func_l = end - obj;
    func = malloc(func_l + 1);
    if (!func) goto error;
    memcpy(func, obj, func_l);
    func[func_l] = '\0';

    js_func = JS_GetPropertyStr(ctx, js_obj, func);
    if (JS_IsException(js_func)) goto error;

exit:
    if (js_obj != global) JS_FreeValue(ctx, js_obj);
    if (js_obj2 != global) JS_FreeValue(ctx, js_obj2);
    if (func) free(func);

    return js_func;

error:
    goto exit;
}

void udf_call(udf_string_t *name, udf_list_u8_t *args, udf_list_u8_t *ret)
//
// Call a function with the given arguments
//
// Parameters
// ----------
// name : string
//     Absolute path to a function. For example, `urllib.parse.urlparse`.
// args : bytes
//     MessagePack blob of function arguments
//
// Returns
// -------
// ret : MessagePack blob of function return values
//
{
    JSValue js_args = 0;
    JSValue js_msgpack_argv[1] = {0}; 
    JSValue js_arg_array = 0;
    JSValue js_func = 0;
    JSValue js_arg_array_len = 0;
    JSValue js_res = 0;
    JSValue js_packed_res = 0;
    JSValue *js_func_argv = NULL;
    JSValue js_out_len = 0;
    JSValue js_item = 0;
    uint64_t arg_array_len = 0;
    uint32_t item = 0;

    if (initialize()) goto error;

    js_args = JS_NewArrayBuffer(ctx, args->ptr, args->len, NULL, NULL, 0);
    if (JS_IsException(js_args)) goto error;

    js_msgpack_argv[0] = js_args;
    js_arg_array = JS_Call(ctx, unpack, global, 1, js_msgpack_argv);
    if (JS_IsException(js_arg_array)) goto error;

    js_func = load_function(name->ptr, name->len);
    if (JS_IsException(js_func)) goto error;

    js_arg_array_len = JS_GetPropertyStr(ctx, js_arg_array, "length");
    if (JS_IsException(js_arg_array_len)) goto error;

    JS_ToIndex(ctx, &arg_array_len, js_arg_array_len);

    js_func_argv = malloc(arg_array_len * sizeof(JSValue));
    if (!js_func_argv) goto error;

    for (uint64_t i = 0; i < arg_array_len; i++)
    {
        js_func_argv[i] = JS_GetPropertyUint32(ctx, js_arg_array, i);
    }

    js_res = JS_Call(ctx, js_func, global, arg_array_len, js_func_argv);
    if (JS_IsException(js_res)) goto error;

    js_msgpack_argv[0] = js_res;
    js_packed_res = JS_Call(ctx, pack, global, 1, js_msgpack_argv);
    if (JS_IsException(js_packed_res)) goto error;

    js_out_len = JS_GetPropertyStr(ctx, js_packed_res, "length");
    if (JS_IsException(js_out_len)) goto error;

    JS_ToIndex(ctx, &arg_array_len, js_out_len);

    ret->len = arg_array_len;
    ret->ptr = malloc(ret->len * sizeof(uint8_t));
    if (!ret->ptr) goto error;

    for (uint64_t i = 0; i < ret->len; i++)
    {
        js_item = JS_GetPropertyUint32(ctx, js_packed_res, i);
        if (JS_IsException(js_item)) { JS_FreeValue(ctx, js_item); goto error; }
        JS_ToUint32(ctx, &item, js_item);
        ret->ptr[i] = (uint8_t)item;
        JS_FreeValue(ctx, js_item);
    }

exit:
    JS_FreeValue(ctx, js_func);
    JS_FreeValue(ctx, js_args);
    JS_FreeValue(ctx, js_arg_array);
    JS_FreeValue(ctx, js_arg_array_len);
    JS_FreeValue(ctx, js_res);
    JS_FreeValue(ctx, js_packed_res);
    if (js_func_argv)
    {
        for (uint64_t i = 0; i < arg_array_len; i++)
        {
            JS_FreeValue(ctx, js_func_argv[i]);
        }
    }

    return;

error:
    js_std_dump_error(ctx);
    ret->ptr = NULL;
    ret->len = 0;
    goto exit;
}
