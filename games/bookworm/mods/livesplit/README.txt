LiveSplit for BookWorm Deluxe (byo-pop)
=======================================

This mod turns on the game's LiveSplit support (features.livesplit). The Windows build of the game then keeps
a small read-only block in its memory up to date, and the auto splitter script BookWormDeluxe-byopop.asl
reads it: it starts LiveSplit's timer when a new game begins to play (with the help screen on, when you close
it), splits when a level is complete (or when the game's score reaches a split's number) and resets when the
run's game ends. It changes nothing else: no gameplay, no high-score tables, and no mods sign on the score.
The web build ignores it.

Using it
1. Enable this mod (natively: put this folder in the game's mods folder, build/native/mods-bookworm).
2. Start the game once: it copies the split templates from livesplit/ to the livesplit folder in its user dir
   (the folder with registry.ini and players/), and never overwrites one there.
3. In LiveSplit: Open Splits, then one of those templates. Load the script: Edit Layout, +, Control,
   Scriptable Auto Splitter, Script Path: the BookWormDeluxe-byopop.asl in this folder.
4. The script's settings: start only in the category's mode (on), only without gameplay mods (off), reset
   when the game ends (a fire tile burns through the bottom in Classic, the countdown runs out in Action,
   you leave the game, another game replaces it, the game quits; each on, each can be turned off), and a log
   of each event with its lag (off).

Notes
- Real time only: the timer runs on through pauses, dialogs and the help screen.
- The timer starts when the game first plays: with the help screen on (the default), that is when you close
  it; without, as soon as you pick Classic or Action.
- Category names: "Classic" or "Action" in the name make the script start only on that mode. BookWorm has no
  speedrun.com board; the templates' categories are only suggestions.
- Segment names: a number of at least 1000 (50000, 25,000 or 25K) splits when the game's score reaches it
  (the moment the word is scored, before the score on screen has rolled up); any other name is a level and
  splits when that level is complete. A finished run is never reset by the script.
- A level is complete when a word takes the score past the level's mark while the board is still playing. A
  word that does that and also lets a fire tile burn through the bottom ends the game instead.
- A continued saved game never starts the timer. Leaving a game (LEAVE GAME) saves it and resets the run;
  continuing it later is practice.
- LiveSplit saves the script's settings into the splits file only while the script is active.
