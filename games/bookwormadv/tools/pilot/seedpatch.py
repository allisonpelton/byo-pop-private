"""seedpatch.py <staged exe> [seed] [--holdhints]: fix the original's random seeds in a staged copy (never the install).

SexyAppBase::Init (0x54bff0) seeds the app's Mersenne Twister with the tick count (mRandSeed = GetTickCount();
SRand(mRandSeed), at 0x54c430) and then the C library's rand() with another tick count (srand(GetTickCount()), at
0x54c4bc). The patch replaces both clock reads with constants: SRand gets <seed> (default 12345), as the port's
-seed=<seed> does, and srand gets 1, where the port's math.random generator (poplua/lmathlib.cpp) always starts.

It also lets the copy see the mouse (the coordinator's go-ahead, the user's "Yes, patch copies", 2026-11-01). On mouse
messages SexyAppBase's handler (0x547ea0) asks whether the real cursor is over its window (ClientToScreen, GetCursorPos
and WindowFromPoint, 0x548635 to 0x5486bc) and, if not, sends the widgets a mouse exit. On rundesk's desktop there is no
cursor (GetCursorPos fails), so the original never showed a hover. The patch makes that test always answer "inside":
origrun's mouse never leaves the window, and the cursor's own position is read only on the "outside" path. A test-harness
change to the original, not to the port.
The third is the trial's end of the adventure (0x451c80), which past the first book's seventh chapter ends a trial; it
answers "not over", as the registered game's does (tools/origrun/patches/bookwormadv.txt has the same patch).
With --holdhints, also the ChapterLoadScreen patch (0x42efa0, as in tools/origrun/patches/bookwormadv.txt): a chapter
load's screen keeps its first hint and draws no new one, the port's -holdloadhints. Only the screens pilot asks for it
(tools/pilot/run.sh, vis3d.sh); the logic comparisons keep the hint's draws and the original's own load lengths.
The expected bytes are checked first; the patch is logged to stdout.
"""
import struct
import sys

def file_offset(data, va):
    pe = struct.unpack_from("<I", data, 0x3c)[0]
    nsec = struct.unpack_from("<H", data, pe + 6)[0]
    opt = struct.unpack_from("<H", data, pe + 20)[0]
    base = struct.unpack_from("<I", data, pe + 24 + 28)[0]
    sec = pe + 24 + opt
    for i in range(nsec):
        vsize, vaddr, rsize, raw = struct.unpack_from("<IIII", data, sec + i * 40 + 8)
        if base + vaddr <= va < base + vaddr + max(vsize, rsize):
            return va - base - vaddr + raw
    sys.exit("0x%x is in no section" % va)


def main():
    args = [a for a in sys.argv[1:] if a != "--holdhints"]
    path = args[0]
    seed = int(args[1]) if len(args) > 1 else 12345
    data = bytearray(open(path, "rb").read())
    patches = [
        # mov edi, [GetTickCount]; call edi  ->  mov eax, <seed>; nop x3 (then mRandSeed = eax; SRand)
        (0x54c430, bytes.fromhex("8b3dc8707900ffd7"), b"\xb8" + struct.pack("<I", seed) + b"\x90\x90\x90"),
        # call edi; push eax  ->  push 1; nop (srand's argument)
        (0x54c4bc, bytes.fromhex("ffd750"), bytes.fromhex("6a0190")),
        # mov byte [esp+0xf], 0 (the cursor is outside the window)  ->  mov byte [esp+0xf], 1
        (0x5486b7, bytes.fromhex("c644240f00"), bytes.fromhex("c644240f01")),
        # push ebp; mov ebp, esp  ->  xor eax, eax; ret: the trial's end of the adventure (0x451c80, book 1 past
        # chapter 7) answers "not over", as the registered game the port plays does
        (0x451c80, bytes.fromhex("558bec"), bytes.fromhex("31c0c3")),
    ]
    if "--holdhints" in sys.argv[1:]:
        # jg (the timer not run out: return)  ->  nop; jmp: ChapterLoadScreen::Update (0x42ef80) always returns, so a
        # chapter load's screen keeps its first hint and draws no Rand() while it is up, as the port's -holdloadhints
        # (the screens pilot only: its boards then do not follow how long a load took on a busy machine)
        patches.append((0x42efa0, bytes.fromhex("0f8f6e010000"), bytes.fromhex("90e96e010000")))
    for va, old, new in patches:
        off = file_offset(data, va)
        cur = bytes(data[off:off + len(old)])
        if cur == new:
            print("seedpatch: 0x%x already patched" % va)
            continue
        if cur != old:
            sys.exit("seedpatch: 0x%x holds %s, expected %s: not patched" % (va, cur.hex(), old.hex()))
        data[off:off + len(old)] = new
        print("seedpatch: 0x%x (file 0x%x) %s -> %s" % (va, off, old.hex(), new.hex()))
    open(path, "wb").write(data)


main()
