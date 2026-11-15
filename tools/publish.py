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
    python tools/publish.py --target <game> --release-zip
                                       builds refs/publish/<game>/next the same way and writes the release ZIP (the exe,
                                       the DLLs it loads and the README) to build/byo-pop-<game>-<date>.zip; uploads
                                       nothing. With --build-check too, one build serves both.
    python tools/publish.py [--target <game>] --out <dir>
                                       writes the tree the snapshot would hold into <dir> (which must not exist) and
                                       makes no commit and no ref: for reading it, or building it before a snapshot

A derived repository's code is native only and has neither the site nor the mod framework: the manifest's rewords
tables give the published copy its own wording of the trunk comments that name either (each entry must still match the
trunk exactly), its unifdef line decides the conditionals on __EMSCRIPTEN__ (the web build) and BYOPOP_DERIVED (the
trunk's marker for the code a derived repository leaves out or has otherwise) in every C/C++ file, and its forbid lines
stop the publish while a file still names either.

A private block is a run of lines between two marker lines, each starting at its line's first column:

    <!-- private: The line the published copy shows instead. -->
    ...
    <!-- /private -->

The opening marker may be a plain <!-- private --> (the block is dropped). Blocks do not nest.
"""
import argparse, datetime, fnmatch, io, os, re, shutil, struct, subprocess, sys, tarfile, tempfile, zipfile

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


def replace_once(text, old, new):
    """text with old, which it must hold, replaced by new; the publish stops when the trunk's file no longer has it."""
    if old not in text:
        sys.exit(f"a transform: no '{old}'; the trunk's file changed, so change the transform")
    return text.replace(old, new, 1)


CMAKE_IF = re.compile(r"^([ \t]*)(if|elseif|else|endif)[ \t]*\((.*)\)[ \t]*$")


def cmake_native(path, text):
    """CMake text with its conditions on EMSCRIPTEN decided for a native build: if(EMSCRIPTEN) and if(NOT EMSCRIPTEN)
    keep one branch, without their own lines and one level less indented, and if((NOT EMSCRIPTEN) AND <rest>) becomes
    if(<rest>). Any other test of EMSCRIPTEN stops the publish."""
    out, stack, dropped = [], [], False  # a frame: [decided, emitting, taken]
    for n, line in enumerate(text.splitlines(keepends=True), 1):
        m = CMAKE_IF.match(line.rstrip("\r\n"))
        kind, cond = (m.group(2), m.group(3).strip()) if m else (None, "")
        named = re.search(r"\bEMSCRIPTEN\b", cond)
        outer = all(f[1] for f in (stack if kind in (None, "if") else stack[:-1]))
        if kind == "if" and named:
            if cond in ("EMSCRIPTEN", "NOT EMSCRIPTEN"):
                c = cond == "NOT EMSCRIPTEN"
                stack.append([True, c, c])
                dropped = True
                continue
            rest = re.fullmatch(r"\(NOT EMSCRIPTEN\) AND (.+)", cond)
            if not rest or re.search(r"\bEMSCRIPTEN\b", rest.group(1)):
                sys.exit(f"{path}:{n}: publish.py cannot decide if({cond}) for a native build")
            line = f"{m.group(1)}if({rest.group(1)})\n"
        elif kind == "if":
            pass
        elif kind:
            if not stack:
                sys.exit(f"{path}:{n}: {kind}() without if()")
            f = stack[-1]
            if kind == "endif":
                stack.pop()
            if named or (f[0] and kind == "elseif"):
                sys.exit(f"{path}:{n}: publish.py cannot decide {kind}({cond}) for a native build")
            if f[0]:
                if kind == "else":
                    f[1], f[2] = not f[2], True
                dropped = True
                continue
        if kind == "if":
            stack.append([False, True, True])
        if not outer:
            continue
        if dropped and line.strip() == "" and (not out or out[-1].strip() == ""):
            continue
        for _ in range(sum(1 for f in stack if f[0])):  # one level less for each branch kept
            line = line[2:] if line.startswith("  ") else line
        out.append(line)
        dropped = False
    if stack:
        sys.exit(f"{path}: an if() is not closed")
    return "".join(out)


def tf_toplevel(text, game):
    """A top-level CMakeLists.txt for one game, natively: the trunk's SDL2 logic, third_party, the framework and the
    runtime DLLs, then the game. The trunk's web and test-app parts and its build-machine settings are left out."""
    sdl, web = line_at(text, "if(EMSCRIPTEN)\n  set(SDL2_LIBRARIES"), line_at(text, "# byopop_web_game()")
    shared, tests = line_at(text, "# Shared by every game"), line_at(text, "# Framework test app")
    if f"add_subdirectory(games/{game})" not in text:
        sys.exit(f"a transform: the top-level CMakeLists.txt has no games/{game}")
    head = ("cmake_minimum_required(VERSION 3.20)\n"
            f"# The game (games/{game}), the SexyAppFramework port it runs on (framework/, platform/) and the libraries they\n"
            "# use (third_party/), for Windows. Generated from byo-pop's top-level CMakeLists.txt by its tools/publish.py.\n"
            f"project(byo-pop-{game} C CXX)\n\n"
            "set(CMAKE_CXX_STANDARD 17)\nset(CMAKE_CXX_STANDARD_REQUIRED ON)\nset(CMAKE_EXPORT_COMPILE_COMMANDS ON)\n"
            "# the exe and the DLLs it loads land in the build folder itself\n"
            "set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})\n\n")
    body = cmake_native("CMakeLists.txt", text[sdl:web] + "\n" + text[shared:tests])
    return head + body + f"add_subdirectory(games/{game})\n"


def tf_framework(text, _):
    """framework/CMakeLists.txt natively, without the MO3 and JPEG 2000 libraries (platform/openmpt, platform/openjpeg)."""
    text = text[:line_at(text, "# MO3 music through libopenmpt")] + text[line_at(text, "# The early-2004 SexyAppFramework's"):]
    a, b = line_at(text, "# The table of what is imported from an installed game"), line_at(text, "set(SAVEIMPORT_JSON")
    text = (text[:a] + "# The table of what is imported from an installed game (platform/saveimport.json), compiled into the\n"
            "# framework for platform/SaveImport.cpp, which the game's main.cpp calls for -importsaves.\n" + text[b:])
    return cmake_native("framework/CMakeLists.txt", text)


def tf_third_party(text, _):
    """third_party/CMakeLists.txt natively, with only stb, libjpeg, libxmp-lite, Tremor and minimp3 (which lives in
    libopenmpt/)."""
    a, b = line_at(text, "# libopenmpt 0.8.9"), line_at(text, "# minimp3 (CC0")
    text = text[:a] + "set(OPENMPT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libopenmpt)\n" + text[b:]
    text = text[:line_at(text, "add_library(openmpt STATIC")]
    text = replace_once(text, " for both native and Emscripten targets.", ".")
    text = replace_once(text, " (an error for C99 compilers such as Emscripten's clang)", " (an error for strict C99 compilers)")
    return cmake_native("third_party/CMakeLists.txt", text)


def tf_game(text, game):
    """games/<game>/CMakeLists.txt natively, under a header of its own (the trunk's names the web build and byo-pop's
    notes)."""
    lines = text.splitlines(keepends=True)
    while lines and lines[0].startswith("#"):
        lines.pop(0)
    head = (f"# The game: its code (src/) and its test hooks (src/testhooks/: the scripted runs that check a build), on the\n"
            "# SexyAppFramework port (framework/, platform/). The exe finds the game's own files by itself (src/main.cpp).\n")
    return head + cmake_native(f"games/{game}/CMakeLists.txt", "".join(lines))


def tf_saveimport(text, _):
    """platform/saveimport.json with a "doc" of its own (the trunk's names the site's page): the table is the same."""
    a = line_at(text, '  "doc": "')
    b = text.index("\n", a) + 1
    doc = text[a:b]
    rules = doc.find("Nothing outside this table")
    if rules < 0 or not doc.endswith('",\n'):
        sys.exit("a transform: platform/saveimport.json's doc changed, so change the transform")
    return (text[:a] + '  "doc": "What -importsaves brings over from an original game installed on this computer '
            '(platform/SaveImport.cpp, which has this table compiled in). ' + doc[rules:] + text[b:])


def tf_toolchain(text, _):
    """cmake/llvm-mingw-i686.cmake without the web build in its comment, and asking for LLVM_MINGW_ROOT where the trunk's
    falls back to the trunk machine's own toolchain folder."""
    text = replace_once(text, " match both\n# the original executable and wasm32.", " match\n# the original executable.")
    fallback = re.search(r'(?m)^    set\(LLVM_MINGW_ROOT "(?!\$ENV)[^"\n]*"\)$', text)
    if not fallback:
        sys.exit("a transform: cmake/llvm-mingw-i686.cmake has no fallback folder, so change the transform")
    return (text[:fallback.start()] + '    message(FATAL_ERROR "Set LLVM_MINGW_ROOT to your llvm-mingw folder")'
            + text[fallback.end():])


def tf_stage(text, _):
    """games/<game>/tools/stage_<game>_assets.py without the web build in its comment, and without the trunk machine's
    own Steam library among the folders it looks in."""
    text = replace_once(text, "systems (Emscripten MEMFS/IDBFS, Linux) see", "systems (Linux) see")
    return re.sub(r'(?m)^ *r"A:\\SteamLibrary\\[^"\n]*",\n', "", text)


TRANSFORMS = {"toplevel": tf_toplevel, "framework": tf_framework, "third_party": tf_third_party, "game": tf_game,
              "saveimport": tf_saveimport, "toolchain": tf_toolchain, "stage": tf_stage}


C_SOURCES = (".c", ".cpp", ".h", ".hpp", ".inc", ".inl")
DIRECTIVE = re.compile(r"^[ \t]*#[ \t]*(if|ifdef|ifndef|elif|else|endif)\b(.*?)\r?\n?$")


def condition(path, n, kind, expr, symbols):
    """A conditional's test, decided by symbols ({name: defined}): True or False when the symbols decide it, the rest of an
    || chain when some of its terms are left, None when it names none of them. Any other test of them stops the publish."""
    e = re.sub(r"/\*.*?\*/|//.*$", "", expr).strip()
    if not any(re.search(rf"\b{s}\b", e) for s in symbols):
        return None
    if kind in ("ifdef", "ifndef"):
        if e not in symbols:
            sys.exit(f"{path}:{n}: publish.py cannot decide #{kind} {e}")
        return symbols[e] == (kind == "ifdef")
    left = []
    for t in (t.strip() for t in e.split("||")):
        m = re.fullmatch(r"(!?)\s*defined\s*(?:\(\s*(\w+)\s*\)|\s(\w+))", t)
        name = m and (m.group(2) or m.group(3))
        if name in symbols:
            if symbols[name] != bool(m.group(1)):
                return True
        elif any(re.search(rf"\b{s}\b", t) for s in symbols):
            sys.exit(f"{path}:{n}: publish.py cannot decide #{kind} {expr.strip()}")
        else:
            left.append(t)
    return " || ".join(left) if left else False


def unifdef(path, text, symbols):
    """The C or C++ text with its conditionals on symbols decided, as unifdef -D/-U would: a branch kept loses its #if,
    #else and #endif, a branch dropped goes whole, and the other conditionals stay as they are."""
    out, stack, dropped = [], [], False  # a frame: [decided, emitting, taken], taken None once a kept chain ended
    for n, line in enumerate(text.splitlines(keepends=True), 1):
        m = DIRECTIVE.match(line)
        eol = "\r\n" if line.endswith("\r\n") else "\n"
        if not m:
            if all(f[1] for f in stack):
                if dropped and line.strip() == "" and (not out or out[-1].strip() == ""):
                    continue
                out.append(line)
                dropped = False
            continue
        kind, expr = m.group(1), m.group(2)
        indent = line[:line.index("#")]
        if kind in ("if", "ifdef", "ifndef"):
            outer = all(f[1] for f in stack)
            c = condition(path, n, kind, expr, symbols) if outer else None
            if isinstance(c, bool):
                stack.append([True, c, c])
                dropped = True
                continue
            if outer:
                out.append(line if c is None else f"{indent}#if {c}{eol}")
            stack.append([False, True, True])
            continue
        if not stack:
            sys.exit(f"{path}:{n}: #{kind} without #if")
        f = stack[-1]
        outer = all(g[1] for g in stack[:-1])
        if kind == "endif":
            stack.pop()
            if f[0]:
                dropped = True
            elif outer:
                out.append(line)
            continue
        c = condition(path, n, "elif", expr, symbols) if (kind == "elif" and outer) else None
        if f[0]:  # a decided chain: its #elif and #else lines go too
            if kind == "else":
                f[1], f[2] = not f[2], True
            elif isinstance(c, bool):
                f[1], f[2] = (c and not f[2]), (f[2] or c)
            elif not f[2]:
                sys.exit(f"{path}:{n}: publish.py cannot turn this #elif into an #if")
            else:
                f[1] = False
            dropped = True
            continue
        if f[2] is None:  # a chain that ended at an #elif the symbols made true
            f[1] = False
            continue
        if isinstance(c, bool):  # an #elif the symbols decide in a chain that stays
            if c and outer:
                out.append(f"{indent}#else{eol}")
            f[1], f[2] = c, (None if c else True)
            dropped = True
            continue
        if outer:
            out.append(line if not isinstance(c, str) else f"{indent}#elif {c}{eol}")
        f[1] = True
    if stack:
        sys.exit(f"{path}: an #if is not closed")
    return "".join(out)


def blobs(shas):
    """The contents of git blobs, read by one git cat-file --batch."""
    data = git("cat-file", "--batch", data="".join(s + "\n" for s in shas).encode())
    out, i = {}, 0
    for s in shas:
        j = data.index(b"\n", i)
        size = int(data[i:j].split()[2])
        out[s] = data[j + 1:j + 1 + size]
        i = j + 1 + size + 1
    return out


def read_rewords(source, path):
    """A rewords table from the source tree: entries {path, count, old, new}, old and new as lists of lines.

        @@ <published path> [x<n>]     an entry; x<n>: the old lines occur n times in the file (else once)
        <the trunk's lines, exactly>
        @to
        <the lines that replace them; none: the old lines go>

    Lines starting "#!" are notes, and blank lines at the end of an entry's old or new lines are not part of them."""
    entries, cur = [], None
    for n, raw in enumerate(git("show", f"{source}:{path}").decode("utf-8").split("\n"), 1):
        line = raw.rstrip("\r")
        if line.startswith("#!"):
            continue
        if line.startswith("@@ "):
            w = line[3:].split()
            if not (len(w) == 1 or (len(w) == 2 and re.fullmatch(r"x[1-9][0-9]*", w[1]))):
                sys.exit(f"{path}:{n}: not an entry: {line}")
            cur = {"path": w[0], "count": int(w[1][1:]) if len(w) == 2 else 1, "old": [], "new": None, "line": n}
            entries.append(cur)
        elif cur is None:
            if line.strip():
                sys.exit(f"{path}:{n}: text before the first entry")
        elif line == "@to" and cur["new"] is None:
            cur["new"] = []
        else:
            cur["old" if cur["new"] is None else "new"].append(line)
    for e in entries:
        for k in ("old", "new"):
            while e[k] and not e[k][-1].strip():
                e[k].pop()
        if not e["old"] or e["new"] is None:
            sys.exit(f"{path}:{e['line']}: the entry for {e['path']} needs its old lines and an @to")
    return entries


def reword(text, e):
    """text with e's old lines replaced by its new ones, and how many times the old lines were found (a line's \\r
    left aside when comparing)."""
    lines = text.split("\n")
    cr = "\r" if lines and lines[0].endswith("\r") else ""
    plain = [l.rstrip("\r") for l in lines]
    out, i, found, size = [], 0, 0, len(e["old"])
    while i < len(lines):
        if plain[i:i + size] == e["old"]:
            out += [l + cr for l in e["new"]]
            i += size
            found += 1
        else:
            out.append(lines[i])
            i += 1
    return "\n".join(out), found


def derived_code(files, m, env, source):
    """For a derived target: applies the manifest's rewords tables to the published files, decides its unifdef symbols
    in its C and C++ files (not in vendor paths), then checks every text file outside them against the forbid patterns.
    Updates files (path -> (mode, blob)). Returns the files whose conditionals were decided and the number of rewords."""
    own = [p for p in files if not ignored(p, m["vendor"])]
    texts = blobs(sorted({files[p][1] for p in own}))
    changed, missed, reworded = [], [], 0

    def put(p, new):
        blob = git("hash-object", "-w", "--no-filters", "--stdin", data=new).decode().strip()
        git("update-index", "--cacheinfo", f"{files[p][0]},{blob},{p}", env=env)
        files[p] = (files[p][0], blob)
        texts[blob] = new

    for table in m["rewords"]:
        for e in read_rewords(source, table):
            if e["path"] not in own:
                missed.append(f"{table}:{e['line']}: {e['path']} is not a published file outside the vendor paths")
                continue
            new, found = reword(texts[files[e["path"]][1]].decode("utf-8"), e)
            if found != e["count"]:
                missed.append(f"{table}:{e['line']}: {e['path']} has its old lines {found} times, not {e['count']}: "
                              f"{e['old'][0].strip()[:90]}")
                continue
            put(e["path"], new.encode("utf-8"))
            reworded += 1
    if missed:
        sys.exit(f"{len(missed)} rewords no longer match the trunk (change the table):\n  " + "\n  ".join(missed))
    if m["unifdef"]:
        for p in own:
            if p.endswith(C_SOURCES):
                old = texts[files[p][1]]
                new = unifdef(p, old.decode("utf-8"), m["unifdef"]).encode("utf-8")
                if new != old:
                    put(p, new)
                    changed.append(p)
    hits, forbid = [], m["forbid"] + [re.compile(rf"\b{s}\b") for s in m["unifdef"]]  # a decided symbol left is a hit too
    for p in own:
        data = texts[files[p][1]]
        if b"\0" in data:
            continue
        for n, line in enumerate(data.decode("utf-8", "replace").splitlines(), 1):
            if any(f.search(line) for f in forbid):
                hits.append(f"{p}:{n}: {line.strip()[:140]}")
    if hits:
        sys.exit(f"{len(hits)} lines name what the manifest forbids (its forbid lines):\n  " + "\n  ".join(hits))
    return changed, reworded


def read_manifest(source, target):
    """The target's manifest from the source tree: its url, name, include and exclude paths, the generated files, the
    unifdef symbols, the vendor paths, the forbid patterns and the rewords tables."""
    m = {"url": None, "name": target, "include": [], "exclude": [], "put": [], "unifdef": {}, "vendor": [], "forbid": [],
         "rewords": []}
    path = f"tools/publish_targets/{target}.txt"
    for n, line in enumerate(git("show", f"{source}:{path}").decode().splitlines(), 1):
        w = line.split()
        if not w or w[0].startswith("#"):
            continue
        if w[0] in ("url", "name") and len(w) == 2:
            m[w[0]] = w[1]
        elif w[0] in ("include", "exclude", "vendor", "rewords") and len(w) == 2:
            m[w[0]].append(w[1])
        elif w[0] == "unifdef" and len(w) > 1 and all(re.fullmatch(r"-[DU]\w+", a) for a in w[1:]):
            m["unifdef"].update((a[2:], a[1] == "D") for a in w[1:])
        elif w[0] == "forbid" and len(w) > 1:
            try:
                m["forbid"].append(re.compile(line.split(None, 1)[1]))
            except re.error as e:
                sys.exit(f"{path}:{n}: {e}")
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


def pe_imports(path):
    """The DLLs a Windows exe or DLL names in its import and delay-load tables."""
    with open(path, "rb") as f:
        d = f.read()
    pe = struct.unpack_from("<I", d, 0x3c)[0]
    sections, optional_size = struct.unpack_from("<H12xH", d, pe + 6)
    opt = pe + 24
    dirs = opt + (96 if struct.unpack_from("<H", d, opt)[0] == 0x10b else 112)  # PE32 or PE32+
    count = struct.unpack_from("<I", d, dirs - 4)[0]
    table = [struct.unpack_from("<8sIIII", d, opt + optional_size + 40 * i) for i in range(sections)]

    def offset(rva):
        for _, size, va, raw_size, raw in table:
            if va <= rva < va + max(size, raw_size):
                return raw + rva - va
        sys.exit(f"{path}: an address outside its sections")
    names = []
    for index, size, name_at in ((1, 20, 12), (13, 32, 4)):  # the import table, the delay-load table
        rva = struct.unpack_from("<I", d, dirs + 8 * index)[0] if index < count else 0
        at = offset(rva) if rva else None
        while at is not None and d[at:at + size] != bytes(size):
            name = offset(struct.unpack_from("<I", d, at + name_at)[0])
            names.append(d[name:d.index(b"\0", name)].decode("ascii", "replace"))
            at += size
    return names


def runtime_dlls(exe, folder):
    """The DLLs of folder that exe loads, directly or through another of them (the system's are not in the folder)."""
    have = {n.lower(): n for n in os.listdir(folder) if n.lower().endswith(".dll")}
    need, todo = [], [exe]
    while todo:
        for name in pe_imports(todo.pop()):
            n = have.get(name.lower())
            if n and n not in need:
                need.append(n)
                todo.append(os.path.join(folder, n))
    return sorted(need, key=str.lower)


def derived_build(target, check, release):
    """Checks refs/publish/<target>/next out into a temporary folder and builds the game there with the trunk's generator
    and toolchain (docs/BUILDS.md). With check, plays one scripted game with that exe and with the trunk's, both under
    rundesk and on the main checkout's staged files, as tools/check.sh's rocketmania_state_game does: the state logs must
    be the same. With release, writes the release ZIP (release_zip)."""
    ref = f"refs/publish/{target}/next"
    git("rev-parse", "--verify", ref)
    common = os.path.dirname(os.path.abspath(git("rev-parse", "--git-common-dir").decode().strip()))  # the main checkout
    rundesk, trunk_exe, assets = (os.path.join(common, *p) for p in
                                  (("build", "native", "rundesk.exe"), ("build", "native", target + ".exe"), ("assets", target)))
    for f in ((rundesk, trunk_exe, assets) if check else ()):
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
        if check:
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
        if release:
            release_zip(target, ref, tree, build)
    finally:
        shutil.rmtree(tmp, ignore_errors=True)


def release_zip(target, ref, tree, build):
    """build/byo-pop-<target>-<the snapshot's date>.zip, for the release page: in one folder the exe, the DLLs of the build
    folder that it loads, README.txt (the snapshot's README.md up to its "## Building" heading, in Windows line ends) and
    the empty assets/<target>/ folder that the exe looks in for the player's own game files. No game file goes in."""
    date = git("log", "-1", "--format=%cs", ref).decode().strip()
    folder, path = f"byo-pop-{target}", os.path.join("build", f"byo-pop-{target}-{date}.zip")
    exe = os.path.join(build, target + ".exe")
    dlls = runtime_dlls(exe, build)
    with open(os.path.join(tree, "README.md"), encoding="utf-8") as f:
        readme = f.read()
    cut = re.search(r"^## Building", readme, re.M)
    readme = (readme[:cut.start()] if cut else readme).rstrip() + "\n"
    os.makedirs("build", exist_ok=True)
    with zipfile.ZipFile(path, "w", zipfile.ZIP_DEFLATED) as z:
        z.write(exe, f"{folder}/{target}.exe")
        for dll in dlls:
            z.write(os.path.join(build, dll), f"{folder}/{dll}")
        z.writestr(f"{folder}/README.txt", readme.replace("\n", "\r\n"))
        empty = zipfile.ZipInfo(f"{folder}/assets/{target}/", date_time=datetime.datetime.now().timetuple()[:6])
        empty.external_attr = (0o40755 << 16) | 0x10  # a folder, for Windows and for unzip
        z.writestr(empty, b"")
    print(f"release ZIP: {path} ({os.path.getsize(path) // 1024} KB): {target}.exe, {', '.join(dlls)}, README.txt, assets/{target}/")


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
    ap.add_argument("--release-zip", action="store_true",
                    help="with --target: build the last derived snapshot and write its release ZIP into build/")
    ap.add_argument("--out", help="write the snapshot's tree into this new folder instead of making a commit")
    a = ap.parse_args()
    if (a.build_check or a.release_zip) and not a.target:
        sys.exit("--build-check and --release-zip need --target")
    if not (a.build_check or a.release_zip or a.out) and not a.model:
        sys.exit("--model is needed")
    if a.out and (a.push or os.path.exists(a.out)):
        sys.exit("--out takes a folder that does not exist yet, and no --push")
    out = a.out and os.path.abspath(a.out)
    os.chdir(git("rev-parse", "--show-toplevel").decode().strip())
    if a.build_check or a.release_zip:
        return derived_build(a.target, a.build_check, a.release_zip)
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
            files[path] = (mode, new)
        for path in dropped:
            del files[path]
        # a derived repository's code: native only, without the site and the mod framework
        decided, reworded = derived_code(files, manifest, env, source) if manifest else ([], 0)

        # The checks: nothing private, no game files' folders
        left = git("grep", "--cached", "-n", "-I", "-E", "-e", MARKER, env=env, ok=(0, 1)).decode()
        if left:
            sys.exit("private markers left in the snapshot:\n" + left)
        published = [p for p in git("ls-files", "-z", env=env).decode().split("\0") if p]
        bad = [p for p in published if p.startswith(FORBIDDEN) or ignored(p, specs)]
        if bad:
            sys.exit("paths that must not be published:\n  " + "\n  ".join(bad))
        tree = git("write-tree", env=env).decode().strip()
        if out:
            git("checkout-index", "-a", f"--prefix={out}/", env=env)
            print(f"the tree of {source[:10]}'s snapshot is in {out}: {len(published)} files; nothing committed")
            print("  left out: " + (", ".join(dropped) or "nothing"))
            print("  private blocks stripped from: " + (", ".join(marked) or "nothing"))
            if manifest:
                print(f"  rewords applied: {reworded}; conditionals decided in {len(decided)} files")
            return

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
    if manifest:
        print(f"  rewords applied: {reworded}; conditionals decided in {len(decided)} files: " + (", ".join(decided) or "none"))
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
