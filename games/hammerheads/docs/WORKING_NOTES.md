# Hammer Heads Deluxe: working notes (analysis state, kept so a compaction loses nothing)

Hammer Heads Deluxe (Nuclide Interactive, "Cubby Catch" while in development; published by PopCap, Steam copy 2018 of the
2006 build). `re/hammerheads/bin/HammerHeadsGame.exe` is the game image out of its wrapper: 1,703,936 bytes, linked 2006-05-17
with VS2003, image base 0x400000, .text 0x401000-0x550000, .rdata 0x551000, .data 0x590000. Data are loose files in the
install: `properties/resources.xml` (363 ids: 238 images, 111 sounds, 14 fonts, five groups Init, LoadingScreen, Gui, MainMenu,
Game), `boards/*.xml` (54 levels: `board001..027`, `board_tough001..027`, `board_marathon`), `boards/{classic,classictough,marathon}.csv`
(per-level rules), `gnomedata/0..24.txt` (the intro text of each level), `images`, `sounds` (Ogg, the music too), `fonts`.
They are staged into `assets/hammerheads/` by `tools/stage_hammerheads_assets.py`.

## The export and its readers (re/hammerheads is git-ignored)

- `tools/ghidra_import.sh` with `MSVC70=1` (as Venice, the other VS2003 build), then `tools/rtti_msvc70.py` (223 classes,
  `re/hammerheads/rtti.jsonl`), then `tools/funcmatch.py` against Big Money (1824 of 8529 functions match, all of them framework or
  library code, 556 carrying Big Money's names) and the merged names applied with `tools/ghidra_headless.sh apply
  re/hammerheads/names_v1.jsonl` (`GHIDRA_PROJECT=hammerheads_game GHIDRA_PROGRAM=HammerHeadsGame.exe EXPORT_DIR=re/hammerheads/ghidra`).
  The Ghidra project is `hammerheads_game` in E:/retools/projects. Ghidra's decompiler times out on 0x428fd0 (the generated Gui
  extraction function, 12.8 KB) and 0x52c07d (CRT).
- `games/hammerheads/tools/decread.py <addr>...|<lo>-<hi>|--list <lo>-<hi>` prints the compact decompilation: exception-frame
  bookkeeping and local declarations dropped, virtual calls annotated with the slot's name in the SexyApp, Widget and Dialog tables
  of `games/bigmoney/docs/FRAMEWORK_12.md` (the same 1.2 layout), resource globals (`DAT_00597c34`) named by their id from
  `docs/resglobals.txt`. Names for functions and globals go in `docs/symbols.txt` (`<address> <name>`). `fn.py`, `callers.py`,
  `refs.py`, `classes.py` as in Venice's tools; `resnames.py` makes `resglobals.txt` from the machine code of the extraction
  functions (357 of the file's 363 ids); `resgen.py` makes `src/Res.h` and `src/Res.cpp` from `resources.xml`.

## Findings

- **No Nuclide code in common with Rocket Mania or Pixelus** (funcmatch, 2026-11-12): 158 of Hammer Heads' functions match Rocket Mania's
  (10 KB, C runtime and tiny library code at 0x520000-0x540000), 3 match Pixelus' (CRT). Nothing under `engines/`.
- **Layout of the code:** 0x401000-0x468e10 is the game and its own libraries, 0x469000+ the framework (RTTI names the 1.2 classes)
  and the CRT. In the game's span: 0x402950 `Object`, 0x405550-0x419c70 the editor and its property widgets (not ported), 0x41a000-0x41d9c0 the
  beta-test login and the DRM client (not ported), 0x41e660-0x422ac0 `Application`, 0x424e40 the CSV rules reader, 0x426100-0x42d081
  generated resource code, 0x42d730 `Avatar`, 0x42f350 `Creature` (the base of every gnome), 0x431c70 `Cubby`, 0x434fc0 `Gameplay`, the gnomes
  0x435b70-0x440030, 0x440830 `Popper`, shop items, `Sticker`, floaters, HUD, then the screens and dialogs 0x44c170-0x462d00, and TinyXML
  0x463240-0x468490 (the game's own copy, with a reader for its length-prefixed chunk files).
- **The boards are not plain XML on disk:** each `boards/board*.xml` is a stream of chunks, a little-endian u16 length and that many bytes
  of text (the writer wrote in pieces). Concatenated they are TinyXML text with `<Level ResX ResY>`, a `<property_list>` (Grid X, Grid Y,
  Snap Grid) and `<object Class="class Sexy::Cubby" Name X Y Width Height Angle>` / `class Sexy::Sticker` elements with a `<choice name="Image"
  index=..>` property. Only these two object classes exist in the 54 files (504 Cubbys, 159 Stickers).

## The game's structure (read so far)

- **Object system** (0x401000-0x405450, translated in `src/Object.h/.cpp`): `Object` (name, layer mask, dead flag, active flag, age, parent ref,
  children refs, a hover flag at +0x48 that Avatar sets), `SimpleObject` (a rect at +0x80, x and y doubles at +0x90 and +0x98),
  `ObjectManager` (vector of objects, name map, refs, a Cubby list sorted by y with the C library's qsort, fixed 0.01 step). Object has two pure
  virtuals at +0x18 (`MouseOver(x, y)`) and +0x1c (`Hit(x, y, mask, w, h)`, the avatar's circular hit mask against the object). The names come
  from `typeid(*this).name()` ("class Sexy::Cubby_00002": first name of a type is _00001, then +1; a name given by hand sets the type's count to
  its last five digits + 1, so the next made-up name is two more); boards store these names. `FUN_004b2aa0` is `Sexy::Rand()` (the MTRand
  generator, 31 bits), not the C library's rand.
- **Class ranges:** Avatar 0x42d730, Creature 0x42f350 (not an Object: its own vtable, a global list of all creatures at 0x597f30), Cubby 0x431af0,
  Gameplay 0x434fc0, gnome classes 0x435b70-0x43fb10 (one per kind: Bomb, Bonus, Bouncing, BubbleHead, Challenge, Chest, Clover, Coin, Cupido,
  Flying, ForkyChap, Heart, Hide, King, Multiplier, PowerUp, Rabbit, Save, Shop, Silver, Simple2, Strawberry, Strong, Switcher, Woman), ImageBlast
  0x43fb10, Popper 0x440830, ShopItem* 0x4411d0, Sticker 0x442f60, HUD (ChallengeMeter 0x443a30, Coins 0x444010, Floater* 0x444500-0x448e40,
  HammerDisplay 0x4491a0, Hearts 0x449b50, LevelIndicator 0x44a680, MarathonMeter 0x44a910, Score 0x44b070, Timer 0x44bb80), screens and dialogs
  0x44c170-0x462d00.
- **Creature kinds** (0x430340): 39 kinds, a per-kind table (image id, shadow, anchor, head point, health, score, escape penalty, the list of items
  it may carry) built by `Creature`'s constructor; the factory 0x432290 maps a kind to a class (1 Woman, 2 Strong, 3 Bomb, 4 BubbleHead, 5 Hide,
  6 ForkyChap, 7 Silver, 8 Multiplier, 9 Cupido, 0xc Heart, 0xd and 0xe Coin, 0xf Shop, 0x10 Challenge, 0x11 Chest, 0x12 Clover, 0x13 and 0x14
  Bonus, 0x15 Strawberry, 0x16 Rabbit, 0x17 King, 0x18 Save, 0x19 Simple2, 0x1a PowerUp, 0x1b Switcher, 0x1c-0x25 ShopItem (0x1f Flame), 0x26
  ShopItemGift, the rest a plain Creature). `tools/kindtable.py` is not kept; the table is in `Creature.cpp`.
- **Stats** (singleton 0x42f1c0, 50 doubles; `Stats::GetInt(i)` is 0x42efc0 with its argument on the stack, which the decompiler drops: read the
  asm with `asm.py <fn> --calls`): 0 gnomes smashed in all, 5 and 0x11 hearts counters, 6 coins picked up, 0xc score, 0xd level of the last bomb,
  0xe level of the last miss, 0xf level of the last escaped gnome, 0x12 hammer level, 0x1b hammer on fire, 0x1d kings destroyed, 0x1e perfect bonus
  rounds, 0x21 level of the last purchase, 0x23 level of the last heart pickup, 0x24 level of the last coin pickup; 10 and 11 health and capacity
  (Hearts), 0x16 the combo multiplier (drops 0.1 on a miss), 0x2a swings. Trophies 0-20 (0x42d0a0 awards them): 0 Gnome Crusher (100 smashed), 1
  Ultimate Gnome Crusher (1000), 2 Blast Master, 3 Supreme Blast Master, 4 Accuracy Award, 5 Master Marksman, 6 No Gnome Escaped, 7 Supply Challenge,
  8 Golden Heart, 9 Vow of Poverty, 10 Heart Hog, 11 Silver and 12 Gold Marathon Medal, 13 Classic and 14 Marathon Score Ribbon, 15 Regicist, 16
  Bonus Banker, 17 Pulverizer (2500), 18 Mogul (2500 coins), 19 Power Player, 20 Trophy Master.
- **Cheats typed in a game:** `jumpto5`, `jumpto10`, `jumpto15`, `jumpto20`, `jumpto25`, `superpower`, `noawaydamage`, `nomissdamage`.

## State

Translated: `Object.h/.cpp`. Nothing else yet.
