#!/usr/bin/env python3
"""A rename proved by the object code (docs/CODE_STANDARDS.md, "Tools"): every unit of a game compiles to the same
instructions before and after.

    python tools/objsnap.py snap <game> <file.json>    before the change: every object of the game's native build
    python tools/objsnap.py cmp <game> <file.json>     after it (and a rebuild): the objects and functions that differ
    python tools/objsnap.py --selftest                 the comparison's own rules on small cases

A snapshot holds, for each object file under build/native/games/<game> (--build picks another build folder), its
functions in order as hashes of their instructions, a hash of its data, and its section sizes:

- the instructions are `llvm-objdump -d -r` with the symbol names taken out, so each line keeps its address, bytes and
  text and each relocation its offset and kind: a rename changes names, never those;
- the data are the bytes of .rdata and .data (`llvm-objdump -s`): a changed string, table or constant shows;
- the section sizes are `llvm-size -A` with a section's `$name` suffix dropped, since a renamed symbol renames its
  section and nothing else.

`cmp` prints one line per object that differs (the functions only one side has, a changed order, data bytes, section
sizes) and a last line "<game>: N objects, M functions, D objects differ", and exits 1 when D is not 0. Renames, named
constants in place of numbers, comments and line breaks leave every object as it was; anything else that shows is a
change to what the code does, to be proved by the game's suite instead.

Build the same targets before both commands (the game, and its tools if they share its sources), and keep the
snapshot outside the repository's tracked files (under build/). The tools are llvm-mingw's, found through
LLVM_MINGW_ROOT as the build finds them.
"""
import argparse, glob, hashlib, json, os, re, subprocess, sys

ROOT = os.environ.get("LLVM_MINGW_ROOT", "E:/retools/llvm-mingw-20261006-ucrt-x86_64")


def tool(name, *args):
    exe = os.path.join(ROOT, "bin", name + (".exe" if os.name == "nt" else ""))
    return subprocess.run([exe] + list(args), capture_output=True, text=True).stdout


def functions(disassembly):
    """[name, hash, lines] of each function of `llvm-objdump -d -r` output, in order, names taken out of the hash."""
    out = []
    current = None
    for line in disassembly.splitlines():
        label = re.match(r"^[0-9a-f]+ <(.*)>:$", line)
        if label:
            current = [label.group(1), hashlib.sha1(), 0]
            out.append(current)
            continue
        if line.startswith("Disassembly of section"):
            current = None
            continue
        if current is None or not line.strip():
            continue
        relocation = re.match(r"^\s+([0-9a-f]+):\s+(IMAGE_REL_\w+|R_\w+)\s", line)
        if relocation:
            text = "R %s %s" % (relocation.group(1), relocation.group(2))
        else:
            text = re.sub(r"<[^>]*>", "<>", line)
        current[1].update(text.encode() + b"\n")
        current[2] += 1
    return [[name, digest.hexdigest(), lines] for name, digest, lines in out]


def sections(sizes):
    """The sections of `llvm-size -A` output as (name without its $suffix, size), sorted."""
    rows = (line.split() for line in sizes.splitlines())
    return sorted((row[0].split("$")[0], row[1]) for row in rows if len(row) >= 2 and row[0].startswith("."))


def snap_object(path):
    data = tool("llvm-objdump", "-s", "-j", ".rdata", "-j", ".data", path)
    data = "\n".join(line for line in data.splitlines() if "file format" not in line)
    return {"funcs": functions(tool("llvm-objdump", "-d", "-r", path)),
            "data": hashlib.sha1(data.encode()).hexdigest(),
            "sections": hashlib.sha1(repr(sections(tool("llvm-size", "-A", path))).encode()).hexdigest()}


def snapshot(game, build):
    paths = glob.glob(os.path.join(build, "games", game, "CMakeFiles", "*", "**", "*.obj"), recursive=True)
    paths = sorted(p.replace("\\", "/") for p in paths if "CompilerId" not in p)
    return {p.split("CMakeFiles/")[1]: snap_object(p) for p in paths}


def differences(old, new):
    """What differs between two objects' snapshots, as a list of notes (empty when nothing does)."""
    notes = []
    before = [f[1] for f in old["funcs"]]
    after = [f[1] for f in new["funcs"]]
    if before != after:
        if sorted(before) == sorted(after):
            notes.append("the same functions in another order")
        else:
            only_old = [f[0] for f in old["funcs"] if f[1] not in set(after)]
            only_new = [f[0] for f in new["funcs"] if f[1] not in set(before)]
            notes.append("%d functions differ: before %s | after %s" %
                         (max(len(only_old), len(only_new)), demangle(only_old[:4]), demangle(only_new[:4])))
    if old["data"] != new["data"]:
        notes.append("data bytes differ")
    if old["sections"] != new["sections"]:
        notes.append("section sizes differ")
    return notes


def demangle(names):
    if not names:
        return []
    out = tool("llvm-cxxfilt", "-n", *names).splitlines()
    return [n[:80] for n in out] if len(out) == len(names) else names


def compare(game, old, new):
    differ = 0
    for key in sorted(set(old) | set(new)):
        if key not in old or key not in new:
            print("OBJECT %s only %s" % (key, "before" if key in old else "after"))
            differ += 1
            continue
        notes = differences(old[key], new[key])
        if notes:
            differ += 1
            print("DIFF %s: %s" % (key, "; ".join(notes)))
    print("%s: %d objects, %d functions, %d objects differ" %
          (game, len(new), sum(len(v["funcs"]) for v in new.values()), differ))
    return 1 if differ else 0


SELFTEST_BEFORE = """
00000000 <__ZN4Game3OldEi>:
       0: 55                            pushl   %ebp
       1: e8 00 00 00 00                calll   0x6 <__ZN4Game3OldEi+0x6>
                        00000002:  IMAGE_REL_I386_REL32 __ZN4Game6HelperEv
       6: c3                            retl
"""


def selftest():
    renamed = SELFTEST_BEFORE.replace("Old", "New").replace("Helper", "Assist")
    changed = SELFTEST_BEFORE.replace("pushl   %ebp", "pushl   %ebx").replace("55   ", "53   ")
    other_kind = SELFTEST_BEFORE.replace("IMAGE_REL_I386_REL32", "IMAGE_REL_I386_DIR32")
    base = functions(SELFTEST_BEFORE)
    cases = [
        ("a rename leaves the instructions", [f[1] for f in functions(renamed)] == [f[1] for f in base]),
        ("a changed instruction shows", [f[1] for f in functions(changed)] != [f[1] for f in base]),
        ("a changed relocation kind shows", [f[1] for f in functions(other_kind)] != [f[1] for f in base]),
        ("a section's $name suffix is dropped",
         sections(".text 10 0\n.rdata$_ZL3OLD 4 0\n") == sections(".text 10 0\n.rdata$_ZL3NEW 4 0\n")),
        ("a section's size shows", sections(".text 10 0\n") != sections(".text 12 0\n")),
        ("equal snapshots have no difference",
         differences({"funcs": base, "data": "d", "sections": "s"}, {"funcs": functions(renamed), "data": "d", "sections": "s"}) == []),
        ("changed data shows",
         differences({"funcs": base, "data": "d", "sections": "s"}, {"funcs": base, "data": "e", "sections": "s"}) == ["data bytes differ"]),
    ]
    failed = [name for name, ok in cases if not ok]
    for name in failed:
        print("selftest: %s: FAILED" % name)
    print("objsnap selftest: %d of %d cases pass" % (len(cases) - len(failed), len(cases)))
    return 1 if failed else 0


def main():
    if sys.argv[1:] == ["--selftest"]:
        return selftest()
    ap = argparse.ArgumentParser(description="A game's object code before and after a rename (docs/CODE_STANDARDS.md).")
    ap.add_argument("mode", choices=["snap", "cmp"])
    ap.add_argument("game", help="the game's folder name under games/")
    ap.add_argument("file", help="the snapshot (JSON): written by snap, read by cmp")
    ap.add_argument("--build", default="build/native", help="the build folder (default: build/native)")
    args = ap.parse_args()
    now = snapshot(args.game, args.build)
    if not now:
        print("objsnap: no object files under %s/games/%s (build the game first)" % (args.build, args.game))
        return 2
    if args.mode == "snap":
        with open(args.file, "w", encoding="utf-8") as f:
            json.dump(now, f)
        print("%s: %d objects, %d functions" % (args.game, len(now), sum(len(v["funcs"]) for v in now.values())))
        return 0
    with open(args.file, encoding="utf-8") as f:
        return compare(args.game, json.load(f), now)


if __name__ == "__main__":
    sys.exit(main())
