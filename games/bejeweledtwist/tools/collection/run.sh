#!/usr/bin/env bash
# The port on the PopCap Game Collection's files (NOTES.md, "The PopCap Game Collection version"): the seeded games of
# tools/check.sh's bejeweledtwist suite (the menu, Classic to 5156, Blitz to its high scores, the 3D take-off), run on a
# staging of the Collection's BejeweledTwistComponent folder with its own properties/, and their screens compared with
# the suite's frozen hashes (BT_* in tools/check.sh). The Collection's partner.xml names the registry key
# PopCap\BejeweledTwist (Steam's: Steam\BejeweledTwist), so the test registry is written under that key.
#   build/native/rundesk.exe bash games/bejeweledtwist/tools/collection/run.sh <the Collection's BejeweledTwistComponent folder>
set -u
cd "$(dirname "$0")/../../../.."
W="$(pwd)"
SRC="${1:?the BejeweledTwistComponent folder of the Collection}"
G="build/bjt-coll/assets/bejeweledtwist" OUT="build/bjt-coll/out" EXE="$W/build/native/bejeweledtwist.exe"
python games/bejeweledtwist/tools/stage_bejeweledtwist_assets.py "$SRC" --own-properties --out "$G" > /dev/null || exit 1
eval "$(grep -oE '^BT_(CLASSIC|BLITZ|TAKEOFF|MENU)="[^"]*"' tools/check.sh)"
game() {  # <name> <autoplay mode> <last update> <updates...>: as check.sh's twist_game, on the Collection's files
	local d="$OUT/$1" mode="$2" last="$3" n
	shift 3
	rm -rf "$d"; mkdir -p "$d/shots" "$d/user"
	printf '[SOFTWARE\\PopCap\\BejeweledTwist]\nScreenMode=dword:0\nArtRes=dword:768\n' > "$d/user/registry.ini"
	{ for n in "$@"; do echo "$n screenshot $d/shots/$n.png"; done; echo "$last quit"; } > "$d/script.txt"
	timeout 300 "$EXE" -gamedir="$G" -userdir="$d/user" "${TWIST_MODE:--nod3d9}" -fast -seed=1234 -autoplay="$mode" \
		-script="$d/script.txt" > "$d/stdout.txt" 2> "$d/stderr.txt"
	for n in "$@"; do sha256sum "$d/shots/$n.png" 2>/dev/null | cut -c1-12; done | paste -sd" " -
}
st=0
expect() {  # <label> <got> <expected>
	if [ "$2" = "$3" ]; then echo "$1: as frozen"; else echo "$1: DIFFERS (got '$2', frozen '$3')"; st=1; fi
}
expect menu "$(game menu 1 170 60 160)" "$BT_MENU"
expect classic "$(game classic 1 5166 656 2156 3656 5156)" "$BT_CLASSIC"
expect blitz "$(game blitz blitz 8166 2656 5156 7156 7606 8156)" "$BT_BLITZ"
expect "3D take-off" "$(TWIST_MODE=-3d game takeoff 1 3021 2780 2900 2940 2980 3020)" "$BT_TAKEOFF"
grep -h "GameText" "$OUT"/*/stderr.txt "$OUT"/*/stdout.txt | sort | uniq -c
exit $st
