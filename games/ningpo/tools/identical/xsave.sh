#!/bin/bash
# xsave.sh: the best times and settings (all Ning Po saves) written by one program and continued by the other.
#   1. Both play won39 from fresh settings; their saved values must be the same, byte for byte.
#   2. The port plays cheat7 on the original's values, and the original plays cheat7 on the port's; the values each
#      saves then must be the same again (cheat7's 40 tiles in 12 s goes between won39's two entries).
# The original's values are read from and written to its registry key (reg.exe), the port's from its registry.ini.
# Files go to $OUT/xsave/; the last line says whether both steps were the same.
source "$(dirname "$0")/common.sh"
D="$OUT/xsave"; rm -rf "$D"; mkdir -p "$D"
S=build/identical/scripts
for c in won39 cheat7; do [ -f "$S/$c.txt" ] || "$TOOLS/cases.sh" $c "$S/$c.txt"; done
python "$TOOLS/toorig.py" "$S/won39.txt" "$D/won39-orig.txt"
python "$TOOLS/toorig.py" "$S/cheat7.txt" "$D/cheat7-orig.txt"

# the game's own values only (the framework's CustomCursors is the port's, the URLs and Cap the installer's)
orig_values() { reg query "$REGKEY" | python "$TOOLS/regvalues.py" reg > "$1"; }
port_values() { python "$TOOLS/regvalues.py" ini < "$2/registry.ini" > "$1"; }

# 1
"$TOOLS/orig.sh" "$D/won39-orig.txt" 39 240 0x47be4c > "$D/orig-won39.txt" 2>&1
orig_values "$D/orig-won39.values"
"$TOOLS/port.sh" "$S/won39.txt" 39 "$D/port-won39/port.log"
port_values "$D/port-won39.values" "$D/port-won39/user"
diff "$D/orig-won39.values" "$D/port-won39.values" > "$D/won39.diff" && one=same || one=DIFFERS
echo "won39's saved values: $one"

# 2: the original continues the port's values (the key cleared of its own first), the port the original's
for name in $(cut -d= -f1 "$D/orig-won39.values" | grep -v " "); do reg delete "$REGKEY" /v "$name" /f > /dev/null; done
reg delete "$REGKEY" /v "Music Volume" /f > /dev/null; reg delete "$REGKEY" /v "Sound Volume" /f > /dev/null
python "$TOOLS/regvalues.py" toreg < "$D/port-won39.values" | tr -d "\r" | while IFS=$'\t' read -r name data; do
	reg add "$REGKEY" /v "$name" /t REG_BINARY /d "$data" /f > /dev/null || echo "xsave: reg add $name failed"
done
KEEP_REG=1 "$TOOLS/orig.sh" "$D/cheat7-orig.txt" 7 240 0x47be4c > "$D/orig-cheat7.txt" 2>&1
orig_values "$D/orig-cheat7.values"
mkdir -p "$D/port-cheat7/user"
python "$TOOLS/regvalues.py" toini < "$D/orig-won39.values" > "$D/port-cheat7/user/registry.ini"
PORT_KEEP_USER=1 "$TOOLS/port.sh" "$S/cheat7.txt" 7 "$D/port-cheat7/port.log"
port_values "$D/port-cheat7.values" "$D/port-cheat7/user"
diff "$D/orig-cheat7.values" "$D/port-cheat7.values" > "$D/cheat7.diff" && two=same || two=DIFFERS
cmp -s "$D/orig-won39.values" "$D/orig-cheat7.values" && two="$two (BUT NOTHING CHANGED)"
echo "cheat7 continued by the other program: $two"
grep -l "dialog appeared" "$D"/orig-*.txt | while read -r f; do grep "dialog appeared" "$f" | grep -qv "\[hooked" && echo "a dialog appeared: $f"; done
[ "$one" = same ] && [ "$two" = same ] && echo "xsave: same" || { echo "xsave: DIFFERS"; exit 1; }
