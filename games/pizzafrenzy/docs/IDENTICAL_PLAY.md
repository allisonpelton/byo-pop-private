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

## Where it stands (2026-11-16)

Case play1 (150 moves, seed 1) matches through the first level. record.py times each input by the frame the bot's last
state reply gave (a held run spends a frame on each round of queries, which the sum of the waits left out), so a replay
plays the game the bot played. Case long1 (500 moves, seed 1: three levels of the first city, its bonus round and the
second city's start, 7742 frames) matches on every compared value, and the random state's 1634 values come in the same
order. Case mem1 (the memory mode, 300 moves, seed 1: two levels and the third's start, 4433 frames) and case conc1
(the concentration mode, 300 moves, seed 1, 5017 frames) match the same way. Case long2 (1200 moves, seed 2: the first
city, its bonus round and the second city to a lost level, where the game ends at frame 11552) matches on every compared
value to the game's end, and the random state's values come in the same order to frame 11507; after it the original's
score post dialog takes a state the port does not have (the deviation below). Next: longer memory and concentration
cases and the editor.
