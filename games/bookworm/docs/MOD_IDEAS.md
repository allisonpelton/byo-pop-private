# BookWorm Deluxe: mod proposals

Mods for the word list, the definitions, the bonus words, the texts and the art. Everything here builds on
what the port already has: the shared VFS and ModManager (a mod is a folder that replaces game files; the
web shell's Mods panel takes a folder or a .zip), HD `@2x` images, and the game texts read by id
(`GT_...`, overridable in `properties/texts.xml`). Nothing is built yet: these are proposals for the user to
choose from. Mods made from the user's own game files stay git-ignored, like Bejeweled's.

## What works as plain data today (no code)

| File | Format | A mod can |
|---|---|---|
| `wordlist.txt` | sorted, prefix-compressed (an optional number of letters kept from the previous word, then the rest); only words of 3+ letters are used; A-Z only | replace the dictionary |
| `bonus_word_dictionary.txt` | one word per line (the bonus-word pool) | replace the bonus words |
| `popdefs.txt` | `WORD<TAB>(pos.) definition`, Latin-1, shown in Lex's balloon after long words | replace or add definitions |
| `properties/texts.xml` | `<String id="GT_...">...</String>` per text (ids in `src/GameTextTable.inc`) | change any text the game shows |
| `images/*` (+ `@2x`) | the original images and their `_name` alpha masks | reskin tiles, Lex, backdrops, dialogs, in HD too |
| `sounds/*`, `music/*` | Ogg / `.au`, the MO3 (orders matter) | replace sounds or the music |

## Proposals

### 1. Custom dictionaries (data only, plus a small tool)
- **Tool**: `games/bookworm/tools/make_wordlist.py`: plain word list (one per line) to the compressed format,
  uppercase A-Z only, sorted, duplicates removed, 3-12 letters, with a report of what was dropped. It also
  goes the other way (the original list to plain text, for editing). The format stays compatible with the
  community's cookworm tools.
- **Ready-made sources** (each to be checked and downloaded only with the user's OK): ENABLE2K (public
  domain), the 12dicts lists (public domain). Commercial lists (Collins/SOWPODS, TWL) only from a file the
  user supplies.
- **Ideas**: "modern words" (the 2004 list lacks recent words), a "family friendly" list, a themed list.

### 2. Better or extra definitions (data only, plus a tool)
- `popdefs.txt` has 2,249 definitions, so most long words show none. **Tool**: `make_popdefs.py` building the
  file from WordNet (Princeton licence, free to use and redistribute with its notice) for every word in the
  active word list, in the original's one-line `(pos.) definition` style, trimmed to fit the balloon.
- Definitions for a custom dictionary come out of the same tool.

### 3. Other languages (needs engine hooks)
Text and word list alone are not enough, because the alphabet and the letter statistics are in the code.
Hooks to add (tunables a mod sets, original values by default):
- the alphabet: letters and their tile images (the 26-letter tile strip `font_tile_letters` is a sprite strip;
  extra letters such as Ä Ö Ü ß, É È Ç, Ñ need extra cels, from a mod's image);
- letter weights for new tiles (the 26 weights summing to 95,500) and letter values for scoring;
- the "Qu" tile rule (on/off, or another digraph such as Spanish "CH" or Dutch "IJ");
- minimum word length (3; some languages play better with 2 or 4).
A language pack is then: word list, bonus words, definitions, `texts.xml` with every `GT_` text translated,
the tile art for its letters, and the tunables. German, French, Spanish and Dutch are good first targets
(open word lists exist, e.g. from LibreOffice's Hunspell dictionaries, with their licences).

### 4. Art (data only, generators like Bejeweled's)
- **HD art**: a generator like Bejeweled's `make_hd4x_mod.py` (Real-ESRGAN is installed in `E:\retools`)
  that upscales the user's own images into `@2x`/`@4x` copies, so the port renders sharply on large screens.
- **Tile skins**: generated letter tiles (clean modern fonts, high-contrast or dyslexia-friendly), generated
  procedurally like Bejeweled 2's gems so nothing of the original is redistributed.
- **Lex and backdrops**: replaceable as images; no generator proposed.

### 5. Gameplay variants (code mods, after the above)
Board size, fire-tile frequency, a timed "sprint" mode, a "no fire" relaxed mode, set seeds and replays (the
`-autoplay` bot and the deterministic clock already make replays possible), as in Bejeweled's code-mods
design.

## Suggested order
1. `make_wordlist.py` + `make_popdefs.py` (custom dictionaries and definitions, all data, quick).
2. The HD art generator.
3. The language hooks, then one language pack as the proof.
