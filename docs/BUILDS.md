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

`tools/setup_worktree.sh` copies these, the staged game files and the generated mods into a new worktree. Copies already there are not touched, so when the coordinator refreezes a baseline or restages a file, an older worktree is brought up to date with `tools/setup_worktree.sh --refresh`, which `tools/check.sh --changed` runs by itself at its start inside a worktree: it copies what differs from the main checkout (by size and time: a file whose size differs, or that is newer in main, or is missing) for every game the branch does not own, keeps the games it changed, deletes nothing and prints one line.

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

CLAUDE.md has the machine's rules for it (one oracle at a time, at most three game processes, no wait loop in the
shell that starts it).

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
builds always run alone). The web build runs in the background beside them, since it starts no game: only `unit-web`,
`verify` and the oracle read `build/web`, so they wait for it and come after the game suites. A game's page is written
apart from its link (`cmake/ByopopWebPage.cmake`), so an edit to `web/shell.html`, `web/modpanel.js` or a game's
`web/page.*` rebuilds the pages in seconds and relinks no game. The coordinator merges with the merge mode (the user, 2026-11-02: running every suite and
the oracle for every merge was not feasible) and runs `all` before each publish.
Neither `all` nor a shared change runs Bejeweled's oracle (the user, 2026-11-11: "Seems like the Bejeweled oracle is
causing a lot of slowdown for no benefit"): it compares Bejeweled's game logic with the original's, and a change outside
`games/bejeweled` that leaves Bejeweled's frozen baseline and `verify` unchanged leaves that logic unchanged too. It runs
when `games/bejeweled` changes, or when named (`tools/check.sh oracle`).

## Trimming an exe

The user (2026-11-13): "Let's stick to trimming the exes to only what the game uses then". It is not `-Os`, which costs speed:
none of the steps below changes the code the compiler makes for the game. Two kinds of trimming, "both" in the user's answer
to why not take unused code out of the source: the linker drops what a game does not reach, and what no game uses is left
out of the build by configuration. Sizes are Bejeweled 2's and Venice's Release exes (32-bit, llvm-mingw).

- **Sections and a static C++ runtime (the top-level `CMakeLists.txt`, MinGW builds).** `-ffunction-sections
  -fdata-sections` and `-Wl,--gc-sections` let the linker drop every function and data item nothing reaches, and
  `-static-libstdc++ -static-libgcc` link libc++ and libunwind into the exe, so only the parts used ship and no C++ runtime DLL
  goes beside it (SDL2.dll is the only DLL left, and `PORT_RUNTIME_DLLS` is empty but for AddressSanitizer's runtime in a
  build whose flags have `-fsanitize`). `-DBYOPOP_STRIP_EXE=ON` also strips a Release exe (`-s`); it is off in this
  repository's builds, whose crash logs (`llvm-symbolizer`), leak tools and profilers (`cpuprof.py`'s sampling mode,
  `llvm-nm`) read the symbols. A derived repository's exe has been built this way since `594ad74a`.
- **libopenmpt reads MO3 only (`third_party/CMakeLists.txt`, `third_party/openmpt-mo3/`; native builds).** Every module the
  games hand to libopenmpt is an MO3 (`platform/openmpt/OpenMPTPlayer.cpp` passes it files that start with "MO3"; the games'
  IT and S3M tunes go through libxmp-lite), and `ReadMO3` builds the module itself. libopenmpt's format table
  (`ModuleFormatLoaders` in `Sndfile.cpp`) names all 59 loaders, which kept every one of them in the exe. The build now
  leaves out the `Load_*.cpp` files of the formats no game uses and `openmpt-mo3/loader_stubs.cpp` gives the table their
  names as loaders that find nothing; libopenmpt's source is not edited. Load_it, Load_dmf, Load_ams and Load_s3m stay,
  because `ReadMO3` calls code in them (the plugin-chunk and extended-property readers, the DMF and AMS sample unpackers,
  the S3M effect converter). Bejeweled 2's exe goes from 4.96 to 4.16 MB. The web build keeps every loader: its music player
  plays every format libopenmpt reads. A game that wants another format takes its file out of the removed list and its name
  out of the stub file.
- **Link-time optimisation (`-DBYOPOP_LTO=thin|full`, off by default).** Measured on Bejeweled 2 and Venice (from-scratch
  builds of both, six compile jobs, back to back): no LTO 64 to 68 s, thin 59 s, full 81 s. Bejeweled 2's exe 4,161,536 bytes
  without it, 4,042,752 with thin (2.9% smaller), 4,051,456 with full; Venice's 3,115,008, 3,159,040 and 3,124,736 (thin 1.4%
  larger). Compressed with xz, thin is larger for both (Bejeweled 2 1,511,540 to 1,530,764 bytes), so it does not trim what a
  player downloads and it is not on. Processor time on the recorded Bejeweled 2 game was no worse with either (cycle counts,
  pinned pairs, the figures are in the commit that added the option); the bots' games, the frozen baselines and the unit suite are identical with thin.
- **SDL (`SDL2_DIR`).** The default build links the prebuilt `SDL2.dll` (1,911,808 bytes). SDL is also built from its own
  source here, configured to the subsystems the ports call and with no source edited, by `third_party/sdl-small/build.py`
  (docs/PUBLISHING.md, "The small SDL"; installed in `E:/retools/SDL2-2.32.10-small-i686`), and a derived repository links
  that static build into its exe. The main build takes it too when `SDL2_DIR` names it
  (`-DSDL2_DIR=E:/retools/SDL2-2.32.10-small-i686/lib/cmake/SDL2`, in a build folder of its own): no `SDL2.dll`, and SDL's part
  of Bejeweled 2's download falls from 567 KB compressed to 167 KB (exe 4,535,808 bytes, xz 1,678,156 against 2,078,412 for
  exe and DLL; Venice's exe 3,488,768). Bejeweled 2 and Venice pass their checks that way, baselines identical. It stays
  opt-in: the small build has no software renderer, which `DDInterface`'s last-resort fallback and the `scaletest` tool ask
  for, and `tools/build.sh` still points at the prebuilt package.
