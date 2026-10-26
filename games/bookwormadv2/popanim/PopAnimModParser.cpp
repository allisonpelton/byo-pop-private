#include "PopAnimModParser.h"

#include <stdlib.h>
#include <algorithm>

#include "Common.h"
#include "MemoryImage.h"
#include "PopAnim.h"
#include "PopAnimNames.h"

using namespace Sexy;

// 0x529e30
PopAnimModParser::PopAnimModParser(PopAnim* thePopAnim, const std::string& theFileName)
{
	mCmdSep = CMDSEP_NO_INDENT;  // a command to a line
	mMode = 1;
	mPopAnim = thePopAnim;
	mFileName = theFileName;
}

// 0x529ee0
bool PopAnimModParser::Error(const std::string& theError)
{
	std::string aMessage = "PopAnim mod file " + mFileName;
	if (mCurrentLine.size() > 0)
		aMessage += StrFormat(", line %d (", mCurrentLineNum) + mCurrentLine + ")";
	aMessage += ": " + theError;
	mPopAnim->SetError(aMessage);
	return DescParser::Error(theError);
}

// 0x52a070: sets the argument of the commands called theName in a sprite's frames to theValue. theCount says
// which: -1 all of them, otherwise the one that many matches in (1 is the first) counted from where the count
// stands, which carries over from one sprite to the next.
static void SetParam(PASpriteDef& theDef, const std::string& theName, int* theCount, const std::string& theValue)
{
	if (*theCount == 0)
		return;
	for (size_t f = 0; f < theDef.mFrames.size(); f++) {
		std::vector<PACommand>& aCommands = theDef.mFrames[f].mCommands;
		for (size_t c = 0; c < aCommands.size(); c++) {
			if (!PopAnimNameMatches(aCommands[c].mCommand, theName))
				continue;
			if (*theCount <= 1)
				aCommands[c].mParam = theValue;
			if (--*theCount == 0)
				return;
		}
	}
}

// 0x54d2a0: multiplies every pixel by theColor (255 for no change, though not exactly: the usual case, a color
// whose parts are all 255 at most, divides by 256). An image with a palette has only its palette changed.
static void ColorizeImage(MemoryImage* theImage, const Color& theColor)
{
	ulong* aPixels;
	int aCount;
	if (theImage->mColorTable != NULL) {
		aPixels = theImage->mColorTable;
		aCount = 256;
	}
	else {
		aPixels = theImage->GetBits();
		aCount = theImage->mWidth * theImage->mHeight;
	}
	if (theColor.mAlpha <= 0xff && theColor.mRed <= 0xff && theColor.mGreen <= 0xff && theColor.mBlue <= 0xff) {
		for (int i = 0; i < aCount; i++) {
			unsigned int p = (unsigned int)aPixels[i];
			unsigned int aRG = (((p & 0xff0000) * theColor.mRed) & 0xff0000ff) | (((p & 0xff00) * theColor.mGreen) & 0xff0000);
			unsigned int aB = ((p & 0xff) * theColor.mBlue) & 0xff00;
			unsigned int anA = (((p >> 8) & 0xff0000) * theColor.mAlpha) & 0xff000000;
			aPixels[i] = ((aRG | aB) >> 8) | anA;
		}
	}
	else {
		for (int i = 0; i < aCount; i++) {
			unsigned int p = (unsigned int)aPixels[i];
			unsigned int anA = ((p >> 24) * theColor.mAlpha) / 255;
			unsigned int aR = (((p >> 16) & 0xff) * theColor.mRed) / 255;
			unsigned int aG = (((p >> 8) & 0xff) * theColor.mGreen) / 255;
			unsigned int aB = ((p & 0xff) * theColor.mBlue) / 255;
			anA = std::min(anA, 255u);
			aR = std::min(aR, 255u);
			aG = std::min(aG, 255u);
			aB = std::min(aB, 255u);
			aPixels[i] = (((anA << 8 | aR) << 8 | aG) << 8) | aB;
		}
	}
	theImage->BitsChanged();
}

// 0x54d990: turns the hue of every pixel by theShift, in 256ths of a turn. The pixel goes to hue, saturation and
// lightness (the last two in 256ths) and back, in whole numbers but for the last step.
static void HueShiftImage(MemoryImage* theImage, int theShift)
{
	if (theShift < 0)
		theShift = theShift + 0x100 + (int)((unsigned int)(-theShift - 1) & 0xffffff00u);
	int aCount = theImage->mHeight * theImage->mWidth;
	ulong* aBits = theImage->GetBits();
	for (int i = 0; i < aCount; i++) {
		unsigned int p = (unsigned int)aBits[i];
		int aR = (p >> 16) & 0xff;
		int aG = (p >> 8) & 0xff;
		int aB = p & 0xff;
		int aMax = std::max(aR, std::max(aG, aB));
		int aMin = std::min(aR, std::min(aG, aB));
		int aLight = (aMax + aMin) / 2;
		int aDelta = aMax - aMin;
		int aSat = 0;
		int aHue = 0;
		if (aDelta != 0) {
			aSat = (aDelta << 8) / (aLight > 0x80 ? 0x200 - aMin - aMax : aMin + aMax);
			int aSector;  // 6 times the hue, 0x100 to a sector
			if (aR == aMax)
				aSector = aG == aMin ? ((aMax - aB) << 8) / aDelta + 0x500 : ((aG - aMax) << 8) / aDelta + 0x100;
			else if (aG == aMax)
				aSector = aB == aMin ? ((aMax - aR) << 8) / aDelta + 0x100 : ((aB - aMax) << 8) / aDelta + 0x300;
			else
				aSector = aR == aMin ? ((aMax - aG) << 8) / aDelta + 0x300 : ((aR - aMax) << 8) / aDelta + 0x500;
			aHue = aSector / 6;
		}
		aHue += theShift;
		if (aHue >= 0x100)
			aHue -= 0x100;

		// back to red, green and blue: q the larger of the pixel's parts, p the smaller, and the third between
		int q = aLight < 0x80 ? ((aSat + 0xff) * aLight) / 255 : aLight - (aLight * aSat) / 255 + aSat;
		int lo = 2 * aLight - q;
		int aSextant = (6 * aHue + (aHue < 0 ? 0xff : 0)) >> 8;
		int aFrac = 6 * (aHue - ((aSextant << 8) / 6));
		double d = (double)aFrac * (double)(q - lo) / 255.0;
		int aUp = std::min((int)(lo + d), 255);
		int aDown = std::max((int)(q - d), 0);
		int r, g, b;
		switch (aSextant) {
			case 1: r = aDown; g = q; b = lo; break;
			case 2: r = lo; g = q; b = aUp; break;
			case 3: r = lo; g = aDown; b = q; break;
			case 4: r = aUp; g = lo; b = q; break;
			case 5: r = q; g = lo; b = aDown; break;
			default: r = q; g = aUp; b = lo; break;
		}
		aBits[i] = (p & 0xff000000) | ((unsigned int)r << 16) | ((unsigned int)g << 8) | (unsigned int)b;
	}
	theImage->BitsChanged();
}

// 0x52a190: theParams are the command and its arguments. A command that is not for this pass is skipped.
bool PopAnimModParser::HandleCommand(const ListDataElement& theParams)
{
	const ElementVector& aVec = theParams.mElementVector;
	if (aVec.empty() || aVec[0]->mIsList)
		return Error("Missing command");
	std::string aCommand = ((SingleDataElement*)aVec[0])->mString;
	int aNumParams = (int)aVec.size() - 1;

	if (aCommand == "SetPamFile") {
		if (mMode != 1)
			return true;
		if (aNumParams != 1)
			return Error("Wrong number of arguments");
		// the first one stands
		if (mPopAnim->mPamFile.empty() && !DataToString(aVec[1], &mPopAnim->mPamFile))
			return Error("The argument is not text");
		return true;
	}

	if (aCommand == "Remap") {
		if (mMode != 1)
			return true;
		if (aNumParams != 2)
			return Error("Wrong number of arguments");
		std::string aPattern, aNew;
		if (!DataToString(aVec[1], &aPattern) || !DataToString(aVec[2], &aNew))
			return Error("The arguments are not text");
		mPopAnim->mRemaps.push_back(std::make_pair(aPattern, aNew));
		return true;
	}

	if (aCommand == "Colorize" || aCommand == "HueShift") {
		if (mMode != 2)
			return true;
		if (aNumParams != 2)
			return Error("Wrong number of arguments");
		std::string aPattern;
		if (!DataToString(aVec[1], &aPattern))
			return Error("The first argument is not text");
		bool aColorize = aCommand == "Colorize";
		// the argument: a color (3 or 4 numbers) or a shift of the hue
		Color aColor;
		int aShift = 0;
		if (aColorize) {
			IntVector aValues;
			if (!DataToIntVector(aVec[2], &aValues) || (aValues.size() != 3 && aValues.size() != 4))
				return Error("The color is not 3 or 4 numbers");
			aColor = aValues.size() == 3 ? Color(aValues[0], aValues[1], aValues[2]) : Color(aValues[0], aValues[1], aValues[2], aValues[3]);
		}
		else if (!DataToInt(aVec[2], &aShift))
			return Error("The shift is not a number");

		// every image whose name matches (the images themselves are changed, whoever else shows them)
		bool aFound = false;
		for (size_t i = 0; i < mPopAnim->mImages.size(); i++) {
			MemoryImage* anImage = mPopAnim->mImages[i].mImage;
			if (anImage == NULL || !PopAnimNameMatches(mPopAnim->mImages[i].mName, aPattern))
				continue;
			if (aColorize)
				ColorizeImage(anImage, aColor);
			else
				HueShiftImage(anImage, aShift);
			aFound = true;
		}
		if (!aFound)
			return Error("Unable to find the image");
		return true;
	}

	if (aCommand == "SetParam") {
		if (mMode != 2)
			return true;
		if (aNumParams != 2)
			return Error("Wrong number of arguments");
		std::string aName, aValue;
		if (!DataToString(aVec[1], &aName) || !DataToString(aVec[2], &aValue))
			return Error("The arguments are not text");
		// "name[n]": only the nth command of that name, in all the sprites in turn (the main one first)
		int aCount = -1;
		size_t aBracket = aName.find('[');
		if (aBracket != std::string::npos) {
			aCount = atol(aName.c_str() + aBracket + 1);
			aName = aName.substr(0, aBracket);
		}
		if (mPopAnim->mData != NULL) {
			SetParam(mPopAnim->mData->mMain, aName, &aCount, aValue);
			for (size_t i = 0; i < mPopAnim->mData->mSprites.size(); i++)
				SetParam(mPopAnim->mData->mSprites[i], aName, &aCount, aValue);
		}
		return true;
	}

	return Error("Unknown command");
}
