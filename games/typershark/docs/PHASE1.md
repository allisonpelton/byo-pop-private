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
   letters typed (shocked, to die), the port's had its word back at 0 letters typed. Fix: `mSecondWord = ""` after taking it in
   `BlackShark::TypeLetter`; `mSecondWord = ""` and `mThirdWord = ""` in `RedShark::TypeLetter`. This is a bug that breaks play (a level
   that cannot be won), so it is fixed in the port itself, not in a mod. The stealth variants reach the same functions.

2. **The abyss went on to the next level every 100 feet, the original does it every 200** (`Board::Update` 0x45c140: the check is
   `mLessonStepDepth / 200 < mScore / 200`, with `mScore` the depth in feet; `Board::SetDepth` 0x452cf0 already counted levels per 200 feet).
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
6. **A fresh profile without the "tutorial seen" flag never spawns waves from level 5 (`mSeenTutorial`).** The flag is the registry value
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
   person types 1882 end-of-level words in one game (the whole adventure is 34 levels), so nothing was changed in this pass.
   **Decided later (the coordinator, 2026-11-09): fixed in the port itself**, since a game that hangs for good is a bug that breaks
   play (CLAUDE.md, "Making a port"): `GetLongWord` empties the set of used words when it holds every word of the lengths that can be
   drawn (WinWSAppGame.cpp), draws no number for it and changes nothing while an unused word is left. Checked with the same
   adventure played on by the port's bot: the state log is byte-identical to the unfixed port's for the first 153195 updates,
   and the game goes on through update 153201 (level 34), the win, level 35 and the endless levels to 175000.
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

Nothing found that needs the user's decision (oddity 1, the freeze after 1882 end-of-level words, is fixed in the port, see there).

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

# Round 2 (job P1b)

Job P1b took the open items of round 1 in order of value. Same method; what is new is below, the results follow in the table
and the lists after it.

## Tools added in round 2 (games/typershark/tools/phase1/)

- **The map route.** The adventure's map is a graph of the 36 sites (site n is level n, `MapScreen.cpp` has the links); the bot of round 1
  took the first chest at every fork, so it sailed 0, 1, 2, 3, 8, 14, 19, 25, 30, 34 and never met a bonus (treasure) level: sites 5,
  18, 20 and 22 are dead-end branches (a `<BonusLevel>` in the cfg's levels 6, 19, 21 and 23). `-typist=...;want=a+b+c` makes the bot take, at a
  fork, the chest of the first site in the list that is on offer (`sc_bonus.py` WANT 4+5 sails 0, 1, 2, 3, 4, 5; `sc_route.py` takes `WANT=`,
  `DIFF=`, `WIN=yes|no`).
- **Streak words.** The end-of-level streak game uses up one of the 1882 words of the lists per word typed (round-1 oddity 1), so a route of
  a dozen levels would freeze the game in both programs; `streak=N` makes the bot type at most N streak words a level.
- **Clams.** `clams=mixed` plays the memory game of a bonus level by a fixed pseudo-random pattern (a pearl, an empty clam, a letter that
  is on no clam, a clam in any state), where the bot of H1 plays it ideally. `wait=N` leaves the diver N updates in the state after a
  won level before the space bar. `tutorerr=N` makes every Nth key of the typing tutor a wrong one (a space, Return, q, z, !),
  `win=yes|no` and `over=yes|no` click a button of the end-of-adventure and the game-over dialogs. `burst=B` (with `typo=N`) makes the
  last B letters of every N wrong: B wrong letters in a row, which a penalty piranha needs (see "The penalty piranha").
- **The state compared.** The statelog and the comparer read, besides the round-1 fields, the pairs of clams found (`pairs`, Board
  +0xd70), the open clam's index (`open`, +0xd6c), the end-of-level countdown (`f38`, +0xf38, a double) and the difficulty
  (`diff`, app +0x690); a clam's last field packs its kind, frame, opened flag, pulse direction and tick (Clam +0xe8, +0xf0/+0xf1,
  GameObject +0xd8/+0xe8). The comparer reads up to 24 clams (a bonus level has 22; round 1 read 14, which showed as a count difference
  at the first clam).
- **Ranges.** `--ranges lo:hi:step,...` (or a scenario's `WINDOW`) reads the original at a fine step around the updates where the lesson
  is at chosen levels and at a coarse step elsewhere. `TSBASE=12000` starts the menu flow later, which keeps the original's loading
  thread (it runs on the wall clock) from losing the first clicks on a busy machine.
- **End state.** `runorig.sh` keeps the registry key (`reg export`) and the users folder of an original run beside its log;
  `endcmp.py <tag>` compares them with the port's `registry.ini` and `users` folder of the same scenario (values and bytes).

## Scenarios and results (round 2)

`TSBASE=12000` (or 40000) starts the menu flow later than round 1's 4000; every update number below is the base-4000 number plus that
shift. "fine" means every update, "coarse" the stated step. All runs pin the seed in both programs, start from the same `users` folder and
compare the state of every update read (the fields of round 1 plus the ones of the tools section).

| scenario | what it plays | range, step | result |
|---|---|---|---|
| bon1_s1 (`sc_bonus.py`) | Easy, seed 1, the bot sails 0, 1, 2, 3, 4, 5 (`want=4+5`): five levels with their bosses and a treasure level: 22 clams (4 pearls), the diver on the floor, the bot opens the pearls in pairs, 100000 and 200000 for the two pairs, the treasure, space, back to the map | 12092..101992, fine 75735..77429 (the clams: 76833..77130), coarse 25 | equal |
| idle_s1 | Easy, nobody types after the hint: three lives are lost, the game-over dialog | 12092..24000, 1 | equal; end state: see the save/score item below |
| tutE12_s1 | the typing tutor, lesson 12 (11 presses of Next), a wrong key every 7th key (a space, Return, q, z, ! in turn): errors, the penalty, the lesson's end | 12072..20000, 1 | equal |
| hardpir_s1 (`sc_route.py`, `DIFF=2`) | Hard, seed 1, every 5th letter a wrong one, mixed clams (`clams=mixed`): sites 0, 1, 2, 3, 4 (seven piranha waves in site 4), the treasure level 5 with duds, wrong letters and blocked clams, then on to site 19 | 12092..89992, fine 74568..76838, coarse 25 | equal. No red piranha came up: see "The penalty piranha" below |
| hardpir6_s1 (`DIFF=2`, `WANT=6+9`) | Hard, seed 1, every 5th letter wrong: sites 0, 1, 6 (three piranha waves among the waves of site 6, many typos, no two in a row), 9 | 12092..70000, 7 | equal; no penalty piranha (see "The penalty piranha" below) |
| expertpir6c_s1 (`DIFF=3`, `WANT=6+9`) | Expert, seed 1, every 3rd letter wrong (a single wrong letter makes a penalty piranha here): sites 0, 1, 6, 9; the penalty piranhas (RedPiranha: 19586 updates with at least one, up to 17 on the screen, the speed growing from the sixth), the typo counter `e94` and the lesson's current wave type `cwt` compared too | 12092..70000, 7 | equal. The `users` folders at the end are equal byte for byte |
| hardpir6d_s1 (`DIFF=2`, `WANT=6+9`) | Hard, seed 1, `typo=5;burst=2` (two wrong letters in a row: Hard's `NumForPenalty` is 2): sites 0, 1, 6, 9; penalty piranhas (21042 updates with at least one, up to 10 on the screen), `e94` and `cwt` compared | 12092..70000, 7 | equal. The `users` folders at the end are equal byte for byte |
| tutE15_s1 | the typing tutor, lesson 15 (14 presses of Next: lines that start or end with a space), every 5th key a wrong one (a space, Return, q, z, ! in turn) | 12072..19000, 1 | equal (state, task, errors, clean keys, seconds, words per minute, accuracy). End state: one token of `user0.dat` differs, the uninitialised field of oddity 1 |
| rt1_s1 (`sc_route.py`, `WANT=6+9+21+26+27+32+33+34+35`) | Easy, seed 1, the whole adventure by a route of its own: sites 0, 1, 6, 9, 21, 26, 27, 32, 33, 34, 35 (every boss: TorpedoBoss, MechaShark with MechaTorpedo, MechaSquid with SquidCannonBall, GhostShip with CannonBall; GhostShark, StealthWhitePiranha, StealthRedShark and the rest of the later levels), the final level 35 won at about 187600, the "CONGRATULATIONS!" dialog answered with the button that keeps going (`win=no`), the endless levels (level -1) begin | 40092..200000 (menu flow at 40000), 7; fine 187400..187900 | equal. The `users` folders of the two programs at the end are equal byte for byte (`endcmp.py`) |

## The penalty piranha (RedPiranha), what it takes

`Board::AddPiranha` (0x452a60) is called by a typo (KeyChar 0x454e40) when the lesson's current wave type (`LessonManager+0x9c`, the type of
the wave that was just spawned: `NextWave` 0x41eb40 copies it before it draws the following wave) is 1 (piranhas), the typos counted
since the last kill (`Board+0xe94`, cleared by every shark or piranha kill: `ResetPiranhaSpeed` 0x4402f0, BoardEvents) reach the level's
`NumForPenalty`, and a piranha that is not dying (state 3) is on the screen (the typo with nothing locked on also wants the diver in
state 9, 0 or 4). `NumForPenalty` is 32765 (never) in the first two levels of Normal, Hard, Expert and X-Treme and in the first 17 of Easy
(then 3 on Normal, 2 on Hard, 1 on Expert and X-Treme, 3 on Easy's 18th), so a penalty piranha needs `NumForPenalty` wrong letters *in a
row*, without a kill between them, in a piranha wave of level 3 or later: a single wrong letter on Expert and X-Treme, two in a row on
Hard, three on Normal. The bot's `typo=N` makes every Nth letter wrong, which never makes two in a row: `burst=B` makes the last B letters of
every N wrong. Piranha waves are in site 4 (level 5, one wave) and site 6 (level 7: waves 2, 3, 4), two links from the start (0, 1, 6).
`hardpir_s1` and `hardpir6_s1` (Hard, `typo=5`) therefore never made one; `expertpir6_s1` (Expert, `typo=3`) and `hardpir6b_s1`
(Hard, `typo=5;burst=2`) do. The state log now holds `e94` and `cwt` (the typo counter and the current wave type), compared with the original's.

## Round 2: differences found

No difference of the game's logic between the original and the port has been found in round 2 (every scenario above that finished is
equal in every field read). The differences the comparer first reported were its own limits (the list below), and the one byte of
difference in an end state is the original's uninitialised memory (the first oddity below).

Comparer faults found on the way (not game differences):
1. A bonus level holds 22 clams; the comparer read at most 14 creatures of a list and then 24 (clams). Now 24.
2. An Expert piranha wave holds up to 39 piranhas (a penalty piranha is added to the wave besides); the comparer read at most 24 per
   list and showed "creature count: orig 24 port 25". The shark and piranha lists are now read to 40. `expertpir6_s1` and
   `hardpir6b_s1` carry that fault in their verdicts; `expertpir6c_s1` and `hardpir6d_s1` are the same scenarios read to 40.
3. A pass of the original sometimes reports "the original never had a board at a peek (late loading)": a menu click lost while the
   original loads on a busy machine (the loading thread runs on the wall clock). The comparer runs the pass again (up to three
   times); `TSBASE=12000` or 40000 (the menu flow starts later) makes it rare.

## Round 2: oddities (not chased)

1. **The original writes an uninitialised field into a new profile's file.** `UserProfile+0x1c8` (the level of a saved adventure game,
   `mSavedLevelNumber`; the port zeroes it, the constructors of 0x421f50 and 0x4222d0 never store it) is written by `WriteDataToDisk` as is. In a
   profile that only ran the typing tutor (`tutE12_s1`) the original's `user0.dat` holds the heap's leftover there ("137381704") and
   the port's holds 0: 8518 against 8510 bytes, the 9-character number against "0" (in `tutE15_s1` it is -2139062144, the heap's
   0x80808080 fill, 8520 against 8510 bytes; in both runs this one number is the only token of the file that differs). The field is read back only together with a saved
   game level (`mSavedLevel` between 1 and 35), which is set in the same call that stores `mSavedLevelNumber`, so nothing plays differently.
   In the files of all the games (`idle_s1`, `bon1_s1`, `hardpir_s1`, `rt1_s1`) the fields were equal, because the profile had been
   saved by a game by then. It would take the allocator's leftovers to match, so it stays.
2. **The original under origrun cannot keep any registry value.** Its `RegistryWrite` (0x4b1be0) opens and creates its key under
   `HKEY_LOCAL_MACHINE\SOFTWARE\PopCap\TyperShark` (0x80000002); origrun is not elevated, and the patch list answers the key checks
   (`CheckRegistryAccess` 0x41ac70, `RegistryGetSubKeys` 0x4bc235) as if all were fine. So no registry value of the original (the
   settings, the current user, the per-user values such as LastLesson or SeenTutorial) survives, nor is any visible to `reg export`, and
   the original's start always meets an empty registry, while the port's emulated registry (`registry.ini`) keeps them. The scenarios
   start both from an empty registry, so none of this shows in a state; but it means the registry values the port writes (90 after a
   long game) cannot be compared with the original's here, and a "quit and restart, then continue" run cannot start the original from
   a profile (it does not find the user). See the spec for origrun in the report.

## Round 2: open items (not compared)

Compared in round 2 and equal: the treasure level (clams: pearls, duds, pairs, the 100000 and 200000 bonuses, the treasure, the way back), the
whole adventure to the final boss, the end of the adventure with the endless levels, Hard and Expert from level 1 to 10 with
penalty piranhas on Expert, an idle game to the game-over dialog, the tutor with wrong keys, and the files of the users folder byte for byte
at the end of each. What did not get run, with the reason where it matters:
- **Seeds 2 and 3 of everything new.** Only seed 1 of the round-2 scenarios finished within the time (the machine had no room for a
  third game process for long stretches). The jobs are written and in `build/ts-p1/q6.txt` (the tools take `queue.sh <file>`): `expertpir6c_s2/s3`,
  `bon2m_s2/s3`, `hardpir6b_s2/s3`, `bon1_s2/s3`, `adv_s2/s3` (a long adventure, seeds 2 and 3, to update 150000), each one line of the scenario's
  options.
- **Not finished before the report:** `bon2m_s1` (mixed clam play: duds, wrong letters,
  blocked clams, reopening), `bonwait_s1` (the diver waits 1200 updates in the state after each won level: the "+1 score every 27 updates"
  of H1, `mDivePos` growing by 8 feet in BoardUpdate, looked at in the code only), `tutE15_s1`, `tutE17_s1`, `tutE2_s1` (the tutor, lessons 15, 17 and 2 with
  wrong keys: line starts and ends with a space), `rt1y_s1` (the end of the adventure answered with "return home" instead of
  "keep going": the end state of the profile, the high score, the next state).
- **X-Treme and Normal.** Not run (the same code as Hard and Expert with other cfg numbers: NumForPenalty 3 on Normal).
- **A saved adventure and "CONTINUE?" ("quit and restart")** cannot be compared with the original under origrun (oddity 2: its registry does
  not persist, so a restart does not find the user). A run inside one session (finish a level, go back to the menu with the pause
  dialog, press Adventure) needs the pause dialog's button positions and was not tried.
- **The registry values** the port writes (90 after a long game: the settings, the current user, the per-user values) are not compared (oddity 2).
- **Hint dialogs, pause, Options values, user switch, delete user, a fresh profile without `SeenTutorial`** (item 7): not run.
