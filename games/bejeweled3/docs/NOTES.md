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
a tutorial box the first time a special is made; they are left out and the help is treated as seen. (3) The exe's sets are ordered by
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
matches, 15 activity (the idle shine's rate). The profile's statistics (Profile::mStats) are not written, and the other 38 callers of
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
233, 0x73b100), so the end dialog (id 0x26) comes 674 updates after the game over (`mEndScreenShown`, the dialog is not made). Left out
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
  ParticleEffect in the layer at +0x12c0. None of these is made yet. The Rand() draws they would make (one per ParticleEffect at its second update:
  Star Gem two, Hypercube one, a player's hint one) are not in the logic either: the comments of MakeStarGem and MakeHypercube say "no Rand()", which holds
  for the Effect constructor (0x729460, type 0x16/0x17) but not for the PIEffect's first real update (0x573290: Rand() for the seed).
- (kept) the gems' other pictures: the Flame Gem's PIEffect (PIEFFECT_BLASTGEM), the Star Gem's and the Hypercube's pictures and effects (0x745a60, 0x755030,
  0x754c60, the flag bits 4 and 0x2000 at the end of 0x757df0), the selection and hint glows (+0x240, bit 0x200), the blend of two cels round a turn (a PIEffect
  "Params" when the board's alpha is 1), the pass order of 0x7568a0 (plain, special, flame gems with the effects between) and the piece's alpha curve
  (+0x90, constant 1). Gems are drawn in one pass in the grid's order.
- The panel's slide in and out (curves +0x2980 and +0x2a60, the whole panel and its HUD move by them), the picture behind it while +0x2bb8 is not 0, the glow while +0x27d0 is
  above 0, the y shift +0x1194, the glows of the frames while the bar's stage is on (slot 218), the overlays 0x7501c0 and 0x751500 (the buttons drawn by hand), and
  the hover curves of the buttons (+0x150, +0x158).
- The level change's overlay (HyperspaceFallback, 0x724110) and the game over screen (the end dialog, 0x26) are not drawn: only their state is translated.
- The Menu button (above), the click sound (0x76aaf0) of the buttons, and 0x742db0 (run when +0x1188 comes to 0).
