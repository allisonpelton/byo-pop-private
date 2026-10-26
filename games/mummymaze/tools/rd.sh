#!/bin/bash
# Read decompiled functions of the Ghidra export in a compact form (decread.py + skim.py), e.g.
#   games/mummymaze/tools/rd.sh 4067f0 406f10
# Run from the repository root.
python games/mummymaze/tools/decread.py "$@" | python games/mummymaze/tools/skim.py | grep -v -E "memcpy|^\s*(local|iStack|uStack|pIStack|pcStack)_[0-9a-f]+ = 0x0;$" | sed -E 's/\(char \*\)|\(undefined4 \*\)|\(undefined1 \*\)|\(int \*\)|\(int\)//g' | cut -c1-170
