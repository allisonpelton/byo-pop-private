# Publishing

The public repository does not get this repository's history. It gets snapshots: `tools/publish.py` takes the
trunk's tree as it is, leaves out what is private and commits the result on top of the last snapshot. Each publish
adds one commit there, and the full history stays on this machine (the user's decision of 2026-10-25). Only the
coordinator publishes, after checking the trunk for game files and game text (CLAUDE.md).

## What is private

Files listed in `.publishignore` are left out whole, each with its reason there: the tools that copy a game's program
out of PopCap's DRM wrapper or CD launcher (`tools/extract_game_exe.py`, `tools/triage_popcap.py` and the like), the
games' scripts that carve or patch a copy, origrun's table of install paths, and the games that stay unpublished. The
decompilation needs the tools, so they stay in the trunk, but the public repository does not show how to take the
protection apart.
BookWorm's GameText is published: it reads the game's texts from the player's own `Bookworm.exe` at run time and
looks inside the launcher only to find them.

Passages of text files that describe the unwrapping (offsets, commands, how a launcher checks its copy) are wrapped
in marker lines:

    <!-- private: The line the published copy shows instead. -->

    ...the private passage...

    <!-- /private -->

Each marker is a line of its own that starts in the first column (indented, as here, it is only text), and blocks
do not nest. The published copy has the replacement line in place of
the whole block, or nothing when the opening marker is a plain `<!-- private -->`. Markdown shows the markers as
nothing, so the docs read as before here. A marker line ends a Markdown list or table, so put the markers around
whole items, and write the replacement as a whole item too (`<!-- private: - The item. -->`). In a code block the
markers would show, so close the block before the private lines and open a new one after them.

## Making a snapshot

```bash
python tools/publish.py --model "Opus 5.5"
```

`--model` names the model making the snapshot, for the commit's `Co-Authored-By` trailer. This builds the snapshot into `refs/publish/next` and checks it: no `.publishignore` path, no marker line, nothing
under `assets/`, `re/` or `build/`. It prints what it left out and which files lost private blocks. Look at the
result with `git show --stat refs/publish/next` or `git show refs/publish/next:<path>`, then publish it:

```bash
python tools/publish.py --push --model "Opus 5.5"
```

This builds the snapshot again and pushes it to origin's `main`. `refs/publish/main` then marks the last published
snapshot, which is the parent of the next one; when the tree has not changed since, there is nothing to publish.
The commit's author is whoever git's configuration names, and its message is "byo-pop snapshot, <date>" unless
`-m` gives another first line.

## Derived repositories

A derived repository is a read-only copy of one finished game: the game's code and notes, the shared code it links
and a top-level `CMakeLists.txt` and `README.md` that build and describe that one game. It is generated from the
trunk, never edited, and its README sends changes to this repository. Rocket Mania is the first
(`https://github.com/allisonpelton/byo-pop-rocketmania`). A game gets one once its entry in `docs/game_status.json`
shows every step done. It has no history either, only snapshots.

A game's manifest is `tools/publish_targets/<game>.txt`, read from the trunk: its repository's url, the paths to take
(`include` and `exclude`, a file or a folder each) and the generated files (a `file` line puts a trunk file at another
path, a `transform` line runs one of the functions in `tools/publish.py`'s `TRANSFORMS` on a trunk file). To add a game,
copy `rocketmania.txt`, list its folders, the libraries its CMake targets link (follow `games/<game>/CMakeLists.txt`
and `third_party/CMakeLists.txt`) and its staging script, leave out the tools that need an original exe, origrun or the
Ghidra exports, write its README next to the manifest, and run the check below. The `toplevel` transform takes the
game's folder name; the other two cut the framework's and `third_party`'s lists down to the libraries that were taken,
so a game that needs another library extends them. The `.publishignore` paths and private blocks apply as for the
whole repository, and so do its checks; a manifest path that matches nothing stops the publish.

No file of a derived repository refers to the site or to the mods (the user, 2026-11-13), and the trunk's sources
carry markers only where the code itself differs (the user, 2026-11-14). Four kinds of manifest line see to it.
`rewords <table>` gives the published copy its own wording of the trunk comments that name the site or the mods,
while the trunk keeps its own: each entry of `tools/publish_targets/<game>.rewords.txt` names a file, the trunk's lines
exactly and the lines that replace them, and the publish stops when an entry's lines are no longer in its file, so a
trunk edit to such a comment means changing the entry too. `unifdef -U__EMSCRIPTEN__ -DBYOPOP_DERIVED` then decides
those two conditionals in every C and C++ file taken, as the `unifdef` tool does: the web build's branches go, and so
does what the trunk wraps in `#ifndef BYOPOP_DERIVED`, with its `#else` branch, when it has one, in its place. The
trunk wraps that way only code: the includes, calls, members, functions and signatures of the mod menu, the mod
manager, the restart, the web build's backup and the autosplit block, with the comments that sit inside that code.
Nothing defines `BYOPOP_DERIVED` in the trunk, so a normal build compiles exactly what it did before; a wrap's lines
start at column 0. Each `forbid <regex>` stops the publish while a line of a published file still matches it, and
lists every such line, so a new reference in the trunk is caught at the next publish and not on GitHub: give it a
rewords entry, wrap its code, or leave its file out. `vendor <path>` leaves a path (`third_party`) as it is, untouched
by all three. The `toolchain` and `stage` transforms reword the CMake toolchain file and the staging script. Rocket
Mania has no fixes and no lite changes, so leaving `platform/GameOptions` out changes nothing; a game that has them
needs its `FixOn` and `LiteOn` calls decided before its manifest can leave that file out.

```bash
python tools/publish.py --target rocketmania --model "Opus 5.5"          # into refs/publish/rocketmania/next
git show --stat refs/publish/rocketmania/next
git diff --stat refs/publish/rocketmania/main refs/publish/rocketmania/next   # against what GitHub has now
python tools/publish.py --target rocketmania --build-check --release-zip # builds it, plays a game, writes the ZIP
python tools/publish.py --target rocketmania --push --model "Opus 5.5"   # to the manifest's url, then refs/publish/rocketmania/main
python tools/publish.py --target rocketmania --source <commit> --out <new folder>   # the tree only, no commit
```

`--build-check` needs `LLVM_MINGW_ROOT` set (here `E:/retools/llvm-mingw-20261006-ucrt-x86_64`): the derived repository's
toolchain file asks for it where the trunk's falls back to this machine's folder, and without it cmake stops before it
looks for Ninja or a compiler.

`--out` writes the tree a snapshot would hold into a new folder and makes no commit, which is how to try a manifest
or new wraps before they are committed (`--source` takes any commit, such as one made with `git commit-tree` from a
temporary index). `--release-zip` builds the snapshot as the build check does and writes
`build/byo-pop-<game>-<date>.zip`, the date being the snapshot's. It holds one folder: the exe, `README.txt` (the
README up to its `## Building` heading, with Windows line ends) and the empty `assets/<game>/` folder for the player's
own game files. No game file goes in, and nothing is uploaded.

### What a release holds, and the small SDL

The user's wish (2026-11-15): "For all the releases, I want to build everything except assets into the exe, and I want
to minimize the uncompressed size of that exe without affecting performance". So a release is one exe, with no DLL
beside it: its imports are Windows' own (`--release-zip` stops when one is not). The derived top-level `CMakeLists.txt`
(the `toplevel` transform) sees to it for every game, and a manifest line sees to what differs per game:

- **SDL is linked in.** `third_party/sdl-small/build.py <SDL2-2.32.10 source> <install folder>` builds SDL 2.32.10 from
  its own release source, unmodified, as a static library at -O3 with its functions and data in sections, without unwind
  tables and with its dynamic API off, keeping only what the ports call: the Windows video driver and the dummy one,
  the Direct3D 11, Direct3D 9, OpenGL and software renderers, WASAPI, DirectSound and the dummy audio driver (the test
  runs set `SDL_AUDIODRIVER=dummy`), events, threads, timers, files, the clipboard and OpenURL. Left out: joysticks,
  haptics, sensors, HIDAPI, XInput, power, Vulkan, OpenGL ES, the offscreen and disk audio drivers, the Direct3D 12
  renderer, and the test library. The ports never call `SDL_Init`, only `SDL_InitSubSystem` for video and audio, so a
  subsystem left out cannot make it fail. On this machine it is installed in
  `E:/retools/SDL2-2.32.10-small-i686` (the derived build and `--release-zip` look there unless `SDL2_DIR` says
  otherwise), and the derived top-level file links `SDL2::SDL2-static`. The recipe is in every derived repository
  (`include third_party/sdl-small`), and its README's "Building from source" says how to run it first.
- **No exceptions.** The derived build compiles with `-fno-exceptions -fno-unwind-tables -fno-asynchronous-unwind-tables`
  and links `--gc-sections` over `-ffunction-sections -fdata-sections`, `-static-libstdc++ -static-libgcc`, and `-s` in
  a Release build. Nothing in a derived repository catches an exception (`framework/XMLWriter.cpp`'s one handler guards
  an `fprintf` that cannot throw), and the only throws are `ResourceManager`'s `Get...Throw` functions, which end the
  program (`std::terminate`) as the uncaught throw did. Both files compile either way (`__cpp_exceptions`), so the trunk
  keeps its exceptions: AstroPop, Bejeweled 2, Bejeweled Twist, BookWorm Adventures 1 and 2 and Hammer Heads catch
  `ResourceManagerException` in their `Res.cpp`, so a derived repository of one of them needs that decided first.
  `platform/DerivedRuntime.cpp`, empty in the trunk, becomes real code in a derived repository and goes into the exe
  itself (the `framework` transform): a `__cxa_demangle` that demangles nothing, which keeps libc++abi's demangler (some
  140 KB) out, and `operator new` and `delete` over `malloc`, which end the program when memory runs out as the
  uncaught `std::bad_alloc` did, and keep the unwinder and its tables out.
- **No iostream.** The framework reads script lines, registry text and point arguments with `framework/TextScanner.h`,
  which parses exactly as `std::istringstream` did (`tools/textscannertest.cpp` compares the two on 600,000 strings),
  so libc++'s locale and iostream code (about 250 KB) is not linked into any game's exe.
- **The save-import table of the game only.** The `saveimport` transform takes the game's name as its argument and
  leaves that game's entry in `platform/saveimport.json`.
- **Only the image formats the game's files hold.** The `stb` transform takes a list (`png,gif` for Rocket Mania) and
  defines `STBI_ONLY_<format>` for each in `third_party/stb/stb_impl.c`. `platform/ImageLoader.cpp` hands JPEG files
  to libjpeg first and GIF files to its own decoder, and stb_image decodes the rest and what they refuse. Read the
  list from the game's files in the staged `assets/<game>/`, the Steam and disc copies and `re/_installers/`: all
  of Rocket Mania's images are GIF, JPEG (baseline or progressive, 8 bits, 1 or 3 components, which libjpeg takes) and
  PNG, with no TGA or BMP. A format a player's copy can hold stays in the list.
- **-O3 stays.** The code is compiled and optimised as the trunk's; the size comes from what is not linked.

A derived repository stays open, not archived: GitHub's documentation ("Archiving repositories") says an archived
repository takes no push and no release until its owner unarchives it, and the user chose to keep it open (2026-11-15).
Each update is the `--push` command, then, in the browser since `gh` is not installed, a new release on the
repository's Releases page, its tag named after the snapshot's date, with the ZIP attached.

The snapshot is "byo-pop-rocketmania snapshot, <date>" with the same trailer. The build check extracts the snapshot into
a temporary folder outside the repository, configures and builds it with the generator and toolchain of
`docs/BUILDS.md`, then runs one scripted `-fast` game of the bot (the one `tools/check.sh rocketmania` plays) with the
derived exe and with the trunk's `build/native` exe, both under rundesk on the main checkout's staged files. The
two state logs must be byte-identical. It takes a few minutes, so run it in the background. The browser version is not
in a derived repository, since the page around it is the site's (`web/`); it is built from the trunk.

Each publish of the trunk also refreshes every derived repository: after `python tools/publish.py --push`, run the
`--target` commands above for each manifest, with the release step, so that the derived repositories
never lag behind the code they were made from.

<!-- private -->

## Backups

The snapshots hold no history, so on 2026-11-08 the user asked for backups as well ("GitHub still only shows the
October 25 snapshot. Better fix that, as otherwise I have no backups"), and made a private repository for what cannot
be shared. There are two, and the coordinator refreshes both after a batch of merges and after each publish.

The first is the private repository `https://github.com/allisonpelton/byo-pop-private`, git's remote `private`. It
gets the trunk's whole history and every local branch, the private blocks and the `.publishignore` paths included:

```bash
git push private --all
```

It holds only what git tracks, so no game file goes there either. GitHub refuses a branch whose newest commit names the
user's own address rather than their noreply one (its email privacy setting). Ten old commits on the trunk do, and
`claude/site-easy-b32d6e`, whose tip is one of them, was refused for that on 2026-11-08; its commits are in the trunk,
so nothing is missing. A branch that a session rebased is refused as not a fast-forward: push that branch alone with
`--force` once you have checked that the branch, not the backup, holds the newer work. The coordinator's permission
check refuses a forced push (2026-11-09, Twist's readability branch after its work-in-progress commit was split), so
such a branch waits for its merge into the trunk, which the next push carries; until then the mirror below holds it,
`.git` included.

The second stays on this machine: the main checkout, git-ignored files and all (staged assets, `re/`, the frozen
baselines and the oracle cache under `build/`, `secrets/`), mirrored to `E:\backups\byo-pop` on another drive. Run it
from PowerShell:

```powershell
robocopy C:\Users\Administrator\Documents\claude\byo-pop E:\backups\byo-pop /MIR /XJ /XD C:\Users\Administrator\Documents\claude\byo-pop\.claude\worktrees /R:1 /W:1 /NP /NFL /NDL
```

It leaves out the sessions' worktrees (their commits are in `.git`, and their ignored files are mostly copies of the
main checkout's) and follows no junction. A git-ignored file a session makes that is not a copy (a game's origrun
config, its test patch list) belongs in the main checkout, or it goes with the worktree: Bejeweled's identical-play
config and seed patch list were lost that way (found 2026-11-09). `/MIR` makes the copy match the checkout, so a file deleted here goes from
the copy at the next run too: it is a mirror, not a history.

<!-- /private -->
