# Talismania Deluxe: working notes

The session's own notes while porting: where the work stands and what the export taught that is not yet in code.
`GAME_NOTES.md` will hold the finished port's description.

## Sources

- The game's files: `python -I games/talismania/tools/stage_talismania_assets.py` copies `Talismania.saf`, `music/`,
  `splash/` and `properties/` from the Steam copy into `assets/talismania/`. The loose `splash/` files are read with
  loose files switched on (the loading screen, below).
- The exe: `re/talismania/bin/TalismaniaGame.exe` (git-ignored), the game PE carved out of the DRM wrapper. The export:
  `re/talismania/gh/` (`all.c`, `functions.jsonl`, `strings.jsonl`, `vtables.jsonl`, `decomp/`). There is no Mac build.
- RTTI: `python -I tools/rtti_msvc70.py re/talismania/bin/TalismaniaGame.exe re/talismania/rtti/tal --functions
  re/talismania/gh/functions.jsonl` gives `tal.classes.json` (836 classes) and `tal.jsonl` (`Class::vfNN` names).
- `tools/fn.py <addr>`: a function, compacted as Feeding Frenzy 2's, with each call of a virtual function annotated by
  its RTTI name (`--info` its callers and callees, `--vt` a vtable, `--asm`). `tools/classdump.py <class>` and `--find`.
  `tools/saf.py list|cat|extract` reads the archive (the entries' names count their NUL in the length).

## The engine

Sprout's second generation, shared with Feeding Frenzy 2 in `engines/sprout2` (docs/ENGINES.md). The engine's own run,
frame and window procedure match Feeding Frenzy 2's: run 0x677bd0 (Feeding Frenzy 2 0x4383b0), the game's creation
0x678230 (0x49bc60), the frame 0x6784b0 (0x438700) and the window procedure 0x67a2d0 (0x424b30). `src/TalismaniaApp`
is the port's host over the framework, copied from Feeding Frenzy 2's `FeedingFrenzy2App` with Talismania's addresses:
800x600, 90 frames a second (the options' `targetfps`; the low-priority rate is 4). It should move into
`engines/sprout2/host` as one template the two games share; asked of the coordinator at the first merge request.

Differences of Talismania's window procedure from Feeding Frenzy 2's, to be handled in the looks pass: after each mouse
event it sets the cursor (0x67a280: the mouse cursor manager's image, else the arrow); an input recorder/player object
at the engine's +0x2d0 (its flag at +0x50) takes the mouse and keys before the signals when it is on (not reached in play);
the engine's own timer messages; and Alt+F4 (0x104 with key 0x73) is its own close.

## The boot (0x401100)

The game's own boot, called by 0x678230: loose files on; if `Talismania.saf` is on the disk, log "FOUND Talismania.saf, USING
ARCHIVE", loose files off and the archive added (the global 0x813aa2 set); then the data set named "baseset" is loaded
(its loader reports each file's description while the boot waits on it with `Sleep(100)`), the application options "std"
read and the application made from them ("__rootapprestore" is the instance's name).

## The data framework

The game is data-driven through `config/*.xml` files read into "classes" by loaders: `AtlantisLoader`, `DataSetLoader`
(a loading thread, `ILoadingThread`), and for each kind a `<Name>Class` of data, a `<Name>ClassLoader`
(`DataItemLoader<T>` over `AbstractDataItemLoader`, an XML sub-handler) and an instance made from it
(`AtlantisClassInstance<T>`). `config/datasets.xml` names the sets (`<datasetloader name baseclass>` with `<load file
description noupdate wait theme>`); "baseset" is `config/scenes.xml` and `config/gameoptions.xml`. The application's data
(`appoptions name="std"`) holds the layout names, the title, `targetfps`, `screendimensions` and `initialdataset`. About 40
class kinds, each loader to read for its exact attributes.

## Loader facts (read from the export)

Params getters in this exe: 0x64f2a0 string, 0x64f380 int, 0x64f4e0 float, 0x64f640 range, 0x64f7a0 bool, 0x64f920
floatpair, 0x64fa80 intpair, 0x64fbc0 rect, 0x64fd00 color, 0x64fe90 pivot. Talismania's pivot names are center,
upperleft and lowerright, unlike Feeding Frenzy 2's: a per-game switch in the shared engine when pivots are reached.
A `baseclass` copies the base item's fields, and an attribute's default is then the field's value. The highscore
`<entry>` reads its `score` default as `(rand() % 0x76d + 100) * 10` even when the attribute is there: one C runtime
draw per entry at load (kept, `engine::crtRand`). The list tokenizer (0x437930) splits at commas outside brackets; whether
it skips empty tokens or trims is not confirmed (the base data has none). `tools/loaderattrs.py <addr>` prints a
loader's attributes with defaults; `tools/classstrings.py <class>` the strings of a class's functions.

## Done

- `src/AtlantisData`: the base data set's kinds (scene, gameoptions, appoptions, debugmanager, mousecursormgr,
  highscorescreen, gamemode) and the data set list, read by one dispatching handler; the boot loads
  `config/datasets.xml` and "baseset" without errors.
- `AtlantisApp::start` makes the scene (`AtlantisScene`: camera, shake and screen containers, the layers as plain, clipping
  or darkening containers, the camera put at the region "sky") and enters `LoadingState` (0x4060a0), which shows
  `splash/loadingscreen.xml` in the "skymenu" layer: the loading screen is on screen (the logos, the bar's frame). Not yet:
  the initial data set's loading, the bar's progress, the layers' darkening and the camera's scrolling (0x5e6680), the sound
  engine and the two popup menu stacks (0x5e98e0), the signal connections. `ClipPane` and `RectangleItem`
  (`ScenePieces`) should move to engines/sprout2. `ScreenDocHandler` logs that `compositeGroup` is not translated.
- `AtlantisTheme` (over the shared `engine::Theme`, `getImage` virtual): every theme file of a data set added to one theme;
  images loaded when first asked for (pivots by Talismania's names; the alpha type `mask`, trim and scales other than 1 are
  logged, not translated); fonts, colours and vectors; sounds, music, composites, animdata, animation and particleEffect
  kept as raw attributes (`mRawEntries`) until their users are translated. The loading state reads the "std" set a file a
  frame (the original: a thread) and scales "loadbar" by the progress; its exit to the title state is not yet translated.
  About 40 data kinds of the set are still logged as not yet translated.
- `TitleState`: after loading, the main menu (`mainmenulayout`, with the theme) is made and shown in the "skymenu" layer:
  it renders with its art. Still to do for it: the front end's other screens and the two menu stacks (0x4039b0, 5 KB; the
  stacks 0x5e98e0 hold normal, popup and modal screen lists with the darken layers), the hero button's state (0x406910), the
  strings (`gamestring`, so the labels "Welcome Back!" and the user name are blank or doubled), the registration badge and
  buttons (the trial badge, "whybuybtn", "moregamesbtn": to be removed, not disabled), the actions.
  The app's IAtlantisApp interface at +0x20: 0x407350 pushes a menu on one of the stacks, 0x4073b0 pops, 0x407f30 shows a
  popup screen by name.
- The host builds and runs (`build/native/talismania.exe`, a silent run under rundesk): the window opens, the archive is
  found, a black frame is shown. The boot's archive step.

## Next

- The data framework: the XML class loaders, `baseset`, the application options, the application made from them.
- The configuration (0x402aa0) and the start (0x402d60), the loading state (0x4060a0) and its screen.

## Front-end screens (menu stack)

`MenuStack` (0x5e98e0, push 0x5ea050, darkening 0x5e9d40) is translated in a reduced form: the sky stack puts a screen in
"skymenu" or, as a popup, "skymenupopup", and the dark layers fade to their opacity over their time while any normal
or popup screen is up (the original's two dark levels and the modal list are read only in outline; assumption noted in
MenuStack.h). `FrontEnd` makes the main menu, options, high scores, credits and quit confirmation (0x4039b0) and
answers their actions as 0x407510 does: optionsmenu, highscores, credits, quit open their screens; optionsDone,
highscoresDone, quityes, quitno close or quit. The click dispatcher's strings run on to startstorymode, startheromode,
manageusers, creditsdone, ok, closejournal, unlocknow and unlocklater (not yet). The credits layout has no action of its
own; what closes it is still to find. Dialog backgrounds are missing until the engine translates a screen image's
scale (the "scaled by ... not yet translated" lines in the log, a shared gap).

## Starting a game (read, not yet translated)

The main menu's click handler (0x407510 to about 0x407c30) answers `startheromode` and `startstorymode` alike: it pops
every screen the front end holds (the app's +0x3c, +0x74, +0x60, +0x30's +0x34, +0x4c and +0x44, through the menu
interface's slot 0x18), looks the game mode up by name in the registry ("hero" or "story"; 0x40e330, then 0x40cc90 and
0x408c10 store it in the app's current-mode reference) and then calls slot 0 of the object at the app's +0x19c, which the
app's +0x1c0 points at: the "start game" step that enters the play state. What that object does, and the play state
that follows, are the next things to read; the data the first board needs (chapters, levels, cells, paths, huds and the
rest of the std set's kinds, listed in "Done") is still untranslated. The credits action pops the main menu and pushes
the credits (0x407ab4, 0x406a90); nothing in the credits layout closes it, so a click does here.
The options screen needs `checkBox` and a slider in the engine; the high scores screen draws no background of its own
(the scroll images in its layout are commented out), so where the original gets one is not found.

## Data kinds translated for a board (chapters, levels, boards, cells, paths, pools)

`LevelData` (chapter 0x5a2350 and game level 0x5b2b00), `BoardData` (board 0x5b0e00, pots, column templates), `CellData`
(cell class 0x5a9a10, layers 0x5ac1c0, sounds, energize map) and `PathData` (path class 0x5dcfb0, path image class
0x5b8030 with its endpoint and connection pictures, path id pools, cell spawning pools) each have a struct header and a
reader the data handler offers every element to first. Notes: a `<layers clearbaselayers="true">` drops the base class's
layers (0x5abf10; the data's `clearoldla` is a typo nothing reads); a layer named path, sparkle or reward, or marked
`container`, holds the path or the reward and defaults to the view mode `__codecontrolled`; a layer's onclick values are
nothing, rotate, rotatesnap and (the data's only other word) animate. Not yet read: cell rewards and their actions
(giveactions, prepareactions), the path length meter, the hud, the map, unlock processors, sparkles, the stats and the
rest of the lines the log lists as "not yet translated". The pools keep their items in file order as sections (one
`<random>` or `<selector>`, or a single item); how a section is drawn from is for the game manager's step.

## The first board (reduced, drawn)

Choosing Story or Hero runs `AtlantisGameMgr` (reduced; `GameMgr.cpp`): the mode's default difficulty names the game class,
its data set is loaded, the "Level Container" goes in the scene's main layer (0x41ca20), the first chapter of the class's
progression is found (the progression's lower-case names meet the chapters' upper-case ones; where the original makes them
meet is not found, so they are compared without case), the chapter's pre-data set and data set are loaded at once (the
original loads a file a frame behind a loading screen, states 0x414da0 and 0x415050) and the first level of its
progression becomes a `GameLevel`, which loads its layout screen (levellayoutfile) and builds a `Gameboard` into the
layout's "boardcells" group. Geometry read from the data and checked by eye: pots are 62 across and 72 up
(`potprops size`), odd columns half a pot higher, row 0 at the bottom, the group's origin the middle of the bottom pot of
column 0; the hexagon sprites are 83 by 83. The pots come from the column templates (vspan) and the layout's own pots
laid over them (the original merges attribute by attribute; here only the initial cell, path class, path id and tooltip).
A pot's first cell is its initialcellclass or a draw by weight from the board's "default" pool (one draw of
`engine::randomFloat`; the original's draws, its order and the falling in from the spawning pots are not read), drawn
from the layers of the cell class shown in the "normal" view mode. Not yet: paths on the cells, the side pots, the
spawning pots, the level's states, the hud, the map, the journal, the camera's move from the sky to the land (here
`setRegion("land")` at once, and nothing paints a land background, so the front end's last pixels show through).

Paths on the cells: a cell's path id is the layout pot's initialpathid, else the spawn item's, else the cell class's
explicit one; "default" draws from the board's path id pool of that name (weighted, then the rotation "__rand" turns every
direction by crtRand() % 6; the original's draw order is not read). The "path" layer draws one endpoint picture of the
path image class per direction, or the connection picture where two directions face each other (d and d+3). The
endpoints are theme composites (the light picture masked by overlaymask), built in `AtlantisTheme::getComposite`: only
the action "mask" (PixelCombinerMask 0x6058f0/0x6194b0, each channel times the mask) exists in the data; trim draws the
same picture and is not done. Seen on the board: the connection pictures overhang the cell (an unmasked or untrimmed
picture? not yet judged).
