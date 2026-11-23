# Pizza Frenzy: the identical-play pass

Phase one of the pass CLAUDE.md names: **plays identical**. The same input gives the same games in the original and the
port, by the game's state, not by pictures. Phase two, **looks identical**, follows in its own section once this one is
done.

The original is the game carved out of its wrapper (`re/pizzafrenzy/bin/PizzaFrenzyGame.exe`, image base 0x400000), run
only through `tools/origrun`, from a copy staged under `E:\byo-og\pf\game` by
`games/pizzafrenzy/tools/identical/setup.sh`, under rundesk, on origrun's virtual clock and with the scripted cursor.
`orig.sh` refuses to start when the game's registry key is there already and removes only the key its own run made, so
every run starts the way a first run does.

## How both sides are driven

**One input for both.** A case starts with the port's bot playing from a first launch (an empty user folder) through
`tools/playbymove`, held, for a number of moves; `record.py` turns its transcript into a port `-realinput` file
(`<update> move|click|type|key ...`), adding up the bot's waits. The port plays that file again with `-fast` and writes
its state log; `toorig.py` turns the same file into an origrun script (`at <update> ...`, delta 0: the first case showed
each input's effects on the same frames that way).

**One start and one step.** The game seeds the C library's `rand` with `time(NULL)` at start; a test patch puts the
run's seed there (origrun's `--seed`), and the port's `-seed=N` is the same `srand(N)`. The frame's seconds come from
the clock; a test patch makes them the period (33.333 ms) and counts the frames at 0x541a00 for origrun's `at`, `peek`
and `--watch`. The game links the multithreaded C library, which keeps `rand`'s state in the thread's data, so a third
patch mirrors the state to 0x541a04 after every `rand` and `srand`. All of them are in
`tools/origrun/patches/pizzafrenzy.txt` (git-ignored; `setup.sh` writes it); none touches anything but the seed, the
frame time and the mirror.

**Compared values.** origrun watches four expressions a run. The game object is the pointer at 0x532900: its state (the
vtable at +0x30), city and level; its stats (+0x120): score, elapsed time, pizzas delivered and ordered, happiness,
customers served, tip money, waves and combo; its mode (+0x110): the wave, spawn and delivery timers, the wave's
customers left, the orders begun and the delivery streak. The port writes the same names with `-statelog=<file>`
(`PizzaFrenzyApp::LogState`, floats as their bits). `cmp.py` compares every change of every value with its frame. The
random state is peeked at every frame instead (a watch can miss a value when several come close together), and
`randseq.py` checks that the port's values come in the original's order, each within two frames: a peek is taken where
the patched counter goes up, after the mode's update and before the scene's, so a frame's particle draws show one peek
later than its mode's.

`case.sh <name> <seed> <moves>` runs all of it; `INPUT=<file>` replays a recorded input instead of recording one.
The original has no test commands, so every case starts from a first launch and reaches its mode by clicks: the
bot's `botmode memory` or `botmode concentration` (`--first`, dropped from the recorded input) has it click that
mode's button on the main menu.

**A saved player.** The editor opens only for a player with five toppings, which takes four cities of play. A case can
start both sides from a saved player instead: `PLAYER_DIR=<user folder>` copies the folder to the bot and the port,
and `ini2reg.py` turns its `registry.ini` (the port's emulated registry, whose keys and values are the original's)
into a `.reg` that `orig.sh` imports (`REG=`) before the original starts. The original keeps the player's pizzas in
`userPizzas.xml` beside its exe, where the port keeps them in its user folder; `orig.sh` removes the file before each
run, so every run starts without pizzas, and the case compares the two files at its end.

**The held run and its replay.** The bot plays held (`-hold`: an update runs only when its driver lets it). Its input
is the framework's own record of the run (`-recordlive`, each input at the update that ran it), which record.py
makes the case's input; the transcript's frames, which record.py read before, put each input a little early, and the
replay's game ran two frames ahead of the bot's from the first launch's name dialog on. `follows.py` checks that the
replay is the bot's game, by the random state's values and their frames. With the live record they are the same on
the same frames until, in a long case, the bot's game makes a draw one frame before the replay's (seed 3: frame 9791
against 9792, in the second city's second level, with no input there and the same values compared before and after),
and the two part. The bot's own queries (state and move, comments in the replay) are the only thing the replay does
not have; which of them moves the draw was not found within the hour (an oddity of the test bot, not of the port). The
original agrees with the replay, so the comparisons hold, but a long case ends somewhere other than where the bot's
game went: long2's lost level, at frame 11552, and the second city's lost level in seed 3 (edit2, which matches the
original to the game's end) are the replay's. The editor's case starts from a saved player.

## Differences found and fixed

**The tip and the vehicles were updated twice a frame.** The original's constructors of the tip (0x42f050) and the
vehicles (0x430860) set bit value 4 of a node's mask (+0x74, 0x467d70), the listeners flag, so their parent's update
leaves them out: the map updates them itself (`TipPopup::updateTip`, `Van::updateVehicle`). The port had read the value
as the mouse bit (value 2), so the tip took no mouse and both were updated by their group and again by the map: their
particle streams emitted at twice the rate and drew twice the random numbers, and from the first delivery's tip on the
random state went apart (in the first long case, a customer then got a different house, and the port hung up an order
the original never had). The engine's node now keeps the listeners bit of the mask (`setListenersMasked`, a shared
change, read only by the signal build), and the tip and the vehicles set it.

**A kitchen let its van go after every trip.** A vehicle that has finished its trip calls its home's slot 0x1a8. For a
house (the police car's) that is 0x41c460, which lets the vehicle go; for a kitchen it is 0x492310, which does nothing,
so the kitchen keeps its vans and sends a parked one the next time. The port called the house's version for both, so a
kitchen made a new van for every order: a new van's emitter has a full credit and emits at once, where the original's
parked van carries the credit its last trip left, and the random state went apart again two frames later. The kitchen
now overrides the slot with nothing.

**The global score post's stage frame.** The original's loading makes the online score post's screen in a frame of its
own (the load bar at 0.8), which the port has no screen for; the port now keeps an empty frame there, so the loading
takes the original's frames.

**The prize blimp dropped its tips every frame.** The blimp waits a random time from a range at 0x53290c between its
drops; the port read the range as one nothing sets, but the exe's static initializer 0x4f87c0 sets it to one to one and
three quarter seconds. Over the middle of the screen the port's blimp dropped three balloon tips and drew seven random
numbers a frame where the original drops every second or so, and its flight held the level's end 86 frames past the
original's. The range is the original's now.

**The level's end title was left out.** The title that says the level is complete (the burst text, 0x42b800, made by the
level's end state 0x458490 and the bonus round's 0x45d1b0) was left out with its looks, but its letters each have a
"BurstFx" particle system that bursts when they leave, and those draw random numbers: from the title's leaving on, the
original's random state went apart from the port's. The title is translated now (`BurstText`), letters, stages, waves
and bursts; the stages that only a hold time reaches, which nothing sets, are left out.

**A hung-up wave cost happiness for every customer.** When the whole wave is hung up (in the memory mode, a pizza
taken to the wrong customer), the customer's popup is told to hang up (slot 0x160, 0x4532e0), which in the original
only closes it the way any popup closes (the closing state, vtable 0x500574); only the customer the pizza went to
enters the hang-up state, which tells the mode and costs ten happiness. The port put every customer into the hang-up
state, so each waiting customer cost ten more; the first memory case lost twenty where the original lost ten, and the
level's end bonus came out 250 short. A peek of the popups' states in the original showed it. The popup closes now.
Seen in passing: the chameleon's state (vtable 0x500638) takes clicks and picks in the original (slots 0x14 and 0x18
are the base's 0x451610), which the port had as not; it takes them now.

**The pending special order looked at the wrong list.** The mode's check for a special order still to come (0x456e50)
ends with the wave's orders not placed yet (the list at +0xc0); the port's last loop read the orders under way (+0xcc)
instead. In the second long case the original placed four orders at frame 9788 that the port held back, and the counts
of orders and customers served went apart from there. The loop reads the wave's orders now.

**The citizen award waited for a second click.** The award's setValue (0x42d630) gives it the mouse flag after its
picture is added (0x467d30 with 2, as the balloon tip's 0x42e110 does), so the scene's hit test finds the award itself
and hands it the click, whatever the game screen is hovering. The port left the flag out, so a click reached the award
only through the screen's hover, which was a kitchen's popup under the cursor there: the original took the award (500)
on the first click, the port on the next one, four frames later, and the random state went apart with its fly to the
tip jar. Peeks of the screen's hover and of the award's flags in the original showed it.

**A backspace erased two letters.** The original's window procedure (0x4bd3b0) sends a key's first press as the
key-down signal and returns; only a key held long enough to repeat sends the repeat signal. The port sent the repeat
signal for the first press too, and the edit box takes both (0x472920 and 0x472a70), so a backspace in the pizza's name
erased two letters: in the editor's case the original saved its name as the remains of the default name followed by
the typed one, the port as the typed one alone. The first press sends the key-down signal alone now.

## Oddities

**The particle warmup's frame.** The port starts the decorations' particle warmup (150 seconds, 24 draws) in frame 50,
during the city's postcards; the original makes the same draws in frame 51. The random state is the same from frame 51
on. The cause was not found within the hour: the original's start, flags, setup and the city setting all look the same
as the port's.

**One more state at the level's start.** The original passes through a state the port does not have within frame 251
(both are in the game from that frame on).

## Deviations left

The global score post is left out of the port (it is outbound), and two frames of play show it. The loading keeps the
post screen's stage as an empty frame, so the loading takes the original's frames. At the game's end the original opens
the post's dialog (a state of its own, until the player closes it), where the port goes to the menu at once: the
comparison of a case ends there, and the input after it lands on other screens on the two sides.

The window's 100 ms sound timer, which the original runs only while its window is dragged or sized, is left out (the
port is not told of the drag). A fade under way when a drag starts goes on when the port's frames go on, where the
original's goes on through the drag; the music and sounds themselves keep playing.

The concentration mode's lock is fixed in the port (a bug that breaks play). An event that changes the orders of the
customers who take a click (the Gossip's and the Clown's, 0x448cd0 and 0x449850) ends in the mode's orders-changed
(0x45b4f0), which sets the seconds the orders show for to the clickable customers' count times a random wait times
1/15. When it comes while a wave's orders still show, every customer is dimmed and none takes a click, so the seconds
become 0. The wave's update turns the orders over only while those seconds run (0x45b6e0), so the dimmed customers stay
dimmed, their waits never start, and the level never ends. The native real-input run of the concentration mode met it
in city 12's third level: the wave's two orders came at update 11136 with 1.3 seconds to show and were still dimmed,
at their full 30 seconds, 2.4 million updates later. The port keeps the seconds when no customer takes a click (the
event changed no order then); the random draw is made either way. A game that never meets it plays as the original's.

## Looks identical

**Pictures of the original.** The original draws partly into its back surface through locks and partly with
DirectDraw blits, and a window capture comes back flat (and its paint, while the game is held, crashes it in a draw
through a null surface). So a second patch list (`setup.sh` writes it beside the first, `-shots.txt`) has the display's
present, after its own unlock, lock the whole back surface again and copy it into a buffer it allocates once; origrun
reads the buffer (`--surface-screen`, with a byte that says a copy is under way as `--surface-gate`). The pictures are
taken only inside Windows Sandbox. `shots.sh <case> <seed> <shot list> <run>` plays a case's recorded input on both
sides and takes a picture at each frame the list names (`shots/look1.txt`: the launch, the menu and the name dialog,
the story pages, play, the level and city ends, the bonus round; `shots/fade.txt` and `shots/levelend.txt`, frame by
frame through the launch's fades and a level's end), and tools/shotdiff compares the two folders.

**The original never clears its screen** (a change of the shared renderer, f8ae2924). The display's lock hands out
the last frame's pixels and the game draws its dirty regions over them, so a translucent screen blends with the frame
before: the fades out of the splash and into the menu build up over their frames. The port's signal build filled each
redrawn tile with black first, so its fades went through black where the original's go through the screen before. It
draws over the last frame now; the menu's fade-in matches frame by frame.

**Slides ended a pixel short** (a shared change of the engine's interpolator). The original works its ease curves
(0x494710, 0x4947e0) and their blends (0x47fea0, 0x47fd30) on the x87, whose registers hold more than a float, and
rounds only where it stores a value. The port rounded every step to a float, so the bonus round's pizza slid in to
204.99997 and was drawn at 204, where a peek of the original shows 205.0 exactly: the left half of the bonus screen
(its title and pizza) stood one pixel left of the original's, and the name dialog's slide ended the same way. The
port now works in double between the stores the original makes (the curve's value is stored as a float in the
position's blend and used unrounded in the scalar's).

**Scaled pictures read other rows** (a shared change of the renderer). A picture drawn at a scale other than 1 (the
Pause button, 0.6 by 0.8, and every other scaled button) goes in the original through a stretch between two
whole-pixel rectangles (0x4ca3b0 and its siblings for the other operations, 0x4c4680 and its siblings): the screen
rectangle is the source's scaled about the pivot and truncated side by side, and an error term steps the source, so
screen row i reads source row i times the source's height over the screen's, rounded down. The port read the row
under each screen pixel's centre, the mapping it had from Word Harmony's renderer, and so drew an edge row of each
scaled button from a neighbouring source row. The signal build now steps as the original does; with the slides'
fix, look1's matching pictures went from 7 of 25 to 15.

**Two texts were missing.** The level start's topping buttons showed no level under their pictures, and the topping
book's line for a topping with no next upgrade was empty: both read the exe's texts through a named address
(`GT(ADDRESS_...)`), which `make_gametext.py` did not look for, so the table of texts the port reads from the exe left
them out. The generator now takes a name's address from its file.

**A scaled picture's pivot** (a shared change of the renderer). The memory mode's order popup showed each pizza's
topping up and to the left of the pizza's middle, where the original centres it (mem1's play_600). A topping's preview
(0x41dc60) is the cheese pizza with the topping drawn on it at 0.55 of its size, from the scaled copy's pivot. The
original's in-place scale (0x4becb0) ends by scaling the pivot with the picture (truncated); the port's kept the
full-size pivot, so the topping was drawn from a point half its full size up and left. The port's scale now scales
the pivot too. The other scaled copies (the topping book's and the new topping screen's coupons, a layout's scaled
pictures) take the same pivot as the original's.

**The blimp's sign was missing.** The prize blimp that ends a level with every order delivered carries a dot matrix
sign that scrolls a message with the player's name (string 0x69, in capitals, 0x46dc60); the port had left it out.
It is made as the original makes it (0x430d50: the text drawn once on a black picture in `dotMatrix.xml`, shown
through a clip window from 82 left of the blimp to 61 right of it, 17 above) and scrolled as 0x430b90 does, crossing
the window every eight seconds; conc1's frame 5000 matches. (The original also makes a balloon tip there and lets it go
at once; its constructor has no effect the port keeps, so it is left out.)

**Sound by state.** origrun logs the original's DirectSound buffers in every mode (each buffer's create, data hash,
volume, play and stop, on the virtual clock), and the port's `-soundlog=<file>` logs each voice's start and stop with
its frame and the sound's length. `tools/identical/sndcmp.py` lines the starts up by length (Word Harmony's
comparison, at the 33.83 ms an update the original's log shows here). Case edit2 (the editor from a saved player:
its screens, each tool, saving and the menu again, seed 3) is the sound case.

**The buttons made no sound** (a shared change of the engine). The original plays a short press sound at each button
press (and the list boxes theirs); the port played none. The signal build's button and list box (0x491760) load their
sounds through the application's slots (0x58, the archive's file, and 0x60, the voice), and the port's went to the
sound system with a loose file's path, which a game in an archive does not have. Both now go through the slots, and
edit2's 33 starts all match, none more than a third of an update off.

**The music faded out too fast.** The original fades a clip in and out on the frame's sound update (0x401d90, with
the frame's milliseconds). Its window also has a 100 ms timer that updates the sounds (0x4010b0), but it sets that
only at WM_ENTERSIZEMOVE and kills it at WM_EXITSIZEMOVE (0x4bd3b0): it keeps the sounds going while the window is
dragged, when the frames stop. The port ran the timer all the time, so every fade got an extra 100 ms each 100 ms and
ended at twice the speed: edit2's menu music stopped at frame 131, where the original's 0.2 s fade-out ends at 134.
The port has no modal loop to tell it of a drag, so it drops the timer; every stop now lands on the original's frame.
The original also logs a stop of a press sound just before it plays again, after the sound has ended (its buffer
rewound for the play); the port's log shows no stop for a voice that is no longer playing, and nothing is heard either
way.

**The play cases' sounds** (look1, mem1 and conc1, from the original's logs of their shot runs). The original's
updates are not evenly spaced on origrun's virtual clock: against a steady 33.83 ms its sounds wander up to about four
updates either way and back over a run, so the starts are compared in order (each side's starts sorted by update
and length, then lined up), which the clock does not touch. Two things were the port's:
- **-fast ran the sounds on the mixer's time.** A sound's voice is free again when its length has passed: on the
  engine's clock in a run that does not wait (`HostSound::sGameClock`, which Word Harmony sets and Pizza Frenzy did
  not), else the mixer's real time. The port read the mixer's, which a fast run outpaces, so a sound with few voices
  was still "playing" when the game played it again and the play was dropped: in look1 the wrong click (one voice)
  sounded 39 times where the original's sounded 51, and the blimp's tips (three voices) 27 where it had 55. A fast
  run now plays them on the engine's clock; a run in real time was not affected.
- **The menu's music started again after the name dialog.** The music player marks itself stopping only when it
  stops a voice (0x450670); the port marked it on every stop, also the first start's with no voice yet, so the mark
  stayed. When the new player's menu asked for the music that already played, the player took the mark to mean the
  clip was ending and started it again. It is marked as the original does now.

With both, the starts are the same sounds in the same order: look1 878 on each side, mem1 501, conc1 516. The only
places out of order are eight pick sounds (8 ms) in look1 and conc1, each sorted one place before or after a
neighbour 2 to 4 updates away, which is the clock's wander and not a sound added or lost.

**The menu's dialogs** (case dlg1, from a saved player, seed 3: the options and the replay box, the high scores of
each mode with the clear question answered No, the player list with the delete question answered No, the quit
question answered No; `shots/dlg1.txt`). Both sides run windowed (the case's registry preset sets `FullScreen` to 0:
the original's first launch is full screen, and its options box then shows the box ticked). Three things were the
port's:
- The options box showed "Version" with no number. The original's setup (0x4669d0) shows "Version" and its
  application's version text (slot 0, 0x4bdb20): the FileVersion of its exe's US English string table. The port
  gives that text.
- The score table drew a black background. The original's table (0x48f410) sets each colour with its alpha (0x477e80),
  and its fill (0x4bfc20) blends by the colour's alpha and the opacity; the layout gives the background an alpha of 0,
  so the original fills nothing. The signal build's draw state now keeps the colour's alpha (a shared change of the
  engine), the table sets its colours with it, and the fill takes it as an opacity.
- The sales had no commas. A score's text (0x418ab0) writes its numbers through 0x46dcb0, the user's locale's number
  form with no decimals (GetNumberFormatA); the port writes US English's, the thousands with commas between them.

12 of the 17 pictures now match. The other 5 are the high scores, where only the row of buttons under the board
differs: the original's button to the online scores is gone and the other two close the gap (the registration and
outbound rule). So the case's clicks on Done and Clear are at points both sides' buttons cover.

**Losing and regaining the focus** (case focus1: look1's input to frame 1000, the focus lost there and regained at
1100, seed 1, windowed; `shots/focus1.txt`). On both sides the play screen goes under its pause menu at the loss and
stays there after the focus comes back (pictures at 1, 10, 50 and 99 frames after each). The pictures differ only by
the toppings' order (the coupons, the kitchens' signs and an order card), explained above. Run full screen, the
original's update counter stopped at the loss and never moved again; its loop skips the update while its window is
minimized (0x4bdf20), which is the likely cause, not checked. The real-input pass covers full screen on the user's
desktop.

**Explained: the toppings' order.** The play shots show the two topping coupons in the other order from the original
in some runs, each kitchen's sign with the other topping (the small boxes over the kitchens), and in the concentration
mode the pizzas on the houses with the other topping. The kitchens' toppings come from maps keyed by the topping's
address; the port orders them by the
manifest's order (`ToppingOrder`), taking the addresses to rise as the manifest loads. Peeks of the original on one
input and seed show the coupons in either order from run to run, so the original's order is not fixed and the port's
is one of its outcomes; nothing else in the coupons differs.

**Oddity: some screens run a frame behind.** The launch's fade-in, the splash's second picture and the story pages
(their panel's slide and their typed text) are the original's of the frame before: each port picture matches the
original's taken one frame earlier, exactly (frame 45's splash and frame 6300's story text at 46 and 6301 in the
port), where play, the menu's fade and the level ends match on their own frames. Checked and the same as the export:
the transitions' update and start (0x468ee0, 0x468e90), the state change (0x4707f0), and the frame's order (the scene
connects to the update signal before the game does, 0x409220, and the signal calls its listeners in order). Finding
it would take peeking a story screen's transition in the original over its first frames, to see which update the
original gives it that the port does not. The level summary's buttons are the same (conc1's summary1: the original
shows its Save/Quit button at frame 2600, the port from 2601), and so is the memory bonus round's start (mem1's
bonus_start matches the port's frame 4051).

**Oddity: smooth draws are resampled differently.** Two faint differences in every play shot come from the pictures
drawn with the smooth type on (not snapped to whole pixels). The order popups' "$" price (PizzaPopup's price text,
centred, so at a half pixel) has two columns at the digits' right edges darker in the port, up to 123 in a channel
over about 18 pixels (mem1's level2_300, boxes at 663,145 and 663,470). The order card that scales and fades in over a
kitchen's truck differs by a few levels all over (up to 58 in a channel; mem1's play_300, boxes at 250,499 and
425,499). Both are on their own frames (port frames 296 to 304 are all further off), and moving the port's picture by
a pixel makes them worse, so the pictures stand in the same place. The cause is the resampling. The original sends a
smooth, unrotated draw through its whole-pixel stretch (0x4cab60 and its siblings, one per pixel operation, chosen in
0x4bf8e0). At a scale of 1 and a fractional position it widens the screen rectangle by a pixel and forward-splats each
source pixel over the four screen pixels it overlaps, by the fractional weights, into a buffer (0x4c47c0 and its
siblings), so the column past a glyph's edge gets only its share. At another scale it resamples in fixed point
(0x4c2d30), screen column i reading source column i times (width − 1) over the screen's width in 1/256ths, so the first
and last columns land on the source's. The port samples every smooth draw backwards, bilinear about each screen
pixel's centre, repeating the edge pixels. Matching it means translating those MMX loops for each pixel operation
(each a couple of hundred lines in the export) into the renderer, well over the hour; the differences are a few levels
on small parts of moving or fading pictures.

**Oddity: a taken tip's flight.** A tip the player takes flies to the tip jar as a picture splat, from the tip's
place to the jar's count in a straight line over the distance divided by the splat's speed. In conc1's play_1500 the
original's picture is about halfway between where the port's is at frames 1499 and 1500 (some 28 pixels a frame), and
no port frame matches. The tips stand at the same points in both (peeks of the map's tips at 1500: (212, 387) at scale
1 and (487, 262) appearing at 0.5, opacity 0, the same as the port's), so the start is the same. Finding it would take
peeking the splat in the original (its origin, destination, lifetime and time left) over the flight's frames.

**Oddity: the level end's sparkles.** When the level's end title leaves, each letter's burst (`BurstFx`: three streams
of `basicSparkle`, a JPEG with a greyscale mask) draws its particles at the same places in both, but the port's are
brighter: from the first frame of the burst (2125 in look1's case) about 2700 to 5000 pixels a frame are lighter by
about a hundred in each channel, none darker, until the particles die (2170). Checked and the same as the export: the
effect's definition and its reading (the start opacity of 3 clamped to 1 in the particle's update, 0x497f00), the
pool's pictures and its random pick (0x4944b0, 0x493d90), the letter's scale and leave (0x42ab80, 0x42afb0), the
particle's transform (0x497e40), the mask's lookup and reading, and the blend's arithmetic. What is left is the
original's draw of a scaled, translucent masked picture against the port's; it would take tracing one particle's draw
in the original (its graphics state and the blit it picks) beside the port's.

**The web build.** tools/web/replay.js plays a case's recorded input on the web page and takes the shot list's
pictures there, to compare with the native port's (`shots.sh` with `PORT_ONLY=1`). `replay_gen.py <case dir> <shot
list> <seed> [--user <folder>]` writes `replay_case.js`, which is the driver with the case put in front (not
committed). It opens as `pizzafrenzy.html?player=1&assets=1&hold=1&fast=1&drawevery=1&silent=1` with
`&driver=games/pizzafrenzy/tools/web/replay_case.js&run=<id>`, and the pictures land in build/webshots/<id>. The
page holds the game between updates and sends each input and takes each picture while the game waits. The first
runs had pictures one update late. On frames 1 to 90 of look1, every web picture that differed was the native
picture of the next update, and the still frames were the same. So the page's picture held before update N shows
what -realinput's picture shows before update N+1. The driver takes each picture one update sooner and sends the
input on its own update. (An earlier draft delayed the input by one update instead; that matched the dialogs but
left every animation a frame off.) With that, every case matches the native port exactly: look1 70 of 71, dlg1 17,
edit2 11, mem1 11, conc1 9 and focus1 9, plus the probe of look1's first 90 frames 89 of 90. The one picture in each
of the first two that does not match is update 1's, the first one the page holds, which cannot be taken a step
sooner. So the web build looks the same as the native port, and the native port's differences from the original
(above) are the web's too. focus1 loses and regains the focus through the page's blur and focus events, and its
pictures match the native window's. In the hidden Browser pane, the page runs at full speed only
when its animation frames are shimmed with message-channel callbacks before it loads. Timers are throttled there, and
the page sometimes stops at "Starting..." or "Running..." until its main loop is paused and resumed.

**Where the looks stand.** Every screen the cases reach has been compared with the original's: the launch and its
fades, the menu, the name dialog, the story pages, play in each mode, the level and city ends, the bonus rounds, the
editor, the menu's dialogs, and losing and regaining the focus. Sound by state was compared on the editor case and the
three play cases. The comparisons use no masks: every picture is compared whole, and each difference left is named
above. Those are the toppings' order (the original's own order varies), the high scores' button row (the removed
online-scores button), the screens a frame behind and the smooth draws' resampling (oddities), the taken tip's flight
and the level end's sparkles (oddities). The web build's pictures are the native port's on every case. Still open:
full screen against the original, which the real-input pass covers on the desktop.

## Real input

The bot's test runs, the script commands and the page's test hooks never pass through the path a player's input
takes, so this pass plays the game only through it, by drivers. The session wrote them and read their results; it
played nothing. The port's bot answers `move` with the next click or key a hand would make on the screen that is up
(`src/TestCommands.cpp`, README "Test commands"). Natively `tools/playbymove` sends each move as `-realinput` events
into SDL's own queue under rundesk (docs/REAL_INPUT.md). On the page, the test kit's `?driver=move` sends DOM events on
the canvas (the player-only page, silent, with the hidden pane's animation frames shimmed as for the looks). Each run
is a whole game from a first launch: the new player's name box (B, O, T and Done), the menu's mode button, the city's
postcard, the story pages (Escape), the level starts and their vans, every pick and delivery, the summaries, the
bonus rounds, and the game's end on the high scores.

| Run | Game | Result |
| --- | --- | --- |
| native `-fast`, windowed, the focus lost and regained at the start | speed, seed 4 | won: 15,099 moves, 174,762 updates, all 12 cities to day 68 and the game's end, 751,345 points |
| native `-fast`, full screen by Alt+Enter at the start | memory, seed 5 | won: 15,743 moves, 175,328 updates, 605,740 points |
| native `-fast`, full screen by Alt+Enter at the start | concentration, seed 6 | won: 17,121 moves, 225,062 updates, 879,425 points |
| native real time, windowed | speed, seed 11 | 1,500 moves to city 2's third level (8,104 updates), no error |
| native `-fast`, full screen, focus1's input | the focus lost in play and regained | the 9 pictures the same as focus1's windowed ones, which match the original's |
| web, `fast=1` | speed, memory and concentration | each played to the game's end (lost in city 2's third level: the page runs on between the driver's moves, about 90 updates a move), no error in the page |
| web, the page check (`tools/web/pagecheck.js`) | focus, keys, resize, a second copy, a reload | 11 pass, 0 fail, 1 information line |

No log has an error, and each native game's player is saved at its end with its furthest city and day and its place at
the top of the mode's high scores.

Findings. The concentration mode's first whole game locked in city 12's third level: two customers stayed dimmed for
2.4 million updates, with their orders never turned over, and the level could not end. The cause is the original's
own (0x45b4f0), and the port fixes it as a lock (under "Deviations left"); the next whole game of the mode was won. The
bot had no move on the box that offers a saved game, where the page's driver stopped (the page's profile kept a game
from an earlier run); it now presses the box's button that starts the mode again. The first script of the native runs
gave every game the same player folder (a `\$N` in a path that bash left unexpanded), so its games began in city 12
with a saved player; the runs above each had their own.

Full screen. When the original loses the focus in full screen, DirectDraw's exclusive mode minimizes its window (the
game itself only notes the minimize in its WM_SIZE, 0x274, which its loop reads to skip its updates), and its updates
stop until the player brings it back from the taskbar. In the sandbox the posted focus loss minimized it and the posted
regain did not restore it, as nothing there restores a window. The port's full screen is a window the size of the
desktop, which stays up while the focus is away; its pause menu shows, as the original's does once it is back. Both
pause the game at the loss (the clock stops), so what the player finds on coming back is the same.

Not covered: the web build in full screen (a browser grants it only to the player's own click), sound in play (the runs
are silent by rule; sound by state was compared in the looks), and a real-time web game (the hidden pane's timers ran
it at a few updates a second; the `fast=1` runs and the page check cover the page's input).

## Where it stands (2026-11-16)

Case play1 (150 moves, seed 1) matches through the first level. record.py times each input by the frame the bot's last
state reply gave (a held run spends a frame on each round of queries, which the sum of the waits left out), so a replay
plays the game the bot played. Case long1 (500 moves, seed 1: three levels of the first city, its bonus round and the
second city's start, 7742 frames) matches on every compared value, and the random state's 1634 values come in the same
order. Case mem1 (the memory mode, 300 moves, seed 1: two levels and the third's start, 4433 frames) and case conc1
(the concentration mode, 300 moves, seed 1, 5017 frames) match the same way. Case long2 (1200 moves, seed 2: the first
city, its bonus round and the second city to a lost level, where the game ends at frame 11552) matches on every compared
value to the game's end, and the random state's values come in the same order to frame 11507; after it the original's
score post dialog takes a state the port does not have (the deviation below). Case mem2 (the memory mode, 800 moves,
seed 2: the first city, its bonus round and three levels of the second, 12129 frames) and case conc2 (the
concentration mode, 800 moves, seed 2: the first city, its bonus round and the second city's third level, 13898
frames) match on every compared value, their random states' 2100 and 1949 values in the same order. Case edit1 (a
player with five toppings, from the bot's seed 3 game; `botmode editor` opens the editor from the menu, where the bot
places three toppings, names the pizza and leaves, and opens it again a few times) matches on every state, and the
two programs save the same pizza file.
