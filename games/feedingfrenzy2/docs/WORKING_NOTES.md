# Feeding Frenzy 2: working notes

The session's own notes while porting: the plan, where the work stands, and what the export taught that is not yet in
code. `GAME_NOTES.md` will hold the finished port's description.

## Sources

- The game's files: `python -I games/feedingfrenzy2/tools/stage_feedingfrenzy2_assets.py` copies `FF2.saf`, `music/`,
  `splash/` and `properties/` from the Steam copy into `assets/feedingfrenzy2/`.
- The exe: `re/feedingfrenzy2/bin/FeedingFrenzy2Game.exe` (main checkout, git-ignored), the game PE carved out of
  the Steam wrapper; linked 2006-02-24 with VS2005, full RTTI.
- The export: `re/feedingfrenzy2/gh/` (`all.c`, `functions.jsonl`, `vtables.jsonl`, `strings.jsonl`, `decomp/`).
- RTTI: `python -I tools/rtti_msvc70.py <exe> re/feedingfrenzy2/rtti/ff2 --functions re/feedingfrenzy2/gh/functions.jsonl`
  writes `ff2.classes.json` (581 classes, each with its bases and vtables). Talismania's, for the shared set, goes to
  `re/feedingfrenzy2/talismania/tal.classes.json`.

## Tools

- `tools/fn.py <addr>...`: a function, compacted (`--raw` as Ghidra wrote it, `--asm` disassembled by llvm-objdump,
  `--info` its callers, callees and strings, `--vt` a vtable read from the exe). Ghidra loses `this` in most
  `__EH_prolog3` functions (it shows up as `in_stack_00000004` or a shifted `param_1`), so read the disassembly
  wherever a body's first lines look wrong.
- `tools/classdump.py <class>`: a class's bases and vtables, each slot's function marked `=` where the primary base's
  own is kept, and the functions that store its vtables (constructors and destructors).

## The engine's scope

Sprout's second-generation engine goes in `engines/sprout2` (library `sprout2`), shared with Talismania. What is in
it: the 151 classes both games' RTTI names (namespaces `engine`, `dxdriver`, `simplesound`, `simplesounddx`, the
sigslot bases, and about 70 global classes such as `GenericApplication`, `Theme`, `ScreenLayout`, `SoundMgr`,
`BassMusicPlayer`, the particles, paths, splats and pixel combiners). What only Feeding Frenzy 2 has stays in
`games/feedingfrenzy2` (the `shape` namespace, `FadeContainer`, `MusicSystem`, `GenericOptionScreen`,
`GenericHighScoreScreen` among them) until Talismania's session finds it needs one.

## The object model

- `engine::Interface` is a virtual base with three slots (addRef, release, and a third that returns the count);
  `engine::Object` implements them over an interlocked count at +8 (a release to 0 calls the object's deleting
  destructor). An object is laid out as its own vtable at +0, the vbtable at +4, its fields, and the `Interface`
  part last.
- Every counted reference is added and released by hand in the original, through the vbtable.
- Ghidra's class recovery names many functions after the wrong class (`engine::Object::Object_Constructor_or_Destructor`
  at 0x438700 is the engine's frame), so a name in the export is a hint.

## The main loop

- `WinMain` 0x4a3960 makes the `dxdriver::EngineImpl` (0x4e8 bytes, 0x494e30) into the globals 0x5ac654 and 0x5ab9e4,
  runs 0x4383b0 and deletes it (slot 0x10c).
- 0x4383b0 is the engine's run: the window message (`Notify`), the single-instance mutex, the screensaver arguments
  (the strings at 0x56130c, 0x560564, 0x561320, 0x561324), the window, then a loop of PeekMessage and the frame
  (0x438700) whenever the clock (slot 0xe4) passes the next frame's time, `Sleep(0)` otherwise.
- The clock (0x438310) reads QueryPerformanceCounter, then switches to `rdtsc` (0x4c1e10) once calibrated; origrun
  cannot hook `rdtsc`, which matters for the identical-play runs.

## Startup

- The run (0x4383b0) reads the command line into the engine (+0x190), takes the game's name from the engine's slot 0,
  registers the "Notify" message and the single-instance mutex (a second copy posts Notify and leaves), and handles the
  screensaver letters `s` (run), `c` (configure), `p` and `l` (preview); a plain start is the `s` path without the
  screensaver's window style.
- 0x49bc60 makes the game's application (`FeedingFrenzyTwo`, 0x378 bytes, the factory 0x49e800) into the engine's
  +0x288 and has it fill the configuration at +0x1c0 (its slot 4, 0x48c080): the archive `FF2.saf` added with loose
  files off, the string table `resource/baseStrings.xml` (its `vendor` and `appName`), the options, the title
  "Feeding Frenzy 2 - Shipwreck Showdown", 60 frames a second and 800x600. A configuration without a name throws
  "Invalid configuration.". The application's slot 8 (0x408050) then starts the game.

## Images

- The surface loader (0x45f530) decodes `.jpg` with libjpeg 6b's defaults and `.png` with libpng; a file `_name.png`
  beside `name.jpg` or `name.png` is the alpha mask (slot 0x2c, 0x463580). Without a mask, a PNG whose colour type has
  an alpha channel is drawn blended (blend mode 2) and its colours premultiplied (0x4754b0, rounded by
  (t + (t >> 8)) >> 8 with t = c * a + 0x80); everything else is opaque (mode 0).
- The archive's PNG images are all 8-bit RGBA; its masks are grey (1, 8 and 16 bits), grey-valued RGB or palette, a
  palette with tRNS (`_infoframe`) and RGBA ones, whose alpha is the mask's alpha. libpng's rgb_to_gray weights
  (6968, 23434, 2366) give a grey pixel its own value, so its gamma tables never matter for the game's masks.

## Animations

- An actor's animation is `resource/actors/<anim>/animation.xml` (its root <animations fileName="...">, each
  <animation name startFrame endFrame frameRate>, the end the last frame shown) and the Theora file it names. That file
  is no Ogg stream: each packet stands behind a 28-byte little-endian header (length, first and last flags, granule
  position, number), the first three the stream's headers. The original links libtheora's old API (theora_decode_header
  0x47ba10, theora_decode_init 0x4873f0, packetin 0x46baf0, YUVout 0x4727e0); the port decodes with libtheora 1.1.1,
  Theora's decoding being exact.
- Each frame a run takes becomes a surface of the picture's size (0x45fd90): the luma from the coded frame's top left,
  the chroma interpolated between its four nearest samples in 256ths, stepping (n * 256 - 256) / (n * 2) per pixel
  over a plane n samples wide or high, and converted with the integer BT.601 weights (298, 409, -100, -208, 516, +128,
  >> 8, clamped). Its alpha is `_alpha.<frame>.png` beside the file, where there is one; an image the engine keeps
  under `<folder><frame>.png` replaces the frame (and its packet is then not decoded). The frames no run takes are
  decoded and dropped, and the runs after them moved down.
- A visual's data (0x47b650) scales each frame to its size over the model's (a scaled copy through the alpha
  blitter, filtered, truncated to whole pixels), lays the water's colour over it by its alpha (ColorFx mode 3,
  truncated) where the colour's alpha is above 0, and anchors it at its centre. Its shapes and points are the model's
  scaled and moved round that anchor, with "visual" the first frame's box; a fish's adds "halo" and "idea" from the
  body's box (0.15 and 0.57, 0.45 and 0.55 of its width and height up and left of its centre) and "tail". A fish out
  of the air drops its "idle" run before the frames load (the spawns always set fish up so); its "eat" run gives
  SuckIntro and SuckOutro at the model's suckFrame, and swimShuttle and idleShuttle cut the swim and idle runs.

## Drawing

- A frame (0x438700) builds the update context (elapsed seconds at most 0.33, the frame number, the detail level),
  emits the update signal, then draws into the display through a DXDrawContext whose first state is at the origin,
  unscaled, opaque and black, emits the draw signal and flips. The port copies the display to the framework's screen
  image, opaque.
- DXDrawContext's slots: 0x00 an image, 0x18 an outline (0x513cd5), 0x20 a fill (0x44ef30), 0x28 a line, 0x2c a
  circle, 0x30 each pixel darkened by its own alpha (over the clip region), 0x34 the target filled with the colour
  (the surface's slot 0x30). Fills, outlines, lines and circles add the state's position truncated (no scale), stop at
  the target's edges and ignore the clip region; a line or circle not wholly on the target is not drawn.
- Colours become bytes by truncating channel * 255 at double precision (the low byte kept). The colour blend copies an
  opaque colour and otherwise adds the colour to the target scaled by 255 - alpha (t = d * (255 - a) + 0x80,
  (t + (t >> 8)) >> 8, saturated); below full opacity every byte of the colour is first scaled by the opacity byte
  (0x48ca50 makes 255 into 0x101).
- An image: the position plus the state's, floored (0x43f520, the CRT's floor) in the snapped filter mode; nothing at
  opacity 0 or a scale of 0. Then by the colour mode (multiply 0x440910, pixel effect 0x418190, add 0x4609d0) or, with
  none, by the image's blend mode: 3 0x44cfd0, 2 at full opacity (|o - 1| < 2^-22) 0x441810, 2 or below full opacity
  0x442740, 1 0x539fb7, else 0x452450 (0x44fc40 when the image's slot 0x28 says 4). Each blitter runs once per
  rectangle of the clip region, or once over the target when the clip stack is empty.
- The clip stack (0x45fb60, used by MaskContainer and 0x52004a) holds rectangles that add to, intersect or subtract
  from the region the ones below leave; the region is rebuilt when the stack changes.
- Translated exactly so far: the dispatch, the unrotated geometry, the unscaled alpha blend at whole pixels (0x444ec0),
  the fills, outlines, lines and circles. The scaled, rotated, sub-pixel, multiply, add, pixel effect, colour key and
  mode 3 blitters are drawn by a plain sampler until the looks pass.
- Original bug (Oddities when it matters): the outline blitters' first blended pixel of each edge goes through a stale
  pointer (the top row's into the lock structure's width), so a translucent outline at full opacity would corrupt
  memory; the game is not seen drawing one. The circle may write one pixel past the target's right or bottom edge.

## Particles

- An effect's file (`<particleSystem>` of `<particleStream>`s) is read by ParticleSystemDocHandler into definitions;
  each stream's images are `path0.png` (or `.jpg`), `path1`, ... up to the first missing number. A stream fills a pool
  when the effect is made: `ftol(longest life * rate * min(1.5, auto-stop) + 1)` particles, at most its maximum. A
  free particle is one whose only reference is the pool's; an emission tries 25 at random (`rand() % size`) and gives
  up if none is free.
- Every emission draws, in order: acceleration, x, y, start scale, end scale (unless constant), life, velocity x and
  y, rotation, start opacity, end opacity (unless constant), jitter amplitude x and y, and when either is not 0 the
  jitter period y then x. Each is `rand() * (1/32767) * (high - low) + low`, so the splash's bubbles take draws from
  the game's sequence from the first frame; their count depends on the frame times (the rate times the elapsed time,
  with the fraction carried).
- Oddities kept: the rotation's "degrees" go to cos and sin as radians; jitter's `rate`, scale's `constantScale` and
  opacity's `constantOpacity` are not read (the files that give them get the defaults); a stream's computeBounds
  empties its particles' kill rectangle rather than its own bounds (nothing reaches it: the effect's computeBounds
  does not recurse); a group-relative stream passes an unset base to its emissions, which do not read it; opacity
  keys out of order leave the ease curve of the last pair used.
- A theme's `<particleEffect>` loads the effect's images at once and keeps a pool of copies (made `count` at a time,
  0 by default); a screen with a theme takes its effects from the pool. A copy is free again when only the pool holds
  it, and is then stopped and emptied before it is handed out.
- Not yet translated: animated particles (`sequence="anim"`, AnimImage; the particle is kept without its animation so
  the draws stay the same).

## Bubble text

- The splash's "LOADING" (BubbleText, a game class): a glyph for each letter that is not a space, each a bubble image,
  a TextItem letter centred on it in the theme's colour (multiplied) and an effect. Each new glyph gets a wave of its
  own (5 pixels up and down, a period of 2.5 to 3.5 s, run once for 0 to 2 s), which takes two draws from `rand()`
  per glyph before the first particle's. The glyphs grow in from scale 0 over 0.2 s one after another, the last
  starting 0.2 s before the duration ends; at the end each bursts into its effect.
- TextItem's wrap drops the character on which a word longer than the width breaks (kept; nothing in the splash
  wraps).

## The splash's phases

- Waiting (until the screen has settled and the timer, 0 at the start, is below 0), loading (30 frames a second; the
  bar follows the loader's progress by at most a hundredth a frame, and fills at once when the progress reaches 1),
  failed (the thread did not start: the program ends), loaded (the bar full, 60 frames a second, the bubbles burst,
  the stage container shown, the burst's sound), bursting (until every glyph's effect is done, then the next state).
- The original loads on a thread while the splash goes on drawing; the port loads at once (at the machine's speed),
  so the bar fills in a frame. A glyph still waiting to grow does not burst: with the loader empty, the burst comes
  about 0.3 s in, after "LOA", and "DING" grows in afterwards and stays. How long the original's loader takes (and
  so how much of "LOADING" has grown when it bursts) is to be measured once the loader is translated.

## Done

- `engines/sprout2`: objects and references, sigslot, streams, the archive and resource manager, the engine's
  file, XML, image and font slots, components and containers, the draw context's state stack and text, fonts,
  surfaces with their loader and masks, the XML parser and its parameters. `host/` has EngineImpl (the disk files,
  the log, the clock).
- `engines/sprout2`: the draw context's clip stack and region; DXDrawContext with its fills, outlines, lines, circles
  and images (the first-draft sampler where noted).
- `games/feedingfrenzy2/src`: the app skeleton (a window, the engine made, frames run on the clock, each drawn into
  the display and shown).
- `engines/sprout2` (shared with Talismania, whose exe has the same classes by RTTI): themes (images, fonts, colours,
  vectors; particle effects, sounds, music and animations logged), screen layouts read by ScreenDocHandler (images,
  groups, modifiers, wave effects, particle systems; Theora frames, scaled copies and composites logged), Image,
  FadeContainer, the wave modifier and effect, GenericScreen with its phases, Transition and FadeTransition, the
  Scene's update and draw (its mouse and activation not yet), State and StateMgr.
- `games/feedingfrenzy2/src`: FeedingFrenzyTwo (the archive of its configuration; of its start the scene, the two
  named containers, the screen manager and the splash state), ScreenMgr, SplashState's screen, load bar and theme.
  The splash screen fades in and shows its background, logos and empty load bar (`-fast`, a shot at update 60).
- `engines/sprout2`: particle effects (their files, ParticleSystem, ParticleStream, Particle) and the float key
  interpolator with its ease curve. The splash's three bubble effects run (`-fast`, a shot at update 180).
- `engines/sprout2`: the theme's particle effects and their pools, TextItem. `games/feedingfrenzy2/src`: BubbleText
  and BubbleGlyph. The splash's "LOADING" grows in letter by letter and rocks (`-fast`, shots at updates 20 and 90).
- The splash's phases and the engine's frame rate (the loader empty, the sounds and the next state not yet).
- The loader's first part: GameData with the game's and the menu's stage manifests (stages and level sets; the
  level sets' splines and the map pins not yet, being the game map's). The engine's log is now SDL's info level, so
  it shows on the console.
- The loader's next parts: the stages' screen data (the intro screens' texts and layouts by stage id), the shell's
  theme (a theme now falls back on another for its images, fonts and particle systems, not its colours, vectors or
  particle definitions), the fun facts and the player ranks (kept sorted by points). Checked by a temporary log: 100
  facts, 34 ranks, stage1's entry with its layouts and objective.
- The actors' file (resource/actors/fishes.xml) read into the fish factory: models with their shapes and anchors,
  fish models, visuals with their colour effects, behaviours (each keeping all its attributes) and fish behaviours
  with their reactions. Checked by a temporary log: every element counted as in the file, and the values of a few
  (Herring's speeds, sight cosine and default turn rate, the lure fish's anchors, queenTrigger's older mouth). A
  model a visual names that does not exist shows a message box in the original; the port logs it (the file has
  none). The shapes' tests are the game's, not yet translated.
- The main menu made by the loader (MainMenu, its actions passed to the application's state), with the engine's
  buttons (ButtonBase, ImageButton, Selector) and the layout's image buttons, texts and key actions. Checked by a
  temporary dump: the menu's eight image buttons and two texts at the high detail level, as the file has them. The
  button's mouse is written but not reached until the scene's mouse is translated; its sounds keep their names.
  - The text element compares its colour, read from 0 to 1, with 255, so every text gets the multiply mode with its
    colour (white without one): kept as the original's.
  - The link button (gameLinkBtn) is hidden and switched off, as the original does when the options name no link of
    a distributor's; it leads out of the game, so it is to be taken out with the registration pass.
  - The debug level select (the application's +0x2c, DebugSelectMenu) is left out: only the main menu's debugSelect
    text button, hidden and switched off, has been seen to lead to it. To be checked again with MenuState's actions
    and keys.
- The scene's mouse and activation (the target under the mouse or the capture, enter and leave, the press's cancel
  when the window goes inactive) and the port's widget turning the framework's mouse, keys and focus into the
  engine's signals, as the window procedure does (no double clicks: the original's window class has none). Not yet
  exercised: no screen with buttons is shown until the menu state is translated.
- The loader's plain shell screens (pause, name entry, the four confirmations of the players' list, the quit's,
  the game exit's and the high scores' reset, restore, generic load, credits), each with its fades and its actions
  passed to the application, in the original's order with its progress steps; the engine's combo button (an image
  button with a text in a font and colour for each state).
  - The combo button's setFonts sets the three colours to the byte value 1 (nearly black) before the reader sets
    them: kept.
  - The application's sound manager (+0x34) is made by the loader and read by nothing the export shows: left out.
- The configuration's string table (resource/baseStrings.xml), the vendor and the name; the players' data as made
  (no player); the menu state: the main menu shown with the player's name, the welcome for a new player and the
  trophies hidden, its credits and back actions, suspend and resume for popups; the state machine's push and pop.
  Checked by screenshots: the splash hides and the main menu fades in over it.
  - The loader finishes in a frame, so the splash's burst comes while the last letters of its LOADING still wait to
    appear: a waiting letter does not burst and the bubble text counts as done, so they grow in as the splash fades
    out, as the original's would on a machine that loads that fast. With no menu stage yet, the frame is not
    painted over and the splash's last frame stays under the menu.
- The players: the engine's players' list (UserManager, UserInfo) over the engine's persistent keys, which the
  port's host keeps in the framework's emulated registry under Software, GameHouse, Feeding Frenzy 2, users; the
  players' data's load and save with their MD5 signatures (the game's own starting words, base64); the popup state
  (darkening the game's screens to the theme's popupDarkenColor over the screen's transition, its own screen manager)
  with the name entry's actions; the engine's edit box (typing, the filter, glyphs, Backspace, Delete, Home, End,
  the arrows, Ctrl+V) and its reader; the time driver and the colour key interpolator. Checked on a fresh folder: the
  name entry comes up over the darkened menu, "A l x Backspace / e Left y" gives Alye (the slash refused), Return
  adds the player, the popup fades out and the menu welcomes Alye; the saved signatures match an independent MD5
  with the game's starting words; a second run loads Alye with no reset and no popup.
  - Deviations: the players are sorted by an ASCII case-blind compare, not CompareStringA; the registry's root is
    ignored (one tree); the framework's own settings are not written to the registry; a key with nothing below it is
    kept by an empty default value; the edit box's paste uses the framework's clipboard and key state (the original
    asks Windows for Ctrl's state now; posted input reaches neither); characters from 0x80 are refused, as the
    original's _ismbcprint refuses them on a single-byte code page.
  - Fix candidates: the paste leaves the clipboard open when the box is full, and never frees its copy of the text
    (the port frees it, a leak).
  - Not yet compared with the original: the popup's look and timing, and the signatures of a player the original
    made (to do through origrun).
  - The popup's soft shadow piles up into a black frame, and the popup's ghost stays after it closes: nothing
    repaints the background until the stage is translated (see the looks item below).
- The players' select screen: the engine's list box (rows of a list model, the mouse's highlight and click, the
  selected row's band) and its reader, the players' list model, the select screen (GenericUserSelectScreen: its init
  from the menu's switchUser, New greyed when the list is full, Delete and Select when it is empty), the players'
  removal from the key and the list, the edit box's setText, and the popup's actions userSelectDone, userSelectNew,
  userSelectDelete, deleteUserYes and deleteUserNo with the name entry's reset (Cancel there only while there are
  players). Checked under rundesk with posted clicks on a copy of the Alye folder: Switch User shows Alye selected;
  New, "Bob" and Return add Bob, selected, and the list comes back; a click on Alye and Select make Alye the active
  player in the registry; a second run deletes Bob through the confirmation (which names him) and leaves Alye alone
  and active.
  - Not yet translated: the list box's over and click sounds (with the sounds).
  - Possible oddities, to compare with the original: New counts one row fewer unless a player after the first has
    the strings' alienUser name, so the alien at the first row does not count; deleting a player who is not the last
    leaves the selection on the next player, not the current one; while the list is up after a new player, the
    menu's Welcome still names the one before (the original may refresh it only when the popup closes); the selected
    row's text is drawn in its colour multiplied over the band, white on the layout's cyan.
- The game speed: the engine's clock is the game's time, SDL's performance counter's steps times the speed (the
  framework's game clock at the unlimited speed), so at speed 1 the frames are paced as before. Checked in real time:
  at speed 4 the splash is further on at the same update, and at the unlimited speed 600 updates reach the menu in
  a second.
- The stage's backdrop: the stage (Stage) made by the loader over the game data and the stage container, with its own
  theme (baseTheme.xml); the first menu stage loaded at the loader's end (menuStage.xml, its set mainMenu.xml); the
  stage file's attributes; the stage's slot 13 as far as the backdrop: the play bounds, the mask container (its
  night mask not yet translated), the layer set and its file, the layers' infos (layoutPan only) and the LayoutPan
  layers with their three containers, the layers fitted to the bounds. Checked under rundesk: the menu shows its
  backdrop (the three layouts), and the splash's leftovers under the menu are gone, as the stage repaints the
  frame. Not yet translated: the spawns (the menu's fish), the camera, the play area's edges, the HUD, the stage's
  start and update, the layers' colour, the panelScroll and propScroll layers.
  - The base theme names fx/playerShieldGlow.xml, whose images (resource/FX/shieldFish) the archive lacks, so the
    effect is not loaded (a warning in the log); the original's data is the same.
- The stage's spawns and run: the actor classes' registry (0x485a60, 30 classes, each making a FishSpawn or a
  NonPlayerSpawn); the stage file's <spawn> (0x41a300 and 0x482e30) read by its spawn's handler (0x469960 and
  0x4802e0: name, size, visual, behavior, poolSize, supply, layer, detailLayers, start, location; a fish's rank,
  foodValue, showMenu, showStageComplete; <phase> max and delay, <tweaks>), registered with the stage (+0x274,
  +0x280, +0x2a0, +0x2b0); each spawn's setup as the stage loads (the supply, a default phase, the behaviour found
  and the tweaks laid over it); the stage's start (slot 5, 0x486950, as far as a stage without players goes: the
  pause lifted, the spawn phase 0, the camera half across the pan range), the run state (0x43acb0, only its spawns'
  step), the stage's update in hundredth-second steps (0x43a570), from the splash's bursting on, and each spawn's
  timing (0x43a180: the next one's delay drawn once the phase has room, the actor brought in once it passes).
  Checked under rundesk over 3000 updates of the menu: the pompano every 5 seconds, the queen triggerfish every 3 and
  the herring's school every 5.5 come due (logged, as the actors are not yet translated).
  - The stage's start gives each ambient sound its first delay, a draw of the random numbers each (0x486860); not
    yet translated, so every draw after it is off by those until the sounds are.
- The spawns' visuals (see "Animations"): the Theora frames and AnimationData in the engine (TheoraReader,
  AnimationData, its handler), VisualData and FishVisualData (setup, the frames' scale and tint, the shapes and
  points, the runs fitted), the shapes' slots (Box and Circle: clone, ends, centre, size, scale, move), the factory's
  lookups (0x4100d0, 0x40c550 and 0x40add0 with their messages), the surface's scaled copy (0x3c, 0x40, 0x48) and the
  in-place colour effect (0x54, mode 3). Checked under rundesk on the menu: the pompano's 28 frames at 0.94, the queen
  triggerfish's at 0.6, the herring's 21 at 0.2 (swim cut to 7 by its shuttle), the great white's 30 at 0.875, each
  frame's colours and mask as the original's art.
  - The surface's scale goes through the draw context's sampler, not the original's scaled alpha blitter (0x441810's
    scaled path), until the looks pass; AnimationData's slots 0x08 and 0x14 are not yet translated.
- The menu's fish: Actor, Fish and AIFish (set-up, entry point, move, keeping in the water, the stage's lists), the
  spawn's pool and live list (taking, filling, removing the dead), FishState, SpawnAI, WanderAI and FlyByAI, the
  stage's edges and areas, and the run state's actor and fish-visual steps. Checked under rundesk over 3000 updates
  of the menu: the pompano and the queen triggerfish come in from the sides, swim and turn behind the plants and
  signs, with no actor lost from the list. See ACTORS.md.
- The fish's sight and reactions (0x437ac0; FleeAI, ChaseAI, AvoidAI), eating (the stage's meetings 0x440660, the
  eater part, EatingState, AIEatingState, EatenState) and the schools (SchoolOfFish, SchoolingAI, the school spawn
  0x474dd0). Checked under rundesk on the menu: the pompano chases and eats the triggerfish, which runs, and the
  herring cross as a school.
- Sound: the engine's simplesound (engines/sprout2/SimpleSound.h: the handle's slots the music calls, the system's
  categories, the engine's slots 0x7c, 0x80, 0x84 and 0xfc) and the port's simplesounddx over the platform's mixer
  (host/SimpleSoundDX.h: sources registered under a name, one handle per stream, the fades and the end seen in the
  handle's update every 100 ms of the window's timer, here on the game's clock from the frame loop), started from the
  application's start (0x4a1a70) at the master volume 0.65. The music system (MusicSystem.h: MusicLayer, MusicSystem
  and its file's reader) and the stage's ambient sounds: made at the stage's load (slot 0x3c, at 0.7), started with
  the stage (each layer's first delay drawn) and updated each step (slot 0x6c). Checked under rundesk on the menu
  over 9000 updates: the water loop plays from the start, the waves, water and bubble layers each start after their
  first delay with their second clip (random-not-last's first pick) and then alternate, a delay drawn as each clip
  ends. The order of those draws against the fish's is not yet compared with the original's.
  - Volume: DirectSound's volume is 2000 times the volume's natural logarithm, in hundredths of a decibel, so the
    mixer's gain is the volume to the power ln 10.
  - Deviations: a "random-not-last" layer of one clip keeps to it (the original's draw of 0 to -1 can pick the clip
    past the list's end); the start, final and bridge clips left off the list are let go of with their layer (the
    original never lets go of them).
  - Not yet translated: the options' SoundEnabled and MusicEnabled (both categories stay at 1, as when enabled),
    a handle's fade-out at a sound's end (no translated caller sets one on a sound that plays out), the listeners, the
    sound effects (the themes' sounds, SoundMgr) and the game's music.
- A stage's load from the menu: the main menu's newGame and newTimedGame (MenuState::startGame: a new game in the
  game data, then the restore-game popup when the mode's record holds a saved game, the map once a stage of the mode
  has been reached, or else LevelLoadState for the first stage, the timed mode carrying the stage's newGameTime), and
  LevelLoadState (its screen, its bar, the stage's loader run at once on its own random numbers, since the original's
  is a new thread each time and so starts from the C library's 1, then the stage back under the screens). The loader
  unloads the stage first (Stage slot 0x08, also the destructor's first step: the spawns, whose destructors now take
  their live actors out of the stage through NonPlayerSpawn's slot 0x60, the container, the mask, the layers and the
  ambient sounds), empties and reads again the actors' library, loads the stage and sets its time and canSuck. A
  game's stage is kept in the player's record (UserData::recordGame) and saved as its load starts. Checked under
  rundesk from the menu: Start Game, stage1 loaded and its backdrop shown, the player's file saved, exit 0. Not yet
  translated: the map (a game's stage loads with no screen, and so without waiting for one), the music, the pause
  screen's stage title, the HUD's canSuck parts, and the states that follow (StageIntroState for a stage with intro
  data, else GamePlayState).
- The map (GameMapScreen.h, the application's +0x84, read by the splash's loader from resource/shell/gameMapBase.xml
  and set up from the stages): a GameMapCheckpoint for each stage (four images of the shell theme's, the high one
  pulsing through the engine's new OpacityModifier, and a hidden hit-box button whose action is "levelStart:<id>"),
  shown for a stage's load (the stage named, those before it passed, it current) or for picking a stage reached (the
  menu's newGame once a stage of the mode is reached, and the menu's levelStart action, which drops the mode's saved
  game). The stage under the mouse is named in stageText. Checked under rundesk: Start Game shows the map with
  "Now Loading", "1. Sandy Shoal" and the bar filling, then stage1 loaded, exit 0. Not yet translated: the level sets'
  buttons (imgOff, imgOn).
- The map's pins (SplinePath.h, StageManifest::placePins): each level set's spline file read into a SplinePath
  (0x552bf4: the curve at a parameter 0x436540 blends the four nodes around it, each node's sv moving its knots apart
  when positive and pulling the curve to it when negative; each stretch measured in tenths, 0x434ed0 and 0x434e10, so
  a place is asked for by the fraction of the path's length, 0x47c0c0), and each set's stages spaced evenly along it
  (0x41ad50; the last set, an end marker with no stages, starts at the end of the spline before it). Translated from
  the assembly, the x87 sums kept in doubles. Checked under rundesk: the load's map shows every pin along the paths,
  one set's flowing into the next. Not compared with the original's map yet (the looks pass).
- The first test command, `unlock all` (FeedingFrenzy2App::HandleScriptCommand, answered OK or ERROR on SDL's log):
  every mode's furthest stage set to the count of stages and the player saved. Checked under rundesk with it: Start
  Game shows the map for picking (every pearl gold, "New Game", the instruction, Back To Menu), and a click on the
  first pearl loads stage1, exit 0.
- The stage's intro (StageIntroScreen.h, the application's +0x98 read from resource/shell/stageStartBase.xml, and
  StageIntroState.h): a loaded stage with screen data goes to it; its title ("<n>. <title>"), caption, paragraphs and
  objective come from the data, over the data's text and art layouts (each a ScreenLayout child, read again for each
  stage), with the timed mode's new time and the clock's totals ("H:mm:ss", 0x44ca60) in place of the objective, the
  clock's starting total alone while the game is fresh (the application's +0x324, now mFreshGame, set by the menu's
  new game). Its Continue logs that the play is not yet translated, Options that the options are not, and Quit opens
  the quit popup. The engine's screen reader now reads an image's Theora frame (TheoraReader::readFrameAt, 0x415b40)
  and makes the scaled copy of an image whose scale is not 1 (the surface's 0x3c), as 0x453c50 does: the intro's fish
  pictures and the stages' scaled backdrop pieces, which were drawn at their files' sizes. (A shared engine change:
  Talismania's scaled images change the same way.) Checked under rundesk: stage1's intro with its art, Quit's popup
  over it, exit 0. Not compared with the original's yet; the paragraphs keep the file's line breaks and indents as
  the text item has them.
- The play (GamePlayState.h, 0x1c bytes, vtable 0x55a8c8): the intro's Continue, or a loaded stage without screen
  data, starts it. Its enter starts the stage at speed 1, takes the screens away and sets the application's +0x323
  (mStageInPlay, for the quit popup's gameExitSave); each frame divides the frame's time by the speed (the slowmo
  action steps it 1, 2, 4, 8, 16, 1) and runs the stage, and once the stage's machine is empty logs which end follows
  (complete or bonus: StageCompleteState; lost: the continue offer 0x543631 or the game over 0x543747), none yet
  translated. Its suspend and resume pause the stage. The application now connects its key-down slot (0x4932a0: the
  key passed to the state's new slot 0x1c, AppState::keyDown) and its activate slot (0x48b520: the pause action on
  losing the focus, and +0x321, mActive, kept; the play's update shows the pause while it is false). The play's keys:
  P, Esc and Space pause; F pauses and would switch the full screen (not yet translated). The pause popup's
  pauseResume closes it (or, after the options from the pause, +0x1d, shows the pause again). The stage's +0x391 is
  now Stage::mCompleted, cleared by its start. Checked under rundesk: Continue starts stage1 with its fish moving,
  Esc shows the pause popup, Resume Game closes it and the play goes on, the focus lost shows it again, exit 0.
- The player (PlayerSpawn.h, the <player> element with its <phase>s; PlayerFish.h with SpawnState and PlayState) and
  the engine's mouse (engines/sprout2/Mouse.h and host/MessageMouse.h, the engine's slot 0x100, its object at
  +0x2a8): the play's enter takes the mouse (0x4a1e80: the cursor hidden, its place kept and the cursor at the
  display's centre) and its exit lets it go (0x4a1eb0); while it is taken a mouse move only keeps the place, and each
  frame before the update (0x438700 calls 0x42b8c0) the place less the centre is the motion, emitted, and the cursor
  goes back to the centre; a button sends the motion first, then its press or release. The stage's spawns' setup
  makes the player and a visual for each phase; its start brings the fish in (dropping from the top of the water to a
  depth, then swimming), and RunState's enter and the pause connect the first player to the mouse. The motion,
  scaled by the sensitivity, the acceleration and the share of the fish under the surface, is the step's force; the
  water's drag, the speed's ellipse, the sides, bottom and surface hold it (out of the water it falls back), and the
  camera follows it (LayerSet::followCamera, 0x43b960: what lies outside the camera's limits, at most 12 times that
  distance plus 150 a second, in the pan range). The game data's +0x40 is the food eaten in the stage (zeroed by a
  new game and each start), which grows the player through its phases. The stage's load had the layers after the
  spawns' setup, and the layers' slot as 0x34: they are slot 0x30, made before the spawns' setup (the original's
  0x410980), which the player's bounds need. Checked under rundesk with posted moves (each `move` to the offset and
  back to the centre, as the framework drops a move to the place it already has): the fish drops in, swims right to
  the play area's side, down and back left, the camera following; Esc pauses and Resume Game takes the mouse again,
  exit 0.
  - Deviations, for now: the mouse's sensitivity is the options' default (1.0, MouseSpeed 1; the options are not
    translated); no ClipCursor (the framework's window keeps the cursor); on the web WarpMouse does nothing, so the
    page will need the pointer lock; the stage's unload takes a player still in out of the stage (the original keeps
    it in the stage's players until the play's ends, not yet translated, take it out); the phase visual's tint, the
    eating, the dash, the suck, the tilt, the effects and the sounds are not yet translated, each marked where it
    would run.
- The player's eating (PlayerFish.h: the eater part's eat 0x47a250 and swallow 0x42a6d0, the frenzy's add 0x47c550
  and its timer in 0x4379f0, the points kept at +0x1fc and added by the update 0x44c3b0; PlayerEatingState, from
  PlayState's eat; the prey's slot 0x90, 0x53c96f, and the AI fish's 0x419580 with the school's bonus; GameData's
  addScore 0x4106f0 through the stage's 0x410710). When the player's mouth meets a fish it may eat, the fish is drawn
  in and the player's visual plays its eat; at the chomp each fish swallowed gives its food to the game data's +0x40
  (which grows the player through its phases) and its points times the frenzy's multiplier, the bonus's scale and,
  for a fish eaten in the air, the count of fish eaten in the air since the player was last in the water. Points go
  to the score at the next update; past the next extra life's score a life is given (not in the timed mode), the
  step to the next growing by 5000 up to 25000. Each bite adds its fish to the frenzy's count (at most 35, the
  multiplier one more each seven); the count drops by one when the frenzy's time runs out (the hold 1.1 seconds, half
  again at each seventh; after a drop the hold times 1.4 at a seventh, a quarter above seven, a third below). The last
  fish of a school eaten by the player gives 500 points. The swallow makes the original's draws in order: two for the
  chomp text's place, one for the chance a small bite splats, and the splat image's when it does. SpawnState's enter
  now also puts the visual in its swim state (0x4262e0 with the visual's +0x488), which the first pass mislabelled
  and left out; the player's eat crashed without it.
  - New test commands: `state` prints `STATE {"mode","level","score","lives","food","phase","frenzy","player"}`
    (`level` the stage's place in the manifest, `phase` the player's phase, `frenzy` its multiplier, `player` its
    place or null), and `set score|lives|food <n>` (`set score` finds the next extra life's score again).
  - Checked under rundesk with posted sweeps across stage 1: the player eats the minnows (10 points and 10 food
    each), grows at 225 food (stage 1's first goal) to the next phase's size, and a score set to 49990 gives one
    extra life at 50000; exit 0.
  - Not yet: the eat's and bonus's effects, the chomp's text and splat, the points' bubble texts, the frenzy's HUD,
    texts and sounds, the school bonus's text and sound, the extra life's text and sound, the player eaten (its
    slot 0x8c, 0x48f270: the fish's for now, so a predator's bite ends its state and its spawn takes it out), the
    poison (the eater's 0x18), the suck and the frenzy bonus's timer (+0x1ec). The effects, texts and sounds left out
    may make draws of their own, so the draws' order is not yet the original's.
- The HUD, first pass (GameHud.h: GameHud 0x4036d0, its setup 0x482f20 and reset 0x48b0e0; GrowMeter 0x48ce40; the
  stage's slot 0x34, 0x40d1f0, and the HUD part of its slot 0x5c, 0x447ef0, with 0x44b9b0; the player's frenzy
  meter 0x4605d0; the engine's slot 0xb8, FormatMessageA from a string). The stage makes the HUD with its theme and
  reads resource/HUD/hudLayout.xml with the shell's theme; a load with players puts it on top of the stage's
  container, shows the score, gives the growth meter the last phase's goal with an arrow (growthArrow, centred,
  3 pixels above the bar's foot less half its height) at each goal before it, shows the bonus's counter only on a
  bonus stage and the suck's meter, then the loader shows or hides the suck by the stage's canSuck. The start
  shows the time (the timed mode's time with the time carried in; the clock hidden without one), the lives (at most
  99) and the stage's number and title ("(id)" without screen data). Each frame of the run, while the first
  player is in, the growth meter shows the food eaten, the time counts down, and the lives and the suck's charge
  (red under a half) are shown. The frenzy's word (the string frenzyMeter) lights a letter for each fish of the
  count's seven in the multiplier's colour, the rest in the step before's, and frenzyMult ("%1!d!x") shows the
  multiplier from two; the six colours are the static initializer 0x4d7310's.
  - Checked under rundesk over stage 1: the band, the growth meter filling with the food (its arrows at 225 and
    500 of 900), the lives going from 3 to 4 at the extra life, the stage's name; exit 0.
  - Deviations: the clock's separator is a colon (the original asks the locale, LOCALE_STIME, as the stage intro
    does); the lives and counters are plain digits (0x44e580 groups them by the locale, which leaves under a
    thousand alone); the engine's formatMessage takes whole numbers only (all the game passes).
  - Not yet: the fish ranks' four icons (menuImg0 to 3 in
    selectors, filled by the stage's slot 0x38, 0x405c60, from the fish spawns' ranks of 0x405590, with the fish
    visuals' HUD pictures, not yet made); the splats (SplatFactory, hudSplats.xml) and the time's warning; the
    jump's height and marks, the medals, the bonuses' bubbles, the bonus stage's
    counter; the stage's timer that holds the clock.

- The HUD's points bubbles (ScoreSplat.h: ScoreSplat 0x47a3c0, its setup 0x4302f0 and update 0x433b30; the queues at
  0x5acc20, 0x4194c0, 0x4301d0 and 0x498bf0; the maker 0x427250 from the player's update 0x44c3b0; the engine's
  KeyInterpolator<Vector2f> and the keys' ease 0x4280c0 and 0x4283f0). Each points scored sends a "+n" from the
  mouth (the fish's layer's place added) to the score's text at 400 pixels a second: a jump of up to 60 aside and 80
  up over the first 0.15 of the flight, slowing to its top, then on to the score, half of it straight and slowing at
  the end, the scale 0.5 to 1 and back; small font to 50 points, large from 1000, measured in the middle one to keep
  it on the screen; coloured by the multiplier (the static initializer 0x4d73c0). A bubble is shown at most every
  0.09 seconds, the others queued; on landing it adds its points to the score shown (not below 0).
  - Checked under rundesk over stage 1: the shown score rises by 10 as each bubble lands, behind the game's score,
    a "+10" seen in flight; exit 0. The state line now gives the score shown ("shown").
  - Not yet: the other texts that use the second queue (every 0.3 seconds).

- The stage's goal and its time's end (Stage.h: the play's slot 0x5c, 0x447ef0, its goal and time parts;
  CompletionState 0x49f3d0 with 0x4994b0, 0x4a3ae0 and 0x4334b0; the word 0x416330, 0x498d20, 0x4990f0, 0x4959a0
  and 0x4a3b00; the spawns' stop 0x4a2390 with the spawn's slots 0x34 and 0x64; EvacuateAI 0x4a02c0 with 0x494870
  and 0x4282b0, from the AI fish's slot 0xe8, 0x4a0260; the timetable's 0x47ae00, 0x45f040 and 0x420090, now in
  Timetable.h). The game data's phase follows the spawn's. With the last phase's food eaten (not on a boss's
  stage) the stage is complete: every spawn's fish swim out of the nearer side at 2.5 times their top speed and the
  spawns stop once they are gone; the word is stagePerfect when the player has not died, else stageClear. With a
  time, once it is out (and the player is not eating) the word is stageTimeUp. Then the completion: the stage plays
  on without the player's part, the time left kept in the game data (the timed mode's carried seconds), the game
  kept in the player's record and saved; the word bubbles in at once over 2 seconds (the theme's bubbleTextFont,
  bubbleTextBubble and bubbleTextFx at (400, 200), 8 apart, in the stage's overlays "StageOverlays"), bursts at
  2.5 seconds (the ambient sounds stopped), and at 3.5 the run ends. The test commands: set time, and the state
  line's time and run.
  - Checked under rundesk over stage 1 with the food set to 880: the goal reached at 900 in the last phase,
    "PERFECT!" bubbled in with the fish gone, the run ended and the play state saw the stage complete; exit 0. The
    time's end is not checked yet (stage 1 of the normal mode has no time).
  - Deviation (for now): after a perfect stage the mermaid's swim (the spawn "mermaid" with the spawn type
    mermaidStars, and MermaidState 0x51cc00) is not yet translated, so the completion follows at once.
  - Not yet: the bonus stage's counts and end; the boss's; the sounds played
    (the theme's stageClear, gameOver, playerDie, bubbleTextBurst; the application's sound stopped); the script's
    end; the orphanage's fish; the epilog after the mystery stage; the play state's next stage and game over.

- The player's death (Stage.h: the play's slot 0x5c, its death part; PlayerDiedState 0x498830 with 0x4034b0,
  0x4a0690 and 0x434cb0; the layer set's pan position). When the spawn finds its fish dead, a life is taken (not
  in the timed mode) and shown, and the stage plays on without the player while the word playerDied bubbles in at
  once over 2 seconds and bursts at 2.5; the camera holds for 1.5 seconds, then eases to the middle of the pan
  range's top by 3; at 3.5 the players start again (the stage's slot 0x18), or with no lives left the run ends.
  The test commands win (the last phase's food) and lose (the fish's state ended, as an eater ends it).
  - Checked under rundesk over stage 1 with lose three times: the lives 3, 2, 1, 0, "SORRY" bubbled in each time,
    the fish back after each, and at none the run ended and the play state saw the stage lost; exit 0.
  - Not yet: the player's bonuses dropped (0x4a2120); the sound playerDie; on a bonus stage its own end.

- The fish's colour effects and pictures (engine: PixelFx.h/.cpp, the surface's slots 0x34, 0x38 and 0x54, the
  engine's 0x9c and 0xa0; VisualData.cpp). Each effect now changes a pixel through its slot 0x00, as the surface's
  effect copy (0x43e4d0) inlines them: ColorFx's add (0x509364, held to 0 to 255), multiply and blend; HSVShiftFx
  (hsvAdd: the colour taken out of the alpha, shifted in hue, saturation and value, and put back) and
  RainbowShiftFx (rainbow: a hue turning along the columns). Every channel is truncated (the copy's fistp runs under
  a truncating control word, as _ftol2 does); the floats the original stores are kept as floats, the rest at the
  x87's double precision. HSVShiftFx's sextant 0 gives the colours of sextant 5, as the original's switch has no case
  for it. A frame's effect goes through the copy then the scale (0x4267e0, 0x426990, the slot 0x34). A fish whose
  model names a menu picture gets it through the effect (rgbBlend left out, 0x427490), and the HUD's two icons drawn
  from it: the picture over its shadow, and the danger icon with the picture multiplied by 0.3 and the shell theme's
  hudDangerOverlay at (43, 37). A draw context now starts with its first state, as the original's constructor
  (0x47d130) pushes it. Checked under rundesk on stage 1: the pictures made, the frames' effects applied, exit 0;
  their pixels are for the looks pass.

- The splats (Splat.h/.cpp: SplatFactory 0x491cc0, its update 0x433cb0 and spawn 0x422ce0, Splat 0x483c60 and its
  update 0x434280, the definitions and their handler 0x467940, 0x479fd0, 0x47a200). A factory reads a splats file
  and throws a splat out by name at a point: its image (pivot centred by default) or its text (a text item centred
  both ways, multiplied by its colour unless white, with an optional shadow), kept within the window's width, and
  nine draws in a fixed order (the acceleration, the velocity's two axes, its turn, the start and end scales, the
  life, two of the jitter's ranges). Each frame a splat's life runs down; its opacity eases over the life (held to 0
  to 1), its scale over the growth's share of it, its velocity speeds or slows along itself (a slowing axis stops at
  0 rather than turn about) and the force pushes it. A new splat waits for the factory's next update; a dead one is
  taken out there. The <rotation> turn, <jitter>, <destination> and themed images are read or skipped as the data
  never uses them. The HUD now has its factory (resource/HUD/hudSplats.xml): the chomp's burst (0x41cdb0) at the
  mouth moved by the two draws (1 ahead facing right, 15 behind facing left) in the layer's space, the frenzy's word
  at the window's centre in the step's colour (0x40d970), and the clock's timeWarning under 11 seconds (0x44c910).
  Checked under rundesk on stage 1: the first fish's chomp splats chompSplatMini1 at the mouth, its GULP image
  thrown up and growing; exit 0.

- The stage's end (StageCompleteState 0x4a4660, its enter 0x49aef0, update 0x4915f0, action 0x49e150; the screens
  StageCompleteScreen 0x490c40 and its loader 0x405270, StageCompleteScreenNormal 0x4a15b0 with its setup 0x53c106,
  components 0x5310c2 and fill 0x53ad86, and its five count states). A won stage that is not a bonus stage now goes
  to the stage's end: the food bank before the stage is set up on the screen, the new total saved as the food bank,
  and the game moved to the next stage and recorded (or, after the last, the mode's furthest stage set and its saved
  game cleared). The screen fills up to five slots from the stage's fish spawns marked for it (each the fish's menu
  picture, its count the spawn's deaths, the first slot of a rank and of the water or the air also counting the
  fish of that rank and place it does not show), counts the stage's bonus spawns (a class name holding Bonus) and
  the barracudas eaten, centres the slots 75 apart, and reckons the total. Its machine then pops each number in
  (the shell theme's fishCountPopIn effect, the stageCompleteChomp splat 30 below, half a second apart after 1.3),
  the bonuses, the barracudas' bonus growing in from twice its size over 0.2 seconds, the food bank counting up by
  one or ten every 0.05 seconds, the rank's header and the rank 0.97 seconds apart, and the fun fact fading in over
  0.3 seconds after half a second (after the final boss, its group, the alien added to the players unless there).
  A press of the mouse while enabled finishes the count. Continue loads the next stage (or the menu's first stage
  and the menu after the last); Quit asks as the intro does. Number helpers moved to NumberText (the grouped number
  with its padded mode, the stage clock, the locale's number). Two port fixes found on the way: the fish spawn's
  behaviour (+0x4d8) is its fish behaviour, as the original's findBehavior ends (0x4102cf), so each fish actor now
  gets it from prepareActor; and the shared StateMgr holds its state while it updates, since a state may replace
  itself (the original's lives on in freed memory). Checked under rundesk on stage 1: won with three fish eaten, the
  slots pop in with the bubbles and CRUNCH, the food bank reaches 000,003 with Newbie Nibbler and a fact; a click
  mid-count shows all at once; Continue reaches stage 2's intro; exit 0. Not yet seen: a boss stage's layout and
  bonus (its actors are not translated) and the final boss's alien. Not translated: the sounds, the options popup,
  the high scores after the last stage, the debug fun fact buttons (hidden, taking no input), and the bonus stage's
  end (StageCompleteScreenBonus, +0xa0), whose stage still stops at a log.

- The stage lost (the play state's 0x543631 and 0x543747, the popup's continueYes, continueNo and gameOverDone in
  0x414e50). The continue offer (+0x90, gameOverContinuesLeft.xml) and the game over (+0x94,
  gameOverNoContinuesLeft.xml) are made in the original as StageCompleteScreenBonus (0x4969a0) but read by the plain
  layout's load, so they are plain shell screens here, each with fades of 0.2 seconds. With continues left the play
  shows the offer with their number; Yes takes one, gives three lives, sets the stage's clock and the timed mode's
  rollover from the manifest's entry, and goes back to the play. Otherwise the mode's saved game is cleared and the
  game over shows the score, the player's name with the strings' rankHdrSuffix, and the rank the food bank earns.
  No and Done clear the saved game and load the menu's first stage and the menu. Checked under rundesk on stage 1:
  three deaths show the offer with 2 left, Yes plays on with three lives, three more show 1 left, Yes again, three
  more show the game over with the score 0 and Newbie Nibbler, and Done reaches the menu; exit 0. No was not pressed
  (it shares Done's steps). Not translated: the gameOver music, the high scores before the menu, the sound stopped.
  To compare with the original in the looks phase: the play behind these popups does not darken here.
  A scripted test note: after a popup's button is pressed, the scene keeps it as the hovered component while the
  play holds the mouse (moves are not passed on), so a scripted press on the same button of the next popup gets no
  enter and does nothing; a real cursor is warped back on the release and moves, so press a neutral spot first.

- The bonus stage's end (StageCompleteScreenBonus 0x4969a0, +0xa0, read from resource/shell/stageCompleteBonus.xml by
  the stage end's loader with one fade of 0.2 seconds; its setup 0x4a1b10, components 0x48c530, fill 0x48a610, fish
  count 0x490a60 and its four states) and the number counting up in a text (TextRollUp, 0x49c4b0: from a start to a
  target, a step each 0.05 seconds of the largest power of ten up to its largest step that does not pass the target,
  written in one of five forms). The stage-complete state now takes the bonus screen after a bonus stage, and the
  play goes to it after every stage won or timed out. The screen sums the supply and the deaths of the stage's spawns
  that have a supply, takes the share eaten in whole hundredths (truncated), reckons the time bonus as the seconds left
  rounded up times a hundred and adds it to the score. Its count: half a second, the fish eaten counting up by up to
  five ("n/total"), half a second, the share ("n%"); half a second more, then with a fish left the no-bonus group,
  else the bonus group and the time left (the clock, by up to ten), the bonus and the final score (by up to 500, the
  score from where it was), each half a second apart; then the food bank by ones or tens, the rank's header and the
  rank 0.97 seconds apart. A press of the mouse finishes it. Also loaded now with one fade each for both ways, as the
  original does: the continue offer and the game over. Port fix on the way: a spawn's pool of a class not yet
  translated (the bonus stages' BonusTime) kept empty places, and spawning one crashed; the pool now stays empty for
  such a class. Checked under rundesk on bonus1 (Herring Herder) with its clock cut to two seconds: nothing eaten
  shows "0", "0", Sorry and the food bank unchanged; fifteen eaten show 15/100 and 15%, the no-bonus group and the
  food bank counting to 000,015 with Newbie Nibbler; a press mid-count shows it all at once; Continue reaches stage 4's
  intro; exit 0. Not seen: the time bonus's branch (every fish of the stage eaten). Not translated: the sounds.
  - Oddity, as the original: the fish's and the share's counts set their text before and after the number only after
    their first writing, so with nothing eaten (no step taken) they stay a bare "0" in place of "0/100" and "0%".
  - Fix candidate: the count's first state fills the screen again (0x493810 calls 0x48a610 as the setup did), so the
    time bonus is added to the score twice; the final score shown counts from the score after the first.

- The bonus bubbles (BonusItem 0x483a90 and its four states, BonusTime 0x4a4130, Bonus1Up 0x5341a4, their makers in
  the registry) and the players' meetings with them (the stage's slot 0x54, 0x438ee0, over the stage's bonuses at
  +0x360). A bubble comes in with its effect from the stage's theme behind it (bonusTimeFx, bonus1UpFx), fading up over
  0.4 seconds; from the bottom it takes a speed drawn in the behaviour's speedRangeX and speedRangeY. It floats; it
  pops (the run "pop" of BubblePop0, BubblePop1 and an empty frame, added to the visual data at 15 frames a second)
  when it meets the water's surface on a stage whose surface is away from the top, or as the stage's spawns leave;
  once seen and out of view it goes. A player that eats it draws it into the mouth, fading, and on the swallow a time
  bubble gives three seconds on the clock with the strings' timeSplat rising in green where it was, and a fish bubble
  a life with extraLife (the HUD's placed texts, 0x408df0 and 0x494510, the rising text 0x416020). The eater refactor
  before it lets an eater take any actor. Port fixes on the way: a spawn's default visual is its class's second name
  (+0x28 of the class's info: "bonusTime", "minnowBrown" for AIFish), not the class's own name, which the port took and
  so found no model for the bonus stages' bubbles; an empty frame added to a visual data with nothing to do to it is
  kept, as the original keeps it, where the port dropped it; the actor's own remove (0x407a80) was missing. The state
  test command now lists the bonuses' places. Checked under rundesk: on bonus1 time bubbles rise from the bottom, one
  reaching the still player is eaten (the clock 40 to 43, "+3 SEC" rising); on stage 5 (Uninvited Guests) two fish
  bubbles rise and leave at the top, and the player steered onto the first eats it (lives 3 to 4, "EXTRA LIFE!");
  stage 1's recorded play is unchanged. Not seen: a pop (no stage seen with a surface and a bubble meeting it). Not
  translated: the sounds bonusTimePickup and bonus1Up; the other bonuses' kinds.
  - Oddity, as the original: the rising text measures itself and clamps its x to the screen, then starts from the
    point as given, so a text at the screen's edge is cut.
  - Test note: a screenshot taken before a bubble spawns changes where it spawns (the runs are not the same with and
    without it), so the pickup scripts take their screenshots after the spawn.

## Next

- The mermaid after a perfect stage (MermaidStar is a BonusItem); the other bonuses (BonusStar, BonusLight, BonusStun,
  Bonus2X, BonusSpeed, BonusShield, BonusFury, BonusShrinkShroom, BonusLure, the pearls); later the fish ranks' icons; the restore-game popup's actions; the quit popup's actions over the intro and
  the pause's End Game; the compositeGroup element (the intro's background group, now skipped).
- The menu's fish, continued: the stunned effect and the fish's sounds (the themes' sounds, SoundMgr, over the engine's
  simplesound). The stage's slot 0x5c (0x447ef0) is not a clean-up but the stage's ending (player died, perfect, stage
  clear, time up) and slot 0x60 (0x43a950) the camera on the first player (+0x344): both return at once without players,
  so they wait for the players.
- Compare the ambient sounds' draws with the original's under origrun's virtual clock: when its window's timer runs
  there, and how long each clip plays, decide where the layers' draws fall among the fish's.

- The popup's other actions (pause, quit, save game, options); the duplicate name's and the full list's
  confirmations are wired but not yet seen.
- The stage (Stage, 0x3e0 bytes, its constructor 0x403aa0, vtable 0x5585c4 with 29 slots of about 1,800 lines in
  all, no Mac build to name them): a boundary, a box and six edges (the shapes), four and five colour ranges
  (0x4998c0) at +0x280 and +0x320, and its init 0x4038d0: its own theme read from resource/themes/baseTheme.xml
  (+0x2d4, not a music player), the game's HUD (GameHud, 0x260 bytes, +0x2e0, over that theme), the stun bubbles'
  effect (StunBubbleFx, 0xf8 bytes, +0x308) and the theme's splashFxBig, Medium and Small. Slot 1 (0x410980) reads a
  stage's file through a MasterDocHandler. The menu needs the stage loaded with its menu level as its background;
  the fish, their AI and the HUD come after. Then the menu's music and sounds and its other actions (new game, the
  map, options, high scores, quit's confirmation).
- The stage's backdrop, as read so far: the stage file (Stage::DocHandler 0x47c730: title, sizeUnit, set, bonus,
  scrollRate, night, finalBoss, minLight, maxLight, gameMusic, ambientSound, waterColor, bossLayout; its spawns)
  names a set. The stage's slot 13 (0x4146b0) makes the play bounds from the display's size (top 75 unless
  0x491730), a MaskContainer (0x1a8 bytes, resource/hud/darknessMask.png) at +0x2ec, the LayerSet (0x144 bytes,
  0x4892a0; a Container overriding only its destructor) at +0x2d8, read by 0x40a4e0, and fits the layers to the
  bounds inset by 200 and 160 (0x4858d0). The set's reader (LayerSet::DocHandler: baseLayer, waterLevel, skyCeiling
  at +0xd8..+0xe0, the night colours at +0x118 and +0x128, nightColorLightPct +0x138; each <layer> by its type,
  layoutPan (LayoutPanInfo, the default), panelScroll or propScroll) keeps an info per layer; LayerInfo (0x478650)
  reads color, blend or colorMode, an alpha-like float, panSize and scrollRate, LayoutPanInfo (0x417680) the
  layout. Each info makes its layer (slot 2, LayoutPan 0x34 bytes, 0x40a0f0) given the water level: 0x480e90 keeps
  the info at +0x18 and makes the containers PanLayerActors (+0x20), PanLayerPlayers (+0x24) and PanLayerOverlays
  (+0x28), then the init (0x411050) loads the layout (+0x1c, a ScreenLayout), adds a colour effect over (-100,
  water level) to the pan size plus 100 when the colour's alpha is above 0, and adds the three containers to the
  layout. LayoutPan's slot 7 (0x43c090) pans the layout by the layer's factors (+0xc, +0x10), which 0x4858d0 sets
  from each layer's pan size against the base layer's.
- The layouts' textButton, checkBox, include, radioGroup, table and compositeGroup's caching; the burst's
  sound, the sound system's and the game's updates in the splash's bursting phase.
- The configuration's options, the start's options, key, activate and music connections, the popups and the debug
  display.
- The looks: the frame does not clear the display, so the splash's quarter-second fade builds up over the last frame
  (the background at 6, 17, 33, 50, 66% of its brightness in the first frames where the fade's progress is 7, 13, 20,
  27, 33%). Compare with the original windowed (its back buffer blitted, so kept) and full screen (a flip chain, so
  the buffer two frames back).
