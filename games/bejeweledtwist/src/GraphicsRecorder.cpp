// Sexy::GraphicsRecorder (GraphicsRecorder.h).
#include "GraphicsRecorder.h"

#include "Graphics.h"
#include "SexyMatrix.h"

#include <cmath>

using namespace Sexy;

GraphicsRecorder* Sexy::gGraphicsRecorder = nullptr;

void GraphicsRecorder::DrawImage(Graphics* g, Image* theImage, int theX, int theY)
{
	if (!mSkipDraw)
		g->DrawImage(theImage, theX, theY);
}

void GraphicsRecorder::DrawImageF(Graphics* g, Image* theImage, float theX, float theY)
{
	if (!mSkipDraw)
		g->DrawImageF(theImage, theX, theY);
}

void GraphicsRecorder::DrawImage(Graphics* g, Image* theImage, int theX, int theY, int theWidth, int theHeight)
{
	// The original also draws a skipped image that cannot be recorded (Image +0x1c is null)
	if (!mSkipDraw)
		g->DrawImage(theImage, theX, theY, theWidth, theHeight);
}

void GraphicsRecorder::DrawImage(Graphics* g, Image* theImage, int theX, int theY, const Rect& theSrcRect)
{
	if (!mSkipDraw)
		g->DrawImage(theImage, theX, theY, theSrcRect);
}

void GraphicsRecorder::DrawImage(Graphics* g, Image* theImage, const Rect& theDestRect, const Rect& theSrcRect)
{
	if (!mSkipDraw)
		g->DrawImage(theImage, theDestRect, theSrcRect);
}

void GraphicsRecorder::DrawImageCel(Graphics* g, Image* theImage, int theX, int theY, int theCel)
{
	if (!mSkipDraw)
		g->DrawImageCel(theImage, theX, theY, theCel);
}

void GraphicsRecorder::DrawImageCel(Graphics* g, Image* theImage, const Rect& theDestRect, int theCel)
{
	if (!mSkipDraw)
		g->DrawImageCel(theImage, theDestRect, theCel);
}

void GraphicsRecorder::DrawImageTransform(Graphics* g, Image* theImage, const Transform& theTransform,
	const Rect& theSrcRect, float theX, float theY, bool theUseFloat)
{
	// The original repeats Graphics::DrawImageTransformHelper here so that it can record the simple cases; without
	// Direct3D it draws the same
	if (mSkipDraw)
		return;
	// A plain scale (0x70e134-0x70e1a7) goes to the scaled draw of a destination rectangle (0x421780), which also
	// applies the graphics' own scale (SetScale): the stretched rectangle's corner is scaled about the origin and its
	// size rounded up. The framework's helper and rectangle draw ignore the graphics' scale.
	if (!theTransform.mComplex && !theTransform.mHaveRot && theTransform.mHaveScale && theTransform.mScaleX != -1.0f &&
		(g->mScaleX != 1.0f || g->mScaleY != 1.0f)) {
		float aHalfWidth = (float)theSrcRect.mWidth / 2.0f;
		float aHalfHeight = (float)theSrcRect.mHeight / 2.0f;
		float aScaledHalfWidth = aHalfWidth * theTransform.mScaleX;
		float aScaledHalfHeight = aHalfHeight * theTransform.mScaleY;
		int aX = (int)(theX + theTransform.mTransX2 - aScaledHalfWidth);
		int aY = (int)(theY + theTransform.mTransY2 - aScaledHalfHeight);
		int aWidth = (int)(aScaledHalfWidth * 2.0f);
		int aHeight = (int)(aScaledHalfHeight * 2.0f);
		// 0x421780: the corner with the translation, then scaled about the origin
		float aCornerX = (float)(int)(g->mTransX + (float)aX);
		float aCornerY = (float)(int)(g->mTransY + (float)aY);
		int aScaledX = (int)(g->mScaleOrigX + (float)std::floor((aCornerX - g->mScaleOrigX) * g->mScaleX));
		int aScaledY = (int)(g->mScaleOrigY + (float)std::floor((aCornerY - g->mScaleOrigY) * g->mScaleY));
		int aScaledWidth = (int)std::ceil((float)aWidth * g->mScaleX);
		int aScaledHeight = (int)std::ceil((float)aHeight * g->mScaleY);
		// Graphics::DrawImage adds the translation itself and does not scale
		float aSavedScaleX = g->mScaleX;
		float aSavedScaleY = g->mScaleY;
		g->mScaleX = 1.0f;
		g->mScaleY = 1.0f;
		g->DrawImage(theImage, Rect(aScaledX - (int)g->mTransX, aScaledY - (int)g->mTransY, aScaledWidth, aScaledHeight),
			theSrcRect);
		g->mScaleX = aSavedScaleX;
		g->mScaleY = aSavedScaleY;
		return;
	}
	if (theUseFloat)
		g->DrawImageTransformF(theImage, theTransform, theSrcRect, theX, theY);
	else
		g->DrawImageTransform(theImage, theTransform, theSrcRect, theX, theY);
}
