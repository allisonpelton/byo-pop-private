#!/usr/bin/env python3
"""The prefixes of docs/CODE_STANDARDS.md, rule 4, applied with clang-tidy's help.

    python games/cosmicbugs/tools/readability/tidyrename.py run     list what clang-tidy flags (one run for each unit)
    python games/cosmicbugs/tools/readability/tidyrename.py apply   rename every flagged declaration and each use of it

`run` starts clang-tidy's readability-identifier-naming (the framework's prefixes as its options) once for each unit of
the native build that holds the game's code and keeps each run's fixes in build/tidy/. clang-tidy names things by its own
case rules, which turns `mode` into `mOde`, so `apply` ignores the name it suggests and keeps only where each
declaration and use is: it reads the old name at that place and writes the name this tool builds from it:

- a member (a field of a struct or class) is `m` and the old name with a capital: `x` is `mX`;
- a parameter is `the` and the old name: `aWeapon` is `theWeapon`, `px` is `thePx`;
- a local is `a` (`an` before a vowel) and the old name; a global is `g`, a static `s`; a constant `kScreenClip` is
  `SCREEN_CLIP`;
- every one-letter parameter and local stays as it is (rule 5 judges those by how far they reach: namecheck.py), and
  so do the names in the files listed in SKIP_FILES.

A place is replaced only where the text there is the old name, and the same place from two units is replaced once. What
the units do not compile (a web-only block, an `offsetof(Type, member)` argument, a member reached through a template)
is not reached: the compiler finds those after the apply, and the web build finds the rest.
"""
import glob, json, os, re, subprocess, sys
import yaml

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..", "..", ".."))
LLVM = os.environ.get("LLVM_MINGW_ROOT", "E:/retools/llvm-mingw-20261006-ucrt-x86_64")
GAME = "cosmicbugs"
OUT = os.path.join(ROOT, "build", "tidy")
SKIP_FILES = {"main.cpp"}   # every game shares this file in its one form

OPTIONS = {
    "MemberCase": "CamelCase", "MemberPrefix": "m",
    "ParameterCase": "CamelCase", "ParameterPrefix": "the",
    "LocalVariableCase": "CamelCase", "LocalVariablePrefix": "a",
    "GlobalVariableCase": "CamelCase", "GlobalVariablePrefix": "g",
    "StaticVariableCase": "CamelCase", "StaticVariablePrefix": "s",
    "GlobalConstantCase": "UPPER_CASE", "LocalConstantCase": "UPPER_CASE",
    "EnumConstantCase": "UPPER_CASE", "ConstexprVariableCase": "UPPER_CASE",
}


def units():
    db = json.load(open(os.path.join(ROOT, "build", "native", "compile_commands.json")))
    out = []
    for e in db:
        f = e["file"].replace("\\", "/")
        if "/games/" + GAME + "/" in f:
            out.append(f)
    return sorted(set(out))


def run():
    os.makedirs(OUT, exist_ok=True)
    cfg = os.path.join(OUT, "tidy.cfg")
    with open(cfg, "w") as f:
        f.write("{CheckOptions: [" + ", ".join(
            "{key: readability-identifier-naming.%s, value: %s}" % (k, v) for k, v in OPTIONS.items()) + "]}\n")
    for u in units():
        name = os.path.basename(u)
        fix = os.path.join(OUT, name + ".yaml")
        if os.path.exists(fix):
            os.remove(fix)
        r = subprocess.run([LLVM + "/bin/clang-tidy.exe", "-p", os.path.join(ROOT, "build", "native"),
                            "--checks=-*,readability-identifier-naming", "--config-file=" + cfg,
                            "--header-filter=.*" + GAME + ".*", "--export-fixes=" + fix, u],
                           capture_output=True, text=True)
        n = r.stdout.count("warning:")
        print("%-24s %4d warnings" % (name, n))


def new_name(kind, old):
    one = len(old) == 1
    if kind == "member":
        return "m" + old[0].upper() + old[1:]
    if kind == "parameter":
        if one:
            return None
        m = re.match(r"^an?([A-Z].*)$", old)
        return "the" + (m.group(1) if m else old[0].upper() + old[1:])
    if kind == "local variable":
        if one or re.match(r"^an?[A-Z]", old):
            return None
        return ("an" if old[0] in "aeiou" else "a") + old[0].upper() + old[1:]
    if kind == "global variable":
        return "g" + old[0].upper() + old[1:]
    if kind == "static variable":
        return "s" + old[0].upper() + old[1:]
    if kind in ("global constant", "local constant") and re.match(r"^k[A-Z]", old):
        return re.sub(r"(?<=[a-z0-9])(?=[A-Z])", "_", old[1:]).upper()
    return None


def apply(dry):
    edits = {}      # file -> {offset: (length, old, new)}
    manual = {}
    for y in sorted(glob.glob(os.path.join(OUT, "*.yaml"))):
        doc = yaml.safe_load(open(y, encoding="utf-8"))
        for d in doc.get("Diagnostics", []):
            m = d["DiagnosticMessage"]
            mm = re.match(r"invalid case style for ([a-z ]+) '(.*)'$", m["Message"])
            if not mm:
                continue
            kind = mm.group(1)
            for r in m.get("Replacements", []):
                path = r["FilePath"].replace("\\", "/")
                if os.path.basename(path) in SKIP_FILES:
                    continue
                data = open(path, "rb").read()
                old = data[r["Offset"]:r["Offset"] + r["Length"]].decode("latin-1")
                nn = new_name(kind, old)
                if nn is None:
                    compliant = kind == "local variable" and re.match(r"^an?[A-Z]", old)
                    if len(old) > 1 and not compliant:
                        manual.setdefault((kind, old), set()).add(os.path.basename(path))
                    continue
                prev = edits.setdefault(path, {}).get(r["Offset"])
                if prev and prev[2] != nn:
                    print("CONFLICT", path, r["Offset"], prev, nn)
                    continue
                edits[path][r["Offset"]] = (r["Length"], old, nn)
    total = 0
    for path, es in edits.items():
        data = bytearray(open(path, "rb").read())
        for off in sorted(es, reverse=True):
            length, old, nn = es[off]
            if data[off:off + length].decode("latin-1") != old:
                print("STALE", path, off, old)
                continue
            data[off:off + length] = nn.encode("latin-1")
            total += 1
        if not dry:
            open(path, "wb").write(bytes(data))
    print("%d places in %d files%s" % (total, len(edits), " (dry run)" if dry else ""))
    for (kind, old), files in sorted(manual.items()):
        print("by hand: %s %s (%s)" % (kind, old, ", ".join(sorted(files))))


if __name__ == "__main__":
    if len(sys.argv) < 2 or sys.argv[1] not in ("run", "apply", "dry"):
        print(__doc__)
        sys.exit(2)
    if sys.argv[1] == "run":
        run()
    else:
        apply(sys.argv[1] == "dry")
