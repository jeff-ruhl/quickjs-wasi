#ifndef MSGPACK_JS_H
#define MSGPACK_JS_H

#define MSGPACK_TXT \
    "(function () {\n" \
    "	\"use strict\";\n" \
    "\n" \
    "	// Serializes a value to a MessagePack byte array.\n" \
    "	//\n" \
    "	// data: The value to serialize. This can be a scalar, array or object.\n" \
    "	// options: An object that defines additional options.\n" \
    "	// - multiple: (boolean) Indicates whether multiple values in data are concatenated to multiple MessagePack arrays. Default: false.\n" \
    "	// - invalidTypeReplacement:\n" \
    "	//   (any) The value that is used to replace values of unsupported types.\n" \
    "	//   (function) A function that returns such a value, given the original value as parameter.\n" \
    "	function serialize(data, options) {\n" \
    "		if (options && options.multiple && !Array.isArray(data)) {\n" \
    "			throw new Error(\"Invalid argument type: Expected an Array to serialize multiple values.\");\n" \
    "		}\n" \
    "		const pow32 = 0x100000000;   // 2^32\n" \
    "		let floatBuffer, floatView;\n" \
    "		let array = new Uint8Array(128);\n" \
    "		let length = 0;\n" \
    "		if (options && options.multiple) {\n" \
    "			for (let i = 0; i < data.length; i++) {\n" \
    "				append(data[i]);\n" \
    "			}\n" \
    "		}\n" \
    "		else {\n" \
    "			append(data);\n" \
    "		}\n" \
    "		return array.subarray(0, length);\n" \
    "\n" \
    "		function append(data, isReplacement) {\n" \
    "			switch (typeof data) {\n" \
    "				case \"undefined\":\n" \
    "					appendNull(data);\n" \
    "					break;\n" \
    "				case \"boolean\":\n" \
    "					appendBoolean(data);\n" \
    "					break;\n" \
    "				case \"number\":\n" \
    "					appendNumber(data);\n" \
    "					break;\n" \
    "				case \"string\":\n" \
    "					appendString(data);\n" \
    "					break;\n" \
    "				case \"object\":\n" \
    "					if (data === null)\n" \
    "						appendNull(data);\n" \
    "					else if (data instanceof Date)\n" \
    "						appendDate(data);\n" \
    "					else if (Array.isArray(data))\n" \
    "						appendArray(data);\n" \
    "					else if (data instanceof Uint8Array || data instanceof Uint8ClampedArray)\n" \
    "						appendBinArray(data);\n" \
    "					else if (data instanceof Int8Array || data instanceof Int16Array || data instanceof Uint16Array ||\n" \
    "						data instanceof Int32Array || data instanceof Uint32Array ||\n" \
    "						data instanceof Float32Array || data instanceof Float64Array)\n" \
    "						appendArray(data);\n" \
    "					else\n" \
    "						appendObject(data);\n" \
    "					break;\n" \
    "				default:\n" \
    "					if (!isReplacement && options && options.invalidTypeReplacement) {\n" \
    "						if (typeof options.invalidTypeReplacement === \"function\")\n" \
    "							append(options.invalidTypeReplacement(data), true);\n" \
    "						else\n" \
    "							append(options.invalidTypeReplacement, true);\n" \
    "					}\n" \
    "					else {\n" \
    "						throw new Error(\"Invalid argument type: The type '\" + (typeof data) + \"' cannot be serialized.\");\n" \
    "					}\n" \
    "			}\n" \
    "		}\n" \
    "\n" \
    "		function appendNull(data) {\n" \
    "			appendByte(0xc0);\n" \
    "		}\n" \
    "\n" \
    "		function appendBoolean(data) {\n" \
    "			appendByte(data ? 0xc3 : 0xc2);\n" \
    "		}\n" \
    "\n" \
    "		function appendNumber(data) {\n" \
    "			if (isFinite(data) && Math.floor(data) === data) {\n" \
    "				// Integer\n" \
    "				if (data >= 0 && data <= 0x7f) {\n" \
    "					appendByte(data);\n" \
    "				}\n" \
    "				else if (data < 0 && data >= -0x20) {\n" \
    "					appendByte(data);\n" \
    "				}\n" \
    "				else if (data > 0 && data <= 0xff) {   // uint8\n" \
    "					appendBytes([0xcc, data]);\n" \
    "				}\n" \
    "				else if (data >= -0x80 && data <= 0x7f) {   // int8\n" \
    "					appendBytes([0xd0, data]);\n" \
    "				}\n" \
    "				else if (data > 0 && data <= 0xffff) {   // uint16\n" \
    "					appendBytes([0xcd, data >>> 8, data]);\n" \
    "				}\n" \
    "				else if (data >= -0x8000 && data <= 0x7fff) {   // int16\n" \
    "					appendBytes([0xd1, data >>> 8, data]);\n" \
    "				}\n" \
    "				else if (data > 0 && data <= 0xffffffff) {   // uint32\n" \
    "					appendBytes([0xce, data >>> 24, data >>> 16, data >>> 8, data]);\n" \
    "				}\n" \
    "				else if (data >= -0x80000000 && data <= 0x7fffffff) {   // int32\n" \
    "					appendBytes([0xd2, data >>> 24, data >>> 16, data >>> 8, data]);\n" \
    "				}\n" \
    "				else if (data > 0 && data <= 0xffffffffffffffff) {   // uint64\n" \
    "					// Split 64 bit number into two 32 bit numbers because JavaScript only regards\n" \
    "					// 32 bits for bitwise operations.\n" \
    "					let hi = data / pow32;\n" \
    "					let lo = data % pow32;\n" \
    "					appendBytes([0xd3, hi >>> 24, hi >>> 16, hi >>> 8, hi, lo >>> 24, lo >>> 16, lo >>> 8, lo]);\n" \
    "				}\n" \
    "				else if (data >= -0x8000000000000000 && data <= 0x7fffffffffffffff) {   // int64\n" \
    "					appendByte(0xd3);\n" \
    "					appendInt64(data);\n" \
    "				}\n" \
    "				else if (data < 0) {   // below int64\n" \
    "					appendBytes([0xd3, 0x80, 0, 0, 0, 0, 0, 0, 0]);\n" \
    "				}\n" \
    "				else {   // above uint64\n" \
    "					appendBytes([0xcf, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff]);\n" \
    "				}\n" \
    "			}\n" \
    "			else {\n" \
    "				// Float\n" \
    "				if (!floatView) {\n" \
    "					floatBuffer = new ArrayBuffer(8);\n" \
    "					floatView = new DataView(floatBuffer);\n" \
    "				}\n" \
    "				floatView.setFloat64(0, data);\n" \
    "				appendByte(0xcb);\n" \
    "				appendBytes(new Uint8Array(floatBuffer));\n" \
    "			}\n" \
    "		}\n" \
    "\n" \
    "		function appendString(data) {\n" \
    "			let bytes = encodeUtf8(data);\n" \
    "			let length = bytes.length;\n" \
    "\n" \
    "			if (length <= 0x1f)\n" \
    "				appendByte(0xa0 + length);\n" \
    "			else if (length <= 0xff)\n" \
    "				appendBytes([0xd9, length]);\n" \
    "			else if (length <= 0xffff)\n" \
    "				appendBytes([0xda, length >>> 8, length]);\n" \
    "			else\n" \
    "				appendBytes([0xdb, length >>> 24, length >>> 16, length >>> 8, length]);\n" \
    "\n" \
    "			appendBytes(bytes);\n" \
    "		}\n" \
    "\n" \
    "		function appendArray(data) {\n" \
    "			let length = data.length;\n" \
    "\n" \
    "			if (length <= 0xf)\n" \
    "				appendByte(0x90 + length);\n" \
    "			else if (length <= 0xffff)\n" \
    "				appendBytes([0xdc, length >>> 8, length]);\n" \
    "			else\n" \
    "				appendBytes([0xdd, length >>> 24, length >>> 16, length >>> 8, length]);\n" \
    "\n" \
    "			for (let index = 0; index < length; index++) {\n" \
    "				append(data[index]);\n" \
    "			}\n" \
    "		}\n" \
    "\n" \
    "		function appendBinArray(data) {\n" \
    "			let length = data.length;\n" \
    "\n" \
    "			if (length <= 0xf)\n" \
    "				appendBytes([0xc4, length]);\n" \
    "			else if (length <= 0xffff)\n" \
    "				appendBytes([0xc5, length >>> 8, length]);\n" \
    "			else\n" \
    "				appendBytes([0xc6, length >>> 24, length >>> 16, length >>> 8, length]);\n" \
    "\n" \
    "			appendBytes(data);\n" \
    "		}\n" \
    "\n" \
    "		function appendObject(data) {\n" \
    "			let length = 0;\n" \
    "			for (let key in data) {\n" \
    "				if (data[key] !== undefined) {\n" \
    "					length++;\n" \
    "				}\n" \
    "			}\n" \
    "\n" \
    "			if (length <= 0xf)\n" \
    "				appendByte(0x80 + length);\n" \
    "			else if (length <= 0xffff)\n" \
    "				appendBytes([0xde, length >>> 8, length]);\n" \
    "			else\n" \
    "				appendBytes([0xdf, length >>> 24, length >>> 16, length >>> 8, length]);\n" \
    "\n" \
    "			for (let key in data) {\n" \
    "				let value = data[key];\n" \
    "				if (value !== undefined) {\n" \
    "					append(key);\n" \
    "					append(value);\n" \
    "				}\n" \
    "			}\n" \
    "		}\n" \
    "\n" \
    "		function appendDate(data) {\n" \
    "			let sec = data.getTime() / 1000;\n" \
    "			if (data.getMilliseconds() === 0 && sec >= 0 && sec < 0x100000000) {   // 32 bit seconds\n" \
    "				appendBytes([0xd6, 0xff, sec >>> 24, sec >>> 16, sec >>> 8, sec]);\n" \
    "			}\n" \
    "			else if (sec >= 0 && sec < 0x400000000) {   // 30 bit nanoseconds, 34 bit seconds\n" \
    "				let ns = data.getMilliseconds() * 1000000;\n" \
    "				appendBytes([0xd7, 0xff, ns >>> 22, ns >>> 14, ns >>> 6, ((ns << 2) >>> 0) | (sec / pow32), sec >>> 24, sec >>> 16, sec >>> 8, sec]);\n" \
    "			}\n" \
    "			else {   // 32 bit nanoseconds, 64 bit seconds, negative values allowed\n" \
    "				let ns = data.getMilliseconds() * 1000000;\n" \
    "				appendBytes([0xc7, 12, 0xff, ns >>> 24, ns >>> 16, ns >>> 8, ns]);\n" \
    "				appendInt64(sec);\n" \
    "			}\n" \
    "		}\n" \
    "\n" \
    "		function appendByte(byte) {\n" \
    "			if (array.length < length + 1) {\n" \
    "				let newLength = array.length * 2;\n" \
    "				while (newLength < length + 1)\n" \
    "					newLength *= 2;\n" \
    "				let newArray = new Uint8Array(newLength);\n" \
    "				newArray.set(array);\n" \
    "				array = newArray;\n" \
    "			}\n" \
    "			array[length] = byte;\n" \
    "			length++;\n" \
    "		}\n" \
    "\n" \
    "		function appendBytes(bytes) {\n" \
    "			if (array.length < length + bytes.length) {\n" \
    "				let newLength = array.length * 2;\n" \
    "				while (newLength < length + bytes.length)\n" \
    "					newLength *= 2;\n" \
    "				let newArray = new Uint8Array(newLength);\n" \
    "				newArray.set(array);\n" \
    "				array = newArray;\n" \
    "			}\n" \
    "			array.set(bytes, length);\n" \
    "			length += bytes.length;\n" \
    "		}\n" \
    "\n" \
    "		function appendInt64(value) {\n" \
    "			// Split 64 bit number into two 32 bit numbers because JavaScript only regards 32 bits for\n" \
    "			// bitwise operations.\n" \
    "			let hi, lo;\n" \
    "			if (value >= 0) {\n" \
    "				// Same as uint64\n" \
    "				hi = value / pow32;\n" \
    "				lo = value % pow32;\n" \
    "			}\n" \
    "			else {\n" \
    "				// Split absolute value to high and low, then NOT and ADD(1) to restore negativity\n" \
    "				value++;\n" \
    "				hi = Math.abs(value) / pow32;\n" \
    "				lo = Math.abs(value) % pow32;\n" \
    "				hi = ~hi;\n" \
    "				lo = ~lo;\n" \
    "			}\n" \
    "			appendBytes([hi >>> 24, hi >>> 16, hi >>> 8, hi, lo >>> 24, lo >>> 16, lo >>> 8, lo]);\n" \
    "		}\n" \
    "	}\n" \
    "\n" \
    "	// Deserializes a MessagePack byte array to a value.\n" \
    "	//\n" \
    "	// array: The MessagePack byte array to deserialize. This must be an Array or Uint8Array containing bytes, not a string.\n" \
    "	// options: An object that defines additional options.\n" \
    "	// - multiple: (boolean) Indicates whether multiple concatenated MessagePack arrays are returned as an array. Default: false.\n" \
    "	function deserialize(array, options) {\n" \
    "		const pow32 = 0x100000000;   // 2^32\n" \
    "		let pos = 0;\n" \
    "		if (array instanceof ArrayBuffer) {\n" \
    "			array = new Uint8Array(array);\n" \
    "		}\n" \
    "		if (typeof array !== \"object\" || typeof array.length === \"undefined\") {\n" \
    "			throw new Error(\"Invalid argument type: Expected a byte array (Array or Uint8Array) to deserialize.\");\n" \
    "		}\n" \
    "		if (!array.length) {\n" \
    "			throw new Error(\"Invalid argument: The byte array to deserialize is empty.\");\n" \
    "		}\n" \
    "		if (!(array instanceof Uint8Array)) {\n" \
    "			array = new Uint8Array(array);\n" \
    "		}\n" \
    "		let data;\n" \
    "		if (options && options.multiple) {\n" \
    "			// Read as many messages as are available\n" \
    "			data = [];\n" \
    "			while (pos < array.length) {\n" \
    "				data.push(read());\n" \
    "			}\n" \
    "		}\n" \
    "		else {\n" \
    "			// Read only one message and ignore additional data\n" \
    "			data = read();\n" \
    "		}\n" \
    "		return data;\n" \
    "\n" \
    "		function read() {\n" \
    "			const byte = array[pos++];\n" \
    "			if (byte >= 0x00 && byte <= 0x7f) return byte;   // positive fixint\n" \
    "			if (byte >= 0x80 && byte <= 0x8f) return readMap(byte - 0x80);   // fixmap\n" \
    "			if (byte >= 0x90 && byte <= 0x9f) return readArray(byte - 0x90);   // fixarray\n" \
    "			if (byte >= 0xa0 && byte <= 0xbf) return readStr(byte - 0xa0);   // fixstr\n" \
    "			if (byte === 0xc0) return null;   // nil\n" \
    "			if (byte === 0xc1) throw new Error(\"Invalid byte code 0xc1 found.\");   // never used\n" \
    "			if (byte === 0xc2) return false;   // false\n" \
    "			if (byte === 0xc3) return true;   // true\n" \
    "			if (byte === 0xc4) return readBin(-1, 1);   // bin 8\n" \
    "			if (byte === 0xc5) return readBin(-1, 2);   // bin 16\n" \
    "			if (byte === 0xc6) return readBin(-1, 4);   // bin 32\n" \
    "			if (byte === 0xc7) return readExt(-1, 1);   // ext 8\n" \
    "			if (byte === 0xc8) return readExt(-1, 2);   // ext 16\n" \
    "			if (byte === 0xc9) return readExt(-1, 4);   // ext 32\n" \
    "			if (byte === 0xca) return readFloat(4);   // float 32\n" \
    "			if (byte === 0xcb) return readFloat(8);   // float 64\n" \
    "			if (byte === 0xcc) return readUInt(1);   // uint 8\n" \
    "			if (byte === 0xcd) return readUInt(2);   // uint 16\n" \
    "			if (byte === 0xce) return readUInt(4);   // uint 32\n" \
    "			if (byte === 0xcf) return readUInt(8);   // uint 64\n" \
    "			if (byte === 0xd0) return readInt(1);   // int 8\n" \
    "			if (byte === 0xd1) return readInt(2);   // int 16\n" \
    "			if (byte === 0xd2) return readInt(4);   // int 32\n" \
    "			if (byte === 0xd3) return readInt(8);   // int 64\n" \
    "			if (byte === 0xd4) return readExt(1);   // fixext 1\n" \
    "			if (byte === 0xd5) return readExt(2);   // fixext 2\n" \
    "			if (byte === 0xd6) return readExt(4);   // fixext 4\n" \
    "			if (byte === 0xd7) return readExt(8);   // fixext 8\n" \
    "			if (byte === 0xd8) return readExt(16);   // fixext 16\n" \
    "			if (byte === 0xd9) return readStr(-1, 1);   // str 8\n" \
    "			if (byte === 0xda) return readStr(-1, 2);   // str 16\n" \
    "			if (byte === 0xdb) return readStr(-1, 4);   // str 32\n" \
    "			if (byte === 0xdc) return readArray(-1, 2);   // array 16\n" \
    "			if (byte === 0xdd) return readArray(-1, 4);   // array 32\n" \
    "			if (byte === 0xde) return readMap(-1, 2);   // map 16\n" \
    "			if (byte === 0xdf) return readMap(-1, 4);   // map 32\n" \
    "			if (byte >= 0xe0 && byte <= 0xff) return byte - 256;   // negative fixint\n" \
    "			console.debug(\"msgpack array:\", array);\n" \
    "			throw new Error(\"Invalid byte value '\" + byte + \"' at index \" + (pos - 1) + \" in the MessagePack binary data (length \" + array.length + \"): Expecting a range of 0 to 255. This is not a byte array.\");\n" \
    "		}\n" \
    "\n" \
    "		function readInt(size) {\n" \
    "			let value = 0;\n" \
    "			let first = true;\n" \
    "			while (size-- > 0) {\n" \
    "				if (first) {\n" \
    "					let byte = array[pos++];\n" \
    "					value += byte & 0x7f;\n" \
    "					if (byte & 0x80) {\n" \
    "						value -= 0x80;   // Treat most-significant bit as -2^i instead of 2^i\n" \
    "					}\n" \
    "					first = false;\n" \
    "				}\n" \
    "				else {\n" \
    "					value *= 256;\n" \
    "					value += array[pos++];\n" \
    "				}\n" \
    "			}\n" \
    "			return value;\n" \
    "		}\n" \
    "\n" \
    "		function readUInt(size) {\n" \
    "			let value = 0;\n" \
    "			while (size-- > 0) {\n" \
    "				value *= 256;\n" \
    "				value += array[pos++];\n" \
    "			}\n" \
    "			return value;\n" \
    "		}\n" \
    "\n" \
    "		function readFloat(size) {\n" \
    "			let view = new DataView(array.buffer, pos + array.byteOffset, size);\n" \
    "			pos += size;\n" \
    "			if (size === 4)\n" \
    "				return view.getFloat32(0, false);\n" \
    "			if (size === 8)\n" \
    "				return view.getFloat64(0, false);\n" \
    "		}\n" \
    "\n" \
    "		function readBin(size, lengthSize) {\n" \
    "			if (size < 0) size = readUInt(lengthSize);\n" \
    "			let data = array.subarray(pos, pos + size);\n" \
    "			pos += size;\n" \
    "			return data;\n" \
    "		}\n" \
    "\n" \
    "		function readMap(size, lengthSize) {\n" \
    "			if (size < 0) size = readUInt(lengthSize);\n" \
    "			let data = {};\n" \
    "			while (size-- > 0) {\n" \
    "				let key = read();\n" \
    "				data[key] = read();\n" \
    "			}\n" \
    "			return data;\n" \
    "		}\n" \
    "\n" \
    "		function readArray(size, lengthSize) {\n" \
    "			if (size < 0) size = readUInt(lengthSize);\n" \
    "			let data = [];\n" \
    "			while (size-- > 0) {\n" \
    "				data.push(read());\n" \
    "			}\n" \
    "			return data;\n" \
    "		}\n" \
    "\n" \
    "		function readStr(size, lengthSize) {\n" \
    "			if (size < 0) size = readUInt(lengthSize);\n" \
    "			let start = pos;\n" \
    "			pos += size;\n" \
    "			return decodeUtf8(array, start, size);\n" \
    "		}\n" \
    "\n" \
    "		function readExt(size, lengthSize) {\n" \
    "			if (size < 0) size = readUInt(lengthSize);\n" \
    "			let type = readUInt(1);\n" \
    "			let data = readBin(size);\n" \
    "			switch (type) {\n" \
    "				case 255:\n" \
    "					return readExtDate(data);\n" \
    "			}\n" \
    "			return { type: type, data: data };\n" \
    "		}\n" \
    "\n" \
    "		function readExtDate(data) {\n" \
    "			if (data.length === 4) {\n" \
    "				let sec = ((data[0] << 24) >>> 0) +\n" \
    "					((data[1] << 16) >>> 0) +\n" \
    "					((data[2] << 8) >>> 0) +\n" \
    "					data[3];\n" \
    "				return new Date(sec * 1000);\n" \
    "			}\n" \
    "			if (data.length === 8) {\n" \
    "				let ns = ((data[0] << 22) >>> 0) +\n" \
    "					((data[1] << 14) >>> 0) +\n" \
    "					((data[2] << 6) >>> 0) +\n" \
    "					(data[3] >>> 2);\n" \
    "				let sec = ((data[3] & 0x3) * pow32) +\n" \
    "					((data[4] << 24) >>> 0) +\n" \
    "					((data[5] << 16) >>> 0) +\n" \
    "					((data[6] << 8) >>> 0) +\n" \
    "					data[7];\n" \
    "				return new Date(sec * 1000 + ns / 1000000);\n" \
    "			}\n" \
    "			if (data.length === 12) {\n" \
    "				let ns = ((data[0] << 24) >>> 0) +\n" \
    "					((data[1] << 16) >>> 0) +\n" \
    "					((data[2] << 8) >>> 0) +\n" \
    "					data[3];\n" \
    "				pos -= 8;\n" \
    "				let sec = readInt(8);\n" \
    "				return new Date(sec * 1000 + ns / 1000000);\n" \
    "			}\n" \
    "			throw new Error(\"Invalid data length for a date value.\");\n" \
    "		}\n" \
    "	}\n" \
    "\n" \
    "	// Encodes a string to UTF-8 bytes.\n" \
    "	function encodeUtf8(str) {\n" \
    "		// Prevent excessive array allocation and slicing for all 7-bit characters\n" \
    "		let ascii = true, length = str.length;\n" \
    "		for (let x = 0; x < length; x++) {\n" \
    "			if (str.charCodeAt(x) > 127) {\n" \
    "				ascii = false;\n" \
    "				break;\n" \
    "			}\n" \
    "		}\n" \
    "\n" \
    "		// Based on: https://gist.github.com/pascaldekloe/62546103a1576803dade9269ccf76330\n" \
    "		let i = 0, bytes = new Uint8Array(str.length * (ascii ? 1 : 4));\n" \
    "		for (let ci = 0; ci !== length; ci++) {\n" \
    "			let c = str.charCodeAt(ci);\n" \
    "			if (c < 128) {\n" \
    "				bytes[i++] = c;\n" \
    "				continue;\n" \
    "			}\n" \
    "			if (c < 2048) {\n" \
    "				bytes[i++] = c >> 6 | 192;\n" \
    "			}\n" \
    "			else {\n" \
    "				if (c > 0xd7ff && c < 0xdc00) {\n" \
    "					if (++ci >= length)\n" \
    "						throw new Error(\"UTF-8 encode: incomplete surrogate pair\");\n" \
    "					let c2 = str.charCodeAt(ci);\n" \
    "					if (c2 < 0xdc00 || c2 > 0xdfff)\n" \
    "						throw new Error(\"UTF-8 encode: second surrogate character 0x\" + c2.toString(16) + \" at index \" + ci + \" out of range\");\n" \
    "					c = 0x10000 + ((c & 0x03ff) << 10) + (c2 & 0x03ff);\n" \
    "					bytes[i++] = c >> 18 | 240;\n" \
    "					bytes[i++] = c >> 12 & 63 | 128;\n" \
    "				}\n" \
    "				else bytes[i++] = c >> 12 | 224;\n" \
    "				bytes[i++] = c >> 6 & 63 | 128;\n" \
    "			}\n" \
    "			bytes[i++] = c & 63 | 128;\n" \
    "		}\n" \
    "		return ascii ? bytes : bytes.subarray(0, i);\n" \
    "	}\n" \
    "\n" \
    "	// Decodes a string from UTF-8 bytes.\n" \
    "	function decodeUtf8(bytes, start, length) {\n" \
    "		// Based on: https://gist.github.com/pascaldekloe/62546103a1576803dade9269ccf76330\n" \
    "		let i = start, str = \"\";\n" \
    "		length += start;\n" \
    "		while (i < length) {\n" \
    "			let c = bytes[i++];\n" \
    "			if (c > 127) {\n" \
    "				if (c > 191 && c < 224) {\n" \
    "					if (i >= length)\n" \
    "						throw new Error(\"UTF-8 decode: incomplete 2-byte sequence\");\n" \
    "					c = (c & 31) << 6 | bytes[i++] & 63;\n" \
    "				}\n" \
    "				else if (c > 223 && c < 240) {\n" \
    "					if (i + 1 >= length)\n" \
    "						throw new Error(\"UTF-8 decode: incomplete 3-byte sequence\");\n" \
    "					c = (c & 15) << 12 | (bytes[i++] & 63) << 6 | bytes[i++] & 63;\n" \
    "				}\n" \
    "				else if (c > 239 && c < 248) {\n" \
    "					if (i + 2 >= length)\n" \
    "						throw new Error(\"UTF-8 decode: incomplete 4-byte sequence\");\n" \
    "					c = (c & 7) << 18 | (bytes[i++] & 63) << 12 | (bytes[i++] & 63) << 6 | bytes[i++] & 63;\n" \
    "				}\n" \
    "				else throw new Error(\"UTF-8 decode: unknown multibyte start 0x\" + c.toString(16) + \" at index \" + (i - 1));\n" \
    "			}\n" \
    "			if (c <= 0xffff) str += String.fromCharCode(c);\n" \
    "			else if (c <= 0x10ffff) {\n" \
    "				c -= 0x10000;\n" \
    "				str += String.fromCharCode(c >> 10 | 0xd800)\n" \
    "				str += String.fromCharCode(c & 0x3FF | 0xdc00)\n" \
    "			}\n" \
    "			else throw new Error(\"UTF-8 decode: code point 0x\" + c.toString(16) + \" exceeds UTF-16 reach\");\n" \
    "		}\n" \
    "		return str;\n" \
    "	}\n" \
    "\n" \
    "	// The exported functions\n" \
    "	let msgpack = {\n" \
    "		serialize: serialize,\n" \
    "		deserialize: deserialize,\n" \
    "\n" \
    "		// Compatibility with other libraries\n" \
    "		encode: serialize,\n" \
    "		decode: deserialize\n" \
    "	};\n" \
    "\n" \
    "	// Environment detection\n" \
    "	if (typeof module === \"object\" && module && typeof module.exports === \"object\") {\n" \
    "		// Node.js\n" \
    "		module.exports = msgpack;\n" \
    "	}\n" \
    "	else {\n" \
    "		// Global object\n" \
    "		globalThis[globalThis.msgpackJsName || \"msgpack\"] = msgpack;\n" \
    "	}\n" \
    "\n" \
    "})();\n" \
    ""

#endif
