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
- Not translated: the mirrored drawing (nothing in the game sets it), the debugging outlines, the transition between
  two animations (the game never starts one), saving and loading an animation's state (0x445540, 0x4453b0, 0x445760,
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
