#ifndef __DDINTERFACE_H__
#define __DDINTERFACE_H__

// Portable replacement for the DirectDraw/Direct3D display layer, built on SDL2's renderer
// (Direct3D/OpenGL natively, WebGL on Emscripten).
//
// Coordinates are always logical (640x480 for Bejeweled). The window/canvas may be any size: the
// logical screen is letterboxed and scaled, and textures are sampled at their native resolution, so
// HD replacement images (see DDImage::mHDBits) render crisply.
//
// Every frame is drawn into mScreenTexture, a render target of (logical size * mRenderScale) pixels,
// which Present() scales into mPresentationRect of the window's drawable. The logical size, the drawable's size and
// mPresentationRect are DisplayGeometry's, which also maps window coordinates and places the cursor.
//
// Only the screen image draws through here; offscreen MemoryImages keep using the framework's
// software blitters. Source images are uploaded as premultiplied-alpha textures cached on
// MemoryImage::mD3DData and refreshed when MemoryImage::mBitsChangedCount changes.

#include "Common.h"
#include "NativeDisplay.h"
#include "DisplayGeometry.h"
#include "Rect.h"
#include "Color.h"

struct SDL_Window;
struct SDL_Renderer;
struct SDL_Texture;

namespace Sexy
{

class SexyAppBase;
class DDImage;
class Image;
class MemoryImage;
class SexyMatrix3;
class TriVertex;
struct DDTextureData;

class DDInterface : public NativeDisplay, public DisplayGeometry
{
public:
	enum
	{
		RESULT_OK = 0,
		RESULT_FAIL = 1,
		RESULT_DD_CREATE_FAIL = 2,
		RESULT_SURFACE_FAIL = 3,
		RESULT_EXCLUSIVE_FAIL = 4,
		RESULT_DISPCHANGE_FAIL = 5,
		RESULT_INVALID_COLORDEPTH = 6,
		RESULT_3D_FAIL = 7
	};

	SexyAppBase* mApp;
	SDL_Window* mWindow;
	SDL_Renderer* mRenderer;
	DDImage* mScreenImage;
	bool mIs3D;  // true when the renderer is hardware accelerated
	bool mIsWindowed;
	bool mVSync;
	bool mPixelPerfect;  // nearest filtering for unscaled 1x blits when the screen is scaled (default true)
	int mFrameCount;

	// cursor
	Image* mCursorImage;
	int mCursorX;
	int mCursorY;
	bool mCursorVisible;

	// The screen texture everything draws into (logical resolution * mRenderScale).
	SDL_Texture* mScreenTexture;
	int mRenderScale;
	int mForcedRenderScale;  // test hook (-renderscale=N): render at N times the logical size whatever the window size

	// Blend modes (SDL_BlendMode values). With mPremultiplied, textures and vertex colours are
	// premultiplied and the modes are custom (ONE, ONE_MINUS_SRC_ALPHA) / (ONE, ONE); renderers without
	// custom blend modes (software) fall back to straight alpha with SDL_BLENDMODE_BLEND / _ADD.
	int mBlendNormal;
	int mBlendAdditive;
	bool mPremultiplied;
	int mMaxTextureWidth;
	int mMaxTextureHeight;

	std::set<MemoryImage*> mTextureImageSet;  // images whose mD3DData holds a texture

	// current clip rectangle on the screen texture (logical coordinates)
	bool mClipEnabled;
	Rect mCurClipRect;

protected:
	void ReleaseRenderer();
	bool CreateScreenTexture(int theRenderScale);
	void SetupBlendModes();
	bool PreDraw();
	bool SetClip(const Rect* theClipRect);
	DDTextureData* GetTextureData(MemoryImage* theImage);
	SDL_Texture* PrepareTexture(MemoryImage* theImage, DDTextureData* theData, int theDrawMode, bool linearHint);
	void DrawTexturedQuad(Image* theImage, const float theX[4], const float theY[4], const Rect& theSrcRect, bool mirror, const Color& theColor, int theDrawMode, const Rect* theClipRect, bool linearHint);
	void FillRects(const Color& theColor, int theDrawMode);
	void DrawCursor();

public:
	DDInterface(SexyAppBase* theApp);
	virtual ~DDInterface();

	// The methods SexyAppBase, DDImage and the games call are virtual, so that another display (GPUDisplay, for games
	// that draw through framework/GPUInterface.h) can stand in for this one; SexyAppBase::CreateDDInterface makes it.

	// Creates the renderer for an existing window. Returns RESULT_OK on success. Calling it again
	// recreates the renderer (all textures are rebuilt on demand; mScreenImage is kept).
	virtual int Init(SDL_Window* theWindow, bool vsync);
	virtual void Cleanup();
	virtual void UpdateDisplaySize();

	// Frees every cached texture and the screen texture; they are recreated on demand. Call on
	// SDL_RENDER_DEVICE_RESET.
	virtual void ReleaseTextures();

	// Frame
	virtual void BeginFrame();
	virtual void Present();

	// Converts window coordinates (as reported by SDL mouse events) to logical screen coordinates.
	virtual void WindowToLogical(int theWindowX, int theWindowY, int* theX, int* theY);
	// port: the inverse, a game point to window coordinates (SexyAppBase::WarpMouse)
	virtual void LogicalToWindow(int theX, int theY, int* theWindowX, int* theWindowY);

	// Texture management
	SDL_Texture* GetTexture(MemoryImage* theImage, int* theTexWidth, int* theTexHeight);
	virtual void RemoveImageData(MemoryImage* theImage);
	// DDImage::Check3D's answer for a destination: whether a draw onto theImage goes through this display (after
	// binding it) rather than the software blitters. Here only the screen does.
	virtual bool CheckDestination(DDImage* theImage);

	// GPU drawing onto the screen (logical coordinates). theClipRect may be NULL.
	virtual void FillRect(const Rect& theRect, const Color& theColor, int theDrawMode, const Rect* theClipRect);
	virtual void FillPoly(const Point theVertices[], int theNumVertices, const Rect* theClipRect, const Color& theColor, int theDrawMode, int tx, int ty);
	virtual void DrawLine(double x1, double y1, double x2, double y2, const Color& theColor, int theDrawMode, bool antiAlias);
	virtual void FillScanLines(const struct Span* theSpans, int theSpanCount, const Color& theColor, int theDrawMode);
	virtual void Blt(Image* theImage, float theX, float theY, const Rect& theSrcRect, const Color& theColor, int theDrawMode, const Rect* theClipRect, bool mirror = false);
	virtual void StretchBlt(Image* theImage, const FRect& theDestRect, const Rect& theSrcRect, const Rect* theClipRect, const Color& theColor, int theDrawMode, bool fastStretch, bool mirror = false);
	virtual void BltRotated(Image* theImage, float theX, float theY, const Rect& theSrcRect, const Rect* theClipRect, const Color& theColor, int theDrawMode, double theRot, float theRotCenterX, float theRotCenterY);
	virtual void BltTransformed(Image* theImage, const Rect* theClipRect, const Color& theColor, int theDrawMode, const Rect& theSrcRect, const SexyMatrix3& theTransform, bool linearFilter, float theX, float theY, bool center);
	virtual void DrawTrianglesTex(Image* theTexture, const TriVertex theVertices[][3], int theNumTriangles, const Rect* theClipRect, const Color& theColor, int theDrawMode, float tx, float ty, bool blend);
	virtual void ClearRect(const Rect& theRect);

	// Reads the logical screen back into ARGB pixels (used for screenshots / GetBits on the screen).
	virtual bool ReadScreen(ulong* theBits);
	// Reads the render target at full resolution (logical size * mRenderScale), ARGB.
	virtual bool ReadScreenFull(std::vector<uint32_t>& theBits, int& theWidth, int& theHeight);

	virtual void SetCursorImage(Image* theImage);
	virtual void SetCursorPos(int theX, int theY);
};

}

#endif
