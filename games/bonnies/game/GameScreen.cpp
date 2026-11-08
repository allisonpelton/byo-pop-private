// The game's screen (see GameScreen.h).

#include "GameScreen.h"
#include "BonniesApp.h"
#include "Book.h"
#include "Dialog.h"
#include "Dictionary.h"
#include "Draw.h"
#include "Events.h"
#include "File.h"
#include "FloatText.h"
#include "Font.h"
#include "Game.h"
#include "Globals.h"
#include "Graphics.h"
#include "HighScores.h"
#include "Host.h"
#include "Input.h"
#include "Language.h"
#include "Music.h"
#include "Options.h"
#include "Platform.h"
#include "Resources.h"
#include "Sound.h"
#include "Widget.h"
#include "App.h"
#include "Screens.h"
#include "Start.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>

namespace bonnies
{

static uint32_t ticks()
{
	return host()->getTicks();
}

// The intro's animations (Bonnie at her typewriter)
float gEyeX = -1.0f;              // 0x473e30: where her eyes look, from the text being typed
float gEyeY = -1.0f;              // 0x473e34
static uint32_t gBlinkTime;       // 0x47a618
static int gBlink;                // 0x47a61c: the blink's frame (0 none)
static int gLongBlink;            // 0x47a620: a blink of 12 frames instead of 4
static uint32_t gMouthTime;       // 0x478520
static uint32_t gMouthFrameTime;  // 0x478524
static int gMouth;                // 0x478528
static uint32_t gHeadTime;        // 0x47852c
static uint32_t gHeadFrameTime;   // 0x478530
static int gHead;                 // 0x478534
static uint32_t gHandsTime;       // 0x47853c
static int gHands;                // 0x478540
static uint32_t gEyeTime;         // 0x478544
static int gEyeFrame;             // 0x478548: her eyes' two frames, swapped while they move
static uint32_t gEyeFrameTime;    // 0x47854c
static uint32_t gLastMove;        // 0x478550: when any of them last moved
static uint32_t gScreenStart;     // 0x47850c
// 0x478538: set with the others and never read
static uint32_t gField478538;

// The chapter's text as it was wrapped last (0x40f240)
static Str gChapterSource;        // 0x47855c
static Str gChapterText;          // 0x478568

// 0x402e80
GameScreen::GameScreen()
{
}

// 0x40eb50
void GameScreen::setChapter(Str theName)
{
	if (!equalsNoCase(theName, mChapter))
	{
		host()->trace("CLIS 1");
		host()->trace(theName.c_str());
		host()->trace("CLIS 2");
		if (mPicture != nullptr)
			gResources->release(mPicture);
		mPicture = nullptr;
		mChapter = theName;
		host()->trace("CLIS 3");
		if (!theName.empty())
		{
			if ((gGame->mLevel == 2 && gBonnies->mLoadImages[0] == 0) ||
				(gGame->mLevel == 3 && gBonnies->mLoadImages[1] == 0))
				theName = lit(0x458210);
			mPicture = gResources->getImage(theName);
		}
		host()->trace("CLIS 4");
	}
}

// 0x40ed00
void GameScreen::refresh()
{
	mField58 = 3;
	mFieldC8 = 0;
	mField5C = ticks();
	mField60 = ticks() + 300;
	gGame->beginLevel();
}

// 0x40ed40
void GameScreen::setMessage(Str theText, int theTime)
{
	mMessage = theText;
	mMessageEnd = ticks() + theTime;
}

// 0x40edb0
void GameScreen::reset()
{
	mFieldE8 = 0;
	mFieldE4 = 0;
	mFieldF4 = ticks();
	mField88 = -1;
	mField8C = -1;
	mField90 = -1.0f;
	mField94 = -1.0f;
	mFieldA4 = 0;
	mField7C = 0;
	mField58 = 1;
	mField5C = ticks();
	mField60 = 0;
	mField80 = -1;
	mField84 = -1;
	mHintTime = 0;
	mHintShown = 0;
	mHintAgain = 0;
	mField70 = 0;
	mField74 = 0;
	mField78 = 0;
	mFieldC8 = 0;
	setChapter(gGame->currentBook()->name());
}

// 0x40ee60
void GameScreen::shutdown()
{
	removeAll();
	mChapter = "";
	if (mPicture != nullptr)
		gResources->release(mPicture);
	mPicture = nullptr;
	if (mField54 != nullptr)
		gResources->release(mField54);
	mField54 = nullptr;
	gGameScreen = nullptr;
}

// 0x40eec0: the top left corner of the cell at theColumn, theRow (the odd columns half a cell lower)
void cellPos(int theColumn, int theRow, float* theX, float* theY)
{
	*theX = (float)(theColumn * (double)0.0575f + gGame->currentBook()->mOffsetX);
	double aY = theRow * (double)0.07666667f + gGame->currentBook()->mOffsetY;
	*theY = (float)aY;
	if ((theColumn & 1) != 0)
		*theY = (float)(0.5f * (double)0.07666667f + aY);
	else
		*theY = (float)(0.0f * (double)0.07666667f + aY);
}

// 0x40ef30: the corner of the first cell of the board, by columns (0, 0 without one)
void firstCellPos(float* theX, float* theY)
{
	int aColumn = 0;
	do
	{
		int aRow = 0;
		do
		{
			if (gGame->currentBook()->isCell(aColumn, aRow) != 0)
			{
				cellPos(aColumn, aRow, theX, theY);
				return;
			}
			aRow++;
		} while (aRow < 9);
		aColumn++;
	} while (aColumn < 9);
	*theY = 0.0f;
	*theX = 0.0f;
}

// 0x40f000
void GameScreen::getRect(float* theX0, float* theY0, float* theX1, float* theY1, int theAll)
{
	int aMinColumn = 999;
	int aMaxColumn = -1;
	int aMinHalf = 999;
	int aMaxHalf = -1;
	int aColumn = 0;
	do
	{
		int aRow = 0;
		do
		{
			if (gGame->currentBook()->isCell(aColumn, aRow) != 0)
			{
				if (aColumn < aMinColumn)
					aMinColumn = aColumn;
				if (aMaxColumn < aColumn)
					aMaxColumn = aColumn;
				int aHalf = (aColumn & 1) + aRow * 2;
				if (aHalf < aMinHalf)
					aMinHalf = aHalf;
				if (aMaxHalf < aHalf)
					aMaxHalf = aHalf;
			}
			aRow++;
		} while (aRow < 9);
		aColumn++;
	} while (aColumn < 9);
	if (aMaxColumn < 0)
	{
		*theX0 = 0.0f;
		*theX1 = 1.0f;
		return;
	}
	float aScratch;
	cellPos(aMinColumn, 0, theX0, &aScratch);
	cellPos(aMaxColumn, 0, theX1, &aScratch);
	cellPos(aMinHalf & 1, aMinHalf >> 1, &aScratch, theY0);
	cellPos(aMaxHalf & 1, aMaxHalf >> 1, &aScratch, theY1);
	*theX1 = (float)(*theX1 + (double)0.0575f);
	*theY1 = (float)(*theY1 + (double)0.07666667f);
	*theX0 = (float)(*theX0 - (double)0.0075f);
	*theX1 = (float)(*theX1 + (double)0.0075f);
	*theY0 = (float)(*theY0 - (double)0.01f);
	*theY1 = (float)(*theY1 + (double)0.01f);
	if (theAll != 0 && frameCount() <= gGraphics->mField00 + 3)
	{
		*theX0 = (float)(*theX0 - (double)0.10625f);
		*theX1 = (float)(*theX1 + (double)0.1375f);
		*theY0 = 0.0f;
		*theY1 = 1.0f;
	}
	*theX0 = (0.0f <= *theX0) ? *theX0 : 0.0f;
	*theX1 = (1.0f < *theX1) ? 1.0f : *theX1;
	*theY0 = (0.0f <= *theY0) ? *theY0 : 0.0f;
	*theY1 = (1.0f < *theY1) ? 1.0f : *theY1;
}

// 0x40f200
void GameScreen::tilePos(int theColumn, int theRow, float* theX, float* theY)
{
	cellPos(theColumn, theRow, theX, theY);
	*theX = (float)(*theX + (double)0.0275f);
	*theY = (float)(*theY + (double)0.036666665f);
}

// 0x40f240: the level's chapter text, a line break put at the last blank before each line that would not fit
static Str chapterText()
{
	int i = 0;
	if (50 < gGame->mLevel)
		return Str("");
	if (equalsNoCase(Str(getText(exeI32(0x46665c + gGame->mLevel * 4))), gChapterSource))
		return gChapterText;
	int aBlank = -1;
	gChapterText = "";
	gChapterSource = getText(exeI32(0x46665c + gGame->mLevel * 4));
	int aLength = (int)gChapterSource.size();
	int aLines = 1;
	if (0 < aLength)
	{
		do
		{
			char c;
			if (i < (int)gChapterSource.size())
			{
				c = gChapterSource[i];
				if (c == ' ' || c == '\n')
					aBlank = i;
			}
			else
				c = 'A';
			gChapterText += c;
			int aCount = gBonnies->mFonts[0]->countLines(gChapterText, 0, 0.56375f, nullptr, 1.0f);
			if (aLines < aCount)
			{
				if (-1 < aBlank)
					gChapterText[aBlank] = '\n';
				aBlank = -1;
				aLines = aCount;
			}
			i++;
		} while (i < aLength);
	}
	return gChapterText;
}

// 0x40eab0: theText (a C string) drawn in theFont
double drawCText(Font* theFont, const char* theText, float theX, float theY, float theW, float theH,
	uint32_t theColor, uint32_t theColor2, uint32_t theFlags, float theScale, float theAngle)
{
	return theFont->drawText(Str(theText), theX, theY, theW, theH, theColor, theColor2, theFlags, theScale, theAngle);
}

static const Frame* appFrame(int theIndex)
{
	return gBonnies->mImages[theIndex]->mFrames;
}

// 0x40f3e0: the chapter's intro: Bonnie types its text
void GameScreen::drawIntro()
{
	Str aSource = chapterText();
	drawFrame(appFrame(18), 0.0f, 0.0f, 1.0f, 1);
	Str aText = "";
	aText += gGame->currentBook()->title();
	int aChapter = gGame->currentBook()->mChapter + 1;
	aText += lit(0x457424);
	aText += getText(0x52a);
	aText += lit(0x4582d0);
	appendInt(aText, aChapter);
	aText += lit(0x457c64);
	float aFade = 0.0f;
	int aTyped = 0;
	if (mField58 == 0xd || mField58 == 9)
	{
		mTyped = 0;
		gEyeX = -1.0f;
		gEyeY = -1.0f;
		gEyeTime = 0;
		int aRandom = randomRange(6000, 9000);
		gBlinkTime = ticks() - aRandom;
		gBlink = 0;
		aRandom = randomRange(0, 6000);
		gMouthTime = ticks() - aRandom;
		gMouth = 0;
		aRandom = randomRange(2000, 9000);
		gHeadTime = ticks() - aRandom;
		gHead = 0;
		aRandom = randomRange(0, 300);
		gField478538 = ticks() - aRandom;
		gHands = 0;
		gLastMove = 0;
	}
	else if (mField58 == 0xe)
	{
		if (mTyped < 999)
		{
			float aDelay;
			if (mLinesTyped < mLines)
			{
				mLinesTyped++;
				playSound(Str(lit(0x4582b0)), 0.5f);
				aDelay = (float)(exeFloat(0x4582ac) * (double)72.0f);
			}
			else
			{
				float aFactor;
				if (mSpacesTyped < mSpaces)
				{
					aTyped = 1;
					mSpacesTyped++;
					playSound(Str(lit(0x45829c)), 0.45f);
					aFactor = 1.8f;
				}
				else
				{
					if ((int)ticks() <= mTypeTime)
						goto typed;
					mTyped++;
					aTyped = 1;
					Str aName = "";
					int aRandom = randomRange(1, 3);
					aName += lit(0x458294);
					appendInt(aName, aRandom);
					playSound(aName, 0.45f);
					aFactor = 1.0f;
				}
				int aRandom = randomRange(0x46, 0x82);
				aDelay = (float)(aRandom * (double)aFactor * 0.01f * 72.0f);
			}
			float aNext = (float)(mTypeTime + (double)aDelay);
			if (aNext <= (double)(int)(ticks() + 20))
				mTypeTime = (int)(ticks() + 20);
			else
				mTypeTime = ftol(mTypeTime + (double)aDelay);
		}
	}
	else if (mField58 == 0xf)
	{
		aFade = mFieldB4;
		mTyped = 9999;
	}
typed:
	{
		int aShown = 0;
		char c = 0;
		mSpaces = 0;
		int i = 0;
		if (0 < (int)aSource.size())
		{
			do
			{
				char d;
				if (i < (int)aSource.size())
				{
					d = aSource[i];
					if (d == ' ')
					{
						mSpaces++;
						goto append;
					}
					if (d <= ' ')
						goto append;
				}
				else
					d = 'A';
				if (mTyped <= aShown)
				{
					c = d;
					break;
				}
				aShown++;
			append:
				aText += d;
				i++;
			} while (i < (int)aSource.size());
		}
		gBonnies->mFonts[0]->drawText(aText, 0.08f, 0.08f, 0.56375f, 0.5f, 0xffefefef, 0, 0x12, 1.0f, 0.0f);
		if (i < (int)aSource.size() || (int)ticks() >= mTypeTime + 500)
		{
			if (0 < c)
				aText += c;
			mLines = gBonnies->mFonts[0]->countLines(aText, 0, 0.56375f, nullptr, 1.0f) - 4;
		}
		else
		{
			int aValue = mTyped;
			if (aValue < 999)
				aValue = 999;
			mTyped = aValue;
		}
	}
	drawCText(gBonnies->mFonts[0], getText(0x52d), 0.199999988f, 0.9f, 0.6f, 0.1f, 0xffffffcf, 0, 0x409, 1.0f, 0.0f);
	// Her eyes follow the end of the last line
	int aColumn = 0;
	int j = (int)aText.size() - 1;
	if (-1 < j)
	{
		do
		{
			if (j < (int)aText.size() && aText[j] == '\n')
				break;
			aColumn++;
			j--;
		} while (-1 < j);
	}
	float aTargetX = (float)((aColumn * 2 - 0x28) * (double)0.00125f);
	float aTargetY = (float)((3 - mLines * 7) * (double)0.0016666667f);
	if (gEyeX <= -0.5)
	{
		gEyeX = aTargetX;
		gEyeY = aTargetY;
		gEyeTime = ticks();
	}
	else
	{
		int aTime = ticks() - gEyeTime;
		gEyeTime = ticks();
		double aDiff = aTargetY - (double)gEyeY;
		float aDistance = (float)std::fabs(aDiff);
		if (aDistance < 0.0018333333333333335)
			gEyeY = aTargetY;
		else
		{
			double aStep = aTime * (double)0.004f * 0.016666668f;
			double aFactor = aStep / (0.0001f < aDistance ? (double)aDistance : (double)0.0001f);
			if (1.0f < aFactor)
				aFactor = 1.0f;
			else
				aFactor = aStep / (0.0001f < aDistance ? (double)aDistance : (double)0.0001f);
			gEyeY = (float)(aDiff * aFactor + gEyeY);
			if (60 < (int)(ticks() - gEyeFrameTime))
			{
				gEyeFrame = gEyeFrame == 0;
				gEyeFrameTime = ticks();
			}
		}
		if (aDistance < 0.00033333333f)
		{
			double aDiffX = aTargetX - (double)gEyeX;
			aDistance = (float)std::fabs(aDiffX);
			if (aDistance < 0.0013750000000000001)
				gEyeX = aTargetX;
			else
			{
				double aStep = aTime * (double)0.002f * 0.0875f;
				double aFactor = aStep / (0.0001f < aDistance ? (double)aDistance : (double)0.0001f);
				if (1.0f < aFactor)
					gEyeX = (float)(1.0f * aDiffX + gEyeX);
				else
					gEyeX = (float)(aStep / (0.0001f < aDistance ? (double)aDistance : (double)0.0001f) * aDiffX +
						gEyeX);
			}
		}
	}
	bool aStill;
	if (gBonnies->mPaused == 0 && isInactive() == 0 && gBonnies->inModal() == 0)
		aStill = false;
	else
	{
		aStill = true;
		gMouthTime = ticks();
		gHeadTime = gMouthTime;
		gField478538 = gMouthTime;
		gBlinkTime = gMouthTime;
	}
	// Blinking
	if (gBlink == 0)
	{
		if (8500 < (int)(ticks() - gBlinkTime) && 1500 < (int)(ticks() - gLastMove) && gHead == 0 && gMouth == 0)
		{
			int aRandom = randomRange(0, 100);
			gLongBlink = aRandom < 25;
			gBlink = 1;
			aRandom = randomRange(0, 2000);
			gBlinkTime = ticks() - aRandom;
		}
	}
	else
	{
		gBlink++;
		if (gLongBlink == 0)
		{
			if (gBlink == 4)
				gBlink = 0;
		}
		else if (gBlink == 12)
			gBlink = 0;
	}
	int aFrame = 0;
	if (gBlink == 1 || gBlink == 3)
		aFrame = 1;
	else if (gBlink == 2)
		aFrame = 2;
	else if (gBlink == 9 || gBlink == 11)
		aFrame = 1;
	else if (gBlink == 10)
		aFrame = 2;
	if (!aStill && aFrame != 0)
		drawFrame(appFrame(aFrame + 25), 0.74875f, 0.18666667f, 1.0f, 0);
	// Her mouth
	if (gMouth == 0)
	{
		if (9000 < (int)(ticks() - gMouthTime) && 1500 < (int)(ticks() - gLastMove) && gHead == 0 && gBlink == 0)
		{
			gMouth = 1;
			int aRandom = randomRange(0, 2000);
			gMouthTime = ticks() - aRandom;
			gMouthFrameTime = ticks();
		}
	}
	else
	{
		if (120 < (int)(ticks() - gMouthFrameTime))
		{
			gMouth++;
			gMouthFrameTime = ticks();
		}
		if (gMouth == 12)
			gMouth = 0;
	}
	aFrame = 0;
	if (gMouth == 1 || gMouth == 11)
		aFrame = 1;
	else if (1 < gMouth && gMouth < 11)
		aFrame = 2;
	if (!aStill && aFrame != 0)
		drawFrame(appFrame(aFrame + 27), 0.74875f, 0.18666667f, 1.0f, 0);
	// Her head
	if (gHead == 0)
	{
		if (11000 < (int)(ticks() - gHeadTime) && 1500 < (int)(ticks() - gLastMove))
		{
			gHead = 1;
			int aRandom = randomRange(0, 2000);
			gHeadTime = ticks() - aRandom;
			gHeadFrameTime = ticks();
			gLastMove = ticks();
		}
	}
	else
	{
		if (950 < (int)(ticks() - gHeadFrameTime))
		{
			gHead++;
			gHeadFrameTime = ticks();
		}
		if (gHead == 2)
			gHead = 0;
	}
	if (!aStill && 0 < gHead)
		drawFrame(appFrame(15), 0.67f, 0.063333333f, 1.0f, 1);
	if (gMouth != 0 || gBlink != 0 || gHead != 0)
		gLastMove = ticks();
	// Her hands, while she types
	if (150 < (int)(ticks() - gHandsTime) && aTyped)
	{
		gHandsTime = ticks();
		gHands++;
		if (2 < gHands)
		{
			gHands = 0;
			goto eyes;
		}
	}
	if (gHands == 1 || gHands == 3)
		aFrame = 1;
	else
	{
		if (gHands != 2)
			goto eyes;
		aFrame = 2;
	}
	drawFrame(appFrame(aFrame + 15), 0.675f, 0.43833333f, 1.0f, 1);
eyes:
	pushClip(0.0f, 0.0f, 1.0f, 0.6666667f);
	drawFrame(appFrame(20), (float)(gEyeX + (double)0.60625f), (float)(gEyeY + (double)0.54f), 1.0f, 0);
	drawFrame(appFrame(22), (float)(gEyeX + (double)0.60625f), 0.54f, 1.0f, 0);
	drawFrame(appFrame(19), (float)(gEyeX + (double)0.60625f), (float)(0.54f - (double)gEyeY), 1.0f, 0);
	popClip();
	drawFrame(appFrame(21), (float)(gEyeX + (double)0.60625f), 0.54f, 1.0f, 0);
	drawFrame(appFrame(gEyeFrame + 24), (float)(gEyeX + (double)0.60625f), 0.54f, 1.0f, 0);
	drawFrame(appFrame(23), 0.60625f, 0.54f, 1.0f, 0);
	// The chapter's picture grows in over her when the chapter begins
	double aLeft = 1.0f - (double)aFade;
	float aX = (float)(0.12125f * aLeft);
	float aY = (float)(0.5683333f * aLeft);
	double aScale = aLeft * 0.25f + aFade;
	float aScaleF = (float)aScale;
	if (!(aScaleF < 1.0f))
		aScaleF = 1.0f;
	if (!(aScale < 1.0f))
		aScaleF = 1.0f;
	if (mPicture != nullptr)
	{
		if (aScaleF == 1.0f)
			drawFrame(mPicture->mFrames, 0.0f, 0.0f, 1.0f, 1);
		else
			drawFrameRotated(mPicture->mFrames, aX, aY, 0.0f, aScaleF, 1.0f, 0, 1);
	}
}

// 0x4101d0: the cell nearest theX, theY (only those that are not fixed and hold no '*' with theFree), and whether
// the point is on its tile
int hitTile(float theX, float theY, int* theColumn, int* theRow, float* theDistance, int theFree)
{
	*theDistance = 9999.0f;
	*theColumn = -1;
	*theRow = -1;
	float aReach = (float)(distance(0.0f, 0.0f, 0.0575f, 0.07666667f) + 0.01f);
	int aColumn = 0;
	int aCell = 0;
	do
	{
		int aRow = 0;
		do
		{
			if (gGame->currentBook()->isCell(aColumn, aRow) != 0 &&
				(theFree == 0 ||
					((gGame->mTileFlags[aCell + aRow] & 2) == 0 && gGame->mLetters[aCell + aRow] != '*')))
			{
				float aX = (float)(aColumn * (double)0.0575f + gGame->currentBook()->mOffsetX);
				double aRowY = aRow * (double)0.07666667f + gGame->currentBook()->mOffsetY;
				double aHalf = (aColumn & 1) != 0 ? 0.5f : 0.0f;
				aX = (float)(aX + (double)0.0275f);
				float aY = (float)(aHalf * 0.07666667f + aRowY + 0.036666665f);
				double aDistance = distance(theX, theY, aX, aY);
				if (aDistance < *theDistance || aDistance <= aReach)
				{
					*theDistance = (float)aDistance;
					*theColumn = aColumn;
					*theRow = aRow;
					if (*theDistance <= aReach && std::fabs(aX - (double)theX) <= 0.029325f &&
						std::fabs(aY - (double)theY) <= 0.0391f)
						return 1;
				}
			}
			aRow++;
		} while (aRow < 9);
		aColumn++;
		aCell += 9;
	} while (aCell < 0x51);
	return 0;
}

// 0x4103a0
void GameScreen::showHint(Str theText, int theStep, float theX, float theY, int theSide, int theFlag)
{
	gGame->mFieldCAC = gGame->mFieldCA8 + gGame->mFieldCA0;
	mStrCC = theText;
	mHintX = theX;
	mHintY = theY;
	mHintSide = theSide;
	mTutorialStep = theStep;
	if (theStep == 0 || theStep == 1 || theStep == 2 || theStep == 3 || theFlag != 0)
		findWidget(0xc9)->mFlags |= WIDGET_INACTIVE;
	if (theStep != 7 && theStep != 0xb && theStep != 0xc && theStep != 0xe && theStep != 0xf && theStep != 0x10 &&
		theStep != 0x16 && theStep != 0x17 && theStep != 0x18 && theStep != 0x1e && theStep != 0x1f &&
		theStep != 0x20 && theStep != 199)
		findWidget(0xcb)->mFlags |= WIDGET_INACTIVE;
	if (theStep != 0 && theStep != 1 && theStep != 2 && theStep != 3)
	{
		if (theFlag == 0)
			return;
		findWidget(0xca)->mFlags |= WIDGET_INACTIVE;
	}
	if (theFlag != 0 && gOverlayScreen->findWidget(0xcc) == nullptr)
	{
		float aRect[4];
		gBonnies->balloon(mStrCC, mHintX, mHintY, mHintSide, theFlag, aRect, 0);
		Button* aButton = new Button();
		aButton->init();
		gOverlayScreen->addWidget(aButton);
		aButton->mX = (float)((aRect[2] - (double)0.17875f) * 0.5f + aRect[0]);
		aButton->mWidth = 0.17875f;
		aButton->mHeight = 0.07f;
		aButton->mColor = 0xffd6f2ff;
		aButton->mColor2 = 0xff163301;
		aButton->mPanelColor = 0x7f3f3f3f;
		aButton->mY = (float)(((aRect[3] + (double)aRect[1]) - 0.058333334f) - 0.041666668f);
		aButton->mFocusPanelColor = addRGB(0x7f3f3f3f, 0x2d, 0x2d, 0x2d);
		aButton->mEdgeColor = 0xff01d39a;
		aButton->mTextFlags = 0x80;
		aButton->mFlags = 0x3a0;
		aButton->setFont(exeFont(0x4787f8));
		aButton->mText = getText(0x541);
		aButton->mKeys[0] = 0xd;
		aButton->mKeys[1] = 0x1b;
		aButton->mId = 0xcc;
	}
}

// 0x410650
bool GameScreen::isBusy()
{
	return !mStrCC.empty();
}

// 0x410660
void GameScreen::updateHint()
{
	mFieldAC = 0;
	Widget* aButton = findWidget(0xcb);
	if (aButton != nullptr && gGame != nullptr)
	{
		if (gGame->mFieldC5C == 0 || 1 < gGame->mGoal || gGame->mFieldC5C == 0)
		{
			aButton->mText = getText(0x542);
			return;
		}
		mFieldAC = 1;
		aButton->mText = getText(0x543);
	}
}

// 0x4106e0: the hint's word found and shown
void GameScreen::showHintWord()
{
	mHintShown = 0;
	if (gGame->mHintFound == 0)
	{
		gGame->findHint();
		if (gGame->mHintFound == 0)
			goto done;
	}
	mHintAgain = 0 < mFieldB0;
	mHintTime = ticks();
	mHintShown = 1;
done:
	mFieldB0 = 0;
}

// 0x410740
void GameScreen::flash(int theTime, uint32_t theColor)
{
	mFlashTime = ticks();
	mFlashLength = theTime;
	mFlashColor = theColor;
}

// 0x410770
int GameScreen::isPlaying()
{
	int aResult = 1;
	if (mField58 != 1 || gGame->mFieldC60 != 0)
		aResult = 0;
	return aResult;
}

// 0x410c00
void GameScreen::init()
{
	gGameScreen = this;
	Screen::init();
	gApp->mStatusFlags = 0x10c;
	mField11C = 0;
	mSpacesTyped = 0;
	mSpaces = 0;
	mLinesTyped = 0;
	mLines = 0;
	mTyped = 0;
	mFieldF0 = 0;
	mFlashTime = 0;
	mFlashLength = 0;
	mFlashColor = 0;
	mFieldC0 = 1.0f;
	mFieldC4 = 0;
	mFieldBC = 1.0f;
	mFieldB8 = 1.0f;
	mFieldB4 = 1.0f;
	mChapter = "";
	mPicture = nullptr;
	mField54 = nullptr;
	mMessage = "";
	mStrCC = "";
	mTutorialStep = -1;
	mHintX = 0.0f;
	mHintY = 0.0f;
	mHintSide = 0;
	mFieldAC = 0;
	mFieldB0 = 0;
	mMessageEnd = 0;
	int i = 0;
	do
	{
		Button* aButton = new Button();
		aButton->init();
		addWidget(aButton);
		if (gGame->mFieldC5C == 0)
			aButton->mX = (float)((i * 0x86 + 0x20) * (double)0.00125f + 0.515f);
		else
			aButton->mX = (float)((i * 0xf4 + 0x20) * (double)0.00125f + 0.44625f);
		aButton->mY = 0.90666664f;
		if (gGame->mFieldC5C == 0)
			aButton->mY = 0.86166668f;
		aButton->mWidth = 0.1575f;
		aButton->mHeight = 0.053333335f;
		aButton->mColor = 0xffffffff;
		aButton->mColor2 = 0xff96540b;
		aButton->mTextFlags = 0x80;
		aButton->mFlags = 0x2c0;
		aButton->setFont(exeFont(0x47866c));
		aButton->setUpImage(Str(lit(0x458378)), 0);
		aButton->setFocusImage(Str(lit(0x458364)), 0);
		aButton->setDownImage(Str(lit(0x458350)), 0);
		if (i == 0)
		{
			aButton->mText = getText(0x528);
			aButton->mId = 0xca;
		}
		else if (i == 1)
		{
			aButton->mText = getText(0x529);
			aButton->mId = 0xcb;
		}
		i++;
	} while (i < 2);
	Button* aMenu = new Button();
	aMenu->init();
	addWidget(aMenu);
	aMenu->mX = 0.0f;
	aMenu->mY = 0.82333332f;
	aMenu->mWidth = 0.185f;
	aMenu->mHeight = 0.17666666f;
	aMenu->mHitX = 0.0f;
	aMenu->mHitY = 0.033333335f;
	aMenu->mHitWidth = 0.16f;
	aMenu->mHitHeight = 0.14333333f;
	aMenu->mFlags = 0x40;
	aMenu->setUpImage(Str(lit(0x458338)), 0);
	aMenu->setFocusImage(Str(lit(0x458320)), 0);
	aMenu->setDownImage(Str(lit(0x458308)), 0);
	aMenu->mId = 0xc9;
	gScreenStart = ticks();
	reset();
	gMusic->select(3, 0);
	updateHint();
}


static uint32_t gBoardDrawTime;   // 0x47851c
static uint32_t gScoreTime;       // 0x478518: when the score shown last moved

// The board's cell at theColumn, theRow: its corner (x as the original rounds it, y unrounded)
static void cellCorner(int theColumn, int theRow, float* theX, double* theY)
{
	*theX = (float)(theColumn * (double)0.0575f + gGame->currentBook()->mOffsetX);
	double aRowY = theRow * (double)0.07666667f + gGame->currentBook()->mOffsetY;
	double aHalf = (theColumn & 1) != 0 ? 0.5f : 0.0f;
	*theY = aHalf * 0.07666667f + aRowY;
}

// Whether theColumn, theRow is on the path
static bool onPath(int theColumn, int theRow)
{
	bool aFound = false;
	int i = 0;
	if (0 < gGame->mPathLength)
	{
		do
		{
			if (gGame->pathColumn(i) == theColumn && gGame->pathRow(i) == theRow)
				aFound = true;
			i++;
		} while (i < gGame->mPathLength);
	}
	return aFound;
}

// 0x410f90: the board: the book's title, the chapter and the score, the board's edges, the cells and the tiles
void GameScreen::drawBoard()
{
	if (gBoardDrawTime == 0)
		gBoardDrawTime = ticks();
	if ((int)(ticks() - gBoardDrawTime) <= 100)
		ticks();
	gBoardDrawTime = ticks();
	host()->trace(lit(0x458410));
	float aWidth = 0.2f;
	Str aText = "";
	aText = gGame->currentBook()->title();
	Font* aFont = gBonnies->mFonts[3];
	const Frame* aPanel = gBonnies->mImages[0]->mFrames;
	if (2 <= aFont->countLines(aText, 0, 0.2f, nullptr, 1.0f))
		aFont = gBonnies->mFonts[2];
	if (2 <= aFont->countLines(aText, 0, 0.2f, nullptr, 1.0f))
	{
		aPanel = gBonnies->mImages[1]->mFrames;
		aWidth = 0.245f;
	}
	drawFrame(aPanel, 0.01375f, 0.036666665f, 1.0f, 0);
	aFont->drawText(aText, 0.04125f, 0.051666666f, aWidth, 0.05f, 0xffffffff, 0x8f000000, 0x109, 1.0f, 0.0f);
	aText = "";
	int aChapter = gGame->currentBook()->mChapter + 1;
	aText += getText(0x52a);
	aText += lit(0x4582d0);
	aText += intToStr(aChapter);
	gBonnies->mFonts[2]->drawText(aText, 0.04125f, 0.0f, aWidth, 0.046666667f, 0xffffffff, 0x8f000000, 0x10c, 1.0f,
		0.0f);
	// The score counts up to the game's
	int aShown = gGame->mShownScore;
	if (aShown == gGame->mScore)
		gScoreTime = ticks();
	else
	{
		if ((int)(ticks() - gGame->mFieldC70) <= 0xf0)
		{
			int aElapsed = ticks() - gScoreTime;
			int aLeft = (gGame->mFieldC70 - ticks()) + 0xf0 + aElapsed;
			int aTotal = 1;
			if (1 <= aLeft)
				aTotal = aLeft;
			double aFraction = (double)aElapsed / aTotal;
			if (!(aFraction < 1.0f))
				aFraction = 1.0f;
			int aFrom = gGame->mShownScore;
			gGame->mShownScore = ftol((gGame->mScore - aFrom) * aFraction + aFrom);
			int aScore = gGame->mScore;
			int aStep;
			if (50000 < aScore)
				aStep = 100;
			else if (20000 < aScore)
				aStep = 50;
			else if (5000 < aScore)
				aStep = 10;
			else if (500 < aScore)
				aStep = 5;
			else
				goto clamp;
			gGame->mShownScore = ((aStep >> 1) + gGame->mShownScore) / aStep * aStep;
			if (10 <= aStep)
				gGame->mShownScore += gGame->mScore % 10;
			if (100 <= aStep)
				gGame->mShownScore += gGame->mScore % 100;
		clamp:
			if (gGame->mScore < gGame->mShownScore)
				gGame->mShownScore = gGame->mScore;
		}
		else
			gGame->mShownScore = gGame->mScore;
		if (aShown != gGame->mShownScore)
			gScoreTime = ticks();
	}
	aText = "";
	int aScoreShown = gGame->mShownScore;
	aText += getText(0x52b);
	aText += intToStr(aScoreShown);
	gBonnies->mFonts[3]->drawText(aText, 0.016249999f, 0.10333333f, (float)(aWidth + (double)0.05f), 0.06666667f,
		0xffffffff, 0x8f000000, 0x10a, 1.0f, 0.0f);
	if (gGame->mFieldC5C != 0)
		drawFrame(gBonnies->mImages[2]->mFrames, 0.44624999f, 0.88666666f, 1.0f, 0);
	else
		drawFrame(gBonnies->mImages[32]->mFrames, 0.51499999f, 0.84166664f, 1.0f, 0);
	// The board's edges, where a cell has no neighbour
	int aColumn = 0;
	do
	{
		int aRow = 0;
		do
		{
			if (gGame->currentBook()->isCell(aColumn, aRow) != 0)
			{
				int aMask = 0;
				int aDirection = 0;
				do
				{
					int aNewColumn;
					int aNewRow;
					if (gGame->neighbour(aColumn, aRow, aDirection, &aNewColumn, &aNewRow) != 0)
						aMask |= 1 << aDirection;
					aDirection++;
				} while (aDirection < 6);
				if (aMask != 0x3f)
				{
					float aX;
					double aY;
					cellCorner(aColumn, aRow, &aX, &aY);
					drawFrame(&gBonnies->mImages[6]->mFrames[aMask], (float)(aX - (double)0.015f),
						(float)(aY - 0.02f), 1.0f, 0);
				}
			}
			aRow++;
		} while (aRow < 9);
		aColumn++;
	} while (aColumn < 9);
	// The cells
	aColumn = 0;
	int aCell = 0;
	do
	{
		int aRow = 0;
		do
		{
			if (gGame->currentBook()->isCell(aColumn, aRow) != 0)
			{
				if (gGame->mLetters[aCell + aRow] == 0)
					goto nextCell;
				float aX;
				double aYD;
				cellCorner(aColumn, aRow, &aX, &aYD);
				float aY = (float)aYD;
				int i = aCell + aRow;
				int aLit = 0;
				uint32_t aColor = 0x5f9fbfaf;
				int aCursor = 0;
				if ((gGame->mTileFlags[i] & 0x20) != 0)
				{
					aLit = 1;
					aColor = addColor(aColor, 0, -0x18, 0x3c, -0x28);
				}
				if (gBonnies->inModal() == 0 && mField7C == 0 && gGame->mFieldC60 == 0 && gBonnies->mPaused == 0 &&
					mField58 == 1)
				{
					bool aOnPath = onPath(aColumn, aRow);
					if (aOnPath)
					{
						aLit = 1;
						if (gGame->isWord() != 0)
							aColor = addColor(aColor, 0x28, 0x14, 0x28, 0x14);
						else
							aColor = addColor(aColor, 0x28, 0x32, 0x1e, 0x1e);
					}
					if (mField88 == aColumn && mField8C == aRow)
					{
						aCursor = 1;
						if (aLit == 0)
						{
							aLit = 1;
							aColor = 0x2fffffff;
						}
						else
							aColor = addColor(aColor, aOnPath ? 0x1e : 0x3c, 0x14,
								(gGame->mTileFlags[i] & 0x20) != 0 ? 0x3c : 0x28, -0x14);
					}
				}
				if ((gGame->mTileFlags[i] & 2) != 0)
				{
					drawFrame(&gBonnies->mImages[5]->mFrames[0], aX, aY, 1.0f, 0);
					drawFrame(&gBonnies->mImages[5]->mFrames[1], aX, aY, 1.0f, 0);
				}
				else if (gGame->mField43C[i] == '^')
				{
					int aTime = ticks() - gGame->mField2F8[i];
					int aFrame = aTime / 40;
					if (10 < aFrame)
						aFrame = 10;
					float aAlpha = (float)((aTime - 0x280) * (double)0.0025f);
					drawFrame(&gBonnies->mImages[5]->mFrames[0], aX, aY, 1.0f, 0);
					drawFrame(&gBonnies->mImages[5]->mFrames[aFrame + 1], aX, aY, 1.0f, 0);
					if (0.0f < aAlpha)
					{
						if (!(aAlpha < 1.0f))
						{
							gGame->mField2F8[i] = 0;
							gGame->mField43C[i] = 0;
						}
						drawFrame(&gBonnies->mImages[3]->mFrames[(aColumn % 3) * 3 + aRow % 3], aX, aY, aAlpha, 0);
					}
				}
				else
				{
					drawFrame(&gBonnies->mImages[3]->mFrames[(aColumn % 3) * 3 + aRow % 3], aX, aY, 1.0f, 0);
					float aX2 = (float)(aX + (double)0.0025f);
					float aY2 = (float)(aY + (double)0.0033333334f);
					if (aLit != 0)
					{
						fillRect(aX2, aY2, 0.055f, 0.07333333f, aColor);
						if (aCursor != 0)
							drawRect(aX2, aY2, 0.055f, 0.07333333f, addColor(aColor, 0, 0, 0, 0), 3);
					}
					if ((gGame->mTileFlags[i] & 8) != 0)
						drawSweep(aX2, aY2, 0.055f, 0.07333333f,
							((gGame->mTileFlags[i] & 0x20) != 0 ? 0x3767e118u : 0u) + 0xc887ff87u);
					if ((gGame->mTileFlags[i] & 0x10) != 0)
					{
						int aShade = (aLit != 0) + 2;
						aColor = addColor(aColor, aLit != 0 ? 0x14 : 0, aShade * -0x18, aShade * 0x3c, aShade * -0x28);
						fillTriangle(aX2, aY2, (float)(aX2 + (double)0.055f), aY2, aX2,
							(float)(aY2 + (double)0.07333333f), aColor);
					}
				}
			}
		nextCell:
			aRow++;
		} while (aRow < 9);
		aColumn++;
		aCell += 9;
	} while (aCell + 0x6d < 0xbe);
	// The tiles: the letter that was there fades out and the new one in
	aColumn = 0;
	do
	{
		int aRow = 0;
		do
		{
			int aBase = aColumn * 9;
			if (gGame->currentBook()->isCell(aColumn, aRow) != 0 && gGame->mLetters[aBase + aRow] != 0)
			{
				float aX;
				double aYD;
				cellCorner(aColumn, aRow, &aX, &aYD);
				float aY = (float)aYD;
				uint32_t aColor = 0xff48252c;
				float aAngle = 0.0f;
				if (gBonnies->inModal() == 0 && mField7C == 0 && gGame->mFieldC60 == 0 && gBonnies->mPaused == 0 &&
					mField58 == 1)
				{
					if (onPath(aColumn, aRow))
					{
						if (gGame->isWord() != 0)
							aColor = addColor(0xff48252c, 0, 10, 0x78, 10);
						else
							aColor = addColor(0xff48252c, 0, 0x8c, 10, 10);
					}
					if (mField88 == aColumn && mField8C == aRow)
					{
						if (gGame->mPathLength < 1)
							aColor = addColor(aColor, 0, 0x50, 0x32, 0x32);
						else
							aColor = addColor(aColor, 0, 0x1e, 0x1e, 0x1e);
					}
				}
				int i = aBase + aRow;
				int aTime = ticks() - gGame->mField2F8[i];
				float aProgress = (float)(aTime * (double)0.0016666667f);
				if (aProgress < 0.0f)
					aProgress = 0.0f;
				if (gGame->mField2F8[i] == 0 || gGame->mField43C[i] == 0)
					aProgress = 1.0f;
				else if (!(aProgress <= 1.0f) && gGame->mField43C[i] != '^')
				{
					gGame->mField2F8[i] = 0;
					gGame->mField43C[i] = 0;
				}
				int aPass = 0;
				do
				{
					char aLetter;
					if (aPass == 0)
						aLetter = gGame->mField43C[i];
					else
						aLetter = gGame->mLetters[i];
					uint32_t aTextColor = aColor;
					uint32_t aShadow = 0x6fffffff;
					float aPhase;
					if (aPass == 0)
						aPhase = (float)(1.0f - (double)aProgress);
					else
						aPhase = aProgress;
					if (gGame->mField43C[i] == '^')
					{
						if (aPass == 0)
							goto nextPass;
						aPhase = (float)((aTime - 0x280) * (double)0.0025f);
					}
					if (aPhase <= 0.0f)
						goto nextPass;
					if (!(aPhase <= 1.0f))
						aPhase = 1.0f;
					{
						char aBuffer[2] = {aLetter, 0};
						Str aLetterText = aBuffer;
						const Frame* aPicture = nullptr;
						if (aLetter == '?')
							aPicture = gBonnies->mImages[9]->mFrames;
						else if (aLetter == '!')
							aPicture = gBonnies->mImages[30]->mFrames;
						else if (aLetter == '*')
							aPicture = gBonnies->mImages[10]->mFrames;
						double aFade = 1.0f;
						if (gGame->mFallOffsets[i] < -0.08f && (gGame->mKinds[i] & 4) != 0)
						{
							aFade = 1.0f - (-0.08f - (double)gGame->mFallOffsets[i]) * 10.0f;
							if (!(aFade <= 1.0f))
								aFade = 1.0f;
						}
						float aAlpha = (float)(aFade * aPhase);
						if ((gGame->mTileFlags[i] & 4) != 0)
							aTextColor = (aColor & 0xff123456) | 0x123456;
						// (the wobble moves the cell's corner itself, so the second pass moves it again)
						if ((gGame->mKinds[i] & 1) != 0)
						{
							int aWobble = (int)gLastActiveTime % 800;
							aX = (float)((std::sin(aWobble * (double)0.00125f * 3.14159f) - 0.5f) * 4.0f * 0.00125f + aX);
							aWobble = (int)gLastActiveTime % 0x514;
							aY = (float)((std::sin(aWobble * (double)0.00076923077f * 3.14159f) - 0.5f) * 4.0f *
								0.0016666667f + aY);
							if (aPhase < 1.0f)
							{
								double aTurn = std::sin(aProgress * (double)1.570795f);
								if (aPass != 0)
									aTurn += 1.0f;
								aAngle = (float)(aTurn * 180.0f);
							}
						}
						if ((gGame->mKinds[i] & 2) != 0)
						{
							int aGlow = (int)gLastActiveTime % 0x708;
							aShadow = 0xaf000000;
							aTextColor = 0xffffffff;
							aAlpha = (float)((std::sin(aGlow * (double)0.00055555557f * 3.14159f) * 0.4f + 0.5f) * aAlpha);
						}
						if ((gGame->mFieldC5C != 0 || (gBonnies->isPaused() == 0 && isInactive() == 0) ||
								gGame->mFieldC44 != 0) &&
							0.02f < aAlpha && (gGame->mTileFlags[i] & 2) == 0)
						{
							float aDrawX = aX;
							float aDrawY = (float)(gGame->mFallOffsets[i] + (double)aY);
							int aSwapping = 0;
							uint32_t aStart;
							if (gGame->mShuffleTime != 0)
							{
								aStart = gGame->mShuffleTime;
								goto shuffle;
							}
							if (gGame->mFieldC7C != 0 && (gGame->mKinds[i] & 2) != 0)
							{
								aStart = gGame->mFieldC7C;
								aSwapping = 1;
							shuffle:
								float aStep = (float)((int)(ticks() - aStart) * (double)0.0016666667f);
								if (!(aStep < 1.0f))
								{
									gGame->mShuffleTime = 0;
									gGame->mFieldC7C = 0;
									aStep = 1.0f;
								}
								float aFromX;
								double aFromY;
								cellCorner(gGame->mFrom[i][0], gGame->mFrom[i][1], &aFromX, &aFromY);
								double aLeft = 1.0f - (double)aStep;
								aDrawX = (float)(aFromX * aLeft + aStep * (double)aX);
								aDrawY = (float)(aLeft * aFromY + aStep * (double)aDrawY);
							}
							if (aPicture != nullptr)
							{
								if (aLetter == '!')
								{
									drawFrame(gBonnies->mImages[31]->mFrames, aDrawX, aDrawY, aAlpha, 0);
									drawFrameGlow(aPicture, aDrawX, aDrawY, aAlpha, 0x55, 1, 2, 0);
								}
								else
									drawFrame(aPicture, aDrawX, aDrawY, aAlpha, 0);
							}
							else
							{
								if (aAlpha < 1.0f)
								{
									aTextColor = (aTextColor & 0xffffff) | (uint32_t)ftol((double)(aTextColor >> 24) * aAlpha) << 24;
									aShadow = (aShadow & 0xffffff) | (uint32_t)ftol((double)(aShadow >> 24) * aAlpha) << 24;
								}
								gBonnies->mFonts[6]->drawText(aLetterText, (float)(aDrawX + (double)0.0025f),
									(float)(aDrawY + (double)0.0016666667f), 0.055f, 0.07333333f, aTextColor, aShadow, 0x100,
									1.0f, aAngle);
								if (aSwapping != 0 && (gGame->mKinds[i] & 2) != 0)
								{
									aDrawY = (float)(gGame->mFallOffsets[i] + (double)aY);
									aLetterText = "";
									aLetterText += gGame->mField43C[i];
									gBonnies->mFonts[6]->drawText(aLetterText, (float)(aX + (double)0.0025f),
										(float)(aDrawY + (double)0.0016666667f), 0.055f, 0.07333333f, aTextColor, aShadow,
										0x100, 1.0f, aAngle);
								}
							}
						}
					}
				nextPass:
					aPass++;
				} while (aPass < 2);
			}
			aRow++;
		} while (aRow < 9);
		aColumn++;
	} while (aColumn < 9);
	// The hint's word glows letter by letter
	int aPeriod = 0xbe;
	if (mHintAgain != 0)
		aPeriod = 300;
	int aHintTime;
	if (mHintShown == 0 || mHintTime == 0)
		aHintTime = 0;
	else
	{
		aHintTime = ticks() - mHintTime;
		if (aHintTime != 0 && (gGame->mHintFound == 0 || aPeriod * 10 * 1.2 < (double)aHintTime))
		{
			mHintShown = 0;
			aHintTime = 0;
		}
	}
	if (mField78 != 0 && 0x4b0 < (int)(ticks() - mField74))
		mField78 = 0;
	aColumn = 0;
	aCell = 0;
	do
	{
		int aRow = 0;
		do
		{
			if (gGame->currentBook()->isCell(aColumn, aRow) != 0)
			{
				int i = aCell + aRow;
				if ((gGame->mTileFlags[i] & 0x40) != 0)
				{
					float aX;
					double aYD;
					cellCorner(aColumn, aRow, &aX, &aYD);
					float aY = (float)aYD;
					int aTurn = ftol((int)ticks() % 600 * (double)0.0016666667f * 360.0f);
					drawAngleFrame(aX, aY, 0.0600000024f, 0.08f, 0xff00f835, 0xbf1939ff, 0xff00f835, 3, (float)aTurn);
				}
				if (aHintTime != 0)
				{
					int j = 0;
					if (0 < gGame->mHintLength)
					{
						do
						{
							if (gGame->mHintColumns[j] == aColumn && gGame->mHintRows[j] == aRow)
							{
								float aX;
								double aYD;
								cellCorner(aColumn, aRow, &aX, &aYD);
								float aY = (float)aYD;
								float aTime = (float)aHintTime;
								double aWave = (aTime - (j + 1.5f) * (double)aPeriod) / aPeriod;
								if (0.0f < aWave)
									aWave = aWave * 0.5f;
								if (-0.8796452f <= aWave && aWave <= 0.8796452f)
								{
									double aCos = std::cos(aWave * 1.7857143f) * 0.15f;
									float aGlow = (float)aCos;
									double aLevel = aCos + 0.3f;
									if (mHintAgain != 0)
										aLevel = aLevel * 1.5f;
									if (0.0f < aLevel)
									{
										uint32_t aEdge = (uint32_t)ftol(aGlow * (double)255.0f) << 24 | 0xffff00;
										uint32_t aMiddle = (uint32_t)ftol(aLevel * 255.0f) << 24 | 0xffffff;
										int aTurn = ftol(aTime * (double)0.0028571428f * 360.0f) % 0x168;
										drawAngleFrame(aX, aY, 0.0600000024f, 0.08f, aEdge, aMiddle, aEdge, 0x1e, (float)aTurn);
									}
								}
							}
							j++;
						} while (j < gGame->mHintLength);
					}
				}
				else if (mField78 != 0 && (gGame->mTileFlags[i] & 0x20) == 0)
				{
					int aTime = ticks() - mField74;
					int aNear = 0x4b0 - aTime;
					if (aTime < aNear)
						aNear = aTime;
					float aLevel = 1.0f;
					if (aNear < 0x78)
						aLevel = (float)(aNear * (double)0.008333334f);
					float aX;
					double aYD;
					cellCorner(aColumn, aRow, &aX, &aYD);
					float aY = (float)aYD;
					int aTurn = ftol(aTime % 600 * (double)0.0016666667f * 360.0f);
					uint32_t aEdge = (uint32_t)ftol(aLevel * (double)255.0f) << 24 | 0xf835;
					uint32_t aMiddle = (uint32_t)ftol(aLevel * (double)191.0f) << 24 | 0x1939ff;
					drawAngleFrame(aX, aY, 0.0600000024f, 0.08f, aEdge, aMiddle, aEdge, 3, (float)aTurn);
				}
			}
			aRow++;
		} while (aRow < 9);
		aColumn++;
		aCell += 9;
	} while (aCell + 0x6d < 0xbe);
	// The path's arrows (a word longer than ten letters gets a mark at the eleventh instead)
	int aLetters = 0;
	int i = 0;
	if (0 < gGame->mPathLength)
	{
		do
		{
			int aRow = gGame->pathRow(i);
			int aPathColumn = gGame->pathColumn(i);
			float aX = (float)(aPathColumn * (double)0.0575f + gGame->currentBook()->mOffsetX);
			double aRowY = aRow * (double)0.07666667f + gGame->currentBook()->mOffsetY;
			double aHalf = (aPathColumn & 1) != 0 ? 0.5f : 0.0f;
			aX = (float)(aX + (double)0.0275f);
			aLetters++;
			float aY = (float)(aHalf * 0.07666667f + aRowY + 0.036666665f);
			char c = gGame->mLetters[gGame->pathColumn(i) * 9 + gGame->pathRow(i)];
			if (c == 'Q' || c == 'a' || c == 'c' || c == 'e' || c == 'i' || c == 'l' || c == 'r' || c == 's' ||
				c == 't')
				aLetters++;
			if (aLetters != 10)
			{
				if (10 < aLetters)
					drawFrame(gBonnies->mImages[12]->mFrames, (float)(aX - (double)0.0275f),
						(float)(aY - (double)0.036666665f), 1.0f, 0);
				else if (i < gGame->mPathLength - 1)
				{
					int aNext = i + 1;
					gGame->pathRow(aNext);
					gGame->pathColumn(aNext);
					gGame->currentBook();
					gGame->currentBook();
					int aDirection;
					if (isNeighbour(gGame->pathColumn(i), gGame->pathRow(i), gGame->pathColumn(aNext), gGame->pathRow(aNext),
							&aDirection) != 0)
					{
						if (gGame->isWord() != 0)
							aDirection += 6;
						drawFrame(&gBonnies->mImages[11]->mFrames[aDirection], (float)(aX - (double)0.0425f),
							(float)(aY - (double)0.0566666685f), 1.0f, 0);
					}
				}
			}
			i++;
		} while (i < gGame->mPathLength);
	}
	// The path's word, its score, and the moves left or the clock
	Str aLine = "";
	uint32_t aShadow = 0xafaf1f1f;
	Str aWord = gGame->word();
	if (gGame->isWord() != 0)
	{
		aWord = gGame->spelling();
		aShadow = 0xaf1faf1f;
	}
	else
		aWord = expandWord(aWord, 1);
	int aScore = gGame->wordScore();
	aLine = "";
	if (10 < (int)aWord.size())
		aLine += aWord.substr(0, 10);
	else
		aLine += Str(aWord);
	if (0 < aScore)
	{
		aLine += lit(0x4583b4);
		aLine += intToStr(aScore);
	}
	float aLineY;
	if (gGame->mFieldC5C != 0)
	{
		aLineY = 0.831666708f;
		if (8 < gGame->currentBook()->mRows)
			aLineY = 0.848333359f;
	}
	else
	{
		aLineY = 0.786666691f;
		if (8 < gGame->currentBook()->mRows)
			aLineY = 0.8f;
		else if (7 < gGame->currentBook()->mRows)
			aLineY = 0.790000021f;
	}
	if (!aWord.empty())
		gBonnies->mFonts[5]->drawText(aLine, 0.49000001f, aLineY, 0.452499986f, 0.0416666679f, 0xffffffff, aShadow,
			0 < aScore ? 0x908 : 0x108, 1.0f, 0.0f);
	if (aScore == 0 || aWord.empty())
		gBonnies->mField1C = 1;
	if (gGame->mFieldC5C != 0)
	{
		aLine = "";
		int aMoves = gGame->mGoal + gGame->mFieldC88;
		aMoves = (aMoves < 0 ? 0 : aMoves);
		aLine += getText(0x52c);
		aLine += intToStr(aMoves);
		int aBlink = 0;
		if (aMoves <= 2)
			aBlink = 0x96;
		else if (aMoves <= 4)
			aBlink = 0x15e;
		uint32_t aColor = 0xffffffff;
		if (aBlink != 0 && !(1 < (int)ticks() / aBlink % 5))
			aColor = addColor(0xffffffff, -100, 0x3c, -0x14, -0x14);
		gBonnies->mFonts[4]->drawText(aLine, 0.638749957f, 0.901666641f, 0.153750002f, 0.06666667f, aColor, 0x6f000000,
			0x109, 1.0f, 0.0f);
	}
	else
	{
		drawFrame(&gBonnies->mImages[34]->mFrames[1], 0.54124999f, 0.943333328f, 1.0f, 0);
		float aClock = (float)(gGame->mFieldC94 + (double)gGame->mFieldC8C);
		if (0.001f < aClock)
		{
			float aAlpha = 1.0f;
			if (aClock <= 0.05f)
				aAlpha = (float)(std::sin((int)ticks() % 0x140 * (double)0.003125f * 3.14159f) * 0.7f + 0.3f);
			else if (aClock <= 0.1f)
				aAlpha = (float)(std::sin((int)ticks() % 600 * (double)0.0016666667f * 3.14159f) * 0.85f + 0.15f);
			else if (aClock <= 0.2f)
				aAlpha = (float)(std::sin((int)ticks() % 0x4b0 * (double)0.00083333335f * 3.14159f) * 0.7f + 0.3f);
			pushClip(0.0f, 0.0f, (float)((aClock * (double)236.0f + 23.0f) * 0.00125f + 0.54124999f), 1.0f);
			drawFrame(gBonnies->mImages[34]->mFrames, 0.54124999f, 0.948333323f, aAlpha, 0);
			popClip();
		}
	}
	gFloatTexts->draw();
	host()->trace(lit(0x458390));
}

// The chapter's summary (0x410790 keeps them for it)
static Str gSummaryName;          // 0x478964: the name the player gave
static int gSummaryLast;          // 0x478500: the chapter was its book's last
static int gSummaryBook;          // 0x478504
static int gSummaryLevel;         // 0x478508

// 0x410790: the next step of the screen's sequence (mField58) when its time (mField60) is up
void GameScreen::advance()
{
	mField5C = ticks();
	mField60 = 0;
	switch (mField58)
	{
	case 2:
		if (gBonnies->mLoadImages[0] == 0)
		{
			gBonnies->mLoadImages[7] = 1;
			mField60 = ticks() + 10;
			return;
		}
		gBonnies->mLoadImages[7] = 0;
		refresh();
		return;
	case 3:
		mField58 = 4;
		mFieldC0 = 0.0f;
		mFieldC4 = 0.0f;
		gSummaryName = gGame->mStrCC8;
		gSummaryLast = gGame->currentBook()->mLast;
		gSummaryBook = gGame->currentBook()->mBook;
		gSummaryLevel = gGame->mLevel - 1;
		mField60 = ticks() + 0x96;
		playSound(Str(lit(0x4582fc)), 0.8f, 1, 10);
		return;
	case 4:
		mField58 = 5;
		mFieldE4 = 0;
		mField60 = ticks() + 0xfa;
		return;
	case 5:
		mField58 = 6;
		mFieldE4 = 0;
		mFieldEC = 0;
		mField60 = 0;
		if (mFieldC8 == 0)
			mFieldC8 = ticks();
		return;
	case 6:
		gGame->currentBook();
		mField58 = 8;
		mField60 = ticks() + 1;
		return;
	case 7:
		mField58 = 8;
		mField60 = ticks() + 100;
		return;
	case 8:
		if (mField54 != nullptr)
			gResources->release(mField54);
		mField54 = mPicture;
		mPicture = nullptr;
		mChapter = "";
		if (gGame->currentBook()->mLast != 0)
		{
			mField58 = 9;
			showBookEnd();
			mFieldB4 = 1.0f;
			mField60 = ticks();
			mField58 = 0xd;
			return;
		}
		gGame->nextLevel();
		reset();
		mFieldB4 = 0.0f;
		mField60 = ticks() + 0xfa;
		mField58 = 0xd;
		return;
	case 12:
	case 13:
		mTyped = 0;
		mSpacesTyped = 0;
		mSpaces = 0;
		mLinesTyped = 0;
		mLines = 0;
		mTypeTime = ticks() + 400;
		mField58 = 0xe;
		mField60 = ticks() + 0xfa;
		return;
	case 14:
		if (mTyped == 9999)
		{
			mField58 = 0xf;
			mFieldB4 = 0.0f;
			mField60 = ticks() + 400;
			Str aName = "";
			aName += lit(0x4582e8);
			appendInt(aName, (gGame->mLevel & 3) + 1);
			playSound(aName);
			return;
		}
		mField60 = ticks() + 5;
		return;
	case 15:
		mField58 = 0x10;
		mField60 = ticks() + 200;
		return;
	case 16:
		mField58 = 1;
		mField60 = 0;
		findWidget(0xc9)->mFlags &= ~WIDGET_INACTIVE;
		findWidget(0xca)->mFlags &= ~WIDGET_INACTIVE;
		findWidget(0xcb)->mFlags &= ~WIDGET_INACTIVE;
		return;
	}
}

// 0x413020
void GameScreen::draw()
{
	if (mField58 == 9 && gGame->currentBook()->mLast == 0)
		drawIntro();
	else if (mField58 == 0xd)
	{
		drawIntro();
		if (mField54 != nullptr)
			drawFrame(mField54->mFrames, 0.0f, 0.0f, (float)(1.0f - (double)mFieldB4), 1);
	}
	else if (mField58 == 0xe || mField58 == 0xf)
		drawIntro();
	else if (mField58 == 9 && mField54 != nullptr)
		drawFrame(mField54->mFrames, 0.0f, 0.0f, 1.0f, 1);
	else if (mField58 == 10 && mField54 != nullptr)
		drawFrame(mField54->mFrames, 0.0f, 0.0f, 1.0f, 1);
	else if (mPicture != nullptr)
		drawFrame(mPicture->mFrames, 0.0f, 0.0f, 1.0f, 1);
	if (!(mFieldB8 < 0.001f))
	{
		drawBoard();
		Screen::draw();
		if (!mStrCC.empty())
		{
			int aStep = mTutorialStep;
			if (aStep == 0xe || ((aStep == 0 || aStep == 1 || aStep == 2 || aStep == 3) && gGame->mPathLength == 4))
			{
				Widget* aButton = findWidget((aStep == 0xe) + 0xca);
				if (aButton != nullptr)
					drawFrameGlow(gBonnies->mImages[4]->mFrames, aButton->mX,
						(float)(aButton->mY - (double)0.0033333334f), 1.0f, 0, 0, 0, 1);
			}
		}
		if (mFieldB8 < 1.0f && mPicture != nullptr)
			drawFrame(mPicture->mFrames, 0.0f, 0.0f, (float)(1.0f - (double)mFieldB8), 1);
	}
	host()->trace(lit(0x45844c));
	if (!mStrCC.empty())
		gBonnies->balloon(mStrCC, mHintX, mHintY, mHintSide, gOverlayScreen->findWidget(0xcc) != nullptr, nullptr, 1);
	if (mFlashTime != 0)
	{
		double aLevel = (double)(int)((ticks() - mFlashTime) * 2) / mFlashLength;
		if (!(aLevel <= 1.0f))
			aLevel = 2.0f - aLevel;
		if (aLevel <= 0.01f)
			mFlashTime = 0;
		else
		{
			if (!(aLevel <= 1.0f))
				aLevel = 1.0f;
			uint32_t aColor = (uint32_t)ftol((double)(mFlashColor >> 24) * aLevel) << 24 | (mFlashColor & 0xffffff);
			if (0x4000000 <= (aColor & 0xff000000))
				fillRect(0.0f, 0.0f, 1.0f, 1.0f, aColor);
		}
	}
	host()->trace(lit(0x458440));
	if (gBonnies->mPaused != 0)
	{
		host()->trace(lit(0x458418));
		return;
	}
	int aState = mField58;
	if (aState == 4 || aState == 5 || aState == 6 || aState == 7 || aState == 0xd || aState == 10 || aState == 8 ||
		(aState == 9 && gGame->currentBook()->mLast != 0))
	{
		// The chapter's summary over the board
		int aFinal = 0;
		aState = mField58;
		if (aState == 0xd || aState == 10 || aState == 8 || (aState == 9 && gGame->currentBook()->mLast != 0))
			aFinal = 1;
		float aShow = mFieldBC;
		aState = mField58;
		if (aState == 0xf || aState == 0xd || aState == 10)
			aShow = (float)(1.0f - (double)mFieldB4);
		float aHide = (float)(1.0f - (double)aShow);
		int aFade = ftol(aHide * (double)-255.0f);
		uint32_t aColor = addColor(0xffffffff, aFade, 0, 0, 0);
		uint32_t aShadow = addColor(0x8f000000, ftol(aHide * (double)-191.0f), 0, 0, 0);
		if (0.0f < aShow)
		{
			double aDark = aShow * (double)1.6666666f;
			if (!(aDark <= mFieldC4))
				aDark = mFieldC4;
			if (!(aDark <= 1.0f))
				aDark = 1.0f;
			uint32_t aBack = (uint32_t)ftol(aDark * 114.75f) << 24;
			if (aBack != 0)
				fillRect(0.0f, 0.0f, 1.0f, 1.0f, aBack);
			if (0.01f < mFieldC0)
			{
				int aImage = (gSummaryLast != 0 ? 0x16 : 0) + 0xd;
				if (mFieldC0 < 0.99f)
					drawFrameRotated(gBonnies->mImages[aImage]->mFrames, 0.5f, 0.466666669f, 0.0f, mFieldC0, aShow, 0, 0);
				else
					drawFrame(gBonnies->mImages[aImage]->mFrames, 0.11f, 0.163333327f, aShow, 0);
			}
			Font* aFont = gBonnies->mFonts[10];
			if (mField58 != 5 && mField58 != 4)
			{
				float aY = 0.423333347f;
				int aLength;
				if (gSummaryName.empty())
					aLength = 4;
				else
					aLength = (int)gSummaryName.size();
				double aBase = 0.45625001f - aLength * (double)0.0025f;
				float aLeft = (float)(aBase - 0.45f);
				float aRight = (float)(aBase + 0.01125f);
				int aRow = 0;
				do
				{
					Str aName = "";
					Str aValue = "";
					switch (aRow)
					{
					case 0:
						aName += getText(0x52e);
						appendInt(aValue, gGame->mScore);
						break;
					case 1:
					{
						int aTime;
						if (gGame->mFieldC5C != 0)
							aTime = gGame->mGoal;
						else
							aTime = gGame->mFieldC9C;
						if (aFinal != 0)
							aTime = 0;
						if (gGame->mFieldC5C != 0)
						{
							aName += getText(0x52f);
							appendInt(aValue, aTime);
						}
						else
						{
							aName += getText(0x530);
							int aSeconds = aTime % 0x3c;
							appendInt(aValue, aTime / 0x3c) += lit(0x458424);
							if (aSeconds < 10)
								aValue += lit(0x457cbc);
							appendInt(aValue, aSeconds);
						}
						break;
					}
					case 2:
						aName += getText(0x534);
						aValue = format(Str(getText(0x535)), intToStr(gSummaryLevel + 1), intToStr(0x32),
							Str(lit(0x457426)));
						break;
					case 3:
						aName += getText(0x531);
						if (gSummaryName.empty())
							aValue += getText(0x532);
						else
							aValue += gSummaryName;
						break;
					}
					aFont->drawText(aName, aLeft, aY, 0.45f, 1.0f, aColor, aShadow, 0x122, 1.0f, 0.0f);
					aFont->drawText(aValue, aRight, aY, 0.45f, 1.0f, aColor, aShadow, 0x112, 1.0f, 0.0f);
					aY = (float)(aY + (double)0.053333335f);
					if (aRow == 2)
						aY = (float)(aY + (double)0.016666668f);
					aRow++;
				} while (aRow < 4);
				drawCText(aFont, getText(0x52d), 0.199999988f, 0.9f, 0.6f, 1.0f, addColor(0xffffffcf, aFade, 0, 0, 0),
					aShadow, 0x50a, 1.0f, 0.0f);
			}
		}
	}
	else if (!mMessage.empty())
	{
		if ((int)(mMessageEnd - ticks()) < 0)
			mMessage = "";
		else
			gBonnies->mFonts[1]->drawText(mMessage, 0.125f, 0.0f, 0.75f, 1.0f, 0xffa8f2ff, 0xbf000000, 0x100, 1.0f, 0.0f);
	}
	host()->trace(lit(0x458418));
}

// 0x413a30: the tutorial's hint is put away (theClear: the marks on the board too)
void GameScreen::leave(int theClear)
{
	if (!mStrCC.empty())
	{
		findWidget(0xc9)->mFlags &= ~WIDGET_INACTIVE;
		findWidget(0xcb)->mFlags &= ~WIDGET_INACTIVE;
		findWidget(0xca)->mFlags &= ~WIDGET_INACTIVE;
		mTutorialStep = -1;
		mStrCC = "";
		if (theClear != 0)
			gGame->clearTileMarks();
	}
	if (gOverlayScreen != nullptr)
	{
		Widget* aTip = gOverlayScreen->findWidget(0xcc);
		if (aTip != nullptr)
			gOverlayScreen->removeWidget(aTip);
	}
}

// 0x478514: the tutorial's hint is to be put away at the next update (0x414400 sets it)
static int gLeaveHint;

// 0x413ae0
void GameScreen::update()
{
	int aDelta = (int)(ticks() - mFieldF4);
	mFieldF4 = ticks();
	if (500 < aDelta)
		aDelta = 500;
	mFieldC0 = 1.0f;
	mFieldBC = 1.0f;
	mFieldB4 = 1.0f;
	if (mField58 == 0x13 && gBonnies->inModal() == 0)
	{
		mField58 = 1;
		if (gBonnies->mLastResult == 1 && gGame != nullptr)
		{
			gGame->mFieldC94 = 0.0f;
			gGame->mGoal = 0;
			gGame->mFieldC8C = 0.0f;
			gGame->mFieldC88 = 0;
			gGame->mFieldC48 = 1;
			gGame->mFieldC60 = ticks() - 1;
		}
	}
	if (mField58 == 0x12 && gBonnies->inModal() == 0)
	{
		showInterstitial(0, 1);
		gGameScreen->mField58 = 0x11;
	}
	int aState = mField58;
	if (aState == 0x11)
	{
		if (gBonnies->inModal() == 0)
		{
			if (gGame->mLevel == 0x33 || gGame->mFieldC44 != 0)
			{
				gGame->mFieldC44 = 1;
				gBonnies->setState(2, 1, 0);
				return;
			}
			mFieldB8 = 0.0f;
			mTyped = 0;
			mSpacesTyped = 0;
			mSpaces = 0;
			mLinesTyped = 0;
			mLines = 0;
			mTypeTime = ticks() + 400;
			mField58 = 0xe;
			mField60 = ticks();
		}
	}
	else
	{
		float aLevel;
		bool aFull = false;
		if (aState == 3)
		{
			int aStart = mField5C;
			aLevel = (float)(1.0 - (double)(float)(int)(ticks() - aStart) / (double)(float)(mField60 - aStart));
			mFieldB8 = aLevel;
			if (aLevel <= 1.0f)
			{
				if (aLevel < 0.0f)
					mFieldB8 = 0.0f;
			}
			else
				aFull = true;
		}
		else if (aState == 4)
		{
			mFieldC0 = 0.0f;
			aLevel = (float)((double)(float)(int)(ticks() - mField5C) / (double)(float)(mField60 - mField5C));
			mFieldC4 = aLevel;
			if (aLevel < 1.0f)
			{
				if (aLevel < 0.0f)
					mFieldC4 = 0.0f;
			}
			else
				mFieldC4 = 1.0f;
		}
		else if (aState == 5)
		{
			aLevel = (float)((double)(float)(int)(ticks() - mField5C) / (double)(float)(mField60 - mField5C));
			mFieldC0 = aLevel;
			if (aLevel < 1.0f)
			{
				if (aLevel < 0.0f)
					mFieldC0 = 0.0f;
			}
			else
				mFieldC0 = 1.0f;
		}
		else if (aState == 7)
		{
			int aStart = mField5C;
			aLevel = (float)(1.0 - (double)(float)(int)(ticks() - aStart) / (double)(float)(mField60 - aStart));
			mFieldBC = aLevel;
			if (aLevel <= 1.0f)
			{
				// The exe tests mFieldB8 here
				if (mFieldB8 < 0.0f)
					mFieldBC = 0.0f;
			}
			else
				mFieldBC = 1.0f;
		}
		else if (aState == 0xd || aState == 0xf || aState == 0xc || aState == 10)
		{
			aLevel = (float)((double)(float)(int)(ticks() - mField5C) / (double)(float)(mField60 - mField5C));
			mFieldB4 = aLevel;
			if (aLevel <= 1.0f)
			{
				if (aLevel < 0.0f)
					mFieldB4 = 0.0f;
			}
			else
				mFieldB4 = 1.0f;
		}
		else if (aState == 0x10)
		{
			// The same as state 3's (the exe jumps into it)
			aLevel = (float)((double)(float)(int)(ticks() - mField5C) / (double)(float)(mField60 - mField5C));
			mFieldB8 = aLevel;
			if (aLevel <= 1.0f)
			{
				if (aLevel < 0.0f)
					mFieldB8 = 0.0f;
			}
			else
				aFull = true;
		}
		else if (aState == 1)
			aFull = true;
		else if (aState == 6)
		{
			// The bonus for the time or the moves left, counted into the score
			int aTime = mFieldE4 + aDelta;
			mFieldE4 = aTime;
			aTime = aTime & -(int)(0 <= aTime);
			mFieldE4 = aTime;
			if (0x9c4 < aTime)
				aTime = 0x9c4;
			int aStep = mFieldEC;
			mFieldE4 = aTime;
			if (aStep == 0)
			{
				mFieldE8 = 0;
				if (999 < aTime)
				{
					mFieldE4 = 0;
					mFieldEC = 1;
					mFieldF0 = ticks();
				}
			}
			else if (aStep == 1)
			{
				int aSince = (int)(ticks() - mFieldF0);
				if (aSince < 0)
					aSince = 0;
				else if (0x9c4 < aSince)
					aSince = 0x9c4;
				int aCount = gGame->mFieldC5C != 0 ? 3 : 4;
				int anInterval = ftol((double)aCount * (((1.0f - aSince * (double)0.0004f) * 0.55f) + 0.45f) * 25.0f);
				bool aCounted = false;
				if (anInterval < mFieldE4)
				{
					do
					{
						mFieldE4 -= anInterval;
						if (gGame->mFieldC5C == 0)
						{
							int aLeft = gGame->mFieldC9C;
							if (aLeft < 1)
							{
								mFieldE4 = 0;
								mFieldEC = 2;
							}
							else
							{
								int aTake = aLeft % 10 + 10;
								if (aTake <= aLeft)
									aLeft = aTake;
								gGame->mFieldC9C -= aLeft;
								gGame->mScore += (unsigned char)gGame->difficultyFactor() * (gGame->mLevel + 1) * aLeft * 5;
								aCounted = true;
							}
						}
						else if (gGame->mGoal < 1)
						{
							mFieldE4 = 0;
							mFieldEC = 2;
						}
						else
						{
							gGame->mGoal--;
							gGame->mScore += (unsigned char)gGame->difficultyFactor() * (gGame->mLevel + 1) * 0x14;
							aCounted = true;
						}
					} while (anInterval < mFieldE4);
					if (aCounted)
					{
						int aRise = 0x50;
						if (mFieldE8 < 0x51)
							aRise = mFieldE8;
						double aPitch = (double)aRise * 0.035f + 1.0f;
						gSound->play(Str(lit(0x45845c)), 0.4f, 1, 0, (float)(aPitch + aPitch));
						mFieldE8++;
					}
				}
				gGame->mFieldC70 = gGame->mScore;
			}
			else if (aStep == 2 && 500 < aTime)
			{
				mFieldE4 = 0;
				mFieldEC = 3;
			}
		}
		if (aFull)
			mFieldB8 = 1.0f;
	}
	gGame->update();
	if (mField60 != 0 && mField60 <= (int)ticks())
	{
		advance();
		return;
	}
	if (gLeaveHint != 0)
	{
		gLeaveHint = 0;
		leave(1);
	}
	if (mField58 == 1)
	{
		if (mField78 != 0)
			mField70 = ticks();
		int aHintAfter = (-(int)(gGame->mDifficulty != 0) & 2000) + 18000;
		int anIdleAfter = (-(int)(gGame->mDifficulty != 0) & 4000) + 13000;
		if (gGame->mRecord.mBestLevel < 1)
		{
			// New players get their hints sooner
			float aFactor;
			if (gGame->mFieldCA0 < 3)
				aFactor = 5.0f;
			else if (gGame->mFieldCA0 < 5)
				aFactor = 0.5f;
			else
				aFactor = 0.75f;
			aHintAfter = ftol((double)aHintAfter * aFactor);
			anIdleAfter = ftol((double)anIdleAfter * aFactor);
		}
		if (!mStrCC.empty() && mHintSide != 6 && mHintShown == 0 && mField78 == 0)
		{
			mField70 = ticks();
			mHintTime = ticks();
			Screen::update();
			return;
		}
		if (gBonnies->inModal() == 0 && mField7C == 0 && gGame->mFieldC60 == 0 && gBonnies->mPaused == 0)
		{
			if (mHintShown == 0)
			{
				if (mField78 == 0 && ((0 < mFieldB0 && mFieldB0 < (int)ticks()) ||
					(aHintAfter < (int)(ticks() - gGame->mFieldD8C) && anIdleAfter < (int)(ticks() - mHintTime))))
				{
					showHintWord();
					mField70 += 3000;
					Screen::update();
					return;
				}
				if (mHintShown == 0 && mField78 == 0 && 2000 < (int)(ticks() - mField70))
				{
					int aMarked = gGame->countFlag20();
					int aTiles = gGame->countTiles();
					int aMissing = aTiles - aMarked;
					int aWait = 7000;
					if (aMissing == 3)
						aWait = 0x157c;
					else if (aMissing == 2)
						aWait = 4000;
					else if (aMissing == 1)
						aWait = 0x9c4;
					int aTotal = 1;
					if (0 < aTiles)
						aTotal = aTiles;
					if (aWait <= (int)(ticks() - mField70))
					{
						if ((double)aMarked / (double)aTotal < 0.85f)
						{
							mField70 = ticks() + 2000;
							Screen::update();
							return;
						}
						mField78 = 1;
						mField70 = ticks();
						mField74 = mField70;
						if (2 < aMissing)
						{
							mHintTime += 0xdac;
							Screen::update();
							return;
						}
					}
				}
			}
		}
		else
		{
			mHintShown = 0;
			mHintTime = ticks();
			mField70 = ticks();
		}
	}
	Screen::update();
}

// 0x478554, 0x478558: the times of the last two mouse-ups
static uint32_t gUpBefore;
static uint32_t gUpLast;

// 0x414400
int GameScreen::handleEvent(Event* theEvent)
{
	int aLastRow = mField8C;
	int aLastColumn = mField88;
	bool aShared = false;
	switch (theEvent->mType)
	{
	case 2:
	case 3:
	case 6:
	case 7:
		aShared = true;
		break;
	case 8:
	case 9:
		if (theEvent->mA != 0x13c)
			aShared = true;
		break;
	}
	if (aShared)
	{
		// A click or key ends the waits, the count and the typing, or puts the tutorial's hint away
		int aState = mField58;
		if (aState == 0xb)
		{
			mField60 = ticks() - 1;
			return 0;
		}
		if (aState == 6)
		{
			mField60 = ticks() - 1;
			gGame->addLevelBonus();
			return 0;
		}
		if (aState == 0xe)
		{
			int aTyped = mTyped;
			if (aTyped < 9999)
				aTyped = 9999;
			mTyped = aTyped;
			return 0;
		}
		if (!mStrCC.empty() && (mHintSide == 5 || mHintSide == 6) && gOverlayScreen->findWidget(0xcc) == nullptr)
		{
			int aType = theEvent->mType;
			if (((aType == 8 || aType == 9) && (theEvent->mA == 0x1b || theEvent->mA == 0xd)) || aType == 6 ||
				aType == 7)
			{
				leave(1);
				return 0;
			}
			if (aType == 2)
			{
				float aX, aY;
				firstCellPos(&aX, &aY);
				if (mHintSide == 6 && 0.23333333f <= theEvent->mY && theEvent->mY <= 0.65f &&
					theEvent->mX <= (double)aX - 0.04375f)
				{
					leave(1);
					return 0;
				}
			}
		}
	}
	switch (theEvent->mType)
	{
	case 1:
		// The mouse moved: the cursor's cell, and with a button down the cells it crossed
		if (mField88 != -1 || !(distance(theEvent->mX, theEvent->mY, mField90, mField94) < 0.00375f))
		{
			if (gBonnies->inModal() == 0 && mField58 == 1 && gBonnies->mPaused == 0 && mField7C == 0 &&
				gGame->mFieldC60 == 0)
			{
				int aSteps = ftol(distance(theEvent->mX, theEvent->mY, mField90, mField94) * 156.52174f);
				if (aSteps < 2)
					aSteps = 1;
				if (mFieldA4 == 0)
					aSteps = 1;
				int i = 0;
				if (0 < aSteps)
				{
					float aCount = (float)aSteps;
					do
					{
						i++;
						double aPart = (double)(float)i / aCount;
						float aX = (float)(aPart * theEvent->mX + (1.0f - aPart) * mField90);
						float aY = (float)(aPart * theEvent->mY + (1.0f - aPart) * mField94);
						int aColumn, aRow;
						float aDistance;
						if (hitTile(aX, aY, &aColumn, &aRow, &aDistance, 1) == 0)
						{
							mField88 = -1;
							mField8C = -1;
						}
						else if (aDistance <= 0.0299f || mFieldA4 == 0)
						{
							mField88 = aColumn;
							mField8C = aRow;
						}
						else
							continue;
						if (mFieldA4 == 0)
							break;
						if (mField88 != -1)
						{
							gGame->pick(mField88, mField8C, aLastColumn, aLastRow, 1);
							aLastColumn = mField88;
							aLastRow = mField8C;
						}
					} while (i < aSteps);
				}
			}
			mField90 = theEvent->mX;
			mField94 = theEvent->mY;
			if (mFieldA4 != 0)
				return 0;
		}
		break;
	case 2:
	{
		if (Screen::handleEvent(theEvent) == 0)
			return 0;
		mField90 = theEvent->mX;
		mField94 = theEvent->mY;
		// A click on the path's last tile again is silent
		bool aMuted = false;
		if (0 < gGame->mPathLength && mField80 == mField88 && mField84 == mField8C &&
			gGame->pathColumn(gGame->mPathLength - 1) == mField88 && gGame->pathRow(gGame->mPathLength - 1) == mField8C)
		{
			aMuted = true;
			gSound->mPaused++;
		}
		if (gBonnies->inModal() == 0 && mField58 == 1 && gBonnies->mPaused == 0 && mField7C == 0 &&
			gGame->mFieldC60 == 0)
		{
			bool aPick = true;
			if (mField88 == -1)
			{
				int aColumn, aRow;
				float aDistance;
				if (hitTile(theEvent->mX, theEvent->mY, &aColumn, &aRow, &aDistance, 0) != 0)
				{
					if (0 < gBonnies->mSpecialMode)
					{
						mField88 = aColumn;
						mField8C = aRow;
					}
					else if ((gGame->mTileFlags[aColumn * 9 + aRow] & 2) != 0)
					{
						// A fixed tile: the hint twice, then the error sound
						aPick = false;
						int aSeen = gGame->mRecord.mFieldAC;
						gGame->mRecord.mFieldAC = aSeen + 1;
						if (aSeen < 2)
							gGameScreen->showHint(Str(getText(0x53c)), 999, 0.5f, 0.4f, 5, 1);
						else
							playSound(Str(lit(0x457620)), 0.65f);
					}
					else if (gGame->mLetters[aColumn * 9 + aRow] == '*')
					{
						aPick = false;
						if (!mStrCC.empty() && mTutorialStep == 10)
							playSound(Str(lit(0x457620)), 0.65f);
						else
						{
							int aSeen = gGame->mRecord.mFieldB0;
							gGame->mRecord.mFieldB0 = aSeen + 1;
							if (aSeen < 2)
								gGameScreen->showHint(Str(getText(0x53d)), 999, 0.5f, 0.4f, 5, 1);
							else
								playSound(Str(lit(0x457620)), 0.65f);
						}
					}
				}
			}
			if (aPick)
				gGame->pick(mField88, mField8C, aLastColumn, aLastRow, 0);
		}
		if (aMuted)
			gSound->mPaused--;
		return 0;
	}
	case 3:
	{
		gUpBefore = gUpLast;
		gUpLast = ticks();
		int aSince = (int)(gUpLast - gUpBefore);
		if (Screen::handleEvent(theEvent) == 0)
			return 0;
		if (gBonnies->mPaused != 0)
		{
			gBonnies->mPaused = 0;
			return 0;
		}
		// Letting go over a word spells it, and so does a double click on the path's last tile
		if ((mFieldA4 == 0 || mField88 != -1) &&
			((mFieldA4 != 0 && gGame->isWord() != 0 && 1 < mFieldA8) ||
				(doubleClickDistance() <= aSince && aSince <= doubleClickTime() && mFieldA4 != 0 && mFieldA8 == 1 &&
					1 < gGame->mPathLength && mField80 == mField88 && mField84 == mField8C &&
					gGame->pathColumn(gGame->mPathLength - 1) == mField88 &&
					gGame->pathRow(gGame->mPathLength - 1) == mField8C)))
			gGame->submit();
		mField80 = mField88;
		mField84 = mField8C;
		mFieldA4 = 0;
		break;
	}
	case 6:
	case 7:
		// The right button drops the path
		if (gOverlayScreen != nullptr && gOverlayScreen->findWidget(0xcc) != nullptr)
			gLeaveHint = 1;
		mFieldA4 = 0;
		if (gBonnies->inModal() == 0 && mField7C == 0 && gGame->mFieldC60 == 0 && mField58 == 1 &&
			gBonnies->mPaused == 0 && (mStrCC.empty() || (mTutorialStep != 1 && mTutorialStep != 3)))
		{
			if (0 < gGame->mPathLength)
			{
				gGame->mFieldCDC = ticks();
				playSound(Str(lit(0x458490)), 0.75f);
			}
			gGame->mPathLength = 0;
			gGame->mWordChecked = 0;
		}
		break;
	case 8:
	{
		int aKey = theEvent->mA;
		if (aKey < 0x21)
		{
			if (aKey == 0x20 || aKey == 0x13)
			{
				// Space and Pause
				if (gBonnies->mPaused != 0 || (mField58 == 1 && gGame->mFieldC60 == 0))
					gBonnies->mPaused = (gBonnies->mPaused == 0);
				return 0;
			}
			if (aKey == 0x1b)
			{
				if (gBonnies->mPaused == 0)
				{
					if (isPlaying() != 0)
						showGameMenu();
					return 0;
				}
				gBonnies->mPaused = 0;
				return 0;
			}
		}
		else if (aKey == 0x11a)
		{
			// F1: the help
			if (gBonnies->inModal() == 0 && isPlaying() != 0 && mStrCC.empty())
				gBonnies->setState(4, 1, 0);
			return 0;
		}
		break;
	}
	case 0xd:
		switch (theEvent->mA)
		{
		case 0xc9:
			showGameMenu();
			return 0;
		case 0xca:
			if (!gGameScreen->mStrCC.empty() && gGameScreen->mTutorialStep == 0xe)
				leave(1);
			gGame->submit();
			return 0;
		case 0xcb:
		{
			if (mField11C == 0)
				return 0;
			if (findWidget(0xcb) != nullptr && (findWidget(0xcb)->mFlags & WIDGET_INACTIVE) != 0)
				return 0;
			if (mFieldAC != 0)
			{
				// No moves left to shuffle: the game is over
				messageDialog(0, Str(getText(0x53e)), Str(getText(0x53f)), Str(getText(0x540)), Str(""), Str(""), 0, 0,
					0, 0, 0);
				mField58 = 0x13;
				return 0;
			}
			int aStar = (int)(0 < gGame->mRecord.mBestLevel);
			gGame->mRecord.mSeen[14] = 1;
			playSound(Str(lit(0x458478)), 0.75f);
			gGame->shuffle(aStar);
			int aText;
			int aStep;
			if (gGame->mRecord.mSeen[15] == 0)
			{
				gGame->mRecord.mSeen[16] = 1;
				gGame->mRecord.mSeen[15] = 1;
				if (aStar != 0)
					gGame->mRecord.mSeen[24] = 1;
				leave(1);
				if (gGame->mFieldC5C == 0)
				{
					aText = exeI32(0x4581dc + (-(int)(aStar != 0) & -7) * 4);
					aStep = 0x10;
				}
				else
				{
					aText = exeI32(0x4581d8 + (-(int)(aStar != 0) & -7) * 4);
					aStep = 0xf;
				}
			}
			else
			{
				if (aStar == 0)
					return 0;
				if (gGame->mRecord.mSeen[24] != 0)
					return 0;
				gGame->mRecord.mSeen[24] = 1;
				leave(1);
				aStep = 0xf;
				aText = 0x46e;
			}
			showHint(Str(getText(aText)), aStep, 0.5f, 0.4f, 6, 0);
			return 0;
		}
		case 0xcc:
			gLeaveHint = 1;
			return 0;
		}
	}
	return Screen::handleEvent(theEvent);
}

// port (test runs): the state tools/realplay.py plays from, one "key value..." a line, positions in 0-1 screen
// fractions (Start.h). Read only: it draws no random number and changes nothing.
std::string testState()
{
	std::string aText;
	char aLine[256];
	snprintf(aLine, sizeof aLine, "modal %d\n", gBonnies != nullptr ? gBonnies->mDepth : -1);
	aText += aLine;
	snprintf(aLine, sizeof aLine, "paused %d\n", gBonnies != nullptr ? gBonnies->mPaused : -1);
	aText += aLine;
	if (gBonnies != nullptr)
	{
		// the buttons a player could press: the top modal screen's, or every visible screen's when none is modal
		for (Screen* aScreen = gBonnies->mHead; aScreen != nullptr; aScreen = aScreen->mNext)
		{
			if (gBonnies->mDepth > 0 && aScreen != gBonnies->mModal[gBonnies->mDepth])
				continue;
			for (Widget* aWidget = aScreen->mHead; aWidget != nullptr; aWidget = aWidget->mNext)
			{
				if ((aWidget->mType != 1 && aWidget->mType != 3) || aWidget->isActive() == 0)
					continue;
				float aX = aScreen->mX + aWidget->mX + aWidget->mWidth / 2;
				float aY = aScreen->mY + aWidget->mY + aWidget->mHeight / 2;
				if (aWidget->mHitX != 0.0f || aWidget->mHitY != 0.0f)
				{
					aX = aScreen->mX + aWidget->mX + aWidget->mHitX + aWidget->mHitWidth / 2;
					aY = aScreen->mY + aWidget->mY + aWidget->mHitY + aWidget->mHitHeight / 2;
				}
				snprintf(aLine, sizeof aLine, "button %d %d %.4f %.4f %s\n", aWidget->mType, aWidget->mId, aX, aY,
					aWidget->mText.c_str());
				aText += aLine;
			}
		}
	}
	if (gGameScreen == nullptr || gGame == nullptr)
		return aText;
	snprintf(aLine, sizeof aLine, "game %d %d %d %d %d %d %d\n", gGame->mLevel, gGame->mScore, gGame->mGoal,
		gGame->mInProgress, gGameScreen->mField58, gGameScreen->isBusy(), gGame->mFieldC60);
	aText += aLine;
	snprintf(aLine, sizeof aLine, "balloon %d %d %d\n", gGameScreen->mStrCC.empty() ? 0 : 1, gGameScreen->mHintSide,
		gGameScreen->mTutorialStep);
	aText += aLine;
	// the tutorial's balloon text, on one line, and the tiles it marks for its word
	std::string aBalloon = gGameScreen->mStrCC;
	for (size_t i = 0; i < aBalloon.size(); i++)
		if (aBalloon[i] == '\n' || aBalloon[i] == '\r')
			aBalloon[i] = ' ';
	aText += "says " + aBalloon + "\n";
	aText += "marked";
	for (int aCell = 0; aCell < 81; aCell++)
	{
		if ((gGame->mTileFlags[aCell] & 0x40) == 0 || gGame->mLetters[aCell] == 0)
			continue;
		float aX, aY;
		gGameScreen->tilePos(aCell / 9, aCell % 9, &aX, &aY);
		snprintf(aLine, sizeof aLine, " %c %d %d %.4f %.4f", gGame->mLetters[aCell], aCell / 9, aCell % 9, aX, aY);
		aText += aLine;
	}
	aText += "\n";
	snprintf(aLine, sizeof aLine, "hint %d %d", gGameScreen->mHintShown, gGame->mHintFound ? gGame->mHintLength : 0);
	aText += aLine;
	for (int i = 0; gGame->mHintFound && i < gGame->mHintLength && i < 20; i++)
	{
		float aX, aY;
		gGameScreen->tilePos(gGame->mHintColumns[i], gGame->mHintRows[i], &aX, &aY);
		snprintf(aLine, sizeof aLine, " %c %.4f %.4f", gGame->mLetters[gGame->mHintColumns[i] * 9 + gGame->mHintRows[i]],
			aX, aY);
		aText += aLine;
	}
	aText += "\n";
	return aText;
}

// port (test runs): whether some word of the dictionary starts with thePrefix (upper case, as mWords keeps them)
static bool testPrefix(const std::string& thePrefix)
{
	int aLetter = thePrefix[0] - 'A';
	int aLow = gDictionary->mStarts[aLetter];
	int aHigh = aLetter < 25 ? gDictionary->mStarts[aLetter + 1] : gDictionary->mCount;
	while (aLow < aHigh)
	{
		int aMiddle = (aLow + aHigh) / 2;
		if (gDictionary->mWords[aMiddle].compare(thePrefix) < 0)
			aLow = aMiddle + 1;
		else
			aHigh = aMiddle;
	}
	return aLow < gDictionary->mCount && gDictionary->mWords[aLow].compare(0, thePrefix.size(), thePrefix) == 0;
}

// port (test runs): what a word over thePath's cells adds to the chapter's fill, in halves, as the score's tile marking
// changes the cells (0x20 filled; 0x10 half filled in a book that takes two words a cell; a cell with 8 turns back)
static int testFill(const std::vector<int>& thePath)
{
	int aFill = 0;
	for (size_t i = 0; i < thePath.size(); i++)
	{
		uint32_t aFlags = gGame->mTileFlags[thePath[i]];
		if ((aFlags & 8) != 0)
			aFill += (aFlags & 0x20) != 0 ? -2 : 2;
		else if ((aFlags & 0x20) == 0)
			aFill += gGame->currentBook()->mField38 == 0 || (aFlags & 0x10) != 0 ? 2 : 1;
	}
	return aFill;
}

// port (test runs): how far a word over thePath brings the stars ('*') down, which leave the board only from its bottom
// cell (Game.cpp): a row a tile taken from under a star, and 4 more for a star it takes out
static int testStars(const std::vector<int>& thePath)
{
	int aGain = 0;
	for (int aCell = 0; aCell < 81; aCell++)
	{
		if (gGame->mLetters[aCell] != '*')
			continue;
		int aColumn = aCell / 9, aRow = aCell % 9;
		int aBelow = 0;
		for (size_t i = 0; i < thePath.size(); i++)
			aBelow += thePath[i] / 9 == aColumn && thePath[i] % 9 > aRow;
		if (aBelow == 0)
			continue;
		int aBottom = aRow;
		while (aBottom < 8 && gGame->currentBook()->isCell(aColumn, aBottom + 1) != 0)
			aBottom++;
		aGain += aBelow + (aBottom - aRow <= aBelow ? 4 : 0);
	}
	return aGain;
}

// port (test runs): how good a word over thePath is: what it adds to the chapter's fill (testFill), the turns it costs
// or earns in the classic mode (Game::submit: a word under 5 letters costs one, a longer one earns its length less 4),
// weighing more as they run out, and the stars it brings down (testStars); then the longest
static int testValue(const std::vector<int>& thePath)
{
	if (thePath.size() < 3)
		return -100000;
	int aLength = (int)thePath.size();
	int aTurns = aLength < 5 ? -1 : aLength - 4;
	int aTurnWeight = gGame->mGoal <= 10 ? 40 : 12;
	return testFill(thePath) * 8 + aTurns * aTurnWeight + testStars(thePath) * 6 + aLength;
}

static bool testTile(int theCell)
{
	char aLetter = gGame->mLetters[theCell];
	return ((aLetter >= 'A' && aLetter <= 'Z') || aLetter == '?' || aLetter == '!') &&
		(gGame->mTileFlags[theCell] & 2) == 0;
}

static void testStep(int theCell, std::string& theWord, std::string& theSpelled, std::vector<int>& thePath,
	std::vector<int>& theBest, std::string& theBestWord, int& theVisits);

// theWord: the letters so far (a wild tile as the letter it stands for), theSpelled: as the dictionary spells them
// (Q is QU, as expandWord makes it)
static void testSearch(int theColumn, int theRow, std::string& theWord, std::string& theSpelled,
	std::vector<int>& thePath, std::vector<int>& theBest, std::string& theBestWord, int& theVisits)
{
	if (++theVisits > 300000)
		return;
	if (theWord.size() >= 3 && testValue(thePath) > testValue(theBest))
	{
		// the dictionary's answer for the letters, as the game checks a path (Game::updateWord); its match restored
		Str aMatch = gDictionary->mMatch;
		int aRarity = gDictionary->mMatchRarity;
		if (gDictionary->isWord(theWord, 1) != 0)
		{
			theBest = thePath;
			theBestWord = theWord;
		}
		gDictionary->mMatch = aMatch;
		gDictionary->mMatchRarity = aRarity;
	}
	if (theWord.size() >= 10)
		return;
	for (int aDirection = 0; aDirection < 6; aDirection++)
	{
		int aColumn, aRow;
		if (gGame->neighbour(theColumn, theRow, aDirection, &aColumn, &aRow) == 0)
			continue;
		int aCell = aColumn * 9 + aRow;
		if (!testTile(aCell) || std::find(thePath.begin(), thePath.end(), aCell) != thePath.end())
			continue;
		testStep(aCell, theWord, theSpelled, thePath, theBest, theBestWord, theVisits);
	}
}

// theCell added to the path, as each letter it can stand for
static void testStep(int theCell, std::string& theWord, std::string& theSpelled, std::vector<int>& thePath,
	std::vector<int>& theBest, std::string& theBestWord, int& theVisits)
{
	char aTile = gGame->mLetters[theCell];
	char aFirst = aTile == '?' || aTile == '!' ? 'A' : aTile;
	char aLast = aTile == '?' || aTile == '!' ? 'Z' : aTile;
	for (char aLetter = aFirst; aLetter <= aLast; aLetter++)
	{
		theWord.push_back(aLetter);
		theSpelled += aLetter == 'Q' ? "QU" : std::string(1, aLetter);
		thePath.push_back(theCell);
		if (testPrefix(theSpelled))
			testSearch(theCell / 9, theCell % 9, theWord, theSpelled, thePath, theBest, theBestWord, theVisits);
		thePath.pop_back();
		theSpelled.resize(theSpelled.size() - (aLetter == 'Q' ? 2 : 1));
		theWord.pop_back();
	}
}

// port (test runs): the best word (up to 10 tiles, by testValue) on the board's letter and wild tiles that are not
// fixed (as Game::findHint takes them), by the game's dictionary, as "word <letters> x y..." (Start.h). Read only,
// and it draws no random number.
std::string testWord()
{
	if (gGameScreen == nullptr || gGame == nullptr || gDictionary == nullptr || gDictionary->mCount == 0)
		return "word\n";
	std::vector<int> aBest;
	std::string aBestWord;
	int aVisits = 0;
	for (int aCell = 0; aCell < 81; aCell++)
	{
		if (!testTile(aCell) || gGame->currentBook()->isCell(aCell / 9, aCell % 9) == 0)
			continue;
		std::string aWord, aSpelled;
		std::vector<int> aPath;
		testStep(aCell, aWord, aSpelled, aPath, aBest, aBestWord, aVisits);
	}
	std::string aText = "word " + aBestWord;
	for (size_t i = 0; i < aBest.size(); i++)
	{
		float aX, aY;
		char aLine[64];
		gGameScreen->tilePos(aBest[i] / 9, aBest[i] % 9, &aX, &aY);
		snprintf(aLine, sizeof aLine, " %.4f %.4f", aX, aY);
		aText += aLine;
	}
	return aText + "\n";
}

} // namespace bonnies
