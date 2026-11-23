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
# Existing files are left alone, so it is safe to run again, except two sets that rerunning refreshes file by file
# (copied when the worktree lacks the file or main's copy is newer): re/bookwormadv/jp2ref and origrun's configs and
# patch lists (tools/origrun/*.cfg, tools/origrun/patches/*.txt). Prints one summary line.
#
# --refresh brings the baselines and staged assets that a worktree already has up to date with the main
# checkout, for the games the worktree does not own (the coordinator refreezes a baseline or restages a file
# later, and an older worktree would keep the stale copy, so check.sh --changed fails on a game it never
# touched). A game is owned when its folder games/<game> differs from the merge base with the local main branch
# (committed or not); an owned game's baseline or assets folder that this worktree lacks altogether is copied all the
# same, since nothing of the worktree's own is replaced (the game's first frozen baseline, made at its merge). Baselines (build/baseline* -> the game's folder; build/baseline and baseline-stage0 are
# Bejeweled's) and assets are compared by file size and modification time, from one listing per side (a file
# differs when the sizes differ or main's is newer: a plain copy gets the time of the copying). A trailing
# -web or other -suffix is dropped to find the game; an assets folder with no game folder is left alone. Files missing here are
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
TARGET="$(cd "$(cd "${TARGET:-.}" && git rev-parse --show-toplevel)" && pwd)"  # in the same spelling as MAIN below
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
	# One listing per side (path, size and time of every file under the baselines and assets), one awk: the files
	# main has that this worktree lacks, has at another size, or has older. Each line is "<folder>\t<file>".
	list_of() { (cd "$1" && find build/baseline build/baseline-* assets -mindepth 1 -type f -printf '%p\t%s\t%Ts\n' 2> /dev/null) || true; }
	differing="$(awk -F'\t' 'NR == FNR { size[$1] = $2; time[$1] = $3; next }
		!($1 in size) || size[$1] != $2 || time[$1] < $3 { split($1, c, "/"); i = length(c[1]) + length(c[2]) + 3; print c[1] "/" c[2] "\t" substr($1, i) }' \
		<(list_of "$TARGET") <(list_of "$MAIN"))"
	done_list=(); kept_list=()
	for rel in $(printf '%s\n' "$differing" | cut -f1 | sort -u); do
		name="${rel#*/}"; g="$(game_of "$name")"
		case "$rel" in
			build/*) label="$name" ;;
			*) label="assets/$name"; [ -n "$g" ] || continue ;;
		esac
		files="$(printf '%s\n' "$differing" | awk -F'\t' -v r="$rel" '$1 == r { print $2 }')"
		if [ -n "$g" ] && [ -e "$rel" ] && ! git diff --quiet "$base" -- "games/$g"; then
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

copy_newer() {  # copy_newer <relative dir>: file by file, when the worktree lacks the file or main's copy is newer
	[ -d "$MAIN/$1" ] || return 0
	local n=0 f p
	while IFS= read -r f; do
		p="${f#"$MAIN"/}"
		if [ ! -e "$TARGET/$p" ] || [ "$f" -nt "$TARGET/$p" ]; then
			mkdir -p "$(dirname "$TARGET/$p")"
			cp -p "$f" "$TARGET/$p"
			n=$((n + 1))
		fi
	done < <(find "$MAIN/$1" -type f)
	[ "$n" -gt 0 ] && copied+=("$1 ($n file$([ "$n" = 1 ] || echo s))")
	return 0
}

for g in "$MAIN"/assets/*/; do
	[ -d "$g" ] && copy_dir "assets/$(basename "$g")"
done
copy_newer re/bookwormadv/jp2ref
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
# origrun's per-game configs and patch lists (docs/ORIGRUN.md): git-ignored, each file copied when the worktree lacks it or main's is newer
n=0
for f in "$MAIN"/tools/origrun/*.cfg "$MAIN"/tools/origrun/patches/*.txt; do
	[ -f "$f" ] || continue
	p="${f#"$MAIN"/}"
	[ -e "$TARGET/$p" ] && ! [ "$f" -nt "$TARGET/$p" ] && continue
	mkdir -p "$(dirname "$TARGET/$p")"
	cp -p "$f" "$TARGET/$p"
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
