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
  +0x454 and +0x460 colours; +0x484 and +0x485 flags; +0x486 and +0x487 flags; +0x368 a fade. Its Draw has a 3D branch (a
  10 by 30 mesh of the curved ribbon, taken when the graphics' flag at +0x80 is set) and a 2D branch (the 40-frame
  picture sheet, frame = update counter / 4 mod 40, the effects drawn by PIEffect::Draw 0x575650 under a scale matrix,
  then the text with the font layers GLOWOUTLINE and BOTTOMGLOW and a curve "CRYSTALBALL_TEXT_SCALE_FACTOR").
  Translated: Update's highlight only (src/MainMenu.cpp). Not translated, because this generation's Graphics
  (a transform stack, 0x432410 copy, 0x432590 SetColor, 0x432610 SetDrawMode, 0x432630 SetColorizeImages, 0x434330 and
  0x434350 Translate, 0x713770 and 0x713850 scaled draws, 0x58f620 and 0x58f920 font-layer colours) is not mapped onto
  the port's Graphics yet, and which resources the effects are made from was not found.
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
- The menu's slot 0x7c on the three balls (called in the constructor with 0 and at the intro start) is not identified;
  the port draws the plates only after the intro starts (MainMenuButton::mActive) and hides the side buttons before.
- The Update's background widget (+0x6d8, its Resize at curve +0x190's first value), the logo phase, the widgets at
  +0x6f0 to +0x6f8, the overlay widgets +0x700 and +0x704, the user dialogs, the loader's unloading and the stars
  (0xdc particles of the constructor) are skipped.
- The port loads neither partner.xml nor reads NumLogos, so the logo phase would not run anyway.
- ResizeButtons (0x6a09e0, run when curve +0x190 is triggered) is not translated; the app's viewport offset ([0x1c5])
  is taken as 0, and which of the ftol and sub-pixel assignments for PLAY comes first was read from the code, not
  checked against a running original.
- Draw: the alpha of the curve +0x2e8 (the background's fade), the foreground picture and the particles are still open.
