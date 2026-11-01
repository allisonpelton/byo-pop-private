// The game's small helpers (TwistUtil.h).
#include "TwistUtil.h"

#include "Common.h"
#include "GraphicsRecorder.h"
#include "Image.h"
#include "ImageFont.h"
#include "SexyMatrix.h"
#include "X86Math.h"

using namespace Sexy;

float Sexy::RandSigned()
{
	return (float)((Rand() % 1000000 - 500000) / 500000.0);
}

void Sexy::DrawImageScaledCentered(Graphics* g, Image* theImage, float theX, float theY, int theAlign,
	float theScaleX, float theScaleY)
{
	if (theScaleX != 1.0f && theScaleY != 1.0f && (theAlign & ALIGN_CENTER) != 0) {
		Transform aTransform;
		aTransform.Scale(theScaleX, theScaleY);
		aTransform.Translate(theX, theY);
		gGraphicsRecorder->DrawImageTransform(g, theImage, aTransform, Rect(0, 0, theImage->mWidth, theImage->mHeight),
			0, 0, true);
		return;
	}
	float aWidth = (float)theImage->mWidth * theScaleX;
	float aHeight = (float)theImage->mHeight * theScaleY;
	if ((theAlign & ALIGN_CENTER_X) != 0)
		theX -= aWidth * 0.5f;
	else if ((theAlign & ALIGN_RIGHT) != 0)
		theX -= aWidth;
	if ((theAlign & ALIGN_CENTER_Y) != 0)
		theY -= aHeight * 0.5f;
	else if ((theAlign & ALIGN_BOTTOM) != 0)
		theY -= aHeight;
	if (theScaleX == 1.0f && theScaleY == 1.0f) {
		// The original draws at a fraction of a pixel (DrawImageF) only with Direct3D
		gGraphicsRecorder->DrawImage(g, theImage, Ftol(theX), Ftol(theY));
	} else {
		gGraphicsRecorder->DrawImage(g, theImage, Ftol(theX), Ftol(theY), Ftol(aWidth), Ftol(aHeight));
	}
}

void Sexy::DrawImageScaledCentered(Graphics* g, Image* theImage, float theX, float theY, float theScaleX,
	float theScaleY)
{
	DrawImageScaledCentered(g, theImage, theX, theY, ALIGN_CENTER, theScaleX, theScaleY);
}

void Sexy::DrawImageCentered(Graphics* g, Image* theImage, const Rect& theSrcRect, float theX, float theY,
	double theRot, float theScaleX, float theScaleY)
{
	Transform aTransform;
	if (theRot == 0.0) {
		int aWidth = Ftol((double)theImage->GetCelWidth() * theScaleX);
		int aHeight = Ftol((double)theImage->GetCelHeight() * theScaleY);
		Rect aDestRect(Ftol((double)theX - aWidth / 2), Ftol((double)theY - aHeight / 2), aWidth, aHeight);
		if (theScaleX == 1.0f && theScaleY == 1.0f)
			gGraphicsRecorder->DrawImage(g, theImage, aDestRect.mX, aDestRect.mY, theSrcRect);
		else
			gGraphicsRecorder->DrawImage(g, theImage, aDestRect, theSrcRect);
		return;
	}
	if (theScaleX != 1.0f || theScaleY != 1.0f)
		aTransform.Scale(theScaleX, theScaleY);
	aTransform.RotateRad((float)theRot);
	gGraphicsRecorder->DrawImageTransform(g, theImage, aTransform, theSrcRect, theX, theY, false);
}

void Sexy::DrawImageCentered(Graphics* g, Image* theImage, float theX, float theY, double theRot, float theScaleX,
	float theScaleY)
{
	DrawImageCentered(g, theImage, Rect(0, 0, theImage->mWidth, theImage->mHeight), theX, theY, theRot, theScaleX,
		theScaleY);
}

void Sexy::SetFontLayerColor(Font* theFont, int theLayer, const Color& theColor)
{
	ImageFont* aFont = dynamic_cast<ImageFont*>(theFont);
	if (aFont == nullptr)
		return;
	int i = 0;
	for (FontLayer& aLayer : aFont->mFontData->mFontLayerList) {
		if (i == theLayer) {
			aLayer.mColorMult = theColor;
			return;
		}
		i++;
	}
}

Color Sexy::GetFontLayerColor(Font* theFont, int theLayer)
{
	// port: the original reads the layer list of any font as an ImageFont's; the game asks only ImageFonts
	ImageFont* aFont = dynamic_cast<ImageFont*>(theFont);
	if (aFont == nullptr)
		return Color(0, 0, 0, 0);
	int i = 0;
	for (FontLayer& aLayer : aFont->mFontData->mFontLayerList) {
		if (i == theLayer)
			return aLayer.mColorMult;
		i++;
	}
	return Color(0, 0, 0, 0);
}

void Sexy::SetFontLayerColor(Font* theFont, const std::string& theName, const Color& theColor)
{
	ImageFont* aFont = dynamic_cast<ImageFont*>(theFont);
	if (aFont == nullptr)
		return;
	// The original compares the name of each layer the font draws; the framework keeps the names only in its map
	for (ActiveFontLayer& anActive : aFont->mActiveLayerList) {
		for (auto& anEntry : aFont->mFontData->mFontLayerMap) {
			if (anEntry.second == anActive.mBaseFontLayer && _stricmp(anEntry.first.c_str(), theName.c_str()) == 0) {
				anActive.mBaseFontLayer->mColorMult = theColor;
				return;
			}
		}
	}
}
