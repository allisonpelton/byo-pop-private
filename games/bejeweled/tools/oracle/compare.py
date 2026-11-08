#!/usr/bin/env python3
"""Regression oracle, comparison side (games/bejeweled/docs/CODE_MODS_DESIGN.md 9.1). compare.sh (next to
this file) runs the baseline exes and the candidate exe over the oracle cases into
build/oracle/<side>/<case>/ and then calls "compare.py compare build/oracle
games/bejeweled/tools/oracle/expect.txt". The other commands prepare scripts for compare.sh or maintain
expect.txt.

  compare.py strip-saves <script> <out>     copy of a -script file with its "save" lines commented out
                                            (for an exe without the save command; line numbers kept)
  compare.py quit-cuts <script> <outdir>    for each "save snaps/<name>.dat" line, <outdir>/<name>.txt:
                                            the script up to that command, then "quit" at its update;
                                            prints "<name> <update>" per save
  compare.py savestate <savegame.dat>...    decodes the header and state of v1 savegames
  compare.py fake-replays <dir>             ten old replays (one pinned) for the pruning case
  compare.py corrupt-replay <in> <out> <kind>
                                            a copy of a replay with one thing broken, for the player's
                                            corrupt-replay cases: "check" (the first CHECK's hash), "select"
                                            (the first SELECT twice), "unpause" (the last UNPAUSE dropped),
                                            "end" (END's score); or "engine<N>": nothing broken, the file as
                                            engine N would have written it (its engine ABI N); prints what it
                                            changed
  compare.py compare <oracle dir> [<expect file>]
                                            compares every baseline side with candidate/ for every case
                                            in <oracle dir>/cases.txt, and checks the candidate against
                                            the expect file; exit status 1 on any difference
  compare.py expect-update <oracle dir> <expect file>
                                            rewrites the expect file's derived facts from the candidate
                                            runs (keeps its comments and "row" lines) and prints the diff.
                                            Only after checking that the change in behaviour is intended.
                                            From the oracle of a BEJ_NO_VARIANTS candidate (exes.txt
                                            candidate.variants=no) it writes only "<case>@novariants" facts:
                                            those of the cases whose facts differ from the default build's
                                            (the rule-mod cases that build plays as before stage 5); the
                                            check takes them instead of <case>'s for such a candidate.
                                            Refuses a partial oracle (compare.sh --cases)
  compare.py select-cases <cases.txt> <glob>,...
                                            for compare.sh --cases: keeps in <cases.txt> the cases a glob
                                            matches and, again and again, the cases those are compared with
                                            (a case named in their flags, the case their name extends: x for
                                            x.plain), and prints their names
  compare.py cache-key <run dir>           for compare.sh's run cache: the sha256 of the key text on
                                            stdin and the prepared run directory (paths and contents, the
                                            day fields of hiscores*.dat masked)
  compare.py tree-hash <dir>                the sha256 of a directory tree (paths and contents)
  compare.py shift-hiscores-days <dir> <days>
                                            adds <days> to both day fields of every hiscores*.dat in <dir>
  compare.py hiscores-day <dir> <day>       exit status 0 if every hiscores*.dat in <dir> holds <day> in
                                            both day fields (compare.sh caches a run only then)

Sides (exes.txt): "baseline" (the build before stage 0, no test hooks), "hooks-baseline" (the frozen
stage-0 build) and "candidate". Each baseline side is compared with the candidate.

What "identical" means, per case and baseline side:
  stdout.txt  (TRACE) and stderr.txt (SDL_Log) line for line, after masking the wall-clock numbers only:
              "Seconds = <s>", "Resource Loading Time: <ms>" and the draw count of the "Seconds =" line,
              which is checked on its own (DRAWS below); its sleep and update counts are compared. They
              are separate files because stderr is block-buffered when redirected: in one file its chunks
              would split stdout lines.
  file tree   the whole run directory, files and directories (an empty directory counts), the same on both
              sides; then every file byte-identical, except:
                shots/*.png   pixel-identical (byte-identical files are not decoded)
                hiscores*.dat equal, or equal once the two day-number fields are masked, and only if each
                              file's two fields agree and the two sides' days are 1 or 2 apart (a run
                              across midnight or a year end): each of the two mode blocks starts with an
                              int32 day (HighScoreWidget::GetDayNumber, tm_year * 366 + tm_yday), at offset
                              0 and at offset 4 + the size of mode 0's six records (each record: u16 name
                              length n, n name bytes, int32 score, int32 checksum = 10 + n bytes). A run
                              compare.sh took from its cache (cached.txt: made-day, restored-day) was made
                              on an earlier day: its days must be that many days behind (again 1 or 2 more
                              or less across midnight or a year end)
              Not in the tree comparison: script.txt (input), stdout/stderr/exit (compared above), retried.txt
              (below), cached.txt (where a cached run came from) and, when the baseline has no hooks (or for a
              .plain case), the hook outputs: hash.txt, autoplay.txt and the files in snaps/.
  DRAWS       the "Seconds = <s>, Sleep Count = <n>, Update Count = <n>, Draw Count = <d>" line of
              MainLoopFinished: under -fast, updates and the sleeps they replace run on the virtual clock
              (sleeps are never counted, so the sleep count is 0), but SexyAppBase::DrawDirtyStuff still
              reads the wall clock: every FAST_DRAW_INTERVAL-th update a frame is drawn if a widget is dirty
              (deterministic) or if a second of real time passed since the last draw. Those extra draws of
              an unchanged screen depend on how fast the machine ran the game (other processes, other oracle
              jobs) and only add: at most one per wall-clock second after the last draw, so at most
              floor(<s>) in a run. Two draw counts that differ by no more than floor(<s>) of the run with
              more draws are a note; a larger difference is a difference. A baseline run of compare.sh's
              cache must not carry such draws into later oracles (a slow run there would widen the note's
              window and could match a candidate's real extra draw): gen/draws-noted.txt lists the
              baseline runs of every draw count difference, and compare.sh drops their cache entries.
  exit.txt    the exit status; besides, every candidate run must exit with 0. A frozen baseline exe that
              crashed at exit (139 or an access violation; those builds predate the audio shutdown fix
              af758b9) was run once more by compare.sh, which left the first status in retried.txt: the
              second run is compared and the report notes the retry (also when the run, retry and all, came
              from the cache). The candidate is never retried
  hash.txt    every candidate run but the .plain ones has one (-tracehash), compared with a hooks
              baseline's; lines "<update> <board state> <logic hash> <full hash>"
  mods folder a run with the real mods/ folder (the examples case) must leave it unchanged
              (compare.sh writes modsdir-changed.txt otherwise)
Cross-checks (cases.txt kinds; the case name gives the related case), all on candidate runs:
  plain       <case>.plain: the candidate run without -tracehash (no stage-0 switch at all) must be
              identical to every baseline's <case>, hook outputs aside
  quitcut     <case>.quit.<name>: snaps/<name>.dat, written by the save command, must equal the
              savegame.dat a real shutdown writes when the same script quits at that update, on every side
  nosave      <case>.nosave: the run of a save script with its save lines commented out must be identical
              to the run with them, hash.txt included: save changes nothing
  replay      <case>.replay: a -script run of the input that -autoplay recorded must be identical to the
              autoplay run (exit, logs, hash.txt, user dir): the bot acts only through script input
  inert       a candidate-only run with mods that the baselines cannot read (cases.txt flags "ref=<case>"):
              stdout.txt (TRACE) and hash.txt must equal the candidate's <case>, so the mods changed nothing
              in play beyond what <case> does (stage 2: rules are not played, they only give each board its
              own RNG, as <case>'s seeds.perGame feature does; stage 5: their boards are variant boards, whose
              "TRACE   variant" lines are left out, and they save v2, which hash.txt does not see); its
              mods-status.json must exist and name the RulesId that -dumprules (dumprules.txt) prints. With
              "boards=" flags, the seed checks too.
  repeat      <case>.repeat: the same run again must be identical (exit, logs, hash.txt, user dir, shots)
Set seed (stage 2; the baselines do not know -gameseed, so candidate runs; cases.txt flags "k=v;k=v"):
  seeded      the boards of the run, from its TRACE lines and hash.txt (a board starts where hash.txt shows
              one created or the board tick stops growing, see trace_boards), must be "boards=" (per board
              "user", "pergame" or "-" for none, from the "TRACE   seed" line); each seeded board's first
              settle must be the board Board::Board fills from MTRand(seed) (the model below), a "user" seed
              must be "gameseed=", and a "pergame" seed must be a draw of the global stream MTRand(-seed)
              ("rngseed="), at increasing positions. "shot=" (a game-over screenshot 59 updates in) must
              show text in the SEED line's band, "plainshot=" (another case's, unseeded, same timing) none;
              "differs=<case>": its first board differs from that one
  sameboard   <case>.seed2 (flags "ref=<case>"): the same -gameseed run with another -seed: stdout.txt and
              the board-state and LOGIC columns of hash.txt identical, the FULL column (which hashes the
              global RNG state) different somewhere, user dir identical: the board stream ignores -seed
  samegrids   <case>.nodialog (flags "ref=<case>;after=<update>;table=<file>"): <case> with the lines of
              a dialog that draws from the global stream left out of its script: every TRACE settle block
              (rows, score, seed and moves; not the update and board tick numbers, which the paused board
              shifts) identical, <case> with a settle after update <after> (a refill after the dialog) and
              user/<table> (the dialog's HighScoreWidget saved it, so its constructor drew the tip), the
              other run without it: an outside draw does not reach the board's own stream
  lint        games/bejeweled/tools/check_rng_use.py (6.1 rule 1) must pass (check.txt, exit.txt); also
              livesplit_layout: games/bejeweled/tools/livesplit/check_asl_offsets.py (stage 2b)
  cli         -gameseed values that must be refused (cli.txt: "<argument> <tab> <exit> <tab> <stderr>"):
              exit status 2 and the range in the message, before anything starts
              A "boards=" kind "user*" is a board that resumed a save of a game on such a seed (stage 3): its
              first settle is at board tick 1 with the loaded grid, its seed "gameseed=" if given
Replays (stage 3; candidate runs, flags "k=v;k=v"):
  recording   (flags "ref=<case>") 9.1 item 5, recording is inert: the run with the replays feature mod
              (mods/stage3/replays: replays with the hover channel, seeds {entry}, perGame off, so every game is
              recorded on the global stream) against the candidate's <case> without mods: stdout.txt, hash.txt
              and stderr.txt (but the mod's "mods:" lines and the "game:" line) identical, every user file of
              <case> identical (its sidecars, a seeded game's, gain the recording), and only these new: mods-status.json, replays/ with its finished replays, the
              sidecars savegame*.bjr; every .bjr well-formed (games/bejeweled/tools/bjr.py), each sidecar goes with
              its save (its SVHA), each sidecar the "save" command left in snaps/ ends in SAVE(snapshot) of its
              snapshot; with -codeccheck, codec.txt has no failure and a line per TRACE settle
  samestream  (flags "first=<case>;ref=<case>") a seeded game played to a quit (first), resumed in its user dir
              (this run), and the same moves played through (ref): the settle blocks of first and of this run
              (after its first, the loaded game, which is first's last) are ref's: the resume restored the RNG
              A recording's mods-status.json is not compared with its reference's (the mods differ by design).
  reference   a candidate run other cases are checked against (a recording's run without the replays mod); only
              its exit status and expect facts
  playback    (flags "trace=<case>[,<case>...]" or "corrupt=<text>", "user=<file>,...") 9.1 item 6, in the game: the candidate
              watches replay.bjr (-replay=replay.bjr -replayquit, a copy of a recorded file) in a fresh user dir.
              Its "REPLAY end" line must be "ok" with the file's CHECK count and END values (every CHECK and END
              matched in the game), its TRACE settle blocks (all but the update number) per segment ("REPLAY
              segment k" lines) a contiguous run of the blocks of the k-th trace case's stdout (the last one for
              the rest), codec.txt without a failure and one settle per TRACE settle, and the user dir only
              registry.ini, with TimesPlayed 0: watching saves, counts and records nothing ("user=" lists the
              files a script's own actions leave, such as hiscores.dat from Options -> High Scores). With
              "corrupt=", the line must say CORRUPT and contain that text. With "speed" (stage 13: the script sets
              2x, 4x or 8x, or steps to the next move at 8x; the test hooks still run once a frame, after several
              board updates), the TRACE settles are not compared with the recording's, nor counted against
              codec.txt: a settle and the next move can fall in one frame. The REPLAY end line carries the check
              (the controller matched every CHECK and END), with the segment count and codec.txt without a failure
The headless verifier (stage 4, games/bejeweled/docs/CODE_MODS_DESIGN.md 6.9; the cases come from
games/bejeweled/tools/verify/oracle_cases.sh, which compare.sh sources):
  verifier    <watch>.verify (flags "watch=<case>" and that case's flags, or "unplayable"): the native verifier
              (native.txt) and the wasm verifier under Node (wasm.txt) played the watch case's replay.bjr with
              -trace. Their REPLAY and TRACE lines must be the game's in <case>/stdout.txt (but the number in
              "TRACE update N:", the app's update count) and each other's (the JSON result too), their JSON
              result the game's verdict ("ok", "corrupt", or "unplayable" for "cannot play") with the game's
              REPLAY line as its message, and their exit status 0 for ok, else 1: game, native verifier and wasm
              verifier agree on TRACE, CHECK and END (9.1 item 6, the shim drift guard). With "verdict=ok" the
              game's verdict must be ok too (a watch run that is no playback case: one without -codeccheck, whose
              saves the save check of an LDSV refuses on purpose, such as a score past the int range). A "speed"
              watch (stage 13) is compared by its REPLAY lines only: the verifiers play at 1x, so their TRACE
              settles are the recording's and not the faster game's; the two verifiers still agree line for line
  wasmlogic   <case>.wasm (flags "ref=<case>"): the game's web build under Node (wasmrun.js game) ran <case>'s
              switches and inputs: hash.txt's update, board state and LOGIC columns and stdout.txt identical to
              the candidate's <case> (the FULL column, same platform only, is reported, not required)
  difftest    bejeweled_verify difftest (report.txt, exit.txt): both modes' summaries say 0 differ (9.1 item 7)
  verify_tests (kind lint) is games/bejeweled/tools/verify/run_tests.sh
Allow invalid moves, the noMoves rule and the v2 save (stage 5; candidate runs with the rule mods of mods/stage5,
left out for a candidate built without variant paths, BEJ_NO_VARIANTS; flags "k=v;k=v"):
  variant     every settle has its "TRACE   variant moves M invalid I" line (a variant board); from one settle to
              the next on the same board exactly one move happened: +1 move (a match), or +1 invalid and +1 move
              (+0 with "countsasmove=0"), or nothing across a vortex, wipe or fall-out (hash.txt); a kept move
              with no refill between is one swap of neighbours in the grid and scores nothing. "kept=N": at least
              N such swaps. Settles without a move only with "continue=N": at least N that the same board goes
              on from (noMoves "continue"). "log=<text>": a stderr line with it (a refused save logs
              "savegame: ... not loaded: <why>"; without the flag no save may be refused). "nosave": no
              savegame*.dat left. "v2" ("v2=last"): every savegame*.dat is a v2 save of the run's RulesId
              (mods-status.json; with "last", its MOVE counts those of the last settle). "timesplayed=N": the
              user dir's registry.ini has TimesPlayed N (game overs counted). "warn=<text>": a warning in
              mods-status.json contains it. "boards=": the seed checks.
              Stage 6 (undo, mods/stage6): a settle whose line says "undos U" one more than the last is an undo:
              the board (grid, moves, invalid) of the settle the newest move not taken back was made from, its
              score at most that one's; a vortex leaves nothing to take back. A swap that went back is a settle
              with nothing changed but the score, lower by the hints taken before it. "undos=N": exactly N undos (else none); "redo=N" / "rerolled=N": at least N
              moves right after an undo whose settle has the grid of the settle the undo took back / another
              grid (undo.restoreRng; the scripts make the same move again). No stderr line "undo: snapshot ...".
              Stage 7 (board sizes and shapes, mods/stage7): "shapes=<cols>x<rows>:<holes>,...", the shapes of
              board.levels in order: every settle's grid has its level's size and that many holes (-1 cells), the
              same cells at every settle of that shape (holes stay holes, a refill fills every playable cell).
              Stage 7c (Twilight gravity, mods/stage7c): "gravity=up|down|alternate": every settle's variant line
              names the direction it now falls. up and down: that direction and no flips at every settle.
              alternate: up at an even count of flips, down at an odd one; on one board a move or a refill flips
              once, a swap that went back or a kept invalid move not at all, and an undo brings back the count of
              the settle it returns to.
              Stage 9 (match rules, mods/stage9): "match=<dir>", the match.* rules of the mod in mods/<dir> that has
              them: no settled grid holds a group of them (the game found and cleared every one), and every
              settle's move lines are, in order, the swaps that put a gem into a group (FindMoveVariant), found
              here by brute force on the grid. "kinds=<kind>,...": the variant line's counts (the groups the board
              scored: L, T, plus, diagonal, shapes) of each of those kinds above 0 at some settle.
              Stage 10 (twists and slides, mods/stage10): "moves=<type>+...", the rules' move types (swap, twist,
              slide), with "twist=both" when twists go both ways (and "match=<dir>" for match rules): no settled
              grid holds a group, every settle's move lines are, in order, MoveSet's moves of those types (the
              swaps, the twists, the slides of the full segments), found here by brute force with where the gem
              that makes the group lands and comes from, and each type has a move at some settle.
              Stage 11 (flame and lightning gems, mods/stage11): "specials=F,L,B": every settle's variant line counts
              the specials made and the blasts and its specials line lists specials on gems only, no more than were
              made; some board made F flame gems, L lightning gems and B blasts or more. "after=<case>": the first
              settle's grid and specials are that case's last settle's (a resume of its save's KIND chunk).
              Stage 12 (the gold board, mods/stage12): "gold=G,L": every settle's variant line counts the gold of the
              goal (and the level's ticks left of its limit, when it has one) and its gold line agrees, with no gold
              outside the goal; within a level the gold stays (but for an undo) and the time only runs down; a level
              ends only on a settle whose goal is all gold, and the next level's first settle has none; some settle has
              G gold cells or more, and the HUD reaches level L. "timeup": the bot's log (autoplay.txt) ends the game,
              and the last settle's level has a limit and gold left (TIME UP: a gold game ends no other way). "after=<case>": the first settle's grid and gold are that case's
              last settle's, its time left no more (a resume of its save's GOLD and GTIM chunks). "nowarn":
              mods-status.json has no warning.
More gem colours (stage 8, games/bejeweled/docs/CODE_MODS_DESIGN.md 4.2; candidate runs with the mods of
mods/stage8, every build: board.gemColors is a tunable; flags "k=v;k=v"):
  colors      "colors=N": every gem in the TRACE rows a colour 0..N-1, all N of them with "all"; above 7 colours the
              game logs the gem sheet grown to at least N rows. "sheet=R": the dump of the script command gemsheet
              (shots/gemsheet.png) is 780 x R*104, every row after the first 7 drawn; with "ref=<case>" the first
              rows are those of that case's dump and the rows after them are the generated ones (a mod's HD art
              makes the 1x rows a scaled copy of its own, so that case has no ref); "glyphs=a-b": those rows carry
              the corner glyph (white at (41, 39) of the first frame); "hd=S": the HD pixels dumped too
              (shots/gemsheet@Sx.png), S times the size
  ui          stage 13 (the watch in the game, games/bejeweled/docs/CODE_MODS_DESIGN.md 4.11): a script that browses,
              watches and leaves replays from the game's own dialogs. "ends=<verdict>,...": its "REPLAY end" and
              "cannot play" lines in order, each ok, corrupt or unplayable; "resume": the last settle before the
              first "REPLAY start" and the first after the last of those lines show the same game (the rows, mode,
              level and score, not the update and board tick: the game left to watch, saved and resumed);
              "replays=N": user/replays holds N files at the quit
Finished replays are named after the wall clock: compared and listed with replays/<yyyymmdd>-<hhmmss>- masked
as replays/*- (normal_path), their contents without META's date (compare_bjr).
Auto LiveSplit (stage 2b, games/bejeweled/docs/CODE_MODS_DESIGN.md 4.15; candidate runs):
  lsref       <case>.lsoff: the candidate with the inert features.livesplit=false mod, the reference
  livesplit   <case>.ls (flags "ref=<case>.lsoff"): the candidate with the livesplit mod must play exactly
              like its reference: exit, stdout.txt, hash.txt, autoplay.txt, snaps/, shots/ and the user dir
              (but mods-status.json, whose features differ, and livesplit/, the copied split templates)
              identical; stderr.txt identical without its "livesplit:" and "mods:" lines; the screenshots
              also equal those of <case> without mods (the mod shows no mod signs). The mod's templates,
              and nothing else, are in user/livesplit/; the reference has none. Its -livesplitlog
              (livesplit.txt, one line per block write) must agree with hash.txt: the state at every
              update, seq and updates counting the writes, a level clear exactly where hash.txt enters the
              vortex (not where a LoadGame restores one), a game over exactly where it enters game over
              (no moves after falling, time up after playing), one GO at tick 150 for every board that did
              not resume a save and none for one that did, counters that never go back, displayScoreMax
              rising with the HUD score until the game over and then frozen, and the last write inactive.
              With stage 3: in a -replay run (replay.bjr) every board carries the replay flag 0x20 and has
              no GO (a playback board is never a run start); elsewhere no board carries it
Expectations (the expect file, checked against the candidate for the script, load, quitcut, inert, autoplay,
seeded, recording and samestream cases): what each case is about must still happen, even when every side changes together (a change to
assets/bejeweled/ or the mods, a script that no longer reaches its scenario). Facts derived from the outputs:
  exit <status>, mods <the "mods:" log line>, user <user dir files>,
  settle <update> <mode level score board tick>   every TRACE settle header (scripts and loads), in order
  settles <count> / last-settle <...>             the same for autoplay runs (and other -autoplay runs) and playbacks
  states <update>:<state> ...                     hash.txt: each update the board enters game over, game
                                                  over done, vortex, level wipe or fall out
  snap <file> <decoded>, save <file> <decoded>|none
                                                  save snapshots, and the savegame*.dat the quit left
  sidecar <file> <summary>, replay <file> <summary>
                                                  stage 3: user/savegame*.bjr and user/replays/*.bjr (bjr.py's
                                                  summary: END, segments, RNG, event counts; whether a sidecar
                                                  goes with its save)
  variant <last counts, settles without a move>   stage 5: a run of variant boards ("TRACE   variant" lines)
  codec <summary>                                 the -codeccheck summary line
  watch <the REPLAY end line>                     playback: how the replay ended (-replay)
  end <the bot's last note>                       autoplay: how the game ended
  status <RulesId prefix, message counts, ...>    user/mods-status.json, when the run left one (rule mods)
  seeds <update>:<seed>/<kind>[@<draw>] ...       when a board has its own RNG: per board (first settle),
                                                  "-" for one on the global stream; @: the position of a
                                                  per-game seed in the global stream (rngseed flag)
and hand-written ones, kept by expect-update:
  row <settle update> <row> <8 colours>           that TRACE row line at that settle
"""

import difflib
import fnmatch
import hashlib
import json
import os
import re
import struct
import sys
import zlib

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
import bjr  # noqa: E402  (games/bejeweled/tools/bjr.py: the .bjr reader)

STATE_NAMES = ["falling", "playing", "swapping", "swapping back", "clearing", "game over", "game over done",
               "vortex", "level wipe", "fall out"]
EVENT_STATES = {5: "game_over", 6: "game_over_done", 7: "vortex", 8: "level_wipe", 9: "fall_out"}
REFILL_STATES = (0, 2, 4, 8, 9)  # a board goes on falling, or refills, from these (UpdateClearing, FillBoard; stage 5: a
                                 # kept swap falls from swapping, Board::KeepInvalidMove)
HOOK_FILES = ("hash.txt", "autoplay.txt")
RUN_FILES = ("script.txt", "stdout.txt", "stderr.txt", "exit.txt", "modsdir-changed.txt", "retried.txt", "cached.txt")
WALL_CLOCK = [(re.compile(r"Seconds = [0-9.e+-]+"), "Seconds = *"),
              (re.compile(r"Resource Loading Time: [0-9]+"), "Resource Loading Time: *"),
              (re.compile(r"(Update Count = -?[0-9]+, Draw Count = )[0-9]+"), r"\1*")]  # DRAWS: compare_log
DRAW_LINE = re.compile(r"Seconds = ([0-9.e+-]+), Sleep Count = -?[0-9]+, Update Count = -?[0-9]+, Draw Count = ([0-9]+)")
EXPECT_KINDS = ("script", "load", "quitcut", "inert", "autoplay", "seeded", "recording", "samestream", "reference",
                "playback", "variant", "colors", "ui")
AUTO_KEYS = ("exit", "mods", "user", "settle", "settles", "last-settle", "states", "snap", "save", "sidecar", "replay",
             "status", "seeds", "end", "codec", "watch", "variant")
NOVARIANTS = "@novariants"  # expect.txt: "<case>@novariants" facts, a BEJ_NO_VARIANTS candidate's own (expect-update)
VARIANT_LINE = "TRACE   variant "  # stage 5: a variant board's line in its settle blocks (moves, kept invalid moves)
# A finished replay is named after the wall clock (<yyyymmdd>-<hhmmss>-<mode>-<score>...): the date and time are
# masked wherever runs are compared or facts derived (normal_path)
REPLAY_NAME = re.compile(r"(^|/)replays/\d{8}-\d{6}-")
NO_RUN_KINDS = ("lint", "cli", "verifier", "difftest")  # checks that run no game (of their own)
LS_LOG_LINES = re.compile(r"^INFO: (livesplit|mods): |^INFO: game: .*\| mods: ")  # dropped from a livesplit run's stderr


# ---------------------------------------------------------------------------------------------------
# Scripts

def parse_script(path):
    """(line index, update, command, args, raw line) per line; update and command are None for a
    comment or blank line."""
    with open(path, encoding="latin-1") as f:
        lines = f.read().splitlines()
    out = []
    for i, line in enumerate(lines):
        text = line.split("#", 1)[0].strip()
        parts = text.split(None, 2)
        if len(parts) >= 2 and re.fullmatch(r"-?\d+", parts[0]):
            out.append((i, int(parts[0]), parts[1], parts[2] if len(parts) > 2 else "", line))
        else:
            out.append((i, None, None, None, line))
    return out


def strip_saves(src, dst):
    with open(dst, "w", encoding="latin-1", newline="\n") as f:
        for _, update, cmd, _, line in parse_script(src):
            f.write(("# [oracle: no save command in this exe] " + line) if cmd == "save" else line)
            f.write("\n")


def quit_cuts(src, outdir):
    entries = parse_script(src)
    os.makedirs(outdir, exist_ok=True)
    for i, update, cmd, args, _ in entries:
        if cmd != "save":
            continue
        name = os.path.splitext(os.path.basename(args))[0]
        # RunScript runs the commands in update order, file order within an update (stable sort)
        kept = [e for e in entries if e[1] is not None and e[2] != "save" and
                (e[1] < update or (e[1] == update and e[0] < i))]
        with open(os.path.join(outdir, name + ".txt"), "w", encoding="latin-1", newline="\n") as f:
            f.write("# %s cut off at update %d: the quit writes the save that \"save %s\" snapshots\n"
                    % (os.path.basename(src), update, args))
            for e in kept:
                f.write(e[4] + "\n")
            f.write("%d quit\n" % update)
        print(name, update)


SAVE_VERSION_VARIANT = 0x101  # the v2 save of a variant board (stage 5, CODE_MODS_DESIGN.md 7.2)


def save_state(path):
    """Header and state of a savegame (Board::WriteSaveGame), as a short text: a v1 save, or the v2 save of a
    variant board (stage 5: its version 0x101 and header with the RulesId, size, level shape and mask before
    the same fields, then its chunks, which the text names: the RNGS seed, the MOVE counts)."""
    try:
        with open(path, "rb") as f:
            b = f.read()
        version, = struct.unpack_from("<i", b, 0)
        o = 4
        cols = rows = 8
        variant = ""
        if version == SAVE_VERSION_VARIANT:
            rules = b[4:36].hex()[:16]
            cols, rows, shape = struct.unpack_from("<HHi", b, 36)
            mask = b[44:44 + (cols * rows + 7) // 8]
            holes = sum(1 for i in range(cols * rows) if not (mask[i // 8] >> (i % 8)) & 1)
            o = 44 + len(mask)
            variant = "rules %s, %dx%d, level shape %d, %d holes" % (rules, cols, rows, shape, holes)
        timed, level, display_level = struct.unpack_from("<Bii", b, o)
        o += 9
        cells = 0
        for _ in range(cols * rows):
            present = b[o]
            o += 1
            if present:
                o += 0x78
                cells += 1
        state, = struct.unpack_from("<i", b, o)
        _, is_new, bar, seconds, idle, score = struct.unpack_from("<BBiiii", b, o + 4)
        name = STATE_NAMES[state] if 0 <= state < len(STATE_NAMES) else "?"
        text = ("v%s %s level %d (shown %d), %d gems, state %d (%s), score %d, play seconds %d, %d bytes"
                % ("2" if version == SAVE_VERSION_VARIANT else str(version), "timed" if timed else "normal", level,
                   display_level, cells, state, name, score, seconds, len(b)))
        if version == SAVE_VERSION_VARIANT:
            o += 116
            chunks = []
            while o + 8 <= len(b):
                tag, size = b[o:o + 4], struct.unpack_from("<I", b, o + 4)[0]
                payload = b[o + 8:o + 8 + size]
                o += 8 + size
                if tag == b"RNGS":
                    seed, user = struct.unpack_from("<IB", payload, 0)
                    chunks.append("RNGS seed %d%s" % (seed, " user" if user else ""))
                elif tag == b"MOVE":
                    chunks.append("MOVE moves %d invalid %d undos %d" % struct.unpack_from("<iii", payload, 0))
                elif tag == b"GRAV":
                    flips, no_flip = struct.unpack_from("<iB", payload, 0)
                    chunks.append("GRAV flips %d%s" % (flips, ", no flip" if no_flip else ""))
                elif tag == b"DIRT":
                    layers = payload[1:]
                    chunks.append("DIRT %d layers on %d cells, next %s" % (sum(layers), sum(1 for v in layers if v), "left" if payload[0] else "right"))
                elif tag == b"KIND":
                    chunks.append("KIND flame %d lightning %d" % (payload.count(1), payload.count(2)))
                elif tag == b"GOLD":
                    chunks.append("GOLD %d cells" % payload.count(1))
                elif tag == b"GTIM":
                    chunks.append("GTIM %d/%d ticks" % struct.unpack_from("<II", payload, 0))
                else:
                    chunks.append(tag.decode("latin-1").strip())
                if tag == b"END ":
                    break
            text += "; %s; %s%s" % (variant, ", ".join(chunks), "" if o == len(b) else ", %d bytes after" % (len(b) - o))
        return text
    except (OSError, struct.error, IndexError) as e:
        return "unreadable (%s)" % e


# ---------------------------------------------------------------------------------------------------
# PNG (enough for stb_image_write's 8-bit RGB/RGBA output; anything else falls back to PIL)

def png_pixels(path):
    with open(path, "rb") as f:
        data = f.read()
    if data[:8] != b"\x89PNG\r\n\x1a\n":
        raise ValueError("not a PNG")
    pos = 8
    idat = b""
    width = height = depth = ctype = interlace = None
    while pos < len(data):
        length, kind = struct.unpack_from(">I4s", data, pos)
        body = data[pos + 8:pos + 8 + length]
        pos += 12 + length
        if kind == b"IHDR":
            width, height, depth, ctype, _, _, interlace = struct.unpack(">IIBBBBB", body)
        elif kind == b"IDAT":
            idat += body
        elif kind == b"IEND":
            break
    if depth != 8 or ctype not in (2, 6) or interlace != 0:
        from PIL import Image  # rare formats only
        img = Image.open(path).convert("RGBA")
        return img.size[0], img.size[1], img.tobytes()
    bpp = 3 if ctype == 2 else 4
    raw = zlib.decompress(idat)
    stride = width * bpp
    out = bytearray()
    prev = bytearray(stride)
    p = 0
    for _ in range(height):
        ftype = raw[p]
        line = bytearray(raw[p + 1:p + 1 + stride])
        p += 1 + stride
        for x in range(stride):
            a = line[x - bpp] if x >= bpp else 0
            b = prev[x]
            c = prev[x - bpp] if x >= bpp else 0
            if ftype == 1:
                line[x] = (line[x] + a) & 255
            elif ftype == 2:
                line[x] = (line[x] + b) & 255
            elif ftype == 3:
                line[x] = (line[x] + ((a + b) >> 1)) & 255
            elif ftype == 4:
                pa, pb, pc = abs(b - c), abs(a - c), abs(a + b - 2 * c)
                line[x] = (line[x] + (a if pa <= pb and pa <= pc else b if pb <= pc else c)) & 255
        if bpp == 3:
            for x in range(width):
                out += line[x * 3:x * 3 + 3] + b"\xff"
        else:
            out += line
        prev = line
    return width, height, bytes(out)


def compare_png(a, b):
    """None if pixel-identical, else a description."""
    with open(a, "rb") as f:
        da = f.read()
    with open(b, "rb") as f:
        db = f.read()
    if da == db:
        return None
    wa, ha, pa = png_pixels(a)
    wb, hb, pb = png_pixels(b)
    if (wa, ha) != (wb, hb):
        return "size %dx%d vs %dx%d" % (wa, ha, wb, hb)
    if pa == pb:
        return None
    count = 0
    x0, y0, x1, y1 = wa, ha, -1, -1
    for i in range(0, len(pa), 4):
        if pa[i:i + 4] != pb[i:i + 4]:
            count += 1
            x, y = (i // 4) % wa, (i // 4) // wa
            x0, y0, x1, y1 = min(x0, x), min(y0, y), max(x1, x), max(y1, y)
    return "%d pixels differ in (%d,%d)-(%d,%d)" % (count, x0, y0, x1, y1)


# The options dialog (WinDMApp::ShowOptionsDialog: x 234, y 50, 348 x 290): the port took the original's "more games" link
# out (the user's rule of 2026-11-07, GAME_NOTES.md section 18) and moved the row of Help and High Scores 12 px down, while
# the frozen baseline exes still have the link. The shots that show the dialog may therefore differ from the baselines',
# inside that box and nowhere else.
OPTIONS_BOX = (234, 50, 234 + 348 - 1, 50 + 290 - 1)
OPTIONS_SHOTS = ("12_options.png", "o01_options.png", "o02_high_scores.png", "o03_back_to_options.png")


def inside_options_box(diff):
    """True if compare_png's description says that every differing pixel lies inside the options dialog's box."""
    m = re.match(r"\d+ pixels differ in \((\d+),(\d+)\)-\((\d+),(\d+)\)$", diff or "")
    if not m:
        return False
    x0, y0, x1, y1 = (int(g) for g in m.groups())
    return OPTIONS_BOX[0] <= x0 and OPTIONS_BOX[1] <= y0 and x1 <= OPTIONS_BOX[2] and y1 <= OPTIONS_BOX[3]


# ---------------------------------------------------------------------------------------------------
# hiscores.dat day fields

def hiscores_day_offsets(data):
    """Offsets of the two int32 day numbers (see the module comment), or None if it does not parse."""
    offsets = []
    pos = 0
    try:
        for _ in range(2):
            offsets.append(pos)
            pos += 4
            for _ in range(6):  # today's record + 5 table records
                n, = struct.unpack_from("<H", data, pos)
                pos += 2 + n + 8
        return offsets if pos == len(data) else None
    except struct.error:
        return None


def compare_hiscores(da, db, shift=0):
    """None if equal, ('masked', text) if equal once a midnight rollover is masked, else a description.
    shift: the days the candidate's day fields must be ahead of the baseline's (a baseline run compare.sh took
    from its cache, made that many days ago; day_shift); None if they are, ('masked', text) if they are 1 or 2
    days more or less (midnight or a year end on top)."""
    if da == db:
        return None
    oa, ob = hiscores_day_offsets(da), hiscores_day_offsets(db)
    if oa is None or oa != ob or len(da) != len(db):
        return first_byte_diff(da, db)
    days_a = [struct.unpack_from("<i", da, o)[0] for o in oa]
    days_b = [struct.unpack_from("<i", db, o)[0] for o in ob]
    ma, mb = bytearray(da), bytearray(db)
    for o in oa:
        ma[o:o + 4] = mb[o:o + 4] = b"\0\0\0\0"
    if ma != mb:
        return "after masking the day fields: " + first_byte_diff(bytes(ma), bytes(mb))
    if days_a[0] != days_a[1] or days_b[0] != days_b[1]:
        return "day fields disagree within a file: baseline %s, candidate %s" % (days_a, days_b)
    if shift and days_b[0] - days_a[0] == shift:
        return None
    if abs(days_b[0] - days_a[0] - shift) not in (1, 2):
        return "day %d vs %d: not a midnight rollover%s" % (days_a[0], days_b[0],
                                                           " (a cached run %d days behind)" % shift if shift else "")
    return ("masked", "day %d vs %d%s" % (days_a[0], days_b[0], " (a cached run %d days behind)" % shift if shift else ""))


def shift_hiscores_days(folder, days):
    """Adds days to both day fields of every hiscores*.dat in folder (compare.sh: a baseline run that starts from
    the user dir of a run taken from the cache, made days ago, gets the days a run of today would have left)"""
    for name in sorted(os.listdir(folder)):
        path = os.path.join(folder, name)
        if not name.lower().startswith("hiscores") or not os.path.isfile(path):
            continue
        data = bytearray(read_bytes(path))
        offsets = hiscores_day_offsets(bytes(data))
        if offsets is None:
            continue
        for o in offsets:
            struct.pack_into("<i", data, o, struct.unpack_from("<i", data, o)[0] + days)
        with open(path, "wb") as f:
            f.write(bytes(data))


def hiscores_of_day(folder, day):
    """Whether every hiscores*.dat in folder holds day in both day fields (compare.sh caches a run only if the user dir
    it started from was of the day it ran: the cache key masks the day fields, and the game keeps "today's" score
    only on the day it was saved). False for a hiscores file that does not parse."""
    if not os.path.isdir(folder):
        return True
    for name in sorted(os.listdir(folder)):
        path = os.path.join(folder, name)
        if not name.lower().startswith("hiscores") or not os.path.isfile(path):
            continue
        data = read_bytes(path)
        offsets = hiscores_day_offsets(data)
        if offsets is None or any(struct.unpack_from("<i", data, o)[0] != day for o in offsets):
            return False
    return True


def tree_digest(root, extra=b"", mask_days=False):
    """sha256 of extra and of the tree under root: every directory and file by relative path, files with their
    size and contents; with mask_days the day fields of hiscores*.dat count as 0 (compare.sh's cache key)"""
    h = hashlib.sha256(extra)
    for path in list_tree(root, lambda p: True):
        if path.endswith("/"):
            h.update(b"D %s\n" % path.encode("utf-8"))
            continue
        data = read_bytes(os.path.join(root, path))
        if mask_days and os.path.basename(path).lower().startswith("hiscores"):
            offsets = hiscores_day_offsets(data)
            if offsets is not None:
                masked = bytearray(data)
                for o in offsets:
                    masked[o:o + 4] = b"\0\0\0\0"
                data = bytes(masked)
        h.update(b"F %s %d %s\n" % (path.encode("utf-8"), len(data), hashlib.sha256(data).hexdigest().encode("ascii")))
    return h.hexdigest()


def cached_info(run):
    """cached.txt of a run compare.sh took from its cache (key=value lines), or None"""
    path = os.path.join(run, "cached.txt")
    return read_kv(path) if os.path.exists(path) else None


def day_shift(run):
    """How many days before its restore a cached run was made (0 for a run made now)"""
    info = cached_info(run)
    try:
        return int(info["restored-day"]) - int(info["made-day"]) if info else 0
    except (KeyError, ValueError):
        return 0


def first_byte_diff(da, db):
    n = min(len(da), len(db))
    for i in range(n):
        if da[i] != db[i]:
            return "first difference at byte %d (0x%02x vs 0x%02x), sizes %d / %d" % (i, da[i], db[i], len(da), len(db))
    return "sizes %d / %d" % (len(da), len(db))


# ---------------------------------------------------------------------------------------------------
# Files

def read_bytes(path):
    with open(path, "rb") as f:
        return f.read()


def read_lines(path):
    text = read_bytes(path).decode("latin-1").replace("\r\n", "\n")
    lines = text.split("\n")
    if lines and lines[-1] == "":
        lines.pop()
    return lines


def mask_wall_clock(line):
    for pattern, replacement in WALL_CLOCK:
        line = pattern.sub(replacement, line)
    return line


DRAWS_RUNS = set()  # the run directories of every draw count difference (compare: gen/draws-noted.txt)


def compare_log(r, la, lb, what, runs=()):
    """Two logs (lists of lines) line for line after mask_wall_clock, then the draw counts that masks: DRAWS in the
    module comment. runs: the two run directories, kept in DRAWS_RUNS on a draw count difference. Returns whether
    they matched (a note is a match)."""
    diff = first_list_diff([mask_wall_clock(x) for x in la], [mask_wall_clock(x) for x in lb], what)
    if diff:
        r.fail(diff)
        return False
    ok = True
    counts_a = [m.groups() for m in (DRAW_LINE.search(x) for x in la) if m]
    counts_b = [m.groups() for m in (DRAW_LINE.search(x) for x in lb) if m]
    for (sa, da), (sb, db) in zip(counts_a, counts_b):
        da, db = int(da), int(db)
        if da == db:
            continue
        DRAWS_RUNS.update(runs)
        # the run with more draws had at most floor(its seconds) draws of an unchanged screen more
        seconds = int(float(sb if db > da else sa))
        if abs(da - db) <= seconds:
            r.note("%s: Draw Count %d / %d: within the %d wall-clock seconds of the run with more draws (DRAWS: a"
                   " frame of an unchanged screen is drawn once a second of real time passed)" % (what, da, db, seconds))
        else:
            r.fail("%s: Draw Count %d / %d: more than the %d wall-clock seconds of the run with more draws can"
                   " explain (DRAWS)" % (what, da, db, seconds))
            ok = False
    return ok


def first_list_diff(a, b, what):
    for i in range(min(len(a), len(b))):
        if a[i] != b[i]:
            return "%s line %d differs:\n        baseline:  %s\n        candidate: %s" % (what, i + 1, a[i], b[i])
    if len(a) != len(b):
        extra = a[len(b)] if len(a) > len(b) else b[len(a)]
        return "%s: %d vs %d lines (first extra: %s)" % (what, len(a), len(b), extra)
    return None


def list_tree(root, keep):
    """Sorted relative paths under root, directories with a trailing '/', for which keep(path) is true."""
    out = []
    if os.path.isdir(root):
        for dirpath, dirs, files in os.walk(root):
            rel = os.path.relpath(dirpath, root).replace("\\", "/")
            prefix = "" if rel == "." else rel + "/"
            out += [prefix + d + "/" for d in dirs]
            out += [prefix + f for f in files]
    return sorted(p for p in out if keep(p))


def tree_filter(hook_outputs):
    """The paths compare_case compares as a tree: everything but the run files, and without the hook
    outputs unless both sides have them."""
    def keep(path):
        if path in RUN_FILES:
            return False
        if not hook_outputs and (path in HOOK_FILES or (path.startswith("snaps/") and path != "snaps/")):
            return False
        return True
    return keep


def normal_path(path):
    """path with a finished replay's wall-clock date and time masked: replays/*-normal-60-keep.bjr"""
    return REPLAY_NAME.sub(lambda m: m.group(1) + "replays/*-", path)


def replay_without_date(data):
    """A .bjr parsed, its META date left out: what two runs of the same game must agree on"""
    replay = bjr.read(data)
    if "meta" in replay:
        meta = json.loads(replay["meta"])
        meta.pop("date", None)
        replay["meta"] = meta
    return replay


def compare_bjr(da, db):
    if da == db:
        return None
    try:
        if replay_without_date(da) == replay_without_date(db):
            return None
        return "the replays differ (beyond META's date)"
    except (bjr.BjrError, ValueError) as e:
        return "not a well-formed replay: %s" % e


def sidecar_text(user, name):
    """A sidecar (user/savegame*.bjr) as a fact: its summary, and whether it goes with the save next to it"""
    try:
        replay = bjr.read(read_bytes(os.path.join(user, name)))
    except bjr.BjrError as e:
        return "unreadable (%s)" % e
    save = os.path.join(user, name[:-4] + ".dat")
    if "saveHash" not in replay:
        match = "no save hash"
    elif not os.path.exists(save):
        match = "no save"
    else:
        match = "goes with the save" if bjr.save_hash(read_bytes(save)) == replay["saveHash"] else "does NOT go with the save"
    return "%s; %s" % (bjr.summary(replay), match)


def replay_text(path):
    try:
        return bjr.summary(bjr.read(read_bytes(path)))
    except bjr.BjrError as e:
        return "unreadable (%s)" % e


def fake_replays(outdir):
    """Old replays for the pruning case, each a well-formed file of only its header and END (score = its day),
    written at the time its name says (mtime, UTC midnight of that day): seven of 2000-01-01..07 and one pinned
    ("-keep") of 2000-01-01; a second "-2" of 2000-01-05, written in the same second as the first (by its name
    the newer, though "-2" sorts before "."); and one whose name says 2099 but that was written on 1999-12-31
    (by its mtime the oldest). ReplayRecorder::Prune orders by the mtime, then the name's time and copy number:
    with keep 3, nomoves_normal (whose own replay is pinned) leaves 2000-01-07, 06 and the 05 "-2"."""
    os.makedirs(outdir, exist_ok=True)
    day = lambda d: 946684800 + (d - 1) * 86400  # 2000-01-0d 00:00:00 UTC
    names = [("20000101-000000-normal-1-keep.bjr", 1, day(1))]
    names += [("2000010%d-000000-normal-%d.bjr" % (d, d), d, day(d)) for d in range(1, 8)]
    names += [("20000105-000000-normal-5-2.bjr", 5, day(5)), ("20991231-000000-normal-9.bjr", 9, day(0))]
    for name, score, mtime in names:
        end = struct.pack("<BIiiiQI", 1, 1000, score, 1, 3, 0, 0)
        data = b"BJRP" + struct.pack("<HHI", 1, 0, 1) + b"END " + struct.pack("<I", len(end)) + end
        bjr.read(data)
        path = os.path.join(outdir, name)
        with open(path, "wb") as f:
            f.write(data)
        os.utime(path, (mtime, mtime))


def corrupt_replay(src, dst, kind):
    """A copy of the replay src with one thing broken (the module comment, "corrupt-replay"), written to dst"""
    replay = bjr.read(read_bytes(src))
    events = [(s, i) for s, seg in enumerate(replay["segments"]) for i in range(len(seg["events"]))]

    def find(op, last=False):
        hits = [(s, i) for s, i in events if replay["segments"][s]["events"][i][1] == op]
        if not hits:
            raise ValueError("%s has no %s" % (src, op))
        return hits[-1] if last else hits[0]

    if kind == "check":
        s, i = find("CHECK")
        tick, op, args = replay["segments"][s]["events"][i]
        replay["segments"][s]["events"][i] = (tick, op, [args[0] ^ 1])
        what = "segment %d event %d: CHECK at tick %d with its hash's lowest bit flipped" % (s, i, tick)
    elif kind == "select":
        s, i = find("SELECT")
        replay["segments"][s]["events"].insert(i + 1, replay["segments"][s]["events"][i])
        what = "segment %d event %d: the SELECT at tick %d twice" % (s, i, replay["segments"][s]["events"][i][0])
    elif kind == "unpause":
        s, i = find("UNPAUSE", last=True)
        tick = replay["segments"][s]["events"][i][0]
        del replay["segments"][s]["events"][i]
        what = "segment %d event %d: the last UNPAUSE (tick %d) dropped" % (s, i, tick)
    elif kind == "end":
        replay["end"]["score"] += 1
        what = "END score %d" % replay["end"]["score"]
    elif re.match(r"engine\d+$", kind):
        replay["engineAbi"] = int(kind[len("engine"):])
        what = "engine ABI %d (was %d)" % (replay["engineAbi"], bjr.read(read_bytes(src))["engineAbi"])
    else:
        raise ValueError("unknown corruption " + kind)
    with open(dst, "wb") as f:
        f.write(bjr.write(replay))
    print(what)


# ---------------------------------------------------------------------------------------------------
# Set seed (stage 2): a model of the board RNG, to check a seeded board against its seed

class MTRand:
    """Sexy::MTRand (MT19937, init_genrand seeding; seed 0 plays as 4357); next() is MTRand::Next(),
    31 bits, the value of Sexy::Rand() and Board::Rand()."""
    N, M = 624, 397

    def __init__(self, seed):
        seed &= 0xffffffff
        if seed == 0:
            seed = 4357
        self.mt = [seed] + [0] * (self.N - 1)
        for i in range(1, self.N):
            self.mt[i] = (1812433253 * (self.mt[i - 1] ^ (self.mt[i - 1] >> 30)) + i) & 0xffffffff
        self.mti = self.N

    def next(self):
        mt, n = self.mt, self.N
        if self.mti >= n:
            for k in range(n):
                y = (mt[k] & 0x80000000) | (mt[(k + 1) % n] & 0x7fffffff)
                mt[k] = mt[(k + self.M) % n] ^ (y >> 1) ^ (0x9908b0df if y & 1 else 0)
            self.mti = 0
        y = mt[self.mti]
        self.mti += 1
        y ^= y >> 11
        y ^= (y << 7) & 0x9d2c5680
        y ^= (y << 15) & 0xefc60000
        y ^= y >> 18
        return y & 0x7fffffff


def model_has_match(grid):
    """Board::HasMatch (0x4c9cf5): a run of 3 in a row or a column; None is an empty cell"""
    for lines in (grid, list(zip(*grid))):
        for line in lines:
            color, count = -1, 0
            for v in line:
                if v is None:
                    color = -1
                elif v == color:
                    count += 1
                    if count == 3:
                        return True
                else:
                    color, count = v, 1
    return False


def model_tutorial_move(grid):
    """The constructor's test on Board::FindMove (0x4c9dda, horizontal runs only): a move one column to
    the right in rows 4..6, columns 1..6"""
    for r in range(8):
        for c in range(8):
            for dc, dr in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                c2, r2 = c + dc, r + dr
                if not (0 <= c2 < 8 and 0 <= r2 < 8):
                    continue
                grid[r][c], grid[r2][c2] = grid[r2][c2], grid[r][c]
                color = grid[r][c]
                left = right = c
                while left > 0 and grid[r][left - 1] == color:
                    left -= 1
                while right < 7 and grid[r][right + 1] == color:
                    right += 1
                grid[r][c], grid[r2][c2] = grid[r2][c2], grid[r][c]
                if right - left >= 2 and c2 == c + 1 and 4 <= r <= 6 and 1 <= c < 7:
                    return True
    return False


def model_first_board(seed, colors=7):
    """The grid Board::Board (0x4cb5bc) leaves with the board's own MTRand(seed): FillBoard (0x4cb182; per
    cell, bottom row first: a %240 draw for the start height, then colours until there is no match) until
    the board has the tutorial move. Rows of colour indices."""
    rng = MTRand(seed)
    while True:
        grid = [[None] * 8 for _ in range(8)]
        for r in range(7, -1, -1):
            for c in range(8):
                rng.next()
                while True:
                    grid[r][c] = rng.next() % colors
                    if not model_has_match(grid):
                        break
        if model_tutorial_move(grid):
            return grid


_STREAMS = {}


def global_draw_position(rngseed, value, limit=200000):
    """1-based position of value among the first draws of the global stream MTRand(rngseed), or None"""
    stream, rng = _STREAMS.setdefault(rngseed, ([], MTRand(rngseed)))
    if value in stream:
        return stream.index(value) + 1
    while len(stream) < limit:
        stream.append(rng.next())
        if stream[-1] == value:
            return len(stream)
    return None


def trace_settles(stdout):
    """The settle blocks of a -trace run: update, mode, board tick, rows, seed and kind ("user", "pergame";
    None for a board on the global stream)"""
    settles = []
    for line in stdout:
        m = re.match(r"TRACE update (\d+): (\w+) level \d+ score -?\d+ board tick (\d+)$", line)
        if m:
            settles.append({"update": int(m.group(1)), "mode": m.group(2), "tick": int(m.group(3)), "grid": [],
                            "seed": None, "kind": None})
            continue
        m = re.match(r"TRACE   row (\d+): (.*)$", line)
        if m and settles:
            settles[-1]["grid"].append([int(v) for v in m.group(2).split()])
            continue
        m = re.match(r"TRACE   seed (\d+) (\w+)$", line)
        if m and settles:
            settles[-1]["seed"], settles[-1]["kind"] = int(m.group(1)), m.group(2)
    return settles


def hash_board_starts(run):
    """The updates at which the run's hash.txt shows a new board: the state becomes falling (Board::Board
    ends in FillBoard) straight from one no board refills from (REFILL_STATES): none, playing, swapping back,
    game over (New Game), game over done (play again) or vortex. Not swapping (stage 5: a kept swap falls from
    it); a board a mode switch makes mid-swap is told by its mode (trace_boards). Empty without hash.txt."""
    path = os.path.join(run, "hash.txt")
    starts = []
    last = -1
    for line in (read_lines(path) if os.path.exists(path) else []):
        parts = line.split()
        if len(parts) < 2:
            continue
        state = int(parts[1])
        if state == 0 and last not in REFILL_STATES:
            starts.append(int(parts[0]))
        last = state
    return starts


def trace_boards(stdout, starts=()):
    """The boards of a -trace run in order: a settle starts a new board when one was created since the
    settle before (starts, from hash_board_starts), when the board tick does not grow (a new or a resumed
    board counts from 0, and one board never settles twice in a tick: a board that replaces an untouched
    one on the same seed settles at the same tick), or when the mode or the seed differ from the board
    before (a resumed board settles at tick 1, the next one later). The tick alone misses a new unseeded
    board that settles later than the untouched one it replaces in the same mode (first settles vary,
    ticks 130..155 or so); starts miss a board created while the one before was falling, clearing or
    refilling, which only a mode switch can do (the mode differs): a new board in the same mode comes
    from game over (New Game, play again). Per board its first settle: update, mode, tick, grid, seed,
    kind."""
    boards = []
    last_update = -1
    for settle in trace_settles(stdout):
        last = boards[-1] if boards else None
        created = any(last_update < update <= settle["update"] for update in starts)
        if (last is None or created or settle["tick"] <= last["last tick"] or settle["mode"] != last["mode"] or
                (settle["seed"], settle["kind"]) != (last["seed"], last["kind"])):
            boards.append(dict(settle))
            last = boards[-1]
        last["last tick"] = settle["tick"]
        last_update = settle["update"]
    return boards


def run_boards(run):
    """trace_boards of a run directory, with its hash.txt"""
    return trace_boards(run_stdout(run), hash_board_starts(run))


def parse_flags(flags):
    """cases.txt flags "k=v;k=v" (also the single "ref=<case>" and "hooksonly")"""
    out = {}
    for part in flags.split(";"):
        if part:
            key, _, value = part.partition("=")
            out[key] = value
    return out


def run_stdout(run):
    path = os.path.join(run, "stdout.txt")
    return read_lines(path) if os.path.exists(path) else []


def seed_facts(run, flags):
    """The "seeds" fact, or None when no board of the run has its own RNG"""
    boards = run_boards(run)
    if not any(b["seed"] is not None for b in boards):
        return None
    rngseed = parse_flags(flags).get("rngseed")
    parts = []
    for b in boards:
        if b["seed"] is None:
            parts.append("%d:-" % b["update"])
            continue
        text = "%d:%d/%s" % (b["update"], b["seed"], b["kind"])
        if b["kind"] == "pergame" and rngseed is not None:
            pos = global_draw_position(int(rngseed), b["seed"])
            text += "@%s" % (pos if pos is not None else "?")
        parts.append(text)
    return " ".join(parts)


SEED_BAND = (192, 91, 624, 109)  # x0, y0, x1, y1 (inclusive): between the stats and the tip, 59 updates in
PANEL_COLOR = bytes((21, 0, 56))   # HighScoreWidget::Draw's panel fill


def seed_band_pixels(path):
    """Pixels of a game-over screenshot that are not the panel colour in the SEED line's band"""
    w, _, px = png_pixels(path)
    x0, y0, x1, y1 = SEED_BAND
    return sum(1 for y in range(y0, y1 + 1) for x in range(x0, x1 + 1)
               if px[(y * w + x) * 4:(y * w + x) * 4 + 3] != PANEL_COLOR)


def seed_check(r, cand, run, flags, label):
    """The "seeded" checks (module comment) of one candidate run"""
    before = r.failures
    f = parse_flags(flags)
    boards = run_boards(run)
    kinds = [b["kind"] or "-" for b in boards]
    # "user*": a board that resumed a save of a game on that kind of seed (stage 3: the sidecar keeps its
    # RNG), which settles first at board tick 1 with the loaded grid, not the seed's first board
    expected = f["boards"].split(",") if "boards" in f else []
    if "boards" in f and kinds != [k.rstrip("*") for k in expected]:
        r.fail("%s: boards %s, expected %s" % (label, ",".join(kinds) or "(none)", f["boards"]))
    notes = []
    last_pos = 0
    for i, b in enumerate(boards):
        if b["seed"] is None:
            notes.append("%d: global stream" % b["update"])
            continue
        if i < len(expected) and expected[i].endswith("*"):
            if b["tick"] > 1:
                r.fail("%s: the board of update %d first settles at board tick %d, not as a resumed game" % (label, b["update"], b["tick"]))
            if "gameseed" in f and b["seed"] != int(f["gameseed"]):
                r.fail("%s: the resumed board of update %d has seed %d, not %s" % (label, b["update"], b["seed"], f["gameseed"]))
            notes.append("%d: resumed on its saved MTRand(%d) stream, %s" % (b["update"], b["seed"], b["kind"]))
            continue
        if b["grid"] != model_first_board(b["seed"]):
            r.fail("%s: the board of update %d is not MTRand(%d)'s first board" % (label, b["update"], b["seed"]))
        text = "%d: MTRand(%d)'s board, %s" % (b["update"], b["seed"], b["kind"])
        if b["kind"] == "user" and "gameseed" in f and b["seed"] != int(f["gameseed"]):
            r.fail("%s: the board of update %d has seed %d, -gameseed=%s" % (label, b["update"], b["seed"], f["gameseed"]))
        if b["kind"] == "pergame" and "rngseed" in f:
            pos = global_draw_position(int(f["rngseed"]), b["seed"])
            if pos is None or pos <= last_pos:
                r.fail("%s: the per-game seed %d of update %d is not a later draw of the global stream MTRand(%s)"
                       % (label, b["seed"], b["update"], f["rngseed"]))
            else:
                last_pos = pos
                text += " = global draw %d" % pos
        notes.append(text)
    if "shot" in f:
        path = os.path.join(run, f["shot"])
        n = seed_band_pixels(path) if os.path.exists(path) else -1
        if n <= 0:
            r.fail("%s: %s: no SEED line between the stats and the tip (%d pixels)" % (label, f["shot"], n))
        else:
            notes.append("SEED line in %s (%d pixels)" % (f["shot"], n))
    if "plainshot" in f:
        path = os.path.join(cand, f["plainshot"])
        n = seed_band_pixels(path) if os.path.exists(path) else -1
        if n != 0:
            r.fail("%s: %s: the unseeded game-over screen has %d pixels in the SEED band" % (label, f["plainshot"], n))
        else:
            notes.append("none in the unseeded %s" % f["plainshot"])
    if "differs" in f:
        mine = [b for b in boards if b["seed"] is not None]
        theirs = [b for b in run_boards(os.path.join(cand, f["differs"])) if b["seed"] is not None]
        if not mine or not theirs or mine[0]["grid"] == theirs[0]["grid"]:
            r.fail("%s: the first seeded board does not differ from %s's" % (label, f["differs"]))
        else:
            notes.append("its first board differs from %s's (seed %d)" % (f["differs"], theirs[0]["seed"]))
    if r.failures == before:
        r.out("    seeds: " + "; ".join(notes))
    return r.failures == before


def sameboard_check(r, ref, run, label):
    """Two runs of one -gameseed with different -seed (module comment, "sameboard")"""
    before = r.failures
    compare_exit(r, ref, run, label + ": ")
    la = [mask_wall_clock(x) for x in run_stdout(ref)]
    lb = [mask_wall_clock(x) for x in run_stdout(run)]
    diff = first_list_diff(la, lb, label + ": stdout.txt")
    if diff or not lb:
        r.fail(diff or "%s: no stdout.txt" % label)
    ha, hb = os.path.join(ref, "hash.txt"), os.path.join(run, "hash.txt")
    full_diffs = 0
    if not (os.path.exists(ha) and os.path.exists(hb)):
        r.fail("%s: hash.txt missing" % label)
    else:
        a, b = read_lines(ha), read_lines(hb)
        if len(a) != len(b):
            r.fail("%s: hash.txt %d vs %d lines" % (label, len(a), len(b)))
        for x, y in zip(a, b):
            xa, ya = x.split(), y.split()
            if xa[:3] != ya[:3]:
                r.fail("%s: hash.txt board state or LOGIC differs:\n        %s\n        %s" % (label, x, y))
                break
            full_diffs += xa[3:] != ya[3:]
        if full_diffs == 0:
            r.fail("%s: the FULL hashes never differ: the two runs had the same global stream" % label)
    compare_tree(r, os.path.join(ref, "user"), os.path.join(run, "user"), label + ": user/", lambda p: True)
    if r.failures == before:
        r.out("    %s: stdout.txt (%d lines), the board state and LOGIC hash of %d updates and the user dir identical;"
              " FULL (global RNG state) differs at %d updates: the board's stream ignores -seed"
              % (label, len(lb), len(read_lines(hb)), full_diffs))
    return r.failures == before


def settle_blocks(stdout):
    """The TRACE settle blocks of a run, each as (update, the header without its update and board tick
    numbers followed by the block's other TRACE lines)"""
    blocks = []
    for line in stdout:
        m = re.match(r"TRACE update (\d+): (.*) board tick \d+$", line)
        if m:
            blocks.append((int(m.group(1)), [m.group(2)]))
        elif line.startswith("TRACE   ") and blocks:
            blocks[-1][1].append(line)
    return blocks


def samegrids_check(r, ref, run, flags, label):
    """A run with a dialog that draws from the global stream against the same run without it (module
    comment, "samegrids")"""
    before = r.failures
    f = parse_flags(flags)
    compare_exit(r, ref, run, label + ": ")
    ba, bb = settle_blocks(run_stdout(ref)), settle_blocks(run_stdout(run))
    after = int(f.get("after", "0"))
    if not any(update > after for update, _ in ba):
        r.fail("%s: no settle after update %d" % (label, after))
    diff = first_list_diff(["\n".join(b) for _, b in ba], ["\n".join(b) for _, b in bb], label + ": settle blocks")
    if diff or not bb:
        r.fail(diff or "%s: no settles" % label)
    table = f.get("table", "")
    has_ref, has_run = (os.path.exists(os.path.join(d, "user", table)) for d in (ref, run))
    if not table or not has_ref or has_run:
        r.fail("%s: user/%s %s / %s: expected only with the dialog" % (label, table, has_ref, has_run))
    if r.failures == before:
        r.out("    %s: %d settle blocks identical (%d after update %d, the dialog's end); only %s wrote user/%s:"
              " the dialog's draw did not reach the board's stream"
              % (label, len(bb), sum(1 for update, _ in ba if update > after), after, os.path.basename(ref), table))
    return r.failures == before


def lint_check(r, run):
    before = r.failures
    exit_path = os.path.join(run, "exit.txt")
    status = read_bytes(exit_path).decode("latin-1").strip() if os.path.exists(exit_path) else "missing"
    lines = read_lines(os.path.join(run, "check.txt")) if os.path.exists(os.path.join(run, "check.txt")) else []
    for line in lines:
        if line.startswith(("VIOLATION", "DIFF")):
            r.out("    " + line)
    if status != "0":
        r.fail("the check's exit status %s" % status)
    r.out("    " + (lines[-1] if lines else "(no output)"))
    return r.failures == before


def livesplit_fields(words):
    """The block's fields of a -livesplitlog line (LiveSplitBlock.h's FormatLiveSplitBlock) as ints"""
    f = {}
    for w in words:
        key, _, value = w.partition("=")
        if key in ("go", "lc", "over"):
            a, b = value.split("/")
            f[key], f[key + "Serial"] = int(a), int(b)
        elif key == "deaths":
            f["noMoves"], f["timeUp"], f["newGame"], f["other"] = (int(x) for x in value.split(","))
        elif key == "flags":
            f[key] = int(value, 16)
        elif key not in ("rules", "self", "qpc", "qpcf"):
            f[key] = int(value)
    return f


def livesplit_log_check(r, run, label):
    """The block's event log against hash.txt (CODE_MODS_DESIGN.md 4.15, test 2)"""
    before = r.failures
    log_path, hash_path = os.path.join(run, "livesplit.txt"), os.path.join(run, "hash.txt")
    if not (os.path.exists(log_path) and os.path.exists(hash_path)):
        r.fail("%s: livesplit.txt or hash.txt missing" % label)
        return False
    states = []
    for line in read_lines(hash_path):
        parts = line.split()
        if len(parts) >= 2:
            states.append((int(parts[0]), int(parts[1])))
    state_at = dict(states)
    enter = {7: [], 5: []}
    for i, (u, st) in enumerate(states):
        if st in enter and (i == 0 or states[i - 1][1] != st):
            enter[st].append(u)
    prev_state = {u: states[i - 1][1] if i else -1 for i, (u, _) in enumerate(states)}
    lines = []
    for line in read_lines(log_path):
        words = line.split()
        if words and words[0].startswith("u="):
            lines.append((int(words[0][2:]), livesplit_fields(words[1:])))
    problems = []
    clears, overs, loads, causes = [], [], [], {}
    boards = {}  # serial -> {"resumed", "replay", "go", "max tick", "over dsmax"}
    prev = None
    for n, (u, f) in enumerate(lines):
        where = "livesplit.txt line %d (update %d)" % (n + 1, u)
        if f["seq"] != 2 * (n + 1) or f["updates"] != n + 1:
            problems.append("%s: seq %d, updates %d: not one write per line" % (where, f["seq"], f["updates"]))
        if f["active"] != (0 if n == len(lines) - 1 else 1):
            problems.append("%s: active %d" % (where, f["active"]))
        if f["active"] and state_at.get(u, f["state"]) != f["state"]:
            problems.append("%s: state %d, hash.txt %d" % (where, f["state"], state_at[u]))
        if f["noMoves"] + f["timeUp"] + f["newGame"] + f["other"] != f["over"]:
            problems.append("%s: the death causes do not add up to the game overs" % where)
        b = f["board"]
        info = boards.setdefault(b, {"resumed": False, "replay": False, "go": 0, "tick": -1, "over": None, "dsmax": 0})
        p = prev or {k: 0 for k in f}
        for key in ("go", "lc", "over", "noMoves", "timeUp", "newGame", "other"):
            if f[key] < p[key] or f[key] > p[key] + 1:
                problems.append("%s: %s went from %d to %d" % (where, key, p[key], f[key]))
        new_board = prev is None or b != prev["board"]
        if new_board and prev is not None and b != 0 and b < prev["board"]:
            problems.append("%s: board serial %d after %d" % (where, b, prev["board"]))
        if new_board and (f["blc"] != 0 or (f["gotick"] != -1 and f["go"] == p["go"])):
            problems.append("%s: a new board starts with level clears %d, GO tick %d" % (where, f["blc"], f["gotick"]))
        if f["flags"] & 0x02 and not info["resumed"]:
            info["resumed"] = True
            if not new_board:
                loads.append(u)
        if b:
            info["tick"] = max(info["tick"], f["tick"])
            # a replay's playback board (stage 3, Board::mPlaybackOnly): never a run start
            info["replay"] = info["replay"] or bool(f["flags"] & 0x20)
        if f["go"] > p["go"]:
            info["go"] += 1
            if f["tick"] != 150 or f["gotick"] != 150 or f["goSerial"] != b or f["flags"] & 0x22:
                problems.append("%s: GO at tick %d (GO tick %d, board %d/%d, flags %02x)" % (where, f["tick"], f["gotick"],
                                                                                          f["goSerial"], b, f["flags"]))
        if f["lc"] > p["lc"]:
            clears.append(u)
            if f["lcSerial"] != b or f["blc"] != (0 if new_board else p["blc"]) + 1:
                problems.append("%s: level clear of board %d on board %d, board clears %d" % (where, f["lcSerial"], b, f["blc"]))
        if f["over"] > p["over"]:
            overs.append(u)
            cause = [k for k in ("noMoves", "timeUp", "newGame", "other") if f[k] > p[k]][0]
            causes[cause] = causes.get(cause, 0) + 1
            if f["overSerial"] != b or not f["flags"] & 0x40:
                problems.append("%s: game over of board %d on board %d, flags %02x" % (where, f["overSerial"], b, f["flags"]))
            if (cause == "noMoves" and prev_state.get(u) != 0) or (cause == "timeUp" and prev_state.get(u) != 1):
                problems.append("%s: game over cause %s after state %s" % (where, cause, prev_state.get(u)))
        if b and not new_board:
            if f["dsmax"] < p["dsmax"]:
                problems.append("%s: displayScoreMax went down" % where)
            if info["over"] is None and f["dsmax"] < f["dscore"]:
                problems.append("%s: displayScoreMax %d below the HUD score %d" % (where, f["dsmax"], f["dscore"]))
            if info["over"] is not None and f["dsmax"] != info["over"]:
                problems.append("%s: displayScoreMax moved after the game over" % where)
        if b and f["flags"] & 0x40 and info["over"] is None:
            info["over"] = f["dsmax"]
        prev = f
    if clears != [u for u in enter[7] if u not in loads]:
        problems.append("level clears at updates %s, hash.txt enters the vortex at %s (loads at %s)" % (clears, enter[7], loads))
    if overs != enter[5]:
        problems.append("game overs at updates %s, hash.txt enters game over at %s" % (overs, enter[5]))
    watching = os.path.exists(os.path.join(run, "replay.bjr"))  # a -replay run: every board a playback board
    for b, info in sorted(boards.items()):
        want = 1 if (b and not info["resumed"] and not info["replay"] and info["tick"] >= 150) else 0
        if info["go"] != want:
            problems.append("board %d (resumed %s, replay %s, last tick %d): %d GO, expected %d"
                            % (b, info["resumed"], info["replay"], info["tick"], info["go"], want))
        if b and info["replay"] != watching:
            problems.append("board %d: replay flag %s in a run that %s a replay" % (b, info["replay"], "watches" if watching else "watches no"))
    for p in problems[:10]:
        r.fail("%s: %s" % (label, p))
    if len(problems) > 10:
        r.fail("%s: ... %d more" % (label, len(problems) - 10))
    if r.failures == before:
        real = [b for b in boards if b]
        r.out("    %s: %d writes agree with hash.txt: %d boards (%d resumed, %d replay), %d GO, level clears at %s%s, game overs at %s (%s)"
              % (label, len(lines), len(real), sum(boards[b]["resumed"] for b in real), sum(boards[b]["replay"] for b in real),
                 sum(boards[b]["go"] for b in real),
                 clears or "-", " (a LoadGame's vortex at %s not counted)" % loads if [u for u in loads if u in enter[7]] else "",
                 overs or "-", ", ".join("%s %d" % kv for kv in sorted(causes.items())) or "none"))
    return r.failures == before


def livesplit_check(r, oracle, ref, run, label):
    """A run with the livesplit mod against its inert twin, then its event log"""
    before = r.failures
    compare_exit(r, ref, run, label + ": ")
    for stream in ("stdout.txt", "stderr.txt"):
        pa, pb = os.path.join(ref, stream), os.path.join(run, stream)
        if not (os.path.exists(pa) and os.path.exists(pb)):
            r.fail("%s: %s missing" % (label, stream))
            continue
        la, lb = read_lines(pa), read_lines(pb)
        if stream == "stderr.txt":
            la = [x for x in la if not LS_LOG_LINES.search(x)]
            lb = [x for x in lb if not LS_LOG_LINES.search(x)]
        compare_log(r, la, lb, label + ": " + stream)
    keep = lambda p: (p in ("hash.txt", "autoplay.txt") or p.startswith(("user/", "shots/", "snaps/"))) and \
        p != "user/mods-status.json" and not p.startswith("user/livesplit")
    counts = compare_tree(r, ref, run, label + ": ", keep)
    status, ref_status = mods_status(run), mods_status(ref)
    if (status or {}).get("features") != "livesplit" or (ref_status or {}).get("features") != "none":
        r.fail("%s: mods-status.json features %s, the reference's %s" % (label, (status or {}).get("features"),
                                                                       (ref_status or {}).get("features")))
    stderr = read_lines(os.path.join(run, "stderr.txt")) if os.path.exists(os.path.join(run, "stderr.txt")) else []
    if len([x for x in stderr if "livesplit: block at 0x" in x]) != 1:
        r.fail("%s: not one \"livesplit: block at\" line in stderr.txt" % label)
    templates = os.path.join(oracle, "gen", "livesplit-mods", "livesplit", "livesplit")
    names = sorted(x for x in os.listdir(templates) if x.endswith(".lss")) if os.path.isdir(templates) else []
    copied = [x for x in names if os.path.exists(os.path.join(run, "user", "livesplit", x)) and
              read_bytes(os.path.join(run, "user", "livesplit", x)) == read_bytes(os.path.join(templates, x))]
    if not names or copied != names or os.path.exists(os.path.join(ref, "user", "livesplit")):
        r.fail("%s: %d of the mod's %d templates in user/livesplit/; the reference has the folder: %s"
               % (label, len(copied), len(names), os.path.exists(os.path.join(ref, "user", "livesplit"))))
    # nothing else there (a copy goes through "<name>.tmp", renamed once complete)
    folder = os.path.join(run, "user", "livesplit")
    extra = sorted(set(os.listdir(folder)) - set(names)) if os.path.isdir(folder) else []
    if extra:
        r.fail("%s: user/livesplit/ also holds %s" % (label, ", ".join(extra)))
    # No mod signs (Q18): the screenshots also equal those of the same case without any mod
    plain = os.path.join(oracle, "candidate", os.path.basename(run)[:-len(".ls")])
    shots = 0
    if os.path.isdir(plain):
        shots = compare_tree(r, plain, run, label + " (and the run without mods): ", lambda p: p.startswith("shots/")).get("shots", 0)
    if r.failures == before:
        r.out("    %s: exit, stdout.txt, hash.txt, %d user files, %d screenshots, %d snapshots%s identical, stderr.txt"
              " too without its livesplit: and mods: lines; %d templates copied; %s"
              % (label, counts.get("user", 0), counts.get("shots", 0), counts.get("snaps", 0),
                 ", autoplay.txt" if counts.get("autoplay.txt") else "", len(copied),
                 "the %d screenshots equal the run without mods (no mod signs)" % shots if os.path.isdir(plain)
                 else "no run without mods to compare the screenshots with"))
    livesplit_log_check(r, run, "livesplit.txt")
    return r.failures == before


def cli_check(r, run):
    before = r.failures
    path = os.path.join(run, "cli.txt")
    lines = read_lines(path) if os.path.exists(path) else []
    if not lines:
        r.fail("cli.txt missing or empty")
    for line in lines:
        arg, status, message = (line.split("\t") + ["", ""])[:3]
        if status != "2" or "1 to 4294967295" not in message:
            r.fail("%s: exit %s, \"%s\": expected exit 2 and the seed range" % (arg, status, message))
    if r.failures == before:
        r.out("    %d bad -gameseed values refused with exit status 2 before the game starts: %s"
              % (len(lines), " ".join(line.split("\t")[0] for line in lines)))
    return r.failures == before


# ---------------------------------------------------------------------------------------------------
# Comparison

class Report:
    def __init__(self):
        self.lines = []
        self.failures = 0
        self.notes = 0

    def out(self, text=""):
        self.lines.append(text)
        print(text)

    def fail(self, text):
        self.failures += 1
        self.out("    DIFF  " + text)

    def note(self, text):
        self.notes += 1
        self.out("    note  " + text)


def compare_exit(r, a, b, label):
    ea, eb = os.path.join(a, "exit.txt"), os.path.join(b, "exit.txt")
    if not (os.path.exists(ea) and os.path.exists(eb)):
        r.fail("%sexit.txt missing: %s / %s" % (label, os.path.exists(ea), os.path.exists(eb)))
        return
    sa, sb = read_bytes(ea).decode("latin-1").strip(), read_bytes(eb).decode("latin-1").strip()
    if sa != sb:
        r.fail("%sexit status %s / %s" % (label, sa, sb))


def compare_streams(r, a, b, label):
    """stdout.txt and stderr.txt; returns the number of stdout lines."""
    count = 0
    for stream in ("stdout.txt", "stderr.txt"):
        pa, pb = os.path.join(a, stream), os.path.join(b, stream)
        if not (os.path.exists(pa) and os.path.exists(pb)):
            r.fail("%s%s missing: %s / %s" % (label, stream, os.path.exists(pa), os.path.exists(pb)))
            continue
        lb = read_lines(pb)
        compare_log(r, read_lines(pa), lb, label + stream, (a, b))
        if stream == "stdout.txt":
            count = len(lb)
    return count


def compare_tree(r, a, b, label, keep, shift=0, options_box=False):
    """The file trees under a and b (paths for which keep() is true): the same list, then every file.
    Finished replays are matched and compared without their wall-clock date (normal_path, compare_bjr).
    shift: compare_hiscores' (a baseline run from compare.sh's cache). options_box: a baseline's shots of the
    options dialog may differ inside its box (OPTIONS_BOX). Returns {top-level entry: number of files compared}."""
    ma = {normal_path(p): p for p in list_tree(a, keep)}
    mb = {normal_path(p): p for p in list_tree(b, keep)}
    fa, fb = sorted(ma), sorted(mb)
    if fa != fb:
        only_a = [p for p in fa if p not in set(fb)]
        only_b = [p for p in fb if p not in set(fa)]
        r.fail("%sfile tree: baseline only %s, candidate only %s" % (label, only_a, only_b))
    counts = {}
    for name in sorted(set(fa) & set(fb)):
        if name.endswith("/"):
            continue
        top = name.split("/", 1)[0] if "/" in name else name
        counts[top] = counts.get(top, 0) + 1
        pa, pb = os.path.join(a, ma[name]), os.path.join(b, mb[name])
        if name.lower().endswith(".png"):
            diff = compare_png(pa, pb)
            if diff and options_box and os.path.basename(name) in OPTIONS_SHOTS and inside_options_box(diff):
                r.note("%s%s: differs from the baseline's only inside the options dialog (%s): the link taken out"
                       % (label, name, diff))
                diff = None
        elif name.lower().endswith(".bjr"):
            diff = compare_bjr(read_bytes(pa), read_bytes(pb))
        else:
            da, db = read_bytes(pa), read_bytes(pb)
            if da == db:
                diff = None
            elif os.path.basename(name).lower().startswith("hiscores"):
                diff = compare_hiscores(da, db, shift)
                if isinstance(diff, tuple):
                    r.note("%s%s: equal once the day-number fields are masked (%s: runs on both sides of midnight)"
                           % (label, name, diff[1]))
                    diff = None
            elif name.endswith(".txt"):
                la, lb = da.decode("latin-1").splitlines(), db.decode("latin-1").splitlines()
                diff = (first_list_diff(la, lb, "") or first_byte_diff(da, db)).lstrip()
            else:
                diff = first_byte_diff(da, db)
        if diff:
            r.fail("%s%s: %s" % (label, name, diff))
    return counts


def compare_case(r, side, a, b, hook_outputs):
    """Baseline side run a against candidate run b. hook_outputs: compare hash.txt, autoplay.txt and
    snaps/ too (both exes have the hooks and the candidate ran with them)."""
    if not os.path.isdir(a) or not os.path.isdir(b):
        r.fail("vs %s: run directory missing: baseline %s, candidate %s" % (side, os.path.isdir(a), os.path.isdir(b)))
        return
    label = "vs %s: " % side
    compare_exit(r, a, b, label)
    lines = compare_streams(r, a, b, label)
    # a run from compare.sh's cache made days ago has hiscores days that many days behind
    counts = compare_tree(r, a, b, label, tree_filter(hook_outputs), day_shift(a) - day_shift(b), options_box=True)
    summary = ["%d stdout lines" % lines, "%d user files" % counts.get("user", 0),
               "%d screenshots" % counts.get("shots", 0)]
    if cached_info(a) is not None:
        label = "vs %s (cached): " % side
    if hook_outputs:
        if counts.get("snaps"):
            summary.append("%d snapshots" % counts["snaps"])
        if os.path.exists(os.path.join(b, "hash.txt")):
            summary.append("%d hashes" % len(read_lines(os.path.join(b, "hash.txt"))))
        if counts.get("autoplay.txt"):
            summary.append("autoplay.txt")
    else:
        summary.append("hook outputs not compared")
    r.out("    " + label + ", ".join(summary))


def same_run(r, a, b, label, with_shots=True):
    """Cross-check: two candidate runs must match (exit, logs, user dir, screenshots, hash.txt)."""
    before = r.failures
    label += ": "
    compare_exit(r, a, b, label)
    compare_streams(r, a, b, label)
    tops = ("user/", "shots/") if with_shots else ("user/",)
    compare_tree(r, a, b, label, lambda p: p == "hash.txt" or p.startswith(tops))
    for p in (a, b):
        if not os.path.exists(os.path.join(p, "hash.txt")):
            r.fail("%shash.txt missing in %s" % (label, p))
    return r.failures == before


def read_kv(path):
    info = {}
    if os.path.exists(path):
        with open(path, encoding="latin-1") as f:
            for line in f:
                key, _, value = line.rstrip("\n").partition("=")
                info[key] = value
    return info


def read_cases(oracle):
    cases = []
    with open(os.path.join(oracle, "cases.txt"), encoding="latin-1") as f:
        for line in f:
            parts = line.rstrip("\n").split("\t")
            if len(parts) >= 3:
                cases.append((parts[0], parts[1], parts[2], parts[3] if len(parts) > 3 else ""))
    return cases


def select_cases(path, globs):
    """compare.sh --cases: <path> (a cases.txt) keeps the cases a glob matches and the cases those are compared
    with, in its order; returns their names"""
    with open(path, encoding="latin-1") as f:
        lines = [line for line in f if line.strip()]
    flags = {}
    for line in lines:
        parts = line.rstrip("\n").split("\t")
        flags[parts[0]] = parts[3] if len(parts) > 3 else ""
    patterns = [g for g in globs.split(",") if g]
    todo = [name for name in flags if any(fnmatch.fnmatchcase(name, g) for g in patterns)]
    picked = set()
    while todo:
        name = todo.pop()
        if name in picked:
            continue
        picked.add(name)
        # a case its flags name ("ref=x", "trace=x,y", "watch=x"), and the case its name extends (x.plain -> x)
        todo.extend(t for t in re.split(r"[;,=\s]+", flags[name]) if t in flags)
        parts = name.split(".")
        todo.extend(p for p in (".".join(parts[:i]) for i in range(1, len(parts))) if p in flags)
    kept = [line for line in lines if line.split("\t")[0] in picked]
    with open(path, "w", encoding="latin-1", newline="\n") as f:
        f.writelines(kept)
    return [line.split("\t")[0] for line in kept]


def partial_of(oracle):
    """The --cases globs of a partial oracle (compare.sh writes partial.txt), or None"""
    path = os.path.join(oracle, "partial.txt")
    return read_lines(path)[0] if os.path.exists(path) else None


# ---------------------------------------------------------------------------------------------------
# Expectations

def savegames(run):
    """The savegame*.dat files in a run's user dir (savegame-<RulesId>.dat for a gameplay mod)."""
    return list_tree(os.path.join(run, "user"), lambda p: re.fullmatch(r"savegame[^/]*\.dat", p) is not None)


def derive_facts(run, kind, flags=""):
    """The facts of a candidate run, as "key value" lines (see the module comment)."""
    facts = []
    exit_path = os.path.join(run, "exit.txt")
    facts.append("exit " + (read_bytes(exit_path).decode("latin-1").strip() if os.path.exists(exit_path) else "missing"))
    stderr = read_lines(os.path.join(run, "stderr.txt")) if os.path.exists(os.path.join(run, "stderr.txt")) else []
    mods = [line.split("mods: ", 1)[1] for line in stderr if line.startswith("INFO: mods: ")]
    facts.append("mods " + (mods[0] if mods else "(no mods line)"))
    user = sorted(normal_path(p) for p in list_tree(os.path.join(run, "user"), lambda p: True))
    facts.append("user " + (" ".join(user) if user else "(empty)"))

    # a bot run (also a seeded one) is summed up: its settles depend on every move it makes; so is a playback (its
    # settles are the recording's, which compare.py checks)
    bot = (kind == "autoplay") or os.path.exists(os.path.join(run, "autoplay.txt"))
    stdout = read_lines(os.path.join(run, "stdout.txt")) if os.path.exists(os.path.join(run, "stdout.txt")) else []
    settles = []
    for line in stdout:
        m = re.match(r"TRACE update (\d+): (.*)$", line)
        if m:
            settles.append("%s %s" % (m.group(1), m.group(2)))
    if bot or kind == "playback":
        facts.append("settles %d" % len(settles))
        facts.append("last-settle " + (settles[-1] if settles else "none"))
    else:
        facts += ["settle " + s for s in settles]

    hash_path = os.path.join(run, "hash.txt")
    if os.path.exists(hash_path):
        events = []
        last = None
        for line in read_lines(hash_path):
            parts = line.split()
            if len(parts) < 2:
                continue
            state = int(parts[1])
            if state != last and state in EVENT_STATES:
                events.append("%s:%s" % (parts[0], EVENT_STATES[state]))
            last = state
        facts.append("states " + (" ".join(events) if events else "none"))
    else:
        facts.append("states (no hash.txt)")

    for snap in list_tree(os.path.join(run, "snaps"), lambda p: not p.endswith("/")):
        if snap.endswith(".bjr"):  # stage 3: the sidecar a "save" command writes next to a recorded game's snapshot
            facts.append("snap %s %s" % (snap, sidecar_text(os.path.join(run, "snaps"), snap)))
        else:
            facts.append("snap %s %s" % (snap, save_state(os.path.join(run, "snaps", snap))))
    saves = savegames(run)
    facts += ["save %s %s" % (s, save_state(os.path.join(run, "user", s))) for s in saves] or ["save none"]
    # stage 3: the sidecars next to the saves and the finished replays (games/bejeweled/tools/bjr.py)
    user_dir = os.path.join(run, "user")
    for s in list_tree(user_dir, lambda p: re.fullmatch(r"savegame[^/]*\.bjr", p) is not None):
        facts.append("sidecar %s %s" % (s, sidecar_text(user_dir, s)))
    for p in list_tree(user_dir, lambda p: re.fullmatch(r"replays/[^/]+\.bjr", p) is not None):
        facts.append("replay %s %s" % (normal_path(p), replay_text(os.path.join(user_dir, p))))

    status = mods_status(run)
    if status is not None:
        facts.append("status " + status_text(status))
    variant = variant_fact(stdout)
    if variant is not None:
        facts.append("variant " + variant)
    seeds = seed_facts(run, flags)
    if seeds is not None:
        facts.append("seeds " + seeds)

    if bot:
        notes = []
        if os.path.exists(os.path.join(run, "autoplay.txt")):
            notes = [line for line in read_lines(os.path.join(run, "autoplay.txt")) if line.startswith("# update")]
        facts.append("end " + (notes[-1][2:] if notes else "(no notes)"))
    if os.path.exists(os.path.join(run, "codec.txt")):
        lines = read_lines(os.path.join(run, "codec.txt"))
        facts.append("codec " + (lines[-1] if lines else "(empty)"))
    watched = [line for line in stdout if line.startswith("REPLAY end: ") or ": cannot play: " in line]
    if watched:
        facts.append("watch " + watched[-1])
    return facts


def mods_status(run):
    """user/mods-status.json (GameConfig::Load, for rule and feature mods) as a dict, None without one"""
    path = os.path.join(run, "user", "mods-status.json")
    if not os.path.exists(path):
        return None
    try:
        with open(path, encoding="utf-8") as f:
            return json.load(f)
    except ValueError as e:
        return {"unreadable": str(e)}


def status_text(status):
    if "unreadable" in status:
        return "unreadable (%s)" % status["unreadable"]
    ids = lambda key: ",".join(sorted(set(m.get("mod", "") for m in status.get(key, [])))) or "-"
    return ("rulesId %s, scoreKey %s, features %s, %d warnings, errors %s, dropped %s"
            % (str(status.get("rulesId", "?"))[:16], status.get("scoreKey", "?") or "-", status.get("features", "?"),
               len(status.get("warnings", [])), ids("errors"), ids("dropped")))


def inert_check(r, ref, run, label):
    """An inert run against the candidate's reference run: the same play (TRACE lines and hash.txt) and a
    status file whose RulesId -dumprules prints too"""
    before = r.failures
    for name in ("stdout.txt", "hash.txt"):
        pa, pb = os.path.join(ref, name), os.path.join(run, name)
        if not (os.path.exists(pa) and os.path.exists(pb)):
            r.fail("%s: %s missing: %s / %s" % (label, name, os.path.exists(pa), os.path.exists(pb)))
            continue
        # stage 5: a board of rules is a variant board, whose settles add their variant line; the reference's
        # boards have none
        diff = first_list_diff([mask_wall_clock(x) for x in read_lines(pa)],
                               [mask_wall_clock(x) for x in read_lines(pb) if not x.startswith(VARIANT_LINE)], label + ": " + name)
        if diff:
            r.fail(diff)
    status = mods_status(run)
    dumped = {}
    if os.path.exists(os.path.join(run, "dumprules.txt")):
        for line in read_lines(os.path.join(run, "dumprules.txt")):
            key, _, value = line.partition(" = ")
            dumped[key] = value
    if status is None:
        r.fail("%s: no user/mods-status.json" % label)
    elif status.get("rulesId") != dumped.get("rulesId") or status.get("canonical") != dumped.get("canonical"):
        r.fail("%s: mods-status.json rulesId %s, -dumprules %s" % (label, status.get("rulesId"), dumped.get("rulesId")))
    if r.failures == before:
        r.out("    %s: stdout.txt (%d lines) and hash.txt (%d hashes) identical: the mods changed nothing in play beyond"
              " the reference; mods-status.json and -dumprules agree on RulesId %s"
              % (label, len(read_lines(os.path.join(run, "stdout.txt"))), len(read_lines(os.path.join(run, "hash.txt"))),
                 str(status.get("rulesId"))[:16]))
    return r.failures == before


RECORDING_EXTRA = re.compile(r"mods-status\.json|replays/|replays/[^/]+\.bjr|savegame[^/]*\.bjr")
LOG_OF_MODS = ("INFO: mods:", "INFO: game:")


def recording_check(r, ref, run, label):
    """A run with the replays feature mod against the candidate's run of the same case without mods
    (games/bejeweled/docs/CODE_MODS_DESIGN.md 9.1 item 5, "recording is inert"): stdout.txt (TRACE), hash.txt and
    stderr.txt (but the mod's "mods:" lines and the "game:" line naming the mods folder) identical, every file of
    the reference's user dir identical, and the run's user dir has only these more: mods-status.json, replays/
    and the finished replays in it, the sidecars savegame*.bjr. Every .bjr must read (bjr.py), each sidecar must
    go with its save, each snapshot sidecar in snaps/ with its snapshot and end in SAVE(snapshot); with
    -codeccheck, codec.txt must report no failure and one settle per TRACE settle."""
    before = r.failures
    for name in ("stdout.txt", "hash.txt", "stderr.txt"):
        pa, pb = os.path.join(ref, name), os.path.join(run, name)
        if not (os.path.exists(pa) and os.path.exists(pb)):
            r.fail("%s: %s missing: %s / %s" % (label, name, os.path.exists(pa), os.path.exists(pb)))
            continue
        la = [x for x in read_lines(pa) if not x.startswith(LOG_OF_MODS)]
        lb = [x for x in read_lines(pb) if not x.startswith(LOG_OF_MODS)]
        compare_log(r, la, lb, label + ": " + name)
    ua, ub = os.path.join(ref, "user"), os.path.join(run, "user")
    fa = list_tree(ua, lambda p: True)
    fb = list_tree(ub, lambda p: True)
    for p in fa:
        if p not in fb:
            r.fail("%s: user/%s is missing" % (label, p))
        # a reference's own sidecar (a seeded game) gains the recording; a reference with mods has a status of its own
        elif not p.endswith("/") and not p.endswith(".bjr") and p != "mods-status.json":
            da, db = read_bytes(os.path.join(ua, p)), read_bytes(os.path.join(ub, p))
            diff = None
            if da != db:
                diff = compare_hiscores(da, db) if os.path.basename(p).startswith("hiscores") else first_byte_diff(da, db)
                if isinstance(diff, tuple):
                    diff = None
            if diff:
                r.fail("%s: user/%s: %s" % (label, p, diff))
    extra = [p for p in fb if p not in fa]
    for p in extra:
        if not RECORDING_EXTRA.fullmatch(p):
            r.fail("%s: user/%s is new, and not a replay, a sidecar or mods-status.json" % (label, p))
    if "mods-status.json" not in fb:
        r.fail("%s: no user/mods-status.json" % label)
    replays = [p for p in fb if p.endswith(".bjr")]
    for p in replays:
        text = sidecar_text(ub, p) if p.startswith("savegame") else replay_text(os.path.join(ub, p))
        if "unreadable" in text or "NOT" in text or "no save" in text:
            r.fail("%s: user/%s: %s" % (label, p, text))
    # snapshots: the reference's identical; a sidecar next to each, for its snapshot
    sa, sb = os.path.join(ref, "snaps"), os.path.join(run, "snaps")
    snaps_a = list_tree(sa, lambda p: True)
    snaps_b = list_tree(sb, lambda p: True)
    for p in snaps_a:
        if p not in snaps_b or read_bytes(os.path.join(sa, p)) != read_bytes(os.path.join(sb, p)):
            r.fail("%s: snaps/%s differs or is missing" % (label, p))
    snap_sidecars = [p for p in snaps_b if p not in snaps_a]
    for p in snap_sidecars:
        if not p.endswith(".bjr"):
            r.fail("%s: snaps/%s is new, and not a sidecar" % (label, p))
            continue
        try:
            replay = bjr.read(read_bytes(os.path.join(sb, p)))
            dat = read_bytes(os.path.join(sb, p[:-4] + ".dat"))
            last = replay["segments"][-1]["events"][-1] if replay["segments"] and replay["segments"][-1]["events"] else None
            if replay.get("saveHash") != bjr.save_hash(dat) or last is None or last[1] != "SAVE" or last[2] != [2, bjr.save_hash(dat)]:
                r.fail("%s: snaps/%s does not end in SAVE(snapshot) of %s" % (label, p, p[:-4] + ".dat"))
        except (OSError, bjr.BjrError) as e:
            r.fail("%s: snaps/%s: %s" % (label, p, e))
    codec = os.path.join(run, "codec.txt")
    codec_note = ""
    if os.path.exists(codec):
        lines = read_lines(codec)
        settles = len(trace_settles(run_stdout(run)))
        m = re.fullmatch(r"codeccheck: (\d+) updates checked, (\d+) settles, (\d+) failures", lines[-1] if lines else "")
        if not m or m.group(3) != "0" or int(m.group(2)) != settles:
            r.fail("%s: codec.txt: %s (%d TRACE settles)" % (label, lines[-1] if lines else "empty", settles))
            for line in lines:
                if line.startswith("FAIL"):
                    r.out("          " + line)
                    break
        else:
            codec_note = "; StateCodec round trip after each of %s updates" % m.group(1)
    if r.failures == before:
        r.out("    %s: TRACE, hash.txt, logs and the user files identical; new: %s; %d .bjr files read, the sidecars go with"
              " their saves, %d snapshot sidecars%s" % (label, " ".join(normal_path(p) for p in extra), len(replays),
                                                        len(snap_sidecars), codec_note))
    return r.failures == before


def variant_settles(stdout):
    """The settles of a -trace run with what stages 5 to 7 check: update, mode, level (the HUD's), score, board tick,
    grid, seed, the variant line's counts (None without one; undos 0 when the line has none) and the number of move
    lines"""
    settles = []
    for line in stdout:
        m = re.match(r"TRACE update (\d+): (\w+) level (\d+) score (-?\d+) board tick (\d+)$", line)
        if m:
            settles.append({"update": int(m.group(1)), "mode": m.group(2), "level": int(m.group(3)), "score": int(m.group(4)),
                            "tick": int(m.group(5)),
                            "grid": [], "seed": None, "moves": None, "invalid": None, "undos": 0, "gravity": None,
                            "flips": 0, "dirt": None, "side": None, "found": 0, "movelist": [], "typed": [], "groups": None,
                            "made": None, "specials": None, "gold": None, "time": None, "goldrows": None})
        elif settles and line.startswith("TRACE   row "):
            settles[-1]["grid"].append([int(v) for v in line.split(":", 1)[1].split()])
        elif settles and line.startswith("TRACE   seed "):
            settles[-1]["seed"] = line[len("TRACE   seed "):]
        elif settles and line.startswith(VARIANT_LINE):
            m = re.match(r"TRACE   variant moves (-?\d+) invalid (-?\d+)(?: undos (\d+))?(?: gravity (up|down) flips (\d+))?"
                         r"(?: dirt (\d+) (left|right))?(?: groups L (\d+) T (\d+) plus (\d+) diagonal (\d+) shapes (\d+))?"
                         r"(?: specials flame (\d+) lightning (\d+) blasts (\d+))?"
                         r"(?: gold (\d+)/(\d+)(?: time (\d+)/(\d+))?)?$", line)
            if m:
                settles[-1]["moves"], settles[-1]["invalid"] = int(m.group(1)), int(m.group(2))
                settles[-1]["undos"] = int(m.group(3) or 0)
                settles[-1]["gravity"], settles[-1]["flips"] = m.group(4), int(m.group(5) or 0)
                settles[-1]["dirt"], settles[-1]["side"] = (int(m.group(6)), m.group(7)) if m.group(6) else (None, None)
                if m.group(8):
                    settles[-1]["groups"] = dict(zip(MATCH_KINDS, (int(v) for v in m.group(8, 9, 10, 11, 12))))
                if m.group(13):
                    settles[-1]["made"] = tuple(int(v) for v in m.group(13, 14, 15))
                if m.group(16):
                    settles[-1]["gold"] = (int(m.group(16)), int(m.group(17)))
                    settles[-1]["time"] = (int(m.group(18)), int(m.group(19))) if m.group(18) else None
        elif settles and line.startswith("TRACE   specials:"):
            # stage 11: the specials on the board, row-major
            settles[-1]["specials"] = [(k, int(c), int(w)) for k, c, w in re.findall(r"(flame|lightning) \((\d+),(\d+)\)", line)]
        elif settles and line.startswith("TRACE   gold:"):
            # stage 12: a word a row: 1 gold, 0 a cell of the goal not yet gold, . outside the goal, x gold outside it
            settles[-1]["goldrows"] = line.split(":", 1)[1].split()
        elif settles and line.startswith("TRACE   move "):
            settles[-1]["found"] += 1
            m = re.match(r"TRACE   move \d+: \((-?\d+),(-?\d+)\)->\((-?\d+),(-?\d+)\)", line)
            if m:
                settles[-1]["movelist"].append(tuple(int(v) for v in m.groups()))
                c, w, c2, w2 = settles[-1]["movelist"][-1]
                settles[-1]["typed"].append(("swap", c, w, c2, w2, (c, w), (c2, w2)))
                continue
            # Stage 10's twists and slides (BoardTrace.cpp PrintVariantMoves): the move, where the gem that makes the
            # group lands and where it comes from
            m = re.match(r"TRACE   move \d+: twist \((\d+),(\d+)\) (cw|ccw) lands \((\d+),(\d+)\) from \((\d+),(\d+)\)", line)
            if m:
                v = [int(x) if x.isdigit() else x for x in m.groups()]
                settles[-1]["typed"].append(("twist", v[0], v[1], 1 if v[2] == "cw" else -1, (v[3], v[4]), (v[5], v[6])))
                continue
            m = re.match(r"TRACE   move \d+: slide (row|col) (\d+) from (\d+) length (\d+) by (\d+) lands \((\d+),(\d+)\) from \((\d+),(\d+)\)", line)
            if m:
                v = [int(x) if x.isdigit() else x for x in m.groups()]
                settles[-1]["typed"].append(("slide", v[0], v[1], v[2], v[3], v[4], (v[5], v[6]), (v[7], v[8])))
    return settles


# Stage 9: the group kinds the variant line counts, and match.shapes presets (GameRules.cpp PresetCells): cells (x, y), rotations, mirror
MATCH_KINDS = ("L", "T", "plus", "diagonal", "shapes")
MATCH_PRESETS = {"square": ([(0, 0), (1, 0), (0, 1), (1, 1)], False, False),
                 "L3": ([(0, 0), (0, 1), (1, 1)], True, True),
                 "T4": ([(0, 0), (1, 0), (2, 0), (1, 1)], True, True)}


def match_rules(spec):
    """Stage 9: the match rules of the mod in mods/<spec> that has rules.match, as MatchFinder reads them: (minRun,
    diagonal minRun or None, a list of each shape's orientations, each a list of (x, y))"""
    root = os.path.join(os.path.dirname(os.path.abspath(__file__)), "mods", spec)
    match = None
    for mod in sorted(os.listdir(root)):
        path = os.path.join(root, mod, "mod.json")
        if os.path.exists(path):
            with open(path, encoding="utf-8") as fh:
                match = json.load(fh).get("rules", {}).get("match", match)
    if match is None:
        return None
    min_run = match.get("minRun", 3)
    diagonal = match.get("diagonalMinRun", min_run) if match.get("diagonal", False) else None
    shapes = []
    for sid in sorted(match.get("shapes", {})):
        shape = match["shapes"][sid]
        if "preset" in shape:
            cells, rotations, mirror = MATCH_PRESETS[shape["preset"]]
        else:
            cells, rotations, mirror = [tuple(c) for c in shape["cells"]], shape.get("rotations", False), shape.get("mirror", False)
        seen = set()
        for flip in (range(2) if mirror else range(1)):
            turned = [(-x, y) for x, y in cells] if flip else list(cells)
            for _ in range(4 if rotations else 1):
                lx, ly = min(x for x, _ in turned), min(y for _, y in turned)
                seen.add(tuple(sorted((x - lx, y - ly) for x, y in turned)))
                turned = [(-y, x) for x, y in turned]
        shapes.append(sorted(seen))
    return min_run, diagonal, shapes


def match_in_group(grid, rules, col, row):
    """Stage 9: whether some group of the rules holds the cell (MatchFinder::InGroup with both directions)"""
    min_run, diagonal, shapes = rules
    rows, cols = len(grid), len(grid[0])

    def at(c, r):
        return grid[r][c] if 0 <= c < cols and 0 <= r < rows and grid[r][c] >= 0 else -1

    color = at(col, row)
    if color < 0:
        return False
    lines = [((1, 0), min_run), ((0, 1), min_run)]
    if diagonal is not None:
        lines += [((1, 1), diagonal), ((-1, 1), diagonal)]
    for (dc, dr), need in lines:
        count = 1
        for sign in (1, -1):
            i = 1
            while at(col + sign * i * dc, row + sign * i * dr) == color:
                count += 1
                i += 1
        if count >= need:
            return True
    for orientations in shapes:
        for cells in orientations:
            for dx, dy in cells:
                if all(at(col - dx + x, row - dy + y) == color for x, y in cells):
                    return True
    return False


def match_check(r, settles, spec, kinds, label, notes):
    """Stage 9, "match=<dir>" and "kinds=<kind>,..." (module comment): no group in a settled grid; the move lines are
    the brute force's; the board scored groups of those kinds"""
    rules = match_rules(spec)
    if rules is None:
        r.fail("%s: match=%s: no mod there has rules.match" % (label, spec))
        return
    moves = 0
    for s in settles:
        grid = s["grid"]
        if not grid:
            continue
        rows, cols = len(grid), len(grid[0])
        found = [(c, w) for w in range(rows) for c in range(cols) if match_in_group(grid, rules, c, w)]
        if found:
            r.fail("%s: the settled grid of update %d holds a group at %s" % (label, s["update"], found[:4]))
            return
        expected = []
        for w in range(rows):
            for c in range(cols):
                if grid[w][c] < 0:
                    continue
                for dc, dr in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                    c2, w2 = c + dc, w + dr
                    if not (0 <= c2 < cols and 0 <= w2 < rows) or grid[w2][c2] < 0:
                        continue
                    grid[w][c], grid[w2][c2] = grid[w2][c2], grid[w][c]
                    if match_in_group(grid, rules, c, w):
                        expected.append((c, w, c2, w2))
                    grid[w][c], grid[w2][c2] = grid[w2][c2], grid[w][c]
        if s["movelist"] != expected:
            r.fail("%s: the moves of update %d are %s, the brute force's %s" % (label, s["update"], s["movelist"][:6], expected[:6]))
            return
        moves += len(expected)
    most = {k: max([s["groups"][k] for s in settles if s["groups"]] or [0]) for k in MATCH_KINDS}
    for kind in [k for k in kinds.split(",") if k]:
        if kind not in most:
            r.fail("%s: kinds=%s: %s is none of %s" % (label, kinds, kind, ", ".join(MATCH_KINDS)))
            return
        if most[kind] == 0:
            r.fail("%s: no settle's variant line counts a group of kind %s" % (label, kind))
            return
    notes.append("match=%s: no group left in %d settled grids, their %d moves the brute force's; groups scored (most on a board) %s"
                 % (spec, len(settles), moves, " ".join("%s %d" % (k, most[k]) for k in MATCH_KINDS)))


def moveset_moves(grid, rules, types, both):
    """Stage 10: the moves of a settled grid in MoveSet's order (MoveSet.h), found by brute force: the swaps that put
    the gem at the first cell into a group, then the twists of each block of four gems (row-major, clockwise then
    anticlockwise with both), then the slides of each full segment of the rows and then the columns (holes, -1,
    break the segments) by 1 to length - 1 cells. A twist or slide counts once, at the first of its moved gems
    (MovedCells' order) that lands in a group. Each move as the trace's lines parse (variant_settles)"""
    rows, cols = len(grid), len(grid[0])
    moves = []

    def lands(moved):
        g = [list(row) for row in grid]
        for (fc, fw), (tc, tw) in moved:
            g[tw][tc] = grid[fw][fc]
        for frm, to in moved:
            if match_in_group(g, rules, to[0], to[1]):
                return to, frm
        return None

    if "swap" in types:
        for w in range(rows):
            for c in range(cols):
                if grid[w][c] < 0:
                    continue
                for dc, dw in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                    c2, w2 = c + dc, w + dw
                    if not (0 <= c2 < cols and 0 <= w2 < rows) or grid[w2][c2] < 0:
                        continue
                    grid[w][c], grid[w2][c2] = grid[w2][c2], grid[w][c]
                    if match_in_group(grid, rules, c, w):
                        moves.append(("swap", c, w, c2, w2, (c, w), (c2, w2)))
                    grid[w][c], grid[w2][c2] = grid[w2][c2], grid[w][c]
    if "twist" in types:
        for w in range(rows - 1):
            for c in range(cols - 1):
                corners = [(c, w), (c + 1, w), (c + 1, w + 1), (c, w + 1)]
                if any(grid[cw][cc] < 0 for cc, cw in corners):
                    continue
                for d in ((1, -1) if both else (1,)):
                    hit = lands([(corners[i], corners[(i + (1 if d > 0 else 3)) % 4]) for i in range(4)])
                    if hit:
                        moves.append(("twist", c, w, d) + hit)
    if "slide" in types:
        for axis, line, start, cells in full_segments(grid):
            n = len(cells)
            for k in range(1, n):
                hit = lands([(cells[i], cells[(i + k) % n]) for i in range(n)])
                if hit:
                    moves.append(("slide", axis, line, start, n, k) + hit)
    return moves


def full_segments(grid):
    """Stage 10: the segments of the rows and then the columns (runs of cells between holes, -1) full of gems, as
    (axis, line, start, cells)"""
    rows, cols = len(grid), len(grid[0])
    for axis, lines, length in (("row", rows, cols), ("col", cols, rows)):
        for line in range(lines):
            cells = [(i, line) if axis == "row" else (line, i) for i in range(length)]
            start = 0
            while start < length:
                if grid[cells[start][1]][cells[start][0]] == -1:
                    start += 1
                    continue
                end = start
                while end < length and grid[cells[end][1]][cells[end][0]] != -1:
                    end += 1
                if all(grid[w][c] >= 0 for c, w in cells[start:end]):
                    yield axis, line, start, cells[start:end]
                start = end


def made_move(grid0, grid1, types, both):
    """Stage 10: whether one move of those types, with or without a group, turns grid0 into grid1 (a kept invalid
    move): a swap of neighbours, a twist of a block of four gems, a slide of a full segment"""
    rows, cols = len(grid0), len(grid0[0])
    if len(grid1) != rows or any(len(row) != cols for row in grid1):
        return False

    def made(moved):
        g = [list(row) for row in grid0]
        for (fc, fw), (tc, tw) in moved:
            g[tw][tc] = grid0[fw][fc]
        return g == grid1

    gem = lambda c, w: 0 <= c < cols and 0 <= w < rows and grid0[w][c] >= 0
    if "swap" in types and swapped_cells(grid0, grid1) is not None:
        return True
    if "twist" in types:
        for w in range(rows - 1):
            for c in range(cols - 1):
                corners = [(c, w), (c + 1, w), (c + 1, w + 1), (c, w + 1)]
                if all(gem(cc, cw) for cc, cw in corners) and any(
                        made([(corners[i], corners[(i + (1 if d > 0 else 3)) % 4]) for i in range(4)]) for d in ((1, -1) if both else (1,))):
                    return True
    if "slide" in types:
        for axis, line, start, cells in full_segments(grid0):
            n = len(cells)
            if any(made([(cells[i], cells[(i + k) % n]) for i in range(n)]) for k in range(1, n)):
                return True
    return False


def moves_check(r, settles, types, both, spec, label, notes):
    """Stage 10, "moves=<type>+..." (module comment): no group of three in a settled grid, and the move lines are, in
    order, MoveSet's (moveset_moves); each of the types has a line at some settle"""
    types = types.split("+")
    rules = match_rules(spec) if spec else (3, None, [])
    counts = dict((t, 0) for t in ("swap", "twist", "slide"))
    for s in settles:
        grid = s["grid"]
        if not grid:
            continue
        rows, cols = len(grid), len(grid[0])
        found = [(c, w) for w in range(rows) for c in range(cols) if match_in_group(grid, rules, c, w)]
        if found:
            r.fail("%s: the settled grid of update %d holds a group at %s" % (label, s["update"], found[:4]))
            return
        expected = moveset_moves(grid, rules, types, both)
        if s["typed"] != expected or s["found"] != len(expected):
            r.fail("%s: the moves of update %d are %s, the brute force's %s" % (label, s["update"], s["typed"][:4], expected[:4]))
            return
        for m in expected:
            counts[m[0]] += 1
    missing = [t for t in types if counts.get(t, 0) == 0]
    if missing:
        r.fail("%s: moves=%s: no settle offered a move of type %s" % (label, "+".join(types), ", ".join(missing)))
        return
    notes.append("moves=%s%s: %d settled grids without a group, their moves MoveSet's (%s)"
                 % ("+".join(types), " both ways" if both else "", len(settles), ", ".join("%s %d" % (t, counts[t]) for t in types)))


def specials_check(r, cand, settles, spec, after, label, notes):
    """Stage 11, "specials=F,L,B" (module comment): every settle's variant line counts the specials made and the blasts,
    and its specials line lists specials on gems only, no more than were made; some board made at least F flame gems
    and L lightning gems and set off B blasts. "after=<case>": the first settle is that case's last (a resume)"""
    try:
        least = [int(v) for v in spec.split(",")]
    except ValueError:
        least = []
    if len(least) != 3:
        r.fail("%s: specials=%s: not F,L,B" % (label, spec))
        return
    most = [0, 0, 0]
    on_board = 0
    for s in settles:
        if s["made"] is None or s["specials"] is None:
            r.fail("%s: the settle of update %d has no specials counts or no specials line" % (label, s["update"]))
            return
        for kind, c, w in s["specials"]:
            if not (0 <= w < len(s["grid"]) and 0 <= c < len(s["grid"][w])) or s["grid"][w][c] < 0:
                r.fail("%s: the settle of update %d lists a %s gem at (%d,%d), where there is no gem" % (label, s["update"], kind, c, w))
                return
        if len(s["specials"]) > s["made"][0] + s["made"][1] and after is None:
            r.fail("%s: the settle of update %d lists %d specials, and the board made %d" % (label, s["update"], len(s["specials"]),
                                                                                    s["made"][0] + s["made"][1]))
            return
        most = [max(most[i], s["made"][i]) for i in range(3)]
        on_board = max(on_board, len(s["specials"]))
    for i, what in enumerate(("flame gems made", "lightning gems made", "blasts")):
        if most[i] < least[i]:
            r.fail("%s: at most %d %s on a board, expected at least %d" % (label, most[i], what, least[i]))
            return
    if after is not None:
        before = variant_settles(run_stdout(os.path.join(cand, after)))
        if not before or (before[-1]["grid"], before[-1]["specials"]) != (settles[0]["grid"], settles[0]["specials"]):
            r.fail("%s: the first settle's grid and specials are not those of %s's last settle (%s / %s)" %
                   (label, after, before[-1]["specials"] if before else "none", settles[0]["specials"]))
            return
        notes.append("the first settle is %s's last, with its %d specials" % (after, len(settles[0]["specials"])))
    notes.append("specials: flame %d lightning %d blasts %d (most on a board), up to %d on the board, each on a gem" %
                 (most[0], most[1], most[2], on_board))


def gold_check(r, cand, run, settles, spec, timeup, after, label, notes):
    """Stage 12, "gold=G,L" (module comment): every settle's gold counts agree with its gold line, no gold lies outside
    the goal, the gold stays within a level (but for an undo) and its time runs down, a level ends on a settle with all
    of its gold and the next one starts with none; some settle has G gold cells or more and the HUD reaches level L.
    "timeup": the bot's game over, the last settle with a time limit and gold left. "after=<case>": the first settle is that case's last (a
    resume), with no more time left"""
    try:
        least, level = [int(v) for v in spec.split(",")]
    except ValueError:
        r.fail("%s: gold=%s: not G,L" % (label, spec))
        return
    prev = None
    levels_done = 0
    for s in settles:
        if s["gold"] is None or s["goldrows"] is None:
            r.fail("%s: the settle of update %d has no gold counts or no gold line" % (label, s["update"]))
            return
        rows = s["goldrows"]
        done, goal = s["gold"]
        if len(rows) != len(s["grid"]) or any(len(w) != len(g) for w, g in zip(rows, s["grid"])):
            r.fail("%s: the settle of update %d: the gold line %s is not the grid's shape" % (label, s["update"], " ".join(rows)))
            return
        text = "".join(rows)
        if "x" in text or (text.count("1"), text.count("1") + text.count("0")) != (done, goal):
            r.fail("%s: the settle of update %d: gold %d/%d, and its line %s" % (label, s["update"], done, goal, " ".join(rows)))
            return
        if s["time"] is not None and not (0 <= s["time"][0] <= s["time"][1]):
            r.fail("%s: the settle of update %d: time %d/%d" % (label, s["update"], s["time"][0], s["time"][1]))
            return
        if prev is not None and s["level"] == prev["level"] and s["undos"] == prev["undos"]:
            kept = all(b == "1" for a, b in zip("".join(prev["goldrows"]), text) if a == "1")
            if not kept or (s["time"] is not None and prev["time"] is not None and s["time"][0] > prev["time"][0]):
                r.fail("%s: level %d, from update %d to %d: the gold %s -> %s, the time %s -> %s" %
                       (label, s["level"], prev["update"], s["update"], " ".join(prev["goldrows"]), " ".join(rows), prev["time"], s["time"]))
                return
        if prev is not None and s["level"] > prev["level"]:
            if prev["gold"][0] != prev["gold"][1] or done != 0:
                r.fail("%s: level %d -> %d at update %d: the last settle's gold %d/%d, the next level's first %d" %
                       (label, prev["level"], s["level"], s["update"], prev["gold"][0], prev["gold"][1], done))
                return
            levels_done += 1
        prev = s
    if not settles:
        r.fail("%s: no settle" % label)
        return
    most = max(s["gold"][0] for s in settles)
    reached = max(s["level"] for s in settles)
    if most < least or reached < level:
        r.fail("%s: at most %d gold cells on a board and level %d, expected at least %d and level %d" % (label, most, reached, least, level))
        return
    last = settles[-1]
    if timeup is not None:
        log = os.path.join(run, "autoplay.txt")
        over = os.path.exists(log) and any(": game over" in line for line in read_bytes(log).decode("latin-1").splitlines())
        if not over or last["time"] is None or last["gold"][0] == last["gold"][1]:
            r.fail("%s: %s; the last settle has gold %d/%d and time %s, expected TIME UP" %
                   (label, "the bot's game over" if over else "no game over in autoplay.txt", last["gold"][0], last["gold"][1], last["time"]))
            return
    if after is not None:
        before = variant_settles(run_stdout(os.path.join(cand, after)))
        b = before[-1] if before else None
        first = settles[0]
        if b is None or (b["grid"], b["goldrows"]) != (first["grid"], first["goldrows"]) or \
                ((b["time"] is None) != (first["time"] is None)) or (b["time"] is not None and first["time"][0] > b["time"][0]):
            r.fail("%s: the first settle's grid, gold and time are not those of %s's last settle (%s %s / %s %s)" %
                   (label, after, " ".join(b["goldrows"]) if b else "none", b["time"] if b else "", " ".join(first["goldrows"]), first["time"]))
            return
        notes.append("the first settle is %s's last, with its %d gold cells%s" % (after, first["gold"][0],
                                                                                 ", %d ticks left" % first["time"][0] if first["time"] else ""))
    timed = [s["time"] for s in settles if s["time"] is not None]
    notes.append("gold: up to %d cells on a board, %d levels done, level %d reached%s%s" %
                 (most, levels_done, reached, ", limits %s" % sorted(set(t[1] for t in timed)) if timed else "",
                  ", TIME UP with %d/%d" % last["gold"] if timeup is not None else ""))


def shapes_check(r, settles, spec, label, notes):
    """Stage 7, "shapes=<cols>x<rows>:<holes>,...": the shapes of board.levels in order (one for a board of one
    shape). Every settle's grid has the size of its level's shape ((HUD level - 1) % shapes) and that many holes
    (-1 cells: a settled board has a gem in every playable cell; a dirt cell, stage 7b, is -1 - its layers), the same
    cells at every settle of that shape"""
    shapes = []
    for part in spec.split(","):
        m = re.match(r"(\d+)x(\d+):(\d+)$", part)
        if not m:
            r.fail("%s: shapes=%s: not <cols>x<rows>:<holes>,..." % (label, spec))
            return
        shapes.append((int(m.group(1)), int(m.group(2)), int(m.group(3))))
    holes_of, seen = {}, {}
    for s in settles:
        i = (s["level"] - 1) % len(shapes)
        cols, rows, count = shapes[i]
        grid = s["grid"]
        if len(grid) != rows or any(len(row) != cols for row in grid):
            r.fail("%s: the settle of update %d (level %d) is %dx%d, its shape %d is %dx%d" %
                   (label, s["update"], s["level"], len(grid[0]) if grid else 0, len(grid), i, cols, rows))
            return
        holes = frozenset((c, y) for y in range(rows) for c in range(cols) if grid[y][c] == -1)
        if len(holes) != count:
            r.fail("%s: the settle of update %d (level %d) has %d holes, its shape %d %d" % (label, s["update"], s["level"], len(holes), i, count))
            return
        if holes_of.setdefault(i, holes) != holes:
            r.fail("%s: the settle of update %d (level %d) has other holes than shape %d's before: %s" %
                   (label, s["update"], s["level"], i, sorted(holes ^ holes_of[i])[:6]))
            return
        seen[i] = seen.get(i, 0) + 1
    notes.append("shapes " + ", ".join("%dx%d with %d holes: %d settles" % (shapes[i] + (seen.get(i, 0),)) for i in range(len(shapes))))


def dirt_layers(grid):
    """The dirt layers of a settle's grid (stage 7b: a dirt cell prints as -1 - its layers), {(col, row): layers}"""
    return {(c, y): -1 - v for y, row in enumerate(grid) for c, v in enumerate(row) if v < -1}


def dirt_check(r, settles, spec, label, notes):
    """Stage 7b, "dirt=<cells>:<layers>,...": the dirt of board.levels' shapes in order (one for a board of one shape),
    its cells and layers in all. Every settle has at most that, on cells its shape's first settle had, and the
    variant line's total; a settle after a level change (the board's first, unless a resumed game's) has all of it;
    from one settle to the next on a level, no cell gains a layer but by an undo (whose board variant_check compares)"""
    shapes = []
    for part in spec.split(","):
        m = re.match(r"(\d+):(\d+)$", part)
        if not m:
            r.fail("%s: dirt=%s: not <cells>:<layers>,..." % (label, spec))
            return
        shapes.append((int(m.group(1)), int(m.group(2))))
    cells_of, cleared, cells_cleared, sides = {}, 0, 0, {"left": 0, "right": 0}
    prev = None
    for s in settles:
        i = (s["level"] - 1) % len(shapes)
        want_cells, want_layers = shapes[i]
        layers = dirt_layers(s["grid"])
        total = sum(layers.values())
        if s["dirt"] != total:
            r.fail("%s: the settle of update %d says dirt %s, its grid has %d layers" % (label, s["update"], s["dirt"], total))
            return
        sides[s["side"]] += 1
        if len(layers) > want_cells or total > want_layers:
            r.fail("%s: the settle of update %d has %d dirt cells with %d layers, its shape %d %d with %d" %
                   (label, s["update"], len(layers), total, i, want_cells, want_layers))
            return
        start = cells_of.setdefault(i, set(layers))
        if not set(layers) <= start:
            r.fail("%s: the settle of update %d has dirt on cells shape %d had none: %s" % (label, s["update"], i, sorted(set(layers) - start)[:6]))
            return
        same_level = prev is not None and s["tick"] > prev["tick"] and s["mode"] == prev["mode"] and s["level"] == prev["level"]
        if prev is not None and not same_level and (len(layers), total) != (want_cells, want_layers):
            r.fail("%s: the settle of update %d starts level %d with %d dirt cells, %d layers; its shape %d has %d, %d" %
                   (label, s["update"], s["level"], len(layers), total, i, want_cells, want_layers))
            return
        if same_level and s["undos"] == prev["undos"]:
            before = dirt_layers(prev["grid"])
            gained = [cell for cell, n in layers.items() if n > before.get(cell, 0)]
            if gained:
                r.fail("%s: from the settle of update %d to update %d dirt gained layers at %s" % (label, prev["update"], s["update"], gained[:6]))
                return
            cleared += sum(before.values()) - total
            cells_cleared += len(before) - len(layers)
        prev = s
    notes.append("dirt: %d layers cleared, %d cells; next slide left %d, right %d" % (cleared, cells_cleared, sides["left"], sides["right"]))


def variant_fact(stdout):
    """The "variant" fact (stage 5), None for a run without variant lines: the last variant line's counts (with
    its gravity, stage 7c) and how many settles had no move (a noMoves "continue" board)"""
    settles = variant_settles(stdout)
    last = [s for s in settles if s["moves"] is not None]
    if not last:
        return None
    gravity = (" gravity %s flips %d" % (last[-1]["gravity"], last[-1]["flips"])) if last[-1]["gravity"] else ""
    dirt = (" dirt %d %s" % (last[-1]["dirt"], last[-1]["side"])) if last[-1]["dirt"] is not None else ""
    return "last moves %d invalid %d%s%s, %d of %d settles without a move" % (
        last[-1]["moves"], last[-1]["invalid"], gravity, dirt, sum(1 for s in settles if s["found"] == 0), len(settles))


def swapped_cells(a, b):
    """The two cells two grids differ in when they are one swap of neighbours apart ([] for the same grid: two
    neighbours of one colour swapped), else None"""
    if len(a) != len(b) or any(len(x) != len(y) for x, y in zip(a, b)):
        return None
    cells = [(c, r) for r in range(len(a)) for c in range(len(a[r])) if a[r][c] != b[r][c]]
    if not cells:
        return cells
    if len(cells) != 2:
        return None
    (c0, r0), (c1, r1) = cells
    if abs(c0 - c1) + abs(r0 - r1) != 1 or a[r0][c0] != b[r1][c1] or a[r1][c1] != b[r0][c0]:
        return None
    return cells


GEM_ROW = 104       # the gem sheet's rows per colour (GemSheet.h)
GEM_GLYPH = (41, 39)  # the generated rows' glyph centre in the first frame


def png_size(path):
    """(width, height) from a PNG's IHDR"""
    with open(path, "rb") as f:
        head = f.read(24)
    if head[:8] != b"\x89PNG\r\n\x1a\n":
        raise ValueError("not a PNG")
    return struct.unpack(">II", head[16:24])


def colors_check(r, cand, run, flags, label):
    """A run with more gem colours (stage 8, module comment "colors")"""
    f = parse_flags(flags)
    n = int(f.get("colors", "7"))
    seen = set()
    for line in run_stdout(run):
        if line.startswith("TRACE   row "):
            seen.update(int(t) for t in line.split(":", 1)[1].split() if t.isdigit())
    if not seen:
        r.fail("%s: no TRACE rows" % label)
        return False
    if max(seen) >= n:
        r.fail("%s: colours %s on the board, colors=%d" % (label, sorted(c for c in seen if c >= n), n))
    if "all" in f and len(seen) != n:
        r.fail("%s: %d of the %d colours on the board (%s)" % (label, len(seen), n, sorted(seen)))
    logged = [m.group(1) for m in (re.search(r"Gem sheet: (\d+) colour rows", line)
                                   for line in read_lines(os.path.join(run, "stderr.txt"))) if m]
    if n > 7 and (not logged or int(logged[-1]) < n):
        r.fail("%s: the gem sheet grown to %s rows for %d colours" % (label, logged[-1] if logged else "no", n))
    if "sheet" in f:
        rows = int(f["sheet"])
        path = os.path.join(run, "shots", "gemsheet.png")
        if not os.path.exists(path):
            r.fail("%s: no gem sheet dump (shots/gemsheet.png)" % label)
            return False
        w, h, px = png_pixels(path)
        if (w, h) != (780, rows * GEM_ROW):
            r.fail("%s: the gem sheet is %dx%d, expected 780x%d" % (label, w, h, rows * GEM_ROW))
            return False
        first = 7
        if "ref" in f:
            _, _, ref = png_pixels(os.path.join(cand, f["ref"], "shots", "gemsheet.png"))
            if px[:len(ref)] != ref:
                r.fail("%s: the gem sheet's first rows differ from %s's" % (label, f["ref"]))
            first = len(ref) // (w * 4 * GEM_ROW)
        stride = w * 4
        for row in range(first, rows):
            alpha = px[row * GEM_ROW * stride + 3:(row + 1) * GEM_ROW * stride:4]
            if sum(1 for a in alpha if a) < w * GEM_ROW // 4:
                r.fail("%s: generated row %d is nearly empty" % (label, row))
        if "glyphs" in f:
            lo, _, hi = f["glyphs"].partition("-")
            for row in range(int(lo), int(hi or lo) + 1):
                x, y = GEM_GLYPH
                at = (row * GEM_ROW + y) * stride + x * 4
                if min(px[at:at + 4]) < 250:
                    r.fail("%s: row %d has no glyph (%s at (%d, %d))" % (label, row, tuple(px[at:at + 4]), x, y))
        r.out("    gem sheet: %d rows, %d generated" % (rows, rows - first))
        if "hd" in f:
            scale = int(f["hd"])
            hd = os.path.join(run, "shots", "gemsheet@%dx.png" % scale)
            if not os.path.exists(hd) or png_size(hd) != (780 * scale, rows * GEM_ROW * scale):
                r.fail("%s: the HD dump %s is %s, expected %dx%d" % (label, os.path.basename(hd),
                       "%dx%d" % png_size(hd) if os.path.exists(hd) else "missing", 780 * scale, rows * GEM_ROW * scale))
    return True


def variant_check(r, cand, run, flags, label):
    """A run with stage-5 rules (module comment, "variant")"""
    before = r.failures
    f = parse_flags(flags)
    stdout = run_stdout(run)
    settles = variant_settles(stdout)
    notes = []
    if not settles:
        r.fail("%s: no settles" % label)
        return False
    missing = [s["update"] for s in settles if s["moves"] is None]
    if missing:
        r.fail("%s: settles without a variant line (updates %s): not a variant board" % (label, missing[:5]))
    # the states hash.txt shows between two settles: a vortex, wipe or fall-out refills the board without a move
    states = []
    hash_path = os.path.join(run, "hash.txt")
    for line in (read_lines(hash_path) if os.path.exists(hash_path) else []):
        parts = line.split()
        if len(parts) >= 2:
            states.append((int(parts[0]), int(parts[1])))
    counts_as_move = f.get("countsasmove", "1") == "1"
    kept = 0
    # stage 7c: the direction each settle names, and the flips from one settle to the next (gravity_step)
    gravity = f.get("gravity")
    if gravity is not None:
        if gravity not in ("up", "down", "alternate"):
            r.fail("%s: gravity=%s: not up, down or alternate" % (label, gravity))
            return False
        for s in settles:
            want = gravity if gravity != "alternate" else ("up" if s["flips"] % 2 == 0 else "down")
            if s["gravity"] != want or (gravity != "alternate" and s["flips"] != 0):
                r.fail("%s: the settle of update %d says gravity %s flips %d, expected %s" % (label, s["update"], s["gravity"], s["flips"], want))
                return False
    flipped = [0, 0]  # settles that flipped, settles that did not

    def gravity_step(s0, s1, want, what):
        """The flips from settle s0 to s1 are s0's + want (stage 7c); False after a failure"""
        if gravity is None:
            return True
        want = (want if gravity == "alternate" else 0) + s0["flips"]
        if s1["flips"] != want:
            r.fail("%s: %s from the settle of update %d to update %d: flips %d -> %d, expected %d" %
                   (label, what, s0["update"], s1["update"], s0["flips"], s1["flips"], want))
            return False
        flipped[0 if s1["flips"] != s0["flips"] else 1] += 1
        return True
    # the autoplay bot's targets for a stale selection (a gem selected before a fall-out, no longer in the grid):
    # the swap goes back and leaves that gem in its old cell, over the gem there (the original game's quirk)
    stale, stale_backs = [], 0
    autoplay_path = os.path.join(run, "autoplay.txt")
    for line in (read_lines(autoplay_path) if os.path.exists(autoplay_path) else []):
        m = re.match(r"# update (\d+): move \d+: selected \((\d+),(\d+)\) \(stale\)", line)
        if m:
            stale.append((int(m.group(1)), int(m.group(2)), int(m.group(3))))
    # stage 6: the settles moves were made from, newest last (an undo goes back to the last one; a vortex empties
    # it, as StartBlackHole empties the undo stack), the settle the last undo took back (the next move again: the
    # same refill, "redo", or another one, "rerolled") and the undo steps seen
    history, undone, undo_steps, redo, rerolled = [], None, 0, 0, 0
    for s0, s1 in zip(settles, settles[1:]):
        same_board = s1["tick"] > s0["tick"] and s1["mode"] == s0["mode"] and s1["seed"] == s0["seed"]
        if not same_board or s0["moves"] is None or s1["moves"] is None:
            history, undone = [], None
            continue
        refilled = any(s0["update"] < u <= s1["update"] and st in (7, 8, 9) for u, st in states)
        vortex = any(s0["update"] < u <= s1["update"] and st == 7 for u, st in states)
        dm, di, du = s1["moves"] - s0["moves"], s1["invalid"] - s0["invalid"], s1["undos"] - s0["undos"]
        if du != 0:
            # an undo, the only step that changes the count: the board of the settle the move was made from again
            # (grid and counts), its score at most that settle's (undo.penalty)
            back = history.pop() if (du == 1 and history and not refilled) else None
            if back is None:
                r.fail("%s: from the settle of update %d to update %d the undos went %+d with %s to take back" %
                       (label, s0["update"], s1["update"], du, "no move" if not history else "a refill"))
                break
            if s1["grid"] != back["grid"] or (s1["moves"], s1["invalid"]) != (back["moves"], back["invalid"]) or s1["score"] > back["score"]:
                r.fail("%s: the undo at update %d is not the board of the settle of update %d (grid %s, moves %d/%d invalid %d/%d, score %d/%d)" %
                       (label, s1["update"], back["update"], "same" if s1["grid"] == back["grid"] else "differs", s1["moves"], back["moves"],
                        s1["invalid"], back["invalid"], s1["score"], back["score"]))
                break
            if not gravity_step(back, s1, 0, "an undo"):
                break
            undone, undo_steps = s0, undo_steps + 1
            continue
        if (dm, di) == (0, 0) and not refilled and s1["grid"] == s0["grid"] and s1["score"] <= s0["score"]:
            # a swap that went back (no moves.allowInvalid): nothing changed but hints taken (their penalty)
            if not gravity_step(s0, s1, 0, "a swap that went back"):
                break
            continue
        if (dm, di) == (0, 0) and not refilled and s1["score"] <= s0["score"] and len(s1["grid"]) == len(s0["grid"]):
            cells = [(c, y) for y in range(len(s0["grid"])) for c in range(len(s0["grid"][y]))
                     if c < len(s1["grid"][y]) and s0["grid"][y][c] != s1["grid"][y][c]]
            if len(cells) == 1 and any(s0["update"] <= u < s1["update"] and (c, y) == cells[0] for u, c, y in stale):
                stale_backs += 1
                if not gravity_step(s0, s1, 0, "a stale selection's swap that went back"):
                    break
                continue  # a stale selection's swap that went back
        allowed = [(1, 0), (1, 1) if counts_as_move else (0, 1)] + ([(0, 0)] if refilled else [])
        if (dm, di) not in allowed:
            r.fail("%s: from the settle of update %d to update %d the counts went %+d moves %+d invalid: not one move" %
                   (label, s0["update"], s1["update"], dm, di))
            break
        # a kept invalid move (no refill between) does not flip "alternate"; a move or a refill flips once
        if not gravity_step(s0, s1, 0 if (di == 1 and not refilled) else 1, "a kept invalid move" if (di == 1 and not refilled) else "a move or refill"):
            break
        if vortex:
            history, undone = [], None
        elif (dm, di) != (0, 0):
            history.append(s0)
            if undone is not None:
                if s1["grid"] == undone["grid"]:
                    redo += 1
                else:
                    rerolled += 1
            undone = None
        if di == 1 and not refilled:
            # a kept invalid move: one swap of neighbours, nothing else changed (no match, no refill); stage 10: one
            # move of the rules' types
            if "moves" in f and not made_move(s0["grid"], s1["grid"], f["moves"].split("+"), f.get("twist") == "both"):
                r.fail("%s: the kept move between updates %d and %d is not one %s move" % (label, s0["update"], s1["update"], f["moves"]))
                break
            if "moves" not in f and swapped_cells(s0["grid"], s1["grid"]) is None:
                r.fail("%s: the kept move between updates %d and %d is not one swap of neighbours" % (label, s0["update"], s1["update"]))
                break
            if s1["score"] > s0["score"]:
                r.fail("%s: the kept move between updates %d and %d scored" % (label, s0["update"], s1["update"]))
                break
            kept += 1
    if gravity is not None:
        notes.append("gravity %s: %d steps flipped, %d did not" % (gravity, flipped[0], flipped[1]))
    if stale_backs:
        notes.append("%d swaps of a stale selection that went back, its old gem in its cell" % stale_backs)
    if "kept" in f:
        if kept < int(f["kept"]):
            r.fail("%s: %d kept invalid moves seen, expected at least %s" % (label, kept, f["kept"]))
        else:
            notes.append("%d kept invalid moves, each one %s" % (kept, ("%s move" % f["moves"]) if "moves" in f else "swap of neighbours"))
    if "undos" in f:
        if undo_steps != int(f["undos"]):
            r.fail("%s: %d undos seen, expected %s" % (label, undo_steps, f["undos"]))
        else:
            notes.append("%d undos, each the board a move was made from" % undo_steps)
    elif undo_steps:
        r.fail("%s: %d undos seen, and this case expects none" % (label, undo_steps))
    for key, count, what in (("redo", redo, "the same refill"), ("rerolled", rerolled, "another refill")):
        if key in f:
            if count < int(f[key]):
                r.fail("%s: %d moves after an undo with %s, expected at least %s" % (label, count, what, f[key]))
            else:
                notes.append("%d moves after an undo with %s" % (count, what))
    nomove = [i for i, s in enumerate(settles) if s["found"] == 0]
    if "continue" in f:
        went_on = [i for i in nomove if i + 1 < len(settles) and settles[i + 1]["tick"] > settles[i]["tick"] and
                   settles[i + 1]["seed"] == settles[i]["seed"]]
        if len(went_on) < int(f["continue"]):
            r.fail("%s: %d settles without a move that the game went on from, expected at least %s" % (label, len(went_on), f["continue"]))
        else:
            notes.append("%d settles without a move, the game went on from %d of them (noMoves \"continue\")" % (len(nomove), len(went_on)))
    elif nomove:
        r.fail("%s: settles without a move at updates %s, and this rule set does not continue" % (label, [settles[i]["update"] for i in nomove][:5]))
    stderr = read_lines(os.path.join(run, "stderr.txt")) if os.path.exists(os.path.join(run, "stderr.txt")) else []
    if "log" in f:
        if not any(f["log"] in line for line in stderr):
            r.fail("%s: stderr.txt has no line with \"%s\"" % (label, f["log"]))
        else:
            notes.append("logged \"%s\"" % f["log"])
    elif any("not loaded:" in line for line in stderr):
        r.fail("%s: a save was refused: %s" % (label, [line for line in stderr if "not loaded:" in line][0]))
    if any("undo: snapshot" in line for line in stderr):
        r.fail("%s: %s" % (label, [line for line in stderr if "undo: snapshot" in line][0]))
    saves = savegames(run)
    if "nosave" in f and saves:
        r.fail("%s: %s left, expected no save" % (label, saves))
    if "timesplayed" in f:
        registry_path = os.path.join(run, "user", "registry.ini")
        registry = read_bytes(registry_path).decode("latin-1") if os.path.exists(registry_path) else ""
        if ("TimesPlayed=dword:%s" % f["timesplayed"]) not in registry.splitlines():
            r.fail("%s: registry.ini has %s, expected TimesPlayed %s" % (label, re.findall(r"TimesPlayed=\S*", registry) or "no TimesPlayed",
                                                                        f["timesplayed"]))
        else:
            notes.append("TimesPlayed %s" % f["timesplayed"])
    if "nowarn" in f:
        warnings = [str(w.get("text", "")) for w in (mods_status(run) or {}).get("warnings", []) if isinstance(w, dict)]
        if warnings:
            r.fail("%s: mods-status.json warns %s, expected nothing" % (label, warnings))
        else:
            notes.append("no warning")
    if "warn" in f:
        warnings = [str(w.get("text", "")) for w in (mods_status(run) or {}).get("warnings", []) if isinstance(w, dict)]
        if not any(f["warn"] in w for w in warnings):
            r.fail("%s: no warning in mods-status.json contains \"%s\" (%s)" % (label, f["warn"], warnings))
        else:
            notes.append("warned \"%s\"" % f["warn"])
    if "v2" in f:
        status = mods_status(run) or {}
        rules = str(status.get("rulesId", "?"))[:16]
        last = settles[-1]
        want = "MOVE moves %d invalid %d undos %d" % (last["moves"] or 0, last["invalid"] or 0, last["undos"])
        for s in saves:
            text = save_state(os.path.join(run, "user", s))
            if not text.startswith("v2 ") or ("rules %s," % rules) not in text or (f["v2"] == "last" and want not in text):
                r.fail("%s: %s: %s (expected a v2 save of rules %s%s)" % (label, s, text, rules, ", " + want if f["v2"] == "last" else ""))
        if not saves:
            r.fail("%s: no save" % label)
        else:
            notes.append("v2 saves of rules %s: %s" % (rules, ", ".join(saves)))
    if "shapes" in f:
        shapes_check(r, settles, f["shapes"], label, notes)
    if "dirt" in f:
        dirt_check(r, settles, f["dirt"], label, notes)
    if "match" in f and "moves" not in f:
        match_check(r, settles, f["match"], f.get("kinds", ""), label, notes)
    if "moves" in f:
        moves_check(r, settles, f["moves"], f.get("twist") == "both", f.get("match"), label, notes)
    if "specials" in f:
        specials_check(r, cand, settles, f["specials"], f.get("after"), label, notes)
    if "gold" in f:
        gold_check(r, cand, run, settles, f["gold"], f.get("timeup"), f.get("after"), label, notes)
    if "boards" in f:
        seed_check(r, cand, run, flags, "seeds")
    if r.failures == before:
        r.out("    %s: %d settles, every one a variant board's, each step one move; %s" % (label, len(settles), "; ".join(notes) or "-"))
    return r.failures == before


def samestream_check(r, first, run, ref, label):
    """A seeded game played, quit and resumed (first: the session up to the quit; run: the session that resumed
    it) against the same moves played without the quit (ref): the settle blocks (rows, score, seed and moves;
    not the update and tick numbers) of first and of run after its first one are ref's, and run's first (the
    loaded game) is first's last: the resumed game goes on with its stream (the sidecar's RNGS)"""
    before = r.failures
    a, b, c = settle_blocks(run_stdout(first)), settle_blocks(run_stdout(run)), settle_blocks(run_stdout(ref))
    if not a or not b:
        r.fail("%s: no settles" % label)
        return False
    if b[0][1] != a[-1][1]:
        r.fail("%s: the resumed game's first settle is not the saved one" % label)
    joined = ["\n".join(x[1]) for x in a + b[1:]]
    diff = first_list_diff(["\n".join(x[1]) for x in c], joined, label + ": settle blocks")
    if diff:
        r.fail(diff)
    if not any("TRACE   seed" in line for x in b for line in x[1]):
        r.fail("%s: the resumed game has no seed line" % label)
    if r.failures == before:
        r.out("    %s: %d settles before the quit and %d after the resume are the %d of the game played through"
              % (label, len(a), len(b) - 1, len(c)))
    return r.failures == before


def trace_blocks(stdout):
    """The TRACE settle blocks of a run, each the header without its update number (the board tick stays) and
    the block's other TRACE lines, split per playback segment at "REPLAY segment" lines: [[block, ...], ...]"""
    segments = [[]]
    for line in stdout:
        if line.startswith("REPLAY segment "):
            segments.append([])
            continue
        m = re.match(r"TRACE update \d+: (.*)$", line)
        if m:
            segments[-1].append([m.group(1)])
        elif line.startswith("TRACE   ") and segments[-1]:
            segments[-1][-1].append(line)
    return segments


def contiguous_in(part, whole):
    """The index at which the list part is a contiguous run of whole, or None"""
    for i in range(len(whole) - len(part) + 1):
        if whole[i:i + len(part)] == part:
            return i
    return None


def playback_check(r, cand, run, flags, label):
    """The candidate watching a recorded replay (the module comment, "playback")"""
    before = r.failures
    f = parse_flags(flags)
    stdout = run_stdout(run)
    ends = [line for line in stdout if line.startswith("REPLAY end: ") or ": cannot play: " in line]
    try:
        replay = bjr.read(read_bytes(os.path.join(run, "replay.bjr")))
    except (OSError, bjr.BjrError) as e:
        r.fail("%s: replay.bjr: %s" % (label, e))
        return False
    if len(ends) != 1:
        r.fail("%s: %d REPLAY end lines" % (label, len(ends)))
        return False
    if "corrupt" in f:
        if not ends[0].startswith("REPLAY end: CORRUPT") or f["corrupt"] not in ends[0]:
            r.fail("%s: expected CORRUPT ... %s, got: %s" % (label, f["corrupt"], ends[0]))
    else:
        end = replay["end"]
        checks = sum(1 for seg in replay["segments"] for _, op, _ in seg["events"] if op == "CHECK")
        want = "REPLAY end: ok, %d CHECKs, END %s tick %d score %d level %d gems %d draws %d" % (
            checks, end["reasonName"], end["endTick"], end["score"], end["level"], end["gems"], end["boardDraws"])
        if ends[0] != want:
            r.fail("%s: %s, expected: %s" % (label, ends[0], want))
        played = trace_blocks(stdout)[1:]
        if len(played) != len(replay["segments"]):
            r.fail("%s: %d segments played, the replay has %d" % (label, len(played), len(replay["segments"])))
        traces = f.get("trace", "").split(",")
        for k, blocks in enumerate(played if "speed" not in f else []):
            source = traces[min(k, len(traces) - 1)]
            recorded = [b for seg in trace_blocks(run_stdout(os.path.join(cand, source))) for b in seg]
            if not recorded or contiguous_in(blocks, recorded) is None:
                first = next((i for i, b in enumerate(blocks) if b not in recorded), None)
                r.fail("%s: segment %d: its %d TRACE settles are not a run of %s's %d%s" % (
                    label, k, len(blocks), source, len(recorded),
                    "" if first is None else " (settle %d: %s)" % (first, blocks[first][0])))
        codec = os.path.join(run, "codec.txt")
        lines = read_lines(codec) if os.path.exists(codec) else []
        settles = sum(len(b) for b in played)
        m = re.fullmatch(r"codeccheck: (\d+) updates checked, (\d+) settles, (\d+) failures", lines[-1] if lines else "")
        if not m or m.group(3) != "0" or ("speed" not in f and int(m.group(2)) != settles):
            r.fail("%s: codec.txt: %s (%d TRACE settles)" % (label, lines[-1] if lines else "missing", settles))
    user = sorted(list_tree(os.path.join(run, "user"), lambda p: True))
    allowed = sorted(f.get("user", "registry.ini").split(","))
    registry = read_bytes(os.path.join(run, "user", "registry.ini")).decode("latin-1") if "registry.ini" in user else ""
    if user != allowed or "TimesPlayed=dword:0" not in registry.splitlines():
        r.fail("%s: the user dir is %s (expected %s), TimesPlayed %s: watching must save, count and record nothing" % (
            label, user, allowed, re.findall(r"TimesPlayed=\S*", registry)))
    if r.failures == before:
        if "corrupt" in f:
            r.out("    %s: %s" % (label, ends[0]))
        elif "speed" in f:
            r.out("    %s: %s; %d segments, faster than 1x (every CHECK matched, the settles not compared); StateCodec round"
                  " trip after every frame; the user dir %s" % (label, ends[0][len("REPLAY end: "):], len(replay["segments"]),
                                                               "untouched" if "user" not in f else "only " + ", ".join(user)))
        else:
            r.out("    %s: %s; %d segments, each a run of %s's TRACE settles; StateCodec round trip after every update;"
                  " the user dir %s" % (label, ends[0][len("REPLAY end: "):], len(replay["segments"]), f.get("trace", ""),
                                        "untouched" if "user" not in f else "only " + ", ".join(user)))
    return r.failures == before


def ui_check(r, cand, run, flags, label):
    """A replay browsed, watched and left in the game (the module comment, "ui")"""
    before = r.failures
    f = parse_flags(flags)
    stdout = run_stdout(run)
    ends = []
    blocks = []  # (stdout index, the settle without its update and board tick numbers)
    first_start = None
    last_end = None
    for i, line in enumerate(stdout):
        if line.startswith("REPLAY start ") and first_start is None:
            first_start = i
        if line.startswith("REPLAY end: ") or ": cannot play: " in line:
            ends.append(line)
            last_end = i
        m = re.match(r"TRACE update \d+: (.*) board tick \d+$", line)
        if m:
            blocks.append((i, [m.group(1)]))
        elif line.startswith("TRACE   ") and blocks:
            blocks[-1][1].append(line)
    verdicts = ["ok" if e.startswith("REPLAY end: ok, ") else "corrupt" if e.startswith("REPLAY end: CORRUPT ")
                else "unplayable" for e in ends]
    want = f.get("ends", "").split(",") if f.get("ends") else []
    if verdicts != want:
        r.fail("%s: the watches ended %s, expected %s: %s" % (label, ",".join(verdicts) or "(none)", ",".join(want) or
                                                              "(none)", " | ".join(ends)))
    if "resume" in f:
        left = [b for i, b in blocks if first_start is not None and i < first_start]
        back = [b for i, b in blocks if last_end is not None and i > last_end]
        if not left or not back or left[-1] != back[0]:
            r.fail("%s: the game after the watch (%s) is not the one left to watch (%s)" % (
                label, back[0][0] if back else "no settle", left[-1][0] if left else "no settle"))
    replays = sorted(p for p in list_tree(os.path.join(run, "user"), lambda p: True) if p.startswith("replays/") and
                     p != "replays/")
    if "replays" in f and len(replays) != int(f["replays"]):
        r.fail("%s: %d files in user/replays, expected %s: %s" % (label, len(replays), f["replays"], " ".join(replays)))
    if r.failures == before:
        r.out("    %s: %d watches (%s)%s%s" % (label, len(ends), ",".join(verdicts) or "none",
                                            "; the game left to watch resumed: " + left[-1][0] if "resume" in f else "",
                                            "; %d replays at the quit" % len(replays) if "replays" in f else ""))
    return r.failures == before


def verifier_lines(lines):
    """The REPLAY and TRACE lines of a watch or a verifier run, with "TRACE update N:" (the app's count) masked"""
    out = []
    for line in lines:
        if line.startswith("TRACE update "):
            out.append(re.sub(r"^TRACE update \d+:", "TRACE update *:", line))
        elif line.startswith(("TRACE", "REPLAY")):
            out.append(line)
    return out


def first_difference(a, b):
    """The index of the first line at which two lists differ (None when equal)"""
    for i in range(max(len(a), len(b))):
        if i >= len(a) or i >= len(b) or a[i] != b[i]:
            return i
    return None


def verifier_check(r, cand, run, flags, label):
    """Both verifiers against the game's watch of the same replay (the module comment, "verifier")"""
    before = r.failures
    f = parse_flags(flags)
    game = verifier_lines(run_stdout(os.path.join(cand, f.get("watch", ""))))
    ends = [line for line in game if line.startswith("REPLAY end: ") or ": cannot play: " in line]
    if len(ends) != 1:
        r.fail("%s: the game's watch %s has %d REPLAY end lines" % (label, f.get("watch", ""), len(ends)))
        return False
    verdict = "ok" if ends[0].startswith("REPLAY end: ok, ") else \
        "corrupt" if ends[0].startswith("REPLAY end: CORRUPT ") else "unplayable"
    if ("unplayable" in f) != (verdict == "unplayable"):
        r.fail("%s: the game's verdict is %s: %s" % (label, verdict, ends[0]))
    if "verdict" in f and f["verdict"] != verdict:
        r.fail("%s: the game's verdict is %s, not %s: %s" % (label, verdict, f["verdict"], ends[0]))
    results = {}
    for side in ("native", "wasm"):
        path = os.path.join(run, side + ".txt")
        lines = read_lines(path) if os.path.exists(path) else []
        exit_path = os.path.join(run, side + "-exit.txt")
        status = read_bytes(exit_path).decode("latin-1").strip() if os.path.exists(exit_path) else "missing"
        try:
            result = json.loads(lines[-1]) if lines else None
        except ValueError:
            result = None
        if not isinstance(result, dict):
            r.fail("%s: the %s verifier printed no JSON result (exit %s)" % (label, side, status))
            continue
        got = verifier_lines(lines[:-1])
        results[side] = (got, result, status)
        if "speed" in f:  # the game's TRACE lines came once a frame at 2x to 8x (the module comment)
            at = first_difference([x for x in got if x.startswith("REPLAY")], [x for x in game if x.startswith("REPLAY")])
        else:
            at = first_difference(got, game)
        if at is not None:
            r.fail("%s: the %s verifier's %d REPLAY/TRACE lines are not the game's %d: line %d: %s / %s" % (
                label, side, len(got), len(game), at + 1, got[at] if at < len(got) else "(none)",
                game[at] if at < len(game) else "(none)"))
        if result.get("result") != verdict or result.get("message") != ends[0]:
            r.fail("%s: the %s verifier's result %s (%s), the game's %s" % (
                label, side, result.get("result"), result.get("message"), verdict))
        if status != ("0" if verdict == "ok" else "1"):
            r.fail("%s: the %s verifier's exit status %s for %s" % (label, side, status, verdict))
    if len(results) == 2 and results["native"][:2] != results["wasm"][:2]:
        r.fail("%s: the native and the wasm verifier differ" % label)
    if r.failures == before and "speed" in f:
        r.out("    %s: the native and the wasm verifier print the game's %d REPLAY lines (a faster watch: its TRACE not"
              " compared) and agree on their %d TRACE settles, and its verdict: %s" % (
                  label, sum(1 for line in game if line.startswith("REPLAY")),
                  sum(1 for line in results["native"][0] if line.startswith("TRACE update ")), ends[0]))
    elif r.failures == before:
        settles = sum(1 for line in game if line.startswith("TRACE update "))
        r.out("    %s: the native and the wasm verifier print the game's %d REPLAY and TRACE lines (%d settles) and"
              " its verdict: %s" % (label, len(game), settles, ends[0]))
    return r.failures == before


def wasmlogic_check(r, cand, run, flags, label):
    """The web build under Node against the candidate's run of the same case (the module comment, "wasmlogic")"""
    before = r.failures
    ref = os.path.join(cand, parse_flags(flags).get("ref", ""))
    hashes = []
    for path in (os.path.join(ref, "hash.txt"), os.path.join(run, "hash.txt")):
        hashes.append([line.split() for line in read_lines(path)] if os.path.exists(path) else [])
    logic = [[" ".join(fields[:3]) for fields in h] for h in hashes]
    at = first_difference(logic[0], logic[1])
    if not logic[0] or at is not None:
        r.fail("%s: hash.txt's LOGIC column: %d and %d updates, first difference at line %s: %s / %s" % (
            label, len(logic[0]), len(logic[1]), "-" if at is None else at + 1,
            logic[0][at] if at is not None and at < len(logic[0]) else "(none)",
            logic[1][at] if at is not None and at < len(logic[1]) else "(none)"))
    out_ref, out_run = run_stdout(ref), run_stdout(run)
    at = first_difference(out_ref, out_run)
    if at is not None:
        r.fail("%s: stdout.txt (%d and %d lines) differs at line %d: %s / %s" % (
            label, len(out_ref), len(out_run), at + 1, out_ref[at] if at < len(out_ref) else "(none)",
            out_run[at] if at < len(out_run) else "(none)"))
    if r.failures == before:
        full = [[fields[3] if len(fields) > 3 else "" for fields in h] for h in hashes]
        at = first_difference(full[0], full[1])
        r.out("    %s: %d updates with the candidate's LOGIC hashes, its %d stdout lines (TRACE%s); the FULL hashes %s" % (
            label, len(logic[0]), len(out_ref), ", REPLAY" if any(x.startswith("REPLAY") for x in out_ref) else "",
            "too" if at is None else "differ from update %s (same platform only)" % hashes[0][at][0]))
    return r.failures == before


def difftest_check(r, run):
    """bejeweled_verify difftest's report (the module comment, "difftest")"""
    before = r.failures
    exit_path = os.path.join(run, "exit.txt")
    status = read_bytes(exit_path).decode("latin-1").strip() if os.path.exists(exit_path) else "missing"
    report = os.path.join(run, "report.txt")
    lines = read_lines(report) if os.path.exists(report) else []
    for line in [line for line in lines if "DIFFERS" in line][:10]:
        r.out("    " + line)
    summaries = [line for line in lines if re.match(r"difftest (normal|timed): ", line)]
    if status != "0" or len(summaries) != 2 or not all(", 0 differ;" in line for line in summaries):
        r.fail("the difftest's exit status %s, %d mode summaries" % (status, len(summaries)))
    for line in summaries:
        r.out("    " + line)
    return r.failures == before


def read_expect(path):
    """({case: [(key, value)]}, header comment lines)."""
    expect, header = {}, []
    if path and os.path.exists(path):
        seen_data = False
        with open(path, encoding="latin-1") as f:
            for line in f:
                line = line.rstrip("\r\n")
                if not line.strip() or line.startswith("#"):
                    if not seen_data and line.startswith("#"):
                        header.append(line)
                    continue
                seen_data = True
                parts = line.split(None, 2)
                if len(parts) >= 2:
                    expect.setdefault(parts[0], []).append((parts[1], parts[2] if len(parts) > 2 else ""))
    return expect, header


def check_row(stdout, value):
    """A hand-written "row <settle update> <row> <colours...>" fact."""
    parts = value.split()
    if len(parts) != 10:
        return "malformed row fact: %s" % value
    header = "TRACE update %s:" % parts[0]
    want = "TRACE   row %s: %s" % (parts[1], " ".join(parts[2:]))
    for i, line in enumerate(stdout):
        if line.startswith(header):
            for row in stdout[i + 1:i + 9]:
                if row.startswith("TRACE   row %s:" % parts[1]):
                    return None if row == want else "row fact: at %s got \"%s\", expected \"%s\"" % (header, row, want)
    return "row fact: no settle at update %s" % parts[0]


def check_expect(r, run, kind, entries, flags=""):
    if not entries:
        r.fail("expect: no facts for this case in the expect file (compare.py expect-update)")
        return
    want = ["%s %s" % (k, v) for k, v in entries if k in AUTO_KEYS]
    got = derive_facts(run, kind, flags)
    before = r.failures
    if want != got:
        for line in difflib.unified_diff(want, got, "expected", "candidate", lineterm="", n=0):
            if not line.startswith(("---", "+++", "@@")):
                r.out("          " + line)
        r.fail("expect: the candidate's facts differ from the expect file (lines above: - expected, + candidate)")
    stdout = read_lines(os.path.join(run, "stdout.txt")) if os.path.exists(os.path.join(run, "stdout.txt")) else []
    rows = [v for k, v in entries if k == "row"]
    for value in rows:
        diff = check_row(stdout, value)
        if diff:
            r.fail("expect: " + diff)
    others = [k for k, _ in entries if k not in AUTO_KEYS and k != "row"]
    if others:
        r.fail("expect: unknown keys %s" % others)
    if r.failures == before:
        r.out("    expect: %d facts%s hold" % (len(want), (" + %d rows" % len(rows)) if rows else ""))


def compare(oracle, expect_path=None):
    info = read_kv(os.path.join(oracle, "exes.txt"))
    sides = info.get("sides", "baseline candidate").split()
    base_sides = [s for s in sides if s != "candidate"]
    hooks = {s: info.get(s + ".hooks") == "yes" for s in sides}
    cand = os.path.join(oracle, "candidate")
    cases = read_cases(oracle)
    expect, _ = read_expect(expect_path)
    novariants = info.get("candidate.variants") == "no"

    r = Report()
    for side in sides:
        r.out("%-15s %s (stage-0 hooks: %s)" % (side + ":", info.get(side + ".exe", "?"), info.get(side + ".hooks", "?")))
        r.out("%-15s sha256 %s, DLLs %s" % ("", info.get(side + ".sha256", "?"), info.get(side + ".dlls", "?")))
        folder = os.path.join(oracle, side)
        runs = sorted(os.listdir(folder)) if side != "candidate" and os.path.isdir(folder) else []
        cached = [c for c in (cached_info(os.path.join(folder, x)) for x in runs) if c is not None]
        if cached:
            r.out("%-15s %d of its %d runs from compare.sh's cache (made %s)"
                  % ("", len(cached), len(runs), ", ".join(sorted(set(c.get("made", "?")[:10] for c in cached)))))
    if info.get("candidate.args"):
        r.out("%-15s every candidate run with %s" % ("", info["candidate.args"]))
    for side in base_sides:
        if info.get(side + ".sha256") and info.get(side + ".sha256") == info.get("candidate.sha256"):
            r.note("%s and candidate are the same binary: comparing them only shows that the runs repeat" % side)
    if not any(hooks[s] for s in base_sides):
        r.note("no baseline has the stage-0 hooks: hash.txt, snaps/ and autoplay.txt are compared with nothing")
    if expect_path and not os.path.exists(expect_path):
        r.fail("expect file %s missing (compare.py expect-update)" % expect_path)
    partial = partial_of(oracle)
    if partial is not None:
        r.out("%-15s a partial oracle (compare.sh --cases %s): only %d cases" % ("", partial, len(cases)))

    for name, kind, desc, flags in cases:
        r.out("")
        r.out("[%s] %s: %s" % (kind, name, desc))
        before = r.failures
        run = os.path.join(cand, name)

        for side in base_sides:
            retried = os.path.join(oracle, side, name, "retried.txt")
            if os.path.exists(retried):
                cached = cached_info(os.path.join(oracle, side, name))
                r.note("%s exited %s at first (a frozen exe's crash at exit, before af758b9) and was run once more:"
                       " the second run is compared%s" % (side, read_bytes(retried).decode("latin-1").strip(),
                                                          " (a cached run, made %s)" % cached.get("made", "?") if cached else ""))
        f = parse_flags(flags)
        if kind == "plain":
            parent = name[:-len(".plain")]
            for side in base_sides:
                compare_case(r, side, os.path.join(oracle, side, parent), run, False)
        elif kind == "inert":
            ref = f.get("ref", "")
            inert_check(r, os.path.join(cand, ref), run, "cross " + ref)
            status = mods_status(run)
            if status is not None:
                r.out("    mods-status.json: " + status_text(status))
            if "boards" in f:
                seed_check(r, cand, run, flags, "seeds")
        elif kind == "seeded":
            seed_check(r, cand, run, flags, "seeds")
        elif kind == "variant":
            variant_check(r, cand, run, flags, "variant")
        elif kind == "colors":
            colors_check(r, cand, run, flags, "colors")
        elif kind == "recording":
            recording_check(r, os.path.join(cand, f.get("ref", "")), run, "cross " + f.get("ref", ""))
            if "boards" in f:
                seed_check(r, cand, run, flags, "seeds")
        elif kind == "playback":
            playback_check(r, cand, run, flags, "watch")
        elif kind == "ui":
            ui_check(r, cand, run, flags, "ui")
        elif kind == "reference":
            r.out("    a reference for other cases: its exit status and expect facts only")
        elif kind == "samestream":
            samestream_check(r, os.path.join(cand, f.get("first", "")), run, os.path.join(cand, f.get("ref", "")),
                             "cross " + f.get("first", "") + " + " + f.get("ref", ""))
        elif kind == "sameboard":
            sameboard_check(r, os.path.join(cand, f.get("ref", "")), run, "cross " + f.get("ref", ""))
        elif kind == "samegrids":
            samegrids_check(r, os.path.join(cand, f.get("ref", "")), run, flags, "cross " + f.get("ref", ""))
        elif kind == "repeat":
            parent = name[:-len(".repeat")]
            if same_run(r, os.path.join(cand, parent), run, "cross " + parent):
                r.out("    candidate %s run again: exit, logs, hash.txt, user dir and screenshots identical: deterministic"
                      " under the same -seed" % parent)
        elif kind == "lint":
            lint_check(r, run)
        elif kind == "lsref":
            r.out("    the reference of %s.ls" % name[:-len(".lsoff")])
        elif kind == "livesplit":
            livesplit_check(r, oracle, os.path.join(cand, f.get("ref", "")), run, "cross " + f.get("ref", ""))
        elif kind == "cli":
            cli_check(r, run)
        elif kind == "verifier":
            verifier_check(r, cand, run, flags, "verifier")
        elif kind == "wasmlogic":
            wasmlogic_check(r, cand, run, flags, "wasm")
        elif kind == "difftest":
            difftest_check(r, run)
        elif kind == "nosave":
            parent = name[:-len(".nosave")]
            if same_run(r, os.path.join(cand, parent), run, "cross " + parent):
                r.out("    candidate %s with and without its save lines: exit, logs, hash.txt, user dir and"
                      " screenshots identical: save changes nothing" % parent)
        else:
            for side in base_sides:
                if flags == "hooksonly" and not hooks[side]:
                    r.out("    vs %s: not run (no stage-0 hooks; %s)" % (side, "the replay case covers it" if kind == "autoplay"
                                                                       else "the script uses a stage-0 command"))
                    continue
                compare_case(r, side, os.path.join(oracle, side, name), run, hooks[side])
            if kind == "quitcut":
                parent, _, snap = name.split(".", 2)
                snapshot = os.path.join(cand, parent, "snaps", snap + ".dat")
                saves = [(side, os.path.join(oracle, side, name, "user", "savegame.dat")) for side in sides]
                have = [os.path.exists(snapshot)] + [os.path.exists(p) for _, p in saves]
                if not any(have):
                    r.out("    no game in progress: no snapshot and no shutdown save (as expected together)")
                elif not all(have):
                    r.fail("snapshot / quit save of %s present: %s" % (", ".join(sides), have))
                else:
                    ds = read_bytes(snapshot)
                    for side, p in saves:
                        if read_bytes(p) != ds:
                            r.fail("snapshot %s.dat != %s shutdown save: %s" % (snap, side, first_byte_diff(ds, read_bytes(p))))
                    r.out("    snapshot %s.dat == shutdown save of every exe: %s" % (snap, save_state(snapshot)))
            elif kind == "replay":
                source = name[:-len(".replay")]
                if same_run(r, os.path.join(cand, source), run, "cross " + source, with_shots=False):
                    r.out("    candidate %s and its recorded input replayed as a script: exit, logs, hash.txt and"
                          " user dir identical: the bot acts only through script input" % source)

        for side in sides:
            changed = os.path.join(oracle, side, name, "modsdir-changed.txt")
            if os.path.exists(changed):
                r.fail("%s: the run changed the mods folder: %s" % (side, read_bytes(changed).decode("latin-1")[:300]))
        exit_path = os.path.join(run, "exit.txt")
        if kind in NO_RUN_KINDS:
            pass
        elif os.path.isdir(run) and (not os.path.exists(exit_path) or read_bytes(exit_path).strip() != b"0"):
            r.fail("the candidate run did not exit with status 0")
        # every candidate run but the plain ones has -tracehash
        if (kind not in NO_RUN_KINDS) and os.path.exists(os.path.join(run, "hash.txt")) != (kind != "plain"):
            r.fail("the candidate run %s hash.txt" % ("has a" if kind == "plain" else "has no"))
        if kind in EXPECT_KINDS and expect_path:
            if novariants and (name + NOVARIANTS) in expect:
                r.out("    expect: the facts of %s%s (a BEJ_NO_VARIANTS candidate)" % (name, NOVARIANTS))
                check_expect(r, run, kind, expect[name + NOVARIANTS], flags)
            else:
                check_expect(r, run, kind, expect.get(name, []), flags)
        if kind in ("script", "load", "inert", "autoplay", "seeded", "recording", "samestream", "reference", "variant", "colors"):
            for s in list_tree(os.path.join(run, "snaps"), lambda p: p.endswith(".dat")):
                r.out("    snapshot %s: %s" % (s, save_state(os.path.join(run, "snaps", s))))
            for s in savegames(run):
                r.out("    %s: %s" % (s, save_state(os.path.join(run, "user", s))))
        if kind == "autoplay" and os.path.exists(os.path.join(run, "autoplay.txt")):
            notes = [line for line in read_lines(os.path.join(run, "autoplay.txt")) if line.startswith("# update")]
            if notes:
                r.out("    bot's last note: %s" % notes[-1][2:])
        r.out("    -> %s" % ("identical" if r.failures == before else "DIFFERENT"))

    r.out("")
    r.out("%d cases, %d differences, %d notes: %s%s" % (len(cases), r.failures, r.notes,
                                                        "ALL IDENTICAL" if r.failures == 0 else "DIFFERENCES FOUND",
                                                        "" if partial is None else " (partial: --cases %s)" % partial))
    with open(os.path.join(oracle, "report.txt"), "w", encoding="latin-1", newline="\n") as f:
        f.write("\n".join(r.lines) + "\n")
    # DRAWS: the baseline runs of every draw count difference, whose cache entries compare.sh drops
    if os.path.isdir(os.path.join(oracle, "gen")):
        noted = sorted(set(os.path.relpath(p, oracle).replace("\\", "/") for p in DRAWS_RUNS))
        with open(os.path.join(oracle, "gen", "draws-noted.txt"), "w", encoding="utf-8", newline="\n") as f:
            f.writelines(p + "\n" for p in noted if p.split("/")[0] in base_sides)
    return 0 if r.failures == 0 else 1


DEFAULT_EXPECT_HEADER = [
    "# Oracle expectations (games/bejeweled/tools/oracle/compare.py, \"Expectations\"): facts each case's",
    "# candidate run must show, whatever the baselines do. \"<case> <key> <value>\" per line. Derived facts are",
    "# rewritten by \"compare.py expect-update build/oracle games/bejeweled/tools/oracle/expect.txt\" (only",
    "# after checking that the change is intended); \"row <settle update> <row> <colours>\" lines are",
    "# hand-written and kept. Game state only: update numbers, scores, gem colour indices, decoded save",
    "# headers; no asset data.",
]


def expect_update(oracle, path):
    if partial_of(oracle) is not None:
        sys.stderr.write("compare.py: %s is a partial oracle (compare.sh --cases %s): the expect file needs the whole one\n"
                         % (oracle, partial_of(oracle)))
        return 2
    old, header = read_expect(path)
    lines = list(header or DEFAULT_EXPECT_HEADER)
    if read_kv(os.path.join(oracle, "exes.txt")).get("candidate.variants") == "no":
        # A BEJ_NO_VARIANTS candidate: only its own facts, "<case>@novariants", for the cases whose facts differ
        # from the default build's; the rest of the file stays as it is
        own = {}
        ran = set()
        for name, kind, _, flags in read_cases(oracle):
            if kind not in EXPECT_KINDS:
                continue
            ran.add(name)
            facts = derive_facts(os.path.join(oracle, "candidate", name), kind, flags)
            if facts != ["%s %s" % (k, v) for k, v in old.get(name, []) if k in AUTO_KEYS]:
                own[name] = facts
        for name, entries in old.items():
            if name.endswith(NOVARIANTS):
                base = name[:-len(NOVARIANTS)]
                if base not in old and base not in ran and base not in own:
                    lines.append("")
                    lines.extend("%s %s %s" % (name, k, v) for k, v in entries)
                continue
            lines.append("")
            lines.extend("%s %s %s" % (name, k, v) for k, v in entries)
            if name in own:
                lines.extend("%s%s %s" % (name, NOVARIANTS, fact) for fact in own[name])
            elif name not in ran:
                lines.extend("%s%s %s %s" % (name, NOVARIANTS, k, v) for k, v in old.get(name + NOVARIANTS, []))
    else:
        for name, kind, _, flags in read_cases(oracle):
            if kind not in EXPECT_KINDS:
                continue
            lines.append("")
            for fact in derive_facts(os.path.join(oracle, "candidate", name), kind, flags):
                lines.append("%s %s" % (name, fact))
            for key, value in old.get(name, []):
                if key == "row":
                    lines.append("%s row %s" % (name, value))
            # a BEJ_NO_VARIANTS candidate's own facts for the case stay (expect-update from its oracle)
            for key, value in old.get(name + NOVARIANTS, []):
                lines.append("%s%s %s %s" % (name, NOVARIANTS, key, value))
    before = []
    if os.path.exists(path):
        before = read_lines(path)
    with open(path, "w", encoding="latin-1", newline="\n") as f:
        f.write("\n".join(lines) + "\n")
    changed = [line for line in difflib.unified_diff(before, lines, "before", "after", lineterm="", n=0)]
    for line in changed:
        print(line)
    print("%s: %d lines, %s" % (path, len(lines), "changed" if changed else "unchanged"))
    return 0


def main(argv):
    if len(argv) == 4 and argv[1] == "strip-saves":
        strip_saves(argv[2], argv[3])
        return 0
    if len(argv) == 4 and argv[1] == "quit-cuts":
        quit_cuts(argv[2], argv[3])
        return 0
    if len(argv) == 3 and argv[1] == "fake-replays":
        fake_replays(argv[2])
        return 0
    if len(argv) == 5 and argv[1] == "corrupt-replay":
        corrupt_replay(argv[2], argv[3], argv[4])
        return 0
    if len(argv) >= 3 and argv[1] == "savestate":
        for p in argv[2:]:
            print("%s: %s" % (p, save_state(p)))
        return 0
    if len(argv) in (3, 4) and argv[1] == "compare":
        return compare(argv[2], argv[3] if len(argv) == 4 else None)
    if len(argv) == 4 and argv[1] == "expect-update":
        return expect_update(argv[2], argv[3])
    if len(argv) == 4 and argv[1] == "select-cases":
        for name in select_cases(argv[2], argv[3]):
            print(name)
        return 0
    if len(argv) == 3 and argv[1] == "cache-key":
        print(tree_digest(argv[2], sys.stdin.buffer.read(), mask_days=True))
        return 0
    if len(argv) == 3 and argv[1] == "tree-hash":
        print(tree_digest(argv[2]))
        return 0
    if len(argv) == 4 and argv[1] == "shift-hiscores-days":
        shift_hiscores_days(argv[2], int(argv[3]))
        return 0
    if len(argv) == 4 and argv[1] == "hiscores-day":
        return 0 if hiscores_of_day(argv[2], int(argv[3])) else 1
    sys.stderr.write(__doc__)
    return 2


if __name__ == "__main__":
    sys.exit(main(sys.argv))
