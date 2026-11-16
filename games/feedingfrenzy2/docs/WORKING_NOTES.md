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

## Next

- The splash: the loader (the original's thread, 0x47e450) and the phases after the wait (the burst, its sound, the
  wait for the effects); then the menu.
- The configuration's string table and options, the start's options, key, activate and music connections, the
  popups and the debug display.
- The engine's log goes to SDL_LogDebug, which nothing shows: a way to see it (the original wrote a log file).
- The looks: the frame does not clear the display, so the splash's quarter-second fade builds up over the last frame
  (the background at 6, 17, 33, 50, 66% of its brightness in the first frames where the fade's progress is 7, 13, 20,
  27, 33%). Compare with the original windowed (its back buffer blitted, so kept) and full screen (a flip chain, so
  the buffer two frames back).
