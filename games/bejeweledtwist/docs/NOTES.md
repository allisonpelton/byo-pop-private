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
(+0xf78) in front of the Logo in front of the loader (LoaderScreen::AddedToManager). Still missing behind it is the
Hyperspace background (0x15e0 bytes, app +0xf20, put up by 0x75f588 just before the loader), and the main menu that
slot 110 (0x76657b, `StartMainMenu`) brings up when loading is done; until then the screen goes black once the loader
finishes.

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
