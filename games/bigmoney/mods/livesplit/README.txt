LiveSplit for Big Money Deluxe (byo-pop)
=========================================

This mod turns on the game's LiveSplit support (features.livesplit). The Windows build of the game then keeps
a small read-only block in its memory up to date, and the auto splitter script BigMoneyDeluxe-byopop.asl
reads it: it starts LiveSplit's timer at the first level's "Go!", splits when a level is complete (or when the
score on screen reaches a split's number) and resets when the run's game ends. It changes nothing else: no
gameplay, no high-score tables, and no mods sign on the score. The web build ignores it.

Using it
1. Enable this mod (natively: put this folder in the game's mods folder, build/native/mods-bigmoney).
2. Start the game once: it copies the split templates from livesplit/ to the livesplit folder in its user dir
   (the folder with savegame.dat), and never overwrites one there.
3. In LiveSplit: Open Splits, then one of those templates. Load the script: Edit Layout, +, Control,
   Scriptable Auto Splitter, Script Path: the BigMoneyDeluxe-byopop.asl in this folder.
4. The script's settings: start only in the category's mode and skill (on), only without gameplay mods and a
   chosen seed (off), reset when the game ends (coins overflow, no moves, New Game, another game, quitting;
   each on, each can be turned off), and a log of each event with its lag (off).

Notes
- Real time only: the timer runs on through pauses and dialogs.
- Category names: "Action", "Strategy" or "Puzzle" and "Easy", "Normal" or "Hard" in the name make the script
  start only on that mode and skill. Big Money has no speedrun.com board; the templates' categories are only
  suggestions.
- Segment names: a number of at least 1000 (10000, 25,000 or 25K) splits when the score on screen reaches it;
  any other name is a level and splits when that level is complete. A finished run is never reset by the
  script.
- A resumed saved game and a replay never start the timer.
- LiveSplit saves the script's settings into the splits file only while the script is active.
