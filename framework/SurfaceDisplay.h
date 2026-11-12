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
//   thread or its memory); Present copies the changed rects, row by row, into the window's SDL surface, each pixel repeated
//   as often as the window is larger, and has SDL update those rects of the window (a GDI blit). At those sizes the renderer
//   shows the same picture (its screen texture at the render scale is those repeated pixels, shown as it is), so the choice
//   is not seen.
//
//   Renderer mode, any other size (a full-screen window is the desktop's size): DDInterface's own path (the screen texture at the render scale,
//   scaled into the window, vsync), with the frame put into the screen texture from a texture of its own of which
//   only the changed rect is uploaded each frame, so that nothing but the changed rect is copied and no image of the app
//   is kept up to date besides the buffer.
//
// An SDL window cannot have a renderer and a window surface at once, so a change of mode (UpdateDisplaySize, which
// runs at every size change and at the start of every frame) destroys the one and makes the other. The window, its
// focus and its position are untouched; the buffer is the app's and nothing in it is lost, so the first frame after a
// change is shown whole.

#include "DDInterface.h"

struct SDL_Surface;

namespace Sexy
{

class SurfaceDisplay : public DDInterface
{
public:
	SurfaceDisplay(SexyAppBase* theApp);
	~SurfaceDisplay() override;

	// The app's frame, once for every frame it draws (before the framework draws the widgets). thePixels: the screen,
	// mWidth x mHeight 32-bit pixels 0x??RRGGBB, thePitch bytes a row (the top byte is not read; the screen is opaque),
	// valid until the next call and until the display is gone. theRects: what changed since the last call (theCount 0:
	// nothing; theCount < 0: everything). The rects accumulate until the next Present.
	void SetFrame(const uint32_t* thePixels, int thePitch, const Rect* theRects, int theCount);

	// true while the frame goes to the window's surface
	bool InSurfaceMode() const { return mSurfaceMode; }
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

protected:
	// the scale (1 to 4) at which the window's drawable is exactly the logical size, or 0 when it is no such size or the
	// renderer is wanted anyway (SURFACE_DISPLAY=0 in the environment, so that a test can compare the two modes in one window)
	int WantedSurfaceScale();
	// the renderer made again after a failed change to surface mode
	void RestoreRenderer();
	bool EnterSurfaceMode(int theScale);
	void LeaveSurfaceMode();
	void PresentToSurface();
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
	bool mPendingAll;
	// surface mode: the window's SDL surface is shown instead of a renderer's frame
	bool mSurfaceMode;
	int mSurfaceScale;         // the window's size in the buffer's: each pixel of the buffer is shown this many times each way
	bool mSurfaceUnavailable;  // the window has no surface the display can use: the renderer for good
	bool mSurfaceDisabled;     // SURFACE_DISPLAY=0 in the environment: the renderer for good
	bool mVerifySurface;       // SURFACE_VERIFY=1 in the environment: every present is checked (VerifySurface)
	int mVerifiedFrames;
	SDL_Texture* mFrameTexture;
	bool mFrameTextureWhole;  // mFrameTexture holds the whole of the buffer as of the last upload
};

}

#endif
