#!/bin/bash
# run.sh <name> <origrun script> [timeout s] [more origrun options]: one coverage run of Ancient Japan's original
# (docs/COVERAGE.md) on the host, under origrun's own rundesk, with --coverage on the list that
# `python -I tools/coverage.py list mahjongjapan` writes. GAME=mahjongchina runs Ancient China's. The copy is staged once
# under C:\byo-og\<game>cov (a short path outside the repository); origrun makes room for the recorder's import itself
# (the exe keeps a copyright text where it wants a free section header; it moves the headers up over the DOS stub). The game's saves are removed
# before the run, so each run is a first run (PLAYER starts it from a player instead) (the user's own, if any, are in build/coverage/<game>/saves-backup; the
# host's registry already holds FullScreen=0 and HardwareAcceleration=0 for the game, so it starts windowed in software).
# The hits go to build/coverage/<game>/hits/<name>.txt and the run's log beside them.
cd "$(git rev-parse --show-toplevel)" || exit 1
export MSYS_NO_PATHCONV=1
GAME="${GAME:-mahjongjapan}"
case "$GAME" in
	mahjongjapan) EXE=MahjongEscapeAJ.exe; SAVES='Mahjong Escape Ancient Japan' ;;
	mahjongchina) EXE=MahjongEscapeAC.exe; SAVES='Mahjong Escape' ;;
esac
NAME="$1"; S="$2"; T="${3:-120}"; shift 3 2> /dev/null || shift $#
H="build/coverage/$GAME/hits"; mkdir -p "$H"
ST="${GAME}cov"; SD="C:/byo-og/$ST"
until python -I tools/gamecount.py --limit 6 > /dev/null; do sleep 15; done
if [ ! -f "$SD/game/$EXE" ]; then
	build/origrun-bin/origrun.exe --game "$GAME" --run "$ST" --stage-root 'C:\byo-og' --copy-only > "$H/stage.log" 2>&1
fi
rm -f "$LOCALAPPDATA/JollyBear/$SAVES"/*.usr "$LOCALAPPDATA/JollyBear/$SAVES"/*.sav
# PLAYER=<folder with list.usr, 0.usr and any saved games> starts the run from that player; KEEP=<folder> receives the saves after the run (resign.py makes one at a later stage)
[ -n "$PLAYER" ] && cp "$PLAYER"/* "$LOCALAPPDATA/JollyBear/$SAVES/"
rm -f "build/origrun/$ST/coverage.txt"
build/origrun-bin/origrun.exe --exe "$(cygpath -w "$SD/game/$EXE")" --run "$ST" \
	--script "$(cygpath -w "$S")" --timeout "$T" \
	--coverage "$(cygpath -w "$PWD/build/coverage/$GAME/functions.txt")" "$@" > "$H/$NAME.log" 2>&1
echo "exit=$?"
if [ -n "$KEEP" ]; then mkdir -p "$KEEP"; cp "$LOCALAPPDATA/JollyBear/$SAVES"/* "$KEEP/"; fi
cp "build/origrun/$ST/coverage.txt" "$H/$NAME.txt" || { echo "$NAME: no hits file"; exit 1; }
echo "$NAME: $(grep -vc '^#' "$H/$NAME.txt") functions; $(grep '^#' "$H/$NAME.txt" | tail -1)"
grep -E "RESULT|SOUND LEAK" "$H/$NAME.log" | cut -c1-200
