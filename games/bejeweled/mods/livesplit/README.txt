LiveSplit for Bejeweled Deluxe (byo-pop)
========================================

This mod turns on the game's LiveSplit support (features.livesplit). The Windows build of the game then
keeps a small read-only block in its memory up to date, and the auto splitter script
BejeweledDeluxe-byopop.asl reads it: it starts LiveSplit's timer at GO, splits at each wormhole (or when
the score on screen reaches a split's number) and resets when the run's game ends. It changes nothing
else: no gameplay, no high-score tables, and none of the small signs other mods show (the crossbar on
GO's G, the fourth hint sparkle). The web build ignores it.

Using it
1. Enable this mod (natively: put this folder in the game's mods folder; mods.json enables new mods).
2. Start the game once: it copies the 12 split templates from livesplit/ to the livesplit folder in its
   user dir (the folder with savegame.dat), and never overwrites one there.
3. In LiveSplit: Open Splits, then one of those templates. Until the script is in LiveSplit's auto
   splitter list, load it by hand: Edit Layout, +, Control, Scriptable Auto Splitter, Script Path: the
   BejeweledDeluxe-byopop.asl in this folder.
4. The script's settings: start only in the category's mode (on), only without gameplay mods and a
   chosen seed (off), reset when the game ends (no moves, time up, New Game, a mode switch, quitting;
   each on, each can be turned off), and a log of each event with its lag (off).

Notes
- Real time only, as speedrun.com times the game: the timer runs on through pauses and dialogs.
- Segment names: a number of at least 1000 (10000, 25,000 or 25K) splits when the score on screen
  reaches it; any other name is a level and splits when that level's wormhole starts. "N Levels" ends at
  the N-th wormhole. A finished run is never reset by the script.
- A resumed saved game never starts the timer; a start by hand on it splits as usual (practice).
- LiveSplit saves the script's settings into the splits file only while the script is active, so
  splits saved without it lose them.
- Whether runs on this port count is for the speedrun.com moderators.
- Licence: BejeweledDeluxe-byopop.asl is CC0-1.0 (public domain dedication); see its header.
