#ifndef __EMSCRIPTEN__

#include "SoftwareScreenDisplay.h"
#include "DDImage.h"
#include "SexyAppBase.h"

#include "../platform/CycleSplit.h"

#include <SDL.h>

#include <algorithm>
#include <cstdlib>
#include <cstring>

namespace Sexy
{

// a block of the screen a draw marks: 16 pixels a side
static const int kBlockSize = 16;

DDInterface* SoftwareScreenDisplay::Create(SexyAppBase* theApp)
{
	if ((theApp != NULL) && (theApp->IsTestRun()))
	{
		const char* aMode = SDL_getenv("SURFACE_DISPLAY");
		if ((aMode != NULL) && (strcmp(aMode, "off") == 0))
			return new DDInterface(theApp);
	}
	return new SoftwareScreenDisplay(theApp);
}

SoftwareScreenDisplay::SoftwareScreenDisplay(SexyAppBase* theApp) :
	SurfaceDisplay(theApp)
{
	mShowWhole = true;
	mBlocksWide = 0;
	mBlocksHigh = 0;
	mAnyDrawn = false;
	mStats = false;
	mStatCycles = 0;
	mStatFrames = 0;
	mStatWholes = 0;
	mStatDraws = 0;
	mStatRects = 0;
	mStatArea = 0;
	if ((theApp != NULL) && (theApp->IsTestRun()))
	{
		const char* anEnv = SDL_getenv("SOFTSCREEN_STATS");
		mStats = (anEnv != NULL) && (anEnv[0] == '1');
	}
	mNoGpuRenderer = false;
	mChangingMode = false;
	mGpuPresents = 0;
	SoftwareScreen();
}

SoftwareScreenDisplay::~SoftwareScreenDisplay()
{
	// SurfaceDisplay's destructor can only call its own Cleanup
	Cleanup();
}

void SoftwareScreenDisplay::SoftwareScreen()
{
	mGpuScreen = false;
	// a stretch onto the screen as a software screen draws one, stretched into an image of the destination's size and
	// then blitted (MemoryImage's own fast stretch steps a column before it samples), its fast stretch sampling pixel
	// centres as the GPU path did
	mSoftwareStretch = true;
	mFastStretchCentre = true;
	No3D();
}

void SoftwareScreenDisplay::No3D()
{
	mIs3D = false;
	mCan3D = false;
}

int SoftwareScreenDisplay::WantedSurfaceScale()
{
	const int aScale = SurfaceDisplay::WantedSurfaceScale();
	// above 1x the screen is drawn on the GPU where the window's renderer is accelerated, so SurfaceDisplay makes the
	// renderer to find out (NoteRenderer); with no accelerated renderer the surface shows the software screen at 2x to 4x
	if ((aScale > 1) && (!mNoGpuRenderer))
		return 0;
	return aScale;
}

void SoftwareScreenDisplay::NoteRenderer()
{
	if ((mGpuScreen) || (mSurfaceMode) || (mRenderer == NULL))
		return;
	SDL_RendererInfo anInfo;
	if (SDL_GetRendererInfo(mRenderer, &anInfo) != 0)
		return;
	if (((anInfo.flags & SDL_RENDERER_ACCELERATED) != 0) && ((anInfo.flags & SDL_RENDERER_SOFTWARE) == 0))
	{
		DrawOnGpu(anInfo.name);
		return;
	}
	if (mNoGpuRenderer)
		return;
	mNoGpuRenderer = true;
	SDL_Log("SoftwareScreenDisplay: the %s renderer is not accelerated, so the screen is drawn in software at every size",
		anInfo.name);
	// at 2x to 4x the window's surface shows it, without the renderer
	mChangingMode = true;
	SurfaceDisplay::UpdateDisplaySize();
	mChangingMode = false;
}

void SoftwareScreenDisplay::DrawOnGpu(const char* theRendererName)
{
	mGpuScreen = true;
	// DDInterface's own path from here: its stretches, and the 3D flags of its accelerated renderer (DDInterface::Init's)
	mSoftwareStretch = false;
	mFastStretchCentre = false;
	mIs3D = true;
	mCan3D = true;
	// the screen texture starts from the last frame drawn in software (none at the start), so that a screen kept from
	// one frame to the next (mKeepScreen) loses nothing; every frame from here draws all the widgets onto it
	mPendingAll = true;
	DrawFrameToScreenTexture();
	ReleaseFrameTexture();
	mDrawnBlocks.clear();
	mBlocksWide = 0;
	mBlocksHigh = 0;
	mAnyDrawn = false;
	SDL_Log("SoftwareScreenDisplay: the %s renderer is accelerated, so the screen is drawn on the GPU from here on", theRendererName);
}

int SoftwareScreenDisplay::Init(SDL_Window* theWindow, bool vsync)
{
	mChangingMode = true;
	int aResult = SurfaceDisplay::Init(theWindow, vsync);
	mChangingMode = false;
	NoteRenderer();
	if (!mGpuScreen)
		No3D();
	mShowWhole = true;
	return aResult;
}

void SoftwareScreenDisplay::Cleanup()
{
	if ((mStats) && (mStatFrames + mStatWholes > 0))
	{
		const double aScreenArea = (double) mWidth * mHeight;
		SDL_Log("SoftwareScreenDisplay: %d frames by the draws' blocks (%d shown whole), %.1f draws noted a frame, %.3f M cycles "
			"making rects, %.1f k a frame; %.1f rects and %.1f%% of the screen handed over a frame", mStatFrames, mStatWholes,
			(double) mStatDraws / (mStatFrames + mStatWholes), mStatCycles / 1e6,
			(mStatFrames > 0) ? mStatCycles / 1e3 / mStatFrames : 0.0, (mStatFrames > 0) ? (double) mStatRects / mStatFrames : 0.0,
			((mStatFrames > 0) && (aScreenArea > 0)) ? 100.0 * mStatArea / mStatFrames / aScreenArea : 0.0);
	}
	mStatCycles = 0;
	mStatFrames = 0;
	mStatWholes = 0;
	mStatDraws = 0;
	mStatRects = 0;
	mStatArea = 0;
	mDrawnBlocks.clear();
	mBlocksWide = 0;
	mBlocksHigh = 0;
	mAnyDrawn = false;
	mShowWhole = true;
	if (mGpuPresents > 0)
		SDL_Log("SoftwareScreenDisplay: %d frames drawn on the GPU", mGpuPresents);
	mGpuPresents = 0;
	mNoGpuRenderer = false;
	SurfaceDisplay::Cleanup();
	SoftwareScreen();
}

void SoftwareScreenDisplay::UpdateDisplaySize()
{
	if (mGpuScreen)
	{
		DDInterface::UpdateDisplaySize();
		return;
	}
	// asked again by the DDInterface::Init of a change of mode: the mode is SurfaceDisplay's to finish first
	if (mChangingMode)
	{
		SurfaceDisplay::UpdateDisplaySize();
		return;
	}
	mChangingMode = true;
	SurfaceDisplay::UpdateDisplaySize();
	mChangingMode = false;
	NoteRenderer();
	// a change of mode sets the 3D flags from the renderer (DDInterface::Init) or as a GPU display's (SurfaceDisplay)
	if (!mGpuScreen)
		No3D();
}

bool SoftwareScreenDisplay::CheckDestination(DDImage* theImage)
{
	// in software the screen too is drawn by the software blitters, into its own bits
	return (mGpuScreen) && (DDInterface::CheckDestination(theImage));
}

const uint32_t* SoftwareScreenDisplay::ScreenPixels()
{
	if ((mScreenImage == NULL) || (mScreenImage->mWidth != mWidth) || (mScreenImage->mHeight != mHeight))
		return NULL;
	bool isNew = mScreenImage->mBits == NULL;
	ulong* aBits = mScreenImage->GetBits();
	// a new screen is opaque black, as a new DirectDraw surface is cleared, not the heap's leftovers
	if ((isNew) && (aBits != NULL))
		std::fill(aBits, aBits + (size_t) mWidth * mHeight, (ulong) 0xFF000000);
	// the screen is opaque, as the original's DirectDraw surface: the blitters take their paths for a destination without
	// alpha (MI_NormalBlt.inc's run-length path, which copies opaque runs and writes 0xFF into the top byte). A new
	// MemoryImage counts as transparent with alpha (MemoryImage::Create), which would blend every pixel with the
	// destination's alpha, two divisions a pixel.
	mScreenImage->mHasAlpha = false;
	mScreenImage->mHasTrans = false;
	return (const uint32_t*) aBits;
}

void SoftwareScreenDisplay::BeginFrame()
{
	if (!mGpuScreen)
	{
		// the mode follows the window (UpdateDisplaySize), which may hand the screen to the GPU
		SurfaceDisplay::BeginFrame();
		if (!mGpuScreen)
		{
			No3D();
			// the buffer SurfaceDisplay reads the screen from (ReadScreen between presents too), nothing marked changed yet
			const uint32_t* aPixels = ScreenPixels();
			if (aPixels != NULL)
				SetFrame(aPixels, mWidth * 4, NULL, 0);
			return;
		}
	}
	DDInterface::BeginFrame();
}

void SoftwareScreenDisplay::ScreenDrawn(const Rect& theRect)
{
	const int aBlocksWide = (mWidth + kBlockSize - 1) / kBlockSize;
	const int aBlocksHigh = (mHeight + kBlockSize - 1) / kBlockSize;
	if ((aBlocksWide != mBlocksWide) || (aBlocksHigh != mBlocksHigh))
	{
		mBlocksWide = aBlocksWide;
		mBlocksHigh = aBlocksHigh;
		mDrawnBlocks.assign((size_t) aBlocksWide * aBlocksHigh, 0);
		mAnyDrawn = false;
		mShowWhole = true;
	}
	const int aLeft = std::max(theRect.mX, 0);
	const int aTop = std::max(theRect.mY, 0);
	const int aRight = std::min(theRect.mX + theRect.mWidth, mWidth);
	const int aBottom = std::min(theRect.mY + theRect.mHeight, mHeight);
	if ((aRight <= aLeft) || (aBottom <= aTop))
		return;
	const int aBlockLeft = aLeft / kBlockSize;
	const int aBlockRight = (aRight - 1) / kBlockSize;
	for (int aBlockY = aTop / kBlockSize; aBlockY <= (aBottom - 1) / kBlockSize; aBlockY++)
		memset(&mDrawnBlocks[(size_t) aBlockY * mBlocksWide + aBlockLeft], 1, (size_t) (aBlockRight - aBlockLeft + 1));
	mAnyDrawn = true;
	mStatDraws++;
}

void SoftwareScreenDisplay::Present()
{
	if (mGpuScreen)
	{
		DDInterface::Present();
		mGpuPresents++;
		return;
	}
	const uint32_t* aPixels = ScreenPixels();
	if (aPixels != NULL)
		HandOverDrawn(aPixels);
	SurfaceDisplay::Present();
}

void SoftwareScreenDisplay::HandOverDrawn(const uint32_t* thePixels)
{
	const int aWidth = mWidth;
	const int aHeight = mHeight;
	const int aBlocksWide = (aWidth + kBlockSize - 1) / kBlockSize;
	const int aBlocksHigh = (aHeight + kBlockSize - 1) / kBlockSize;

	// the whole screen: asked for (the first frame, a change of mode, ScreenInvalidated), or its size changed
	if ((mPendingAll) || (mShowWhole) || (aBlocksWide != mBlocksWide) || (aBlocksHigh != mBlocksHigh))
	{
		mBlocksWide = aBlocksWide;
		mBlocksHigh = aBlocksHigh;
		mDrawnBlocks.assign((size_t) aBlocksWide * aBlocksHigh, 0);
		mAnyDrawn = false;
		mShowWhole = false;
		SetFrame(thePixels, aWidth * 4, NULL, -1);
		mStatWholes++;
		return;
	}

	const unsigned long long aStartCycles = mStats ? Platform::CurrentThreadCycles() : 0;

	// the drawn blocks as rects: a run of them in a block row, carried down while the rows below have a run of the same
	// columns
	mDrawnRects.clear();
	if (mAnyDrawn)
	{
		mOpenRuns.clear();
		for (int aBlockY = 0; aBlockY <= aBlocksHigh; aBlockY++)
		{
			mNextRuns.clear();
			if (aBlockY < aBlocksHigh)
			{
				const uint8_t* aBlockRow = &mDrawnBlocks[(size_t) aBlockY * aBlocksWide];
				int aBlock = 0;
				while (aBlock < aBlocksWide)
				{
					if (!aBlockRow[aBlock])
					{
						aBlock++;
						continue;
					}
					OpenRun aRun;
					aRun.mLeft = aBlock;
					while ((aBlock < aBlocksWide) && (aBlockRow[aBlock]))
						aBlock++;
					aRun.mRight = aBlock - 1;
					aRun.mTop = aBlockY;
					for (size_t i = 0; i < mOpenRuns.size(); i++)
					{
						if ((mOpenRuns[i].mLeft == aRun.mLeft) && (mOpenRuns[i].mRight == aRun.mRight))
						{
							aRun.mTop = mOpenRuns[i].mTop;
							mOpenRuns[i].mLeft = -1;  // carried on, not finished
							break;
						}
					}
					mNextRuns.push_back(aRun);
				}
			}
			// the runs above that did not go on end at this block row
			for (size_t i = 0; i < mOpenRuns.size(); i++)
			{
				const OpenRun& aRun = mOpenRuns[i];
				if (aRun.mLeft < 0)
					continue;
				const int aLeft = aRun.mLeft * kBlockSize;
				const int aTop = aRun.mTop * kBlockSize;
				const int aRight = std::min((aRun.mRight + 1) * kBlockSize, aWidth);
				const int aBottom = std::min(aBlockY * kBlockSize, aHeight);
				mDrawnRects.push_back(Rect(aLeft, aTop, aRight - aLeft, aBottom - aTop));
			}
			mOpenRuns.swap(mNextRuns);
		}

		// past SurfaceDisplay's limit, their bounding rect
		if (mDrawnRects.size() > mMaxPending)
		{
			Rect aBounds = mDrawnRects[0];
			for (size_t i = 1; i < mDrawnRects.size(); i++)
				aBounds = aBounds.Union(mDrawnRects[i]);
			mDrawnRects.assign(1, aBounds);
		}
		std::fill(mDrawnBlocks.begin(), mDrawnBlocks.end(), (uint8_t) 0);
		mAnyDrawn = false;
	}

	SetFrame(thePixels, aWidth * 4, mDrawnRects.empty() ? NULL : &mDrawnRects[0], (int) mDrawnRects.size());

	if (mStats)
	{
		mStatCycles += Platform::CurrentThreadCycles() - aStartCycles;
		mStatFrames++;
		mStatRects += (long long) mDrawnRects.size();
		for (size_t i = 0; i < mDrawnRects.size(); i++)
			mStatArea += (long long) mDrawnRects[i].mWidth * mDrawnRects[i].mHeight;
	}
}

bool SoftwareScreenDisplay::ReadScreen(ulong* theBits)
{
	if (mGpuScreen)
		return DDInterface::ReadScreen(theBits);
	const uint32_t* aPixels = ScreenPixels();
	if ((aPixels == NULL) || (theBits == NULL))
		return false;
	const size_t aSize = (size_t) mWidth * mHeight;
	for (size_t i = 0; i < aSize; i++)
		theBits[i] = 0xFF000000 | (aPixels[i] & 0xFFFFFF);
	return true;
}

bool SoftwareScreenDisplay::ReadScreenFull(std::vector<uint32_t>& theBits, int& theWidth, int& theHeight)
{
	if (mGpuScreen)
		return DDInterface::ReadScreenFull(theBits, theWidth, theHeight);
	// at the scale the window shows the screen at: the surface's, or the renderer's screen texture, which holds the
	// screen's pixels repeated (the nearest filter, mPixelPerfect); with a smoothed scale the renderer's own is read back
	const int aScale = std::max(1, mSurfaceMode ? mSurfaceScale : mRenderScale);
	if ((!mSurfaceMode) && (aScale > 1) && (!mPixelPerfect))
		return SurfaceDisplay::ReadScreenFull(theBits, theWidth, theHeight);
	const uint32_t* aPixels = ScreenPixels();
	if (aPixels == NULL)
		return false;
	theWidth = mWidth * aScale;
	theHeight = mHeight * aScale;
	theBits.resize((size_t) theWidth * theHeight);
	for (int y = 0; y < mHeight; y++)
	{
		const uint32_t* aSrc = aPixels + (size_t) y * mWidth;
		uint32_t* aRow = &theBits[(size_t) y * aScale * theWidth];
		for (int x = 0; x < mWidth; x++)
		{
			const uint32_t aPixel = 0xFF000000 | (aSrc[x] & 0xFFFFFF);
			for (int i = 0; i < aScale; i++)
				aRow[x * aScale + i] = aPixel;
		}
		for (int j = 1; j < aScale; j++)
			memcpy(aRow + (size_t) j * theWidth, aRow, (size_t) theWidth * 4);
	}
	return true;
}

}

#endif
