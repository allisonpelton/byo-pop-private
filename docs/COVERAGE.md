# The coverage check

A game is done only once it has been through the coverage check (CLAUDE.md, "Done means every phase but modding";
`docs/COORDINATION.md`, "A coverage check after the identical-play passes"). The comparisons with the original test
only the code their runs reach; a slip in an option, a rare branch or an error path gets past them. So the check asks
two questions of a port, using runs of the original: does the port have a counterpart for every function of the
original's game code that the runs reach, and which of the port's functions do the runs never reach? The first finds
code a translation missed. Each function of the second is read again against the export, or given a run that reaches
it; one that no play of the original can reach is a candidate for taking out the code a game does not need.

It has two parts. origrun's `--coverage` records which of the original's functions a run reaches (`docs/ORIGRUN.md`,
"Coverage"). `tools/coverage.py` writes the list of functions for it to watch, and puts the runs' hits beside the
port's sources in a report.

## Running it

```bash
python -I tools/coverage.py list <game>
```

This writes `build/coverage/<game>/functions.txt`: every function of the game's Ghidra export (`re/<game>/.../functions.jsonl`)
except its thunks, one address a line. Then run the original through origrun as the game's comparisons already do, on
the host, with `--coverage build/coverage/<game>/functions.txt` added (a Windows path, as for `--script`). Each run
writes `build/origrun/<run>/coverage.txt`. The runs should between them cover the game: every mode played to its end,
every dialog, the options, the high scores, losing and regaining the focus, quitting from each screen. Hits from many
runs add up, so a run that only opens a dialog is worth having.

```bash
python -I tools/coverage.py report <game> build/origrun/<run1>/coverage.txt build/origrun/<run2>/coverage.txt -o build/coverage/<game>/report.md
```

The report is Markdown. A coverage run is a run of the original like any other (CLAUDE.md, "Running games in tests"):
it counts as a game process, and it does not run while an oracle runs.

## The settings file

`games/<game>/coverage.txt` tells the report what to compare. Without one, the whole exe counts as game code and the
export is the first `functions.jsonl` under `re/<game>/game`, `gh` or `ghidra`.

```
export re/bejeweled/game/functions.jsonl
src games/bejeweled/src
game 004c9a94-004d82d2
none 004d2f3b-004d35b5 RegisterDialog: registration code, removed with the rest of it
```

`export` names the export and `src` the port's sources (more than one `src` line may be given). `game` is the range of
the original's own game code, both ends being function entries; give several lines for a game whose code is not in one
piece. The rest of the exe is the framework, the C runtime and the libraries the game links, which are ported once for
every game, so they are only counted in the report. Wherever the linker put them, the C++ library's templates (`std::`)
and the compiler's deleting destructors count as library code too. `none` names a function (or a range) of the game
code that the port leaves out on purpose, with the reason: registration code, buttons that lead out of the game, a
`WinMain` that the platform layer replaces.

## How the port's counterparts are found

The ports write the original's address in the comment block just above each function they translate (`// 0x4cb5bc`), and
often beside a declaration in a header (`void DrawEditBox(...);  // 0x4d2857`). An address in the comment block above a
definition or a declaration, or in the comment on its own line, makes that function a counterpart. A declaration may run
over several lines and have default arguments, and a constructor or destructor declared in its class counts (its name
the class's own, at the class's own scope), while a call statement in a function body never does. An address in any
other comment is a mention: code that cites the function without being it. An address inside a function, not at its
entry, counts for the function that holds it, as a mention. The report counts a function as ported only with a
counterpart, so a function translated inline into another needs its address in the comment above the function that holds
it.

## Reading the report

The report opens with the runs (how many functions each reached, and whether the game exited by itself or origrun
stopped it at a timeout; a run that origrun stops has still played the script it was given) and the counts for the game
code. Then three lists:

- **Reached in the original, with no definition in the port.** The work: each is code the original ran that the port
  has no function for. Translated inline somewhere: give its address in the comment above the function that holds it.
  Missing: translate it. Left out on purpose: a `none` line with the reason.
- **Defined in the port, never reached in these runs.** No comparison has tested these. Give each a run that reaches it
  (a feature the runs missed), or read the port's function again against the export. One that no play of the original
  can reach is a candidate for taking out; that is the game session's call, and code that only some input reaches (an
  error path, a rare dialog) stays.
- **Never reached and not defined in the port.** Dead code of the original, or code only input the runs never gave
  reaches. Nothing to do unless a run should have reached it.

Last come the functions left out on purpose, and the functions outside the game code by group (framework, C runtime,
libraries, unnamed), with how many the runs reached.

## A game's whole check

Ning Po took the first (2026-11-08; its `GAME_NOTES.md`, "Coverage check", is the example to follow, and
`games/ningpo/tools/coverage/run.sh` the script). A game's session takes it in its own worktree:

1. Merge main and rebuild origrun (`tools/origrun/build.sh`). Run `python -I tools/coverage.py list <game>`, and write
   `games/<game>/coverage.txt`: the export, the sources, the game code's range or ranges, and a `none` line, with its
   reason, for each function the port leaves out on purpose.
2. Run the original with `--coverage`: the game's own identical-play runs first, then short runs for what they miss
   (every mode to its end, every dialog, the options, the high scores, losing and regaining the focus, quitting from
   each screen). A run on the virtual clock with no sound device skips the sound code, so give one run the real clock
   and the silent stand-in. Each run is a game process like any other: at most 3 on the machine
   (`python -I tools/gamecount.py --limit 3`), none during an oracle, and no link or "More Games" button clicked.
3. Run `python -I tools/coverage.py report <game> <hits>...` and work through the report. A function reached with no
   counterpart gets one of three things: its address placed above the port function that holds it, a translation if
   it is missing, or a `none` line. A port function never reached is either given a run that reaches it, or read again
   against the export and any slip fixed. Those no play can reach are listed in the notes as candidates for taking out,
   not removed yet. One Sonnet agent can sort the first report's long list (Ning Po's 253 took it 7.5 minutes).
4. Write a section "Coverage check" in the game's notes: the runs, the counts before and after, each gap and what was
   done, the candidates, anything left open, and roughly what it cost. Then message the coordinator to merge. A spec
   for the tool goes to the coordinator; the session does not edit the tool itself.

On Ning Po (815 game functions) the runs took about 15 minutes of machine time, and every change to the sources was a
comment. A game that finds missing logic translates it from the export and runs its suite and frozen baseline again.

## Limits

Functions only: a function is reached or not, with no count of calls and no lines or branches inside it. The exe's own
code only, not the game's DLLs. Not inside Windows Sandbox yet, so a game whose comparisons run there (Bejeweled's do)
takes its coverage runs on the host, where origrun posts input and takes no shots. A comparison of the framework's own
coverage across games is not part of it.
