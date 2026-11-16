#!/bin/bash
# Stages the original for the identical-play runs: a copy of the installed game under a short path, with the game
# carved out of PizzaFrenzy.exe next to it, and the patch list. Nothing here runs the game.
#
# The patches (all on the carved exe, none on the wrapper, which is not run):
# - seed: the configure (0x4050d0) seeds rand with time(NULL) (the call at 0x405206); the run's seed takes its place,
#   as the port's -seed=N does.
# - The frame (0x4bd210) takes its elapsed seconds from the clock (the tick difference over the rate, clamped to 0.33).
#   On origrun's virtual clock the time read varies around the period, so the frame's seconds are made the period, the
#   port's -fast frame: 33333 microseconds, the float 0x3d08882f. The same stretch counts the frames in a dword past
#   .data's virtual size (0x541a00, in its last page), the update counter for origrun's "at" and "--watch".
# - The C library is the multithreaded one: rand's state is in the thread's data, which origrun cannot watch. srand
#   (0x4ad1c4) and rand (0x4ad1d1) also store it at 0x541a04, from code in .text's zero tail past its virtual size,
#   which the header's VirtualSize is made to cover.
# - Pictures (shots.sh, a second list with the first's patches and one more): the screen is drawn into a system-memory
#   back surface, partly through its lock (0x4ccee0, which maps a dirty rectangle at the display's +0x30) and partly
#   by DirectDraw blits while it is unlocked, and present (0x4ccb20) blits it to the window. A window capture shows
#   nothing (and its paint while held draws through the cleared pointer and crashes), and the pixels cannot be read
#   unlocked. So present, after its own unlock, locks the whole surface (the display's slot 8, 0x4ccce0), copies it
#   to a buffer of its own (VirtualAlloc'd once: its address at 0x541a08, the pitch in bytes at 0x541a0c, 0x541a10
#   set while it copies) and unlocks it again; origrun reads the last frame shown from the buffer (--surface-screen,
#   --surface-gate). Code in the same tail as the mirror's.
source "$(dirname "$0")/common.sh"
[ -f "$GAME_EXE" ] || { echo "$GAME_EXE is missing (tools/extract_game_exe.py)"; exit 1; }
[ -f "$PF_INSTALL/PizzaFrenzy.saf" ] || { echo "no game at $PF_INSTALL"; exit 1; }
rm -rf "$OG_GAME"; mkdir -p "$OG_GAME" || exit 1
cp -r "$PF_INSTALL/." "$OG_GAME/" || exit 1
cp "$GAME_EXE" "$OG_GAME/PizzaFrenzyGame.exe" || exit 1
mkdir -p "$(dirname "$PATCHES")"
cat > "$PATCHES" <<'P'
seed PizzaFrenzyGame.exe va:0x405206 e8e87f0a00 b8{seed} srand(time(NULL)) in the configure takes the run's seed
test PizzaFrenzyGame.exe va:0x4bd29e d9542430d81dfca85000dfe0f6c4417508c7442430c3f5a83e d95c2430c74424302f88083dff05001a540090909090909090 the frame's seconds are the period (33333 us), and the frame counter at 0x541a00
test PizzaFrenzyGame.exe off:0x208 4e7d0f00 007e0f00 .text's VirtualSize covers its file tail (the rand mirror's code)
test PizzaFrenzyGame.exe off:0xf8150 0000000000000000000000000000000000000000000000000000000000000000 894814890d041a54008bc1e98a44fbff8b4c2404894814890d041a5400c30000 the mirror's code: rand's and srand's state also at 0x541a04
test PizzaFrenzyGame.exe va:0x4ad1e5 8948148bc1 e966bb0400 rand jumps to the mirror's code
test PizzaFrenzyGame.exe va:0x4ad1c9 8b4c2404894814c3 e992bb0400909090 srand jumps to the mirror's code
P
SHOT_PATCHES="${PATCHES%.txt}-shots.txt"
{ cat "$PATCHES"; cat <<'P'
test PizzaFrenzyGame.exe off:0xf8170 0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000 89ce8b0657ff500c8b0689f1ff5008837e30007456608b3d081a540085ff751e6a046800300000680000200057ff1520914f00a3081a540089c785c0742c8b4664a30c1a54000faf46283d000020007719c1e80289c18b7630c605101a540001fcf3a5c605101a540000618b0689f1ff500ce9473dfdff present's copy of the screen: the whole back surface locked, copied to a buffer at [0x541a08] (pitch 0x541a0c; 0x541a10 is 1 while it copies) and unlocked
test PizzaFrenzyGame.exe va:0x4ccb26 8bf18b0657ff500c e945c20200909090 present jumps to that code after its unlock
P
} > "$SHOT_PATCHES"
echo "staged in $OG_GAME"
