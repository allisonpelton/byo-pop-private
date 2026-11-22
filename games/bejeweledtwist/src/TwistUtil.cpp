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

#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
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


std::string Sexy::CrtFormatFloat(const std::string& theFormat, double theValue)
{
	// the spec: '%', flags, width, '.' precision, 'f' ("%%" is skipped)
	size_t aStart = std::string::npos;
	size_t anEnd = 0;
	bool aLeft = false, aPlus = false, aSpace = false, aZero = false;
	int aWidth = 0, aPrecision = 6;
	for (size_t i = 0; i < theFormat.size(); i++) {
		if (theFormat[i] != '%')
			continue;
		if (i + 1 < theFormat.size() && theFormat[i + 1] == '%') {
			i++;
			continue;
		}
		size_t j = i + 1;
		for (; j < theFormat.size() && std::strchr("-+ #0", theFormat[j]) != nullptr; j++) {
			aLeft |= theFormat[j] == '-';
			aPlus |= theFormat[j] == '+';
			aSpace |= theFormat[j] == ' ';
			aZero |= theFormat[j] == '0';
		}
		for (; j < theFormat.size() && std::isdigit((unsigned char)theFormat[j]); j++)
			aWidth = aWidth * 10 + (theFormat[j] - '0');
		if (j < theFormat.size() && theFormat[j] == '.') {
			aPrecision = 0;
			for (j++; j < theFormat.size() && std::isdigit((unsigned char)theFormat[j]); j++)
				aPrecision = aPrecision * 10 + (theFormat[j] - '0');
		}
		if (j < theFormat.size() && theFormat[j] == 'f') {
			aStart = i;
			anEnd = j + 1;
		}
		break;
	}
	if (aStart == std::string::npos || !std::isfinite(theValue))
		return StrFormat(theFormat.c_str(), theValue);

	// 17 significant digits, correctly rounded, then rounded half up at the precision
	bool aNegative = std::signbit(theValue);
	char aBuf[40];
	std::snprintf(aBuf, sizeof(aBuf), "%.16e", std::fabs(theValue));
	std::string aDigits;
	const char* p = aBuf;
	for (; *p != 'e'; p++)
		if (std::isdigit((unsigned char)*p))
			aDigits += *p;
	int aPoint = std::atoi(p + 1) + 1;  // digits before the point
	if (theValue == 0)
		aPoint = 1;
	int aKeep = aPoint + aPrecision;
	if (aKeep < 0) {
		aDigits = "0";
		aPoint = 1;
	} else {
		bool anUp = aKeep < (int)aDigits.size() && aDigits[aKeep] >= '5';
		aDigits.resize(aKeep < (int)aDigits.size() ? aKeep : aDigits.size());
		while ((int)aDigits.size() < aKeep)
			aDigits += '0';
		if (anUp) {
			int k = (int)aDigits.size() - 1;
			for (; k >= 0 && aDigits[k] == '9'; k--)
				aDigits[k] = '0';
			if (k >= 0)
				aDigits[k]++;
			else {
				aDigits.insert(aDigits.begin(), '1');
				aPoint++;
			}
		}
	}
	// the digits about the point
	std::string aNumber;
	if (aPoint <= 0)
		aNumber = "0";
	else
		aNumber = aDigits.substr(0, aPoint);
	if (aPrecision > 0) {
		aNumber += '.';
		for (int k = 0; k < aPrecision; k++) {
			int anIndex = aPoint + k;
			aNumber += (anIndex >= 0 && anIndex < (int)aDigits.size()) ? aDigits[anIndex] : '0';
		}
	}
	std::string aSign = aNegative ? "-" : aPlus ? "+" : aSpace ? " " : "";
	int aPad = aWidth - (int)(aSign.size() + aNumber.size());
	std::string aField;
	if (aPad <= 0)
		aField = aSign + aNumber;
	else if (aLeft)
		aField = aSign + aNumber + std::string(aPad, ' ');
	else if (aZero)
		aField = aSign + std::string(aPad, '0') + aNumber;
	else
		aField = std::string(aPad, ' ') + aSign + aNumber;
	std::string aPrefix = theFormat.substr(0, aStart);
	std::string aSuffix = theFormat.substr(anEnd);
	// the text about the spec keeps StrFormat's handling of "%%"
	return StrFormat(aPrefix.c_str()) + aField + StrFormat(aSuffix.c_str());
}
