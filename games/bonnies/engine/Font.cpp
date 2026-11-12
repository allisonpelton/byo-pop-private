// The bitmap fonts (see Font.h).

#include "Font.h"
#include "Draw.h"
#include "File.h"
#include "Graphics.h"
#include "Host.h"
#include "Platform.h"
#include "Resources.h"

#include <cmath>

namespace bonnies
{

int gTextPos = 0;

namespace
{
// A character's advance before the scale: its frame's width in screen fractions and the font's spacing
inline double charAdvance(const Font* theFont, unsigned int theChar, float thePixelWidth)
{
	float aSpacing = theChar == 0x20 ? theFont->mDesc->mSpaceWidth : theFont->mDesc->mSpacing;
	return (double)theFont->mImage->mFrames[theChar].mWidth * thePixelWidth + aSpacing;
}
}

// 0x428190
Str Font::getLine(const Str& theText, int theStart, float theWidth, float* theRect, float theScale)
{
	Str aLine = "";
	float aSum = 0.0f;
	int aBreak = -1;
	float aBreakSum = 0.0f;
	float aPixelWidth = gInvWidth;
	bool aVisible = false;
	bool aBroke = false;
	int aLength = (int)theText.size();
	int i = theStart;
	if (i < aLength)
	{
		do
		{
			if (theText[i] == '\r' || theText[i] == '\n')
				break;
			unsigned int aCharCode = at(theText, i);
			signed char aChar = (signed char)at(theText, i);
			double aWidth = charAdvance(this, aCharCode, aPixelWidth) * theScale;
			if (aCharCode == 0x7c)
			{
				aWidth = 0.0;
				aVisible = true;
			}
			else if (aCharCode > 0x20)
				aVisible = true;
			if ((double)aSum + aWidth > theWidth)
			{
				if (aBreak < 0)
				{
					aBreak = i - 1;
					aBreakSum = aSum;
				}
				aLine = substr(theText, theStart, aBreak + 1);
				aSum = aBreakSum;
				aBroke = true;
				break;
			}
			if ((aChar < '0' || aChar == '-') && i > 0)
			{
				// A break after blanks and punctuation, but not inside a number ("1,000") nor after a quote or a
				// bracket that opens a word
				bool aKeep = false;
				int j = i + 1;
				if (j < aLength)
				{
					bool aNumber = false;
					if (aChar == '.' || aChar == ',')
					{
						signed char aPrev = (signed char)at(theText, j - 2);
						signed char aNext = (signed char)at(theText, j);
						if (aPrev >= '0' && aPrev <= '9' && aNext >= '0' && aNext <= '9')
							aNumber = true;
					}
					if (aChar == '\'' || aChar == '"' || aChar == '(')
					{
						if (j >= aLength)
							aKeep = true;
						else
						{
							signed char aNext = (signed char)theText[j];
							if ((aNext >= 'a' && aNext <= 'z') || (aNext >= 'A' && aNext <= 'Z'))
								aKeep = true;
						}
					}
					if (aNumber)
						aKeep = true;
				}
				if (!aKeep)
				{
					aBreak = aChar > 0x20 ? i : j - 2;
					aBreakSum = aSum;
				}
			}
			aWidth = charAdvance(this, aCharCode, aPixelWidth) * theScale;
			if (aCharCode == 0x7c)
				aWidth = 0.0;
			aSum = (float)(aWidth + aSum);
			i++;
		} while (i < aLength);
		if (aVisible && !aBroke)
			aLine = substr(theText, theStart, i);
	}
	int aLineLength = (int)aLine.size();
	gTextPos = aLineLength + theStart;
	if (gTextPos >= (int)theText.size())
		gTextPos = (int)theText.size();
	else if ((unsigned char)theText[gTextPos] <= 0x20)
		gTextPos++;
	if (aLineLength == 0 && i < (int)theText.size() && (theText[i] == '\r' || theText[i] == '\n'))
		aLine += " ";
	if (theRect != nullptr)
	{
		theRect[0] = 0.0f;
		theRect[1] = 0.0f;
		theRect[2] = aSum;
		theRect[3] = (float)((double)gInvHeight * mImage->mHeight * theScale);
	}
	return aLine;
}

// 0x428580
int Font::countLines(const Str& theText, int theStart, float theWidth, float* theRect, float theScale)
{
	Str aLine = "";
	gTextPos = theStart;
	float aMax = 0.0f;
	int aCount = 0;
	float aPixelHeight = gInvHeight;
	for (;;)
	{
		aLine = getLine(theText, gTextPos, theWidth, theRect, theScale);
		if (theRect != nullptr)
		{
			float aWidth = theRect[2] > aMax ? theRect[2] : aMax;
			theRect[2] = aWidth;
			if (aMax <= aWidth)
				aMax = aWidth;
		}
		if (aLine.empty())
			break;
		aCount++;
	}
	if (theRect != nullptr)
	{
		int aGaps = (aCount - 1 < 0) - 1 & aCount - 1;
		theRect[3] = (float)(((double)(mImage->mHeight * aCount) * aPixelHeight + (double)aGaps * mDesc->mLineGap) *
			theScale);
	}
	return aCount;
}

// 0x4286c0
float Font::textWidth(const Str& theText, float theWidth)
{
	float aRect[4];
	countLines(theText, 0, theWidth, aRect, 1.0f);
	return aRect[2];
}

// 0x4286f0
double Font::lineHeight(int theWithGap)
{
	float aGap = theWithGap != 0 ? mDesc->mLineGap : 0.0f;
	return (double)gInvHeight * mImage->mHeight + aGap;
}

// 0x428730
float Font::drawText(const Str& theText, float theX, float theY, float theW, float theH, uint32_t theColor,
	uint32_t theColor2, uint32_t theFlags, float theScale, float theAngle)
{
	if (theText.empty())
		return theY;
	if ((signed char)theFlags < 0)
	{
		mDrawingStroke = 1;
		if (mStrokeSize > 0)
			drawText(theText, theX, theY, theW, theH, theColor2, 0, theFlags & 0xffffff7f, theScale, theAngle);
		mDrawingStroke = 0;
	}
	if ((theFlags & 0x100) != 0)
	{
		mDrawingGlow = 1;
		if (mGlowSize > 0)
			drawText(theText, theX, theY, theW, theH, theColor2, 0, theFlags & 0xfffffeff, theScale, theAngle);
		mDrawingGlow = 0;
	}
	if ((theFlags & 0x40) != 0)
	{
		mDrawingShadow = 1;
		uint32_t aShadow = theColor2 != 0 ? theColor2 : 0x7f000000;
		drawText(theText, (float)((double)theX - (double)0.00375f), (float)((double)theY + (double)0.0033333334f), theW,
			theH, aShadow, 0, theFlags & 0xffffffbf, theScale, theAngle);
		mDrawingShadow = 0;
	}
	float aRect[4];
	int aLines = countLines(theText, 0, theW, aRect, theScale);
	float aY = theY;
	if ((theFlags & 2) == 0)
	{
		double aSpace = (double)theH - aRect[3];
		if ((theFlags & 4) == 0)
			aSpace *= 0.5;
		aY = (float)(aSpace + theY);
	}
	Str aLine = "";
	mCharIndex = 0;
	gTextPos = 0;
	bool aShimmered = false;
	float aLineHeight = (float)(((double)gInvHeight * mImage->mHeight + mDesc->mLineGap) * theScale);
	if (aLines > 0)
	{
		do
		{
			aLine = getLine(theText, gTextPos, theW, aRect, theScale);
			float aX = theX;
			if ((theFlags & 0x10) == 0)
			{
				double aSpace = (double)theW - aRect[2];
				if ((theFlags & 0x20) == 0)
					aSpace *= 0.5;
				aX = (float)(aSpace + theX);
			}
			int i = 0;
			if ((int)aLine.size() > 0)
			{
				do
				{
					Image* anImage = mImage;
					unsigned int aChar = at(aLine, i);
					float aDrawX = aX;
					float aDrawY = aY;
					if (mDrawingGlow != 0)
					{
						anImage = mGlow;
						aDrawX = (float)((double)aX - (double)gInvWidth * mGlowSize);
						aDrawY = (float)((double)aY - (double)gInvHeight * mGlowSize);
					}
					if (mDrawingStroke != 0)
					{
						anImage = mStroke;
						aDrawX = (float)((double)aDrawX - (double)gInvWidth * mStrokeSize);
						aDrawY = (float)((double)aDrawY - (double)gInvHeight * mStrokeSize);
					}
					uint32_t aColor = theColor;
					if ((theFlags & 0xe00) != 0)
					{
						// A light sweeps across the box, once a box's x + y + w + 0.3 in a 1200th of a second each
						double aPeriodD = (double)theX + theY + theW + (double)0.3f;
						float aPeriod = 1.0 > aPeriodD ? 1.0f : (float)aPeriodD;
						float aTime = (float)((double)(int)host()->getTicks() * (double)0.00083333335f);
						double aPos = (double)aDrawY * 0.25 + aDrawX;
						double aPhase = std::fmod((double)aTime, (double)aPeriod);
						float aDist = (float)std::fabs(aPos - aPhase);
						if (std::fabs(aPos - aPhase) < (double)0.2f)
						{
							aShimmered = true;
							if (gPlatform->mShimmerWaits == 0)
							{
								double aStrength = (theFlags & 0x200) != 0 ? 1.0 : -0.5;
								if ((theFlags & 0x800) != 0)
									aStrength = 0.5;
								int aLight = ftol(aStrength * std::cos((double)aDist * 5.0 * (double)1.570795f) * 70.0);
								aColor = addColor(theColor, aLight, aLight, aLight, aLight);
							}
						}
					}
					float aSpacing;
					if (aChar == 0x3a)
					{
						aDrawX = (float)((double)aDrawX + mDesc->mColonX);
						aDrawY = (float)((double)aDrawY + mDesc->mColonY);
						aSpacing = mDesc->mSpacing;
					}
					else if (aChar == 0x7c)
					{
						if (i < (int)aLine.size() - 1)
							aDrawX = (float)((double)aDrawX - (double)0.0025f);
						aSpacing = mDesc->mSpacing;
					}
					else if (aChar == 0x20)
						aSpacing = mDesc->mSpaceWidth;
					else
						aSpacing = mDesc->mSpacing;
					float anAdvance = (float)(((double)gInvWidth * mImage->mFrames[aChar].mWidth + aSpacing) * theScale);
					if (aChar == 0x7c)
						anAdvance = 0.0f;
					if (mDrawingGlow == 0 && mDrawingStroke == 0 && mDrawingShadow == 0 && mSelecting != 0 &&
						mCharIndex >= mSelStart && mCharIndex <= mSelEnd)
					{
						fillRect(aDrawX, aDrawY, anAdvance, aLineHeight, 0xff1f1f4f);
						aColor = 0xffcfcfcf;
					}
					const Frame* aFrame = &anImage->mFrames[aChar];
					bool aRotate = true;
					if (theScale == 1.0f)
					{
						if (theAngle == 0.0f)
						{
							aRotate = false;
							if (aChar != 0x20)
								drawFrameTinted(aFrame, aDrawX, aDrawY, aColor);
						}
					}
					if (aRotate)
					{
						if (theScale != 1.0f)
						{
							// Scaled about the character's centre
							float aShrink = (float)(1.0 - theScale);
							float aHalfW = (float)(((double)aFrame->mWidth * aShrink + 1.0) * 0.5);
							aDrawX = (float)((double)aHalfW / gWidth + aDrawX);
							float aHalfH = (float)(((double)aFrame->mHeight * aShrink + 1.0) * 0.5);
							aDrawY = (float)((double)aHalfH / gHeight + aDrawY);
						}
						if (aChar != 0x20)
							drawFrameRotated(aFrame, aDrawX, aDrawY, theAngle, theScale, 1.0f, aColor, 1);
					}
					mCharIndex++;
					aX = (float)((double)anAdvance + aX);
					i++;
				} while (i < (int)aLine.size());
			}
			aY = (float)((double)aLineHeight + aY);
		} while (--aLines != 0);
	}
	if (gPlatform->mDebugTexts != 0)
		drawRect(theX, theY, theW, theH, 0xff00ffff, 1);
	if (gPlatform->mShimmerWaits != 0 && !aShimmered && (theFlags & 0xe00) != 0)
		gPlatform->mShimmerWaits = 0;
	return aY;
}

// 0x42a9e0
Font* loadFont(FontDesc* theDesc)
{
	Font* aFont = new Font;
	aFont->mImage = gResources->getImage(theDesc->mImage);
	aFont->mDesc = new FontDesc;
	aFont->mDesc->mName = theDesc->mName;
	aFont->mDesc->mLineGap = theDesc->mLineGap;
	aFont->mDesc->mSpacing = theDesc->mSpacing;
	aFont->mDesc->mSpaceWidth = theDesc->mSpaceWidth;
	aFont->mDesc->mColonX = theDesc->mColonX;
	aFont->mDesc->mColonY = theDesc->mColonY;
	aFont->mSelecting = 0;
	aFont->mSelStart = 0;
	aFont->mSelEnd = 0;
	aFont->mCharIndex = 0;
	aFont->mDrawingGlow = 0;
	aFont->mDrawingStroke = 0;
	aFont->mDrawingShadow = 0;
	aFont->mStroke = nullptr;
	aFont->mGlow = nullptr;
	aFont->mStrokeSize = 0;
	aFont->mGlowSize = 0;
	int i = 1;
	do
	{
		Str aName = "";
		aName += theDesc->mName;
		aName += "_stroke_";
		aName += intToStr(i);
		Str aPath = graphicsDir(true) + aName;
		aPath += ".png";
		if (fileExists(aPath))
		{
			aFont->mStroke = gResources->getImage(aName);
			aFont->mStrokeSize = i;
			break;
		}
		i++;
	} while (i < 0x14);
	i = 1;
	do
	{
		Str aName = "";
		aName += theDesc->mName;
		aName += "_Glow_";
		aName += intToStr(i);
		Str aPath = graphicsDir(true) + aName;
		aPath += ".png";
		if (fileExists(aPath))
		{
			aFont->mGlow = gResources->getImage(aName);
			aFont->mGlowSize = i;
			return aFont;
		}
		i++;
	} while (i < 0x14);
	return aFont;
}

// 0x42aed0
void freeFont(Font* theFont)
{
	gResources->release(theFont->mImage);
	if (theFont->mStroke != nullptr)
		gResources->release(theFont->mStroke);
	if (theFont->mGlow != nullptr)
		gResources->release(theFont->mGlow);
	delete theFont->mDesc;
	delete theFont;
}

} // namespace bonnies
