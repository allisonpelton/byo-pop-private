// The game's small helpers (TwistUtil.h).
#include "TwistUtil.h"

#include "Common.h"
#include "GraphicsRecorder.h"
#include "Image.h"
#include "ImageFont.h"
#include "SexyMatrix.h"
#include "TwistD3D.h"
#include "X86Math.h"

#include <SDL.h>

#include <cmath>
#include <set>

using namespace Sexy;

void Sexy::NotPorted(const char* theWhat)
{
	static std::set<std::string> sSaid;
	if (sSaid.insert(theWhat).second)
		SDL_Log("%s is not ported yet", theWhat);
}

float Sexy::gSinTable[4096];
float Sexy::gCosTable[4096];

// part of the WinTwistApp constructor (0x766ce7): the sine and cosine tables, made once
void Sexy::MakeTrigTables()
{
	static bool sMade = false;  // 0xe0b691
	if (sMade)
		return;
	int i = 0;
	do {
		double anAngle = ((double)i * 3.14159 + (double)i * 3.14159) * 0.000244140625;
		gSinTable[i] = (float)sin(anAngle);
		gCosTable[i] = (float)cos(anAngle);
		i++;
	} while (i < 0x1000);
	sMade = true;
}

float Sexy::RandSigned()
{
	return (float)((Rand() % 1000000 - 500000) / 500000.0);
}

float Sexy::RandFloat()
{
	return (float)((Rand() % 1000000) / 1000000.0);
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
		// At a fraction of a pixel it draws with DrawImageF (0x70dae6), with Direct3D only
		int aWholeX = Ftol(theX);
		if ((theX == (float)aWholeX && (double)theY == (double)Ftol(theY)) || gD3DInterface == nullptr)
			gGraphicsRecorder->DrawImage(g, theImage, aWholeX, Ftol(theY));
		else
			gGraphicsRecorder->DrawImageF(g, theImage, theX, theY);
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

Color Sexy::GetFontLayerColor(Font* theFont, const std::string& theName)
{
	ImageFont* aFont = dynamic_cast<ImageFont*>(theFont);
	if (aFont == nullptr)
		return Color(0, 0, 0, 0);
	for (ActiveFontLayer& anActive : aFont->mActiveLayerList) {
		for (auto& anEntry : aFont->mFontData->mFontLayerMap) {
			if (anEntry.second == anActive.mBaseFontLayer && _stricmp(anEntry.first.c_str(), theName.c_str()) == 0)
				return anActive.mBaseFontLayer->mColorMult;
		}
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

