# BookWorm Deluxe: mods

The user's list and order (2026-10-24): the word solver first, then undo, then a bigger dictionary with the
Collins Scrabble Words, then everything else proposed (below), including the user's rude-words mod and the
comparison with greedy bots on the game-over screen. Everything builds on what the port already has: the
shared VFS and ModManager (docs/MOD_MENU.md: a mod is a folder with a mod.json; the web shell's Mods panel
takes a folder or a .zip), HD `@2x` images, and the game texts read by id (`GT_...`, overridable in
`properties/texts.xml`). Mods made from the user's own game files, and word lists that may not be
redistributed, stay git-ignored. How to install and test the mods that exist: README.md, Mods.

| Order | Mod | State |
|---|---|---|
| 1 | Word solver (`mods/solver`) | done |
| 2 | Undo (`mods/undo`) | done |
| 3 | Bigger dictionary, with Collins (`mods/collins`) | done |
| 4 | Rude words that set fires (`mods/rude-words`) | done |
| 5 | The greedy-bot comparison; HD art; the smaller ideas | next |

Code mods read their switches from the mod's `"features"` (`src/ModFeatures.h`: a feature on, a live
setting's value) and say `"affectsScores": true`, so their games get their own high scores and saved games
(`Scores-<key>`, `classic-<key>.dat`).

## 1. Word solver (done)

`src/Solver.cpp`, mod `solver`. The search is the `-autoplay` bot's, moved into the game (`FindBoardWords`,
which the bot now calls too): a DFS from every tile over neighbours not yet in the path, cut as soon as no
dictionary word starts with the letters so far; every path counts (a word around a gold tile and through it are
two candidates). Each path is scored with the game's own `WordScore`, so the points are exactly Submit's.

The hints, in order (HINT beside SUBMIT, or Tab; each press the next):
- **Fire** (while there are fire tiles, setting "fire first"): the word that takes the lowest fire tile, then
  the most fire tiles, then the points.
- **Bonus word**, when the board spells it.
- **Best**: the 10 best-scoring words (setting, 1..30).
- **Longest**: special tiles come from length (6 gold; 7-10 mostly sapphire; 11+ diamond).

Safety, from Submit's order (it burns before it removes the word's tiles): in Classic a word that leaves out a
fire tile on the bottom row ends the game; a word that eats the tiles under a fire tile drops it, onto the
bottom row at worst, where the next word must take it. Safe words come first, and the tag warns about the
others. Tested by the bot's `-autoplayhints=1` (README.md): every hinted word scored what the solver said.

Possible later: the whole list of hints in the widescreen layout's side panel (4.4).

## 2. Undo (done)

`src/Undo.cpp`, mod `undo`. UNDO (the OPTIONS plank split like SUBMIT | HINT) and Ctrl+Z. Before each word
Submit takes and each scramble, a snapshot: the board's saved-game record (`SaveGameSnapshot`, into memory)
plus what that record leaves out but the next tiles depend on:
- both generators whole (`gGameRand` draws letters, fire, special tiles and bonus words; the record keeps the
  board generator's state words but not its position in them);
- RandomLetter's program-wide previous letter (never the same letter twice running, and each rejected letter
  costs a draw) and Refill's previous level;
- the app's next bonus word (a bonus word played picks the next one, avoiding the current one).

Undo reads the last record back (`ReadSavedGame`, LoadGame's reader split out) once the tiles rest and the
eaten word's tiles have reached Lex. The clocks, the pause count and the held Ctrl key stay as they are; Lex's
definition balloon of the undone word fades and its "+points" popup goes. Only in play and within a level (a
level-up leaves the earlier level's moves behind); up to 10 moves (setting). The bot test (README.md) checks
the board after each undo and after the same word played again.

Possible later: undo after the move that ended a Classic game (the game-over sequence would have to be
stopped).

## 3. Bigger dictionary, with Collins (done)

`src/ModWords.cpp`, mod `collins`, a general feature `"words"`: every enabled mod with it adds the words of its
files to the dictionary at start-up (after `wordlist.txt`, in its format), and its definitions to Lex's (before
`popdefs.txt`, which wins where both define a word). A word only a mod has and no definition gets the mod's
note in the balloon. The rude-words mod can add its words the same way.
- **The list** is the player's own Collins Scrabble Words, never committed: `tools/make_collins_mod.py` writes
  `mods/collins/collins-words.txt` (git-ignored by the mod folder's own `.gitignore`) from a NASPA Zyzzyva
  install, whose word graph (`data/words/CSW24/*.dwg`) is plain, unencrypted data. CSW24 has 280,887 words:
  280,652 kept (no Q without a U, which the Qu tile can't spell; 3 letters or more), 129,593 of them new to
  the game's 151,381. Zyzzyva's own definitions are encrypted and are not read.
- **Definitions** come from a text list the user supplied (Collins Scrabble Words 2019 with definitions): the
  tool writes `collins-defs.txt` in popdefs.txt's style ("(n., French) Gloss.", "(v.) Form of BASE: gloss.",
  the senses one after another). 278,842 of the words have one; the rest (new in 2024) get the note.
- **A click on a definition balloon** opens the word in an online dictionary in the player's browser (setting:
  Collins Dictionary, which also covers the Scrabble words and sends inflected forms to their entry,
  Wiktionary, or nothing). Only Lex's definitions are links; his other speech and thoughts are not. This is a
  link, not an online feature: the game sends nothing anywhere until the player clicks. An online API for
  the checks themselves stays ruled out (too slow for the solver, and every word tried would go to a third
  party).

## 4. The rest (planned)

### 4.1 Rude words set fires (done)
Mod `rude-words`, the feature `"words"` (section 3) with `"fire"`: 312 swear words, crude slang and internet
slang (with their inflected forms) that the game's list lacks, in `mods/rude-words/rude-words.txt`. The list
is our own, written for the mod, so it is committed; it leaves out slurs aimed at groups of people (racial,
ethnic, homophobic, transphobic, ableist). 230 of them are in Collins too: with both mods on they burn all
the same, since each mod's words are those the game's own list lacks, whatever other mods add.
- **Settings** (`ModWordGetEffects`, read live): a points multiplier (-5..5 in halves, default 1: 0 scores
  nothing, below 0 takes points away, never more than the level has so far; `ModWordScore` in
  `WordScore::Build`, in tens like every score; a negative popup is in LockerGnome, the score font has no
  minus); every new tile burns, or a number of them, fixed (0..9, default 2) or one per so many letters (1..6,
  at least one); bonus tiles as usual, none for a rude word, or a rude word turns every special tile on the
  board plain (`Board::RemoveBonusTiles`; one under a fire tile then burns like a plain one).
- **Fire**: Submit sets `Board::mModFireDue` from the effects (every tile: all of them), and Refill sets that
  many of the tiles it drops in on fire: normal ones, never on the bottom row, picked with `gGameRand`, so an
  undo and the same word again bring the same fire. None when the word ends the level.
- **Lex** (setting "Lex comments", on by default): his balloon (`Lex::SayReaction`) has the word, one of 65
  comments of ours (`rude-reactions.txt`, the feature's `"reactions"`; picked from the word and the game's
  count, so an undo and the word again get the same one) and, with definitions on, the word's definition
  under it (the balloon's third block, `WordBalloon::SetFooter`); once he has eaten the word he plays his
  danger clip (`LEXANIM_DANGER` and its sound). The note "Such language!" stays for a mod without reactions.
  The solver ranks a word that sets fire after the safe ones and tags it "SETS FIRE".
- **Stats** (the feature's `"stats"` label): the game's count (`Board::mModWordsPlayed`, an int after the
  saved game's record while the mod is on) and the player's all-time total (a profile int,
  `ModWords-rude-words`) on the level and game-over page, under the times: "Rude words: 4, all time 4". An
  undo takes the undone word off both.
- Tested with a copy of the mod whose list is Collins (so the bot plays such words), hidden under rundesk: the
  default fires 2 tiles and the balloon reads word, comment, definition, and Lex's danger pose follows the
  eating where a plain word leaves him idle; -1 with fire by length (3 letters a tile) scores a 7-letter word
  -1400 with 2 fire tiles and turns the board's green tile plain; 0.5 with every tile burning scores it 700
  with 6 fire tiles; the solver bot with undo, points 2 and no fire (so its words are safe), 30 words with 9
  undone, leaves an all-time total of 21. Earlier: the plain bot burns out on level 2 (level 8 without), and
  the solver bot with undo, Classic 109 checks and Action 56, found no mismatch.

### 4.2 Against greedy bots, on the game-over screen (the user's idea)
At game over the stats screen also shows what two simple players would have scored from the same start: one
that always plays the best-scoring word, one that always puts out the most urgent fire first. At the start of a
game the board and the generators' state are kept; at game over a headless model of the Classic rules (Submit,
burning, refill, the fire meter, special tiles, level thresholds and bonus words, drawing from the generators
in the game's order) plays both bots from that start with the solver's search. The model is checked against
the real game with the `-autoplay` bot (same moves, same boards and scores).

### 4.3 HD art
A generator like Bejeweled's (Real-ESRGAN in `E:\retools`) that upscales the player's own images into a
git-ignored `@2x` mod: images and their alpha masks upscaled together; sprite strips cel by cel with padding;
JPEG backdrops with a denoising model; tile letters redrawn from a free vector font if upscaled ones look soft.

### 4.4 Smaller ideas
- **Missed-word review**: after each word or at game over, the best word that was there and the points missed.
- **Par mode**: each board's par is the solver's best word; the score is a percentage of par.
- **Daily board**: seeded from the date, with a local history (no online leaderboard).
- **Word journal**: every word played, the best per level, a letter heatmap, stored locally.
- **Colour-blind mode**: a symbol on green, gold, sapphire, diamond and fire tiles; a high-contrast tile skin.
- **LiveSplit**: levels as splits, as in Bejeweled and Big Money.
- **Widescreen layout**: a side panel for the solver's list and the word history.

### 4.5 Earlier proposals, still open
- **Definitions** for more words without Collins: `popdefs.txt` has 2,249; WordNet (free with its notice)
  through the `"words"` feature's `"definitions"`, in the original's one-line `(pos.) definition` style.
- **Other languages** need engine hooks (tunables, original values by default): the alphabet and its tile
  cels, letter weights (26 summing to 95,500) and values, the "Qu" rule, the minimum length. A language pack is
  then a word list, bonus words, definitions, `texts.xml` and tile art.
- **Gameplay variants**: board size, fire-tile rate, a timed sprint, a relaxed no-fire mode, set seeds.

## What works as plain data today (no code)

| File | Format | A mod can |
|---|---|---|
| `wordlist.txt` | sorted, prefix-compressed (an optional number of letters kept from the previous word, then the rest); only words of 3+ letters are used; A-Z only | replace the dictionary |
| `bonus_word_dictionary.txt` | one word per line (the bonus-word pool) | replace the bonus words |
| `popdefs.txt` | `WORD<TAB>(pos.) definition`, Latin-1, shown in Lex's balloon after long words | replace or add definitions |
| `properties/texts.xml` | `<String id="GT_...">...</String>` per text (ids in `src/GameTextTable.inc`) | change any text the game shows |
| `images/*` (+ `@2x`) | the original images and their `_name` alpha masks | reskin tiles, Lex, backdrops, dialogs, in HD too |
| `sounds/*`, `music/*` | Ogg / `.au`, the MO3 (orders matter) | replace sounds or the music |
