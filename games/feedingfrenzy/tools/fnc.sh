#!/bin/bash
# usage: games/feedingfrenzy/tools/fnc.sh <hex address>... -- a function's decompile without the declarations and noise
cd "$(dirname "$0")/../../.."
for a in "$@"; do
python -I games/feedingfrenzy/tools/fn.py $a | grep -v '^\s*$' | grep -vE 'ExceptionList|local_4 = |uStack_4 = |iStack_4 = |_free\(|^\s*(undefined[0-9]*|int|uint|float|float10|char|bool|ushort|byte|void|longlong|ulonglong|code|HWND|LPPOINT|tagPOINT)\s+[\*a-zA-Z_0-9\[\] ]*;\s*$|^\s*(undefined[0-9]*|int|uint|float|char|void) \*+[a-zA-Z_0-9]*;$' | cut -c1-210
done
