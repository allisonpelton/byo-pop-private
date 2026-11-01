// Sexy::GraphicsRecorder (GraphicsRecorder.h).
#include "GraphicsRecorder.h"

#include "Graphics.h"
#include "SexyMatrix.h"

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
	if (theUseFloat)
		g->DrawImageTransformF(theImage, theTransform, theSrcRect, theX, theY);
	else
		g->DrawImageTransform(theImage, theTransform, theSrcRect, theX, theY);
}
