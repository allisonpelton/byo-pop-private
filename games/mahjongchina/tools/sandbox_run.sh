#!/usr/bin/env bash
# Run Ancient China's original inside Windows Sandbox with an origrun script, for screens and sound at once:
#
#   games/mahjongchina/tools/sandbox_run.sh <run> <origrun script> [<port user dir>]
#
# A fresh copy (build/origrun/<run>/game) gets the two stand-ins: tools/ddrawcap (DDRAW.dll: without it every shot of
# this exe is black) and tools/bassproxy (bass.dll, the real one renamed bass_real.dll; the BASS calls go to
# basslog.txt in the copy). The players of <port user dir>, re-signed for the sandbox's C:\run\game\, go into SYSTEM's
# %LOCALAPPDATA%\JollyBear\Mahjong Escape inside. The settings are windowed, software and 4:3, as the port's runs use
# (FullScreen, HardwareAcceleration and WideScreen 0). One sandbox at a time: origrun itself refuses a second one.
set -euo pipefail
run="$1"; script="$2"; users="${3:-}"
root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
cd "$root"
tools="games/mahjongchina/tools"
bin="build/mjc_bin"
tc="$(ls -d /e/retools/llvm-mingw-*-ucrt-x86_64 | sort | tail -1)"
mkdir -p "$bin"
"$tc/bin/i686-w64-mingw32-clang.exe" -O2 -shared "$tools/ddrawcap/ddrawcap.c" "$tools/ddrawcap/ddrawcap.def" \
	-o "$bin/DDRAW.dll" -luser32
"$tc/bin/i686-w64-mingw32-clang.exe" -O2 -shared "$tools/bassproxy/bassproxy.c" "$tools/bassproxy/bassproxy.def" \
	-o "$bin/bass.dll"
printf 'Windows Registry Editor Version 5.00\r\n\r\n[HKEY_CURRENT_USER\\Software\\JollyBear\\Mahjong Escape]\r\n"FullScreen"=dword:00000000\r\n"HardwareAcceleration"=dword:00000000\r\n"WideScreen"=dword:00000000\r\n' \
	> "$bin/china.reg"

dir="build/origrun/$run"
rm -rf "${dir:?}"
./build/origrun-bin/origrun.exe --game mahjongchina --run "$run" --copy-only > /dev/null
cp "$bin/DDRAW.dll" "$dir/game/"
mv "$dir/game/bass.dll" "$dir/game/bass_real.dll"
cp "$bin/bass.dll" "$dir/game/"
if [ -n "$users" ]; then
	saves="$dir/sandbox-files/C/Windows/System32/config/systemprofile/AppData/Local/JollyBear/Mahjong Escape"
	mkdir -p "$saves"
	for f in "$users"/*.usr; do
		case "$(basename "$f")" in
			list.usr) cp "$f" "$saves/" ;;   # the list has no signature
			*) python "$tools/resign.py" "$f" "$saves/$(basename "$f")" ;;
		esac
	done
fi
./build/origrun-bin/origrun.exe --game mahjongchina --run "$run" --sandbox --reg "$(cygpath -w "$bin/china.reg")" \
	--script "$(cygpath -w "$script")" --timeout 480 > "$dir.log" 2>&1
grep -E "RESULT|shot .* \[" "$dir/logs/origrun-sandbox.log" | grep -vc "shot splash" | sed 's/^/lines: /'
grep -cE "shot [A-Za-z0-9_]+ -> .*\[printwindow" "$dir/logs/origrun-sandbox.log" | sed 's/^/printwindow shots: /'
wc -l < "$dir/game/basslog.txt" | sed 's/^/BASS calls: /'
