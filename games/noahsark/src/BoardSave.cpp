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
		int v = 0;
		if (mPos + 4 > mData.size())
			mOk = false;
		else
		{
			memcpy(&v, mData.data() + mPos, 4);
			mPos += 4;
		}
		return v;
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
	double Double()
	{
		double v = 0;
		if (mPos + 8 > mData.size())
			mOk = false;
		else
		{
			memcpy(&v, mData.data() + mPos, 8);
			mPos += 8;
		}
		return v;
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
	Writer w;
	w.Int(2);
	uint32_t aState[MTRAND_N];
	int aIndex = 0;
	mRand.GetState(aState, &aIndex);
	w.Int(MTRAND_N);
	for (int i = 0; i < MTRAND_N; i++)
		w.Int((int) aState[i]);
	w.Int(aIndex);
	w.Byte(mGravity);
	w.Int(mApp->mGameMode);
	w.Int(mWaterY);
	w.Int(mState);
	w.Int(mUpdateCnt);
	w.Int(mState314);
	w.Int(mLastInputTick);
	w.Int(mStartTick);
	w.Int(mState320);
	w.Int(mState324);
	w.Int(mLastMoveTick);
	w.Byte(mShowGetReady);
	w.Int(mState330);
	w.Byte(mLightning);
	w.Byte(mThunderCue);
	w.Int(mStormTimer);
	w.Int(mArkX);
	w.Int(mArkY);
	w.Int(mFade);
	w.Int(mWaterTargetY);
	w.Int(mWavePhaseB);
	w.Int(mWavePhaseA);
	w.Int(mArkLevelY);
	w.Int(mBackdropTick);
	w.Int(mBonusPoints);
	w.Int(mLastType);
	w.Int(mCombo);
	w.Int(mComboPairs);
	w.Int(mBestCombo);
	w.Int(mHintIndex);
	w.Int(mHintTimer);
	w.Int(mComboShownAt);
	w.Int(mComboShown);
	w.Int(mPulseA);
	w.Int(mPulseADir);
	w.Int(mPulseB);
	w.Int(mPulseBDir);
	w.Int(mAbandonAlpha);
	w.Int(mAbandonIndex);
	w.Int(mAbandonCount);
	w.Int(mNoahFrame);
	w.Byte(mNoahWorried);
	w.Int(mNoahSprite);
	w.Int(mArkStage);
	w.Int(mSpawnRate);
	w.Int(mRowCount);
	w.Byte(mApp->mBonusRound);
	w.Byte(mSetUp3ac);
	w.Byte(mNoMoves);
	w.Int(mApp->mLevel);
	w.Int(mApp->mDifficulty);
	w.Int(mFloodSpeed);
	for (int x = 0; x < 9; x++)
	{
		for (int y = 0; y < 9; y++)
		{
			Piece* p = mGrid[x][y];
			w.Byte(p->mMatchSwap);
			w.Byte(p->mFlash);
			w.Int(p->mX);
			w.Int(p->mAlpha);
			w.Byte(p->mPending);
			w.Int(p->mType);
			w.Byte(p->mSelected);
			w.Int(p->mDestX);
			w.Int(p->mDestY);
			w.Byte(p->mMatch);
			w.Int(p->mY);
			w.Double(p->mScale);
			w.Int(p->mFallSpeed);
			w.Int(p->mPathIndex);
			w.Int((int) p->mPixelX);
			w.Int(p->mOffsetX);
			w.Int((int) p->mPixelY);
			w.Int(p->mOffsetY);
		}
	}
	w.Int(mApp->mGameScore);
	w.Int(mApp->mShownScore);
	return w.mData;
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
	Reader r;
	r.mData = theData;
	if (r.Int() != 2)
		return false;
	if (r.Int() != MTRAND_N)
		return false;
	uint32_t aState[MTRAND_N];
	for (int i = 0; i < MTRAND_N; i++)
		aState[i] = (uint32_t) r.Int();
	int aIndex = r.Int();
	if (r.mOk)
		mRand.SetState(aState, aIndex);
	mGravity = r.Byte();
	mApp->mGameMode = r.Int();
	mWaterY = r.Int();
	mState = r.Int();
	mUpdateCnt = r.Int();
	mState314 = r.Int();
	mLastInputTick = r.Int();
	mStartTick = r.Int();
	mState320 = r.Int();
	mState324 = r.Int();
	mLastMoveTick = r.Int();
	mShowGetReady = r.Byte();
	mState330 = r.Int();
	mLightning = r.Byte();
	mThunderCue = r.Byte();
	mStormTimer = r.Int();
	mArkX = r.Int();
	mArkY = r.Int();
	mFade = r.Int();
	mWaterTargetY = r.Int();
	mWavePhaseB = r.Int();
	mWavePhaseA = r.Int();
	mArkLevelY = r.Int();
	mBackdropTick = r.Int();
	mBonusPoints = r.Int();
	mLastType = r.Int();
	mCombo = r.Int();
	mComboPairs = r.Int();
	mBestCombo = r.Int();
	mHintIndex = r.Int();
	mHintTimer = r.Int();
	mComboShownAt = r.Int();
	mComboShown = r.Int();
	mPulseA = r.Int();
	mPulseADir = r.Int();
	mPulseB = r.Int();
	mPulseBDir = r.Int();
	mAbandonAlpha = r.Int();
	mAbandonIndex = r.Int();
	mAbandonCount = r.Int();
	mNoahFrame = r.Int();
	mNoahWorried = r.Byte();
	mNoahSprite = r.Int();
	mArkStage = r.Int();
	mSpawnRate = r.Int();
	mRowCount = r.Int();
	mApp->mBonusRound = r.Byte();
	mSetUp3ac = r.Byte();
	mNoMoves = r.Byte();
	mApp->mLevel = r.Int();
	mApp->mDifficulty = r.Int();
	mFloodSpeed = r.Int();
	for (int x = 0; x < 9; x++)
	{
		for (int y = 0; y < 9; y++)
		{
			Piece* p = mGrid[x][y];
			p->mMatchSwap = r.Byte();
			p->mFlash = r.Byte();
			p->mX = r.Int();
			p->mAlpha = r.Int();
			p->mPending = r.Byte();
			p->mType = r.Int();
			p->mSelected = r.Byte();
			p->mDestX = r.Int();
			p->mDestY = r.Int();
			p->mMatch = r.Byte();
			p->mY = r.Int();
			p->mScale = r.Double();
			p->mFallSpeed = r.Int();
			p->mPathIndex = r.Int();
			p->mPixelX = (float) r.Int();
			p->mOffsetX = r.Int();
			p->mPixelY = (float) r.Int();
			p->mOffsetY = r.Int();
		}
	}
	mApp->mGameScore = r.Int();
	mApp->mShownScore = r.Int();
	return r.mOk;
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
