# Bejeweled 3: working notes

What the analysis has found about the exe, for reading the export. The plan is in `PORT_PLAN.md`.

## Reading the export

- `games/bejeweled3/tools/re3.py` prints a function of `re/bejeweled3/gh2` compacted and named (`dec`), its callers and
  callees (`calls`), its disassembly (`asm`), a table of pointers (`table`), the classes placed by their own vtable slots
  (`classes`), a class's vtable (`vt`), the strings matching a pattern with the functions that use them (`str`), and the
  names matching a pattern (`name`). Names come from `re/bejeweled3/names/*.jsonl` and the Mac build's pairs.
- `games/bejeweled3/tools/autoname.py` names the functions that use the texts of `properties/default.xml`: each text's
  AUTOGEN comment names the `Class::Method` that uses it (160 functions; a text several methods share, or one used in
  more than three places, names nothing).
- The exe's `operator delete` is 0x64dbcf, `operator new` 0x64dca8, `std::string::assign(const char*, n)` 0x401710;
  the stack cookie is `DAT_008bdf24`.
- Link-time code generation passes `this` and arguments in whatever register suits the caller: the decompiler shows
  them as `unaff_EDI`, `in_EAX` and the like, and a call whose arguments go in registers shows none (`FUN_00401710()`).
  Read such a function's callers in `asm` to see what each register holds. Turning the 1,080 functions' stack
  alignments (`and esp, -8`) into NOPs before the analysis was tried and changed nothing.
- Zuma's Revenge's Mac build kept its symbols and has the same framework: `tools/macmatch.py` with it against this
  export pairs 399 functions (234 of the framework: PopAnim, PIEffect, the resource manager), in
  `re/bejeweled3/macmatch_zr`, which `re3.py` reads after the session's own names.

## Sources

- `properties/default.xml` is the game's text table (`<String id>`), each with the method that uses it; the 45 source
  files it names are the game's own (Badge.cpp, Bej3App.cpp, Board.cpp, the boards of each mode, the dialogs, the menus,
  Profile.cpp, HighScoreMgr.cpp, QuestBoard.cpp, DigGoal.cpp...).
- The Mac build names 2,191 functions, mostly inline templates; macmatch pairs 38 (`PORT_PLAN.md`).

## The data

- `resources.xml` is ResourceGen3's manifest version 2: `<Resources id="1200_Main" res="1200" parent="Main">` groups by
  art size (600, 768, 1200) under `<CompositeResources id="Main">`, which lists the size groups; resource kinds `Image`
  (6,628, of which 157 have an empty id and are skipped, as the exe skips them), `Sound` (189), `Font` (128), `PopAnim` (64), `PIEffect` (118), `RenderEffect` (14) and `File` (36).
- Fonts are DescParser scripts of the commands `framework/ImageFont.cpp` already reads (layers, tags, kerning, colour
  multipliers, draw modes).
- PopAnim files are versions 4 (47 files) and 5 (17); Twist's player reads up to 3.
- Music is one MO3 suite, `music/bejeweled3_suite.mo3`, with `properties/music.xml` naming each song by its order
  offset and its events (fades as curve strings, row-synchronised switches).
- 487 curve strings sit in the exe (`b+0,1,0.01,1,~###  ~####` and the like), as in Twist.

## Resources

- The resource manager (`engines/prime/PrimeResourceManager`, the exe's 0x45b540-0x465600, vtable 0x871c8c) is the
  framework's of this generation, with three more kinds (PopAnim, PIEffect, RenderEffect), art sizes and composite
  groups. A group's `res` makes each of its ids "ID|res"; `GetBaseRes` tries "ID|<the app's art size>" before "ID".
  Loading a composite group loads its parts whose `res` is 0 or the app's art size, in order. `<File>` entries are
  skipped. Animations are scaled by the app's art size over the manifest's base size.
- Not translated, because the PC build never reaches them: the resource streams (app+0x85c, no streams file ships), the
  decoded-image cache the manager keeps under the name "ResMan", reparsing the manifest while running, system fonts, and
  an image font's flag at +0x35 of the image.
- The generated code (ResourceGen3, 0x6e8ca0-0x70e2ff) is `src/Res.h` and `src/Res.cpp`, written by
  `tools/resgen.py` from the exe's id table (2,636 ids, GetStringIdById's jump table at 0x6eca80), the manifest's kinds
  and the export's extraction functions (3,935 getter calls). The game calls four of them:
  `ExtractLoadingThreadResources` (0x70e0f0, from the loading thread), `ExtractLoaderResources` (0x70e1b0),
  `ExtractInitResources` (0x70e260) and `Extract2DOnlyResources` (0x704c10, from Bej3App at 0x7748e0). The first three
  take the part for the app's art size (600, 768 or 1200) and then the part every size shares. Each part first gives
  every global to the manager (0x6efb20), then gets its resources in a try block that returns false on a
  ResourceManagerException. The getters also record each resource's id under a lock (0x8e1f98).
- `b3_restest` (tools/restest) parses the manifest and loads and extracts the groups in the game's order at each art
  size: at 600, 768 and 1200 every group the boot loads (Init, Loader, 2DOnly and LoadingThread's 1,535 resources, 1,534
  at 1200) loads and extracts. The pictures are mostly JPEG 2000 (4,557 `.jp2` files), read by the platform's OpenJPEG
  decoder (`sexyopenjpeg`).
- An image named "file|ID" (the animations name theirs so) is first looked up as the image resource ID: this
  generation's GetSharedImage (0x46ff40) takes a reference to the resource (loading it if nothing holds one), returns
  its picture if it has one and lets the reference go, leaving isNew as it was; else it loads the file before the bar
  as any shared image. The port does this in `PrimeResourceManager::GetSharedImageByRef`, which Bej3App's
  GetSharedImage (and b3_restest's app) call first.
- Three fonts (flaregothic32 at each size) are UTF-8 with a byte order mark, which the exe's EncodingParser honours;
  the framework's DescParser reads such a file as UTF-8 now, each character as its Latin-1 byte and the euro sign, the
  only character above U+00FF, as 0x80. The others are Windows-1252 bytes.

## Animations

- `engines/prime/PopAnim` is this generation's .pam player (vtable 0x8704c4, code 0x440750-0x44b0b0), with the
  manifest's mod files (PopAnimModParser) and the names of its commands (PopAnimNames). The resource manager loads an
  animation as the exe's 0x463000 does: a PopAnim with no listener, both scales the art size over 1200, and a failure
  only when the player gives an error text.
- `b3_pamtest` (tools/pamtest) loads every .pam of the game (64: 47 of version 4, 17 of version 5), plays each for a
  few hundred updates and draws it onto a picture: all 64 load, play and draw at 600 (`-play -draw`).
- Not translated: the mirrored drawing (nothing in the game sets it), the debugging outlines; the transition between
  two animations is PopAnim::TransitionToLabel (0x444910, started by Lightning's HUD, see "Zen and Lightning"); not translated: saving and loading an animation's state (0x445540, 0x4453b0, 0x445760,
  0x445a00: needed when the saved games are), and its particle effects (their creation in FrameHit 0x445c00, their
  matrix and colour at 0x447f70 and 0x444410), which wait for PIEffect's update and drawing.

## Particles

- `engines/prime/PIEffect` reads every `.ppf` file of the game to its last byte (`b3_pieffecttest`: 118 files, 276
  emitters, 298 particle definitions, 253 layers). The random generator is framework/MTRand at PIEffect+0x78.
- Translated: the update (Update 0x573290, UpdateParticleDef 0x5707d0, UpdateParticleGroup 0x571ad0,
  CalcParticleTransform 0x56fe50, GetGeomPos 0x569f80, GetEmitterPos 0x56fda0, ResetAnim 0x56f730, Clear 0x56fab0,
  IsActive 0x575790), the run-time classes (layers, emitter instances, particles, groups, free emitters), the
  generator's seeding (from the effect's seed list, else Rand, on the first update after it is made or reset) and the cached starting state that
  eleven files end with (0x56e4f0, which the first update loads in place of running those frames). `b3_pieffecttest`
  plays every effect for 300 updates with a fixed seed, the same twice; every cached state reads to its last byte.
- Readings checked against the exe's instructions: a deflector's bounce mirrors the particle's step about the edge,
  makes it the new velocity times 100 and softens the vertical part toward the bounce strength's square root by the
  steepness (0x5728cb-0x5729ed).
- Translated since: the textures' images (0x5679b0, through the app's shared images; a texture without one gets a
  64x64 cell so that a test can run without images), the drawing (Draw 0x575650, DrawLayer 0x574700,
  DrawParticleGroup 0x574060) and Duplicate (0x567930 with the copy constructor 0x5670b0, which copies neither the
  cached state, the instance mask pictures nor the time). `b3_pieffecttest` draws every effect into a software image
  (hash the same twice) and plays 107 duplicates beside their originals, none differing.
- PopAnim's effects: made in FrameHit's "addparticleeffect" (0x445c00), from the resource by path, then by id,
  duplicated, else read from the .ppf. The first effect seeds the animation's own generator (PopAnim+0x1e4) from
  Rand(), and each effect is seeded from it. An object that plays ahead runs its effect frames / time scale / frame
  rate x 100 + 0.5 updates first (checked in the asm, 0x4473d4-0x447420). Their matrix and speed (0x4482a0, 0x447f70)
  and colour and draw (0x444410) are translated; `b3_pamtest -play -draw` plays them, the same twice.
- Not translated: SaveState 0x56eed0 (nothing reaches it), the editor's drawing, the ".ip3" tries (PIEffect::Load reads
  only .ppf, so they fail), and the instance mask picture, which is never loaded. The exe upper-cases the resource
  path it looks up (0x43c210); the port looks up in lower case, which its own keys use. For the main sprite, which has
  no object, the exe would read through a null pointer; the port takes the origin and leaves the speed alone.
- Names left: mValueA, B, C, E and H and mUnreadFlag, which nothing reads; three words at +0xa58 to +0xa60 of the
  copy constructor.

## Boot

- Bej3App: vtable 0x8647b4 (105 slots), constructor 0x76d9e0, ReadFromRegistry 0x76fe40 (slot 19: ArtRes, the Zen
  volumes, AnimateBackground, TipIdx and the rest), Init 0x7748e0 (slot 44), LoadingThreadProc 0x772740 (slot 17). The
  resource manager is at app+0x7f0.
- Init, in order: the properties (default.xml, then defaultFramework.xml, defaultUIConstants.xml and
  defaultFilenames.xml); the manager's base art size 1200 and the app's art size (app+0x9e8, "ArtRes"); tips.txt, one
  tip a line; ranks.txt and badges.txt, lines split at ':' with ',' and '/' lists; the high scores (users\hiscores.dat)
  and the last profile ("LastUser"); the groups Init, Loader and, without 3D, 2DOnly, each loaded and extracted (a
  failure shows the resource error); temple.p3d; the loader screen's widget sized from the art size (1600 by 1200
  scaled, the 1920-wide layout offset by -160), the windowed and resize buttons, four values given to a
  manager at app+0x5d8; then the Zen mode's file lists (affirmations\*.txt, ambient\*.ogg and *.wav, binaural\*.sbg).
- LoadingThreadProc: its task count is the LoadingThread group's resources plus 7; it loads them one by one (counting
  each, stopping if the app shuts down), extracts them (a failure shows the resource error), runs two set-up functions
  of the app (0x76bfc0, 0x7712b0), loads questtemple.p3d, the hyperspace meshes (0x76b160) and its animation
  (0x770bd0), each failure a "Fail!" box, sets three layers' colours on three fonts, and logs
  "LoadComplete Res=%d Seconds=%d".
- The RTTI classes of the boot: LoaderScreen, MainMenu, Menu, PlayMenu, QuestMenu, SecretMenu, the dialogs
  (Bej3Dialog, WelcomeDialog, NewUserDialog, UserSelectionDialog, OptionsDialog, ZenOptionsDialog, HelpDialog,
  HintDialog, BadgeAwardDialog, UnlockDialog, EndLevelDialog), Profile, RecordsScreen with its tabs, the Steam and
  local leaderboards, BinauralManager and CustomSoundManager.
- Init's calls in order (0x7748e0): a high-score manager (0x671490) is locked, SexyApp::Init (0x456d20), the high
  scores load from users\hiscores.dat (0x6717a0), the lock is released; LoadProperties (default.xml), then
  LoadProperties(file, required, no signature check) for defaultFramework.xml, defaultUIConstants.xml and
  defaultFilenames.xml; the manager's base art size 1200 and its art size from +0x9e8; the quest configuration
  (0x76bc30: properties\quest.cfg, else defaultquest.cfg); an object at +0xbac (0x78a740, 0x78a8c0); tips.txt,
  ranks.txt and badges.txt through EncodingParser; the Profile at +0xb80 (0x71b530), its "Test" entry, its users
  (0x71d410), the registry's "LastUser" selected (0x71d7c0) or another (0x71d800); 0x4a5120; LoadResourceManifest
  (0x46e0e0, properties/resources.xml); the music (+0xa30) reads properties\music.xml (0x73a890) and is given
  "LoadingScreen" and "Play"; the groups Init, Loader and (without 3D, 0x46f830) 2DOnly, each loaded (manager slot 23)
  and extracted, a failure calling ShowResourceError (0x46e190); three cursors (0x46c420); temple.p3d through the
  graphics driver (+0x10, slot 26) into +0xc34, a failure a "Fail!" box; the MainMenu at +0xba0 (0x6a54b0); the
  TooltipManager at +0xbf0 (0x67c540); the windowed button at +0xd70 and the resize widget at +0xd74; the four
  sound kinds' volumes (+0x990, +0x998, +0x9a8, +0x9b0) given to the sound manager (+0x5d8, slot 5) as kinds 1 to 4;
  two calls with the screen's size and half of it (0x459c40-0x459d10); the Zen file lists.
- ReadFromRegistry (0x76fe40): "ArtRes" into +0x9e8 unless a switch chose the size (+0x9dc), then the height is the
  art size and the width 4/3 of it; "VoiceVolume", "ZenAmbientVolume", "ZenAmbientMusicVolume", "ZenBinauralVolume"
  and "ZenBreathVolume" as percentages into the doubles at +0x990, +0x998, +0x9a0, +0x9a8 and +0x9b0 (defaults 0.85,
  0.5, 0, 0.25, 0.5); "RegCodeNotNeeded" (+0xa78); "AnimateBackground" (+0x9e0, default on); "ClientId" (+0xa9c),
  made when empty (0x76f4c0: an MD5 of the C: drive's volume details, its free space and the time, for PopCap's
  online reports; the port leaves it out) and written at once; "TipIdx" (+0xd78). WriteToRegistry (0x76d450, slot 18)
  writes the same, the volumes times 100 truncated, and first "GameData" (registration data) and "LastUser".
- HandleCmdLineParam (0x76f9d0, slot 41): -600, -768 and -1200 choose the art size (the width 800, 1024 or 1600) and
  set +0x9dc; -wide sets three aspect ratios; -classic, -speed, -quest (+0x9dd to +0x9df) and -webroot (+0x9d8) are
  developer switches; -testbkg takes a value.
- The constructor's default art size: 600 when the machine has 512 MB of memory or less or the desktop is under 601
  pixels high (+0x748), else 768, unless compat.cfg's GetAppDefaultArtRes names one (the port has no compat.cfg).
- The boot screen is the MainMenu (vtable 0x86c35c, constructor 0x6a54b0, made by Init at +0xba0): its Draw
  (0x6a0ca0) shows "Loading..." and the player's line ("Click here to change user"), its Update (0x6a3530) runs the
  loader's curves and loads the "Loader" group's look, and 0x6a2a20 loads its background: main_menu.pam (and
  overworld.pam) in 3D, the flat picture images\NonResize\2donly\mainmenu\background in 2D. The constructor reads the
  logos from the properties ("Logo%dFile", "Logo%dHoldTime") and makes the PLAY, RECORDS and OPTIONS buttons.
  LoaderScreen (vtable 0x86db74) is only a progress bar (its Draw 0x6761f0); its constructor is inline and was not
  found yet.
- Translated in Init's second half (checked by a -fast run to the 300th update, no popups): the high-score manager and
  its local server (src/HighScores.*), the four quest files (src/QuestData.*, the .cfg format read by the port's own
  reading of 0x791c80), tips, ranks and badges through EncodingParser (src/EncodingParser.*), the Profile with users.dat
  and profile.dat at version 0x47 (src/Profile.*; several fields are named by position because their roles are not known),
  "LastUser" in Init and WriteToRegistry, the music (src/Bej3Music.*: music.xml read, "Play" of "LoadingScreen" started by
  order; curves, row syncs and tracks not translated), the three cursors. Left out: the Steam object at +0xbac, temple.p3d,
  the tooltip manager, the +0x218 start value, the temporary-player folder move (0x71b940) and 0x71ba40's folder delete.
- The MainMenu (src/MainMenu.*) is made at the end of Init and shown: the 2D background (the 2DOnly group's mainmenu background,
  stretched over the menu: the Background widget 0x77dbb0 made by 0x6a2a20), the PLAY, RECORDS and OPTIONS buttons
  (string ids 0x11f, 0x11e, 0x11d; they are CrystalBall objects, 0x78ab50, 0x678 bytes, with particle effects, and are plain
  plates here, lit under the mouse), "Loading..." (id 0x122) while the loading thread runs and the player's name. Positions, the
  curves of the logos and buttons, the foreground picture, the tips and the 3D branch are not translated; the buttons'
  places are the port's own guess. The LoaderScreen (0x6761f0) is not translated.
- Translated so far (src/Bej3App.cpp): the constructor's names, manager and art size; ReadFromRegistry and
  WriteToRegistry without GameData (registration, removed), RegCodeNotNeeded (to check what +0xa78 gates), LastUser
  (comes with the Profile) and ClientId; the art-size switches; the 2D mode (0x76b020); Init's properties, manifest and
  groups; the loading thread's resources; the shared images' "file|ID" lookup. Not yet: the rest of Init and of the loading thread as listed above, the
  developer switches (-wide, -classic, -speed, -quest, -webroot, -testbkg) and the load-time line 0x7702d0 writes.
- The game's texts are properties/default.xml's numbered `<String id="N">` entries (579, each with the method that
  uses it in a comment); the exe looks one up by its number with the English text as the fallback (0x475e30, e.g.
  285 "OPTIONS", 286 "RECORDS", 287 "PLAY" for the main menu's buttons). The port can read them through the
  framework's string properties by the number's text.

## Boot readings settled, and what the menu still needs

- Settled: the quest files' command separator is the semicolon: DescParser's constructor (0x54d0b0) stores 1 at +0x28
  and QuestDataParser's (0x404560) leaves it. The cursors: Init calls 0x46c420 with kinds 0, 1 and 2 and the images
  0x304 (IMAGE_CURSOR_POINTER), 0x303 (HAND), 0x302 (DRAGGING), the framework's CURSOR_POINTER, CURSOR_HAND and
  CURSOR_DRAGGING in that order; 0x46c420 forwards to slot 0x74 of the object at app+8.
- Not settled: slots 31 and 32 of the app's vtable are one-line forwarders to slots 0x34 and 0x38 of the object at
  app+8 (as SetCursorImage is to 0x74), so they are not shown to be Popup; ShowResourceError (0x46e190) calls slot 32
  with the error text. The port shows that text through Popup, which has the same visible effect (an error box).
- CrystalBall (vtable 0x865564, derives from ButtonWidget 0x49c490; ctor 0x78ab50): it overrides slot 22 (Update
  0x78c730), slot 26 (Draw 0x78ae40, 583 lines), slot 40 (0x78c620: after the base's Resize 0x46aa00 it builds a
  3x3 matrix and copies it into both effects at +0xba4) and slot 50 (0x78aaa0). Fields: +0x184 and +0x188 two
  effects made by PIEffect::Duplicate (0x567930) with +0xb86 set; +0x450 the highlight (Update: counter +0x47c plus
  one, both effects updated by 0x573290, highlight moves 0.05 toward 1 while +0x69 (mouse over) else toward 0);
  +0x454 and +0x460 colours; +0x484 and +0x485 flags; +0x486 and +0x487 flags; +0x368 a fade; +0x168 a shared image.
  Translated: Update's highlight only (src/MainMenu.cpp).
- CrystalBall Draw 0x78ae40, read in its instructions. Nothing is drawn when +0x24 is set and the width is 0. With 3D
  (0x4322a0, Graphics' Get3D, returns an interface) a matrix moved by the sub-pixel doubles +0x270/+0x278 is pushed
  (0x583210, 0x583490, 0x4358a0). A copy of the Graphics (0x432410) is translated to the ball's centre (0x434350,
  width and height x 0.5), and, while +0x484 is clear and +0x485 set, by two curves x art size / 1200 x (1 - a curve).
  The scale is a curve's value, times 0.00255 / +0x280 (the depth) when that is above 0; x 5.3, and x (1 + a curve x
  0.5) across and (1 + a curve x 0.2) down. Three times, both effects' colour (+0xb74..+0xb80) is set from colour
  curves. Without +0x484 the effects are drawn (PIEffect::Draw 0x575650 on +0x188, under the scale matrix 0x583570 x
  art size / 1200). Then the colour (+0x458, with +0x484 the alpha from +0x464 x 0.5), colourising if it is not white
  (0x436390 against 0x8e205c), the normal draw mode, and the picture: IMAGE_CRYSTALBALL_SHADOW (0x306, global
  0x8d7f04) whole through the scale matrix (0x5835b0, 0x5836d0, 0x433fa0), then IMAGE_CRYSTALBALL (0x305, global
  0x8d6db8) at cel (update counter +0x47c / 4) mod 40 of its columns and rows (0x434020), and again in the additive mode
  while +0x68 (pressed or over) is set. With 3D and without +0x484 a refraction mesh of 10 rings of 30 is built from
  the curves (+0x488, +0x4b0, +0x4d8, +0x4fc, +0x57c..+0x5f8) and drawn as triangle strips (0x435800, 0x1c4 vertices)
  over the shared image +0x168; then the second effect (+0x184) is drawn as the first. Without 3D, while a curve is
  not 0 and +0x168 has a picture, that picture is drawn colourised white at alpha curve x 255, at (-transX, -transY)
  when it is 800 wide, else stretched to 960 by 600 at (art size x -160 / 1200 - transX, -transY): the screen behind
  shows through the ball's rectangle. The text: the scale is (0x4378f0 of the highlight +0x450 x the property
  CRYSTALBALL_TEXT_SCALE_FACTOR, default 4.0, + +0x478) x the ball's scale, with the curve "b+1,1.3,0,1,####
  ~~###"; its alpha (1 - 2 x a curve) x +0x368. Below alpha 1, while one of two curves is above 0, a picture is drawn
  through a scale matrix (mirrored, -scale across, for the first), moved art size x 200 / 1200 x (1 - that curve) to
  one side (0x433fa0 at 0x78c0ab; which picture is still to read in the asm). FONT_PLAYBUTTONS (font id 3, global
  0x8d8c80) draws the label +0xc0 at the middle, art size x 34 / 1200 below the centre, scaled about the centre
  (0x713770), with the layers GLOWOUTLINE, BOTTOMGLOW and a third coloured (0x58f620) and restored after (0x58f920).
- MainMenu: Draw 0x6a0ca0 and Update 0x6a3530 are driven by about fifteen CurvedVals (curve strings such as
  "b;0,1,0.005,1,####         ~~###" in the 2D-or-3D constructor), the logos ("Logo%dFile", "Logo%dHoldTime") and the
  tips line. The buttons' places come from Update 0x6a3530, in 2D as well: each crystal ball is a point in 3D, turned
  and moved by matrices built from the curves (0x419850, 0x428600, 0x4198b0, 0x4197a0, 0x4287e0) and projected to the
  screen by the camera object at MainMenu+0x134 (its slot 3); the button is then resized around the projected point
  (slot 40), PLAY art size x 180/1200 across and RECORDS and OPTIONS x 120/1200, and the point's offset from the
  integer place is kept at the button's +0x270 and +0x278. The PLAY point is moved by the app's fields [0x1c5],
  [0x1c7] and [0x1c8] (the screen's offset and scale) and art size x 160/1200, scaled by 1 - a curve. Until this is
  translated the menu keeps the port's own placement (a "port:" note in src/MainMenu.cpp), and the foreground picture
  and logos are not drawn.

## The menu's placement (Update 0x6a3530), translated

The buttons' places are now the exe's (src/MainMenu.cpp, src/Math3D.{h,cpp}). Each update, both cameras (the objects at
+0xdc and +0x134, vtable 0x865ff8, constructor 0x419910) get SetFov(38.5 x aspect, aspect, 0.1, 1000) and are put at
the eye (0, 0, 2c + 0.25) looking at (0, 10, 7c + 2.35), up (0, 0, 1), c being the curve +0x718 (1 at the start, so
the cameras begin far back). Both transforms are then moved by (0, -0.4, 0); camera A is turned about z by curve
+0x190 x -0.78 and camera B (the one the balls go through) by curve +0x200. The ball points are PLAY (0, 2.0, 0.7),
OPTIONS (-0.802, 1.93, 0.64) and RECORDS (0.802, 1.93, 0.64); the screen place is width x x + art size x 160/1200 and
height x y (the PLAY point also blended to the menu's middle by the PLAY button's own +0x190 curve). Matrix helpers:
0x419850 and 0x4198b0 copy a transform in and out, 0x428600 composes, 0x4197a0 is the turn about z, 0x4287e0 takes a
world point into a camera's space, 0x419a50 projects it. The intro (curves started once the loading thread ends and the
smoothed progress reaches 0.995) and the background fade (progress 0.2) are translated; the 0x59b0a0 "cache" branch
(which sets some curves to constants instead) is not, the port never takes it.

Settled: 0x59b0a0 is a flag the exe sets when it was started with an already filled resource cache; every use here
takes its false branch.

Oddities / not translated, each with a "port:" note in the source:
- Slot 0x7c on the balls is Widget::SetVisible (framework virtual order); the constructor calls it with 0 and the
  intro start with 1, and the port does the same (the old mActive stand-in is gone).
- The Update's background widget (+0x6d8, its Resize at curve +0x190's first value), the widgets at +0x6f0 to +0x6f8,
  the overlay widgets +0x700 and +0x704, the user dialogs, the loader's unloading and the stars (0xdc particles of the
  constructor, with their grey backdrop fade, progress * 3 - 1.6 clamped) are skipped.
- Logo phase: partner.xml is loaded by SexyApp::Init (an earlier note said it was not), NumLogos is read, and the
  constructor loads "Logo%dFile" and the Update runs "Logo%dHoldTime" (about 350 updates in all) with a black cover
  over the menu. The logo file is not in assets/, so its image is null and its draw is skipped: the shot at update
  300 is plain black. Not known whether the user's install has the file.
- Foreground picture (images/NonResize/2donly/mainmenu/foreground) is drawn with its curve as the exe's formula.
- The crystal ball's cover (+0x168) is a capture of the PopAnim background taken through slot 0x148 at the end of
  Draw; it is not translated, so the cover has no picture. Which object draws IMAGE_2DONLY_MAINMENU_BACKGROUND in the
  exe is not settled (the port draws it first in MainMenu::Draw), nor the draw order against the child widgets.
- ResizeButtons (0x6a09e0, run when curve +0x190 is triggered) is not translated; the app's viewport offset ([0x1c5])
  is taken as 0, and which of the ftol and sub-pixel assignments for PLAY comes first was read from the code, not
  checked against a running original.
- Draw: the alpha of the curve +0x2e8 (its block draws nothing) and the particles are still open.

## The crystal ball, 2D path (CrystalBall 0x78ab50, Draw 0x78ae40, Update 0x78c730)

Translated in src/CrystalBall.{h,cpp} (the stand-in plate is gone). Read from the asm and checked: the Graphics helpers
are the framework's (0x432410 and 0x432520 PushState and PopState, 0x432590 SetColor, 0x432610 SetDrawMode, 0x432630
SetColorizeImages, 0x434350 and 0x434330 Translate, 0x5835c0 Transform::Reset, 0x5836d0 Transform::Scale, 0x433fa0 and
0x434020 DrawImageTransform without and with a source rect (the image's middle at the given point), 0x433040 DrawString,
0x713770 PushState plus SetScale about a point in 2D, 0x583570 the scale matrix of a PIEffect (+0xba4), 0x58f620 and
0x58f920 a font layer's colour set and put back). Images: the shadow is IMAGE_CRYSTALBALL_SHADOW (0x306), the ball
IMAGE_CRYSTALBALL (0x305; the frame is (update count / 4) mod 40, the column the cel modulo the columns at the image's
+0x38 and the row the cel over them, as Image::GetCelRect; 0x45b1d0 and 0x45b1e0 are the cell width and height), the additive pass while pressed (+0x68) is IMAGE_CRYSTALBALL_GLOW
(0x78a, not the ball again), and the arrow at 0x78c0ab is IMAGE_MENU_ARROW (0x7b8), mirrored for curve +0x370. The effects
are made by the constructor from PIEFFECT_CRYSTALBALL (+0x184, drawn after the ball) and PIEFFECT_CRYSTALRAYS (+0x188,
drawn before the shadow), both looping, their colour a grey 255 x (1 - curve +0x190). The label colours its font
layers GLOWOUTLINE white, BOTTOMGLOW (224, 64, 160) and MAIN from +0x468 (constructor: (128, 0, 64, 160)); +0x458 is white.

The constructor clears +0x51, the widget's mClip, so the ball, its rays and its label reach past the button's
rectangle; without that the port drew each ball as a pale square, its middle clipped to the rectangle. With it, the
balls, their glow and the sparkles show at update 1500 as in the game. Not yet compared with the original: the shadow's
colour and the effects' look, frame by frame.
Not translated, each with a "port:" note: the 3D drawing (refraction mesh), the 3D-only curves (+0x288, +0x2f8 stay at
their default), the hint drawn through 0x8d7a80 when +0x487 and the app's +0x687 are set, +0x486's picture (0x8d7054),
and the call at the end of Draw to 0x584d60 with +0x20 and +0x480. The width-0 test at the start is "+0x24 set and
width 0" in the exe; the port tests the width only (+0x24 not mapped).

## The menu's buttons (ButtonDepress 0x6a2de0)

The balls are ButtonWidgets (0x49c490) with ids 0 (OPTIONS), 1 (RECORDS) and 2 (PLAY) and the menu as listener: its
second vtable, at +0xbc, is 0x86c4a4 (slot 2 ButtonDepress 0x6a2de0, slot 4 ButtonMouseEnter 0x6a0860, which plays
SOUND_BUTTON_MOUSEOVER; 0x76aaf0 is the app's sound call: id, volume, pan, pitch). Nothing runs until the intro has
started, nor while the PlayMenu (+0x270, made by 0x680760 in the menu's constructor, hidden at first) is active (fade
curve +0xe8 not 0, or its end value 1: 0x6899c0). PLAY (2): when its ball's +0x200 curve is not running, it plays
SOUND_CLICKFLYIN, sets the PopAnim background's curve (+0x6e0 object's +0x1a8, not translated), the PlayMenu's fade
+0xe8 ("b?0,1,0.01,1,####    C#.Z[    r~###j~###"), the ball's +0x190 and +0x200 (the ball grows from 0.17 to 0.93) and
gives the PlayMenu the focus (slot 8 of the widget manager, which is SetFocus). The PlayMenu is shown (Update
0x696ad0) while its fade is above 0 and its +0x490 curve is not 1; the menu's Update hides the menu at +0x490 = 1 and
stops "showing" it (balls hidden, cover dropped) at fade = 1.
OPTIONS and RECORDS (0, 1): when the camera turn (+0x190) is already at -1 or 1 they turn it back to 0, else to -1 or 1
(a "b=-1,0,..." curve), starting the side-fade (+0x588) and name (+0x5f8) curves and disabling all five buttons
(SetDisabled, slot 37); the screens they show (+0x6f0 to +0x6f8 widgets, ids 3 quit, 4 user list, 7 help) are not
translated, so the port only logs the press. The first-run name dialog is not in this handler: Update opens it (below).

## The first player's name (WelcomeDialog, NewUserDialog, Bej3Dialog)

Test path (port: test only): in a fresh user folder the welcome dialog stays open, and the end dialog waits for no other dialog to be up
(UpdateGameOver asks GetDialogCount() == 0). The test command `name <text>` types the name into the welcome dialog's edit and presses its
own OK (Bej3App::EnterScriptName, from DoWelcomeDialog and from the command), so the profile is made as the game makes it; with no welcome
dialog up it does nothing. Real play is unchanged. The suite scripts start with it and check `endscreen` at the end of the Classic and the
Lightning game (Zen has no game over); the frozen hashes were renewed once for the extra OK line and the endscreen key.

The menu's Update (0x6a3530) opens the welcome dialog (0x771040, "Bej3App::DoWelcomeDialog()") once the intro has
started, the distance and turn curves (+0x718, +0x190) are 0, the profile's name is empty (Profile+0x74 is the size of
the std::wstring that starts at +0x60) and no dialog with id 1 is open. The exe first selects a name given in the
string at app+0xc14 (a command-line user) and does nothing when app+0x380 is set: the port has neither (what sets
them is not read). The WelcomeDialog (0x683600) is the NewUserDialog (0x6824b0, dialog id 1, no buttons) with the title
string 0x1fd; the same class with `(1, 1)` (0x76fdb0, from the user list) has OK and Cancel and id 1, with `(1, x)` as
a rename it is id 2. The dialog is 900 by 400 (1200ths of the art size), 450 left of the middle and 420 down; its
name box (an EditWidget, id 1, listener the dialog, colours transparent, transparent, white, white, black, at most
500 wide) is focused. Enter in the box (EditListener slot 0, 0x673b80) is the dialog's OK (1000); AllowText
(0x675db0) takes a first typed letter's capital (only when the box already has text) and lets letters, digits, spaces
and what the font has through.
Bej3Dialog (0x76a2c0) is the framework's Dialog (its Draw 0x584320 and the label's picture 0x4343b0 are
DrawImageBox calls) with: header colour white, lines black, space after the header 45, content insets
(90, 22, 90, 45), button spacing 10, side padding 25, line spacing -6 (all x art size / 1200), the dialog fonts, and a
fade curve at +0x190, "b+0,2,0.033333,1,####        cY### >P###", that Update (0x76a8a0) steps: at its end value 1
the children take the mouse (0x76a060), at 0 the dialog is killed. Closing (0x76a670) starts "b+0,1,0.05,1,~###
~#A5t" from the value now. While a dialog grows the exe also scales its picture about its middle (0x76a760), but only
when the graphics have a 3D device: the 2D mode does not. The exe's dialog buttons (0x76a190, Bej3Button) are not
translated: the framework's DialogButton stands in (only the welcome dialog is shown so far, which has none).
DialogButtonDepress (0x771a70) for id 1 and OK (1000): the name is the box's text with the white space at both ends
cut off; empty gives the "ENTER NAME" message (strings 0x53 to 0x55, a dialog with id 0 and a footer button) and
stays open; else Profile::CreateProfile (0x71c820) adds the name (the same name in any case shows "SAME NAME", strings
0x19c to 0x19e, and stays open), saves the player that was loaded, resets the player, takes the name and writes
users.dat and the registry's "LastUser" (checked in a run: users/users.dat and LastUser=Zed). Every dialog that has
not returned early is then closed (the end of the function). The Steam object's calls (0x78a4e0, 0x78a460) and the
user list dialog's rows (0x6774e0, 0x6779c0) are left out. The other ids (0x21, 0x2d, 0x13, 4, 2, 3, 0x14, 0x18) belong to
dialogs not made yet. The exe's second FUN_0043b3b0 (before the profile is saved) and the folder-making call 0x43b460
go through the file driver's slots 0x68 and 0x60 (0x5d3d30, 0x5d3c10: not read); the port's file writing makes the
folder. Not read: the password-less "param_3 == 0" branch of id 1 (a button id 0 sets the name to "" and creates it).

## The PlayMenu (0x680760, Update 0x696ad0, Draw 0x689d20, listener at +0xbc, vtable 0x86e4d4)

The screen PLAY opens: a child of the main menu (art size 1920/1200 wide, the art size high, at 0,0 of the menu, so the
menu's -160 offset puts it in the window), the listener of eight CrystalBalls (ids 0 to 7, texts from string ids 0x158 to
0x15f: CLASSIC, ZEN, LIGHTNING, QUEST, POKER, BUTTERFLIES, ICE STORM, DIAMOND MINE) and of two Bej3Buttons, BACK (id 100,
IMAGE_OVERWORLD_BACK_BUTTON) and QUIT (id 0x65, IMAGE_MAIN_MENU_QUIT_BUTTON), each put where the resources' x= and y= put
its picture (159, 1100 and 1567, 1100 in 1200ths), cel 1 at rest and cel 0 over and down. Balls 0 to 3 are drawn at scale
0.2, 4 to 7 at 0.1 (their mScaleCurve constants).

Update (only after the menu's intro has started): visible while the fade (+0xe8) is above 0 and the hide curve (+0x490) is
not 1; the balls and buttons take the mouse only while no ball's scale curve is running; with the fade up and its target 1
it is brought to the front, and with its target 0 the label is hidden. Ball placement in 1200ths: balls 0 to 3 at x 460
(mirrored across the width for odd ids), y 250 (+650 for ids 2 and 3), then pushed out from the middle by (1 - fade) x 1.25;
balls 4 to 7 at x 711 (mirrored, +4), y 420 (+295 for the lower pair), pushed out by (1 - fade) x 3 across and x 3 or 4
down. The ball that the +0x488 index names (0) is drawn towards the middle by the hide curve. Balls 4 to 7 get colour
(255, 119, 255), text colour (34, 34, 34, 160), then (170, 170, 170); they are flat, drawn with the question mark
(IMAGE_OVERWORLD_QUESTIONMARK, which CrystalBall now draws) and do not move while the profile's flag at +0x180 + n is 0
(the port's mTutorialFlags[n], so the one array of flags holds the four modes' unlocks). With the fade at 1, nothing said
of a mode (label curve +0x1e8 at 0) and no game played (profile stat +0xc0 is 0), the hint curve (+0x418) fades in; with
the label up it is stepped down (SetCurveMult).

Draw: the first four balls' text colours blend from their colour at construction (kept at +0x50c) towards (255, 0, 85) by
the four aux curves (+0x258...), the first one pulsing towards (255, 96, 144) by (0.5 + cos(update count x 0.075) x 0.5) x
the hint curve; each ball's text scale add is its curve. While the label curve is above 0 or the hint is up, the label
(+0x1c8) is written centred under the middle in the hover font with black glow and outline, and for a locked mode
"SECRET GAME" (string 0x160) above it in the locked font (MAIN 0xff6666, GLOW and OUTLINE black), the label then 12/1200
lower; the hint (string 0x236) is written at the label's place with the hint curve as alpha.

Mouse over a ball (0x68e0e0): the sound SECRETMOUSEOVER1 to 4 by id mod 4 at volume 0.5, pitch -4 for balls 0 to 3 and 0 for
the rest (BACK and QUIT: MOUSEOVER at 1), then the label, strings 0x16d to 0x170 for the first four. For the four locked
modes the open text is 0x171, 0x173, 0x175, 0x177 and the locked 0x172, 0x174, 0x176, 0x178, each filled in with 5 (levels
of Classic for POKER, of Zen for BUTTERFLIES), 100,000 (points in Lightning for ICE STORM; the number's own formatter,
0x43b2e0, is read as a thousands separator) and the colour text "ff93c0" for %S; a locked mode also fades its own ball's
aux curve in and the others out. Mouse leave (0x6847b0): the label and the red are faded out (0x684660).

Click (0x69b360): ignored while the fade's target is 0; BACK takes the screen away, QUIT asks to quit, a locked mode shows a
dialog (strings 0x161 to 0x16c) and an open one starts the game. Here every click logs "PlayMenu: mode n (NAME)[, locked]",
"PlayMenu: BACK" or "PlayMenu: QUIT".

Left out, and listed in PlayMenu.h: the background widget the exe draws behind the balls when the menu's cover widget (+0x6e4)
is hidden, the music start in Update, the balls' cover pictures, the Bej3Buttons' own class (the alpha at +0x168 that
fades BACK and QUIT with the first aux curve), the exe's list of buttons (0x474fc0), and the screens at app +0xbd8 and
+0xbe8.

Fixes found on the way: the PlayMenu's size was written transposed (the menu's constructor passes the rectangle 0, 0,
art size x 1920/1200, art size); the Profile is saved when it is deleted (0x71c4b0), which the app's destructor now does,
so the first player's profile.dat exists after a run and the next run loads the player instead of asking again.

## The Classic board: start, data and first fill (Board.h, Board.cpp, TestCommands.cpp)

Start (0x7737d0, called by PlayMenu::ButtonDepress case 0 after it clears the app's +0xc31 and its own +0x3cc): the screen
at app +0xba4 and the board at +0xbe8 are taken off their parent and deleted; a ClassicBoard (RTTI vftable 0x86511c, 0x3088
bytes, constructor 0x73b390, derived from Board, constructor 0x74ae30, vftable 0x864cb4) is made, put at +0xbe8, sized (0, 0,
the app's width, height), ResetGame (slot 85, 0x74a880), slot 213 (0x743f40) with 0x15, the saved-game question
(Bej3App::DoSavedGameCheck, 0x76cde0: when it says yes it sets +0xc31 and the board is not added), then AddWidget and
SetFocus on the widget manager (+0x360), NewGame (slot 135, 0x74e410), and the music: FadeOut with no song, then Play of
the mode's name (slot 89, 0x7413e0: "Classic").

The grid: Piece* at board +0xf8 + (col + row * 8) * 4, 8 by 8; rows 8 and 9 of the same array (+0x1f8, +0x218) are two
tables the fill uses (a column's last fall delay, floats, and a column tag, -1 after a fill), so a write to "the cell under
row 7" of a gap would land in the delay table (it never does: row 7 is the first one scanned). The last row is at +0x22e4 (7).
A cell is 128 in 1200ths of the art height; the grid's corner is (0x20b, 0x4b) of the screen (slots 171 and 172, 0x73bd80 and
0x73bd70, read +0x2c4c and +0x2c50, both 0 from the constructor).

Piece (0x630 bytes, 0x7203b0 with 0x720120, made by 0x7415c0 which also puts it in the grid, x = col * 128, y = row * 128,
update counts from the board's +0x38): +0 board, +4 id (the board's counter at +0xf4), +8 col, +0xc row, +0x10 x, +0x14 y,
+0x18 and +0x218 the update it was made and first fell in, +0x1c a float, the vertical speed in 1200ths of the art height per update (named mFallSpeed; 0 until it first falls; the first port called it a fall delay), +0x188 the
column tag it was made with, +0x17e 1 while the fill has seen it (0x17f and 0x182 start at 1), +0x20c -1 keeps it from
falling, +0x220 the colour (-1 none, 0 to 6), +0x228 the flag bits (0 plain; 0x10, 0x4, 0x80, 0x2, 0x400 to 0x2000 are the
specials, not translated), +0x230 a busy count (the fill waits while any piece's is above 0), ten curves at +0x20, +0x90,
+0x100, +0x198, +0x280... (idle on a new piece: every test of them in the fill reads "not running").

Random numbers: the board has its own MTRand at +0x258, seeded in its constructor with the app's global generator's next
number (0x439a90, the generator at 0x8e20e8: the framework's Rand()); a seed of 0 becomes 0x1105, as the framework's does. The
test command `seed <n>` seeds the framework's generator, so the board made after it is the same each time. Classic's NewGame
draws nothing before the fill: the background index is set to -1 and 0x749c20 takes the next one (index 0, which the table
at 0x8d2170 maps to the 11th name of the table at 0x8d2118, lion_tower_cascade), with no draw.

First fill (slot 203, 0x763e90, argument 0): it returns at once when the board's +0x2ef8 is not 0 or any piece has a busy count.
Then passes over the 8 columns, left to right, until a pass changes nothing: in a column from the last row up, the first empty
cell starts a run of gaps; a piece above a gap falls one row (row +1, the grid cell moved; +0x1c gets the column's delay + 1 when
it was 0), a piece with +0x20c = -1 ends the run; after the column a new piece is made in row 0 when there was a gap, with
fall delay = the last fallen piece's - 0.55, y = (the highest y in the column, as a whole number, - 128) - Next(15) - 10 (clamped
to -128 - 75 when its place on screen would be below -128), and the column's tag is set to -1. One Next(15) is drawn for each new
piece, so on an empty board the order of draws is pass by pass, column by column: the 64 pieces end in rows 7, 6, ... 0 in that
order (the first pass's pieces fall furthest). It also works out how long the columns take to land into +0xe40 (translated, below).
Then the colours: all of the new pieces (the list in making order) get flags 0 and colour -1, then each gets
colourList[Next(14)], the list at +0x105c being each of 0 to 6 twice (made by the constructor at 0x74b960); this repeats until the
grid has no run of three (0x73ba40, rows first, then columns; run counts restart at a gap or a colourless cell) and has a move
(0x75a8e0 with the first move wanted: every cell against its right, lower, left and upper neighbour, swapped in the grid, the
colour that came to the cell counted along its row and column; for the first five tries only a run of four or more or a crossing
of two counts, after that any run of three), or until 200,000 tries (a flag at +0x1190 lowers that, and it is 0). Classic's
overrides read in place: slots 126, 128, 182, 183 return 0 and 184, 185 return 1, 190 returns 5, slot 90 (0x73c410) is 0 after a
reset (+0xe04 = 0), slot 199 (0x73c870) finds a run through a piece of this update with bit 0x10, which a plain piece has not.
Slot 204 with 0 does nothing and 0x749220 does nothing for a plain piece.

The first board for seed 1 (rows top to bottom, red R, white W, green G, yellow Y, purple P, orange O, blue B; identical in two
runs of two processes): OOROOGBW BOYRPWYB OWBWRRYW GOYBGBOG OGBYRYYB YBOOWPOG GPYWWRGY BPRYPORW.

Test commands (TestCommands.cpp): `seed <n>`, `goto classic` (the start above, as the PLAY menu's ball makes it), `state`
(`STATE {"screen","mode","score","board":[8 strings],"background"}`).

(The next two paragraphs are the first reading of the drawing; "The Classic screen's drawing", at the end, replaces them.)
Draw: the Classic board's Draw (slot 26, 0x73b250) runs 0x751d50 (the panel) and then Board::DrawGameElements (0x750320),
which draws, among much else, each piece by slot 237 (0x757df0). A plain piece (no flags, nothing running on it) is
IMAGE_GEMS_<colour> (resource ids 0x310 to 0x316: red, white, green, yellow, purple, orange, blue; 5 by 4 cels of 128 at
1200) drawn at ((0x20b + x), (0x4b + y)) x art / 1200, the cel from the piece's own spin curve. The score is Board_DrawScore
(0x746f20): FONT_SCORE (id 10), the number through CommaSeperate (board +0xd64), at (250, 202).

Left out: the checkered panel of 0x751d50 (a dark square for every other cell of the 8 by 8 grid, two colours made at the top of
that function, the side glows 0x712c90 and the score bar), the background widget (Sexy::Background, 0x310 bytes, made by
0x749c20 from the table of .pam names at 0x8d2118: a PopAnim with worker threads, added to the widget manager under the board),
the saved game of the mode (0x74e1f0, read through slot 88), the
camera setup of NewGame (0x73e440).

### The update and the fall (Update 0x759170, UpdateTick 0x767090, UpdateGame 0x767e20, gravity slot 214 0x73dd70)

Board::Update eases the game speed (0x8d2da8, 1.0 from the exe's data, wanted 1.0 while a game is played) and adds it to the
board's step fraction (+0xf0); each whole step is UpdateTick: WidgetContainer::Update (the update count, mUpdateCnt) and
UpdateGame. At speed 1 that is one step per update. UpdateGame steps the shine and cover curves, counts the game ticks
(+0xe38), runs the fill (slot 203, which does nothing while a piece is busy), then gravity, then the column table (+0x1f8: each
entry +0.275 per step, capped at 0), then the fill again, the settled check and the idle shine, and counts +0xe40 down.

Gravity, column by column from the last row up: y += speed; above its row the piece speeds up by 0.275 times the fall factor
(slot 117, 0x756390: (curve at +0xe58 - 1) * 0.65 + 1, 1 here) unless the piece under it is within 128, when it is put
there and takes that piece's speed; at its row it is put in the row and its speed is zeroed (a speed of 2 or more counts as a
landing). Landings play SOUND_GEM_HIT at most every 8 updates, panned by the landings' mean x. The fill's landing estimate
(+0xe40) is the most over the new pieces of sqrt(2 * distance / (factor * 0.265)) in steps. The board is settled (0x75fca0,
0x755ad0) when +0xe40 is 0, every piece has speed 0, is in its cell, can fall and has no busy count, and no match exists.

Sprite: the exe does not spin a plain gem in the 2D path: it draws from a pre-rendered 15-frame table per colour (app +0xb5c),
frame 7 at rest, so cel 0 of IMAGE_GEMS_* is kept and the piece is drawn at its own (x, y).

Random draws the update makes, in order: none while pieces fall. In every step in which the board is settled, the app's Rand()
(not the board's MTRand) is drawn up to three times for the idle shine: first one (< 0.01 starts it when none ran and the update
count is at least 0x32); only when that does not start it, a second (against 0.003 times the shine rate, shine off); only when
that does not, a third (against 0.0006 times the shine rate). The board's MTRand is drawn only by the fill (Next(15) per new
piece, Next(14) per colour).

Left out, each with its address in a port: note in Board.cpp: the dialog holds, saved-game and replay branches, the effects
widget, the intro (slot 228), the pieces' own idle curves, the hint, the bars and the fades; the move's own leftovers are listed below.
The `state` test command also prints update, moving (pieces not settled), landticks, ysum and speedsum.

### The player's move, the matches and the score (BoardMoves.cpp)

Input: the exe overrides the four-argument MouseDown and MouseUp and the two-argument MouseDrag (mouse pixels divided by
art / 1200 give the 1200ths the pieces use). MouseDown (0x73f140) needs the board to have settled once (+0x1190): a click on
a gem that can be swapped picks it up (+0x23c); with a gem already in hand, a click on a neighbour swaps them (CanSwap 0x75fae0:
both swappable, both settled, no bit 0x100, next to each other), a click elsewhere puts the first down and picks up the one
clicked. MouseUp (0x73d990) remembers the gem in hand (+0x27e0); a second release on the same gem puts it down. MouseDrag
(0x73c510) with the button held and the mouse 40 1200ths away from where it went down swaps the gem in hand with its
neighbour in the direction it moved most. A swap is queued (list +0xc60) and begun in the next UpdateTick (0x75ee20): a move
record gets the board's id counter (+0xe1c) and both gems take it as their tag (+0x188).

The swap (0x759ba0): curve string 0x85efb0 (30 updates, output 1 to -1) puts the gems on a line between the cells; at its
end they change cells and the board is searched; with no match the swap goes back along 0x85fe00 (45 updates) and they
change cells again. A script's `swap <col> <row> <up|down|left|right>` asks for the move the player would make; `state` also
prints points (+0xd24), matches (live match groups) and swaps (running swaps).

Matches (0x75af60, run first in UpdateGame): two passes, the first down each column, the second along each row (the exe's outer
loop is the first index of its grid expression). A cell takes part when its piece is at rest (0x755ad0: speed 0, in its cell,
not held by a swap, can fall, busy count 0, the piece's swap curve (+0x198) at 0, no bit 0x2000) or was
matched earlier in the same search (its remembered colour, +0x21c, counts). A run of 3 or more of one colour takes the next id
of the board's counter (+0xe1c) at once, kept or not, and is one match of its own: a crossing is two matches sharing a gem,
each scored on its own. A run is held for a later update when, from any of its gems, the 7 cells on each side along the run
(of the gem's colour, contiguous) hold a free gem that is not yet at rest, or when across the run both sides together hold 2 or
more such same-colour cells and one is such a gem. A kept run's gems take the highest tag (+0x188) and the run's id (+0x190).

Match groups (0x789a90, 0x75dee0, 0x74c570; translated in BoardMoves.cpp as MatchGroup, UpdateMatchGroups, MakeMatchGroup and
ClearPiece). A group is an effect of 0x2e8 bytes in the list at +0x125c, kept for a piece (its id is the group's +0x34, found by
0x73c9d0). It is NOT made for a plain match, which was first thought: the matcher (0x75af60) makes one only at 0x75bb0f, and the
asm before it (0x75bac5, 0x75bace) takes that path only for a piece with bit 4 (a flame gem, set by 0x7494d0) that was not matched
in this update; a plain piece goes to 0x75bcd0 (first match) or 0x75bbdb (met again in the same update), where it gets the pulse curve instead. The other makers are 0x74c570 (slot 320: a
bit 4 piece without a group gets type 2), 0x74db70 and two places at 0x762b37 and 0x762ba7. The type is 3, or 2 when the board's
+0xe8 is not 0; a piece with bit 1 makes it 6. 0x789a90 draws no Rand(); it puts the first piece's id first in the group's list
(+0x8c), sets the piece's +0x174 and puts its timer (+0x178) at 0.9, and makes two streak effects (0x789860).

0x75dee0 (every update, at the end of 0x767e20): the glow at +0x27b8 falls by 0.02 and rises by 0.05 while a group lives; then
for each group (the loop stops at a group whose age, +0xc, is below 25 times the groups before it): 0x78a390 adds one to the age,
steps the group's curves and its streaks and takes 0.01 off the fade at +0x2e0. Below type 7 (0x7870f0, the hypercube's own
update, is only run for type 7; it sets busy 1 at 0x78721d on pieces whose timer passed 1, as 0x75dee0 does, and is not
translated), the group's pieces from the second on take 0.015 on their timers (0.01 when slot 0x1c4 is true: 0x73c1f0 returns 0,
Zen's and Butterflies' boards return 1) when their +0x182 is set. At 1 or more (the exe's test is >=, not >) a piece is let go
and leaves the list: unless +0x17c holds it, it takes the larger tag (+0x188), the group's run id (+0x190), the first piece's id
(+0x234) and the group's flags (+0x238), and gets busy 1 (UpdateGame destroys it in the next update) unless slot 320 takes it,
or 5 with bit 0x10. The group's ring timer (+0x4c) falls by 0.01 and at 0 makes a ring (0.1 later; 0.15 for slot 0x1c4):
for the directions of the type (0 along the row, 1 along the column, 4 all eight, 5 the border of a square, the rest the four
sides, type 6 three wide) the cell at the ring's distance (+0x48, from 0) from the first piece is looked at. Its cell goes to the
group's cell list (+0xa4) with timer 0.01 (entries leave at 1; slot 0x314, 0x73bb40, returns at once), and the piece there, if
no clear holds it and no group's list has it, keeps the group from finishing and, with a timer of 0, joins the list with 0.01 and
makes 20 particles (0x729c20 type 0xd: 3 draws in its constructor, 4 in the ring, 7 each, 140 per piece; DrawRingParticleRandoms).
The ring's distance counts up to the group's farthest (+0x14: 3 for type 3, else 7: the asm is (type != 3) * 4 + 3, so the other
way round from what was first read). From then on, each ring with every piece in the list already let go clears the first piece's
bit 4 and makes it busy 1 (slot 320 takes it when it has bit 2), sets the board's +0x1240 to 1, takes 0.25 off the group's life
(+0x30) and sets the speed of EVERY piece on the board to 0; at life 0 the group ends (so about 50 updates for type 3, about 90
for the others), as it does when its countdown (+0x2e4, which nothing translated sets) runs out. When the last group ends,
0x75dee0 asks for the fill (slot 203 with 1, unless +0x12f8 is set). Left out, each marked `// port:` in BoardMoves.cpp: the
streaks and their own update (0x786dd0, which draws Rand() every fifth update and more with the streak's length: the draws are
missing for every group), the sounds, the curves, 0x73d170's counters, and the hypercube's type 7. No piece in this port has
bit 4 (0x7494d0 makes it for a crossing: see "Special gems" below).

What clears a plain gem (found by following 0x75bbdb and 0x75bcd0 in 0x75af60 to the end of 0x767e20): no match group and no busy
count. A plain gem matched for the first time in an update (0x75bcd0) gets a pulse on its scale curve (+0x20; the curve is the
constant 1 from the constructor 0x720120): 0x85f07c (3 updates, 1 to 1.2) when a gem of the run was swapped in this update (+0x214 is
the update count), else 0x85f0a4 (30 updates, 1 to 1.22; slot 0x204 is 1, 0x85f0d0 with 15 updates would follow from a 0), and its
step (+0x28) is multiplied by the fall factor (slot 0x1dc, GetFallFactor). Nothing else is set on it. In 0x767e20 (asm 0x768f7b to
0x7690b8, after the busy-count loop and just before the call of 0x75dee0) every piece's scale curve is stepped (IncInVal); only when
the curve is not running: above 1 it becomes 0x860574 (20 updates, 1.2 down to 0), at exactly 1 it is set to the constant 1 again, and at 0
(the shrink has run out) with the piece's swap curve (+0x198) at rest slot 0x2c0 (0x755b80, RemovePiece) takes the piece from the
board. So a plain match lasts 30 plus 20 updates (3 plus 20 after a swap), the gem stays on the board meanwhile (0x73ba40 still sees the
run, so the board is not settled and the fill waits for the gap), and it gets no busy count and no 0x74c770: no 50 points for the gem
(the match's own 50 from 0x75d86f is the whole score) and none of the 432 draws below. DestroyPiece now runs only for what has a busy count.
Translated: Piece::mScaleCurve, the first-match pulse in FindMatches, UpdatePiecePulses. Slot 0x298 (0x745ca0) only sets bit 1 of a
piece's flags (0x745ceb) and 0x7494d0 (slot 0x2a0) is the flame maker (it sets bit 4, with 0x749305's flags), as the match group
paragraph says.

Score: every call of AddPoints is slot 152 (0x260) of the board's vtable. For plain gems there are two: the match (below, 0x75d86f) and
the destroy function (0x74c87e): 50 for each gem without the bits 0x1800. The others belong to the special gems (0x73d380 250,
0x73f8e0 and 0x74c070, 0x749650 1000 times a count, 0x767e20 300, 20 and 1000 times a level count). A match scores (n - 3 + k) * 50, n the gems, k the matches the move and its cascades have made so far (at least 1),
and 350 * (n - 4) more from 5 gems (0x75d86f); AddPoints (0x74a210) takes it times the level number (+0xe0c, 1) in steps of
10, each times the level index + 1 (+0xe04, 0), into +0xd24. The shown score (+0xd64) follows it every fourth update by a
fifth of the distance, plus or minus 1 (0x73b6d0).

Random draws, in order: the swap, the match search and the score draw nothing (AddPoints makes a points text, 0x71f030 with the
effect 0x71e5f0, which draws 5 times in a loop; not translated, so those draws are missing). Destroying a piece that has a busy count (0x74c770, not a plain match: see above; after its 50 points) draws the app's Rand() 432 times: 14 sparks of type 0xe (the constructor 0x729460 draws 2, the loop 5),
25 of type 0xb (constructor 2, loop 8), 14 of type 0xe (constructor 2, loop 4); a 0x712da0 draw is one Rand(). The count holds for
a gem at rest (so its place equals the caller's and the 25 sparks take the branch without extra draws), slot 0x1c4 false and a
match timer of 0 (a flame gem's clear; a plain match does not reach it); a gem with a timer above 0 makes 15 more sparks of type 10. DrawSparkRandoms makes these draws and drops the
values. The fill refills with the board's MTRand as before (Next(15) per new gem, Next(14) per colour); the idle shine draws
as above.

Special gems (BoardMoves.cpp: FindMatches, MakeFlameGem, MakeStarGem, MakeHypercube, Explode, MakeHypercubeGroup,
UpdateHypercubeGroup, UpdateBusyPieces, StepSlideCurves, UpdateSlidePositions). Naming correction: the paragraphs above call bit 4
"a flame gem" and say the exe makes a flame, a star and a hypercube; that was a guess before the makers were read. By what the exe
does with the bits in Classic: FLAG_FLAME (1) goes off as a 3 by 3 explosion (0x73f8e0) and is made by slot 0x298 = 0x745ca0 for a run
of 4; FLAG_HYPERCUBE (2) clears a colour and is made by slot 0x29c = 0x749570 for a run of 5; FLAG_STAR (4) makes the match group with
rings along its row and column (the group paragraph above) and is made by slot 0x2a0 = 0x7494d0 for a crossing (slot 0x174 is true in Classic, so a crossing makes a star gem; where it is false the exe calls slot 0x298). A run of 6 or more makes both
FLAG_STAR and FLAG_FLAME. The other bits (0x10, 0x80, 0x400, 0x800, 0x1000, 0x2000, 0x8000, 0x80000) are made only in other modes and
no Classic path sets them; the code that tests them is kept in the exe's shape with a `// port:` note.

The makers (0x745ca0 and 0x7494d0 need a colour, set the bit, clear the bits of the mask 0x2e0 and start the countdown +0x184 at 25;
0x749570 sets the flags to 2, stamps +0x210 with the update, copies the colour to +0x21c, sets the colour to -1 and starts the
countdown at 25). Their effects (types 0x16, 0x17, 0x14) draw no Rand(). The countdown (+0x184) is lowered by one per update
(UpdateGame, after the column speeds) and while it is above 0 a clear of a flame or star gem leaves it standing (Explode and the
busy loop test it).

The matcher (0x75af60), rewritten in the exe's shape: the scan is per cell. A cell's colour counts when the piece is "free" (its scale
curve is not a pulse, ramp 6; no swap holds it; mCanFall; busy 0; the slide curve (+0x198) at 0; no 0x2000) or was met earlier in this
call; the colour is the remembered one (+0x21c) when +0x210 holds this update. A run that ends with 3 or more takes an id
(mMatchCounter++) even when it is held; a run is held when any gem of it is not at rest (0x755ad0) and was not met in this call, or sits
in a held run (set local_224), or the neighbour test of 0x75b4d0 to 0x75b6f1 (RunIsHeldBack: a same-colour neighbour in the 7 cells that
could still settle, which excludes a gem whose scale curve is a pulse and a gem that slides). A kept run: the id and tag go on its gems;
a gem with bit 1 is listed to go off; a gem with bit 4 not matched in this update gets a match group (0x789a90) and takes no more
part; a gem matched earlier in this update (the second visit of a crossing) is put in the sets of targets and of gems met twice and
gets the scale constant 1; any other gem pulses as above, is stamped, and a plain one is listed by its last move (+0x218) and by place.
The centre of a run (the plain gem that moved last, the middle one of those, or the last plain gem with the colour on both sides across
the run) and the plain gem two steps from it towards the longer side are noted for a run of 4 or more with the tag; the exe has the
same record for its branches of 6 or more, 5 or more and 4 or more. After both passes: the listed bit 1 gems get busy count index + 1,
themselves as destroyer and the scale constant 1; each gem met twice goes off at once when it has bit 1 and then becomes a star gem; each
candidate makes its special on the centre (the neighbour when the centre is a star gem) if that has no flags: a run of 4 a flame gem, 5
a hypercube, 6 or more a star gem and a flame gem; each cluster freezes (scale 1, mCanFall off) when a gem of it has bit 4 and was not
matched in this update, else sends its plain gems to the special's cell (scale curve 0x85f138 and slide curve 0x85f164, both times
the fall factor; target cell in +0x208 and +0x20c) and scores (X - 3 + n) * 50 plus (n - 4) * 350 from 5 gems. UpdatePiecePulses now
removes a gem only when its slide curve is not running either, and UpdateSlidePositions (0x769189) puts a gem that slides on the line
to its target. 0x755ad0 (PieceIsSettled) got its full test.

The busy loop (0x767e20, row by row): a gem whose count goes 1 to 0 explodes (20 points) when it has bit 1 or has bit 4 with the
countdown above 0; else a gem with bit 2, 4, 0x400 or 0x10 goes to slot 320 (ClearPiece, no clearer) and is left when that takes it;
else 300 points for a bit of 0x1800 and DestroyPiece. Explode (0x74d940 with 0x73f8e0): the gem's middle, then the 8 neighbours
and the centre last. For each neighbour that is not busy the app's Rand() is drawn 474 times (14*10 + 25*10 + 14*6, all with speed 0;
DrawExplosionRandoms). A gem whose countdown is 0 takes the larger tag and the centre's run id; a neighbour with bit 1 gets busy 15 and a
chain; any other without bit 0x10 is cleared by 0x74c570 or, when that does not take it (or it is a flame gem only), removed at once for
20 points.

The hypercube: swapping it with a gem that can be cleared does not swap (0x75ee20 ends in StartHypercube, 0x740f20): it makes a match
group of type 7 (MakeMatchGroup; no 0.9 timer, colour -1, then the partner's colour). When it is cleared by a match, 0x74c570 makes the
group with the clearer's colour, or the remembered one, or the board's MTRand % 7 (not the app's Rand()). 0x7870f0 (UpdateHypercubeGroup)
picks the gems of the colour one at a time: with the draws from the board's generator (a pick when no bolt is on, or 1 in 0x14 /
(picked + 1) + 5 (half for no colour)), the source of a bolt from the newest picked gems, else from the picked ones, else a random
candidate; a bolt lives about 84 updates (0.012 a step) and draws 40 app Rand() every fourth update of the group. The group ends 50
updates after nothing is busy, picked or left, and the fill follows. The normal swap now sets +0x218 (0x75f8fc) as the exe does:
the gem moved by the player gets the update count, the other one the count less 1; the centre of a run follows it.

Test commands for these (TestCommands.cpp; `docs/TEST_COMMANDS.md` is the coordinator's): `board <8 words of 8 letters RWGYPOBR>` puts
those colours on a settled board and takes the specials off; `put <col> <row> <letter> [flame|cube|star]` colours a gem and makes it
that special through the real maker, then lets it go off at once (countdown 0); `state` also prints `specials` (per cell f flame,
c hypercube, s star, x star and flame), `matchcounter` and `pieces`. A script file is `<update count> <command> [args]` lines, the
update count the app's (the board's is 2 less); end it with `quit`.

Oddities (not chased, `// port:` in the source): (1) RESOLVED: how the exe keeps a gem whose pulse runs out of the next update's runs.
The matcher's test is "the scale curve's ramp (+0x24) is not 6" (a running pulse, RAMP_CURVEDATA), in the free test and in 0x755ad0,
and 0x755ad0 also reads the slide curve. IsPulsing is gone. (The earlier text, which looked at +0x198 only, and the reading of
0x75bbdb that led to a stand-in, are replaced by this.) (2) The help boxes of slot 0x20c (0x73c4c0, and 0x745bf0 for each special) show
a tutorial box the first time a special is made; they are made now ("Tutorials and help"). (3) The exe's sets are ordered by
address, the port's by id; this matters only where the order of the sets changes what is done (the order the candidates are made and the
clusters' targets), which it does not for the cases tried. (4) IsHeldBySwap (0x73caa0) takes a second argument in the exe (1 in the
matcher, 0 in 0x755ad0); the port has one version and the swap record's flag (+0xf9, set from that argument) is not translated. The
swap-list flag at 0x75b02b to 0x75b08b (a run is held while a swap of that kind is going) is left out too: where the exe sets it
was not found. (5) The effect objects (particles of the specials, the bolt, the explosion's text) run without their own Rand() draws
except the ones listed above; the star gem's rings draw as before. (6) The pulse and the specials are not drawn. (7) The other
bits of the flags are unreachable in Classic (list above).

Uncertain or left out, each marked `// port:` in BoardMoves.cpp: whether the exe begins a swap in the update of the click or
the next; the level bar, the end of the game, the hint, sounds, the sparks and the counters of 0x73d170 (+0xcc0 and others), the
speed bonus, the swap-list flag above, the bolt's and the explosion's drawing.

## Levels, hints and the end of the game (BoardLevels.cpp, BoardMoves.cpp AddPoints, TestCommands.cpp)

Counters (0x73d170, 0x73cf10): the board has 38 (+0xc84), each move record 38 (+0x40); `AddCounter` adds to both. Keys used: 1 points, 0x1a
matches, 15 activity (the idle shine's rate), 0 seconds (one every 100 game ticks, 0x768125). The profile's statistics (Profile::mStats) are not written, and the other 38 callers of
0x73d170 (the match search, the specials, the swaps) are not translated (`// port:` on the enum).

AddPoints (0x74a210, via slot 152 = 0x74df80): points times the level number (+0xe0c, 1 in Classic: 0x749650 is the only thing that raises it
and nothing translated calls it), in chunks of 10. Each chunk, when the call counts for progress, raises the level bar's progress (+0xe00)
by ftol((pow(base+chunk, 0.8) - pow(base, 0.8)) * 3) where base = the move's points counter over (level index + 1); then the chunk times
(index + 1) goes to the points counters and the score (+0xd24). Every call passes the piece's tag (+0x188; a match its cluster's) as the move
id. Not called yet: 0x73d380 (250), 0x74c070 and 0x749650 (1000 times the count), which are the special gems' own paths.

The level bar: the level needs 750 * index + 2500 (slot 154). The drawn bar (+0xd68) follows progress over that (at most 1) up by 0.025 of the
way plus 0.0005, down by 0.05 of the way less 0.0001 (0x755940; the curve at +0x2258 is the constant 1 after a reset, so the 0.0255 and 0.0012
pair is not used). When the bar is 1 and the progress has reached the size, slot 156 (0x74e190) sets the pending flag (+0x288c) unless a game
over, a level change or a hold is under way. With the board at rest (and no swap asked for) the overlay (HyperspaceFallback, 0x723320) is made.
Its update (0x725ef0): its count goes up, event 2 at the 200th update (the level index goes up, the progress is 0), the gems are put in a new
order at the 250th (0x7604d0, translated in full: the board's own generator, mirror, 2 by 2 turns, random swaps), and its curve (400 updates)
ends the change at about the 401st (event 5: the overlay goes). Seen: progress 2556 at level 1 -> level change at the next rest, level 2 at count
200, change over at 401. Not translated (`// port:`): the overlay's drawing, the events 0, 1, 3 and 4 (background, flame gems' look), event 5's
saves and the "Level N" announcement, the bar stage (0x75a760, music and look only), the second bar (0x755620), +0x1188.

No move: the exe, with the board at rest, no tutorial box, +0xd1c 0 and no change asked for, asks the move search (0x75a8e0, `FindMove`) and,
when it finds none, runs slot 161 (Classic 0x73b2d0): the end screen's wait (+0x307c) goes down and, below 1, is set to 0x113 and the score is
handed to the leaderboard (`SubmitScore`, 0x6728b0: in front of the first lower score, ten kept); then 0x7602c0 sets the game-over flag
(+0xd8c) and the hold counter (+0xd1c) to 1. The counter goes up each update; at 400 the end screen's wait counts down from the 0x113 (slot
233, 0x73b100), so the end dialog (id 0x26) comes 674 updates after the game over (`mEndScreenShown`; `ClassicEndLevelDialog` is made then, see "The end-of-game dialog"). Left out
(`// port:`): the announcement, sounds, music events ("Break", "Play"), the player's save, the leaderboard's flush. The file users\hiscores.dat
(0x671b80) is not written and its format was not read (Oddity); the top list is kept in the board only.

The hint (0x75ae10, 0x75aeb0): with the board at rest and slot 98 (0x73d4f0) true, the timer (+0x26a4) goes up; at 15 * 100 the hint is asked
for. FindHint draws Rand(2) (which end first) and Rand(10) (moves to skip), and tries the skips down to 0 from that end, then from the other.
The glow, tip and sound (0x748480, 0x72b180: draws of the app's Rand for a player's hint) are not made. Oddity: the timer is put to 0 here when a
swap is asked for; the exe's function that does it (0x75f852) was not identified.

FindMove (0x75a8e0): rows from the top (or bottom), columns left to right, neighbours right, left, below, above; a move counts when the swap
makes a run of 3 (4 or a crossing with the bonus flag) or the gem is a hypercube with a gem at the other cell. The fill's boolean `HasMove` is
this with no skip; the direction order matters only to which move is first.

UpdateGame now runs in the exe's order: busy pieces, pulses, slide positions, the fill (when no group lives), the no-move check, the groups,
the level change, the bar, the hint, the game-over hold, the idle shine (its Rand draws), the countdowns. Before, the second fill came before
the pulses and the shine before the groups; the match groups' ring draws now come before the shine's.

Test commands (docs/TEST_COMMANDS.md): `move` prints `MOVE {"swap":[c,r,c2,r2],"input":[{"click":[x,y]},{"click":[x,y]}]}` (the first move of
the move search, from the top; two clicks in pixels, the gem is picked up and its neighbour clicked) or `MOVE null` (board not at rest or no
move); `bot [n]` plays that move each time the board is at rest (n moves, none: until the game is over; a game the bot plays logs
`BOT GAMEOVER update= points= level= moves=`); `hint`; `set progress|points|index <n>`; `state` adds level, levelnumber, progress, levelmax, bar
(ten-thousandths), levelchange (the overlay's count, -1 pending), gameover, gameoverticks, endscreen, hinttimer, botmoves and highscores.

Results (seed 7, -fast): a bot that plays the first move never runs out of moves on its own (100000 updates: level 12, 766 moves; 1.5 million
updates: level 50, 30 million points, every level's reshuffle left a move), so a whole game is: `bot 150` from update 400 (level 4, 27720 points
at update 30000), the board put to a pattern with no move at update 30001, game over at once, the end screen at 674 updates after. The run twice
gives identical `state` output. Level-up: progress 2490 and one more move, level change started at 398 to 498, level 2 at count 200.

## The Classic screen's drawing (src/BoardDraw.cpp)

Draw order of the exe: ClassicBoard::Draw (0x73b250) runs the panel (0x751d50) and 0x750320. The panel calls slot 250 (0x751c20 at its end), which
draws the HUD: slot 249 (0x7423e0: the top widget and the level number), slot 255 (0x746f20: the score) and slot 251 (0x741440: the bottom
widget). 0x750320 then draws the frames (slot 238, 0x743ad0: slot 239 0x745880 the frame over the grid, slot 240 0x745430 the bar's frame under
it and, through slot 241 0x7540c0, the bar's back and fill) and the gems (slot 246, 0x7568a0, in passes). The "N MATCH CHAIN" and "SPEED +N" block
of 0x750320 is behind slot 91, which is 0 for Classic. The pictures of the left panel are put where resources.xml puts them (the x= and y= of the
image; the exe keeps them in globals 0x8da624 to 0x8da6a4, in the order of the images) with the panel's 400 taken off x and 250 (slot 248,
0x73b6c0) added: the top widget (IMAGE_INGAMEUI_TOP_WIDGET), the level number (level index + 1 through CommaSeperate, FONT_LEVEL, "GLOW"
layer (128, 128, 128), the middle of x 250 at y 250), the score (FONT_SCORE at y 202) and IMAGE_INGAMEUI_BOTTOM_WIDGET_CLASSIC. The frames round the grid
are centred on x = grid + 0x200: IMAGE_INGAMEUI_TOP_FRAME at y = grid - 5, BOTTOM_FRAME_BACK at + 0x41e and THIN_BOTTOM_FRAME at + 0x421
(0x713850 draws a picture by its middle).

The gems. 0x76bfc0 (run at the end of loading) makes the table at app +0xb5c: for each colour 15 MemoryImages, frame k being the colour's cel of
IMAGE_GEMS_SHADOWED (7 cels, one a colour) stretched to the scale 2 - (k + 1) / 8 (above 1 its excess is halved) in a square of the gem's size
(128 x art / 1200), or 1.5 times that for the frames above 1. The exe makes only frame 7 when 0x46f830 (3D on) is true; this port runs the 2D
mode and makes all 15 (Bej3App::MakeGemFrames, when the first board is made). 0x757df0 (DrawPiece): a gem that is not held by a swap, not turning
(+0x170 = 0) and has none of the flag bits 1, 4, 0x10, 0x800 and 0x1000 is frame Ftol((2 - x) * 8 - 1) (0 to 14) of its row, x being the scale
curve (+0x20), or 2s - 1 when it is above 1 (the inverse of the table's halving), drawn with the sprite's middle on the cell's middle. Any other gem
is cel Ftol(20 x +0x170) of IMAGE_GEMS_<colour> (5 by 4 cels) at the cell's corner, with no shadow. +0x170 (Piece::mSpinPhase) goes up by 0.02
each update while it is not 0 or the gem is picked up (0x7690d0, UpdatePieceSpins; the same loop at 0x767bbf in UpdateTick has a condition,
0x75a6e0, that is not translated) and is 0 again at 1. Seen: the pulse and shrink of a match are the table's frames, a swap shows the sheet's cel 0.

The panel. 0x751d50 fills the 8 by 8 squares of the grid with two colours alternating ((column + row) even: the board's +0xd2c (6, 6, 6, 160);
odd: +0xd3c (24, 24, 24, 160); both made by 0x74a880), lit towards two glow colours by +0x1188 ("the flash": min(1, 5 x it) is the blend by
0x712c90, a plain lerp; the glows are (180, 14u + 100, 8u + 48) and (160, 12u + 90, 7u + 40) with u = 60 x it, alpha 200 x the board's alpha, which
for u above about 10 go past 255). +0x1188 is 0 after a reset and 0x74a590 sets it to 1; the two callers (0x74c291, 0x762986) are in the special gems'
going off and are not translated, so it stays 0 and the squares are the board's two colours. It goes down by 0.00125 each update with no match
group alive (0x769e01, in UpdateGame).

The buttons (0x741840, Classic's mode 0): Bej3Button (vtable 0x86498c, ctor 0x49c490) with id 0 (Hint, IMAGE_INGAMEUI_HINT_BUTTON_CLASSIC, +0x286c) and id 1 (Menu,
IMAGE_INGAMEUI_MENU_BUTTON_CLASSIC, +0x2870), each at its picture's x= and y= with the panel's shift, a cel in size, cel 0 normal, cel 1 over and
down. The listener is 0x74bc40: nothing while a dialog (+0xd20) or the game's end (+0xd1c) holds the board; id 0 asks for a hint (slot 202 when +0x120c
is 0); id 1 opens the menu dialog 0x15 (0x681370), which is not translated, so the Menu button does nothing yet. The other modes' buttons (the
Replay and Reset ones, 0x70e880 for Zen) are not made. Seen in the port: a click at the Hint button resets the hint timer.

The background (src/Background.cpp, 0x749c20 slot 164, class Sexy::Background vtable 0x863c0c, 0x310 bytes, ctor 0x77dbb0, Update 0x77f8e0, Draw 0x77eb50):
a widget of the app's manager with z-order -1 (+0x60), not a child of the board (the board holds it at +0x2868), sized (-160, 0, 1920, 1200) in 1200ths of the art
height. It owns a PopAnim (+0x12c) of imagesP0ackgrounds\<name>\<name>.pam, loaded by a worker thread (0x77f090) and started by Update once loaded:
PlayFrame(0, true), sized 1920 by 1200, then each update UpdateF(speed) and, each time the sum of the speed (a curve, the constant 1) goes above 1, Update.
Draw runs the PopAnim's Draw, or fills black while nothing is loaded. The index (+0x2864) goes up by the step 1 modulo 19 (NewGame: from -1, so 0) and the
table at 0x8d2170 (10, 7, 5, 9, 14, 11, 8, 18, 12, 16, 2, 17, 4, 0, 15, 13, 1, 3, 6) names the animation in the list at 0x8d2118: the first game is
Lion_tower_cascade. The level change (0x769c36 and 0x760bcd) calls the setter again with 1 and the old background's slot 82 (0x77dde0, a fade). Deviation:
the PopAnim's generator is seeded with the constant 0 instead of the app's Rand() the exe draws at the first effect its animation makes (on the update its loader
thread happens to be done by), so the hints' Rand() sequence is the board's alone; the state of the bot game is the same with and without the background.

The Menu button's dialog (src/OptionsDialog.cpp; 0x74bc40 case 1 makes it, 0x681370 is its constructor with 1, 0x697460 its button listener,
vtable 0x86ebcc for the listener part): a Bej3Dialog with id 0x15 and the header of string 0x133, modal, 1000 by 704 (1200ths) at (350, 250) and then
moved to x = (grid x 0x20b + 0x200) less half its width, y = 200. In the game it has BACK (id 0, string 0x135, 300 wide, at the middle + 10) and MAIN
MENU (id 1, string 0x137, or QUEST MENU 0x136 when the app's +0xbd8 is not 0; at the middle - 310), on its lower edge; HOW TO PLAY (id 2, string 0x134)
only while the board's +0x2b44 is not -1. The listener plays the click, then id 0 starts the dialog's fade out (0x76a670), and id 1 calls the board's
slot 232 (0x756730) and does the same. Slot 232: the app's 0x7716e0 (the play menu at the main menu's +0x270 made visible, 0x689a10 sets its +0x490 curve
"b;0,1,0.006667,..." and the ball's, the music's FadeOut and Play "MainMenu"), the background hidden, the cover curve (+0x2898) set to
"b;0,1,0.02,1,~###..." (1 to 0 over 50 updates) and the zoom curve (+0x2908) to "b;1,1.2,0.02,..." , the board's +0x2978 set (UpdateGame, 0x767f85, then
steps nothing else) and slot 253 (0x73b510) disables its buttons. When the cover reaches 0 the board's update (0x759684) removes the board from the
manager and the app's +0xbe8 becomes 0. HOW TO PLAY is made ("Tutorials and help"). Not translated: the three sliders (ids 0 to 2, music, sound and
voice) and the checkboxes (ids 4 to 10: the art size 600, 768 and 1200, 3D and the others) on the dialog's face, the zoom curve, the background's fade
(0x77dde0), the Quest branch (0x76c500), the click sound of 0x76aaf0, the ball's curves and the labels' (0x689a10, 0x684660), the first screen's
own version of the dialog (0x6a3830, made with 0) and the pause the exe's board takes from the dialog (+0xd20). The port's buttons are DialogButtons
with the picture's three cels. The play menu comes back from both starts: the goto command first presses PLAY (MainMenu::OpenPlayMenu, the body of 0x6a2de0 case 2, which sets the play
menu's fade curve) as a player would, and then starts Classic. The mode balls on the play menu are not drawn (their curves are not translated).

The rest of the Classic screen's drawing, what was read and not made (each as a `port:` note at its place):
- The level bar's length. 0x755620 (slot 189, 0x2f4, gives the target, the progress over the level's size) keeps a second smoothed fraction (+0xd6c)
that follows the target by 0.025 of the way and 0.0005 when rising (0.0275 and 0.00125 while the +0x?? curve that 0x755620 tests is 0), and
by 0.05 and 0.0001 when falling, and puts two key points of the PopAnim-like object's emitter layers at (0, half the height) and (width x +0xd6c, half the
height); the particles the effect emits between them are the visible bar. The port clips the copy of PIEFFECT_LEVELBAR by mLevelBar (+0xd68, the
logic's own, 0x755940) instead; the two differ only by the easing constants. Making the key points needs the effect's layers (engines/prime PIEffect) to expose them.
- The selection. A click picks a gem up by setting the piece's +0x23c and the curve at +0x100 to the constant 1 (0x73f140; 0 again when it is let go);
what reads the curve at +0x100 was not found in the draw functions read, and the gem's turn (+0x170, 0x7690d0) is what shows. The 0x240 float and the
bit 0x200 (0x757df0, 0x750320, 0x74f130) were not found written in Classic; the glow rectangle they draw is left out. The hint's glow is
0x748480 (PIEFFECT_HINTFLASH in the +0x12c0 layer), listed with the missing Rand() draws above.
- The level change. SetBackground(1) now runs when the change starts (the exe's 0x769c36); the exe hides the new picture until the change's last
event and fades the old one (0x77dde0), and the overlay it draws meanwhile (0x724110) is not made, so the new picture shows at once and the overlay
is not drawn. The end-of-game dialog (id 0x26) is made (see "The end-of-game dialog"). The bot game's state gains one field change: "background" is 2 where it
was 0 (the level went up twice); nothing else changes.

Oddities (not translated, each with a `port:` note in the source):
- The level bar's fill. The exe's 0x7540c0 clips the resource PIEFFECT_LEVELBAR to the bar's rectangle (the back picture's size, centred at grid x + 0x200,
  y + 0x41e; slot 87, 0x73d5e0) by the drawn bar's fraction (+0xd68) and draws it. 0x755620 sets, in a second object (reached by 0x565f40, with 0x18-byte key
  points at +0xc of 0x60-byte records: a PopAnim's, not the PIEffect's), the points (0, h / 2) and (w x bar, h / 2). The port draws a copy of the PIEffect at
  the bar's top-left, clipped to the fraction, updated each Board::Update: the glow is there but only as long as the effect is by itself (about 130 pixels at
  600), so a bar above a third does not show more. What 0x755620 and 0x75a760 drive has not been read.
- The special gems (read this pass; the passes and the Hypercube's picture are translated, the effects are not). 0x7568a0 runs three passes
  (DrawPieces): (1) the shadow (slot 236, 0x754670: IMAGE_GEMSSHADOW_<colour>, the cel from the turn) of each gem that is not a Flame Gem and is held
  by a swap, turning, or has bits 1/4/0x10/0x800/0x1000, then the effects layer at +0x12bc is drawn; (2) every gem not held by a swap, slot 237; (3) each
  swap's second gem then its first (scale 1 plus or minus a lift curve that the exe sets only in 3D, so 1). A gem drawn from its sheet is scaled round
  its cell's middle by the scale curve times the third argument; the sprites of 0x76bfc0 are not (the exe resets the scale for them). The Hypercube
  (flag 2) is slot 235, 0x73f630: IMAGE_HYPERCUBE_FRAME and then IMAGE_HYPERCUBE_COLORGLOW (additive), cel (update count / 5) mod 60. 0x755030 (the sun
  light on gems near a Hypercube's bolts) and 0x754c60 (a shader's "Params") run only when the Graphics is in 3D (0x4322a0), not in this port's 2D; 0x745a60
  draws lens rings for the effects of the layer at +0x12c0 that have a radius (+0x60: types 3 to 6, which Classic does not make).
  The persistent looks are Effects (Sexy::EffectsManager, vtable 0x865df4, two layers: +0x12bc under the gems, +0x12c0 over them; Sexy::ParticleEffect
  0x72b180, a PIEffect copy held at +0x1c0 that follows the piece with the id at +0x28 and is run by 0x732880: its first update is ResetAnim and its second
  PIEffect::Update, which draws one Rand() for the seed; Sexy::PopAnimEffect 0x72af30, a PopAnim clone). 0x749220 picks by flag: 0x10 0x748c50, 4
  (Star Gem) 0x749050 (two ParticleEffects of PIEFFECT_STARGEM, one in each layer, "Stars" layer off in the first), 0x80 0x748ad0, 2 (Hypercube) 0x748a00
  (PIEFFECT_HYPERCUBE, under layer), 0x80000 0x748930 (PIEFFECT_BLASTGEM; the exe never sets 0x80000 in Classic: 0x75af60 treats it like bit 1). The
  Flame Gem (0x745ca0) makes two PopAnimEffects of POPANIM_FLAMEGEMCREATION (labels with "Creation_Above Gem", in the two layers; the animation has no
  "addparticleeffect"), and has no persistent effect in 2D: the gem is its sheet's cel. The hint's PIEFFECT_HINTFLASH (0x748480, for a player's hint) is a
  ParticleEffect in the layer at +0x12c0.
  Made (src/Effects.cpp, src/BoardEffects.cpp): the Star Gem's two effects (0x749050), the Hypercube's (0x748a00), the Flame Gem's two creation PopAnimEffects
  (0x745ca0) and a player's hint flash (0x748480), updated at 0x767293 / with the board's children and drawn in the passes above. The Rand() draws they make are
  PIEffect::Update's seed (0x573290), one at each ParticleEffect's second real update, inside Update and never Draw: Star Gem two, Hypercube one, hint one. They
  feed only the effects' own particles; the board's seed and FindHint draw from the same app Rand(), so a game's state is unchanged by them in the bot run
  checked (seed 7, bot 150, state at 11999: identical before and after). Left out, each noted at its place: the sounds (0x76aaf0), the hint piece's pulse curves
  (+0x440, +0x280, +0x2f0), the effects' own curves (+0xa0, +0x110), the manager's list at +0xf8, the 3D-only 0x745a60, 0x755030, 0x754c60, and the level change's
  effect (event 1 of 0x760ac0, 0x749130, flag 8, released at event 4). BLASTGEM is made only for flag 0x80000, which Classic never sets, so the Flame Gem has no
  BLASTGEM effect here. The Flame creation animation (POPANIM_FLAMEGEMCREATION) plays the label the exe gives each of its two effects: the one under the gems "Creation_Below Gem_Horizontal" (string
  0x85eea8, which 0x745ca0 pushes for the first PopAnimEffect; before, the port played its empty label and only the animation's first small sprite showed) and
  the one over them "Creation_Above Gem" (0x85eec8). It now shows the flash, the ring and the ring of flame round the new gem for about 40 updates. The
  animation's other labels ("Creation_Below Gem_Vertical") are used by no code that was read. The Flame Gem's own picture was read through
  0x7568a0 and 0x757df0 and has nothing of its own in 2D: flag 1 leaves the first (shadow) pass, takes the second pass's branch of a gem with a flag in 0x15
  (FUN_0073e330 false: IMAGE_GEMS_<colour>'s cel Ftol(cels x +0x170) at the cell's corner, cel 0 for a gem that is not turning, no shadow), and the
  things 0x74f130 draws are for flag 0x200 (set by 0x73ccc0, whose only caller 0x6c60b0 is another mode's) and by 0x745a60, 0x755030 and 0x754c60 only
  in 3D. A Flame Gem therefore has no burning look in this port's 2D beyond its creation animation; the exe's 3D renderer was not read for one.
- (kept) the gems' other pictures: the Flame Gem's PIEffect (PIEFFECT_BLASTGEM), the Star Gem's and the Hypercube's pictures and effects (0x745a60, 0x755030,
  0x754c60, the flag bits 4 and 0x2000 at the end of 0x757df0), the selection and hint glows (+0x240, bit 0x200), the blend of two cels round a turn (a PIEffect
  "Params" when the board's alpha is 1), the pass order of 0x7568a0 (plain, special, flame gems with the effects between) and the piece's alpha curve
  (+0x90, constant 1). Gems are drawn in one pass in the grid's order.
- The panel's slide in and out (curves +0x2980 and +0x2a60, the whole panel and its HUD move by them), the picture behind it while +0x2bb8 is not 0, the glow while +0x27d0 is
  above 0, the y shift +0x1194, the glows of the frames while the bar's stage is on (slot 218), the overlays 0x7501c0 and 0x751500 (the buttons drawn by hand), and
  the hover curves of the buttons (+0x150, +0x158).
- The level change's overlay (HyperspaceFallback, 0x724110) and the game over screen (the end dialog, 0x26) are not drawn: only their state is translated.
- The Menu button (above), the click sound (0x76aaf0) of the buttons, and 0x742db0 (run when +0x1188 comes to 0).

## The end-of-game dialog (src/EndLevelDialog.cpp, ClassicEndLevelDialog 0x7842b0 over EndLevelDialog 0x6a7c80, vtable 0x86c01c)

Made by `Board::UpdateGameOver` (slot 233, 0x73b100) once the wait has run out and the app has no other dialog; the board's counters,
score (+0xd24), level index and the rank of the score just submitted are copied into it (+0x2c8, +0x2c4, +0x378, +0x380). It is a modal
Bej3Dialog, id 0x26, 1600 by 1200 (1200ths) at (0, 0), `mMouseDownAllowed` false, bottom inset 0x3c; its Update (0x784340) runs only while
no dialog 0x27 is up, and steps the score's count-up curve (+0x250). Buttons (Classic lifts all by 0x28): MAIN MENU (id 0, string 0xe3, the
small button picture, 0x118 wide, right of the middle) closes the dialog and runs the board's slot 232 (`LeaveToMenu`); PLAY AGAIN (id 2,
string 0xe5, the large picture, 0x168 wide, in the middle) closes it and runs slots 85 and 135 (`ResetGame`, `NewGame`); BADGES (id 3,
string 0xe4, left) opens the badges dialog 0x2a, which is not made, so it does nothing. Draw (0x6a87a0): the dialog picture, the stamp,
"Final Score:" (string 0xec) and the counting score in the huge font with layer colours, then 0x7839f0: Statistics (0x6a6470, 0x6a60d0,
0x783a80, 0x783f00: level, best move, longest cascade, time as "%d:%02d"), Top Scores (0x6a7840, 0x6a68b0: five rows, the new score's row white
with a glow of alpha 215 + 40 sin(0.07 n)) and the three special gem pictures with "x %d" (0x6a70f0). Left out: NEW QUEST (id 1, Quest
only), the rank bar widget (0x682f90), the music queue of PLAY AGAIN, the font's character filter on names (0x712fa0). Counters 0
(seconds), 17 to 19 (special gems made) and 24, 25 (longest cascade, best move) are written by code that is not translated, so they show 0.
The leaderboard (`mHighScores`, `mHighScoreNames`, `mLastScoreRank`) is the port's own list of ten; the name is the player's.
Shown by script: `set points`, `board` with no move, a wait of about 1000 updates, then `click 401 531` (PLAY AGAIN) or `click 563 531`.

### The Lightning end dialog (EndLevelDialog.cpp, LightningEndLevelDialog; the exe's SpeedEndLevelDialog, vtable 0x86d034)

Made by the board's slot 233 (0x716ab0, in the port Board::MakeEndLevelDialog): the same dialog as Classic's (0x6a7c80 builds it: id 0x26,
the same three real widget buttons Main Menu id 0, Play Again id 2, Badges id 3 (a stub: the badges dialog is not made), none that leads out of
the game), with the board's +0x30b0 (the seconds of all the time gems) at +0x38c and its multiplier (+0xe0c) at +0x37c. Update (0x782550)
is Classic's. Slot 26 (0x780aa0) draws Classic's 0x6a87a0 (the box, the stamp, Final Score) with the content of slot 94 (0x780130): the
statistics table and the top scores, no section labels and no shift; labels Highest Multiplier, Best Move, Longest Cascade, Total Time (strings
0x1e3 to 0x1e6; slot 89, 0x780160), values "x%d" (0x1e7) of the multiplier, the best move and longest cascade (counters 25 and 24) and the
seconds + 60 as "%d:%02d" (slot 90, 0x7805e0; right at 0x2ee). Then 0x780aa0 draws SECTION_GRAPH and LINES (x - 0xa0), the icons FLAME,
STAR, HYPERCUBE and LIGHTNING and the boxes YELLOW, ORANGE and PINK at their x= and y= plus the setting LIGHTNING_GAMEOVER_DLG_*_OFFSET (all
-160 in defaultUIConstants.xml), each with a label right of it at y 0x2d3 spaced by LIGHTNING_GAMEOVER_DLG_ICON_LABEL_SPACING (5): "FLAME
x%d", "STAR x%d", "HYPER x%d" (counters 17, 18, 19; strings 0x1e9 to 0x1eb), "TIME +%d:%02d" (0x1ec; the seconds), SPEED, SPECIAL, MATCHES
(0x1ed to 0x1ef), in FONT_GAMEOVER_DIALOG_SMALL; then the chart's scale "%dk" (0x1f0, 1k to 5k, 0x2e apart up from y 0x3cf, right at 0x14a)
in FONT_GAMEOVER_DIALOG_MED. Oddity: the chart's bars (one stacked bar of three coloured pieces, IMAGE_GAMEOVER_BAR_YELLOW, PINK and
ORANGE, for each entry of the round's record at +0x36c, and the "Last" / "x%d" labels under them; the scale grows past 5000 by 5000s) are
not drawn: the record is written by code that is not translated, so the chart stays empty with the scale of an empty record (1k to 5k).
The labels' OUTLINE layer is the current colour (white), as read.

### Oddities: not done in this pass (each is a read of the export started, not a translation)

- **Level-change overlay (0x724110).** Read: it is a Draw (slot 26) of the board's overlay widget: the picture's own origin is moved by
  (ArtRes x 0x780 / 0x4b0 less the widget's width) / 2, then the widget at +0xc4 is placed between its own centre (w/2, h/2 of this
  widget, +0x48 and +0x4c) and the point (0x3c0, 600) scaled, by the curve value (FUN_00437c60, `1 - t` weighting), shifted by the
  Ftol'd whole pixels and drawn through its slot 26 under a Translate. Not translated: what the +0xc4 widget is (the 3D level-up
  banner) was not traced, and the port draws no 3D.
- **Background fade at a level change (0x77dde0).** Read: it keeps two backgrounds (+0x12c and +0x138: the shown one and the next) and, while
  `+0x133` (a fade runs) is set, draws the old one onto a render target (FUN_00459c50 over the app's +0x71c, +0x720 size), shifts by
  0xa0 scaled less the app's +0x714, and the new one over it; it ends by deleting the old one (param_4). The 2D port's `SetBackground`
  swaps at once. A copy of this fade needs a render-to-image path the port's Graphics has not been checked for; not chased.
- **Menu dialog's face (sliders ids 0 to 2, checkboxes ids 4 to 10, the board's pause while it is open, +0xd20).** Not started: 0x5a26a0 (the
  slider maker) is 23 lines, but the sliders' listener (the volume, music and sound set-ups), the checkboxes' options and the places that
  read +0xd20 (0x6c4ac0, 0x6c4fb0, 0x6c56c0, 0x6d8c60 are other modes'; 0x73b640 and 0x73c330 are Classic's) were not read. The dialog is
  still the BACK and MAIN MENU buttons only.

## Zen and Lightning (ZenBoard.cpp, LightningBoard.cpp, Bej3App::StartZen and StartLightning)

The exe's three game boards derive from Board (vftable 0x864cb4, 256 slots) directly: ClassicBoard (0x86511c), ZenBoard (0x866b6c, 0x36d8
bytes, made by 0x7126f0) and SpeedBoard (0x8665c4, 0x3480 bytes, made by 0x718090). SpeedBoard is Lightning: "Speed" is its mode name
(slot 89, 0x7144a0: the song, the log, speed.cfg and speed.sav) and "Lightning" its Title. The play menu's balls 1 and 2 call the starts
0x773100 and 0x773440 (the port: StartZen and StartLightning, which share StartBoard with Classic's); Lightning first copies the settings of
the first quest of speed.cfg (0x404870) into the board's map at +0xc8 (the port: Board::mSettings), then slot 85 resets it, which reads them.
The port's Board stands for Classic's (the Classic board's slots are the ones the base translated), so each mode's class overrides what it
changes. The slots that differ from Board's, with the address in the mode's own vtable (Zen, then Lightning):

- Zen, 33: 0 (0x41d110), 22 (0x7116c0), 26 (0x710e80), 43 (0x711e70), 47 (0x70e490), 83 (0x70e880), 88 (0x70e590), 90 to 92 (0x70e350,
  0x70e360, 0x70e3b0), 100 and 110 (0x70e380, 0x70e370), 111 (0x70e340), 113 (0x70e3a0: true), 116 (0x70e390), 117 to 119 (0x70f910,
  0x70f8f0, 0x70f8d0), 122 (0x70e330: 1), 123 (0x70e320), 134 (0x41d100), 135 (0x710bf0), 139 (0x70e4f0), 154 (0x70e450: the level's size),
  157 (0x7120e0), 161 (0x70e4a0: no game over), 174 (0x70f8b0), 191 (0x70f160), 198 (0x70f2b0), 234 (0x712c60), 249 (0x70fdf0),
  251 (0x70e500), 255 (0x70e5c0).
- Lightning, 40: 0 (0x41c1d0), 22 (0x7173f0: update), 26 (0x716c30), 43 (0x716ef0), 47 (0x714120), 85 (0x718500: reset), 86 (0x7141b0),
  87 (0x714400), 88 (0x7144d0), 89 (0x7144a0: "Speed"), 104 (0x7140f0: 60 seconds), 105 (0x714150), 106 (0x714110), 122 (0x714090: 5),
  123 (0x714080), 124 (0x714500), 127 (0x7140a0), 131 (0x714420: the help boxes), 138 (0x714890), 139 (0x718fe0), 143 (0x715360: a piece
  is cleared), 152 (0x715300: a thunk of Board's 0x74df80), 154 (0x7140d0), 155 (0x7140b0), 156 (0x714280), 158 (0x7151e0), 161 (0x719350),
  172 (0x714100: 0x82), 182 (0x7143a0), 183 (0x714dd0), 185 (0x716c60: the fill's time gems), 186 (0x7183e0), 187 (0x713fa0: the time left),
  188 (0x716f70: the bar's target), 218 (0x714130), 233 (0x716ab0: the end dialog), 238 (0x714840), 241 (0x715940), 250 (0x713f90),
  255 (0x714660).

Translated and overridden (marked `override`, each with its slot and address in the comment): Zen 89 (the Title), 113 (IsSlowMode), 122,
154 and 161; Lightning 22, 85, 89, 104, 122, 143, 154, 155, 161, 172, 185, 187 (GetTimeLeft), 188. The hooks they needed in Board are
virtual there (the translations of slots 85, 135, 154, 161, 171 and 172 became virtual): IsSlowMode (slot 113: the match groups' timers,
the explosions' busy counts and the sparks take the Zen branch), GetLevelFactor (122, in AddPoints), OnPieceCleared (143, from RemovePiece),
OnFillAccepted (185, from the fill), GetTimeLimit (104), GetBarTarget (188, from UpdateLevelBar) and GetLevelProgress (155). The end of
Board::NoMoreMoves (0x7602c0) is Board::EndGame, which Lightning's slot 161 calls too. UpdateLevelBar's "level is full" test is slot 217's:
with no time limit the bar is 1 and the progress the level's size; with one (Lightning) the size is above 0 and the progress has reached it
(never with the shipped file, which has no goal).

### Zen

No game over. Slot 161 (0x70e4a0), called where Classic ends the game (the board is at rest with no move): it draws the board's own
generator twice (the column, then the row, each & 7) and the gem there, if any, becomes a Hypercube (slot 167, 0x749570, with its effect);
the board then has a move again. Slot 154: the level's size is 2500 + 750 for each level index up to index 29 (the same as Classic's up to
there; Classic's has no cap). Slot 122 is 1: a match's points are not multiplied by the level, and the bar's progress is of the plain score.
Slot 113 (true) puts the match groups on the "slow" branch (0.01 a step instead of 0.015) and the sparks on their own draws.
Left as `// port:` notes: the mantras (the affirmations' texts, 0x712c60), the breathing exercise (slot 43, 0x711e70: the pictures
IMAGE_ZENUI_BREATHE_IN_TEXT and BREATHE_OUT_TEXT faded by the curve at +0x3168; slot 26, 0x710e80 draws the mantra's text and the pause),
the ambient and binaural sound and slots 88, 90 to 92, 117 to 119 and 234 (the state at +0x3170 to +0x33c0 with the pause at +0x33b0): all
presentation, and nothing in it changes what the board does; none of it is part of the HUD's own state. Test: `goto zen`, `state` (mode
"zen"), `bot`.

The Zen panel (ZenBoard.cpp, drawn through the three virtual pieces of Board::DrawScreenFrame): slot 249 (0x70fdf0) draws the necklace's score
plate (IMAGE_ZENNECKLACE_SCORE at the panel's middle - 0xfc plus its x= and y=, in the board's alpha) where Classic has its top widget and the
level number; slot 251 (0x70e500) the Zen bottom widget (IMAGE_INGAMEUI_BOTTOM_WIDGET_ZEN, the panel's 400 off); slot 255 (0x70e5c0) the shown
score in the middle of the panel at y 0x7d, or "MAX SCORE" (string 0x200) at 0x7fffffff. The level bar and the frames are Classic's: the
ZenBoard does not override them, so Zen has the level bar.
Oddities (Zen): (1) the necklace itself (the rest of 0x70fdf0, about 270 lines: the pictures ZENNECKLACE_JEWEL1 to 4 (at 0x8da3a8) and
STRING1 to 4 (0x8da3c0) placed along the necklace by the level index (+0xe04), a pattern table of 4096 entries built at the first draw
(0xd25bd8) from a run of 2 to 8 counted down, and for each jewel a scale and a fade by CurvedVals (0x437c60); the position tables are the
constants at the start of the function) is not drawn: it was read as far as its structure and not worked out. (2) The Zen buttons: the exe
makes the hint, menu and options buttons from IMAGE_INGAMEUI_HINT_BUTTON_ZEN, MENU_BUTTON_ZEN and OPTIONS_BUTTON_ZEN; 0x741840 has no case
for Zen (mode 0 is Classic, 1 and 2 the Lightning buttons), so the code that makes them was not found and the port's Zen board has Classic's
hint and menu buttons. (3) The score's font: the exe draws it with whatever font the screen frame left; FONT_SCORE is used.

### Lightning

A game is 60 seconds of the board's own clock (100 game ticks a second; the clock, +0x3284, a float, starts 250 ticks late) with the time
gems the fill makes; the account of their seconds is what the Last Hurrah plays.

- The clock (slot 22, 0x7173f0): when the game tick went on, the clock goes up by min(1, 0.7 + 0.3 left / 600) (so it slows in the last
  6 seconds), or by the override (+0x3288) that a big explosion sets: while +0x1188 is above 0 and the override is 0, the override becomes
  left / ftol(flash * 800) when that is more than 1.3 times left (so the time nearly stops during a big explosion). The time left
  (slot 187, 0x713fa0) is ftol(6000 - held(clock - 250, 0 to 6000)), at most the round's length (+0x30f4).
- The time gems: each game tick the three chance counters (0x41c090) step by TimeStep (2) over 100; each new piece the fill makes after the
  first 100 ticks steps them by 1 and draws the board's generator once (twice when the first is not under the 10-second chance, the second
  being under the 5-second one); the 10-second one wins first. One more draw picks the batch's piece, which becomes the time gem (flag
  0x20000, seconds at +0x244) when it is plain. The chance is g * clamp((1.5 counter g)^1.2, 0.2, 2.5) with g the curve's value
  (5SecChanceCurve and 10SecChanceCurve of the quest; the shipped file has no 15SecChanceCurve, so that chance is 0 and never read); a
  gem resets its length's counter. Every 400 ticks +0x30e4 (20 at first) is lowered by draws (not below 5); nothing translated reads it.
- A time gem cleared (slot 143, 0x715360 from RemovePiece): counter 0x22 goes up, its seconds go to the account (+0x30ac) and the game's
  (+0x30b0), and from the 60th second of the account on each gem is worth 50 points a second (AddPoints, with the move id the piece's tag).
- The round ends (slot 188, 0x716f70, which UpdateLevelBar and the update ask): the time left is 0, the board at rest, no end running
  (+0xd1c is 0): flag +0xe28 is set and slot 161 (0x719350) runs. It first turns the time gems still on the board into a Flame Gem (under
  10 seconds) or a Star Gem when there is an account, else it gives their seconds' points. With an account: the Last Hurrah. The chances
  step by LevelTimeStep (8) and their curves move on, the multiplier (+0xe0c, which AddPoints multiplies by) goes up by 1, the round's length
  is the account's seconds, the account is 0 and the clock and the game ticks are put to (60 - account) * 100 (so the time left starts at the
  account's seconds). Without one: the time up. The score is noted (+0x3080), the time-up is shown (+0x3096), and each special gem (the bits
  0x17, 0x800, 0x1000, 0x20000 and 0x80000) gets a busy count of 25 times its place in the grid's order (150 more in the first update), so
  that they go off one by one; when none is left the end counter (+0xd1c) is 1 (0x7602c0), and 200 when the exe's curve at +0x33e0 is above 0,
  which it is once the specials were started. 0x714430 hands the score in (the leaderboard, as Classic's 0x73b2d0 does).
- No move left (UpdateGame asks slot 161 as in Classic): the same function runs, so a board with no move and seconds in the account begins
  the Last Hurrah at once, and without any it ends the game.
- State lines (`state` with the mode "lightning"): timeleft, clock (hundredths), limit, bonus, bonustotal, timeup, gameticks, the chances
  (millionths) and a grid of the time gems ("5", "a" for 10, "." for none). The suite's second script (suite_modes.txt) plays a seeded Zen
  game (bot, 150 moves) and a Zen board with no move, then a seeded Lightning game of 36000 updates through four Last Hurrahs to its end.

Not translated (the `// port:` notes in the source), none of them changes what a game does except where said:
- The Lightning screen is drawn (the HUD pass of this entry's commit); what is still not translated, none of it changes what a game does
  (the app's generator is drawn less often than the exe's in Lightning, which only the sparks and effects read):
  (1) The time gem's look is done (see "Done since", the TimeBonusEffect commit). (2) The burst when one is cleared (Sexy::SpeedCollectEffect, vftable 0x866a08,
  ctor 0x714520, size 0x3d0, Update 0x719ec0, Draw 0x713f80; made by 0x715360 with an image from the table at 0x8cf5c8[Rand() % 9 + 0x566])
  and the "+N sec" text (0x71e5f0, FONT_DIALOG_HEADER, string 0x1df); every draw of the app's generator of them belongs in the update.
  (3) The banners (Sexy::SpeedText, vftable 0x866a1c; Sexy::LightningBarFillEffect 0x865f94; Sexy::TextNotifyEffect;
  Sexy::LightningScoreBadge), all in the +0x309c object (an EffectsManager, 0x72a510), and the shake of the screen in the last 100 ticks of the
  end (two draws of the app's generator every third update, 0x7173f0). (4) The particle effects of the HUD: PIEFFECT_LIGHTNING_POWERED_MEGASHARD
  (clipped to (0x44, 0, 0xec, 0x18c), at (0xaa, 300)) and ..._LEFTRIGHT (at (0xf0, 0xf0)), whose emitter keyframes 0x7173f0 edits each update
  from the curve at +0x3290 (pow 0.7 and the like), the STEAMPULSE effect, and PIEFFECT_COUNTDOWNBAR drawn by 0x715940 (in 3D with a scale and
  rotation; in 2D the effect's own). (5) The curves at +0x3290 (the mega gem's rise and the "+m:ss": a CurvedVal that 0x719150 re-sets to
  "b;0,1,0.02,1,#.^$         ~~###" with +0x32a0/+0x32a8; here the plain share of the account's seconds, at once), +0x3300, +0x3370
  (TIMER_DEAD_LIGHTNING's alpha), +0x33e0 (the Last Hurrah's text alpha; here the flag mLastHurrah) and +0x3450 (3D: the text's swell);
  the colour of 0x73d9f0 (the red warning: its formula is reasoned from the decompile, not checked against a run: the screen's edge and
  the timer glow red under 10 seconds without an account) and the button of Reset (RESET_BUTTON_LIGHTNING, modes 1 and 2 of 0x741840).
  (6) The sounds.
  Done since (commits d1af6047 and 1a8cb2e4): the shake of the ending (0x7173f0: while +0x30a4 is 1 to 99 and the update count is a multiple of
  3, the board's place +0x40 and +0x44 are set from two draws of the app's Rand(), each (draw % 1e6 - 500000) / 500000, times 12 of 1200 of the
  art size, times (100 - count) / 100; two draws per third update, none elsewhere); the exact red warning (0x73b7c0, 0x73d9f0: it is white, not
  red, with alpha board alpha * 127 * (1 + sin(0.15 updates)) * the share of the 1000-tick window (1500 when the round is over 60 s), fading
  over 450 ticks once the time is out; the red part is the mega gem's shard MEGASHARD_RED_LIGHTNING at (0x2d, 0x22) in its translation; the
  flag at +0x12f8 is not translated); the Reset button (id 2, +0x2878, made by 0x745240 for modes 1 to 3, placed by 0x741840; it is shown, the
  black disc seen earlier was the empty socket of the bottom widget's art; it opens a Yes/No dialog, id 0x16, strings 155 to 156, and Yes
  starts the game over through slots 85 and 135; the exe also deletes the saved game, which the port does not keep).
  Done since (the TimeBonusEffect commit): the time gem's look. TimeBonusEffect (type 0x15; TimeBonusEffect.cpp) is made for each time gem by
  LightningBoard::OnFillAccepted (0x741130: place 64 and 64, added to the layer over the gems, not held by the piece) with the update 0x730de0
  (every draw of the app's Rand() in the exe's order: the spark chance and its SparkEffect, the bolt chances, the bolt's kind and points, the
  random walk of the points and the bolts' ends, and the flash's four draws) and the draw 0x72ec70 (the glow outline while the gem is not
  turning, each bolt as two ribbons by 0x729da0, the seconds number cel (seconds / 5 - 1) of IMAGE_LIGHTNING_GEMNUMS_<colour> with the white one
  over it that blinks in the last 500 ticks). The spark (type 5, SparkEffect, additive IMAGE_SPARKLET at the layer under the gems) needed the
  manager's velocity, damping, gravity and angle change and its draw of an effect that follows a piece (the piece's corner added, restored
  after). The gem silhouette table (0x83a548: 20 frames x 7 colours x 256 floats) is game data: GemSilhouettes reads it once at start-up from the
  game folder's bejeweled3.exe through the exe's section table (the web bundle keeps the whole exe in its import: web/exetrim.js has no entry
  for bejeweled3, and the table is outside .text anyway); when the exe or the table is missing one line goes to the log and every radius is 52.
  Deviations: the effect's scale on the draw while the gem's scale curve is not 1 (the gem as it is cleared: 0x71fcf0, 0x71fd20, 0x713770), the
  early return on the graphics byte +8 (0x8e171c), the 3D draws (two numbers for a bolt's points, the 0.15 bolt chance still drawn but unused),
  the layer pushes (0x58f620) and the type-0x11 flash's object (only its draws), the effect's two curves (+0x1e0 = 0, +0x250 = 1, nothing
  starts them). The bolts that jump to a neighbour (the targeted kind) are made, but the exe's test of their reach (0x731f2b) compares a
  distance that includes the grid's x (the neighbour's x is board-relative, the effect's is not), so they should last one update; the port
  repeats it. The suite's state hashes (Classic, Zen, Lightning) did not change: the effects draw from the app's generator, which no game
  logic reads.
  Oddities, read but not translated (about an hour was not enough for each; the first needs data that cannot be copied):
  - SpeedCollectEffect (0x866a08, ctor 0x714520, size 0x3d0, Update 0x719ec0 with 0x714980; its Draw 0x713f80 is empty, so it is a controller):
    it needs the exe's Bezier path class (0x59a380, 0x59a400, 0x59ad90, 0x59aef0), CurvedVal strings, a PIEffect trail (image DAT_008d735c),
    the lookup of the type-0x15 effect and the "+N sec" text effect 0x71e5f0. Read again after the TimeBonusEffect pass: the Ghidra export of
    0x715360, 0x714980 and 0x719ec0 has lost the x87 arguments of its calls (the places, the curve strings), so each needs the asm read
    call by call, and the path class and the text effect (45 calls) are not in the port; the app's generator is called directly only by
    0x715360 (one Rand() % 9 for the image of the table at 0x8cf5c8[0x566 + n]) and by 0x71e5f0 (one call at 0x71e707, in the loop of 5);
    the rest of its draws are in the callees (0x729460, the base constructor, by the effect's type; the trail's PIEffect). Not translated
    here: no partial set of draws was added, since a count that is still short of the exe's is no nearer. The sibling text of AddPoints
    (0x71f030, which makes the same 0x71e5f0 effect: 5 draws in every mode, Classic included) would change nothing in a state hash (the
    app's generator is read by no game logic), only the effects' own seeds.
  - The SpeedText banners, the curves +0x3290, +0x3300, +0x3370, +0x33e0, +0x3450 and the keyframe edits of the HUD's particle effects in
    0x7173f0 (DAT_008d8f1c, DAT_008d86f0, DAT_008d6d4c, DAT_008d6e5c): not started.
  What is drawn, in the exe's order: slot 238/250 (0x714840, 0x713f90): the bottom widget and the LIGHTNINGUIBOTTOM PopAnim translated
  by -0xa0 (1200ths of the art size); the overlay (slot 43, 0x716ef0, called from Board::Draw through the new virtual DrawModeOverlay: the
  widget manager draws a DrawOverlay only when the widget asked for it in its Draw) is 0x715cb0: the mega gem (GEM_MEGA, a clip from y 300,
  translated by (0x38, 0x118 sqrt(share) - 0x16)), "+m:ss" (string 481) at (0xba, 0x1ba), TOP_BACK_LIGHTNING at its x= and y= plus
  (grid x - 0x2ab, grid y - 0x82) (drawn as an image: the exe puts no translation on it; the earlier clones were drawn away from the grid
  because nothing had updated or labelled them), the bar's fill (slot 241: (180, 120, 70) under pow(alpha, 4), the width the level bar's
  share of the rectangle of slot 86), LIGHTNINGUI translated by -0xa0, the Last Hurrah text (string 482, FONT_SPEED_TEXT, no glow or
  outline) at (grid x + 0x200, 0x62), the timer's marker (TIMER_LIGHTNING at the bar's end + 0x40 - 2), the red warning, the score at
  (0xf2, 0xd2), and then LIGHTNINGUITOP translated by -0xa0. The four PopAnims (UIBOTTOM, UI, UITOP, GEM_MEGA) are shared resources the exe
  draws directly; each update steps them and sets their colour to the board's alpha. Their labels go by 0x718400 (the bottom, the middle,
  the top and the mega gem's get the label with a transition time; the top one only "death" or else "unpowered"): "unpowered" at a game's
  start (0), "pulse1" to "pulse5" (12) at a time gem (0x719150: the account's seconds x 5 / 60 + 1, at most 5; the exe calls it from the
  burst's curve marker, here from the clear), "energize" (15) when the Last Hurrah begins and "death" (15) when the game ends (0x719350).
  The labels' transition is PopAnim::TransitionToLabel (0x444910 in engines/prime, shared with Zuma's Revenge and additive: nothing that
  predates it calls it). The buttons (0x741840 for the Lightning modes) are HINT_BUTTON_LIGHTNING and MENU_BUTTON_LIGHTNING where
  resources.xml puts them. Test: `goto lightning` with `screenshot`.
- The round's record (slot 153, 0x745f10: a list at +0xd58 of 0x18-byte entries that only the end dialog's chart reads: three points
  figures a record, summed to size the chart).
- The form with a points goal (PointsGoalStart above 0 with a TimeStart: +0x3095): the shipped speed.cfg has none. GetBarTarget has its
  branch (half the bar by the progress over the goal); the goal's bookkeeping in 0x7173f0 is left out.
- The exact value the bar's target returns (0x716f70 leaves it in ST0; its decompile does not show it): it is the share of the time left
  here, 0 once the time-up is shown. Only the bar's look reads it.
- The curve at +0x33e0: the exe tests whether its value is above 0 before setting the end counter to 200 after the specials; its string
  and rate were not found, so a flag stands for "it was started" (mSpecialsStarted). If the curve is above 0 also when no special was on
  the board, the counter would be 200 there too (the end dialog 200 updates sooner).
- Time gems' help box (slot 131, 0x714420 and 0x73c4c0): every help is taken as seen, as in the Classic code.
- The flag 0x40000 (gems aging by +0x244 every 150 ticks) is translated, but nothing translated sets it.
- The new-player dialog (id 1) stays open over a run with a fresh user folder, so a scripted game's end dialog does not come (as in Classic's
  suite, which checks "gameover" only).

## Butterflies and Poker (QuestBoard.cpp, ButterflyBoard.cpp, PokerBoard.cpp)

The two secret modes are Board subclasses in the exe's shape: ButterflyBoard (vftable 0x867d0c, constructor 0x6dcdf0, 0x3388 bytes) and
PokerBoard (vftable 0x869364, constructor 0x6da1c0) both derive from QuestBoard (vftable 0x866fbc, constructor 0x6d9000, 266 slots) over
Board (0x864cb4). The port has a QuestBoard base class with what they share: the secret flag (+0x3184, mIsSecretMode, set by 0x774060's
fourth argument), the quest's settings (+0xc8, copied from secret.cfg), no levels (HasLevels, GetLevelMax 0, no level number in the top
widget, slot 156 does nothing), no fill once the end has begun (slot 203, 0x6dd4d0), the quest's background (slot 100: BackgroundIdx, step 0),
the move records' clean-up (0x759840, which Board::UpdateGame runs for every mode, as the exe does at 0x76818e) and the level
factor 1 (slot 122, 0x73b600; Butterflies' 0x6c5340 gives 5). The secret quests are at secret.cfg indexes 0 (Poker) and 1 (Butterflies).

QuestBoard's update (slot 22, 0x6dd8a0; the port's QuestBoard::Update): Board's update (0x759170) first, then, unless a demonstration runs (+0x1414),
the board is paused (+0x12c4) or the app has dialog 0x17 open, in this order: a first block for the secret flag clear and the hold counter (+0xd1c) at 300
(the Quest end screen, 0x6dd760); a curve at +0x31b8 copied to the menu button's +0x148 while it is changing; the counter +0x31a0 goes up; the quest's
clock at +0xd20 (set from +0x318c, 200, by 0x6ca350, the level's end, slot 156 through 0x6ca7e0 only with the secret flag clear) counts down by 1 per
update once the board is settled, with the level bar caught up at 198 (0x76ab60, dialog 0x1a1), the slot 162 call at 100 and the end screen (slot 233,
0x6ded60) at 0; the goal's slot 0xd8; the menu button's +0x65 and +0x66 copied to the widget at +0x31b0; and the counter +0x3198 goes up. In a secret
mode only the two counters and the goal's slot 0xd8 do anything: the flag is set, so the first block is skipped, and nothing starts the curve or the
clock (the clock is only set by 0x6ca350). The port names the counters mUpdatesPlayed (+0x3198: updates this game has run, put to 0 by 0x6cda70 and 0x6ddc70;
Ice Storm's 0x6e63c0, 0x6e5850 and 0x6e0450 and 0x6d6d60, 0x6db550 read it as the age of the ice's columns and chains, the port uses Board's mGameTicks for
those, the same at the normal speed) and mUpdatesRun (+0x31a0, only written; no function of the export reads it). Both are state keys (updatesplayed, updatesrun).
Butterflies is the one secret mode with a goal object (secret.cfg's Goal "Catch Butterflies": ButterflyGoal, vftable 0x86a90c); its slots 0x2c (0x401df0)
and 0x44 (0x6a9b90), which QuestBoard's slots 234 (0x6e5660) and 211 (0x6c56a0) forward to, are plain returns, and its slot 0xd8 (0x6a9aa0) adds 1 to
a counter (+0x108) when the board's slot 103 (0x73c260) is false, which nothing in the export reads; the port keeps no goal object. Poker and Ice Storm have none (their
Challenge has no Goal). Slot 22 of ButterflyBoard (0x6e3600) and PokerBoard (0x6dedd0) call 0x6dd8a0 first; the port's two inherit QuestBoard::Update, and Ice Storm's
Update (0x6e63c0) now calls it in place of Board's. The Total Time of the end dialogs (Butterflies, Poker, Ice Storm) is counter 0 (+0xc84), not one of these two.

How a mode starts: the play menu's balls 4 (POKER) and 5 (BUTTERFLIES) call Bej3App::StartSecretMode (0x774060): the quest's setting Challenge,
compared without regard to case, picks the board (BUTTERFLIES or BUTTERFLY, POKER; the exe's other challenges are not made), the settings are
copied in, the flag is set and the common start (StartBoard) runs. Unlock: the profile's tutorial flags 0 to 3 (+0x180) are the balls 4 to 7
(Poker, Butterflies, Ice Storm, Diamond Mine). The test command `unlock all` sets all four; `goto butterflies` and `goto poker` need the mode
unlocked (they say so) and go through the play menu. State keys: Butterflies has butterflies (eight strings of digits for the moves each
butterfly waits, capitals for a caught one), spiderx (hundredths), spiderto, dropcountdown, credit, spawncount, sidechance (thousandths), freed and
bestcombo; Poker has hand (each card's suit, "/" and a second suit, f, s or w for the card's type, "-" for no card), besthand (index, -1 none),
skull (-1 none), skullclock and buster (thousandths), hands, busted, counts (the kinds' counts, the first the hands a skull took), handcurve,
flipcurve and flycurve (the curves' values in thousandths), handscore and flips. `bot` and `move` work in both modes (the move search is Board's).

### Butterflies

Gems with the bit 0x80 (FLAG_BUTTERFLY) climb one row each move; the spider over the top row walks to the nearest one, a butterfly that reaches
the top is caught (its +0x20c at -1) and the spider's arrival over a caught one ends the game. Matching a butterfly frees it.
SpawnButterflies (0x6d7c70): the drop's countdown, the count of a drop (to SpawnCountMax) and the side chance (to SideSpawnChanceMax) are
stepped; while the credit is 1 or more a butterfly is made in two passes of up to 100 tries, each from two draws of the board's own generator
(+0x258): the first, over 100000 and kept as a double, against the side chance says whether the column is one of the six inner ones (a second
draw, modulo 6, plus 1) or a side one (a second draw's low bit: 0 or 7); the bottom row's gem must be settled, not a butterfly and have no flags
(in the second pass any flags but no colour). 1 is taken off the credit every try. The gem is made a butterfly by 0x7492c0 (the flags cleared,
the effects of 0x748ad0, PIEFFECT_BUTTERFLY looping over the gems, and PIEFFECT_BUTTERFLY_CREATE) and gets its countdown (the larger of 1 and
MoveCountdown less the whole part of level times MoveCountdownPerLevel). The climb is made of forced swaps (the fifth argument of StartSwap,
SwapRecord +0xf9, PendingSwap +0x10): such a swap is neither sent back when it makes no match nor counted as a move (0x74c070 and slot 191 skip
it); 0x73c7e0 counts the countdowns of all butterflies down on every matched swap that is not forced and before a hypercube's swap (0x75f85c,
0x75f8be). Scoring (slot 143, 0x6d03d0): (the game's count of butterflies freed including this one + 10 + 20 times the move's own count) times
5, then AddPoints (level factor 5). Game over (0x6e82a0): the spider over a caught butterfly's column, its walk not running, the game not over
and no tagged gem that is not caught on the board call NoMoreMoves(true). The spider is POPANIM_ANIMS_SPIDER (labels IDLE, WALK and GRAB); its
walk is the curve "b+0,1,0.005,1,####         ~~###" whose step is scaled by (row times -0.1 plus 1.8 less 0.1 times the distance); in the first 0x96
game ticks it jumps and looks 8 rows down, afterwards 5.

End dialog (ButterflyEndLevelDialog, vtable 0x86be7c, constructor 0x784c30): Classic's dialog with slot 89 (0x784400) the labels of strings 0x9f
to 0xa2 (Butterflies Freed, Best Move, Best Butterfly Combo, Total Time) and slot 90 (0x784880) their values (counters 0x1c, 25, 0x1d and the seconds
as "%d:%02d", string 0xa3); slots 22 (0x784cc0), 26 (0x784cb0, a jump to Classic's 0x6a87a0) and 94 (0x784370) do what Classic's do.

Slots of ButterflyBoard that differ from QuestBoard's (the address of the Butterflies one): 0 (0x41ec80), 22 (0x6e3600), 43 (0x6dcd00, the spider
and the HUD), 85 (0x6ceda0), 89 (0x6c78d0, the name), 98 (0x6c6bd0), 113 (0x6c5320), 122 (0x6c5340), 123 (0x6c5330), 135 (0x6e3660, NewGame), 138
(0x6caf80), 139 (0x6cec10), 143 (0x6d03d0), 152 (0x6cff10), 156 (0x6ca7e0), 161 (0x6dcd50), 172 (0x6c52f0), 180 (0x6d0390), 191 (0x6d7f40), 194
(0x6c8430, the forced climbing swaps), 218 (0x6c6b50), 220 (0x6c5300), 231 (0x6e82a0), 238 (0x6ca7f0), 256 (0x6dcce0) and 257 (0x6d0590, the end
dialog). The ones with no translation of their own are the ones the port has no use for (the save and restore, the goal's text, the HUD's numbers).

Butterflies, left out or uncertain: the exact place of the per-gem countdown (0x73c710) is the matched non-forced swaps and the hypercube swaps
(read from 0x74c070 and its callers); the exe's list at +0x26b8 (0x68-byte entries) is taken as empty; the target's slide curve in the game-over
test uses a leftover register of the exe (read as the target's own); the butterflies' colour tint, the spider's smoothed place (+0x3284, a look
only), the sounds and the combo text (+0x3298) are not translated. The spider's x in the draw is by looking.

### Poker

Every swap that finds a match deals a card (slot 191, 0x6cfaa0, with a place left among the five; a hypercube swap deals the partner's suit by
slot 179, 0x6cfa50). The suit is the gem's colour. When the two gems of the swap were matched by runs whose ids (+0x190) differ by exactly one
(and neither is a hypercube) the card has two suits and is whichever of them makes the better hand; else the card is the one of the gem whose run
has the larger id. A hypercube makes a wild card (type 1, any of the seven); a card whose gem's tag (+0x188) is the one slot 166 (0x6cab70) or
slot 168 (0x6ce1d0) kept for the flame or star gem just made is a flame card or a star card (types 2, 3). The hand's kind (0x6c80a0 with
0x6c7e20): the lists of suits each card may be are walked like an odometer (the first card turning fastest) and the first walk to reach the best
kind is kept; with more than one card the suits are put back into the cards (a wild card takes the kept suit, a two-suit card swaps its suits when
its second is the kept one). The kinds: 1 pair, 2 spectrum (no suit twice, which is also what fewer cards give), 3 two pair, 4 three of a kind,
5 full house, 6 four of a kind, 7 flush. With five cards, the board running and no end begun the hand is scored (0x6cb520): HandValues of its kind
into the score and counter 1 (0x6c7560), the eliminator's bar (+0x3984) up by HandBuster of the kind (only with a skull on the board, to at most
1), counter 0x20 for a flush; then the flame and star cards pay FlameBonus (100) and StarBonus (250) straight into the score, one at a time
(0x6dedd0); then, with the skull's level (+0x398c, -1 for none) below the hand's index, the hand is cleared (0x6cdce0) at once when the bar is 1
and otherwise after the cards have gone; with the skull's level at or above the hand's index the skull takes the hand: 0x6cb430 flips the coin and
after its curve the game ends when it was lost. The coin is never lost for a player whose profile has not flipped one before (profile +0xcc, the
statistic 3), always when the count of hands a skull has taken is 2, and else when the app's Rand() modulo (that count + 2) is above 0; counter 3
and the profile statistic go up. Clearing a hand (0x6cdce0, a secret mode): the skull's clock (+0x3994) goes up by HandCountdown of the kind times
(hands cleared times CountdownMultPerHand plus 1); with the bar short of 1 a clock at 1 raises the skull's level by one (to SkullMax, 5) and starts
the clock again; with the bar at 1 and a skull on the board the level goes down by one, a skull is counted as busted, and the clock and the bar
start again; the hands cleared go up and the cards go. A game's first clear is made by slot 256 (0x6cf440) when the board first stands still. While
a hand is being paid nothing is matched (slot 201, 0x6d90a0, asks slot 98, 0x6c6470, which is false then): a swap in that time finds no match and
goes back. The grid is at y 0x5a (slot 172, 0x6c4d00). Matches are scored by Board (slot 143 is not changed), with level factor 1.

End dialog (PokerEndLevelDialog, vtable 0x86c4c4, constructor 0x782cf0, made by slot 257, 0x6d1540, with the name "Poker"): slot 94 (0x782580)
draws only the statistics table (slot 92) and the top scores (slot 88), slot 89 (0x782870) the labels of strings 0x191 to 0x194 (Best Hand,
Number of Hands, Skulls Busted, Skull Coin Flips), slot 90 (0x7825b0) the four values at x 0x2f8, slot 26 (0x782e10) Classic's drawing and a chart
of the seven kinds' counts (the dialog's +0x39c on, the board's +0x39e8 on); slot 22 (0x7839c0) is not read.

Slots of PokerBoard that differ from QuestBoard's (the address of the Poker one): 0, 22 (0x6dedd0), 43 (0x6d90f0), 47 (0x6c5880), 85 (0x6ce510),
86 (0x6c5900), 89 (0x6c7870), 95 (0x6dd540), 98 (0x6c6470), 101 (0x6c5a00), 102 (0x6c59f0), 123 (0x6c4d60, 1.3333334), 135 (0x6dfa40), 138
(0x6cab90), 139 (0x6ce1f0), 152 (0x6c4d30, 0), 154 (0x6c4d70, 0 in a secret mode), 166 (0x6cab70), 168 (0x6ce1d0), 172 (0x6c4d00), 179 (0x6cfa50),
190 (0x6c4d50, 0x14), 191 (0x6cfaa0), 201 (0x6d90a0), 203 (0x6dd570), 220 (0x6c4d20), 228 (0x6c90e0), 244 (0x6cb6d0), 249 (0x6d4300), 254
(0x6c4d40, 300), 255 (0x6c8de0), 256 (0x6cf440), 257 (0x6d1540), 258 (0x6c4d10, 0x41) and 260 (0x6d1650).

Timing (Poker): the exe's update (0x6dedd0) waits on animation curves, and the port steps the same curves (CurvedVal) from the exe's strings: the
hand's display (+0x3728; 0x7d7c4c, 450 updates, for a flush and 0x7d7c70, 350, for the others, set by 0x6cb520), the skull's fly-away (+0x3888, 0x7d7b44,
about 75 updates, started when the bar is full and ends with the clear), the coin (+0x3ab0, 0x7d8298, 150, set by 0x6cb430) and the wait after a won coin
(+0x3b20, 0x7d7c10, 250), and the bonus throttle (+0x37a0, 0x7d7bb0, 20, one flame or star card paid per run of it, once the display is past 0.35).
Slot 98 (0x6c6470) is false while the fly-away, the display, the coin or the wait runs, so a match is only found at rest, as in the exe. The
exe's list at +0x26b8 (which the exe also asks before scoring) is taken as empty. Deviations (Poker): The flip's safe end clears the hand when the wait after it ends (the exe makes a small
widget there, string 0x180, which the port leaves out). Not translated: the cards' flight (a PopAnimEffect of 0x1e8 bytes per card), the
hand's name display and awards (FONT_POKER_AWARD), the skull's pictures and the coin, every sound, and the hand's own HUD (the port draws the five
cards, the hand's name and a line of text for the skull's level and the bar, at places found by looking, in the strip left of the grid). The
setting at +0x3968 (first read by 0x6dfa40) and Hands belong to the games that are not secret.

### Oddities (Butterflies and Poker)

- Poker's end dialog chart (0x782e10): the exe draws axis labels (step 1, or 5 times a count over 10 for larger counts, from x 0x1ae at y 0x2cb) and
  bars scaled by 920 or 910 over the largest count; the port draws seven plain bars with the kinds' names and counts. Slot 22 (0x7839c0) was not
  read.
- What the exe passes to 0x43b2e0 in slot 90 of the Poker dialog (the values) is not in the export (they come in registers): the best hand is
  shown by name, the others as counts of the board.
- Poker's hands' names are the ones of secret.cfg's comment (the exe builds a table at 0x8d222c from strings the port has not located).
- Poker's skull: a hand the skull takes is also paid first (0x6cb520 runs before the test of the skull's level); this is read from the order of
  the calls in 0x6dedd0 and is not confirmed by a game of the original.

### The suite for the two modes

suite_butterflies.txt and suite_poker.txt (seed 7, name Tester, `unlock all`, `goto`, `bot` to the game over, three state lines and two shots
each, the end dialog on screen at the last), each run twice by suite.sh: the state lines and the shots must be the same on the rerun, the second
state line must be in play and the last must have the game over and the end dialog. Their state lines have no hash of their own yet: the
modes' logic is frozen by the rerun only, and the Classic (51086c43ed2a) and modes (94f857c247d1) hashes are unchanged.

## Ice Storm (IceStormBoard.cpp)

The secret quest 2 of secret.cfg ("Ice Storm", Challenge "Inferno", the play menu's ball 6) is Sexy::InfernoBoard in the exe (vftable 0x8685ec, 268
slots, constructor 0x6e2d90; 0x6db270 is its destructor, not the constructor) over QuestBoard; the port's class is IceStormBoard. Its end dialog is
InfernoEndLevelDialog (vftable 0x8662c4, 0x390 bytes, made inline by the board's slot 257, 0x6d3180). The exe's only names for it are
InfernoBoard::RefreshMultiplierText (0x6cc010), InfernoBadge, InfernoEndLevelDialog::DrawStatsLabels (0x78fbb0) and ::Draw (0x790500); every other
name here is the port's, read from the code. The quest text says "crush ice before it reaches the top", which is what the board does: eight
columns of ice (the objects at +0x3d28, 0x348 bytes each) rise from the bottom of the grid, 0 to 8 rows.

What happens. Each update (slot 234, 0x6e5850, which runs Board's 0x767090 first; the port made Board::UpdateTick virtual for it) the ice
counts its own ticks and, after a grace of 0x96 ticks, steps (0x6e0cc0). A made column waits 0x32 ticks (cut to 0x14 when no column is moving), then
rises by a speed that is the product of the stage's fire speed (the Level rows of the quest, times FireSpeedMult, moved from the last row's by
the row's curve over the stage's time), the column's place (ColDistribution, the right half mirrored; a double column's rank is one less on the
right), its height (RowFireSpeed), the ramp that follows a multiplier and a deviation of its own (up to MaxRandFireSpeedColDelta, drawn when it
is made); a double column's speed is also multiplied by DoubleColSpeedMult, and the edge double's by DoubleEdgeMult. The pushes of the cleared gems
(MatchPushStr for each gem of the column, SpecialGemPushMod times as much when a special gem cleared it, and when a flame gem is made there; a
push decays by 0.003 a tick) are taken off that speed; when they win, the ice goes down by the difference (never more than 0.2 a tick), the part
of it that was ice (up to the height) is added to the bar's ice removed, and a freeze builds up (FreezeDurationPerNegStrength times the push, up
to FreezeMax) that slows the next rise. A column that has been over 0.5 does not go below it again unless it is crushed. At 8 the ice cracks: its
crack ticks count, and when they reach SecondsUntilLose seconds (12, shorter by the column's crack speed from the Level row) and the board is
settled the game is lost (slot 161, 0x6e0450): a death curve of 100 ticks runs (slot 22, 0x6e63c0) and the game ends (0x6d8e30, Board's NoMoreMoves).

A run of three or more gems that stand in one column (slot 211, 0x6e2250, which the port reaches through a new Board hook, OnRunFound, called
for each kept run where the exe's slot 0x34c returns at once) crushes the ice of that column when its gems are cleared (slot 143, 0x6dd580, then
0x6db1a0): the column is marked crushed, the ice it will take off the bar is its height (1 at least), 0x6d6d60 scores it (ColDestroyBonus times
the multiplier into a total the exe only keeps; with ColCountBonus a column crushed within the chain's window of the last counts as the next of
a chain, the window being the ColComboCoolDownVsCount curve's value at the chain's length, in ticks, and the list's entry for that length is added
to the score as it is) and counter 0x23 (columns crushed) goes up. A hypercube's group at work (type 7, every colour) crushes the ice of every
column it clears a gem in. 5 ticks after the crush the ice it took is added to the bar; 50 ticks after, the column goes (the exe waits for its
crush effect, the curve "b;0,1,0.01,.5"; the port counts the ticks).

The bar: the ice removed (pushed back and crushed) against MultiplierIceReq (each entry times 8, rows; 10 when the list is empty). When it is past
it (0x6e5850's last part) the multiplier (the score's factor, +0xe0c, mLevelNumber; the end dialog's "Highest Multiplier") goes up by one, the ice
holds still for 200 ticks and every column is given a reprieve: after 0x50 ticks and 5 more for each column on, a push (ReprieveStrVsRow, 0x6ceba0)
takes the high ice down; the game goes on when the pause is over and no column is still being pushed (or 300 ticks later), and each made column,
in a random order, gets a ramp (from 0.2 to 1) that starts later for each. Slot 122 (the level factor) is 2 in the secret mode, slot 106 (the
hint wait) 5 seconds (Board's 15; a new virtual, GetHintWaitSeconds), slot 172 puts the grid 0x28 lower.

Stages: the quest's Level1 to Level15 rows ("doubleCols, colCount, fireSpeed, seconds, crackSpeedMod[, curve]") are read by 0x6c6600 (past the last
row the values go on by the step of the last two); when the stage's seconds are up the next row is read and the columns planned again (0x6e5690:
with some singles and some doubles wanted the edges are doubles; with only doubles, all). 0x6e08a0 makes one column at a time, in a random
order (the CRT's random_shuffle with the app's Rand, the last candidate kept last), with edge columns put off until enough others are made, by
the stage's double count; it runs from the step when the next is due (0x6c5d30 holds the due tick to 0x4b after a crush) or when none is made.

Settings and state keys. NewGame (0x6e6a10) reads MatchPushStr, SpecialGemPushMod, SecondsUntilLose, DoubleEdgeMult, FireSpeedMult,
ColDestroyBonus, ColCountBonus, MultiplierIceReq, MaxRandFireSpeedColDelta, DoubleColSpeedMult, FreezeDurationPerNegStrength, FreezeMax, the four
curves and the Level rows. `goto icestorm` needs `unlock all` (tutorial flag 2). State keys: ice (eight heights in thousandths of a row, -1 where
no ice is made or the column is a double's second), icespan, icecrushed, icecrack (crack ticks, -1 none), iceremoved and icebar (thousandths of a
row), icemult, icestage, icepaused, iceticks, icecrushedcount, icebestchain and icechain. `bot` and `move` work (the move search is Board's);
the suite's bot stops after 30 moves so that the ice reaches the top.

### Not done (Ice Storm)

The ice is a flat translucent block (blue towards red with the crack, fading when crushed) from the grid's bottom; the exe's PIEffects of each
column (0x330, 0x334 set by 0x6c5ea0 from the position and crack), the shake offsets, the "crush", "panicfast" and "panicslow" animations, the
glow (+0x264) of the gems in the ice, the intensity curve (+0x3b60) and its effects (0x6e5850's start), the bar, InfernoBadge, the multiplier text
(0x6cc010) and the popups (0x6ce7c0), the sounds, the death's "death" animation (0x6e0450), save and restore (slots 138, 139: 0x6cfbc0, 0x6e7e40),
slot 218's danger flag, slots 108, 133 and 191's counter (+0x3cf8, which the swap hooks set to 0xf), slots 98 (0x6d3f00), 123, 125, 239 (0x6d3970), 249,
251 and 255 of the board, the end dialog's Draw (0x790500: the boxes and icons of the ICESTORM_GAMEOVER_DLG_ constants) and the tutorial dialog of the
first vertical run (0x6e2250).

### Oddities (Ice Storm)

- Slot 143's test of what cleared a gem is the bit 2 of mDestroyerFlags (+0x238), which the port's Piece calls FLAG_HYPERCUBE; slot 166 and 143 both
  apply the push of special gems, as read. Whether that bit is a flame or a hypercube in the exe's own naming was not followed.
- The crack's fraction (0x6ce540) passes through a curve object (0x4378f0 with the column's crack ticks over SecondsUntilLose) that the export does not
  show (its object is in a register); the port reads it as linear.
- 0x6e24b0, which makes columns when a stage starts (called with a bound in a register), is read as making them until StartColCount columns are
  made; the quest has no StartColCount, so none are made at a stage's start (the first appears after the grace and the 0x19 ticks of the due tick).
- 0x6c6600's curve copy for a row with no curve is not read; the port uses the constant 1 for it, so the stage's fire speed is the row's from the
  start (Level1 to Level3 have curves, from 0.5 or 0 up to 1 over the stage).
- The column's ramp (+0x218) before the first multiplier is taken as the constant 1 (not read from the constructor, 0x6e2d90).
- 0x6d6d60's ColDestroyBonus times the multiplier is only added to +0x366c (and shown in a popup, 0x6ce7c0); nothing translated adds it to the score
  (the ColCountBonus entry is added to mScorePoints and +0x3668 directly, not through the multiplier). The score's multiplier is Board's AddPoints.
- 0x6c6520's second test (a group of type 2 whose +0x40 is the column) is of a group the port never makes (match groups of type 2 are made when
  +0xe8 is set, which no port mode sets).
- The stage's time (0x6e5790) is run from the exe's Update; the port runs it in the step, the same at the normal speed.
- (Settled) The "Total Time" of the end dialog reads counter 0 (seconds), as in the other modes' dialogs. Board::UpdateGame now adds a
  second to it every 100 game ticks (0x768125, AddCounter(0, 1, -1)), so every mode's end dialog shows the time played; it is not one of
  QuestBoard's update counters (+0x3198, +0x31a0), which no end dialog reads.
- Board's UpdateGame calls slot 161 when no move is left; Ice Storm's slot 161 begins the death instead, and the port guards the start (the exe
  asks whether the death curve is above 0).

### The suite for Ice Storm

suite_icestorm.txt (seed 7, name Tester, `unlock all`, `goto icestorm`, `bot 30`, three state lines and two shots, the end dialog on screen at the
last) is run twice by suite.sh in the same loop as the other two secret modes: the second state line must be in play, the last must have the game
over and the end dialog, and the rerun must be the same. The Classic and modes hashes are unchanged by it (see the suite's result).

## Diamond Mine (DiamondMineBoard.cpp)

The secret quest 3 of secret.cfg ("Diamond Mine", Challenge "Dig", Goal "Dig", the play menu's ball 7) is Sexy::DigBoard in the exe (vftable 0x8657d4, 266
slots, 0x3280 bytes, constructor 0x737420; 0x7373b0 is the destructor) over Sexy::TimeLimitBoard (vftable 0x86a234, constructor 0x6dd3b0, which sets
+0x324c to 10 and +0x3250 to 3000) over QuestBoard. The mode's logic is in Sexy::DigGoal (vftable 0x86bbac, 72 slots, constructor 0x6c4230, called by
QuestBoard's new game 0x6ddc70, held at the board's +0x3180, its board at +0x104); the port has both as DigBoard and DigGoal in DiamondMineBoard.{h,cpp}. The
board's mode name (slot 89, 0x736e20) is the string "BuriedTreasure" at +0x3258, the music's and the log's name. The exe's names: DigGoalEndLevelDialog
(vftable 0x865c54; Update 0x736010, DrawStatsLabels 0x732c90, DrawFrames 0x7331f0, Draw 0x733af0, DrawStatsText 0x7353d0), DigBackground, DigBar (the board's
slot 43 calls DrawDigBarAnim on the goal), DiamondMineScoreBadge and DiamondMineArtifactsBadge; the rest are the port's.

What happens. The bottom of the grid is dirt: each cell is a gem the fill made that the goal turns into a brick (flag 0x10000, no colour, not swappable,
not falling, an entry by the piece's id in the goal's map at +0x228: hits left, hits at the start, kind, a random number). Kinds: 1 dirt (strengths 1 to 3 are
hits, 4 needs a powered hit, 5 is the dark rock), 2 money (1 to 3 gold, 4 to 7 diamonds), 0 artifact (the entry holds the artifact's index). A gem that
is cleared (slot 143, 0x6ad0d0) puts its cell and move id in a list; the goal's update (0x6c2ad0) hits the four cells round each (offset tables 0x8d39fc and
0x8d3a00: right, left, below, above), first a powered hit when the gem had no colour or was cleared by a hypercube (bit 2 of what cleared it), with the cell's mask,
and, when that does not break the brick (or the hit was not powered), a second hit of the same power with no mask (0x6b59f0; the mask is a per-row set of cells a
hit has reached since the update began, cleared at its start). A gem that a flame or star cleared (a clearer with bits 1 or 4, with a colour) hits no neighbours;
its blast hits what it reaches instead: a brick taken out by an explosion or a ring (slot 176, 0x6bb6a0, with slot 142, 0x6b6a80) is hit there, powered when its
clearer had a special's bit (7), and stays if it stands. Dirt of strength 4 only takes a powered hit (it goes to 1); other dirt with hits left loses one,
and the dark rock two (5 goes to 3); at 0 it is gone. Money and artifacts are collected by a hit: the value is ArtifactBaseValue times the artifact's value
(Artifact1 to Artifact46: "id, minDepth, value, text"), or for money the roll from the kind's pair of TreasureRange (min + Rand % (max - min + 1)) times
GoldValue (strengths 1 to 3) or DiamondValue; 0x6a9780 adds it to the progress (+0xe00), which is the score (slot 161, 0x737500, sets +0xd24 to it at the end;
the port also sets it each step). A block of dirt marked H in the grid with a strength hides a hypercube: when it breaks (0x6bb6a0) the piece becomes a
hypercube whose group (the most common colour in the grid) is made at once.

The rows. 0x6bbb10 makes the bricks of the rows from a start row down: the setting Grids (read by 0x744e00: a digit starts a cell, a letter belongs to the cell
before it; H hypercube (alone, a hypercube gem), S shuffle (flag 0x1000), R random, M money, A artifact) gives the rows of the table, 8 cells each (cell n is row
n / 8); with the setting MaxBrickStrPerLevel there (+0xb10), a cell that says R and every row past the table is made by 0x6b3a50, which reads the curves
(MinBrickStrPerLevel, MaxBrickStrPerLevel, EdgeBrickStrPerLevel for the first and last columns, MinMineStrPerLevel, MaxMineStrPerLevel, MineProbPerLevel,
DarkRockFrequency) at the number of rows made (+0x4f8) and the spreads (BrickStrSpread, MineStrSpread, ArtifactSpread: tables of 500 floats built from a curve by
0x784fe0 and read by 0x784e90). Each brick made counts one off the run to the next artifact (+0x404, ArtifactMinTiles to ArtifactMaxTiles); when it is due and the
brick is a random one, a random one of the row's candidates gets an artifact picked by ArtifactSpread from the pool (the artifacts whose minimum depth is reached,
not yet placed or collected, in groups by their depth, deepest first, until ArtifactPossRange allows no more, reversed). The first eight rows are made at the
start (0x6bbb10 with 0).

The scroll. When the first six rows hold no brick (0x6aae50), the board is at rest with nothing swapped and no dialog is up, the goal starts it (0x6c2ad0): with
all eight rows free the scroll is the long one (twice DigCountPerScroll rows) and its bonus the mega one; the time is given (0x736f20): MegaTimeBonus or
TimeBonus seconds are added, the time left is kept in +0x3274, and with less than 9001 hundredths left the limit is put to 90 seconds and the game ticks (+0xe38) to
0x23f0 less what is left, with more, the limit is the seconds rounded up and the ticks 200. Two curves are set ("b+0,1,0.01,1.5,..." and "b+0,10,0.01,1.5,...");
each time the second's value (rounded) changes from a number below the rows of the scroll (0x6be730), a row is scrolled (0x6be3e0: the top row goes, everything
moves up a row, a row of gems with random colours that make no match (25 tries) is made at the bottom and given its bricks by 0x6bbb10 (7); with no move left, a
hypercube is made of a random piece of the top row, up to 49 tries). When the curve is done the depth (+0x4f0) goes up by the rows of the scroll. The clock (slot 187,
0x736b50) is the limit's hundredths less the game ticks past 250 (0x73bb80), and no more than the kept time left; at 0, with the board at rest, the game is lost
(slot 216, 0x73b890: slot 161 with +0xe28 set). The gravity (slot 214, 0x736d60) is Board's, then the bricks have no speed.

Slots translated, as the port's overrides: DigBoard::Update (slot 22, 0x7374d0: QuestBoard's, then the goal's update), UpdateTick (slot 234 through 0x6e5660,
with the time's end), ResetGame (85, 0x737290), NewGame (135, 0x7375e0 with 0x6e5030: Time, TimeBonus, TimePenalty, MegaTimeBonus, then the goal's settings, the
fill and the first rows), GetModeName (89), GetTimeLimit (104, 0x6c4a30), GetTimeLeft (187), UpdateFalling (214), OnPieceCleared (143), OnPieceRemoving (176 and
142), NoMoreMoves (161, 0x737500), MakeEndLevelDialog (257), DrawModeOverlay (43, 0x737370, plainly). Shared changes: Board::OnPieceRemoving (a hook at the start
of RemovePiece, slot 176/142: true keeps the piece), Board::UpdateFalling is virtual, Board::BotMayMove (test only: the bot waits while the first six rows hold no
dirt, so the board is at rest when the goal looks; a swap in the same update would hide it), and QuestData's parser reads `Name= value` and `Name ="value"`
(secret.cfg's Artifact10 to Artifact21 and Grids have them; the parser took the whole line for a setting name). The Classic and modes hashes are unchanged.

Settings and state keys. `goto diamondmine` needs `unlock all` (tutorial flag 3). State keys: digmap (eight rows of eight characters, rows split by "/": . no brick, 1 to
5 dirt with that many hits left, g gold, d diamond, a artifact, h dirt that hides a hypercube), digbricks, digdepth (rows: the scrolled and, while a scroll runs, its
share), digtime (hundredths left), digtimelimit (seconds), digtimecap (the kept time left), digtimeup, digtreasure (the progress, the score), diggold, digdiamonds,
digartifacts, digbest (the most one treasure was worth), digrows (rows of bricks made), digscrolls, digscrolling, digdestroyed, digplaced (artifacts put in the dirt). The test
command `dig clear [lastrow]` hits the dirt of the rows up to lastrow (5) with powered hits until it is gone, and `dig hit <col> <row>` hits one brick once. `bot` and
`move` work (Board's move search; a brick cannot be swapped and has no colour).

### Not done (Diamond Mine)

The looks: dirt, gold, diamonds and artifacts are flat coloured blocks over their cells (the exe's brick effects: the records' +0x10, +0x14 and +0x1c and 0x6b54d0, the
crumble and the sparkle, DigBackground, the dig bar and its animation DrawDigBarAnim, the time bonus' text rising (the port writes "+n" beside the clock), the
flying treasure (the list +0x658; 0x6a9780's tally timer +0x3c0 is kept but nothing draws the tally), the badges, the end dialog's frames and badges (0x733af0,
0x7331f0, "Treasure Found:", "Gold Collected", "Diamonds Collected", "Artifacts Collected"), the sounds ("drill", "predrill" and the collect sounds), the
scroll's pixel offset (+0x248 and 0x6b9690: the port moves the rows at once at each step of the curve, which the exe slides). The helping hand: the power-gem
specs (+0x468 and +0x4a8, slots 184 0x736bf0 and 185 0x736b70, PowerGemThresholdDepth0, 20 and 40 into +0x3188 by the board's slot 0x414(6)); slots 81 (0x736cc0), 123
(0x736ab0, returns 2.6666667), 133 (0x736a60, goal +0xa60 = 10), 191 (0x736b10), 204 (0x736ac0) and 177 (0x736e60: the goal's top-row hidden list +0x444); the shuffle
cell S (flag 0x1000, no use in the quest's grid); save and restore; the tutorial (TutorialHeader and TutorialText); the TimePenalty (slot 263, 0x6cb130, DoQuestPenalty:
read, never used here); the leaderboard scale (HighScoreBase and HighScoreIncr).

### Oddities (Diamond Mine)

- 0x6b59f0 after the collection of a treasure goes on into the code that takes hits off a brick; for a block of money the hits left is its strength and for an artifact
  its index, so read literally a block of money of strength 3 would take 3 hits and an artifact 0. The port collects a treasure with its first hit and removes it; what
  the exe does with the rest was not followed (the flying treasure's own arrival may be what removes it).
- 0x6b3a50's dark rock: the export gives a draw and a compare with DarkRockFrequency after a strength of 4 or more; the port reads it as 5 when the draw is at most the
  frequency and 4 otherwise (the setting's comment says it is the frequency of dark rock). The reading "4 becomes 5, then 4 by chance" was also possible.
- 0x6bbb10's absolute depth of a row (the table row, the artifact pool's depth) is the rounded curve times the rows of the scroll plus +0x4f0, then the row: the sum is
  lost in the export (an x87 add) and was read from where the values are stored; the pool's depth is taken without the row.
- The second artifact set (+0x1bc) is the list of artifacts collected (the end dialog's best treasure reads it); 0x6bbb10 leaves out artifacts in it and in the placed list.
- +0x17d of a piece (0x6ad0d0, 0x6b6a80) is not a field of the port's Piece; it is read as "something cleared it" (a destroyer id or flags are set).
- The exe's brick hit has a path for a piece with bit 2 or 0x1000 (a hypercube or shuffle brick: its colour is picked at random among the grid's) that the port does not
  make (no such brick is made from the quest's grid).
- Slot 216's condition 0x2e8 (slot 186) was not read: the port asks for the board at rest, as Lightning's time's end does.
- At the start the port runs the scroll's "no move" fix (0x6be3e0's end) after the first rows, since the bricks can take away the move the fill left; the exe may not.
- 0x6be3e0 keeps the top row's pieces in a list (+0x444) that slide off the screen; the port deletes them.
- Slot 123 (0x736ab0) returns 2.6666667 and its caller was not found.
- The run to the first artifact (+0x404) is the shortest run (ArtifactMinTiles); the exe's initial value was not found.

### The suite for Diamond Mine

suite_diamondmine.txt (seed 7, name Tester, `unlock all`, `goto diamondmine`, `bot 150`, `dig clear 5` three times so that the grid scrolls, `dig hit 0 7`, three state lines and
two shots, the end dialog on screen at the last) is run twice by suite.sh in the same loop as the other secret modes: the second state line must be in play, the last
must have the game over (the time's end) and the end dialog, and the rerun must be the same.


## Quest (QuestMenu.cpp, QuestBoard.cpp, QuestGoal.cpp, MoveLimitBoard.cpp, TimeLimitBoard.cpp, Bej3App::StartQuest and ShowQuestMenu)

The first part of the Quest mode: the play menu's QUEST ball (id 3) shows the quest map, a click on a quest's gem starts it, and Stratamax (index 3 of quest.cfg: Move Limit, Clear
Gems, 120 gems in 20 moves) and the other Move Limit quests with the Clear Gems goal (indices 3, 10, 22, 31, 34) can be played to a win and a loss.

**Starting.** Ball 3 calls 0x773b70 (`Bej3App::ShowQuestMenu(bool theDeleteBoard)`): with its argument set the app's board (+0xbe8) goes; the menu (0x693760) is made at x -160, 1920 wide
and the art's height, put on the app's screen slot +0xbd8 with the focus, and the music events FadeOut and Play "QuestMenu" are queued. A quest's button calls 0x774060 with the index
(place plus 8 times the section), the app's quest.cfg and the secret flag 0 (`Bej3App::StartQuest`): the board is picked by the setting Challenge, compared upper case as for the secret
modes (MOVE LIMIT is MoveLimitBoard, vftable 0x868a84; TIME LIMIT is TimeLimitBoard, 0x86a234; the others are not made and the map stays), the quest's settings are copied into the board's
map, its index is +0x3234, and the board is started as for a secret mode.

**The map (QuestMenu, vftable 0x86df4c).** Five sections of eight quests (section n holds the quests 8n to 8n+7). The profile's +0x158 flags (`mQuestsDone[5][8]`) say which are done; a
section's state (0x6734d0) is 0 locked (the section before it has fewer than 4 done), 1 the section the player is at (fewer than 4 done), 2 (4 to 7 done), 3 (all 8). Every quest of a section
that is not locked is open (there is no lock per quest). PREV and NEXT (ids 0 and 1) turn the page round the five and keep it in the profile's +0x184 (clamped to 0..4 when read); BACK (id 2)
takes the map and the board under it away and shows the play menu; QUIT (id 3) asks the app to quit (only logged, as the play menu's). A quest's gem is the button 1000+place (pending) or
2000+place (done). The map of names to pictures (+0x129, made by 0x693760) is: "Gold Lines" and "Gold Lines Flip" Alchemy, "Filler" Avalanche, "Balance", "Hidden Objects" Sandstorm,
"Butterflies", "Dig" Gold Rush, "Inferno" Ice Storm, "Poker", "Clear Gems" Stratamax, "Clear Bombs" Time Bomb, "Wall Blast". The port looks a quest up by its Challenge and then its Goal
(the keys are of both kinds, and Avalanche is Filler with Clear Gems and must keep its own picture; the order of the lookup in 0x697d40 was not read).

**The quest path of QuestBoard (0x6dd8a0 with the secret flag clear).**
- *NewGame* (slot 135, 0x6ddc70): +0x3187 and +0x31ac (`mScoresPoints`, `mRecordsHighScores`) start as the secret flag and RecordHighScores TRUE raises the second; the goal is made by the
  setting Goal; Board's new game deals; the goal's Load (slot 53) reads its settings. MoveLimitBoard::NewGame (0x6e0170) reads Moves first (+0x3248 and +0x324c), TimeLimitBoard's (0x6e5030)
  Time, TimeBonus and TimePenalty.
- *The goal factory* (0x6ddc70, each compare independent, a later match replaces an earlier): CLEAR GEMS ClearGoal (0x11c bytes, vftable 0x86ab5c); WALL BLAST 0x6ab720 (0x260); GOLD GoldGoal
  (0x114); DETONATOR; ARSENAL; GOLD LINES 0x6afcd0 (0x940); GOLD LINES FLIP 0x6afe00 (0x944); GOLD YELLOW 0x6ae170; CLEAR BOMBS 0x6ae760; SCORE; FIRESTORM; PRESERVER (0x118); HIDDEN GEMS
  (0x11c); HIDDEN GRID (0x21c); SURVIVAL (0x118); CATCH BUTTERFLIES ButterflyGoal (0x114, vftable 0x86a90c); HIDDEN OBJECTS 0x6afb60 (0x1f80); DIG 0x6c4230 (DigGoal, 0xb18). The port makes
  CLEAR GEMS and DIG (`MakeQuestGoal` in QuestGoal.cpp); ButterflyGoal's only live part is a counter that nothing reads, so it is not made.
- *QuestGoal* (vftable 0x86a6bc, 72 slots; `QuestGoal` in QuestGoal.h): the base's slots that the translated goals reach: Load (53), Update (54; the base counts a counter that nothing
  reads), CanEndGame (10, 1), GetLevelMax (22, 0), OnPieceCleared (40), OnPieceRemoving (18 and 39). DigGoal derives from it (`class DigGoal : public QuestGoal`, with Load, Update,
  OnPieceCleared and OnPieceRemoving as overrides; its board is `Dig()`), so the goal the factory makes for "Dig" is the same object as before and the Diamond Mine suites' states are
  unchanged. The board delegates to the goal where the exe's slots do: 154 (GetLevelMax), 143 (OnPieceCleared), 176 and 142 (OnPieceRemoving), 22 (Update), 161 (the goal's slot 10).
- *ClearGoal* overrides three slots: 53 (0x6ae510) reads Gems, GemsPerLevel and GemsMax; 22 (0x6aa320) is the level's size, the level's index times GemsPerLevel plus Gems (at most GemsMax
  when that is 1 or more); 40 (0x6aa360) adds 1 to the score (+0xd24) and to the progress (+0xe00) for each gem cleared. QuestBoard's slot 152 (0x6cf290, AddPoints) adds the points of a
  match only when +0x3187 or the goal's +0x10c is set, so in a quest the gems cleared are the whole progress.
- *The end of a level* (slot 156 = 0x6ca7e0, which runs 0x6ca350; `QuestBoard::OnLevelFull`, a new virtual of Board whose own is the level change): when the level bar is full, the game's
  flag (+0xd8c, `mGameOver`) is set and the clock +0xd20 starts at 200. `QuestBoard::Update` steps it (`StepQuestClock`): at 200 it counts down once the board is at rest; at 198, with the
  board at rest and the bar equal to its target, the win is told (the quest map is shown over the board, 0x6dd760); then it counts down, and at 0 slot 233 (0x6ded60) runs.
- *A lost quest*: NoMoreMoves (slot 161, 0x6d8e30) ends the game unless the goal's slot 10 says no or the level is full; with the game-over counter (+0xd1c) at 300 the map is shown over the
  board (0x6dd760). A quest that does not record high scores has no end dialog (0x6ded60 makes one only for the other), so `mEndScreenShown` is set at its new game.
- *MoveLimitBoard*: slot 191 (0x6c5a60, a swap that found a match) and slot 180 (0x6cfb50, a hypercube's group with a colour) use a move; slot 22 (0x6e00e0) ends the game when the board is at
  rest with no moves left and +0xd8c is clear; slot 98 (0x6c6500) gives no hint with no moves left.
- *TimeLimitBoard*: Time, TimeBonus and TimePenalty, the clock in hundredths (0x73bb80) and the time's end (slot 216); Diamond Mine's board now derives from it and its own clock and time's
  end are the base's.
- *The profile*: a won quest is marked in `mQuestsDone` and the profile saved (0x71bb10) when the win is told (the exe's map does it, 0x684380, 0x118 updates after the win). The profile's
  fields that were called the flag rows, the counter before the flag, the flag after the counter and the hint flags are `mQuestsDone` (+0x158), `mQuestSection` (+0x184), `mQuestMenuFlag`
  (+0x188) and `mQuestHelpSeen` (+0x2ac); the save's order is unchanged, so the old saves load.

**Test commands** (the state line keeps every other mode's keys). `goto quest` shows the map; `quest <index> [Name=Value ...]` starts a quest, each Name=Value replacing a setting of the quest
(`quest 3 Gems=40` is a short Stratamax); `quest done <index>` marks a quest done in the profile and `quest undo` clears every mark. The framework's `click x y` presses a gem or a button of
the map. `bot` and `move` work as on any board. State keys: with the map up `questmenu` (1), `questsection`, `quests` (40 characters of 0 and 1), `sectiondone` and `sectionstate` (five numbers
each), `questbuttons`; with a quest board `quest` (the index), `questwon`, `questclock` (+0xd20), `goal` (the setting Goal, upper case), `goalsize` (slot 22), `goalcount` (the progress),
`questscore`, `quests`, `questmenu`, and `movesleft` and `moves` (Move Limit) or `timeleft` (Time Limit); `screen` is `quest`.

### Not done (Quest)
- The map's relic pages (0x67cc80 builds a page for each section with the relics, sockets, ghost sockets and their animations; 0x690110 and 0x687f90 draw them), the pillars' parallax and the
  page's turn (the curve at +0x138, 0x690f10), the progress pictures and panes, the sparkles, the percentage window, the quest-complete banner and animation (0x67ed00, 0x684380's effects),
  the sounds (SOUND_QUESTMENU_RELIC*), the exe's wait for the relic animation before a click (0x674060), the text "Complete quests to restore the Lost Relics of Bejeweled". The gems are put in
  two rows of four; the exe puts each in a socket of its page.
- QuestHelpDialog (vftable 0x8697d4, made by 0x6ccfd0 from slot 140, 0x6d0af0): the exe shows it before a quest that has not been started (the profile's `mQuestHelpSeen`); made now ("Tutorials and help").
- The exe keeps the map (+0xbd8) and the board (+0xbe8) both alive and shows the map over the board at a win (clock 198) and a loss (300); the port does the same but makes the map again each
  time and deletes it when a quest starts.
- The fanfare at clock 100 (slot 162), the sounds and music events ("Break", "Play", "Wait") of 0x6ca350, the text of the finished level (0x7411f0, slot 160).
- The settings ColorCount (slot 261), HighScoreBase, HighScoreIncr, WantPointComplements, NeverAllowCascades (+0x3228), ShowAutohints (+0x4dc), Description, HyperMixers (+0x3186) and
  PowerGemThreshold (slot 190; 0x6cda70 reads it into +0x3188), and Board's slot 84 (0x150) after the goal's load.
- Goals: made by part 2 and part 3 (see "Quest, part 3"); the rest are not reached by any data file. A quest whose Goal is not made logs "not made yet" and has none.
- The moves counter's bounce (+0x3250, MoveLimitBoard slot 22), the board's drawing for a quest (the goal's text "TitleText", "Moves Left: %i" in the bottom frame, the move counter's picture;
  slots 26, 43, 47, 240, 242, 250: the port writes the moves left and the gems cleared plainly), the leaderboard of a quest that records high scores (0x6ca240, 0x6c7c60).
- QuestMenu's QUIT (the app's quit question), hover effects and the buttons' glow curves.
- TimeLimitBoard's slots 47 (0x6c6060), 262 (0x6cb260) and 263 (0x6cb130, DoQuestPenalty); no quest with the Clear Gems goal has a time limit, so the board is only exercised through Diamond
  Mine.

### Oddities (Quest)
- 0x693760 fills the name map's "Hidden Objects" twice (Buried Treasure's pictures, then Sandstorm's); the second replaces the first, so Buried Treasure and Sandstorm both show Sandstorm's icon.
- quest.cfg gives some quests a setting twice (Alchemy and Wall Blast have two Challenge lines, "Time Limit" then "No Lose"). The exe's parser (0x791c80) stores each setting with std::map::insert (0x403740), which keeps
  the FIRST value; part 3 made the port's QuestData do the same (it had kept the last), so those quests are Time Limit boards and no quest names No Lose.
- 0x6dd760 is called for a loss and for a win; the call for a win passes the "quest complete" dialog object, which makes the map start the countdown (0x67ed00) that marks the quest done 0x118
  updates later; a lost quest shows the map and marks nothing.
- QuestBoard::Update's test at clock 198 does not pass until the level bar has reached its target (slot 188 against +0xd68), so a win waits for the bar.
- The profile's +0x184 can be any number in a save file; the port clamps it to 0..4 when the map is made.
- The bot's twenty moves of Stratamax clear about 60 of the 120 gems: the bot loses it, as the suite shows.

### The suite for Quest

suite_quest.txt (seed 7, name Tester, `unlock all`, `goto quest`, a click on Stratamax's gem, `bot` to the loss, `quest 3 Gems=40` and `bot` to the win, `quest done 0` to `2` and a click on
NEXT) has five state lines and five shots (the map, the game, the lost game under the map, the won game under the map, the second section's map); suite.sh runs it twice and checks each state
line's keys.


## Quest, part 2 (FillerBoard, BalanceBoard, NoLoseBoard, TimeBombBoard, BadMoveLimitBoard, ButterflyGoal; the secret boards as quests)

Bej3App::StartQuest now maps every Challenge of 0x774060's list: TIME BOMB and TIME BOMB (REAL TIME) (TimeBombBoard, 0x6dd440 with 0 or 1), INFERNO (IceStormBoard), BALANCE, BUTTERFLIES and
BUTTERFLY, FILLER, MOVE LIMIT, TIME LIMIT, DIG, NO LOSE, BAD MOVE LIMIT and POKER. The test command's setting changes (`quest <i> Name=Value`; a value in quotes may hold spaces) are made before the
Challenge is read, so `quest 3 Challenge="Bad Move Limit" BadMovesAllowed=2` makes a Bad Move Limit board. The quest numbers below are the file's order (the map's): 0 Butterflies, 1 Gold Rush, 2 Alchemy,
3 Stratamax, 4 Balance, 5 Avalanche, 7 Time Bomb, 8 Poker, 9 Ice Storm, 14 Time Bomb II (real time), and 15, 20, 27 and 38 Avalanche II to V.

Hooks added to Board (each a no-op, or the old code, for the boards before): FillInBlanks is virtual (slot 203), OnSwapFailed (slot 192, called where 0x759ba0 sends a swap back), OnFillDone (slot 204, the
last call of 0x763e90), CountDownPiece (slot 212: CountDownMoves' per-piece part), UpdateWarnings (slot 181: once in each UpdateGame between the specials' countdown and the busy counts) and IsAtRest
(0x75fc10: IsSettled's tests without the first-settled call). Piece gains the bomb fields (+0x26c, +0x270, +0x24c, +0x250). QuestBoard::OnFirstSettled (slot 140) calls the new OnQuestStart
(slot 256); Butterflies' and Poker's start moved there and lost their secret-flag test (the exe's help dialog that comes between is not shown). The Classic and modes hashes are unchanged.

The secret boards' NewGame (Butterflies 0x6e3660, Poker 0x6dfa40, Ice Storm 0x6e6a10) now end with QuestBoard::NewGame (0x6ddc70) as the exe's do (Dig's already made the goal itself), so a quest gets its goal; in a
secret mode the only goal is Butterflies' (Catch Butterflies), which counts as the board did. Ice Storm's slot 143 ends with the goal's slot (0x6dd580), and Ice Storm's and Butterflies' end of the game goes
through QuestBoard::NoMoreMoves (0x6d8e30), which for a secret mode is Board's. Ice Storm's UpdateTick no longer returns when the secret flag is clear.

### ButterflyGoal (0x86a90c)
Slots 53 (0x6ae830, the setting Butterflies), 22 (0x6aa420: 0 in a secret mode, else Butterflies) and 40 (0x6ab2f0: the freed-butterflies counter 0x1c for the move's tag while the game is not lost, and
in a quest score and progress up by one). ButterflyBoard::OnPieceCleared (0x6d03d0) runs the goal's slot first, scores the points as before, and when the progress has reached the size (not in a secret mode)
every butterfly on the grid gets one move more. Tested with quest 0 (`Butterflies=1` and `2`: won, the map comes back; `Butterflies=6` is lost to the spider).
Not done: the burst at the butterfly (0x6d01b0).
Oddities: the exe's goal counts a freed butterfly only while +0xd1c is 0 (or in a secret mode); the board's own points are not guarded in the exe, and the port's old guard for them was dropped.

### Filler (0x86817c, constructor 0x6dcc00): quest 5 Avalanche (Clear Gems), tested with `quest 5 Gems=8`
Translated: slot 135 (0x6e3420: NumStartGems, DropPerMove, AssistCurve), 22 (0x6e3130: credit 0 when the level is full, the on-screen count and the counts at a move's start), 172 (0x6c52d0, 0x7d), 176
(0x6d7c20), 180 (0x6cff00, credit +2), 191 (0x6c52a0, credit + DropPerMove), 161 (0x6c50f0: credit +3 and no end), 98 (0x6c6b10), 214 (0x6c6e90: the loss's fall), 181 (0x6dc770: the top row's warning,
0.0075 a step at rest, the game ends at 1) and 203 (0x6db550: the fill, see the function's comment: the gems fall into gaps, then as many new gems as the credit allows are placed by weighted columns
with the exe's draws in order: colour, column list, column, speed, height; slot 199, 0x6c5100, rejects a run through a new gem). The loss is the top row staying full: `quest 5 Gems=900` with `bot 25`
is lost (the map comes back after 300 updates).
State keys: credits, droppermove, gemsdropped, gemsremoved, gemsatstart, fullwarning (thousandths), dropallowed.
Not done: the drop-in effect lists (strings "b+0.25,1,0.05,...", the curve at +0x3288, +0x3264 to +0x3280), the warning's effect and the gems' glow (+0x264) and tumble (+0x608, +0x60c; the four draws of
the app's Rand() are kept), the pictures of slots 218, 220, 115, 26 and 47 (the credit and the warning are written plainly beside the score), the extra colours (+0x1040, +0x1050), slots 182 and 183.
Oddities: the exe has no limit on the tries of a fill; the port stops at 20000 (never reached in the games run). The gems' mCreatedUpdate and mMovedUpdate are the board's +0x3198 count
(QuestBoard::mUpdatesPlayed), as 0x6db550 reads them, where Board's fill uses the widget's update count. The bot does not lose Avalanche by itself (it clears the top row as it fills), so a loss is reached by
stopping it.

### Balance (0x86789c, constructor 0x6dd090): quest 4 Balance (no Goal), tested with `quest 4 Gems=2` (won) and `quest 4 Balance=1 RollSpeedDivisor=20` (the scale tips: lost)
Translated: slot 135 (0x6e4c80: Gems, Balance, RollSpeedDivisor, CascadeThreshold), 143 (0x6d06a0: red and blue clears counted up to Gems each; score and progress are their sum), 154 (0x6c53a0: Gems times
2), 22 (0x6e3c90: the scale rolls by the blue-red difference over Balance, held to 1 in size, over RollSpeedDivisor, while the hint is allowed and the level is not full; past 1 either way the game ends),
171 and 172 (0x6c5380, 0x6c5370: the grid's place, 0x1c2 and 0x73). State keys: balance (thousandths), red, blue, gemspercolor.
Not done: the scale's picture, the arrow and the smoothed counts (+0x3264, +0x3268, which follow the counts by at most 0.02), the effects at +0x3368; slots 220, 238, 239, 244 and 248 (the scale and the
counts are written plainly beside the score).
Oddities: slot 203 (0x6dd720) gives QuestBoard's fill 0 when a move's cascade (counter 0x1b) is longer than CascadeThreshold and its own argument otherwise; every call of the port's fill passes 0 for that
argument, so the board has no override. The grid's place assumes +0x2c4c and +0x2c50 are 0 (as Butterflies' slot 172 does).

### No Lose (0x868ef4, constructor 0x6da7a0): no quest of quest.cfg makes it (part 3: the first of a repeated setting wins, so Alchemy and Wall Blast are Time Limit quests)
No slot of its vftable differs from QuestBoard's, so NoLoseBoard is an empty class; the rules come from the goal. `quest 2 Goal="Clear Gems" Gems=8` in suite_quest2 now makes a Time Limit board (Time 90).
Oddities: see "Quest" ("Oddities") for the repeated Challenge lines.

### Time Bomb (0x869dc4, constructor 0x6dd440) and Real Time Bomb (0x869954): quests 7 and 23 (Time Bomb), 14 and 29 (real time); the goal Clear Bombs is made by part 3
Tested with `quest 7 Goal="Clear Gems" Gems=8` (won) and `quest 14 Goal="Clear Gems" Gems=25 BombCountdown=3` (lost: a bomb ran out). The exe makes TimeBombBoard for both names (the real-time one with
the flag at +0x3262); RealTimeBombBoard's vftable is written by no constructor, so it is not made, and its slot 181 (0x6d8c60) and the drop at 0x6c6200 are dead code. Translated: 135 (0x6e52e0), 143
(0x6d0930: a cleared bomb is one less), 191 (0x6c61c0: a move less until the drop), 204 (0x6c53d0: the drop is counted), 185 (0x6c60b0: a plain new gem of the first five rows, in the middle columns while
the score is 0, becomes a bomb, up to 100 draws of the app's Rand(); MakeBomb is 0x73ccc0), 212 (0x6c6090 over 0x73c710: a move starts the bomb's shake, a real-time tick counts it down and a bomb at 0
ends the game), 256 (0x6c4a40) and the loop over the pieces in 0x767e20 (a bomb at 0 at rest ends the game, slot 163, 0x755e50; the shake counts a move off between phases 5 and 10; a real-time bomb ticks
every 100 updates). State keys: bombgrid (each bomb's count as a digit), bombs, untildrop, bombstarted, realtime.
Not done: the bombs' pictures and effects (the count is written over the gem), the sound of slot 204, the pieces with the bit 0x100,
the +0x22c test of 0x73ccc0.
Oddities: with a Clear Gems goal (only the suite's overrides give one) the first fill of a game makes no bomb, because ClearGoal reads Gems in Load, after the first fill (the level's size is 0 then), so the first
bomb comes with the first refill; the exe's order is in "Quest, part 3". A Clear Gems goal counts a defused bomb as a gem.

### Bad Move Limit (0x86742c, constructor 0x6dd370): no quest of quest.cfg has it; tested with `quest 3 Challenge="Bad Move Limit" BadMovesAllowed=2 Gems=60` (lost)
Translated: 135 (0x6e4f60), 22 (0x6e4f30: at rest with all used the game ends), 192 (0x6c6050: a swap that found no match, sent back, uses one), 98 (0x6c6c40). State keys: badallowed, badused.
Not done: slot 124's text in the frame (written plainly beside the score).
Oddities: the end test is equality, so a swap made while the board is not at rest can take the count past the limit (the suite's swaps reach 3 of 2).

### The secret boards as quests
Butterflies (quest 0, `Butterflies=1`: won), Ice Storm (quest 9, `Gems=8`: won), Gold Rush (quest 1, `TargetCount=1` and a hit on the gold: won) and Poker (quest 8) start and end the quest way.
Dig's goal reads its size from TargetCount (slot 22, 0x6a9810) and counts each treasure as 1 (the other branch of 0x6a9780); Ice Storm's goal counts the cleared gems; Poker's quest has the Goal 3000
(a score, see "Quest, part 3").
Not done: Ice Storm's non-secret branch of 0x6e5790 (it sets +0x3278 and the glow at +0x369c and +0x36a0): in a quest the stage does not advance and the multiplier does not rise, as the exe's
secret-flag test of that function has it; the artifacts of Dig in a quest (only a secret mode holds them, as part 1's guard says).
Oddities: the Gold Rush bot does not dig by itself far enough in the suite's span; the script uses `dig clear` and `dig hit` as the secret mode's suite does.

### The suite for Quest, part 2
suite_quest2.txt (seed 7, name Tester, `unlock all`) starts ten quests in a row, each with its bot game and a state line and a shot at its end: Avalanche, Balance, Alchemy (Time Limit, Clear Gems), Stratamax as
Bad Move Limit (four swaps; two or more find no match), Time Bomb, Time Bomb II in real time (lost), Butterflies, Poker, Ice Storm and Gold Rush. suite.sh checks each state line's keys and runs it twice.

## Quest, part 3 (QuestGoal.cpp ClearBombsGoal, WallGoldGoals.cpp, HiddenObjectGoal.cpp, PokerBoard.cpp's quest path)
The goals that quest.cfg names and that were left: Clear Bombs, Hidden Objects, Wall Blast, Gold Lines, Gold Lines Flip; and the Poker quest's numeric Goal. QuestBoard now passes to its goal what the exe's slots do: 142 (the goal's
slot 39, OnPieceClearing, called by RemovePiece with the flag), 197 (slot 16, OnCellHit: Explode calls it for each of its nine cells and a match group for its ring cells, before the piece lookup), 211 (slot 17,
OnRunFound), 234 (slot 11, UpdateTick, before Board's step), 217 (slot 26, IsLevelFull); Piece has `mDestroyed` (+0x17d, set by DestroyPiece 0x74c770 and by Explode 0x73f8e0 on the pieces it takes) and Board has
`DrawBehindPieces` for what a goal draws under the gems. The quest clock's start and wait (+0x318c, +0x3190) are members a goal's constructor may set (Hidden Objects: 550 and 200). QuestData keeps the first value of a
repeated setting (see "Quest", "Oddities").

### What the empty and numeric Goals are
Balance (quests 4, 13, 30, 32) has no Goal and BalanceBoard has its own rules (part 2). The three Poker quests (8, 18, 37) have the Goal 3000, 3500 or 4000 and Hands = 10: 0x6dfa40 reads the first setting (the string at
0x7d74b8, "Goal") to +0x3968 and Hands to +0x396c; slot 154 (0x6c4d70) returns +0x3968 for a game that is not secret (0 for a secret one). There is no goal object: QuestBoard falls back to Board's 0x73b810, and the
level is full when the score reaches the Goal. The non-secret branch of ClearHand (0x6cdce0) sets the progress (+0xe00) to the score; with the Goal reached it ends there (the cards stay); else Hands goes down and one
past the last (below 0) calls slot 161 (NoMoreMoves); the secret branch's skull, clock and +0x3970 count are not done in a quest. OnQuestStart (0x6cf440) clears a hand while +0x3970 is 0, which in a quest is the
first call: it takes one of the Hands, so a quest has Hands hands. The port: PokerBoard has mGoalScore and mHandsLeft, GetLevelMax and the quest branch; the scoring test of UpdateGame is `mQuestClock == 0` (the exe's) in
place of `mHands > 0`. Keys: handsleft, pokergoal. Tested with `quest 8 Goal=40 Hands=3` (won) and `quest 8 Goal=100000 Hands=1` (lost).

### Time Bomb's first fill
0x6ddc70 runs: the goal's constructor, goal slot 52 (a plain return), Board's NewGame (0x74e410, the first fill), goal slot 53 (Load), board slot 84. ClearGoal's constructor reads nothing (Gems is read in Load), so a Clear Gems
goal has the size 0 in the first fill, as the port had; ClearBombsGoal's constructor reads Bombs at once, so with the Clear Bombs goal (the only goal of the Time Bomb quests) the first fill already has the size and
makes its bomb. The port's ClearBombsGoal reads Bombs in its constructor and in Load, in the exe's order; `quest 7 Bombs=2` shows one bomb at once.

### Clear Bombs (ClearBombsGoal, vftable 0x86aa34, constructor 0x6ae760): quests 7, 14, 23 and 29, tested with `quest 7 Bombs=1` (won) and `quest 14 Bombs=50 BombCountdown=3` (lost)
Slots: 53 (0x6ae6b0, Bombs), 22 (0x6aa3b0, the count of Bombs), 40 (0x6ab280: a cleared piece with the bits 0x60 scores and counts). Key: bombgoal.
Not done: nothing is known to be missing.
Oddities: none.

### Wall Blast (WallBlastGoal, vftable 0x86b814, constructor 0x6ab720): quests 17 and 35, tested with `quest 17 Grids="<64 digits>"` and a flame (won), `quest 35 Time=3` (lost)
Load (slot 53, 0x6b0fb0): DropThreshold; Grids cut into grids of 64 digits (0x744e00 with its second argument: a grid with fewer digits has the rest at 0, as quest 17's 56); one drawn by Rand() modulo their count; each digit that is not 0
is a wall cell with two draws for its looks' flags; the count of the wall cells is the size (slot 22). 0x6ad740 breaks a cell (the score and the progress up by one); slot 16 (0x6afcb0) and slot 39 (0x6ad9d0: a cleared
piece with a match timer above 0 or the destroyed flag, in the grid) call it. A plain match of three leaves the timer at 0, so only a special's reach, a hypercube or a gem a special takes breaks the wall, as
the quest's help text says. Keys: wallcount, wallleft, dropthreshold. Drawn as a grey block on each cell.
Not done: the wall's stones, cracks and sparks (0x6afe80, 0x6ad5f0 with its random draws), the tutorial (slots 7, 54's tutorial part, 0x6b76c0; the goal's state at +0x250 to +0x25c is never set, so slot 69 allows hints), the special-gem drops
(slots 46 to 48: slot 46 is a stub that returns false, so board slot 182 never asks for them and 0x6ab080's hypercube, flame and star drops are dead; the DropThreshold is read and kept).
Oddities: the wall's picture is not the exe's; the Time Limit board comes from the first of the repeated Challenge lines (see "Quest", "Oddities").

### Gold Lines and Gold Lines Flip (GoldLinesGoal, vftable 0x86affc, constructor 0x6afcd0; GoldLinesFlipGoal, 0x86b93c, 0x6afe00): quests 2 and 16 (Gold Lines), 26 (Flip); tested with `quest 2 Squares=10` (won), `quest 16 Squares=64 Moves=3` (lost) and `quest 26 Squares=8` (won)
Cells: 8x8 records with a list of delays and a level. Slots: 53 (0x6b04f0: every level 0, Squares, OnlyCheckSwaps; Flip's 0x6b0f20 then sets every level to -1), 10 (0x6aaa00: the game may end only with no delay waiting), 11
(0x6abeb0: the first delay of each cell counts down and at 0 the cell flips), 26 (0x6aa1d0: Squares <= score runs the board's slot 156 and says true), 22 (0x6aa200, Squares), 40 (0x6aa220: a destroyed piece flips its cell), 17 (0x6b1380: a run of
gems: the middle of the run is the mean of its gems' rows (or columns), the cell at the middle flips at once and the others at the distance d from it after 10 * d updates, both sides, along the run's line) and 72 (0x6b78f0, FlipCell: a level
below 1 goes up; above 0 it scores). Flip's cells start at -1, so the first flip makes lead and the second gold. Keys: squares, gold. Gold cells are drawn in gold, Flip's lead cells in grey.
Not done: the plates' pictures and flips (slot 59, the effect at +0x940 of Flip, whose +0xdc the slot 10 also waits for), the sounds and the combo counter (+0x93c), the wave counts (+0x138 of each cell), OnlyCheckSwaps true (no quest has it: all say false) and the run list
at +0x928 (never filled by Gold Lines).
Oddities: a run's flip reaches the whole line, not only the run, so a single match can score up to eight cells (the suite's won game reaches 15 for 10).

### Hidden Objects (HiddenObjectGoal, vftable 0x86ba64, constructor 0x6afb60, 0x1f80 bytes): quests 6, 11 and 33, tested with `quest 6` and three flames (won) and `quest 11 Time=3` (lost)
It is a class of its own: HiddenGoal (0x86b24c) and HiddenGridGoal (0x86b374) are not its base or its parts. The constructor sets the quest clock to 550 and the wait to 200 and the refill time to 10,000,000 (never). Load (slot 53, 0x6af970): ItemSet,
RefillRateSec (times 100, in updates, when the quest has it) and the placement 0x6aec80: up to 15 layouts, each from the shapes of the item set (ids 0 to 2, 3 to 5 or 6 to 9 of the ten records: sizes 1x3, 2x2, 2x3, 4x2, 2x2, 2x3, 2x2, 3x3 "X  X  XXX", 3x2, 2x4) shuffled by the CRT's
random_shuffle over the app's Rand(), taken last first; each shape has 499 tries (a draw for the top row modulo 8 less its height, a draw for the left column modulo 7 less its width, plus 1; the cells of the shape must be free and not a corner;
a placed shape has one more draw modulo 1000); the layout with the largest sum of nearest-neighbour squared distances (the exe's float arithmetic) is kept. The tiles hold the objects' indexes. Slots 16 (0x6b9a80) and 17 (0x6b7b80: with NeverAllowCascades off a cluster counts only
with a piece of a swap) queue the pieces' ids (0x6b7a50: a map of ids, each once until the board stands settled) and 0x6b75e0 uncovers the tile each queued piece stands on (0x6b7400: the tile's countdown is the refill time). Update (0x6c24c0): a tile with
a countdown below 10,000,000 counts down; 0x6baff0 finds an object when all its cells are fully uncovered (the goal's size is the object count, slot 22 0x6a9950). Keys: hiddenobjects (column.row.width.height, an f when found), hiddenfound, cover (a digit for each tile),
refill, itemset.
Not done: the exact curves of the tiles' cover (0x6c24c0 starts a curve of 100 updates when a tile is uncovered and another when it falls, the fall waiting for the others of the same state: the port steps 0.01 an update up or down), the objects' pictures and the sand, the effect of a find and its wait
(0x6ab520), the effect lists at +0x1f58 and +0x1f5c (a find also waits for the young effects of the second), the sounds of 0x6b7400, slots 50, 51, 55, 56, 63, 64 and 68, the helper widgets, the tutorial.
Oddities: RefillRateSec is in seconds in quest.cfg and the exe multiplies it by 100 (the constant at 0x880d90). No shape reaches the last row or the first and last columns, so the layout's score term for the last row never applies.

### Goals that no data file reaches
Checked against quest.cfg and secret.cfg: the Goal settings are Catch Butterflies, Clear Bombs, Clear Gems, Dig, Gold Lines, Gold Lines Flip, Hidden Objects, Wall Blast and the three numeric ones (Poker). So no data file reaches these goals, which stay untranslated:
- Gold (GoldGoal, 0x114 bytes), Gold Yellow (0x6ae170), Detonator, Arsenal, Score, Firestorm, Preserver (0x118), Hidden Gems (0x11c), Hidden Grid on its own (0x21c) and Survival (0x118): no quest of quest.cfg or secret.cfg names them.

### The suite for Quest, part 3
suite_quest3.txt (seed 7, name Tester, `unlock all`) plays eleven games, each ending in a state line: Buried Treasure (three flames, won), Sandstorm (Time=3, lost), Wall Blast (a flame breaks the nine cells, won), Wall Blast II (Time=3, lost), Alchemy (won), Alchemy II
(Moves=3, lost), Alchemy III (Flip, won), Time Bomb with its Clear Bombs goal (won), Time Bomb II (lost), the Poker quest (won on its score, lost on its hands). Hidden Objects comes first, because its layout comes from the app's draws (6.4.1.3, 1.2.2.3, 5.0.2.2 under seed 7).

## Real input (tools/realinput.sh, TestCommands.cpp's `move`, `state`, `play`, `atmove`, `idle`)

The native window under rundesk, driven by posted-as-SDL-events input (`-realinput`, docs/REAL_INPUT.md) from `tools/playbymove/playbymove.py`: the
driver asks `state` and `move` and plays the answer's `input` as real events, until the state says `"over":true`. Run `games/bejeweled3/tools/realinput.sh
[mode ...]` from the worktree root (no mode: all eight; `REALINPUT_DISPLAYS="win"`, `"full"` or `"alt"` picks a display; `STEPS`, `ZEN_STEPS`, `QUEST_INDEX`
change the limits). It waits for a free game slot (`tools/gamecount.py --limit`), runs one game at a time with a fresh user folder under
`build/realinput/<mode>-<display>/` (driver.txt, game.log, end.png), prints one line a run (`<mode> <display>: <steps> moves, stop <over|limit>`) and exits 0
when every run but Zen's stopped on "over" (Zen has no end: it is played for 400 steps and the limit is its end). The whole script takes about 4.5 minutes.
The folder is not kept: delete `build/realinput` once the results are here.

**What `state` and `move` do now.** `state` ends with `"over":true|false`: true when the board's game is over and its end dialog is up (every mode but Quest),
or, for a quest, when the quest map has come back over the finished board (won or lost). A state with no board also carries `"update"` (the app's update
count) and `"dialogs"` (how many are up), so a driver can see a menu change. The frozen hashes (suite.sh) are of the state lines without the `"over"` key (the
suite strips it with `sed` before hashing): the key is last in the line, and Classic's 51086c43ed2a and the modes' 873d262bb577 are unchanged.
`move` answers, in game pixels, what a hand does next:
- a dialog that is still fading in or out: a wait (`"input":[]` with `"updates"`); the first player's name box (the welcome dialog): the keys of "Tester" and Enter;
  the end dialog: MOVE null (the game is over); any other dialog (a locked mode's, the empty-name message, the reset question): a click on its Yes button.
  The tutorial boxes, the help dialogs and the quest help ("Tutorials and help") come in a fresh player's game; each one's footer button is its Yes button, so the same click answers it (the boxes to tick are never clicked).
- no board, the quest map up: the click on the button of the quest `play quest <i>` named (a wait while the map's page is not that quest's section);
- no board, the menus: a wait until the intro has ended and the camera is still; then PLAY's ball; then, once the play menu is up and the ball's size curve
  has stopped, the ball of the mode `play <mode>` named (classic, zen, lightning, quest, poker, butterflies, icestorm, diamondmine). Every button click is a
  move, three updates, a press, three updates and a release (a button reads its state once an update), at the middle of the widget's own rectangle;
- a board: a wait while it is not at rest, a level change runs (`mHyperspace`, `mLevelUpPending`) or no move is found; else the bot's swap as the two clicks.
  Once the game is over, MOVE null.
Test commands that make a run possible: `play <mode> [quest index]` (the target the menus' clicks lead to; unset: MOVE null at the menu); `unlock all` before
the player exists is kept and done by `move` once the name has made the profile; `atmove <n> <command...>` runs a test command once `move` has given n swaps
(classic: `board` with a board that has no move, which is the suite's way to the end dialog; Ice Storm: `idle`); `idle` makes `move` stop swapping (a player
who walked away: Ice Storm's bot always finds a move and crushes the ice for ever, 3000 steps and a score of 688,657 without an end).

**The runs** (window = `registry.ini` with ScreenMode 0; full screen = a new player's own start, which the port opens full screen; "alt" = the full screen
start with Alt+Enter at once, so Classic is played in the window it switches to). Steps are the driver's moves and waits, as playbymove counts them:

| Mode | Window | Full screen | What ends it |
| --- | --- | --- | --- |
| Classic (`atmove 40 board ...`) | 323, over | 323, over; alt: 323, over | the no-move board, the end dialog up |
| Zen | 400, limit | 400, limit | none (Zen has no end) |
| Lightning | 1992, over | 1992, over | its clock and the Last Hurrahs, the end dialog |
| Quest 3 (Stratamax, Move Limit) | 293, over | 293, over | the 20 moves run out, the map comes back (a loss) |
| Butterflies | 186, over | 186, over | the end dialog |
| Poker | 572, over | 572, over | the end dialog |
| Ice Storm (`atmove 60 idle`) | 626, over | 626, over | the ice cracks, the end dialog |
| Diamond Mine | 1021, over | 1021, over | the clock runs out, the end dialog |

Window and full screen give the same step counts in every mode, so the same input made the same game in the two displays (the window's mapping to game
pixels, the full screen's 1600x1200 and the desktop-sized 3840x2160 client the log shows for it, took every click and key to the place asked). The port
has no 3D path (Is3D is 0 in its registry), so there is no second display mode to try. Not played by this: the end dialogs' PLAY AGAIN and MAIN MENU
buttons, the options screen and a second quest of the map's other sections (the quests' own games are the suite's).

**What a hand's input showed that the scripted suite did not.** No bug of the port. Three things of the runs themselves: a click on a button must come as
move, wait, press, wait, release (the board's two clicks need none of it); the menus' balls take no click while their size curves run (about 60 updates after
PLAY), so `move` waits for them; and the end dialog's first frame shows "Final Score" as 0 (its count-up starts when it opens), so an `end.png` taken at once
has not counted up yet. The fresh player's default start is full screen, as the exe's first run is.

**On the web** (2026-11-23, the Browser pane on `bejeweled3-webtest`, port 18830). `tools/web/playmode.js` is the web twin of
`realinput.sh`: from a wiped test profile it starts the game, sends `seed 1`, `unlock all`, `play <mode>` (and the same `atmove`), and
`playByMove` plays the bot's moves as DOM input from the name box to the mode's end; the end's shot is `build/webshots/playmode-<mode>/end.png`.
Every mode reached its end: Classic 63 moves (over), Zen 400 (its limit), Lightning 34 (over), Quest 3 41 (over, lost), Butterflies 33,
Poker 48, Ice Storm 45 and Diamond Mine 32 (all over), none with a page error. One page load failed before the game started (the page's
clips script was not defined); its reload ran. The page check (`tools/web/pagecheck.js`, settle 400: by then the first fill has landed):
11 pass, 0 fail, 1 information line (the browser keys the game takes). The focus line matches the native answer: the board's update and
hint timer go on while the focus is away (Classic cannot be paused; see "Focus").
The web run showed one fault of the test command: `move` answered an atmove's command with that command's own `OK`, which a web driver reads
as the move's answer; the command's answers are now logged with the prefix `atmove: `, and `move` answers as usual.


## Tutorials and help (src/BoardHints.cpp, src/HintDialog.*, the hooks listed below)

The tutorial boxes (HintDialog, vtable 0x86e654, made by 0x682bf0), the help dialogs (HelpDialog, 0x86e0bc, made by 0x696100, with its pages in HelpWindow,
0x86e244) and the quest help (QuestHelpDialog, 0x8697d4, made by 0x6ccfd0) are translated, with what opens them, the flags that remember them, and how the game
waits.

Flags. The profile's +0x21c (`Profile::mHintFlags`, saved in profile.dat in its old place, so old saves load) holds one bit for each box or help seen:
Board::HINT_FLAME 1, STAR 2, HYPERCUBE 3, MULTIPLIER 4, SUPERNOVA 6, COIN 8, TIME_GEM 9, VERTICAL_MATCH 0xd, DARK_ROCK 0x10, and a help's id as its bit (0 Zen,
10 Lightning, 0x11 Butterflies, 0x12 Poker, 0x14 Ice Storm, 0x15 Classic, 0x16 Diamond Mine). Bit 19 (0x80000, Board::HINTS_OFF) turns every box and help off: it
is the only switch the exe has (the "Don't show hints" box of the help dialog and the "Disable Hints" box of a tutorial box); there is no reset option, and the
options have nothing else for tutorials. The board copies the flags in its constructor (0x74ae30: +0x26a8 from +0x21c) and sets bits in both when a box is
queued (0x745bf0, 0x73b990). `mQuestHelpSeen` (+0x2ac, 0x28 bytes) is the quest help's own, set by 0x6cd9f0.

Queue. Board::mHintQueue (+0x26b8) holds requests {bit, piece id}; a help is a request with piece id -2. `CanShowHint` is slot 131 (0x73c4c0): the bit unseen,
HINTS_OFF clear, +0x1188 (`mCheckerFlash`) 0 and not +0xe28 (the time is up: `IsTimeUp`, Lightning's `mEnding`, TimeLimit's `mTimeUp`). `RequestHelp` is slot 213
(0x743f40): the mode's start (0x7737d0 Classic 0x15, 0x773100 Zen 0, 0x773440 Lightning 0 then 10, the secret modes' 0x774860: Butterflies 0x11, Poker 0x12,
Ice Storm 0x14, Diamond Mine 0x16) calls it after the reset (slot 85) and before the board is dealt; if the help is unseen and boxes are on it sets the hold
(+0x2b40) to 100 and queues the request. `CheckForTutorialDialogs` (0x765d50) runs from UpdateGame when the board is at rest (before the no-move check) and at hold 10:
with neither dialog 0x12 nor 0x13 up, the game going on and boxes on, it takes the first request: a help is made as dialog 0x13 (the request goes), a gem's box
as dialog 0x12 over the gem (below it for the top four rows, above it for the rest; the request stays until OK), a request whose gem is gone is dropped and its
bit freed, and a time-up empties the queue. UpdateTick holds the board (0x767090): no step while dialog 0x13 is up; while the hold counts down the board does
not step either, and its update count stands still. While the queue is not empty `CanSwap` (0x75fae0) refuses and a level change does not start. While dialog 0x12 is up
the game's tick (+0xe38, `mGameTicks`) and the seconds counter stand still (slot 101, 0x73c330, among others). Board::DialogButtonDepress (0x7563c0) takes the box's OK (button
1000): its request goes; with "Disable Hints" ticked, HINTS_OFF is set, the board takes the profile's flags and the queue is emptied.

Opens (when). The boxes: the first Flame (a run of 4, FindMatches 0x75af60 where the specials are made), Hypercube (5), Supernova (6 or more) and Star (a gem in two runs)
of a profile, queued on the gem that becomes the special; Lightning's first time gem (0x716c60, bit 9); Ice Storm's first vertical run in the secret mode (0x6e2250, bit 0xd, then
the check at once). The helps: a mode's first game (the ids above), and the options dialog's HOW TO PLAY button (id 2, string 0x134, 0x681370; it stands above BACK and MAIN MENU
while the board's +0x2b44 is not -1; 0x697460 adds the dialog with the board's help id; the button reads CLOSE). The quest help: QuestBoard slot 140 (0x6d0af0) the first
time the board is at rest in a quest of the map that has not shown it (`mQuestHelpSeen`); Bej3Dialog::Update (0x76a8a0) tells the board (slot 141, 0x6d0ac0) when
dialog 0x28 has faded out and the quest's play begins (slot 256). The help dialog marks its help seen in the profile (the exe does it in the destructor, 0x6770c0: bit of the
id, Classic's 0x15 also bit 0, Zen's 0 also bit 0x15; the port does it at the button, the only way out).

`move` (src/TestCommands.cpp). Each of the dialogs has its footer button as the Yes button: a tutorial box's OK, a help's START GAME or CLOSE, the quest help's OK; `move`
clicks it at its middle (move, wait 3, press, wait 3, release) once the dialog has faded in, as for any other dialog. The two boxes to tick are never clicked. The test command
`hints off|on` sets or clears bit 19 of the profile's flags (before the first player exists, `move` applies it once the name has made the profile), and every suite script now says
`hints off` after `name Tester`.

Suite. The frozen hashes are unchanged, Classic 51086c43ed2a and modes 873d262bb577: with hints off the board does not queue or stop for anything, so the states are those
frozen before. The only difference of the state files is the new command's own `OK hints off` line, which suite.sh's hash leaves out.

Verified. A real-input run of Classic shows the HOW TO PLAY dialog after the first 90 updates, START GAME answered by `move`, and a Flame Gem's box answered by `move` (the screenshots
were looked at). Every mode still reaches its end through them (tools/realinput.sh).

Port notes. The HelpDialog shows the title and the page texts in columns over the pages' rectangle (0x696100: 90, 150, width less 180, height less 330 of 1200ths), with no
picture; the exe's HelpWindow (0x68a7f0) draws a picture (PopAnim) for each page and the pages one by one. The box of a HintDialog has room under its text for "Disable Hints" by
60 of 1200ths (the port's measure) and the checkbox stands left of its label (0x6829c0 gives the label's place; the checkbox's own was not found).

Not done.
- The REPLAY button of a tutorial box and the copy of the move the exe keeps in each request (0x68 bytes: 0x742e60 puts the board back); the demonstration flag (+0x1412) is always 0.
- The pulse of the gem a box points at (the curve +0x26c8, drawn by 0x756cb0 and 0x752580), and slot 253's disabling of the buttons during the hold.
- The HelpWindow's pictures and its paging (0x694900 SetupHelp, 0x68a7f0 Draw).
- The multiplier gem's box (bit 4, 0x714dd0) and the coin's (bit 8): the port's Lightning makes neither gem; the dark rock's box (bit 0x10: DigGoal's 0x6c2ad0 keeps a piece id at +0x2c0 from 0x6b3700, for a
  record of kind 4, and asks the box); Wall Blast's goal tutorial (bit 0xc: 0x6aa580, 0x6aa620, 0x6c0430) and Diamond Mine's TutorialHeader/TutorialText (bit 0xf: 0x6ada10, 0x6b0fb0), which change the
  goal's timing and values and do not use HintDialog; Zen's flag 0x800 (0x7116c0) and Poker's 0x4000 (0x6dedd0) were not read further.
- QuestBoard slot 140's curve test (0x437c60 of an object the port does not keep) and the two curves of 0x6d09c0; the quest help of the secret modes (the exe reads the flag at index 1000 and more, past the flags).

Oddities.
- The exe stops FindMatches from collecting special-gem candidates for the rest of a call that queued a tutorial box (a local flag in 0x75af60), which seems to leave the first special unmade; the port
  makes the special as before and queues the box on it, so the box shows on the new gem.
- Lightning asks for help twice (0x773440: ids 0, then 10), so a fresh player gets two help dialogs, the first with Zen's pages; the port does the same.
- Where the exe puts a box above a gem (row 4 or more) the decompile mixes the grid's measure and a pixel offset (a 15 taken in pixels from a measure in 1200ths); the port subtracts 15 in 1200ths.
- The quest help's flag is read at `mQuestIndex` below 0x28 only; the quests of the map reach 0x28 or more only if quest.cfg grows.


## Focus (Board's pause, Bej3App's GotFocus and LostFocus; src/Board.cpp, BoardDraw.cpp, BoardMoves.cpp, TestCommands.cpp)

What the exe does when the window loses or regains the focus (or is minimized or restored), by its code; every address is of Bejeweled3.exe.

The chain is PopCap's. The driver's window procedure (0x5b9xxx, the size message) keeps the minimized flag (app +0x63b) and, when the app's mute-on-lost-focus byte (+0x479) is set, mutes
(slot 57, 0xe4, with 1) as the window is minimized and unmutes (slot 58, 0xe8) as it is restored. Then 0x5bf7b0 (RehupFocus) works out the wanted state, active (+0x63a) and not minimized, and
when it differs from the app's `mHasFocus` (+0x63e) stores it and, **on a regain**: Unmute(1) if +0x479, the widget manager's GotFocus (0x5853e0: its flag at +0x9c, and the focus widget's slot 45),
then the app's slot 78 (GotFocus); **on a loss**: Mute(1) if +0x479, the widget manager's LostFocus (0x585410: every key that is down comes up, its flag clears, the focus widget's slot 46), the
app's slot 79 (LostFocus), ReleaseCapture and 0x584ce0 (the held mouse button comes up). The framework's `RehupFocus` (SexyAppBase.cpp) does the same; the port's `mMuteOnLostFocus` and the
`focus lose|regain` test commands (docs/REAL_INPUT.md) already reach it, and the web page's blur and focus too. No widget class of the game overrides slots 45 and 46 (the vtables of every
board, dialog and menu have Widget's; the name box's EditWidget does), so the widgets themselves do nothing on a focus change.

Bej3App's two slots. Slot 78, GotFocus (0x76ac80), and slot 79, LostFocus (0x76ac20), act on the sound manager at +0x5d8 (CustomSoundManager; its slot 5 sets the volume of a kind of sound):
LostFocus gives kinds 2, 3 and 4 (Zen's ambient sound, binaural beats and breathing voice) the volume 0, GotFocus gives them back +0x998 (ZenAmbientVolume), +0x9a8 (ZenBinauralVolume) and +0x9b0
(ZenBreathVolume). They are not the music (the app's Mute, which the exe does not override, silences that). Slot 64 (0x76afc0), which was suspected, is not a focus slot: slots 65 and 66
(0x46daa0, 0x46dad0) call it with the windowed flag and the 3D test, so it is SwitchScreenMode(wantWindowed, is3D, force); after the base's it gives the windowed button (+0xd70) a call
(slot 31) with 1 when the app is going to full screen and 0 otherwise. It belongs to the screen-mode pass.

The board's pause. This is what a lost focus does to a game, and it is the same machinery as the space bar.
- Fields of Board: +0x12c4 the player's pause (`mPlayerPaused`), +0x12c8 the fade of the gems (`mPauseFade`), +0x12cc the alpha of the word PAUSED (`mPauseAlpha`). The constructor (0x74ae30) clears
  the floats; the reset 0x74e410 (NewGame) clears the alpha and the flag.
- Slot 107 (0x1ac), CanPause, is on for a mode with a clock: Board's 0x73c220 asks slot 104 (the time limit) and says whether it is not 0. So Classic, Zen and the turn-based quests cannot be
  paused; Lightning (60), the time-based quests, Diamond Mine (DigBoard 0x6c4a20), Ice Storm (InfernoBoard 0x6c50b0), Balance (0x6c53b0) and the real-time Time Bomb (0x6c4ab0) can; the Time Bomb
  board answers with its real-time flag (+0x3262, 0x6c4a90).
- Slot 103 (0x19c), IsPaused (0x73c260): no when the player has not paused, the app has the focus and neither the options (0x15) nor the reset (0x16) dialog is up; else CanPause and not +0x1410
  (a flag the port keeps clear). The goals and boards of a quest read it (0x6a9aa0, 0x6a9e70, 0x6aa040, 0x6aa7e0, 0x6aa880, 0x6b76c0, 0x6c24c0, 0x6c2910, 0x6c2ad0, 0x6e00e0, 0x6e5850) to hold
  their own counters (Not done below).
- Slot 47 (0x73b670), KeyChar: the space bar toggles +0x12c4 when the mode can be paused or is paused (and +0x1410 is clear and the 3D cursor's id, +0x2c44, is -1).
- 0x73f140, MouseDown: while +0x12c4 is set or the alpha is above 0.5 the click only clears +0x12c4 and goes no further.
- 0x767090, the board's tick, after the under layer's effects update: the game runs (the two floats go down by 0.075, if the zoom curve at +0x2908 is 1) when the player has not paused, the app has
  the focus, no quest help (0x28; with the bar-rate curve at +0x2258 at 1, which it always is), options (0x15) or reset (0x16) dialog is up, or the mode cannot be paused (or +0x1410). Otherwise the
  fade goes up by 0.035 (to 1), the alpha too when the app's +0x39c is 0 and the help hold (+0x2b40) is 0, and **the tick returns once the fade is 0.75 or more**: about 22 updates after the
  loss the game stops, a Lightning clock included. The board's own update (0x759170) keeps running, so the update count goes on; the tick's help-dialog return (0x13) and the hold come
  after the fade in the exe.
- 0x767e20 sets the alpha of the two effects layers (+0x12c0 over, +0x12bc under) to slot 175 (0x73bd10: slot 174, the cover curve, times 1 less the fade), each update; the effects layer's
  own update (0x7303f0) does nothing while the fade is 1. The gems are drawn at that alpha, so they fade away.
- 0x756cb0, the draw: in a mode that can be paused, while the alpha is above 0, the word PAUSED (string 0x93, FONT_HUGE, resource 17) in white at the alpha, centred on the grid's middle, baseline
  0x21c of 1200ths. The exe's block is for a cover below 1 or no end hold (+0xd1c below 1).
- QuestBoard's update (0x6dd8a0) stops its clock and goal only for +0x12c4, the player's pause, not for the lost focus; with the focus lost a quest's clock goes on while the board is at rest
  (it counts down when 0x75fca0, at rest, says so), as the exe has it. Poker's update (0x6dedd0) and Zen's own pause (+0x33b0, 0x7116c0) read the flag too.

What the port does. The same in Board: `mPlayerPaused`, `mPauseFade`, `mPauseAlpha`, `CanPause` (virtual, Board's from `GetTimeLimit`), `IsPaused`, `GetBoardAlpha`, `KeyChar`,
`UpdatePauseFade` (called by `UpdateTick` after the under layer's update), `DrawPausedWord`, the click in `MouseDown`, the layer alphas in `UpdateGame`, the effects layer's stop at fade 1, and
QuestBoard's player-pause test. Classic's answer is unchanged: it cannot be paused, so the update count and the hint timer go on with the focus away and after (`tools/web/pagecheck.js`, whose
start now names the player, turns hints off and waits for the menus' intro). Lightning's, from `pagecheck_native.py ... --first "goto lightning"`: its game ticks and clock stop after the fade
(gameticks 425 at both samples with 200 updates between) and go on after the regain; a screenshot shows the word PAUSED over the faded board. The test command `state` gains, only while a pause is
on or fading (so a game that never pauses prints what it printed), `"paused"` (IsPaused), `"playerpause"`, `"pausefade"` and `"pausealpha"` (thousandths), before `"over"`. In a Lightning game
`key space` pauses (fade 1000 within 40 updates) and a second one resumes.

Dialogs across a lost and a regained focus (looked at with screenshots and `state`): a fresh player's Classic with hints on: the first game's help dialog (HOW TO PLAY, 0x13) stays up, the board
stands at update 0 behind it, and `move`'s click on START GAME after the regain answers it; a Flame Gem's box (0x12) likewise; Classic's end dialog opens at the same update with or without a loss
before it and survives one; in Lightning, a loss with the end dialog up fades the gems (fade 1000) and leaves the word's alpha at 0 (a dialog is up), and the dialog still works.

Not done.
- The sound kinds 2 to 4 of slots 78 and 79 (Zen's ambient, binaural and breath sounds are not made, see ZenBoard.h).
- The slot 103 reads of the quest goals and boards listed above (they hold their own counters for the first 22 updates of a fade; the tick stops after that) and Poker's and Zen's own pause.
- The reads of `mHasFocus` that start a fade or a sound only with the focus (0x49cfe0, 0x6733e0, 0x6a3530, and the helper 0x76ab60 called from several places): the port's sounds are muted by Mute anyway.
- The tick's order: the exe puts the help dialog's return and the hold (+0x2b40) after the pause fade; the port keeps them in front of the update count (Board::UpdateTick), so a lost focus
  during the help hold of a mode with a clock does not move the fade until the hold has run out.
- The zoom curve (+0x2908) that gates the fade-out, +0x1410 and the 3D cursor id: constants here. The web page check has not been run with the new start.

Oddities.
- A quest's clock keeps running with the focus lost while the board is at rest (above), because the exe's pause for it is only the player's.
- With the focus lost the effects layers keep the alpha of the last update before the tick stopped (0.25 at fade 0.75), while the gems fade out to 0; the exe does the same, since the alpha is set in
  the part of the tick that no longer runs.
