#include "DisplayGeometry.h"

#include <algorithm>
#include <math.h>

using namespace Sexy;

static const int MAX_RENDER_SCALE = 4;
static const int CURSOR_AREA_SIZE = 64;	// 1.3 centers the cursor image in a 64x64 area

DisplayGeometry::DisplayGeometry()
{
	mWidth = 640;
	mHeight = 480;
	mDisplayWidth = mWidth;
	mDisplayHeight = mHeight;
	mPresentationRect = Rect(0, 0, mWidth, mHeight);
}

int DisplayGeometry::UpdateDisplaySize(int theDrawableWidth, int theDrawableHeight, int theMaxTextureWidth, int theMaxTextureHeight,
	int theForcedRenderScale)
{
	if ((theDrawableWidth > 0) && (theDrawableHeight > 0))
	{
		mDisplayWidth = theDrawableWidth;
		mDisplayHeight = theDrawableHeight;
	}

	// Letterbox the logical screen (keeps its aspect ratio)
	double aScale = std::min(mDisplayWidth / (double) mWidth, mDisplayHeight / (double) mHeight);
	int aPresWidth = std::max(1, std::min(mDisplayWidth, (int) (mWidth * aScale + 0.5)));
	int aPresHeight = std::max(1, std::min(mDisplayHeight, (int) (mHeight * aScale + 0.5)));
	mPresentationRect = Rect((mDisplayWidth - aPresWidth) / 2, (mDisplayHeight - aPresHeight) / 2, aPresWidth, aPresHeight);

	// Render at the next integer scale at or above the presentation scale, so that HD textures keep
	// their detail and the final copy only ever scales down (or not at all)
	int aRenderScale = (theForcedRenderScale > 0) ? theForcedRenderScale : (int) ceil(aScale - 0.01);
	aRenderScale = std::max(1, std::min(MAX_RENDER_SCALE, aRenderScale));
	while ((aRenderScale > 1) && ((mWidth * aRenderScale > theMaxTextureWidth) || (mHeight * aRenderScale > theMaxTextureHeight)))
		aRenderScale--;
	return aRenderScale;
}

void DisplayGeometry::WindowToLogical(int theWindowWidth, int theWindowHeight, int theWindowX, int theWindowY, int* theX, int* theY) const
{
	double aX = theWindowX;
	double aY = theWindowY;

	// HiDPI: window coordinates are in points, the drawable is in pixels
	if ((theWindowWidth > 0) && (theWindowHeight > 0))
	{
		aX = aX * mDisplayWidth / theWindowWidth;
		aY = aY * mDisplayHeight / theWindowHeight;
	}

	if ((mPresentationRect.mWidth > 0) && (mPresentationRect.mHeight > 0))
	{
		aX = (aX - mPresentationRect.mX) * mWidth / mPresentationRect.mWidth;
		aY = (aY - mPresentationRect.mY) * mHeight / mPresentationRect.mHeight;
	}

	if (theX != NULL)
		*theX = (int) floor(aX);
	if (theY != NULL)
		*theY = (int) floor(aY);
}

void DisplayGeometry::LogicalToWindow(int theWindowWidth, int theWindowHeight, int theX, int theY, int* theWindowX, int* theWindowY) const
{
	// the middle of the game's pixel, so that WindowToLogical gives theX, theY back
	double aX = theX + 0.5;
	double aY = theY + 0.5;

	if ((mWidth > 0) && (mHeight > 0) && (mPresentationRect.mWidth > 0) && (mPresentationRect.mHeight > 0))
	{
		aX = aX * mPresentationRect.mWidth / mWidth + mPresentationRect.mX;
		aY = aY * mPresentationRect.mHeight / mHeight + mPresentationRect.mY;
	}

	if ((mDisplayWidth > 0) && (mDisplayHeight > 0) && (theWindowWidth > 0) && (theWindowHeight > 0))
	{
		aX = aX * theWindowWidth / mDisplayWidth;
		aY = aY * theWindowHeight / mDisplayHeight;
	}

	if (theWindowX != NULL)
		*theWindowX = (int) floor(aX);
	if (theWindowY != NULL)
		*theWindowY = (int) floor(aY);
}

void DisplayGeometry::CursorRect(int theCursorX, int theCursorY, int theImageWidth, int theImageHeight, float* theX, float* theY,
	float* theWidth, float* theHeight) const
{
	float aScaleX = mPresentationRect.mWidth / (float) mWidth;
	float aScaleY = mPresentationRect.mHeight / (float) mHeight;

	int aX = theCursorX - (CURSOR_AREA_SIZE / 2) + (CURSOR_AREA_SIZE - theImageWidth) / 2;
	int aY = theCursorY - (CURSOR_AREA_SIZE / 2) + (CURSOR_AREA_SIZE - theImageHeight) / 2;

	*theX = mPresentationRect.mX + aX * aScaleX;
	*theY = mPresentationRect.mY + aY * aScaleY;
	*theWidth = theImageWidth * aScaleX;
	*theHeight = theImageHeight * aScaleY;
}
