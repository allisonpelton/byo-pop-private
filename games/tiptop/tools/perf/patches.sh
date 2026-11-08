#!/bin/bash
# patches.sh: the patch list of the performance runs of the original (perf.py orig): the comparison setup's list
# (tools/origrun/patches/tiptop-music.txt, written by games/tiptop/tools/identical/setup.sh, git-ignored) without what changes the
# original's timing or only serves the identical-play comparison (the main loop's wait and counter, the clock, the cave that
# copies rand's state, the keyboard, PlaySound and Blit patches): the original then plays as the registered game, with the
# seed pinned and the MIDI music left out, and runs on its own timing. Writes build/tt/perf/tiptop-perf.txt.
cd "$(git rev-parse --show-toplevel)" || exit 1
SRC=tools/origrun/patches/tiptop-music.txt
[ -f "$SRC" ] || { echo "no $SRC: run 'OG_STAGE=C:/byo-og/ttperf games/tiptop/tools/identical/setup.sh' first"; exit 1; }
mkdir -p build/tt/perf
grep -v -e 'vertical blank' -e 'counts its passes' -e 'PerfFrequency' -e 'PerfCounter' -e 'rand goes through' -e 'the cave' \
	-e 'keyboard' -e 'PlaySound' -e 'Blit keeps' "$SRC" > build/tt/perf/tiptop-perf.txt
echo "$(wc -l < build/tt/perf/tiptop-perf.txt) patches in build/tt/perf/tiptop-perf.txt"
