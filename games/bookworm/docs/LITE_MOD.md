# BookWorm Deluxe: the lite mod (2026-11-14)

BookWorm has no lite mod: nothing in its game logic is worth a toggle. This file says what was looked at, what each candidate
costs where it runs and where a faster method could differ, and why nothing is taken (`docs/MOD_ROADMAP.md`, "The lite mod").
The like-for-like processor time is in `GAME_NOTES.md`, "Performance pass", "Processor time like for like" and "The cycle split".

## What was looked at

The game's logic is a board of 7 columns, 52 tiles (the odd columns hold 8, the even ones 7), that the player chains into words, a dictionary of 151,381 words
(`wordlist.txt`, 3 letters and up, read into a `std::set` of strings), a weighted letter deal, fire and special tiles, a score
and a level bar. The original has no search of any kind: no hint, no solver, no "find a word". The word solver (`Solver.cpp`,
`FindBoardWords`) is the port's mod "solver", marked in its source as not part of the original game, and the test bot of
`-autoplay` uses the same search to choose its moves; neither is a computation of the game, so neither is a lite change. The
cycle split of the like-for-like scenario (`GAME_NOTES.md`, "The cycle split") puts the game's whole update, which holds all of
the logic, at 0.055 to 0.058 million cycles an update, about 3% of the process's 1.8 to 1.96 million; the draw is 0.18, the
present 1.1 to 1.5, the graphics driver's thread 0.78 and the audio thread 0.23 to 0.27, which a lite change may not touch since
it keeps the look.

The candidates are every place the logic does work that scales with something: the word check, the letter deal, the refill and
burn passes, the submit, the mouse handlers, the board's per-update pass, the effects, and the one-off loads. Temporary timers,
not committed, wrapped each in a build of the port: `Platform::CurrentThreadCycles` for calls of some microseconds and more, and
`rdtsc` for the calls of a few hundred cycles, with the calls that ran over 200,000 cycles counted apart (a preemption or a
one-off event, not the function's work). The game was the check's two bot games under `-fast` (`tools/check.sh bookworm`:
`-seed=1 -autoplay=1`, classic to level 5 with 19,230 points in 6,460 updates, action to level 11 with 87,580 points in 17,659
updates), run under rundesk with `-cyclesplit` for the totals, no mods. A bracket's own two readings cost 2.4 to 3.5 thousand
cycles each in the running port ("What counting costs"), so every figure measured with them (`CheckForFalling`, `BurnTiles`, `Submit`, the mouse handlers and the three one-off
calls) is an upper bound, and the board's update, the one called every update, was measured again with `rdtsc` brackets for that
reason.

| Computation | What it does | A faster method | Result |
|---|---|---|---|
| `WordScore::Validate` (0x406130), from 10 places in `Board.cpp` | one `std::set<string>` lookup of the selected letters, each time a tile is added, taken off or changed | a hash set, a sorted array or a trie | the same yes or no for every string: not lite, so a performance-pass change, and not earned (below) |
| `Board::RandomLetter` (0x403d10) | a roll of 0 to 95,499 against the 26 letter weights, never the same letter as the one before (a rejected roll is a draw spent) | a table lookup in place of the walk; another way of avoiding the repeat | the same letters, if the draws and their order are kept. Another table or a rule that spends fewer draws deals other letters from the same seed: lite in kind, and it gains nothing (below) |
| `Board::CheckForFalling` (0x40b540), `Board::BurnTiles` (0x408700) | after a word: compacts each column, deals the new tiles with their falling start and the one-in-twenty green tile, sets the due fire tile, passes the burn countdowns on | none: a few loops over the 56 cells | |
| `Board::Submit` (0x40f5b0), `MouseDown` (0x409de0), `MouseDrag` (0x40a0c0), `MouseUp` (0x40fc40) | the word's score, its special tiles and the tiles' flight to Lex; the selection by mouse, with its adjacency test and a `Validate` | none | |
| `Board::Update` (0x40bd40) | per update: the countdowns, the fall and the jitter of each tile, a fire tile's sparks, the popups, the score rolling up | none: it is the picture's motion | |
| `EffectMgr::Update` (0x41f990), `EmberListMgr::Update` (0x420fd0) | the sparkles, the flying tiles, and the embers and smoke that rise from fire tiles | fewer embers, a cheaper step | another picture: a lite change keeps the look, so not lite |
| `BookWormApp::LoadWordList` (the dictionary, through 0x41a860), `Definitionator::Load` (0x41d920), `LoadBonusWords` (0x41a880) | once, while the game loads: reads `wordlist.txt` (767,626 bytes, each word a prefix length and a tail) a character at a time into the set, and `popdefs.txt` into a map | read each file in one call, a sorted array in place of the tree | the same words, so not lite |
| `FindBoardWords` (`Solver.cpp`) | the mod "solver" and the test bot: a depth-first search from every tile with the dictionary's prefix test, over the board's tiles | a trie | not the game's: see below |

## What they cost

The whole run's processor time was 5,931 M cycles for the classic game (28 s of wall time on the shared machine, load
included) and 9,523 M for the action game. The calls and cycles of each candidate, in the two games:

| Candidate | Calls, classic / action | Cycles, classic / action | Share of the process, classic / action |
|---|---|---|---|
| `Validate` (`rdtsc`: 4,500 cycles a call, the lookup's cache misses) | 118 / 442 | 0.53 M / 2.27 M | 0.009% / 0.024% |
| `RandomLetter` (`rdtsc`: 240 cycles a call) | 209 / 526 | 0.05 M / 0.17 M | 0.001% / 0.002% |
| `CheckForFalling` (up to 9 thousand a call) | 30 / 106 | at most 0.27 M / 1.07 M | 0.005% / 0.011% |
| `BurnTiles` | 24 / 97 | at most 0.04 M / 0.17 M | under 0.003% |
| `Submit`, which holds the refill, the burn and a `Validate` or two | 25 / 93 | at most 1.63 M / 5.75 M | 0.027% / 0.060% |
| the three mouse handlers, which hold the `Validate`s of the selection | 311 / 1,171 | at most 3.92 M / 13.52 M | 0.066% / 0.142% |
| `Board::Update` (`rdtsc`), the calls under 200,000 cycles | 6,170 / 17,364 | 51.9 M / 115.1 M (8.4 / 6.6 thousand an update) | 0.9% / 1.2% |
| `Board::Update`, the three one-off calls of the classic game (thread cycles) | 3 | 35.2 M (6.4, 9.2 and 19.5 M) | 0.6% |
| `EmberListMgr::Update` / `EffectMgr::Update` (`rdtsc`) | 5,711 / 16,902 | 10.8 M / 18.7 M and 1.0 M / 2.6 M | 0.18% / 0.20% and 0.02% / 0.03% |
| the dictionary's load (once, 1,440 cycles a word) | 1 | 218 M / 212 M (about 104 ms) | 3.7% / 2.2% |
| `Definitionator::Load`, `LoadBonusWords` | 1 | 9.9 + 1.5 M / 9.6 + 1.0 M | 0.2% / 0.1% |
| `FindBoardWords` (the test bot: 2.3 ms a call, the slowest 3 ms) | 26 / not timed | 125.2 M / not timed | 2.1% / not timed |

The classic game's three slow calls of `Board::Update` are one-offs: the level-complete announcement at the end of the fourth
level (state 5, 6.4 M cycles) and the two ticks of the game-over sequence on the fifth (states 2 to 1 and 1 to 2, 9.2 M and
19.5 M). Each creates widgets and writes the player's files, as the original does, and none is a stretch of play. The game's other threads are the graphics
driver's and the audio's.

## Why none is taken

A change goes into the lite mod only when it measurably cuts the processor time of a stretch of play where it runs. In play the
logic that a lite change could touch, the submits, the mouse handlers and the deal, is under 0.3% of a bot game's process,
about 860 to 1,090 cycles an update on the 1.8 to 1.96 million that the port spends on one (`GAME_NOTES.md`, "Processor time
like for like"), 0.09 to 0.11 million cycles a second at 100 updates a second, 0.005% of a core. The five-pair medians of the
performance pass resolve a few percent of a stretch at best (the same exe's runs spread from 7.3 to 12.2% of a core), and
counting itself costs 0.4 to 1.1%, so nothing in a stretch could show. The board's update, which is the picture's motion, is
0.9 to 1.5% of the process (with the classic game's one-off calls), and the embers in it 0.2%.

The changes that could go in the mod fall in three groups.
- A faster method with the same answer, which is not lite and belongs to the port's performance pass: a hash set or trie for
  `Validate`, and a one-call read for the dictionary. The check gains 4,500 cycles a call, 0.01 to 0.02% of the process. The
  load is the larger, 104 ms of processor time once, on the loading screen, 3.7% of a classic bot game and about 8% of the
  2.3 to 2.9 G cycles (1.11 to 1.38 s at 2.1 GHz) the port spends to reach the title; the port loads in half the original's wall time already, so it
  is not earned, and nothing was changed.
- A faster method that gives another result: another letter deal, which would deal other letters from the same seed, and fewer
  embers, which would change the picture. The deal costs 240 cycles a letter and 0.002% of the process, and the embers are the
  look the lite mod must keep.
- Work that is not the game's: the solver and the test bot's search, 2.3 ms a call (3 ms at worst), which a player meets only
  as a hint key press under the mod "solver", and which a test run spends 2% on.

The cost that is left is the present, the graphics driver, the draw and the audio thread, which a lite change may not touch.

The lite-mod step is therefore done with no mod.

## To repeat

Wrap the functions in a scratch build of the port (`Platform::CurrentThreadCycles()` from `platform/CycleSplit.h`, or
`__rdtsc()` around the short ones), add up their cycles at exit, and run the check's two games with the cycle split for the
totals (an empty `-modsdir`, so that no mod changes the dictionary or the play):

```bash
build/native/rundesk.exe <full path of build/native/bookworm.exe> -userdir=<dir>/user -modsdir=<dir>/mods -fast -seed=1 -autoplay=1 \
    -autoplaymode=classic -autoplaylog=<dir>/log.txt -cyclesplit=<dir>/split.jsonl
```

with `-autoplaymode=action` for the other. The word count of the dictionary is the loader's: the digits of a line are the
length of the previous word's prefix that it keeps, the rest of the line its tail, and the words of 3 letters and up are
kept (151,381 of them, 968 of 3 letters to 28,391 of 8).
