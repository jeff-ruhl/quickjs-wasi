#!/usr/bin/env python3

import msgpack
import wasmtime
from bindings import Udf

store = wasmtime.Store()

module = wasmtime.Module(store.engine, open('s2-udf-quickjs.wasm', 'rb').read())

linker = wasmtime.Linker(store.engine)
linker.define_wasi()

wasi = wasmtime.WasiConfig()
wasi.inherit_stdin()
wasi.inherit_stdout()
wasi.inherit_stderr()
store.set_wasi(wasi)

udf = Udf(store, linker, module)

# You *must* call _initialize for wasi to work
udf.instance.exports(store)['_initialize'](store)

out = udf.exec(store, 'globalThis.foo = function(x) { print("Calling foo with " + x + "."); return x; };')
print('Result of exec:', out)

out = udf.call(store, 'foo', msgpack.packb(["hello"]))
print('Result of foo("hello"):', out)
print('Unpacked result of foo("hello"):', msgpack.unpackb(out))

out = udf.call(store, 'main.sqrt', msgpack.packb([2]))
print('Result of main.sqrt(2):', out)
print('Unpacked result of main.sqrt(2):', msgpack.unpackb(out))

