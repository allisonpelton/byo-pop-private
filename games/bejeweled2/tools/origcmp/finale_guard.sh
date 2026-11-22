#!/bin/bash
# The frozen guard of the screens the screen-coverage pass fixed (docs/IDENTICAL_PLAY.md, "Screen coverage pass"): the port's recording of
# the Puzzle finale (the warp, the credits roll, GALAXY COMPLETED, the MODE UNLOCKED message with its OK button, the Back to Main Menu
# link and the board after it, 2D 640x480, 40 shots) and one hash over every shot. tools/check.sh's bejeweled2 suite compares it with
# the frozen value.
#   finale_guard.sh <exe> <work dir>      prints "<12 hex>" on its last line; exit 1 when the recording failed
exe=$1; work=$2
cd "$(dirname "$0")/../../../.." || exit 2
rm -rf "$work"; mkdir -p "$work"
# the player: make_profile.py's Ali, on the galaxy's last planet with Cognito not yet unlocked (make_gapprofile.py --puzzle 75 --galaxy 0)
python -I games/bejeweled2/tools/origcmp/make_profile.py --exe "$exe" --out "$work/base" > "$work/profile.log" 2>&1 || { echo "profile failed"; exit 1; }
python -I games/bejeweled2/tools/origcmp/make_gapprofile.py --base "$work/base" --out "$work/prof" --puzzle 75 --galaxy 0 > /dev/null || { echo "profile failed"; exit 1; }
mkdir -p "$work/rec.user" && cp -r "$work/prof/users" "$work/rec.user/users"
env TOUR_DOME=1 TOUR_WORLDS=1 TOUR_FINALE=1 TOUR_WARP_STEP=100 TOUR_WARP_SHOTS=8 \
  python -I games/bejeweled2/tools/origcmp/record.py --exe "$exe" --userdir "$work/rec.user" --out "$work/rec" --mode tour:puzzle --moves 1 \
  --seed 909 --play-at 500 --lowres --drawevery > "$work/rec.log" 2>&1
n=$(ls "$work/rec"/*.png 2>/dev/null | wc -l)
[ "$n" -ge 40 ] || { echo "only $n shots"; exit 1; }
(cd "$work/rec" && sha256sum *.png | sed 's/ .*\*\?//' | sort | sha256sum | cut -c1-12)
