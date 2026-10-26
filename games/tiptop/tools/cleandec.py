"""Makes Ghidra's C of TipTop.exe easier to read: frame-pointer locals of the functions with an exception-handling
prolog (__EH_prolog, 0x44d1b0) become named variables, and *(T *)(x + 0xNN) becomes x->_NN.

    python games/tiptop/tools/cleandec.py <in.txt> > <out.txt>

  *(T *)(unaff_EBP + -0x4c)  ->  l_4c        (a local)
  *(T *)(unaff_EBP + 0x8)    ->  p1          (the first stack parameter; 0xc p2, ...)
  (unaff_EBP + -0x4c)        ->  &l_4c
  l_4c = extraout_ECX;       ->  this_ = ECX (the local that keeps the this pointer is renamed this_)
  *(T *)(x + 0x34c)          ->  x->_34c     (x a frame local or a chain of their fields)
  *(T *)((int)x + 0x10)      ->  x->_10
  (T *)(x + 0x34c)           ->  &x->_34c
"""
import re
import sys

# Only names whose value is a byte address: the frame locals (read as int by Ghidra) and chains of their fields.
# A typed pointer (param_1 + 1 on a uint *) counts in elements, so other names are left alone.
NAME = r'(?:(?:this_|l_[0-9a-f]+|p\d+)(?:->_[0-9a-f]+)*)'
TYPE = r'(?:[\w ]+?\s*\**)'


def suffix(t):
    """The field's width from Ghidra's cast: b byte, w 16 bits, f float, d double, p pointer, none 32 bits."""
    t = t.strip()
    if t.endswith('**'):
        return 'p'
    t = t.rstrip(' *')
    if t in ('undefined1', 'char', 'byte', 'bool', 'uchar'):
        return 'b'
    if t in ('undefined2', 'short', 'ushort', 'wchar_t'):
        return 'w'
    if t == 'float':
        return 'f'
    if t in ('double', 'float10', 'longlong', 'ulonglong', 'undefined8'):
        return 'd'
    return ''


def param_name(off):
    return 'p%d' % ((off - 8) // 4 + 1)


def clean_function(text):
    def frame(m):
        off = int(m.group(2), 0)
        return ('l_%x' % off) if m.group(1) else param_name(off)
    text = re.sub(r'\*\(' + TYPE + r' \*\)\(unaff_EBP \+ (-?)(0x[0-9a-f]+|\d+)\)', frame, text)
    text = re.sub(r'\(unaff_EBP \+ (-?)(0x[0-9a-f]+|\d+)\)', lambda m: '&' + frame(m), text)
    m = re.search(r'\b(l_[0-9a-f]+) = extraout_ECX;', text)
    if m:
        text = re.sub(r'\b' + m.group(1) + r'\b', 'this_', text)
    # Ghidra's own byte offsets: *(T *)((int)x + 0x10)
    text = re.sub(r'\*\((' + TYPE + r') \*\)\(\(int\)(\w+) \+ (0x[0-9a-f]+|\d+)\)',
                  lambda m: '%s->_%x%s' % (m.group(2), int(m.group(3), 0), suffix(m.group(1))), text)
    for _ in range(8):
        new = re.sub(r'\*\((' + TYPE + r') \*\)\((' + NAME + r') \+ (0x[0-9a-f]+|\d+)\)',
                     lambda m: '%s->_%x%s' % (m.group(2), int(m.group(3), 0), suffix(m.group(1))), text)
        new = re.sub(r'\*\((' + TYPE + r') \*\)(' + NAME + r')\b(?!->|\[)',
                     lambda m: '%s->_0%s' % (m.group(2), suffix(m.group(1))), new)
        if new == text:
            break
        text = new
    text = re.sub(r'\(' + TYPE + r' \*\)\((' + NAME + r') \+ (0x[0-9a-f]+|\d+)\)',
                  lambda m: '&%s->_%x' % (m.group(1), int(m.group(2), 0)), text)
    return text


def main():
    src = open(sys.argv[1], encoding='utf-8', errors='replace').read()
    parts = re.split(r'(?m)^(?=// [0-9a-f]{8}  )', src)
    sys.stdout.write(''.join(clean_function(p) for p in parts))


if __name__ == '__main__':
    main()
