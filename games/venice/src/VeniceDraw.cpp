// Drawing and overlap tests (0x438db0-0x43a740, 0x441650-0x442080).

#include "VeniceDraw.h"

#include "Color.h"
#include "DDImage.h"
#include "Graphics.h"
#include "ImageRecord.h"
#include "Level.h"
#include "SceneLoader.h"
#include "SexyMatrix.h"
#include "VeniceApp.h"
#include "VeniceGlobals.h"
#include "VeniceMath.h"

// 0x441880
bool IsAccelerated()
{
	return gApp->Is3DAccelerated();
}

// 0x441a20
bool IsWindowedOnScreen()
{
	return gApp->mIsWindowed;
}

// 0x4418a0
void SetAcceleratedDisplay(bool theAccelerated)
{
	if (theAccelerated) {
		if (!gApp->Is3DAccelerationSupported()) {
			PushSceneByName("3d_not_supported");
			return;
		}
		if (!gApp->Is3DAccelerationRecommended()) {
			PushSceneByName("3d_not_recommended");
			return;
		}
	}
	gApp->Set3DAcclerated(theAccelerated, true);
}

// 0x441930 (the original also refuses a window on a desktop that is neither 16 nor 32 bits deep, which no desktop the port runs
// on is)
void SetWindowedDisplay(bool theWindowed)
{
	if ((float)gApp->mDesktopWidth <= gScreenWidth && (float)gApp->mDesktopHeight <= gScreenHeight) {
		if (gScenes.mHead != nullptr)
			PushSceneByName("no_windowed");
		return;
	}
	gApp->SwitchScreenMode(theWindowed, IsAccelerated(), true);
	if (gScenes.mTail != nullptr) {
		Level* aTopScene = static_cast<Level*>(gScenes.mTail);
		gApp->WarpMouse((int)aTopScene->mMouseX, (int)aTopScene->mMouseY);
	}
}

// 0x441de0: a style other than 0 and 1 changes nothing
void SetBlitStyleOnTarget(int theBlitStyle)
{
	if (theBlitStyle == 0)
		gGraphics->SetDrawMode(Sexy::Graphics::DRAWMODE_NORMAL);
	else if (theBlitStyle == 1)
		gGraphics->SetDrawMode(Sexy::Graphics::DRAWMODE_ADDITIVE);
}

// 0x441a60
void SetDrawClip(int theLeft, int theTop, int theRight, int theBottom)
{
	float aHeight = (float)(theBottom - theTop) <= gScreenHeight ? (float)(theBottom - theTop) : gScreenHeight;
	float aWidth = (float)(theRight - theLeft) <= gScreenWidth ? (float)(theRight - theLeft) : gScreenWidth;
	gGraphics->SetClipRect(theLeft < 0 ? 0 : theLeft, theTop < 0 ? 0 : theTop, (int)aWidth, (int)aHeight);
}

// 0x442080: a colour channel (0 to 1) as a byte, the way the original converts it: the value is cut off at 0 and 255 only when the
// colour is not plain white
static int ColorChannel(float theValue)
{
	return (int)((double)theValue * 255.0);
}

// 0x442080: a channel cut off at 0 and 255
static int ClampChannel(int theChannel)
{
	if (theChannel > 0xff)
		theChannel = 0xff;
	return theChannel < 0 ? 0 : theChannel;
}

// 0x442490: the picture drawn into the render target (the layer merge's tall pictures) with a graphics of its own on the target.
// The original keeps this as a second copy of the draw below, with its own differences: the offset of one pixel is always
// there (on an accelerated display too), a picture wider than 700 pixels is read one column wider (and a flip of the height
// shifts it a column left, which the original's test of that flag does), and the picture is transformed whenever the display
// is accelerated.
static void DrawImageRecordOnTarget(ImageRecord* theImage, float theSourceLeft, float theSourceTop, float theSourceRight,
	float theSourceBottom, float theX, float theY, float theRotation, float theXScale, float theYScale, float theRed,
	float theGreen, float theBlue, float theAlpha, bool theFlipX, bool theFlipY)
{
	Sexy::Graphics aGraphics(gRenderTarget);
	aGraphics.SetClipRect(0, 0, gRenderTarget->mWidth, gRenderTarget->mHeight);
	int aRed = ColorChannel(theRed);
	int aGreen = ColorChannel(theGreen);
	int aBlue = ColorChannel(theBlue);
	int anAlpha = ColorChannel(theAlpha);
	float aXScale = theFlipX ? theXScale * -1.0f : theXScale;
	float aYScale = theFlipY ? theYScale * -1.0f : theYScale;
	if (aRed == 0xff && aGreen == 0xff && aBlue == 0xff && anAlpha == 0xff) {
		aGraphics.SetColorizeImages(false);
	} else {
		aGraphics.SetColorizeImages(true);
		aGraphics.SetColor(Sexy::Color(ClampChannel(aRed), ClampChannel(aGreen), ClampChannel(aBlue), ClampChannel(anAlpha)));
	}
	theX = theX + 1.0f;
	theY = theY + 1.0f;
	int anExtraWidth = 0;
	if (700.0f < theSourceRight - theSourceLeft) {
		if (theFlipY)
			theX = theX - 1.0f;
		anExtraWidth = 1;
	}
	Sexy::Rect aSourceRect((int)theSourceLeft, (int)theSourceTop, ((int)theSourceRight - (int)theSourceLeft) + anExtraWidth,
		(int)theSourceBottom - (int)theSourceTop);
	if (aXScale == 1.0f && aYScale == 1.0f && theRotation == 0.0f && !IsAccelerated()) {
		aGraphics.DrawImageMirror(theImage->mImage, (int)theX, (int)theY, aSourceRect, theFlipX);
	} else {
		Sexy::SexyTransform2D aTransform;
		aTransform.LoadIdentity();
		aTransform.Scale(aXScale, aYScale);
		aTransform.RotateDeg(theRotation);
		aTransform.Translate((float)((double)(theSourceRight - theSourceLeft) / 2.0 + 0.0),
			(float)((double)(theSourceBottom - theSourceTop) / 2.0 + 0.0));
		aGraphics.DrawImageMatrix(theImage->mImage, aTransform, aSourceRect, theX, theY);
	}
}

// 0x442080
void DrawImageRecord(ImageRecord* theImage, float theSourceLeft, float theSourceTop, float theSourceRight,
	float theSourceBottom, float theX, float theY, float theRotation, float theXScale, float theYScale, float theRed,
	float theGreen, float theBlue, float theAlpha, bool theFlipX, bool theFlipY)
{
	if (gRenderTarget != nullptr) {
		DrawImageRecordOnTarget(theImage, theSourceLeft, theSourceTop, theSourceRight, theSourceBottom, theX, theY, theRotation,
			theXScale, theYScale, theRed, theGreen, theBlue, theAlpha, theFlipX, theFlipY);
		return;
	}
	bool anAccelerated = IsAccelerated();
	float anAcceleratedOffset = anAccelerated ? 1.0f : 0.0f;
	int aRed = ColorChannel(theRed);
	int aGreen = ColorChannel(theGreen);
	int aBlue = ColorChannel(theBlue);
	int anAlpha = ColorChannel(theAlpha);
	float aXScale = theFlipX ? theXScale * -1.0f : theXScale;
	float aYScale = theFlipY ? theYScale * -1.0f : theYScale;
	if (aRed == 0xff && aGreen == 0xff && aBlue == 0xff && anAlpha == 0xff) {
		gGraphics->SetColorizeImages(false);
	} else {
		gGraphics->SetColorizeImages(true);
		gGraphics->SetColor(Sexy::Color(ClampChannel(aRed), ClampChannel(aGreen), ClampChannel(aBlue), ClampChannel(anAlpha)));
	}
	if (!anAccelerated) {
		theX = theX + 1.0f;
		theY = theY + 1.0f;
	}
	Sexy::Rect aSourceRect((int)theSourceLeft, (int)theSourceTop, (int)theSourceRight - (int)theSourceLeft,
		(int)theSourceBottom - (int)theSourceTop);
	if (aXScale == 1.0f && aYScale == 1.0f && theRotation == 0.0f && !IsAccelerated()) {
		gGraphics->DrawImageMirror(theImage->mImage, (int)theX, (int)theY, aSourceRect, theFlipX);
	} else {
		Sexy::SexyTransform2D aTransform;
		aTransform.LoadIdentity();
		aTransform.Scale(aXScale, aYScale);
		aTransform.RotateDeg(theRotation);
		aTransform.Translate((float)((double)(theSourceRight - theSourceLeft) / 2.0 + (double)anAcceleratedOffset * 0.5),
			(float)((double)(theSourceBottom - theSourceTop) / 2.0 + (double)anAcceleratedOffset * 0.5));
		// phase two, found by comparing the loading screen's sparkles (blit style 1, scaled) with the original's window: its
		// software surface blends a transformed picture normally whatever the draw mode says; only the accelerated target
		// adds. (The draw mode stays what the scene set, for the pictures that are not transformed.)
		int aSavedMode = gGraphics->GetDrawMode();
		if (!anAccelerated)
			gGraphics->SetDrawMode(Sexy::Graphics::DRAWMODE_NORMAL);
		gGraphics->DrawImageMatrix(theImage->mImage, aTransform, aSourceRect, theX, theY);
		gGraphics->SetDrawMode(aSavedMode);
	}
}

// 0x4416c0: the rectangle between the corners, filled with one colour and outlined in another (0 to 1 each)
void DrawBox(int theLeft, int theTop, int theRight, int theBottom, float theFillRed, float theFillGreen, float theFillBlue,
	float theFillAlpha, float theLineRed, float theLineGreen, float theLineBlue, float theLineAlpha)
{
	gGraphics->SetColor(Sexy::Color((int)(theFillRed * 255.0), (int)(theFillGreen * 255.0), (int)(theFillBlue * 255.0),
		(int)(theFillAlpha * 255.0)));
	gGraphics->FillRect(theLeft, theTop, theRight - theLeft, theBottom - theTop);
	gGraphics->SetColor(Sexy::Color((int)(theLineRed * 255.0), (int)(theLineGreen * 255.0), (int)(theLineBlue * 255.0),
		(int)(theLineAlpha * 255.0)));
	gGraphics->DrawRect(theLeft, theTop, theRight - theLeft, theBottom - theTop);
}

// 0x441650
void FillScreenBlack()
{
	DrawBox(0, 0, (int)gScreenWidth - 1, (int)gScreenHeight - 1, 0, 0, 0, 1, 0, 0, 0, 1);
}

// 0x438db0
bool RectsOverlap(float theX1, float theY1, float theWidth1, float theHeight1, float theX2, float theY2, float theWidth2,
	float theHeight2)
{
	return VeniceAbs(theX1 - theX2) <= (double)theWidth2 * 0.5 + (double)theWidth1 * 0.5 &&
		VeniceAbs(theY1 - theY2) <= (double)theHeight2 * 0.5 + (double)theHeight1 * 0.5;
}

// 0x43a740: whether the point is inside the polygon (even-odd rule on the edges that cross the point's row)
static bool PointInPolygon(int theCornerCount, const float* theXs, const float* theYs, float theX, float theY)
{
	bool anInside = false;
	int aPrevious = theCornerCount - 1;
	for (int aCorner = 0; aCorner < theCornerCount; aCorner++) {
		if (((theYs[aCorner] <= theY && theY < theYs[aPrevious]) || (theYs[aPrevious] <= theY && theY < theYs[aCorner])) &&
			theX < ((theXs[aPrevious] - theXs[aCorner]) * (theY - theYs[aCorner])) / (theYs[aPrevious] - theYs[aCorner]) +
				theXs[aCorner])
			anInside = !anInside;
		aPrevious = aCorner;
	}
	return anInside;
}

// 0x438ff0: the corners of a rectangle turned about its centre, in the order top left, top right, bottom right, bottom left (the
// original's x87 sums are kept in a double between the rounding to float that its stores to locals make)
static void RotatedCorners(float theX, float theY, float theWidth, float theHeight, float theRotation, float* theXs,
	float* theYs)
{
	float anAngle = (0.0f - theRotation) * DegreesToRadians;
	double aCos = VeniceCos(anAngle);
	double aSin = VeniceSin(anAngle);
	static const int aSignX[4] = {-1, 1, 1, -1};
	static const int aSignY[4] = {-1, -1, 1, 1};
	for (int aCorner = 0; aCorner < 4; aCorner++) {
		double aOffsetX = (double)aSignX[aCorner] * (double)theWidth / 2.0;
		double aOffsetY = (double)aSignY[aCorner] * (double)theHeight / 2.0;
		float aPartX = (float)(aOffsetX * aCos + (double)theX);
		theXs[aCorner] = (float)((double)aPartX - aOffsetY * aSin);
		float aPartY = (float)(aOffsetX * aSin + (double)theY);
		theYs[aCorner] = (float)(aOffsetY * aCos + (double)aPartY);
	}
}

// 0x438ff0
bool RotatedRectsOverlap(float theX1, float theY1, float theWidth1, float theHeight1, float theRotation1, float theX2,
	float theY2, float theWidth2, float theHeight2, float theRotation2)
{
	float aXs1[4];
	float aYs1[4];
	float aXs2[4];
	float aYs2[4];
	RotatedCorners(theX1, theY1, theWidth1, theHeight1, theRotation1, aXs1, aYs1);
	RotatedCorners(theX2, theY2, theWidth2, theHeight2, theRotation2, aXs2, aYs2);
	bool anOverlap = false;
	for (int aCorner = 0; aCorner < 4; aCorner++)
		anOverlap = PointInPolygon(4, aXs1, aYs1, aXs2[aCorner], aYs2[aCorner]) || anOverlap;
	for (int aCorner = 0; aCorner < 4; aCorner++)
		anOverlap = PointInPolygon(4, aXs2, aYs2, aXs1[aCorner], aYs1[aCorner]) || anOverlap;
	return anOverlap;
}
