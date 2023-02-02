#!/usr/bin/env python3

import re
from urllib.request import urlopen


MSGPACK_URL = 'https://raw.githubusercontent.com/ygoe/msgpack.js/master/msgpack.js'

with open('msgpack.h', 'w') as outfile:
    print('#ifndef MSGPACK_JS_H', file=outfile)
    print('#define MSGPACK_JS_H', file=outfile)
    print('', file=outfile)
    print('#define MSGPACK_TXT \\', file=outfile)
    for line in urlopen(MSGPACK_URL):
        line = line.decode('utf-8').replace(u'\ufeff', '').rstrip().replace('"', '\\"')
        line = re.sub(r'\bwindow\b', r'globalThis', line)
        print('    "%s\\n" \\' % line, file=outfile)
    print('    ""', file=outfile)
    print('', file=outfile)
    print('#endif', file=outfile)
