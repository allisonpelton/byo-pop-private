# Noah's Ark Deluxe: mods

The user approved mods for this one game (2026-10-25, the Sonnet trial in CLAUDE.md). This is the brainstorm, what was
chosen and why, and how each mod works. The mods are the shared mod system's (docs/MOD_MENU.md: a folder with a
mod.json; the mod menu opens with F2 in the game, the web page has a Mods panel). A code mod names a feature in
`"features"` and the game reads its settings live (`src/ModFeatures.h`); every mod here says `"affectsScores": true`,
so the games played with a mod set have their own high scores (`hiscores-<key>.dat`) and saved game
(`savegame-<key>.dat`) and never mix with the original's.

## The brainstorm

What this game is: a path-linking game (join two equal animals through empty cells) with a flood that rises, in three
modes, plus a bonus round. What it lacks that a player would want: a way back from a misclick, a way to ask for a move
on demand (its own hint only comes after idle time), control over the flood, and anything that makes a game
comparable between players. Ideas, in the order they came:

1. Undo for the last moves. Cheap here, because the saved game already writes the whole board into a string, so a
   snapshot is a call to `SaveState`. Chosen.
2. A move helper on a key: show a move, and play it with a second press. The original's hint code does the finding. Chosen.
3. A tide control: the flood's pace as a percentage, with 0 for a game of pure puzzling and 400 for a rush. One number in
   the flood's period, so it is nearly free. Chosen.
4. A daily board: every level starts from the same animals for everyone on a day (and a setting to play another day's).
   The level generator draws from the C library's generator, so seeding it from the date, mode, difficulty and level is
   enough. Chosen.
5. Not made: a turn limit for paths (the Onet rule, at most two turns), which changes the search the game uses to find
   trails; replays (the game's own input is mouse-only and the framework has a recorder in BookWorm's port that could
   be copied, but replays need the generator state to be repeatable, which Noah's cosmetic draws share); colour-blind
   symbols (the animals differ in shape as well as colour); a statistics journal; HD art (needs an upscaler that is
   not on this machine, and a download needs the user's OK); widescreen.

## The mods

| Mod | Feature | What it does |
|---|---|---|
| `undo` | `undo` | Ctrl+Z or U takes back the last move (setting `depth`, 1 to 100, default 10): the animals, the score and the combo as they were. The flood stays where it is, so a take-back costs time; the setting `restoreWater` brings the old water level back too. A snapshot is taken before a click that starts a move, and only while the board holds still. Within a level. |
| `helper` | `helper` | H flashes a pair that can be joined, or in the puzzle a swap that makes three, at any time; H again plays it as two clicks (setting `playSecond`). Not in the Stampede bonus round. |
| `tide` | `tide` | Setting `speed`, 0 to 400 percent of the original pace of the flood in the action game; 0 holds the water back. From the next level. |
| `daily-board` | `daily` | Seeds each level's board from the date, the mode, the difficulty and the level, so everyone gets the same one; names the day under the ark. Setting `date` (yyyy-mm-dd) plays another day's. Strategy rows and puzzle refills that arrive later still depend on the player's timing. |

## Testing

`games/noahsark/tools/run_mod_tests.sh` (part of `tools/check.sh noahsark`) runs a script that presses H twice and
Ctrl+Z and then quits to the menu (the game logs "mod helper: ..." and "mod undo: ..."), checks that the saved game
has the mods' key in its name, plays a daily board twice with different game seeds (same screenshot) and on another
day (a different one), and plays the action bot with the tide at 400 and at 0.

## Install

Natively copy a mod's folder into `build/native/mods-noahsark/` (or pass `-modsdir=`); on the web give the Mods panel the
folder or a .zip of it.
