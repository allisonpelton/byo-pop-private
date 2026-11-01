// The game's small helpers (TwistUtil.h).
#include "TwistUtil.h"

#include "Common.h"
#include "GraphicsRecorder.h"
#include "Image.h"
#include "ImageFont.h"
#include "SexyMatrix.h"
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

namespace {

void CrtSwap(char* theA, char* theB, size_t theWidth)
{
	if (theA == theB)
		return;
	while (theWidth--) {
		char aByte = *theA;
		*theA++ = *theB;
		*theB++ = aByte;
	}
}

// Moves the largest to the end, again and again
void CrtShortSort(char* theLo, char* theHi, size_t theWidth, int (*theCompare)(const void*, const void*))
{
	while (theHi > theLo) {
		char* aMax = theLo;
		for (char* p = theLo + theWidth; p <= theHi; p += theWidth) {
			if (theCompare(p, aMax) > 0)
				aMax = p;
		}
		CrtSwap(aMax, theHi, theWidth);
		theHi -= theWidth;
	}
}

}  // namespace

void Sexy::CrtQsort(void* theBase, size_t theNum, size_t theWidth, int (*theCompare)(const void*, const void*))
{
	if (theNum < 2)
		return;
	char* aLoStack[30];
	char* aHiStack[30];
	int aStackPtr = 0;
	char* aLo = (char*)theBase;
	char* aHi = (char*)theBase + theWidth * (theNum - 1);
	for (;;) {
		size_t aSize = (size_t)(aHi - aLo) / theWidth + 1;
		if (aSize <= 8) {
			CrtShortSort(aLo, aHi, theWidth, theCompare);
		} else {
			// The median of three, then the partition
			char* aMid = aLo + (aSize / 2) * theWidth;
			if (theCompare(aLo, aMid) > 0)
				CrtSwap(aLo, aMid, theWidth);
			if (theCompare(aLo, aHi) > 0)
				CrtSwap(aLo, aHi, theWidth);
			if (theCompare(aMid, aHi) > 0)
				CrtSwap(aMid, aHi, theWidth);
			char* aLoGuy = aLo;
			char* aHiGuy = aHi;
			for (;;) {
				if (aMid > aLoGuy) {
					do {
						aLoGuy += theWidth;
					} while (aLoGuy < aMid && theCompare(aLoGuy, aMid) <= 0);
				}
				if (aMid <= aLoGuy) {
					do {
						aLoGuy += theWidth;
					} while (aLoGuy <= aHi && theCompare(aLoGuy, aMid) <= 0);
				}
				do {
					aHiGuy -= theWidth;
				} while (aHiGuy > aMid && theCompare(aHiGuy, aMid) > 0);
				if (aHiGuy < aLoGuy)
					break;
				CrtSwap(aLoGuy, aHiGuy, theWidth);
				if (aMid == aHiGuy)
					aMid = aLoGuy;
			}
			aHiGuy += theWidth;
			if (aMid < aHiGuy) {
				do {
					aHiGuy -= theWidth;
				} while (aHiGuy > aMid && theCompare(aHiGuy, aMid) == 0);
			}
			if (aMid >= aHiGuy) {
				do {
					aHiGuy -= theWidth;
				} while (aHiGuy > aLo && theCompare(aHiGuy, aMid) == 0);
			}
			// The smaller part next, the larger one on the stack
			if (aHiGuy - aLo >= aHi - aLoGuy) {
				if (aLo < aHiGuy) {
					aLoStack[aStackPtr] = aLo;
					aHiStack[aStackPtr] = aHiGuy;
					aStackPtr++;
				}
				if (aLoGuy < aHi) {
					aLo = aLoGuy;
					continue;
				}
			} else {
				if (aLoGuy < aHi) {
					aLoStack[aStackPtr] = aLoGuy;
					aHiStack[aStackPtr] = aHi;
					aStackPtr++;
				}
				if (aLo < aHiGuy) {
					aHi = aHiGuy;
					continue;
				}
			}
		}
		aStackPtr--;
		if (aStackPtr < 0)
			return;
		aLo = aLoStack[aStackPtr];
		aHi = aHiStack[aStackPtr];
	}
}
