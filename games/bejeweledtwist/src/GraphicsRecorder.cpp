// Sexy::GraphicsRecorder (GraphicsRecorder.h).
#include "GraphicsRecorder.h"

#include "Graphics.h"
#include "Image.h"
#include "ModFeatures.h"
#include "SexyMatrix.h"
#include "X86Math.h"

#include <cmath>

using namespace Sexy;

GraphicsRecorder* Sexy::gGraphicsRecorder = nullptr;

namespace {

// 0x421780, the exe's Graphics::DrawImage of a destination and a source rectangle: it also applies the graphics' own
// scale (SetScale): the stretched rectangle's corner is scaled about the scale's origin and its size rounded up. The
// framework's rectangle draw ignores the graphics' scale.
void DrawImageScaled(Graphics* g, Image* theImage, const Rect& theDestRect, const Rect& theSrcRect)
{
	if (g->mScaleX == 1.0f && g->mScaleY == 1.0f) {
		g->DrawImage(theImage, theDestRect, theSrcRect);
		return;
	}
	float aCornerX = (float)(int)(g->mTransX + (float)theDestRect.mX);
	float aCornerY = (float)(int)(g->mTransY + (float)theDestRect.mY);
	int aScaledX = (int)(g->mScaleOrigX + (float)std::floor((aCornerX - g->mScaleOrigX) * g->mScaleX));
	int aScaledY = (int)(g->mScaleOrigY + (float)std::floor((aCornerY - g->mScaleOrigY) * g->mScaleY));
	int aScaledWidth = (int)std::ceil((float)theDestRect.mWidth * g->mScaleX);
	int aScaledHeight = (int)std::ceil((float)theDestRect.mHeight * g->mScaleY);
	// Graphics::DrawImage adds the translation itself and does not scale
	float aSavedScaleX = g->mScaleX;
	float aSavedScaleY = g->mScaleY;
	g->mScaleX = 1.0f;
	g->mScaleY = 1.0f;
	g->DrawImage(theImage, Rect(aScaledX - (int)g->mTransX, aScaledY - (int)g->mTransY, aScaledWidth, aScaledHeight),
		theSrcRect);
	g->mScaleX = aSavedScaleX;
	g->mScaleY = aSavedScaleY;
}

// Only an image loaded from a file is recorded (the exe tests its path's length, Image +0x1c)
bool CanRecord(Image* theImage)
{
	return !theImage->mFilePath.empty();
}

}  // namespace

void GraphicsOperation::Execute(Graphics* g)
{
	g->SetDrawMode(mDrawMode);
	g->SetColor(mColor);
	int aX = Ftol(mX);
	int aY = Ftol(mY);
	int aWidth = Ftol(mWidth);
	int aHeight = Ftol(mHeight);
	switch (mType) {
	case TYPE_DRAW_F:
		g->DrawImageF(mImage, (float)mX, (float)mY);
		break;
	case TYPE_DRAW_STRETCHED:
		g->DrawImage(mImage, aX, aY, mSrcRect.mWidth, mSrcRect.mHeight);
		break;
	case TYPE_DRAW_SRC:
		g->DrawImage(mImage, aX, aY, mSrcRect);
		break;
	case TYPE_DRAW_DEST_SRC:
		DrawImageScaled(g, mImage, Rect(aX, aY, aWidth, aHeight), mSrcRect);
		break;
	case TYPE_DRAW_CEL:
		g->DrawImageCel(mImage, aX, aY, mSrcRect.mX);
		break;
	case TYPE_DRAW_CEL_DEST:
		g->DrawImageCel(mImage, Rect(aX, aY, aWidth, aHeight), mSrcRect.mX);
		break;
	case TYPE_DRAW_ROTATED:
		g->DrawImageRotated(mImage, aX, aY, (double)mRot, &mSrcRect);
		break;
	case TYPE_DRAW_ROTATED_AT:
		g->DrawImageRotated(mImage, aX, aY, (double)mRot, aWidth, aHeight, &mSrcRect);
		break;
	}
}

GraphicsOperation* GraphicsRecorder::AddOperation(Graphics* g, int theType)
{
	mOperations.push_back(GraphicsOperation());
	GraphicsOperation* anOperation = &mOperations.back();
	anOperation->mType = theType;
	anOperation->mFrame = mFrame;
	anOperation->mDrawMode = g->mDrawMode;
	anOperation->mColor = g->mColorizeImages ? g->mColor : Color::White;
	return anOperation;
}

int GraphicsRecorder::GetLastFrame()
{
	if (mOperations.empty())
		return -1;
	return mOperations.back().mFrame;
}

void GraphicsRecorder::DrawFrame(Graphics* g, int theFrame)
{
	int aCount = (int)mOperations.size();
	if (aCount == 0)
		return;
	int i = aCount;
	while (0 < i && theFrame <= mOperations[i - 1].mFrame)
		i--;
	// (the exe reads the draw at i unchecked; DrawOverlay passes the last frame, so i is within the list)
	int aFrame = mOperations[i].mFrame;
	g->PushState();
	g->SetColorizeImages(true);
	while (i < (int)mOperations.size() && mOperations[i].mFrame == aFrame) {
		mOperations[i].Execute(g);
		i++;
	}
	g->PopState();
}

void GraphicsRecorder::PopFramesFrom(int theFrame)
{
	while (!mOperations.empty()) {
		if (mOperations.back().mFrame < theFrame)
			break;
		mOperations.pop_back();
	}
}

void GraphicsRecorder::DrawImage(Graphics* g, Image* theImage, int theX, int theY)
{
	if (!mSkipDraw)
		g->DrawImage(theImage, theX, theY);
	if (mRecording && CanRecord(theImage)) {
		GraphicsOperation* anOperation = AddOperation(g, GraphicsOperation::TYPE_DRAW_F);
		anOperation->mImage = theImage;
		anOperation->mX = (double)(Ftol(g->mTransX) + theX);
		anOperation->mY = (double)(Ftol(g->mTransY) + theY);
	}
}

void GraphicsRecorder::DrawImageF(Graphics* g, Image* theImage, float theX, float theY)
{
	if (!mSkipDraw)
		g->DrawImageF(theImage, theX, theY);
	if (mRecording && CanRecord(theImage)) {
		GraphicsOperation* anOperation = AddOperation(g, GraphicsOperation::TYPE_DRAW_F);
		anOperation->mImage = theImage;
		anOperation->mX = (double)(g->mTransX + theX);
		anOperation->mY = (double)(g->mTransY + theY);
	}
}

void GraphicsRecorder::DrawImage(Graphics* g, Image* theImage, int theX, int theY, int theWidth, int theHeight)
{
	// The exe also draws a skipped image that cannot be recorded
	if (!mSkipDraw || !CanRecord(theImage))
		g->DrawImage(theImage, theX, theY, theWidth, theHeight);
	if (mRecording && CanRecord(theImage)) {
		GraphicsOperation* anOperation = AddOperation(g, GraphicsOperation::TYPE_DRAW_STRETCHED);
		anOperation->mImage = theImage;
		anOperation->mX = (double)(Ftol(g->mTransX) + theX);
		anOperation->mSrcRect.mWidth = theWidth;
		anOperation->mY = (double)(Ftol(g->mTransY) + theY);
		anOperation->mSrcRect.mHeight = theHeight;
	}
}

void GraphicsRecorder::DrawImage(Graphics* g, Image* theImage, int theX, int theY, const Rect& theSrcRect)
{
	if (!mSkipDraw)
		g->DrawImage(theImage, theX, theY, theSrcRect);
	if (mRecording && CanRecord(theImage)) {
		GraphicsOperation* anOperation = AddOperation(g, GraphicsOperation::TYPE_DRAW_SRC);
		anOperation->mImage = theImage;
		anOperation->mX = (double)(Ftol(g->mTransX) + theX);
		anOperation->mY = (double)(Ftol(g->mTransY) + theY);
		anOperation->mSrcRect = theSrcRect;
	}
}

void GraphicsRecorder::DrawImage(Graphics* g, Image* theImage, const Rect& theDestRect, const Rect& theSrcRect)
{
	if (!mSkipDraw)
		DrawImageScaled(g, theImage, theDestRect, theSrcRect);
	if (mRecording && CanRecord(theImage)) {
		GraphicsOperation* anOperation = AddOperation(g, GraphicsOperation::TYPE_DRAW_DEST_SRC);
		anOperation->mImage = theImage;
		anOperation->mX = (double)theDestRect.mX;
		anOperation->mY = (double)theDestRect.mY;
		anOperation->mWidth = (double)theDestRect.mWidth;
		anOperation->mHeight = (double)theDestRect.mHeight;
		anOperation->mX = (double)Ftol(g->mTransX) + anOperation->mX;
		anOperation->mY = (double)Ftol(g->mTransY) + anOperation->mY;
		anOperation->mSrcRect = theSrcRect;
	}
}

void GraphicsRecorder::DrawImageCel(Graphics* g, Image* theImage, int theX, int theY, int theCel)
{
	if (!mSkipDraw)
		g->DrawImageCel(theImage, theX, theY, theCel);
	if (mRecording && CanRecord(theImage)) {
		GraphicsOperation* anOperation = AddOperation(g, GraphicsOperation::TYPE_DRAW_CEL);
		anOperation->mImage = theImage;
		anOperation->mX = (double)(Ftol(g->mTransX) + theX);
		anOperation->mSrcRect.mX = theCel;
		anOperation->mY = (double)(Ftol(g->mTransY) + theY);
	}
}

void GraphicsRecorder::DrawImageCel(Graphics* g, Image* theImage, const Rect& theDestRect, int theCel)
{
	if (!mSkipDraw)
		g->DrawImageCel(theImage, theDestRect, theCel);
	if (mRecording && CanRecord(theImage)) {
		GraphicsOperation* anOperation = AddOperation(g, GraphicsOperation::TYPE_DRAW_CEL_DEST);
		anOperation->mImage = theImage;
		anOperation->mX = (double)theDestRect.mX;
		anOperation->mY = (double)theDestRect.mY;
		anOperation->mWidth = (double)theDestRect.mWidth;
		anOperation->mHeight = (double)theDestRect.mHeight;
		anOperation->mX = (double)Ftol(g->mTransX) + anOperation->mX;
		anOperation->mSrcRect.mX = theCel;
		anOperation->mY = (double)Ftol(g->mTransY) + anOperation->mY;
	}
}

void GraphicsRecorder::DrawImageRotated(Graphics* g, Image* theImage, int theX, int theY, double theRot,
	int theRotCenterX, int theRotCenterY, const Rect* theSrcRect)
{
	if (!mSkipDraw)
		g->DrawImageRotated(theImage, theX, theY, theRot, theRotCenterX, theRotCenterY, theSrcRect);
	if (mRecording && CanRecord(theImage)) {
		GraphicsOperation* anOperation = AddOperation(g, GraphicsOperation::TYPE_DRAW_ROTATED_AT);
		anOperation->mImage = theImage;
		anOperation->mX = (double)(Ftol(g->mTransX) + theX);
		anOperation->mY = (double)(Ftol(g->mTransY) + theY);
		anOperation->mWidth = (double)theRotCenterX;
		anOperation->mHeight = (double)theRotCenterY;
		anOperation->mRot = (float)theRot;
		Rect aWhole(0, 0, theImage->mWidth, theImage->mHeight);
		anOperation->mSrcRect = theSrcRect != nullptr ? *theSrcRect : aWhole;
	}
}

void GraphicsRecorder::DrawImageTransform(Graphics* g, Image* theImage, const Transform& theTransform,
	const Rect& theSrcRect, float theX, float theY, bool theUseFloat)
{
	// The exe repeats Graphics::DrawImageTransformHelper here, as it runs without Direct3D, so that it can record the
	// simple cases. The cases it hands to the Graphics are drawn even while the draws are skipped (but the matrix).
	// port(mods): the fixes mod's "rewindSkips" (NOTES.md, "The fixes mod") skips them too
	bool aSkipAll = mSkipDraw && ModFeatureBool("fixes", "rewindSkips", false);
	if (theTransform.mComplex) {
		if (!mSkipDraw)
			g->DrawImageMatrix(theImage, theTransform.GetMatrix(), theSrcRect, theX, theY);
		return;
	}
	float aHalfWidth = (float)theSrcRect.mWidth * 0.5f;
	float aHalfHeight = (float)theSrcRect.mHeight * 0.5f;
	if (theTransform.mHaveRot) {
		float aCenterX = aHalfWidth - theTransform.mTransX1;
		float aCenterY = aHalfHeight - theTransform.mTransY1;
		float aX = theTransform.mTransX2 + theX - aCenterX + 0.5f;
		float aY = theTransform.mTransY2 + theY - aCenterY + 0.5f;
		if (theUseFloat) {
			if (aSkipAll)
				return;
			g->DrawImageRotatedF(theImage, aX, aY, (double)theTransform.mRot, aCenterX, aCenterY, &theSrcRect);
		} else
			DrawImageRotated(g, theImage, Ftol(aX), Ftol(aY), (double)theTransform.mRot, Ftol(aCenterX),
				Ftol(aCenterY), &theSrcRect);
	} else if (theTransform.mHaveScale) {
		bool aMirror = false;
		if (theTransform.mScaleX == -1.0f) {
			if (theTransform.mScaleY == 1.0f) {
				float aY = theTransform.mTransY1 + theY + theTransform.mTransY2 - aHalfHeight + 0.5f;
				float aX = theTransform.mTransX1 + theX + theTransform.mTransX2 - aHalfWidth + 0.5f;
				if (aSkipAll)
					return;
				g->DrawImageMirror(theImage, Ftol(aX), Ftol(aY), theSrcRect, true);
				return;
			}
			aMirror = true;
		}
		float aScaledHalfWidth = theTransform.mScaleX * aHalfWidth;
		float aScaledHalfHeight = theTransform.mScaleY * aHalfHeight;
		float aX = theTransform.mTransX2 + theX - aScaledHalfWidth;
		float aY = theTransform.mTransY2 + theY - aScaledHalfHeight;
		Rect aDestRect(Ftol(aX), Ftol(aY), Ftol((double)aScaledHalfWidth * 2.0),
			Ftol((double)aScaledHalfHeight * 2.0));
		if (aMirror) {
			if (!aSkipAll)
				g->DrawImageMirror(theImage, aDestRect, theSrcRect, true);
		} else
			DrawImage(g, theImage, aDestRect, theSrcRect);
	} else {
		float aX = theTransform.mTransX1 + theX + theTransform.mTransX2 - aHalfWidth + 0.5f;
		float aY = theTransform.mTransY1 + theY + theTransform.mTransY2 - aHalfHeight + 0.5f;
		if (theUseFloat) {
			if (aSkipAll)
				return;
			g->DrawImageF(theImage, aX, aY, theSrcRect);
		} else
			DrawImage(g, theImage, Ftol(aX), Ftol(aY), theSrcRect);
	}
}
