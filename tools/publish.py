#!/usr/bin/env python3
"""Publish the trunk as one snapshot commit (docs/PUBLISHING.md).

The public repository gets the trunk's tree as it is now, without the paths listed in .publishignore and with every
private block of a text file replaced by its one published line. Each snapshot is one commit on top of the last one,
so this repository's history never leaves the machine.

    python tools/publish.py            build and check the snapshot into refs/publish/next, print what it holds
    python tools/publish.py --push     build it again and push it to the remote's main (then refs/publish/main)
        [--source <rev>]               the tree to publish (default main)
        [--remote <name>]              default origin
        [-m <message>]                 the commit's first line (default "byo-pop snapshot, <date>")

    python tools/publish.py --target <game> [--push] --model "..."
                                       the same for one game's derived repository (docs/PUBLISHING.md, "Derived
                                       repositories"): the tree is the paths of tools/publish_targets/<game>.txt, in
                                       refs/publish/<game>/next and /main, pushed to the url the manifest names
    python tools/publish.py --target <game> --build-check
                                       builds refs/publish/<game>/next in a temporary folder and plays a scripted game
                                       with its exe and the trunk's, which must give the same state log

A private block is a run of lines between two marker lines, each starting at its line's first column:

    <!-- private: The line the published copy shows instead. -->
    ...
    <!-- /private -->

The opening marker may be a plain <!-- private --> (the block is dropped). Blocks do not nest.
"""
import argparse, datetime, fnmatch, io, os, re, shutil, subprocess, sys, tarfile, tempfile

NEXT, LAST = "refs/publish/next", "refs/publish/main"
START = re.compile(r"^<!-- private(?:: (.*?))? -->[ \t]*\r?$")
END = re.compile(r"^<!-- /private -->[ \t]*\r?$")
MARKER = r"^<!-- (/private|private(: .*)?) -->[[:space:]]*$"  # the same lines, for git grep -E
FORBIDDEN = ("assets/", "re/", "build/")  # staged game files, Ghidra exports, builds (all git-ignored)
# the commit names the model that made it (CLAUDE.md, "Commits"): --model, e.g. "Opus 5.5"
TRAILER = "Co-Authored-By: Claude {model} <noreply@anthropic.com>"


def git(*args, env=None, data=None, ok=(0,)):
    r = subprocess.run(["git", *args], input=data, capture_output=True, env=env)
    if r.returncode not in ok:
        sys.exit(f"git {' '.join(args)}: {r.stderr.decode(errors='replace').strip()}")
    return r.stdout


def strip_private(path, data):
    """The file without its private blocks; each block becomes its replacement line, if it has one."""
    out, start = [], 0
    for n, line in enumerate(data.decode("utf-8").splitlines(keepends=True), 1):
        m, end = START.match(line.rstrip("\n")), END.match(line.rstrip("\n"))
        if m:
            if start:
                sys.exit(f"{path}:{n}: a private block inside the one from line {start}")
            start = n
            if m.group(1):
                out.append(m.group(1) + ("\r\n" if line.endswith("\r\n") else "\n"))
        elif end:
            if not start:
                sys.exit(f"{path}:{n}: <!-- /private --> without a block")
            start = 0
        elif not start:
            out.append(line)
    if start:
        sys.exit(f"{path}:{start}: the private block is not closed")
    return "".join(out).encode("utf-8")


def ignored(path, specs):
    return any(path == s or path.startswith(s.rstrip("/") + "/") or fnmatch.fnmatchcase(path, s) for s in specs)


# ---- derived repositories (--target): the manifest, the generated files and the build check ----

def line_at(text, start):
    """The offset of the line that starts with <start>; the publish stops when the trunk's file no longer has it."""
    i = text.find(start) if text.startswith(start) else text.find("\n" + start) + 1
    if i <= 0 and not text.startswith(start):
        sys.exit(f"a transform: no line starts with '{start}'; the trunk's file changed, so change the transform")
    return i


def tf_toplevel(text, game):
    """The trunk's top-level CMakeLists.txt for one game: the toolchain and SDL2 logic, the framework, and the game."""
    a, b = line_at(text, "# byo-pop (Bring Your Own"), line_at(text, "set(CMAKE_CXX_STANDARD 17)")
    web = line_at(text, "# byopop_web_game()")
    shared, tests = line_at(text, "# Shared by every game"), line_at(text, "# Framework test app")
    if "project(byo-pop C CXX)\n" not in text[a:b] or f"add_subdirectory(games/{game})" not in text:
        sys.exit(f"a transform: the top-level CMakeLists.txt changed (its project line, or no games/{game})")
    head = (f"cmake_minimum_required(VERSION 3.20)\n"
            f"# byo-pop-{game}: one game of byo-pop (https://github.com/allisonpelton/byo-pop), generated from its top-level\n"
            f"# CMakeLists.txt by tools/publish.py. The game's files come from the user's own install (assets/{game}/).\n"
            f"# Native builds only: the browser version is built from byo-pop, which has the site.\n"
            f"project(byo-pop-{game} C CXX)\n"
            f"if(EMSCRIPTEN)\n  message(FATAL_ERROR \"This repository builds the native game only; build the web version from byo-pop\")\nendif()\n\n")
    # from the standards to the Emscripten branch of SDL2, then third_party; the web and test-app blocks are left out
    body = text[b:web] + text[shared:tests]
    tail = f"option(BUILD_{game.upper()} \"Build the {game} port\" ON)\nif(BUILD_{game.upper()})\n  add_subdirectory(games/{game})\nendif()\n"
    return head + body + tail


def tf_framework(text, _):
    """framework/CMakeLists.txt without the MO3 and JPEG 2000 libraries (platform/openmpt, platform/openjpeg)."""
    return text[:line_at(text, "# MO3 music through libopenmpt")] + text[line_at(text, "# The early-2004 SexyAppFramework's"):]


def tf_third_party(text, _):
    """third_party/CMakeLists.txt with only stb, libjpeg, libxmp-lite, Tremor and minimp3 (which lives in libopenmpt/)."""
    a, b = line_at(text, "# libopenmpt 0.8.9"), line_at(text, "# minimp3 (CC0")
    text = text[:a] + "set(OPENMPT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libopenmpt)\n" + text[b:]
    return text[:line_at(text, "add_library(openmpt STATIC")]


TRANSFORMS = {"toplevel": tf_toplevel, "framework": tf_framework, "third_party": tf_third_party}


def read_manifest(source, target):
    """The target's manifest from the source tree: its url, name, include and exclude paths, and the generated files."""
    m = {"url": None, "name": target, "include": [], "exclude": [], "put": []}
    path = f"tools/publish_targets/{target}.txt"
    for n, line in enumerate(git("show", f"{source}:{path}").decode().splitlines(), 1):
        w = line.split()
        if not w or w[0].startswith("#"):
            continue
        if w[0] in ("url", "name") and len(w) == 2:
            m[w[0]] = w[1]
        elif w[0] in ("include", "exclude") and len(w) == 2:
            m[w[0]].append(w[1])
        elif (w[0] == "file" and len(w) == 3) or (w[0] == "transform" and len(w) in (4, 5) and w[3] in TRANSFORMS):
            m["put"].append((w[0], w[1:]))
        else:
            sys.exit(f"{path}:{n}: not a directive: {line}")
    if not m["url"] or not m["include"]:
        sys.exit(f"{path}: it needs a url and at least one include")
    return m


def target_files(source, m, env):
    """Fills the index with the manifest's tree. Returns the include lines that matched nothing."""
    trunk = {}
    for entry in git("ls-tree", "-r", "-z", source).decode().split("\0"):
        if entry:
            info, path = entry.split("\t", 1)
            mode, _, blob = info.split()
            trunk[path] = (mode, blob)
    rows, unused = [], []
    for inc in m["include"]:
        hit = [p for p in trunk if ignored(p, [inc]) and not ignored(p, m["exclude"])]
        if not hit:
            unused.append(inc)
        rows += [f"{trunk[p][0]} {trunk[p][1]}\t{p}\n" for p in hit]
    for kind, args in m["put"]:
        if args[1] not in trunk:
            sys.exit(f"the manifest names {args[1]}, which the trunk does not have")
        if kind == "file":
            mode, blob = trunk[args[1]]
        else:
            text = git("cat-file", "blob", trunk[args[1]][1]).decode("utf-8")
            data = TRANSFORMS[args[2]](text, args[3] if len(args) > 3 else None).encode("utf-8")
            mode, blob = "100644", git("hash-object", "-w", "--no-filters", "--stdin", data=data).decode().strip()
        rows.append(f"{mode} {blob}\t{args[0]}\n")
    git("update-index", "--add", "--index-info", env=env, data="".join(rows).encode())
    return unused


def build_check(target):
    """Checks refs/publish/<target>/next out into a temporary folder, builds the game there with the trunk's generator and
    toolchain (docs/BUILDS.md), and plays one scripted game with that exe and with the trunk's, both under rundesk and
    on the main checkout's staged files, as tools/check.sh's rocketmania_state_game does: the state logs must be the same."""
    ref = f"refs/publish/{target}/next"
    git("rev-parse", "--verify", ref)
    common = os.path.dirname(os.path.abspath(git("rev-parse", "--git-common-dir").decode().strip()))  # the main checkout
    rundesk, trunk_exe, assets = (os.path.join(common, *p) for p in
                                  (("build", "native", "rundesk.exe"), ("build", "native", target + ".exe"), ("assets", target)))
    for f in (rundesk, trunk_exe, assets):
        if not os.path.exists(f):
            sys.exit(f"build check: {f} is missing (the main checkout's native build and staged game files)")
    tmp = tempfile.mkdtemp(prefix=f"byopop-{target}-")
    try:
        tree, build = os.path.join(tmp, "tree"), os.path.join(tmp, "build")
        os.mkdir(tree)
        tarfile.open(fileobj=io.BytesIO(git("archive", "--format=tar", ref)), mode="r:").extractall(tree)
        sdl = os.environ.get("SDL2_DIR", "E:/retools/SDL2-2.32.10/i686-w64-mingw32/lib/cmake/SDL2")
        # tools/build.sh finds pip's cmake and ninja here
        env = dict(os.environ, PATH=os.path.join(os.environ.get("APPDATA", ""), "Python", "Python312", "Scripts") + os.pathsep + os.environ["PATH"])
        for cmd in (["cmake", "-S", tree, "-B", build, "-G", "Ninja", "-DCMAKE_BUILD_TYPE=Release",
                     f"-DCMAKE_TOOLCHAIN_FILE={tree}/cmake/llvm-mingw-i686.cmake", f"-DSDL2_DIR={sdl}"],
                    ["cmake", "--build", build, "--target", target]):
            print("+", " ".join(cmd), flush=True)
            cmd[0] = shutil.which(cmd[0], path=env["PATH"]) or sys.exit("build check: cmake is not installed")
            if subprocess.run(cmd, env=env).returncode:
                sys.exit("build check: the derived tree does not build")
        exe = os.path.join(build, target + ".exe")
        if not os.path.isfile(exe):
            sys.exit(f"build check: {exe} was not made")
        logs = []
        for name, e in (("derived", exe), ("trunk", trunk_exe)):
            d = os.path.join(tmp, name)
            os.makedirs(d)
            cmd = [rundesk, e, f"-gamedir={assets}", f"-userdir={d}/user", "-fast", "-drawevery", "-seed=777", "-uibot=2",
                   "-uistart=300000", "-uimodes=0", "-uidiffs=2", "-uigames=1", "-uicap=3000", f"-statelog={d}/state.txt",
                   f"-uiscript={d}/script.txt"]
            r = subprocess.run(cmd, capture_output=True, timeout=600)
            if r.returncode or not os.path.isfile(f"{d}/state.txt"):
                sys.exit(f"build check: the {name} exe's run failed (status {r.returncode}): {r.stderr.decode(errors='replace')[-400:]}")
            with open(f"{d}/state.txt", "rb") as f:
                logs.append(f.read())
        if not logs[0]:
            sys.exit("build check: the state log is empty")
        if logs[0] != logs[1]:
            sys.exit("build check: the derived exe's state log differs from the trunk's")
        print(f"build check: the derived tree builds, and its scripted game's state log ({len(logs[0])} bytes) is the trunk exe's")
    finally:
        shutil.rmtree(tmp, ignore_errors=True)


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--push", action="store_true")
    ap.add_argument("--source", default="main")
    ap.add_argument("--remote", default="origin")
    ap.add_argument("-m", dest="message")
    ap.add_argument("--model", help='the model making the snapshot, for its trailer, e.g. "Opus 5.5"')
    ap.add_argument("--target", help="one game's derived repository: tools/publish_targets/<target>.txt")
    ap.add_argument("--build-check", action="store_true",
                    help="with --target: build the last derived snapshot and compare a game's state log with the trunk's")
    a = ap.parse_args()
    if a.build_check and not a.target:
        sys.exit("--build-check needs --target")
    if not a.build_check and not a.model:
        sys.exit("--model is needed")
    os.chdir(git("rev-parse", "--show-toplevel").decode().strip())
    if a.build_check:
        return build_check(a.target)
    source = git("rev-parse", "--verify", a.source + "^{commit}").decode().strip()
    nxt, last = (f"refs/publish/{a.target}/next", f"refs/publish/{a.target}/main") if a.target else (NEXT, LAST)
    manifest = read_manifest(source, a.target) if a.target else None
    specs = [s.strip() for s in git("show", f"{source}:.publishignore").decode().splitlines()
             if s.strip() and not s.lstrip().startswith("#")]

    with tempfile.TemporaryDirectory() as tmp:
        env = dict(os.environ, GIT_INDEX_FILE=os.path.join(tmp, "index"))
        if manifest:
            unused = target_files(source, manifest, env)
            if unused:
                sys.exit("manifest paths that match nothing in the trunk: " + ", ".join(unused))
        else:
            git("read-tree", source, env=env)
        files = {}  # path -> (mode, blob)
        for entry in git("ls-files", "-s", "-z", env=env).decode().split("\0"):
            if entry:
                info, path = entry.split("\t", 1)
                mode, blob, _ = info.split()
                files[path] = (mode, blob)

        dropped = sorted(p for p in files if ignored(p, specs))
        git("update-index", "--force-remove", "-z", "--stdin", env=env, data="".join(p + "\0" for p in dropped).encode())
        unmatched = [] if manifest else [s for s in specs if not any(ignored(p, [s]) for p in dropped)]

        marked = [p for p in git("grep", "--cached", "-l", "-z", "-I", "-E", "-e", MARKER, env=env, ok=(0, 1))
                  .decode().split("\0") if p]
        for path in marked:
            mode, blob = files[path]
            text = strip_private(path, git("cat-file", "blob", blob))
            new = git("hash-object", "-w", "--no-filters", "--stdin", data=text).decode().strip()
            git("update-index", "--cacheinfo", f"{mode},{new},{path}", env=env)

        # The checks: nothing private, no game files' folders
        left = git("grep", "--cached", "-n", "-I", "-E", "-e", MARKER, env=env, ok=(0, 1)).decode()
        if left:
            sys.exit("private markers left in the snapshot:\n" + left)
        published = [p for p in git("ls-files", "-z", env=env).decode().split("\0") if p]
        bad = [p for p in published if p.startswith(FORBIDDEN) or ignored(p, specs)]
        if bad:
            sys.exit("paths that must not be published:\n  " + "\n  ".join(bad))
        tree = git("write-tree", env=env).decode().strip()

    parent = git("rev-parse", "--verify", "-q", last, ok=(0, 1)).decode().strip()
    if parent and git("rev-parse", parent + "^{tree}").decode().strip() == tree:
        print(f"nothing new: {last} ({parent[:10]}) already has this tree")
        return
    title = f"byo-pop-{a.target}" if a.target else "byo-pop"
    message = (a.message or f"{title} snapshot, {datetime.date.today().isoformat()}") + \
        f"\n\nPublished by tools/publish.py from {source[:10]}.\n\n{TRAILER.format(model=a.model)}\n"
    commit = git("commit-tree", tree, *(["-p", parent] if parent else []), "-F", "-", data=message.encode()).decode().strip()
    git("update-ref", nxt, commit)
    print(f"snapshot {commit[:10]} of {source[:10]} in {nxt}: {len(published)} files, parent "
          f"{parent[:10] if parent else '(none: the first)'}")
    print("  left out: " + (", ".join(dropped) or "nothing"))
    print("  private blocks stripped from: " + (", ".join(marked) or "nothing"))
    if unmatched:
        print("  .publishignore lines that match nothing: " + ", ".join(unmatched))
    if a.push:
        remote = manifest["url"] if manifest else a.remote
        if subprocess.run(["git", "push", remote, f"{commit}:refs/heads/main"]).returncode:
            sys.exit(f"the push failed; {last} is unchanged")
        git("update-ref", last, commit)
        print(f"pushed to {remote} main; {last} is {commit[:10]}")


if __name__ == "__main__":
    main()
