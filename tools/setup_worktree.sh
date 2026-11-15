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
# --refresh brings the baselines and staged assets that a worktree already has up to date with the main
# checkout, for the games the worktree does not own (the coordinator refreezes a baseline or restages a file
# later, and an older worktree would keep the stale copy, so check.sh --changed fails on a game it never
# touched). A game is owned when its folder games/<game> differs from the merge base with the local main branch
# (committed or not). Baselines (build/baseline* -> the game's folder; build/baseline and baseline-stage0 are
# Bejeweled's) are compared byte for byte; an assets folder, by file size and modification time (a trailing
# -web or other -suffix is dropped to find the game; no game folder, no refresh). Files missing here are
# copied too; nothing the worktree has and main lacks is ever deleted, and nothing is written to main.
# Prints one line, and nothing when nothing differs; in the main checkout it only says it does nothing.
# check.sh runs it at its start inside a worktree (with --dry-run it only reports).
#
#   tools/setup_worktree.sh [--no-cache] [<worktree>]     (default: the current directory's worktree)
#   tools/setup_worktree.sh --refresh [--dry-run] [<worktree>]
set -euo pipefail

CACHE=1
REFRESH=0
DRY=0
TARGET=""
while [ $# -gt 0 ]; do
	case "$1" in
		--no-cache) CACHE=0; shift ;;
		--refresh) REFRESH=1; shift ;;
		--dry-run) DRY=1; shift ;;
		-h|--help) sed -n '2,/^set -e/p' "$0"; exit 0 ;;
		*) TARGET="$1"; shift ;;
	esac
done
TARGET="$(cd "${TARGET:-.}" && git rev-parse --show-toplevel)"
# The main checkout: the folder that holds the shared .git directory
MAIN="$(cd "$TARGET" && cd "$(git rev-parse --git-common-dir)/.." && pwd)"
if [ "$MAIN" = "$TARGET" ]; then
	[ "$REFRESH" = 1 ] && { echo "setup_worktree.sh: --refresh: $TARGET is the main checkout itself; nothing to refresh"; exit 0; }
	echo "setup_worktree.sh: $TARGET is the main checkout itself; nothing to do"
	exit 0
fi
# A worktree cut from origin/main has a publish snapshot under it, with no history in common with main
if ! (cd "$TARGET" && git merge-base HEAD main > /dev/null 2>&1); then
	echo "setup_worktree.sh: WARNING: this branch shares no history with main (cut from a publish snapshot?)." \
		"It cannot be merged: git rebase --onto main <the snapshot commit> (CLAUDE.md, \"A new worktree\")" >&2
fi

if [ "$REFRESH" = 1 ]; then
	cd "$TARGET"
	base="$(git merge-base HEAD main 2> /dev/null)" || { echo "setup_worktree.sh: --refresh: no merge base with main; nothing refreshed" >&2; exit 0; }
	# game_of <name>: the game folder name for a baseline or assets name, empty when there is none
	game_of() {
		case "$1" in
			baseline|baseline-stage0) echo bejeweled ;;
			baseline-*) echo "${1#baseline-}" ;;
			*) if [ -d "games/$1" ]; then echo "$1"; elif [ "${1%-*}" != "$1" ] && [ -d "games/${1%-*}" ]; then echo "${1%-*}"; fi ;;
		esac
	}
	# differing <kind> <relative folder>: the files (relative to the folder) main has that this worktree lacks or has different
	differing() {
		if [ "$1" = baseline ]; then
			(cd "$MAIN/$2" && find . -type f | sed 's|^\./||') | while IFS= read -r f; do
				cmp -s "$MAIN/$2/$f" "$2/$f" 2> /dev/null || printf '%s\n' "$f"
			done
		else
			awk -F'\t' 'NR == FNR { have[$1] = $2 "\t" $3; next } !(($1 in have) && have[$1] == $2 "\t" $3) { print $1 }' \
				<(cd "$2" 2> /dev/null && find . -type f -printf '%P\t%s\t%Ts\n' || true) \
				<(cd "$MAIN/$2" && find . -type f -printf '%P\t%s\t%Ts\n')
		fi
	}
	done_list=(); kept_list=()
	for src in "$MAIN"/build/baseline "$MAIN"/build/baseline-*/ "$MAIN"/assets/*/; do
		src="${src%/}"; [ -d "$src" ] || continue
		rel="${src#"$MAIN"/}"; name="$(basename "$rel")"
		g="$(game_of "$name")"
		case "$rel" in
			build/*) kind=baseline; label="$name" ;;
			*) kind=assets; label="assets/$name"; [ -n "$g" ] || continue ;;
		esac
		files="$(differing "$kind" "$rel")"
		[ -n "$files" ] || continue
		if [ -n "$g" ] && ! git diff --quiet "$base" -- "games/$g"; then
			case " ${kept_list[*]:-} " in *" $g "*) ;; *) kept_list+=("$g") ;; esac
			continue
		fi
		n=0
		while IFS= read -r f; do
			if [ "$DRY" = 0 ]; then mkdir -p "$(dirname "$rel/$f")"; cp -p "$MAIN/$rel/$f" "$rel/$f"; fi
			n=$((n + 1))
		done <<< "$files"
		done_list+=("$label ($n file$([ "$n" = 1 ] || echo s))")
	done
	line=""
	if [ ${#done_list[@]} -gt 0 ]; then
		line="$([ "$DRY" = 1 ] && echo "would refresh" || echo refreshed) from main: $(IFS=,; echo "${done_list[*]}" | sed 's/,/, /g')"
	fi
	if [ ${#kept_list[@]} -gt 0 ]; then
		line="${line:+$line; }kept (owned): $(IFS=,; echo "${kept_list[*]}" | sed 's/,/, /g')"
	fi
	[ -z "$line" ] || echo "setup_worktree.sh: $line"
	exit 0
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
# origrun's per-game configs and patch lists (docs/ORIGRUN.md): git-ignored, each file copied only when the worktree lacks it
n=0
for f in "$MAIN"/tools/origrun/*.cfg "$MAIN"/tools/origrun/patches/*.txt; do
	[ -f "$f" ] || continue
	p="${f#"$MAIN"/}"
	[ -e "$TARGET/$p" ] && continue
	mkdir -p "$(dirname "$TARGET/$p")"
	cp "$f" "$TARGET/$p"
	n=$((n + 1))
done
[ "$n" -gt 0 ] && copied+=("$n origrun configs and patch lists")
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
