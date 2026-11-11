# Mod roadmap

The rules for mods, the fixes mod every game gets, the user's notes on the mods that exist, and the mods planned for
every game. No new mod is made until the user asks for one, and the changes listed here wait until every
decompilation is finished (CLAUDE.md, "Port first, unmodified").

## Kinds of mod

- **Mods (the user, 2026-10-26):** code that makes a game moddable makes things configurable (rules, settings and
  their ranges in `mod.json` and the mod menu); it never just overwrites the original, which plays unchanged with
  no mod on. Every mod is data: a feature's code lives in the port, and a mod's `features` or `rules` turns it on.
  So each feature the port adds also ships a mod that turns it on. There are three kinds of mod:
  - An **example** mod has `example` in its name (`example-flame-gems`, `example-twist`). It shows how a single
    feature can be configured, for playing with its settings and finding which features work well together. It
    starts off (`"defaultEnabled": false`).
  - A **feature** mod gives the player one feature under its own name (BookWorm's `widescreen` or `undo`). Its
    settings are in the mod menu.
  - A **curated** mod gives the player a tuned experience, much like a fan game: features, settings and art
    chosen to play well together. It never copies a game mode from another game (the user, 2026-10-26): it always
    has a twist of its own that makes a player want to pick it over the game it draws on. A port may implement
    features from other games so that they can be recombined in new ways, and it is fine if a player can combine them
    into something close to another game's mode; a curated mod itself is never just that mode. Bejeweled 2's
    Twilight alone in Bejeweled is not interesting, but with other tunables it can be.

Bejeweled's own mod ideas (board shapes, gravity, special gems, replays, leaderboards and the rest) are designed stage
by stage in `games/bejeweled/docs/CODE_MODS_DESIGN.md`. One idea has no stage: a Sweet Tooth look (maybe), Bejeweled
restyled to look like Sweet Tooth To Go, PopCap's candy match-three made for Pogo, which the user decided on 2026-10-25
not to port, though they may want this look instead. Images only; like the Steven Universe gems, its art would come
from a local generator and stay git-ignored, since it would recreate another game's look. Not yet planned.

## The fixes mod

The user, 2026-11-04: "I think we'll want the default way to serve games on the site be to include a bunch of mods
making changes that fix bugs in the original or make it more performant (without doing things like Bejeweled's Lean
mod) by default. The user can turn them off for a more 'true' experience. That way these don't have to be
'deviations' and can instead be features."

Then: "I think it would make the most sense to have it as a single mod with each fix/improvement being a toggle in
that mod, actually." And: "Before a decomp can be considered done, such a mod should be made for it. So next week
will have a bunch of that in addition to finishing phase 2 of identical play. One goal of the mod should be for the
game to play exactly the same way. Memory issues stay part of the decomp rather than the default mod." And: "Same with
each run, I mean, so no 15-update variance on Twist."

So each game gets one **fixes** mod, a fourth kind, and each of its settings is a toggle for one fix. A fix mends a
bug of the original, or stops the original wasting the player's time or the machine's, and otherwise leaves the game
as it was: its look, rules, sounds and pace of play. Slow loading is not one of them: it is mended in the port itself
(the user, 2026-11-07: "Let's make faster loading a core part of the recomps instead of a mod"; CLAUDE.md, "Port
first, unmodified"). A mod that trades the original's look or feel for speed or memory, like Bejeweled's `lean` with
its lower frame rate, is not a fix: it stays a mod of its own and starts off as before.

The site starts each game with its fixes mod on. A fix starts on, except where the original's behaviour looks
intended and is not widely known as a bug: such a fix starts off (`"default": false`), so the game does not look
changed to someone who knows it (the user, 2026-11-11, of Bejeweled 2's podFade: "Off by default for functionality
that seems intended but where the bug is not well known to be a bug"). The player can turn single fixes on or off in
the mod menu, or the whole mod off to play the original as it was. The port itself stays the unmodified original
(CLAUDE.md, "Port first, unmodified", "Done means identical").

With the mod on, a game plays exactly the same way on every run: with the same seed and the same input, the same
series of games, update for update. Nothing from outside the game's own updates (the machine's speed, load times, the
heap, memory never set) changes play, as it does in Twist's original, whose autoplay moves 15 updates early or late
from one run to the next (games/bejeweledtwist/docs/IDENTICAL_PLAY.md, "Oddities"). Memory faults are not fixes for
the mod: the port fixes every leak of the original itself.

Bugs that break play are not fixes for the mod either (the user, 2026-11-07, of Seven Seas' dead squares: "That sort
of bug shouldn't be part of a fix mod, IMO"; asked which bugs, they chose "bugs that break play"). Controls that stop
working, a soft lock, a crash or a lost save are fixed in the port itself, mod or no mod, and listed with the game's
deviations. The fixes mod keeps the bugs that change how a game plays (its scores, timing or rules) and the waits that
only cost time.

A game's fixes mod is part of its decompilation: the game is not done until the mod is made. A port that loads no
mods yet (the `BYOPOP_WEB_NO_MODS` list in `cmake/ByopopWeb.cmake`) learns to load them first. Until then, a session
that finds a fix keeps the original's behaviour in the port and lists the fix in its game's notes under "Fix
candidates": what the original does, how to see it, and what the fix would change.

## The user's notes on the mods that exist

From the user's test notes, moved here in their own words when the notes were cleared (2026-11-04). Like every mod,
they wait until every decompilation is finished (`docs/COORDINATION.md`, "Mods").

The mod system:

- "We should classify mods that overwrite a gamemode separately from ones that just change bits of gameplay. Mostly
  relevant to Bejeweled currently but any game should support it in theory."
- "Mod menu should have a button to quickly copy a list of mods and versions installed and their hash. Useful for
  debugging."

Bejeweled:

- "Bejeweled Arrowhead mod seems to leave a gap when the arrowhead goes off screen, at least going down."
- "In Halo, Hints should probably indicate where to twist and which direction instead of highlighting gems. Maybe
  highlight both? It was very hard for me to spot."
- "Also, would be nice to enable all four (later on, more) gamemodes at once, then being able to select one of them to
  play through the in-game menu, rather than enabling only one in the mod menu and restarting."
- "Bejeweled drawn graphics 'CLICK HERE TO PLAY AGAIN' text is cut off so it says LICK and AGAI"
- "I think Arrowhead would be more fun if instead of just seemingly scoring more points with each subsequent one
  matched, you get points based on how many turns it spends on-screen. It could be exponential with rounding. Also
  would be neat if arrow gems exploded when they collided with each other. Maybe they would explode but not give
  points (although the destroyed gems could still give points). They could have a move counter on them. Direction
  changes could also be what increases their score."
- "I shouldn't lose my power gems when leveling up. It's ok if they change color, though."
- "The Bejeweled 2 music mod should use any of the tracks that can appear during gameplay and randomly select one."
- "Another mode to consider with arrow gems: collector (maybe think of a better name?) where you have a big board with
  few colors and the goal is to keep arrow gems from leaving the board. They could start randomly distributed near the
  middle. Maybe make them immune to being matched?"

BookWorm:

- "Bookworm word type mod should not repeat the sound every frame while a key for a letter that can't be played is
  held down. It should also turn off highlighting every tile matching the letter typed once narrowed down to a single
  sequence on the board."
- "It seems like Collins definitions aren't appearing. 'SPIRALS' showed nothing. I had BWA2 words, right click closes
  balloons, Collins words, greedy bots, replays, rude words, definitions while searching, word solver, type a word,
  undo, widescreen, and word journal all enabled when this happened."
- "Bookworm type words mod should have a key to open/close the menu, a key to scramble the board, in-game tab
  navigation, tab to cycle through different ways to play a single typed word. Should also have an option to disable
  typing if the current selection isn't a prefix of any word (off by default, since it may be sort of cheaty). Type
  words should allow typing the U after Q to be skipped."
- "Experiment with having the Bookworm solver try to facilitate the construction of the bonus word, potentially
  allowing burning tiles to fall to make it possible if that improves score."
- "If rude words are enabled, solver should have an option to show rude words on the board separately (option turned
  off by default)."
- "Should be a mod to disable the Bookworm level up screen and let you just play continuously, showing the stats
  elsewhere on the board at all times (probably the sidebar for widescreen)."
- "Find a current-ish APK for Boggle With Friends and extract the wordlist. Use it to create another dictionary
  expanding mod for Bookworm. I also want to know how many new words it would add compared to base game, base + BWA2,
  base + Collins, and all of them together." Getting the APK is a download, so it needs the user's OK first, with its
  name, source and size.
- "Bookworm solver mod shows '(when the tiles rest)' in the pause menu." (2026-11-07)

Big Money:

- "Big Money: does a puzzle bot perform better if it tries to only click coin groups with an even number of coins
  (since those could be groups from the original build)? Solver should stop thinking once level is unwinnable."
- "Big Money pickpockets mod should have a toggleable option to stop the collection of extra bags in puzzle early if
  no more bags can be obtained, using the solver." (2026-11-07)
- "Big Money solver should have an option to show if a move would cause a money bag to be impossible to drop. Solver
  should also show if there is no possible sequence of moves that can cause the level to be lost." (2026-11-07)

Bookworm Adventures:

- "Sound effect volumes vary a lot: at a volume where most effects are normal, the cheer after beating a chapter hurts
  the ears. Normalization would be welcome (if the original's sounds really differ that much, this is an option for
  the mod phase; first check that the port plays each sound at the original's volume)." The pass found every sound at
  the original's volume and fixed the chapter's cheer (games/bookwormadv/docs/FIXES_FOR_VOLUME2.md), so normalisation
  is left as a mod idea.

## Standard mods for every game

The user approved this list on 2026-10-26 as the mod plan of every game. In a game's mod phase (CLAUDE.md, "Order of
the work") its session goes through the list, makes each mod that fits the game, and says in the game's README why it
skipped any. Games that already have mods (Bejeweled, Big Money, BookWorm, Noah's Ark) make none of these until the
user says every decomp runs in the browser to their satisfaction (2026-10-26). Each is a feature mod (or an example
mod for a single setting), under the mod rules above. Where a game already has one, it is named in brackets.

**Moved to the site (the user, 2026-11-07: "Yes, move them").** Parts of items 8, 14, 15, 17 and 27 work the same in
every game, from the input the page passes on, the canvas it draws on or the framework's clock, so they become site
settings that the site session builds, off by default (the default must not feel modded, `docs/WEB_PLAYER.md`). Only
the parts that need a game's own code stay mods here; each item says which. The site also records clips of play, for
every game.

The user's first six:

1. LiveSplit: an autosplitter for the desktop builds, from a tagged memory block and a script LiveSplit runs (Bejeweled,
   Big Money, BookWorm).
2. Widescreen: 16:9, with the session deciding what goes in the extra space (BookWorm).
3. High-res textures: an upscaled image pack generated locally from the player's own files, never committed (Bejeweled).
4. Replays: a seed plus the input of each update, played back through the same update loop (Bejeweled, Big Money,
   BookWorm).
5. Seeded RNG: the seed shown at game over and settable from the options, the command line or the URL.
6. Bot scoring: a bot that plays games without a window and scores them, for tuning curated mods, where the game suits it.

Speedrunning and competition:

7. Replay checker: a saved replay replayed without a window to confirm its score, so scores can be shared as files
   without online features.
8. Timer and input display on screen, for speedruns in the browser, where LiveSplit cannot read the game. The input
   display and a run timer are site settings now; splits at the game's own milestones stay a mod.
9. Daily board: the same seed for everyone on a date, with its own high scores (BookWorm, Noah's Ark).

Practice and help:

10. Undo, in turn-based games (Bejeweled, BookWorm, Noah's Ark).
11. Hints or a solver (BookWorm's word solver, Noah's Ark's move helper, Big Money's puzzle solver), and a bot that
    plays the game on screen (the user, 2026-11-07), all from the bot behind the `move` test command that the test
    drivers use (`docs/TEST_COMMANDS.md`, "The bot behind `move`").
12. Level select: start at any level or stage, with separate scores.
13. Game stats: the game's numbers at game over, and lifetime totals (Bejeweled's stage 17, BookWorm's word journal).
14. Speed control: a slower or faster game clock. A site setting now, through the framework's clock, for every port
    that uses it.

Accessibility:

15. Colour-blind help: symbols or other palettes for anything the game tells apart only by colour. Palettes are a site
    filter over the canvas now; symbols stay a mod.
16. Reduced motion: screen shake, flashes and heavy particle bursts turned off.
17. Other controls: keyboard-only or gamepad play for mouse games, touch for phone browsers, remappable keys. Site
    settings now, translated by the page before the game sees the input; a mod only where a game needs controls of its
    own.
18. Larger text and a high-contrast interface.

Rules and content:

19. Rule settings: timers, scoring, lives and the difficulty curve as settings in the mod menu; every curated mod builds
    on them.
20. A relaxed variant: no timer, or no game over.
21. Custom levels: a documented level format, so a mod can add levels, in level-based games.
22. Randomizer: a shuffled level order or generated levels, in level-based games.
23. Developer leftovers: cheats, debug keys and unused content found in the exe, as an example mod.
24. Translations: a table that replaces the game's texts by id, so a mod can carry a language. Translations of a game's
    texts are derived from them, so they are made locally and never committed.
25. Local achievements, for games whose versions had Steam achievements, shown and saved locally (their names read from
    the player's exe like other game texts).
26. Two players taking turns or competing, where the game's rules allow it.

Presentation:

27. Pixel-perfect scaling and an optional scanline filter, for the 640x480 games. Both are site settings:
    pixel-perfect since 3bd48ae1, the scanline filter to come.
28. Smooth frames on high-refresh screens: frames drawn between updates without changing the update clock, so replays
    stay exact.
29. Jukebox: the player chooses the music, or the game's other tracks play on other levels. The site's soundtrack
    player covers listening outside the game; this mod is what the game plays during play.

## Nintendo 3DS port (after Bejeweled works fully with mods)

Attempt a native 3DS build of Bejeweled; the goal is to get it running in an emulator. First research how
native 3DS ports have been done before (for example the Super Mario 64 decompilation's 3DS port, and other
homebrew ports built with devkitPro's devkitARM / libctru / citro3d, or SDL2's 3DS backend), then pick the
approach. Known constraints to plan for: the 640x480 game against 400x240 (top) + 320x240 (bottom touch)
screens, 64 MB RAM on the original 3DS (more on the New 3DS), touch instead of mouse, GPU and audio APIs
different from SDL's desktop backends, and asset size/format limits. The SDL-based platform layer and the
game-agnostic framework port are the parts to reuse.

<!-- private -->
## Very far out: an original match-3 game

From the user's test notes (moved here 2026-11-04), in their words. It comes after every decompilation, the
third-party ones too.

"Add gems that work like fruit, hypercube, and supernova, so more gamemodes can be made with them. Also add sliding
support like Chuzzle and time bombs, doom gems, and coal from Twist. New ideas for gems too: a detonator gem that sets
off all power gems, or a gem that can only be set off by a detonator. Instead of just points, coal could spawn a flame
gem or blow up. So many ways you can be creative. What about support for a board that can change shape over the course
of the game? Could also add support for bonuses from making speedy moves (like Blazing Speed in 3 or hyperspeed in
Twist). I am mostly interested in maximizing what can be done with this one title rather than making every Bejeweled
game the same; I'm kind of thinking of Bejeweled as a centerpiece of the whole project, showing what can be done with
modding now that the decomps exist."

"Crazy mod idea: a fan game built on Bejeweled 1 called 'Bejeweled 64' with Nintendo 64-style graphics and 64 game
modes to choose from."

"You could make Bejeweled into a roguelike with a goal like 'create an infinite cascade' or something."

"Why not diagonal matches or even arbitrary rules for what kind of matches are permissible? How about adding conveyors
that move with each match made? A gem could reorganize your board to make a huge cascade!"

"Actually, now that I think about it, some of this stuff sounds like great material for an original match 3 game with
incremental/upgrade progression elements. Actually, let's call all the stuff I just mentioned ideas for this new
original game I'm thinking about. Not necessarily based on the PopCap framework, but whatever is very lightweight and
would work on virtually any platform. This would be after all the decomps, even the third party ones."
<!-- /private -->

## Very far out: the Build-a-Lot series

Decompilations of the full Build-a-Lot series (HipSoft, not PopCap, so a different engine and no
SexyAppFramework to lean on; the general pipeline - unwrap, Ghidra, RTTI, translate onto an SDL port of
the engine - still applies). Installed in `A:\SteamLibrary\steamapps\common`: Build-a-lot Bundle - 4 in 1,
Build-a-lot -- The Elizabethan Era, Build-a-Lot- On Vacation, Build-a-Lot- Fairy Tales, Build-a-lot
Mysteries, Build-a-lot 10. Survey them (engine, protection, shared code between entries) before starting.
