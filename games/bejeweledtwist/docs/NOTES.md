# Bejeweled Twist: working notes

What the analysis has found about the exe's framework generation and the game's classes, for reading the export.
The plan and the region map are in `PORT_PLAN.md`; the names are in `symbols/*.jsonl` (`tools/ghidra_export.sh`
applies them; `tools/decread.py` and `tools/asm.py` read them too).

## Reading the export

- `_ftol2` takes its argument on the x87 stack; `tools/ghidra/FixFtol.java` gives it that signature, so the decompiler
  shows `_ftol2(expr)`.
- Ghidra's `CRect::CRect(p, r, g, b, a)` is `Color(r, g, b, a)` (the MFC name is a wrong library match).
- The decompiler drops `this` (ecx) of `CurvedVal` calls and the string arguments of `std::string` temporaries: read
  them in `tools/asm.py <function>` (`lea ecx, [esi + 0x...]` before each call; the pushed string is annotated).
- `DAT_00e0b698` is the app (`gApp`); `+0xf30` is `mScale`, `+0xc8`/`+0xcc` `mWidth`/`mHeight`.

## The framework generation

The exe's SAF is 1.3 with a few fields added. Widget's vtable is 1.3's exactly (70 slots; `tools/saf_vtable_layout.py
E:/retools/refs/sexy13/source/SexyAppFramework --dump Widget`). Widget's fields are 1.3's with an 8-byte list
iterator (VS2005 checked iterators): from `+0x20` on, 4 bytes later than `tools/saf_struct_layout.py` prints, so
`mUpdateCnt` +0x28, `mX`/`mY` +0x30/+0x34, `mWidth`/`mHeight` +0x38/+0x3c, `mHasAlpha` +0x40, `mClip` +0x41,
`mPriority` +0x4c, `mZOrder` +0x50, `mVisible` +0x54, `mMouseVisible` +0x55, `mDisabled` +0x56; sizeof 0x88.

SexyAppBase: 1.3's fields 8 bytes later from `mWidth` (+0xc8) on (`mWidgetManager` +0x328, `mDDInterface` +0x374,
`mUpdateCount` +0x48c, `mHasFocus` +0x4da, `mLoaded` +0x503, `mLoadingFailed` +0x505, `mNumLoadingThreadTasks`
+0x510); the property maps and `mResourceManager` (+0x64c) are 0x24 later (1.3's layout tool gets `Rect` wrong
there). Its vtable follows 1.3's at least to slot 49 (`Init`); WinTwistApp's overrides are listed in
`symbols/20_classes.jsonl`.

## The app

- The art size (`mArtRes`, +0xf2c) is 1200, 768 or 600 (constructor 0x766ce7); `mScale` (+0xf30) is it over 1200.
  Without Direct3D the game switches to 2D (0x760659): art 600, 800x600, 16 bits. The port runs that mode.
- `CustomResourceManager::DoLoadImage` (0x6f3b75) loads `images\1200\X` from `images\<art>\X`, except a `2dbig`
  image in 2D mode; `images\!` is a blank image. `DoLoadFont` (0x6f3d65) takes `path768`/`path600` when the art is
  that size and loads a `!ref:` font's base from the Dynamic group first.
- Resource ids (0x44e = 1102 of them) come from the exe: the shipped resources.xml lacks the 20 `SHADER_` ids (3D,
  loaded by hand in ExtractLoadScreenNonReleaseResources) and orders three images differently (`tools/resgen.py`).
- Init (0x7652cc) loads LoadScreen, LoadScreenNonRelease and (2D) 2DOnly, sets the cursors and shows the loader
  screen unless PopCap's SexyCache server has the pictures (global 0xe48aa4; the port has none). LoadingThreadProc
  (0x76497d) loads NonResize, LoadingThread, the 3D models and Sounds, after the Tube group (0x763092).
- `-record`/`-playback` (0x76350c) record and replay input to `recording.dat`: useful for comparing the port with the
  original later.

## The music

- Two MO3 modules, each holding many tunes from their own orders: song 0 `music\bejewel twist 06.10.mo3` (37
  channels, 165 BPM) and song 1 `music\BejeweledTwist24.mo3` (Blitz's, 18 channels, 133 BPM).
  `CustomBassMusicInterface` (vtable 0x7e1c5c) adds tracks to the framework's BASS music: a track has its song, the
  order it starts at, a tempo for each of two states (0 normal, 1 danger: interface +0x18, set by the board) and a
  volume per channel per state. Playing a track (0x6edc8a, slot 21) sets every channel to 0, then the state's tempo
  and the channels' volumes, then plays the song from the track's order; Update (0x6edfbd) sets the tempo when the
  state's changes and steps every channel's volume by 1 a frame towards its state's, times a factor (+0x2c, 1).
- 0x763ca4, in Init just before properties\default.xml, reads properties\music.xml and makes the tracks: 100 on for
  the game's tunes (`MUSIC_INGAME<n>_OFFSET`, `_TEMPO_NORMAL`, `_TEMPO_DANGER`; channels 0 to 3 silent in the normal
  state), 0 the main menu, 200 win, 201 lose, 202 loading, 203 and 204 the wheel, 205 the trophy room, and Blitz's
  300 to 308 (0x7f3cfc, their offsets named at 0x850ec8) on song 1. `WinTwistApp::PlayMusic` (0x75fc23) plays one at
  once or fades the music out first; UpdateFrames starts the waiting track once song 0's volume is 0. Leaving track
  305 (the turn-based challenge's) puts its channels back.
- BASS 2.2 (the game's bass.dll, version 0x00020002), measured on a copy with its "no sound" device: the channel
  volume attribute (`0x100 + n`) reads 100 for each channel until set and -1 past the last, and keeps to 0..100; the
  BPM attribute (3) keeps to 1..255. The framework's `GetMusicChannelVolume` and `SetMusicTempo` follow it, and
  libopenmpt counts the same channels.
- The tracks play without BASS's loop flag. The main menu's (orders 54 and 55) loops by its own jump; the loading
  screen's (order 70) plays about seven seconds and ends. Both are to be checked against the original's music.

## The loader screen

LoaderScreen (0x668 bytes; Widget + ButtonListener at +0x88): `mPlayButton` +0x8c (PlayButton, 0x2e0 bytes, a
TwistButton : ButtonWidget), `mLogo` +0x90 (Logo, 0x3c8 bytes, a TwistWidget : Widget), the app +0x94, the loading
progress +0x98 (double) and its smoothed value +0xac (float), `mDone` +0xa8, 13 CurvedVals at +0xb0 + 0x70 n (their
curves are exe strings, read with `GTS`), `mFinished` +0x660 (WinTwistApp::UpdateFrames then removes it and deletes
the LoadScreen group), a countdown +0x664 (150 updates; at 125 the app's slot 54 is called).
Methods: Update 0x71ff54, Draw 0x71f5c0 (a black fade), DrawAll 0x71f65f (the PopCap logo, the glass tube and its
fill, the glow bars), Resize 0x7202b4, MouseDown 0x71ff31.

TwistWidget (vtable slots 22 Update 0x755eec, 26 Draw 0x75602c, 27 DrawAll 0x755cd2, 50-54 the mouse) has a fade in
and out (+0xa4, +0xa8, +0xac), a tooltip text (+0x88, drawn with FONT_MENU_TINY), a background image (+0xb4) and a
zoom on show (+0xcc, two CurvedVals; in 2D a scale about its centre).

Ported (LoaderScreen, PlayButton, Logo): the loader comes up as the original's does, with the app's EffectsManager
(+0xf78) in front of the Logo in front of the loader (LoaderScreen::AddedToManager), and Hyperspace behind them all
(0x15e0 bytes, app +0xf20, z order -101, put up by 0x75f588 just before the loader). Still missing is the main menu that
slot 110 (0x76657b, `StartMainMenu`) brings up when loading is done; until then only space shows once the loader
finishes.

Hyperspace is the jump between levels as well as the space behind every screen. In 2D its Draw (0x713e2d) draws
IMAGE_STATIC_SPACE centred, the tunnel of a jump for transitions 8, 9, 10 and 14 (0x711b65) and a white flash (+0x1320,
set only by world 14's curves); everything else (the level's worlds, the camera, the planets of DrawOverlay) is
Direct3D only. SetTransition(0) (0x7181b3) only stores the transition and calls Update; the world switch by level runs
for transition 8 with Direct3D. Its Update draws no random numbers outside a game: the thrust particles run only while
the board's +0x200c points at it.

The framework's Is3DAccelerated says whether SDL's renderer is accelerated, which it usually is; WinTwistApp hides it
with one that always says no, since the game must take its 2D paths (the 2DOnly resources, the effects' 2D drawing).

Random draws on this screen, in order: Logo's constructor (one Rand() for the shine delay), each update of the Logo
once its drop is done (Rand() % 25, and for a sparkle RandSigned() then Rand() % the number of bright pixels), the
Logo's shine (Rand() % 200 when it starts again), and the PlayButton's sparkles every 8th update once it is in (three
RandSigned()).

## Drawing helpers and effects

Constants: many doubles in the exe are floats widened (0.1 is 0.10000000149011612); the port writes them as the exe
has them. `tools/consts.py` prints the values a function reads.

GraphicsRecorder (gGraphicsRecorder 0x85d6f8, 0x1c bytes): every image draw of the game's helpers goes through it.
Only the board sets its flags (+0x08 skip drawing, +0x09 record); otherwise it is the Graphics call it wraps.

DrawImageCentered (0x757137): unturned, it places the image by one cel's size (GetCelWidth/Height), whatever the
source rectangle; turned, it uses a Transform. DrawImageScaledCentered (0x756ee7) takes alignment flags (1 centre x,
2 centre y, 4 bottom, 0x10 right) and uses a Transform only when both scales differ from 1.

Effect (0xc0 bytes) and EffectsManager (0xbc bytes): the effects are a multimap keyed by type (+0x88), so they update
and draw by type, and within a type in the order they were added. The common motion (velocity / mUpdateDiv, the
scalars, gravity, alpha and scale steps, the scale and alpha limits) is ported, and type 0; the types 1-43 are big
switch cases in the constructor (0x6f4a05), Update (0x6f8792) and DrawEffects (0x6f6451) that come with the board.
Type 44 effects are objects of their own (virtual Update and Draw). The list at +0x94 holds heat waves, Direct3D only.

## The main menu

StartMainMenu (app slot 110, 0x76657b) runs once the loader is done. Unless +0xf85 is set (then it only starts game
mode 1 through 0x760492), it creates the Gates (0x75ec15: a Sexy::Gates of 0x3d0 bytes at app +0xc70, z order the
VizManager's + 15, centred) and the Menu (0x75ecb8: a Sexy::Menu of 0x4b8 bytes at app +0x864); resumes a saved game
when +0x930 is set (game mode +0x934; mode 2 shows Hyperspace and runs 500 updates); creates the RegisterWidget (0x98
bytes, app +0x924, z order 99) and the windowed-mode button (a ButtonWidget id 0x2a, IMAGE_WINDOWEDBUTTON's three cels,
app +0x91c, placed by 0x75e966 at the top right); prerenders the gems at seven sizes (0x7620a1: app +0x7f4 a 7x7 table
of IMAGE_GEMS cels and +0x7f8 seven of IMAGE_BOMBGEMS, scaled from the art's size by 1.5 - (n+1)/8); and, without a
saved game, shows the menu (0x765e04(0, 0, 0)). Last it updates the Gates and the Menu once.

The Menu's classes and code ranges: Menu 0x699a11-0x69c529 (vtable 0x7d9efc; 20 AddedToManager 0x69c05f, 21
RemovedFromManager 0x699ae1, 22 Update 0x69b642, 26 Draw 0x69a695, 27 DrawAll 0x699bbd, 40 Resize 0x699a40; its
ButtonListener's ButtonDepress 0x69c529, its TwistDialogListener's DialogButtonDepress 0x699d33, its
EditListener's EditWidgetText 0x69baaf, AllowChar 0x699dc6, AllowText 0x69a881), MenuButtonWidget 0x69d48b-0x69de89,
HighScoresPanel 0x694076-0x696a02 (0x350 bytes, Menu +0x2dc), HelpPanel 0x6981b0-0x6984b0 (400 bytes, a PopAnim
listener, +0x2e0), OptionsPanel 0x69e0f1-0x69fc10 (0x1a0 bytes, +0x2ec), ProfilePanel 0x6a041f-0x6a0e33 (0x1f0 bytes,
+0x2e8), ChallengeMenu 0x6dd2cd-0x6e665e (0x9e8 bytes, +0x4a8, z order -98), Gates 0x6fead4-0x6ff4f7, RegisterWidget
0x73b020-0x73b188, TwistDialog 0x754acb-0x7558fe, VizManager 0x7587a0-0x758f3e. The Menu also owns an EffectsManager
(+0x3e0, z order its own - 1) and takes over the loader's PlayButton (+0x2e4, now with IMAGE_MAINMENU_PLAYBUTTON).

The menu is three screens wide and three high, centred on the screen; it pans (SetPan 0x69af49) to show a panel:
the middle for the spiral button, the top row for the options, the middle row's left for the high scores and right
for the help, and its y at -1.5 screens (+0x2f0) for the profiles, which ride above the gates. mPanel (+0x3d8) is
0 main, 1 profiles, 2 scores, 4 options, 5 help. The gates' five buttons send their presses to the menu, which flips
them to the panel's own buttons (back, the score lists, previous and next page) and turns their glow (the covers)
off while a panel is up. ShowMainMenu (0x765e04) puts the menu just behind the gates and shuts the gates, unless the
loader is still up (the loader shuts them itself).

Ported: StartMainMenu, ShowMainMenu (without the board), the Gates, MenuButtonWidget, the Menu and its colour
cycles (ColorCycle, 0x6e6e1b-0x6e70ad, four of red, yellow, green and cyan a quarter apart), the windowed-mode
button, the profiles' files (Profile), the ranks' names (Ranks), the profiles panel (ProfilePanel), the new player
panel (NewUserPanel, menu +0x3d4, 0x699924-0x69ab42), the game's dialogs (TwistDialog), the high scores and the
high scores panel (HighScoresPanel; without the new score going into its list, the board behind it and the replays,
which come with the end of a game), the help panel (HelpPanel) and the options panel (OptionsPanel, without its
buttons over a game). Not yet: the challenges, the end of game stats (EndStats), the registration widget, the gem
prerender, the app's music, and starting a game.

The ranks (app +0xf58, 0x739af7) come from properties\ranks.txt, one name per line in its first comma-separated field
(quotes kept through commas, then dropped). Their points are not in the file: 0, 15, then 30 more, then 25 more each
step than the step before. A player's rank (profile +0x1ec) is -1 until the tutorial is done ("Trainee", a game text).

The new player panel's edit box is the exe's own EditWidget build (0x134 bytes), which has four more ints than the
framework's: the clip inset (+0xc8, 4), the text's x (+0xcc, 4), the cursor's x offset (+0xd0) and the selection's
extra width (+0xd4). Only Draw reads them; TwistEditWidget is the framework's EditWidget with that Draw.

The by-name font layer colour (0x757784) looks the name up in the font's layer map, then sets the colour of every
drawn layer made from that layer; the by-number one (0x7578ff) and its getter (0x75796a) count the font's layers.

users\users.dat: the framework's ReadUTF8String did not move past the characters it read, so every name after the
first read as junk; the exe's (0x461580) does, and the shared fix makes the framework's do the same. The user's own
file (written 2020) stores "Allison" with a length of 4 where WriteUTF8String (0x460fb0) writes the character count,
7; it comes from another build, and the retail exe reads it as the port now does.

The dialogs. Every dialog is a TwistDialog (0x218 bytes, made by the app's NewDialog 0x75fa42, placed by its DoDialog
0x75f2db: as wide as its buttons need and half the screen at least, its middle a little above the screen's). Its
buttons come from mTwistButtonMode's bits (1 OK, 2 Cancel, 4 a third, id 1011), not the framework's button modes:
the game's yes/no dialogs pass 3. It tells its own listener (+0x150, a TwistDialogListener, the app unless the caller
sets it), and only once: a pressed button turns its mouse off. Escape answers Cancel, Return answers OK (twice, from
KeyDown and KeyChar, as in the original). It pops up from 0.4 with SOUND_BUBBLEPOP, and the app's KillDialog
(0x75fab7) only starts it shrinking; it leaves the app's dialogs when it is gone. The size change is drawn with
Direct3D only, so in 2D it shows at once and goes ten updates after its button. GotFocus shows it and LostFocus hides
it. Behind the dialogs (and the options) the app fades in a whole-screen layer (UnderDialogWidget, app +0xf7c, by
+0xf90: 0.04 an update in, 0.03 out) that darkens the screen by up to 160/255; with Direct3D it is a blurred, rippling
copy of the screen instead. The app's own dialog answers (0x7633ea) kill the dialog; 3 closes the options and 4 is
the options' question; the board's (0xb, 0x28) come with the board.

The high scores (app +0x970, 20 of 0x24 bytes: name, level, score) are two top tens, classic (game mode 1) and the
other timeless mode (3); ScoreList (0x75fada) maps a mode to its list. They are in users\scores.dat (version 9:
a long version, then each row's UTF-8 name, level and score as longs; LoadScores 0x764ea9, SaveScores 0x76387e). A
new install's rows (0x7637ee) come from three tables in the exe: name pointers at 0x850e50, scores at 0x850e00 and
levels at 0x850ea0. GameText reads such tables too: GTB(address, length) for numbers, GTP(address, count) for text
pointers (their texts are listed as well), read with GameInt.

The high scores panel fades the old category out (+0x2d0, 0.05 an update) when a gate button picks a new one, then
shows its rows one by one, one each three updates, each with a glow that fades by 0.04 an update. Each row's name
runs through the rainbow (a ColorCycle brightened by half, a twentieth apart each row and starting where the update
count is in 400); the score is drawn right to left a character at a time, centred in cells of 22 (12 for a comma).
The stats page (any category but 1 and 3) has eight lines, from properties\default.xml where the game has an id
for them (HIGH_SCORES_POINTS) and from the exe's texts otherwise, with a view-replay button beside each of the three
best moves whose replay the profile keeps (profile +0xa0, 3 of 0x2c bytes, ordered by the table at 0x84e550). The
original reads properties\default.xml in Init (0x4a3c20); the port did not until this panel needed it.

The help panel's sixteen pages are animations of images\NonResize\pam\twistpam.pam (labels from the table at
0x84e5b0; a page's animation plays again whenever it stops), with the pictures from images\<art>\howtoplay. The
port's PopAnim (popanim/, the library bjt_popanim) is Bookworm Adventures 2's: the .pam format is the same (version
3). Twist's PopAnim (0x174 bytes, 0x4d30a0-0x4d8530) has two scales more: mDrawScale (+0xa4) scales the main sprite
and mImageScale (+0xa8) draws each picture at its inverse, its position kept; the help sets both to the art's scale,
so the pictures, made for that art, come out at their own size. In the menu the gates' buttons turn the pages (the
scores button back, 0x6983aa, the options button on, 0x6983bc); over a game the panel has its own buttons and zooms.

The options panel (0x1a0 bytes, 0x69df27-0x6a028b) is the main menu's (menu +0x2ec) and, over a game, the options
dialog (app +0xf40), which then also has the return, quit and restart (help in classic) buttons and zooms. Its sliders
are TwistSliders (0xb8 bytes, 0x7561fd: the framework's track of three cels, with the music slider's fill drawn over
it up to the thumb, for both sliders) and its boxes TwistCheckboxes (0x108 bytes, 0x7546e6: every one, the resolutions'
radio buttons too, uses IMAGE_MAINMENU_RADIOBUTTONCUSTOMCURSORS, checked cel 0 and unchecked cel 1, lit additively
once and once more under the mouse unless pressed). Full screen switches at once; the hints, custom cursors and
visualizers boxes set the app (+0xf8e ShowHints, +0x508 and the profile's +0x9c CustomCursors, +0xf86 DoViz); 3D
acceleration and the resolutions ask in dialog 4 (or say why not in 0x28 to 0x30), and a yes (0x6a028b, by +0x198)
sets the boxes and applies them. The rest is applied when the options close (FinishOptionsDialog 0x7632f7, then
0x6a006a): 3D or the art's size changed restart the game. The original without Direct3D says 3D cannot be enabled, the
visualizers need it and the larger art needs it; the port, which draws as 2D mode does, answers so too. The app's
registry values (0x762b40, 0x762da8) are ported for the options' boxes; its Shutdown (0x762a0d) saves the profile,
where the custom cursors are kept.

Game texts: the original's wide texts (L"...", such as the tooltips and the dialog texts) are UTF-16 in the exe;
gametext.py lists them as GAMETEXTW (length in bytes) and GameText.cpp converts them. Before this, every wide text
was read as its first letter.

## The board (being ported)

`Sexy::Board` (0x6a3d7b-0x6d6899, about 200 KB of code in 1,470 functions with its templates; constructor 0x6c4df9,
destructor 0x6befa1) is about 0x3940 bytes. Its bases are a TwistWidget, a ButtonListener (+0x148), a
TwistDialogListener (+0x14c) and a PopAnimListener (+0x150). Its arguments are the app (+0x954), the game mode
(+0x20c: 0 casual, 1 classic, 2 a challenge, 3 blitz, 4 zen, 5 puzzle, as the saved games' names (0x763964) and the
config files (0x6b2b77) show), flags (+0x210: 8 for a replay) and the challenge's number (+0x2674).

Its vtable (0x7dd0d4, 71 slots) overrides AddedToManager (20, 0x6b8aa2), RemovedFromManager (21, 0x6ca084), Update
(22, 0x6d6899, which calls the 24 KB 0x6d090f), Draw (26, 0x6cc7ed), DrawAll (27, 0x6aac5e), SetVisible (31,
0x6a3d7b), Resize (40, 0x6a5b53), DrawOverlay (44, 0x6b4a39, 7 KB), GotFocus and LostFocus (45 and 46), KeyChar (47,
0x6b3217), KeyDown (48, 0x6ca439), MouseMove (52), MouseDown (53, 0x6c7232), MouseUp (56) and two others (58, 59),
and adds slot 70 (0x6bdbd9).

The constructor makes a GoodTube (0x770 bytes, 0x707b7f: the progress tube beside the board; its constructor draws
16 Rand() values and a RandSigned, so it comes before the rest of the tube), three
EffectsManagers (+0x1648, +0x164c, +0x21b8), a PointsManager (0x158 bytes, +0x21fc) and a 42x42 MemoryImage, reads
properties\tips.txt, and fills a 7x7 table of each cell's angle and distance from the middle (+0x1674, 0x1c bytes a
cell). The game's other classes come with it: Piece (0x729b41-0x72fc8e), Points, the shockwaves and travellers,
Mandala, LevelUpShow, GameOverShow, SpeedText, OrbitText, FlyingText, GemSoul, Tutorial, EndStats, WheelSpin,
FruitAwardScreen, RankBonusWidget, AwardTelly, PuzzleUI, ReloadDisplay and ReplayWidget.

A piece (`Piece`, 0x53c bytes, no RTTI) is ported whole: its drawing (0x729b41 and its back and front, 0x72af5c and
0x72fc8e), its update (0x72c67b: locks, the flight to a new place, coal's embers, the power, electro, fruit, type 6 and
7 and devil gems' effects, an electro gem's arcs, the growth of a new gem, a bomb's counter and the break-up) and
the counter's image (0x72bab1). The port splits the update's longest parts into helpers (UpdatePowerSparks,
UpdateDevilSmoke, UpdateElectro, AddBreakDust) and keeps their draws in order. Its +0x528 is only ever copied or
cleared, so the calls through it are left out. Only the 2D paths are ported: with Direct3D some effects draw other
images and an electro arc takes 2 to 6 segments instead of 3 or 4.

The order of the work: Piece and the grid, then the board's update, input and drawing for classic mode, then the
other modes, the shows and the end of a game.

The background behind a game is the VizManager (0x1e8 bytes, a TwistWidget, app +0xc6c, z order -100; the board sits
at its z order + 10, the hyperspace + 1 or - 1, the Gates + 15). It runs one Visualizer (0x24 bytes, vtable 0x7d804c)
at +0x1a0 of its updates per update. With Direct3D and the options' visualizers box it picks one of 24 animated ones
by the game's mode (Hyperspace 0x70e623, 0x7593ea); in 2D it shows Viz2D, the level's picture: IMAGE_ZENDROP1 to 47
(by the level, +0x194, modulo 47), or IMAGE_VIRTUTORIAL for a player's first classic game, faded over the old one on a
new level by the curve at 0x7f2ccc. While the board stands still the picture is drawn only around it (0x67fdb6). The
port has the 2D path only. Which copies earn Blitz by play: Init (0x7653a1) sets +0x759 unless partner.xml's
MarketingMode is LevelLock, and the mode select opens Blitz at once only for a registered LevelLock copy.

The port stages Steam's `properties/` (PORT_PLAN.md, "Which copy's files"): the retail resources.xml names the zendrops
by paths no copy has, and the retail game quits on the first one it needs. So the port reads LevelLock, Steam's
registry key (`Steam\BejeweledTwist`) and its sound volumes. For the comparisons, origrun's copy of the original carries
the same `properties/` and the patch that skips the properties-signature check (0x4a39c7), as the user's Steam exe
does; its registry preset goes under the Steam key. During a 2D jump Hyperspace draws the level's picture over space
while the VizManager is hidden (0x711b65, the old picture for the first half, faded in by +0x1570), and at the end of
one the old picture at +0x12b0's alpha (0x70f836), or space fading out over the board.

The tips and the basics tutorial (Tutorial, 0x220 bytes, 0x750a5a-0x7540ad) run lines of properties	utorial.txt
from a label named after the tip (ShowNextTip, 0x6bbc58, reads the file once into board +0x254c). A tip waits in
+0x256c by its kind until the board is still; its Tutorial goes into the board's dialog queue and sits at +0x2584 until
its dialog is clicked away, when 0x6c78a5 (Board::CloseTip) marks the tip seen in the profile and, for the basics
(kind 14), resets the score and the tube. While it runs, the board's hints, clicks and twists ask its state (+0x16c),
its "dialog up" flag (+0x210) and whether moves are allowed (+0x21d); 0x6aa3bc draws the forced move's "click here" and
the arrows around the cursor. A few commands still reach parts of the board not yet ported (the fruit award and the
replay of the tip from its saved board).

The end of a level: LevelComplete (0x6b3634) fades the music, shows the LevelUpShow ("LEVEL n COMPLETE", 0x2c8 bytes,
0x71cc30, at +0x26ac) and starts the hyperspace's transition 8. Meanwhile UpdateJump (in UpdateGame's busy branch,
while +0x2108 > 0) breaks the board up row by row; each gem left in a row becomes a GemSoul (0x8f8 bytes, 0x70188f),
which swirls over the board with its gem shrinking inside it. On the new board each soul is sent to a plain gem
(RetargetTravellers, 0x6a9f1d, or the copy at the end of UpdateGame's shake), and where it lands that gem becomes the
special gem it was. With Direct3D the souls also move through the hyperspace's 3D space; only the 2D path is ported.
The app's command line (0x76350c) takes -autoplay[=classic|zen|blitz], which plays games on its own; its other test
switches (screen and art sizes, -skipmenu, Direct3D, seed recording) are not ported.

The tube (GoodTube, 0x770 bytes, 0x701efb-0x70c8b5) is drawn whole in 2D: its glow, the fill with its bubbles and
specks, the level's arc of lights and multiplier bulbs, the score's ticker, the twist door and the menu button. Its
head holds the player's rank badge (Sexy::RankBadge, 0x158 bytes, +0x32c), which draws the badge at 0.4 scale while
the tube is shown and keeps a hover and click state for its tooltip; a burst of sparks leaves it while +0x2a4 is above
0. Still missing: the challenges' goal and bracket, the bonus challenge's hint wave, the glass breaking (0x70c7ec)
and the tube's text popups (+0x1cc, +0x1d0: they are never made in play; the tube's tip text, +0x224, is what 0x70c94a sets).
In a first shot of classic play the tube matches the original except for
the badge, which the port draws a little softer, as the framework's stretched DrawImage filters where the original's
2D stretch looks sharper; the identical-play pass looks at that stretch for every scaled image.

Tooltips: Board::UpdateToolTips (the 0x6d5279-0x6d6695 block of UpdateGame) picks a text from the mouse over the tube's parts, the rank insignia, the meter and the special gems, waits +0x6d0 updates (100, 120 for a far hover), then shows it through the curves at +0x718 and +0x788; Board::DrawToolTip (part of DrawOverlay, 0x6b4a39) draws it. The RankBadge needs no hover of its own. The bonus challenge's hint wave is FruitShockwave (0x74b94e, in Shockwave.cpp), made by GoodTube::UpdateTick; its Direct3D arc is NotPorted (3D-only) and the original leaks its particles, which the port frees.

Ported since: the tube's tip text (GoodTube::SetTip, 0x70c94a: wraps in FONT_TOOLTIP, fades in; "%d CHAIN!" at a chain of
50, 60 and so on), the count popup (ShowCountPopup, 0x6b3ffc), the goal check (Board::CheckGoal, 0x6b3432: the bonus
challenge being met, with its two sounds and 0x703642; the challenge branch, +0x2630, stays NotPorted), the mode
unlock (Board::UnlockMode, 0x6b40b5, with its banner in DrawOverlay and the curves at +0x428 to +0x658), the +0x211c
object (BoardStepper, 0x74c8c8: a developer's stepper that the +0x210c flag, which nothing sets, would run) and coal.
Coal breaking (BreakSpecial's coal branch, 0x6c03a4, from BreakPiece with type 1) throws one to five geodes
(Board::AddGeode 0x6ba87f, a 0x198-byte object without a vtable in the list at +0x14ac, which the original frees
with free() and never deletes when they land): their update is in UpdateGame (0x6d090f, Board::UpdateGeodes), their
draw in Board::Draw (0x6cc7ed), their light in DrawPieces and their points in UpdateRecords; effect types 40 (a spark)
and 43 (the coal's nova) are new. Doom gems and gem locks in BreakSpecial are still missing.

The tube's MENU button opens the options over the game (Board::ShowMenu 0x6a6983, WinTwistApp::ShowOptionsDialog
0x75f6fc). Over a game the panel (0x69e4c5) has a return button, a quit button (the challenge button in a challenge)
and, by the mode, restart (challenge), restart (Blitz) or How to Play. Quit asks the board's quit question (dialog 1,
0x6b38f3), which the board answers itself (0x6c8632, its dialog listener at +0x14c); restart asks dialog 0xb, which
the app answers. Yes to the quit question sets +0x34ec, and UpdateGame then hides the board and starts the app's
hyperspace on transition 13 (Hyperspace::StartEndGame, 0x714883): space fades in over the board by +0x12b0, and when
that curve reaches 0 the board sets +0x1ff5 and the app goes back to the main menu. A challenge's end there and the
restarts of a challenge (0x6c9987) and of Blitz (0x6c7a75) are not ported yet. In the original, as in the port, Escape does not
close How to Play.

Comparing with the original in origrun's sandbox: the sandbox has no cursor, and the original's framework reads the
real one (GetCursorPos) after posted input. So its widget manager's last mouse point (+0xe0, +0xe4) turns into the
window's offset from the screen's corner, (-67, -90), rather than the last click. FinishLevelUp (0x6bab49) hands that
point to the board, so in the sandbox the board takes the mouse as off the grid and draws no twist selector; on a real
desktop, as in the port, the selector sits under the mouse. Shots of the first tip differ by that selector alone.

## The challenges' data and goal (Challenge.cpp)

The app owns the challenge list (WinTwistApp+0xf5c, `ChallengeInfo`, 0x18 bytes, built right after Ranks): the categories read from properties\challenges.xml by 0x6dcff3. A category (0x74 bytes) holds its id, the layer pictures, additive, the visualizer name, bkg2d, its challenges, the nightmode "special" challenge and a colour. A challenge (0xe4 bytes) holds its texts, the rules (stat, operator, value; operators are only "=", ">=", "<=" and "<"), a flags word (+0x80: count, time, moves, points, onboard, time limit, onemove, pattern, constantcounters, nightmode, nocount, nocascades, constantdoom, quitonfail, badgem), the time and move goals, its category and number, the pattern (colour letters of RWGYVOB) and its progress, the config file and the best and low scores. The parser takes a rules string by removing spaces and quotes and reading comma-separated name/operator/value fields, the keywords case-insensitively; Time is given in minutes and kept in seconds. The board's active challenge is at Board+0x2630 and is null outside Challenge mode, so Classic, Zen and Blitz never reach this code. `ChallengeInfo::Check` (0x6db273) compares each rule's stat (the app's whole-game array, the board's one-move array or its on-board array, by flags) against its goal and returns 0 not met, 1 met, 2 failed; `GetText` (0x6db73e) builds the description with the numbers and a 0-1 fraction; `GetPercentDone` (0x6d8d19) feeds the Mandala. Board::CheckGoal's challenge branch, UpdateChallengeTube (0x6af390), the pattern counter in GoodTube::UpdateTick, FillInBlanks' re-deal while the goal is already met and CountDownCounters' stop read this object. The menu, and the board's start, end, restart, timer and counter flow, are not ported yet. Deviations: a rule with an unknown stat name is skipped, and a category or challenge that fails to parse is freed, not leaked; errors go to the log, not to a message box. The port reads the Moves total through `mUnknownC74[0x21]`, which is the same memory as the original's +0xcf8 (the port also has a separate, never written `mUnknownCF8`).

## The challenge menu (ChallengeMenu.cpp)

The menu is ChallengeMenu (0x9e8 bytes, 0x6dd2cd-0x6e665e, main menu +0x4a8, z order -98; a TwistWidget, ButtonListener and TwistDialogListener) with one ChallengeButton (0x658 bytes, vtable 0x7e011c) for each category and two TwistButtons below them (100 "PLAY MENU" at +0x174, 101 "MAIN MENU" at +0x178). A planet's id is its category's number. It is reached from the Mandala's challenge door (`Mandala::ButtonDepress`, mode 2: `Menu::ShowChallenges(false)`, `Show(1, false)`), and from the end of a challenge (Board::UpdateGame: `SetState(4, false)`, shown at once). `UpdateProgress` (0x6e0ddb, also run by `Menu::RefreshPlayer`) reads the player's stage of each category (Profile +0x1f4, the new `GetChallengeStage` 0x73497f, `IsChallengeCategoryOpen` 0x7349fc, `OpenChallengeCategory` 0x734a14, which grow the lists to the category): 0 is closed, a stage past the category's last challenge makes the planet an eclipse (its night challenge, number count + 1), and the planet's best is Profile +0x204. A click on a planet (`ButtonDepress`) picks its category and stage; with a saved game for it (`SaveExists(2, category)`) the "continue or new" dialog (id 0, Yes 1000 continues, No 1001 deletes the save) asks, otherwise `Start(true)`. `Start` (0x6e4ab6) is `NewGame(2, new, category)`, and for a new game `Board::SetChallenge(ChallengeInfo::GetChallenge(category, stage))`, then `Hyperspace::StartChallenge` (0x71c18f: state 3, transition 11, the board's three curves), the board's brightness at 0 and `Close(3)`; the hyperspace's transition 11 hides the menu when its curve ends (0x715dc7). `Board::SetChallenge` (0x6c7c7b) keeps the challenge, loads its own config and restarts the lock, counter and coal level values at level 1, sets the tube (the night mode's stage from Profile +0x204, a time limit's blitz-like clock, the tube kind 1, 2 or 3 by the pattern and moves flags), the visualizer (Viz2d, with the category's bkg2d) and the tube's first fill. The trial gate (more than the third stage or the third category in an unregistered copy) only plays SOUND_TRIALCLICK here, as the copy is registered.

Drawing is NOT ported yet (placeholder, to be replaced); the camera and the motion are. The original with 3D off (`Is3DAccelerated()` false) still runs a small 3D world in software. **Ported** (Math3D.h/.cpp, ChallengeMenu.cpp): `SetState` (0x6e2318) sets the curves of every state, all but a few linked to the open curve `mOpen` (+0x1c8, whose value runs the menu in or out): the buttons' slide `mButtonSlide` (+0x238), the camera's turn about x, y and z (+0x2b0, +0x320, +0x390, in degrees), its distance along the third axis from 16.15 (+0x400), along the first and second axes (+0x470, +0x4e0), its shifts of x and y (+0x550, +0x5c0), three curves not yet understood (+0x640, +0x6b0, +0x720), the turn of the hyperspace's camera `mCameraTurn` (+0x790), the fly to the picked planet `mZoom` (+0x830), and the sun's (global 0x85c558); state 1 also turns the camera's start (the three rotations, 0.625 of the way to the identity by quaternions, transposed) into `mCameraTarget` (+0x800); states 3 and 4 copy the picked planet's world position (ChallengeButton +0x5a8, now `mWorldPos`, still zero until the planets' placement in AddedToManager 0x6dfc49 is ported) to +0x824; state 3 continues from where state 4's curve stood. `Update` (0x6e3f41) steps the sun, +0x640 and +0x6b0 and keeps 1 - +0x6b0 (not below 0.25) in global 0x85c508, fades the hover texts (+0x198, and +0x9d0 and +0x9d4 while nothing is hovered), blends the glow colours (globals 0x85c5c8 and 0x85c50c) by the curve at +0x928, updates the glow's EffectsManager (+0x1bc), drags the camera stack's entry with the mouse (see below), waits out +0x2a8 and steps the open curve, at whose end it makes the buttons and planets clickable (a state 1 or 4 menu fully in also starts +0x640's second curve), makes the camera Transform3D (global 0x85c51c: the position from the camera curves in states 1 and 2, otherwise a line from (0, -12, 16.15) to the picked planet's place plus (0, 0.15, 7.5) by +0x830; none in state 5), turns the hyperspace's camera (+0x1548) from the identity to +0x800 by +0x790, puts the two buttons at y = ((1 - +0x238) * 0.5 + 0.925) * height, moves the glow manager down by (1 - open) * the art scale * 1500, sets its alpha to the sun's value (states 1, 4, 5) and the aspect globals (0x84ebf8, 0x84ebfc), and, with the menu all the way in and no challenge started, throws a glow particle (type 0, one of the two ECLIPSE_FLARE images, grey 240 to 254, additive off) about the sun's centre with a chance of 1 in 5 an update (with Direct3D two each update: NotPorted). The camera stack (+0x998) is the file properties/camerastack.txt (two lines of three numbers for each camera) that the mouse drag edits (left button: y and x by 0.002 a pixel, left with right: z); nothing reads it, and the mouse wheel's choice of entry (0x6dfe2a) is only enabled by a flag (+0x9bc), which Show and the constructor clear and which was not traced further. `Close` waits while the open curve runs backwards. The 0x6e32c9 function (the night mode's unlock message, which starts the +0x928 curve) and the unlock message widget at +0x9dc (set by the board's code, 0x6b3412; Update's block for it, 0x6e4989-0x6e4a1f, steps it and, once it is done, removes it and goes from state 5 to state 4; its type is not identified) are not ported. **Not ported yet** (pieces 2 and 3): each planet's place comes from projecting its world position through the camera (gMenuCamera, gAspect), which also places its button (ChallengeButton::Draw 0x6dde98); each planet is a software-lit sphere (the 384 by 384 IMAGE_PLANET lookup against the category's layer pictures, into a 512 by 512 MemoryImage at +0x9e0) with its name, its bar of stage boxes ("ECLIPSE" once every challenge is done), the sun (0x6e132d) and the moon, which becomes the "CHALLENGE MODE" eclipse disc; Draw (0x6dd322) draws the glow manager and the menu graphic behind the buttons. Until that is ported the menu lays the planets out flat (5 to a row, 150 by 130 apart, 100 wide, the two buttons below, but at the real y), draws each as a plain disc with its category's id and "stage/count" and fades them with the open curve (`PlaceholderAmount`); none of that is the original's. Its logic is the original's: UpdateProgress, the picks, the saved-game dialog, Start, Close and the buttons. The unlock animations of UpdateProgress's animating path (the glow and the stage advance after it, the sounds) are not played and the stage moves at once. SetState plays track 0x130 for states 1 and 4 only (the two PlayMusic calls at 0x6e237f and 0x6e2b91); Close(2) plays track 0. Still NotPorted in the board's challenge flow: the timer 0x6bedd8, goal met 0x6b330d, the hyperspace end 0x714960, restart 0x6c9987, the music 0x6b2fda and 0x6b66d2 and the twists 0x6bde87.

## Deviations

Each is a change without a visible effect in play.

- The arrow keys on the web (Board::KeyDown, 0x6ca439): after moving the cursor, the original moves the mouse
  pointer onto it (Board::WarpToCursor, 0x6a45da, SetCursorPos) while +0x1430 holds a piece. Natively the port does
  the same (SexyAppBase::WarpMouse, never in a test run); a web page cannot move the pointer, so in the browser it
  stays put and the next small move of the mouse takes the cursor back under it. This one shows, on the web only.
- PopCap's beta dump: after loading the players, the original reads a text file at a path on PopCap's own network
  (0x761f11) into a buffer (app +0x1024) when it exists (0x764814). It never exists outside PopCap; the port leaves it
  out.
- The layer behind the dialogs (UnderDialogWidget) is never deleted by the original; the port deletes it with the app.
- DoDialog over a dialog of the same id: the original shrinks and deletes the old one but leaves it among the app's
  dialogs, where AddDialog would then find it deleted. The port takes it out too. No dialog of the game is ever asked
  for twice.
- A saved game's music position (board +0x3928): leaving a game, the original stores BASS's position in the song
  playing (0x6ee282) when none is stored yet; it goes into the save, and the music never reads it back (PlayMusic drops
  it). The port keeps 0 there, so its saves differ from the original's in those eight bytes. A save of the original's
  loads and saves again byte for byte otherwise.
- The high scores panel's view-replay buttons: the original deletes the ones it added to the panel (with the panel's
  children) but not the ones it kept without adding (a best move without a replay). The port deletes them all.
- Two reads through null in the board's fill are guarded: turning a 2x2 with a gap in it (allowed by +0x2054) reads
  the gap's flags (0x6bb619), and filling at once while +0x2108 holds a gap open sets the gap's height (0x6c25a4). The
  original would crash on either; the port skips the gap. Whether play can reach either is not yet known. The same holds for FindMatches (0x6cd468), which reads
  every cell's +0xc4 without a check for a gap while +0x149c holds anything.
- The options panel: the original never deletes the resolutions' three radio buttons or the restart (help) button;
  the port deletes them with the panel.
- The music: the original prints the track, state and tempo to the debugger when it plays one and when the tempo
  changes; the port does not. It keeps its CustomBassMusicInterface in a static and frees its tracks' memory only on
  exit; the port's is the app's music interface, deleted with the app with its tracks. BASS's two extra play
  arguments (Blitz passes the board's +0x3928 and +0x392c) are left out until the board shows what they do. Without
  sound the original's PlayMusic reads past a plain MusicInterface; the port does nothing.
- The gems drawn ahead (WinTwistApp::MakeGemImages): the original frees the images but never the eight arrays of
  pointers that held them; the port keeps them in the app.
- A piece's row and column (Piece::DrawFront): the original draws them over each piece while the board's +0x21db
  is set, which nothing ever sets; the port leaves the labels out.
- A devil gem's spark (Piece::AddSpark): the original makes the effect and then returns null without adding or
  deleting it; the port deletes it.
- The points' smoke (SmokeTraveller): the board's destructor frees its list (+0x2170) without deleting the smoke
  still on its way; the port deletes it there, without the points or the sparks its own destructor would add.
- The shockwaves (+0x1660): the board's destructor frees their list without deleting them either; the port deletes
  them there.
- A floater's +0x68 (the font layer that takes the points' own colour) is set only by 0x732a41, which nothing calls,
  so the board's floaters read it as the heap left it; the port starts it at 0, and every layer takes its cycle's colour.
- A shard's cel (effect type 16) comes from the manager's update count times the effect's address, in the original
  and the port alike; the port's addresses differ, so the shards turn through other shapes. Purely visual, and it draws
  no random number.
- The mode select (Mandala, 0x721d5c) deletes its effects manager while the manager is still one of its children;
  the port takes it out first. The zen jump's choice also goes into PopCap's debug text (app +0x1024, see the beta
  dump above) as a line with the portal, level, rank and play time; the port leaves the line out.
- Loading a visualizer's picture (0x748f54) also files it under its id for GetIdByImage; nothing asks that of these
  pictures, and Res.cpp keeps that table to itself, so the port leaves the filing out.
- The pictures are JPEG 2000 files; the port's decoder (OpenJPEG) gives colours within 2 of the original's.
- Web only: an image drawn stretched (the framework's 2D software DrawImage into a rectangle of another size) can
  land half a pixel apart from the native build, which rounds its edges in x87 precision as the original does, while
  wasm computes them in doubles. It belongs to the framework's 2D path, whose work is queued after Twist.
- The level-up show (LevelUpShow) never deletes its effects manager in the original; the port deletes it with the show.
- A gem soul's gem (GemSoul +0x67c): at its 80th update the original clears the gem's +0x535 without checking that the
  gem is still there (it is deleted when it blasts); the port checks.
