// The game in progress (see Game.h).

#include "Game.h"
#include "BonniesApp.h"
#include "Book.h"
#include "Dialog.h"
#include "Dictionary.h"
#include "File.h"
#include "FloatText.h"
#include "Font.h"
#include "GameScreen.h"
#include "Globals.h"
#include "Graphics.h"
#include "HighScores.h"
#include "Host.h"
#include "Input.h"
#include "Language.h"
#include "Options.h"
#include "Platform.h"
#include "Sound.h"
#include "Widget.h"

#include <cmath>
#include <cstring>

namespace bonnies
{

static uint32_t ticks()
{
	return host()->getTicks();
}

// 0x405d60
void sparkleTile(int theColumn, int theRow, float theOffsetY, int theSmall, int theExact, float theCount, int theRate)
{
	int aCount;
	if (theExact != 0)
		aCount = (int)(theCount + 0.5f);
	else
	{
		double aWanted = theRate * (double)theCount * 0.001f;
		aCount = (int)aWanted;
		float aFraction = (float)(aWanted - aCount);
		if (randomRange(0, 1000) * 0.001f < aFraction)
			aCount++;
	}
	if (0 < aCount)
	{
		int aLeft = aCount;
		do
		{
			float aX;
			float aY;
			gGameScreen->tilePos(theColumn, theRow, &aX, &aY);
			FloatText* aText = new FloatText();
			aText->mScale1 = (float)(randomRange(0x32, 0x46) * 0.0099999998f);
			aText->mScale2 = (float)(randomRange(0x46, 0x5a) * 0.0099999998f);
			aText->mStartX = (float)(randomRange(-0xf, 0xf) * 0.0012499999f + aX);
			aText->mStartY = (float)(randomRange(-0xf, 0xf) * 0.0016666667f + aY + theOffsetY);
			int aDuration = randomRange(700, 1000) << 1;
			aText->mDuration = aDuration;
			aText->mFadeTime = (int)(aDuration * 0.5f);
			double anAngle = randomRange(0, 1000) * 0.001f * 6.2831802f;
			aText->mSpeedX = (float)(std::sin(anAngle) * 0.15f);
			aText->mSpeedY = (float)(std::cos(anAngle) * 0.15f);
			aText->mGravity = (float)(randomRange(0x2d, 0x69) * 0.0064999997f);
			aText->mAngle = 0.0f;
			aText->mSpin = (float)randomRange(-10, 10);
			int aFrame;
			if (theSmall != 0)
				aFrame = randomRange(4, 6);
			else
			{
				aFrame = randomRange(8, 0xd);
				aText->mAngle = (float)randomRange(0, 0x5a);
				aText->mSpin = (float)randomRange(0xfa, 0x1c2);
				aText->mScale1 = aText->mScale1 * 1.3f;
				aText->mScale2 = aText->mScale2 * 1.3f;
				aText->mDuration = (int)(aText->mDuration * 0.8f);
				aText->mFadeTime = (int)(aText->mFadeTime * 0.8f);
				aText->mGravity = 0.7f;
				aText->mSpeedX = aText->mSpeedX * 0.8f;
				aText->mSpeedY = aText->mSpeedY * 0.8f;
			}
			aText->mFrame = &gBonnies->mImages[14]->mFrames[aFrame];
			gFloatTexts->add(aText);
			gBonnies->mRedrawUntil = (int)(ticks() + 500 + aText->mDuration);
			aLeft--;
		} while (aLeft != 0);
	}
}

// 0x406070
void sparkleBoard()
{
	int aCount = 0;
	float aSumY = 0.0f;
	float aSumX = 0.0f;
	int aColumn = 0;
	do
	{
		int aRow = 0;
		do
		{
			if (gGame->currentBook()->isCell(aColumn, aRow) != 0)
			{
				float aX;
				float aY;
				gGameScreen->tilePos(aColumn, aRow, &aX, &aY);
				aSumX = aX + aSumX;
				aCount++;
				aSumY = aY + aSumY;
			}
			aRow++;
		} while (aRow < 9);
		aColumn++;
	} while (aColumn < 9);
	int aDivisor = 1;
	if (0 < aCount)
		aDivisor = aCount;
	float aCentreX = aSumX / (float)aDivisor;
	aDivisor = 1;
	if (0 < aCount)
		aDivisor = aCount;
	float aCentreY = aSumY / (float)aDivisor;
	aColumn = 0;
	do
	{
		int aRow = 0;
		do
		{
			if (gGame->currentBook()->isCell(aColumn, aRow) != 0)
			{
				float aX;
				float aY;
				gGameScreen->tilePos(aColumn, aRow, &aX, &aY);
				int aLeft = 3;
				do
				{
					FloatText* aText = new FloatText();
					aText->mScale1 = (float)(randomRange(0x32, 0x46) * 0.0099999998f);
					aText->mScale2 = (float)(randomRange(0x46, 0x5a) * 0.0099999998f);
					aText->mStartX = (float)(randomRange(-0x14, 0x14) * 0.0012499999f + aX);
					aText->mStartY = (float)(randomRange(-0x14, 0x14) * 0.0016666667f + aY);
					aText->mDuration = randomRange(0x44c, 0x578);
					gBonnies->mRedrawUntil = (int)(ticks() + 500 + aText->mDuration);
					aText->mFadeTime = 0;
					double anAngle = angleOf(aText->mStartX - aCentreX, aText->mStartY - aCentreY);
					aText->mSpeedX = (float)(std::sin(anAngle + 0.31415901f) * 0.17f);
					aText->mSpeedY = (float)(std::cos(anAngle + 0.31415901f) * 0.17f);
					aText->mGravity = (float)(randomRange(0x2d, 0x69) * 0.0064999997f);
					aText->mFrame = &gBonnies->mImages[14]->mFrames[randomRange(1, 3)];
					aText->mAngle = 0.0f;
					aText->mSpin = (float)randomRange(-10, 10);
					gFloatTexts->add(aText);
					aLeft--;
				} while (aLeft != 0);
			}
			aRow++;
		} while (aRow < 9);
		aColumn++;
	} while (aColumn < 9);
}

// 0x406330
Str& appendInt(Str& theStr, int theValue)
{
	theStr += intToStr(theValue);
	return theStr;
}

// 0x406360
int playSound(const Str& theName, float theVolume, int theUnused, int theDelay)
{
	return gSound->play(theName, theVolume, theUnused, theDelay, 1.0f);
}

// 0x4063e0
int playSound(const Str& theName)
{
	return gSound->play(theName, 1.0f, 1, 0, 1.0f);
}

// 0x406ff0
Game::Game()
{
	mInProgress = 0;
}

// 0x406490
Game::~Game()
{
}

// 0x406520
int Game::countFlag20()
{
	int aCount = 0;
	for (int i = 0; i < 81; i++)
	{
		if ((mTileFlags[i] & 0x20) != 0)
			aCount++;
	}
	return aCount;
}

// 0x406570
int Game::countTiles()
{
	int aCount = 0;
	for (int i = 0; i < 81; i++)
	{
		if (mLetters[i] != '\0')
			aCount++;
	}
	return aCount;
}

// 0x4065d0
int Game::countStars()
{
	int aCount = 0;
	for (int i = 0; i < 81; i++)
	{
		if (mLetters[i] == '*')
			aCount++;
	}
	return aCount;
}

// 0x406620
int isNeighbour(int theColumn1, int theRow1, int theColumn2, int theRow2, int* theDirection)
{
	if (theColumn1 == theColumn2 && std::abs(theRow2 - theRow1) == 1)
	{
		if (theDirection == nullptr)
			return 1;
		*theDirection = theRow2 < theRow1 ? 0 : 3;
		return 1;
	}
	if (std::abs(theColumn2 - theColumn1) != 1)
		return 0;
	int aRows = theRow2 - theRow1;
	if ((theColumn1 & 1) == 0)
	{
		if (theRow2 != theRow1 && aRows != -1)
			return 0;
		if (theDirection != nullptr)
		{
			if (theColumn1 < theColumn2)
			{
				*theDirection = (aRows != -1) + 1;
				return 1;
			}
			*theDirection = (aRows == -1) + 4;
		}
	}
	else
	{
		if (theRow2 != theRow1 && aRows != 1)
			return 0;
		if (theDirection != nullptr)
		{
			if (theColumn2 <= theColumn1)
			{
				*theDirection = 5 - (theRow2 != theRow1);
				return 1;
			}
			*theDirection = (theRow2 != theRow1) + 1;
			return 1;
		}
	}
	return 1;
}

// 0x406710
Book* Game::currentBook()
{
	return &gBooks[(mLevel - 1) % 0x32];
}

// 0x406730
void Game::setDifficulty(int theDifficulty)
{
	mDifficulty = theDifficulty;
	mGoal = (int)(currentBook()->mGoalBase * (double)exeFloat(0x457bdc + theDifficulty * 4));
}

// 0x406770
void Game::save(Str theName)
{
	Str aPath = "";
	aPath += savedGamesDir(true);
	aPath += theName;
	aPath += exeGlobalString(0x476abc);
	int aFile = fileOpen(aPath, true, false);
	if (aFile == -1)
	{
		// The name with the user's added: the original adds to theName itself, so each try is longer
		Str aUser = userName();
		theName += lit(0x457bf4);
		theName += aUser;
		Str aNewName = theName;
		aPath = "";
		aPath += savedGamesDir(true);
		aPath += aNewName;
		aPath += exeGlobalString(0x476abc);
		aFile = fileOpen(aPath, true, false);
		if (aFile == -1)
		{
			int i = 1;
			do
			{
				aPath = "";
				int aNumber = i;
				if (3 < i)
					aNumber = randomRange(100, 300);
				theName += lit(0x457bf4);
				theName += aUser;
				theName += lit(0x457bf4);
				theName += intToStr(aNumber);
				aNewName = theName;
				aPath += savedGamesDir(true);
				aPath += aNewName;
				aPath += exeGlobalString(0x476abc);
				aFile = fileOpen(aPath, true, false);
				if (aFile != -1)
					goto opened;
				i++;
			} while (i < 8);
			return;
		}
	opened:
		gOptions->mName = aNewName;
		gOptions->save();
	}
	mFieldC8C = 0;
	mFieldC88 = 0;
	int aValue = 100;
	fileWriteInt(aFile, &aValue);
	aValue = 0x3e9;
	fileWriteInt(aFile, &aValue);
	int aCell = 0;
	int aLeft = 9;
	do
	{
		int aRow = 0;
		do
		{
			fileWriteByte(aFile, &mLetters[aCell + aRow]);
			fileWriteInt(aFile, &mKinds[aCell + aRow]);
			fileWriteInt(aFile, &mTileFlags[aCell + aRow]);
			fileWriteByte(aFile, &mField43C[aCell + aRow]);
			fileWriteInt(aFile, &mFallOffsets[aCell + aRow]);
			fileWriteInt(aFile, &mFallSpeeds[aCell + aRow]);
			fileWriteInt(aFile, &mBounces[aCell + aRow]);
			aRow++;
		} while (aRow < 9);
		aCell += 9;
		aLeft--;
	} while (aLeft != 0);
	int i = 0;
	do
	{
		fileWriteInt(aFile, &mPathColumns[i]);
		fileWriteInt(aFile, &mPathRows[i]);
		i++;
	} while (i < 0x14);
	if (mLevel == 0x32)
	{
		if (mFieldC5C == 0)
		{
			if (!(mFieldC94 > 0.0f) || countFlag20() >= countTiles())
				mFieldC44 = 1;
		}
		else if (mGoal <= 0 || countFlag20() >= countTiles())
			mFieldC44 = 1;
	}
	aValue = 0x3ea;
	fileWriteInt(aFile, &aValue);
	int aZero = 0;
	fileWriteInt(aFile, &mFieldC5C);
	fileWriteInt(aFile, &mDifficulty);
	fileWriteInt(aFile, &mScore);
	fileWriteInt(aFile, &mLevel);
	fileWriteInt(aFile, &mFieldC44);
	fileWriteInt(aFile, &mFieldC94);
	fileWriteInt(aFile, &mGoal);
	fileWriteInt(aFile, &mFieldCA0);
	fileWriteInt(aFile, &mFieldCAC);
	fileWriteInt(aFile, &mPathLength);
	fileWriteInt(aFile, &mFieldCA4);
	fileWriteInt(aFile, &mWildOdds);
	fileWriteInt(aFile, &mSuperWildOdds);
	fileWriteInt(aFile, &mStarOdds);
	fileWriteInt(aFile, &mStoneOdds);
	fileWriteInt(aFile, &mFieldC64);
	fileWriteString(aFile, &mStrCC8);
	fileWriteString(aFile, &mStrCD0);
	fileWriteInt(aFile, &mFieldC58);
	fileWriteInt(aFile, &aZero);
	fileWriteInt(aFile, &mCopyOdds);
	fileWriteInt(aFile, &mFieldB88);
	fileWriteInt(aFile, &mFieldCA8);
	char aByte = 0;
	int aBytes = 0xb4;
	do
	{
		fileWriteByte(aFile, &aByte);
		aBytes--;
	} while (aBytes != 0);
	aValue = 0x3eb;
	fileWriteInt(aFile, &aValue);
	mRecord.save(aFile);
	fileClose(aFile);
}

// 0x406e40
void Game::clearTileMarks()
{
	for (int i = 0; i < 81; i++)
		mTileFlags[i] = mTileFlags[i] & 0xffffffbf;
}

// 0x406e80
char Game::difficultyFactor()
{
	if (mDifficulty == 0)
		return 1;
	return (mDifficulty != 1) * 2 + 2;
}

// 0x406ea0
int Game::pathColumn(int theIndex)
{
	if (-1 < theIndex && theIndex < mPathLength && theIndex < 0x14)
		return mPathColumns[theIndex];
	return 0;
}

// 0x406ed0
int Game::pathRow(int theIndex)
{
	if (-1 < theIndex && theIndex < mPathLength && theIndex < 0x14)
		return mPathRows[theIndex];
	return 0;
}

// 0x406f00
void Game::setPath(int theIndex, int theColumn, int theRow)
{
	if (-1 < theIndex && theIndex < mPathLength && theIndex < 0x14)
	{
		mPathColumns[theIndex] = theColumn;
		mPathRows[theIndex] = theRow;
	}
}

// 0x406f30
char Game::pathLetter(int theIndex)
{
	return mLetters[pathColumn(theIndex) * 9 + pathRow(theIndex)];
}

// 0x406f90
char Game::pathKind(int theIndex)
{
	return (char)mKinds[pathColumn(theIndex) * 9 + pathRow(theIndex)];
}

// 0x407090
void Game::updateWord()
{
	char aBuffer[400];
	int aLength = mPathLength;
	int i = 0;
	mWordChecked = 1;
	if (0 < aLength)
	{
		do
		{
			aBuffer[i] = mLetters[pathColumn(i) * 9 + pathRow(i)];
			i++;
		} while (i < aLength);
	}
	aBuffer[i] = '\0';
	mWord = aBuffer;
	int anIsWord = gDictionary->isWord(mWord, 1);
	mIsWord = anIsWord;
	if (anIsWord != 0)
		mSpelling = gDictionary->mMatch;
	else
		mSpelling = mWord;
}

// 0x407170
void Game::shuffle(int theStar)
{
	int aMoves = mFieldCA0;
	int aLastMoves = mFieldD90;
	mFieldD90 = aMoves;
	mPathLength = 0;
	mWordChecked = 0;
	mHintFound = 0;
	mFieldCA8++;
	if (theStar != 0 && countStars() < currentBook()->mMaxStars)
	{
		int aTries = 0;
		do
		{
			aTries++;
			int aColumn = randomRange(0, 8);
			int aRow = randomRange(0, 7);
			int aCell = aColumn * 9 + aRow;
			char aLetter;
			if ((mTileFlags[aCell] & 6) == 0 && (aLetter = mLetters[aCell]) != '\0' && mLetters[aCell + 1] != '\0' &&
				aLetter != '*' && aLetter != '!' && (mKinds[aCell] & 3) == 0)
			{
				mLetters[aCell] = '*';
				break;
			}
		} while (aTries < 500);
	}
	// Each cell's tile goes to aTo (-2: it has none to move, -1: it moves)
	int aTo[81][2];
	int aKinds[81];
	int aLetters[81];
	int aTaken[81];
	for (int aRow = 0; aRow < 9; aRow++)
	{
		for (int aColumn = 0; aColumn < 9; aColumn++)
		{
			int aCell = aColumn * 9 + aRow;
			aLetters[aCell] = mLetters[aCell];
			aTaken[aCell] = 0;
			aTo[aCell][0] = -2;
			aTo[aCell][1] = -2;
			aKinds[aCell] = mKinds[aCell];
		}
	}
	for (int aRow = 0; aRow < 9; aRow++)
	{
		for (int aColumn = 0; aColumn < 9; aColumn++)
		{
			int aCell = aColumn * 9 + aRow;
			char aLetter;
			if ((mTileFlags[aCell] & 6) == 0 && (aLetter = mLetters[aCell]) != '\0' &&
				((aLetter != '*' && aLetter != '!' && aLetter != '?') ||
					aLetter != currentBook()->mCells[aCell].mTile || (char)mTileFlags[aCell] < '\0'))
			{
				aTo[aCell][0] = -1;
				aTo[aCell][1] = -1;
			}
		}
	}
	int aPass = 0;
	for (;;)
	{
		for (int aRow = 0; aRow < 9; aRow++)
		{
			for (int aColumn = 0; aColumn < 9; aColumn++)
			{
				int aCell = aColumn * 9 + aRow;
				int* aTarget = aTo[aCell];
				if (*aTarget != -1)
					continue;
				if (aPass == 0)
				{
					if (mLetters[aCell] == '*')
					{
						// A '*' goes to a cell with a tile below it, more likely one of flag 0x20
						int aTries = 0;
						int aNewColumn;
						int aNewRow;
						int aChance;
						do
						{
							int aTry;
							int aNew;
							do
							{
								aTry = aTries + 1;
								if (0x5db < aTries)
									goto starPlaced;
								aNewColumn = randomRange(0, 8);
								aNewRow = randomRange(0, 7);
								aTries = aTry;
							} while ((aNewColumn == aColumn && aNewRow == aRow) ||
								(aNew = aNewRow + aNewColumn * 9, aTaken[aNew] != 0) || aTo[aNew][0] == -2 ||
								aTo[aNew + 1][0] == -2);
							if ((mTileFlags[aNewColumn * 9 + aNewRow] & 0x20) != 0 || 299 < aTry)
								break;
							aChance = 0x32;
							if (mFieldC5C == 0)
							{
								if (0.15f <= mFieldC94)
								{
									if (mFieldC94 < 0.3f)
										aChance = 0x4b;
								}
								else
									aChance = 100;
							}
							else if (mGoal < 6)
								aChance = 100;
							else if (mGoal < 0xc)
								aChance = 0x4b;
						} while (randomRange(0, 100) < aChance);
						aTarget[0] = aNewColumn;
						aTarget[1] = aNewRow;
						aTaken[aNewRow + aNewColumn * 9] = 1;
					starPlaced:
						if (*aTarget == -1)
						{
							aTarget[0] = aColumn;
							aTarget[1] = aRow;
						}
					}
				}
				else
				{
					int aTries = 0;
					int aNewColumn;
					int aNewRow;
					do
					{
						int aTry = aTries + 1;
						if (0x5db < aTries)
							goto placed;
						aNewColumn = randomRange(0, 8);
						aNewRow = randomRange(0, 8);
						aTries = aTry;
					} while ((aNewColumn == aColumn && aNewRow == aRow) || aTaken[aNewColumn * 9 + aNewRow] != 0 ||
						aTo[aNewColumn * 9 + aNewRow][0] == -2);
					aTarget[1] = aNewRow;
					aTarget[0] = aNewColumn;
					aTaken[aNewRow + aNewColumn * 9] = 1;
				placed:
					if (*aTarget == -1)
					{
						aTarget[0] = aColumn;
						aTarget[1] = aRow;
					}
				}
			}
		}
		aPass++;
		if (1 < aPass)
			break;
	}
	for (int i = 0; i < 81; i++)
	{
		mFrom[i][0] = -2;
		mFrom[i][1] = -2;
	}
	int aRow = 0;
	do
	{
		for (int aColumn = 0; aColumn < 9; aColumn++)
		{
			int aCell = aColumn * 9 + aRow;
			if (-1 < aTo[aCell][0])
			{
				int aNew = aTo[aCell][0] * 9 + aTo[aCell][1];
				mFrom[aNew][0] = aColumn;
				mFrom[aNew][1] = aRow;
				mLetters[aNew] = (char)aLetters[aCell];
				mKinds[aNew] = aKinds[aCell];
			}
		}
		aRow++;
	} while (aRow < 9);
	aRow = 0;
	do
	{
		int aColumn = 0;
		do
		{
			int aCell = aColumn * 9 + aRow;
			if (mLetters[aCell] != '\0' && mFrom[aCell][0] < 0)
			{
				mFrom[aCell][0] = aColumn;
				mFrom[aCell][1] = aRow;
			}
			aColumn++;
		} while (aColumn < 9);
		aRow++;
	} while (aRow < 9);
	mShuffleTime = ticks();
	if (gBonnies->mDebug1E8 == 0)
	{
		if (0.02f <= mFieldC94)
		{
			double aValue = (double)mFieldC94 - 0.050000001f;
			if (aValue < 0.02f)
				aValue = 0.02f;
			mFieldC94 = (float)aValue;
		}
		if (1 < mGoal)
			mGoal--;
	}
	gGameScreen->updateHint();
	if (aMoves != aLastMoves)
	{
		if (aMoves - aLastMoves == 1)
			gGameScreen->mFieldB0 = ticks() + 6000;
		return;
	}
	gGameScreen->mFieldB0 = ticks() + 0x9c4;
}

// 0x4079a0
void Game::refill()
{
	mHintFound = 0;
	int aColumn = 0;
	int aCell = 0;
	do
	{
		int aRow = 0;
		do
		{
			if (mLetters[aCell + aRow] == '*' && (currentBook()->mCells[aCell + aRow].mFlags & 4) == 0 &&
				mBounces[aCell + aRow] < 3 && (aRow == 8 || currentBook()->isCell(aColumn, aRow + 1) == 0))
			{
				float aX;
				float aY;
				gGameScreen->tilePos(aColumn, aRow, &aX, &aY);
				char aText[2];
				aText[0] = mLetters[aCell + aRow];
				aText[1] = '\0';
				FloatText* aFloat = new FloatText();
				if (aText[0] == '?')
					aFloat->mFrame = gBonnies->mImages[9]->mFrames;
				else if (aText[0] == '!')
					aFloat->mFrame = gBonnies->mImages[30]->mFrames;
				else if (aText[0] == '*')
					aFloat->mFrame = gBonnies->mImages[10]->mFrames;
				else
				{
					aFloat->mColor2 = 0x6fffffff;
					aFloat->mEndColor2 = 0x6fffffff;
					aFloat->mColor1 = 0xff000000;
					aFloat->mEndColor1 = 0xff000000;
					aFloat->mFlags = 0x109;
					aFloat->mText = aText;
					aFloat->mFont = gBonnies->mFonts[6];
				}
				aFloat->mScale1 = 1.0f;
				aFloat->mScale2 = 0.7f;
				aFloat->mStartX = (float)(aX + 0.0024999999f);
				aFloat->mSpeedX = 0.0f;
				aFloat->mSpeedY = 0.125f;
				aFloat->mGravity = 1.0f;
				aFloat->mDuration = 0x578;
				aFloat->mStartY = (float)(aY + 0.0016666667f);
				gFloatTexts->add(aFloat);
				mLetters[aCell + aRow] = '\0';
				gSound->play(lit(0x457c08), 0.65f, 1, 0, 1.0f);
			}
			aRow++;
		} while (aRow < 9);
		aCell += 9;
		aColumn++;
	} while (aCell < 0x51);
	// The cells where a special tile may come in. The original's count can go below 0 (row 4 of a column without
	// the cell below it), and then the next ones are written before the array, over other locals: the padding takes
	// them
	int aSpawnStore[2 * (0x13 + 9)];
	int* aSpawns = aSpawnStore + 2 * 9;
	int aSpawnCount = 0;
	aColumn = 0;
	aCell = 0;
	do
	{
		int aNew = 0;
		int aRow = 8;
		do
		{
			if (currentBook()->isCell(aColumn, aRow) != 0 && mLetters[aCell + aRow] == '\0')
			{
				mTileFlags[aCell + aRow] = mTileFlags[aCell + aRow] | 0x80;
				float aSpeed;
				if (aRow < 8)
					aSpeed = mFallSpeeds[aCell + aRow + 1];
				else
					aSpeed = 0.0001f;
				mFallSpeeds[aCell + aRow] = (float)(aSpeed - randomRange(2, 0x1e) * 0.0016666667f);
				mBounces[aCell + aRow] = randomRange(4, 6);
				int anAbove = aRow - 1;
				if (-1 < anAbove)
				{
					do
					{
						if (currentBook()->isCell(aColumn, anAbove) == 0 || (mTileFlags[aCell + anAbove] & 2) != 0)
							break;
						if (mLetters[aCell + anAbove] != '\0')
						{
							mFallOffsets[aCell + aRow] = (float)(anAbove - aRow) * 0.076666668f;
							mLetters[aCell + aRow] = mLetters[aCell + anAbove];
							mLetters[aCell + anAbove] = '\0';
							mKinds[aCell + aRow] = mKinds[aCell + anAbove];
							mKinds[aCell + anAbove] = 0;
							goto next;
						}
						anAbove--;
					} while (-1 < anAbove);
				}
				if (aRow < 8 && currentBook()->isCell(aColumn, aRow + 1) != 0 && aSpawnCount < 0x13)
				{
					aSpawns[aSpawnCount * 2] = aColumn;
					aSpawns[aSpawnCount * 2 + 1] = aRow;
					aSpawnCount++;
				}
				if (mRecord.mSeen[2] == 0 && aRow == 4 && mLevel != 1)
					aSpawnCount--;
				mLetters[aCell + aRow] = randomLetter(aColumn, 1);
				float aNewF = (float)aNew;
				mKinds[aCell + aRow] = 0xc;
				aNew++;
				mFallOffsets[aCell + aRow] = (float)(-0.090000004f - (aNewF + 1.0f) * 1.1f * 0.076666668f);
			}
		next:
			aRow--;
		} while (-1 < aRow);
		aRow = 7;
		do
		{
			if (currentBook()->isCell(aColumn, aRow) != 0 && currentBook()->isCell(aColumn, aRow + 1) != 0 &&
				(mKinds[aCell + aRow] & 8) != 0 && mFallOffsets[aCell + aRow + 1] < 0.0f)
			{
				double aLimit = mFallOffsets[aCell + aRow + 1] - 0.02f;
				float* anOffset = &mFallOffsets[aCell + aRow];
				if (*anOffset < aLimit)
					aLimit = *anOffset;
				*anOffset = (float)aLimit;
			}
			aRow--;
		} while (-1 < aRow);
		for (int i = 0; i < 9; i++)
			mKinds[aCell + i] = mKinds[aCell + i] & 0xfffffff7;
		aCell += 9;
		aColumn++;
	} while (aCell < 0x51);
	if (currentBook()->mStones != 0)
	{
		double aStones = aSpawnCount + 0.0099999998f;
		aStones = aStones - (int)mStarOdds;
		aStones = aStones - (int)mWildOdds;
		aStones = aStones - (int)mSuperWildOdds;
		if (!(0.0f <= aStones))
			aStones = 0.0f;
		mStoneOdds = (float)aStones;
	}
	bool aSuperWild = false;
	if (0 < aSpawnCount)
	{
		float aLast = (float)(aSpawnCount - 1);
		for (;;)
		{
			int anIndex = randomRange(0, (int)aLast);
			int aSpawnColumn = aSpawns[anIndex * 2];
			int aTries = 0;
			while (aSpawnColumn == -1 && aTries < 1000)
			{
				anIndex = randomRange(0, (int)aLast);
				aSpawnColumn = aSpawns[anIndex * 2];
				aTries++;
			}
			aSpawnColumn = aSpawns[anIndex * 2];
			int* aSpawn = aSpawns + anIndex * 2;
			if (aSpawnColumn == -1)
				break;
			int aTile;
			if (1.0f <= mStarOdds && currentBook()->allows('*', aSpawnColumn) != 0)
			{
				aTile = '*';
				mStarOdds = mStarOdds - 1.0f;
			}
			else if (1.0f <= mWildOdds && currentBook()->allows('?', aSpawnColumn) != 0)
			{
				aTile = '?';
				mWildOdds = mWildOdds - 1.0f;
			}
			else if (1.0f <= mSuperWildOdds && currentBook()->allows('!', aSpawnColumn) != 0)
			{
				int aCount = 0;
				for (int i = 0; i < 81; i++)
				{
					if (mLetters[i] == '!')
						aCount++;
				}
				if (aCount < 2)
					aTile = '!';
				else
				{
					aTile = '?';
					mFieldC74 = 1;
				}
				mSuperWildOdds = mSuperWildOdds - 1.0f;
			}
			else if (1.0f <= mStoneOdds && currentBook()->allows('#', aSpawnColumn) != 0)
			{
				aTile = '#';
				mStoneOdds = mStoneOdds - 1.0f;
			}
			else if (1.0f <= mCopyOdds && currentBook()->allows('(', aSpawnColumn) != 0)
			{
				aTile = '(';
				mCopyOdds = mCopyOdds - 1.0f;
			}
			else
				break;
			int aSpawnRow = aSpawn[1];
			int aSpawnCell = aSpawnColumn * 9 + aSpawnRow;
			if (aTile == '#')
			{
				mKinds[aSpawnCell] = mKinds[aSpawnCell] | 1;
				mLetters[aSpawnCell] = '@';
				*aSpawn = -1;
			}
			else if (aTile == '(')
			{
				mKinds[aSpawnCell] = mKinds[aSpawnCell] | 2;
				mLetters[aSpawnCell] = mCopyLetter;
				*aSpawn = -1;
			}
			else
			{
				mLetters[aSpawnCell] = (char)aTile;
				if (aTile == '!')
					aSuperWild = true;
				*aSpawn = -1;
			}
		}
	}
	int i = 0;
	aColumn = 0;
	do
	{
		int aRow = 0;
		do
		{
			if ((mTileFlags[i] & 0x100) != 0 && aSuperWild)
				sparkleTile(aColumn, aRow, 0.0f, 0, 1, 7.0f, 0);
			aRow++;
			i++;
		} while (aRow < 9);
		aColumn++;
	} while (aColumn < 9);
	mWildOdds = mWildOdds <= 1.01f ? mWildOdds : 1.01f;
	mSuperWildOdds = mSuperWildOdds <= 1.01f ? mSuperWildOdds : 1.01f;
	mStoneOdds = 0.0f;
	mCopyOdds = 0.0f;
	gGameScreen->mField7C = 0;
	aColumn = 0;
	aCell = 0;
	do
	{
		int aRow = 0;
		do
		{
			if (mLetters[aCell + aRow] == '*' && (aRow == 8 || currentBook()->isCell(aColumn, aRow + 1) == 0))
				gGameScreen->mField7C = 1;
			aRow++;
		} while (aRow < 9);
		aColumn++;
		aCell += 9;
	} while (aColumn < 9);
}

// 0x408440
int Game::neighbour(int theColumn, int theRow, int theDirection, int* theNewColumn, int* theNewRow)
{
	switch (theDirection)
	{
	case 0:
		*theNewColumn = theColumn;
		*theNewRow = theRow - 1;
		break;
	case 1:
		*theNewColumn = theColumn + 1;
		*theNewRow = (((theColumn & 1) != 0) - 1) + theRow;
		break;
	case 2:
		*theNewColumn = theColumn + 1;
		*theNewRow = ((theColumn & 1) != 0) + theRow;
		break;
	case 3:
		*theNewColumn = theColumn;
		*theNewRow = theRow + 1;
		break;
	case 4:
		*theNewColumn = theColumn - 1;
		*theNewRow = ((theColumn & 1) != 0) + theRow;
		break;
	case 5:
		*theNewColumn = theColumn - 1;
		*theNewRow = (((theColumn & 1) != 0) - 1) + theRow;
		break;
	}
	int aColumn = *theNewColumn;
	if (-1 < aColumn && aColumn < 9 && -1 < *theNewRow && *theNewRow < 9 &&
		currentBook()->isCell(aColumn, *theNewRow) != 0)
		return 1;
	return 0;
}

// 0x408530
void Game::addLevelBonus()
{
	int aFactor = mDifficulty;
	if (mFieldC5C == 0)
	{
		if (aFactor == 0)
			aFactor = 1;
		else
			aFactor = (aFactor != 1) * 2 + 2;
		mScore = mScore + (mLevel + 1) * aFactor * mFieldC9C * 5;
		mFieldC9C = 0;
	}
	else
	{
		if (aFactor == 0)
			aFactor = 1;
		else
			aFactor = (aFactor != 1) * 2 + 2;
		mScore = mScore + (mLevel + 1) * mGoal * aFactor * 0x14;
		mGoal = 0;
	}
	mShownScore = mScore;
	mFieldC70 = 0;
}

// 0x4085e0
void Game::beginLevel()
{
	mRecord.mGames++;
	mFieldC8C = 0;
	mFieldC88 = 0;
	int aLevel = mLevel;
	int aBest = mRecord.mBestLevel;
	if (mRecord.mBestLevel <= aLevel)
		aBest = aLevel;
	mRecord.mLastLevel = aLevel;
	mRecord.mBestLevel = aBest;
	mFieldC9C = (int)(currentBook()->mBonusBase * (double)exeFloat(0x457be8 + mDifficulty * 4) * mFieldC94);
}

// 0x408650
void Game::fillOneTile()
{
	int aTries = 0;
	int aColumn;
	int aRow;
	char aLetter;
	for (;;)
	{
		int aTry = aTries + 1;
		if (4999 < aTries)
			return;
		aColumn = randomRange(0, 8);
		aRow = randomRange(0, 8);
		aLetter = randomLetter(aColumn, 1);
		aTries = aTry;
		if (aTry < 0xbb9)
		{
		checkStar:
			if (aLetter == '*' && (aRow == 8 || currentBook()->isCell(aColumn, aRow + 1) == 0))
				continue;
		}
		else
		{
			int aPick = aTry & 3;
			if (aPick == 0)
				aLetter = 'Q';
			else if (aPick == 1)
				aLetter = 'J';
			else
			{
				if (aPick != 2)
					goto checkStar;
				aLetter = 'Z';
			}
		}
		if (mLetters[aColumn * 9 + aRow] == '\0' && currentBook()->isCell(aColumn, aRow) != 0)
			break;
	}
	BookCell& aBookCell = currentBook()->mCells[aColumn * 9 + aRow];
	if (mFilling != 0 || (aBookCell.mFlags & 4) != 0)
	{
		int aLevel = mLevel;
		if (!(aBookCell.mTile < '\x01' ||
			(aLevel == 1 && 0 < gGame->mRecord.mBestLevel && gGame->mRecord.mSeen[0] != 0 &&
				gGame->mRecord.mSeen[2] != 0) ||
			(aLevel == 2 && 1 < gGame->mRecord.mBestLevel && gGame->mRecord.mSeen[10] != 0 &&
				currentBook()->mCells[aColumn * 9 + aRow].mTile != '*')))
		{
			// Level 32's first tiles are the book's only in English
			bool aKeep = false;
			if (aLevel == 0x20 && equalsNoCase(getText(2), lit(0x457c18)) == 0)
				aKeep = true;
			if (!aKeep)
				aLetter = aBookCell.mTile;
		}
	}
	mLetters[aColumn * 9 + aRow] = aLetter;
}

// 0x408900
int Game::newBoard()
{
	mFieldC9C = 0;
	mCopyLetter = 'A';
	mFieldB88 = 0;
	mFieldC74 = 0;
	mFieldC84 = 0;
	mFieldC88 = 0;
	mFieldC8C = 0;
	mShuffleTime = 0;
	mFieldC7C = 0;
	mFieldCA0 = 0;
	mFieldCA8 = 0;
	mFieldD90 = -99;
	mFieldD8C = ticks();
	mFieldC64 = mScore;
	mFilling = 1;
	mPathLength = 0;
	mFieldCDC = 0;
	mWordChecked = 0;
	mHintFound = 0;
	mWildOdds = 0.0f;
	mSuperWildOdds = 0.0f;
	mStarOdds = 0.0f;
	mStoneOdds = 0.0f;
	mCopyOdds = 0.0f;
	mStrCC8 = "";
	mFieldC58 = 0;
	mFieldC94 = 1.0f;
	mFieldCAC = -1;
	mGoal = (int)(currentBook()->mGoalBase * (double)exeFloat(0x457bdc + mDifficulty * 4));
	if (gGameScreen != nullptr)
		gGameScreen->updateHint();
	int aCell = 0;
	do
	{
		int aRow = 0;
		do
		{
			mFrom[aCell + aRow][0] = 0;
			mFrom[aCell + aRow][1] = 0;
			mField2F8[aCell + aRow] = 0;
			mField43C[aCell + aRow] = '\0';
			mLetters[aCell + aRow] = '\0';
			mKinds[aCell + aRow] = currentBook()->mCells[aCell + aRow].mKind;
			mTileFlags[aCell + aRow] = currentBook()->mCells[aCell + aRow].mFlags;
			mFallOffsets[aCell + aRow] = 0.0f;
			mFallSpeeds[aCell + aRow] = 0.0f;
			aRow++;
		} while (aRow < 9);
		aCell += 9;
	} while (aCell < 0x51);
	while (countTiles() < currentBook()->mFillCount)
		fillOneTile();
	mFilling = 0;
	return (mLevel - 1) / 0x32;
}

} // namespace bonnies

namespace bonnies
{

static uint32_t gLastErrorSound;  // 0x476acc
static uint32_t gLastUpdate;      // 0x4766bc

// 0x408b30: a click (or, theDrag, the mouse dragged) on the tile at theColumn, theRow (-1: off the board); theLastColumn,
// theLastRow where the drag was before. Answers 1 when the click is ignored
int Game::pick(int theColumn, int theRow, int theLastColumn, int theLastRow, int theDrag)
{
	// (the original logs "HMP 1" to "HMP 5" through a log that does nothing)
	if (0 < gBonnies->mSpecialMode && theDrag == 0)
	{
		// The debug mode that turns the tile into mSpecialMode's kind
		mPathLength = 0;
		if (theColumn == -1)
			return 1;
		int aMode = gBonnies->mSpecialMode;
		int aCell = theRow + theColumn * 9;
		char aLetter = mLetters[aCell];
		char aNew = aLetter;
		if (0x40 < aMode && aMode < 0x5b)
			aNew = (char)gBonnies->mSpecialMode;
		uint32_t aFlags;
		switch (aMode)
		{
		case '!':
			mLetters[aCell] = aLetter != '!' ? '!' : 'A';
			return 1;
		default:
			mLetters[aCell] = aNew;
			return 1;
		case '#':
			if ((mKinds[aCell] & 1) == 0)
				mKinds[aCell] = mKinds[aCell] | 1;
			else
				mKinds[aCell] = mKinds[aCell] & 0xfffffffe;
			mLetters[aCell] = aNew;
			return 1;
		case '&':
			aFlags = mTileFlags[aCell];
			if ((aFlags & 0x20) == 0)
				aFlags = aFlags | 0x20;
			else
				aFlags = aFlags & 0xffffffdf;
			break;
		case '(':
			if ((mKinds[aCell] & 2) == 0)
				mKinds[aCell] = mKinds[aCell] | 2;
			else
				mKinds[aCell] = mKinds[aCell] & 0xfffffffd;
			mLetters[aCell] = aNew;
			return 1;
		case ')':
			aFlags = mTileFlags[aCell];
			if ((aFlags & 8) == 0)
				aFlags = aFlags | 8;
			else
				aFlags = aFlags & 0xfffffff7;
			break;
		case '*':
			mLetters[aCell] = aLetter != '*' ? '*' : 'A';
			return 1;
		case '?':
			mLetters[aCell] = aLetter == '?' ? 'A' : '?';
			return 1;
		case '^':
			aFlags = mTileFlags[aCell];
			if ((aFlags & 2) == 0)
				aFlags = aFlags | 2;
			else
				aFlags = aFlags & 0xfffffffd;
			break;
		}
		mTileFlags[aCell] = aFlags;
		mLetters[aCell] = aNew;
		return 1;
	}
	mWordChecked = 0;
	if (gGameScreen->isBusy())
	{
		// A hint is up: its step allows only some tiles
		if (gGameScreen->mTutorialStep == 1 && (theColumn != 5 || theRow != 3))
			return 1;
		if (gGameScreen->mTutorialStep == 3)
		{
			if (theColumn != 6)
				return 1;
			if (theRow != 2)
				return 1;
			goto checkStep;
		}
	}
	if (theColumn == -1)
	{
		if (mPathLength != 0)
			mFieldCDC = ticks();
		mPathLength = 0;
		goto placed;
	}
checkStep:
	if (gGameScreen->isBusy())
	{
		int aStep = gGameScreen->mTutorialStep;
		// The words the first steps spell, a tile at a time: the k-th tile may be picked with k or k + 1 on the path
		static const int aWords[3][4][2] = {
			{{3, 1}, {4, 2}, {5, 2}, {5, 3}},
			{{4, 3}, {5, 3}, {6, 3}, {6, 2}},
			{{4, 5}, {4, 6}, {5, 6}, {6, 7}},
		};
		int aWord = aStep == 0 ? 0 : aStep == 2 ? 1 : aStep == 10 ? 2 : -1;
		if (aWord != -1)
		{
			int aLength = mPathLength;
			bool anAccept = false;
			for (int k = 0; k < 4; k++)
			{
				if ((aLength == k || aLength == k + 1) && theColumn == aWords[aWord][k][0] &&
					theRow == aWords[aWord][k][1])
					anAccept = true;
			}
			if (!anAccept)
			{
				if (0 < aLength && theColumn == pathColumn(aLength - 1) && theRow == pathRow(aLength - 1))
					return 0;
				if (700 < (int)(ticks() - gLastErrorSound))
				{
					gLastErrorSound = ticks();
					playSound(lit(0x457620), 0.65f);
				}
				return 0;
			}
		}
		else if (!(aStep == 4 || aStep == 6 || aStep == 7 || aStep == 8 || aStep == 9 || aStep == 0xb || aStep == 0xc ||
			aStep == 0xf || aStep == 0x10 || aStep == 0x11 || aStep == 0x16 || aStep == 0x17 || aStep == 0x18 ||
			aStep == 0x1e || aStep == 0x1f || aStep == 0x20 || aStep == 199 ||
			(mTileFlags[theColumn * 9 + theRow] & 0x40) != 0))
			return 0;
	}
	gGameScreen->mFieldA4 = 1;
	if (theDrag == 0)
		gGameScreen->mFieldA8 = 1;
	else if (theLastColumn != theColumn || theLastRow != theRow)
		gGameScreen->mFieldA8 = gGameScreen->mFieldA8 + 1;
	if (mPathLength == 0)
	{
		mPathLength = 1;
		setPath(0, theColumn, theRow);
	}
	else
	{
		// A tile on the path cuts it back to there (the only one: to nothing, unless dragging)
		bool aFound = false;
		int i = 0;
		if (0 < mPathLength)
		{
			do
			{
				if (pathColumn(i) == theColumn && pathRow(i) == theRow)
				{
					aFound = true;
					if (mPathLength == 1)
						mPathLength = theDrag != 0;
					else
						mPathLength = i + 1;
				}
				i++;
			} while (i < mPathLength);
			if (aFound)
				goto placed;
		}
		// A neighbour of a tile on the path, the last first, continues from it (the parity is the new tile's)
		i = mPathLength - 1;
		if (-1 < i)
		{
			do
			{
				int aRow = pathRow(i);
				int aColumn = pathColumn(i);
				bool aNext = false;
				if (theColumn == aColumn && std::abs(aRow - theRow) == 1)
					aNext = true;
				else if (std::abs(aColumn - theColumn) == 1)
				{
					if ((theColumn & 1) == 0)
						aNext = aRow == theRow || aRow - theRow == -1;
					else
						aNext = aRow == theRow || aRow - theRow == 1;
				}
				if (aNext && i < 0x13)
				{
					mPathLength = i + 2;
					setPath(i + 1, theColumn, theRow);
					goto placed;
				}
				i--;
			} while (-1 < i);
		}
		if (theDrag != 0)
			return 0;
		mPathLength = 1;
		mPathColumns[0] = theColumn;
		mPathRows[0] = theRow;
	}
placed:
	if (0 < mPathLength && (theDrag == 0 || theLastColumn != theColumn || theLastRow != theRow))
	{
		Str aName = lit(0x457c28);
		aName += intToStr(randomRange(1, 3));
		playSound(aName);
	}
	return 0;
}

// 0x4093f0: looks for a word of 3 to 10 tiles by random walks for at most 120 ms, the best in the hint
int Game::findHint()
{
	int aTries = 0;
	mHintFound = 0;
	int aBest = -1;
	uint32_t aStart = ticks();
	do
	{
		aTries++;
		if (0x77 < (int)(ticks() - aStart))
			break;
		char aWord[52];
		std::memset(aWord, 0, 11);
		int aColumns[10];
		int aRows[10];
		aColumns[0] = randomRange(0, 8);
		aRows[0] = randomRange(0, 8);
		int aCell = aRows[0] + aColumns[0] * 9;
		char aLetter = mLetters[aCell];
		if (aLetter != '\0' && (mTileFlags[aCell] & 2) == 0 && aLetter != '*')
		{
			int aFresh = ((mTileFlags[aCell] & 0x20) == 0) * 0x32;
			aWord[0] = aLetter;
			int aCount = 1;
			int aLength = 0x1e;
			int k = 0;
			for (;;)
			{
				int aDirection = randomRange(0, 5);
				if (neighbour(aColumns[k], aRows[k], aDirection, &aColumns[k + 1], &aRows[k + 1]) == 0 &&
					neighbour(aColumns[k], aRows[k], (aDirection + 1) % 6, &aColumns[k + 1], &aRows[k + 1]) == 0)
					break;
				int aColumn = aColumns[k + 1];
				int aRow = aRows[k + 1];
				aCell = aColumn * 9 + aRow;
				uint32_t aFlags = mTileFlags[aCell];
				if ((aFlags & 2) != 0 || (aLetter = mLetters[aCell]) == '*')
					break;
				if ((aFlags & 0x20) == 0)
					aFresh = aFresh + 0x32;
				aWord[aCount] = aLetter;
				int i = 0;
				bool aRepeat = false;
				do
				{
					if (aColumns[i] == aColumn && aRows[i] == aRow)
					{
						aRepeat = true;
						break;
					}
					i++;
				} while (i < aCount);
				if (aRepeat)
					break;
				aCount++;
				k++;
				aLength = aLength + 0x1e;
				if (1 < k && gDictionary->isWord(Str(aWord), 1) != 0)
				{
					int aScore = (aLength + aFresh) * gDictionary->mMatchRarity;
					if (gDictionary->mMatchRarity < 2)
						aScore = -999;
					if (k < 3)
						aScore = aScore >> 1;
					if (3 < k)
						aScore = aScore + 0x14;
					if (aBest < aScore)
					{
						mHintFound = 1;
						mHintLength = aCount;
						aBest = aScore;
						for (i = 0; i < aCount; i++)
						{
							mHintColumns[i] = aColumns[i];
							mHintRows[i] = aRows[i];
						}
					}
				}
				if (8 < k)
					break;
			}
		}
	} while (aTries < 0x9c4);
	return mHintFound;
}

// Marks theCount cells for a hint
static void markCells(Game* theGame, const int (*theCells)[2], int theCount)
{
	for (int i = 0; i < theCount; i++)
	{
		uint32_t& aFlags = theGame->mTileFlags[theCells[i][0] * 9 + theCells[i][1]];
		aFlags = aFlags | 0x40;
	}
}

// 0x409680: the tutorial's hints, shown once each, and the end of the one up
void Game::updateHints()
{
	if (gGameScreen != nullptr)
		gGameScreen->mField11C = 1;
	if (gBonnies->mDebug1FC != 0)
	{
		gGameScreen->leave(1);
		return;
	}
	bool aMoved = mFieldCAC < mFieldCA8 + mFieldCA0;
	float aX = 0.5f;
	float aY = 0.4f;
	int aText;
	int aStep;
	int aSide;
	int aFlag;
	if (!gGameScreen->isBusy())
	{
		if (gBonnies->inModal())
			return;
		if (gGameScreen->mField58 != 1)
			return;
		if (mRecord.mSeen[0] == 0 && mLevel == 1)
		{
			mRecord.mSeen[0] = 1;
			static const int aCells[4][2] = {{3, 1}, {4, 2}, {5, 2}, {5, 3}};
			markCells(this, aCells, 4);
			gGameScreen->tilePos(3, 1, &aX, &aY);
			aFlag = 0;
			aSide = 0;
			aStep = 0;
			aText = 0x456;
			goto show;
		}
		if (mRecord.mSeen[2] == 0 && mLevel == 1)
		{
			// The second word, when the board still has it
			mRecord.mSeen[2] = 1;
			Str aWord = getText(0x3ea);
			if (mLetters[39] == (char)toUpper((unsigned char)at(aWord, 0)) &&
				mLetters[48] == (char)toUpper((unsigned char)at(aWord, 1)) &&
				mLetters[57] == (char)toUpper((unsigned char)at(aWord, 2)) &&
				mLetters[56] == (char)toUpper((unsigned char)at(aWord, 3)))
			{
				static const int aCells[4][2] = {{4, 3}, {5, 3}, {6, 3}, {6, 2}};
				markCells(this, aCells, 4);
				gGameScreen->tilePos(4, 3, &aX, &aY);
				gGameScreen->showHint(Str(getText(0x458)), 2, aX, aY, 2, 0);
			}
			return;
		}
		if (mRecord.mSeen[8] == 0 && mRecord.mSeen[9] == 0 && 7 < mFieldCA0)
		{
			mRecord.mSeen[8] = 1;
			mRecord.mSeen[9] = 1;
			aFlag = 1;
			aSide = 1;
			if (mFieldC5C == 0)
			{
				aStep = 9;
				aText = 0x45f;
				aX = 0.715f;
				aY = 0.98f;
			}
			else
			{
				aStep = 8;
				aText = 0x45e;
				aX = 0.7525f;
				aY = 0.92333335f;
			}
			goto show;
		}
		if (mRecord.mSeen[10] == 0 && mLevel == 2 && mFieldCA0 == 0)
		{
			mRecord.mSeen[10] = 1;
			static const int aCells[4][2] = {{4, 5}, {4, 6}, {5, 6}, {6, 7}};
			markCells(this, aCells, 4);
			gGameScreen->tilePos(4, 4, &aX, &aY);
			aFlag = 0;
			aSide = 2;
			aStep = 10;
			aText = 0x460;
			goto show;
		}
		if (mRecord.mSeen[13] == 0)
		{
			int aColumn = 0;
			do
			{
				int aRow = 0;
				do
				{
					if (mLetters[aColumn * 9 + aRow] == '?')
					{
						mRecord.mSeen[13] = 1;
						gGameScreen->tilePos(aColumn, aRow, &aX, &aY);
						mTileFlags[aRow + aColumn * 9] = mTileFlags[aRow + aColumn * 9] | 0x40;
						int anIndex = (mFieldCA0 == 0 || mLevel == 3) ? 0x1a : 0xd;
						aText = exeI32(0x457b58 + anIndex * 4);
						aFlag = 1;
						aSide = aRow < 5 ? 0 : 2;
						aStep = 0xd;
						goto show;
					}
					aRow++;
				} while (aRow < 9);
				aColumn++;
			} while (aColumn < 9);
		}
		if (mRecord.mSeen[12] == 0 && mRecord.mSeen[13] == 0 && mLevel == 2 && 2 < mFieldCA0)
		{
			// The tutorial's words still on the board
			static const int aCells[3][5][2] = {
				{{0, 1}, {0, 2}, {1, 1}, {2, 1}, {2, 2}},
				{{0, 6}, {0, 7}, {1, 7}, {2, 7}, {3, 6}},
				{{6, 1}, {6, 2}, {6, 3}, {7, 2}, {8, 2}},
			};
			static const int aTexts[3] = {0x3ef, 0x3f0, 0x3f1};
			mRecord.mSeen[12] = 1;
			int aLeft[3] = {1, 1, 1};
			int aCount = 0;
			Str aWords = "";
			for (int aWord = 0; aWord < 3; aWord++)
			{
				for (int i = 0; i < 5; i++)
				{
					Str aText = getText(aTexts[aWord]);
					if ((int)mLetters[aCells[aWord][i][0] * 9 + aCells[aWord][i][1]] != (unsigned char)at(aText, i))
						aLeft[aWord] = 0;
				}
				if (aLeft[aWord] != 0)
				{
					if (aCount != 0)
						aWords += getText(0x49a);
					aCount++;
					aWords += getText(aTexts[aWord]);
				}
			}
			if (aCount != 0)
			{
				Str aMessage = getText(0x462);
				Str aPlaceholder = lit(0x457c50);
				aMessage = replace(aMessage, aPlaceholder, aWords, false, false);
				gGameScreen->showHint(aMessage, 0xc, aX, aY, 6, 0);
			}
			return;
		}
		if (mRecord.mSeen[14] == 0 && 1 < mLevel && mLevel < 5 && 5 < mFieldCA0 && mPathLength == 0 &&
			gInput->mDownTime + 4000 < (int)ticks())
		{
			mRecord.mSeen[14] = 1;
			aX = (float)(gGameScreen->findWidget(0xcb)->mX + 0.0975f);
			aY = (float)(gGameScreen->findWidget(0xcb)->mY + 0.0066666668f);
			aFlag = 1;
			aSide = 1;
			aStep = 0xe;
			aText = 0x464;
			goto show;
		}
		if (mRecord.mSeen[18] == 0)
		{
			int aColumn = 0;
			do
			{
				int aRow = 0;
				do
				{
					if (mLetters[aColumn * 9 + aRow] == '!')
					{
						mRecord.mSeen[18] = 1;
						gGameScreen->tilePos(aColumn, aRow, &aX, &aY);
						mTileFlags[aRow + aColumn * 9] = mTileFlags[aRow + aColumn * 9] | 0x40;
						aFlag = 1;
						aSide = aRow < 5 ? 0 : 2;
						aText = exeI32(0x457b58 + (mFieldCA0 != 0 ? 0x12 : 0x1b) * 4);
						aStep = 0x12;
						goto show;
					}
					aRow++;
				} while (aRow < 9);
				aColumn++;
			} while (aColumn < 9);
		}
		if (mRecord.mSeen[19] == 0)
		{
			int aColumn = 0;
			do
			{
				int aRow = 0;
				do
				{
					if ((mKinds[aColumn * 9 + aRow] & 2) != 0)
					{
						mRecord.mSeen[19] = 1;
						gGameScreen->tilePos(aColumn, aRow, &aX, &aY);
						mTileFlags[aRow + aColumn * 9] = mTileFlags[aRow + aColumn * 9] | 0x40;
						aFlag = 1;
						aSide = aRow < 5 ? 0 : 2;
						aText = exeI32(0x457b58 + (mFieldCA0 != 0 ? 0x13 : 0x1c) * 4);
						aStep = 0x13;
						goto show;
					}
					aRow++;
				} while (aRow < 9);
				aColumn++;
			} while (aColumn < 9);
		}
		if (mRecord.mSeen[20] == 0)
		{
			int aColumn = 0;
			do
			{
				int aRow = 0;
				do
				{
					if ((mKinds[aColumn * 9 + aRow] & 1) != 0)
					{
						mRecord.mSeen[20] = 1;
						gGameScreen->tilePos(aColumn, aRow, &aX, &aY);
						mTileFlags[aRow + aColumn * 9] = mTileFlags[aRow + aColumn * 9] | 0x40;
						aFlag = 1;
						aSide = aRow < 5 ? 0 : 2;
						aText = exeI32(0x457b58 + (mFieldCA0 != 0 ? 0x14 : 0x1d) * 4);
						aStep = 0x14;
						goto show;
					}
					aRow++;
				} while (aRow < 9);
				aColumn++;
			} while (aColumn < 9);
		}
		if (mRecord.mSeen[21] == 0 && mFieldC74 != 0)
		{
			aFlag = 1;
			aSide = 5;
			aStep = 0x15;
			mFieldC74 = 0;
			mRecord.mSeen[21] = 1;
			aText = 0x46b;
			goto show;
		}
		if (mRecord.mSeen[25] == 0 && 0x62 < mFieldB88)
		{
			aFlag = 1;
			aSide = 5;
			aStep = 0x19;
			mRecord.mSeen[25] = 1;
			aText = 0x46f;
			goto show;
		}
		if (mRecord.mSeen[30] == 0 && mLevel == 3)
		{
			aFlag = 0;
			aSide = 6;
			aStep = 0x1e;
			mRecord.mSeen[30] = 1;
			aText = 0x474;
			goto show;
		}
		if (mRecord.mSeen[31] == 0 && mLevel == 6)
		{
			aFlag = 0;
			aSide = 6;
			aStep = 0x1f;
			mRecord.mSeen[31] = 1;
			aText = 0x475;
			goto show;
		}
		if (mRecord.mSeen[32] == 0 && mLevel == 10)
		{
			aFlag = 0;
			aSide = 6;
			aStep = 0x20;
			mRecord.mSeen[32] = 1;
			aText = 0x476;
			goto show;
		}
		if (mFieldC58 != 0)
			return;
		if (currentBook()->hasText() == 0)
			return;
		// The chapter's own text
		mFieldC58 = 1;
		gGameScreen->showHint(currentBook()->text(), 199, aX, aY, 6, 0);
		return;
	}
	switch (gGameScreen->mTutorialStep)
	{
	case 0:
		if (equalsNoCase(mStrC4C, Str(getText(0x3e9))) || 0 < mFieldCA0)
		{
			gGameScreen->leave(1);
			return;
		}
		if (mPathLength != 4)
			return;
		if (Input::leftButtonDown())
			return;
		gGameScreen->leave(0);
		aX = (float)(gGameScreen->findWidget(0xca)->mX + 0.0975f);
		aY = (float)(gGameScreen->findWidget(0xca)->mY + 0.0066666668f);
		aFlag = 0;
		aSide = 1;
		aStep = 1;
		aText = 0x457;
		goto show;
	case 1:
		if (equalsNoCase(mStrC4C, Str(getText(0x3e9))) || 0 < mFieldCA0)
		{
			gGameScreen->leave(1);
			return;
		}
		return;
	case 2:
		if (equalsNoCase(mStrC4C, Str(getText(0x3ea))) || 1 < mFieldCA0)
		{
			gGameScreen->leave(1);
			aFlag = 0;
			aSide = 6;
			aStep = 4;
			aText = 0x45a;
		}
		else
		{
			if (mPathLength != 4)
				return;
			if (Input::leftButtonDown())
				return;
			gGameScreen->leave(0);
			aX = (float)(gGameScreen->findWidget(0xca)->mX + 0.0975f);
			aY = (float)(gGameScreen->findWidget(0xca)->mY + 0.0066666668f);
			aFlag = 0;
			aSide = 1;
			aStep = 3;
			aText = 0x459;
		}
		goto show;
	case 3:
		if (!equalsNoCase(mStrC4C, Str(getText(0x3ea))) && mFieldCA0 < 2)
			return;
		gGameScreen->leave(1);
		aFlag = 0;
		aSide = 6;
		aStep = 4;
		aText = 0x45a;
		goto show;
	case 4:
	case 5:
		if (mFieldCA0 < 3)
			return;
		gGameScreen->leave(1);
		aFlag = 0;
		aSide = 6;
		aStep = 6;
		aText = 0x45c;
		goto show;
	case 6:
		if (mFieldCA0 < 4)
			return;
		gGameScreen->leave(1);
		aFlag = 0;
		aSide = 6;
		aStep = 7;
		aText = 0x45d;
		goto show;
	case 7:
		if (4 < mFieldCA0)
			gGameScreen->leave(1);
		return;
	case 8:
	case 9:
	case 0xb:
	case 0xf:
	case 0x10:
	case 0x16:
	case 0x17:
	case 0x18:
	case 0x1e:
	case 0x1f:
	case 0x20:
	case 199:
		if (aMoved)
			gGameScreen->leave(1);
		return;
	case 10:
		if (!equalsNoCase(mStrC4C, Str(getText(0x3eb))) && mFieldCA0 < 1)
			return;
		gGameScreen->leave(1);
		aFlag = 0;
		aSide = 6;
		aStep = 0xb;
		aText = 0x461;
		goto show;
	case 0xc:
	case 0xd:
		if (aMoved)
			gGameScreen->leave(1);
		if (0 < mPathLength && pathLetter(mPathLength - 1) == '?')
			gGameScreen->leave(1);
		return;
	case 0x12:
		if (0 < mPathLength && pathLetter(mPathLength - 1) == '!')
			gGameScreen->leave(1);
		return;
	case 0x13:
		if (0 < mPathLength && (pathKind(mPathLength - 1) & 2) != 0)
			gGameScreen->leave(1);
		return;
	case 0x14:
		if (0 < mPathLength && (pathKind(mPathLength - 1) & 1) != 0)
			gGameScreen->leave(1);
		return;
	default:
		return;
	}
show:
	gGameScreen->showHint(Str(getText(aText)), aStep, aX, aY, aSide, aFlag);
}

// 0x40a8b0
Str Game::word()
{
	if (mWordChecked == 0)
		updateWord();
	return mWord;
}

// 0x40a8f0
Str Game::spelling()
{
	if (mWordChecked == 0)
		updateWord();
	return mSpelling;
}

// 0x40a930
int Game::isWord()
{
	if (mWordChecked == 0)
		updateWord();
	return mIsWord;
}

// 0x40a950
void Game::nextLevel()
{
	addLevelBonus();
	mLevel = mLevel + 1;
	newBoard();
	mFieldC94 = 1.0f;
	save(gOptions->mName);
}

// 0x40a990: a new game (theReset: the record too)
void Game::prepareSave(int theReset)
{
	mInProgress = 1;
	if (theReset != 0)
		mRecord.reset();
	mFieldC80 = 0;
	mFieldCD8 = 0;
	mStrC4C = "";
	mFieldC5C = 0;
	mDifficulty = 0;
	mScore = 0;
	mShownScore = 0;
	mFieldC70 = 0;
	mFieldC60 = 0;
	mLevel = 1;
	mStrCD0 = "";
	mFieldC44 = 0;
	mFieldC48 = 0;
	mFieldCA4 = 0;
	newBoard();
}

// 0x40aa30
void Game::update()
{
	int anElapsed = 0;
	if (gLastUpdate != 0)
	{
		anElapsed = (int)(ticks() - gLastUpdate);
		if (0x78 < anElapsed)
			anElapsed = 0x78;
	}
	gLastUpdate = ticks();
	int aPending = 0 < mFieldC88 ? mFieldC88 : 0;
	mFieldC88 = aPending;
	if (0.0f < mFieldC8C)
		mFieldC8C = 0.0f;
	if (mFieldC5C == 0)
	{
		if (mFieldC8C < 0.0f)
		{
			int aTime = anElapsed;
			if ((int)(ticks() - mFieldC84) <= anElapsed)
				aTime = (int)(ticks() - mFieldC84);
			if (0 < aTime)
			{
				double aValue = aTime * (double)0.001f * 0.065f + mFieldC8C;
				mFieldC8C = (float)aValue;
				if (0.0 < aValue)
					mFieldC8C = 0.0f;
			}
		}
	}
	else if (aPending != 0)
	{
		uint32_t aLast = mFieldC84;
		int aDelay = 6 < mGoal + aPending ? 500 : 200;
		if (aDelay < (int)(ticks() - aLast))
		{
			mFieldC88 = mFieldC88 + 1;
			mFieldC84 = aLast + aDelay;
		}
		goto counted;
	}
	mFieldC84 = ticks();
counted:
	if (gBonnies->mPaused != 0)
		return;
	updateHints();
	int aStep = anElapsed;
	if (0x31 < anElapsed)
		aStep = 0x32;
	bool aFalling = false;
	int aColumn = 0;
	do
	{
		int aRow = 8;
		do
		{
			int aCell = aColumn * 9 + aRow;
			if (currentBook()->isCell(aColumn, aRow) != 0 && mFallOffsets[aCell] != 0.0f)
			{
				mFieldC80 = frameCount();
				aFalling = true;
				double aTime = aStep * (double)0.001f;
				float aTimeF = (float)aTime;
				double aSpeed = aTime * 1.6666666f + mFallSpeeds[aCell];
				mFallSpeeds[aCell] = (float)aSpeed;
				double anOffset = mFallOffsets[aCell] + aTimeF * aSpeed;
				mFallOffsets[aCell] = (float)anOffset;
				if (!(anOffset < 0.0))
				{
					if (0 < mBounces[aCell])
					{
						mFallOffsets[aCell] = -8.3333333e-05f;
						mBounces[aCell] = mBounces[aCell] - 1;
						mFallSpeeds[aCell] = (float)(aSpeed * -0.37f);
					}
					else
					{
						mFallSpeeds[aCell] = 0.0f;
						mFallOffsets[aCell] = 0.0f;
						mKinds[aCell] = mKinds[aCell] & 0xfffffffb;
					}
				}
			}
			aRow--;
		} while (-1 < aRow);
		aColumn++;
	} while (aColumn < 9);
	if (aFalling)
		refill();
	else
		gGameScreen->mField7C = 0;
	if (200 < anElapsed)
		anElapsed = 200;
	if (!gBonnies->inModal() && !gGameScreen->isBusy() && gBonnies->mDebug1E8 == 0 && gGameScreen->mField7C == 0 &&
		mFieldC60 == 0 && gGameScreen->mField58 == 1 && isInactive() == 0)
	{
		if (mFieldC5C == 0)
		{
			Book* aBook = currentBook();
			if (mFieldC8C == 0.0f)
			{
				int aTime = anElapsed;
				if (0x77 < anElapsed)
					aTime = 0x78;
				double aValue =
					mFieldC94 - aTime / ((double)(aBook->mBonusBase * 1000) * exeFloat(0x457be8 + mDifficulty * 4));
				mFieldC94 = (float)aValue;
				if (!(aValue < 1.0f))
					mFieldC94 = 1.0f;
			}
		}
		else if (mGoal < 1)
			mFieldC60 = ticks() + 0x2d0;
		if (mFieldC5C == 0 && mFieldC94 <= 0.0f)
			mFieldC60 = ticks();
	}
	if (mFieldC60 != 0 && (int)mFieldC60 < (int)ticks())
	{
		// The game is over
		mFieldC60 = 0;
		mFieldC94 = 0.0f;
		gGameScreen->mFieldA4 = 0;
		mPathLength = 0;
		mWordChecked = 0;
		mFieldC44 = 1;
		save(gOptions->mName);
		playSound(lit(0x457c6c), 1.0f, 1, 0x96);
		gGameScreen->setMessage(Str(""), 0);
		Str aMessage = "";
		int aText;
		if (mFieldC48 == 0)
			aText = mFieldC5C == 0 ? 0x4a6 : 0x4a5;
		else
			aText = 0x4a4;
		aMessage += getText(aText);
		aMessage += lit(0x457c64);
		aMessage += getText(0x4a7);
		aMessage += lit(0x457c68);
		aMessage += intToStr(mScore);
		gHighScoreTable = mFieldC5C == 0;
		gHighScoreRank = gHighScores.add(mFieldC5C == 0, gOptions->mName, mStrCD0, mScore);
		if (-1 < gHighScoreRank)
		{
			aMessage += lit(0x457c64);
			aMessage += getText(0x4a8);
		}
		messageDialog(1, aMessage, Str(getText(0x4a9)), Str(""), Str(""), Str(""), 0xd, 0, 0, 0);
		gBonnies->setState(gHighScoreRank < 0 ? 2 : 7, 1, 0);
	}
}

} // namespace bonnies

namespace bonnies
{

// 0x40b100: the saved game theName, played in theMode (its clock and goal converted when the save's was the other)
void Game::load(Str theName, int theMode)
{
	mInProgress = 1;
	mFieldC60 = 0;
	mFieldC9C = 0;
	mStrC4C = "";
	mFieldD8C = ticks();
	mFieldC88 = 0;
	mFieldC8C = 0.0f;
	mShuffleTime = 0;
	mFieldC7C = 0;
	mFieldC74 = 0;
	mFieldC80 = 0;
	mFieldD90 = -99;
	Str aPath = "";
	aPath += savedGamesDir(true);
	aPath += theName;
	aPath += Str(exeGlobalString(0x476abc));
	int aFile = fileOpen(aPath, false, false);
	if (aFile == -1)
		return;
	mIsWord = 0;
	mWordChecked = 0;
	mHintFound = 0;
	mFieldCDC = 0;
	int aVersion;
	fileReadInt(aFile, &aVersion);
	int aMarker;
	fileReadInt(aFile, &aMarker);
	int aCell = 0;
	int aLeft = 9;
	do
	{
		int aRow = 0;
		do
		{
			fileReadByte(aFile, &mLetters[aCell + aRow]);
			fileReadInt(aFile, &mKinds[aCell + aRow]);
			fileReadInt(aFile, &mTileFlags[aCell + aRow]);
			fileReadByte(aFile, &mField43C[aCell + aRow]);
			fileReadInt(aFile, &mFallOffsets[aCell + aRow]);
			fileReadInt(aFile, &mFallSpeeds[aCell + aRow]);
			fileReadInt(aFile, &mBounces[aCell + aRow]);
			mField2F8[aCell + aRow] = 0;
			mFrom[aCell + aRow][0] = 0;
			mFrom[aCell + aRow][1] = 0;
			aRow++;
		} while (aRow < 9);
		aCell += 9;
		aLeft--;
	} while (aLeft != 0);
	int i = 0;
	do
	{
		fileReadInt(aFile, &mPathColumns[i]);
		fileReadInt(aFile, &mPathRows[i]);
		i++;
	} while (i < 0x14);
	fileReadInt(aFile, &aMarker);
	int anUnused;
	fileReadInt(aFile, &mFieldC5C);
	fileReadInt(aFile, &mDifficulty);
	fileReadInt(aFile, &mScore);
	fileReadInt(aFile, &mLevel);
	fileReadInt(aFile, &mFieldC44);
	fileReadInt(aFile, &mFieldC94);
	fileReadInt(aFile, &mGoal);
	fileReadInt(aFile, &mFieldCA0);
	fileReadInt(aFile, &mFieldCAC);
	fileReadInt(aFile, &mPathLength);
	fileReadInt(aFile, &mFieldCA4);
	fileReadInt(aFile, &mWildOdds);
	fileReadInt(aFile, &mSuperWildOdds);
	fileReadInt(aFile, &mStarOdds);
	fileReadInt(aFile, &mStoneOdds);
	fileReadInt(aFile, &mFieldC64);
	fileReadString(aFile, mStrCC8);
	fileReadString(aFile, mStrCD0);
	fileReadInt(aFile, &mFieldC58);
	fileReadInt(aFile, &anUnused);
	fileReadInt(aFile, &mCopyOdds);
	fileReadInt(aFile, &mFieldB88);
	fileReadInt(aFile, &mFieldCA8);
	int aSavedMode = mFieldC5C;
	mFieldC5C = theMode;
	char aByte = 0;
	int aBytes = 0xb4;
	do
	{
		fileReadByte(aFile, &aByte);
		aBytes--;
	} while (aBytes != 0);
	fileReadInt(aFile, &aMarker);
	mRecord.load(aFile);
	fileClose(aFile);
	int aMode = mFieldC5C;
	if (aSavedMode != aMode)
	{
		int aGoal = (int)(currentBook()->mGoalBase * (double)exeFloat(0x457bdc + mDifficulty * 4));
		if (aMode != 0)
		{
			int aNewGoal = (mGoal + (int)(aGoal * (double)mFieldC94 + 0.5f)) >> 1;
			if (aNewGoal < 2)
				aNewGoal = 1;
			mGoal = aNewGoal;
		}
		else
		{
			int aLeftGoal = 1;
			if (1 <= mGoal)
				aLeftGoal = mGoal;
			int aFullGoal = 1;
			if (1 <= aGoal)
				aFullGoal = aGoal;
			double aValue = (((double)aLeftGoal / aFullGoal - 0.02f) + mFieldC94) * 0.5f;
			mFieldC94 = (float)aValue;
			if (!(aValue < 1.0f))
				mFieldC94 = 1.0f;
		}
		if (aMode == 0 && mFieldC94 <= 0.15f)
			mPathLength = 0;
	}
	// A level already won goes on to the next
	bool aWon;
	if (aMode != 0 && mGoal <= 0)
		aWon = true;
	else if (aMode == 0 && mFieldC94 <= 0.0f)
		aWon = true;
	else
		aWon = countTiles() <= countFlag20();
	if (aWon && mLevel < 0x32)
	{
		addLevelBonus();
		mLevel = mLevel + 1;
		newBoard();
		mFieldC94 = 1.0f;
		save(gOptions->mName);
	}
	if (mPathLength == 1)
		mPathLength = 0;
	mShownScore = mScore;
	mFieldC70 = 0;
	if (gGameScreen != nullptr)
		gGameScreen->setChapter(currentBook()->name());
	for (i = 0; i < 81; i++)
		mTileFlags[i] = mTileFlags[i] & 0xffffffbf;
	if (gGameScreen != nullptr)
		gGameScreen->updateHint();
}

// Whether a tile counts as two letters (Qu and the two-letter tiles)
static bool isDouble(char theLetter)
{
	switch (theLetter)
	{
	case 'Q':
	case 'a':
	case 'c':
	case 'e':
	case 'i':
	case 'l':
	case 'r':
	case 's':
	case 't':
		return true;
	}
	return false;
}

// 0x40b760: the path's word's score
int Game::wordScore()
{
	if (mWordChecked == 0)
		updateWord();
	if (mIsWord == 0)
		return 0;
	int aLength = mPathLength;
	int i = 0;
	int aLetters = aLength;
	if (0 < aLength)
	{
		do
		{
			if (isDouble(pathLetter(i)))
				aLetters++;
			i++;
		} while (i < aLength);
	}
	int aScore = aLetters < 3 ? 0 : (mLevel + 1) * 5;
	if (3 < aLetters)
	{
		aLetters = aLetters - 3;
		do
		{
			aScore = aScore * 2;
			aLetters--;
		} while (aLetters != 0);
	}
	if (mDifficulty != 0)
		return ((mDifficulty != 1) * 2 + 2) * aScore;
	return aScore;
}

// 0x40b8d0: the path's word is spelled: scored, its tiles taken (and the odds of the special tiles changed by it)
void Game::submit()
{
	if (gGameScreen->isBusy())
	{
		int aStep = gGameScreen->mTutorialStep;
		if (aStep == 0 && mPathLength != 4)
			return;
		if (aStep == 1 && mPathLength != 4)
			return;
		if (aStep == 2 && mPathLength != 4)
			return;
		if (aStep == 3 && mPathLength != 4)
			return;
		if (aStep == 10 && mPathLength != 4)
			return;
	}
	if (mWordChecked == 0)
		updateWord();
	if (mIsWord == 0)
	{
		playSound(lit(0x457620), 0.85f);
		return;
	}
	// (the original logs "submit 1" to "submit 3" through a log that does nothing)
	Str aMatch = gDictionary->mMatch;
	int aLength = mPathLength;
	int i = 0;
	int aLetters = aLength;
	if (0 < aLength)
	{
		do
		{
			if (isDouble(mLetters[pathColumn(i) * 9 + pathRow(i)]))
				aLetters++;
			i++;
		} while (i < aLength);
	}
	mCopyLetter = mLetters[pathColumn(aLength - 1) * 9 + pathRow(aLength - 1)];
	if (mIsWord != 0 && (int)aMatch.size() != 0)
		mCopyLetter = (char)at(aMatch, (int)aMatch.size() - 1);
	int aLastColumn = pathColumn(mPathLength - 1);
	int aLastRow = pathRow(mPathLength - 1);
	mStrC4C = aMatch;
	int aScore = wordScore();
	mFieldCA0 = mFieldCA0 + 1;
	mFieldCA4 = mFieldCA4 + 1;
	// A word in alphabetical order brings a stone tile, a doubled letter a copycat
	bool anOrdered = true;
	i = 1;
	if ((int)aMatch.size() < 2)
	{
	ordered:
		if ((int)aMatch.size() != 0)
		{
			if (2 < mRecord.mBestLevel)
				mStoneOdds = mStoneOdds + 1.0f;
			goto doubled;
		}
	}
	else
	{
		do
		{
			if ((unsigned char)at(aMatch, i) <= (unsigned char)at(aMatch, i - 1))
				anOrdered = false;
			i++;
		} while (i < (int)aMatch.size());
		if (anOrdered)
			goto ordered;
	doubled:
		if ((int)aMatch.size() != 0)
		{
			i = 1;
			if (1 < (int)aMatch.size())
			{
				do
				{
					if ((char)at(aMatch, i) == (char)at(aMatch, i - 1) && 3 < gGame->mRecord.mBestLevel)
						mCopyOdds = mCopyOdds + 1.0f;
					i++;
				} while (i < (int)aMatch.size());
			}
		}
	}
	// The bonus of a long word: time, or moves
	int aBonus = 0;
	int aShortest = (mFieldC5C != 0) + 5;
	if (aLetters < aShortest)
	{
		if (gBonnies->mDebug1E8 == 0)
			mGoal = mGoal - 1;
	}
	else
		aBonus = (aLetters - aShortest) + 1;
	if ((mFieldC5C != 0 ? 0x14 : 0x28) < mFieldB88 + aBonus)
	{
		aBonus = 0;
		mFieldB88 = 99;
	}
	double aRate = 0.0;
	gGameScreen->mFieldB0 = 0;
	mFieldB88 = mFieldB88 + (aBonus < 1 ? 0 : aBonus);
	// The odds of the special tiles
	if (6 < aLetters)
	{
		mStarOdds = 0.0f;
		mSuperWildOdds = mSuperWildOdds + 1.0f;
	}
	else if (5 < aLetters)
	{
		mStarOdds = 0.0f;
		mWildOdds = mWildOdds + 1.0f;
	}
	else if (4 < aLetters)
	{
		mWildOdds = mWildOdds + 1.0f;
		double aValue = mStarOdds - (double)0.334f;
		if (!(0.0f <= aValue))
			aValue = 0.0f;
		mStarOdds = (float)aValue;
		if (!(aValue < 0.99f))
			aValue = 0.99f;
		mStarOdds = (float)aValue;
	}
	else if (aLetters == 4)
	{
		if (currentBook()->mField3C != 0)
		{
			aRate = 0.501f;
			goto addRate;
		}
		// (the original stores the odds back unchanged when below 0.99)
		if (!(mStarOdds < 0.99f))
			mStarOdds = 0.99f;
	}
	else if (aLetters == 3)
	{
		int aLevel;
		aLevel = mLevel;
		if (currentBook()->mField3C != 0)
			aRate = 1.5f;
		else
		{
			if (mFieldC5C == 0 || aLevel <= 6)
				aRate = 0.401f;
			else if (aLevel <= 0xf)
				aRate = 0.501f;
			else if (aLevel <= 0x1e)
				aRate = 0.601f;
			else if (aLevel <= 0x28)
				aRate = 0.651f;
			double aLimit = 1.99f - aRate;
			if (!(aLimit <= mStarOdds))
				aLimit = mStarOdds;
			mStarOdds = (float)aLimit;
		}
		if (!(mStarOdds < 1.0f) && currentBook()->mMaxStars <= countStars())
			mStarOdds = 0.0f;
		else if (0.0 < aRate)
		{
		addRate:
			int aStars;
			aStars = countStars();
			if (9 < aStars)
				mStarOdds = (float)(aRate * 0.5f + mStarOdds);
			else if (7 < aStars)
				mStarOdds = (float)(aRate * 0.6f + mStarOdds);
			else if (5 < aStars)
				mStarOdds = (float)(aRate * 0.7f + mStarOdds);
			else if (3 < aStars)
				mStarOdds = (float)(aRate * 0.85f + mStarOdds);
			else
				mStarOdds = (float)(aRate + mStarOdds);
		}
	}
	if (gGame->mRecord.mBestLevel < 1)
	{
		mSuperWildOdds = 0.0f;
		mWildOdds = 0.0f;
		mStarOdds = 0.0f;
	}
	mFieldCD8 = frameCount();
	mFieldC70 = ticks();
	mScore = mScore + aScore;
	if ((int)aMatch.size() != 0)
	{
		if ((int)mStrCC8.size() <= (int)aMatch.size())
			mStrCC8 = aMatch;
		if ((int)mStrCD0.size() <= (int)aMatch.size())
			mStrCD0 = aMatch;
		if ((int)mRecord.mStr14.size() <= (int)aMatch.size())
			mRecord.mStr14 = aMatch;
		mRecord.mField0C = mRecord.mField0C + 1;
		mRecord.mField10 = mRecord.mField10 + (int)aMatch.size();
		if ((int)aMatch.size() < 5)
			mRecord.mField20 = 0;
		else
		{
			int aRun = mRecord.mField20 + 1;
			mRecord.mField20 = aRun;
			if (aRun < mRecord.mField1C)
				aRun = mRecord.mField1C;
			mRecord.mField1C = aRun;
		}
	}
	for (i = 0; i < 81; i++)
		mTileFlags[i] = mTileFlags[i] & 0xfffffeff;
	// The path's letters fly off and its tiles go
	int aPathLength = mPathLength;
	float aSumX = 0.0f;
	float aSumY = 0.0f;
	i = 0;
	if (0 < aPathLength)
	{
		do
		{
			uint32_t aColor = addColor(0xff0f0f0f, 0, 0, 100, 0x32);
			int aColumn = pathColumn(i);
			int aRow = pathRow(i);
			float aX;
			float aY;
			gGameScreen->tilePos(aColumn, aRow, &aX, &aY);
			aSumX = aX + aSumX;
			int aCell = aColumn * 9 + aRow;
			char aText[2];
			aText[0] = mLetters[aCell];
			aSumY = aY + aSumY;
			aText[1] = '\0';
			if (aText[0] == '?' || aText[0] == '!')
			{
				Str aSpelling = spelling();
				if (i < (int)aSpelling.size())
					aText[0] = (char)at(aSpelling, i);
			}
			FloatText* aFloat = new FloatText();
			aFloat->mFlags = 0x109;
			aFloat->mColor2 = 0x6fffffff;
			aFloat->mEndColor2 = 0x6fffffff;
			aFloat->mColor1 = aColor;
			aFloat->mEndColor1 = aColor;
			aFloat->mText = aText;
			aFloat->mStartX = (float)(aX + 0.0024999999f);
			aFloat->mDuration = 400;
			aFloat->mFadeTime = 0;
			aFloat->mStartY = (float)(aY + 0.0016666667f);
			int aSpeed = randomRange(-0x19, 0x19);
			aFloat->mScale2 = 1.0f;
			aFloat->mSpeedY = 0.21666667f;
			aFloat->mSpeedX = (float)(aSpeed * 0.0012499999f);
			aFloat->mSpin = (float)randomRange(-0x78, 0x78);
			int aGravity = randomRange(0x5f, 0x69) * 5;
			aFloat->mDuration = 600;
			aFloat->mGravity = (float)(aGravity * 0.0099999998f);
			aFloat->mFont = gBonnies->mFonts[6];
			gFloatTexts->add(aFloat);
			uint32_t aFlags = mTileFlags[aCell];
			if ((aFlags & 8) == 0)
			{
				if (currentBook()->mField38 == 0)
					mTileFlags[aCell] = aFlags | 0x20;
				else if ((aFlags & 0x10) != 0)
					mTileFlags[aCell] = (aFlags & 0xffffffef) | 0x20;
				else if ((aFlags & 0x20) == 0)
					mTileFlags[aCell] = aFlags | 0x10;
			}
			else if ((aFlags & 0x20) == 0)
				mTileFlags[aCell] = aFlags | 0x20;
			else
				mTileFlags[aCell] = aFlags & 0xffffffdf;
			mTileFlags[aCell] = mTileFlags[aCell] | 0x100;
			if ((currentBook()->mCells[aCell].mFlags & 4) == 0 && mLetters[aCell] != '!')
				mLetters[aCell] = '\0';
			// The fixed tiles next to it are freed
			int aDirection = 0;
			do
			{
				int aNewColumn;
				int aNewRow;
				if (neighbour(aColumn, aRow, aDirection, &aNewColumn, &aNewRow) != 0)
				{
					int aNew = aNewRow + aNewColumn * 9;
					if ((mTileFlags[aNew] & 2) != 0)
					{
						mTileFlags[aNew] = mTileFlags[aNew] & 0xfffffffd;
						mField2F8[aNew] = (int)ticks();
						mField43C[aNew] = '^';
					}
				}
				aDirection++;
			} while (aDirection < 6);
			i++;
		} while (i < mPathLength);
	}
	refill();
	int aDivisor = 1;
	if (0 < aPathLength)
		aDivisor = aPathLength;
	float aX = (float)aDivisor;
	aDivisor = 1;
	aX = aSumX / aX;
	if (0 < aPathLength)
		aDivisor = aPathLength;
	float aY = aSumY / (float)aDivisor;
	if (0 < mPathLength)
		gGameScreen->tilePos(pathColumn(mPathLength - 1), pathRow(mPathLength - 1), &aX, &aY);
	// The score and the bonus, beside the last tile
	Str aScoreText = "";
	Str aBonusText = "";
	float aWidth = 0.0f;
	if (0 < aScore)
	{
		aScoreText += lit(0x457ccc);
		aScoreText += intToStr(aScore);
		aWidth = gBonnies->mFonts[9]->textWidth(aScoreText, 1.0f);
	}
	if (0 < aBonus)
	{
		if (mFieldC5C == 0)
		{
			aBonusText += getText(0x4a3);
			aBonusText += lit(0x457cc0);
			if (aBonus * 4 < 10)
				aBonusText += lit(0x457cbc);
			aBonusText += intToStr(aBonus * 4);
		}
		else
		{
			aBonusText += getText(0x4a2);
			aBonusText += lit(0x457cc8);
			aBonusText += intToStr(aBonus);
		}
		float aBonusWidth = gBonnies->mFonts[9]->textWidth(aBonusText, 1.0f);
		if (aWidth <= aBonusWidth)
			aWidth = aBonusWidth;
	}
	if (0.0f < aWidth)
		aWidth = (float)(aWidth * 0.5f + 0.0125f);
	float aShift = 0.0f;
	if (0 < aScore)
	{
		double aRoom = 0.995f - ((double)aWidth + aX + 0.0024999999f);
		if (0.0f < aRoom)
			aShift = 0.0f;
		else
			aShift = (float)aRoom;
		FloatText* aFloat = new FloatText();
		aFloat->mColor1 = 0xffffff8f;
		aFloat->mEndColor1 = 0xffffff8f;
		aFloat->mFlags = 0x89;
		aFloat->mColor2 = 0xff000001;
		aFloat->mEndColor2 = 0xff000001;
		aFloat->mText = aScoreText;
		aFloat->mScale1 = 1.0f;
		aFloat->mScale2 = 1.0f;
		aFloat->mStartX = (float)((double)aShift + aX + 0.0024999999f);
		aFloat->mSpeedX = 0.0125f;
		aFloat->mSpeedY = -0.075f;
		aFloat->mStartY = (float)(aY + 0.0016666667f);
		int aDuration = aBonus < 1 ? 800 : 1000;
		aFloat->mDuration = aDuration;
		aFloat->mFadeTime = (int)(aDuration * 0.7f);
		aFloat->mFont = gBonnies->mFonts[9];
		gFloatTexts->add(aFloat);
	}
	if (0 < aBonus)
	{
		FloatText* aFloat = new FloatText();
		aFloat->mColor1 = 0xff7fff7f;
		aFloat->mEndColor1 = 0xff7fff7f;
		aFloat->mFlags = 0x89;
		aFloat->mColor2 = 0xff000001;
		aFloat->mEndColor2 = 0xff000001;
		aFloat->mText = aBonusText;
		if (mFieldC5C == 0)
		{
			double aTime = (double)(aBonus * 4) /
				((double)currentBook()->mBonusBase * exeFloat(0x457be8 + mDifficulty * 4));
			if (!(aTime + mFieldC94 <= 1.0f))
				aTime = 1.0f - (double)mFieldC94;
			mFieldC94 = (float)(aTime + mFieldC94);
			mFieldC8C = (float)(mFieldC8C - aTime);
		}
		else
		{
			mFieldC88 = mFieldC88 - aBonus;
			mGoal = mGoal + aBonus;
		}
		aFloat->mScale1 = 1.0f;
		aFloat->mScale2 = 1.0f;
		aFloat->mStartX = (float)((double)aShift + aX + 0.0024999999f);
		aFloat->mSpeedX = 0.0125f;
		aFloat->mSpeedY = -0.075f;
		aFloat->mDuration = 1000;
		aFloat->mFadeTime = 699;
		aFloat->mStartY = (float)(aY + 0.043333333f);
		aFloat->mFont = gBonnies->mFonts[9];
		gFloatTexts->add(aFloat);
	}
	mFieldD8C = ticks();
	mPathLength = 0;
	mFieldCDC = ticks();
	mWordChecked = 0;
	mHintFound = 0;
	// The stones count up a letter and the copycats copy the word's last
	int aColumn = 0;
	int aCell = 0;
	do
	{
		int aRow = 0;
		do
		{
			mFrom[aCell + aRow][0] = aColumn;
			mFrom[aCell + aRow][1] = aRow;
			if ((mKinds[aCell + aRow] & 1) != 0)
			{
				mField2F8[aCell + aRow] = (int)ticks();
				char aLetter = mLetters[aCell + aRow];
				mField43C[aCell + aRow] = aLetter;
				if (aLetter == 'Z')
					mLetters[aCell + aRow] = 'A';
				else
				{
					aLetter = (char)(aLetter + 1);
					if (aLetter < 'A' || 'Z' < aLetter)
						aLetter = 'A';
					mLetters[aCell + aRow] = aLetter;
				}
			}
			if ((mKinds[aCell + aRow] & 2) != 0)
			{
				mFrom[aCell + aRow][0] = aLastColumn;
				mFrom[aCell + aRow][1] = aLastRow;
				mFieldC7C = ticks();
				mField43C[aCell + aRow] = mLetters[aCell + aRow];
				mLetters[aCell + aRow] = mCopyLetter;
			}
			aRow++;
		} while (aRow < 9);
		aColumn++;
		aCell += 9;
	} while (aCell < 0x51);
	gGameScreen->updateHint();
	if (currentBook()->mFillCount <= countFlag20())
		levelWon();
	int aSound;
	if (aLetters < 4)
		aSound = 3;
	else
	{
		aSound = aLetters;
		if (5 < aLetters)
			aSound = 6;
	}
	Str aName = lit(0x457c90);
	aName += intToStr(aSound);
	gSound->play(aName, 1.0f, 1, 0, 1.0f);
}

} // namespace bonnies

namespace bonnies
{

// Part of 0x40a9d0 (submit): the level is won. port: a function of its own, which the test command "win" calls too
void Game::levelWon()
{
	gGameScreen->findWidget(0xc9)->mFlags |= 8;
	gGameScreen->findWidget(0xca)->mFlags |= 8;
	gGameScreen->findWidget(0xcb)->mFlags |= 8;
	gSound->play(lit(0x457c98), 1.0f, 1, 0, 1.0f);
	gGameScreen->flash(300, 0x4f3cff00);
	sparkleBoard();
	gGameScreen->mField58 = 2;
	gGameScreen->mField5C = ticks();
	gGameScreen->mField60 = ticks() + 0x44c;
}

// 0x40e540
Record::Record()
{
	reset();
}

// 0x40e260
void Record::reset()
{
	mGames = 0;
	mBestLevel = 0;
	mLastLevel = 0;
	mField0C = 0;
	mField10 = 0;
	mStr14 = "";
	mField1C = 0;
	mField20 = 0;
	mField24 = 0;
	mFieldAC = 0;
	mFieldB0 = 0;
	for (int i = 0; i < 33; i++)
		mSeen[i] = 0;
}

// 0x40e310: an old record (before version 0x65) starts with mGames
void Record::load(int theFile)
{
	char aSkip[52];
	int aVersion;
	fileReadInt(theFile, &aVersion);
	if (aVersion < 0x65)
		mGames = aVersion;
	else
		fileReadInt(theFile, &mGames);
	fileReadInt(theFile, &mBestLevel);
	fileReadInt(theFile, &mField0C);
	fileReadInt(theFile, &mField10);
	fileReadString(theFile, mStr14);
	fileReadInt(theFile, &mField1C);
	fileReadInt(theFile, &mField20);
	if (100 < aVersion)
	{
		fileReadInt(theFile, &mField24);
		fileReadInt(theFile, &mFieldAC);
		fileReadInt(theFile, &mFieldB0);
		if (aVersion < 0x66)
		{
			mLastLevel = mBestLevel;
			fileRead(theFile, aSkip, 4);
		}
		else
			fileReadInt(theFile, &mLastLevel);
		fileRead(theFile, aSkip, 0x26);
	}
	for (int i = 0; i < 0x21; i++)
		fileReadInt(theFile, &mSeen[i]);
	if (100 < aVersion)
		fileRead(theFile, aSkip, 0x2f);
}

// 0x40e440
void Record::save(int theFile)
{
	char aZeros[52] = {};
	int aVersion = 0x66;
	fileWriteInt(theFile, &aVersion);
	fileWriteInt(theFile, &mGames);
	fileWriteInt(theFile, &mBestLevel);
	fileWriteInt(theFile, &mField0C);
	fileWriteInt(theFile, &mField10);
	fileWriteString(theFile, &mStr14);
	fileWriteInt(theFile, &mField1C);
	fileWriteInt(theFile, &mField20);
	fileWriteInt(theFile, &mField24);
	fileWriteInt(theFile, &mFieldAC);
	fileWriteInt(theFile, &mFieldB0);
	fileWriteInt(theFile, &mLastLevel);
	crtWrite(theFile, aZeros, 0x26);
	for (int i = 0; i < 0x21; i++)
		fileWriteInt(theFile, &mSeen[i]);
	crtWrite(theFile, aZeros, 0x2f);
}

} // namespace bonnies
