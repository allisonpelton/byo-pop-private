#ifndef __EMSCRIPTEN__

#include "SurfaceDisplay.h"
#include "DDImage.h"
#include "SexyAppBase.h"

#include <SDL.h>

#include <algorithm>
#include <cstdlib>
#include <cstring>

namespace Sexy
{

// more rects than this pile up between two presents and they become their bounding rect
static const size_t kMaxPendingRects = 6;

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
	mSurfaceUnavailable = false;
	const char* anEnv = SDL_getenv("SURFACE_DISPLAY");
	mSurfaceDisabled = (anEnv != NULL) && (anEnv[0] == '0');
	anEnv = SDL_getenv("SURFACE_VERIFY");
	mVerifySurface = (anEnv != NULL) && (anEnv[0] == '1');
	mFrameTexture = NULL;
	mFrameTextureWhole = false;
}

SurfaceDisplay::~SurfaceDisplay()
{
	// the frame texture must go before the renderer does (DDInterface's destructor can only call its own Cleanup)
	Cleanup();
}

///////////////////////////////////////////////////////////////////////////////
// The mode

int SurfaceDisplay::WantedSurfaceScale()
{
	if (mWindow == NULL)
		return 0;
	if ((mSurfaceDisabled) || (mSurfaceUnavailable))
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

	if (mScreenImage == NULL)
	{
		mScreenImage = new DDImage(this);
		mScreenImage->mIsScreen = true;
	}
	if ((mScreenImage->mWidth != mWidth) || (mScreenImage->mHeight != mHeight))
		mScreenImage->Create(mWidth, mHeight);

	mSurfaceMode = true;
	mSurfaceScale = theScale;
	SDL_Log("SurfaceDisplay: the window's surface (%dx%d, the screen at %dx)", mWidth * theScale, mHeight * theScale, theScale);
	// as GPUDisplay: no SDL renderer, so the screen image is read back through ReadScreen, and a window that is
	// minimised draws nothing (DrawDirtyStuff)
	mIs3D = true;
	mCan3D = true;
	mRenderScale = theScale;
	mPendingAll = true;
	mModeChanges++;
	// no vsync here: at most one frame in each refresh interval of the window's display (a -fast run draws on its own
	// clock and is not held back)
	mMinPresentInterval = 0;
	if ((mApp == NULL) || (!mApp->mFastMode))
	{
		int aRefresh = 0;
		SDL_DisplayMode aMode;
		if (SDL_GetCurrentDisplayMode(std::max(0, SDL_GetWindowDisplayIndex(mWindow)), &aMode) == 0)
			aRefresh = aMode.refresh_rate;
		mMinPresentInterval = 1000 / ((aRefresh > 0) ? aRefresh : 60);
	}
	return true;
}

void SurfaceDisplay::RestoreRenderer()
{
	mSurfaceMode = false;
	mMinPresentInterval = 0;
	SDL_DestroyWindowSurface(mWindow);
	int aResult = DDInterface::Init(mWindow, mVSync);
	if (aResult != RESULT_OK)
		SDL_Log("SurfaceDisplay: the renderer could not be made again (%d)", aResult);
	else
	{
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
	int aResult = DDInterface::Init(theWindow, vsync);
	if (aResult == RESULT_OK)
	{
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
	if (mSurfacePresents + mRendererPresents > 0)
		SDL_Log("SurfaceDisplay: %d frames through the surface, %d through the renderer, %d changes of mode", mSurfacePresents,
			mRendererPresents, mModeChanges);
	if (mVerifiedFrames > 0)
		SDL_Log("SurfaceDisplay: %d frames of the surface compared with the screen, none differing", mVerifiedFrames);
	mVerifiedFrames = 0;
	mSurfacePresents = 0;
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
	int aWantedScale = WantedSurfaceScale();
	if ((aWantedScale > 0) && ((!mSurfaceMode) || (aWantedScale != mSurfaceScale)))
	{
		if (!EnterSurfaceMode(aWantedScale))
			RestoreRenderer();
	}
	else if ((aWantedScale == 0) && (mSurfaceMode))
		LeaveSurfaceMode();

	if (mSurfaceMode)
	{
		int aWidth = 0, aHeight = 0;
		SDL_GetWindowSizeInPixels(mWindow, &aWidth, &aHeight);
		DisplayGeometry::UpdateDisplaySize(aWidth, aHeight, mMaxTextureWidth, mMaxTextureHeight, 0);
		mRenderScale = mSurfaceScale;
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
	mPending.push_back(aRect);
	if (mPending.size() > kMaxPendingRects)
	{
		Rect aBounds = mPending[0];
		for (size_t i = 1; i < mPending.size(); i++)
			aBounds = aBounds.Union(mPending[i]);
		mPending.clear();
		mPending.push_back(aBounds);
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
		PresentToSurface();
	else
		PresentToRenderer();
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

	// a present with no rect pending is a draw the framework asked for (an expose, a restore, the idle second): the
	// whole window is made again from the buffer
	bool aWhole = (mPendingAll) || (mPending.empty());
	SDL_Rect aRects[kMaxPendingRects];  // in the buffer's pixels
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
		for (size_t i = 0; (i < mPending.size()) && (i < kMaxPendingRects); i++)
		{
			aRects[aCount].x = mPending[i].mX;
			aRects[aCount].y = mPending[i].mY;
			aRects[aCount].w = mPending[i].mWidth;
			aRects[aCount].h = mPending[i].mHeight;
			aCount++;
		}
	}

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
		theWidth = mWidth * mSurfaceScale;
		theHeight = mHeight * mSurfaceScale;
		theBits.resize((size_t) theWidth * theHeight);
		CopyOpaque((const uint8_t*) mPixels, mPitch, mWidth, mHeight, &theBits[0], mSurfaceScale);
		return true;
	}
	// the screen texture is made again from the buffer, at the render scale, and read back
	if (!DrawFrameToScreenTexture())
		return false;
	return DDInterface::ReadScreenFull(theBits, theWidth, theHeight);
}

}

#endif
