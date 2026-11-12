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
#include <cstdlib>
#include <cstring>
#include <string>
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
static uint32_t gHandsFrameTime;  // 0x47853c
static int gHands;                // 0x478540
static uint32_t gEyeTime;         // 0x478544
static int gEyeFrame;             // 0x478548: her eyes' two frames, swapped while they move
static uint32_t gEyeFrameTime;    // 0x47854c
static uint32_t gLastMove;        // 0x478550: when any of them last moved
static uint32_t gScreenStart;     // 0x47850c
// 0x478538: the hands' time, set with the others and never read (the mouth's and the head's are used as these)
static uint32_t gHandsTime;

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
	mSequenceState = 3;
	mSummaryStart = 0;
	mStepStart = ticks();
	mStepEnd = ticks() + 300;
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
	mCountedPoints = 0;
	mCountTime = 0;
	mLastUpdate = ticks();
	mCursorColumn = -1;
	mCursorRow = -1;
	mMouseX = -1.0f;
	mMouseY = -1.0f;
	mMouseDown = 0;
	mStarAtBottom = 0;
	mSequenceState = 1;
	mStepStart = ticks();
	mStepEnd = 0;
	mLastColumn = -1;
	mLastRow = -1;
	mHintTime = 0;
	mHintShown = 0;
	mHintAgain = 0;
	mHintClock = 0;
	mBlinkStart = 0;
	mBlinking = 0;
	mSummaryStart = 0;
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
	if (mLastPicture != nullptr)
		gResources->release(mLastPicture);
	mLastPicture = nullptr;
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
		} while (aRow < BOARD_ROWS);
		aColumn++;
	} while (aColumn < BOARD_COLUMNS);
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
		} while (aRow < BOARD_ROWS);
		aColumn++;
	} while (aColumn < BOARD_COLUMNS);
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
	if (theAll != 0 && frameCount() <= gGraphics->mFloatTextFrame + 3)
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
	if (LEVEL_COUNT < gGame->mLevel)
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
	if (mSequenceState == 0xd || mSequenceState == 9)
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
		gHandsTime = ticks() - aRandom;
		gHands = 0;
		gLastMove = 0;
	}
	else if (mSequenceState == 0xe)
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
	else if (mSequenceState == 0xf)
	{
		aFade = mLeaveFade;
		mTyped = 9999;
	}
typed:
	{
		int aShown = 0;
		char aTypingChar = 0;
		mSpaces = 0;
		int i = 0;
		if (0 < (int)aSource.size())
		{
			do
			{
				char aSourceChar;
				if (i < (int)aSource.size())
				{
					aSourceChar = aSource[i];
					if (aSourceChar == ' ')
					{
						mSpaces++;
						goto append;
					}
					if (aSourceChar <= ' ')
						goto append;
				}
				else
					aSourceChar = 'A';
				if (mTyped <= aShown)
				{
					aTypingChar = aSourceChar;
					break;
				}
				aShown++;
			append:
				aText += aSourceChar;
				i++;
			} while (i < (int)aSource.size());
		}
		gBonnies->mFonts[0]->drawText(aText, 0.08f, 0.08f, 0.56375f, 0.5f, 0xffefefef, 0, 0x12, 1.0f, 0.0f);
		if (i < (int)aSource.size() || (int)ticks() >= mTypeTime + 500)
		{
			if (0 < aTypingChar)
				aText += aTypingChar;
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
		gHandsTime = gMouthTime;
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
	if (150 < (int)(ticks() - gHandsFrameTime) && aTyped)
	{
		gHandsFrameTime = ticks();
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
		} while (aRow < BOARD_ROWS);
		aColumn++;
		aCell += BOARD_ROWS;
	} while (aCell < BOARD_CELLS);
	return 0;
}

// 0x4103a0
void GameScreen::showHint(Str theText, int theStep, float theX, float theY, int theSide, int theFlag)
{
	gGame->mMovesAtHint = gGame->mShuffles + gGame->mWordsPlayed;
	mHintText = theText;
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
		gBonnies->balloon(mHintText, mHintX, mHintY, mHintSide, theFlag, aRect, 0);
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
	return !mHintText.empty();
}

// 0x410660
void GameScreen::updateHint()
{
	mShuffleEndsGame = 0;
	Widget* aButton = findWidget(0xcb);
	if (aButton != nullptr && gGame != nullptr)
	{
		if (gGame->mClassic == 0 || 1 < gGame->mGoal || gGame->mClassic == 0)
		{
			aButton->mText = getText(0x542);
			return;
		}
		mShuffleEndsGame = 1;
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
	mHintAgain = 0 < mHintDue;
	mHintTime = ticks();
	mHintShown = 1;
done:
	mHintDue = 0;
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
	if (mSequenceState != 1 || gGame->mGameOverTime != 0)
		aResult = 0;
	return aResult;
}

// 0x410c00
void GameScreen::init()
{
	gGameScreen = this;
	Screen::init();
	gApp->mStatusFlags = 0x10c;
	mHintsChecked = 0;
	mSpacesTyped = 0;
	mSpaces = 0;
	mLinesTyped = 0;
	mLines = 0;
	mTyped = 0;
	mCountStart = 0;
	mFlashTime = 0;
	mFlashLength = 0;
	mFlashColor = 0;
	mBookOpen = 1.0f;
	mSummaryDark = 0;
	mSummaryFade = 1.0f;
	mBoardFade = 1.0f;
	mLeaveFade = 1.0f;
	mChapter = "";
	mPicture = nullptr;
	mLastPicture = nullptr;
	mMessage = "";
	mHintText = "";
	mTutorialStep = -1;
	mHintX = 0.0f;
	mHintY = 0.0f;
	mHintSide = 0;
	mShuffleEndsGame = 0;
	mHintDue = 0;
	mMessageEnd = 0;
	int i = 0;
	do
	{
		Button* aButton = new Button();
		aButton->init();
		addWidget(aButton);
		if (gGame->mClassic == 0)
			aButton->mX = (float)((i * 0x86 + 0x20) * (double)0.00125f + 0.515f);
		else
			aButton->mX = (float)((i * 0xf4 + 0x20) * (double)0.00125f + 0.44625f);
		aButton->mY = 0.90666664f;
		if (gGame->mClassic == 0)
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

// 0x41f4b0, the getter of the last active time, is read inline as gLastActiveTime here and in the game's other draws
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
		if ((int)(ticks() - gGame->mScoreTime) <= 0xf0)
		{
			int aElapsed = ticks() - gScoreTime;
			int aLeft = (gGame->mScoreTime - ticks()) + 0xf0 + aElapsed;
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
	if (gGame->mClassic != 0)
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
		} while (aRow < BOARD_ROWS);
		aColumn++;
	} while (aColumn < BOARD_COLUMNS);
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
				if (gBonnies->inModal() == 0 && mStarAtBottom == 0 && gGame->mGameOverTime == 0 && gBonnies->mPaused == 0 &&
					mSequenceState == 1)
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
					if (mCursorColumn == aColumn && mCursorRow == aRow)
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
				else if (gGame->mOldLetters[i] == '^')
				{
					int aTime = ticks() - gGame->mChangeTimes[i];
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
							gGame->mChangeTimes[i] = 0;
							gGame->mOldLetters[i] = 0;
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
		} while (aRow < BOARD_ROWS);
		aColumn++;
		aCell += BOARD_ROWS;
	} while (aCell + 0x6d < 0xbe);
	// The tiles: the letter that was there fades out and the new one in
	aColumn = 0;
	do
	{
		int aRow = 0;
		do
		{
			int aBase = aColumn * BOARD_ROWS;
			if (gGame->currentBook()->isCell(aColumn, aRow) != 0 && gGame->mLetters[aBase + aRow] != 0)
			{
				float aX;
				double aYD;
				cellCorner(aColumn, aRow, &aX, &aYD);
				float aY = (float)aYD;
				uint32_t aColor = 0xff48252c;
				float aAngle = 0.0f;
				if (gBonnies->inModal() == 0 && mStarAtBottom == 0 && gGame->mGameOverTime == 0 && gBonnies->mPaused == 0 &&
					mSequenceState == 1)
				{
					if (onPath(aColumn, aRow))
					{
						if (gGame->isWord() != 0)
							aColor = addColor(0xff48252c, 0, 10, 0x78, 10);
						else
							aColor = addColor(0xff48252c, 0, 0x8c, 10, 10);
					}
					if (mCursorColumn == aColumn && mCursorRow == aRow)
					{
						if (gGame->mPathLength < 1)
							aColor = addColor(aColor, 0, 0x50, 0x32, 0x32);
						else
							aColor = addColor(aColor, 0, 0x1e, 0x1e, 0x1e);
					}
				}
				int i = aBase + aRow;
				int aTime = ticks() - gGame->mChangeTimes[i];
				float aProgress = (float)(aTime * (double)0.0016666667f);
				if (aProgress < 0.0f)
					aProgress = 0.0f;
				if (gGame->mChangeTimes[i] == 0 || gGame->mOldLetters[i] == 0)
					aProgress = 1.0f;
				else if (!(aProgress <= 1.0f) && gGame->mOldLetters[i] != '^')
				{
					gGame->mChangeTimes[i] = 0;
					gGame->mOldLetters[i] = 0;
				}
				int aPass = 0;
				do
				{
					char aLetter;
					if (aPass == 0)
						aLetter = gGame->mOldLetters[i];
					else
						aLetter = gGame->mLetters[i];
					uint32_t aTextColor = aColor;
					uint32_t aShadow = 0x6fffffff;
					float aPhase;
					if (aPass == 0)
						aPhase = (float)(1.0f - (double)aProgress);
					else
						aPhase = aProgress;
					if (gGame->mOldLetters[i] == '^')
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
						if ((gGame->mClassic != 0 || (gBonnies->isPaused() == 0 && isInactive() == 0) ||
								gGame->mGameOver != 0) &&
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
							if (gGame->mCopyTime != 0 && (gGame->mKinds[i] & 2) != 0)
							{
								aStart = gGame->mCopyTime;
								aSwapping = 1;
							shuffle:
								float aStep = (float)((int)(ticks() - aStart) * (double)0.0016666667f);
								if (!(aStep < 1.0f))
								{
									gGame->mShuffleTime = 0;
									gGame->mCopyTime = 0;
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
									aLetterText += gGame->mOldLetters[i];
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
		} while (aRow < BOARD_ROWS);
		aColumn++;
	} while (aColumn < BOARD_COLUMNS);
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
	if (mBlinking != 0 && 0x4b0 < (int)(ticks() - mBlinkStart))
		mBlinking = 0;
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
				else if (mBlinking != 0 && (gGame->mTileFlags[i] & 0x20) == 0)
				{
					int aTime = ticks() - mBlinkStart;
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
		} while (aRow < BOARD_ROWS);
		aColumn++;
		aCell += BOARD_ROWS;
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
			char c = gGame->mLetters[gGame->pathColumn(i) * BOARD_ROWS + gGame->pathRow(i)];
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
	if (gGame->mClassic != 0)
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
		gBonnies->mShimmerWaits = 1;
	if (gGame->mClassic != 0)
	{
		aLine = "";
		int aMoves = gGame->mGoal + gGame->mMovesLag;
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
		float aClock = (float)(gGame->mClock + (double)gGame->mClockLag);
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

// 0x410790: the next step of the screen's sequence (mSequenceState) when its time (mStepEnd) is up
void GameScreen::advance()
{
	mStepStart = ticks();
	mStepEnd = 0;
	switch (mSequenceState)
	{
	case 2:
		if (gBonnies->mLoadImages[0] == 0)
		{
			gBonnies->mLoadImages[7] = 1;
			mStepEnd = ticks() + 10;
			return;
		}
		gBonnies->mLoadImages[7] = 0;
		refresh();
		return;
	case 3:
		mSequenceState = 4;
		mBookOpen = 0.0f;
		mSummaryDark = 0.0f;
		gSummaryName = gGame->mLevelBestWord;
		gSummaryLast = gGame->currentBook()->mLast;
		gSummaryBook = gGame->currentBook()->mBook;
		gSummaryLevel = gGame->mLevel - 1;
		mStepEnd = ticks() + 0x96;
		playSound(Str(lit(0x4582fc)), 0.8f, 1, 10);
		return;
	case 4:
		mSequenceState = 5;
		mCountTime = 0;
		mStepEnd = ticks() + 0xfa;
		return;
	case 5:
		mSequenceState = 6;
		mCountTime = 0;
		mCountStep = 0;
		mStepEnd = 0;
		if (mSummaryStart == 0)
			mSummaryStart = ticks();
		return;
	case 6:
		gGame->currentBook();
		mSequenceState = 8;
		mStepEnd = ticks() + 1;
		return;
	case 7:
		mSequenceState = 8;
		mStepEnd = ticks() + 100;
		return;
	case 8:
		if (mLastPicture != nullptr)
			gResources->release(mLastPicture);
		mLastPicture = mPicture;
		mPicture = nullptr;
		mChapter = "";
		if (gGame->currentBook()->mLast != 0)
		{
			mSequenceState = 9;
			showBookEnd();
			mLeaveFade = 1.0f;
			mStepEnd = ticks();
			mSequenceState = 0xd;
			return;
		}
		gGame->nextLevel();
		reset();
		mLeaveFade = 0.0f;
		mStepEnd = ticks() + 0xfa;
		mSequenceState = 0xd;
		return;
	case 12:
	case 13:
		mTyped = 0;
		mSpacesTyped = 0;
		mSpaces = 0;
		mLinesTyped = 0;
		mLines = 0;
		mTypeTime = ticks() + 400;
		mSequenceState = 0xe;
		mStepEnd = ticks() + 0xfa;
		return;
	case 14:
		if (mTyped == 9999)
		{
			mSequenceState = 0xf;
			mLeaveFade = 0.0f;
			mStepEnd = ticks() + 400;
			Str aName = "";
			aName += lit(0x4582e8);
			appendInt(aName, (gGame->mLevel & 3) + 1);
			playSound(aName);
			return;
		}
		mStepEnd = ticks() + 5;
		return;
	case 15:
		mSequenceState = 0x10;
		mStepEnd = ticks() + 200;
		return;
	case 16:
		mSequenceState = 1;
		mStepEnd = 0;
		findWidget(0xc9)->mFlags &= ~WIDGET_INACTIVE;
		findWidget(0xca)->mFlags &= ~WIDGET_INACTIVE;
		findWidget(0xcb)->mFlags &= ~WIDGET_INACTIVE;
		return;
	}
}

// 0x413020
void GameScreen::draw()
{
	if (mSequenceState == 9 && gGame->currentBook()->mLast == 0)
		drawIntro();
	else if (mSequenceState == 0xd)
	{
		drawIntro();
		if (mLastPicture != nullptr)
			drawFrame(mLastPicture->mFrames, 0.0f, 0.0f, (float)(1.0f - (double)mLeaveFade), 1);
	}
	else if (mSequenceState == 0xe || mSequenceState == 0xf)
		drawIntro();
	else if (mSequenceState == 9 && mLastPicture != nullptr)
		drawFrame(mLastPicture->mFrames, 0.0f, 0.0f, 1.0f, 1);
	else if (mSequenceState == 10 && mLastPicture != nullptr)
		drawFrame(mLastPicture->mFrames, 0.0f, 0.0f, 1.0f, 1);
	else if (mPicture != nullptr)
		drawFrame(mPicture->mFrames, 0.0f, 0.0f, 1.0f, 1);
	if (!(mBoardFade < 0.001f))
	{
		drawBoard();
		Screen::draw();
		if (!mHintText.empty())
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
		if (mBoardFade < 1.0f && mPicture != nullptr)
			drawFrame(mPicture->mFrames, 0.0f, 0.0f, (float)(1.0f - (double)mBoardFade), 1);
	}
	host()->trace(lit(0x45844c));
	if (!mHintText.empty())
		gBonnies->balloon(mHintText, mHintX, mHintY, mHintSide, gOverlayScreen->findWidget(0xcc) != nullptr, nullptr, 1);
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
	int aState = mSequenceState;
	if (aState == 4 || aState == 5 || aState == 6 || aState == 7 || aState == 0xd || aState == 10 || aState == 8 ||
		(aState == 9 && gGame->currentBook()->mLast != 0))
	{
		// The chapter's summary over the board
		int aFinal = 0;
		aState = mSequenceState;
		if (aState == 0xd || aState == 10 || aState == 8 || (aState == 9 && gGame->currentBook()->mLast != 0))
			aFinal = 1;
		float aShow = mSummaryFade;
		aState = mSequenceState;
		if (aState == 0xf || aState == 0xd || aState == 10)
			aShow = (float)(1.0f - (double)mLeaveFade);
		float aHide = (float)(1.0f - (double)aShow);
		int aFade = ftol(aHide * (double)-255.0f);
		uint32_t aColor = addColor(0xffffffff, aFade, 0, 0, 0);
		uint32_t aShadow = addColor(0x8f000000, ftol(aHide * (double)-191.0f), 0, 0, 0);
		if (0.0f < aShow)
		{
			double aDark = aShow * (double)1.6666666f;
			if (!(aDark <= mSummaryDark))
				aDark = mSummaryDark;
			if (!(aDark <= 1.0f))
				aDark = 1.0f;
			uint32_t aBack = (uint32_t)ftol(aDark * 114.75f) << 24;
			if (aBack != 0)
				fillRect(0.0f, 0.0f, 1.0f, 1.0f, aBack);
			if (0.01f < mBookOpen)
			{
				int aImage = (gSummaryLast != 0 ? 0x16 : 0) + 0xd;
				if (mBookOpen < 0.99f)
					drawFrameRotated(gBonnies->mImages[aImage]->mFrames, 0.5f, 0.466666669f, 0.0f, mBookOpen, aShow, 0, 0);
				else
					drawFrame(gBonnies->mImages[aImage]->mFrames, 0.11f, 0.163333327f, aShow, 0);
			}
			Font* aFont = gBonnies->mFonts[10];
			if (mSequenceState != 5 && mSequenceState != 4)
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
						if (gGame->mClassic != 0)
							aTime = gGame->mGoal;
						else
							aTime = gGame->mClockBonus;
						if (aFinal != 0)
							aTime = 0;
						if (gGame->mClassic != 0)
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
						aValue = format(Str(getText(0x535)), intToStr(gSummaryLevel + 1), intToStr(LEVEL_COUNT),
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
	if (!mHintText.empty())
	{
		findWidget(0xc9)->mFlags &= ~WIDGET_INACTIVE;
		findWidget(0xcb)->mFlags &= ~WIDGET_INACTIVE;
		findWidget(0xca)->mFlags &= ~WIDGET_INACTIVE;
		mTutorialStep = -1;
		mHintText = "";
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
	int aDelta = (int)(ticks() - mLastUpdate);
	mLastUpdate = ticks();
	if (500 < aDelta)
		aDelta = 500;
	mBookOpen = 1.0f;
	mSummaryFade = 1.0f;
	mLeaveFade = 1.0f;
	if (mSequenceState == 0x13 && gBonnies->inModal() == 0)
	{
		mSequenceState = 1;
		if (gBonnies->mLastResult == 1 && gGame != nullptr)
		{
			gGame->mClock = 0.0f;
			gGame->mGoal = 0;
			gGame->mClockLag = 0.0f;
			gGame->mMovesLag = 0;
			gGame->mNoMoves = 1;
			gGame->mGameOverTime = ticks() - 1;
		}
	}
	if (mSequenceState == 0x12 && gBonnies->inModal() == 0)
	{
		showInterstitial(0, 1);
		gGameScreen->mSequenceState = 0x11;
	}
	int aState = mSequenceState;
	if (aState == 0x11)
	{
		if (gBonnies->inModal() == 0)
		{
			if (gGame->mLevel == LEVEL_COUNT + 1 || gGame->mGameOver != 0)
			{
				gGame->mGameOver = 1;
				gBonnies->setState(2, 1, 0);
				return;
			}
			mBoardFade = 0.0f;
			mTyped = 0;
			mSpacesTyped = 0;
			mSpaces = 0;
			mLinesTyped = 0;
			mLines = 0;
			mTypeTime = ticks() + 400;
			mSequenceState = 0xe;
			mStepEnd = ticks();
		}
	}
	else
	{
		float aLevel;
		bool aFull = false;
		if (aState == 3)
		{
			int aStart = mStepStart;
			aLevel = (float)(1.0 - (double)(float)(int)(ticks() - aStart) / (double)(float)(mStepEnd - aStart));
			mBoardFade = aLevel;
			if (aLevel <= 1.0f)
			{
				if (aLevel < 0.0f)
					mBoardFade = 0.0f;
			}
			else
				aFull = true;
		}
		else if (aState == 4)
		{
			mBookOpen = 0.0f;
			aLevel = (float)((double)(float)(int)(ticks() - mStepStart) / (double)(float)(mStepEnd - mStepStart));
			mSummaryDark = aLevel;
			if (aLevel < 1.0f)
			{
				if (aLevel < 0.0f)
					mSummaryDark = 0.0f;
			}
			else
				mSummaryDark = 1.0f;
		}
		else if (aState == 5)
		{
			aLevel = (float)((double)(float)(int)(ticks() - mStepStart) / (double)(float)(mStepEnd - mStepStart));
			mBookOpen = aLevel;
			if (aLevel < 1.0f)
			{
				if (aLevel < 0.0f)
					mBookOpen = 0.0f;
			}
			else
				mBookOpen = 1.0f;
		}
		else if (aState == 7)
		{
			int aStart = mStepStart;
			aLevel = (float)(1.0 - (double)(float)(int)(ticks() - aStart) / (double)(float)(mStepEnd - aStart));
			mSummaryFade = aLevel;
			if (aLevel <= 1.0f)
			{
				// The exe tests mBoardFade here
				if (mBoardFade < 0.0f)
					mSummaryFade = 0.0f;
			}
			else
				mSummaryFade = 1.0f;
		}
		else if (aState == 0xd || aState == 0xf || aState == 0xc || aState == 10)
		{
			aLevel = (float)((double)(float)(int)(ticks() - mStepStart) / (double)(float)(mStepEnd - mStepStart));
			mLeaveFade = aLevel;
			if (aLevel <= 1.0f)
			{
				if (aLevel < 0.0f)
					mLeaveFade = 0.0f;
			}
			else
				mLeaveFade = 1.0f;
		}
		else if (aState == 0x10)
		{
			// The same as state 3's (the exe jumps into it)
			aLevel = (float)((double)(float)(int)(ticks() - mStepStart) / (double)(float)(mStepEnd - mStepStart));
			mBoardFade = aLevel;
			if (aLevel <= 1.0f)
			{
				if (aLevel < 0.0f)
					mBoardFade = 0.0f;
			}
			else
				aFull = true;
		}
		else if (aState == 1)
			aFull = true;
		else if (aState == 6)
		{
			// The bonus for the time or the moves left, counted into the score
			int aTime = mCountTime + aDelta;
			mCountTime = aTime;
			aTime = aTime & -(int)(0 <= aTime);
			mCountTime = aTime;
			if (0x9c4 < aTime)
				aTime = 0x9c4;
			int aStep = mCountStep;
			mCountTime = aTime;
			if (aStep == 0)
			{
				mCountedPoints = 0;
				if (999 < aTime)
				{
					mCountTime = 0;
					mCountStep = 1;
					mCountStart = ticks();
				}
			}
			else if (aStep == 1)
			{
				int aSince = (int)(ticks() - mCountStart);
				if (aSince < 0)
					aSince = 0;
				else if (0x9c4 < aSince)
					aSince = 0x9c4;
				int aCount = gGame->mClassic != 0 ? 3 : 4;
				int anInterval = ftol((double)aCount * (((1.0f - aSince * (double)0.0004f) * 0.55f) + 0.45f) * 25.0f);
				bool aCounted = false;
				if (anInterval < mCountTime)
				{
					do
					{
						mCountTime -= anInterval;
						if (gGame->mClassic == 0)
						{
							int aLeft = gGame->mClockBonus;
							if (aLeft < 1)
							{
								mCountTime = 0;
								mCountStep = 2;
							}
							else
							{
								int aTake = aLeft % 10 + 10;
								if (aTake <= aLeft)
									aLeft = aTake;
								gGame->mClockBonus -= aLeft;
								gGame->mScore += (unsigned char)gGame->difficultyFactor() * (gGame->mLevel + 1) * aLeft * 5;
								aCounted = true;
							}
						}
						else if (gGame->mGoal < 1)
						{
							mCountTime = 0;
							mCountStep = 2;
						}
						else
						{
							gGame->mGoal--;
							gGame->mScore += (unsigned char)gGame->difficultyFactor() * (gGame->mLevel + 1) * 0x14;
							aCounted = true;
						}
					} while (anInterval < mCountTime);
					if (aCounted)
					{
						int aRise = 0x50;
						if (mCountedPoints < 0x51)
							aRise = mCountedPoints;
						double aPitch = (double)aRise * 0.035f + 1.0f;
						gSound->play(Str(lit(0x45845c)), 0.4f, 1, 0, (float)(aPitch + aPitch));
						mCountedPoints++;
					}
				}
				gGame->mScoreTime = gGame->mScore;
			}
			else if (aStep == 2 && 500 < aTime)
			{
				mCountTime = 0;
				mCountStep = 3;
			}
		}
		if (aFull)
			mBoardFade = 1.0f;
	}
	gGame->update();
	if (mStepEnd != 0 && mStepEnd <= (int)ticks())
	{
		advance();
		return;
	}
	if (gLeaveHint != 0)
	{
		gLeaveHint = 0;
		leave(1);
	}
	if (mSequenceState == 1)
	{
		if (mBlinking != 0)
			mHintClock = ticks();
		int aHintAfter = (-(int)(gGame->mDifficulty != 0) & 2000) + 18000;
		int anIdleAfter = (-(int)(gGame->mDifficulty != 0) & 4000) + 13000;
		if (gGame->mRecord.mBestLevel < 1)
		{
			// New players get their hints sooner
			float aFactor;
			if (gGame->mWordsPlayed < 3)
				aFactor = 5.0f;
			else if (gGame->mWordsPlayed < 5)
				aFactor = 0.5f;
			else
				aFactor = 0.75f;
			aHintAfter = ftol((double)aHintAfter * aFactor);
			anIdleAfter = ftol((double)anIdleAfter * aFactor);
		}
		if (!mHintText.empty() && mHintSide != 6 && mHintShown == 0 && mBlinking == 0)
		{
			mHintClock = ticks();
			mHintTime = ticks();
			Screen::update();
			return;
		}
		if (gBonnies->inModal() == 0 && mStarAtBottom == 0 && gGame->mGameOverTime == 0 && gBonnies->mPaused == 0)
		{
			if (mHintShown == 0)
			{
				if (mBlinking == 0 && ((0 < mHintDue && mHintDue < (int)ticks()) ||
					(aHintAfter < (int)(ticks() - gGame->mLastMoveTime) && anIdleAfter < (int)(ticks() - mHintTime))))
				{
					showHintWord();
					mHintClock += 3000;
					Screen::update();
					return;
				}
				if (mHintShown == 0 && mBlinking == 0 && 2000 < (int)(ticks() - mHintClock))
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
					if (aWait <= (int)(ticks() - mHintClock))
					{
						if ((double)aMarked / (double)aTotal < 0.85f)
						{
							mHintClock = ticks() + 2000;
							Screen::update();
							return;
						}
						mBlinking = 1;
						mHintClock = ticks();
						mBlinkStart = mHintClock;
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
			mHintClock = ticks();
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
	int aLastRow = mCursorRow;
	int aLastColumn = mCursorColumn;
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
		int aState = mSequenceState;
		if (aState == 0xb)
		{
			mStepEnd = ticks() - 1;
			return 0;
		}
		if (aState == 6)
		{
			mStepEnd = ticks() - 1;
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
		if (!mHintText.empty() && (mHintSide == 5 || mHintSide == 6) && gOverlayScreen->findWidget(0xcc) == nullptr)
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
		if (mCursorColumn != -1 || !(distance(theEvent->mX, theEvent->mY, mMouseX, mMouseY) < 0.00375f))
		{
			if (gBonnies->inModal() == 0 && mSequenceState == 1 && gBonnies->mPaused == 0 && mStarAtBottom == 0 &&
				gGame->mGameOverTime == 0)
			{
				int aSteps = ftol(distance(theEvent->mX, theEvent->mY, mMouseX, mMouseY) * 156.52174f);
				if (aSteps < 2)
					aSteps = 1;
				if (mMouseDown == 0)
					aSteps = 1;
				int i = 0;
				if (0 < aSteps)
				{
					float aCount = (float)aSteps;
					do
					{
						i++;
						double aPart = (double)(float)i / aCount;
						float aX = (float)(aPart * theEvent->mX + (1.0f - aPart) * mMouseX);
						float aY = (float)(aPart * theEvent->mY + (1.0f - aPart) * mMouseY);
						int aColumn, aRow;
						float aDistance;
						if (hitTile(aX, aY, &aColumn, &aRow, &aDistance, 1) == 0)
						{
							mCursorColumn = -1;
							mCursorRow = -1;
						}
						else if (aDistance <= 0.0299f || mMouseDown == 0)
						{
							mCursorColumn = aColumn;
							mCursorRow = aRow;
						}
						else
							continue;
						if (mMouseDown == 0)
							break;
						if (mCursorColumn != -1)
						{
							gGame->pick(mCursorColumn, mCursorRow, aLastColumn, aLastRow, 1);
							aLastColumn = mCursorColumn;
							aLastRow = mCursorRow;
						}
					} while (i < aSteps);
				}
			}
			mMouseX = theEvent->mX;
			mMouseY = theEvent->mY;
			if (mMouseDown != 0)
				return 0;
		}
		break;
	case 2:
	{
		if (Screen::handleEvent(theEvent) == 0)
			return 0;
		mMouseX = theEvent->mX;
		mMouseY = theEvent->mY;
		// A click on the path's last tile again is silent
		bool aMuted = false;
		if (0 < gGame->mPathLength && mLastColumn == mCursorColumn && mLastRow == mCursorRow &&
			gGame->pathColumn(gGame->mPathLength - 1) == mCursorColumn && gGame->pathRow(gGame->mPathLength - 1) == mCursorRow)
		{
			aMuted = true;
			gSound->mPaused++;
		}
		if (gBonnies->inModal() == 0 && mSequenceState == 1 && gBonnies->mPaused == 0 && mStarAtBottom == 0 &&
			gGame->mGameOverTime == 0)
		{
			bool aPick = true;
			if (mCursorColumn == -1)
			{
				int aColumn, aRow;
				float aDistance;
				if (hitTile(theEvent->mX, theEvent->mY, &aColumn, &aRow, &aDistance, 0) != 0)
				{
					if (0 < gBonnies->mSpecialMode)
					{
						mCursorColumn = aColumn;
						mCursorRow = aRow;
					}
					else if ((gGame->mTileFlags[aColumn * BOARD_ROWS + aRow] & 2) != 0)
					{
						// A fixed tile: the hint twice, then the error sound
						aPick = false;
						int aSeen = gGame->mRecord.mFixedTileTaps;
						gGame->mRecord.mFixedTileTaps = aSeen + 1;
						if (aSeen < 2)
							gGameScreen->showHint(Str(getText(0x53c)), 999, 0.5f, 0.4f, 5, 1);
						else
							playSound(Str(lit(0x457620)), 0.65f);
					}
					else if (gGame->mLetters[aColumn * BOARD_ROWS + aRow] == '*')
					{
						aPick = false;
						if (!mHintText.empty() && mTutorialStep == 10)
							playSound(Str(lit(0x457620)), 0.65f);
						else
						{
							int aSeen = gGame->mRecord.mStarTileTaps;
							gGame->mRecord.mStarTileTaps = aSeen + 1;
							if (aSeen < 2)
								gGameScreen->showHint(Str(getText(0x53d)), 999, 0.5f, 0.4f, 5, 1);
							else
								playSound(Str(lit(0x457620)), 0.65f);
						}
					}
				}
			}
			if (aPick)
				gGame->pick(mCursorColumn, mCursorRow, aLastColumn, aLastRow, 0);
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
		if ((mMouseDown == 0 || mCursorColumn != -1) &&
			((mMouseDown != 0 && gGame->isWord() != 0 && 1 < mClicks) ||
				(doubleClickDistance() <= aSince && aSince <= doubleClickTime() && mMouseDown != 0 && mClicks == 1 &&
					1 < gGame->mPathLength && mLastColumn == mCursorColumn && mLastRow == mCursorRow &&
					gGame->pathColumn(gGame->mPathLength - 1) == mCursorColumn &&
					gGame->pathRow(gGame->mPathLength - 1) == mCursorRow)))
			gGame->submit();
		mLastColumn = mCursorColumn;
		mLastRow = mCursorRow;
		mMouseDown = 0;
		break;
	}
	case 6:
	case 7:
		// The right button drops the path
		if (gOverlayScreen != nullptr && gOverlayScreen->findWidget(0xcc) != nullptr)
			gLeaveHint = 1;
		mMouseDown = 0;
		if (gBonnies->inModal() == 0 && mStarAtBottom == 0 && gGame->mGameOverTime == 0 && mSequenceState == 1 &&
			gBonnies->mPaused == 0 && (mHintText.empty() || (mTutorialStep != 1 && mTutorialStep != 3)))
		{
			if (0 < gGame->mPathLength)
			{
				gGame->mPathClearTime = ticks();
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
				if (gBonnies->mPaused != 0 || (mSequenceState == 1 && gGame->mGameOverTime == 0))
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
			if (gBonnies->inModal() == 0 && isPlaying() != 0 && mHintText.empty())
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
			if (!gGameScreen->mHintText.empty() && gGameScreen->mTutorialStep == 0xe)
				leave(1);
			gGame->submit();
			return 0;
		case 0xcb:
		{
			if (mHintsChecked == 0)
				return 0;
			if (findWidget(0xcb) != nullptr && (findWidget(0xcb)->mFlags & WIDGET_INACTIVE) != 0)
				return 0;
			if (mShuffleEndsGame != 0)
			{
				// No moves left to shuffle: the game is over
				messageDialog(0, Str(getText(0x53e)), Str(getText(0x53f)), Str(getText(0x540)), Str(""), Str(""), 0, 0,
					0, 0, 0);
				mSequenceState = 0x13;
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
				if (gGame->mClassic == 0)
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
		gGame->mInProgress, gGameScreen->mSequenceState, gGameScreen->isBusy(), gGame->mGameOverTime);
	aText += aLine;
	snprintf(aLine, sizeof aLine, "balloon %d %d %d\n", gGameScreen->mHintText.empty() ? 0 : 1, gGameScreen->mHintSide,
		gGameScreen->mTutorialStep);
	aText += aLine;
	// the tutorial's balloon text, on one line, and the tiles it marks for its word
	std::string aBalloon = gGameScreen->mHintText;
	for (size_t i = 0; i < aBalloon.size(); i++)
		if (aBalloon[i] == '\n' || aBalloon[i] == '\r')
			aBalloon[i] = ' ';
	aText += "says " + aBalloon + "\n";
	aText += "marked";
	for (int aCell = 0; aCell < BOARD_CELLS; aCell++)
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
		snprintf(aLine, sizeof aLine, " %c %.4f %.4f", gGame->mLetters[gGame->mHintColumns[i] * BOARD_ROWS + gGame->mHintRows[i]],
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
			aFill += gGame->currentBook()->mTwoFills == 0 || (aFlags & 0x10) != 0 ? 2 : 1;
	}
	return aFill;
}

// port (test runs): how far a word over thePath brings the stars ('*') down, which leave the board only from its bottom
// cell (Game.cpp): a row a tile taken from under a star, and 4 more for a star it takes out
static int testStars(const std::vector<int>& thePath)
{
	int aGain = 0;
	for (int aCell = 0; aCell < BOARD_CELLS; aCell++)
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
// or earns in the classic mode (Game::submit: a word under 6 letters costs one, a longer one earns its length less 5),
// weighing more as they run out, and the stars it brings down (testStars); then the longest
static int testValue(const std::vector<int>& thePath)
{
	if (thePath.size() < 3)
		return -100000;
	int aLength = (int)thePath.size();
	int aShortest = (gGame->mClassic != 0) + 5;
	int aTurns = aLength < aShortest ? -1 : aLength - aShortest + 1;
	int aTurnWeight = gGame->mGoal <= 10 ? 40 : 12;
	return testFill(thePath) * 8 + aTurns * aTurnWeight + testStars(thePath) * 6 + aLength;
}

static bool testTile(int theCell)
{
	char aLetter = gGame->mLetters[theCell];
	// the letters, the wild tiles, and the two-letter tiles (their codes as Game.cpp's isDouble lists them)
	return ((aLetter >= 'A' && aLetter <= 'Z') || aLetter == '?' || aLetter == '!' ||
		(aLetter != 0 && std::string("aceilrst").find(aLetter) != std::string::npos)) &&
		(gGame->mTileFlags[theCell] & 2) == 0;
}

static void testStep(int theCell, std::string& theWord, std::string& theSpelled, std::vector<int>& thePath,
	std::vector<int>& theBest, std::string& theBestWord, int& theVisits);

// theWord: the letters so far (a wild tile as the letter it stands for), theSpelled: as the dictionary spells them
// (Q is QU and a two-letter tile its two letters, as expandWord makes them)
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
		int aCell = aColumn * BOARD_ROWS + aRow;
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
		// a wild tile is one letter in the game's check (Dictionary::isWord), so never the Q tile's QU
		if (aLetter == 'Q' && aFirst != aLast)
			continue;
		theWord.push_back(aLetter);
		size_t aSize = theSpelled.size();
		theSpelled += expandWord(Str(1, aLetter), 0);
		thePath.push_back(theCell);
		if (testPrefix(theSpelled))
			testSearch(theCell / BOARD_ROWS, theCell % BOARD_ROWS, theWord, theSpelled, thePath, theBest, theBestWord, theVisits);
		thePath.pop_back();
		theSpelled.resize(aSize);
		theWord.pop_back();
	}
}

// port (test runs): the bot's best word (up to 10 tiles, by testValue) on the board's letter and wild tiles that are not
// fixed (as Game::findHint takes them), by the game's dictionary: its cells into thePath and its letters into theWord
// (a wild tile as the letter it stands for). Read only, and it draws no random number. A board already searched gives
// the answer it gave before, since a driver asks many times while the board stands still.
static void testBest(std::vector<int>& thePath, std::string& theWord)
{
	static std::string sKey, sWord;
	static std::vector<int> sPath;
	std::string aKey(gGame->mLetters, gGame->mLetters + BOARD_CELLS);
	aKey.append((const char*)gGame->mTileFlags, sizeof gGame->mTileFlags);
	aKey += std::to_string(gGame->mGoal) + " " + std::to_string(gGame->mLevel);
	if (aKey == sKey)
	{
		thePath = sPath;
		theWord = sWord;
		return;
	}
	sKey = aKey;
	std::vector<int> aBest;
	std::string aBestWord;
	int aVisits = 0;
	// the cells still to fill first, so that the search's budget goes first to the words that fill them
	std::vector<int> aStarts;
	for (int aPass = 0; aPass < 2; aPass++)
	{
		for (int aCell = 0; aCell < BOARD_CELLS; aCell++)
		{
			if (!testTile(aCell) || gGame->currentBook()->isCell(aCell / 9, aCell % 9) == 0)
				continue;
			if ((testFill(std::vector<int>(1, aCell)) > 0) == (aPass == 0))
				aStarts.push_back(aCell);
		}
	}
	for (size_t i = 0; i < aStarts.size(); i++)
	{
		std::string aWord, aSpelled;
		std::vector<int> aPath;
		testStep(aStarts[i], aWord, aSpelled, aPath, aBest, aBestWord, aVisits);
	}
	sPath = aBest;
	sWord = aBestWord;
	thePath = aBest;
	theWord = aBestWord;
}

// port (test runs): the bot's best word as "word <letters> x y..." (Start.h)
std::string testWord()
{
	if (gGameScreen == nullptr || gGame == nullptr || gDictionary == nullptr || gDictionary->mCount == 0)
		return "word\n";
	std::vector<int> aBest;
	std::string aBestWord;
	testBest(aBest, aBestWord);
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

// port (test runs): whether two cells touch (Game::neighbour's six directions)
static bool testTouch(int theCell, int theOther)
{
	for (int aDirection = 0; aDirection < 6; aDirection++)
	{
		int aColumn, aRow;
		if (gGame->neighbour(theCell / BOARD_ROWS, theCell % BOARD_ROWS, aDirection, &aColumn, &aRow) != 0 && aColumn * BOARD_ROWS + aRow == theOther)
			return true;
	}
	return false;
}

// port (test runs): a path over theMarked's cells (the tiles a tutorial balloon marks) that spells theWord
static bool testMarkedPath(const std::string& theWord, const std::vector<int>& theMarked, std::vector<int>& thePath)
{
	if (thePath.size() == theWord.size())
		return true;
	for (size_t i = 0; i < theMarked.size(); i++)
	{
		int aCell = theMarked[i];
		if (std::find(thePath.begin(), thePath.end(), aCell) != thePath.end() ||
			gGame->mLetters[aCell] != theWord[thePath.size()])
			continue;
		if (!thePath.empty() && !testTouch(thePath.back(), aCell))
			continue;
		thePath.push_back(aCell);
		if (testMarkedPath(theWord, theMarked, thePath))
			return true;
		thePath.pop_back();
	}
	return false;
}

// port (test runs): a widget's centre in game coordinates (800x600), its hit box's where it has one
static void testCentre(Screen* theScreen, Widget* theWidget, int* theX, int* theY)
{
	float aX = theScreen->mX + theWidget->mX + theWidget->mWidth / 2;
	float aY = theScreen->mY + theWidget->mY + theWidget->mHeight / 2;
	if (theWidget->mHitX != 0.0f || theWidget->mHitY != 0.0f)
	{
		aX = theScreen->mX + theWidget->mX + theWidget->mHitX + theWidget->mHitWidth / 2;
		aY = theScreen->mY + theWidget->mY + theWidget->mHitY + theWidget->mHitHeight / 2;
	}
	*theX = (int)(aX * 800.0f + 0.5f);
	*theY = (int)(aY * 600.0f + 0.5f);
}

// port (test runs): the bot's next move on the board, for the test command "move" (docs/TEST_COMMANDS.md), as a player
// makes it: a tutorial balloon's own button (Continue); the word a balloon names over the tiles it marks; else the best
// word (testBest), or the game's own hint when the bot has none. Its input clicks off the board first (Game::pick clears
// a path left over), then each tile, then Submit. Read only.
static std::string testMove()
{
	if (gBonnies == nullptr || gBonnies->mState != 3 || gGameScreen == nullptr || gGame == nullptr ||
		gDictionary == nullptr || gDictionary->mCount == 0 || gBonnies->inModal() || gGameScreen->mSequenceState != 1 ||
		gGame->mGameOverTime != 0)
		return "MOVE null";
	char aLine[96];
	// a balloon's own button (Continue, on the balloon's screen): any active button of a visible screen but the board's
	// Menu, Submit and Scramble
	for (Screen* aScreen = gBonnies->mHead; aScreen != nullptr; aScreen = aScreen->mNext)
	{
		for (Widget* aWidget = aScreen->mHead; aWidget != nullptr; aWidget = aWidget->mNext)
		{
			if ((aWidget->mType != 1 && aWidget->mType != 3) || aWidget->isActive() == 0 || aWidget->mId == 0xc9 ||
				aWidget->mId == 0xca || aWidget->mId == 0xcb)
				continue;
			int aX, aY;
			testCentre(aScreen, aWidget, &aX, &aY);
			snprintf(aLine, sizeof aLine, "MOVE {\"kind\":\"button\",\"id\":%d,\"input\":[{\"click\":[%d,%d]}]}",
				aWidget->mId, aX, aY);
			return aLine;
		}
	}
	std::vector<int> aMarked;
	for (int aCell = 0; aCell < BOARD_CELLS; aCell++)
	{
		if ((gGame->mTileFlags[aCell] & 0x40) != 0 && gGame->mLetters[aCell] != 0)
			aMarked.push_back(aCell);
	}
	std::vector<int> aPath;
	std::string aWord;
	const char* aKind = "word";
	if (!gGameScreen->mHintText.empty() && !aMarked.empty())
	{
		// the balloon's word: its first word of three or more capitals
		const std::string& aText = gGameScreen->mHintText;
		std::string aToken;
		for (size_t i = 0; i <= aText.size() && aWord.empty(); i++)
		{
			char aChar = i < aText.size() ? aText[i] : ' ';
			if ((aChar >= 'A' && aChar <= 'Z') || (aChar >= 'a' && aChar <= 'z'))
			{
				aToken += aChar;
				continue;
			}
			bool aCapitals = aToken.size() >= 3;
			for (size_t j = 0; j < aToken.size(); j++)
				aCapitals = aCapitals && aToken[j] >= 'A' && aToken[j] <= 'Z';
			if (aCapitals)
				aWord = aToken;
			aToken.clear();
		}
		if (!aWord.empty() && !testMarkedPath(aWord, aMarked, aPath))
			aPath.clear();
		aKind = "tutorial";
	}
	else
		testBest(aPath, aWord);
	if (aPath.size() < 3 && (gGameScreen->mHintText.empty() || aMarked.empty()) && gGameScreen->mHintShown != 0 &&
		gGame->mHintFound != 0 && gGame->mHintLength >= 3)
	{
		aPath.clear();
		aWord.clear();
		for (int i = 0; i < gGame->mHintLength && i < 20; i++)
		{
			int aCell = gGame->mHintColumns[i] * BOARD_ROWS + gGame->mHintRows[i];
			aPath.push_back(aCell);
			aWord += gGame->mLetters[aCell];
		}
		aKind = "hint";
	}
	if (aPath.size() < 3)
		return "MOVE null";
	std::string aMove = std::string("MOVE {\"kind\":\"") + aKind + "\",\"word\":\"" + aWord + "\",\"cells\":[";
	std::string anInput = "{\"click\":[20,300]}";
	for (size_t i = 0; i < aPath.size(); i++)
	{
		float aX, aY;
		gGameScreen->tilePos(aPath[i] / 9, aPath[i] % 9, &aX, &aY);
		snprintf(aLine, sizeof aLine, ",{\"click\":[%d,%d]}", (int)(aX * 800.0f + 0.5f), (int)(aY * 600.0f + 0.5f));
		anInput += aLine;
		aMove += (i == 0 ? "" : ",") + std::to_string(aPath[i]);
	}
	Widget* aSubmit = gGameScreen->findWidget(0xca);
	if (aSubmit == nullptr)
		return "MOVE null";
	int aX, aY;
	testCentre(gGameScreen, aSubmit, &aX, &aY);
	snprintf(aLine, sizeof aLine, ",{\"click\":[%d,%d]}", aX, aY);
	anInput += aLine;
	return aMove + "],\"input\":[" + anInput + "]}";
}

// port (test runs): the standard test commands (docs/TEST_COMMANDS.md), for -script and the live command channel
// (Start.h). The game keeps its own loops and the framework runs inside its sleeps, so a command only changes state the
// game's own code then acts on: a screen is asked for as the game asks (setState's later switch), and a level ends
// through the game's own checks or its own ending (Game::levelWon). A command never given changes nothing.
static const char* const kScreens[8] = {"none", "title", "menu", "game", "help", "story", "credits", "highscores"};

static std::string testJsonState()
{
	std::string aText = "{";
	char aLine[256];
	int aState = gBonnies != nullptr ? gBonnies->mState : 0;
	snprintf(aLine, sizeof aLine, "\"screen\":\"%s\",\"modal\":%d,\"paused\":%d", aState >= 0 && aState < 8 ?
		kScreens[aState] : "none", gBonnies != nullptr ? gBonnies->mDepth : 0, gBonnies != nullptr ? gBonnies->mPaused : 0);
	aText += aLine;
	if (aState == 3 && gGameScreen != nullptr && gGame != nullptr)
	{
		bool aClassic = gGame->mClassic != 0;
		snprintf(aLine, sizeof aLine, ",\"mode\":\"%s\",\"level\":%d,\"score\":%d,\"phase\":%d", aClassic ? "classic" :
			"action", gGame->mLevel, gGame->mScore, gGameScreen->mSequenceState);
		aText += aLine;
		if (aClassic)
			snprintf(aLine, sizeof aLine, ",\"moves\":%d", gGame->mGoal);
		else
			snprintf(aLine, sizeof aLine, ",\"timer\":%.3f", gGame->mClock);
		aText += aLine;
		snprintf(aLine, sizeof aLine, ",\"filled\":%d,\"fill\":%d", gGame->countFlag20(), gGame->currentBook()->mFillCount);
		aText += aLine;
		// the board by rows from the top, a character a column: the tile's letter ('*' a star, '?' and '!' wild,
		// lower case a two-letter tile), '.' no cell, ' ' an empty cell; "fixed" the cells of locked tiles, as column*9+row
		aText += ",\"board\":[";
		std::string aFixed;
		for (int aRow = 0; aRow < BOARD_ROWS; aRow++)
		{
			std::string aLineText;
			for (int aColumn = 0; aColumn < BOARD_COLUMNS; aColumn++)
			{
				int aCell = aColumn * BOARD_ROWS + aRow;
				char aLetter = gGame->mLetters[aCell];
				if (gGame->currentBook()->isCell(aColumn, aRow) == 0)
					aLineText += '.';
				else
					aLineText += aLetter == 0 ? ' ' : (aLetter == '"' || aLetter == '\\') ? '?' : aLetter;
				if ((gGame->mTileFlags[aCell] & 2) != 0)
					aFixed += (aFixed.empty() ? "" : ",") + std::to_string(aCell);
			}
			aText += (aRow == 0 ? "\"" : ",\"") + aLineText + "\"";
		}
		aText += "],\"fixed\":[" + aFixed + "]";
		// the cells still to fill (no flag 0x20), as column*9+row
		std::string aOpen;
		for (int aCell = 0; aCell < BOARD_CELLS; aCell++)
		{
			if (gGame->currentBook()->isCell(aCell / 9, aCell % 9) != 0 && (gGame->mTileFlags[aCell] & 0x20) == 0)
				aOpen += (aOpen.empty() ? "" : ",") + std::to_string(aCell);
		}
		aText += ",\"unfilled\":[" + aOpen + "]";
	}
	return aText + "}";
}

void memoryHolders(std::vector<std::pair<std::string, size_t>>* theRows)
{
	if (gResources == nullptr)
		return;
	Resources::MemoryUse aUse = Resources::memoryUse();
	theRows->push_back({"images in the cache (" + std::to_string(aUse.mImageCount) + ", 32 bits; the fonts' are among them)",
		aUse.mImageBytes});
	if (gDictionary != nullptr)
		theRows->push_back({"dictionary (" + std::to_string(gDictionary->mCount) + " words)", gDictionary->memoryBytes()});
}

static bool testInPlay()
{
	return gBonnies != nullptr && gBonnies->mState == 3 && gGameScreen != nullptr && gGame != nullptr &&
		gGameScreen->mSequenceState == 1 && !gBonnies->inModal();
}

std::string testCommand(const std::string& theCommand, const std::string& theArgs)
{
	std::string aLine = theArgs.empty() ? theCommand : theCommand + " " + theArgs;
	if (theCommand == "state")
		return "STATE " + testJsonState();
	if (theCommand == "move")
		return testMove();
	if (theCommand == "goto")
	{
		for (int i = 1; i < 8; i++)
		{
			if (theArgs != kScreens[i] || i == 3 || i == 5)
				continue;
			if (gBonnies == nullptr || gBonnies->inModal())
				return "ERROR " + aLine + ": a dialog is open";
			gBonnies->setState(i, 1, 0);
			return "OK " + aLine;
		}
		if (theArgs == "options")
			return "ERROR " + aLine + ": the options are a dialog the menu and the game open, not a screen";
		if (theArgs == "classic" || theArgs == "action")
			return "ERROR " + aLine + ": a mode starts from the menu's buttons, through its player and difficulty "
				"dialogs; use goto menu, then the buttons";
		return "ERROR " + aLine + ": no such screen (title, menu, help, credits, highscores)";
	}
	if (theCommand == "level")
	{
		int aLevel = atoi(theArgs.c_str());
		if (aLevel < 1 || aLevel > LEVEL_COUNT)
			return "ERROR " + aLine + ": levels are 1 to 50";
		if (!testInPlay())
			return "ERROR " + aLine + ": no level in play";
		// the level before it won, so that the game goes on to it as after any won level (its story page, its board)
		gGame->mLevel = aLevel - 1;
		gGame->levelWon();
		return "OK " + aLine;
	}
	if (theCommand == "win")
	{
		if (!testInPlay())
			return "ERROR " + aLine + ": no level in play";
		gGame->levelWon();
		return "OK " + aLine;
	}
	if (theCommand == "lose")
	{
		if (!testInPlay())
			return "ERROR " + aLine + ": no level in play";
		// out of turns or of time: Game::update ends the game as it does then
		if (gGame->mClassic != 0)
			gGame->mGoal = 0;
		else
			gGame->mClock = 0.0f;
		return "OK " + aLine;
	}
	if (theCommand == "set")
	{
		size_t aSpace = theArgs.find(' ');
		std::string aName = theArgs.substr(0, aSpace);
		std::string aValue = aSpace == std::string::npos ? "" : theArgs.substr(aSpace + 1);
		if (aValue.empty())
			return "ERROR " + aLine + ": set <name> <value>";
		if (gBonnies == nullptr || gBonnies->mState != 3 || gGame == nullptr)
			return "ERROR " + aLine + ": no game in play";
		if (aName == "score")
		{
			gGame->mScore = atoi(aValue.c_str());
			gGame->mShownScore = gGame->mScore;
		}
		else if (aName == "moves" && gGame->mClassic != 0)
			gGame->mGoal = atoi(aValue.c_str());
		else if (aName == "timer" && gGame->mClassic == 0)
			gGame->mClock = (float)atof(aValue.c_str());
		else
			return "ERROR " + aLine + ": the keys are score, and moves in the classic mode or timer (0 to 1) in the action "
				"mode";
		return "OK " + aLine;
	}
	if (theCommand == "unlock")
		return "ERROR " + aLine + ": nothing is locked in Bonnie's (every mode and chapter is open from the start)";
	if (theCommand == "seed")
	{
		if (theArgs.empty())
			return "ERROR " + aLine + ": seed <n>";
		// the game draws every random number from rand (randomRange, 0x41f500)
		seedRandom(strtoul(theArgs.c_str(), nullptr, 10));
		return "OK " + aLine;
	}
	return "";
}

} // namespace bonnies
