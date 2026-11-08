#!/bin/bash
# tools/fwtest/commands.sh <log> <commands file> script|realinput <game dir>: the live command channel
# (-commands=<file>, docs/TEST_COMMANDS.md "Sending them") driven as a driver drives it. fwtest starts with -fast before
# the file exists; each step appends lines and waits for their answers on the log (OK, ERROR), so every command is
# chosen after the last one's answer came back. "realinput" adds -realinput with an empty file, so the channel's
# lines are real SDL events. A partial line (no end of line yet) must wait for its end. The last line says
# "commands <mode>: ok" or what went wrong. tools/check.sh unit runs it under rundesk.
LOG="$1"; CF="$2"; MODE="$3"; GAMEDIR="$4"
rm -f "$CF" "$LOG"
EXTRA=()
POINT="548 420"
EMPTY="100 100"
if [ "$MODE" = realinput ]; then
	: > "$CF.empty"
	EXTRA=(-realinput="$CF.empty")
	POINT="game 548 420"
	EMPTY="game 100 100"
fi
build/native/fwtest.exe -commands="$CF" "${EXTRA[@]}" -fast -placeholders -gamedir="$GAMEDIR" > "$LOG" 2>&1 &
PID=$!

fail() {
	echo "commands $MODE: FAILED: $1"
	kill $PID 2> /dev/null
	wait $PID 2> /dev/null
	exit 1
}
# wait_for <fixed text>: up to 30 s for a log line holding it
wait_for() {
	local t=0
	until grep -q -F -- "$1" "$LOG"; do
		kill -0 $PID 2> /dev/null || fail "fwtest ended before \"$1\""
		sleep 0.1
		t=$((t + 1))
		[ $t -lt 300 ] || fail "no \"$1\" within 30 s"
	done
}

wait_for "commands: waiting for"
printf 'expect board\n' > "$CF"
wait_for "OK expect board"
printf 'click %s\n' "$POINT" >> "$CF"
wait_for "OK click $POINT"
printf 'expect dialog 1\n' >> "$CF"
wait_for "OK expect dialog 1"
printf 'killdialog\n# a comment, then a blank line\n\nnosuch 1 2\n' >> "$CF"
wait_for "ERROR nosuch 1 2: unknown command"
grep -q -F "OK killdialog" "$LOG" || fail "killdialog did not answer"
printf 'expect dial' >> "$CF"
sleep 1
grep -q -F "expect dial:" "$LOG" && fail "a line was run before its end of line"
grep -q -F "unknown expect 'dial'" "$LOG" && fail "a line was run before its end of line"
printf 'og 0\n' >> "$CF"
wait_for "OK expect dialog 0"
printf 'click 10\n' >> "$CF"
wait_for "ERROR click 10: click needs <x> <y>"
# A test command that retries through the script (fwtest's waitdown, AddScriptCommand) until a press reaches the board:
# a click on an empty spot of the board must reach it meanwhile. HandleEvent once dropped every real event while a
# retry sat in mScript, so one waiting goto kept Bookworm Adventures' real clicks out (2026-11-08)
printf 'waitdown\n' >> "$CF"
sleep 0.5
grep -q -F "OK waitdown" "$LOG" && fail "waitdown answered before any press"
printf 'click %s\n' "$EMPTY" >> "$CF"
wait_for "OK waitdown"
printf 'report\n' >> "$CF"
wait_for "OK report"
wait $PID
grep -q -E "commands: [0-9]+ checks, 0 failed" "$LOG" || fail "$(grep -E 'commands: [0-9]+ checks' "$LOG")"
rm -f "$CF" "$CF.empty"
echo "commands $MODE: ok"
