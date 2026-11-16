# Bookworm Adventures Deluxe: the lite mod (2026-11-16)

Bookworm Adventures has no lite mod: nothing in its game logic is worth a toggle. This file says what was looked at, what it
costs in a stretch of battles, and why nothing is taken (`docs/MOD_ROADMAP.md`, "The lite mod"). The like-for-like processor
time against the original is in `README.md`, "Performance".

## What was looked at

The game's C++ logic is small. A battle's board is a grid of 4 by 4 tiles that the player chains into a word. The word is
checked against a list of 167,624 words (`Board::IsValidWord`, 0x41a4d0, which calls `Lex::IsWord`): each time a tile is
selected or deselected (`SelectTile` 0x4195f0, `DeselectTile` 0x4199a0, `CheckIfWordStillValid` 0x419840) and once more when
the word is submitted (`BattleEngine::SubmitTiles`, 0x414810). The port keeps the words in one sorted array, and `IsWord` is a
binary search of about 18 string comparisons with the original's `std::set` answers. After a word, the tiles fall into its
gaps and new ones fill the grid (`Board::Refill`, 0x41d3a0). Each new tile's letter comes from `TileEngine::GenerateLetter`
(0x49a120). It counts the board's letters, then keeps the letters below their maximum that make no avoided word in the grid's
four rows and four columns (`IsLetterAllowed`, 0x499e30) and keep the vowels and consonants in bounds. It weights each kept
letter with the script's `GetExtraLetterFreq` and draws one. `Board::Scramble` (0x41a620) deals the whole grid again.

The original has no search of any kind. It gives no hint, has no solver, and never asks whether the board holds a word. Its
rules are in its Lua scripts, which the port runs unchanged on its translation of PopCap's Lua 5.0.2 (`poplua/`). These cover
the enemies, their attacks and the damage, the gems' and treasures' effects, the potions, and the mini-games. The test
command `word` is the port's own (`GameApp::BestWordForTests` in `TestHooks.cpp`, the bot's longest word on the board). It is
the bot's work, not the game's.

## What it costs

The stretch measured is the battles of the check's chapter run (`tools/check.sh`, `bookwormadv_chapter`): the test profile's
first chapter under `-fast -seed=1`, each battle won by the bot's longest word, 7 words and the chapter won and saved, as in
the check. The run was made under rundesk with `-cyclesplit` and two script snapshots. One is at update 5550, when the first
battle starts after the map and the conversations. The other is at update 24990, back on the map after the last battle.

| The battles, 19,440 updates (5.45 s) | M cycles an update | Share of the process |
|---|---|---|
| update (the game's logic: the scripts, the board, the widgets) | 0.314 | 49.1% |
| picture (draw, present and the driver's threads) | 0.300 | 46.8% |
| audio (SDL's mixer thread) | 0.004 | 0.6% |
| loop (events, the readings) | 0.022 | 3.5% |
| the process | 0.640 | 100% |

The port draws through Direct3D 11, so its drawing is shown as "picture" and not split (`docs/TEST_COMMANDS.md`, "The cycle
split", "The limit"). Under `-fast` it draws one update in ten (1,944 draws), so the picture's share in real play is larger
than it is here.

A second run added snapshots 5 updates after each of the bot's 40 moves and 5 updates before the next one. The update cost
0.316 M cycles an update in the stretches without a move (15,600 updates). It cost 0.58 M in the stretches that hold a move,
its click and the start of the attack that the move sets off (4,345 updates). The cycle split does not divide an update
further, so the update's contents were read from a sampling profile: `games/bookworm/tools/profile/cpuprof.py`, run on a
scratch copy that took no samples for the first 5.5 s (the load, the title and the map). Over the battles it sampled 5.62 G
cycles:

- **Lua, 21%.** The interpreter and its tables: `luaH_getstr` 8.9%, `luaV_execute` 4.4%, `luaS_newlstr` 2.4%, `gettable` 1.0%,
  `luaH_get` 0.9%, and the rest of Lua's functions below that.
- **`strlen`, 4.7%** (in the C runtime). A flat profile cannot say who calls it. `lua_pushstring` calls it, and so does
  `lua_getglobal`, which is that call followed by a table lookup. The C++ side makes these each time it calls a script
  function by name, and each one also hashes the name and looks it up in the string table (`luaS_newlstr`).
- **The bot's search, 1.7%** (`BestWordForTests`, `Spell`).
- **The screenshot at the run's end, 4.2%** (the PNG encoder and `ReadScreen`).
- **The rest of the exe's code, 3.8%:** fonts, blits, PopAnim. The report lists 90 functions, down to 0.1%. None of the board's
  or the tile engine's functions is among them, so the word check, the deal, the refill and the scramble are each under
  0.1%.
- **The remainder** is the graphics driver, its thread and the loop's system calls.

The game updates 100 times a second (`mFrameTime` 10 ms). In real play the battles' update is about 31 to 37 M cycles a
second, which is 1.5 to 1.8% of one core at the cycle counter's 2.08 GHz. Most of it is the scripts.

## Why none is taken

A change goes into the lite mod only when it can give another result and it measurably cuts the processor time of a stretch
of play where it runs. The possible changes fall into three groups.

- **The same answer, faster.** This is not lite; it belongs to the port's performance pass.
  - The word check was made a binary search over a flat array in that pass, for its memory.
  - The scripts' share could be cut by keeping references to the script functions and names that the C++ side looks up by
    name, so they are not hashed and interned on every call. The scripts would run the same.
  - Neither is earned. The port takes a third of the original's processor time at the title, and the whole update in battle
    is under 2% of a core.
- **Another result, faster.**
  - Another letter deal, such as one without the avoided-word patterns or without the weights, would deal other letters from
    the same seed. That makes it lite in kind, but the deal is under 0.1% of the battles and nothing could show.
  - The rules that cost the most are the scripts. Changing them changes the game, not its cost.
- **Work that is not the game's.** The bot's search and the screenshot, together 5.9% of the profile, are only in test runs.

The cost that is left is the picture, which a lite change may not touch because the mod keeps the look, and the original's
scripts running on the interpreter.

The lite-mod step is therefore done with no mod.

## To repeat

Make a folder under `build/`, copy `tests/menu/user` into it, and write the check's chapter script
(`bookwormadv_chapter` in `tools/check.sh`) with `<update> cyclesplit battles` before the first `word attack` (5550) and
`<update> cyclesplit end` before the screenshot (24990). Then run it from the worktree's root with Windows paths (`pwd -W`)
and sum it:

```bash
build/native/rundesk.exe build/native/bookwormadv.exe -userdir=<dir>/user -boardlog=<dir>/board.txt -script=<dir>/script.txt -fast -seed=1 -cyclesplit=<dir>/split.jsonl
```

```bash
python -I tools/cyclesplit.py <dir>/split.jsonl --from battles --to end
```

For the profile, run `cpuprof.py` under rundesk on the same command with its output file in `<dir>`. Its samples were skipped
for the first 5.5 s by a two-line scratch change: in the sampling loop, after `last[tid] = c`, continue while
`time.time() - t0` is below the skip.
