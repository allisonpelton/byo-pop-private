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
#include "TriangleWrap.h"

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
	bool mIs3D;  // true when the renderer is hardware accelerated and the game has not switched 3D off (SexyAppBase::Set3DAcclerated)
	bool mCan3D;  // port: the renderer is hardware accelerated (what mIs3D is before a game switches 3D off)
	bool mIsWindowed;
	bool mVSync;
	// port: the pace of the drawing on a renderer that no vertical blank holds (SDL's software renderer); see FrameHeldByPace
	double mPaceIntervalMs;   // the display's refresh interval, 0 when the renderer is held by vsync (no pace)
	double mPaceNextMs;       // the earliest time of the next frame, in ms from mPaceOriginMs
	unsigned int mPaceOriginMs;
	bool mPaceStarted;
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
	// Texture addressing of DrawTrianglesTex's u and v (TEXTURE_ADDRESS_CLAMP, the default, or _WRAP: the picture repeats where a
	// coordinate leaves 0..1), as Direct3D's stage state 0xd and 0xe; kept until set again (framework/TriangleWrap.h)
	int mTextureAddressU;
	int mTextureAddressV;
	void SetTextureAddress(int theU, int theV) { mTextureAddressU = theU; mTextureAddressV = theV; }

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

	// The software stretch (off by default; a game whose original drew its screen in software sets it, as Bejeweled
	// Twist does in 2D). The first piece of the framework's software 2D path: it covers StretchBlt only. The original's
	// software screen (Twist's DDImage::StretchBlt, 0x41a3e0) stretches a non-fast draw into a temporary image of the
	// destination's size by averaging (MemoryImage::StretchCopy, 0x433ca0) and blits that with the draw's colour, mode
	// and clip; a fast one it samples straight onto the screen, texel (srcX * 65536 + i * step) >> 16 with step
	// srcWidth * 65536 / destWidth, from the corner where SDL's nearest filter samples pixel centres. Here both become
	// an image of the destination's size, cached and drawn unscaled. Mirrored stretches, and a stretch that is not a
	// MemoryImage, keep SDL's filtering; so do rotations, transforms and every other draw. A clipped stretch is
	// stretched whole and clipped as it is drawn, where the original stretched only the clipped part (the same pixels
	// but for rounding at the clip's edge). A fast stretch whose source rectangle is not inside the image keeps SDL's. A
	// destination of fractional width or height is cut to whole pixels. A copy is remade only when its source's
	// mBitsChangedCount changes, so a source whose pixels change without BitsChanged keeps an old copy.
	bool mSoftwareStretch;

	// With mSoftwareStretch: a fast stretch steps its column before it samples (texel (srcX * 65536 + (i + 1) * step) >> 16),
	// as 1.5's DDImage::StretchBlt does (AstroPop's 0x4c68c0; Bejeweled Twist's 0x41a3e0 samples first). Off by default.
	bool mFastStretchPreAdd;

	// The software transforms (off by default; set with mSoftwareStretch by a game whose original drew its screen in
	// software, as Bejeweled Twist does in 2D). The second piece of the software 2D path: DDImage's BltRotated,
	// BltMatrix and BltTrianglesTex on the screen draw with MemoryImage's software code (the bilinear rotation of
	// BltRotatedHelper.inc, SWTri's triangles) into an image that covers the draw's bounds within its clip, and that
	// image is blitted onto the screen, as the original's software screen drew them straight onto its surface (Twist's
	// DDImage::BltRotated, 0x413150; its BltMatrix, 0x4355e0). BltF, a blit at a fractional place, is BltRotated at no
	// angle, as on a software screen (Twist's 0x412fd0). A normal draw goes into a transparent image whose alpha then
	// blends it, which gives the original's colours to within one step; an additive rotation goes into an opaque black
	// image that is then added. Additive matrix and triangle draws keep SDL's: SWTri here has no additive triangles
	// (Twist's later SWTri has them, 0x60d250's flag 0x80), and neither do a mirrored draw or a source that is not a
	// MemoryImage. SWTri draws only once the game has registered its drawers (SWTri_AddAllDrawTriFuncs, as 1.3's games
	// do). The images are kept by size (GetTransformCopy); with them a frame of Twist costs about 2 ms more.
	bool mSoftwareTransforms;

	// Whether the screen keeps what the last frame drew (off by default: every frame starts from black). The original's software
	// screen is one surface that persists from frame to frame, so a widget that draws nothing in a frame leaves what was there
	// before (Bejeweled 2's 2D whirlpool opens over the main menu's last picture, which neither the menu nor the board draws any
	// more). A game that draws its whole screen every frame gains nothing from it.
	bool mKeepScreen;

	// An image of at least theWidth x theHeight for the software transforms, its size rounded up to 16 pixels; the
	// caller fills and draws only its top-left theWidth x theHeight. At most kTransformCacheSize are kept, the least
	// recently used going first, so a texture is not made for every draw.
	MemoryImage* GetTransformCopy(int theWidth, int theHeight);
	// A stretched copy of theSrcRect at theWidth x theHeight (the software stretch, below); DDImage draws a stretch into
	// a picture of its own through it too
	MemoryImage* GetStretchCopy(MemoryImage* theSource, const Rect& theSrcRect, int theWidth, int theHeight, bool fast);

protected:
	// The stretched copies: at most kStretchCacheSize and kCopyCacheBytes of pixels (the transform copies have their own
	// kCopyCacheBytes; each copy's texture takes as much again), the least recently drawn going first. An entry holds its source's
	// mBitsChangedCount and is remade when that changes; RemoveImageData drops a source's entries, so a deleted image
	// whose address is reused never finds an old copy.
	struct StretchCopyEntry
	{
		MemoryImage* mSource;
		Rect mSrcRect;
		int mWidth, mHeight;
		bool mFast;
		int mSourceVersion;
		MemoryImage* mCopy;
		unsigned int mLastUse;
	};
	enum { kStretchCacheSize = 64 };
	enum { kTransformCacheSize = 32 };
	enum { kCopyCacheBytes = 64 * 1024 * 1024 };
	struct TransformCopyEntry
	{
		MemoryImage* mCopy;
		unsigned int mLastUse;
	};
	std::vector<TransformCopyEntry> mTransformCopies;
	std::vector<StretchCopyEntry> mStretchCopies;
	unsigned int mStretchUse;
	void ClearStretchCopies();

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
	// port: SDL's software renderer is held by no vertical blank, so the loop draws as often as it has time: every update (100 a
	// second in Bejeweled 2), far more than a display shows. True when a frame drawn at theNowMs (SexyAppBase's millisecond clock) would
	// come sooner than the display's refresh rate allows; false when it is due, and the next one is set one interval on (or from now,
	// when the frames take longer than that). A renderer held by vsync (a GPU's) is never held here.
	bool FrameHeldByPace(unsigned int theNowMs);

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
	// What a display's DrawTrianglesTex starts with: with a wrap set, cuts the triangles at whole-number u and v and draws the
	// pieces through DrawTrianglesTex clamped, and says true; with clamp both ways it does nothing and says false.
	bool DrawTrianglesTexWrapped(Image* theTexture, const TriVertex theVertices[][3], int theNumTriangles, const Rect* theClipRect, const Color& theColor, int theDrawMode, float tx, float ty, bool blend);
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
