#!/bin/bash

# Location of wasi-sdk
export WASI_SDK_PATH=/opt/wasi-sdk

export LIBS="-Wl,--stack-first -Wl,-z,stack-size=83886080"
export CC="clang --target=wasm32-wasi"
export CFLAGS="-g -D_WASI_EMULATED_GETPID -D_WASI_EMULATED_SIGNAL -D_WASI_EMULATED_PROCESS_CLOCKS -isystem -I${WASI_SDK_PATH}/share/wasi-sysroot/include --sysroot=${WASI_SDK_PATH}/share/wasi-sysroot"
export CPPFLAGS="${CFLAGS}"
export LIBS="${LIBS} -L/opt/lib -lwasi_vfs -L${WASI_SDK_PATH}/share/wasi-sysroot/lib/wasm32-wasi -lwasi-emulated-signal --sysroot=${WASI_SDK_PATH}/share/wasi-sysroot -lcrypt -lm -lwasi-emulated-mman -lwasi-emulated-signal -lwasi-emulated-getpid -lwasi-emulated-process-clocks -lc -L/home/ksmith/src/python-wasi/wasix -lwasix"

$CC \
    -mexec-model=reactor \
    -I. \
    -I.. \
    $CFLAGS \
    $LIBS \
    udf.c udf_impl.c \
    ../libquickjs.a \
    -o udf-quickjs.wasm

wasi-vfs pack udf-quickjs.wasm --mapdir "/app::./app" --output s2-udf-quickjs.wasm
