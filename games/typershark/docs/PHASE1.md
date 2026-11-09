# Typer Shark: phase 1 of the identical-play pass (plays identical)

Job P1a. The port is compared with the original by the game's state: the original's memory is read at every update (origrun's
`at <N> peek`, exact on the virtual clock), the port writes its own state log (`-statelog`), and the two are compared update by
update, field by field. Nothing here uses pictures.

## How it works (games/typershark/tools/phase1/)

- `cmp.py <scenario.py> --seed S [--step K] [--lo U --hi U]` runs one scenario in both and prints the first differing updates and a
  verdict line. A scenario (`sc_*.py`) is a list of inputs at update numbers (`STEPS`), the range to compare (`LO`, `HI`), and
  optionally `TYPIST` (the port's own bot plays on top of the steps and its input is written down, then replayed identically in both).
- The original runs through `runorig.sh` (restores the copy's `users` folder from `build/ts-p1/fixtures/users-clean`, clears the
  copy's registry key, runs `games/typershark/tools/run_original.sh`: virtual clock, seed pinned). Two passes of the original: pass 1
  reads the creature lists' sizes and each creature's vtable pointer (its class; the class layouts come from RTTI), pass 2 reads each
  creature's fields at its class's offsets (the GameObject and Enemy parts sit at other offsets in each class).
- The port runs with `-fast -seed=S -script=... -statelog=...` under rundesk from a copy of the same `users` fixture
  (`build/ts-p1/fixtures/port-clean/users`). The statelog line was extended (test hook only): the board's update count, the game
  timer's raw values, the hint and pause flags, the typed letters, the dive, the diver's state, the EMP charge and the game
  generator's index `rng` (how many numbers have been drawn), and per creature `kind:word@x,y,state,typed`.
- `-typist=<file>;every=K;typo=N;stop=U;enter=U` (testhooks/Typist.cpp) is the port's bot writing down every input it plays as
  `-script` lines (`<update> char c`, `<update> click x y`). `cmp.py` turns them into `at <update-1> text c` for the original: an
  origrun input at counter N is read by update N+1, a port script line N runs before update N (`tsp.PORT_SHIFT`).
- Other helpers: `show.py <tag> <update>` (both states side by side), `summ.py <statelog>` (what a run covered), `peekparse.py`,
  `scen_menu.py` (the menu clicks), `build.sh` (rebuilds the port with the right PATH), `peek_expr.py` and `peek_more.py` (read
  address expressions such as `BRD+0xf24` out of the original at chosen updates, played with the inputs of a finished run; `peek_more.py`
  can add letters after the run's last input and start the whole script later, `OFFSET=`).

### The common start (scenario A)

Both programs start the same way: the game loads, the title screen goes by itself (the original is a registered copy: no Play
button), the menu shows the NEW USER dialog. The load finishes at an update that depends on the machine in the original (between
about update 800 and 1550, the loading thread runs on the wall clock) and the menus draw no random numbers, so the scenario waits:
at update 4000 it types `Tester` (one letter per update), clicks OK (347,228), No to "ASSESS WPM?" (392,341), Adventure (510,70) and
Easy (320,163). The board's first update is 4091 in both. The hint "Welcome to Typer Shark!" opens at board update 139 and the
scenario clicks its button (320,381).

### Running it

`games/typershark/tools/phase1/queue.sh <jobs file>` runs the jobs one after another (two runs of the original at once would share
the copy's `users` folder and registry key and spoil each other; the machine also allows only three game processes). A job line is
`tag scenario-file seed step [ENV=value ...]`, e.g. `blk4_s1 sc_t2.py 1 3 HI=100000 LO=49400 TYPIST=every=3`; its verdict goes to
`build/ts-p1/queue.log`. `mine.ps1 [-Kill]` lists (and stops) the processes of such runs: stopping a python or bash parent leaves its
origrun and game running, so a stopped queue is always followed by `mine.ps1 -Kill`. `peek_cre.py` reads raw words of a list's
creatures out of the original at chosen updates (used to find the cause of a difference). `build/ts-p1/` (scratch, git-ignored)
holds the users fixtures, logs and caches.
`cmp.py` waits up to 20 minutes for room under the three-process limit (`tools/gamecount.py`) before each game process and then gives
up. The original's loading thread runs on the wall clock while its main thread spins through updates (tens of thousands a second
on the virtual clock), so on a busy machine the menu clicks at update 4000 can come before the game has loaded and are lost: the
original then never draws a number (rng 624) and has no board. `cmp.py` runs an original again (up to three times) when it saw no
board, or no typing tutor, at any peek, and says "THE ORIGINAL NEVER LOADED THE MENU" in the verdict when nothing helped. `TSBASE=<update>`
moves the whole menu flow later (a scenario's update numbers follow it); `TSORIGTIMEOUT=<s>` lengthens the 240 s limit of one run of
the original (a long adventure run under load needs 600 s or more). A port run that is killed can stay behind as an
unkillable `typershark.exe`; it counts for nothing and goes away when its rundesk is gone.

## Scenarios and results

Seeds 1, 2 or 3 pinned in both programs, the same `users` folder, the menu flow of scenario A. "step 3" means the original's memory
was read every third update (the port logs every update); a run of anything new starts with step 1 or 2 so that a difference that
lives for one update cannot slip through. The bot is the port's own (`-typist`); its input is written down and replayed in the original
at the same update. Everything compared: update count, score, lives, level timer (three raw values), pause and hint flags, typed
letters, the dive, the diver's state, EMP charge, the generator's index (every number drawn), and per creature kind, word, x, y,
state and letters typed, in list order.

| scenario | what it plays | range, step | result |
|---|---|---|---|
| A | menu to the first board | 4092..4700, 1 | equal |
| L1 | level 1, bot a letter every 4 updates, seeds 1, 2, 3 | 4092..12000, 1 | equal |
| L1-3 | levels 1, 2, 3 (piranhas, stealth sharks, the changes through the map), bot a letter every 4 updates, seeds 1, 2, 3 | 4092..34000, 3 | equal (seed 1 showed a false alarm at 29268, the comparer's fault, see comparer fault 3 below) |
| idle | nobody types: three lives are lost, game over, the dialog stays | 4092..15000, 2 | equal (lives 2, 1, 0, then -1 at update 13160 in both) |
| typo | the bot types a wrong letter for every 7th letter, levels 1 and 2 | 4092..20000, 3 | equal |
| black | levels 3 and 4, black sharks (two words each), bot a letter every 3 updates, seed 1 | 40000..42300, 2 | equal after fix 1 |
| adventure to 100000 | one game on Easy from the start through the first bosses: levels 1 to 25 (TorpedoBoss and its Torpedo shots, MechaShark with MechaTorpedo, Shark, Piranha, StealthShark, StealthPiranha, BlackShark, StealthBlackShark, RedShark, ToxicShark, WhitePiranha, StealthWhitePiranha, GhostShark, bonus creatures; the diver's states 0, 1, 4, 5, 6, 7, 9), bot a letter every 3 updates, seed 1 | 49400..100000, 3 (40000..49400 by the earlier runs) | equal after fix 1 |
| adventure to level 34 | the same game on to its level 34 (the menu flow started at update 24000, `TSBASE=24000`, so every update number is the base-4000 number plus 20000): MechaSquid with SquidCannonBall, BlackShark, StealthBlackShark, RedShark, StealthRedShark, ToxicShark, WhitePiranha, StealthWhitePiranha, StealthPiranha, GhostShark, bonus creatures; the lesson at level 25, 30 and 34, 30 lives, a score of 11.18 million; the bot a letter every 3 updates, seed 1 | 120000..153190, 3 (= 100000..133190 of the base-4000 flow; its 100000..116000 stretch was read only by the run before it, see below) | equal (0 of 11064 differ), including the 264 numbers the generator draws at 153188 to find the last unused end-of-level word (index 108 to 372 in both) |
| EMP | as L1-2 with the Enter key (the Shark Zapper) pressed once at update 9000 | 4092..16000, 2 | equal |
| abyss | Abyss mode, new profile, bot a letter every 4 updates: levels 0 to 4 down to 484 feet, Shark, Piranha, bonus creatures, WhitePiranha, BlackShark, StealthPiranha, StealthShark; lives 0 all the way (seeds 2 and 3: the menu flow started at update 24000, down to 544 feet in the lessons 0 to 2: Shark, Piranha, bonus creatures) | seed 1 4072..22000, seeds 2 and 3 24072..42000, 2 | equal after fix 2 (seed 1), equal (seeds 2 and 3, 8965 updates each) |
| tutor lesson 1 | the typing tutor from the menu: the port's bot types the key that is next (one every 3 updates) through lesson 1, the lesson ends | 4072..7000, 1 | equal (state, task, right keys, errors, clean keys, seconds, words per minute and accuracy at the end, the key that is next, generator index) |
| tutor lesson 13 | the same after 12 presses of Next, 772 keys, the lesson ends | 4072..8000, 1 | equal |
| tutor lesson 15 | the same after 14 presses of Next (a lesson with lines that start or end with a space), 1059 keys at 100 per cent, the lesson ends at update 7446 of the base-4000 flow | 8072..13000 (menu flow at 8000), 1 | equal |
| tutor lesson 17 | the same after 16 presses of Next, 1407 keys at 100 per cent, the lesson ends at update 8508 | 4072..9000, 1 | equal |

(The rows after this are added as they finish.)

## Differences found and fixed

1. **BlackShark and RedShark kept their second and third word for ever** (original `BlackShark::TypeLetter` 0x409fb0 and
   `RedShark::TypeLetter` 0x40b600). When the last letter of a word is typed and a further word exists, the original makes it the
   word, sets the typed count to 0 and then assigns the empty string to the word it took (the assignment of "" through 0x406da0 at
   0x40a0c1, 0x40b711 and 0x40b857). Ghidra's export leaves that statement out, and the port left it out too: a black shark that
   had taken its second word loaded it again once it was typed and never died, so the level never ended (a bot sat in level 3 for
   100000 updates). The first update that differed was 41670 in the black shark scenario: the original's shark held no word with 3
   letters typed (shocked, to die), the port's had its word back at 0 letters typed. Fix: `mWord2 = ""` after taking it in
   `BlackShark::TypeLetter`; `mWord2 = ""` and `mWord3 = ""` in `RedShark::TypeLetter`. This is a bug that breaks play (a level
   that cannot be won), so it is fixed in the port itself, not in a mod. The stealth variants reach the same functions.

2. **The abyss went on to the next level every 100 feet, the original does it every 200** (`Board::Update` 0x45c140: the check is
   `mField1c0 / 200 < mScore / 200`, with `mScore` the depth in feet; `Board::SetDepth` 0x452cf0 already counted levels per 200 feet).
   The first update that differed was 7199 of the abyss scenario, at 100 feet: the port took the lesson to level 1 and spawned a wave
   of ten single-letter piranhas, the original stayed in level 0 and spawned a wave of nine sharks (the generator then 12 numbers
   apart for good). Fix: `/ 200` in the two places of the test in `BoardUpdate.cpp`. After it the abyss scenario is equal for 18000
   updates, 484 feet. A play-breaking difference (every wave and word of the abyss follows from it), so it belongs in the port itself.

## The H1 agent's bug list, judged against the original

1. **Lives read -1 after the last life is lost.** The original does the same: it subtracts one, and a value below 0 ends the game
   (the HUD then shows "Game Over!"). The idle scenario reads 2, 1, 0 and -1 at update 13160 in both programs. Not a bug; kept.
2. **Abyss lives start at 0.** The original's `LessonManager` setup for the abyss (`SetLessonManager` 0x451cb0) sets 0 lives, and the
   abyss is played without lives. The lives value is equal in the original and the port for the whole abyss runs. Kept.
3. **The game timer keeps counting while a hint dialog pauses play.** The original does the same: the "Welcome" hint opens at board
   update 139 and stays 22 updates; the timer's three raw values, the pause flag and the hint flag are equal in both through it. Kept.
4. **Clam bonus phase: the score ticks +1 about every 27 updates while the diver waits.** Not reached. Clams appear only on the
   floor of a bonus level, which the bot never gets to. The diver's state 6 (the wait on the floor) phases of 554 updates in levels
   1 to 3 show no score tick in either program. Open item (below).
5. **The map after level 5 landed on level 1.** Not reproduced. The bot went through the map many times in one game (the adventure
   scenario to level 34) and the lesson level (`lvl`), the score, the lives and everything else are equal in both programs after
   each map.
6. **A fresh profile without the "tutorial seen" flag never spawns waves from level 5 (`mFlag1dc`).** The flag is the registry value
   `SeenTutorial`. Every scenario starts with a new profile, so the flag is 0 at the start; the first group the player kills in
   level 1 sets it (`BoardEvents.cpp`, event 6, as the original) and the waves of the later levels are equal in both programs. A
   level 5 reached with the flag still 0 is not reachable by play (the first group dies in level 1); a level jump on a fresh profile
   spawns only the two tutorial sharks, in the port's code as read from the original (the wave maker 0x455bf0 tests the flag). Not compared in a run.
7. **Typing tutor soft-lock candidate (CR, LF or Space at the first or last character of a line counts neither a hit nor an error).**
   Not a soft lock. The lessons 13, 15 and 17 (the ones with a space at the start or end of a line) end in both programs when
   the key that is next is typed as the tutor shows it (the bot sends a space as a character, CR and LF as the Return key): the
   right-key count, the errors, the clean-key count, the seconds, the words per minute and the accuracy at the end are equal, 772,
   1059 and 1407 keys with no error. The rule in the port is the translation of `KeyChar` 0x460680 and the original does the
   same. What a player sees if the key is not the one the tutor shows (a space typed where a line starts) was not tried.

## Oddities (not chased)

1. **The streak words at the end of a level run out after 1882 words (Easy) and then both programs freeze.** When the screen is
   empty at a level's end the board draws a word for the streak game with `GetLongWord` 0x430b20 (lengths `MinEndDiff` to
   `MaxEndDiff` - 1, Easy 3 to 6) and draws again until the word is not in the set of used words (+0x78c, emptied only when a game
   starts). Each word the player types brings the next draw. The bot of the adventure scenario types them one after the other and,
   in level 34 of Easy at update 153188 of the `TSBASE=24000` flow, has used all but one of the 1882 words of the lists of 3, 4, 5
   and 6 letters: the search for the last one draws 264 numbers (the generator's index goes from 108 to 372 in
   both programs, the word is WEAK) and when the bot types it (update 153201) the next search has no word left to find and never
   ends. The original does exactly this. Read at 153190, 153195, 153199 and 200 (a run past the comparison's end, with the letters W, E,
   A and K that the bot types next) it has the board update count 13163, 13168, 13172, 13173, the generator index 372, 374, 375,
   375, the word WEAK with 0, 2, 3, 3 letters typed, all equal to the port's log; at 153201 the string is empty, the index has
   gone round (214 and counting: the search is running) and the update counter never reaches 153202 (the run, started 40000
   updates later because of the load flake, shows the same update for its last three minutes until the time limit). The port is the same loop. No
   person types 1882 end-of-level words in one game (the whole adventure is 34 levels), so nothing was changed. If it should be,
   the fix is small (empty the set of used words when it holds every word of the range) and belongs in the fixes mod.
2. **The typing tutor holds uninitialised numbers until a lesson ends.** The words per minute (raw and adjusted) and the accuracy
   (+0x8c, +0x90, +0x9c) are written when the lesson ends; until then the original reads whatever its allocation held, and the port
   holds 0. The comparer leaves those three out until the lesson's state is 2 (finished). Nothing shows them before the end.
3. **The original's loading finishes at an update that depends on the machine.** Its loading thread runs on the wall clock, while the
   main thread spins through updates (about 1550 updates on a quiet machine). On a busy machine (other sessions' games running) the
   menu clicks of the scenarios, at update 4000 or even 24000, were sometimes lost: the original then drew no number (rng 624) and had
   no board or tutor at any peek. Not a game difference; the comparer now runs such an original again (up to three times) and says
   so. Origrun would need a way to wait for the loaded state itself (shared spec in the report).

## Open items (not compared in this pass)

- **Clams and the bonus level's floor** (the clam list, the diver on the floor, the score tick of H1 item 4). Clams appear only in a
  bonus level; the bot's play does not reach one in the scenarios run. The next step is a test command that starts a bonus level
  (`docs/TEST_COMMANDS.md`) and a bot that types the clam's word.
- **The stretch 100000..116000 of the base-4000 adventure** (between the two long comparisons: 49400..100000 and 116000..133190) was
  never read in the original: a first run over it was cut by the time limit of a busy machine and its first pass saw no board.
  The port played it, and both neighbours are equal; one more job with that range closes the gap.
- **RedPiranha, GhostShip, the final boss and the end of the adventure.** The adventure scenario went through level 34 of Easy; the
  last levels of the three difficulties and what follows them (the ending, the high score entry) are not played.
- **Hard and Expert** (`sc_d.py` with `DIFF=` is written, not run in a comparison). They differ in the word lists (end words of 10 to
  14 and 13 to 17 letters), the waves and the speeds.
- **Seeds 2 and 3 beyond level 3** (the adventure scenarios run to level 25 and 34 with seed 1; seeds 2 and 3 run to level 3 and,
  for the abyss, to 484 feet with seed 1 and 544 feet with seeds 2 and 3).
- **High scores, statistics, the profile and a saved game** (the screens, the files written at the end of a game, loading a game
  from the menu). The comparer reads the game's state in memory; the profile files are not compared.
- **Real mouse and key events** (not part of this pass: the scripted input reaches the same handlers as the posted events, but the
  driver pass of phase 2 covers the real path).
- **The fresh-profile case of H1 item 6 with a level jump** (a profile that has not seen the tutorial, started at a level above 1).
- **Sound, music and the pictures** are phase 2.

## Undecidable here

Nothing found that needs the user's decision. The only open question is whether the freeze of oddity 1 (both programs, 1882
end-of-level words in one game) should be fixed in the fixes mod.

## Comparer faults found on the way (not game differences)

1. Without the users folder the port wrote `users/user.idx` and drew one random number for it (log item 2 below).
2. The original's key `Space` has to be sent as `key 32`; origrun's `text` trims it.
3. The list reader looked only at the offset-0 vtable of a class. A creature held in a Board list by a pointer to another part of
   the complete object (a StealthShark by its Shark part at +0x218, a BlackShark at +0x210) showed up as unknown. The reader now maps
   every vtable of every class and takes the GameObject and Enemy part offsets relative to the pointer the list holds.
4. Lists were read to 10 to 12 creatures; a bonus wave holds 14 or more. Now 24.
5. The boss and its shots were not read from the original. Added (the boss at Board+0xeb8, its shots in the vector at boss+0xec).

## Log

1. Scenario A to the first board, no typing, 4092..4700, seed 1: equal (first creatures BEGIN and TYPING at update 4287, same
   positions, same words, same RNG index).
2. First typed run differed from update 4092 (RNG index 6 against 7): not a game difference. The port ran from an empty users folder
   and so wrote a new `users\user.idx` with a draw from the game generator (UserProfileMgr::SaveHighScores 0x41a1b0), where the
   original's copy already has the file. With both from the same `users` folder: equal.
3. Typed play (bot, a letter every 6 updates) level 1, seed 1, 4092..6200: equal. A letter every 4 updates, 4092..12000, seeds 1, 2,
   3: equal (about 7900 updates each: waves, kills, scores, EMP charge, timers incl. the hint pause).
