#include "Board.h"
#include "WinBMApp.h"
#include "MrMoney.h"
#include "Points.h"
#include "HighScoreWidget.h"
#include "Compat12.h"

#include "WidgetManager.h"
#include "DialogButton.h"
#include "MemoryImage.h"
#include "Image.h"
#include "ImageFont.h"
#include "SoundManager.h"
#include "SoundInstance.h"
#include "MusicInterface.h"
#include "Buffer.h"
#include "Common.h"

#include <cmath>
#include <cstdlib>
#include <cstdint>

using namespace Sexy;

// Board.cpp, the game rules half. Spec: re/bigmoney/analysis/board_logic.md (section numbers below).
// Every `mRand.Next()` is the board's own MTRand (the original's Board+0x88, saved in savegame.dat);
// every CrtRand() stands for the original's MSVC rand() (0x4d9372). Board never calls Sexy::Rand().

std::vector<std::string> Sexy::gRankNames;

// The SexyAppFramework 1.2 MTRand seeding (0x45ae40): Knuth's 69069 generator, seed 0 -> 4357, the
// next draw starts a new block (mti = 624). The port's MTRand(seed) / SRand(seed) are 1.3's
// (1812433253 initialiser) and would give another stream.
static void SeedMTRand12(MTRand& theRand, unsigned long theSeed)
{
	if (theSeed == 0)
		theSeed = 4357;

	uint32_t aState[MTRAND_N];
	aState[0] = (uint32_t) theSeed;
	for (int i = 1; i < MTRAND_N; i++)
		aState[i] = aState[i - 1] * 69069u;
	theRand.SetState(aState, MTRAND_N);
}

// Music cues (section 8): main / pause / level-up song of each game type
static void FadeToPauseMusic(WinBMApp* theApp)
{
	MusicInterface* aMusic = theApp->mMusicInterface;
	switch (theApp->mGameType)
	{
	case GAMETYPE_ACTION:
		aMusic->FadeOut(SONG_ACTION, false, 0.008);
		aMusic->FadeIn(SONG_ACTION_PAUSE, ORDER_ACTION_PAUSE, 0.008, false);
		break;
	case GAMETYPE_STRATEGY:
		aMusic->FadeOut(SONG_STRATEGY, false, 0.008);
		aMusic->FadeIn(SONG_STRATEGY_PAUSE, ORDER_STRATEGY_PAUSE, 0.008, false);
		break;
	case GAMETYPE_PUZZLE:
		aMusic->FadeOut(SONG_PUZZLE, false, 0.008);
		aMusic->FadeIn(SONG_PUZZLE_PAUSE, ORDER_PUZZLE_PAUSE, 0.008, false);
		break;
	}
}

static void FadeFromPauseMusic(WinBMApp* theApp)
{
	MusicInterface* aMusic = theApp->mMusicInterface;
	switch (theApp->mGameType)
	{
	case GAMETYPE_ACTION:
		aMusic->FadeOut(SONG_ACTION_PAUSE, true, 0.008);
		aMusic->FadeIn(SONG_ACTION, -1, 0.008, false);
		break;
	case GAMETYPE_STRATEGY:
		aMusic->FadeOut(SONG_STRATEGY_PAUSE, true, 0.008);
		aMusic->FadeIn(SONG_STRATEGY, -1, 0.008, false);
		break;
	case GAMETYPE_PUZZLE:
		aMusic->FadeOut(SONG_PUZZLE_PAUSE, true, 0.008);
		aMusic->FadeIn(SONG_PUZZLE, -1, 0.008, false);
		break;
	}
}

static Particle MakeParticle(int theFrameDelay, double theX, double theY, double theVX, double theVY, double theGravity, bool bright)
{
	Particle aParticle;
	aParticle.mFrameDelay = theFrameDelay;
	aParticle.mX = theX;
	aParticle.mY = theY;
	aParticle.mVX = theVX;
	aParticle.mVY = theVY;
	aParticle.mGravity = theGravity;
	aParticle.mFrame = 0;
	aParticle.mBright = bright;
	return aParticle;
}

// savegame.dat helpers: raw little-endian fields, packed
template <typename T> static void WriteRaw(Buffer* theBuffer, const T& theValue)
{
	theBuffer->WriteBytes((const uchar*) &theValue, (int) sizeof(T));
}

template <typename T> static void ReadRaw(Buffer* theBuffer, T* theValue)
{
	theBuffer->ReadBytes((uchar*) theValue, (int) sizeof(T));
}

static void WriteBool(Buffer* theBuffer, bool theValue)
{
	uchar aByte = theValue ? 1 : 0;
	theBuffer->WriteBytes(&aByte, 1);
}

static bool ReadBool(Buffer* theBuffer)
{
	uchar aByte = 0;
	theBuffer->ReadBytes(&aByte, 1);
	return aByte != 0;
}

// 0x4012b0: the 108-byte record
static void WritePiece(Buffer* theBuffer, const Piece* thePiece)
{
	WriteRaw(theBuffer, thePiece->mX);
	WriteRaw(theBuffer, thePiece->mY);
	WriteRaw(theBuffer, thePiece->mVX);
	WriteRaw(theBuffer, thePiece->mVY);
	WriteRaw(theBuffer, thePiece->mShakeX);
	WriteRaw(theBuffer, thePiece->mShakeY);
	WriteRaw(theBuffer, thePiece->mCol);
	WriteRaw(theBuffer, thePiece->mRow);
	WriteRaw(theBuffer, thePiece->mColor);
	WriteBool(theBuffer, thePiece->mIsSpecial);
	WriteBool(theBuffer, thePiece->mFallThrough);
	WriteRaw(theBuffer, thePiece->mSpecialType);
	WriteRaw(theBuffer, thePiece->mImageSet);
	WriteRaw(theBuffer, thePiece->mFrame);
	WriteBool(theBuffer, thePiece->mFlying);
	WriteRaw(theBuffer, thePiece->mFlyT);
	WriteRaw(theBuffer, thePiece->mFlySpeed);
	WriteRaw(theBuffer, thePiece->mStartX);
	WriteRaw(theBuffer, thePiece->mStartY);
	WriteRaw(theBuffer, thePiece->mTargetX);
	WriteRaw(theBuffer, thePiece->mTargetY);
	WriteBool(theBuffer, thePiece->mDying);
	WriteRaw(theBuffer, thePiece->mLifeTimer);
	WriteRaw(theBuffer, thePiece->mUnusedA0);
}

// 0x401120
static void ReadPiece(Buffer* theBuffer, Piece* thePiece)
{
	ReadRaw(theBuffer, &thePiece->mX);
	ReadRaw(theBuffer, &thePiece->mY);
	ReadRaw(theBuffer, &thePiece->mVX);
	ReadRaw(theBuffer, &thePiece->mVY);
	ReadRaw(theBuffer, &thePiece->mShakeX);
	ReadRaw(theBuffer, &thePiece->mShakeY);
	ReadRaw(theBuffer, &thePiece->mCol);
	ReadRaw(theBuffer, &thePiece->mRow);
	ReadRaw(theBuffer, &thePiece->mColor);
	thePiece->mIsSpecial = ReadBool(theBuffer);
	thePiece->mFallThrough = ReadBool(theBuffer);
	ReadRaw(theBuffer, &thePiece->mSpecialType);
	ReadRaw(theBuffer, &thePiece->mImageSet);
	ReadRaw(theBuffer, &thePiece->mFrame);
	thePiece->mFlying = ReadBool(theBuffer);
	ReadRaw(theBuffer, &thePiece->mFlyT);
	ReadRaw(theBuffer, &thePiece->mFlySpeed);
	ReadRaw(theBuffer, &thePiece->mStartX);
	ReadRaw(theBuffer, &thePiece->mStartY);
	ReadRaw(theBuffer, &thePiece->mTargetX);
	ReadRaw(theBuffer, &thePiece->mTargetY);
	thePiece->mDying = ReadBool(theBuffer);
	ReadRaw(theBuffer, &thePiece->mLifeTimer);
	ReadRaw(theBuffer, &thePiece->mUnusedA0);
}

static void WriteColor(Buffer* theBuffer, const Color& theColor)
{
	for (int i = 0; i < 4; i++)
		WriteRaw(theBuffer, theColor[i]);
}

static void ReadColor(Buffer* theBuffer, Color* theColor)
{
	for (int i = 0; i < 4; i++)
	{
		int aValue = 0;
		ReadRaw(theBuffer, &aValue);
		(*theColor)[i] = aValue;
	}
}

// port: the original reads and writes "savegame.dat" in the game folder. The framework maps
// "userdata\\" to the user dir; without the prefix a read falls back to the (read-only) game folder,
// whose copy DeleteSaveGame could never erase, so the "continue?" prompt would return every run.
std::string Board::GetSaveGameFileName()
{
	return "userdata\\savegame.dat";
}

// 0x4010f0
bool Sexy::SaveGameExists()
{
	return gSexyAppBase->FileExists(Board::GetSaveGameFileName());
}

// 0x401440
void Sexy::DeleteSaveGame()
{
	gSexyAppBase->EraseFile(Board::GetSaveGameFileName());
}

// 0x4010d0
bool Board::CanSaveGame()
{
	return mPlayStarted && (mState != STATE_GAME_OVER_SCORES);
}

// 0x401460
void Board::OrderInManagerChanged()
{
	if (mHighScoreWidget != NULL)
		mWidgetManager->PutInfront((Widget*) mHighScoreWidget, this);
	if (mContinueButton != NULL)
		mWidgetManager->PutInfront((Widget*) mContinueButton, this);
}

// 0x401490: level parameters, L = mLevel + 0/4/7 for skill 1/2/3 (section 1)
void Board::SetupLevelParams()
{
	int aLevel = mLevel;
	int aSkill = mApp->mSkillLevel;
	if (aSkill != 1)
		aLevel += (aSkill == 2) ? 4 : 7;

	double aL = (double) aLevel;
	int aBags;
	if (mApp->mGameType == GAMETYPE_PUZZLE)
	{
		mNumColors = 5;
		aBags = (int)(aL * mPuzzleBagsPerLevel + mPuzzleBagsBase);
	}
	else
	{
		mNumColors = 4;
		aBags = (int)(aL * mBagsPerLevel + mBagsBase);
	}

	if (aBags >= 8)
		aBags = 8;
	else if (aBags <= 1)
		aBags = 1;
	mBagsNeeded = aBags;

	if (aLevel < 1)
		mNumColors--;

	mMintSpeed = aL * mMintSpeedPerLevel + mMintSpeedBase;
	mMintAccel = aL * mMintAccelPerLevel + mMintAccelBase;
	mMeterDrainRate = aL * mDrainPerLevel + mDrainBase;
	mMeterCapacity = aL * mMeterCapPerLevel + mMeterCapBase;
	double aMult = aL * mPointsMultPerLevel + mPointsMultBase;
	mPointsMult = aMult;
	mMintPerClick = aL * mPerClickPerLevel + mPerClickBase;
	if (mApp->mGameType == GAMETYPE_PUZZLE)
		mPointsMult = aMult * 3.0;
}

// 0x4015e0
int Board::ColX(int theCol)
{
	return theCol * COIN_SIZE + BOARD_X;
}

// 0x4015f0
int Board::RowY(int theRow)
{
	return theRow * COIN_SIZE + BOARD_Y;
}

// 0x401600
void Board::ResetMarks()
{
	for (int aRow = 0; aRow < mRows; aRow++)
	{
		for (int aCol = 0; aCol < mCols; aCol++)
		{
			if (mGrid[aRow][aCol] != NULL)
				mGrid[aRow][aCol]->mMark = 0;
		}
	}
}

// 0x401660: flags the pieces of the columns whose top reaches row 0 or 1, and sets the per-column
// warning arrow frame
void Board::SetDangerFlash(bool on)
{
	for (int aCol = 0; aCol < mCols; aCol++)
	{
		int aTop = mRows;
		bool aFlag = on;
		for (int aRow = 0; aRow < mRows; aRow++)
		{
			Piece* aPiece = mGrid[aRow][aCol];
			if (aPiece == NULL)
			{
				if (aRow >= 1)
					aFlag = false;
			}
			else
			{
				if (aRow < aTop)
					aTop = aRow;
				aPiece->mDanger = aFlag;
			}
		}

		if (aTop == 0)
			mColWarning[aCol] = on ? 2 : 3;
		else if (aTop == 1)
			mColWarning[aCol] = on ? 0 : 1;
		else
			mColWarning[aCol] = -1;
	}
}

// 0x401720: marks and counts the coins connected to (theCol, theRow); the start cell's own mark is
// not checked. theSum (optional) accumulates the pixel positions as truncated ints.
void Board::FloodFill(int theCol, int theRow, int theMark, bool sameColor, int* theCount, int* theSum)
{
	static const int aDirs[4][2] = { {-1, 0}, {1, 0}, {0, -1}, {0, 1} };

	Piece* aPiece = mGrid[theRow][theCol];
	if (aPiece->mIsSpecial)
		return;

	if (theSum != NULL)
	{
		theSum[0] = (int)((double) theSum[0] + aPiece->mX);
		theSum[1] = (int)((double) theSum[1] + aPiece->mY);
	}
	(*theCount)++;
	aPiece->mMark = theMark;

	for (int i = 0; i < 4; i++)
	{
		int aCol = theCol + aDirs[i][0];
		int aRow = theRow + aDirs[i][1];
		if ((aCol < 0) || (aCol >= mCols) || (aRow < 0) || (aRow >= mRows))
			continue;
		Piece* aNext = mGrid[aRow][aCol];
		if ((aNext == NULL) || (aNext->mIsSpecial))
			continue;
		if ((aPiece->mColor != aNext->mColor) && sameColor)
			continue;
		if (aNext->mMark != theMark)
			FloodFill(aCol, aRow, theMark, sameColor, theCount, theSum);
	}
}

// 0x401840: group sizes and mHasMoves. The marks are left equal to the group sizes (kept: a later
// flood marked with mUpdateCnt can stop at them, section 4).
void Board::FindGroups()
{
	ResetMarks();
	mHasMoves = false;
	for (int aRow = 0; aRow < mRows; aRow++)
	{
		for (int aCol = 0; aCol < mCols; aCol++)
		{
			Piece* aPiece = mGrid[aRow][aCol];
			if ((aPiece != NULL) && (aPiece->mMark == 0))
			{
				int aCount = 0;
				FloodFill(aCol, aRow, -1, true, &aCount, NULL);
				int aSize = 0;
				FloodFill(aCol, aRow, aCount, true, &aSize, NULL);
				if (mMinGroup <= aSize)
					mHasMoves = true;
			}
		}
	}

	for (int aRow = 0; aRow < mRows; aRow++)
	{
		for (int aCol = 0; aCol < mCols; aCol++)
		{
			Piece* aPiece = mGrid[aRow][aCol];
			if (aPiece != NULL)
				aPiece->mGroupSize = aPiece->mMark;
		}
	}
}

// 0x401960
Piece* Board::PieceAt(int x, int y)
{
	for (int aRow = 0; aRow < mRows; aRow++)
	{
		for (int aCol = 0; aCol < mCols; aCol++)
		{
			Piece* aPiece = mGrid[aRow][aCol];
			if ((aPiece != NULL) && (aPiece->mX <= (double) x) && (aPiece->mY <= (double) y) &&
				((double) x < aPiece->mX + 29.0) && ((double) y < aPiece->mY + 29.0))
				return aPiece;
		}
	}
	return NULL;
}

// 0x401a20: instructor hint at a fixed point, only at skill 1
void Board::ShowHintAt(int theId, const Point& thePos, int theDuration)
{
	if (mApp->mSkillLevel != 1)
		return;

	mHintVisible = true;
	mHintAlt = false;
	mHintX = thePos.mX;
	mHintY = thePos.mY;
	mHintId = theId;
	mHintTimer = theDuration;
	mHintPiece = NULL;
	mHintFollowMeter = false;
}

// 0x401a80
void Board::HideHint()
{
	mHintVisible = false;
	MarkDirty();
}

// 0x401a90
int Board::CountCoinColumns()
{
	int aCount = 0;
	for (int aCol = 0; aCol < mCols; aCol++)
	{
		for (int aRow = 0; aRow < mRows; aRow++)
		{
			Piece* aPiece = mGrid[aRow][aCol];
			if ((aPiece != NULL) && (!aPiece->mIsSpecial))
			{
				aCount++;
				break;
			}
		}
	}
	return aCount;
}

// 0x401af0
int Board::GetTopCoinRow()
{
	for (int aRow = 0; aRow < mRows; aRow++)
	{
		for (int aCol = 0; aCol < mCols; aCol++)
		{
			Piece* aPiece = mGrid[aRow][aCol];
			if ((aPiece != NULL) && (!aPiece->mIsSpecial))
				return aRow;
		}
	}
	return mRows;
}

// 0x401b40
double Board::GetAverageCoinRow()
{
	int aSum = 0;
	int aCount = 0;
	for (int aRow = 0; aRow < mRows; aRow++)
	{
		for (int aCol = 0; aCol < mCols; aCol++)
		{
			Piece* aPiece = mGrid[aRow][aCol];
			if ((aPiece != NULL) && (!aPiece->mIsSpecial))
			{
				aSum += aRow;
				aCount++;
			}
		}
	}

	if (aCount != 0)
		return (double) aSum / (double) aCount;
	return (double) mRows;
}

// 0x401bd0: Strategy clicks left before the next row enters
void Board::UpdateMovesLeft()
{
	if (mApp->mGameType != GAMETYPE_STRATEGY)
		return;

	double aMint = mMintTarget;
	int aRows = GetTopCoinRow();
	while (aMint >= 1.0)
	{
		aMint -= 0.75;
		aRows--;
	}
	mMovesLeft = (int) ceil(((double) aRows * 0.75 + (1.0 - aMint)) / mMintPerClick);
}

// 0x401c70 ButtonListener slot 1 (shared with HighScoreWidget)
void Board::ButtonPress(int theId)
{
	mApp->PlaySample(SOUND_BUTTONCLICK);
}

// 0x401d30
void Board::GameOver()
{
	MusicInterface* aMusic = mApp->mMusicInterface;
	aMusic->FadeOut(SONG_ACTION, true, 0.02);
	aMusic->FadeOut(SONG_STRATEGY, true, 0.02);
	aMusic->FadeOut(SONG_PUZZLE, true, 0.02);
	aMusic->PlayMusic(SONG_GAMEOVER, ORDER_GAMEOVER, false);
	mApp->PlaySample(SOUND_GAMEOVERTONE);
	SetDangerFlash(false);
	mCountdownActive = false;
	mMeterFraction = 0.0;
	mStatsFade = 0.0;
	mState = STATE_GAME_OVER;
	mGameOverRiseDelay = 100;
	mStatsDirty = true;
	mFlingDelay = (mApp->mGameType == GAMETYPE_PUZZLE) ? 200 : 1;
	HideHint();
}

// 0x401e90: highlights the hovered group (when it is clickable)
void Board::UpdateHover()
{
	Piece* aPiece = PieceAt(mMouseX, mMouseY);
	bool aHoverBag = (aPiece != NULL) && (aPiece->mIsSpecial) && (aPiece->mSpecialType == PIECE_MONEYBAG);
	if (aHoverBag != mHoverBag)
	{
		mHoverBag = aHoverBag;
		MarkDirty();
	}

	int aCount = 0;
	if ((aPiece != NULL) && (!aPiece->mIsSpecial) && (mState == STATE_PLAYING))
		FloodFill(aPiece->mCol, aPiece->mRow, mUpdateCnt, true, &aCount, NULL);

	for (int aRow = 0; aRow < mRows; aRow++)
	{
		for (int aCol = 0; aCol < mCols; aCol++)
		{
			Piece* aCell = mGrid[aRow][aCol];
			if (aCell == NULL)
				continue;
			bool aHighlighted = (aCount >= mMinGroup) && (aCell->mMark == mUpdateCnt);
			if (aCell->mHighlighted != aHighlighted)
			{
				aCell->mHighlighted = aHighlighted;
				MarkDirty();
			}
			aCell->mMark = 0;
		}
	}
}

// 0x401fa0 slot 35
void Board::MouseLeave()
{
	Widget::MouseLeave();
	UpdateHover();
}

// 0x401fb0 slot 36
void Board::MouseMove(int x, int y)
{
	Widget::MouseMove(x, y);
	mMouseX = x;
	mMouseY = y;
	UpdateHover();
}

// 0x401fe0: instructor hint on a piece, only when the help screen was shown before this game
void Board::ShowHint(int theId, Piece* thePiece, int theDuration)
{
	if (!mApp->mShowInstructor)
		return;

	mHintVisible = true;
	mHintAlt = false;
	mHintX = (int)(thePiece->mX + 14.0);
	mHintY = (int)(thePiece->mY + 14.0);
	mHintPiece = thePiece;
	mHintId = theId;
	mHintTimer = theDuration;
	mHintFollowMeter = false;
}

// 0x402930: nested pause, only while playing (section 6.7)
void Board::Pause(bool pause)
{
	if (mState != STATE_PLAYING)
		return;

	int anOldCount = mPauseCount;
	mPauseCount = pause ? anOldCount + 1 : anOldCount - 1;
	if (mPauseCount < 0)
		mPauseCount = 0;

	for (std::list<Points*>::iterator anItr = mPointsWidgets.begin(); anItr != mPointsWidgets.end(); ++anItr)
		(*anItr)->SetVisible(mPauseCount == 0);

	if (mPauseCount < 1)
	{
		if (anOldCount >= 1)
		{
			mTimerTick = GetTickCount();
			FadeFromPauseMusic(mApp);
		}
	}
	else if ((anOldCount <= 0) && (mApp->mHasFocus))
	{
		mPlaySeconds = (int)((unsigned long) mPlaySeconds + ((unsigned long) GetTickCount() - mTimerTick) / 1000);
		FadeToPauseMusic(mApp);
	}

	MarkDirty();
}

// 0x402b70: gravity per column, then closing empty bottom columns (section 6.1)
void Board::Compact()
{
	int aColMap[BOARD_COLS];
	for (int aCol = 0; aCol < mCols; aCol++)
		aColMap[aCol] = aCol;

	bool isPuzzle = (mApp->mGameType == GAMETYPE_PUZZLE);
	for (int aCol = 0; aCol < mCols; aCol++)
	{
		int aDest = mRows - 1;
		bool aGapBelow = false;
		for (int aSrc = mRows - 1; aSrc >= 0; aSrc--)
		{
			Piece* aPiece = mGrid[aSrc][aCol];
			if (aPiece == NULL)
			{
				aGapBelow = true;
				continue;
			}

			if (aSrc != aDest)
			{
				mGrid[aDest][aCol] = aPiece;
				aPiece->mRow = aDest;
				// A special leaves the board when the coin directly under it was removed (Puzzle: when it
				// reaches the bottom row)
				if (isPuzzle)
					aPiece->mFallThrough = (aPiece->mIsSpecial) && (aDest == mRows - 1);
				else
					aPiece->mFallThrough |= (aPiece->mIsSpecial) && (aGapBelow);
				if ((aPiece->mFallThrough) && (aPiece->mSpecialType == PIECE_MONEYBAG))
					mRepaintTimer = 0;
			}
			aDest--;
			aGapBelow = false;
		}

		for (int aRow = aDest; aRow >= 0; aRow--)
			mGrid[aRow][aCol] = NULL;
	}

	bool aShifted = false;
	int aBottom = mRows - 1;
	for (int aCol = 0; aCol < mCols; aCol++)
	{
		if (mGrid[aBottom][aCol] != NULL)
			continue;

		int aMoveRight = 0;
		int aMoveLeft = 0;

		int aLeft;
		int aLeftSize = 0;
		for (aLeft = aCol; aLeft >= 0; aLeft--)
		{
			if (mGrid[aBottom][aLeft] != NULL)
			{
				int aCount = 0;
				FloodFill(aLeft, aBottom, mUpdateCnt, false, &aCount, NULL);
				aLeftSize = aCount;
				break;
			}
		}

		int aRightSize = 0;
		int aRight = aCol;
		while ((aRight < mCols) && (mGrid[aBottom][aRight] == NULL))
			aRight++;
		if (aRight < mCols)
			FloodFill(aRight, aBottom, mUpdateCnt, false, &aRightSize, NULL);

		if ((aLeft >= 0) && (aRight < mCols))
		{
			int aGap = aRight - aLeft - 1;
			aMoveRight = aGap / 2;
			aMoveLeft = aGap / 2;
			if (aGap % 2 != 0)
			{
				// The smaller blob moves the extra step
				if (aLeftSize < aRightSize)
					aMoveRight++;
				else
					aMoveLeft++;
			}
		}

		ResetMarks();

		int aPos = aCol;
		for (int aStep = 0; aStep < aMoveRight; aStep++)
		{
			aShifted = true;
			// Everything left of aPos moves one column right
			for (int k = aPos; k >= 1; k--)
			{
				aColMap[k] = aColMap[k - 1];
				aColMap[k - 1] = -1;
				mColTowerHeight[k] = mColTowerHeight[k - 1];
				mColTowerHeight[k - 1] = 0.0;
				for (int aRow = 0; aRow < mRows; aRow++)
				{
					Piece* aPiece = mGrid[aRow][k - 1];
					mGrid[aRow][k - 1] = NULL;
					mGrid[aRow][k] = aPiece;
					if (aPiece != NULL)
						aPiece->mCol++;
				}
			}
			aPos++;
		}

		for (int aStep = 0; aStep < aMoveLeft; aStep++)
		{
			aShifted = true;
			// Everything right of aPos moves one column left
			for (int k = aPos; k < mCols - 1; k++)
			{
				aColMap[k] = aColMap[k + 1];
				aColMap[k + 1] = -1;
				mColTowerHeight[k] = mColTowerHeight[k + 1];
				mColTowerHeight[k + 1] = 0.0;
				for (int aRow = 0; aRow < mRows; aRow++)
				{
					Piece* aPiece = mGrid[aRow][k + 1];
					mGrid[aRow][k + 1] = NULL;
					mGrid[aRow][k] = aPiece;
					if (aPiece != NULL)
						aPiece->mCol--;
				}
			}
		}
	}

	if (aShifted)
	{
		// Loose pieces (falling-through bags...) follow their column
		int anInverse[BOARD_COLS];
		for (int i = 0; i < mCols; i++)
			anInverse[i] = -1;
		for (int i = 0; i < mCols; i++)
		{
			if (aColMap[i] != -1)
				anInverse[aColMap[i]] = i;
		}
		for (std::list<Piece*>::iterator anItr = mLoosePieces.begin(); anItr != mLoosePieces.end(); ++anItr)
		{
			Piece* aPiece = *anItr;
			if (anInverse[aPiece->mCol] != -1)
				aPiece->mCol = anInverse[aPiece->mCol];
		}
	}

	FindGroups();
}

// 0x403050 slot 31
void Board::KeyChar(SexyChar theChar)
{
	if (theChar == ' ')
		Pause(mPauseCount == 0);
}

// 0x403300: savegame.dat. Called by ~WinBMApp when CanSaveGame(). The pause / unpause banks the game
// seconds (which are not saved) and changes nothing that is.
bool Board::SaveGame()
{
	Pause(true);
	Pause(false);

	Buffer aBuffer;
	WriteSaveGame(&aBuffer);
	return mApp->WriteBufferToFile(GetSaveGameFileName(), &aBuffer);
}

// port (web persistence, WinBMApp::GetShutdownSave): the bytes SaveGame would write now. Its
// Pause(true) / Pause(false) affects only unsaved values (and the music), so it is skipped here.
void Board::SaveGameSnapshot(Buffer* theBuffer)
{
	WriteSaveGame(theBuffer);
}

// The body of 0x403300 (section 9)
void Board::WriteSaveGame(Buffer* theBuffer)
{
	int aVersion = SAVEGAME_VERSION;
	WriteRaw(theBuffer, aVersion);
	WriteRaw(theBuffer, mApp->mGameType);
	WriteRaw(theBuffer, mApp->mSkillLevel);
	WriteRaw(theBuffer, mState);
	std::string aRandState = mRand.Serialize();
	int aRandLen = (int) aRandState.length();
	WriteRaw(theBuffer, aRandLen);
	theBuffer->WriteBytes((const uchar*) aRandState.c_str(), aRandLen);

	for (int aRow = 0; aRow < BOARD_ROWS; aRow++)
	{
		for (int aCol = 0; aCol < BOARD_COLS; aCol++)
		{
			Piece* aPiece = mGrid[aRow][aCol];
			WriteBool(theBuffer, aPiece != NULL);
			if (aPiece != NULL)
				WritePiece(theBuffer, aPiece);
		}
	}
	for (int aCol = 0; aCol < BOARD_COLS; aCol++)
		WritePiece(theBuffer, mNextRow[aCol]);

	WriteRaw(theBuffer, mMinGroup);
	WriteRaw(theBuffer, mRiseOffset);
	WriteBool(theBuffer, mHasMoves);
	WriteRaw(theBuffer, mCols);
	WriteRaw(theBuffer, mRows);
	WriteRaw(theBuffer, mMintPixels);
	WriteRaw(theBuffer, mMint);
	WriteRaw(theBuffer, mMintTarget);
	WriteRaw(theBuffer, mMintSpeed);
	WriteRaw(theBuffer, mMintAccel);
	WriteRaw(theBuffer, mMintPerClick);
	WriteColor(theBuffer, mMovesColor);
	WriteRaw(theBuffer, mScore);
	WriteRaw(theBuffer, mDisplayScore);
	WriteRaw(theBuffer, mPointsMult);
	WriteRaw(theBuffer, mMeterFlash);
	WriteRaw(theBuffer, mMeterJustFull);
	WriteRaw(theBuffer, mMeterFraction);
	WriteRaw(theBuffer, mMeterTarget);
	WriteRaw(theBuffer, mMeterCoins);
	WriteRaw(theBuffer, mMeterDrained);
	WriteRaw(theBuffer, mMeterPending);
	WriteRaw(theBuffer, mMeterLaunchTimer);
	WriteRaw(theBuffer, mMeterEmptying);
	WriteRaw(theBuffer, mMeterCapacity);
	WriteRaw(theBuffer, mMeterDrainRate);

	int aCount = (int) mLoosePieces.size();
	WriteRaw(theBuffer, aCount);
	for (std::list<Piece*>::iterator anItr = mLoosePieces.begin(); anItr != mLoosePieces.end(); ++anItr)
		WritePiece(theBuffer, *anItr);

	aCount = (int) mTubeCoins.size();
	WriteRaw(theBuffer, aCount);
	for (std::list<TubeCoin>::iterator anItr = mTubeCoins.begin(); anItr != mTubeCoins.end(); ++anItr)
	{
		WriteRaw(theBuffer, anItr->mY);
		WriteRaw(theBuffer, anItr->mDist);
		WriteRaw(theBuffer, anItr->mFrame);
	}

	WriteRaw(theBuffer, mBillsToSpawn);
	WriteRaw(theBuffer, mBagsReleased);
	WriteRaw(theBuffer, mBagsBanked);
	WriteRaw(theBuffer, mMeterFlashFrame);
	WriteRaw(theBuffer, mMeterFlashTimer);
	WriteRaw(theBuffer, mBagsNeeded);
	WriteBool(theBuffer, mLevelUpActive);
	WriteRaw(theBuffer, mLevelUpY);
	WriteRaw(theBuffer, mLevelUpAngle);
	WriteRaw(theBuffer, mLevelUpUnused);
	WriteRaw(theBuffer, mLevelUpStretch);
	WriteBool(theBuffer, mLevelUpBouncing);
	WriteRaw(theBuffer, mLevelUpLeftY);
	WriteRaw(theBuffer, mLevelUpLeftVY);
	WriteRaw(theBuffer, mLevelUpRightY);
	WriteRaw(theBuffer, mLevelUpRightVY);
	WriteRaw(theBuffer, mMintDelay);
	WriteBool(theBuffer, mPiecesMoving);
	WriteRaw(theBuffer, mLevel);
	WriteRaw(theBuffer, mLevelEndDelay);
	WriteRaw(theBuffer, mPauseCount);
	WriteRaw(theBuffer, mPauseFade);
	WriteRaw(theBuffer, mPauseAnim);
	WriteBool(theBuffer, mPauseClickable);
	WriteRaw(theBuffer, mGoTimer);
	WriteBool(theBuffer, mBannerVisible);
	WriteRaw(theBuffer, mBannerTimer);
	WriteRaw(theBuffer, mOverflowTicks);
	WriteRaw(theBuffer, mFlingDelay);
	WriteBool(theBuffer, mCountdownActive);
	WriteRaw(theBuffer, mCountdownValue);
	WriteRaw(theBuffer, mGameOverRiseDelay);
	WriteRaw(theBuffer, mLevelCoinsCleared);
	WriteRaw(theBuffer, mGameCoinsCleared);
	WriteRaw(theBuffer, mLevelBiggestBonus);
	WriteRaw(theBuffer, mGameBiggestBonus);
	WriteRaw(theBuffer, mLevelBiggestCombo);
	WriteRaw(theBuffer, mGameBiggestCombo);
	WriteRaw(theBuffer, mBombColor);
	WriteRaw(theBuffer, mBombIdle);
	WriteBool(theBuffer, mMoveDigitActive);
	WriteRaw(theBuffer, mMoveDigit);
	WriteRaw(theBuffer, mMoveDigitFade);
	WriteBool(theBuffer, mContinueButton != NULL);
}

// 0x403a20
void Board::RemovePointsWidgets()
{
	for (std::list<Points*>::iterator anItr = mPointsWidgets.begin(); anItr != mPointsWidgets.end(); ++anItr)
	{
		mWidgetManager->RemoveWidget(*anItr);
		mApp->SafeDeleteWidget(*anItr);
	}
	mPointsWidgets.clear();
}

// 0x403aa0 slot 8
void Board::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Widget::RemovedFromManager(theWidgetManager);

	if (mContinueButton != NULL)
	{
		theWidgetManager->RemoveWidget((Widget*) mContinueButton);
		mApp->SafeDeleteWidget((Widget*) mContinueButton);
		mContinueButton = NULL;
	}

	if (mHighScoreWidget != NULL)
	{
		theWidgetManager->RemoveWidget((Widget*) mHighScoreWidget);
		delete mHighScoreWidget;
		mHighScoreWidget = NULL;
	}

	for (std::list<Points*>::iterator anItr = mPointsWidgets.begin(); anItr != mPointsWidgets.end(); ++anItr)
	{
		theWidgetManager->RemoveWidget(*anItr);
		mApp->SafeDeleteWidget(*anItr);
	}
	mPointsWidgets.clear();
}

// 0x403b80: the GameSelector covers a running game
void Board::Freeze()
{
	RemovePointsWidgets();
	mState = STATE_FROZEN;
}

// 0x403f10
Board::~Board()
{
	if (mExplodeSound != NULL)
		mExplodeSound->Release();
	delete mStatsImage;
	delete mStatsFadeImage;
	delete mLevelUpImage;

	// port: the original leaks every piece
	for (int aRow = 0; aRow < BOARD_ROWS; aRow++)
	{
		for (int aCol = 0; aCol < BOARD_COLS; aCol++)
			delete mGrid[aRow][aCol];
	}
	for (int aCol = 0; aCol < BOARD_COLS; aCol++)
		delete mNextRow[aCol];
	for (std::list<Piece*>::iterator anItr = mLoosePieces.begin(); anItr != mLoosePieces.end(); ++anItr)
		delete *anItr;
}

// 0x4075d0 (section 9). The constructor already made a board for the app's game type and skill; the
// header replaces them. Reading the 624 MT words keeps the constructor's draw index (the original's
// SRand_state does not touch mti): faithful, so a resumed game draws from that index of the saved
// block. mNumColors is not saved and keeps the constructor's (level 0) value.
bool Board::LoadGame()
{
	Buffer aBuffer;
	if (!mApp->ReadBufferFromFile(GetSaveGameFileName(), &aBuffer, false))
		return false;

	int aVersion = -1;
	ReadRaw(&aBuffer, &aVersion);
	if (aVersion != SAVEGAME_VERSION)
		return false;

	ReadRaw(&aBuffer, &mApp->mGameType);
	ReadRaw(&aBuffer, &mApp->mSkillLevel);
	ReadRaw(&aBuffer, &mState);
	unsigned int aRandLen = 0;
	ReadRaw(&aBuffer, &aRandLen);
	std::string aRandState;
	aRandState.resize(aRandLen);
	if (aRandLen > 0)
		aBuffer.ReadBytes((uchar*) &aRandState[0], (int) aRandLen);
	// 1.2's SRand_state: the 624 words (mti kept), or the 4357 seed for any other length
	if (aRandState.length() == MTRAND_N * 4)
		mRand.SRand(aRandState);
	else
		SeedMTRand12(mRand, 4357);

	// port: the original overwrites the constructor's pieces without deleting them
	for (int aRow = 0; aRow < BOARD_ROWS; aRow++)
	{
		for (int aCol = 0; aCol < BOARD_COLS; aCol++)
		{
			Piece* aPiece = NULL;
			if (ReadBool(&aBuffer))
			{
				aPiece = new Piece();
				ReadPiece(&aBuffer, aPiece);
			}
			delete mGrid[aRow][aCol];
			mGrid[aRow][aCol] = aPiece;
		}
	}
	for (int aCol = 0; aCol < BOARD_COLS; aCol++)
	{
		Piece* aPiece = new Piece();
		ReadPiece(&aBuffer, aPiece);
		delete mNextRow[aCol];
		mNextRow[aCol] = aPiece;
	}

	ReadRaw(&aBuffer, &mMinGroup);
	ReadRaw(&aBuffer, &mRiseOffset);
	mHasMoves = ReadBool(&aBuffer);
	ReadRaw(&aBuffer, &mCols);
	ReadRaw(&aBuffer, &mRows);
	ReadRaw(&aBuffer, &mMintPixels);
	ReadRaw(&aBuffer, &mMint);
	ReadRaw(&aBuffer, &mMintTarget);
	ReadRaw(&aBuffer, &mMintSpeed);
	ReadRaw(&aBuffer, &mMintAccel);
	ReadRaw(&aBuffer, &mMintPerClick);
	ReadColor(&aBuffer, &mMovesColor);
	ReadRaw(&aBuffer, &mScore);
	ReadRaw(&aBuffer, &mDisplayScore);
	ReadRaw(&aBuffer, &mPointsMult);
	ReadRaw(&aBuffer, &mMeterFlash);
	ReadRaw(&aBuffer, &mMeterJustFull);
	ReadRaw(&aBuffer, &mMeterFraction);
	ReadRaw(&aBuffer, &mMeterTarget);
	ReadRaw(&aBuffer, &mMeterCoins);
	ReadRaw(&aBuffer, &mMeterDrained);
	ReadRaw(&aBuffer, &mMeterPending);
	ReadRaw(&aBuffer, &mMeterLaunchTimer);
	ReadRaw(&aBuffer, &mMeterEmptying);
	ReadRaw(&aBuffer, &mMeterCapacity);
	ReadRaw(&aBuffer, &mMeterDrainRate);

	// The old loose pieces are only unlinked, as the original (port: also deleted)
	for (std::list<Piece*>::iterator anItr = mLoosePieces.begin(); anItr != mLoosePieces.end(); ++anItr)
		delete *anItr;
	mLoosePieces.clear();
	int aCount = 0;
	ReadRaw(&aBuffer, &aCount);
	for (int i = 0; i < aCount; i++)
	{
		Piece* aPiece = new Piece();
		ReadPiece(&aBuffer, aPiece);
		mLoosePieces.push_back(aPiece);
	}

	mTubeCoins.clear();
	aCount = 0;
	ReadRaw(&aBuffer, &aCount);
	for (int i = 0; i < aCount; i++)
	{
		TubeCoin aCoin;
		ReadRaw(&aBuffer, &aCoin.mY);
		ReadRaw(&aBuffer, &aCoin.mDist);
		ReadRaw(&aBuffer, &aCoin.mFrame);
		mTubeCoins.push_back(aCoin);
	}

	ReadRaw(&aBuffer, &mBillsToSpawn);
	ReadRaw(&aBuffer, &mBagsReleased);
	ReadRaw(&aBuffer, &mBagsBanked);
	ReadRaw(&aBuffer, &mMeterFlashFrame);
	ReadRaw(&aBuffer, &mMeterFlashTimer);
	ReadRaw(&aBuffer, &mBagsNeeded);
	mLevelUpActive = ReadBool(&aBuffer);
	ReadRaw(&aBuffer, &mLevelUpY);
	ReadRaw(&aBuffer, &mLevelUpAngle);
	ReadRaw(&aBuffer, &mLevelUpUnused);
	ReadRaw(&aBuffer, &mLevelUpStretch);
	mLevelUpBouncing = ReadBool(&aBuffer);
	ReadRaw(&aBuffer, &mLevelUpLeftY);
	ReadRaw(&aBuffer, &mLevelUpLeftVY);
	ReadRaw(&aBuffer, &mLevelUpRightY);
	ReadRaw(&aBuffer, &mLevelUpRightVY);
	ReadRaw(&aBuffer, &mMintDelay);
	mPiecesMoving = ReadBool(&aBuffer);
	ReadRaw(&aBuffer, &mLevel);
	ReadRaw(&aBuffer, &mLevelEndDelay);
	ReadRaw(&aBuffer, &mPauseCount);
	ReadRaw(&aBuffer, &mPauseFade);
	ReadRaw(&aBuffer, &mPauseAnim);
	mPauseClickable = ReadBool(&aBuffer);
	ReadRaw(&aBuffer, &mGoTimer);
	mBannerVisible = ReadBool(&aBuffer);
	ReadRaw(&aBuffer, &mBannerTimer);
	ReadRaw(&aBuffer, &mOverflowTicks);
	ReadRaw(&aBuffer, &mFlingDelay);
	mCountdownActive = ReadBool(&aBuffer);
	ReadRaw(&aBuffer, &mCountdownValue);
	ReadRaw(&aBuffer, &mGameOverRiseDelay);
	ReadRaw(&aBuffer, &mLevelCoinsCleared);
	ReadRaw(&aBuffer, &mGameCoinsCleared);
	ReadRaw(&aBuffer, &mLevelBiggestBonus);
	ReadRaw(&aBuffer, &mGameBiggestBonus);
	ReadRaw(&aBuffer, &mLevelBiggestCombo);
	ReadRaw(&aBuffer, &mGameBiggestCombo);
	ReadRaw(&aBuffer, &mBombColor);
	ReadRaw(&aBuffer, &mBombIdle);
	mMoveDigitActive = ReadBool(&aBuffer);
	ReadRaw(&aBuffer, &mMoveDigit);
	ReadRaw(&aBuffer, &mMoveDigitFade);
	if (ReadBool(&aBuffer))
		ShowContinueButton();

	mPlayStarted = true;
	delete mStatsImage;
	mStatsImage = NULL;
	delete mStatsFadeImage;
	mStatsFadeImage = NULL;
	delete mLevelUpImage;
	mLevelUpImage = NULL;
	mHintVisible = false;
	mMintDelay = 200;
	UpdateMovesLeft();
	FindGroups();
	return true;
}

// 0x407ea0: a "+N" popup (centred at x, y), the score, and (not Puzzle) the money meter.
// thePoints is the raw value; the score and the popup are thePoints * mPointsMult.
void Board::AddPoints(int thePoints, int x, int y, bool toMeter)
{
	double aPoints = (double) thePoints;
	int aScore = (int)(aPoints * mPointsMult);
	int aLife = (int)(aPoints * 0.6666666666666666);
	if (aLife > 600)
		aLife = 600;
	else if (aLife < 100)
		aLife = 100;

	SexyString aText = StrFormat("+%d", aScore);
	Points* aPointsWidget = new Points(mApp, mApp->mFloatFont, aText, mX + x, mY + y, aLife, 0);
	mPointsWidgets.push_back(aPointsWidget);
	mWidgetManager->AddWidget(aPointsWidget);

	if (mLevelBiggestBonus < aScore)
		mLevelBiggestBonus = aScore;
	if (mGameBiggestBonus < aScore)
		mGameBiggestBonus = aScore;
	mScore += aScore;

	if ((mApp->mGameType != GAMETYPE_PUZZLE) && (toMeter))
	{
		int anOldUnits = (int) mMeterTarget;
		double aFill = aPoints / mMeterCapacity;
		mMeterTarget = aFill * 64.0 + mMeterTarget;
		int aNewUnits = (int) mMeterTarget;
		mMeterFraction = aFill + mMeterFraction;
		mMeterPending += aNewUnits - anOldUnits;
		mMeterFlash = 16;
		MarkDirty();
	}
}

// 0x408110: a bonus message, stacked under the previous one while that is alive
void Board::ShowBonusText(const SexyString& theText)
{
	int aY = (mBannerTimer > 0) ? 148 : 180;
	if ((mLastBonusText != NULL) && (mLastBonusText->mLife > 0))
	{
		int aBelow = mLastBonusText->mY + 36;
		if (aBelow > aY)
			aY = aBelow;
	}

	Points* aText = new Points(mApp, mApp->mFont1, theText, 212, mY + aY, 200, -1);
	mPointsWidgets.push_back(aText);
	mWidgetManager->AddWidget(aText);
	mLastBonusText = aText;
}

// 0x408220: game over, every grid piece is flung (section 6.6)
void Board::FlingAllCoins()
{
	for (int aRow = 0; aRow < mRows; aRow++)
	{
		for (int aCol = 0; aCol < mCols; aCol++)
		{
			Piece* aPiece = mGrid[aRow][aCol];
			if (aPiece == NULL)
				continue;

			mGrid[aRow][aCol] = NULL;
			aPiece->mVX = (double)(mRand.Next() % 300) * 0.01 - 1.5;
			aPiece->mImageSet = 1;
			aPiece->mSpin = 0.0;
			aPiece->mSpinSpeed = (double)(CrtRand() % 1000) * 4e-05;
			double aHeightFactor = (double) aRow / (double) mRows + 0.1;
			aPiece->mVY = (double)(-(int)(mRand.Next() % 500)) * aHeightFactor * 0.01;
			if (aPiece->mVX > 0.0)
				aPiece->mVX += 0.5;
			else
				aPiece->mVX -= 0.5;
			if (aPiece->mIsSpecial)
			{
				aPiece->mSpecialType = PIECE_POOF;
				aPiece->mFrame = 0;
			}
			mLoosePieces.push_back(aPiece);
		}
	}
}

// 0x4083d0: all needed bags banked
void Board::LevelComplete()
{
	MusicInterface* aMusic = mApp->mMusicInterface;
	switch (mApp->mGameType)
	{
	case GAMETYPE_ACTION:
		aMusic->FadeOut(SONG_ACTION, false, 0.02);
		aMusic->PlayMusic(SONG_ACTION_LEVELUP, ORDER_ACTION_LEVELUP, false);
		break;
	case GAMETYPE_STRATEGY:
		aMusic->FadeOut(SONG_STRATEGY, false, 0.02);
		aMusic->PlayMusic(SONG_STRATEGY_LEVELUP, ORDER_STRATEGY_LEVELUP, false);
		break;
	case GAMETYPE_PUZZLE:
		aMusic->FadeOut(SONG_PUZZLE, false, 0.02);
		aMusic->PlayMusic(SONG_PUZZLE_LEVELUP, ORDER_PUZZLE_LEVELUP, false);
		break;
	}
	mApp->PlaySample(SOUND_LEVELUP);
	mApp->mMrMoney->PlayAnim(MRANIM_LEVELUP);

	// 60 bills
	for (int i = 0; i * 5 < 300; i++)
	{
		Bill aBill;
		aBill.mX = (double)(CrtRand() % 500 + 80);
		int aSpeed = CrtRand() % 1000;
		aBill.mVY = ((double) aSpeed * 0.001 * 0.4 + (double) i * 0.02) + 1.6;
		int aHeight = CrtRand() % 92;
		aBill.mY = (double)(-50 - aHeight);
		aBill.mFrame = CrtRand() % 16;
		aBill.mType = 4 - (i * 5) / 60;
		mBills.push_back(aBill);
	}

	mLevelUpY = -150.0;
	mLevelUpAngle = 0.0;
	mLevelUpUnused = 0.03;
	mLevelUpBouncing = true;
	mLevelUpLeftY = -150.0;
	mLevelUpLeftVY = 0.0;
	mLevelUpRightY = -174.0;
	mLevelUpRightVY = 0.0;
	mLevelUpStretch = 189.0;
	mStatsFade = 0.0;
	mStatsDirty = true;
	mLevelUpActive = true;
	mState = STATE_LEVEL_COMPLETE;
	mLevelEndDelay = 160;
	mMeterFraction = 0.0;
	MarkDirty();
}

// 0x408650: end of state 2, the board falls away; Excess Baggage for the bags left on it
void Board::ClearBoardForLevelEnd()
{
	mMeterFraction = 0.0;
	HideHint();

	int aBags = 0;
	for (int aRow = 0; aRow < mRows; aRow++)
	{
		for (int aCol = 0; aCol < mCols; aCol++)
		{
			Piece* aPiece = mGrid[aRow][aCol];
			if (aPiece == NULL)
				continue;

			aPiece->mRow = mRows + 2;
			aPiece->mVY = (double)(mRand.Next() % 200) * 0.01;
			if ((mApp->mGameType != GAMETYPE_PUZZLE) && (aPiece->mIsSpecial) && (aPiece->mSpecialType == PIECE_MONEYBAG))
			{
				aBags++;
				if (aBags > 1)
					AddPoints((int)(mPointsMult * 500.0), (int)(aPiece->mX + 14.0), (int)(aPiece->mY + 14.0), false);
			}
		}
	}

	if (aBags >= 3)
	{
		ShowBonusText(mApp->GetString("EXCESS_BAGGAGE_BONUS"));
		mApp->mMrMoney->PlayAnim(MRANIM_BONUS);
		mApp->PlaySample(SOUND_BONUS);
	}
	mState = STATE_LEVEL_CLEAR_FALL;
}

// 0x4088a0: a minted row enters from the bottom; the pieces move up one grid row but keep their pixel
// y, and the bottom floor (RowY(14) + mRiseOffset) rises back 1 px per tick, so they rise smoothly
void Board::PushRow()
{
	mApp->PlaySample(SOUND_MOVEUP);
	mMintPixels = 0;

	for (int aCol = 0; aCol < mCols; aCol++)
	{
		Piece* aTop = mGrid[0][aCol];
		if ((aTop != NULL) && (aTop->mIsSpecial))
		{
			// A special pushed off the top disappears
			aTop->mIsSpecial = true;
			aTop->mSpecialType = PIECE_POOF;
			mLoosePieces.push_back(aTop);
		}

		for (int aRow = 0; aRow < mRows - 1; aRow++)
		{
			Piece* aPiece = mGrid[aRow + 1][aCol];
			mGrid[aRow][aCol] = aPiece;
			if (aPiece != NULL)
				aPiece->mRow--;
		}

		Piece* aNext = mNextRow[aCol];
		mGrid[mRows - 1][aCol] = aNext;
		aNext->mRow = mRows - 1;

		Piece* aPiece = new Piece();
		aPiece->mCol = aCol;
		aPiece->mRow = mRows - 1;
		aPiece->mX = (double) ColX(aCol);
		aPiece->mY = (double)(RowY(mRows) + 6);
		aPiece->mColor = (int)(mRand.Next() % (unsigned long) mNumColors);
		mNextRow[aCol] = aPiece;
	}

	mMintPixels = 29;
	mMintTarget -= 0.75;
	mRiseOffset += 29;
	mMint = 0.0;
	FindGroups();
}

// 0x408a60 (section 5.4)
void Board::UpdatePhysics()
{
	bool aRemoved = false;
	bool aBagSeen = false;
	mPiecesMoving = false;

	// 1. Life timers (unused) and special animations
	for (int aRow = mRows - 1; aRow >= 0; aRow--)
	{
		for (int aCol = 0; aCol < mCols; aCol++)
		{
			Piece* aPiece = mGrid[aRow][aCol];
			if (aPiece == NULL)
				continue;

			if (aPiece->mLifeTimer > 0)
			{
				aPiece->mLifeTimer--;
				if (aPiece->mLifeTimer == 0)
				{
					delete aPiece;
					mGrid[aRow][aCol] = NULL;
					aRemoved = true;
					continue;
				}
				if (aPiece->mLifeTimer < 200)
				{
					bool aHidden = (aPiece->mLifeTimer / 4) % 2 == 0;
					if (aHidden != aPiece->mBlinkHidden)
					{
						aPiece->mBlinkHidden = aHidden;
						MarkDirty();
					}
				}
			}

			if (!aPiece->mIsSpecial)
				continue;

			if (aPiece->mSpecialType == PIECE_MONEYBAG)
			{
				aBagSeen = true;
				if (aPiece->mFrame < 3)
				{
					aPiece->mFrame++;
					MarkDirty();
				}
				if ((mApp->mGameType != GAMETYPE_PUZZLE) && (CrtRand() % 10 == 0))
				{
					int anOffset = CrtRand() % 28;
					mParticles.push_back(MakeParticle(5, (double) anOffset + aPiece->mX, aPiece->mY + 20.0, 0.0, 0.0, 0.035, false));
				}
			}

			if ((aPiece->mSpecialType == PIECE_DOLLAR_BOMB) && (mUpdateCnt % 3 == 0))
			{
				aPiece->mFrame = (aPiece->mFrame + 1) % 32;
				MarkDirty();
			}
		}
	}

	if (aBagSeen)
	{
		mBagGlow = mBagGlow + 0.02;
		if (mBagGlow > 1.0)
			mBagGlow = mBagGlow - 1.0;
		MarkDirty();
	}

	double aBottom = (double)(RowY(mRows - 1) + mRiseOffset);
	if (mState != STATE_BOMB)
	{
		// 2a. Gravity
		for (int aRow = mRows - 1; aRow >= 0; aRow--)
		{
			for (int aCol = 0; aCol < mCols; aCol++)
			{
				Piece* aPiece = mGrid[aRow][aCol];
				if (aPiece == NULL)
					continue;

				double anOldY = aPiece->mY;
				aPiece->mVY = aPiece->mVY + 0.14;
				aPiece->mY = aPiece->mVY + aPiece->mY;

				if (mState == STATE_LEVEL_CLEAR_FALL)
				{
					if (aPiece->mY < (double) mHeight)
						mPiecesMoving = true;
					else
					{
						delete aPiece;
						mGrid[aRow][aCol] = NULL;
					}
					continue;
				}

				double aFloor = aBottom;
				Piece* aBelow = NULL;
				if (aRow < mRows - 1)
				{
					aBelow = mGrid[aRow + 1][aCol];
					if (aBelow != NULL)
						aFloor = aBelow->mY - 29.0;
				}

				if (aPiece->mY <= aFloor)
				{
					mPiecesMoving = true;
					continue;
				}

				if (aPiece->mFallThrough)
				{
					// A special leaves the board
					if (aPiece == mHintPiece)
						HideHint();

					if (aPiece->mSpecialType == PIECE_MONEYBAG)
					{
						mApp->PlaySample(SOUND_GOTBAG);
						mGrid[aPiece->mRow][aPiece->mCol] = NULL;
						int aPoints = 100;
						if (mUpdateCnt - mLastBagTick < 500)
						{
							ShowBonusText(mApp->GetString("SPEED_BAGGER_BONUS"));
							mApp->mMrMoney->PlayAnim(MRANIM_BONUS);
							mApp->PlaySample(SOUND_BONUS);
							aPoints = 150;
						}
						AddPoints(aPoints, (int)(aPiece->mX + 14.0), (int)(aPiece->mY + 14.0), false);

						Point aSlot;
						GetBagSlotPos(&aSlot, mBagsReleased);
						mApp->mBagCollectedOnce = true;
						mBagsReleased++;
						aPiece->mFlying = true;
						aPiece->mTargetX = aSlot.mX;
						aPiece->mTargetY = aSlot.mY;
						aPiece->mStartX = (int) aPiece->mX;
						aPiece->mStartY = (int) aPiece->mY;
						int aDX = aPiece->mTargetX - aPiece->mStartX;
						int aDY = aPiece->mTargetY - aPiece->mStartY;
						double aSpeed = 15.0 / sqrt((double)(aDY * aDY + aDX * aDX));
						if (aSpeed >= 0.4)
							aSpeed = 0.4;
						aPiece->mFlySpeed = aSpeed;
						aPiece->mFlyT = 0.0;
						mLoosePieces.push_back(aPiece);
						mLastBagTick = mUpdateCnt;
					}
					else if (aPiece->mSpecialType == PIECE_DOLLAR_BOMB)
					{
						mApp->mBombSeenOnce = true;
						aPiece->mSpecialType = PIECE_EXPLODING;
						aPiece->mFrame = 0;
						mGrid[aPiece->mRow][aPiece->mCol] = NULL;
						mLoosePieces.push_back(aPiece);
						if ((mHintVisible) && (mHintId == HINT_BOMB))
							HideHint();
						if (mState == STATE_PLAYING)
							mState = STATE_BOMB;
						mBombColor = aPiece->mColor;
					}
					aRemoved = true;
					continue;
				}

				// Landing
				if ((aPiece->mVY > 4.0) && (mUpdateCnt - mLastPieceHitTick >= 8))
				{
					mApp->PlaySample(SOUND_PIECEHIT);
					mLastPieceHitTick = mUpdateCnt;
				}

				if (aBelow == NULL)
				{
					aPiece->mY = aFloor;
					aPiece->mVY = aPiece->mVY * -0.15;
				}
				else if (aBelow->mVY > 0.0)
				{
					aPiece->mY = aFloor;
					aPiece->mVY = aPiece->mVY * 0.85;
				}
				else if (anOldY <= aFloor)
				{
					aPiece->mY = aFloor;
					aPiece->mVY = aPiece->mVY * -0.15;
					aBelow->mVY = aBelow->mVY * -0.15;
				}
				else
				{
					// Already overlapping (the floor rose): ease out of it
					double anOverlap = anOldY - aFloor;
					if (anOverlap >= 0.05)
						aPiece->mY = anOldY - anOverlap * 0.6;
					else
						aPiece->mY = aFloor;
					aPiece->mVY = 0.0;
				}
			}
		}

		// 2b. Columns slide sideways after Compact closed a gap
		for (int aCol = 0; aCol < mCols; aCol++)
		{
			double aTargetX = (double) ColX(aCol);
			bool aLeader = true;
			for (int aRow = mRows - 1; aRow >= 0; aRow--)
			{
				Piece* aPiece = mGrid[aRow][aCol];
				if (aPiece == NULL)
					continue;

				if (aLeader)
				{
					double aDiff = aTargetX - aPiece->mX;
					if (aDiff > 0.0)
					{
						if (aDiff >= 16.0)
							aDiff = 16.0;
					}
					else if (aDiff <= -16.0)
						aDiff = -16.0;
					aPiece->mVX = aDiff * 0.004 + aPiece->mVX;

					double aNextX = aPiece->mX + aPiece->mVX;
					if (((aTargetX <= aPiece->mX) && (aNextX <= aTargetX)) || ((aTargetX >= aPiece->mX) && (aNextX >= aTargetX)))
					{
						aPiece->mX = aTargetX;
						aPiece->mVX = 0.0;
					}
					else
						aLeader = false;
				}
				else
				{
					// Followers spring toward the piece below (and above). mGrid[aRow + 1] is used twice
					// where the original probably meant aRow + 2 (kept: 0.5*x + 0.5*x is x).
					Piece* aBelow = mGrid[aRow + 1][aCol];
					if (aBelow != NULL)
					{
						double aTarget = aBelow->mX;
						if (aRow < mRows - 2)
							aTarget = aTarget * 0.5 + aBelow->mX * 0.5;
						if (aRow > 0)
						{
							Piece* anAbove = mGrid[aRow - 1][aCol];
							if (anAbove != NULL)
								aTarget = aTarget * 0.67 + anAbove->mX * 0.33;
						}

						aPiece->mVX = aPiece->mVX * 0.7 + (aTarget - aPiece->mX) * 0.45499999999999996;
						if (abs((int) aPiece->mVX) > abs((int)(aTarget - aPiece->mX)))
							aPiece->mVX = aTarget - aPiece->mX;
						if (((double) abs((int)(aPiece->mX - aTarget)) < 0.005) &&
							((double) abs((int)(aTargetX - aTarget)) < 0.005) &&
							(aPiece->mVX < 0.005))
						{
							aPiece->mX = aTargetX;
							aPiece->mVX = 0.0;
						}
					}
				}

				aPiece->mX = aPiece->mVX + aPiece->mX;
			}
		}
	}

	MarkDirty();
	if (aRemoved)
		Compact();
}

// 0x409520: pieces outside the grid (section 5.2)
void Board::UpdateLoosePieces()
{
	if (mLoosePieces.empty())
	{
		if ((mFlingDelay == 0) && (mState == STATE_GAME_OVER))
		{
			mApp->PlaySample(SOUND_GAMEOVER);
			mApp->mMrMoney->PlayAnim(MRANIM_GAMEOVER);
			mState = STATE_GAME_OVER_SCORES;
			mHighScoreDelay = 180;
			mHighScoreWidget = new HighScoreWidget(mApp);
			mWidgetManager->AddWidget((Widget*) mHighScoreWidget);
			ShowContinueButton();
			// Placed by Update state 6 once the high scores slide in
			((Widget*) mContinueButton)->Resize(0, 0, 0, 0);
		}
		return;
	}

	int aCoinFrames = mApp->mImages[IMAGE_COINS]->GetWidth() / 29;
	int aHorzFrames = mApp->mImages[IMAGE_HORZCOINS]->GetWidth() / 29;

	std::list<Piece*>::iterator anItr = mLoosePieces.begin();
	while (anItr != mLoosePieces.end())
	{
		Piece* aPiece = *anItr;
		bool anErase = false;
		bool aDelete = false;

		if (aPiece->mFlying)
		{
			aPiece->mFlyT = aPiece->mFlySpeed + aPiece->mFlyT;
			if (aPiece->mFlyT >= 1.0)
			{
				if (aPiece->mIsSpecial)
				{
					// A bag arrives in the holder
					if (mBagsBanked < mBagsNeeded)
					{
						mBagsBanked++;
						mBagFlashCount = 3;
					}
					if ((mBagsBanked == mBagsNeeded) && (mState == STATE_PLAYING))
					{
						HideHint();
						if (mApp->mGameType == GAMETYPE_PUZZLE)
							mState = STATE_PUZZLE_SWEEP;
						else
							LevelComplete();
					}
				}
				anErase = true;
				aDelete = true;
			}

			if (!aPiece->mIsSpecial)
			{
				// A combo coin flying to the meter
				double aT4 = aPiece->mFlyT * 4.0;
				if (aT4 >= 3.0)
					aPiece->mImageSet = 0;
				aPiece->mFrame = (int)((aT4 - (double)(int) aT4) * (double) aHorzFrames);
			}

			double aRest = 1.0 - aPiece->mFlyT;
			aPiece->mX = (double)(int)((double) aPiece->mStartX * aRest + (double) aPiece->mTargetX * aPiece->mFlyT);
			aPiece->mY = (double)(int)((double) aPiece->mStartY * aRest + (double) aPiece->mTargetY * aPiece->mFlyT);

			bool isBag = (aPiece->mIsSpecial) && (aPiece->mSpecialType == PIECE_MONEYBAG);
			if ((isBag) || (CrtRand() % 15 == 0))
			{
				int anOffsetX = CrtRand() % 8;
				double aX = (((double) anOffsetX + aPiece->mX) + 14.0) - 4.0;
				int anOffsetY = CrtRand() % 8;
				double aY = (((double) anOffsetY + aPiece->mY) + 14.0) - 4.0;
				int aVX = CrtRand() % 1000;
				mParticles.push_back(MakeParticle(3, aX, aY, (double)(aVX - 500) * 0.002, -2.0, 0.1, true));
			}
		}
		else if (mState != STATE_BOMB)
		{
			if (aPiece->mIsSpecial)
			{
				if (aPiece->mSpecialType != PIECE_POOF)
				{
					// A bag or bomb dropping in (or an exploding coin outside state 7)
					aPiece->mVY = aPiece->mVY + 0.14;
					double aNewX = aPiece->mVX + aPiece->mX;
					aPiece->mX = aNewX;
					aPiece->mY = aPiece->mVY + aPiece->mY;

					int aFloorY = RowY(mRows);
					int aFloorRow = mRows;
					for (int aRow = 0; aRow < mRows; aRow++)
					{
						Piece* aCoin = mGrid[aRow][aPiece->mCol];
						if ((aCoin != NULL) && (!aCoin->mIsSpecial))
						{
							aFloorRow = aCoin->mRow;
							aFloorY = (int) aCoin->mY;
							break;
						}
					}

					double aTargetX = (double) ColX(aPiece->mCol);
					if (aNewX < aTargetX)
						aPiece->mX = aNewX + 1.0;
					else if (aNewX > aTargetX)
						aPiece->mX = aNewX - 1.0;

					if (mState == STATE_PLAYING)
					{
						if (aPiece->mY >= (double)(aFloorY - 29))
						{
							anErase = true;
							if (aFloorRow <= 0)
								aDelete = true; // no room
							else
							{
								mApp->PlaySample(SOUND_BAGHIT);
								aPiece->mRow = aFloorRow - 1;
								aPiece->mVY = 0.0;
								aPiece->mY = (double) RowY(aPiece->mRow);
								mGrid[aPiece->mRow][aPiece->mCol] = aPiece;
								if ((!mApp->mBagCollectedOnce) && (aPiece->mSpecialType == PIECE_MONEYBAG) && (aPiece->mRow < mRows - 1))
								{
									ShowHint(HINT_BAG, mGrid[aPiece->mRow + 1][aPiece->mCol], 0);
									mHintAlt = true;
								}
								if ((!mApp->mBombSeenOnce) && (aPiece->mSpecialType == PIECE_DOLLAR_BOMB))
									ShowHint(HINT_BOMB, aPiece, 1000);
							}
						}
					}
					else if (aPiece->mY > (double) mHeight)
					{
						anErase = true;
						aDelete = true;
					}

					if (CrtRand() % 2 == 0)
					{
						int anOffset = CrtRand() % 28;
						mParticles.push_back(MakeParticle(2, (double) anOffset + aPiece->mX, aPiece->mY + 20.0, 0.0, 0.0, 0.035, false));
					}
				}
			}
			else
			{
				// A removed coin vanishing, or a flung coin
				if (!aPiece->mDying)
				{
					aPiece->mVY = aPiece->mVY + 0.14;
					aPiece->mY = aPiece->mVY + aPiece->mY;
					aPiece->mX = aPiece->mVX + aPiece->mX;
				}
				if ((aPiece->mX < 84.0) || ((double) mWidth < aPiece->mX))
				{
					anErase = true;
					aDelete = true;
				}
				if (aPiece->mSpinSpeed > 0.0)
				{
					aPiece->mSpin = aPiece->mSpinSpeed + aPiece->mSpin;
					aPiece->mFrame = (int)((aPiece->mSpin - (double)(int) aPiece->mSpin) * (double) aHorzFrames);
				}
				if ((double) RowY(mRows - 1) <= aPiece->mY)
				{
					aPiece->mVY = aPiece->mVY * -0.8;
					if (aPiece->mVY > -4.0)
						aPiece->mVY = -4.0 - (double)(mRand.Next() % 100) * 0.01;
					aPiece->mY = (double) RowY(mRows - 1);
					if (aPiece->mVX < 0.0)
						aPiece->mVX -= 0.5;
					if (aPiece->mVX > 0.0)
						aPiece->mVX += 0.5;
				}
			}
		}

		if ((aPiece->mDying) && (!aPiece->mFlying))
		{
			aPiece->mFrame++;
			if (aPiece->mFrame >= aCoinFrames)
			{
				anErase = true;
				aDelete = true;
			}
		}

		int aType = aPiece->mSpecialType;
		if ((aType == PIECE_POOF) && (mUpdateCnt % 2 == 0))
		{
			aPiece->mFrame++;
			if (aPiece->mFrame >= 14)
			{
				anErase = true;
				aDelete = true;
			}
		}
		if ((aType == PIECE_EXPLODING) && (mUpdateCnt % 3 == 0))
		{
			aPiece->mFrame++;
			if (aPiece->mFrame >= 14)
			{
				anErase = true;
				aDelete = true;
			}
		}
		if ((aType == PIECE_DOLLAR_BOMB) && (mUpdateCnt % 3 == 0))
			aPiece->mFrame = (aPiece->mFrame + 1) % 32;

		if (aDelete)
			delete aPiece;
		if (anErase)
			anItr = mLoosePieces.erase(anItr);
		else
			++anItr;
	}

	MarkDirty();
}

// 0x409e00: a column that stood above both neighbours lost more than 4.2 rows at once. Run every
// playing tick (result ignored) and by OnClick (+125).
bool Board::TowerToppler()
{
	if (mApp->mGameType == GAMETYPE_PUZZLE)
		return false;

	int aTop[BOARD_COLS];
	for (int aCol = 0; aCol < mCols; aCol++)
	{
		aTop[aCol] = mRows;
		for (int aRow = 0; aRow < mRows; aRow++)
		{
			Piece* aPiece = mGrid[aRow][aCol];
			if ((aPiece != NULL) && (!aPiece->mIsSpecial))
			{
				aTop[aCol] = aRow;
				break;
			}
		}
	}

	bool aBonus = false;
	for (int aCol = 0; aCol < mCols; aCol++)
	{
		int aLeft = (aCol == 0) ? mRows : aTop[aCol - 1];
		int aRight = (aCol == mCols - 1) ? mRows : aTop[aCol + 1];
		int aLeftDiff = aLeft - aTop[aCol];
		if (aLeftDiff < 1)
			aLeftDiff = 0;
		int aRightDiff = aRight - aTop[aCol];
		if (aRightDiff < 1)
			aRightDiff = 0;
		int aHeight = (aRightDiff <= aLeftDiff) ? aRightDiff : aLeftDiff;

		double aNewHeight = (double) aHeight;
		double aSmoothed = mColTowerHeight[aCol];
		if ((aSmoothed <= 0.0) || (aSmoothed - aNewHeight < 4.2))
			aNewHeight = aNewHeight * 0.0035 + aSmoothed * 0.9965;
		else
		{
			mApp->mMrMoney->PlayAnim(MRANIM_BONUS);
			mApp->PlaySample(SOUND_BONUS);
			ShowBonusText(mApp->GetString("TOWER_TOPPLER_BONUS"));
			aBonus = true;
		}
		mColTowerHeight[aCol] = aNewHeight;
	}
	return aBonus;
}

// 0x40a110 (section 6.5)
void Board::GenerateBoard()
{
	mMeterCoins = 0;
	mMeterDrained = 0.0;
	mMeterTarget = 0.0;
	mMeterPending = 0;
	mMeterLaunchTimer = 0.0;
	mMeterEmptying = 0.0;

	// The pieces are not deleted (the original's grid is always empty here, or uninitialised in the ctor)
	for (int aRow = 0; aRow < mRows; aRow++)
	{
		for (int aCol = 0; aCol < mCols; aCol++)
			mGrid[aRow][aCol] = NULL;
	}
	int aNextRowIndex = mRows; // the loop counter the original reuses as the next row's mRow

	if (mApp->mGameType == GAMETYPE_PUZZLE)
	{
		// The constructive generator: grow random same-colour groups until every column reaches row 6
		static const int aDirs[5][2] = { {0, 0}, {-1, 0}, {1, 0}, {0, 1}, {0, -1} };

		for (;;)
		{
			std::vector<int> aCandidates;
			bool allFull = true;
			for (int aCol = 0; aCol < mCols; aCol++)
			{
				Piece* aRow6 = mGrid[6][aCol];
				if ((aRow6 != NULL) && (!aRow6->mIsSpecial))
					continue;

				bool aNearStack = false;
				for (int k = aCol - 1; k <= aCol + 1; k++)
				{
					if ((k >= 0) && (k < 14) && (mGrid[14][k] != NULL))
						aNearStack = true;
				}
				if (aNearStack)
					aCandidates.push_back(aCol);
				allFull = false;
			}
			if (allFull)
				break;

			int aCol;
			if (aCandidates.empty())
				aCol = (int)(mRand.Next() % 14);
			else
				aCol = aCandidates[mRand.Next() % (unsigned long) aCandidates.size()];

			int anEmpty = 0;
			while ((anEmpty < 15) && (mGrid[anEmpty][aCol] == NULL))
				anEmpty++;
			if (anEmpty <= 1)
				continue;

			unsigned long aRand = mRand.Next() % 100000;
			int aStartRow = (int) ceil(14.0 - (double)(15 - anEmpty) * pow((double) aRand * 5e-06, 2.0) * 4.0);
			int aColor = (int)(mRand.Next() % (unsigned long) mNumColors); // overwritten before use
			ResetMarks();
			Piece* aFirst = NULL;
			int aPlaced = 0;
			aRand = mRand.Next() % 100000;
			int aTargetSize = (int)(pow((double) aRand * 5e-06, 4.0) * 16.0 * 15.0);

			for (int anAttempt = 0; ; anAttempt++)
			{
				if ((anAttempt >= aTargetSize) && (aPlaced >= 2))
					break;

				// port: the first coin can only go into aCol and needs mGrid[2][aCol] empty; when that row is
				// taken (a column of 13 picked by Next() % 14 with no candidate column left) the original
				// loops forever. Leaving the group changes no run that does not hang.
				if ((aFirst == NULL) && (mGrid[2][aCol] != NULL))
					break;

				int aX;
				int aY;
				if (aFirst == NULL)
				{
					if ((anAttempt > 4) && (aStartRow < 14))
						aStartRow++;
					aX = aCol;
					aY = aStartRow;
					aColor = (int)(mRand.Next() % (unsigned long) mNumColors);
				}
				else
				{
					// Grow from a random member of the group
					std::vector<Point> aMembers;
					for (int aRow = 0; aRow < 15; aRow++)
					{
						for (int aMemberCol = 0; aMemberCol < 14; aMemberCol++)
						{
							Piece* aPiece = mGrid[aRow][aMemberCol];
							if ((aPiece != NULL) && (aPiece->mMark == 1))
								aMembers.push_back(Point(aMemberCol, aRow));
						}
					}
					unsigned long anIndex = mRand.Next() % (unsigned long) aMembers.size();
					unsigned long aDir = mRand.Next() & 3; // dir 4 (0,-1) is never drawn
					aX = aMembers[anIndex].mX + aDirs[aDir][0];
					aY = aMembers[anIndex].mY + aDirs[aDir][1];
				}

				if ((aX >= 0) && (aX < 14) && (aY >= 0) && (aY < 15) && ((aY == 14) || (mGrid[aY + 1][aX] != NULL)))
				{
					// No contact with another coin of the same colour (also next to the pieces this
					// insertion pushes up)
					bool isOk = true;
					Piece* aCheck = mGrid[aY][aX];
					if ((aCheck != NULL) && (aCheck->mColor == aColor) && (aCheck->mMark != 1))
						isOk = false;
					if ((aX - 1 >= 0) && (aX - 1 < 14) && ((aCheck = mGrid[aY][aX - 1]) != NULL) && (aCheck->mColor == aColor) && (aCheck->mMark != 1))
						isOk = false;
					if ((aX + 1 >= 0) && (aX + 1 < 14) && ((aCheck = mGrid[aY][aX + 1]) != NULL) && (aCheck->mColor == aColor) && (aCheck->mMark != 1))
						isOk = false;
					if ((aY + 1 >= 0) && (aY + 1 < 15) && ((aCheck = mGrid[aY + 1][aX]) != NULL) && (aCheck->mColor == aColor) && (aCheck->mMark != 1))
						isOk = false;
					for (int aRow = aY - 1; aRow >= 0; aRow--)
					{
						Piece* aMember = mGrid[aRow + 1][aX];
						if ((aMember == NULL) || (aMember->mMark != 1))
							break;
						for (int aNeighbourCol = aX - 1; aNeighbourCol <= aX + 1; aNeighbourCol++)
						{
							aCheck = mGrid[aRow][aNeighbourCol];
							if ((aCheck != NULL) && (aCheck->mColor == aColor) && (aCheck->mMark != 1))
								isOk = false;
						}
					}

					if ((mGrid[2][aX] == NULL) && (isOk))
					{
						// Rows 1..aY of the column move up one to make room
						for (int aRow = 1; aRow <= aY; aRow++)
						{
							Piece* aPiece = mGrid[aRow][aX];
							if (aPiece != NULL)
							{
								aPiece->mRow--;
								mGrid[aRow - 1][aX] = aPiece;
								mGrid[aRow][aX] = NULL;
							}
						}

						Piece* aPiece = new Piece();
						aPiece->mCol = aX;
						aPiece->mRow = aY;
						aPiece->mX = (double) ColX(aX);
						aPiece->mColor = aColor;
						aPiece->mMark = 1;
						mGrid[aY][aX] = aPiece;
						if (aFirst == NULL)
							aFirst = aPiece;
						aPlaced++;
					}
				}

				if ((aPlaced == 1) && (anAttempt > 8))
				{
					// A lone coin is no group: take it back and repack its column
					delete aFirst;
					mGrid[aStartRow][aCol] = NULL;
					int aDest = 14;
					for (int aRow = 14; aRow >= 0; aRow--)
					{
						Piece* aPiece = mGrid[aRow][aCol];
						if (aPiece != NULL)
						{
							mGrid[aDest][aCol] = aPiece;
							aPiece->mRow = aDest;
							aDest--;
						}
					}
					for (int aRow = aDest; aRow >= 0; aRow--)
						mGrid[aRow][aCol] = NULL;
					break;
				}
			}
		}

		// Money bags on top of (mBagsNeeded - skill + 4) random columns (CRT rand)
		std::vector<int> aColumns;
		for (int aCol = 0; aCol < mCols; aCol++)
			aColumns.push_back(aCol);
		int aBagColumns = (mBagsNeeded - mApp->mSkillLevel) + 4;
		for (int i = 0; i < mCols - aBagColumns; i++)
		{
			unsigned int anIndex = (unsigned int) CrtRand() % (unsigned int) aColumns.size();
			aColumns.erase(aColumns.begin() + anIndex);
		}
		for (size_t i = 0; i < aColumns.size(); i++)
		{
			Piece* aBag = new Piece();
			int aCol = aColumns[i];
			aBag->mCol = aCol;
			int aRow = 14;
			while (mGrid[aRow][aCol] != NULL)
				aRow--;
			aBag->mRow = aRow;
			aBag->mIsSpecial = true;
			aBag->mSpecialType = PIECE_MONEYBAG;
			aBag->mX = (double) ColX(aCol);
			mGrid[aRow][aCol] = aBag;
		}

		// The columns drop in from above the screen
		for (int aCol = 0; aCol < mCols; aCol++)
		{
			int aY = -58 - (int)(((double)(mRand.Next() % 10) * 0.3333333333333333 + 1.2) * 29.0);
			for (int aRow = mRows - 1; aRow >= 0; aRow--)
			{
				Piece* aPiece = mGrid[aRow][aCol];
				if (aPiece == NULL)
					break;
				aPiece->mY = (double) aY;
				aY += (int)(((double)(mRand.Next() % 10) * 0.3333333333333333 + 1.2) * -29.0);
			}
		}
	}
	else
	{
		// Action / Strategy: 6 rows of random coins dropping in from above
		for (int aCol = 0; aCol < mCols; aCol++)
		{
			int aY = -58;
			double aX = (double) ColX(aCol);
			for (int aRow = mRows - 1; aRow >= mRows - 6; aRow--)
			{
				aY += (int)(((double)(mRand.Next() % 10) * 0.3333333333333333 + 1.2) * -29.0);
				Piece* aPiece = new Piece();
				aPiece->mY = (double) aY;
				aPiece->mCol = aCol;
				aPiece->mRow = aRow;
				aPiece->mX = aX;
				aPiece->mColor = (int)(mRand.Next() % (unsigned long) mNumColors);
				mGrid[aRow][aCol] = aPiece;
			}
		}
	}

	// The next row
	double aNextY = (double)(RowY(15) + 6);
	for (int aCol = 0; aCol < mCols; aCol++)
	{
		mColTowerHeight[aCol] = 0.0;
		mColWarning[aCol] = -1;
		Piece* aPiece = new Piece();
		aPiece->mCol = aCol;
		aPiece->mRow = aNextRowIndex;
		aPiece->mY = aNextY;
		aPiece->mX = (double) ColX(aCol);
		aPiece->mColor = (int)(mRand.Next() % (unsigned long) mNumColors);
		delete mNextRow[aCol]; // port: the original leaks the previous level's next row
		mNextRow[aCol] = aPiece;
	}

	mMintDelay = 160;
	FindGroups();
	mMint = 0.0;
	mMintTarget = 0.25;
	mMintPixels = 29;
	mLastGroupSize = 0;
	mPrevGroupSize = 0;
	mLastColor = 0;
	mColorStreak = 0;
	mSweepTone = 0;
	UpdateMovesLeft();
}

// 0x40ae20
void Board::NextLevel()
{
	mLevel++;
	mState = STATE_PLAYING;
	if (mContinueButton != NULL)
	{
		mWidgetManager->RemoveWidget((Widget*) mContinueButton);
		mApp->SafeDeleteWidget((Widget*) mContinueButton);
		mContinueButton = NULL;
	}
	mLevelCoinsCleared = 0;
	mLevelBiggestBonus = 0;
	mLevelBiggestCombo = 0;
	mBagsReleased = 0;
	mBagsBanked = 0;
	mGoTimer = 80;
	SetupLevelParams();
	GenerateBoard();
}

// 0x40aea0: a money bag (meter full) or a dollar bomb drops into a free column
void Board::DropSpecial(int theType)
{
	std::vector<int> aCandidates;
	for (int aCol = 0; aCol < mCols; aCol++)
	{
		bool isFree = true;
		for (int aRow = 0; aRow < mRows; aRow++)
		{
			Piece* aPiece = mGrid[aRow][aCol];
			if ((aPiece != NULL) && ((aRow == 0) || (aPiece->mIsSpecial)))
				isFree = false;
		}
		for (std::list<Piece*>::iterator anItr = mLoosePieces.begin(); anItr != mLoosePieces.end(); ++anItr)
		{
			if (((*anItr)->mCol == aCol) && ((*anItr)->mIsSpecial))
				isFree = false;
		}
		if (isFree)
			aCandidates.push_back(aCol);
	}

	// The very first bag of the session never drops into the last column
	if ((theType == PIECE_MONEYBAG) && (!mApp->mBagCollectedOnce) && (mApp->mGameType != GAMETYPE_PUZZLE))
	{
		for (std::vector<int>::iterator anItr = aCandidates.begin(); anItr != aCandidates.end(); ++anItr)
		{
			if (*anItr == mCols - 1)
			{
				aCandidates.erase(anItr);
				break;
			}
		}
	}

	if (aCandidates.empty())
		return;

	int aCol = aCandidates[mRand.Next() % (unsigned long) aCandidates.size()];
	Piece* aPiece = new Piece();
	aPiece->mY = -29.0;
	aPiece->mVX = 0.5;
	aPiece->mCol = aCol;
	aPiece->mX = (double) ColX(aCol);
	aPiece->mIsSpecial = true;
	aPiece->mSpecialType = theType;
	if (theType == PIECE_DOLLAR_BOMB)
		aPiece->mColor = (int)(mRand.Next() % (unsigned long) mNumColors);
	mLoosePieces.push_back(aPiece);
}

// 0x40b120: a clicked group (section 6.8)
void Board::OnClick(Piece* thePiece)
{
	bool aVoiced = (mState != STATE_PLAYING);
	if (thePiece->mIsSpecial)
		return;

	int aCount = 0;
	int aSum[2] = { 0, 0 };
	FloodFill(thePiece->mCol, thePiece->mRow, mUpdateCnt, true, &aCount, aSum);
	if (aCount < mMinGroup)
	{
		MarkDirty();
		return;
	}

	if (!mPlayStarted)
	{
		mApp->mTimesPlayed++;
		mPlayStarted = true;
	}
	mApp->mHasClickedGroup = true;

	if ((mHintVisible) && ((mHintId == HINT_CLICK_GROUP) || (mHintId == HINT_CLICK_GROUP_PUZZLE)))
	{
		HideHint();
		if (mApp->mGameType == GAMETYPE_PUZZLE)
		{
			for (int aRow = 0; aRow < 15; aRow++)
			{
				for (int aCol = 0; aCol < 14; aCol++)
				{
					Piece* aPiece = mGrid[aRow][aCol];
					if ((!mHintVisible) && (aPiece != NULL) && (aPiece->mIsSpecial) && (aPiece->mSpecialType == PIECE_MONEYBAG))
						ShowHint(HINT_PUZZLE_BAG, aPiece, 1000);
				}
			}
		}
		else
		{
			ShowHintAt(HINT_METER, Point(610, 0), 1000);
			mHintFollowMeter = true;
		}
	}

	// Specials directly above the group
	int aBagsAbove = 0;
	for (int aRow = 1; aRow < mRows; aRow++)
	{
		for (int aCol = 0; aCol < mCols; aCol++)
		{
			Piece* aPiece = mGrid[aRow][aCol];
			Piece* anAbove = mGrid[aRow - 1][aCol];
			if ((aPiece != NULL) && (aPiece->mMark == mUpdateCnt) && (anAbove != NULL) && (anAbove->mIsSpecial))
				aBagsAbove++;
		}
	}

	int aScore;
	if (aCount - 2 < 14)
		aScore = gGroupBaseScores[aCount - 2];
	else
		aScore = (aCount - 2) * 25 - 145;
	if (aBagsAbove > 0)
		aScore *= 2;

	// Combos over the last three clicks
	int aPrev1 = mLastGroupSize;
	int aPrev2 = mPrevGroupSize;
	const char* aComboId = NULL;
	int aComboAnim = 0;
	int aComboSound = 0;
	int aComboBonus = 0;
	if ((aCount >= 13) && (aPrev1 >= 13) && (aPrev2 >= 13))
	{
		aComboId = "TRIPLE_HYPER_COMBO"; aComboAnim = MRANIM_AWESOME; aComboSound = SOUND_AWESOME; aComboBonus = 450;
	}
	else if (aCount > 19)
	{
		aComboId = "AWESOME_COMBO"; aComboAnim = MRANIM_AWESOME; aComboSound = SOUND_AWESOME; aComboBonus = 400;
	}
	else if ((aCount > 9) && (aPrev1 > 9) && (aPrev2 > 9))
	{
		aComboId = "TRIPLE_MEGA_COMBO"; aComboAnim = MRANIM_UNBELIEVABLE; aComboSound = SOUND_UNBELIEVABLE; aComboBonus = 350;
	}
	else if ((aCount > 12) && (aPrev1 > 12))
	{
		aComboId = "DOUBLE_HYPER_COMBO"; aComboAnim = MRANIM_UNBELIEVABLE; aComboSound = SOUND_UNBELIEVABLE; aComboBonus = 350;
	}
	else if ((aCount > 9) && (aPrev1 > 9))
	{
		aComboId = "DOUBLE_MEGA_COMBO"; aComboAnim = MRANIM_INCREDIBLE; aComboSound = SOUND_INCREDIBLE; aComboBonus = 250;
	}
	else if (aCount > 14)
	{
		aComboId = "BIG_MONEY_COMBO"; aComboAnim = MRANIM_BIGMONEY; aComboSound = SOUND_BIGMONEY; aComboBonus = 200;
	}
	else if ((aCount > 6) && (aPrev1 > 6) && (aPrev2 > 6))
	{
		aComboId = "TRIPLE_COMBO"; aComboAnim = MRANIM_AWESOME; aComboSound = SOUND_AWESOME; aComboBonus = 100;
	}
	else if ((aCount > 6) && (aPrev1 > 6))
	{
		aComboId = "DOUBLE_COMBO"; aComboAnim = MRANIM_GOODJOB; aComboSound = SOUND_GOODJOB; aComboBonus = 75;
	}
	else if (aCount > 9)
	{
		aComboId = "EXCELLENT_COMBO"; aComboAnim = MRANIM_EXCELLENT; aComboSound = SOUND_EXCELLENT; aComboBonus = 25;
	}

	if (aComboId != NULL)
	{
		if (!aVoiced)
		{
			mApp->mMrMoney->PlayAnim(aComboAnim);
			mApp->PlaySample(aComboSound);
		}
		ShowBonusText(mApp->GetString(aComboId));
		aScore += aComboBonus;
		aVoiced = true;
	}

	// Colour combo
	if (thePiece->mColor == mLastColor)
	{
		mColorStreak++;
		if (mColorStreak == 4)
		{
			if (!aVoiced)
			{
				mApp->mMrMoney->PlayAnim(MRANIM_BONUS);
				mApp->PlaySample(SOUND_BONUS);
				aVoiced = true;
			}
			ShowBonusText(mApp->GetString("COLOR_COMBO_BONUS"));
			aScore += 20;
		}
		else if (mColorStreak == 6)
		{
			if (!aVoiced)
			{
				mApp->mMrMoney->PlayAnim(MRANIM_SUPERBONUS);
				mApp->PlaySample(SOUND_SUPERBONUS);
				aVoiced = true;
			}
			ShowBonusText(mApp->GetString("SUPER_COLOR_COMBO_BONUS"));
			aScore += 50;
		}
	}
	else
	{
		mLastColor = thePiece->mColor;
		mColorStreak = 1;
	}

	// Speed bonus (Action)
	if (mApp->mGameType == GAMETYPE_ACTION)
	{
		mSpeedBonus = mSpeedBonus + 0.16;
		if (mSpeedBonus >= 1.0)
		{
			mSpeedBonus = 0.0;
			if (!aVoiced)
			{
				mApp->mMrMoney->PlayAnim(MRANIM_SPEEDBONUS);
				mApp->PlaySample(SOUND_SPEEDBONUS);
				aVoiced = true;
			}
			ShowBonusText(mApp->GetString("SPEED_BONUS"));
			aScore += 40;
		}
	}

	mLevelCoinsCleared += aCount;
	mGameCoinsCleared += aCount;
	if (mLevelBiggestCombo < aCount)
		mLevelBiggestCombo = aCount;
	if (mGameBiggestCombo < aCount)
		mGameBiggestCombo = aCount;

	if (aCount <= 4)
		mApp->PlaySample(SOUND_CLEARCOINS);
	else if (aCount <= 9)
		mApp->PlaySample(SOUND_CLEARCOINS2);
	else
		mApp->PlaySample(SOUND_CLEARCOINS3);
	if (mApp->mGameType == GAMETYPE_PUZZLE)
		mApp->PlaySample(SOUND_TONEPUZZLE);
	else
	{
		double aTone = mMeterFraction * 9.0;
		if (8.0 < aTone)
			aTone = 8.0;
		mApp->PlaySample((int) aTone + SOUND_TONE1);
	}

	int aColumnsBefore = CountCoinColumns();

	// Remove the group
	int aMinRow = mRows;
	int aMaxRow = -1;
	for (int aRow = 0; aRow < mRows; aRow++)
	{
		for (int aCol = 0; aCol < mCols; aCol++)
		{
			Piece* aPiece = mGrid[aRow][aCol];
			if ((aPiece == NULL) || (aPiece->mMark != mUpdateCnt))
				continue;

			if (aPiece == mHintPiece)
				HideHint();
			if (aRow < aMinRow)
				aMinRow = aRow;
			if (aMaxRow < aRow)
				aMaxRow = aRow;
			aPiece->mDanger = false;
			aPiece->mGroupSize = 0;
			aPiece->mHighlighted = false;
			aPiece->mVX = 0.0;
			aPiece->mVY = 0.0;

			if (aBagsAbove >= 1)
			{
				aPiece->mIsSpecial = true;
				aPiece->mSpecialType = PIECE_POOF;
				aPiece->mFrame = 0;
			}
			else if (aCount >= 7)
			{
				// Big groups fly to the meter
				aPiece->mFlying = true;
				aPiece->mTargetX = METER_TARGET_X;
				aPiece->mTargetY = METER_TARGET_Y;
				aPiece->mStartX = (int) aPiece->mX;
				aPiece->mStartY = (int) aPiece->mY;
				int aDX = aPiece->mTargetX - aPiece->mStartX;
				int aDY = aPiece->mTargetY - aPiece->mStartY;
				double aSpeed = 9.0 / sqrt((double)(aDY * aDY + aDX * aDX));
				if (aSpeed >= 0.2)
					aSpeed = 0.2;
				aPiece->mFlySpeed = aSpeed;
				aPiece->mFlyT = 0.0;
				aPiece->mImageSet = CrtRand() % 2 + 1;
				aPiece->mDying = true;
			}
			else
			{
				if (aCount >= 5)
				{
					for (int k = -1; k < 2; k++)
						mParticles.push_back(MakeParticle(2, aPiece->mX + 14.0, aPiece->mY + 14.0, (double) k * 1.5, -3.0, 0.2, true));
				}
				aPiece->mDying = true;
				aPiece->mFrame = 0;
			}

			mLoosePieces.push_back(aPiece);
			mGrid[aRow][aCol] = NULL;
		}
	}

	if (aMinRow <= aMaxRow)
	{
		int aDelay = (aMaxRow - aMinRow) * 8 + 24;
		if (aDelay <= mMintDelay)
			aDelay = mMintDelay;
		mMintDelay = aDelay;
	}

	Compact();
	if (TowerToppler())
		aScore += 125;

	int aColumnsCleared = aColumnsBefore - CountCoinColumns();
	if (aColumnsCleared >= 6)
	{
		if (!aVoiced)
		{
			mApp->mMrMoney->PlayAnim(MRANIM_SUPERBONUS);
			mApp->PlaySample(SOUND_SUPERBONUS);
		}
		ShowBonusText(mApp->GetString("SUPER_COLUMN_CLEARING_BONUS"));
		aScore += 200;
	}
	else if (aColumnsCleared >= 3)
	{
		if (!aVoiced)
		{
			mApp->mMrMoney->PlayAnim(MRANIM_BONUS);
			mApp->PlaySample(SOUND_BONUS);
		}
		ShowBonusText(mApp->GetString("COLUMN_CLEARING_BONUS"));
		aScore += 50;
	}

	AddPoints(aScore, aSum[0] / aCount + 14, aSum[1] / aCount + 14, true);

	if ((mApp->mGameType == GAMETYPE_STRATEGY) && (aBagsAbove == 0))
	{
		mMintTarget = mMintPerClick + mMintTarget;
		UpdateMovesLeft();
		if ((mMovesLeft <= 3) && (mMovesLeft >= 1))
		{
			int aDigit = 3 - mMovesLeft;
			mApp->mMrMoney->PlayAnim(aDigit + MRANIM_THREE);
			mApp->PlaySample(aDigit + SOUND_THREE);
			mMoveDigitActive = true;
			mMoveDigit = aDigit;
			mMoveDigitFade = 1.0;
		}
	}

	mPrevGroupSize = mLastGroupSize;
	mLastGroupSize = aCount;
	MarkDirty();
}

// 0x40c5c0
void Board::ClickAt(int x, int y)
{
	Piece* aPiece = PieceAt(x, y);
	if ((aPiece != NULL) && (!aPiece->mIsSpecial))
		OnClick(aPiece);
}

// 0x40c5f0 slot 38: any mouse button
void Board::MouseDown(int x, int y, int theClickCount)
{
	Widget::MouseDown(x, y, theClickCount);
	if (mPauseCount > 0)
	{
		Pause(false);
		return;
	}
	if (mState == STATE_PLAYING)
		ClickAt(x, y);
}

// 0x40c640: the money meter (section 5.3)
void Board::UpdateMeter()
{
	if ((mMeterLaunchTimer <= 0.0) && (mMeterPending > 0))
	{
		unsigned int aStacked = (unsigned int)((int) mTubeCoins.size() + mMeterCoins);
		int aTop = (int)(440.0 - ((((double) aStacked) - mMeterDrained) + 3.0) * 6.0);
		TubeCoin aCoin;
		aCoin.mDist = (double)(aTop - 32);
		aCoin.mY = (double) aTop - aCoin.mDist;
		aCoin.mFrame = 0;
		mTubeCoins.push_back(aCoin);
		mMeterPending--;
		mMeterLaunchTimer = mMeterLaunchTimer + 1.0;
	}

	if (mMeterLaunchTimer > 0.0)
		mMeterLaunchTimer = mMeterLaunchTimer - (double) mMeterPending * 0.125;

	std::list<TubeCoin>::iterator anItr = mTubeCoins.begin();
	while (anItr != mTubeCoins.end())
	{
		anItr->mDist = anItr->mDist - 15.0;
		anItr->mY = anItr->mY + 15.0;
		anItr->mFrame = (int)(9.0 * anItr->mDist * 0.0022222222222222222);
		if (anItr->mDist < 0.0)
		{
			anItr = mTubeCoins.erase(anItr);
			mMeterCoins++;
			mApp->PlaySample(SOUND_COINTUBE);
		}
		else
			++anItr;
		MarkDirty();
	}

	double aFill = ((double) mMeterCoins - mMeterDrained) * 0.015625;
	if (aFill > 1.0)
	{
		if (mMeterFlashTimer != 0)
			return;

		if (mMeterEmptying == 0.0)
		{
			// Full: a money bag drops and the meter empties by one full meter
			mMeterEmptying = mMeterEmptying + 64.0;
			mMeterPending = 0;
			mMeterFlashFrame = 0;
			mMeterFlashTimer = 160;
			mMeterJustFull = 1.0;
			mMeterFraction = 0.0;
			if (mState == STATE_PLAYING)
			{
				mApp->PlaySample(SOUND_BONUSDROP);
				DropSpecial(PIECE_MONEYBAG);
			}
		}
	}

	if (mMeterFlashTimer != 0)
		return;

	if (mMeterEmptying > 0.0)
	{
		if (mMeterEmptying >= 0.5)
		{
			mMeterDrained = mMeterDrained + 0.5;
			mMeterEmptying = mMeterEmptying - 0.5;
		}
		else
		{
			mMeterDrained = mMeterEmptying + mMeterDrained;
			mMeterEmptying = 0.0;
		}
	}

	if ((aFill > 0.0) && (mState == STATE_PLAYING) && (mApp->mGameType == GAMETYPE_ACTION))
	{
		// Action: the meter drains over time
		if (mUpdateCnt % 2 == 0)
			mMeterDrained = mMeterDrainRate * 64.0 + mMeterDrained;
		if (mMeterDrained <= 0.0)
			mMeterDrained = 0.0;

		double aLevel = aFill * 6.0;
		if (5.0 < aLevel)
			aLevel = 5.0;
		int aMeterLevel = (int) aLevel;
		if (aMeterLevel != mMeterLevel)
			mMeterLevel = aMeterLevel;
		MarkDirty();
	}
}

// 0x40c9f0 slot 9: the per-tick order of section 5
void Board::Update()
{
	if (mState == STATE_FROZEN)
		return;

	if (mPauseCount > 0)
	{
		mPauseClickable = (mApp->GetDialogCount() == 0);
		if (mPauseFade < 1.0)
		{
			mPauseFade = mPauseFade + 0.05;
			if (mPauseFade > 1.0)
				mPauseFade = 1.0;
			MarkDirty();
		}
	}
	else if (mPauseFade > 0.0)
	{
		mPauseFade = mPauseFade - 0.05;
		if (mPauseFade < 0.0)
			mPauseFade = 0.0;
		MarkDirty();
	}

	if (mPauseFade > 0.0)
	{
		mPauseAnim++;
		if (mPauseAnim % 8 == 0)
			MarkDirty();
	}

	if (mPauseCount > 0)
	{
		mApp->mMrMoney->mTargetX = 0;
		mApp->mMrMoney->mTargetY = 290;
		return;
	}

	Widget::Update();

	// 0x41d2e0: at update 100 an unregistered Puzzle game shows the "one level only" dialog; not ported
	// (the build is registered)

	if (mUpdateCnt == 300)
	{
		mApp->mMrMoney->mTargetX = 0;
		mApp->mMrMoney->mTargetY = 290;
	}

	if ((mUpdateCnt == 600) && (!mApp->mHasClickedGroup))
	{
		// The instructor points at a clickable group: an exact-size one first, then any
		int aHintId = (mApp->mGameType == GAMETYPE_PUZZLE) ? HINT_CLICK_GROUP_PUZZLE : HINT_CLICK_GROUP;
		for (int aPass = 0; aPass < 2; aPass++)
		{
			for (int aRow = 10; aRow < mRows; aRow++)
			{
				for (int aCol = 3; aCol < mCols - 3; aCol++)
				{
					Piece* aPiece = mGrid[aRow][aCol];
					if (aPiece == NULL)
						continue;
					bool aMatch = (aPass == 0) ? (aPiece->mGroupSize == mMinGroup) : (mMinGroup <= aPiece->mGroupSize);
					if ((aMatch) && (!mHintVisible))
						ShowHint(aHintId, aPiece, 0);
				}
			}
		}
	}

	if (mBillsToSpawn > 0)
	{
		Bill aBill;
		aBill.mX = (double)(CrtRand() % 500 + 80);
		int aSpeed = CrtRand() % 1000;
		aBill.mY = -50.0;
		aBill.mVY = (double) aSpeed * 0.001 * 1.2 + 1.6;
		aBill.mFrame = CrtRand() % 16;
		aBill.mType = CrtRand() % 5;
		mBills.push_back(aBill);
		mBillsToSpawn--;
	}

	if (mFlingDelay > 0)
	{
		mFlingDelay--;
		if (mFlingDelay == 0)
			FlingAllCoins();
	}

	// The glint (written only: Board::Draw never reads it)
	if (!mGlintActive)
	{
		if (CrtRand() % 2000 == 0)
		{
			mGlintActive = true;
			mGlintTick = 0;
			mGlintFrame = 0;
			MarkDirty();
		}
	}
	else
	{
		if (mUpdateCnt % 10 == 0)
		{
			mGlintFrame++;
			if (mGlintFrame > 14)
				mGlintActive = false;
			MarkDirty();
		}
		if (mUpdateCnt % 4 == 0)
		{
			mGlintTick++;
			MarkDirty();
		}
	}

	if (mRepaintTimer > 0)
	{
		mRepaintTimer--;
		if (mRepaintTimer == 0)
			MarkDirty();
	}

	if (mSpeedBonus > 0.0)
	{
		mSpeedBonus = mSpeedBonus - 0.0025;
		if (mSpeedBonus < 0.0)
			mSpeedBonus = 0.0;
	}

	if ((mState == STATE_BOMB) && (mUpdateCnt % 8 == 0))
	{
		// The next coin of the bomb's colour explodes
		bool aFound = false;
		for (int aRow = 0; (aRow < mRows) && (!aFound); aRow++)
		{
			for (int aCol = 0; aCol < mCols; aCol++)
			{
				Piece* aPiece = mGrid[aRow][aCol];
				if ((aPiece == NULL) || (aPiece->mIsSpecial) || (aPiece->mColor != mBombColor))
					continue;

				if (mExplodeSound != NULL)
					mExplodeSound->Play(false, false);
				if ((mHintVisible) && (aPiece == mHintPiece))
					HideHint();
				aPiece->mIsSpecial = true;
				aPiece->mSpecialType = PIECE_EXPLODING;
				mGrid[aRow][aCol] = NULL;
				mLoosePieces.push_back(aPiece);
				aFound = true;
				break;
			}
		}
		Compact();
		if (aFound)
			mBombIdle = 8;
		else
		{
			mBombIdle--;
			if (mBombIdle < 1)
				mState = STATE_PLAYING;
		}
	}

	if (mState == STATE_PLAYING)
		TowerToppler();

	if (mUpdateCnt == 1)
	{
		mApp->mMrMoney->mTargetX = 350;
		mApp->mMrMoney->mTargetY = 24;
		mApp->PlaySample(SOUND_GETREADY);
		mApp->mMrMoney->PlayAnim(MRANIM_GETREADY);
	}

	if (mGoTimer > 0)
	{
		mGoTimer--;
		if (mGoTimer == 0)
		{
			mApp->PlaySample(SOUND_GO);
			mApp->mMrMoney->PlayAnim(MRANIM_GO);
			mBannerTimer = (mLevel != 0) ? 384 : 480;
		}
	}

	if (mBannerTimer > 0)
	{
		mBannerTimer--;
		bool aVisible = ((mBannerTimer / 12) % 4) != 0;
		if (aVisible != mBannerVisible)
		{
			mBannerVisible = aVisible;
			MarkDirty();
		}
	}

	if (mMeterFlashTimer > 0)
	{
		mMeterFlashTimer--;
		int aFrame = 3 - abs(3 - ((mMeterFlashTimer / 2) % 6));
		if (aFrame != mMeterFlashFrame)
		{
			mMeterFlashFrame = aFrame;
			MarkDirty();
		}
	}

	UpdateLoosePieces();
	UpdateMeter();
	UpdatePhysics();

	if (mState == STATE_PLAYING)
	{
		if ((mApp->mGameType == GAMETYPE_ACTION) && (mRand.Next() % 15000 == 0) && (mLevel > 0))
			DropSpecial(PIECE_DOLLAR_BOMB);

		int aTop = GetTopCoinRow();
		if ((aTop < 4) && (!mApp->mDangerHintShown) && (mApp->mGameType != GAMETYPE_PUZZLE))
		{
			mApp->mDangerHintShown = true;
			ShowHintAt(HINT_DANGER, Point(380, 8), 1000);
		}

		// Danger flashes and warning beeps
		bool aDanger;
		int aWarningInterval = 0;
		if (mApp->mGameType == GAMETYPE_ACTION)
		{
			aDanger = (aTop < 2);
			if (aDanger)
			{
				if (mUpdateCnt % 12 == 0)
				{
					mDangerFlash = !mDangerFlash;
					SetDangerFlash(mDangerFlash);
					MarkDirty();
				}
				aWarningInterval = (int)(200.0 - (((2.0 - (double) aTop) - (double) mMintPixels * 0.034482758620689655) * 0.5 * 160.0));
			}
		}
		else
		{
			aDanger = (aTop == 0);
			if (aDanger)
			{
				if (mUpdateCnt % 24 == 0)
				{
					mDangerFlash = !mDangerFlash;
					SetDangerFlash(mDangerFlash);
					MarkDirty();
				}
				aWarningInterval = 150;
			}
		}
		if (aDanger)
		{
			if (mUpdateCnt - mLastWarningTick >= aWarningInterval)
			{
				mApp->PlaySample(SOUND_WARNING);
				mLastWarningTick = mUpdateCnt;
			}
		}
		else if (mDangerFlash)
		{
			SetDangerFlash(false);
			mDangerFlash = false;
		}

		// Mint speed factor: faster while the coins are low
		double anAverage = GetAverageCoinRow();
		double aBlend = (0.6 - (double) aTop / (double) mRows);
		aBlend = aBlend + aBlend;
		if (aBlend < 0.0)
			aBlend = 0.0;
		else if (aBlend > 1.0)
			aBlend = 1.0;
		double aSpeedFactor = (((1.0 - aBlend) * anAverage + (double) aTop * aBlend) / (double) mRows - 0.5) + 1.0;

		if (mApp->mGameType == GAMETYPE_ACTION)
		{
			if (mMintDelay > 0)
				mMintDelay--;
			if ((mMintDelay == 0) && (mUpdateCnt % 2 == 0))
			{
				mMint = aSpeedFactor * mMintSpeed + mMint;
				mMintSpeed = mMintAccel + mMintSpeed;
			}
		}

		if (mMint < mMintTarget)
		{
			mMint = mMint + 0.015;
			if (mMint > mMintTarget)
				mMint = mMintTarget;
		}

		if ((mApp->mGameType == GAMETYPE_STRATEGY) && ((!mHasMoves) || (mMint >= 1.0)))
			mMint = mMint + 0.015;

		int aHidden = (int)((1.0 - mMint) * 29.0);
		if (mMintPixels == aHidden)
		{
			mOverflowTicks = 0;
			mCountdownActive = false;
		}
		else if (aHidden >= 0)
		{
			mMintPixels = aHidden;
			MarkDirty();
		}
		else if (aTop != 0)
		{
			PushRow();
			MarkDirty();
		}
		else if (mApp->mGameType != GAMETYPE_ACTION)
		{
			GameOver();
			MarkDirty();
		}
		else
		{
			// Action overflow: "three, two, one", the board shakes, then game over
			mOverflowTicks++;
			double aCountdown = (double) mOverflowTicks * 0.01;
			if (aCountdown >= 2.99)
				aCountdown = 2.99;

			bool aChanged = true;
			bool aSay = true;
			if (mCountdownActive)
			{
				if (aCountdown == mCountdownValue)
					aChanged = false;
				else if ((int) mCountdownValue == (int) aCountdown)
					aSay = false;
			}
			if (aChanged)
			{
				if (aSay)
				{
					mApp->mMrMoney->PlayAnim((int)(aCountdown + 4.0));
					mApp->PlaySample((int)(aCountdown + 32.0));
				}
				mCountdownValue = aCountdown;
				mCountdownActive = true;
				MarkDirty();
			}

			if (mOverflowTicks % 4 == 0)
			{
				int aShakeRows = (mRows * mOverflowTicks) / 300;
				// port: the original sets only the 1.0 entries and reads the others uninitialised (stale
				// stack doubles); 0.0 is the evident intent (the 0.6 rule tests for it)
				double aWeight[BOARD_COLS];
				for (int aCol = 0; aCol < mCols; aCol++)
				{
					aWeight[aCol] = 0.0;
					Piece* aPiece = mGrid[0][aCol];
					if ((aPiece != NULL) && (!aPiece->mIsSpecial))
						aWeight[aCol] = 1.0;
				}
				for (int aCol = 0; aCol < mCols; aCol++)
				{
					if ((aWeight[aCol] == 0.0) &&
						(((aCol > 0) && (aWeight[aCol - 1] == 1.0)) || ((aCol < mCols - 1) && (aWeight[aCol + 1] == 1.0))))
						aWeight[aCol] = 0.6;
				}
				for (int aCol = 0; aCol < mCols; aCol++)
				{
					for (int aRow = 0; aRow < aShakeRows; aRow++)
					{
						Piece* aPiece = mGrid[aRow][aCol];
						if (aPiece == NULL)
							continue;
						double anAmplitude = ((double) mOverflowTicks * 0.0033333333333333335 * 0.5 + 0.6) * aWeight[aCol];
						aPiece->mShakeX = (int)(((double)(mRand.Next() % 100) * 0.01 - 0.5) * anAmplitude * 5.0);
						double aShakeY = ((double)(mRand.Next() % 100) * 0.01 - 0.5) * anAmplitude;
						aPiece->mShakeY = (int)(aShakeY + aShakeY);
					}
				}
				MarkDirty();
			}

			if (mOverflowTicks == 300)
				GameOver();
			MarkDirty();
		}
	}
	else if (mState == STATE_LEVEL_COMPLETE)
	{
		if (mUpdateCnt % 2 == 0)
			mMintPixels++;
		MarkDirty();
		if (mLevelEndDelay > 0)
			mLevelEndDelay--;
		else
			ClearBoardForLevelEnd();
	}
	else if (mState == STATE_LEVEL_CLEAR_FALL)
	{
		if ((!mPiecesMoving) && (!mLevelUpBouncing))
		{
			if (mLevelUpY > 16.0)
			{
				mLevelUpY = mLevelUpY - 1.0;
				MarkDirty();
			}
			if (mLevelUpStretch > 150.0)
			{
				mLevelUpStretch = mLevelUpStretch - 0.75;
				MarkDirty();
			}
			if (mLevelUpY == 16.0)
			{
				mState = STATE_LEVEL_DONE;
				ShowContinueButton();
				mPlaySeconds = (int)((unsigned long) mPlaySeconds + ((unsigned long) GetTickCount() - mTimerTick) / 1000);
			}
		}
	}
	else if (mState == STATE_PUZZLE_SWEEP)
	{
		if (mUpdateCnt % 16 == 0)
		{
			// One row per 16 ticks, top first
			bool aFound = false;
			for (int aRow = 0; aRow < 15; aRow++)
			{
				if (aFound)
					continue;
				int aPoints = 0;
				for (int aCol = 0; aCol < 14; aCol++)
				{
					Piece* aPiece = mGrid[aRow][aCol];
					if (aPiece == NULL)
						continue;
					aFound = true;
					mGrid[aRow][aCol] = NULL;
					for (int k = -1; k < 2; k++)
						mParticles.push_back(MakeParticle(2, aPiece->mX + 14.0, aPiece->mY + 14.0, (double) k * 1.5, -3.0, 0.2, true));
					if (!aPiece->mIsSpecial)
					{
						aPiece->mDying = true;
						aPiece->mFrame = 0;
						mLoosePieces.push_back(aPiece);
						aPoints += 3;
					}
					else
						delete aPiece;
				}
				if (aPoints > 0)
				{
					AddPoints(aPoints, 308, RowY(aRow) + 14, false);
					int aTone = mSweepTone;
					if (aTone >= 8)
						aTone = 8;
					mApp->PlaySample(aTone + SOUND_TONE1);
					mSweepTone++;
				}
			}
			if (!aFound)
				LevelComplete();
		}
	}
	else if (mState == STATE_LEVEL_DONE)
	{
		if (mStatsFade < 1.0)
		{
			mStatsDirty = true;
			mStatsFade = mStatsFade + 0.015;
			if (mStatsFade > 1.0)
				mStatsFade = 1.0;
			MarkDirty();
		}
	}
	else if (mState == STATE_GAME_OVER_SCORES)
	{
		if (mNoMovesY < 500)
		{
			mNoMovesY += 5;
			MarkDirty();
		}
		if (mHighScoreDelay > 0)
			mHighScoreDelay--;
		else
		{
			if (mHighScoreSlide > -146)
				mHighScoreSlide -= 2;
			((Widget*) mHighScoreWidget)->Resize(mX + 182, mHighScoreSlide * 3 + 664 + mY, 396, 236);
			((Widget*) mContinueButton)->Resize(212, ((Widget*) mHighScoreWidget)->mY + 200, 336, 29);
			MarkDirty();
		}
	}

	// The meter display drains at the level end
	if (((mState == STATE_LEVEL_CLEAR_FALL) || (mState == STATE_LEVEL_DONE)) && (mMeterDrained < mMeterTarget))
	{
		mMeterDrained = mMeterDrained + 0.5;
		MarkDirty();
	}

	// The "level up" sign falls and bounces on its two ends
	if ((mLevelUpActive) && (mLevelUpBouncing))
	{
		double aVY = mLevelUpLeftVY + 0.15;
		mLevelUpLeftVY = aVY;
		mLevelUpLeftY = aVY + mLevelUpLeftY;
		if (mLevelUpLeftY > 80.0)
		{
			mLevelUpLeftY = 80.0;
			mLevelUpLeftVY = aVY * -0.6;
			if (mLevelUpLeftVY > -0.5)
				mLevelUpLeftVY = 0.0;
		}

		aVY = mLevelUpRightVY + 0.15;
		mLevelUpRightVY = aVY;
		mLevelUpRightY = aVY + mLevelUpRightY;
		if (mLevelUpRightY > 80.0)
		{
			mLevelUpRightY = 80.0;
			mLevelUpRightVY = aVY * -0.6;
			if (mLevelUpRightVY > -0.5)
				mLevelUpRightVY = 0.0;
		}

		mLevelUpY = (mLevelUpRightY + mLevelUpLeftY) * 0.5;
		mLevelUpAngle = atan2(mLevelUpRightY - mLevelUpLeftY, (double) mApp->mImages[IMAGE_LEVELUP]->GetWidth());
		if ((mLevelUpLeftVY == 0.0) && (mLevelUpRightVY == 0.0))
		{
			mLevelUpBouncing = false;
			mLevelUpAngle = 0.0;
		}
		MarkDirty();
	}

	// Strategy: the moves-left lamp eases to red (0), yellow (1) or green
	if (mApp->mGameType == GAMETYPE_STRATEGY)
	{
		int aMoves = (int)((1.0 - mMintTarget) / mMintPerClick);
		Color aTarget = (aMoves == 0) ? Color(255, 0, 0) : Color((aMoves == 1) ? 255 : 0, 255, 0);
		for (int i = 0; i < 4; i++)
		{
			int& aChannel = mMovesColor[i];
			int aGoal = aTarget[i];
			if (aGoal < aChannel)
			{
				aChannel += (aGoal - aChannel) / 8 - 1;
				if (aChannel < aGoal)
					aChannel = aGoal;
				MarkDirty();
			}
			else if (aGoal > aChannel)
			{
				aChannel += (aGoal - aChannel) / 8 + 1;
				if (aChannel > aGoal)
					aChannel = aGoal;
				MarkDirty();
			}
		}
	}

	if ((mState == STATE_GAME_OVER) || (mState == STATE_GAME_OVER_SCORES))
	{
		if (mStatsFade < 1.0)
		{
			mStatsDirty = true;
			mStatsFade = mStatsFade + 0.015;
			if (mStatsFade > 1.0)
				mStatsFade = 1.0;
			MarkDirty();
		}
		if (mGameOverRiseDelay > 0)
			mGameOverRiseDelay--;
		if ((mUpdateCnt % 3 == 0) && (mGameOverRiseDelay == 0))
			mMintPixels++;
	}

	if (mRiseOffset > 0)
	{
		UpdateHover();
		mRiseOffset--;
		MarkDirty();
	}

	// Floating texts rise every other tick and expire
	std::list<Points*>::iterator aPointsItr = mPointsWidgets.begin();
	while (aPointsItr != mPointsWidgets.end())
	{
		Points* aPoints = *aPointsItr;
		aPoints->MarkDirty();
		aPoints->mHue = (aPoints->mHue + 7) % 255;
		if ((mUpdateCnt % 2 == 0) && (aPoints->mY > 0))
			aPoints->Move(aPoints->mX, aPoints->mY - 1);
		aPoints->mLife--;
		if (aPoints->mLife < 1)
		{
			if (aPoints == mLastBonusText)
				mLastBonusText = NULL;
			aPointsItr = mPointsWidgets.erase(aPointsItr);
			mWidgetManager->RemoveWidget(aPoints);
			mApp->SafeDeleteWidget(aPoints);
		}
		else
			++aPointsItr;
	}

	if (mDisplayScore != mScore)
	{
		MarkDirty();
		if (mDisplayScore < mScore)
			mDisplayScore = (int)(((double)(mScore - mDisplayScore) / mPointsMult * 0.02 + 2.0) * mPointsMult) + mDisplayScore;
		if (mDisplayScore > mScore)
			mDisplayScore = mScore;
	}

	std::list<Particle>::iterator aParticleItr = mParticles.begin();
	while (aParticleItr != mParticles.end())
	{
		if (mUpdateCnt % aParticleItr->mFrameDelay == 0)
		{
			MarkDirtyFull();
			aParticleItr->mFrame++;
			if (aParticleItr->mFrame >= 14)
			{
				aParticleItr = mParticles.erase(aParticleItr);
				continue;
			}
		}
		aParticleItr->mVY = aParticleItr->mGravity + aParticleItr->mVY;
		aParticleItr->mX = aParticleItr->mVX + aParticleItr->mX;
		aParticleItr->mY = aParticleItr->mVY + aParticleItr->mY;
		MarkDirty();
		++aParticleItr;
	}

	std::list<Bill>::iterator aBillItr = mBills.begin();
	while (aBillItr != mBills.end())
	{
		if (mUpdateCnt % 4 == 0)
			aBillItr->mFrame = (aBillItr->mFrame + 1) % 16;
		aBillItr->mY = aBillItr->mVY + aBillItr->mY;
		if (aBillItr->mY > (double) mHeight)
			aBillItr = mBills.erase(aBillItr);
		else
			++aBillItr;
		MarkDirty();
	}

	if (mHintVisible)
	{
		mHintTimer--;
		if (mHintTimer == 0)
			HideHint();
		if (mHintPiece != NULL)
		{
			mHintX = (int)(mHintPiece->mX + 14.0);
			mHintY = (int)(mHintPiece->mY + 14.0);
		}
		if (mHintFollowMeter)
		{
			int aMeterTop = (int)(440.0 - ((double) mMeterCoins - mMeterDrained) * 6.0);
			if (mHintY > 160)
				mHintY = (mHintY - (mHintY - aMeterTop) / 3) - 1;
			if (mHintY < aMeterTop)
				mHintY = aMeterTop;
		}
	}

	if (mContinueButton != NULL)
	{
		mButtonBlink = (mButtonBlink + 1) % 300;
		Color aColor = (mButtonBlink <= 275) ? Color(0, 255, 0) : Color(255, 255, 255);
		if (((Widget*) mContinueButton)->GetColor(0) != aColor)
			((Widget*) mContinueButton)->SetColor(0, aColor);
	}

	if (mMoveDigitActive)
	{
		mMoveDigitFade = mMoveDigitFade - 0.01;
		if (mMoveDigitFade < 0.0)
			mMoveDigitActive = false;
		MarkDirty();
	}

	// Puzzle: no group left before all needed bags are out
	if ((mApp->mGameType == GAMETYPE_PUZZLE) && (!mHasMoves) && (!mPiecesMoving) &&
		(mBagsReleased < mBagsNeeded) && (mState == STATE_PLAYING))
		GameOver();

	if ((mBagFlashCount > 0) || (mBagFlash != 0.0))
	{
		mBagFlash = mBagFlash + 0.02;
		if (mBagFlash > 1.0)
		{
			mBagFlash = 0.0;
			mBagFlashCount--;
		}
	}
}

// 0x40e4b0: the continue button after a level
void Board::StartNextLevel()
{
	MusicInterface* aMusic = mApp->mMusicInterface;
	switch (mApp->mGameType)
	{
	case GAMETYPE_ACTION:
		aMusic->FadeOut(SONG_ACTION_LEVELUP, true, 0.02);
		aMusic->FadeIn(SONG_ACTION, -1, 0.004, false);
		break;
	case GAMETYPE_STRATEGY:
		aMusic->FadeOut(SONG_STRATEGY_LEVELUP, true, 0.02);
		aMusic->FadeIn(SONG_STRATEGY, -1, 0.004, false);
		break;
	case GAMETYPE_PUZZLE:
		aMusic->FadeOut(SONG_PUZZLE_LEVELUP, true, 0.02);
		aMusic->FadeIn(SONG_PUZZLE, -1, 0.004, false);
		break;
	}
	mLevelUpActive = false;
	mTimerTick = GetTickCount();
	NextLevel();
}

// 0x40e580 ButtonListener slot 2: mContinueButton
void Board::ButtonDepress(int theId)
{
	if (mState == STATE_GAME_OVER_SCORES)
	{
		mApp->NewGame(); // deletes this board
		return;
	}

	// 0x41d4c0: an unregistered Puzzle game asks to register before the next level; not ported (the
	// build is registered)
	StartNextLevel();
}

// 0x40e760
Board::Board(WinBMApp* theApp)
{
	mApp = theApp;
	mHintX = 0;
	mHintY = 0;
	mHintPiece = NULL;
	gRankNames = mApp->GetStringVector("gRankNames");
	mCols = BOARD_COLS;
	mRows = BOARD_ROWS;

	// port: the original leaves the grid to GenerateBoard (it only nulls it); zeroed here so the port
	// can delete the next row's pieces it replaces
	for (int aRow = 0; aRow < BOARD_ROWS; aRow++)
	{
		for (int aCol = 0; aCol < BOARD_COLS; aCol++)
			mGrid[aRow][aCol] = NULL;
	}
	for (int aCol = 0; aCol < BOARD_COLS; aCol++)
		mNextRow[aCol] = NULL;

	int aSeed = CrtRand();
	SeedMTRand12(mRand, (unsigned long)(aSeed * 0xffff));
	mCreateTick = GetTickCount();
	mTimerTick = mCreateTick;

	mStatsSeconds = 0;
	mPlaySeconds = 0;
	mState = STATE_PLAYING;
	mPlayStarted = false;
	mRiseOffset = 0;
	mHasMoves = false;
	mMintPixels = 0;
	mMintTarget = 0.0;
	mMint = 0.0;
	mMintSpeed = 0.0;
	mMintAccel = 0.0;
	mMintPerClick = 0.0;
	mLastPieceHitTick = 0;
	mScore = 0;
	mDisplayScore = 0;
	mPointsMult = 1.0;
	mMouseX = 0;
	mMouseY = 0;
	mLastWarningTick = 0;
	mMeterFlash = 0;
	mMeterJustFull = 0.0;
	mMeterFraction = 0.0;
	mMeterCapacity = 0.0;
	mMeterDrainRate = 0.0;
	mDangerFlash = false;
	mMeterLevel = 0;
	mBagsReleased = 0;
	mBagsBanked = 0;
	mMeterFlashFrame = 0;
	mMeterFlashTimer = 0;
	mBagsNeeded = 1;
	mLevelUpActive = false;
	mLevelUpAngle = 0.0;
	mLevelUpUnused = 0.03;
	mLevelUpY = 0.0;
	// port: the original leaves the remaining level-up fields to LevelComplete
	mLevelUpStretch = 0.0;
	mLevelUpBouncing = false;
	mLevelUpLeftY = 0.0;
	mLevelUpLeftVY = 0.0;
	mLevelUpRightY = 0.0;
	mLevelUpRightVY = 0.0;
	mPiecesMoving = false;
	mLevel = 0;
	mLevelEndDelay = 0;
	mPauseCount = 0;
	mPauseFade = 0.0;
	mPauseClickable = false;
	mGoTimer = 0;
	mBannerVisible = false;
	mBannerTimer = 0;
	mFlingDelay = 0;
	mOverflowTicks = 0;
	mCountdownActive = false;
	mCountdownValue = 0.0;
	mGameOverRiseDelay = 0;
	mMintDelay = 0;
	mContinueButton = NULL;
	mButtonBlink = 0; // port: set by ShowContinueButton in the original
	mLevelCoinsCleared = 0;
	mGameCoinsCleared = 0;
	mLevelBiggestBonus = 0;
	mGameBiggestBonus = 0;
	mLevelBiggestCombo = 0;
	mGameBiggestCombo = 0;
	for (int aCol = 0; aCol < BOARD_COLS; aCol++)
	{
		mColTowerHeight[aCol] = 0.0;
		mColWarning[aCol] = -1; // port: set by GenerateBoard in the original
	}
	mSpeedBonus = 0.0;
	mMintSpeedBase = 0.0025;
	mMintSpeedPerLevel = 0.001;
	mMintAccelBase = 1e-06;
	mMintAccelPerLevel = 5e-08;
	mMeterCapBase = 300.0;
	mMeterCapPerLevel = 20.0;
	mDrainBase = 0.0;
	mDrainPerLevel = 7e-05;
	mPointsMultBase = 0.5;
	mPointsMultPerLevel = 0.1;
	mBagsBase = 2.6;
	mBagsPerLevel = 0.26;
	mPuzzleBagsBase = 2.6;
	mPuzzleBagsPerLevel = 0.26;
	mPerClickBase = 0.24501;
	mPerClickPerLevel = 0.011;
	mBombColor = 0;
	mBombIdle = 0;
	mHoverBag = false;
	mRepaintTimer = 0;
	mNumColors = 0;
	mMovesLeft = 0;      // port: the remaining fields up to the hints are uninitialised in the original
	mMoveDigit = 0;
	mMoveDigitFade = 0.0;
	mGlintFrame = 0;
	mGlintTick = 0;
	mSweepTone = 0;
	mHighScoreDelay = 0;
	mHintAlt = false;
	mHintTimer = 0;
	mHintId = 0;
	mHintFollowMeter = false;
	mGoTimer = 140;
	mLastBonusText = NULL;
	mMinGroup = (mApp->mGameType != GAMETYPE_PUZZLE) ? 3 : 2;
	// port: the original assumes a sound manager
	mExplodeSound = (mApp->mSoundManager != NULL) ? mApp->mSoundManager->GetSoundInstance(SOUND_EXPLODE) : NULL;
	mGlintActive = false;

	// Draw caches: made by Draw, deleted only by ShowContinueButton, LoadGame and the destructor
	mStatsImage = NULL;
	mStatsFadeImage = NULL;
	mLevelUpImage = NULL;
	mHighScoreWidget = NULL;

	SetupLevelParams();
	GenerateBoard();

	mStatsFade = 0.0;
	mMeterCoins = 0;
	mMeterDrained = 0.0;
	mMeterTarget = 0.0;
	mMeterPending = 0;
	mMeterLaunchTimer = 0.0;
	mMeterEmptying = 0.0;
	mBillsToSpawn = 0;
	mMoveDigitActive = false;
	mStatsDirty = false;
	mHighScoreSlide = 0;
	mBagGlow = 0.0;
	mPauseAnim = 0;
	mHintVisible = false;
	mMovesColor = Color(0x80, 0x80, 0x80);
	mClipExtra = 0;
	mUnused7a4 = 0;
	mNoMovesY = 0;
	mLastGroupSize = 0;
	mPrevGroupSize = 0;
	mLastBagTick = 0;
	mLastColor = 0;
	mColorStreak = 0;
	mBagFlashCount = 0;
	mBagFlash = 0.0;
}
