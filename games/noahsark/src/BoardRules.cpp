// Board.cpp, part 2: joining pairs, scoring, hints, the end of a level and the small effects
// (WinNoah.exe 0x4060d0-0x4090d0).

#include "Board.h"
#include "NoahApp.h"
#include "Compat2004.h"
#include "SexyAppBase.h"
#include "WidgetManager.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

using namespace Sexy;

// DAT_00494120: the colour of each animal kind (the score text and the trail)
static const int gAnimalColors[13][3] =
{
	{245, 131, 130}, {253, 207, 23}, {235, 255, 235}, {18, 109, 153}, {190, 190, 190}, {213, 161, 235}, {89, 89, 89},
	{253, 197, 140}, {224, 224, 224}, {74, 42, 3}, {171, 246, 127}, {181, 1, 2}, {255, 173, 1}
};

Color Board::AnimalColor(int theType)
{
	if (theType < 0)
		theType = 0;
	if (theType > 12)
		theType = 12;
	return Color(gAnimalColors[theType][0], gAnimalColors[theType][1], gAnimalColors[theType][2]);
}

// 0x407510: forget the pair or group in mLink (each piece loses its path)
void Board::ClearLinkList()
{
	for (size_t i = 0; i < mLink.size(); i++)
	{
		Piece* aPiece = mLink[i];
		if (aPiece != NULL)
		{
			for (size_t j = 0; j < aPiece->mPath.size(); j++)
				delete aPiece->mPath[j];
			aPiece->mPath.clear();
		}
	}
	mLink.clear();
}

// 0x4075d0: no animal flashes; the idle clock restarts
void Board::ClearHint()
{
	for (int x = 0; x < 9; x++)
		for (int y = 0; y < 9; y++)
			if (mGrid[x][y] != NULL)
				mGrid[x][y]->mFlash = false;
	mLastMoveTick = mUpdateCnt;
}

// 0x406a20: no animal left anywhere (nothing on the board counts as one)
bool Board::BoardCleared()
{
	for (int x = 0; x < 9; x++)
		for (int y = 0; y < 9; y++)
		{
			Piece* aPiece = mGrid[x][y];
			if ((aPiece->mType < 0xd) || aPiece->mRemoved || aPiece->mMoving || aPiece->mFading)
				return false;
		}
	return true;
}

// 0x406a60: animals still standing
int Board::AnimalCount()
{
	int aCount = 0;
	for (int x = 0; x < 9; x++)
		for (int y = 0; y < 9; y++)
		{
			Piece* aPiece = mGrid[x][y];
			if ((aPiece->mType < 0xd) && !aPiece->mRemoved && !aPiece->mFading && !aPiece->mMoving)
				aCount++;
		}
	return aCount;
}

// 0x406710: some piece is still above its cell and not released (the start of a level)
bool Board::AnyWaiting()
{
	bool aResult = false;
	for (int x = 0; x < 9; x++)
		for (int y = 0; y < 9; y++)
			if ((mGrid[x][y]->mOffsetY < 0) && !mGrid[x][y]->mPending)
				aResult = true;
	return aResult;
}

// 0x406750: let one waiting piece of a random column start to fall
void Board::ReleaseWaiting()
{
	bool aDone = false;
	do
	{
		int x = CrtRand() % 9;
		for (int y = 8; y >= 0; y--)
		{
			Piece* aPiece = mGrid[x][y];
			if (!aDone && (aPiece->mOffsetY < 0) && !aPiece->mPending)
			{
				aDone = true;
				aPiece->mPending = true;
			}
		}
	}
	while (!aDone);
}

// 0x4067a0: some piece is still falling in
bool Board::AnyFalling()
{
	bool aResult = false;
	for (int x = 0; x < 9; x++)
		for (int y = 0; y < 9; y++)
			if ((mGrid[x][y]->mOffsetY < 0) || (mGrid[x][y]->mFallSpeed > 1))
				aResult = true;
	return aResult;
}

// 0x406cd0: does another animal of the piece's kind exist that can reach it?
bool Board::HasPartner(Piece* thePiece, int theX, int theY)
{
	for (int x = 0; x < 9; x++)
	{
		for (int y = 0; y < 9; y++)
		{
			Piece* aOther = mGrid[x][y];
			if (!aOther->mFading && !aOther->mRemoved && !aOther->mMoving && (aOther != thePiece) && (aOther->mType == thePiece->mType) &&
				((x != theX) || (y != theY)) && Reachable(aOther, thePiece))
				return true;
		}
	}
	return false;
}

// 0x406aa0: is there a move? A pair that can join, or in the puzzle mode a swap that makes three in a row
bool Board::HasMove()
{
	if (mApp->mGameMode != GAMEMODE_PUZZLE)
	{
		for (int x = 0; x < 9; x++)
		{
			for (int y = 0; y < 9; y++)
			{
				Piece* aPiece = mGrid[x][y];
				if ((aPiece->mType < 0xd) && !aPiece->mRemoved && !aPiece->mFading && !aPiece->mMoving && HasPartner(aPiece, x, y))
					return true;
			}
		}
		return false;
	}

	static const int aDirs[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
	for (int x = 0; x < 9; x++)
	{
		for (int y = 0; y < 9; y++)
		{
			if (mGrid[x][y] == NULL)
				continue;
			for (int aDir = 0; aDir < 4; aDir++)
			{
				int aNextX = x + aDirs[aDir][0];
				int aNextY = y + aDirs[aDir][1];
				if ((aNextX < 0) || (aNextX > 8) || (aNextY < 0) || (aNextY > 8))
					continue;
				std::swap(mGrid[x][y], mGrid[aNextX][aNextY]);
				int aType = mGrid[x][y]->mType;
				int aLeft = x, aRight = x, aUp = y, aDown = y;
				while ((aLeft > 0) && (mGrid[aLeft - 1][y] != NULL) && (mGrid[aLeft - 1][y]->mType == aType))
					aLeft--;
				while ((aRight < 8) && (mGrid[aRight + 1][y] != NULL) && (mGrid[aRight + 1][y]->mType == aType))
					aRight++;
				while ((aUp > 0) && (mGrid[x][aUp - 1] != NULL) && (mGrid[x][aUp - 1]->mType == aType))
					aUp--;
				while ((aDown < 8) && (mGrid[x][aDown + 1] != NULL) && (mGrid[x][aDown + 1]->mType == aType))
					aDown++;
				std::swap(mGrid[x][y], mGrid[aNextX][aNextY]);
				if ((aRight - aLeft > 1) || (aDown - aUp > 1))
					return true;
			}
		}
	}
	return false;
}

// 0x406d70: flash a pair that can be joined (or a swap that makes three), and say so in the how-to line
void Board::ShowHint()
{
	ClearHint();
	if (!mApp->mShowHints || !HasMove())
		return;

	bool aDone = false;
	while (mApp->mGameMode == GAMEMODE_PUZZLE)
	{
		static const int aDirs[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
		int x = CrtRand() % 9;
		int y = CrtRand() % 9;
		if (mGrid[x][y] == NULL)
			continue;
		for (int aDir = 0; aDir < 4 && !aDone; aDir++)
		{
			int aNextX = x + aDirs[aDir][0];
			int aNextY = y + aDirs[aDir][1];
			if ((aNextX < 0) || (aNextX > 8) || (aNextY < 0) || (aNextY > 8))
				continue;
			std::swap(mGrid[x][y], mGrid[aNextX][aNextY]);
			int aType = mGrid[x][y]->mType;
			int aLeft = x, aRight = x, aUp = y, aDown = y;
			while ((aLeft > 0) && (mGrid[aLeft - 1][y] != NULL) && (mGrid[aLeft - 1][y]->mType == aType))
				aLeft--;
			while ((aRight < 8) && (mGrid[aRight + 1][y] != NULL) && (mGrid[aRight + 1][y]->mType == aType))
				aRight++;
			while ((aUp > 0) && (mGrid[x][aUp - 1] != NULL) && (mGrid[x][aUp - 1]->mType == aType))
				aUp--;
			while ((aDown < 8) && (mGrid[x][aDown + 1] != NULL) && (mGrid[x][aDown + 1]->mType == aType))
				aDown++;
			std::swap(mGrid[x][y], mGrid[aNextX][aNextY]);
			if ((aRight - aLeft > 1) || (aDown - aUp > 1))
			{
				aDone = true;
				mGrid[x][y]->mFlash = true;
				mGrid[aNextX][aNextY]->mFlash = true;
			}
		}
		if (aDone)
			goto done;
	}

	{
		Piece* aFirst = mGrid[CrtRand() % 9][CrtRand() % 9];
		Piece* aSecond = mGrid[CrtRand() % 9][CrtRand() % 9];
		while ((aFirst->mType > 0xc) || (aSecond->mType > 0xc) || (aSecond->mType != aFirst->mType) || ((aSecond->mX == aFirst->mX) && (aSecond->mY == aFirst->mY)) ||
			!Reachable(aFirst, aSecond))
		{
			aSecond = mGrid[CrtRand() % 9][CrtRand() % 9];
			aFirst = mGrid[CrtRand() % 9][CrtRand() % 9];
		}
		aSecond->mFlash = true;
		aFirst->mFlash = true;
		if (CountEmpty() < 2)
			ShowHintLine(0);
	}
done:
	mFlashPulse = 0xfa;
	mFlashPulseDir = -1;
}

// 0x407460: show the how-to-play line of the given kind for 50 ticks
void Board::ShowHintLine(int theIndex)
{
	mHintTimer = 0x32;
	mHintIndex = theIndex;
}

// 0x4074c0: no pair is left. Action and strategy abandon the rest one pair at a time; the other modes stop at once.
void Board::TriggerNoMoves()
{
	int aMode = mApp->mGameMode;
	if ((aMode != GAMEMODE_STRATEGY) && (aMode != GAMEMODE_ACTION))
	{
		mState = BOARD_NO_MOVES;
		mNoMoves = true;
		return;
	}
	mState = BOARD_ABANDON;
	mAbandonAlpha = 0;
	mAbandonIndex = -1;
}

// 0x407bd0: add to the score and float the number up from a cell
void Board::AddFloatingText(int thePoints, int theMultiplier, const Color& theColor, int theCellX, int theCellY)
{
	if (thePoints < 1)
		mApp->mGameScore += thePoints;
	else
		mApp->mGameScore += thePoints * theMultiplier;
	if (mApp->mGameScore < 0)
		mApp->mGameScore = 0;

	FloatingText* aText = new FloatingText();
	int aMode = mApp->mGameMode;
	const char* aFormat = (thePoints < 0) ? "-%d" : (((aMode == GAMEMODE_STAMPEDE) || (aMode == GAMEMODE_PUZZLE)) ? "+%d" : "%d");
	snprintf(aText->mText, sizeof(aText->mText), aFormat, thePoints < 0 ? -thePoints : thePoints * theMultiplier);
	int aRed = std::max(0, theColor.mRed - 0x14);
	int aGreen = std::max(0, theColor.mGreen - 0x14);
	int aBlue = std::max(0, theColor.mBlue - 0x14);
	aText->mColor = Color(aRed, aGreen, aBlue);
	aText->mX = CellLeft(theCellX);
	aText->mY = CellTop(theCellY) + 0x19;
	aText->mStars = (thePoints < 1) ? 0 : theMultiplier;
	aText->mAlpha = 0x14f;
	mTexts.push_back(aText);
}

// 0x407f00
void Board::UpdateTexts()
{
	for (size_t i = 0; i < mTexts.size(); )
	{
		FloatingText* aText = mTexts[i];
		aText->mAlpha -= 10;
		aText->mY -= 1;
		if (aText->mAlpha < 0x14)
		{
			delete aText;
			mTexts.erase(mTexts.begin() + i);
		}
		else
			i++;
	}
}

// 0x408580 / 0x408770: sparkles where a pair is joined
void Board::AddSparkle(int theX, int theY)
{
	Sparkle* aSparkle = new Sparkle();
	aSparkle->mX = theX;
	aSparkle->mY = theY;
	aSparkle->mLife = 0xd2;
	aSparkle->mHeight = CrtRand();
	aSparkle->mFrame = aSparkle->mHeight;
	mSparkles.push_back(aSparkle);
}

void Board::UpdateSparkles()
{
	for (size_t i = 0; i < mSparkles.size(); )
	{
		Sparkle* aSparkle = mSparkles[i];
		if ((mUpdateCnt & 1) == 0)
			aSparkle->mFrame++;
		aSparkle->mLife -= 2;
		if (aSparkle->mLife < 0x32)
		{
			delete aSparkle;
			mSparkles.erase(mSparkles.begin() + i);
		}
		else
			i++;
	}
}

// 0x407600: the pair in mLink has been joined: score it (a run of the same kind scores a combo and lowers the
// // water), and let both walk towards each other along their paths. The assignments to mShownCombo and mComboTextTimer
// are the original's small setter, 0x407440.
void Board::ConnectPair()
{
	// the original returns only for an empty link (a vector of 0 bytes), not for a single animal
	if (mLink.empty())
		return;

	mLastMoveTick = mUpdateCnt;
	Piece* aLast = mLink.back();
	Color aColor = AnimalColor(aLast->mType);

	if (mLastType != aLast->mType)
	{
		mCombo = 1;
		mLastType = aLast->mType;
		mApp->PlaySample(SOUND_POP);
		int aSound = mCombo - 1;
		if (aSound > 7)
			aSound = 7;
		mApp->PlaySample(SOUND_COMBO1 + aSound);
	}
	else
	{
		mCombo++;
		if (mCombo == 2)
		{
			mComboPairs++;
			mApp->mComboCount++;
			if (mApp->mComboCount == 1)
				ShowHintLine(3);
		}
		if (mBestCombo < mCombo)
			mBestCombo = mCombo;
		if (mApp->mBestCombo < mBestCombo)
			mApp->mBestCombo = mBestCombo;

		int aStep = mApp->mLevel / 2;
		if (aStep < 1)
			aStep = 1;
		int aWater;
		if (mApp->mGameMode == GAMEMODE_ACTION)
			aWater = mWaterY + (mCombo - 1) * aStep * 2;
		else
			aWater = mWaterY + ((mCombo - 1) * aStep * 2) / 2;
		int aMax = mHeight - 0x14;
		if (aWater > aMax)
			aWater = aMax;
		mWaterY = aWater;
		mStormTimer = 3;
		int aSound = mCombo - 1;
		if (aSound > 7)
			aSound = 7;
		mApp->PlaySample(SOUND_COMBO1 + aSound);
		mApp->PlaySample(SOUND_POP);
		mShownCombo = mCombo;
		mComboTextTimer = 0xc;
	}

	AddFloatingText(((mApp->mDifficulty / 4) + mApp->mLevel) * 10, mCombo, aColor, aLast->mX, aLast->mY);
	mApp->mAnimalsSaved += 2;
	mSelected = NULL;
	for (size_t i = 0; i < mLink.size(); i++)
	{
		Piece* aPiece = mLink[i];
		aPiece->mMoving = true;
		aPiece->mPathIndex = 0;
		aPiece->mOffsetX = 0;
		aPiece->mOffsetY = 0;
	}
	mLink.clear();
	ClearLink();
}

// 0x407930: the Stampede: a group of three or more of one kind is taken out at once
void Board::RemoveGroup()
{
	if (mLink.empty())
		return;
	mLastMoveTick = mUpdateCnt;
	Piece* aLast = mLink.back();
	Color aColor = AnimalColor(aLast->mType);
	mApp->mAnimalsSaved += (int) mLink.size();
	int aType = 0;
	for (size_t i = 0; i < mLink.size(); i++)
	{
		Piece* aPiece = mLink[i];
		aPiece->mPixelX = (float) CellLeft(aPiece->mX);
		aPiece->mPixelY = (float) CellTop(aPiece->mY);
		aPiece->mRemoved = true;
		aType = aPiece->mType;
	}
	mApp->PlaySample(SOUND_POP);
	mApp->PlaySample(aType + 1);
	int aCount = (int) mLink.size();
	int aCellY = CellAtY(mMouseY);
	int aCellX = CellAtX(mMouseX);
	AddFloatingText((mFloodSpeed / 3 + 1) * aCount * 10, 1, aColor, aCellX, aCellY);
	for (size_t i = 0; i < mLink.size(); i++)
		mApp->mSavedThisLevel++;
	mLink.clear();
}

// 0x4067e0: abandon one pair (the animals that cannot be joined when the level ends), lowering the points and
// raising the flood
void Board::AbandonPair()
{
	Piece* aFirst = NULL;
	for (;;)
	{
		if (mAbandonIndex > 0x4f)
			goto finished;
		mAbandonIndex++;
		// the original walks the cells in column-major order of a transposed index: x = i % 9, y = i / 9
		aFirst = mGrid[mAbandonIndex % 9][mAbandonIndex / 9];
		if ((aFirst->mType <= 0xc) && !aFirst->mFading && !aFirst->mRemoved)
			break;
	}
	aFirst->mFading = true;
	aFirst->mPartner = NULL;
	{
		int aPoints = 5;
		mAbandonCount++;
		for (int i = 0; i < mAbandonCount; i++)
			if (aPoints < 0x78)
				aPoints *= 2;
		aPoints /= 2;
		if (aPoints > 0x3b)
			aPoints = 0x3c;
		AddFloatingText(-aPoints, 1, Color(0xb7, 0, 0), aFirst->mX, aFirst->mY);
		mApp->PlaySample(SOUND_ABANDON);
		if (mApp->mGameMode == GAMEMODE_STRATEGY)
		{
			mApp->PlaySample(SOUND_WARNING);
			mWaterY -= 10;
			if (mWaterY < mArkLevelY)
				mWaterY = mArkLevelY;
		}

		// the original counts from -1 and tests "0x4f < i" before it increments: cells 0 to 80, the last one included
		for (int i = 0; i <= 0x50; i++)
		{
			Piece* aSecond = mGrid[i % 9][i / 9];
			if ((aSecond->mType == aFirst->mType) && !aSecond->mFading && !aSecond->mRemoved)
			{
				aSecond->mFading = true;
				aSecond->mPartner = aFirst;
				aFirst->mPartner = aSecond;
				mAbandonCount++;
				aPoints = 5;
				for (int j = 0; j < mAbandonCount; j++)
					if (aPoints < 0x78)
						aPoints *= 2;
				aPoints /= 2;
				if (aPoints > 0x3b)
					aPoints = 0x3c;
				AddFloatingText(-aPoints, 1, Color(0xb7, 0, 0), aSecond->mX, aSecond->mY);
				if (mApp->mGameMode == GAMEMODE_STRATEGY)
				{
					mApp->PlaySample(SOUND_WARNING);
					mWaterY -= 5;
					if (mWaterY < mArkLevelY)
						mWaterY = mArkLevelY;
				}
				break;
			}
		}
	}
finished:
	if (mWaterY <= mArkLevelY)
		EndRound();
}

// 0x407b40: the round is over (the water reached the ark, or everything was abandoned)
void Board::EndRound()
{
	ClearLink();
	ConnectPair();
	if ((mApp->mGameMode == GAMEMODE_STRATEGY) && (mArkLevelY < mWaterY))
	{
		mState = BOARD_LEVEL_DONE;
		mApp->ShowNextLevel();
		return;
	}
	AddFish(4, false);
	if (!mNoahWorried)
	{
		mNoahWorried = true;
		mNoahFrame = 0;
		mState = BOARD_LOST;
		return;
	}
	mState = BOARD_WON;
}

// 0x407480: the game is over
void Board::GameOver()
{
	if (mSelected != NULL)
		mSelected = NULL;
	ClearLink();
	mApp->PlaySample(SOUND_GAMEOVER);
	mApp->ShowHighScoreWidget();
}

// 0x4070d0 / 0x407160: the group of equal animals under (theX, theY) in the Stampede; fewer than three is no group
void Board::HighlightGroup(int theX, int theY)
{
	ClearLinkList();
	if ((theX >= 0) && (theX < 9) && (theY >= 0) && (theY < 9))
	{
		Piece* aPiece = mGrid[theX][theY];
		if ((aPiece != NULL) && (aPiece->mType < 0xd) && !aPiece->mRemoved)
		{
			mLink.push_back(aPiece);
			FloodGroup(theX, theY);
			if (mLink.size() < 3)
				ClearLinkList();
		}
	}
}

void Board::FloodGroup(int theX, int theY)
{
	Piece* aPiece = mGrid[theX][theY];
	if (aPiece != NULL)
	{
		aPiece->mX = theX;
		aPiece->mY = theY;
	}
	for (int aOffsetX = -1; aOffsetX < 2; aOffsetX++)
	{
		for (int aOffsetY = -1; aOffsetY < 2; aOffsetY++)
		{
			if ((aOffsetX != 0) && (aOffsetY != 0))
				continue;
			int aNextX = theX + aOffsetX;
			int aNextY = theY + aOffsetY;
			if ((aNextX < 0) || (aNextX > 8) || (aNextY < 0) || (aNextY > 8))
				continue;
			Piece* aOther = mGrid[aNextX][aNextY];
			bool aListed = std::find(mLink.begin(), mLink.end(), aOther) != mLink.end();
			if (!aOther->mRemoved && (aOther->mType == aPiece->mType) && !aListed)
			{
				mLink.push_back(aOther);
				FloodGroup(aNextX, aNextY);
			}
		}
	}
}

// 0x402260: puzzle mode: mark every run of three or more and, if theAnimate, take them out (the animals leave
// scoring points); without it they just become empty (the generator). The top cells are refilled.
bool Board::MarkRuns(bool theAnimate)
{
	bool aFound = false;
	for (int x = 0; x < 9; x++)
	{
		for (int y = 0; y < 9; y++)
		{
			Piece* aPiece = mGrid[x][y];
			int aType = aPiece->mType;
			if (aPiece->mRemoved)
				aType = PIECE_EMPTY;
			if (aType == PIECE_EMPTY)
				continue;
			if (x < 7)
			{
				Piece* aOther = mGrid[x + 1][y];
				Piece* aThird = mGrid[x + 2][y];
				if (!aOther->mRemoved && !aThird->mRemoved && (aOther->mType == aType) && (aThird->mType == aType))
				{
					if (!aPiece->mMatch)
						mApp->PlaySample(aType + 1);
					aFound = true;
					aPiece->mMatch = aOther->mMatch = aThird->mMatch = true;
				}
			}
			if (y < 7)
			{
				Piece* aOther = mGrid[x][y + 1];
				Piece* aThird = mGrid[x][y + 2];
				if (!aOther->mRemoved && !aThird->mRemoved && (aOther->mType == aType) && (aThird->mType == aType))
				{
					if (!aPiece->mMatch)
						mApp->PlaySample(aType + 1);
					aFound = true;
					aPiece->mMatch = aOther->mMatch = aThird->mMatch = true;
				}
			}
		}
	}

	for (int x = 0; x < 9; x++)
	{
		for (int y = 0; y < 9; y++)
		{
			Piece* aPiece = mGrid[x][y];
			if (!aPiece->mMatch)
				continue;
			if (!theAnimate)
				aPiece->mType = PIECE_EMPTY;
			else
			{
				ClearHint();
				aPiece->mRemoved = true;
				aPiece->mPixelX = (float) CellLeft(x);
				aPiece->mPixelY = (float) CellTop(y);
				AddFloatingText((mApp->mDifficulty + 1) * 10, 1, AnimalColor(aPiece->mType), x, y);
				mWaterY += 1;
				int aMax = mHeight - 0x14;
				if (mWaterY > aMax)
					mWaterY = aMax;
				mStormTimer = 3;
			}
			aPiece->mMatch = false;
			mApp->mSavedThisLevel++;
			ClearHint();
		}
	}

	for (int x = 0; x < 9; x++)
	{
		Piece* aPiece = mGrid[x][0];
		if (aPiece->mType == PIECE_EMPTY)
		{
			aPiece->mType = CrtRand() % ((mApp->mDifficulty / 4) + 7);
			aPiece->mFallSpeed = 1;
			aPiece->mY = 0;
			aPiece->mPixelY = (float) CellTop(0);
			aPiece->mOffsetY = -0x32;
			if (mGrid[x][1]->mOffsetY < 0)
				aPiece->mOffsetY += mGrid[x][1]->mOffsetY;
			aPiece->mAlpha = 0xff;
		}
	}
	return aFound;
}

// 0x403a90: strategy and stampede: the whole board rises one row; the waiting row slides in at the bottom
// and a new waiting row is chosen. A full top row ends the round.
void Board::PushRow()
{
	mRowCount++;
	if (!RowEmpty(0))
	{
		if (!mNoahWorried)
		{
			mNoahWorried = true;
			mNoahFrame = 0;
		}
		mApp->PlaySample(SOUND_LEVELUP);
		mState = BOARD_LOST;
	}
	else
	{
		for (int x = 0; x < 9; x++)
		{
			for (int y = 0; y < 8; y++)
			{
				Piece* aUpper = mGrid[x][y];
				Piece* aLower = mGrid[x][y + 1];
				// the upper cell takes over the lower piece's contents, with its pixel position, 50 pixels below where
				// they belong, and the lower cell gets the upper piece's old state and becomes empty (the original
				// swaps the fields of two pieces; the lower cell's pixel position is the upper's old one cut to
				// whole pixels, and its offset in y is overwritten by the next row or the new one)
				int aUpperOffsetY = aUpper->mOffsetY;
				float aUpperPixelX = (float) (int) aUpper->mPixelX;
				float aUpperPixelY = (float) (int) aUpper->mPixelY;
				std::swap(aUpper->mRemoved, aLower->mRemoved);
				std::swap(aUpper->mMoving, aLower->mMoving);
				std::swap(aUpper->mFading, aLower->mFading);
				std::swap(aUpper->mOffsetX, aLower->mOffsetX);
				std::swap(aUpper->mAlpha, aLower->mAlpha);
				std::swap(aUpper->mFallSpeed, aLower->mFallSpeed);
				aUpper->mType = aLower->mType;
				aUpper->mOffsetY = aLower->mOffsetY + 0x32;
				aUpper->mPixelX = aLower->mPixelX;
				aUpper->mPixelY = aLower->mPixelY;
				aLower->mType = PIECE_EMPTY;
				aLower->mOffsetY = aUpperOffsetY - 0x32;
				aLower->mPixelX = aUpperPixelX;
				aLower->mPixelY = aUpperPixelY;
			}
		}
		for (int x = 0; x < 9; x++)
		{
			Piece* aPiece = mGrid[x][8];
			Piece* aNext = mNextRow[x];
			aPiece->mType = aNext->mType;
			aPiece->mRemoved = false;
			aPiece->mMoving = false;
			aPiece->mFading = false;
			aPiece->mOffsetY = 0x32;
			aPiece->mOffsetX = 0;
			aPiece->mAlpha = 0xff;
			aPiece->mX = x;
			aPiece->mY = 8;
			aPiece->mPixelX = (float) CellLeft(x);
			aPiece->mPixelY = (float) CellTop(8);
			aPiece->mFallSpeed = 1;
			int aKinds = mNewRowKinds;
			if (aKinds > 0xc)
				aKinds = 0xd;
			aNext->mType = CrtRand() % aKinds;
			aNext->mOffsetY = 0x32;
		}
		int aCellY = CellAtY(mMouseY);
		int aCellX = CellAtX(mMouseX);
		HighlightGroup(aCellX, aCellY);
	}
	if (!RowEmpty(0) && (mState != BOARD_LOST))
	{
		mNoahWorried = true;
		mNoahFrame = 0;
	}
}

// 0x408df0: fish for the flood (from the sides, or anywhere when theAtSides is false)
void Board::AddFish(int theCount, bool theAtSides)
{
	for (int i = 0; i < theCount; i++)
	{
		Fish* aFish = new Fish();
		if (!theAtSides)
		{
			aFish->mX = CrtRand() % (mWidth + 400) - 0x104;
			aFish->mY = mHeight;
		}
		else
		{
			aFish->mX = ((CrtRand() & 1) == 0) ? -0x3c : mWidth;
			aFish->mY = CrtRand() % 0x14 + mWaterY;
		}
		if ((CrtRand() & 1) == 0)
		{
			aFish->mDX = 1;
			aFish->mTurnFrame = 9;
		}
		else
		{
			aFish->mDX = -1;
			aFish->mTurnFrame = 0;
		}
		aFish->mDY = ((CrtRand() & 1) == 0) ? 1 : -1;
		aFish->mFrame = CrtRand() % 0x14;
		aFish->mTurning = false;
		mFish.push_back(aFish);
	}
	MarkDirty();
}

// 0x408950: the fish swim, turn at the edges, stay below the surface and now and then let a bubble go
void Board::UpdateFish()
{
	for (size_t i = 0; i < mFish.size(); i++)
	{
		Fish* aFish = mFish[i];
		aFish->mFrame++;
		if (CrtRand() % 0x28 == 0)
		{
			aFish->mTurning = true;
			aFish->mDX = -aFish->mDX;
		}
		if ((mState == BOARD_WON) || (mState == BOARD_LOST))
		{
			if ((CrtRand() % 0x1e == 0) && (aFish->mDY == 1))
				aFish->mDY = -1;
			else if ((CrtRand() % 0xab == 0) && (aFish->mDY == -1))
				aFish->mDY = 1;
		}
		else if (CrtRand() % 0x28 == 0)
			aFish->mDY = -aFish->mDY;

		int aX = aFish->mX;
		if (aX < 1)
		{
			aFish->mTurning = true;
			aFish->mDX = 1;
		}
		if (aX >= mWidth - 0x3c)
		{
			aFish->mTurning = true;
			aFish->mDX = -1;
		}
		if (aFish->mTurning)
		{
			if (aFish->mDX == -1)
			{
				if (aFish->mTurnFrame < 1)
					aFish->mTurning = false;
				else
					aFish->mTurnFrame--;
			}
			else if (aFish->mDX == 1)
			{
				if (aFish->mTurnFrame > 8)
					aFish->mTurning = false;
				else
					aFish->mTurnFrame++;
			}
		}
		aFish->mX = aFish->mDX + aX;
		int aTop = mWaterY + 10;
		if (aFish->mY <= aTop)
		{
			aFish->mY = aTop;
			aFish->mDY = 1;
		}
		if (mHeight <= aFish->mY)
		{
			aFish->mY = mHeight;
			aFish->mDY = -1;
		}
		aFish->mY += aFish->mDY;
		if (CrtRand() % 0xfa == 0)
		{
			Bubble* aBubble = new Bubble();
			aBubble->mY = (double) (aFish->mY + 5);
			aBubble->mX = (double) (aFish->mX - 2);
			if (aFish->mDX > 0)
				aBubble->mX += 20.0;
			aBubble->mSpeed = 1.9 + (double) (CrtRand() % 10) * 0.1;
			mBubbles.push_back(aBubble);
		}
	}
}

// 0x4090d0: bubbles rise and pop at the surface
void Board::UpdateBubbles()
{
	for (size_t i = 0; i < mBubbles.size(); )
	{
		Bubble* aBubble = mBubbles[i];
		aBubble->mY -= aBubble->mSpeed;
		if ((double) (mWaterY + 0xf) > aBubble->mY)
		{
			delete aBubble;
			mBubbles.erase(mBubbles.begin() + i);
		}
		else
			i++;
	}
}
