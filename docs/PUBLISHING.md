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

```bash
python tools/publish.py --target rocketmania --model "Opus 5.5"          # into refs/publish/rocketmania/next
git show --stat refs/publish/rocketmania/next
python tools/publish.py --target rocketmania --build-check               # builds that snapshot and plays a game
python tools/publish.py --target rocketmania --push --model "Opus 5.5"   # to the manifest's url, then refs/publish/rocketmania/main
```

The snapshot is "byo-pop-rocketmania snapshot, <date>" with the same trailer. The build check extracts the snapshot into
a temporary folder outside the repository, configures and builds it with the generator and toolchain of
`docs/BUILDS.md`, then runs one scripted `-fast` game of the bot (the one `tools/check.sh rocketmania` plays) with the
derived exe and with the trunk's `build/native` exe, both under rundesk on the main checkout's staged files. The
two state logs must be byte-identical. It takes a few minutes, so run it in the background. The browser version is not
in a derived repository, since the page around it is the site's (`web/`); it is built from the trunk.

Each publish of the trunk also refreshes every derived repository: after `python tools/publish.py --push`, run the
`--target` commands above for each manifest, so that the derived repositories never lag behind the code they were made
from.

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
