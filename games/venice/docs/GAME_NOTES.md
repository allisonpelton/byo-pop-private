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
- **Candidates for taking out unneeded code: 12 functions.** No play of the original reaches these, and they are not removed
  now:
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
