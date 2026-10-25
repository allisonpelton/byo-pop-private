# Publishing

The public repository does not get this repository's history. It gets snapshots: `tools/publish.py` takes the
trunk's tree as it is, leaves out what is private and commits the result on top of the last snapshot. Each publish
adds one commit there, and the full history stays on this machine (the user's decision of 2026-10-25). Only the
coordinator publishes, after checking the trunk for game files and game text (CLAUDE.md).

## What is private

Files listed in `.publishignore` are left out whole. Today they are the two tools that copy a game's program out of
PopCap's DRM wrapper or CD launcher, `tools/extract_game_exe.py` and `tools/triage_popcap.py`. The decompilation
needs them, so they stay in the trunk, but the public repository does not show how to take the protection apart.
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
python tools/publish.py
```

This builds the snapshot into `refs/publish/next` and checks it: no `.publishignore` path, no marker line, nothing
under `assets/`, `re/` or `build/`. It prints what it left out and which files lost private blocks. Look at the
result with `git show --stat refs/publish/next` or `git show refs/publish/next:<path>`, then publish it:

```bash
python tools/publish.py --push
```

This builds the snapshot again and pushes it to origin's `main`. `refs/publish/main` then marks the last published
snapshot, which is the parent of the next one; when the tree has not changed since, there is nothing to publish.
The commit's author is whoever git's configuration names, and its message is "byo-pop snapshot, <date>" unless
`-m` gives another first line.
