"""Writes games/tiptop/engine/ResTable.inc from TipTop.exe's resource table (0x477060, 0x74 bytes an entry).

Usage: python games/tiptop/tools/restable.py <TipTop.exe> [out.inc]
"""
import struct
import sys

TABLE = 0x477060
IMAGE_BASE = 0x400000
ENTRY = 0x74

# The images of the original's registration and trial, which the port took out (GAME_NOTES.md, "Registration out"): the
# splash's register button and its unregistered pictures ("Trial Version" and "Unregistered"; the registered picture is the
# "Deluxe" of the logo and stays), the options screen's register button, the "no more plays" dialog, the quit and save
# questions with a register button, the registration dialog, the played-times font and the options screen's link to the
# publisher. No code of the port loads them; leaving them out of the table keeps the load bar from loading them.
OMIT = {
    'BUTTONSPLASHREGISTER.JPG', 'SPLASHUNREGISTERED.JPG', 'SPLASHUNREGISTERED2.JPG',
    'SPLASHTIMESPLAYEDFONT.PCX', 'BUTTONOPTIONSREGISTER.JPG', 'NOMOREEXECUTIONS.JPG', 'BUTTONNMEQUIT.JPG',
    'BUTTONNMEREGISTER.JPG', 'EXITGAMEMENU.JPG', 'BUTTONEXITGAMEYES.JPG', 'BUTTONEXITGAMENO.JPG',
    'BUTTONEXITGAMEREGISTER.JPG', 'EXITGAMESAVEMENU.JPG', 'BUTTONEXITSAVEREGISTER.JPG', 'REGISTERBACKGROUND.JPG',
    'BUTTONREGISTEROK.JPG', 'BUTTONREGISTERCANCEL.JPG', 'BUTTONREGISTERGETCODE.JPG', 'BUTTONREGISTERPASTE.JPG',
    'BUTTONOPTIONSVISITPOPCAP.JPG',
}


def main():
    exe = open(sys.argv[1], 'rb').read()
    out_path = sys.argv[2] if len(sys.argv) > 2 else 'games/tiptop/engine/ResTable.inc'
    base = TABLE - IMAGE_BASE  # .data's raw offset equals its RVA in this exe
    lines = ['// The resource table (TipTop.exe 0x477060, 0x74 bytes an entry): source flags, archive name, converter',
             '// type, group and id. Made by games/tiptop/tools/restable.py from the exe; the names are files in',
             '// TipTop.dat (some are missing there: the game never loads those).']
    i = 0
    omitted = 0
    while True:
        e = exe[base + i * ENTRY:base + (i + 1) * ENTRY]
        if e[0] & 0x3f == 0:
            break
        name = e[1:0x44].split(b'\0')[0].decode('latin-1')
        typ, grp, rid = struct.unpack_from('<3I', e, 0x4c)
        i += 1
        if name.upper().replace(chr(92), '/').split('/')[-1] in OMIT:
            omitted += 1
            continue
        lines.append('{0x%02x, "%s", %d, %d, %d},' % (e[0], name.replace('\\', '\\\\'), typ, grp, rid))
    open(out_path, 'w', newline='\n').write('\n'.join(lines) + '\n')
    print(out_path, i - omitted, 'entries,', omitted, 'left out')


if __name__ == '__main__':
    main()
