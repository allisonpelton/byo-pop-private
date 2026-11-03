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

## Checking a merge

`tools/check.sh` runs the builds and tests, one summary line per suite, with the logs in `build/logs/check/`
(`tools/check.sh --help` lists the suites and how long each takes; `all` takes about 45 minutes). Its merge mode,
`tools/check.sh --changed <rev>`, picks the suites a change can affect from `git diff --name-only <rev>` (the
working tree, staged changes included, so `--changed HEAD` after `git merge --no-commit`; `<a>..<b>` names two
revisions). `--dry-run` prints the choice, one line per suite with a rough time and the folders that chose it, and
exits. Docs, `*.md`, `testnotes/` and a game's `docs/` need nothing. A game's folder chooses the build, the unit tests,
that game's own suites and the web build of that game only; the website (`web/`, `cmake/ByopopWeb.cmake`,
`cmake/ByopopStores.cmake`) chooses the web build and `unit-web`; a folder under `tools/` chooses what runs it.
Shared code (`framework/`, `platform/`, `third_party/`, the CMake files) chooses every suite, as `all`, unless the
review of the merge found each shared change inert (off unless a game opts in, or reached only from the merging game)
and the merge is run with `--inert`: then it is the build, the unit tests, `gpu` when a GPU or GL file changed, the
web build of the games that changed and of the shared libraries, and every game suite in a quick form (about 7
minutes for a typical merge). The quick forms of `pixelus`, `bookwormadv` and `tiptop` (`<suite>:quick`, or
`CHECK_QUICK=1`) leave out only what dominates their time, and still start the game, play it and compare a rerun.
In merge mode the game suites run up to three game processes at a time (`--jobs`; the oracle, livesplit and the
builds always run alone). The coordinator merges with the merge mode (the user, 2026-11-02: running every suite and
the oracle for every merge was not feasible) and runs `all` before each publish.
