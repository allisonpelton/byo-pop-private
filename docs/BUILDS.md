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
