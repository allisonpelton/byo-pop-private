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
button, and the profiles' files (Profile). Not yet: the panels (MenuPanels.h has their fields and the methods the
menu calls, which do nothing yet), the new player panel (menu +0x3d4, class NewUserPanel 0x699924-0x69ab42), the end
of game stats (EndStats), the registration widget, the gem prerender, the app's music, and starting a game.

Open question for the profiles: the user's own users\users.dat (written 2020) stores "Allison" with a length of 4
where WriteUTF8String (0x460fb0) writes the character count, 7; the exe's ReadUTF8String (0x461580) would then read
"Alli". The retail exe the port follows may not be the one that wrote the file (the Steam exe differs); check with
the original before the profile panel is ported. The port reads it as the retail exe does, and so finds no player.

Game texts: the original's wide texts (L"...", such as the tooltips and the dialog texts) are UTF-16 in the exe;
gametext.py lists them as GAMETEXTW (length in bytes) and GameText.cpp converts them. Before this, every wide text
was read as its first letter.
