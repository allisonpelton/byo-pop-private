"""inireg.py <registry.ini> <out.reg>: the port's emulated registry (a registry.ini of the framework) as a .reg file of the
same values under HKEY_CURRENT_USER, for origrun's --reg presets in Windows Sandbox (docs/ORIGRUN.md)."""
import sys


def main():
    src, dst = sys.argv[1:3]
    out = ["Windows Registry Editor Version 5.00", ""]
    for line in open(src, encoding="utf-8"):
        line = line.strip()
        if not line or line.startswith(";"):
            continue
        if line.startswith("["):
            out += ["", "[HKEY_CURRENT_USER\\" + line[1:-1] + "]"]
            continue
        name, _, value = line.partition("=")
        kind, _, data = value.partition(":")
        if kind == "dword":
            out.append('"%s"=dword:%08x' % (name, int(data) & 0xffffffff))
        elif kind == "sz":
            out.append('"%s"="%s"' % (name, data.replace("\\", "\\\\").replace('"', '\\"')))
        else:
            sys.exit("inireg: %s has a value of kind %s" % (name, kind))
    open(dst, "w", encoding="utf-16", newline="\r\n").write("\r\n".join(out) + "\r\n")


main()
