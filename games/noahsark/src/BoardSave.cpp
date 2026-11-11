// Board.cpp, part 6: the saved game (WinNoah.exe 0x40d3c0 loads it, 0x40dbe0 writes it to data//savegame.dat)
// and the small text helpers.
//
// The original's file is version 2: the generator state as a string, then the board's fields one by one and
// the 81 pieces. The port writes the same fields in the same order into the user folder's savegame.dat, with
// the generator state as 624 words and an index (the original's string form is a different generator).

#include "Board.h"
#include "NoahApp.h"
#include "Screens.h"
#include "GameText.h"
#include "ModFeatures.h"
#include "platform/UserFiles.h"
#include "platform/VFS.h"

#include <cstdio>
#include <cstring>
#include <string>

using namespace Sexy;

namespace
{

class Writer
{
public:
	std::string mData;
	void Int(int v) { mData.append((const char*) &v, 4); }
	void Byte(bool v) { mData.push_back(v ? 1 : 0); }
	void Double(double v) { mData.append((const char*) &v, 8); }
	void Float(float v) { mData.append((const char*) &v, 4); }   // the piece's float position, as its bytes
};

class Reader
{
public:
	std::string mData;
	size_t mPos;
	bool mOk;
	Reader() : mPos(0), mOk(true) {}
	int Int()
	{
		int aValue = 0;
		if (mPos + 4 > mData.size())
			mOk = false;
		else
		{
			memcpy(&aValue, mData.data() + mPos, 4);
			mPos += 4;
		}
		return aValue;
	}
	bool Byte()
	{
		if (mPos + 1 > mData.size())
		{
			mOk = false;
			return false;
		}
		return mData[mPos++] != 0;
	}
	float Float()
	{
		float aValue = 0;
		if (mPos + 4 > mData.size())
			mOk = false;
		else
		{
			memcpy(&aValue, mData.data() + mPos, 4);
			mPos += 4;
		}
		return aValue;
	}
	double Double()
	{
		double aValue = 0;
		if (mPos + 8 > mData.size())
			mOk = false;
		else
		{
			memcpy(&aValue, mData.data() + mPos, 8);
			mPos += 8;
		}
		return aValue;
	}
};

}

SexyString Sexy::FormatText(const SexyString& theFormat, int theValue)
{
	// the original passes the text to sprintf; a text with "%d" gets the number, any other text stays as it is
	SexyString aResult = theFormat;
	size_t aPos = aResult.find("%d");
	if (aPos != SexyString::npos)
		aResult.replace(aPos, 2, StrFormat("%d", theValue));
	return aResult;
}

// 0x40bf80 region: the four how-to-play boxes, three lines each (addresses of the original's texts)
const SexyString& Board::HintLine(int theIndex, int theLine)
{
	static const unsigned int aLines[4][3] =
	{
		{0x00494324, 0x00494310, 0x00494300},
		{0x004942f0, 0x004942e0, 0x004942d0},
		{0x004942c0, 0x004942b0, 0x0049429c},
		{0x0049428c, 0x00494278, 0x00494268}
	};
	if ((theIndex < 0) || (theIndex > 3) || (theLine < 0) || (theLine > 2))
		return GT(0);
	return GT(aLines[theIndex][theLine]);
}

// the saved game numbers the two obstacles the other way round: the port's 0xf (PlaceCrates) is the original's 0x10 there
static int FileType(int theType)
{
	return (theType == 0xf) ? 0x10 : ((theType == 0x10) ? 0xf : theType);
}

std::string Board::SavePath()
{
	// port(mod): games played with mods that change play have their own saved game
	return "savegame" + ScoreKeySuffix() + ".dat";
}

// 0x40dbe0
bool Board::SaveGame()
{
	mApp->DeleteSavedGame();
	if (mApp->mHighScoreWidget != NULL)
		return false;
	if (mApp->mNextLevelWidget != NULL)
		mApp->mNextLevelWidget->ButtonDepress(1);   // the original presses its button: the next level starts first

	return Platform::WriteUserFileAtomic(SavePath(), SaveState());
}

// the board's state as the saved game holds it (port(mod): also the undo mod's snapshots)
std::string Board::SaveState()
{
	Writer aWriter;
	aWriter.Int(2);
	uint32_t aState[MTRAND_N];
	int aIndex = 0;
	mRand.GetState(aState, &aIndex);
	aWriter.Int(MTRAND_N * 4);                // the saved game holds the generator's 624 words as a string of 2496 bytes (0x423400), no index
	for (int i = 0; i < MTRAND_N; i++)
		aWriter.Int((int) aState[i]);
	aWriter.Byte(mGravity);
	aWriter.Int(mApp->mGameMode);
	aWriter.Int(mWaterY);
	aWriter.Int(mState);
	aWriter.Int(mUpdateCnt);
	aWriter.Int(mLastThunderUpdate);
	aWriter.Int(mLastInputTick);
	aWriter.Int(mStartTick);
	aWriter.Int(mUnsetAfterStartTick);
	aWriter.Int(mUnsetBeforeLastMoveTick);
	aWriter.Int(mLastMoveTick);
	aWriter.Byte(mShowGetReady);
	aWriter.Int(mUnsetAfterShowGetReady);
	aWriter.Byte(mLightning);
	aWriter.Byte(mThunderCue);
	aWriter.Int(mStormTimer);
	aWriter.Int(mArkX);
	aWriter.Int(mArkY);
	aWriter.Int(mLostFade);
	aWriter.Int(mWaterTargetY);
	aWriter.Int(mFrontWavePhase);
	aWriter.Int(mBackWavePhase);
	aWriter.Int(mArkLevelY);
	aWriter.Int(mBackdropTick);
	aWriter.Int(mBonusPoints);
	aWriter.Int(mLastType);
	aWriter.Int(mCombo);
	aWriter.Int(mComboPairs);
	aWriter.Int(mBestCombo);
	aWriter.Int(mHintIndex);
	aWriter.Int(mHintTimer);
	aWriter.Int(mShownCombo);
	aWriter.Int(mComboTextTimer);
	aWriter.Int(mDangerPulse);
	aWriter.Int(mDangerPulseDir);
	aWriter.Int(mFlashPulse);
	aWriter.Int(mFlashPulseDir);
	aWriter.Int(mAbandonAlpha);
	aWriter.Int(mAbandonIndex);
	aWriter.Int(mAbandonCount);
	aWriter.Int(mNoahFrame);
	aWriter.Byte(mNoahWorried);
	aWriter.Int(mNoahSprite + 1);                 // the original numbers its images with the registration picture (id 47) in them
	aWriter.Int(mArkStage);
	aWriter.Int(mNewRowKinds);
	aWriter.Int(mRowCount);
	aWriter.Byte(mApp->mBonusRound);
	aWriter.Byte(mLeftBehindFading);
	aWriter.Byte(mNoMoves);
	aWriter.Int(mApp->mLevel);
	aWriter.Int(mApp->mDifficulty);
	aWriter.Int(mFloodSpeed);
	for (int x = 0; x < 9; x++)
	{
		for (int y = 0; y < 9; y++)
		{
			Piece* aPiece = mGrid[x][y];
			aWriter.Byte(aPiece->mInSwap);
			aWriter.Byte(aPiece->mFlash);
			aWriter.Int(aPiece->mX);
			aWriter.Int(aPiece->mAlpha);
			aWriter.Byte(aPiece->mPending);
			aWriter.Int(FileType(aPiece->mType));
			aWriter.Byte(aPiece->mSelected);
			aWriter.Int(aPiece->mDestX);
			aWriter.Int(aPiece->mDestY);
			aWriter.Byte(aPiece->mMatch);
			aWriter.Int(aPiece->mY);
			aWriter.Double(aPiece->mScale);
			aWriter.Int(aPiece->mFallSpeed);
			aWriter.Int(aPiece->mPathIndex);
			aWriter.Float(aPiece->mPixelX);
			aWriter.Int(aPiece->mOffsetX);
			aWriter.Float(aPiece->mPixelY);
			aWriter.Int(aPiece->mOffsetY);
		}
	}
	aWriter.Int(mApp->mGameScore);
	aWriter.Int(mApp->mShownScore);
	return aWriter.mData;
}

// 0x40d3c0
bool Board::LoadGame()
{
	mLoadedGame = false;
	std::string aData;
	if (!Platform::ReadHostFile(Platform::VFS::Get().GetUserDir() + "/" + SavePath(), &aData))
		return false;
	return LoadState(aData);
}

bool Board::LoadState(const std::string& theData)
{
	Reader aReader;
	aReader.mData = theData;
	if (aReader.Int() != 2)
		return false;
	if (aReader.Int() != MTRAND_N * 4)
		return false;
	uint32_t aState[MTRAND_N];
	for (int i = 0; i < MTRAND_N; i++)
		aState[i] = (uint32_t) aReader.Int();
	if (aReader.mOk)
		mRand.SetState(aState, MTRAND_N);   // the index is not saved: the next draw makes new words (the generator is not drawn from after the seeding)
	mGravity = aReader.Byte();
	mApp->mGameMode = aReader.Int();
	mWaterY = aReader.Int();
	mState = aReader.Int();
	mUpdateCnt = aReader.Int();
	mLastThunderUpdate = aReader.Int();
	mLastInputTick = aReader.Int();
	mStartTick = aReader.Int();
	mUnsetAfterStartTick = aReader.Int();
	mUnsetBeforeLastMoveTick = aReader.Int();
	mLastMoveTick = aReader.Int();
	mShowGetReady = aReader.Byte();
	mUnsetAfterShowGetReady = aReader.Int();
	mLightning = aReader.Byte();
	mThunderCue = aReader.Byte();
	mStormTimer = aReader.Int();
	mArkX = aReader.Int();
	mArkY = aReader.Int();
	mLostFade = aReader.Int();
	mWaterTargetY = aReader.Int();
	mFrontWavePhase = aReader.Int();
	mBackWavePhase = aReader.Int();
	mArkLevelY = aReader.Int();
	mBackdropTick = aReader.Int();
	mBonusPoints = aReader.Int();
	mLastType = aReader.Int();
	mCombo = aReader.Int();
	mComboPairs = aReader.Int();
	mBestCombo = aReader.Int();
	mHintIndex = aReader.Int();
	mHintTimer = aReader.Int();
	mShownCombo = aReader.Int();
	mComboTextTimer = aReader.Int();
	mDangerPulse = aReader.Int();
	mDangerPulseDir = aReader.Int();
	mFlashPulse = aReader.Int();
	mFlashPulseDir = aReader.Int();
	mAbandonAlpha = aReader.Int();
	mAbandonIndex = aReader.Int();
	mAbandonCount = aReader.Int();
	mNoahFrame = aReader.Int();
	mNoahWorried = aReader.Byte();
	mNoahSprite = aReader.Int() - 1;
	mArkStage = aReader.Int();
	mNewRowKinds = aReader.Int();
	mRowCount = aReader.Int();
	mApp->mBonusRound = aReader.Byte();
	mLeftBehindFading = aReader.Byte();
	mNoMoves = aReader.Byte();
	mApp->mLevel = aReader.Int();
	mApp->mDifficulty = aReader.Int();
	mFloodSpeed = aReader.Int();
	for (int x = 0; x < 9; x++)
	{
		for (int y = 0; y < 9; y++)
		{
			Piece* aPiece = mGrid[x][y];
			aPiece->mInSwap = aReader.Byte();
			aPiece->mFlash = aReader.Byte();
			aPiece->mX = aReader.Int();
			aPiece->mAlpha = aReader.Int();
			aPiece->mPending = aReader.Byte();
			aPiece->mType = FileType(aReader.Int());
			aPiece->mSelected = aReader.Byte();
			aPiece->mDestX = aReader.Int();
			aPiece->mDestY = aReader.Int();
			aPiece->mMatch = aReader.Byte();
			aPiece->mY = aReader.Int();
			aPiece->mScale = aReader.Double();
			aPiece->mFallSpeed = aReader.Int();
			aPiece->mPathIndex = aReader.Int();
			aPiece->mPixelX = aReader.Float();
			aPiece->mOffsetX = aReader.Int();
			aPiece->mPixelY = aReader.Float();
			aPiece->mOffsetY = aReader.Int();
		}
	}
	mApp->mGameScore = aReader.Int();
	mApp->mShownScore = aReader.Int();
	return aReader.mOk;
}

SexyString Sexy::FormatText2(const SexyString& theFormat, int theFirst, int theSecond)
{
	// "%d:%d" style texts (the level-up panel's time)
	SexyString aResult = theFormat;
	size_t aPos = aResult.find("%d");
	if (aPos != SexyString::npos)
	{
		aResult.replace(aPos, 2, StrFormat("%d", theFirst));
		aPos = aResult.find("%d", aPos);
		if (aPos != SexyString::npos)
			aResult.replace(aPos, 2, StrFormat("%d", theSecond));
	}
	return aResult;
}
