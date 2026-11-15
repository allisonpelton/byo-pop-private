#ifndef __EMSCRIPTEN__

#include "SurfaceDisplay.h"
#include "DDImage.h"
#include "SexyAppBase.h"
#include "SurfaceDisplayGdi.h"

#include <SDL.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace Sexy
{

// the most rects kept between two presents (SURFACE_MAXRECTS in the environment sets another number, so that a test can
// make the rects merge often); past it the two rects whose union adds the least area become that union
static const size_t kDefaultMaxPendingRects = 128;

static bool RectContains(const Rect& theOuter, const Rect& theInner)
{
	return (theInner.mX >= theOuter.mX) && (theInner.mY >= theOuter.mY) && (theInner.mX + theInner.mWidth <= theOuter.mX + theOuter.mWidth) &&
		(theInner.mY + theInner.mHeight <= theOuter.mY + theOuter.mHeight);
}

// the window's surface goes up to this many times the screen's size
static const int kMaxSurfaceScale = 4;

// the screen's pixels as opaque 0xffRRGGBB, each repeated theScale times each way (what the renderer's screen texture holds
// at the render scale theScale)
template<class T> static void CopyOpaque(const uint8_t* thePixels, int thePitch, int theWidth, int theHeight, T* theBits, int theScale = 1)
{
	const int aRowWidth = theWidth * theScale;
	for (int y = 0; y < theHeight; y++)
	{
		const uint32_t* aSrc = (const uint32_t*) (thePixels + y * thePitch);
		T* aRow = theBits + (size_t) y * theScale * aRowWidth;
		for (int x = 0; x < theWidth; x++)
			for (int i = 0; i < theScale; i++)
				aRow[x * theScale + i] = 0xFF000000 | (aSrc[x] & 0xFFFFFF);
		for (int j = 1; j < theScale; j++)
			memcpy(aRow + (size_t) j * aRowWidth, aRow, (size_t) aRowWidth * sizeof(T));
	}
}

// a rect of 32-bit pixels, each repeated theScale times each way (theScale 1: a plain copy of the rows)
static void CopyRectScaled(const uint8_t* theSrc, int theSrcPitch, uint8_t* theDst, int theDstPitch, int theWidth, int theHeight, int theScale)
{
	for (int y = 0; y < theHeight; y++)
	{
		const uint32_t* aSrc = (const uint32_t*) (theSrc + (size_t) y * theSrcPitch);
		uint8_t* aRow = theDst + (size_t) y * theScale * theDstPitch;
		if (theScale == 1)
			memcpy(aRow, aSrc, (size_t) theWidth * 4);
		else if (theScale == 2)
		{
			uint32_t* aDst = (uint32_t*) aRow;
			for (int x = 0; x < theWidth; x++)
			{
				const uint32_t aPixel = aSrc[x];
				aDst[2 * x] = aPixel;
				aDst[2 * x + 1] = aPixel;
			}
		}
		else
		{
			uint32_t* aDst = (uint32_t*) aRow;
			for (int x = 0; x < theWidth; x++)
				for (int i = 0; i < theScale; i++)
					aDst[x * theScale + i] = aSrc[x];
		}
		for (int j = 1; j < theScale; j++)
			memcpy(aRow + (size_t) j * theDstPitch, aRow, (size_t) theWidth * theScale * 4);
	}
}

SurfaceDisplay::SurfaceDisplay(SexyAppBase* theApp) :
	DDInterface(theApp)
{
	mSurfacePresents = 0;
	mRendererPresents = 0;
	mModeChanges = 0;
	mPixels = NULL;
	mPitch = 0;
	mPendingAll = true;
	mSurfaceMode = false;
	mSurfaceScale = 1;
	mVerifiedFrames = 0;
	mLastWholeTick = 0;
	mSurfaceUnavailable = false;
	mWindowHadRenderer = false;
	mSurfaceDisabled = false;
	mStretchAllowed = false;
	mStretchHook = -1;
	mStretchWholeScales = false;
	mStretching = false;
	mStretchUnavailable = false;
	mGdi = NULL;
	mStretchPresents = 0;
	mVerifySurface = false;
	mVerifyWindow = false;
	mFrameTexture = NULL;
	mFrameTextureWhole = false;
	mMaxPending = kDefaultMaxPendingRects;
	// test hooks: only a test run (SexyAppBase::IsTestRun) reads them, so a player's exe never meets them
	if ((theApp != NULL) && (theApp->IsTestRun()))
	{
		const char* anEnv = SDL_getenv("SURFACE_DISPLAY");
		mSurfaceDisabled = (anEnv != NULL) && (anEnv[0] == '0');
		anEnv = SDL_getenv("SURFACE_STRETCH");
		if ((anEnv != NULL) && ((anEnv[0] == '0') || (anEnv[0] == '1') || (anEnv[0] == '2')))
			mStretchHook = anEnv[0] - '0';
		anEnv = SDL_getenv("SURFACE_VERIFY");
		mVerifySurface = (anEnv != NULL) && ((anEnv[0] == '1') || (anEnv[0] == '2'));
		mVerifyWindow = (anEnv != NULL) && (anEnv[0] == '2');
		anEnv = SDL_getenv("SURFACE_MAXRECTS");
		if ((anEnv != NULL) && (atoi(anEnv) > 0))
			mMaxPending = (size_t) atoi(anEnv);
	}
}

SurfaceDisplay::~SurfaceDisplay()
{
	// the frame texture must go before the renderer does (DDInterface's destructor can only call its own Cleanup)
	Cleanup();
#ifdef _WIN32
	delete mGdi;
	mGdi = NULL;
#endif
}

void SurfaceDisplay::AllowStretch(bool theAllowed, bool theWholeScalesOnly)
{
	// a test run's environment decides (so that one exe can be tried each way), whatever the app says
	mStretchAllowed = (mStretchHook >= 0) ? (mStretchHook >= 1) : theAllowed;
	mStretchWholeScales = (mStretchHook >= 0) ? (mStretchHook == 2) : theWholeScalesOnly;
}

///////////////////////////////////////////////////////////////////////////////
// The mode

int SurfaceDisplay::WantedSurfaceScale()
{
	if (mWindow == NULL)
		return 0;
	if ((mSurfaceDisabled) || (mSurfaceUnavailable) || (mWindowHadRenderer))
		return 0;
	// a drawn cursor image is the renderer's to draw
	if (mCursorImage != NULL)
		return 0;
	// the size decides, not whether the window is full screen: a full-screen window the size of the screen is 1:1 as well
	int aWidth = 0, aHeight = 0;
	SDL_GetWindowSizeInPixels(mWindow, &aWidth, &aHeight);
	for (int aScale = 1; aScale <= kMaxSurfaceScale; aScale++)
		if ((aWidth == mWidth * aScale) && (aHeight == mHeight * aScale))
			return aScale;
	return 0;
}

bool SurfaceDisplay::EnterSurfaceMode(int theScale)
{
	// SDL does not give a window a surface while it has a renderer
	ReleaseRenderer();
	// the surface is the CPU's, shown by GDI: never one that SDL makes of a texture behind our backs
	SDL_SetHint(SDL_HINT_FRAMEBUFFER_ACCELERATION, "0");
	SDL_Surface* aSurface = SDL_GetWindowSurface(mWindow);
	if ((aSurface == NULL) || (aSurface->format->BytesPerPixel != 4) || (aSurface->format->Rmask != 0xff0000) ||
		(aSurface->format->Gmask != 0xff00) || (aSurface->format->Bmask != 0xff) || (aSurface->w != mWidth * theScale) ||
		(aSurface->h != mHeight * theScale))
	{
		SDL_Log("SurfaceDisplay: the window has no 32-bit surface of %dx%d (%s)", mWidth * theScale, mHeight * theScale,
			(aSurface == NULL) ? SDL_GetError() : "another format or size");
		if (aSurface != NULL)
			SDL_DestroyWindowSurface(mWindow);
		mSurfaceUnavailable = true;
		return false;
	}

	mStretching = false;
	mSurfaceScale = theScale;
	SDL_Log("SurfaceDisplay: the window's surface (%dx%d, the screen at %dx)", mWidth * theScale, mHeight * theScale, theScale);
	mRenderScale = theScale;
	EnteredNoRendererMode();
	return true;
}

// what both modes without a renderer share, once their window is ready
void SurfaceDisplay::EnteredNoRendererMode()
{
	if (mScreenImage == NULL)
	{
		mScreenImage = new DDImage(this);
		mScreenImage->mIsScreen = true;
	}
	if ((mScreenImage->mWidth != mWidth) || (mScreenImage->mHeight != mHeight))
		mScreenImage->Create(mWidth, mHeight);

	mSurfaceMode = true;
	// as GPUDisplay: no SDL renderer, so the screen image is read back through ReadScreen, and a window that is
	// minimised draws nothing (DrawDirtyStuff)
	mIs3D = true;
	mCan3D = true;
	mPendingAll = true;
	mModeChanges++;
	// no vertical blank holds the window's surface: its frames are paced at the display's refresh rate by the framework's
	// own pace (FrameHeldByPace, which SexyAppBase::Process asks before it draws; not under -fast, -drawevery or -hold)
	SDL_DisplayMode aMode;
	int aDisplay = SDL_GetWindowDisplayIndex(mWindow);
	int aRate = ((aDisplay >= 0) && (SDL_GetCurrentDisplayMode(aDisplay, &aMode) == 0)) ? aMode.refresh_rate : 0;
	mPaceIntervalMs = 1000.0 / ((aRate > 0) ? aRate : 60);
	mPaceStarted = false;
}

bool SurfaceDisplay::WantsStretch()
{
#ifdef _WIN32
	if ((!mStretchAllowed) || (mWindow == NULL) || (mSurfaceDisabled) || (mStretchUnavailable) || (mWindowHadRenderer))
		return false;
	// a drawn cursor image is the renderer's to draw
	if (mCursorImage != NULL)
		return false;
	int aWidth = 0, aHeight = 0;
	SDL_GetWindowSizeInPixels(mWindow, &aWidth, &aHeight);
	return (aWidth > 0) && (aHeight > 0);
#else
	return false;
#endif
}

bool SurfaceDisplay::EnterStretchMode()
{
#ifdef _WIN32
	// the window shows what GDI paints into it: no renderer, and not the SDL surface of an earlier mode
	ReleaseRenderer();
	SDL_DestroyWindowSurface(mWindow);
	if (mGdi == NULL)
		mGdi = new GdiStretch();
	if (!mGdi->Attach(mWindow, mVerifySurface))
	{
		SDL_Log("SurfaceDisplay: the window has no handle for GDI (%s)", SDL_GetError());
		mStretchUnavailable = true;
		return false;
	}
	mStretching = true;
	int aWidth = 0, aHeight = 0;
	SDL_GetWindowSizeInPixels(mWindow, &aWidth, &aHeight);
	SDL_Log("SurfaceDisplay: the window's GDI stretch (the screen of %dx%d into the window of %dx%d)", mWidth, mHeight, aWidth, aHeight);
	// the render scale is the renderer's (the screen read back at the scale the window needs), set by UpdateDisplaySize
	EnteredNoRendererMode();
	return true;
#else
	return false;
#endif
}

void SurfaceDisplay::RestoreRenderer()
{
	mSurfaceMode = false;
	mStretching = false;
	mPaceIntervalMs = 0;  // DDInterface::Init sets it again from the renderer it makes
	mPaceStarted = false;
	SDL_DestroyWindowSurface(mWindow);
	int aResult = DDInterface::Init(mWindow, mVSync);
	if (aResult != RESULT_OK)
		SDL_Log("SurfaceDisplay: the renderer could not be made again (%d)", aResult);
	else
	{
		NoteRendererOnWindow();
		int aWidth = 0, aHeight = 0;
		SDL_GetWindowSizeInPixels(mWindow, &aWidth, &aHeight);
		SDL_Log("SurfaceDisplay: the renderer (the window is %dx%d, %s)", aWidth, aHeight,
			((SDL_GetWindowFlags(mWindow) & SDL_WINDOW_FULLSCREEN) != 0) ? "full screen" : "windowed");
	}
	mPendingAll = true;
	mFrameTextureWhole = false;
	mModeChanges++;
}

void SurfaceDisplay::LeaveSurfaceMode()
{
	RestoreRenderer();
}

void SurfaceDisplay::NoteRendererOnWindow()
{
	// Found with a probe on the visible desktop (a window made, a Direct3D 11 renderer on it, the renderer destroyed, the
	// window's surface filled and updated): the window goes on showing the renderer's last frame, whatever the surface holds,
	// through hiding and showing, minimising, resizing and a full-screen round trip; Direct3D 9's does not do this. So a
	// window that has had a hardware renderer (a full-screen start, or a resize to a size that is no whole scale) stays in
	// renderer mode, and only a window that has never had one (the 1x and 2x windows that start in surface mode) uses the surface.
	SDL_RendererInfo anInfo;
	if ((mRenderer != NULL) && (SDL_GetRendererInfo(mRenderer, &anInfo) == 0) && ((anInfo.flags & SDL_RENDERER_SOFTWARE) == 0) &&
		(!mWindowHadRenderer))
	{
		mWindowHadRenderer = true;
		SDL_Log("SurfaceDisplay: the window has had the %s renderer, so it stays in renderer mode (its last frame would stay on the window)",
			anInfo.name);
	}
}

int SurfaceDisplay::Init(SDL_Window* theWindow, bool vsync)
{
	ReleaseRenderer();
	mWindow = theWindow;
	mVSync = vsync;
	if (mApp != NULL)
	{
		mWidth = mApp->mWidth;
		mHeight = mApp->mHeight;
	}
	if ((mWindow == NULL) || (mWidth <= 0) || (mHeight <= 0))
		return RESULT_FAIL;

	mSurfaceMode = false;
	int aWantedScale = WantedSurfaceScale();
	if ((aWantedScale > 0) && (EnterSurfaceMode(aWantedScale)))
	{
		UpdateDisplaySize();
		return RESULT_OK;
	}
	if ((aWantedScale == 0) && (WantsStretch()) && (EnterStretchMode()))
	{
		UpdateDisplaySize();
		return RESULT_OK;
	}
	int aResult = DDInterface::Init(theWindow, vsync);
	if (aResult == RESULT_OK)
	{
		NoteRendererOnWindow();
		int aWidth = 0, aHeight = 0;
		SDL_GetWindowSizeInPixels(mWindow, &aWidth, &aHeight);
		int aPointsWidth = 0, aPointsHeight = 0, aOutWidth = 0, aOutHeight = 0;
		SDL_GetWindowSize(mWindow, &aPointsWidth, &aPointsHeight);
		if (mRenderer != NULL)
			SDL_GetRendererOutputSize(mRenderer, &aOutWidth, &aOutHeight);
		SDL_Log("SurfaceDisplay: the renderer (the window is %dx%d, %s; %dx%d points, output %dx%d)", aWidth, aHeight,
			((SDL_GetWindowFlags(mWindow) & SDL_WINDOW_FULLSCREEN) != 0) ? "full screen" : "windowed", aPointsWidth, aPointsHeight,
			aOutWidth, aOutHeight);
	}
	return aResult;
}

void SurfaceDisplay::Cleanup()
{
	if (mSurfacePresents + mRendererPresents + mStretchPresents > 0)
		SDL_Log("SurfaceDisplay: %d frames through the surface, %d stretched by GDI, %d through the renderer, %d changes of mode",
			mSurfacePresents, mStretchPresents, mRendererPresents, mModeChanges);
	if (mVerifiedFrames > 0)
		SDL_Log("SurfaceDisplay: %d frames of the surface compared with the screen, none differing", mVerifiedFrames);
	mVerifiedFrames = 0;
	mSurfacePresents = 0;
	mStretchPresents = 0;
	mRendererPresents = 0;
	ReleaseFrameTexture();
	mPixels = NULL;
	mSurfaceMode = false;
	DDInterface::Cleanup();
}

void SurfaceDisplay::UpdateDisplaySize()
{
	if (mWindow == NULL)
		return;

	// the window at 1x to 4x of the screen's size shows it through the window's surface, each pixel repeated scale times each way:
	// the same picture as the renderer's (its screen texture is those repeated pixels, and a window of that size shows the
	// texture as it is)
	// any other size is stretched by GDI when the app allows it (also no renderer), and is the renderer's otherwise
	int aWantedScale = WantedSurfaceScale();
	const bool aWantStretch = (aWantedScale == 0) && (WantsStretch());
	if ((aWantedScale > 0) && ((!mSurfaceMode) || (mStretching) || (aWantedScale != mSurfaceScale)))
	{
		if (!EnterSurfaceMode(aWantedScale))
			RestoreRenderer();
	}
	else if ((aWantStretch) && ((!mSurfaceMode) || (!mStretching)))
	{
		if (!EnterStretchMode())
			RestoreRenderer();
	}
	else if ((aWantedScale == 0) && (!aWantStretch) && (mSurfaceMode))
		LeaveSurfaceMode();

	if (mSurfaceMode)
	{
		int aWidth = 0, aHeight = 0;
		SDL_GetWindowSizeInPixels(mWindow, &aWidth, &aHeight);
		if (mStretching)
		{
			// the render scale as the renderer would have it (the window's scale rounded up, at most 4), which is the scale the screen
			// is read back at; no texture limit to lower it
			mRenderScale = DisplayGeometry::UpdateDisplaySize(aWidth, aHeight, 16384, 16384, 0);
			if ((mStretchWholeScales) && (aWidth >= mWidth) && (aHeight >= mHeight))
			{
				// the largest whole scale that fits, in the middle (the mouse maps through the same rect)
				const int aScale = std::min(aWidth / mWidth, aHeight / mHeight);
				mPresentationRect = Rect((aWidth - mWidth * aScale) / 2, (aHeight - mHeight * aScale) / 2, mWidth * aScale, mHeight * aScale);
				mRenderScale = std::min(aScale, kMaxSurfaceScale);
			}
		}
		else
		{
			DisplayGeometry::UpdateDisplaySize(aWidth, aHeight, mMaxTextureWidth, mMaxTextureHeight, 0);
			mRenderScale = mSurfaceScale;
		}
	}
	else
		DDInterface::UpdateDisplaySize();
}

void SurfaceDisplay::ReleaseFrameTexture()
{
	if (mFrameTexture != NULL)
	{
		SDL_DestroyTexture(mFrameTexture);
		mFrameTexture = NULL;
	}
	mFrameTextureWhole = false;
}

void SurfaceDisplay::ReleaseTextures()
{
	ReleaseFrameTexture();
	DDInterface::ReleaseTextures();
}

///////////////////////////////////////////////////////////////////////////////
// The frame

void SurfaceDisplay::AddPending(const Rect& theRect)
{
	if (mPendingAll)
		return;
	Rect aRect = theRect.Intersection(Rect(0, 0, mWidth, mHeight));
	if ((aRect.mWidth <= 0) || (aRect.mHeight <= 0))
		return;
	// a rect that is inside a pending one adds nothing; pending ones inside this one go
	for (size_t i = 0; i < mPending.size(); i++)
		if (RectContains(mPending[i], aRect))
			return;
	for (size_t i = 0; i < mPending.size();)
	{
		if (RectContains(aRect, mPending[i]))
		{
			mPending[i] = mPending.back();
			mPending.pop_back();
		}
		else
			i++;
	}
	mPending.push_back(aRect);
	while (mPending.size() > mMaxPending)
	{
		// the pair whose union is least larger than the two rects themselves
		size_t aBestA = 0, aBestB = 1;
		long long aBestCost = -1;
		for (size_t a = 0; a < mPending.size(); a++)
		{
			const long long aAreaA = (long long) mPending[a].mWidth * mPending[a].mHeight;
			for (size_t b = a + 1; b < mPending.size(); b++)
			{
				Rect aUnion = mPending[a].Union(mPending[b]);
				long long aCost = (long long) aUnion.mWidth * aUnion.mHeight - aAreaA - (long long) mPending[b].mWidth * mPending[b].mHeight;
				if ((aBestCost < 0) || (aCost < aBestCost))
				{
					aBestCost = aCost;
					aBestA = a;
					aBestB = b;
				}
			}
		}
		mPending[aBestA] = mPending[aBestA].Union(mPending[aBestB]);
		mPending[aBestB] = mPending.back();
		mPending.pop_back();
	}
}

void SurfaceDisplay::SetFrame(const uint32_t* thePixels, int thePitch, const Rect* theRects, int theCount)
{
	mPixels = thePixels;
	mPitch = thePitch;
	if (theCount < 0)
	{
		mPendingAll = true;
		mPending.clear();
		return;
	}
	for (int i = 0; i < theCount; i++)
		AddPending(theRects[i]);
}

void SurfaceDisplay::BeginFrame()
{
	// the mode follows the window
	UpdateDisplaySize();
	if ((mSurfaceMode) || (mRenderer == NULL))
		return;
	if (!PreDraw())
		return;
	// the screen texture is not cleared: Present copies the whole frame over it
	SetClip(NULL);
}

void SurfaceDisplay::Present()
{
	if (mSurfaceMode)
	{
		if (mStretching)
			PresentToStretch();
		else
			PresentToSurface();
	}
	else
		PresentToRenderer();
}

// nothing pending: the app has nothing new (it may mark its widget dirty at every update all the same), and the window
// shows the last frame. The window's contents can be lost without a word (expose, restore and size changes are told
// by ScreenInvalidated, which asks for the whole), so a draw the framework asks for with nothing pending, once a
// second at most, is a whole present as a safety net
int SurfaceDisplay::PendingRects(bool* theWhole)
{
	if ((!mPendingAll) && (mPending.empty()))
	{
		const Uint32 aNow = SDL_GetTicks();
		if ((Uint32) (aNow - mLastWholeTick) < 1000)
			return 0;
	}
	const bool aWhole = (mPendingAll) || (mPending.empty());
	*theWhole = aWhole;
	if (aWhole)
		mLastWholeTick = SDL_GetTicks();
	mSdlRects.resize(aWhole ? 1 : mPending.size());  // in the buffer's pixels
	SDL_Rect* aRects = &mSdlRects[0];
	int aCount = 0;
	if (aWhole)
	{
		aRects[0].x = 0;
		aRects[0].y = 0;
		aRects[0].w = mWidth;
		aRects[0].h = mHeight;
		aCount = 1;
	}
	else
	{
		for (size_t i = 0; i < mPending.size(); i++)
		{
			aRects[aCount].x = mPending[i].mX;
			aRects[aCount].y = mPending[i].mY;
			aRects[aCount].w = mPending[i].mWidth;
			aRects[aCount].h = mPending[i].mHeight;
			aCount++;
		}
	}
	return aCount;
}

void SurfaceDisplay::PresentToSurface()
{
	if (mPixels == NULL)
		return;
	// nothing is shown while minimised; what is pending stays, and the restore marks everything dirty
	if ((SDL_GetWindowFlags(mWindow) & SDL_WINDOW_MINIMIZED) != 0)
		return;
	SDL_Surface* aSurface = SDL_GetWindowSurface(mWindow);
	const int aScale = mSurfaceScale;
	if ((aSurface == NULL) || (aSurface->w != mWidth * aScale) || (aSurface->h != mHeight * aScale))
	{
		// the window changed size under us: UpdateDisplaySize will see it at the next frame
		mPendingAll = true;
		return;
	}

	bool aWhole = false;
	const int aCount = PendingRects(&aWhole);
	if (aCount == 0)
		return;
	SDL_Rect* aRects = &mSdlRects[0];

	if (SDL_MUSTLOCK(aSurface))
		SDL_LockSurface(aSurface);
	for (int i = 0; i < aCount; i++)
	{
		const uint8_t* aSrc = (const uint8_t*) mPixels + aRects[i].y * mPitch + aRects[i].x * 4;
		uint8_t* aDst = (uint8_t*) aSurface->pixels + aRects[i].y * aScale * aSurface->pitch + aRects[i].x * aScale * 4;
		CopyRectScaled(aSrc, mPitch, aDst, aSurface->pitch, aRects[i].w, aRects[i].h, aScale);
		// the window's rect is the buffer's, scale times each way
		aRects[i].x *= aScale;
		aRects[i].y *= aScale;
		aRects[i].w *= aScale;
		aRects[i].h *= aScale;
	}
	if (mVerifySurface)
		VerifySurface(aSurface);
	if (SDL_MUSTLOCK(aSurface))
		SDL_UnlockSurface(aSurface);
	SDL_UpdateWindowSurfaceRects(mWindow, aRects, aCount);

	mPending.clear();
	mPendingAll = false;
	mFrameCount++;
	mSurfacePresents++;
}

// Stretch mode: GDI stretches the whole buffer into the presentation rect, painting only the clip region of the rects that changed (see
// SurfaceDisplayGdi.h for why the whole and not the rect alone). A rect of the buffer covers, in the window, its edges scaled and rounded
// outwards, and a pixel more each side so that nothing a changed pixel could touch is left out; what the clip region adds that did not
// change is stretched to the colour it already has.
void SurfaceDisplay::PresentToStretch()
{
#ifdef _WIN32
	if ((mPixels == NULL) || (mGdi == NULL))
		return;
	// nothing is shown while minimised; what is pending stays, and the restore marks everything dirty
	if ((SDL_GetWindowFlags(mWindow) & SDL_WINDOW_MINIMIZED) != 0)
		return;
	int aWindowWidth = 0, aWindowHeight = 0;
	SDL_GetWindowSizeInPixels(mWindow, &aWindowWidth, &aWindowHeight);
	if ((aWindowWidth != mDisplayWidth) || (aWindowHeight != mDisplayHeight))
	{
		// the window changed size under us: UpdateDisplaySize will see it at the next frame
		mPendingAll = true;
		return;
	}

	bool aWhole = false;
	const int aCount = PendingRects(&aWhole);
	if (aCount == 0)
		return;
	SDL_Rect* aRects = &mSdlRects[0];
	const SDL_Rect aDest = { mPresentationRect.mX, mPresentationRect.mY, mPresentationRect.mWidth, mPresentationRect.mHeight };
	if (!aWhole)
	{
		const double aFactorX = (double) aDest.w / mWidth;
		const double aFactorY = (double) aDest.h / mHeight;
		for (int i = 0; i < aCount; i++)
		{
			const int aLeft = std::max(aDest.x, aDest.x + (int) std::floor(aRects[i].x * aFactorX) - 1);
			const int aTop = std::max(aDest.y, aDest.y + (int) std::floor(aRects[i].y * aFactorY) - 1);
			const int aRight = std::min(aDest.x + aDest.w, aDest.x + (int) std::ceil((aRects[i].x + aRects[i].w) * aFactorX) + 1);
			const int aBottom = std::min(aDest.y + aDest.h, aDest.y + (int) std::ceil((aRects[i].y + aRects[i].h) * aFactorY) + 1);
			aRects[i].x = aLeft;
			aRects[i].y = aTop;
			aRects[i].w = aRight - aLeft;
			aRects[i].h = aBottom - aTop;
		}
	}
	if (!mGdi->Present(mPixels, mPitch, mWidth, mHeight, aDest, aWhole ? NULL : aRects, aWhole ? 0 : aCount, aWindowWidth, aWindowHeight))
	{
		// no device context just now: all of it again at the next frame
		mPendingAll = true;
		return;
	}
	if (mVerifySurface)
		VerifyStretch(aDest, aWindowWidth, aWindowHeight);

	mPending.clear();
	mPendingAll = false;
	mFrameCount++;
	mStretchPresents++;
#endif
}

// SURFACE_VERIFY=1 in the environment: after every present the bitmap that is painted the same way as the window is the stretch of the
// whole buffer, at every pixel, bars included (the process aborts at a difference)
void SurfaceDisplay::VerifyStretch(const SDL_Rect& theDest, int theWindowWidth, int theWindowHeight)
{
#ifdef _WIN32
	int aX = 0, aY = 0;
	uint32_t aShown = 0, aWanted = 0;
	if (!mGdi->CheckShadow(mPixels, mPitch, mWidth, mHeight, theDest, theWindowWidth, theWindowHeight, &aX, &aY, &aShown, &aWanted))
	{
		SDL_Log("SurfaceDisplay: the stretched window is not the stretched screen at (%d, %d): %06x, not %06x", aX, aY, aShown, aWanted);
		fprintf(stderr, "SurfaceDisplay: the stretched window is not the stretched screen at (%d, %d): %06x, not %06x\n", aX, aY, aShown, aWanted);
		fflush(stderr);
		abort();
	}
	// SURFACE_VERIFY=2: the window itself, read back through its device context, is what a painting of the whole gives (the window's own stretch
	// need not be the bitmap's: GDI's nearest-neighbour steps may differ between the two kinds of device context)
	if (mVerifyWindow)
	{
		if (!mGdi->CheckWindow(mPixels, mPitch, mWidth, mHeight, theDest, theWindowWidth, theWindowHeight, &aX, &aY, &aShown, &aWanted))
		{
			SDL_Log("SurfaceDisplay: the window is not what was painted into it at (%d, %d): %06x, not %06x", aX, aY, aShown, aWanted);
			fprintf(stderr, "SurfaceDisplay: the window is not what was painted into it at (%d, %d): %06x, not %06x\n", aX, aY, aShown, aWanted);
			fflush(stderr);
			abort();
		}
	}
	mVerifiedFrames++;
#endif
}

// SURFACE_VERIFY=1 in the environment: after every present the window's surface is the buffer's screen at every pixel, the
// rects copied into it and all (the process aborts at a difference)
void SurfaceDisplay::VerifySurface(SDL_Surface* theSurface)
{
	for (int y = 0; y < mHeight * mSurfaceScale; y++)
	{
		const uint32_t* aSrc = (const uint32_t*) ((const uint8_t*) mPixels + (y / mSurfaceScale) * mPitch);
		const uint32_t* aDst = (const uint32_t*) ((const uint8_t*) theSurface->pixels + y * theSurface->pitch);
		for (int x = 0; x < mWidth * mSurfaceScale; x++)
		{
			if ((aSrc[x / mSurfaceScale] & 0xFFFFFF) != (aDst[x] & 0xFFFFFF))
			{
				SDL_Log("SurfaceDisplay: the window's surface is not the screen at (%d, %d): %06x, not %06x", x, y,
					aDst[x] & 0xFFFFFF, aSrc[x / mSurfaceScale] & 0xFFFFFF);
				abort();
			}
		}
	}
	mVerifiedFrames++;
}

// The pending rects go into the frame texture (one upload: their bounding rect), and the texture is copied over the whole
// screen texture, at the render scale, with the nearest filter DDInterface's own 1x blits use (mPixelPerfect), so that
// the screen texture is the buffer's pixels repeated render-scale times each way.
bool SurfaceDisplay::DrawFrameToScreenTexture()
{
	if ((mRenderer == NULL) || (mScreenTexture == NULL) || (mPixels == NULL))
		return false;

	FlushQuads();
	if (mFrameTexture == NULL)
	{
		mFrameTexture = SDL_CreateTexture(mRenderer, SDL_PIXELFORMAT_XRGB8888, SDL_TEXTUREACCESS_STATIC, mWidth, mHeight);
		if (mFrameTexture == NULL)
		{
			SDL_Log("SurfaceDisplay: no frame texture (%s)", SDL_GetError());
			return false;
		}
		SDL_SetTextureBlendMode(mFrameTexture, SDL_BLENDMODE_NONE);
		mFrameTextureWhole = false;
	}

	if ((mPendingAll) || (!mFrameTextureWhole))
	{
		SDL_UpdateTexture(mFrameTexture, NULL, mPixels, mPitch);
		mFrameTextureWhole = true;
	}
	else if (!mPending.empty())
	{
		Rect aBounds = mPending[0];
		for (size_t i = 1; i < mPending.size(); i++)
			aBounds = aBounds.Union(mPending[i]);
		SDL_Rect aRect = { aBounds.mX, aBounds.mY, aBounds.mWidth, aBounds.mHeight };
		SDL_UpdateTexture(mFrameTexture, &aRect, (const uint8_t*) mPixels + aRect.y * mPitch + aRect.x * 4, mPitch);
	}
	mPending.clear();
	mPendingAll = false;

	if (!PreDraw())
		return false;
	SetClip(NULL);
	SDL_SetTextureScaleMode(mFrameTexture, mPixelPerfect ? SDL_ScaleModeNearest : SDL_ScaleModeLinear);
	SDL_Rect aDest = { 0, 0, mWidth * mRenderScale, mHeight * mRenderScale };
	SDL_RenderCopy(mRenderer, mFrameTexture, NULL, &aDest);
	return true;
}

void SurfaceDisplay::PresentToRenderer()
{
	if (mRenderer == NULL)
		return;
	DrawFrameToScreenTexture();
	DDInterface::Present();
	mRendererPresents++;
}

///////////////////////////////////////////////////////////////////////////////
// Reading the screen back: both modes give the buffer's pixels

bool SurfaceDisplay::ReadScreen(ulong* theBits)
{
	if (theBits == NULL)
		return false;
	if (mPixels == NULL)
		return DDInterface::ReadScreen(theBits);
	CopyOpaque((const uint8_t*) mPixels, mPitch, mWidth, mHeight, theBits);
	return true;
}

bool SurfaceDisplay::ReadScreenFull(std::vector<uint32_t>& theBits, int& theWidth, int& theHeight)
{
	if (mPixels == NULL)
		return DDInterface::ReadScreenFull(theBits, theWidth, theHeight);
	if (mSurfaceMode)
	{
		// at the render scale, as the renderer's screen texture is read back
		theWidth = mWidth * mRenderScale;
		theHeight = mHeight * mRenderScale;
		theBits.resize((size_t) theWidth * theHeight);
		CopyOpaque((const uint8_t*) mPixels, mPitch, mWidth, mHeight, &theBits[0], mRenderScale);
		return true;
	}
	// the screen texture is made again from the buffer, at the render scale, and read back
	if (!DrawFrameToScreenTexture())
		return false;
	return DDInterface::ReadScreenFull(theBits, theWidth, theHeight);
}

}

#endif
