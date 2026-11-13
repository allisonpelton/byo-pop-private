# Venice Deluxe: game notes

What the port does differently from the original, what it leaves out, and what is odd. The analysis state is in
`WORKING_NOTES.md`; the code is the translation of `VeniceGame.exe` (Retro64 and PopCap, 2007), written to
`docs/CODE_STANDARDS.md` from its first draft.

## Test commands

Reachable only from a test (`-script`, `-commands`, the web page's test call; `docs/TEST_COMMANDS.md`). Replies go to stdout:
`STATE {json}`, `OK <command>`, `ERROR <command>: <reason>`.

| command | what it does |
| --- | --- |
| `state` | `{screen, scenes, mode, modename, level, score, lives, profile, over}` (`over`: a summary screen is up, or the level in play is won or lost) and, while a level is in play, `timer` (updates), `moves` (shots fired) and `board` (`plates`, `slots`, `filled`, `middle`, `shooter`, `hits`) |
| `goto <screen>` | `menu`, `options`, `help`, `highscores`, `credits`, a mode (`journey`, `flood`, `survival`, `trickshot`) or the name of any screen the game has; the scenes there are killed |
| `level <n>` | starts level n of the current mode (a trick shot's number in Trick Shot) over the scenes there |
| `unlock all` | the flags that unlock Flood, Trick Shot and Survival and every Journey and Flood level |
| `set score\|mode\|level\|lives <v>` | the profile's score, mode, level or tries |
| `save` | writes the profiles file (the game writes it when a player is made or a level ends, not at quit), so that a run can make a player for another run |
| `win`, `lose` | the level in play goes to its victory stage, or is lost as a shot that fell is |
| `seed <n>` | seeds the game's random numbers |
| `move` | `MOVE {"kind","input"}` for the bot's next shot (`Bot.cpp`), a `wait` while the level is not ready for one, or `MOVE null` |
| `memreport` | the framework's, `MEMREPORT {...}` |
| `plates` | `PLATES [...]` for the level in play: each plate's shape, position, stage and its slots' `[kind, filled]` |
| `buttons` | `BUTTONS [...]` for the screen on top: each object a click works on, with its centre `x`, `y`, size `w`, `h`, the event's `action` number and `parm1` (the menu buttons' places for a driver that clicks) |
| `after <frame> <command> ...` | runs the command before the first update after the frame (frames are counted from the first update that runs with the game loaded), for a script that replays another run's input at its frames |
| `trace <file>` / `trace off` | writes a `FRAME` line of the game's state for every frame (tick counter, load state, seed, scene tail, score and, while a level is in play, its plates, slots and hits) and an `INPUT` line for each input of the bot; the identical-play harness compares it with the original's |
| `ticks <file>` | the number of game updates to run in each frame, one a line, so that a run takes the same ticks in the same frames as the original's (catch-up frames run more than one) |
| `player [name]` | the name box's work for a test that does not type the name: the first free slot gets the name (Test by default) and is the current player; a player already chosen stays |
| `bot <frames> [continue]` / `bot off` | plays for that many frames with the bot's moves, as real input (`move`, `down` and `up` at the pointer's place); with `continue` it also clicks the summaries and the level select away, so that it goes on from level to level |

The commands wait for a player: the name screen is up until a name is typed (`key T` ... then `keydown RETURN` and `keyup RETURN`
a few updates apart, since a button's alt key needs both). The bot reads the game's state and draws no random number. The shooter always points straight up and follows the pointer's x, so a shot is an x: the bot aims at the lowest unfilled slot on the screen that the ready jewel fills (any for a wild one), moves the pointer there, waits for the shooter to catch up (it closes 1/1.3 of the distance an update), presses, waits two updates and releases; it also clicks away the cupid's messages and the click that starts the tutorial or explains a Flood level. When the ready jewel fits no slot but the one waiting behind it does, it swaps them with a right click (the burst of one is seen: the game keeps the right button as a key state). In Trick Shot, where one shot goes by way of bumpers, each attempt aims at another x. It leaves a slot alone while a jewel it threw is on the way, but does not allow for plates that move or turn, nor for bumpers, and it never uses a power-up, so it fills about two thirds of what it shoots at on the first levels and loses the later ones. The press is held two updates because the game reads the button's state once an update: a press and release that both come between two updates (the burst a posted click is) is not seen, as in the original.

## Comparing screens, sound and focus with the original

`games/venice/tools/idplay/idplay.py` runs a scenario of `scenarios/` on both sides. `shots <scenario>` runs the original in
Windows Sandbox under origrun (the only place it takes shots) and the port natively under rundesk (`-fast -drawevery
-renderscale=1`, the original's ticks for each frame), then `tools/shotdiff` compares each pair; `sound <scenario>` compares the
original's BASS calls (`tools/bassproxy`, a logging BASS beside the exe) with the port's `BYOPOP_SOUNDLOG` by `soundcmp.py`, and
`webshots.py <scenario>` takes the same shots from the web build under Node (`wasmrun.js`) and compares them with the native
ones. The tours: `shots_name`, `shots_tutorial`, `shots_menus`, `shots_ingame`, `shots_flood`, `shots_survival`,
`shots_trickshot`, `shots_journey` and `shots_focus` (the window losing the focus and getting it back; its original run needs
the sandbox, and a shot in the lost state is a `waitshot`). `shots_menus3d` is the 3D original's menus (see "Left in phase two"
in `IDENTICAL_PLAY.md`). A scenario's `SHOT_MARGIN` (1000 frames) keeps the run going past its last shot.

## Deviations (each without a visible effect)

- Loading is done in slices of a few files a frame with the window drawing meanwhile, pictures at first use and sounds at
  their first play (decoded then); the original reads and decodes everything on a thread while the loading bar draws.
- The layer merge (0x40de30) is the original's, with its bands: positions reset from the stage start values, the content
  height measured, the filler pictures of the mirrored backdrop added on a display that is not accelerated, and the pictures
  drawn into 800 x 1024 pictures (the original's textures, here framework images) that take their place. The draw into a band
  is the original's second copy of the picture draw (0x442490, `DrawImageRecordOnTarget`), the render target being the
  wrapper's field at 0x5ba42c + 4 (`gRenderTarget`). A band on a display that is not accelerated starts opaque black (the
  original's surface has no alpha; where no picture covers it the screen is black, not see-through), and the framework's
  software triangle drawers are registered when the app is made (`SWTri_AddAllDrawTriFuncs`, as the 1.3 games do), since the
  band's flipped pictures are drawn by them. Those drawers clip a transformed draw to the clip rectangle's own far edges
  (x + width, y + height: 0x504860 passes them so), where the released source clips one pixel inside; the app sets
  `SWTri_SetClipToFarEdges(true)`, or the last column of a flipped picture that reaches the clip (the Journey sky's right
  column, in every band) stays black.
- A transformed picture on a display that is not accelerated is blended normally whatever the scene's draw mode says (the
  original's software surface does; only the accelerated target adds), so `DrawImageRecord` draws it in the normal mode and
  gives the mode back. Found with the loading screen's sparkles, blit style 1 and scaled.
- `VeniceWidget::Draw` sets `mDDInterface->mSoftwareTransforms` whenever the display is not accelerated, as Bejeweled 2 and
  Rocket Mania do: the original's software screen draws a transformed picture (text, sprites, jewels, flipped pictures) with its
  own triangle code straight on the surface, and the framework's hardware-style transform path samples at other places, an
  edge a fraction of a pixel off. It costs time in the 2D port (a `-fast` Survival tour of 20 shots takes about 486 s native
  with it); the performance pass starts there.
- `Powerup`'s wings text calls `SceneObject::AddFilter` and `SetAttribute` directly (0x417b90, 0x428610, 0x428700), as the
  original does; the virtual `Layer` versions had put the filter on the wrong object.
- The sine and cosine are `VeniceMath.cpp`'s, not the C library's: the original's x87 `fsin` and `fcos` give the correctly
  rounded double, and the libraries of the two back ends (UCRT natively, musl in the web build) each differ from it in the last
  bit for about 3 % of the angles, enough to move a plate or a sprite by a subpixel and show in the picture. The port computes
  the sine and cosine of the float in double-double arithmetic (a quarter-turn reduction with 33-bit pieces of pi/2, Taylor terms
  to the 17th; the terms from the tenth on in plain double), the same bits natively and in the web build (300,000 angles
  compared) and the correctly rounded double in every one of 1,980 angles compared with a 70-digit reference. An angle of 10^6
  or more, which the game never makes, goes to the library.
- No pool of 4000 objects (the original's limit); objects are heap objects.
- `Emitter` guards a division by zero.
- The name box does not write the typed text back into the translator's static buffer.
- `DrawTextOnObject` keeps its text object in a heap static; `PlaySoundByName` and the checkbox's hot sound do nothing for a
  sound that is not there; `FormatScore` has static buffers; the tooltip buffer is as long as its text.
- `Slot::Load` frees the siblings it removes when they are not selectable; `Warp` and the quotes and tips use
  `std::vector`; a quote number out of range shows none.
- The volume setters ignore the original's flag at 0x5b51cc (it is never set in play).
- `UserDataDisplay` draws nothing for a kind of display the game never makes; an unknown slider kind starts at the left.
- Switching to a window does not refuse a desktop that is neither 16 nor 32 bits.
- `MissTrickShot` does nothing to the music when the level has none.
- `Straighten` and `UpdateJewelColours` are simplified to what the data files use.
- `MergeChildren` guards a missing backdrop picture and a missing lowest child.
- A firework's last event rebuilds its stages (0x404580 from the custom event), which frees the stage that is firing its events;
  the original goes on reading and writing that freed stage (0x42a980, 0x429f10), the port stops the stage's update there
  (`SceneStage::CheckEvents` returns false) and gives the result the original's read of the stale progress gave. Found by the
  AddressSanitizer run of the win sequence (level 72).

## Fixes (the fixes mod)

`mods/fixes/mod.json` holds the fixes mod, each fix a toggle read through the options layer (`Platform::FixOn`, `platform/GameOptions.h`;
`-fix=<id>` on the command line, `-original` for none). Nothing set means every option off, the original, which is what every
test and the frozen baseline run. `main.cpp` loads the mods folder (`-modsdir=`, `-nomods`; `mods-venice` next to the exe, or
`/venicepersist/mods` on the web). `tools/fixes_check.sh` plays `tools/fixes_test.txt` four times (as the original, with the
mod's defaults, with `-fix=typos`, with both fixes on) and checks the screens that must differ, the ones that must not and
that no state line changes.

| id | default | what the original does | what the fix does |
| --- | --- | --- | --- |
| `typos` | on | seven texts of the data files are misspelt: the name box's request on an invalid name ("PLESE TYPE IN A", `levels/invalid_profile_name.xml`), the titles "DONT LOOK AT ME" (trick shot 46) and "SWIRLD IN, SWIRL OUT" (the bonus level, trick shot 90) and four messages of trick shot levels ("SQUAKED IT THROUGH!", "YOU GOT THE RYTHM!", "GODE JUDGEMENT!", "GET THAN ANGLE!") | `CorrectedText` (`TextFixes.cpp`) spells them right where a level's title and messages and a text object's string are read |
| `trickMessages` | off | the summary of a trick shot (`TRICKSHOT_RESULT` in the text translator, 0x405e10: the text of the red frame) copies the level's own message (`win_message` or `lose_message` of its file, offsets 0x320 and 0x360 of the level) into its text and the next statement writes "GREAT SHOT!" or "MAYBE NEXT TIME" over it, so none of the 104 levels' own messages shows | the summary shows the level's message (`GameText.cpp`); off by default, since the original's summary looks intended and the fault is not widely known |

Candidates not made: none.

## Oddities (not chased)

- The first update of a `Firework` draws random numbers, as does its constructor, whatever the effect shows.
- `BonusBasket`'s search for a jewel continues from a jewel that is in the basket.
- A `Powerup` that closes in the Journey mode turns Rewind into Wings.
- The tutorial's first lesson shows its tip again on every update until a target slot is found.
- `LevelScroller` runs its setup twice.
- `CutTextLine` with no space within the width gives an empty line and skips a letter.
- The layer parallax is unstable in the original too, when the parent layer is taller than the tiles layer and the tiles are at
  rest (the content heights measured at the merge decide it).
- A scripted key press needs its release some updates apart.
- The destructor of the old `Level` (0x40ea20) sets the volume of the music's channel to 0 for one update when the next level
  has started the same music: BASS gives the new replay the old, stopped replay's channel handle, so the call reaches the music
  that is playing, and the next update's fade sets the volume back. The port's old level finds its own voice stopped and does
  nothing. It is the 4 ms of an update (15 times in the Trick Shot tour, 3 times in the Flood tour); to make it the same the
  port's sounds would have to share one voice handle for each file.
- The screenshots of the port and the original differ by single pixels and thin edges, never beyond the tolerance of
  `tools/shotdiff` (the largest, 696 pixels of 480,000 in one pair, at most 24 levels off in a colour): the antialiased edge of
  a transformed picture, a sparkle's blend, a glyph drawn through a transform. The cause was not looked for further: it
  has no effect on play, and the likely place is the rounding of the triangle code's subpixel positions (the original's x87
  against the port's compiler code).
- The original's 3D mode (Direct3D 7, here over dgVoodoo2) plays a little differently from its software mode and the port,
  and the two part after about 16000 updates. In the `journey3d` script (the first Journey levels played by the bot) the
  plate that turns and wobbles after a hit (plate 8 of the level in play) is 0.014 degrees off in the 3D original before any
  jewel is near it (the plates that turn at a constant rate agree to 0.00005 degrees, the rounding of a float), a jewel that
  bounces off one of its slots leaves 0.05 degrees off, and at update 16144 a falling jewel that is past the bottom edge by
  0.24 pixels in the port is already past it, by the box test of `Jewel::KeepInBounds`, in the original: the original draws
  the 58 random numbers of the splash one update sooner and the seeds differ from then on. The same inputs on the original in
  its software mode match the port for all 20001 frames, and the 3D original matches the port for the first 16143. Cause
  not found: `docs/FRAMEWORK_2004.md` ("3D and the FPU") infers that Direct3D 7 leaves the x87 at 24-bit precision (no
  DDSCL_FPUPRESERVE), and the plate's drift is the size of a rounded sine filter's, but rounding that filter to float in
  the port changed nothing in plate 8's rotation, and the drift is already there 6 updates before the jewel that starts
  the visible difference is thrown. What it would take: the original's plate 8 value by update from its last hit on
  (origrun peeks, one run of about 7 minutes each, the line of peeks limited to about 900 characters) to find the first
  value that differs, or the x87 rounded to 24 bits for every double expression of the port when its 3D mode is on.

## Not translated: what nothing in play can reach

The editor (class 0x42dc60, no caller of its constructor) and everything only it uses: 0x42dc60-0x42e560, 0x42eb20-0x434b70.
Functions by address that nothing calls: 0x41f290, 0x414bd0, 0x410020, 0x410320, 0x410370, 0x410550, 0x410570, 0x410630,
0x414be0, 0x414d90, 0x418bc0, 0x41bda0, 0x41ba70, the plate and slot editors' save functions, the level's editor slots, and the
tutorial's 0x421130 and no-ops 0x421100, 0x421110, 0x421120. The library's own list and allocator helpers and the object pool
functions are replaced by the standard library.

## Coverage check (2026-11-12)

The whole coverage check of the game (`docs/COVERAGE.md`): origrun's `--coverage` recorded which of the original's functions
ran, in 50 runs on the host (`games/venice/tools/coverage/run.sh`: a copy of the patched game for each run, the virtual clock
and no sound device; the hits are in `build/coverage/venice/hits/`, kept out of the repository). Each run took one to three
minutes of machine time, and they ran one at a time, since the original allows one instance.

- **The identical-play tours:** tutorial, Journey, Flood, Survival, Trick Shot, the menus and the name screen (each reached
  about 1400 to 1580 of the 6019 functions that the export watches).
- **Scenarios for what they miss** (`games/venice/tools/idplay/scenarios/cov_*.py`; each makes a profile with the test
  commands `unlock all`, `set mode` and `set level`, clicks Play Now and Continue, and then the port's bot plays, its moves
  given to the original as input): Journey from 21 levels that `tools/coverage/levelfeat.py` picked (it says which levels
  hold which kind of object or action: levels 4, 8, 10, 11, 13, 14, 16, 22, 25, 29, 32, 38, 41, 46, 47, 52, 56, 58, 68, 71,
  72), Flood and Survival to its end, Trick Shot from levels 26, 41, 53 and 87, the options' sliders, the name screen's
  players (`cov_players`) and the window losing and getting back the focus (`cov_focus`).
- **Runs on a patched copy of the original**, so that the play reaches what the bot seldom does (a coverage run reads only
  the port's inputs, so the original may play on differently from the port): every power-up of one kind (rewind, combo
  shot, score multiplier, cupid: a store in the Powerup constructor at 0x4166df made unconditional), every falling jewel
  landing in a bonus basket that waits (the overlap test's jump at 0x4010fd) and every filled slot cascading as a combo
  shot's does (the test at 0x41cb35). A copy takes static byte patches of the game's own code; they are `run.sh`'s
  `POWERUP_KIND` and `EXTRA_PATCH`.

A scripted `click` (press and release in the same update) fires nothing in the original, whose buttons read the press in one
update and the release in a later one: the scenarios send `move`, `down` and `up` at least two frames apart.

`games/venice/coverage.txt` sets the game's own code as two ranges: the game, the Retro64 scene engine under it and the app
(0x401000-0x442f60), and the app's destructor and button-listener glue that the linker put after the framework's first
functions (0x4434a0-0x4434d0). The rest of the exe is the SexyAppFramework, the C runtime and the libraries. The game's own
code has 967 functions; the first report's runs reached 596 of them, and the 50 runs reach 696.

**Reached in the original with no counterpart in the port: none left.** Each function of the first lists got one of two
things, and none changed play (the port's own tests are unchanged):

- **Placed:** functions the port translates inline or in another shape got their address in the comment above the function
  that holds them (the quick constructors of the level locker and of the bumpers, the Survival scroll's tail, a picture's
  release, the four message wrappers of the popup 0x41d600 and the table of the game's own classes at 0x4076b0).
- **Left out on purpose, as `none` lines with a reason each:** the C++ library's stream, locale and string instantiations
  (the port uses the standard library), the compiler's deleting destructors and one-line wrappers, the engine's list and
  object pool functions (the port's objects are made with `new`), and the constructors of the bonus basket (0x409a30), the
  endless bonus (0x409a50) and the level scroller (0x409a10), which only set a virtual table.

No missing game logic was found.

**Never reached and not defined in the port: 251 at the end, all left out on purpose.** `tools/coverage/deadcode.py` sorts
them by who calls them in the export. Six have a caller that a run reached and were judged by hand: the engine list's
remove, the level scroller's constructor, a wrapper, two empty callbacks, and the level editor's use of the name box (0x42d930;
Return reaches it only with the editor up, so the port's `InputBox` leaves it out, as its header says). The rest are dead
clusters, whose every caller is itself never reached: the level editor (about 140 functions: its classes, the writers that
save an object, a stage, a filter or an event as a tag, and the destructors of its MFC frame window; nothing in play makes the
editor, since no function calls its constructor), the C++ library's instantiations (70), and 36 functions that nothing calls
and no virtual table the game's code calls through holds (a private random number generator, string scanners, the editor's
win-message writer and the like). Each has a `none` line, and none is in the port.

**Defined in the port, never reached: 104 at first, 20 at the end.** The runs reached the rest: the rewind, combo shot, score
multiplier and cupid power-ups, the bonus basket's catch and dump, a bumper that breaks into particles, the cascade's lines,
the tosser plate, and the Survival, options and name screens. Of the 20:

- **Read again against the export, each matching it:** `ShowCatchScore` (0x417200: a power-up caught after the level is won),
  the M plate's hit (0x416280, which the port shares with the A and step plates' `WobbleOnHit`; the plate is only in level 68
  of Journey and Flood, where three runs never threw a jewel at it), `TextObject::GetBounds` (0x425220), `Plate::TossSlots`
  (0x412ba0) and `VeniceWidget::MouseWheel` (0x441230: the wheel counts as a press of the up or down key; origrun has no
  wheel input).
- **For a night run (22:00 to 08:00), still open:** the options' two check boxes, hardware acceleration and full screen
  (`Checkbox::Toggle` 0x4379f0, `SetAcceleratedDisplay` 0x4418a0 and `SetWindowedDisplay` 0x441930), which change the
  display, and with the 3D mode on the glow of a text that the pointer comes onto (the only caller of `GetBounds`). The
  tutorial's "don't show this again" check box (the same `Toggle`) only sets a flag, but two tries at its place reached
  nothing (the screen's `buttons` list does not hold it); it waits for the night too.
- **Kept, a virtual's empty base:** `SceneObject::OnCustomEvent` (0x429d20); the objects that get a custom event (the basket,
  the slot) have their own.
- **Candidates for taking out unneeded code: 12 functions.** No play of the original reaches these; they were taken out on
  2026-11-12 ("Unneeded code taken out" below):
  - `LevelScroller` (0x411a30, 0x411a50, its factory entry and the `DrawHot` 0x427760 that only it calls): the object exists
    only in the editor's palette (`objects/level_scroller.xml`); no level file creates one, and the level select scrolls
    with its own stages.
  - `ShooterPath::Update` (0x41bb70) and its factory entry: no level has a `shooter_path` object, and the scroll point that
    derives from it has its own update.
  - `SceneObject`'s `Rewind` (0x426000; only itself calls it), `Rotate` (0x4262a0), `GetLocalRotation` (0x427c60),
    `SetImageFrom` (0x427fe0; its only caller, 0x41f290, has none), `CircleOverlapWith` (0x428e80), `CountStagesBefore`
    (0x4295e0) and `AngleTo` (0x429810).
  - `Plate::TossSlots` and the tuning `toss_on_filled_plate`: the shipped config sets it to 0, and the plate's `fade_out`
    branch is its only caller.

Left open: the night run above. The 50 runs took about two hours of machine time. The fixes mod was written alongside
(`Fixes (the fixes mod)` above).

## The autosplit block (2026-11-12)

Venice Deluxe has a speedrun.com board (Journey and Flood by building, and Trick Shots), so the port writes the shared autosplit block the
site's Splits window reads (`platform/AutosplitBlock.h`, layout 2; `docs/WEB_PLAYER.md`, "One design for every port"). The observer is
`src/LiveSplitObserver.cpp`, called after each update of the scenes from `RunDueUpdates` (the updates a game without the focus does not run
are not counted) and from `~VeniceApp` for the last write. The block's game id is `VNDX`. It reads the scene stack, the level in play and the
current profile, and writes nothing of the game's: until something asks for it, it only tests a flag (the mod list is looked at every 64
updates). The page asks by calling `byopop_livesplit_block` (exported in the web build: `EXPORTS _byopop_livesplit_block` in
`games/venice/CMakeLists.txt`); natively the `livesplit` feature of an enabled mod does (`mods/livesplit/mod.json`, a manifest only).
`-livesplitlog=<file>` writes one line for each write that changes what a splitter acts on (board, events, state, level, mode, flags), and the
events go to the log as `livesplit: board 1 GO at tick 8936 (mode 1)` and the like. The game itself gained nothing for it: no counter, no hook
(its state is the scene stack and the profile that the game keeps anyway).

**A block "board" is one pass through a mode.** It starts the update after a click on a mode of the mode menu (a scene is above
`mode_select_menu`) and ends when the game is back at the main menu (the mode menu closed, a quit from the pause menu, `goto menu`). It holds
the continue prompt, the tutorial, the maps, the levels, the summaries, the honor roll and the win scene of that mode, and a second click on
a mode is a new board. A block asked for in the middle of a level takes that level's game as its board, with no GO.

**Numbering.** `mode` as the profile numbers it (`GameMode`): 0 Flood, 1 Journey, 2 Survival, 3 Trick Shot; `skill` 0 (the game has none).
Death kinds: 0 a Journey run out of tries, 1 a Flood level sunk, 2 Survival sunk, 3 a Trick Shot level sunk (a missed trick shot is no game
over: the mode goes on to the next one). `state` is the screen on top of the stack: 0 a menu or a message (the main menu, the mode menu, the
continue prompt, loading, the options), 1 the world map (`level_select`), 2 a level in play (the tutorial, Survival and the trick shots
included), 3 the level won (its middle layer is in the victory stage), 4 the level lost (the defeat stage), 5 a summary (the level's, the
trick shot's, Survival's), 6 the honor roll, 7 the win scene, 8 the pause menu or the in-game menu over a level. `level` is the profile's
level index of the mode (0 on the first building: the profile moves on at the victory, so it changes with the level clear) and
`displayLevel` is `level + 1`, the number the map shows (for a trick shot, the trick shot's own). `displayScore` and `score` are the profile's
score. `tick` counts the updates of the scenes since the game started (4 ms each). `progress` is -1 (the ends of the categories are events).
`gameOwn` holds two int32: the level in play's time in milliseconds (its update count times 4; 0 with none) and the tries left of the mode
(Journey: 3 at the start of each level, -1 once the last is lost). The flags are RESUMED (a continued game: the continue prompt's "continue"
with progress in the mode), VANILLA (no score key) and PAUSED (state 8).

**Events** (for the current board; the rules of the board's time are the site session's):
- *GO*. Journey and Flood: the click away from the world map in a new game, when the progress of the mode is 0 (the first update with the
  `loading` scene above `level_select`: the click's stage lasts 10 ms). A new game's first map comes after the tutorial (the very first
  start of a player's game) or straight after the mode menu, so the click is the same either way, and a continued game (the continue
  prompt's "continue": progress above 0) has the RESUMED flag and no GO. "New game" of the prompt resets the progress and clears RESUMED
  before the click. Trick Shot: the board's first update in a new game (progress 0 and no trick shot made), or the update "new game" of
  the prompt resets it. Survival: the first update with a level in play.
- *Level clear*. Journey, Flood and Survival: the update the middle layer of the level in play enters its victory stage (`ProgressBar::
  CheckVictory`, the last plate gone; `win` of the test commands does the same). Trick Shot: the update the count of made trick shots rises
  (a missed shot's level also goes to its victory stage, which is no clear).
- *Game over*. The update the middle layer of the level in play enters its defeat stage (`Level::LoseTry`: a plate sank, or a Journey level
  with no try left), once for a board; a Journey try lost with tries left has no defeat stage and no game over.
- *Win*. Journey and Flood: the update the world map comes up with CONGRATULATIONS! on it, which is when the progress of the mode is the
  last building's (level index + 1 of 72 or more); every mode: the win scene is on the stack. After the last building (71) the map shows
  CONGRATULATIONS!, a click on it pushes the win scene, which resets the progress and opens the honor roll (a second click).

A board's first update is a baseline for the level clear, the game over and the win, and a level scene's first update is a baseline for its
own events (a level seen already won or lost). A game over is counted once for a board: the click that plays the level again runs the same
board on. The Trick Shot board's end (all trick shots made: the win scene; else the honor roll) and the Journey board's end after a game over
(the honor roll) are in `state` (7 and 6), for a splitter that wants them.

**Tests.** `games/venice/tools/livesplit/run_tests.py <out dir>` (under rundesk with `-fast`, one game at a time): a new player for each run
(made by the port with the standard test commands), the menus clicked at the places `buttons` gives, and the port's bot for the tutorial and
the levels: a new Journey game (the tutorial, the map, the GO at the click, building 1 won by `win` and then the tries lost: one GO, one
clear, one game over of kind 0, states 1 to 5 all seen), a new Flood game (the bot wins the first level itself, `lose` on the second: kind
1), Survival (`lose`: kind 2; `goto menu` ends the board), Trick Shot (the GO at the board's first write, the bot makes some of its shots:
clears only for the made ones), Journey from building 5 by "continue" (RESUMED, no GO) and by "new game" (RESUMED cleared before the GO, one
GO) and from building 72 (the map's CONGRATULATIONS!: one win, the win scene 7 and the honor roll 6). The block logs of these runs went to
the site session.

**Proof that it is inert.** `run_tests.py` also runs `games/venice/tools/tour_test.txt` (the tour that `tools/check.sh venice` freezes) twice,
with no mod and with a mods folder that holds only `livesplit` (the port's log says `livesplit: block at ...`), and plays the Journey, Flood,
Survival and Trick Shot games above twice, with the block on and with no mod, under the `trace` command (a line for each update: the random
seed and the values of the game's state). The tour's screens, state lines and player file, and each game's 3 MB trace (17,000 updates of the
Journey game) and player file, are the same byte for byte in the two runs. The tour goes through `goto` and `level`, which no mode click
precedes, so it never starts a board; the four games go through the menus, which is the observer's board logic. `tools/check.sh venice` runs
with no mods as before, and its frozen hashes are unchanged.

## Performance (2026-11-12)

**Result: the port's processor time is about half the original's over the same stretch of play, in both window sizes.** Median of five
alternating pairs (`games/venice/tools/profile/pairs.sh`, under `tools/pincore.py`), the process's cycles (every thread, the ones that
ended too) over 4000 updates of Journey in play, in million cycles an update and as a share of one core (the cycle counter ran at 2092 MHz):

| | runs | update | draw | present | driver threads | total | share of a core |
|---|---|---|---|---|---|---|---|
| original (origrun, real clock, software display, window 800x600) | 5 | 0.056 | 4.255 | 0.576 | 0 | **4.918** | 58.8 % |
| port, window of the game's own size (`-windowscale=1`) | 5 | 0.036 | 0.884 | 0.077 | 1.567 | **2.606** | 31.1 % |
| port, default window (2x, 1600x1200) | 5 | 0.035 | 0.800 | 0.075 | 1.424 | **2.370** | 28.3 % |

The runs' totals were 4.83 4.72 5.01 4.92 5.32 (original), 4.07 4.23 2.53 2.61 2.34 (port 1x) and 2.11 2.37 2.22 4.40 2.39 (port 2x): every
port run is below every original run, and the port's two runs near 4.1 and 4.4 are the driver thread's noise (below). The port's display line
says `client 800x600 px, logical 800x600, render scale 1, windowed, renderer direct3d11` (2x: `1600x1200 ... render scale 2`); both
sides ran in a window with the registry's `ScreenMode` 0 and the original's and the port's software display (`Is3D` 0, which is what the
original runs in on rundesk's desktop). The port's 3D display (the registry's `Is3D` left at its default), three runs, window 1x:
**0.749** M cycles an update, 9.0 % of a core (0.737 1.088 0.749); the original's 3D display is not measured, since origrun runs it
through dgVoodoo2, whose own cycles are not the original's.

**How it was measured** (`docs/COORDINATION.md`, "A performance pass"). The stretch is `scenario.py`: a player with every mode unlocked,
Journey from building 10, Play Now, Journey and Continue clicked, then the shots and clicks of the port's bot, which ran once (`profile.py
prep`) to make the input and is not in a measured run; both games are given the same lines, at the same frame of the game's own count
(the updates since the loading ended: the port's `after <frame>`, the original's update counter at +0x484 of its app, counted from the
first update its tick counter at 0xa2a1ac leaves 0). The stretch is updates 2500 to 6500 of that input, label a to label b. The player's
exe carries only the script's lines: no `-commands`, no state log, no bot. Counting is the shared one: `-cyclesplit` in the port (SexyApp's
three sites), origrun's `--count` in the original with `games/venice/tools/profile/count.txt` (DoUpdateFrames 0x482d70, DrawDirtyStuff
0x48bd70 less its Redraw, Redraw 0x497360; their prologue lengths checked by `tools/origrun/prologue.py`; the update counter at +0x484
equals the update function's calls). Each run ran under pincore's core 14, the original through origrun (its game under rundesk), the port
under rundesk, the order of the port's two windows swapped every other round, each run started when `tools/gamecount.py --limit 6` had
room. Counting costs: the port 11836 readings of 1635 cycles each (0.27 % of its total), the original 23476 readings of 3571 cycles each
(0.37 %); neither changes a result of this size. Sound: the port's audio thread (SDL's, silent under rundesk) is 0.005 M cycles an
update; origrun's stand-in DirectSound mixes nothing, and the original's one other busy thread (a thread of its exe, 0.08 % of a core) is
counted in its total, so neither side's number holds a mixer's synthesis beyond that.

**What the numbers say.** Both sides run 250 updates a second (4 ms). The original draws and presents on every update (3894 draws in the
4000 updates: its registry here has `VSyncUpdates` 0 and `WaitForVSync` 1 with a 60 Hz refresh, so it neither waits for a vertical blank nor
skips a draw), 4.26 M cycles a draw in its own software blits. The port draws once a display frame (958 draws, 59.9 a second, the
framework's vsync present), 3.4 M cycles a draw in its software transforms, and its GPU driver's thread (nvwgf2um) spends another 1.4 to 1.6
M an update (about 6 M a present) on uploading the picture and the wait for the blank, which is 55 to 60 % of the port's total: it is the
framework's known shared cost ("Presenting in step with the display", `docs/COORDINATION.md`), not a cost of Venice's code, and no
change in `games/venice/` moves it. The game's own update is 0.036 M against the original's 0.056 M. Caveats: the same input at the same
frame can land a tick or two apart in two real-time runs (the 4 ms updates follow each run's clock), so the shots the bot aimed need not
all hit again; the number of updates and draws is the same in every run, and the cost of a stretch of play of this kind does not depend
on whether a shot hit. This is Journey only: Flood, Survival and Trick Shot draw the same scenes, and the menus are cheaper.

**Changes made for performance:** none. Nothing in the port changed in this pass (the frozen baseline is as it was); the port is already
below the original with the framework's fast paths, so the pass stands as a measurement. **Lite candidates:** none added (the port
already draws only when the display does). **Shared cost sent on:** the driver thread's present cost above (known to the coordinator).
To measure again: `python -I games/venice/tools/profile/profile.py prep build/perf`, then `python -I tools/pincore.py -- bash
games/venice/tools/profile/pairs.sh build/perf 5` and `python -I games/venice/tools/profile/profile.py sum build/perf` (the run folders
are deleted once this section holds their numbers).

## Unneeded code taken out (2026-11-12)

The twelve candidates of the coverage check above are out of the port, each a function that no play of the original reaches and
that no live code of the port calls: `LevelScroller` (its class, `Draw`, `OnCustomEvent` and the factory's `level_scroller`) with
`SceneObject::DrawHot` that only it called; `ShooterPath::Update` and the factory's `shooter_path` (the class stays as the base of
`ScrollPoint`, whose own `Update` is the one that runs; no level file names either object, and the two object files are in the
game's data only because the original's level editor lists them); `SceneObject`'s `Rewind`, `Rotate`, `GetLocalRotation`,
`SetImageFrom`, `CircleOverlapWith`, `CountStagesBefore` and `AngleTo`; and `Plate::TossSlots`. The tuning `toss_on_filled_plate`
stays: `Slot` reads it in two places that the runs do reach (it is 0 in the shipped tuning), so only the plate's `fade_out` branch
that threw the slots' jewels away when it is set is gone, and `Plate::Update` now returns from a fading plate as the original does
with the key at 0. 167 lines out of 9 files. The port's frozen baseline, its tour and the rest of `tools/check.sh venice` are as
before (the check ran after the removal), and the port builds natively and for the web.

## The PopCap Game Collection version (2026-11-12)

The port was made from the retail install (`Venice.exe`, the DRM wrapper with the game inside; the game carved out of it is
`re/venice/bin/VeniceGame.exe`), and Venice Deluxe is on the user's PopCap Game Collection disc (the group `VeniceDeluxeComponent`
among the 49 of its cabinet), so it is checked against that version (`docs/COORDINATION.md`, "Done means"). `unshield` (the user
allowed it on 2026-11-10) read the disc image's cabinet, which was mounted read-only and dismounted afterwards: 10 files, 36 MB, in the
main checkout's `re/_installers/VeniceDeluxeCollection` (git-ignored): `Venice.exe` (1,983,792 bytes, the game itself with no wrapper),
`main.pak`, `config.xml`, `bass.dll`, `developer_logo.png`, `license.txt`, `readme.html`, `properties/partner.xml` and its `.sig`, and a
`userdata/profiles.dat`.

**It is the same game.** `main.pak` (35,934,548 bytes), `config.xml`, `bass.dll` and `developer_logo.png` are byte for byte the retail
copy's, and so is `partner.xml`. The exe, set beside the carved game image (`games/bookworm/tools/collection/cmpexe.py`), was linked at the
same second (6 June 2007, 17:45:46): `.text` and `.data` are the same bytes, `.rdata` differs in four (a build stamp of eight digits in
the string `CAPPACPOPPOPCAPXXXXXXXXX27390487`, which the Collection's has as `...47340415`; nothing in the port reads it), `.rsrc` in one
resource of the sixteen (2,532 bytes in the retail game, 2,206 in the Collection's: a block of the wrapper's data, which the port never
reads) and 5,424 bytes follow the last section of the Collection's (a signature). Unlike BookWorm's and Astropop's, no code differs, so
no function needs a second look.

**The port on the Collection's files.** `games/venice/tools/stage_venice_assets.py` staged the Collection's folder as it does the retail
one: 863 files, the same 863 as the retail copy's staged set, `diff -r` finding nothing (so a run of the port on them is the run it has
on the retail files: the frozen baseline, the tour and every scenario read the same bytes). The port reads nothing from the exe, and the
web page's markers are `main.pak` and `config.xml` (`games/venice/web/page.js`), both in the Collection's folder, so a player who drops
the Collection's folder on the site gets the game.

**Saves.** The Collection's install carries a `userdata/profiles.dat` of its own (52,284 bytes, the size of this machine's profiles file:
the format is a fixed record). The original (with `VistaCompliant` true in `partner.xml`, in both copies) keeps its players in
`C:/ProgramData/PopCap Games/Venice/userdata/profiles.dat`, where this machine's retail copy keeps them, and the same exe code does so for
the Collection's. The save-import table's row for Venice (`platform/saveimport.json`) names the file beside the exe; the folder under
ProgramData (`"appdata": "Venice"`) is where a player who ran the game has it, so the row may need that field (sent to the coordinator,
who owns the table).

## The page check (2026-11-12)

`games/venice/tools/web/pagecheck.js` is the game's table for the shared page check (`tools/web/pagecheck.js`; `docs/WEB_PLAYER.md`,
"The page check"): a player made by the new test command `player`, every building unlocked and Journey's building 5 started, the game in
play once its level scene is up. Opened as `venice.html?player=1&assets=1&hold=1&fast=1&silent=1&driver=games/venice/tools/web/pagecheck.js&run=pagecheck`
on `tools/webtest/serve.py` in the Browser pane (hidden, with the requestAnimationFrame shim), it passed: **11 checks pass, none fails,
1 is information.** The focus line matches the native build's answer (`tools/web/pagecheck_native.py`: only `timer` changes while the
page is hidden and after it is back, in both); the keys the game takes (Space, Backspace, and the arrows and Tab the shared check
adds) are kept from the browser and `'` and `/` open no quick find; the information line lists F5, F11, F12, F7, Ctrl+R, Ctrl+F and Ctrl+L as
cancelled and Alt as left; the pointer lands at 400,300 in canvases drawn at 1.5, 0.75, 2 and 1 times the game's size; a second copy
of the page waits for the first; and after a reload the two files the game keeps (`profiles.dat` and `registry.ini`) are back with the
same bytes, backed up while the game was in progress, with no error on the page before or after.
