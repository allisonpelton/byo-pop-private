"""Writes the port's registry.ini for a comparison run (port.sh): the settings the original starts from.

    python games/bigmoney/tools/origcmp/regini.py --hklm             the original's own HKLM key (the host runs, orig.sh)
    python games/bigmoney/tools/origcmp/regini.py --settings <file>  settings.txt (the sandbox runs, ORIG_SANDBOX=1)

On the host the original reads HKLM\\SOFTWARE\\PopCap\\BigMoney (the 32-bit view) without registry virtualization and
cannot write it, so every host run starts from it; in the sandbox orig.sh imports settings.txt.
"""
import subprocess
import sys


def main():
    print("[SOFTWARE\\PopCap\\BigMoney]")
    if sys.argv[1] == "--settings":
        for ln in open(sys.argv[2]):
            p = ln.split(None, 2)
            if len(p) == 3 and not p[0].startswith("#"):
                print("%s=%s:%s" % (p[0], p[1], p[2].strip()))
        return
    out = subprocess.run(["reg", "query", "HKLM\\SOFTWARE\\WOW6432Node\\PopCap\\BigMoney"], capture_output=True,
                         text=True, check=True).stdout
    for ln in out.splitlines():
        p = ln.strip().split(None, 2)
        if len(p) == 3 and p[1] == "REG_DWORD":
            print("%s=dword:%d" % (p[0], int(p[2], 16)))
        elif len(p) == 3 and p[1] == "REG_SZ":
            print("%s=sz:%s" % (p[0], p[2]))


if __name__ == "__main__":
    main()
