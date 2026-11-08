#!/usr/bin/env bash
# Give a fresh git worktree the git-ignored files the builds and tests need, copied from the main checkout:
#   - assets/<game> for every game staged there (copies, never junctions: deleting a worktree then cannot
#     reach the main checkout's files);
#   - re/bookwormadv/jp2ref (the reference PNGs of jp2test in the unit suite; tools/jp2test/write_refs.sh remakes them);
#   - build/baseline and build/baseline-stage0 (Bejeweled's frozen oracle exes), every game's frozen
#     build/baseline-<game> (check.sh compares against them) and build/oracle-cache;
#   - re/<game>/bin (the original exes some tools read, e.g. Mummy Maze's make_res.py) and re/<game>/mac
#     (the Mac builds and their symbol lists a new decomp starts from, CLAUDE.md "Mac versions");
#   - the generated, git-ignored files under games/*/mods (e.g. Bejeweled's bejeweled2-audio, which the
#     oracle's "mods" case expects).
# Existing files are left alone, so it is safe to run again. Prints one summary line.
#
#   tools/setup_worktree.sh [--no-cache] [<worktree>]     (default: the current directory's worktree)
set -euo pipefail

CACHE=1
TARGET=""
while [ $# -gt 0 ]; do
	case "$1" in
		--no-cache) CACHE=0; shift ;;
		-h|--help) sed -n '2,14p' "$0"; exit 0 ;;
		*) TARGET="$1"; shift ;;
	esac
done
TARGET="$(cd "${TARGET:-.}" && git rev-parse --show-toplevel)"
# The main checkout: the folder that holds the shared .git directory
MAIN="$(cd "$TARGET" && cd "$(git rev-parse --git-common-dir)/.." && pwd)"
if [ "$MAIN" = "$TARGET" ]; then
	echo "setup_worktree.sh: $TARGET is the main checkout itself; nothing to do"
	exit 0
fi
# A worktree cut from origin/main has a publish snapshot under it, with no history in common with main
if ! (cd "$TARGET" && git merge-base HEAD main > /dev/null 2>&1); then
	echo "setup_worktree.sh: WARNING: this branch shares no history with main (cut from a publish snapshot?)." \
		"It cannot be merged: git rebase --onto main <the snapshot commit> (CLAUDE.md, \"A new worktree\")" >&2
fi

copied=()
copy_dir() {  # copy_dir <relative path>: only when the source has it and the target does not
	if [ -d "$MAIN/$1" ] && [ ! -e "$TARGET/$1" ]; then
		mkdir -p "$(dirname "$TARGET/$1")"
		cp -r "$MAIN/$1" "$TARGET/$1"
		copied+=("$1")
	fi
}

for g in "$MAIN"/assets/*/; do
	[ -d "$g" ] && copy_dir "assets/$(basename "$g")"
done
copy_dir re/bookwormadv/jp2ref
copy_dir build/baseline
copy_dir build/baseline-stage0
for b in "$MAIN"/build/baseline-*/; do
	[ -d "$b" ] && copy_dir "build/$(basename "$b")"
done
for d in "$MAIN"/re/*/bin/ "$MAIN"/re/*/mac/ "$MAIN"/re/*/*/mac/; do
	p="${d#"$MAIN"/}"
	[ -d "$d" ] && copy_dir "${p%/}"
done
[ "$CACHE" = 1 ] && copy_dir build/oracle-cache
# the site's test key (docs/WEB_PLAYER.md, "The terms"): the web build reads it, agents read it for their test URLs
copy_dir secrets

# Generated mod files: what git ignores under games/*/mods in the main checkout
n=0
while IFS= read -r path; do
	path="${path%/}"
	[ -e "$TARGET/$path" ] && continue
	mkdir -p "$(dirname "$TARGET/$path")"
	cp -r "$MAIN/$path" "$TARGET/$path"
	n=$((n + 1))
done < <(cd "$MAIN" && git status --short --ignored -- games | sed -n 's/^!! //p' | grep '^games/[^/]*/mods/')
[ "$n" -gt 0 ] && copied+=("$n generated mod paths")

if [ ${#copied[@]} -eq 0 ]; then
	echo "setup_worktree.sh: nothing to copy (all present) into $TARGET"
else
	echo "setup_worktree.sh: copied into $TARGET: $(IFS=,; echo "${copied[*]}" | sed 's/,/, /g')"
fi
