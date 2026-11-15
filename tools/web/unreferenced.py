#!/usr/bin/env python3
"""Files of a game that nothing names, for leaving them out of the browser's stored copy (the user, 2026-11-13: "#2 should
look for files never referenced in code though, since it is ~impossible to guarantee full-play test runs reach every
screen"). A static check: a file the import stores (its page's gameDirs, web/gameimport.js) counts as referenced when its
name appears anywhere a game or the site could read it:

  - the strings of the game's own files as staged (assets/<game>/: its exe and DLLs, resource lists, data, scripts, the
    members of a zip such as Ning Po's .pyz), media files apart, and the unwrapped exe of the original's export
    (re/<game>/bin/), whose strings a launcher-wrapped exe hides;
  - the port's source (games/<game>/src, web, mods, CMakeLists.txt) and every shared source (framework/, platform/,
    engines/);
  - the site's own tables (web/*.js: the soundboards name unused sounds the Sounds window plays, Music its tunes, the
    desktop's themes their pictures and sounds).

A name is the file's stem, any case, its alpha-mask underscores stripped ("_gem1_" is gem1); one that ends in digits
counts also as its stem without them, for a name the code builds from a number ("gem%d"), and the extension never counts,
as the framework tries every picture format. A stem of one word must appear as a whole word; a stem with spaces or dashes
as a substring. Whatever the check is unsure of stays: a file is listed only when no source names it at all.

The list is one to check by hand, never one to drop: a name a game builds in a way no string shows (a level's name with
"_bg" after it, too short a format to count; a stem from a table of numbers) leaves its files here though the game opens
them. The first run (2026-11-16, the user's copies) found 20.5 MB of 381 named by nothing; 15.4 MB of it is AstroPop's and
Dynomite's sounds/cached_*.wav, the 2004 framework's decoded copies of the Ogg sounds, which those two ports never ask for
and which web/gameimport.js's NEVER_OPENED leaves out. The other 5 MB is many small lists (alpha-mask leftovers, old fonts,
Bookworm Adventures 2's cutscenes and Heavy Weapon's backgrounds among them, both reached through built names), left stored.

  python tools/web/unreferenced.py [--assets DIR] [--re DIR] [--games a,b] [--out build/unreferenced]

Writes <out>/unreferenced.json ({game: {stored, files, unref: [[path, bytes], ...]}}) and a summary per game.
"""
import argparse
import json
import os
import re
import sys
import zipfile

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
MEDIA = {".ogg", ".mp3", ".wav", ".au", ".jpg", ".jpeg", ".png", ".gif", ".jp2", ".j2k", ".bmp", ".tga", ".mo3", ".it",
         ".xm", ".mod", ".s3m", ".ttf", ".otf", ".wma", ".avi", ".mpg", ".ogv", ".ico", ".cur", ".ani", ".dds", ".p3d",
         ".pcx", ".psd", ".tif", ".tiff"}
SRC_EXT = {".cpp", ".cc", ".c", ".h", ".hpp", ".inc", ".js", ".json", ".txt", ".cmake", ".py", ".xml", ".html"}
README = re.compile(r"^read ?me\.(html?|txt)$", re.I)
ASCII = re.compile(rb"[\x20-\x7e]{3,}")
UTF16 = re.compile(rb"(?:[\x20-\x7e]\x00){3,}")
WORD = re.compile(r"[a-z0-9_]+")


def strings(data):
    out = [m.group(0).decode("latin-1") for m in ASCII.finditer(data)]
    out += [m.group(0).decode("utf-16-le", "ignore") for m in UTF16.finditer(data)]
    return "\n".join(out)


def page_spec(game):
    """gameDirs and gameFiles from games/<game>/web/page.js"""
    p = os.path.join(ROOT, "games", game, "web", "page.js")
    if not os.path.exists(p):
        return None
    s = open(p, encoding="utf-8").read()
    def field(name):
        m = re.search(name + r":\s*\[(.*?)\]", s, re.S)
        return [x.lower() for x in re.findall(r'"([^"]+)"', m.group(1))] if m else []
    return {"dirs": field("gameDirs"), "files": field("gameFiles")}


def stored(game, assets, spec):
    """the files the import stores, as (rel lower, full path, bytes), and which of them are in the game's folders"""
    base = os.path.join(assets, game)
    out = []
    for d, _, fs in os.walk(base):
        for f in fs:
            full = os.path.join(d, f)
            rel = os.path.relpath(full, base).replace("\\", "/")
            low = rel.lower()
            top = low.split("/")[0]
            in_dir = "/" in low and top in spec["dirs"]
            if in_dir or low in spec["files"] or README.match(low) or (spec["files"] and "/" not in low and low.endswith(".exe")):
                out.append((low, full, os.path.getsize(full), in_dir))
    return out


def text_of(path):
    ext = os.path.splitext(path)[1].lower()
    data = open(path, "rb").read()
    if zipfile.is_zipfile(path):
        parts = []
        try:
            with zipfile.ZipFile(path) as z:
                for n in z.namelist():
                    parts.append(n)
                    if os.path.splitext(n)[1].lower() not in MEDIA:
                        parts.append(strings(z.read(n)))
        except (zipfile.BadZipFile, OSError, NotImplementedError):
            parts.append(strings(data))
        return "\n".join(parts)
    if ext in MEDIA:
        return ""
    return strings(data)


def source_text(folders, skip_dirs=()):
    parts = []
    for top in folders:
        if not os.path.exists(top):
            continue
        if os.path.isfile(top):
            parts.append(open(top, encoding="utf-8", errors="ignore").read())
            continue
        for d, ds, fs in os.walk(top):
            ds[:] = [x for x in ds if x not in skip_dirs and not x.startswith(".")]
            for f in fs:
                if os.path.splitext(f)[1].lower() in SRC_EXT or f == "CMakeLists.txt":
                    parts.append(open(os.path.join(d, f), encoding="utf-8", errors="ignore").read())
    return "\n".join(parts)


SHARED = None


def shared_text():
    global SHARED
    if SHARED is None:
        SHARED = source_text([os.path.join(ROOT, x) for x in ("framework", "platform", "engines", "web")],
                             skip_dirs=("third_party", "fonts", "licenses", "brand", "jp2", "music", "counter"))
    return SHARED


FMT = re.compile(r"[^\s\"'<>|=,;()]*%[-+ 0#]*\d*(?:\.\d+)?(?:l|ll|h|I64)?[diouxXsSc][^\s\"'<>|=,;()]*")
SPEC = re.compile(r"%[-+ 0#]*(\d*)(?:\.\d+)?(?:l|ll|h|I64)?([diouxXsSc])")


def patterns(corpus):
    """the names the code builds: each printf-style format in the strings ("story_%d%c.jpg", "animations/mask_%s",
    "book%dintro") as a pattern for a path, one that holds at least three letters of its own (a bare "%s.ogg" says nothing
    about which file: its %s is a name from the data, which the names themselves cover)"""
    out = {}
    # the site's own templates (web/soundtracks.js: "music/tubetricks menu one v02 {}.ogg", a playlist's word for the {}) as
    # a %s
    found = [m.group(0) for m in FMT.finditer(corpus)]
    found += [m.group(1).replace("{}", "%s") for m in re.finditer(r'"([^"\n]*\{\}[^"\n]*)"', corpus)]
    for f in found:
        f = f.replace("\\\\", "/").replace("\\", "/")
        lit = SPEC.sub("", f)
        if len(re.sub(r"\.[a-z0-9]{2,4}$", "", re.sub(r"[^a-z]", "", lit))) < 3 and \
                len(re.sub(r"[^a-z]", "", re.sub(r"\.[a-z0-9]{2,4}$", "", lit))) < 3:
            continue
        parts, at = [], 0
        for s in SPEC.finditer(f):
            parts.append(re.escape(f[at:s.start()]))
            w, k = s.group(1), s.group(2)
            if k in "diouxX":
                parts.append(r"\d{%d,}" % int(w) if w else (r"[0-9a-f]+" if k in "xX" else r"\d+"))
            elif k == "c":
                parts.append(".")
            else:
                parts.append(".+")
            at = s.end()
        parts.append(re.escape(f[at:]))
        rx = "".join(parts)
        # a format may stop short of the extension, or lead with a folder: it is matched at the end of the path, without
        # the extension
        out[rx] = True
    return [re.compile(r"(^|/)" + rx.lstrip("/") + r"(\.[a-z0-9]+)?$") for rx in out]


def keys(rel):
    stem = os.path.splitext(os.path.basename(rel))[0].strip("_").lower()
    out = [stem] if stem else []
    m = re.match(r"^(.*?)[ _-]*\d+$", stem)
    if m and len(m.group(1)) >= 2:
        out.append(m.group(1))
    return out


def check(game, assets, redir):
    spec = page_spec(game)
    if spec is None:
        return None
    files = stored(game, assets, spec)
    parts = [text_of(full) for low, full, size, in_dir in files]
    # the names of the stored files themselves are no reference; a resource list or a pak's index is a file of its own
    rb = os.path.join(redir, game, "bin")
    if os.path.isdir(rb):
        for f in os.listdir(rb):
            parts.append(strings(open(os.path.join(rb, f), "rb").read()))
    g = os.path.join(ROOT, "games", game)
    parts.append(source_text([os.path.join(g, x) for x in ("src", "web", "mods", "engine", "CMakeLists.txt")]))
    parts.append(shared_text())
    corpus = "\n".join(parts).lower()
    words = set(WORD.findall(corpus))
    pats = patterns(corpus)
    unref = []
    for low, full, size, in_dir in files:
        if not in_dir:
            continue
        found = False
        for k in keys(low):
            if (WORD.fullmatch(k) and k in words) or (not WORD.fullmatch(k) and k in corpus):
                found = True
                break
        if not found:
            bare = low.replace("/_", "/").replace("_.", ".")
            found = any(p.search(low) or p.search(bare) for p in pats)
        if not found:
            unref.append([low, size])
    return {"stored": sum(s for _, _, s, _ in files), "files": len(files), "unref": sorted(unref)}


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--assets", default=os.path.join(ROOT, "assets"))
    ap.add_argument("--re", default=None, help="the Ghidra exports (default: re/ here, else the main checkout's)")
    ap.add_argument("--games", default="")
    ap.add_argument("--out", default=os.path.join(ROOT, "build", "unreferenced"))
    a = ap.parse_args()
    redir = a.re or next((p for p in (os.path.join(ROOT, "re"), os.path.join(ROOT, "..", "..", "..", "re"))
                          if os.path.isdir(p)), os.path.join(ROOT, "re"))
    games = a.games.split(",") if a.games else sorted(os.listdir(a.assets))
    os.makedirs(a.out, exist_ok=True)
    res = {}
    total = gone = 0
    for game in games:
        r = check(game, a.assets, redir)
        if r is None:
            continue
        res[game] = r
        b = sum(s for _, s in r["unref"])
        total += r["stored"]
        gone += b
        print("%-16s %4d of %5d files, %6.2f of %7.2f MB" % (game, len(r["unref"]), r["files"], b / 1e6, r["stored"] / 1e6),
              flush=True)
    json.dump(res, open(os.path.join(a.out, "unreferenced.json"), "w", encoding="utf-8"), indent=1)
    print("all: %.2f of %.2f MB named by nothing" % (gone / 1e6, total / 1e6))


if __name__ == "__main__":
    sys.exit(main())
