#!/bin/bash
# orig.sh <origrun script> <seed> <timeout s> <expr> [<expr> ...]: the original on a script, under rundesk, on origrun's
# virtual clock and scripted cursor, with up to four watched values (origrun --watch). The counter is the app's
# mEventTime (0x47be70), so each change is logged with the game time of the event handled last, as the port's -statelog
# does. The game's own values in the registry (its settings and best times) are removed first (KEEP_REG=1 keeps them),
# and it starts windowed, so every run starts the way a first run does. The log is on stdout.
source "$(dirname "$0")/common.sh"
S="$1"; SEED="$2"; T="$3"; shift 3
if [ -z "$KEEP_REG" ]; then
	for v in Layout Background Tileset Autohint "Sound Volume" "Music Volume" Fullscreen; do
		reg delete "$REGKEY" /v "$v" /f > /dev/null 2>&1
	done
	for ((l = 0; l < 18; l++)); do for ((r = 0; r < 8; r++)); do
		reg delete "$REGKEY" /v "HS:${l}x${r}" /f > /dev/null 2>&1
	done; done
fi
W=""; for e in "$@"; do W="$W --watch $e"; done
# --sound nodriver: with origrun's working sound stand-in the original stops reaching its waits once the title's music
# starts (after the first best times; IDENTICAL_PLAY.md), so it runs with no sound device, as it did before origrun's
# stand-in listed one; its "DirectSoundCreate failed" box is answered by origrun's hook, with no window and no sound
# the exe's own sound thread (it mixes the music) keeps origrun's guard on, so the main thread's waits would take
# their real time and a run would go slower than real time; it draws nothing from the generator and changes no state
ORIGRUN_CLOCK_GUARD=0 "$ORIGRUN" --exe "$(win "$OG_GAME/Ningpo.exe")" --run "$(basename "$OG_STAGE")" \
	--script "$(win "$S")" --clock virtual --cursor scripted --counter 0x47be70 --update-ms 1 --sound nodriver --seed "$SEED" \
	--patches tools/origrun/patches/ningpo.txt --timeout "$T" $W
