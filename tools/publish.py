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

A private block is a run of lines between two marker lines, each starting at its line's first column:

    <!-- private: The line the published copy shows instead. -->
    ...
    <!-- /private -->

The opening marker may be a plain <!-- private --> (the block is dropped). Blocks do not nest.
"""
import argparse, datetime, fnmatch, os, re, subprocess, sys, tempfile

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


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--push", action="store_true")
    ap.add_argument("--source", default="main")
    ap.add_argument("--remote", default="origin")
    ap.add_argument("-m", dest="message")
    ap.add_argument("--model", required=True, help='the model making the snapshot, for its trailer, e.g. "Opus 5.5"')
    a = ap.parse_args()
    os.chdir(git("rev-parse", "--show-toplevel").decode().strip())
    source = git("rev-parse", "--verify", a.source + "^{commit}").decode().strip()
    specs = [s.strip() for s in git("show", f"{source}:.publishignore").decode().splitlines()
             if s.strip() and not s.lstrip().startswith("#")]

    with tempfile.TemporaryDirectory() as tmp:
        env = dict(os.environ, GIT_INDEX_FILE=os.path.join(tmp, "index"))
        git("read-tree", source, env=env)
        files = {}  # path -> (mode, blob)
        for entry in git("ls-files", "-s", "-z", env=env).decode().split("\0"):
            if entry:
                info, path = entry.split("\t", 1)
                mode, blob, _ = info.split()
                files[path] = (mode, blob)

        dropped = sorted(p for p in files if ignored(p, specs))
        git("update-index", "--force-remove", "-z", "--stdin", env=env, data="".join(p + "\0" for p in dropped).encode())
        unmatched = [s for s in specs if not any(ignored(p, [s]) for p in dropped)]

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

    parent = git("rev-parse", "--verify", "-q", LAST, ok=(0, 1)).decode().strip()
    if parent and git("rev-parse", parent + "^{tree}").decode().strip() == tree:
        print(f"nothing new: {LAST} ({parent[:10]}) already has this tree")
        return
    message = (a.message or f"byo-pop snapshot, {datetime.date.today().isoformat()}") + \
        f"\n\nPublished by tools/publish.py from {source[:10]}.\n\n{TRAILER.format(model=a.model)}\n"
    commit = git("commit-tree", tree, *(["-p", parent] if parent else []), "-F", "-", data=message.encode()).decode().strip()
    git("update-ref", NEXT, commit)
    print(f"snapshot {commit[:10]} of {source[:10]} in {NEXT}: {len(published)} files, parent "
          f"{parent[:10] if parent else '(none: the first)'}")
    print("  left out: " + (", ".join(dropped) or "nothing"))
    print("  private blocks stripped from: " + (", ".join(marked) or "nothing"))
    if unmatched:
        print("  .publishignore lines that match nothing: " + ", ".join(unmatched))
    if a.push:
        if subprocess.run(["git", "push", a.remote, f"{commit}:refs/heads/main"]).returncode:
            sys.exit(f"the push failed; {LAST} is unchanged")
        git("update-ref", LAST, commit)
        print(f"pushed to {a.remote} main; {LAST} is {commit[:10]}")


if __name__ == "__main__":
    main()
