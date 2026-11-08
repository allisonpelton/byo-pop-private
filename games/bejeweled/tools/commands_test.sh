#!/usr/bin/env bash
# The standard test commands (docs/TEST_COMMANDS.md) on the native build: runs commands_test.txt as a -script under rundesk
# (CLAUDE.md) with -fast and checks the answers. Prints one line, "commands_test: ok" or what failed.
#   games/bejeweled/tools/commands_test.sh [<exe>]     (default build/native/bejeweled.exe)
set -uo pipefail
ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"
EXE="${1:-$ROOT/build/native/bejeweled.exe}"
DIR="$ROOT/build/cmdtest"
rm -rf "$DIR"; mkdir -p "$DIR/user" "$DIR/mods"
printf '[SOFTWARE\Steam\Bejeweled]\nScreenMode=dword:0\n' > "$DIR/user/registry.ini"
cd "$DIR"
timeout 300 "$ROOT/build/native/rundesk.exe" "$(cygpath -w "$EXE")" -gamedir="$(cygpath -w "$ROOT/assets/bejeweled")" \
	-userdir="$(cygpath -w "$DIR/user")" -modsdir="$(cygpath -w "$DIR/mods")" -seed=1 -fast \
	-script="$(cygpath -w "$ROOT/games/bejeweled/tools/commands_test.txt")" > out.txt 2>&1
fail=""
need() {  # <description> <extended regex that must match a line of out.txt>
	grep -Eq "$2" out.txt || fail="$fail; $1"
}
need "state at the title" '^STATE \{"screen":"title"'
need "unlock all" '^OK unlock all$'
need "seed" '^OK seed 5$'
need "unknown screen refused" '^ERROR goto nowhere: unknown screen'
need "title refused" '^ERROR goto title: '
need "win without a game refused" '^ERROR win: no game is on'
need "goto normal" '^OK goto normal$'
need "busy board refused" '^ERROR level 3: the board is busy'
need "move waits while the board falls" '^MOVE \{"kind":"wait","input":\[\{"wait":10\}\]\}$'
need "state of a game" '^STATE \{"screen":"game","mode":"normal","level":1,"score":0,.*"phase":"playing".*"board":\["[0-9a-z.]{8}"'
need "a swap move" '^MOVE \{"kind":"swap","from":\[[0-9],[0-9]\],"to":\[[0-9],[0-9]\],"input":\[\{"move":\[[0-9]+,[0-9]+\]\},\{"click":\[[0-9]+,[0-9]+\]\},\{"move":\[[0-9]+,[0-9]+\]\},\{"click":\[[0-9]+,[0-9]+\]\},\{"wait":4\}\]'
need "level 3" '^STATE \{"screen":"game","mode":"normal","level":3,'
need "win goes up a level" '^STATE \{"screen":"game","mode":"normal","level":4,'
need "goto timed" '^OK goto timed$'
need "timed state" '^STATE \{"screen":"game","mode":"timed",'
need "set drain" '^OK set drain 12$'
need "unknown set refused" '^ERROR set bar 3: '
need "options" '^STATE \{"screen":"options"'
need "high scores" '^STATE \{"screen":"highscores"'
need "help" '^STATE \{"screen":"help"'
need "menu closes them" '^OK goto menu$'
need "lose" '^OK lose$'
need "over" '"over":true'
[ -z "$fail" ] && echo "commands_test: ok" || { echo "commands_test: FAILED$fail (see $DIR/out.txt)"; exit 1; }
