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
Only the board sets its flags (+0x08 skip drawing, +0x09 record); otherwise it is the Graphics call it wraps. While
recording it keeps each draw of an image loaded from a file (Image +0x1c, the path's length) as a GraphicsOperation
(0x60 bytes: frame, type 0 to 7, image, four doubles with the Graphics' translation added, a source rectangle or cel,
the colour if colorizing, else white, the draw mode and a rotation). UpdateGame records a move's frames (+0x3444,
up to 500, sparser after 20 and 50) and StartPieceMove drops them; a replay's rewind (UpdateGame state 1, DrawOverlay)
draws the last frame back and drops it (0x70d293, 0x70d704). DrawImageTransform (0x70dfad) repeats
DrawImageTransformHelper's 2D cases; the rotated and plain float cases and the mirrors go to the Graphics unrecorded
and drawn even while skipping, the matrix one not drawn while skipping. 0x421780, the rectangle draw it records as
type 3, applies the Graphics' own scale (the framework's does not; GraphicsRecorder.cpp does it).

DrawImageCentered (0x757137): unturned, it places the image by one cel's size (GetCelWidth/Height), whatever the
source rectangle; turned, it uses a Transform. DrawImageScaledCentered (0x756ee7) takes alignment flags (1 centre x,
2 centre y, 4 bottom, 0x10 right) and uses a Transform only when both scales differ from 1.

Effect (0xc0 bytes) and EffectsManager (0xbc bytes): the effects are a multimap keyed by type (+0x88), so they update
and draw by type, and within a type in the order they were added. The common motion (velocity / mUpdateDiv, the
scalars, gravity, alpha and scale steps, the scale and alpha limits) is ported, and type 0; the types 1-43 are big
switch cases in the constructor (0x6f4a05), Update (0x6f8792) and DrawEffects (0x6f6451) that come with the board.
Type 44 effects are objects of their own (virtual Update and Draw). The list at +0x94 holds heat waves, Direct3D only.
Type 10 (Piece::AddShards 0x728f69, the credits' sparks 0x6e903c; nothing else makes one) has no constructor case. Its
update fades it by 0.0025 more, turns it by 0.1 of its fall, and turns its velocity by 0.05 of a rate that blends from
+0xa0 to +0xa4 as +0x9c runs from 0 to 1 by 0.01; past 1, +0x9c goes back to 0, +0xa4 to +0xa0 and a RandSigned to
+0xa4. Its draw is IMAGE_FX_STEAM additive in Color(125, 145, 255) at 105 times the opacity (the effect's colour is not
used), five times, from scale 1 down by 0.12 each, stepping back six velocities along a path turned by -0.3 times the
rate; it leaves the draw mode and colorizing set. AddShards' second argument picks the board's second manager
(+0x164c) over the first: Piece::Update passes true (0x72da35), BreakPiece's never-reached ice gem false.

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
high scores panel (HighScoresPanel; without the new score going into its list, which comes with the end of a game; its
replays are ported with the board's, below), the help panel (HelpPanel) and the options panel (OptionsPanel, without its
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
was read as its first letter. The table (GameTextTable.inc) lists only the addresses the code uses: after any new GT,
GTS, GTA, GTB or GTP use, run `python games/bejeweledtwist/tools/gametext.py`. An address missing from the table reads
as an empty text, and GameText logs it once (a curve string read as empty is a flat curve, which is how the
hyperspace's constructor curves went unnoticed).

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
the arrows around the cursor. Its replay button rewinds the move and plays it again (Board::StartReplay, the
replays below).

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
0. The tube's text popups (+0x1cc, +0x1d0, 0xd4-byte objects of constructor 0x725732) are made only by 0x707103 and
0x707215, and +0x2f0 by 0x70c7ec; nothing in the exe calls those three or holds their address (no call,
jump or pointer in the whole image), so the popups never exist and the port keeps only their unreachable NotPorted in
DrawFill. The tube's tip text, +0x224, is what 0x70c94a sets.

The challenge's display (GoodTube::DrawChallenge, the 0x7087b2-0x70a209 block of DrawBody 0x7081c9) replaces the
score while the board has a challenge (+0x2630). The score's row (0x7028b7) moves up 55 for a night-mode challenge
whose best ever is shown (+0x768, set by Board::SetChallenge from Profile +0x204), and the text is faded by the tip
(1 - +0x244). By the tube's kind and the challenge's flags (tested in this order):
- kind 3 (a "Moves" rule alone): the moves left (string 0x7e40dc through the app's GetString, or 0x7e40ec for one),
  word-wrapped in FONT_SCORE, 50 below the row with a Points rule, 80 above it without; SetUnknown158(3) sets +0x580
  to 1 (0x702b87);
- a pattern: 0x6db0ec fills the tube's four lists (+0x56c the stats, +0x53c their values, +0x55c the comparisons,
  +0x54c the goals; the only code that fills them). A night-mode pattern shows the count's title, the count's ticker
  (+0x660) and, while +0x580 is above 0, "Best" and the best's ticker (+0x6e4); otherwise the pattern's gems in a row
  0.55 of the tube wide at most, from the table 0x84eff4 (two pictures for each colour), those matched (+0x538) lit
  and the next one scaled by +0x4c8 while that curve runs;
- on-board: an IMAGE_CHALLENGEICONS cel (the colours 0-6; 7 flame gems or flame gems made, 8 the same of
  lightning, 9 stat 24, which no rule name gives, 10 coal) with "x" and the count for each counted stat, in a column
  shrunk by hundredths to 150 high;
- one move: each ">=" goal as an icon with "x" and its value, or (no icon) the count's title ("Gems" if empty) over the
  count's ticker; a single goal rises with +0x580 and has "Best" and the best's ticker under it;
- otherwise the count's title ("Count" if empty) and ticker unless "nocount", then "Moves Left" (or "Moves" without a
  goal) and the moves left (or made), in FONT_SCORE when the moves rule is the only one; without a Moves rule "Best"
  and the best's ticker while +0x580 is above 0.
A night-mode challenge then shows "Best Ever" and +0x150 under the display. Every pixel offset above is times the
app's scale.

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
and 43 (the coal's nova) are new. Doom gems and gem locks in BreakSpecial are described under "Gem locks and doom
gems breaking".

The tube's MENU button opens the options over the game (Board::ShowMenu 0x6a6983, WinTwistApp::ShowOptionsDialog
0x75f6fc). Over a game the panel (0x69e4c5) has a return button, a quit button (the challenge button in a challenge)
and, by the mode, restart (challenge), restart (Blitz) or How to Play. Quit asks the board's quit question (dialog 1,
0x6b38f3), which the board answers itself (0x6c8632, its dialog listener at +0x14c); restart asks dialog 0xb, which
the app answers. Yes to the quit question sets +0x34ec, and UpdateGame then hides the board and starts the app's
hyperspace on transition 13 (Hyperspace::StartEndGame, 0x714883): space fades in over the board by +0x12b0, and when
that curve reaches 0 the board sets +0x1ff5 and the app goes back to the main menu. A challenge's end there and the
restarts of a challenge (0x6c9987) and of Blitz (0x6c7a75) are described under "The challenge's flow on the board". In the original, as in the port, Escape does not
close How to Play.

Comparing with the original in origrun's sandbox: the sandbox has no cursor, and the original's framework reads the
real one (GetCursorPos) after posted input. So its widget manager's last mouse point (+0xe0, +0xe4) turns into the
window's offset from the screen's corner, (-67, -90), rather than the last click. FinishLevelUp (0x6bab49) hands that
point to the board, so in the sandbox the board takes the mouse as off the grid and draws no twist selector; on a real
desktop, as in the port, the selector sits under the mouse. Shots of the first tip differ by that selector alone.

## The challenges' data and goal (Challenge.cpp)

The app owns the challenge list (WinTwistApp+0xf5c, `ChallengeInfo`, 0x18 bytes, built right after Ranks): the categories read from properties\challenges.xml by 0x6dcff3. A category (0x74 bytes) holds its id, the layer pictures, additive, the visualizer name, bkg2d, its challenges, the nightmode "special" challenge and a colour. A challenge (0xe4 bytes) holds its texts, the rules (stat, operator, value; operators are only "=", ">=", "<=" and "<"), a flags word (+0x80: count, time, moves, points, onboard, time limit, onemove, pattern, constantcounters, nightmode, nocount, nocascades, constantdoom, quitonfail, badgem), the time and move goals, its category and number, the pattern (colour letters of RWGYVOB) and its progress, the config file and the best and low scores. The parser takes a rules string by removing spaces and quotes and reading comma-separated name/operator/value fields, the keywords case-insensitively; Time is given in minutes and kept in seconds. The board's active challenge is at Board+0x2630 and is null outside Challenge mode, so Classic, Zen and Blitz never reach this code. `ChallengeInfo::Check` (0x6db273) compares each rule's stat (the app's whole-game array, the board's one-move array or its on-board array, by flags) against its goal and returns 0 not met, 1 met, 2 failed; `GetText` (0x6db73e) builds the description with the numbers and a 0-1 fraction; `GetPercentDone` (0x6d8d19) feeds the Mandala. Board::CheckGoal's challenge branch, UpdateChallengeTube (0x6af390), the pattern counter in GoodTube::UpdateTick, FillInBlanks' re-deal while the goal is already met and CountDownCounters' stop read this object. The menu, and the board's start, end, restart, timer and counter flow, are not ported yet. Deviations: a rule with an unknown stat name is skipped, and a category or challenge that fails to parse is freed, not leaked; errors go to the log, not to a message box. The port reads the Moves total through `mUnknownC74[0x21]`, which is the same memory as the original's +0xcf8; LevelValue::GetCount reads it there too.

## The challenge menu (ChallengeMenu.cpp)

The menu is ChallengeMenu (0x9e8 bytes, 0x6dd2cd-0x6e665e, main menu +0x4a8, z order -98; a TwistWidget, ButtonListener and TwistDialogListener) with one ChallengeButton (0x658 bytes, vtable 0x7e011c) for each category and two TwistButtons below them (100 "PLAY MENU" at +0x174, 101 "MAIN MENU" at +0x178). A planet's id is its category's number. It is reached from the Mandala's challenge door (`Mandala::ButtonDepress`, mode 2: `Menu::ShowChallenges(false)`, `Show(1, false)`), and from the end of a challenge (Board::UpdateGame: `SetState(4, false)`, shown at once). `UpdateProgress` (0x6e0ddb, also run by `Menu::RefreshPlayer`) reads the player's stage of each category (Profile +0x1f4, the new `GetChallengeStage` 0x73497f, `IsChallengeCategoryOpen` 0x7349fc, `OpenChallengeCategory` 0x734a14, which grow the lists to the category): 0 is closed, a stage past the category's last challenge makes the planet an eclipse (its night challenge, number count + 1), and the planet's best is Profile +0x204. A click on a planet (`ButtonDepress`) picks its category and stage; with a saved game for it (`SaveExists(2, category)`) the "continue or new" dialog (id 0, Yes 1000 continues, No 1001 deletes the save) asks, otherwise `Start(true)`. `Start` (0x6e4ab6) is `NewGame(2, new, category)`, and for a new game `Board::SetChallenge(ChallengeInfo::GetChallenge(category, stage))`, then `Hyperspace::StartChallenge` (0x71c18f: state 3, transition 11, the board's three curves), the board's brightness at 0 and `Close(3)`; the hyperspace's transition 11 hides the menu when its curve ends (0x715dc7). `Board::SetChallenge` (0x6c7c7b) keeps the challenge, loads its own config and restarts the lock, counter and coal level values at level 1, sets the tube (the night mode's stage from Profile +0x204, a time limit's blitz-like clock, the tube kind 1, 2 or 3 by the pattern and moves flags), the visualizer (Viz2d, with the category's bkg2d) and the tube's first fill. The trial gate (more than the third stage or the third category in an unregistered copy) only plays SOUND_TRIALCLICK here, as the copy is registered.

The camera, the motion, the planets' software drawing and the sun, the moon and the texts over them are ported (the planets' tips too, see "The challenge's flow on the board"). The original with 3D off (`Is3DAccelerated()` false) still runs a small 3D world in software. **Ported** (Math3D.h/.cpp, ChallengeMenu.cpp): `SetState` (0x6e2318) sets the curves of every state, all but a few linked to the open curve `mOpen` (+0x1c8, whose value runs the menu in or out): the buttons' slide `mButtonSlide` (+0x238), the camera's turn about x, y and z (+0x2b0, +0x320, +0x390, in degrees), its distance along the third axis from 16.15 (+0x400), along the first and second axes (+0x470, +0x4e0), its shifts of x and y (+0x550, +0x5c0), three curves not yet understood (+0x640, +0x6b0, +0x720), the turn of the hyperspace's camera `mCameraTurn` (+0x790), the fly to the picked planet `mZoom` (+0x830), and the sun's (global 0x85c558); state 1 also turns the camera's start (the three rotations, 0.625 of the way to the identity by quaternions, transposed) into `mCameraTarget` (+0x800); states 3 and 4 copy the picked planet's world position (ChallengeButton +0x5a8, now `mWorldPos`, set in AddedToManager 0x6dfc49) to +0x824; state 3 continues from where state 4's curve stood. `Update` (0x6e3f41) steps the sun, +0x640 and +0x6b0 and keeps 1 - +0x6b0 (not below 0.25) in global 0x85c508, fades the hover texts (+0x198, and +0x9d0 and +0x9d4 while nothing is hovered), blends the glow colours (globals 0x85c5c8 and 0x85c50c) by the curve at +0x928, updates the glow's EffectsManager (+0x1bc), drags the camera stack's entry with the mouse (see below), waits out +0x2a8 and steps the open curve, at whose end it makes the buttons and planets clickable (a state 1 or 4 menu fully in also starts +0x640's second curve), makes the camera Transform3D (global 0x85c51c: the position from the camera curves in states 1 and 2, otherwise a line from (0, -12, 16.15) to the picked planet's place plus (0, 0.15, 7.5) by +0x830; none in state 5), turns the hyperspace's camera (+0x1548) from the identity to +0x800 by +0x790, puts the two buttons at y = ((1 - +0x238) * 0.5 + 0.925) * height, moves the glow manager down by (1 - open) * the art scale * 1500, sets its alpha to the sun's value (states 1, 4, 5) and the aspect globals (0x84ebf8, 0x84ebfc), and, with the menu all the way in and no challenge started, throws a glow particle (type 0, one of the two ECLIPSE_FLARE images, grey 240 to 254, additive off) about the sun's centre with a chance of 1 in 5 an update (with Direct3D two each update: NotPorted). The camera stack (+0x998) is the file properties/camerastack.txt (two lines of three numbers for each camera) that the mouse drag edits (left button: y and x by 0.002 a pixel, left with right: z); nothing reads it, and the mouse wheel's choice of entry (0x6dfe2a) is only enabled by a flag (+0x9bc), which Show and the constructor clear and which was not traced further. `Close` waits while the open curve runs backwards. The 0x6e32c9 function (the night mode's unlock message: a DoDialog with the texts 0x7e0aa8 and 0x7e09a8, the +0x928 curve 0x7e0964, the sound at 0x85e918 and UpdateProgress(false, false)) is never called: the exe has no call to it and no pointer to it, so it is not ported and +0x928 stays at 0 and the unlock message widget at +0x9dc (set by the board's code, 0x6b3412; Update's block for it, 0x6e4989-0x6e4a1f, steps it and, once it is done, removes it and goes from state 5 to state 4; its type is not identified) are not ported. **Planets (ported, piece 2).** AddedToManager (0x6dfc49) puts the planets on an ellipse: the angles spread over 0.9 of a turn from 0.25 turn less half the gap, world position (cos * 4.8 * 1.5 + camera x, sin * 4.8 + camera y, 0). ChallengeButton::Draw (0x6dde98), with Direct3D off, projects mWorldPos through a PerspectiveCamera (Math3D.h: gMenuCamera, gAspect45, gAspect, planes 1 and 10000) every Draw and sets the button's x and y from it, so clicks land on the planet; the size is 76 px times 0.00151515 over the depth. The sphere is the 384 wide IMAGE_PLANET lookup (R light, G/2 row and B column into the category's layer pictures, found by GetIdByName on the upper-cased IMAGE_ name) written into the menu's 512 by 512 MemoryImage (+0x9e0) and drawn with a source rectangle; then the ring, the name, the moon-meter bar of cells, the lock glow, the eclipse title and the bar and name glows. ChallengeButton::Update (0x6e3b0e) makes the sparks, the glow chain and the eclipse flares; UpdateProgress (0x6e0ddb) sets the bar's curves and plays the unlock sample. Menu Draw (0x6dd322) draws the glow manager and the menu graphic behind the two buttons. **Not ported yet**, NotPorted with addresses in the code: the zoom's own effects; UpdateProgress's glow; and what only Direct3D runs (the sphere meshes 0x6e4fad, light and fog, the eclipse title's flare strips, two glow particles each update). SetState plays track 0x130 for states 1 and 4 only (the two PlayMusic calls at 0x6e237f and 0x6e2b91); Close(2) plays track 0. The board's challenge flow is described under "The challenge's flow on the board" at the end.

## 3D mode

`-3d` (or the options' 3D box, which needs WebGL2 on the web) runs the game as the original does with Direct3D: the port's
GPUDisplay draws the 2D layers, TwistD3D (DrawPrimitiveEx, the .p3d meshes, render states and texture stages) the 3D
ones, through the shared GPU2D interface. A run without it keeps the 2D path (DDInterface), and the checks compare
2D runs against a reference to show that 3D work leaves them alone.

The original makes its Direct3D device (0x6193b0, flags 0x140) without D3DCREATE_FPU_PRESERVE, so once it runs, the
x87 computes in single precision. Two places show it in play: a curve's input steps and thresholds (CurvedVal's X87
rounds them to float while gD3DInterface is set), and a curve string's numbers, which the original reads with
StringToDouble (0x45dbf0) digit by digit, so "0.002" is 0.001999999862 and a 500-update curve is 501 long
(CurveNumber). Without them the hyperspace's states changed one update early. Other float code in the 3D paths is
written in double where the original's is, as in the 2D paths; if a 3D path is found to differ by a rounding, look
here first.

All 24 visualizers are ported (Visualizers3D*.cpp), each with its own generator kept in the original's order: some
draw from the global Rand, VizJellyBlob and VizZenWheels from the C library's rand (CrtRand, seeded from the game's
seed), VizMagicSnakes from its own LCG seeded 0xbeef0b01 and VizKaleidoCat from another. VizBuckyShards' environment
map is a cube map (the image's `cubemap` attribute, flag 0x20): the shared GPU code makes a GL cube texture from its
six cells in the original's face order and samples it by stage 0's camera-space reflection vector. It is checked
only in the port (a forced-visualizer run): the original shows it in blitz after an even number of blitz games,
which no seeded run here reaches yet. Slot 4 of a visualizer takes a key (VizZenWheels answers b, c, e, g and r), but
the VizManager's key function that asks it (0x758034) has no callers in the original.

Compared with the original (seeded autoplay, 3D, updates 1000 to 3000 and the generators' states to 6000; the
original on the desktop through visrun for this comparison only): the board's generator matches at every sample, and
the jump, the cube, the tube's arrival and the boards match to within a few levels per channel. The global Rand's
draws match call for call in every update compared (visrun's Rand hook, which tags each call with the update
counter: 1002 to 1365 and 1795 to 1815). They once parted at 1804, and the rotating backgrounds stood at other
angles, because the mode select ran one update early: Twist's framework gives a widget the reference's z-order in
PutBehind (gPutCopiesZOrder), which keeps the new mode select behind the menu and out of the frame it is made in.
The web build's 3D run (the
same seed, script and shots, in the Browser pane) matches the native port's from update 1300 to 3000 within a few
levels per channel; before that its main menu shows the site's player name and its buttons' labels sooner, because
the site makes the player rather than the autoplay.

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
- The menus' replays: the original sets a flag of its sound manager (+0x300a0) that sends every sound started while
  the replay board is up through DirectSound's parametric equaliser (GUID_DSFX_STANDARD_PARAMEQ, 0x6f41aa); the
  board's RemovedFromManager clears it. The port's sound manager has no effects, so those sounds play plain. This one
  is heard.
- A ReplayWidget's button is never deleted by the original; the port deletes it with the pod.
- A gem soul's gem (GemSoul +0x67c): at its 80th update the original clears the gem's +0x535 without checking that the
  gem is still there (it is deleted when it blasts); the port checks.

Challenge menu, piece 3 (partly done): the menu's Update now steps the end-of-challenge message at +0x9dc (`mAwardTelly`, an AwardTelly the board makes at 0x6b330d): at its update count 250 the rank bar's timer gains UpdateProgress(true, false) over 100, and once clicked it is removed and deleted and state 5 goes to 4 (0x6e4989). A click on a closed planet in the trial also starts the curve at +0x8a0 (0x6e54c3). Ported (0x6e4a25, 0x6e132d, 0x6dda7c, 0x6e665e): the menu's DrawAll (0x6e4a25) draws nothing at all while the sun's curve (0x85c558) is 0, unless space is bringing back the old level's picture (Hyperspace state 12 with +0x159 clear, then drawn over the menu by 0x70f836); otherwise the children and then `DrawSunAndTexts` (0x6e132d). With Direct3D off that draws, through the billboard 0x6dda7c (a PerspectiveCamera(gAspect45, gAspect, 1, 10000) on gMenuCamera projects the point; DrawImageTransformF, 0x422050, with the scale 0.00151515 / depth / the picture's width * size * 80, centred there, skipped below a depth of 0.0001; it sets the draw mode, the colour, unpacked by 0x401880 with its alpha as it is, and colorizing, and leaves them set), IMAGE_ECLIPSE_SUN at the camera's start (size 4.75, grey 255 - 255 * +0x928, alpha the sun's), IMAGE_ECLIPSE_MOON 4.1 along z from it turned about y by +0x720 * 1.5 pi (0x68fa0b, then 0x4c0c80 and 0x68b369, now `TimesMat3` and `TransformPoint` in Math3D; size 2.7, grey min(255 * +0x6b0 + 48, 255)), the additive IMAGE_ECLIPSE_RING there (2.9) while the sun is at 1, by (1 - +0x6b0) * sun * sun, the two dancing glows (IMAGE_ECLIPSE_DANCE2 and DANCE1, additive, while +0x640 is above 0: 280 * scale from the middle, from 0.3 of a turn to none and turned from half a turn to none by +0x640's input over its end), and with the menu all the way in (state 1 or 4): with no planet hovered IMAGE_CHALLENGEMODETITLE at y 516 * scale by +0x9d4 times the sun (the "CHALLENGE MODE" over the eclipse); with one hovered, "BEST: %d" (+0x19c) in FONT_DIALOG_TEXT, the description pictures +0x190 and +0x194 (the latter by +0x15c), the name picture +0x18c with its fading trail of larger copies (0.02 larger each, fainter by the fade squared, at least 0.03), and, in the trial, for a planet past the third (id above 2) or a challenge past the third, IMAGE_TRIALVERSION_CHALLENGE with its additive flash by +0x8a0. ButtonMouseEnter (0x6e665e) makes those pictures: the description (ChallengeInfo::GetText of the planet's next challenge) as two TextImages in FONT_PLANET_TEXT 0.3 of the width wide (the first with the font's layers 0x80dfb4 and 0x7dd6b0, the second with 0x80dfb4 only, OUTLINE clear in both), and the category's id (underscores as spaces, "  %d" with the stage added unless eclipsed) word-wrapped in FONT_FRUIT_AWARD, 500 * scale wide with 50 * scale line spacing, coloured from a table of 13 colours (0x6e6a8c) with a black OUTLINE, in a MemoryImage, and IMAGE_ECLIPSE_TITLE at 0.65 under an eclipsed name. The helpers: 0x41ff20 Graphics::SetDrawMode, 0x41ff40 SetColorizeImages, 0x4223e0 StringWidth (the font's slot 0x1c), 0x421180 DrawString (slot 0x28), 0x7575c2 DrawImageCentered (the whole picture), 0x422eb0 WriteWordWrapped and 0x4233d0 GetWordWrappedHeight (both with one argument more than this framework's, 0 in these calls), 0x4216f0 DrawImage stretched, 0x6e6619 Graphics' copy, 0x694387 a float to an int power, 0x6a1984 a CurvedVal as a white of its alpha. The constructor sets +0x198 to 1 (the port had 0) and the destructor (0x6e0c6c) frees the three pictures. The tip widget, an OrbitText that ChallengeButton 0x6e0aa5 makes, is ported now (see the board's flow below). Not ported: the zoom's own effects; UpdateProgress's glow. The planet ring is drawn smaller than the original's rim (about 46 px against 76 px for a settled planet at 800x600) and looks darker. A logged run showed the 600-art image (82x81), mOpen set, and the scale chain of 0x6dec2c-0x6dee33 (ring scale 2.25, Graphics scale about 1.31 when settled), so the cause is not found yet; the identical-play pass looks at it with the other scaled draws.

## The challenge's flow on the board

A challenge game now runs to its end, as the original does with 3D off.

**Timer.** `Board::UpdateChallengeTimer` (0x6bedd8) runs from SetChallenge and from UpdateGame on each update while the
board is idle. A time-limit challenge's clock on the tube is the limit less the seconds played (app +0xd00) and this
second's hundredths (+0x14e4). From the third second on, each new whole second goes through Blitz's marks
(ShowBlitzTime 0x6bea4b).

**Met.** UpdateGame's settle runs this once CheckGoal has set +0x2669 and the board and tube are idle. It sets +0x266a.
An ordinary challenge then records its stage (`Profile::SetChallengeDone` 0x733afc: the category's stage becomes the
number + 1, and +0x214 the category), saves the profile, opens the next category and calls
`Board::ShowChallengeComplete(false)` (0x6b330d). A night challenge calls it with true and makes sure Profile +0x204
reaches its category. A night challenge also ends met from the nuke (UpdateNuke, at 0.79 of +0x27b8), which then ends
UpdateGame.

ShowChallengeComplete does nothing while the menu already has its +0x9dc. Otherwise it makes an AwardTelly with
`ShowTrophy` (0x6a2318), adds it, sets +0x34ec, hides the board and starts space's end (0x714883). It then sets
space's +0x12b0 to 0x7dc510, puts the menu in state 5 with the telly at +0x9dc and clears space's +0x159, so the menu
draws the old picture. ShowTrophy plays SOUND_TROPHY_AWARDED and sets kind 1 and state 4. It shows a RankBonusWidget at
the middle and 0.275 of the height, worth 3 * number + 2 points; a night challenge with a best score gets
15 * ((max(+0x2634, +0x2638) - low) / (best - low))^0.8 instead. The trophy's curves are 0x7dbbd4 and 0x7dbba4.

**Space's end in a challenge** (0x714960). 0x70e567 gives space a screen-sized DDImage at +0x160. The VizManager and
the board are drawn into it: DrawAll, then the deferred overlays through the widget manager's mCurG. Then +0x155 is
set, the level becomes 0, the VizManager is hidden and +0x159 is set from gD3DInterface (0). A deviation: when +0x160
holds a level picture from a jump (a resource), the port makes its own DDImage and frees it later.

**Failed.** CheckGoal's result 2 ends the game (EndGame), or the nuke sets result 2. A timed challenge fails through
the per-second CheckGoal(true). Either way, UpdateGame's +0x34ec path puts the menu in state 4 and starts space's end.

**Quit.** The quit question sets +0x34ec, which takes the same path.

**RemovedFromManager.** A met challenge (ChallengeInfo's result 1, or +0x266a) counts as over (+0x36b0), so its save
goes. A night challenge whose +0x2634 beats Profile +0x204 stores it there and on its planet (+0x5c8) and makes the
planet's tip (`ChallengeButton::MakeTip` 0x6e0aa5). The tip uses FONT_PLANET_TEXT's layers (OUTLINE clear, 0x80dfb4
white, 0x7dd6b0 black). Its text is 0x7e0258, the best and 0x7e0250, in FONT_SCORE without Direct3D, in a rectangle
500 art pixels wide at -250 and 100 high. It shows twice, at 0.15 of the speed.

**OrbitText** (OrbitText.h/.cpp: 0xd4 bytes, a plain Widget; ctor 0x725732, Update 0x724e91, Draw 0x724f7c, dtor
0x725812). It holds a TextImage with 16 pixels round it. For each showing, mPos runs from -1 to 1 by mSpeed (0.01)
while the alpha rises by 0.02; after the last showing the alpha falls by 0.02 and the text is freed. In 2D it is drawn
at mX less the art scale times the text's width, 100 art pixels above the middle. A black shadow goes 3 art pixels off,
then the text in the ColorCycle's colour. The Direct3D ring of strips is not ported. The button now frees its tip
(0x6e0864). The original leaves an older tip behind when it makes a new one; the port frees it.

**Restarts** (dialog 0xb's OK). `RestartChallenge` (0x6c9987) sets the challenge again and clears:

- the app's stats (0x75fc8a);
- the board's +0x2234, +0x2614 and +0x3560;
- the pattern's progress and the tube's +0x538;
- +0x2634 and +0x2638;
- the saved game, and +0x168, +0x16c, +0x1f8 and +0x1fc.

It then sets +0x1390, plays SOUND_RESHUFFLE1 and runs NewLevel, ClearBoard and FillInBlanks. It restarts a timed
challenge's music and redraws the tube. With track 305 playing, its channels are set first.

`RestartBlitz` (0x6c7a75) does the following, in order:

- frees the points' smoke (+0x2170);
- clears the app's three best boards (+0xfa0), the saved game and the stats;
- resets the bonus (tube 0x7035dc, +0x3a8, +0x3f8, +0x3fc, +0x2660, +0x2664) and makes a new bonus challenge;
- turns the tube's colour green at once;
- clears the counts and the chain's steps and fades the music out;
- gives 30000 hundredths and sets state 3;
- runs NewLevel and ClearBoard, and fills with +0x20dc at 2;
- calls the VizManager's slot 7.

**Music.** `PlayChallengeMusic` (0x6b66d2) runs from StartUnknown34F0 in a challenge and from the restart. An untimed
challenge fades to track 306, or to 305 with a Moves goal (then `UpdateTurnBasedMusic`). A timed one stops the music
and plays track 300 at once, or 308 for a 180-second limit.

`UpdateTurnBasedMusic` (0x6b2fda) also runs from MouseDown's twist while track 305 plays. It silences track 305's
channels 6 to 12 and 18. Then it walks a table of (moves, channel + 1) pairs: each channel plays at its default once the
moves made (app +0xcf8) reach its count, and is silenced at once otherwise. The first count is 7, or the Moves goal less
5 when that goal is over 10. The tempo is MUSIC_CHALLENGE_TURNBASED_MIN_TEMPO plus the moves, at most _MAX_TEMPO.

**LoadGame** reads the challenge's category, number and pattern progress and sets the challenge (GetChallenge,
SetChallenge). The port skips the progress when there is no such challenge, where the original would crash.

**FillPieces.** In a no-cascades challenge (0x800), new gems still above the board that would match get new colours,
for up to 100 passes. A constant-doom challenge (0x1000) without a devil gem runs 0x6bde87.

**0x6bde87 is not only the challenges'.** `Board::NextTurnGems` (0x6bde87) spawns the lock gems and doom gems, and
StartTwist runs it after every twist in every mode (0x6c7225). Its steps:

- It counts the locked gems, and stops in a puzzle or with +0x21ac set.
- mLockGem's turn rolls (`LevelValue::NextTurn` 0x74ec17).
- A gem gets the lock when CanAddCounter holds, there are fewer locks than MaxLocksOnScreen and the lock is due
  (`PreLockPiece` 0x6ace4d: +0x504 to 0, +0x4fc to 2, SOUND_GEM_PRELOCK). In a constant-counters challenge it is a gem
  next to the bomb or devil gem with the lowest count (the directions table 0x84e978). Because each direction is taken
  out of the list before the loop steps on, only directions 0, 2, 4 and 6 are tried. Otherwise the gem comes from a
  random place: an even Rand85B970 picks any cell, an odd one a cell inside the border, and the first that can is taken.
- Then a doom gem (flag 12) may appear. That needs no doom gem on the board, mDevil's count at 0 or more, an idle board
  and a mode other than Zen. It still does not appear if the level's matches are under three times its moves
  (+0x2234 [0x39] / [0x21]), or if |RandUnit (0x6a3477)| * (tube progress * 1.2)^4 is under 0.05. A constant-doom
  challenge always wants one unless a devil or doom gem is there.
- The doom gem goes on a random cell inside the border that no cursor position would match (rows 0-7, columns 0-5,
  MoveCursor with a MatchInfo). The original loops until it finds one.

Classic's config has lock gems from level 4 and devil gems from level 13. The port ran this only in a challenge at
first; it now runs it in every mode, as the original does, so Classic and Blitz get their lock gems and doom gems (and
their Rand85B970 draws) where the original has them. Blitz's and Zen's configs have neither (first level -1), so there
NextTurnGems draws nothing and changes nothing.

**Gem locks and doom gems breaking.** A locked gem in a match, or one a fruit's wave reaches on its third ring
(FruitShockwave::Update 0x74c1aa), breaks its lock: `Piece::BreakLock` (0x72952f) runs BreakSpecial with the type,
adds stat 0x48 and clears the lock flag. BreakSpecial's lock branch (0x6c03a4) plays SOUND_GEM_LOCK_BREAK and puts two
lines of nine effects of type 13 (14 for a type) into the board's first manager, their cels alternating, the second
line turned by pi; the second line's x does not move with the row (the original's sum). A doom gem (flag 8) in a match
adds stat 0x44, runs BreakSpecial and is blasted. BreakSpecial's doom branch sets +0x3454 to 1, plays
SOUND_DOOMGEM_DESTROYED, adds 25 effects of type 38 ten steps along their own velocity, then a hundred times three
IMAGE_SPARKLET effects (purple 40,0,40 at 3.01 times the size, 100,0,100 at 1.1, and 200,250,200, the size 1.5 to 1.6,
additive, flung at a random angle) into the second manager, with the framework's Rand drawn once before each colour,
and, unless the board is between levels (+0x26a8), ten lots of 50 points whose smoke goes dark purple (100,24,64), not
additive, each later and slower. Its two heat waves (0x6f9efe) are Direct3D only.

**The replays** (`build/logs/r3f_notes.md` has the decoding). Every move starts from the game saved before it (+0x2384,
with the cursor's row and column at +0x23a4/+0x23a8, the rank and +0x160 at +0x23b0/+0x23ac); UpdateRecords keeps
the best move, cascade and score with it in the app (+0xfa0, for the end stats) and the profile (+0xa0, for the high
scores' stats page). After a good move (+0x1e8 up by more than 20 in the move, a cascade, +0x1508, over 3, or +0x262a; EndTurn), out of
challenges and blitz, ShowReplayPod (0x6ab48d) raises the ReplayWidget pod (+0x2624, ReplayWidget.cpp); the next
twist, a wheel or the level's end sends it down (the level's end sets +0x2628, and the pod comes back when the rank
bar goes). Its button, and the tutorial's replay button, save the game as it is to +0x23b4 (restored only if the
board is left during the replay), put the rank back and call StartReplay (0x6ca727) with the move's save: the game
is loaded, the cursor put back, the board ignores the mouse, +0x2629 is set and, with the playback, state 1 rewinds
the recorded frames to SOUND_REWIND (faster and faster: +0x3448 drops by min(n / 100 + 2, n / 20) an update, the
streaks' generator +0x2a80 seeded from the update count over 3) before StartTwist makes the move again. The global
generator comes back with the save, so the same gems fall. While +0x2629 is set the board runs slower (Update's
speed), +0x2a80 is seeded from the update count over 7 and REPLAY blinks in the corner; the move's end clears it and
gives the mouse back. The menus' replays (HighScoresPanel 0x695124, EndStats 0x6fbde0) make a board of their own
(mode classic or the last game's, flags 8, +0x37c4) that starts the move at once; the panel waits for its move to end
(+0x3921, +0x3444) and removes it.

**Effects of the gem locks and doom gems** (EffectsManager.cpp; the constructor 0x6f4a05, Update 0x6f8792, Draw
0x6f6451, whose switch goes through the byte table 0x6f7a19). Type 13 is a lock's link flying off: a random turn, a
random sideways speed of up to 0.025 of a gem's width times the art scale, a random vertical speed of up to the art
scale and a fall of 0.05 times it; each update it fades by 0.002, ends below the screen and moves across a second
time; it is drawn as IMAGE_GEM_LOCK_LINK1 or 2 by its cel.
Type 14 is the same with a red additive IMAGE_GEM_LOCK_LINKS_GLOW under it and a puff of type 7 (IMAGE_GEM_FIRE_SMOKE,
rising, fading from at most 0.5) left behind each update. Type 38 is a doom gem's shard (IMAGE_SHARD1, dark grey, flung
up to 5 times the art scale an update, growing by 0.01): it lives 75 to 175 updates, turns and fades by 0.01, and
leaves a puff of grey steam every tenth update; it is drawn like type 0.

**Electro gems** (ElectroStrike.h/.cpp, BoardMatch.cpp, BoardUpdate.cpp, BoardDraw.cpp). An electro gem going off in
BreakPiece (0x6c0ccf) does nothing while a strike of its own runs; otherwise it leaves the other strikes' lists of lit
gems, counts its stats (0x40 and 0x53 for a power electro gem, 0x41, 0x3e) once, sets its points' entry to -0x65,
plays SOUND_WILDCARD_DESTROYED, starts an ElectroStrike (0x71f10e) at +0x149c, scores 150 points, stays on the board
(+0x51d) and plays SOUND_FLAMELIGHTNING for a power electro gem, else SOUND_ELECTRO_START1. The strike lays a bolt
(ElectroBolt, 0x71f003) along the gem's row and one down its column, three of each for a power electro gem, which also
gets the IMAGE_BOOM_NOVA and IMAGE_BOOM_NUKE curves (0x7e6750, 0x7e672c, 0x7e66f8, 0x7e66d0) and SOUND_BOMB_EXPLODE at
1.6 of the nuke's curve. A bolt grows for ten of its seconds and every fifth update is made anew from Rand (the
framework's): a line of segments half a cel of IMAGE_LIGHTNING long, jagged by up to 80 art pixels (more as it grows)
except at the ends, drawn as quads of IMAGE_GRITTYBLURRY (the glow) and of a random fifth of IMAGE_LIGHTNING, each
twice and additive, at eight times its remaining life in alpha. UpdateStrikes (0x6c87d2) lights the gems: every tenth
of a second the lightning reaches a cell further each way (the first step plays SOUND_ELECTRO_EXPLODE), each gem it
reaches that no strike has lit gets 20 sparks (type 17) and glows up by 0.015 an update until it breaks (coal through
BreakSpecial); once nothing is left to light and the lightning has crossed the board, the electro gem itself breaks and
the strike fades out in four steps. A strike after the n-th waits while the n-th is younger than 25 * n updates. With
Direct3D the bolts go through the 3D interface: not ported. Two things differ from the exe, neither visibly: the port
deletes a finished strike (the original only takes it off the list, a leak), and ClearBoard deletes the strikes it
clears (0x71eb9e). The bolts take their colour from the table at 0x85d850, by the gem's colour: red (255, 32, 32),
white, green, yellow, magenta, orange (255, 128, 0), blue (0, 128, 255) and black, made by the static initialiser
0x7d5bf3. (An earlier reading missed that initialiser and drew every bolt grey; the user spotted it from videos.)

**The fruit gem** (FruitAwardScreen.h/.cpp, BoardTurn.cpp, BoardMatch.cpp, BoardUpdate.cpp, BoardDraw.cpp). A fruit is
awarded when the multiplier goes past its top level (SetLevelSteps sets +0x214) and at update 2190 of a fruit dance.
SettleTurn and CheckSettled then call `Board::AwardFruit` (0x6ba5b4) with -1, -1 and take the chain's steps back: steps
past 151 (at most 14) go on top of the steps of the levels below the board's (GetLevelSteps), through SetLevelSteps.
AwardFruit's colour is the count of fruits awarded (+0x218) modulo 7. Its gem is FindHintPiece's (the bomb with the
lowest counter or a locked gem, whatever its colour), else the first plain gem of that colour from a cell picked by
Rand85B970 round the grid, else up to 100 cells picked at random (Rand85B970, the column first), any plain gem; after
100 misses there is no fruit. The tutorial's `awardfruit a,b` names the cell (a - 1, b - 1). The award is a
FruitAwardScreen (0x228 bytes, vtable 0x7e24c4) queued in the board's dialog queue (+0x23dc); the tube stops taking the
mouse. When it comes up it plays SOUND_TROPHY_AWARDED and over 1 / 0.015 updates leaves a trail of IMAGE_FRUITS cels
(type 0 effects on the board's +0x1648, every third update, additive, scale 2.5 down to 1) from the tube's top (its
bottom during a fruit dance) to the gem, which then becomes the fruit (MakeFruitGem) and the screen closes. Only its
state 4 ever runs; states 0 to 3 are left out (see the header). FruitTraveller (vtable 0x7f1aa4, constructor 0x75089a)
is never made by the exe and is not ported.

A fruit breaking (BreakPiece's fruit branch, 0x6c0ccf) waits if another's blast runs (+0x88 and +0x29c set, not
broken). Otherwise it counts stat 0x45, plays SOUND_GEM_FRUIT_DESTROYED,
counts +0x22c and sets +0x24c when the board has no lock or bomb (neither is read), starts the blast (+0x248 at
0.0001), notes the fruit's colour, cell and id (+0x420, +0x250, +0x254, +0x258), sets the four curves +0x260, +0x2d0,
+0x340, +0x3b0 (0x7dd2f8, 0x7dd2dc, 0x7dd2b0, 0x7dd28c) and moves +0x265c from 0 to 2, or 1 while the tube's +0x3b0 is
set (an already started challenge sets +0x2664 to -1 instead). `UpdateUnknown248` (0x6c4466) runs the blast: +0x248
grows by 0.002 an update; at 0.5 the fruit's own 500 points pop up. Up to 0.85, four rings spread from the fruit's cell
(ring i at 2.8 * blast - 0.15 * i - 0.5 on +0x260, the last one slower), and each cell a ring crosses (within 0.3 of a
cell of its radius, 4.4 * +0x260) is touched: gems of the fruit's colour light up (+0x29c) and on the last ring break
(BlowUpPiece, or BreakPiece for a special gem; 10 points, SOUND_ROWBLOW and 20 type 12 sparks drawn from RandSigned
pushed away from the fruit), another fruit there becomes a power gem and goes off for 1000 points; bombs and devil gems
count up by the ring's number (their glow curve 0x7dd424, with SOUND_BLITZ_COUNTUP pitched by the blast) and on the last
ring locks break. The fruit fades (+0x340, alpha 1 - 6 * (blast - 0.4)) and grows (+0x3b0). Past 0.85 the board drops
and refills (with the amnesty count at 0), and at 1 the blast ends and the turn settles. While it runs, the gems the
blast can reach (+0x2f6 clear) are left out of the board's own pass and drawn over the darkening in DrawOverlay
(0x6b4a39): black at up to 0.6 (in over the first quarter, out over the last fifth), the rings in IMAGE_FRUITNOVA,
their colour's hue from +0x260 (lightness 128, 200 for the last), alpha from +0x2d0, and the fruit's glow
(IMAGE_FRUITBLAST_FRUITGLOWS, cel the fruit's colour) at +0x3b0's scale and +0x340's alpha.

In state 2, once the last ring's radius reaches a point by the tube (or the blast ends or passes 0.9) and the tube has
no loaded challenge (+0x330), the bonus challenge starts (+0x265c = 3, the tube's curve 0x7dcf14, StartBonusChallenge
with the fruit's colour excluded); state 5 (missed, ResetFruitCount) clears it. Met (+0x265c = 4) brings the fruit
dance once the board is idle: fruitdance.pam (a PopAnim at +0x267c, `PopAnimListener` at Board +0x150) over the board at
1600x1200 times the art's scale, hidden as a widget and drawn by DrawOverlay, with track 0x133 at the louder of the
music's and the sounds' volumes. Board::Update stops the game and the background (VizManager +0x1a0) over the dance's
2200 updates, easing in and out over the first and last hundred, darkens the screen to 180 the same way, counts the
updates only while the dance is not paused (PopAnim +0x170 `mPaused`, set while the options, a pause or a frozen board
are up, which also pauses the track; Mute and Unmute, 0x75f631 and 0x75f672, do the same), and at 2190 sets +0x214 for
a fruit. The dance's commands (`Board::PopAnimCommand`, 0x6c3cf5) pop up points: "small c x y" the next of a list of
42 (100, 200 three times, 300 and 400 five times each, then 100) halved, larger by 0.25, and "large c x y" the next of
15 (1000 to 5000, 10000 five times, then 0) halved, half again larger and living 0.4 longer, at x, y in the animation
in gem colour c, none stopping high. At 2200 the dance is deleted, the challenge's state cleared, +0x21da set and the
game's music played again (Blitz's own, the level's track, or the menu's). While the bonus challenge's bracket shows
(+0x5f0 above 0, the board's +0x268c at 1 and no +0x267c), GoodTube::UpdateTick throws three IMAGE_GEM_FRUIT_SPARK
sparks a frame (additive, scale 0.5, shrinking and fading by 0.01, a spin of 0.05 plus or minus 0.02) from a random one
of 512 angles round the bottom's middle (0.525 across, 0.36 of the bottom image down), at that angle's radius in the
table 0x7e3390 (512 floats of game data), flying out at 1.1 times the scale, with +0x5f0's alpha.

**The zen jump** (Board::ShowZenJump, 0x6a615b): at the end of the game's last turn, before the game over, a Classic
game below +0x21e0 3 or a Zen game of 180000 points or more puts up the mode select in its zen-jump form (Mandala mode
1 over Classic, 2 over Zen) as the board's dialog (+0x2208), once for each player: it plays SOUND_MENU_SLIDE, sets the
profile's flag 0x11 and saves the profile, and clears +0x20d0. Board::Update also calls it with 1 when its countdown
+0x257c reaches 0, but nothing in the exe sets that countdown above 0.

**The hyperspace's camera path in 2D.** Hyperspace::Update (0x715c21) handles the 2D jumps itself (transitions 8 and
14, 9 and 10, 11) and skips to its end; every other transition, and state 0, runs the camera's path from 0x716366 in 2D
too. That is transition 12 (a board that takes the app's hyperspace without the Play button: every Zen game, which the
mode select starts that way, and every continued game), 13 (StartEndGame), 0 (ShowHyperspace) and the menus' 1, 2, 4
and 5. In 2D it does three visible things: the VizManager's speed (+0x1a0) follows the curve +0x1e8 (SetTransition(12)
sets it to a constant 1; nothing else in 2D sets it, so under transition 13 it reads 0 until ~Board puts 1 back), the
board's brightness follows +0x258 (SetTransition(12) sets that to a constant 1 as well, so a Zen or continued game is
at full brightness every update, whatever 12's own fade at +0x12b0 says; the fade shows only as space over the board),
and, while the app's board holds this hyperspace (+0x200c), the ship's nine engines each draw Rand() % 6 puffs of
thrust, each with three RandSigned, a RandFloat and two more RandSigned and a fresh Rand() for the loop's test
(0x717620-0x717dbe). The puffs are particles only Direct3D's DrawOverlay draws, so the port makes none but draws the
same numbers on the framework's stream. The planets, the camera and the states 2 to 9 of this path are Direct3D's.
Board::DrawAll draws the jump's tunnel (0x711b65) under the board during transition 11, a challenge's start.
LoadGame (0x6c967c) ends a running hyperspace at once (its background chosen at the level one down, the widget, the
LevelUpShow and the dialog +0x26a0 deleted, the VizManager shown at speed 1); a game loaded at a lower level than the
board's sets the background through a hyperspace made and deleted for it. After the zen jump, Board::Update puts up the
"returning to game" dialog (0x6d6fa2) when its countdown (400, set by the Mandala) runs out, with the screen's darkening
behind it.

**Never reached, left as NotPorted.** These paths exist in the exe but no game reaches them, each for the reason given
in its comment: the puzzle mode (5) with its panel, board, fill, sparkles and tube widget (no menu starts mode 5: the
mode select's buttons are modes 1 to 4 and the main menu's Play takes the last game's mode); the board's state 2
(nothing sets +0x160 to 2); the "ice gem" (FLAG_6, from a challenge's square match, which needs the profile's bit 6 from
a tutorial line "unlock icegem" that tutorial.txt lacks and "IceGemsAllowed", false in config.xml); the "angel gem"
(FLAG_7, set only by Make7, whose callers need it already); the droppers (+0x21e4, made only for FLAG_1 gems, a flag
set only by the droppers' own update); FlyingText's kind 1 (both makers are given kind 0); the tutorial's title
(+0x1f4, +0x1f8 only ever 0); the board's image +0x15dc, the objects at +0x1488 and the pieces' +0x528 (only ever 0);
+0x284's growth (only ever 0 or copied); 0x6c425f (+0x23d8 set only by 0x6bb145, which nothing calls); 0x74db1b
(+0x210c only ever 0); NewLevel's backdrop (no twist-back image in the game's files). BurstGems' two property reads
("ExplosionChainBonus" and "ExplosionChainBonusAdd") are dropped by the original and left out.

## The credits (Credits.cpp)

Sexy::Credits (0x288 bytes, a TwistWidget and ButtonListener, 0x6e7f36-0x6e95f1) is the mode select's easter egg.
Mandala::ButtonMouseEnter (0x722c74) counts the mode buttons entered in the order 0, 3, 2, 1, 0, ... (classic, zen,
blitz, challenge, round the circle backwards); any other button entered (the main menu button too) starts the count
again, and the twelfth sets +0x419, on which the next Mandala::Update calls the app's 0x75fd40
(`WinTwistApp::ShowCredits`) instead of its own update: a new Credits at app +0xf94, z order 1000, added and made a
base modal (0x4c3a10 is AddBaseModal with the manager's default flags). AddedToManager (0x6e8760) plays
SOUND_TROPHY_AWARDED, makes the Continue button (IMAGE_RANKBAR_CONTINUEBUTTON, centred, 115 art pixels above the
bottom; only its ButtonDepress, 0x6e8403, is heard: state 5) and adds the sparks' EffectsManager, and starts two
looping curves (0x7e0e10 the fruits' tilt, 0x7e0dc8 their bob). Update (0x6e903c): the box is three quarters of the
screen each way, in the middle; state 1 darkens the screen (+0x160 by 0.02, drawn by DrawAll as Color(0, 0, 4) at 200
times it), 2 grows the bars (+0x170 by 0.01), 3 brings in the box (+0x15c by 0.005) until both are 1, 4 adds a line
of the list each time +0x194 falls to 0 or below (then +50 art pixels), and 5 fades both back to 0, when it sets
mClicked; the app's UpdateFrames (0x765f8d, before the framework's) then removes and deletes it. While the bars grow
(states 2 to 4, grow below 1) every update makes five type-10 effects at the box's top or bottom edge in the gem
colours (Rand() % 7, Rand() % 2, RandSigned, in that order; type 10 draws in its own blue, so the colour is never seen). Every other update (+0x28 even) the lines rise by mScale:
each line's alpha goes 5 down at or above 150 art pixels under the box's top and 5 up below it; a line above the top
goes, and the last one gone closes the credits. The list (0x85c668, 142 wide texts, the count at 0x84ecb4) is set by
the static initialiser 0x7d51ae: a line starting with a backslash loses two characters, "\B" making it a heading in
FONT_SCORE Color(200, 40, 150), the others FONT_DIALOG_CHECKBOX Color(255, 220, 52); "GEM" (0x7e0f88) becomes
IMAGE_FRUITS's next cel (0 to 6) in white. Draw (0x6e817f): the box tiled with IMAGE_INGAMEUI_GRIDSQUARE in Color(128,
128, 255) at 225 times +0x15c, the two bars (both as tall as the top one) stretched to the box's width times the growth
times 1.03 in Color(200, 200, 255), then inside the box the lines centred, the fruits by DrawImageCentered turned by
the tilt times pi / 4 and raised by 25 times the bob. The constructor, destructor and RemovedFromManager free
everything (no leak). gGemColors before the first board are ten Color() (opaque black, static initialiser 0x7d5096;
the board copies only seven, 0x6c59c4), so the sparks' colour is black until a game has started (unseen: see above).

## Oddities

- **The credits' sparks start their turn from the heap.** The Effect constructor (0x6f4a05) does not set +0x9c,
  +0xa0 or +0xa4, which type 10's update and draw read (the turning rate's blend), and the credits (0x6e903c) do not
  set them either (AddShards does). In the original they hold what the heap block held, often a dead effect's values;
  a +0x9c above 1 makes the first update draw a RandSigned, a NaN never does. The credits' sparks do not move (no
  velocity), so the turn has no visible effect, only the possible Rand draws. The port starts them at 0, so a spark
  dies (80 updates) before +0x9c reaches 1 and draws nothing. Matching it would mean copying uninitialised memory.

- **A one-move goal's colour comes from another list.** The challenge display's one-move branch (GoodTube::DrawBody,
  0x7095fd-0x709625) gives a colour stat (75 to 81) the icon of the tube's own stat list at the same index (+0x56c,
  which only the pattern and on-board displays fill, so it holds whatever the last of those left), not the goal's own
  colour; past that list's end the original's checked index would stop the game. No shipped challenge has an
  "Onemove" rule that counts a colour, so it is never seen. The port reads the list the same way and gives a goal past
  its end no icon. Fixing it would be reading the goal's own stat, which the original does not do.

- **The doom gem's placement can hang.** NextTurnGems (0x6bde87) picks random cells inside the border until one is a
  plain gem (no flags, colour 0 to 6, not breaking or blasted) that no cursor position would match (0x6be313 to
  0x6be46a: the hundred-try count is only counted down after a doom gem is placed, so it never limits the search). When
  no inner cell qualifies, for instance when every inner gem is locked or special, or every one sits in a possible
  match, the original loops for ever and the game freezes. The port keeps the loop as it is. To see it, a board whose
  36 inner gems all carry flags when a doom gem is due (level 13 or later in Classic, or a constant-doom challenge).
  Fixing it would take a limit on the search, which the original does not have.
- **The fruit dance's points can run past their lists.** PopAnimCommand (0x6c3cf5) takes each "small" or "large"
  command's points from a list on its stack (42 and 15 entries) by a count it never checks, so further commands would
  read whatever lies beside the list. fruitdance.pam sends 37 and 15, so the original stays inside them; the port gives
  no points past the end. To see it, an animation with more commands. It would take a bound the original does not have.
- **A replay's rewind still draws a few things.** While the recorded frames play back, GraphicsRecorder skips the
  board's own draws, but its DrawImageTransform (0x70dfad) hands the float, mirrored and float-rotated cases straight
  to the Graphics without looking at the skip flag, so those images still appear over the rewind. The port keeps it.
  To see it, a replay's rewind while such an image is on the board. Fixing it would be a skip check the original lacks.
- **An award's gem ignores its colour when there is a bomb or lock.** AwardFruit (0x6ba5b4) works out the next
  fruit colour (+0x218 modulo 7) and looks for a plain gem of it, but only when FindHintPiece finds no bomb or locked
  gem; otherwise the fruit goes to that gem whatever its colour, and the count still moves on. Seen on any board with a
  bomb or lock when the multiplier passes its top.
- **A coal gem's embers start from the heap.** Piece's constructor (0x72c1fa) never sets the piece's update counter
  (+0x4f0), which Piece::Update counts up (0x72c67b) and a coal gem tests every 20 updates (0x72cf01) to light an
  ember with two Rand draws. In the original the counter starts at whatever the heap block held, so the updates where
  a coal gem draws, and every random number after, change from run to run: the same seeded 3D autoplay game drew its
  first ember at update 10003, 10014 or 10016 in three runs and not at all by then in a fourth. The port starts the
  counter at 0. For comparisons, visrun's `zerocnt` makes the copy's constructor clear it too (a jump from 0x72c57c).
  Matching the original would mean copying uninitialised memory.
- **3D mode computes at single precision.** The original makes its Direct3D device without D3DCREATE_FPU_PRESERVE,
  so while 3D mode runs its x87 rounds every arithmetic result to a float's 24 bits (the window thread's control word
  reads 0x007f, by visrun's `fpucw`), while double constants keep their value. The port's float math is SSE natively
  and WebAssembly on the web, and neither has a precision control, so the port follows it only where it was found to
  decide play: a curve's steps and a curve string's numbers (CurvedVal), and SpeedText's sweep. The rest keeps the
  2D precision. To see it: the seeded 3D autoplay game (`-3d -autoplay=1 -seed=1234`, the original through visrun with
  `nomouse` and `zerocnt`) draws the global Rand call for call like the original through update 10282; at 10283 a
  BlobDrift blob, whose drift sums rounding over thousands of updates, falls off the top and re-enters one update
  sooner in the original, and the draws part from there. Following it everywhere would mean rounding each step of the
  3D-only code and of everything the board does in 3D (hundreds of places, the accumulators included), or running the
  native build's math on the x87, which the web build cannot do. 2D mode, where the original keeps the CRT's 53-bit
  precision, is where the two are compared for identical play.
