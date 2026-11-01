# The build folder

Everything under `build/` is generated and git-ignored. `tools/build.sh <kind>` makes the four builds; the
test tools write their results next to them.

## Builds

| Folder | Made by | What it is for |
|---|---|---|
| `build/native` | `tools/build.sh native` | The Windows builds (32-bit, llvm-mingw) of every game (`bejeweled.exe`, `bigmoney.exe`, `bookworm.exe`) and of the tools: `rundesk.exe` (runs tests on a hidden desktop), `fwtest.exe` (framework tests), `rulestest.exe`, `modsettingstest.exe`, `bejeweled_verify.exe` (the headless replay verifier), `lsprobe.exe` (LiveSplit probe), `mo3info.exe`. What you play natively. |
| `build/native-novariants` | `tools/build.sh novariants` | Bejeweled built without its code-mod variant paths (`BEJ_NO_VARIANTS`). The oracle's third candidate: it proves the original rules play the same with the variant code compiled out. |
| `build/web` | `tools/build.sh web` | The browser builds: `<game>.html`, `.js` and `.wasm` for each game, plus the web builds of the tests. What you open to play in a browser (`.claude/launch.json` serves it). |
| `build/web-test` | `tools/build.sh web-test` | A second browser build, served on port 18766, so automated browser tests never touch the copy you play from. |

## Test inputs that are kept

| Folder | What it is |
|---|---|
| `build/baseline` | The frozen Bejeweled exe from before the code mods (the port at commit `9dcba7d`): the reference the oracle compares every change against. It is never rebuilt, so copy it to new checkouts; do not delete it. |
| `build/baseline-stage0` | The same port with the oracle's test hooks (code-mods stage 0): the reference for hashes, snapshots and bot runs. Never rebuilt either. |
| `build/oracle-cache` | The two baselines' oracle runs, keyed by exe, inputs and machine, so each oracle runs only the candidate. Safe to delete: it is rebuilt on the next run. |

`tools/setup_worktree.sh` copies these, the staged game files and the generated mods into a new worktree.

## Test results (replaced by the next run)

| Folder | Written by |
|---|---|
| `build/oracle`, `build/oracle-forcevariant`, `build/oracle-novariants` | `games/bejeweled/tools/oracle/compare.sh --variants default,forcevariant,novariants`; each has a `report.txt` |
| `build/verify-tests` | `games/bejeweled/tools/verify/run_tests.sh` |
| `build/livesplit-test` | `games/bejeweled/tools/livesplit/run_tests.sh` |
| `build/modmenutest` | `tools/modmenutest/run_tests.sh` |
| `build/logs` | build logs and test logs, when tools write them there |

Any other folder in `build/` is a leftover of a past review or experiment and can be deleted.

## Running the oracle

Moved here from CLAUDE.md on 2026-11-01; CLAUDE.md keeps the machine's rules for it (one oracle at a time, at most
three game processes, no wait loop in the shell that starts it).

- The oracle's three runs per change (default, `-forcevariant`, the novariants build) are one command:
  `games/bejeweled/tools/oracle/compare.sh --variants default,forcevariant,novariants` (into build/oracle,
  build/oracle-forcevariant, build/oracle-novariants; build the novariants exe first with
  `tools/build.sh novariants`). The frozen baselines' runs come from `build/oracle-cache` (keyed on the exes,
  inputs, game files and machine; see compare.sh's header); `--no-cache` makes every run now, `--jobs 1` runs
  one game at a time in the old order. The candidate is never cached, so the cache cannot hide a regression;
  delete `build/oracle-cache` when in doubt. A "Draw Count" note in a report is the machine's speed within a
  bound, not a difference (compare.py, DRAWS); compare.sh then drops the cache entries of the baseline runs
  involved, so the next oracle makes them again. If such a note repeats in the same case, check it with
  `--no-cache --jobs 1`.
- While working on a few cases, run only those (it saves time and tokens): `compare.sh --cases <glob>,...`,
  e.g. `--cases "shape_*,rules_inert"` (matched against the whole case name), with any `--variants`. It runs the
  matching cases, the cases compare.py compares them with, and the runs they read (Bejeweled's stage 7 cases,
  three ways: 164 s instead of about 950 s). Its report says it was partial and expect-update refuses it: the
  whole oracle is still the check before a commit.
