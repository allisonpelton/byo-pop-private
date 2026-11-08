# screencave.py: prints the patch lines of the screen's code (identical/setup.sh, "The screen"). The original draws into
# a DirectDraw back buffer, locked a dirty rectangle at a time; PrintWindow never sees it in Windows Sandbox and its
# pixels cannot be read between locks. So the lock's success path (0x456009) notes the rectangle, and the display's unlock
# (0x455e10) first copies that rectangle of the locked pixels into a 800x600 buffer of 32-bit pixels, allocated once with
# the game's own malloc (0x447342): the buffer always holds the last frame drawn, whole, for origrun's --surface-screen
# (its pointer at 0x4aa604, its pitch, 3200, at 0x4aa608). The code goes in the zero bytes past the end of .text, after
# the counter's (file 0x7f540 on; the image maps file offset + 0x400000).
import struct

BUF, PITCH, R = 0x4aa604, 0x4aa608, 0x4aa610  # R: left, top, right, bottom
MALLOC = 0x447342


class Asm:
    def __init__(self, va):
        self.va, self.code, self.labels, self.fix = va, bytearray(), {}, []

    def b(self, *xs):
        for x in xs:
            self.code += bytes.fromhex(x) if isinstance(x, str) else x
        return self

    def d(self, v):
        return struct.pack('<I', v & 0xffffffff)

    def here(self):
        return self.va + len(self.code)

    def label(self, name):
        self.labels[name] = self.here()

    def jcc8(self, op, name):  # a short jump to a label
        self.code += bytes([op, 0])
        self.fix.append(('8', len(self.code) - 1, name))

    def rel32(self, op, target):  # a call or jmp to an address
        self.code += bytes.fromhex(op)
        self.code += self.d(target - (self.here() + 4))

    def done(self):
        for kind, at, name in self.fix:
            rel = self.labels[name] - (self.va + at + 1)
            assert -128 <= rel < 128, name
            self.code[at] = rel & 0xff
        return bytes(self.code)


# the lock's success: the two instructions the jump replaces, the rectangle (ebx) noted, back to 0x45600f
lock = Asm(0x47f540)
lock.b('8b4e78', '894628', '50')
for i in range(4):
    lock.b('8b43%02x' % (4 * i) if i else '8b03', 'a3', lock.d(R + 4 * i))
lock.b('58')
lock.rel32('e9', 0x45600f)
lock = lock.done()

# the unlock: the three instructions the jump replaces; when the display is locked, the rectangle copied; on to 0x455e16
un = Asm(0x47f540 + len(lock) + 4)
un.b('56', '8bf1', '8b4e30', '85c9')
un.jcc8(0x74, 'back')
un.b('60')                                          # pushad
un.b('a1', un.d(BUF), '85c0')
un.jcc8(0x75, 'have')
un.b('68', un.d(800 * 600 * 4))
un.rel32('e8', MALLOC)
un.b('83c404', 'a3', un.d(BUF), 'c705', un.d(PITCH), un.d(3200))
un.label('have')
un.b('85c0')
un.jcc8(0x74, 'done')
un.b('8bf8')                                        # edi = buffer
un.b('8b0d', un.d(R + 4), '69c9', un.d(3200), '03f9')  # + top * 3200
un.b('8b0d', un.d(R), '8d3c8f')                     # + left * 4
un.b('8b462c', 'c1e002', '8be8')                    # ebp = the surface's pitch in bytes
un.b('8b7630')                                      # esi = the locked pixels (the rectangle's corner)
un.b('8b15', un.d(R + 12), '2b15', un.d(R + 4))     # edx = rows
un.jcc8(0x7e, 'done')
un.b('8b1d', un.d(R + 8), '2b1d', un.d(R))          # ebx = columns
un.jcc8(0x7e, 'done')
un.label('row')
un.b('56', '57', '8bcb', 'f3a5', '5f', '5e', '03f5', '81c7', un.d(3200), '4a')
un.jcc8(0x75, 'row')
un.label('done')
un.b('61')                                          # popad
un.label('back')
un.rel32('e9', 0x455e16)
un = un.done()

pad = b'\0' * 4
cave = lock + pad + un
unva = 0x47f540 + len(lock) + 4
print('test WordHarmonyGame.exe off:0x7f540 %s %s the screen\'s code: the lock\'s rectangle noted; the unlock copies it into '
      'the frame buffer [0x4aa604] (pitch 3200 at 0x4aa608)' % ('00' * len(cave), cave.hex()))
print('test WordHarmonyGame.exe va:0x456009 8b4e78894628 e9%s90 the lock\'s success (0x455f90) notes its rectangle'
      % struct.pack('<i', 0x47f540 - 0x45600e).hex())
print('test WordHarmonyGame.exe va:0x455e10 568bf18b4e30 e9%s90 the display\'s unlock (0x455e10) copies it first'
      % struct.pack('<i', unva - 0x455e15).hex())
