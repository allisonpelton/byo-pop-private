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

## Next

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
