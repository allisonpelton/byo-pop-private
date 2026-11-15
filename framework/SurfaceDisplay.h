#ifndef __SURFACEDISPLAY_H__
#define __SURFACEDISPLAY_H__

// port: the display of an app whose whole screen is one opaque image of its own, drawn in software into a buffer it
// keeps (the two Mahjong Escape ports: the engine in engines/jollybear draws an 800x600 buffer and knows which rects of
// it changed since the last frame). It stands in for DDInterface only where the app's CreateDDInterface makes it, as
// GPUDisplay does for Bejeweled Twist's 3D; no other app is changed, and the web build makes DDInterface.
//
// The app hands it each frame with SetFrame (the buffer and the rects that changed). Nothing is drawn through the
// widgets: the widget that used to draw the buffer into the screen image draws nothing, and DrawDirtyStuff only decides
// when a frame is shown. Present shows the frame in one of two ways, chosen by the size of the window:
//
//   Surface mode, when the window's drawable is exactly the logical size, or 2, 3 or 4 times it each way (the original's own
//   window, and the one the framework starts at 2x): there is no SDL renderer (and so no Direct3D device, its driver's
//   thread or its memory). At 1x Present copies the changed rects into the window's SDL surface and has SDL update those
//   rects of the window (a GDI blit). At 2x to 4x, on Windows, GDI stretches the buffer into the window itself (GdiStretch,
//   SurfaceDisplayGdi.h, the same as stretch mode's, the changed rects as the clip region), and the window keeps no surface
//   of its size; elsewhere, or with SURFACE_PRESENT=copy in a test run, the changed rects are copied into the window's
//   surface with each pixel repeated as often as the window is larger. Both give the same pixels at a whole scale, and the
//   stretch costs the processor about what the 1x copy does, where the repeated copy grew with the window: Dynomite's
//   scripted minute at 2x on its software screen took 12.4 G cycles with the stretch and 23.6 G with the copy (12.4 G at
//   1x), and Big Money's 4.2 to 5.8 s of processor time with the stretch at 2x to 4x against 11.5 to 26.6 s
//   (docs/MEMORY.md, "Windows at 2x to 4x"). At those sizes the renderer shows the same picture (its screen texture at the
//   render scale is those repeated pixels, shown as it is), so the choice is not seen.
//
//   No vertical blank holds a window without a renderer: the framework's own millisecond pace places the frames on the
//   update loop's 10 ms steps, and a quarter to a half of a 60 Hz display's refreshes showed a frame twice or skipped one
//   where a vsync present shows one new frame each. So on Windows each present of surface and stretch mode waits for the
//   desktop compositor's next composition instead (DwmFlush, GdiStretch::WaitForComposition), as a vsync present waits for
//   the blank: every refresh then showed one new frame, for about 2% more of the game's processor time (Dynomite, six
//   pairs turn about; docs/MEMORY.md, "The compositor wait"). The user chose it as the default on 2026-11-15;
//   SURFACE_PACE=ms in a test run gives the pace back. The wait is skipped with -fast, -drawevery and -hold, as the pace
//   is, and before the game has loaded. A wait that fails, or that holds the frame for more than 250 ms (the compositor
//   not composing), hands the frames back to the pace for good.
//
//   Renderer mode, any other size where stretch mode is not taken (a full-screen window is the desktop's size), and a drawn
//   cursor at any size: DDInterface's own path (the screen texture at the render scale,
//   scaled into the window, vsync), with the frame put into the screen texture from a texture of its own of which
//   only the changed rect is uploaded each frame, so that nothing but the changed rect is copied and no image of the app
//   is kept up to date besides the buffer.
//
//   Stretch mode, a third way for any other size (Windows): still no renderer, and Present paints only what changed into the
//   window's presentation rect (SurfaceDisplayGdi.h), in one of two flavours. Its cost is that of the surface at any window
//   size (a 3840x2160 desktop included), where the renderer costs a Direct3D device, its driver's thread and a present of the
//   whole window every frame.
//   - The smooth one, the renderer's own picture computed on the processor (LinearStretch.h; docs/MEMORY.md, "A smooth
//     stretch on the processor"): the same pixels as the renderer's at most sizes, and at the others a few seams one level
//     off. It is drawn a band of rows at a time into a bitmap of under 1 MB, and a changed rect repaints the window pixels
//     that read it. It is taken where the renderer would be, while kLinearStretchByDefault says so (SURFACE_STRETCH=3 in a
//     test run gives it, 0 neither flavour).
//   - The nearest-neighbour one, when the app allows it (AllowStretch; the two Mahjong ports, off in their ports): GDI's
//     stretch of the buffer, whose picture is the buffer's pixels each repeated as often as the window is larger, 3 or 4 times
//     at one place and 4 or 5 at another when the window is no whole scale, where the renderer's texture filter blends them.
//
// An SDL window cannot have a renderer and a window surface at once, so a change of mode (UpdateDisplaySize, which
// runs at every size change and at the start of every frame) destroys the one and makes the other. The window, its
// focus and its position are untouched; the buffer is the app's and nothing in it is lost, so the first frame after a
// change is shown whole. One change is not undone: a window that has had a hardware renderer (a full-screen start, a size that
// is no whole scale) never uses the surface again, since Direct3D 11's swap chain leaves the window showing its last frame
// whatever a GDI blit puts there afterwards (found on the visible desktop; NoteRendererOnWindow). A display that takes stretch mode never
// makes a renderer for those sizes, so its window keeps both of the modes that need none for good.
//
// Other apps that draw their screen into a CPU buffer take it the same way: override CreateDDInterface (under
// #ifndef __EMSCRIPTEN__) to return a SurfaceDisplay, keep the screen as one 32-bit buffer the app owns, and call SetFrame
// before the widgets draw, with the buffer, its pitch and the rects it redrew (or everything, or nothing).
//   - TipTop: the buffer is its own std::vector<uint32_t>, the rects its list of changed rects (usually under 30, up to
//     about 150); no widget image is kept.
//   - Word Harmony: the pixels of the MemoryImage its ScreenGraphics draws into, and the rects of the tiles it redrew.
//   - Cosmic Bugs and Water Bugs (the Retro64 engine): the 16-bit screen is converted into an app-owned 32-bit buffer for the
//     bounding rect it marked dirty, which is the one rect handed over (everything when it marks the whole screen).
//   - Alchemy cannot: its screen is drawn by the GPU path (no CPU image of it exists), so it keeps DDInterface.
// The display shows the buffer and nothing else, so an app that draws widgets over it must have them in the buffer first.
// The framework's own widget is the exception (SexyAppBase::OverlayWidget, the mod menu, which F2 opens over any game): while
// it is open, renderer mode puts the frame into the screen texture at BeginFrame, under the widgets, instead of over them at
// the present, and surface and stretch mode draw it in software over a copy of the frame (a MemoryImage of the screen's size,
// kept only while it is open) and show that copy whole; the app's buffer is not written. Nothing of the game updates while
// the menu is open, so the frame under it does not move. SoftwareScreenDisplay draws every widget into its screen itself.
// A drawn cursor image (mCursorImage) keeps the display in renderer mode.

#include "DDInterface.h"
#include "LinearStretch.h"

#include <SDL.h>

namespace Sexy
{

class GdiStretch;
class MemoryImage;
class Widget;

class SurfaceDisplay : public DDInterface
{
public:
	// whether stretch mode's smooth flavour shows the sizes the renderer would otherwise (SURFACE_STRETCH=3 or 0 in a test run
	// says): its picture is the renderer's at most sizes, a few seams one level off at the others (docs/MEMORY.md, "A smooth
	// stretch on the processor")
	static const bool kLinearStretchByDefault = true;

	SurfaceDisplay(SexyAppBase* theApp);
	~SurfaceDisplay() override;

	// The app's frame, once for every frame it draws (before the framework draws the widgets). thePixels: the screen,
	// mWidth x mHeight 32-bit pixels 0x??RRGGBB, thePitch bytes a row (the top byte is not read; the screen is opaque),
	// valid until the next call and until the display is gone. theRects: what changed since the last call (theCount 0:
	// nothing; theCount < 0: everything). The rects accumulate until the next Present: one inside another is dropped and,
	// past 128 of them, the two whose union adds the least area are made one.
	void SetFrame(const uint32_t* thePixels, int thePitch, const Rect* theRects, int theCount);

	// Lets the display show a window of a size that is no whole scale by GDI's stretch (stretch mode) instead of the renderer's. An app that
	// draws a cursor image of its own does not allow it. theWholeScalesOnly: the screen is shown at the largest whole scale that fits the window
	// (its pixels all the same size, bars around it) instead of filling the window as far as its aspect ratio allows. A test run's
	// SURFACE_STRETCH in the environment decides instead: 0 off, 1 filling, 2 whole scales, 3 the smooth stretch (below).
	void AllowStretch(bool theAllowed, bool theWholeScalesOnly = false);
	// true while stretch mode paints the renderer's smooth stretch computed on the processor (LinearStretch) rather than GDI's
	// nearest-neighbour one
	bool InLinearStretch() const { return mSurfaceMode && mStretching && mStretchLinear; }
	// true while the frame goes to the window's surface or is stretched into it (no renderer either way)
	bool InSurfaceMode() const { return mSurfaceMode; }
	bool InStretchMode() const { return mSurfaceMode && mStretching; }
	int mStretchPresents;
	// frames shown in each mode, for tests
	int mSurfacePresents;
	int mRendererPresents;
	// the changes of mode so far
	int mModeChanges;

	int Init(SDL_Window* theWindow, bool vsync) override;
	void Cleanup() override;
	void UpdateDisplaySize() override;
	void ReleaseTextures() override;
	void ScreenInvalidated() override { mPendingAll = true; }
	void BeginFrame() override;
	void Present() override;
	bool ReadScreen(ulong* theBits) override;
	bool ReadScreenFull(std::vector<uint32_t>& theBits, int& theWidth, int& theHeight) override;
	bool ReadWindow(std::vector<uint32_t>& theBits, int& theWidth, int& theHeight) override;

protected:
	// the scale (1 to 4) at which the window's drawable is exactly the logical size, or 0 when it is no such size or the
	// renderer is wanted anyway (SURFACE_DISPLAY=0 in the environment, so that a test can compare the two modes in one window);
	// SoftwareScreenDisplay asks for the renderer at 2x to 4x too, to learn whether it can draw on the GPU
	virtual int WantedSurfaceScale();
	// the renderer made again after a failed change to surface mode
	void RestoreRenderer();
	bool EnterSurfaceMode(int theScale);
	// the stretch mode (above): wanted when the app allows it and the window is of any size but a whole scale
	virtual bool WantsStretch();
	bool EnterStretchMode();
	void EnteredNoRendererMode();
	void LeaveSurfaceMode();
	// the rects the next present shows, in the buffer's pixels, in mSdlRects (the count; 0: nothing to show; theWhole: all of it)
	int PendingRects(bool* theWhole);
	void PresentToSurface();
	// surface mode at 2x to 4x with mGdiPresent (Windows)
	void PresentScaledByGdi();
	void PresentToStretch();
	static int MergeOverlapping(SDL_Rect* theRects, int theCount);
	void VerifyStretch(const SDL_Rect& theDest, int theWindowWidth, int theWindowHeight);
	void VerifySurface(SDL_Surface* theSurface);
	void PresentToRenderer();
	// the pending rects into mFrameTexture and mFrameTexture into the screen texture
	bool DrawFrameToScreenTexture();
	void ReleaseFrameTexture();
	void AddPending(const Rect& theRect);

	const uint32_t* mPixels;
	int mPitch;
	// the rects of the frame not shown yet; mPendingAll: all of it
	std::vector<Rect> mPending;
	size_t mMaxPending;  // the most kept; more are merged (AddPending)
	std::vector<SDL_Rect> mSdlRects;
	bool mPendingAll;
	// surface mode: the window's SDL surface is shown instead of a renderer's frame
	bool mSurfaceMode;
	int mSurfaceScale;         // the window's size in the buffer's: each pixel of the buffer is shown this many times each way
	bool mSurfaceUnavailable;  // the window has no surface the display can use: the renderer for good
	bool mSurfaceDisabled;     // SURFACE_DISPLAY=0 in the environment: the renderer for good
	bool mStretchAllowed;      // AllowStretch
	int mStretchHook;          // SURFACE_STRETCH in the environment of a test run: -1 not set, 0 off, 1 filling, 2 whole scales
	bool mStretchWholeScales;  // AllowStretch's second argument
	bool mStretching;          // surface mode's other flavour: the buffer is stretched into the window by GDI, not copied into its surface
	bool mLinearAllowed;       // stretch mode may paint the smooth stretch (the renderer's look) at sizes the renderer would show otherwise
	bool mStretchLinear;       // stretch mode paints the smooth stretch (mLinear) and not GDI's nearest-neighbour one
	LinearStretch mLinear;     // its tables, for the presentation rect's size
	bool mStretchUnavailable;  // the window has no GDI handle: no stretch mode
	GdiStretch* mGdi;  // Windows only (SurfaceDisplayGdi.h)
	// Windows, unless SURFACE_PRESENT=copy in a test run: surface mode at 2x to 4x has GDI stretch the buffer into the window
	// (mGdi) instead of copying it into a window surface of that size (above); false for good when the window has no handle
	bool mGdiPresent;
	// Windows, unless SURFACE_PACE=ms in a test run: each present without a renderer waits for DWM's next composition, in
	// place of the framework's pace (above); false for good once a wait fails or holds a frame over 250 ms (StopDwmPace)
	bool mDwmPace;
	void WaitForComposition();
	void StopDwmPace();
	// -presentlog for the modes without a renderer (DDInterface::LogPresent): theWay names the mode
	void LogNoRendererPresent(const char* theWay, Uint64 theStart);
	// a hardware renderer (Direct3D 11) has been made on the window: its swap chain stays the window's picture for good, and
	// the window never shows a GDI blit again, so the surface is not used any more (the renderer for good)
	bool mWindowHadRenderer;
	void NoteRendererOnWindow();
	bool mVerifySurface;       // SURFACE_VERIFY=1 in the environment: every present is checked (VerifySurface, VerifyStretch)
	bool mVerifyWindow;        // SURFACE_VERIFY=2: stretch mode also reads the window back and compares it
	// the framework's own widget over the frame (the mod menu; the comment at the top)
	Widget* OpenOverlay() const;
	const uint32_t* ComposeOverlay();
	bool mWidgetsInBuffer;     // the app's screen is what all of its widgets draw into (SoftwareScreenDisplay), the framework's too
	bool mFrameDrawnAtBegin;   // renderer mode: this frame's BeginFrame put the frame into the screen texture, under the widgets
	MemoryImage* mOverlayImage;  // surface and stretch mode: the frame with the overlay drawn over it, while it is open
	bool mOverlayShown;        // the window shows mOverlayImage, so the frame alone is shown whole once it closes
	int mVerifiedFrames;
	Uint32 mLastWholeTick;     // SDL_GetTicks() at the last present of the whole screen (surface mode)
	SDL_Texture* mFrameTexture;
	bool mFrameTextureWhole;  // mFrameTexture holds the whole of the buffer as of the last upload
};

}

#endif
