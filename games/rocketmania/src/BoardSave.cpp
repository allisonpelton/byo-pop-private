// The Board's saved games: the writer 0x41c830 (with the piece writer 0x41c5a0), the loader 0x41b880 (with the piece
// reader 0x41b430, 0x41b6f0 and the lookup 0x41b660/0x41b690). The file is the Buffer of the 2003 framework: every field
// is written as its raw bytes (int 4, bool 1, double 8, little endian), in the order the original's writer and loader share.
// Here one visitor (Fields) lists the plain fields once, for the writer and the loader both, so they cannot drift apart.
// A piece is written with its own address as its number (a pointer has 32 bits in every build of this port, as in the
// original's), and the special and host pieces it refers to by the same numbers; the loader matches them up again.

#include "Board.h"
#include "RocketApp.h"
#include "TalkingHead.h"
#include "Buffer.h"
#include "MTRand.h"

#include <stdint.h>

using namespace Sexy;

static_assert(sizeof(int) == 4 && sizeof(double) == 8 && sizeof(bool) == 1, "the save format has fixed field sizes");

// writes and reads a field's raw bytes
struct SaveWriter
{
	Buffer*		mBuffer;
	template<class T> void	operator()(T& theValue) { mBuffer->WriteBytes((const unsigned char*) &theValue, (int) sizeof(T)); }
	template<class T, int N> void	operator()(T (&theArray)[N]) { for (int i = 0; i < N; i++) (*this)(theArray[i]); }
	void	Raw(void* theBytes, int theSize) { mBuffer->WriteBytes((const unsigned char*) theBytes, theSize); }
};

struct SaveReader
{
	const Buffer*	mBuffer;
	template<class T> void	operator()(T& theValue) { mBuffer->ReadBytes((unsigned char*) &theValue, (int) sizeof(T)); }
	template<class T, int N> void	operator()(T (&theArray)[N]) { for (int i = 0; i < N; i++) (*this)(theArray[i]); }
	void	Raw(void* theBytes, int theSize) { mBuffer->ReadBytes((unsigned char*) theBytes, theSize); }
};

// port: the number a piece is saved under (and the number that a reference to it holds)
static int PieceNumber(Piece* thePiece)
{
	return (int) (uintptr_t) thePiece;
}

// 0x41c5a0: one piece
void Board::WritePiece(Buffer* theBuffer, Piece* thePiece)
{
	SaveWriter aOut = { theBuffer };
	int aNumber = PieceNumber(thePiece);
	aOut(aNumber);
	aOut(thePiece->mX);
	aOut(thePiece->mY);
	aOut(thePiece->mSpeedX);
	aOut(thePiece->mSpeedY);
	aOut(thePiece->mMoving);
	aOut(thePiece->mSlideProgress);
	aOut(thePiece->mSlideStep);
	aOut(thePiece->mSlideStartX);
	aOut(thePiece->mSlideStartY);
	aOut(thePiece->mSlideEndX);
	aOut(thePiece->mSlideEndY);
	aOut(thePiece->mRow);
	aOut(thePiece->mColumn);
	aOut(thePiece->mFromRow);
	aOut(thePiece->mFromColumn);
	aOut(thePiece->mUsed);
	aOut(thePiece->mBlocked);
	aOut(thePiece->mType);
	aOut(thePiece->mKind);
	aOut(thePiece->mRotation);
	int anAge = mUpdateCnt - thePiece->mBorn;
	aOut(anAge);
	aOut(thePiece->mPath);
	aOut(thePiece->mPathFrom);
	aOut(thePiece->mFrame);
	aOut(thePiece->mTilt);
	aOut(thePiece->mTurnedAt);
	int aSpecial = PieceNumber(thePiece->mSpecial);
	int aHost = PieceNumber(thePiece->mHost);
	aOut(aSpecial);
	aOut(aHost);
	aOut(thePiece->mInMoverList);
	aOut(thePiece->mAlwaysZero);
	aOut(thePiece->mIndex);
	aOut(thePiece->mIndex2);
	aOut(thePiece->mHitsLeft);
	aOut(thePiece->mTrailLife);
	aOut(thePiece->mMovedFromRow);
	aOut(thePiece->mMovedFromColumn);
}

// 0x41b430: one piece (the references to other pieces are kept as their numbers until 0x41b690)
void Board::ReadPiece(const Buffer* theBuffer, Piece* thePiece)
{
	SaveReader aIn = { theBuffer };
	aIn(thePiece->mId);
	aIn(thePiece->mX);
	aIn(thePiece->mY);
	aIn(thePiece->mSpeedX);
	aIn(thePiece->mSpeedY);
	aIn(thePiece->mMoving);
	aIn(thePiece->mSlideProgress);
	aIn(thePiece->mSlideStep);
	aIn(thePiece->mSlideStartX);
	aIn(thePiece->mSlideStartY);
	aIn(thePiece->mSlideEndX);
	aIn(thePiece->mSlideEndY);
	aIn(thePiece->mRow);
	aIn(thePiece->mColumn);
	aIn(thePiece->mFromRow);
	aIn(thePiece->mFromColumn);
	aIn(thePiece->mUsed);
	aIn(thePiece->mBlocked);
	aIn(thePiece->mType);
	aIn(thePiece->mKind);
	aIn(thePiece->mRotation);
	int anAge;
	aIn(anAge);
	thePiece->mBorn = mUpdateCnt - anAge;
	aIn(thePiece->mPath);
	aIn(thePiece->mPathFrom);
	aIn(thePiece->mFrame);
	aIn(thePiece->mTilt);
	aIn(thePiece->mTurnedAt);
	aIn(thePiece->mLoadedSpecial);
	aIn(thePiece->mLoadedHost);
	aIn(thePiece->mInMoverList);
	aIn(thePiece->mAlwaysZero);
	aIn(thePiece->mIndex);
	aIn(thePiece->mIndex2);
	aIn(thePiece->mHitsLeft);
	aIn(thePiece->mTrailLife);
	aIn(thePiece->mMovedFromRow);
	aIn(thePiece->mMovedFromColumn);
}

// 0x41b6f0: reads a piece; one that was read before (the same number, a piece that stands in two of the lists) is the
// one that is returned
Piece* Board::LoadPiece(const Buffer* theBuffer)
{
	Piece* aPiece = new Piece(0);
	ReadPiece(theBuffer, aPiece);
	for (std::list<Piece*>::iterator anItr = mLoadedPieces.begin(); anItr != mLoadedPieces.end(); ++anItr)
	{
		if ((*anItr)->mId == aPiece->mId)
		{
			delete aPiece;
			return *anItr;
		}
	}
	mLoadedPieces.push_back(aPiece);
	return aPiece;
}

// 0x41b690: the pieces read from the save get the pieces their numbers stand for
void Board::ResolveLoadedPieces()
{
	for (std::list<Piece*>::iterator anItr = mLoadedPieces.begin(); anItr != mLoadedPieces.end(); ++anItr)
	{
		Piece* aPiece = *anItr;
		if (!aPiece->mReferencesResolved)
		{
			aPiece->mSpecial = FindLoadedPiece(aPiece->mLoadedSpecial);
			aPiece->mHost = FindLoadedPiece(aPiece->mLoadedHost);
			aPiece->mReferencesResolved = true;
		}
	}
}

// the plain fields of the board, in the order of 0x41c830 and 0x41b880 (what comes before them is the header, the random
// numbers and the pieces of the grid; after them come the pieces that move about)
template<class Visitor>
void Board::Fields(Visitor& theVisitor)
{
	theVisitor(mLaunchedRockets);
	theVisitor(mNightLength);
	theVisitor(mLaunchesNeeded);
	theVisitor(mLaunchesNeededCopy);
	theVisitor(mLeftCount);
	theVisitor(mRightCount);
	theVisitor(mRightEdgeLoaded);
	theVisitor(mRank);
	theVisitor(mSpecialCounts);
	for (int i = 0; i < 9; i++)
	{
		theVisitor(mLeftReach[i]);
		theVisitor(mRightReach[i]);
		theVisitor(mLeftTouched[i]);
		theVisitor(mRightTouched[i]);
	}
	theVisitor(mFallTimer);
	theVisitor(mFallDisplay);
	theVisitor(mScoreShown);
	theVisitor(mBurning);
	theVisitor(mBurnCountdown);
	theVisitor(mBurningPath);
	theVisitor(mFlameCount);
	theVisitor(mTilesDropped);
	theVisitor(mLevelFraction);
	theVisitor(mLevelFractionShown);
	theVisitor(mNightLeft);
	theVisitor(mNightLeftShown);
	theVisitor(mNightStep);
	for (int i = 0; i < 6; i++)
	{
		theVisitor(mHintCountdown[i]);
		theVisitor(mHintDone[i]);
	}
	theVisitor(mDealStart);
	theVisitor(mDealRow);
	theVisitor(mPause);
	theVisitor(mPauseFade);
	theVisitor(mSecondsPlayed);
	theVisitor(mPlayedMs);
	theVisitor(mStartTicks);
	theVisitor(mLevelEndCountdown);
	theVisitor(mTilesRemoved);
	theVisitor(mLaunchesDone);
	theVisitor(mMatchesLeft);
	theVisitor(mMatchesWarning);
	theVisitor(mEndCountdown);
	theVisitor(mLaunchesCounted);
	theVisitor(mQuickDrops);
	// (the originals' loop never moves its three addresses: thirty times it writes, and reads, the same four bytes at +0x3fc,
	// the byte at +0x41a and the byte at +0x438, so the path flags mPathEdges, mPathLeftLive and mPathRightLive are not kept; a loaded game has them
	// as a new board has them until the connection finder, which sets them afresh at every pass, runs)
	for (int i = 0; i < 30; i++)
	{
		theVisitor.Raw(mPathLeftLive, 4);
		theVisitor(mPathRightLive[0]);
		theVisitor(mStarted);
	}
	theVisitor(mStarted);
	theVisitor(mRightClicked);
	theVisitor(mBannerTime);
	theVisitor(mGameOverDelay);
	theVisitor(mFallPeriod);
	theVisitor(mDropColumn);
	theVisitor(mOverflowTimer);
	theVisitor(mTimedFallPeriod);
	theVisitor(mTimedDropColumn);
	theVisitor(mSettledAt);
	theVisitor(mSettleFlag);
	theVisitor(mGameOverKind);
	theVisitor(mLastLanternOut);
	theVisitor(mLanternPuff);
	theVisitor(mComboCount);
	theVisitor(mLaunchStep);
	theVisitor(mMoved);
	theVisitor(mNightWarned);
	theVisitor(mLaunchFlames);
	theVisitor(mLaunchRow);
	theVisitor(mSpecialsDelay);
	theVisitor(mRefilling);
	theVisitor(mJustRefilled);
	theVisitor(mAlwaysZero);
	theVisitor(mChainBonusTime);
	theVisitor(mShakeTime);
	theVisitor(mShakeAmount);
	theVisitor(mFrozenTime);
	theVisitor(mHintsQuiet);
	theVisitor(mPingCooldown);
	theVisitor(mClockMade);
	theVisitor(mClockDue);
	theVisitor(mClockNightLeft);
	theVisitor(mMatchAttached);
	theVisitor(mJustInTime);
	theVisitor(mChainBurn);
	theVisitor(mCovered);
	theVisitor(mBuyRocketsState);
	theVisitor(mBuyRocketsPending);
	theVisitor(mLevelMostTiles);
	theVisitor(mLevelMostRockets);
	theVisitor(mLevelCoins);
	theVisitor(mLevelGems);
	theVisitor(mLevelBombs);
	theVisitor(mLevelClocks);
	theVisitor(mLevelMatches);
	theVisitor(mTotalMostTiles);
	theVisitor(mTotalMostRockets);
	theVisitor(mTotalCoins);
	theVisitor(mTotalGems);
	theVisitor(mTotalBombs);
	theVisitor(mTotalClocks);
	theVisitor(mTotalMatches);
	// the piece generator's counters (+0x00 to +0x63)
	theVisitor(mGen->mLaunched);
	theVisitor(mGen->mCoins);
	theVisitor(mGen->mTotal);
	theVisitor(mGen->mCoinCount);
	theVisitor(mGen->mMeter);
}

// 0x41c830: the game in progress is written to the mode's save file. The pause that comes first (0x421f30 true, then
// false) adds the time played so far to the seconds count.
bool Board::Save()
{
	Buffer aBuffer;
	SetPause(true);
	SetPause(false);
	SaveWriter aOut = { &aBuffer };
	unsigned int aMagic = 0xFADA0002;
	aOut(aMagic);
	aOut(mApp->mMode);
	aOut(mApp->mDifficulty);
	aOut(mScore);
	aOut(mLevel);
	aOut(mState);
	aBuffer.WriteString(mRand->Serialize());

	Piece** aGroups[] = { &mGrid[0][0], mTop, &mSpare, mLeft, mRight };
	int aCounts[] = { 54, 6, 1, 9, 9 };
	for (int aGroup = 0; aGroup < 5; aGroup++)
	{
		for (int i = 0; i < aCounts[aGroup]; i++)
		{
			Piece* aPiece = aGroups[aGroup][i];
			bool aPresent = aPiece != NULL;
			aOut(aPresent);
			if (aPresent)
				WritePiece(&aBuffer, aPiece);
		}
	}

	Fields(aOut);

	int aCount = (int) mObjects.size();
	aOut(aCount);
	for (std::list<Piece*>::iterator anItr = mObjects.begin(); anItr != mObjects.end(); ++anItr)
		WritePiece(&aBuffer, *anItr);

	return mApp->WriteBufferToFile(RocketApp::SavePath(mApp->mMode), &aBuffer);
}

// 0x41b880: the mode's saved game is read into this board (which was made new for it). False when the file is missing
// or is not a save.
bool Board::Load()
{
	Buffer aBuffer;
	mLoadedPieces.clear();
	if (!mApp->ReadBufferFromFile(RocketApp::SavePath(mApp->mMode), &aBuffer))
		return false;
	SaveReader aIn = { &aBuffer };
	unsigned int aMagic;
	aIn(aMagic);
	if (aMagic != 0xFADA0002)
		return false;
	aIn(mApp->mMode);
	aIn(mApp->mDifficulty);
	aIn(mScore);
	aIn(mLevel);
	aIn(mState);
	mRand->SRand(aBuffer.ReadString());

	Piece** aGroups[] = { &mGrid[0][0], mTop, &mSpare, mLeft, mRight };
	int aCounts[] = { 54, 6, 1, 9, 9 };
	for (int aGroup = 0; aGroup < 5; aGroup++)
	{
		for (int i = 0; i < aCounts[aGroup]; i++)
		{
			Piece* aPiece = NULL;
			bool aPresent;
			aIn(aPresent);
			if (aPresent)
				aPiece = LoadPiece(&aBuffer);
			aGroups[aGroup][i] = aPiece;
		}
	}

	Fields(aIn);
	mPause = 0;

	int aCount;
	aIn(aCount);
	// (the pieces that moved about, which the constructor's list had none of: the list is cleared first as the original's is)
	mObjects.clear();
	for (int i = 0; i < aCount; i++)
		mObjects.push_back(LoadPiece(&aBuffer));
	ResolveLoadedPieces();
	AfterLoad();
	return true;
}

// 0x41b7e0: what a loaded game was in the middle of (a screen of the end of a level, the buying of rockets) is flagged in
// mCovered, mBuyRocketsState and mBuyRocketsPending and put up again here: the hint of a bonus item (mBuyRocketsPending 2) goes on to the buy-rockets screen, the level-up
// screen (mCovered) to the high scores after a game and to the question after a level, and the buy-rockets screen (mBuyRocketsState) to
// the next level.
void Board::AfterLoad()
{
	mLevelEndCountdown = 0;
	if (mBuyRocketsPending == 2)
		ShowBuyRockets();
	if (mCovered == 1)
	{
		if (mState == 2)
			mApp->ShowHighScores(0);
		else
			AfterLevelScreen();
	}
	if (mBuyRocketsState == 1)
		NextLevel();
	mCovered = 0;
	mBuyRocketsState = 0;
	mBuyRocketsPending = 0;
	if (mState != 4)
		mApp->mHead->GoToRest(true);
}
