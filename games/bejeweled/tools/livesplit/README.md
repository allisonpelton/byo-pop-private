# Auto LiveSplit for the Bejeweled Deluxe port

The design is [CODE_MODS_DESIGN.md 4.15](../../docs/CODE_MODS_DESIGN.md). In short: with the `livesplit`
mod on (`features.livesplit`), the Windows build fills a tagged, read-only block in its `.data` after every
update (`games/bejeweled/src/LiveSplitBlock.h`, `LiveSplitObserver.cpp`), and the auto splitter script
here finds it in the game's memory and starts, splits and resets LiveSplit's timer. Real time only.

| File | What |
|---|---|
| `BejeweledDeluxe-byopop.asl` | The auto splitter (C# 5, for LiveSplit 1.8.37's ScriptableAutoSplit). Port only: the original 1.87 (`popcapgame1`) comes after its test (Q15) |
| `lsprobe/lsprobe.cpp` | `build/native/lsprobe.exe` (64-bit, like LiveSplit): finds and reads the block the way the script does, logs every poll; `-dumplayout` prints the layout |
| `check_lslog.py` | Every snapshot lsprobe accepted is one the game wrote (`-livesplitlog`), at the address it logged |
| `lsdecide.py` | The script's decisions re-implemented, with the scenarios and their expected events; runs `asltest.exe` on the same polls |
| `asltest/asltest.cs` | Compiles the script as LiveSplit does (the .NET Framework's C# 5 compiler) against stubs of the LiveSplit members it uses, and replays polls through it |
| `check_asl_offsets.py` | The script's magic and field table against `lsprobe -dumplayout`; the magic once in the exe's `.data` |
| `run_tests.sh` | All of the above on scripted and bot games (real time and `-fast`), on an invisible desktop |

The mod itself is generated: `python games/bejeweled/tools/make_example_mods.py --only livesplit` writes
`games/bejeweled/mods/livesplit/` (`mod.json`, the 12 split templates in `livesplit/`, a copy of the script
and `README.txt` for players). The game copies missing templates to `<user dir>/livesplit/` on activation
and never overwrites one.

## Tests

- `games/bejeweled/tools/livesplit/run_tests.sh`: the games (about 3 minutes, two at a time), lsprobe on
  each, `check_lslog.py`, `check_asl_offsets.py`, `asltest.exe -compileonly` and `lsdecide.py run`, which
  checks 27 scenarios on the game's own log (every update) and on lsprobe's log (real polling), each
  against its expected events and against the real script's decisions through `asltest.exe`. Three of them
  drop polls from a log (a LiveSplit stall), so that one poll sees a segment reached and the game over:
  the segment counts (it was reached by the game over), so the script splits first and resets in the next
  poll.
- The oracle (`games/bejeweled/tools/oracle/compare.sh`) runs every playtest, stage-0 and autoplay case
  with the mod and with an inert twin (`features.livesplit: false`): everything the game outputs must be
  identical, and the block's event log must agree with `-tracehash`'s states (compare.py `livesplit`).

What `asltest` shows: the script compiles as C# 5 with ScriptableAutoSplit's using list, every action runs
without an exception, and it decides what `lsdecide.py` decides. What it does not show: that LiveSplit's
real `ReadBytes`, `MainModuleWow64Safe`, settings and `TimerModel` behave like the stubs, LiveSplit's real
timing, or the Activate flow. The stubs follow the design's reading of LiveSplit 1.8.37 and
ScriptableAutoSplit 9c1e207; the manual test with LiveSplit itself (4.15 test 7) covers the rest.

## Waiting for the user

Every step below needs the user's approval (CODE_MODS_DESIGN.md section 11):
- Q16: a licence for the script (the registry requires an open-source one; byo-pop has none). Until then
  the script carries none.
- Q23: downloading LiveSplit 1.8.37 for the manual test (Normal 5 Levels and Score Attack 10K, recorded and
  retimed from video; the `log` setting measures the lag).
- Q14, Q17, Q22: contacting HyperNervie and the speedrun.com moderators, hosting, the registry PR.
- Q15: running the original 1.87 (with `lsprobe` extended by its `-original` adapter) before the script
  gets its `popcapgame1` path.
- The template names are speedrun.com's (bejeweled1, checked through its API on 2026-10-18): the
  categories `Normal`, `Timetrial` and `Timetrial - Score Attack`, whose subcategory variables are named
  `Normal`, `Timetrial` and `Timetrial - Score rush` ("5 Levels", "100K Points", ...).

Notes for the script: ScriptableAutoSplit ends an action where its brace characters balance, counting
those in strings, chars and comments too, so the script writes them as `(char)0x7B` and `(char)0x7D` and
keeps those in comments balanced; `asltest.exe` fails when that count would end an action anywhere but at
its closing brace. LiveSplit writes `AutoSplitterSettings` into a splits file only while the splitter is
active, so splits saved without it lose the script's settings. An undone split splits again on the next
poll (the script decides from the split index, the names and the block alone).
