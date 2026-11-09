# Typer Shark: memory leaks of the original, and the port's fixes

Job L1. The rule is that every memory leak of the original is fixed in the port, and that a fix changes nothing the player or
the game's state can see: no random draw, no state, no screen. This file lists what the original leaks, what the port does, how
each fix was proved harmless, how much memory each flow held before and after, and what could not be fixed.

## How it was done

The static audit went through every `new`, `malloc`, factory function and owning container in `games/typershark/src`, asked where
the object should die, and read the original's code for it (`python games/typershark/tools/decread.py ADDR`) to see whether the
original frees it. The leaks in the table below are the original's own.

The dynamic check is a counting build (`-DTS_LEAKCOUNT=ON`, off by default, nothing of it in any other build): `src/testhooks/
LeakCount.cpp` replaces the global `operator new` and `delete`, wraps `malloc`, `calloc`, `realloc` and `free`, hooks SDL's
allocator, and keeps a side table of every live block with its size and the call stack that made it (frame pointers, with a scan
of the stack where a frame pointer is missing). The exe is linked statically so that the libc++ DLL's own blocks do not show up.
The scratch test command `leak <tag> [sites]` (only in this build, with `press`, `dyes`, `dno`, `dlist` and `dname`, which click
the dialogs that have no screen coordinates a script could use) prints one line,

    LEAK <tag> live=<blocks> bytes=<bytes> allocs=<n> frees=<n> heap=<blocks>/<bytes> untracked=<blocks>/<bytes> handles=<n> stale=<n>

and, with `sites`, writes `leak_<tag>.sites`: the live blocks grouped by allocation stack. `live` is what the counter knows;
`heap` is what the Win32 heap walk finds; `untracked` is the difference, memory that something other than the game's own
allocator made (see "What is left"). `stale` counts side-table entries found to belong to a block that had been freed behind the
counter's back; it was 0 in every run reported here.

A scenario repeats one flow several times and puts a `leak` marker after each repeat. A leak shows as a count that grows by the
same amount at each repeat; a cache shows as growth in the first repeat only. The runs use `-fast`, a seed, the same user
directory fixture as the phase 1 runs, and run under rundesk like every game run.

## The leaks

"Original" says what the original does, with the address of the function that does it. Everything under "this pass" is new in
this job; the earlier ones were fixed while the code was translated and are listed so the file is complete.

| Object | Made where | Original | Port |
|---|---|---|---|
| The profile a `SaveUser` replaces, with its pairs and stat records (about 0x2c8 bytes plus 12 vectors, 16 blocks) | the startup profile in `UserProfileMgr`'s constructor, a profile made by the select dialog's OK | 0x420c60 overwrites the list slot and never frees the old profile | **this pass (F1).** `UserProfileMgr::SaveUser` deletes the replaced profile. Both callers (the select dialog's OK for a profile whose file did not load, and `ShowMapScreen` with no user) have just lost their `mUser` to a failed load or never had one, so nothing else points to it |
| A boss shot that has finished (CannonBall, Torpedo, MechaTorpedo, SquidCannonBall), with its sprite vector and its int vector | `GhostShip::CreateProjectile`, `TorpedoBoss::Update`, the other bosses | 0x40b360 takes the shot out of the boss's list and never deletes it | **this pass (F2).** `ProjectileBoss::Update` hands the shot to `mFinishedShots`, `Board::Update` moves them to `Board::mRetired`, and `~Board` deletes them after the boss. A shot is not deleted earlier because `mTarget` may still point to it: `HandleBossEvents` event 0x15 deletes it and removes it from `mRetired` first |
| A bonus creature that left through the top | the bonus level | 0x45c140 takes it off the list and never deletes it | **this pass (F3).** Put on `mRetired` and deleted in `~Board`, for the same reason |
| The game generator (`gGameRand`) | `WinWSApp::Init` | never deleted | **this pass (F4).** Deleted at the end of `~WinWSApp`, after everything that draws from it |
| A `TutorExercise` whose `<TASK>` is never closed with a line of its own `</TASK>` | `TutorLessons::NextLesson`, reading a lesson file | 0x429930 assigns the new task to its local without a look at the old one | **this pass (F6).** The old task is deleted when a new `<TASK>` starts and at the end of the file if one is still open. Lesson 4 has two such tasks (the `</TASK>` follows a `<TEXT>` on its line), so every load of lesson 4 leaked two exercises with their strings |
| The Courier font | `WinWSApp` constructor | never deleted | earlier: deleted in `~WinWSApp` |
| `Board`'s timer at +0x58 (a `WSTimer`, 0x64 bytes) | `Board` constructor | never deleted | earlier: deleted in `~Board` |
| The depth markers of the Abyss board | `Board::SetDepth` | forgets the old ones without deleting them | earlier: deleted before the list is cleared |
| `DeleteUserDialog`'s user list (0xc0 bytes for each dialog) | the dialog's constructor | 0x438f20 does not delete it | earlier: deleted in the destructor |
| The timer of `ProjectileBoss`, of `TutorialPanel`, and the three timers of `TypingTutor` | their constructors | never deleted | earlier: deleted in the destructors |
| The lesson file | `TutorLessons` | never closed | earlier: closed (`p_fclose`) |

The numbering of F-fixes follows the order they were found in; F5 was a record growth in `UserProfile::LoadDataFromDisk` that
turned out not to be a leak (see "What is left").

Nothing else turned up. The abyss and adventure levels, the high score, stats, options and credits screens and the delete and
select user dialogs return to their count after every repeat once the first repeat has filled the caches. Making a new user adds
that user's profile (164 blocks, which the profile manager keeps), so the new-user flow grows by that much at each repeat in the
before and after runs alike; it is data, not a leak.

## The fixes and what changed in the code

All in `games/typershark/src`, each with a `// leak fix (the original leaks this): ...` comment that carries the original's
address: `UserProfileMgr.cpp` (`SaveUser`), `ProjectileBoss.h/.cpp` (`mFinishedShots`, the destructor), `Board.h` (`mRetired`),
`Board.cpp` (`~Board`), `BoardUpdate.cpp` (the bonus creature, and adopting the boss's finished shots after `mBoss->Update()`),
`BoardEvents.cpp` (event 0x15 removes the target from `mRetired` before it deletes it), `TypingTutorLessons.cpp` and `WinWSApp.cpp`
(`~WinWSApp`). No fix adds a draw from a generator, a branch on a game value, or any state a later update reads; each only frees
memory the game never reads again, and the freeing waits for the board's or the app's destructor wherever a pointer could still
be held (`mTarget`).

## Proof that nothing changed

The "before" exe is the tree at HEAD (the commit before this job), the "after" exe is the tree with the fixes; both are normal
builds, started with the same seed, script and user directory fixture under rundesk with `-fast`. The comparison is
`build/ts-p1/r1/cmp_dirs.py`: every `-statelog` file byte for byte, every screenshot PNG by hash, and the update count of each run.

| Batch | What | Result |
|---|---|---|
| R1 (`run_batch.py`) | 12 runs: abyss1, abyss2, abyss3, blk7 (153195 updates: the adventure to level 34 with MechaSquid), emp, idle, long1, scA, tutor15d, tutor17, typo, ui | all 12 statelogs equal, 132 PNGs equal, update counts equal |
| route | the whole adventure of phase 1's rt1_s1 (200600 updates; every boss, so every kind of boss shot, bonus creatures, the endless levels), 9 screenshots; the 83,923,139-byte statelog | equal, PNGs equal |
| tutor | the typing tutor with the lessons paged (5530 updates), 12 screenshots | equal, PNGs equal |
| counting build, usersel | the counting exes before and after; the select dialog's OK for three profiles whose files are missing, five repeats (8150 updates), 6 screenshots | statelog equal, PNGs equal |
| counting build, menus | the same exes; the menu screens, the tutor, new, select and delete user dialogs, idle (24810 updates), 31 screenshots | statelog equal, PNGs equal |

The route is the run that matters most: it is the phase 1 route that was compared with the original, so equality with HEAD's exe
is equality with the original. The fix for a bonus creature that leaves through the top (F3) never ran in any of these (no
bonus creature left through the top, see below); it is proved only by the code being a push to a list that is read nowhere before
`~Board`. The select-user fix (F1) runs only in the counting-build usersel scenario, whose logs and screenshots are equal.

## Live blocks before and after

Counting build, same seed, the marker numbers of `scenarios.py`. "Before" is HEAD's code in the counting build, "after" has the
fixes. The numbers are live blocks counted by the counter (`live=`); a repeat's growth is the difference between its marker and
the one before.

| Flow | Marker | Before | After |
|---|---|---|---|
| Select user, OK for three profiles with missing files, five repeats | s0, then s1..s5 | 33260, then 33268 and flat (+8 in the first repeat) | 33252, then 33252 and flat |
| Select user, at exit | exit | 126 | 109 |
| Typing tutor, enter, page lessons 3, 8, 0, leave, four repeats | t1 to t4 | 32934, 32950, 32958, 32970 (+36 over three repeats) | 32926, 32934, 32934, 32938 (+12, SDL texture caches; 8 blocks a repeat fewer, the two exercises of lesson 4) |
| Menu screens, tutor, new, select, delete user dialogs, idle | u0 to i4 | 32523 to 34546 | 32523 to 34514 (32 fewer, the four lesson 4 loads) |
| Menu flow, at exit | exit | 142 | 109 |
| Tutorial 15d, 17: start to end of the game | tu0..tuend, tv0..tvend | 33857 to 33067, 33857 to 33072 | 33857 to 33059, 33857 to 33064 |
| Tutorial runs, at exit | exit | 118 | 109 |
| Abyss, one game | ab0..abend | 33857, 32700 | 33857, 32700 |
| Abyss, at exit | exit | 111 | 110 (the generator) |
| Whole adventure (route), back at the menu | r0, rend | 33856, 33510 | 33856, 33162 (348 fewer: the boss shots) |
| Whole adventure, at exit | exit | 459 | 110 |

In the route, the sites that grow in the "before" run and not in the "after" run are the boss shots: `GhostShip::CreateProjectile`
(416 bytes each, 40 of them), the sprite vector of each `Torpedo`, and the int vectors `Torpedo::Update` fills (116 blocks
from `TorpedoBoss`, `GhostShip` and `MechaSquid`), among others. The remaining growth of both runs is data the game keeps: the profile's map
lists (`CopyMapListsToUser`, 70 blocks, the map's size), the registry emulation's entries (`RegistryWrite`, one block per key),
and the SDL texture caches.

At exit the fixed build holds 110 blocks: about 100 are SDL2.dll's own (about 4.6 KB, see below), 7 are the C runtime's startup
strings (608 bytes), and 2 are the framework's statics (`gRects`, 16 bytes, and `gPixelBuffer`, 522,240 bytes), which static
destruction frees after the report. No block the game made is live.

## What is left

**The graphics driver's memory.** The `untracked` column grows during play, in both builds: in the route from 13,514 blocks
(6.7 MB) to about 31,100 (10.3 MB), and it falls back only at exit. The blocks are not the game's: the allocation hook on the
process heap (`TS_LEAK_HEAPHOOK=1`, `LEAK_HEAP=1` for `leaksites.py`) attributes them to nvwgf2um.dll and d3d11.dll, the NVIDIA
Direct3D 11 driver under SDL's renderer. They scale with what is drawn and with the texture uploads, not with what the game
leaves behind. It cannot be fixed in the port; whether the number of textures `DDInterface` makes and releases adds to it is a
question for the shared renderer.

**SDL is never shut down (shared code).** `SexyAppBase` never calls `SDL_Quit`, so SDL2.dll's 100 or so blocks (4.6 KB: window,
renderer, audio and event bookkeeping) are live when the process ends. The operating system reclaims them. Spec for the
coordinator: after the app's shutdown has destroyed its window and renderer, call `SDL_Quit()`; it changes no game behaviour. Not
done here because `framework/` is shared.

**Caches that are not leaks.** SDL's render command pool and the texture-data caches of `DDInterface` (`GetTextureData`,
`PrepareTexture`) grow in the first repeat and then hold steady; they are what the shared renderer keeps for speed. The two framework
statics named above are held until static destruction.

**Data the game keeps.** `UserProfile::LoadDataFromDisk` appends the records it reads after the 35 default records of each group
(the original does the same; the comment at the function says so). A freshly made profile loads once, so nothing grows from
repeated loads, and the memory is freed with the profile. It is held, not leaked, and was not touched.

**A fix never seen at run time.** The bonus creature that leaves through the top (F3) did not do so in any scenario or phase 1
run, so only the code reading and the clean statelog proof stand behind it.

## Running the counting build again

From the worktree, in order:

    bash games/typershark/tools/leak/build.sh dbgA            # configures and builds build/ts-leak/b (-DTS_LEAKCOUNT=ON), copies the exe and SDL2.dll to build/ts-leak/dbgA
    python games/typershark/tools/leak/scenarios.py           # writes build/ts-leak/scripts/*.txt (route needs build/ts-p1/rt1_s1.port.txt)
    bash games/typershark/tools/leak/runall.sh dbgA rdA menus route abyss1 tutor15d tutor17
    python games/typershark/tools/leak/leaksites.py build/ts-leak/dbgA/typershark.exe diff build/ts-leak/rdA/leak_u1.sites build/ts-leak/rdA/leak_u4.sites

`runall.sh <exe folder> <result folder> [scenarios]` runs the scenarios one after another; each waits for room under
`tools/gamecount.py`. `run.py <exe> <outdir> <tag> <seed> <script>` runs one; `RUN_USERDIR=<dir>` runs on that user directory as it
is (the select-user scenario needs a directory made by `usermk` with `users/user1.dat` to `user3.dat` deleted). The result folder
gets `<scenario>.out` (the `LEAK` lines are in it), `<scenario>.sl` and the `leak_<tag>.sites` files.

`leaksites.py <exe> show|diff|udiff|total <files>`: `show` lists the live blocks by site, `diff A B` the sites that grew from A to
B (a leak shows as growth by the same amount at every repeat), `total` the sum. Both files of a `diff` must come from the same exe
(a frame is module plus relative address). `LEAK_LINES=1` shows file and line from the exe's line tables, `LEAK_HEAP=1` reads the
`H` lines of a run with `TS_LEAK_HEAPHOOK=1`.

To measure the "before" state, build the counting exe from a tree with the nine fixed files (`UserProfileMgr.cpp`,
`TypingTutorLessons.cpp`, `Board.h`, `Board.cpp`, `BoardUpdate.cpp`, `BoardEvents.cpp`, `ProjectileBoss.h`, `ProjectileBoss.cpp`,
`WinWSApp.cpp`) at HEAD's version and `LeakCount.cpp`, the CMake option and the scratch commands of `TestCommands.cpp` kept; build
it to its own folder (`build.sh dbgB`) and run the same scenarios.
