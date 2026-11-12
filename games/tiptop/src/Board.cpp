// TipTop's board (TipTop.exe 0x401000-0x410d35, without the glyph blits 0x40a792-0x40aaf9 that are in Gui.cpp). See
// Board.h for the classes. The functions keep the original's order and shape; a comment above each names its address.

#include "Board.h"
#include "platform/GameOptions.h"

#include "App.h"
#include "Config.h"
#include "Game.h"
#include "GameText.h"
#include "Gui.h"
#include "Lowos.h"
#include "Res.h"

#include <cmath>
#include <cstdlib>
#include <cstring>

namespace tiptop
{

// SHARED: the App method at 0x41bbce (called when the blown-away blocks have all gone): it counts the level
// summary's events (App::mSummaryEvents, +0x140) and starts the summary. The App's session names and defines it.
void AppNextSummaryEvent(App* theApp);

namespace
{

const int32_t kFailure = (int32_t)0x80004005;  // the E_FAIL the original's functions return

// MSVC's _ftol (0x44d33c): the x87 value truncated to 64 bits; the code uses the low 32
inline int64_t Ftol(double theValue)
{
	return (int64_t)theValue;
}

// The original's `throw` of nothing, which no handler catches
void Throw()
{
	abort();
}

// Plays sound theId of group 2 if it is loaded, at the game's volume. theMode is LOWOS_PlaySound's: 0 stops it, 1 starts it
// over, 2 starts it unless it plays (the original repeats these three lines at each use)
void Play(int theId, int theMode)
{
	if (gSounds[theId] != nullptr)
		lowos::PlaySound(gSounds[theId], (uint32_t)gSoundVolume << 16, (char)theMode);
}

// A random number in [theMin, theMax] the way the original computes it (rand % (max - min + 1) + min, unsigned)
uint32_t RandomIn(int32_t theMin, int32_t theMax)
{
	uint32_t aRandom = (uint32_t)Rand();
	return aRandom % ((uint32_t)(theMax - theMin) + 1u) + (uint32_t)theMin;
}

// The flags a removal gets from two settings of the configuration (the original spells them out at each use):
// PF_NOFALL when theFall is off and PF_NOFLOATS when theCreateFloats is off
uint16_t RemovalFlags(char theCreateFloats, char theFall)
{
	return (uint16_t)((((-(int)(theCreateFloats != 0)) & 0xffc0) + 0x40) | (((-(int)(theFall != 0)) & 0xffe0) + 0x20));
}

} // namespace

// --- Piece ---

// 0x401000: a block that is nothing yet. The original leaves mTargetY, mPower, mGroup, mFlyTime and mFlyDelay as
// the heap had them; they are zero here.
Piece::Piece()
{
	mKind = KIND_NONE;
	mMarked = 0;
	mSpeed = 0;
	mMoveTime = 0;
	mTargetY = 0;
	mX = 0;
	mY = 0;
	mFrame = 0;
	mSeed = (uint32_t)Rand();
	mDelay = 0;
	mFlags = 0;
	mPower = 0;
	mGroup = 0;
	mFlyTime = 0;
	mFlyDelay = 0;
	mShakeX = 0;
	mShakeY = 0;
	mShakeTargetX = 0;
	mShakeTargetY = 0;
	mShakeTime = GameTime();
	mEffectKind = KIND_NONE;
	mEffectDelay = 0;
	mCrossed = 0;
}

// 0x401099: a copy
Piece::Piece(const Piece& theOther)
{
	mKind = theOther.mKind;
	mMarked = theOther.mMarked;
	mSpeed = theOther.mSpeed;
	mMoveTime = theOther.mMoveTime;
	mTargetY = theOther.mTargetY;
	mX = theOther.mX;
	mY = theOther.mY;
	mFrame = theOther.mFrame;
	mSeed = theOther.mSeed;
	mDelay = theOther.mDelay;
	mFlags = theOther.mFlags;
	mPower = theOther.mPower;
	mGroup = theOther.mGroup;
	mFlyTime = theOther.mFlyTime;
	mFlyDelay = theOther.mFlyDelay;
	mShakeX = theOther.mShakeX;
	mShakeY = theOther.mShakeY;
	mShakeTargetX = theOther.mShakeTargetX;
	mShakeTargetY = theOther.mShakeTargetY;
	mShakeTime = theOther.mShakeTime;
	mEffectKind = theOther.mEffectKind;
	mEffectDelay = theOther.mEffectDelay;
	mCrossed = theOther.mCrossed;
}

// 0x4011bb: nothing to undo (0x403bbb is the deleting form: delete)
Piece::~Piece()
{
}

// 0x4011c4: assignment
Piece& Piece::operator=(const Piece& theOther)
{
	mMarked = theOther.mMarked;
	mMoveTime = theOther.mMoveTime;
	mSpeed = theOther.mSpeed;
	mPower = theOther.mPower;
	mFlags = theOther.mFlags;
	mKind = theOther.mKind;
	mDelay = theOther.mDelay;
	mTargetY = theOther.mTargetY;
	mFrame = theOther.mFrame;
	mSeed = theOther.mSeed;
	mFlyTime = theOther.mFlyTime;
	mFlyDelay = theOther.mFlyDelay;
	mShakeX = theOther.mShakeX;
	mShakeY = theOther.mShakeY;
	mX = theOther.mX;
	mGroup = theOther.mGroup;
	mShakeTargetX = theOther.mShakeTargetX;
	mShakeTargetY = theOther.mShakeTargetY;
	mShakeTime = theOther.mShakeTime;
	mEffectKind = theOther.mEffectKind;
	mCrossed = theOther.mCrossed;
	mEffectDelay = theOther.mEffectDelay;
	mY = theOther.mY;
	return *this;
}

// 0x402c5d: starts a piece's removal: it dissolves from now on. A piece that is already dissolving is left alone.
void Piece::StartRemoval(Piece* thePiece, uint16_t theFlags, uint32_t theEffectKind, uint32_t theEffectDelay)
{
	if (thePiece != nullptr && (~thePiece->mFlags & PF_REMOVING) != 0)
	{
		thePiece->mFlags = thePiece->mFlags | PF_REMOVING;
		thePiece->mSeed = GameTime();
		thePiece->mFlags = thePiece->mFlags | theFlags;
		thePiece->mEffectKind = (int32_t)theEffectKind;
		thePiece->mEffectDelay = theEffectDelay;
	}
}

// --- Field ---

// 0x4012e6: a field of theColumns by theRows cells; theBlockSize is the width and height of a block in pixels
Field::Field(int theColumns, int theRows, const int32_t* theBlockSize)
{
	mScore = 0;
	mLockFall = 0;
	mLockSpawn = 0;
	// mNext is made by Piece::Piece (0x401000)
	mTotalTime = 0;
	mLevelTime = 0;
	mBestChain = 0;
	mBestMatch = 0;
	mLowRow = -1;
	mHighRow = -1;
	mState = nullptr;
	mPendingSpawns = 0;
	mFloaterCredit = 0;
	mPendingFloaters = 0;
	mRowDelay = 0;
	mClock = 0;
	mPlayTime = 0;
	if (theColumns == 0 || theRows == 0)
	{
		Throw();
	}
	else
	{
		mPieces = nullptr;
		mCount = theColumns * theRows;
		mColumns = theColumns;
		mRows = theRows;
		mBlockWidth = theBlockSize[0];
		mBlockHeight = theBlockSize[1];
	}
	mColumnWeights = (uint32_t*)malloc(mColumns << 2);
	if (mColumnWeights == nullptr)
	{
		NoMemory();
	}
}

// 0x40147e: the destructor (0x401455 is its deleting form: delete). Frees the pieces and the grid.
Field::~Field()
{
	FreePieces();
	free(mPieces);
	free(mColumnWeights);
}

// The chain counter theIndex. (The original addresses it as *(short*)(field + 0xdc + 2 * theIndex), so that index CHAIN_COUNT,
// NO_CHAIN in CheckGroup, would be the low half of mLowRow; no path of the game reaches it: CheckGroup is only
// called, from Update, for pieces that carry PF_CHAIN.)
int16_t& Field::Chain(int32_t theIndex)
{
	return mChains[theIndex];
}

// 0x4014e1: the piece in a cell, 0 if it is empty or off the grid
Piece* Field::PieceAt(uint32_t theIndex)
{
	if (theIndex < mCount)
		return mPieces[theIndex];
	return nullptr;
}

// 0x40150a: sets the board state this field reads its rules from; 0 clears it. Returns the one before (0 when clearing)
BoardState* Field::SetState(BoardState* theState)
{
	BoardState* aResult;
	if (theState == nullptr)
	{
		mState = nullptr;
		aResult = nullptr;
	}
	else
	{
		aResult = mState;
		mState = theState;
	}
	return aResult;
}

// 0x401547: the board state the field reads its rules from
BoardState* Field::GetState()
{
	return mState;
}

// 0x401559: fills the field with the blocks of a bonus round's fixed layout (the configuration's 0x80 cells), which
// fly in from the top in the order of their rows
int32_t Field::LoadLayout()
{
	uint32_t aTime = GameTime();
	uint32_t aLastRow = 0xffffffff;
	int32_t aDelay = -200;
	uint32_t anIndex = mCount;
	uint32_t aCell;
	while (aCell = anIndex, anIndex = aCell - 1, -1 < (int32_t)anIndex)
	{
		if (gConfig.mBonusLayout[anIndex] != KIND_NONE)
		{
			Piece* aPiece = new Piece();
			uint32_t aRow = anIndex / mColumns;
			uint32_t aColumn = anIndex % mColumns;
			aPiece->mX = aColumn * mBlockWidth;
			aPiece->mY = aRow * mBlockHeight * FIXED_ONE;
			aPiece->mFlags = PF_ENTERING;
			aPiece->mFlyTime = aTime;
			if (aRow != aLastRow)
				aDelay = aDelay + 200;
			int32_t aRandom = Rand();
			aPiece->mFlyDelay = aDelay + aRandom % 200;
			aPiece->mKind = gConfig.mBonusLayout[anIndex];
			mPieces[aRow * mColumns + aColumn] = aPiece;
			aLastRow = aRow;
		}
	}
	UpdateRows();
	return 0;
}

// 0x4016ae: starts a game on the field: a few rows of blocks (or a bonus round's layout) and the counters at zero.
// Fails (E_FAIL) without a board state or if the grid is already made.
int32_t Field::Reset()
{
	int32_t aResult;
	uint32_t aTime = GameTime();
	mQueueHead = 0;
	mQueueCount = 0;
	mColumnCounter = 0;
	mNext.mFlags = 0;
	mClearStart = 1;
	mPlayTime = 0;
	mLevelTime = 0;
	mPlayTime2 = 0;
	mLockFall = 0;
	mLockSpawn = 0;
	mLevelBestChain = 0;
	mLevelBestMatch = 0;
	mLevelStartScore = mScore;
	mFloaterCredit2 = 0;
	if (mState == nullptr)
	{
		aResult = kFailure;
	}
	else if (mPieces == nullptr)
	{
		ClearLockBit();
		mIdle = 0;
		mNextChain = 0;
		mPendingSpawns = 0;
		mFloaterCredit = 0;
		mPendingFloaters = 0;
		mLowRow = -1;
		mHighRow = -1;
		mPieces = (Piece**)malloc(mColumns * mRows * 4);
		for (uint32_t i = 0; i < mCount; i++)
			mPieces[i] = nullptr;
		if (gBonusRound == 0)
		{
			Config& aConfig = mState->mConfig;
			mCountdown[0] = RandomIn(CfgRange(aConfig, 0), CfgRange(aConfig, 1));
			mCountdown[1] = RandomIn(CfgRange(aConfig, 2), CfgRange(aConfig, 3));
			mCountdown[2] = RandomIn(CfgRange(aConfig, 4), CfgRange(aConfig, 5));
			mCountdown[3] = RandomIn(CfgRange(aConfig, 6), CfgRange(aConfig, 7));
			mCountdown[4] = RandomIn(CfgRange(aConfig, 8), CfgRange(aConfig, 9));
			mCountdown[5] = RandomIn(CfgRange(aConfig, 10), CfgRange(aConfig, 11));
			mCountdown[6] = RandomIn(CfgRange(aConfig, 12), CfgRange(aConfig, 13));
			mCountdown[7] = RandomIn(CfgRange(aConfig, 14), CfgRange(aConfig, 15));
			uint32_t aStartRows = CfgStartRows(aConfig);
			uint32_t aPieceCount = aStartRows * mColumns;
			int32_t anAbs;
			if (CfgStartRowsAdd(aConfig) < 1)
				anAbs = -CfgStartRowsAdd(aConfig);
			else
				anAbs = CfgStartRowsAdd(aConfig);
			if ((uint32_t)(mRows - anAbs) <= aStartRows)
			{
				int32_t anAbs2;
				if (CfgStartRowsAdd(aConfig) < 1)
					anAbs2 = -CfgStartRowsAdd(aConfig);
				else
					anAbs2 = CfgStartRowsAdd(aConfig);
				aStartRows = (mRows - anAbs2) - 1;
			}
			uint32_t aFirstRow = ((mRows >> 1) - ((aStartRows + 1) >> 1)) + 1 + CfgStartRowsAdd(aConfig);
			for (uint32_t i = 0; i < aPieceCount; i++)
			{
				Piece* aPiece = new Piece();
				uint32_t aRow = aFirstRow + i / mColumns;
				uint32_t aColumn = i % mColumns;
				aPiece->mX = aColumn * mBlockWidth;
				aPiece->mY = aRow * mBlockHeight * FIXED_ONE;
				aPiece->mFlags = PF_ENTERING;
				aPiece->mFlyTime = aTime;
				int32_t aRandom = Rand();
				aPiece->mFlyDelay = (((aFirstRow - 1) + aStartRows) - aRow) * 200 + aRandom % 200;
				int32_t aLeft;
				if (aColumn == 0)
					aLeft = KIND_NONE;
				else
					aLeft = mPieces[(aFirstRow * mColumns + i) - 1]->mKind;
				int32_t aBelow;
				if (aFirstRow < aRow)
					aBelow = mPieces[(aFirstRow * mColumns + i) - mColumns]->mKind;
				else
					aBelow = KIND_NONE;
				if (aConfig.mRandomBlocks == 0)
					aPiece->mKind = RandomKind(0, aLeft, aBelow);
				else
					aPiece->mKind = RandomKind(0, KIND_NONE, KIND_NONE);
				mPieces[aRow * mColumns + aColumn] = aPiece;
			}
			UpdateRows();
			aResult = 0;
		}
		else
		{
			aResult = LoadLayout();
		}
	}
	else
	{
		aResult = kFailure;
	}
	return aResult;
}

namespace
{

// 0x408fa9: the sound of a piece of kind theKind. theStart is 0 for the sound of it landing (it stops the others of
// its kind and starts its own), 1 for the sound of it starting to move; theFaller is 1 for the player's block
// (positive mSpeed), which has the other sounds of the exploder and of the plain blocks.
void PlayPieceSound(int theKind, char theStart, char theFaller)
{
	if (theStart == 0)
	{
		if (theKind == 10)
		{
			Play(0x1e, 0);
			Play(0x15, 1);
		}
		else if (theKind == 0xb)
		{
			if (theFaller == 0)
			{
				Play(0x20, 0);
				Play(0x17, 1);
			}
			else
			{
				Play(0x1f, 0);
				Play(0x16, 1);
			}
		}
		else if (theKind == 0xc)
		{
			Play(0x21, 0);
			Play(0x18, 1);
		}
		else if (theKind == 0xd)
		{
			Play(0x22, 0);
			Play(0x19, 1);
		}
		else if (theKind == 0xe)
		{
			Play(0x23, 0);
			Play(0x1a, 1);
		}
		else if (theKind == 0xf)
		{
			Play(0x24, 0);
			Play(0x1b, 1);
		}
		else if (theKind == 0x10)
		{
			Play(0x25, 0);
			Play(0x1c, 1);
		}
		else
		{
			if (theFaller == 0)
				Play(0xb, 0);
			else
				Play(0x1d, 0);
			Play(0x14, 1);
		}
	}
	else if (theKind == 10)
		Play(0x1e, 1);
	else if (theKind == 0xb)
	{
		if (theFaller == 0)
			Play(0x20, 1);
		else
			Play(0x1f, 1);
	}
	else if (theKind == 0xc)
		Play(0x21, 1);
	else if (theKind == 0xd)
		Play(0x22, 1);
	else if (theKind == 0xe)
		Play(0x23, 1);
	else if (theKind == 0xf)
		Play(0x24, 1);
	else if (theKind == 0x10)
		Play(0x25, 1);
	else if (theFaller == 0)
		Play(0xb, 1);
	else
		Play(0x1d, 1);
}

} // namespace

namespace
{

// The seven sounds of the matches that the original stops before it plays the one that fits (0x401cb4)
const int kMatchSounds[7] = {0x11, 0x12, 0x13, 0xd, 0xe, 0xf, 0x10};

// Plays theSound as the sound of a match: the others are stopped first
void PlayMatchSound(int theSound)
{
	for (int i = 0; i < 7; i++)
		Play(kMatchSounds[i], 0);
	Play(theSound, 1);
}

// Gives thePiece a new chain counter (the original spells this out wherever a piece is removed)
void AssignChain(Field* theField, Piece* thePiece)
{
	thePiece->mGroup = (int32_t)theField->mNextChain;
	theField->Chain(thePiece->mGroup) = 0;
	theField->mNextChain = theField->mNextChain + 1;
	if (theField->mNextChain == CHAIN_COUNT)
		theField->mNextChain = 0;
}

// The flag a removal gets when the configuration creates no floaters from it (without the PF_NOFALL half)
uint16_t NoFloatsFlag(char theCreateFloats)
{
	return (uint16_t)(((-(int)(theCreateFloats != 0)) & 0xffc0) + 0x40);
}

// Counts a special block's removal against the goal: one block less to clear, one more on the meter
void CountSpecial(BoardState* theState)
{
	if (theState->mBlocksLeft != 0)
		theState->mBlocksLeft = theState->mBlocksLeft - 1;
	theState->mMeter = (float)((double)theState->mMeter + 1.0);
}

} // namespace

// --- matches ---

// 0x401cb4: checks the group of same-kind blocks that theIndex belongs to, and if it is big enough removes it:
// sounds, score, the goal and the meter, the chain, the statistics and the score message. Unless theOnlyOne it goes
// on with the blocks above in the column. Returns the blocks removed.
uint32_t Field::CheckGroup(uint32_t theIndex, char theOnlyOne)
{
	int32_t aTotal = 0;
	Piece* aPiece = mPieces[theIndex];
	int32_t aChain = NO_CHAIN;
	uint8_t aFlags = 0;
	do
	{
		if ((aPiece->mFlags & (PF_MOVING | PF_REMOVING)) == 0)
		{
			uint32_t aCount = (uint32_t)FloodFill(theIndex, &aChain, &aFlags);
			if ((uint32_t)mState->mConfig.mGroupSize <= aCount)
			{
				if (Chain(aChain) == 0)
					Chain(aChain) = 1;
				if (gBonusRound == 0)
				{
					int32_t aBest;
					if (mBestChain < (uint32_t)(int32_t)Chain(aChain))
						aBest = (int32_t)Chain(aChain);
					else
						aBest = (int32_t)mBestChain;
					mBestChain = aBest;
					if (mLevelBestChain < (uint32_t)(int32_t)Chain(aChain))
						aBest = (int32_t)Chain(aChain);
					else
						aBest = (int32_t)mLevelBestChain;
					mLevelBestChain = aBest;
					uint32_t aMatch = aCount;
					if (aCount <= mBestMatch)
						aMatch = mBestMatch;
					mBestMatch = aMatch;
					aMatch = aCount;
					if (aCount <= mLevelBestMatch)
						aMatch = mLevelBestMatch;
					mLevelBestMatch = aMatch;
				}
				int16_t aStep = Chain(aChain);
				if (aStep == 1)
				{
					if (aCount == 3)
						Play(0xc, 1);
					else if (aCount == 4)
						PlayMatchSound(0x11);
					else if (aCount == 5)
						PlayMatchSound(0x12);
					else
						PlayMatchSound(0x13);
				}
				else if (aStep == 2)
					PlayMatchSound(0xd);
				else if (aStep == 3)
					PlayMatchSound(0xe);
				else if (aStep == 4)
					PlayMatchSound(0xf);
				else
					PlayMatchSound(0x10);
				if (((~aFlags & 0x40) != 0) || mState->mConfig.mFloaterMatchesAwardPoints != 0)
				{
					uint32_t aPoints = ScorePoints((int)aCount, (uint32_t)(int32_t)Chain(aChain));
					mScore = aPoints + mScore;
				}
				aTotal = aTotal + aCount;
				uint32_t aSumX = 0;
				uint32_t aSumY = 0;
				for (uint32_t i = 0; i < mCount; i++)
				{
					if (mPieces[i] != nullptr && mPieces[i]->mMarked != 0)
					{
						aSumX = aSumX + mPieces[i]->mX + mBlockWidth / 2;
						aSumY = mBlockHeight / 2 + aSumY + ((uint32_t)mPieces[i]->mY >> FIXED_SHIFT);
						Piece::StartRemoval(mPieces[i], PF_MATCHED, KIND_NONE, 0);
						mPieces[i]->mFlags = mPieces[i]->mFlags | (uint16_t)aFlags;
						mPieces[i]->mGroup = aChain;
					}
				}
				if (((~aFlags & 0x40) != 0) || mState->mConfig.mFloaterMatchesAwardPoints != 0)
				{
					int32_t aReduce = (int32_t)MeterPoints((int)aCount, (uint32_t)(int32_t)Chain(aChain));
					mState->mBlocksLeft = mState->mBlocksLeft - aReduce;
					int32_t aLeft;
					if (mState->mBlocksLeft < 0)
						aLeft = 0;
					else
						aLeft = mState->mBlocksLeft;
					mState->mBlocksLeft = aLeft;
					mState->mMeter = (float)((double)aReduce + (double)mState->mMeter);
				}
				aSumX = aSumX / aCount;
				aSumY = aSumY / aCount;
				gApp.Combo((int16_t)((int16_t)aSumX + (int16_t)gConfig.mBlockgridX),
					(int16_t)((int16_t)aSumY + (int16_t)gConfig.mBlockgridY), (uint8_t)aCount, (uint8_t)Chain(aChain));
				Chain(aChain) = Chain(aChain) + 1;
			}
			if ((int32_t)aCount < 2)
			{
				mPieces[theIndex]->mMarked = 0;
				if ((~mPieces[theIndex]->mFlags & PF_REMOVING) != 0)
					mPieces[theIndex]->mFlags = mPieces[theIndex]->mFlags & 0xff9f;
			}
			else
			{
				for (uint32_t i = 0; i < mCount; i++)
				{
					if (mPieces[i] != nullptr && mPieces[i]->mMarked != 0)
					{
						mPieces[i]->mMarked = 0;
						if ((~mPieces[i]->mFlags & PF_REMOVING) != 0)
							mPieces[i]->mFlags = mPieces[i]->mFlags & 0xff9f;
					}
				}
			}
		}
		theIndex = theIndex + mColumns;
	} while (theIndex < mCount && (aPiece = mPieces[theIndex], theOnlyOne == 0) && aPiece != nullptr);
	return aTotal;
}

// 0x4029c6: marks the group of blocks of the same kind around theIndex (the four neighbors, those that are at rest)
// and returns its size. *theChain becomes the chain counter of the group (the highest of its blocks that came with
// one, NO_CHAIN if none did), *theFlags gets the PF_NOFLOATS bit of those.
int32_t Field::FloodFill(uint32_t theIndex, int32_t* theChain, uint8_t* theFlags)
{
	int32_t aCount = 1;
	uint32_t aBelowIndex = 0;
	uint32_t aLeftIndex = 0;
	uint32_t aRightIndex = 0;
	uint32_t anAboveIndex = 0;
	Piece* aPiece = mPieces[theIndex];
	if (aPiece == nullptr)
	{
		aCount = 0;
	}
	else
	{
		Piece* aBelow;
		Piece* aLeft;
		Piece* aRight;
		Piece* anAbove;
		if (theIndex < mColumns)
		{
			aBelow = nullptr;
		}
		else
		{
			aBelowIndex = theIndex - mColumns;
			aBelow = mPieces[aBelowIndex];
		}
		if (theIndex == 0 || theIndex % mColumns == 0)
		{
			aLeft = nullptr;
		}
		else
		{
			aLeftIndex = theIndex - 1;
			aLeft = mPieces[aLeftIndex];
		}
		if (theIndex < mCount - 1U && theIndex % mColumns != mColumns - 1U)
		{
			aRightIndex = theIndex + 1;
			aRight = mPieces[aRightIndex];
		}
		else
		{
			aRight = nullptr;
		}
		if (theIndex < (uint32_t)(mCount - mColumns))
		{
			anAboveIndex = theIndex + mColumns;
			anAbove = mPieces[anAboveIndex];
		}
		else
		{
			anAbove = nullptr;
		}
		aPiece->mMarked = 1;
		if ((aPiece->mFlags & PF_CHAIN) != 0)
		{
			if (*theChain == NO_CHAIN || Chain(*theChain) < Chain(aPiece->mGroup))
				*theChain = aPiece->mGroup;
			aPiece->mFlags = aPiece->mFlags & 0xffef;
			*theFlags = *theFlags | ((uint8_t)aPiece->mFlags & 0x40);
		}
		if (aBelow != nullptr && aPiece->mKind == aBelow->mKind && aBelow->mMarked == 0 &&
			(aBelow->mFlags & (PF_MOVING | PF_REMOVING)) == 0)
		{
			aCount = FloodFill(aBelowIndex, theChain, theFlags);
			aCount = aCount + 1;
		}
		if (aLeft != nullptr && aPiece->mKind == aLeft->mKind && aLeft->mMarked == 0 &&
			(aLeft->mFlags & (PF_MOVING | PF_REMOVING)) == 0)
		{
			int32_t aSub = FloodFill(aLeftIndex, theChain, theFlags);
			aCount = aCount + aSub;
		}
		if (aRight != nullptr && aPiece->mKind == aRight->mKind && aRight->mMarked == 0 &&
			(aRight->mFlags & (PF_MOVING | PF_REMOVING)) == 0)
		{
			int32_t aSub = FloodFill(aRightIndex, theChain, theFlags);
			aCount = aCount + aSub;
		}
		if (anAbove != nullptr && aPiece->mKind == anAbove->mKind && anAbove->mMarked == 0 &&
			(anAbove->mFlags & (PF_MOVING | PF_REMOVING)) == 0)
		{
			int32_t aSub = FloodFill(anAboveIndex, theChain, theFlags);
			aCount = aCount + aSub;
		}
	}
	return aCount;
}

// 0x402cc0: a piece has come to rest (or hit something): a special block acts here (rocket, exploder, hammer, magnet,
// rainbow, color bomb, column nuke), and any piece plays its landing sound. Its PF_MOVING flag is cleared.
void Field::Land(Piece* thePiece)
{
	int16_t aX;  // local_10
	int16_t aY;  // local_e
	if ((~thePiece->mFlags & PF_TARGET) != 0)
	{
		uint32_t aHasRows = HasTapRows(thePiece->mKind, 0.0f < thePiece->mSpeed);
		if ((aHasRows & 0xff) == 0 || thePiece->mKind == KIND_EXPLODER || thePiece->mKind == KIND_RAINBOW ||
			thePiece->mKind == KIND_COLORBOMB || thePiece->mKind == KIND_COLUMNNUKE)
		{
			PlayPieceSound(thePiece->mKind, 0, 0.0f < thePiece->mSpeed);
		}
	}
	int32_t aKind = thePiece->mKind;
	if (aKind == KIND_ROCKET)
	{
		Piece::StartRemoval(thePiece, 0x60, KIND_NONE, 0);
		AssignChain(this, thePiece);
	}
	else if (aKind == KIND_EXPLODER)
	{
		if ((thePiece->mFlags & PF_REMOVING) == 0)
		{
			Piece::StartRemoval(thePiece, 0x60, KIND_NONE, 0);
			AssignChain(this, thePiece);
			for (uint32_t i = 0; i < mCount; i++)
			{
				if (mPieces[i] != nullptr)
				{
					int32_t aDX;
					int32_t aDY;
					int32_t aOtherX = mPieces[i]->mX;
					if (aOtherX == thePiece->mX || aOtherX - thePiece->mX < 0)
						aDX = -(mPieces[i]->mX - thePiece->mX);
					else
						aDX = mPieces[i]->mX - thePiece->mX;
					int32_t aOtherY = mPieces[i]->mY;
					if (aOtherY == thePiece->mY || aOtherY - thePiece->mY < 0)
						aDY = -(mPieces[i]->mY - thePiece->mY);
					else
						aDY = mPieces[i]->mY - thePiece->mY;
					double aScaledY = ((double)mBlockWidth / (double)mBlockHeight) * (double)(aDY / FIXED_ONE);
					double aSum = aScaledY * aScaledY + (double)aDX * (double)aDX;
					uint32_t aDistance = (uint32_t)Ftol(sqrt(aSum) - 0.5);
					if (aDistance <= (uint32_t)(mState->mConfig.mExploderRadius * mBlockWidth) &&
						(mPieces[i]->mFlags & PF_REMOVING) == 0)
					{
						if (thePiece->mSpeed <= 0.0f)
						{
							Piece::StartRemoval(mPieces[i], RemovalFlags(mState->mConfig.mFloatingExploderCreateFloats,
								mState->mConfig.mFloatingExploderFall), KIND_NONE, 0);
							AssignChain(this, mPieces[i]);
						}
						else
						{
							Piece::StartRemoval(mPieces[i], RemovalFlags(mState->mConfig.mFallingExploderCreateFloats,
								mState->mConfig.mFallingExploderFall), KIND_NONE, 0);
							AssignChain(this, mPieces[i]);
							if (mState->mConfig.mExploderPoints != 0)
								CountSpecial(mState);
						}
						mScore = mScore + gConfig.mScoreRemovedbyspecial;
					}
				}
			}
		}
	}
	else if (aKind == KIND_HAMMER)
	{
		Piece::StartRemoval(thePiece, 0x40, KIND_NONE, 0);
		AssignChain(this, thePiece);
	}
	else if (aKind == KIND_MAGNET)
	{
		Piece::StartRemoval(thePiece, 0x60, KIND_NONE, 0);
		AssignChain(this, thePiece);
	}
	else if (aKind == KIND_RAINBOW)
	{
		if ((thePiece->mFlags & PF_REMOVING) == 0)
		{
			uint32_t aThisIndex = IndexOf(thePiece);
			uint32_t aFound = Neighbor(aThisIndex, 1);
			if (aFound != 0xffffffff)
			{
				Move(aFound);
				int32_t aDistance = (gConfig.mTapSpeed + 1) * FIXED_ONE;
				aThisIndex = IndexOf(thePiece);
				aFound = Within(aThisIndex, aDistance);
			}
			if (aFound == 0xffffffff || 9 < mPieces[aFound]->mKind)
			{
				int32_t aDistance = (gConfig.mTapSpeed + 1) * -FIXED_ONE;
				aThisIndex = IndexOf(thePiece);
				aFound = Within(aThisIndex, aDistance);
				if (aFound == 0xffffffff || 9 < mPieces[aFound]->mKind)
				{
					Piece::StartRemoval(thePiece, 0x60, KIND_NONE, 0);
					AssignChain(this, thePiece);
				}
				else
				{
					thePiece->mKind = mPieces[aFound]->mKind;
				}
			}
			else
			{
				thePiece->mKind = mPieces[aFound]->mKind;
			}
		}
	}
	else if (aKind == KIND_COLORBOMB)
	{
		if ((thePiece->mFlags & PF_REMOVING) == 0)
		{
			Piece::StartRemoval(thePiece, 0x40, KIND_NONE, 0);
			AssignChain(this, thePiece);
			int32_t anEffectDelay = gConfig.mColorbombEffects.mDelayStep;
			uint32_t aThisIndex = IndexOf(thePiece);
			uint32_t aFound = Neighbor(aThisIndex, 1);
			if (aFound != 0xffffffff)
			{
				Move(aFound);
				int32_t aDistance = (gConfig.mTapSpeed + 1) * FIXED_ONE;
				aThisIndex = IndexOf(thePiece);
				aFound = Within(aThisIndex, aDistance);
			}
			if (aFound != 0xffffffff)
			{
				uint32_t anIndex = 0;
				do
				{
					Piece* anOther = mPieces[anIndex];
					if (anOther != nullptr && (anOther->mFlags & PF_REMOVING) == 0 && anOther->mKind == mPieces[aFound]->mKind &&
						((anOther->mFlags & PF_MOVING) == 0 ||
							((((~anOther->mFlags & PF_TARGET) == 0 || anOther->mSpeed <= 0.0f) ||
								((anOther->mFlags & PF_CHAIN) == 0 || Chain(anOther->mGroup) != 1)) &&
								((~anOther->mFlags & PF_TARGET) == 0 || 0.0f <= anOther->mSpeed))))
					{
						anOther->mGroup = (int32_t)mNextChain;
						Chain(anOther->mGroup) = 0;
						mNextChain = mNextChain + 1;
						if (mNextChain == CHAIN_COUNT)
							mNextChain = 0;
						Piece::StartRemoval(anOther, RemovalFlags(mState->mConfig.mColorbombCreateFloats,
							mState->mConfig.mColorbombFall), KIND_COLORBOMB, anEffectDelay);
						mScore = mScore + gConfig.mScoreRemovedbyspecial;
						if (mState->mConfig.mColorbombPoints != 0)
							CountSpecial(mState);
						aX = (int16_t)(gConfig.mBlockWidth >> 1) + (int16_t)anOther->mX + (int16_t)gConfig.mBlockgridX;
						aY = (int16_t)((uint32_t)anOther->mY >> FIXED_SHIFT) + (int16_t)(gConfig.mBlockHeight >> 1) +
							(int16_t)gConfig.mBlockgridY;
						GuiRect aWhere = {aX, aY, 0, 0};
						gApp.Effects(&aWhere, anEffectDelay + (uint32_t)gConfig.mColorbombEffects.mFlashCount * FLASH_TIME,
							&gConfig.mColorbombEffects, gConfig.mColorbombEffects.mEffects);
						anEffectDelay = anEffectDelay + gConfig.mColorbombEffects.mDelayStep;
					}
					anIndex = anIndex + mColumns;
				} while (anIndex < mCount || (anIndex = (anIndex + 1) - mCount, anIndex < mColumns));
			}
		}
	}
	else if (aKind == KIND_COLUMNNUKE && (thePiece->mFlags & PF_REMOVING) == 0)
	{
		uint32_t aFlashCount = (uint32_t)gConfig.mColumnnukeEffects.mFlashCount;
		uint32_t aNext = IndexOf(thePiece);
		Piece::StartRemoval(thePiece, 0x60, KIND_NONE, 0);
		aX = (int16_t)(gConfig.mBlockWidth >> 1) + (int16_t)thePiece->mX + (int16_t)gConfig.mBlockgridX;
		aY = (int16_t)((uint32_t)thePiece->mY >> FIXED_SHIFT) + (int16_t)(gConfig.mBlockHeight >> 1) + (int16_t)gConfig.mBlockgridY;
		GuiRect aWhere = {aX, aY, 0, 0};
		gApp.Effects(&aWhere, aFlashCount * FLASH_TIME, &gConfig.mColumnnukeEffects, gConfig.mColumnnukeEffects.mEffects);
		int32_t anEffectDelay = aFlashCount * FLASH_TIME + gConfig.mColumnnukeEffects.mDelayStep;
		AssignChain(this, thePiece);
		while ((aNext = Neighbor(aNext, 1), aNext != 0xffffffff &&
			(((mPieces[aNext]->mFlags & PF_MOVING) == 0 || (~mPieces[aNext]->mFlags & PF_TARGET) == 0) ||
				0.0f <= mPieces[aNext]->mSpeed)))
		{
			if ((~mPieces[aNext]->mFlags & PF_REMOVING) != 0)
			{
				Piece::StartRemoval(mPieces[aNext], NoFloatsFlag(mState->mConfig.mColumnnukeCreateFloats), KIND_COLUMNNUKE, 0);
				mPieces[aNext]->mGroup = (int32_t)mNextChain;
				Chain(mPieces[aNext]->mGroup) = 0;
				mNextChain = mNextChain + 1;
				if (mNextChain == CHAIN_COUNT)
					mNextChain = 0;
				if (mState->mConfig.mColumnnukePoints != 0)
					CountSpecial(mState);
				mScore = mScore + gConfig.mScoreRemovedbyspecial;
			}
			aX = (int16_t)(gConfig.mBlockWidth >> 1) + (int16_t)mPieces[aNext]->mX + (int16_t)gConfig.mBlockgridX;
			aY = (int16_t)((uint32_t)mPieces[aNext]->mY >> FIXED_SHIFT) + (int16_t)(gConfig.mBlockHeight >> 1) +
				(int16_t)gConfig.mBlockgridY;
			GuiRect aWhere2 = {aX, aY, 0, 0};
			gApp.Effects(&aWhere2, anEffectDelay, &gConfig.mColumnnukeEffects, gConfig.mColumnnukeEffects.mEffects);
			anEffectDelay = anEffectDelay + gConfig.mColumnnukeEffects.mDelayStep;
		}
	}
	thePiece->mFlags = thePiece->mFlags & 0xfffe;
}

namespace
{

// Whether the srand of the first random kind has been done (0x4882c0)
char gRandomSeeded = 0;

// The test the original repeats where it looks over the cells above a removed piece (0x403be4): a cell that is empty,
// dissolving, or holds a moving block that is neither pushed to a target nor floating
bool SkipAbove(Piece* thePiece)
{
	return thePiece == nullptr || (~thePiece->mFlags & PF_REMOVING) == 0 ||
		(((~thePiece->mFlags & PF_MOVING) == 0 && (thePiece->mFlags & (PF_MOVING | PF_TARGET)) != (PF_MOVING | PF_TARGET)) &&
			(((~thePiece->mFlags & PF_TARGET) == 0 || (thePiece->mFlags & PF_MOVING) == 0) || thePiece->mSpeed <= 0.0f));
}

// The test the original repeats where it looks for the lowest or highest block at rest (0x40542a, 0x4055b0,
// 0x4057f7): true for a cell that does not count: empty, dissolving, a special block, or a block that is moving (except
// a floater just entered or a pushed one)
bool SkipForRow(Field* theField, Piece* thePiece)
{
	return thePiece == nullptr || (~thePiece->mFlags & PF_REMOVING) == 0 || 9 < thePiece->mKind ||
		(((~thePiece->mFlags & PF_MOVING) == 0 && (thePiece->mFlags & (PF_MOVING | PF_TARGET)) != (PF_MOVING | PF_TARGET)) &&
			((thePiece->mFlags & PF_CHAIN) == 0 || (theField->Chain(thePiece->mGroup) == 1 || thePiece->mSpeed <= 0.0f)));
}

// The test of a block that is on its way down (a floater that has entered) that 0x4047c7 repeats
bool IsFloatingDown(Piece* thePiece)
{
	return (~thePiece->mFlags & PF_MOVING) == 0 && ((thePiece->mFlags & PF_MOVING) == 0 || thePiece->mSpeed <= 0.0f);
}

} // namespace

// --- the grid ---

// 0x403b50: removes the piece in a cell
void Field::DeletePiece(uint32_t theIndex)
{
	if (theIndex < mCount && mPieces[theIndex] != nullptr)
	{
		Piece* aPiece = mPieces[theIndex];
		if (aPiece != nullptr)
			delete aPiece;  // 0x403bbb
		mPieces[theIndex] = nullptr;
	}
}

// 0x403be4: a piece has gone: the pieces below it start to move up ("secondary fall"), each after the one before
void Field::StartSecondaryFall(uint32_t theIndex)
{
	if (mState->mConfig.mSecondaryDistance != 0)
	{
		int32_t aChain = mPieces[theIndex]->mGroup;
		if (Chain(aChain) == 1)
		{
			aChain = (int32_t)mNextChain;
			Chain(aChain) = 0;
			mNextChain = mNextChain + 1;
			if (mNextChain == CHAIN_COUNT)
				mNextChain = 0;
		}
		uint32_t aTime = GameTime();
		int32_t aDelay = mState->mConfig.mSecondaryfallInitdelay + mRowDelay;
		uint32_t aFound = Within(theIndex, (gConfig.mTapSpeed + 1) * -FIXED_ONE);
		if (aFound != 0xffffffff)
			mRowDelay = mRowDelay + mState->mConfig.mSecondaryfallRowdelay;
		uint16_t aFlags = mPieces[theIndex]->mFlags;
		while (true)
		{
			theIndex = Within(theIndex, (gConfig.mTapSpeed + 1) * -FIXED_ONE);
			Piece* aPiece;
			if (theIndex == 0xffffffff)
				aPiece = nullptr;
			else
				aPiece = mPieces[theIndex];
			if (aPiece == nullptr || ((aPiece->mFlags & PF_MOVING) != 0 && (aPiece->mFlags & (PF_MOVING | PF_TARGET)) != 3))
				break;
			int32_t aDistance = mState->mConfig.mSecondaryDistance;
			uint32_t anAbove = theIndex;
			if (mState->mConfig.mSecondaryUnlimited != 0)
			{
				do
				{
					anAbove = anAbove + mColumns;
					if (mCount <= anAbove)
						goto AfterUnlimitedScan;
				} while (SkipAbove(mPieces[anAbove]));
				aDistance = -1;
			}
AfterUnlimitedScan:
			uint32_t aHigh;
			if (mHighRow == -1 && mLowRow != -1)
				aHigh = mRows - 1;
			else
				aHigh = mHighRow;
			uint32_t aLow;
			if (mLowRow == -1 && mHighRow != -1)
				aLow = 0;
			else
				aLow = mLowRow;
			uint32_t aStop;
			if ((aHigh == 0xffffffff || mState->mConfig.mSecondaryStopCenter != 0) && gBonusRound == 0)
			{
				if (CfgStopRow(mState->mConfig) == -1)
				{
					if (aHigh == aLow)
						aStop = (mRows >> 1) + CfgStartRowsAdd(mState->mConfig);
					else
						aStop = (int32_t)(aHigh + aLow) / 2 + CfgStopRowAdd(mState->mConfig);
				}
				else
				{
					aStop = CfgStopRow(mState->mConfig) + CfgStopRowAdd(mState->mConfig);
				}
			}
			else
			{
				aStop = mHighRow;
			}
			uint32_t aMin;
			if (aLow == aHigh || aLow == 0)
				aMin = 1;
			else
				aMin = aLow;
			uint32_t aClamped;
			if ((int32_t)aStop < (int32_t)aMin)
			{
				if (aLow == aHigh || aLow == 0)
					aClamped = 1;
				else
					aClamped = aLow;
			}
			else
			{
				aClamped = aStop;
			}
			uint32_t aMax;
			if (aLow == aHigh || aHigh == gConfig.mNumRows - 1U)
				aMax = gConfig.mNumRows - 2;
			else
				aMax = aHigh;
			uint32_t aTargetRow;
			if (aMax < aClamped)
			{
				if (aLow == aHigh || aHigh == gConfig.mNumRows - 1U)
					aTargetRow = gConfig.mNumRows - 2;
				else
					aTargetRow = aHigh;
			}
			else
			{
				aTargetRow = aClamped;
			}
			uint32_t aScan = theIndex;
			if (aTargetRow * mBlockHeight * FIXED_ONE <= (uint32_t)aPiece->mY)
			{
				do
				{
					aScan = aScan + mColumns;
					if (mCount <= aScan)
						return;
				} while (SkipAbove(mPieces[aScan]));
			}
			if (aDistance == -1)
			{
				aPiece->mFlags = aPiece->mFlags | PF_MOVING;
				aPiece->mFlags = aPiece->mFlags & 0xfffd;
				aPiece->mSpeed = mState->mConfig.mSecondaryfallerMotion.mParam[0];
			}
			else
			{
				aPiece->mFlags = aPiece->mFlags | (PF_MOVING | PF_TARGET);
				aPiece->mSpeed = mState->mConfig.mSecondaryfallerMotion.mParam[0];
				uint32_t aThisIndex = IndexOf(aPiece);
				uint32_t aTargetY = (aThisIndex / mColumns) * mBlockHeight + mBlockHeight * aDistance;
				if ((int32_t)aTargetY < 0)
					aTargetY = 0;
				if ((uint32_t)(mBlockHeight * (mRows - 1)) < aTargetY)
					aTargetY = mBlockHeight * (mRows - 1);
				aPiece->mTargetY = aTargetY << FIXED_SHIFT;
			}
			aPiece->mPower = 0;
			aPiece->mDelay = aDelay;
			aPiece->mMoveTime = aTime;
			aPiece->mFlags = aPiece->mFlags | ((uint8_t)aFlags & 0x40) | PF_CHAIN;
			aPiece->mGroup = aChain;
			aDelay = aDelay + mState->mConfig.mSecondaryfallAdddelay;
		}
	}
}

// 0x40428c: the kind of the next block. theMode -1 is for the blocks that enter from the top, 1 for the player's next
// block, anything else for a plain color. The specials come when their countdown (mCountdown) reaches zero, each
// then draws a new one; otherwise a random color that is neither theNotA nor theNotB (0x11 for none).
int32_t Field::RandomKind(int32_t theMode, int32_t theNotA, int32_t theNotB)
{
	Config& aConfig = mState->mConfig;
	if (gRandomSeeded == 0)
	{
		// fixes mod, "seededBlocks": the original seeds again from its clock here, which makes a seed given at the start
		// change nothing; with the fix that seeding is left out
		if (!Platform::FixOn("seededBlocks"))
		{
			uint32_t aTime = GameTime();
			SRand(aTime);
		}
		gRandomSeeded = 1;
	}
	if (theMode == -1)
	{
		if (mCountdown[0] == 0 && CfgRange(aConfig, 1) != 0)
		{
			mCountdown[0] = RandomIn(CfgRange(aConfig, 0), CfgRange(aConfig, 1));
			return KIND_ROCKET;
		}
		if (mCountdown[2] == 0 && CfgRange(aConfig, 5) != 0)
		{
			mCountdown[2] = RandomIn(CfgRange(aConfig, 4), CfgRange(aConfig, 5));
			return KIND_EXPLODER;
		}
		if (mCountdown[0] != 0)
			mCountdown[0] = mCountdown[0] - 1;
		if (mCountdown[2] != 0)
			mCountdown[2] = mCountdown[2] - 1;
	}
	if (theMode == 1)
	{
		if (mCountdown[3] == 0 && CfgRange(aConfig, 7) != 0)
		{
			mCountdown[3] = RandomIn(CfgRange(aConfig, 6), CfgRange(aConfig, 7));
			return KIND_HAMMER;
		}
		if (mCountdown[1] == 0 && CfgRange(aConfig, 3) != 0)
		{
			mCountdown[1] = RandomIn(CfgRange(aConfig, 2), CfgRange(aConfig, 3));
			return KIND_EXPLODER;
		}
		if (mCountdown[4] == 0 && CfgRange(aConfig, 9) != 0)
		{
			mCountdown[4] = RandomIn(CfgRange(aConfig, 8), CfgRange(aConfig, 9));
			return KIND_MAGNET;
		}
		if (mCountdown[5] == 0 && CfgRange(aConfig, 11) != 0)
		{
			mCountdown[5] = RandomIn(CfgRange(aConfig, 10), CfgRange(aConfig, 11));
			return KIND_RAINBOW;
		}
		if (mCountdown[6] == 0 && CfgRange(aConfig, 13) != 0)
		{
			mCountdown[6] = RandomIn(CfgRange(aConfig, 12), CfgRange(aConfig, 13));
			return KIND_COLORBOMB;
		}
		if (mCountdown[7] == 0 && CfgRange(aConfig, 15) != 0)
		{
			mCountdown[7] = RandomIn(CfgRange(aConfig, 14), CfgRange(aConfig, 15));
			return KIND_COLUMNNUKE;
		}
		if (mCountdown[3] != 0)
			mCountdown[3] = mCountdown[3] - 1;
		if (mCountdown[1] != 0)
			mCountdown[1] = mCountdown[1] - 1;
		if (mCountdown[4] != 0)
			mCountdown[4] = mCountdown[4] - 1;
		if (mCountdown[5] != 0)
			mCountdown[5] = mCountdown[5] - 1;
		if (mCountdown[6] != 0)
			mCountdown[6] = mCountdown[6] - 1;
		if (mCountdown[7] != 0)
			mCountdown[7] = mCountdown[7] - 1;
	}
	int32_t aColors = CfgNumColors(aConfig);
	if (theNotA != KIND_NONE)
		aColors = aColors - 1;
	if (theNotB != KIND_NONE && theNotB != theNotA)
		aColors = aColors - 1;
	int32_t aSkip = Rand();
	aSkip = aSkip % aColors;
	int32_t aKind = 0;
	while (true)
	{
		for (; aKind == theNotA || aKind == theNotB; aKind = aKind + 1)
		{
		}
		if (aSkip == 0)
			break;
		aKind = aKind + 1;
		aSkip = aSkip - 1;
	}
	return aKind;
}

// 0x40476d: the cell a piece is in (its x and its y rounded to the nearest row)
uint32_t Field::IndexOf(Piece* thePiece)
{
	return (uint32_t)thePiece->mX / (uint32_t)mBlockWidth +
		((((uint32_t)thePiece->mY >> FIXED_SHIFT) + mBlockHeight / 2) / (uint32_t)mBlockHeight) * mColumns;
}

// 0x4047c7: prepares the next block (mNext) to enter from the top: its kind, and the column. The column is chosen by
// the game mode's floater determination: 0 at random, weighted by what is free under the top, 1 in turn, 2 from the
// queue of the columns the player has dropped in (the mirror of them).
void Field::PrepareNext()
{
	int32_t aKind = RandomKind(-1, KIND_NONE, KIND_NONE);
	uint32_t aTotal = 0;
	int32_t aDetermination = (&gConfig.mFloaterdeterminationNormal)[gGameMode];
	uint32_t aColumn = 0;
	if (aDetermination == 0)
	{
		for (aColumn = 0; aColumn < mColumns; aColumn++)
		{
			Piece* aTop = mPieces[(mRows - 1) * mColumns + aColumn];
			if (aTop == nullptr || IsFloatingDown(aTop))
			{
				Piece* aSecond = mPieces[(mRows - 2) * mColumns + aColumn];
				if (aSecond == nullptr || (uint32_t)aSecond->mY <= (uint32_t)(mBlockHeight * FIXED_ONE * (mRows - 2)) ||
					IsFloatingDown(aSecond))
				{
					if (aKind == KIND_ROCKET)
					{
						mColumnWeights[aColumn] = 1;
						aTotal = aTotal + 1;
					}
					else
					{
						int32_t aRow = mRows - 2;
						while ((uint32_t)((mRows - 2) - aRow) < 7 &&
							(mPieces[aRow * mColumns + aColumn] == nullptr || IsFloatingDown(mPieces[aRow * mColumns + aColumn])))
						{
							aRow = aRow - 1;
						}
						mColumnWeights[aColumn] = gConfig.mFloaterprobability[(mRows - 2) - aRow];
						aTotal = aTotal + mColumnWeights[aColumn];
					}
				}
				else
				{
					mColumnWeights[aColumn] = 0;
				}
			}
			else
			{
				mColumnWeights[aColumn] = 0;
			}
		}
		if (aTotal == 0)
		{
			uint32_t aRandom = (uint32_t)Rand();
			aColumn = aRandom % mColumns;
		}
		else
		{
			uint32_t aRandom = (uint32_t)Rand();
			uint32_t aPick = aRandom % aTotal + 1;
			for (aColumn = 0; aColumn < mColumns && mColumnWeights[aColumn] < aPick; aColumn++)
			{
				aPick = aPick - mColumnWeights[aColumn];
			}
		}
	}
	else if (aDetermination == 1)
	{
		aColumn = mColumnCounter;
		mColumnCounter = mColumnCounter + 1;
		mColumnCounter = mColumnCounter % gConfig.mNumColumns;
	}
	else if (aDetermination == 2)
	{
		if (mQueueCount != 0)
		{
			mQueueCount = mQueueCount - 1;
			mQueueHead = mQueueHead + 1;
			mQueueHead = mQueueHead & 0x3f;
		}
		if (mQueueCount == 0)
		{
			int32_t aSlider = (int32_t)Send(gApp.mColumnCursor, MSG_CURSOR_COLUMN, 0, 0);
			aColumn = (gConfig.mNumColumns - 1) - aSlider;
		}
		else
		{
			aColumn = mQueue[mQueueHead];
			mQueueHead = mQueueHead + 1;
			mQueueHead = mQueueHead & 0x3f;
			mQueueCount = mQueueCount - 1;
		}
	}
	mNext.mX = aColumn * mBlockWidth;
	mNext.mY = (mRows - 1) * mBlockHeight * FIXED_ONE;
	mNext.mKind = aKind;
	mNext.mFlags = PF_PREVIEW;
	mNext.mMoveTime = GameTime();
	mNext.mSpeed = 0;
	mNext.mSeed = (uint32_t)Rand();
}

// 0x404ce5: lets the next block (mNext) enter from the top if its column is free; returns 0 when it did, 1 when the
// entry is delayed (or a bonus round, whose blocks do not enter)
uint32_t Field::Spawn()
{
	uint32_t aResult;
	if (gBonusRound == 0)
	{
		if (mLockSpawn == 0)
		{
			if ((~mNext.mFlags & PF_PREVIEW) != 0)
				PrepareNext();
			uint32_t aColumn = (uint32_t)mNext.mX / (uint32_t)mBlockWidth;
			int32_t aRows = mRows;
			if (mPieces[(aRows - 1) * mColumns + aColumn] == nullptr &&
				(mPieces[(aRows - 2) * mColumns + aColumn] == nullptr ||
					(uint32_t)mPieces[(aRows - 2) * mColumns + aColumn]->mY <= (uint32_t)(mBlockHeight * FIXED_ONE * (mRows - 2))))
			{
				Piece* aPiece = new Piece(mNext);
				aPiece->mFlags = aPiece->mFlags & 0xfff9;
				aPiece->mFlags = aPiece->mFlags | PF_MOVING;
				if (mState->mConfig.mFloatersMatch != 0)
				{
					aPiece->mFlags = aPiece->mFlags | PF_CHAIN;
					if (mState->mConfig.mFloaterMatchesCreateFloats == 0)
						aPiece->mFlags = aPiece->mFlags | PF_NOFLOATS;
					aPiece->mGroup = (int32_t)mNextChain;
					Chain(aPiece->mGroup) = 1;
					mNextChain = mNextChain + 1;
					if (mNextChain == CHAIN_COUNT)
						mNextChain = 0;
				}
				Motion* aMotion = MotionOf(aPiece->mKind, 0);
				aPiece->mSpeed = -aMotion->mParam[0];
				if (aPiece->mKind == KIND_ROCKET)
				{
					int32_t aPower;
					if (mState->mConfig.mRocketTapRows == 0)
						aPower = 0;
					else
						aPower = mState->mConfig.mSecondaryCarry + 1;
					aPiece->mPower = aPower;
				}
				else if (aPiece->mKind == KIND_EXPLODER)
				{
					int32_t aPower;
					if (mState->mConfig.mExploderTapRows == 0)
						aPower = 0;
					else
						aPower = mState->mConfig.mSecondaryCarry + 1;
					aPiece->mPower = aPower;
				}
				else
				{
					int32_t aPower;
					if (mState->mConfig.mFloaterTapRows == 0)
						aPower = 0;
					else
						aPower = mState->mConfig.mSecondaryCarry + 1;
					aPiece->mPower = aPower;
				}
				aPiece->mMoveTime = GameTime();
				mIdle = 0;
				PlayPieceSound(aPiece->mKind, 1, 0);
				mPieces[(aRows - 1) * mColumns + aColumn] = aPiece;
				PrepareNext();
				if (gConfig.mTurnbased != 0)
				{
					mLockFall = mLockFall | 4;
					mLockSpawn = mLockSpawn | 4;
				}
				aResult = 0;
			}
			else
			{
				QueueSpawn();
				aResult = 1;
			}
		}
		else
		{
			QueueSpawn();
			aResult = 1;
		}
	}
	else
	{
		aResult = 1;
	}
	return aResult;
}

// 0x40500e: a block could not enter: makes it wait (the first one for a random time)
void Field::QueueSpawn()
{
	if (mPendingSpawns == 0)
	{
		uint32_t aRandom = (uint32_t)Rand();
		mSpawnDelay = aRandom % (mState->mConfig.mDequeueFloatDelay + 1U);
	}
	mPendingSpawns = mPendingSpawns + 1;
}

// 0x405058: puts the player's block (a copy of a preview block) in the bottom of its column, if that is free; it
// moves up from there. Locks the player or the floaters as the configuration says and earns floaters. Returns 0.
uint32_t Field::Place(Piece* thePiece)
{
	if (thePiece->mKind != KIND_NONE)
	{
		uint32_t aColumn = (uint32_t)thePiece->mX / (uint32_t)mBlockWidth;
		uint32_t aFree = CanPlace(aColumn);
		if (aFree != 0)
		{
			Piece* aCopy = new Piece(*thePiece);
			mPieces[aColumn] = aCopy;
			Piece* aPlaced = mPieces[aColumn];
			aPlaced->mFlags = aPlaced->mFlags & 0xfffb;
			aPlaced->mFlags = aPlaced->mFlags | PF_CHAIN;
			aPlaced->mGroup = (int32_t)mNextChain;
			Chain(aPlaced->mGroup) = 1;
			mNextChain = mNextChain + 1;
			if (mNextChain == CHAIN_COUNT)
				mNextChain = 0;
			if (mState->mConfig.mLockFallers != 0)
				mLockFall = mLockFall | 1;
			if (mState->mConfig.mLockFloaters != 0)
				mLockSpawn = mLockSpawn | 1;
			if (gConfig.mTurnbased != 0)
			{
				mLockFall = mLockFall | 4;
				mLockSpawn = mLockSpawn | 4;
			}
			PlayPieceSound(aPlaced->mKind, 1, 1);
			mFloaterCredit = (float)((double)mFloaterCredit + (double)CfgFloatsPerTap(mState->mConfig));
			while (1.0 <= (double)mFloaterCredit)
			{
				mFloaterCredit = (float)((double)mFloaterCredit - 1.0);
				if ((&gConfig.mFloaterdeterminationNormal)[gGameMode] == 2)
				{
					uint32_t aSlot = mQueueHead + mQueueCount & 0x3f;
					if (mQueueCount < 0x3e)
					{
						mPendingFloaters = mPendingFloaters + 1;
						mQueueCount = mQueueCount + 1;
						mQueue[aSlot] = (uint8_t)(((char)gConfig.mNumColumns - 1) - (char)aColumn);
						if (mQueueCount == 1)
						{
							mNext.mX = ((gConfig.mNumColumns - 1) - aColumn) * mBlockWidth;
						}
						else
						{
							mQueueCount = mQueueCount + 1;
							mQueue[(aSlot + 1) & 0x3f] = (uint8_t)(((char)gConfig.mNumColumns - 1) - (char)aColumn);
						}
					}
				}
				else
				{
					mPendingFloaters = mPendingFloaters + 1;
				}
			}
			mIdle = 0;
		}
	}
	return 0;
}

// 0x40533e: whether the player's block can go in a column: the bottom cell is empty and nothing is rising out of
// the cell above it
uint32_t Field::CanPlace(int theColumn)
{
	uint32_t aResult;
	if (mPieces[theColumn] == nullptr)
	{
		if (mPieces[theColumn + mColumns] == nullptr || (uint32_t)(mBlockHeight << FIXED_SHIFT) <= (uint32_t)mPieces[theColumn + mColumns]->mY)
			aResult = 1;
		else
			aResult = 0;
	}
	else
	{
		aResult = 0;
	}
	return aResult;
}

// 0x4053a4: whether a block can enter at the top of a column: its top two cells are free
uint32_t Field::CanSpawn(int theColumn)
{
	uint32_t aResult;
	if (mPieces[(mRows - 1) * mColumns + theColumn] == nullptr)
	{
		if (mPieces[(mRows - 2) * mColumns + theColumn] == nullptr ||
			(uint32_t)(mBlockHeight << FIXED_SHIFT) <= (uint32_t)mPieces[(mRows - 2) * mColumns + theColumn]->mY)
			aResult = 1;
		else
			aResult = 0;
	}
	else
	{
		aResult = 0;
	}
	return aResult;
}

// 0x40542a: finds mLowRow, the row below the lowest block at rest (-1 when that is the bottom row); tells the app the
// position of the block if it is in the first row
void Field::UpdateLowRow()
{
	uint32_t anIndex = 0;
	while (true)
	{
		if (mCount <= anIndex)
		{
			break;
		}
		if (!SkipForRow(this, mPieces[anIndex]))
		{
			if (anIndex < mColumns)
				gApp.SetPosition(mBlockWidth * anIndex + mBlockWidth / 2, mBlockHeight / 2);
			break;
		}
		anIndex = anIndex + 1;
	}
	if (anIndex < mCount)
		mLowRow = anIndex / mColumns - 1;
	else
		mLowRow = mRows >> 1;
}

// 0x4055b0: finds mHighRow, the row above the highest block at rest (-1 when that is past the top row)
void Field::UpdateHighRow()
{
	uint32_t anIndex = mCount;
	do
	{
		anIndex = anIndex - 1;
		if ((int32_t)anIndex < 0)
			goto AfterHighestPiece;
	} while (SkipForRow(this, mPieces[anIndex]));
	if ((uint32_t)((mRows - 1) * mColumns) <= anIndex)
	{
		gApp.SetPosition(mBlockWidth * (anIndex % mColumns) + mBlockWidth / 2, mBlockHeight * (mRows - 1) + mBlockHeight / 2);
	}
AfterHighestPiece:
	if ((int32_t)anIndex < 0)
	{
		mHighRow = mRows >> 1;
	}
	else
	{
		mHighRow = anIndex / mColumns + 1;
		if (mRows <= (uint32_t)mHighRow)
			mHighRow = -1;
	}
}

// 0x405778: finds the lowest and highest rows at rest
void Field::UpdateRows()
{
	UpdateLowRow();
	UpdateHighRow();
}

// 0x405791: the next occupied cell above (theUp) or below a cell in its column, 0xffffffff if none
uint32_t Field::Neighbor(uint32_t theIndex, char theUp)
{
	do
	{
		if (theUp == 0)
		{
			if (theIndex < mColumns)
				return 0xffffffff;
			theIndex = theIndex - mColumns;
		}
		else
		{
			theIndex = theIndex + mColumns;
			if (mCount <= theIndex)
				return 0xffffffff;
		}
	} while (mPieces[theIndex] == nullptr);
	return theIndex;
}

// 0x4057f7: the row (y >> 8) of the first block at rest of a column, from the bottom (theFromBottom) or from the top,
// 0xffffffff if the column has none
uint32_t Field::EdgeRow(int theColumn, char theFromBottom)
{
	uint32_t anIndex = theColumn + (~-(uint32_t)(theFromBottom != 0) & (gConfig.mNumRows - 1) * gConfig.mNumColumns);
	while (SkipForRow(this, mPieces[anIndex]))
	{
		if (theFromBottom == 0)
		{
			if (anIndex < mColumns)
				return 0xffffffff;
			anIndex = anIndex - mColumns;
		}
		else
		{
			anIndex = anIndex + mColumns;
			if (mCount <= anIndex)
				return 0xffffffff;
		}
	}
	return (uint32_t)mPieces[anIndex]->mY >> FIXED_SHIFT;
}

// 0x405963: the neighbor of a cell above (theDistance >= 0) or below that is closer than theDistance (24.8) to it,
// 0xffffffff if none
uint32_t Field::Within(uint32_t theIndex, int theDistance)
{
	uint32_t aFound = Neighbor(theIndex, -1 < theDistance);
	if (aFound == 0xffffffff)
	{
		aFound = 0xffffffff;
	}
	else
	{
		int32_t aGap = mPieces[aFound]->mY - mPieces[theIndex]->mY;
		if (aGap < 1)
			aGap = -aGap;
		int32_t aLimit;
		if (theDistance < 1)
			aLimit = -theDistance;
		else
			aLimit = theDistance;
		if (aLimit <= aGap + mBlockHeight * -FIXED_ONE)
			aFound = 0xffffffff;
	}
	return aFound;
}

// --- collisions and shaking ---

// 0x405a19: theA, a piece that is moving, has reached the piece theB. Depending on their speeds and powers it passes it
// on (a special block acts, pushes the pieces in its way or hands over to them), lands, or sets both moving to the
// target rows that the kind of block pushes to. May recurse into the pieces it pushes.
void Field::Collide(int theA, uint32_t theB)
{
	Piece* aPiece = mPieces[theA];
	Piece* aHit = mPieces[theB];
	uint32_t aNext;
	if (0.0f < aPiece->mSpeed && (~aPiece->mFlags & PF_TARGET) != 0 && (aPiece->mFlags & PF_CHAIN) != 0 &&
		Chain(aPiece->mGroup) == 1 && mState->mConfig.mLockFloaters == 0)
	{
		while (mPendingFloaters != 0)
		{
			QueueSpawn();
			mPendingFloaters = mPendingFloaters - 1;
		}
	}
	if ((~aHit->mFlags & PF_MOVING) == 0)
	{
		if ((aHit->mSpeed < 0.0f && aPiece->mSpeed < 0.0f) || (0.0f < aHit->mSpeed && 0.0f < aPiece->mSpeed))
		{
			Move(theB);
			return;
		}
		if (aPiece->mPower <= aHit->mPower)
		{
			if (aPiece->mPower != aHit->mPower)
				return;
			Piece* aTargetA = nullptr;
			Piece* aTargetB = nullptr;
			if ((aPiece->mFlags & (PF_MOVING | PF_TARGET)) == 3)
				aTargetA = aPiece;
			if ((aHit->mFlags & (PF_MOVING | PF_TARGET)) == 3)
				aTargetB = aHit;
			if (aTargetA == nullptr || aTargetB != nullptr)
			{
				if (aTargetA == nullptr && aTargetB != nullptr)
					return;
				if ((uint32_t)aPiece->mY <= (uint32_t)aHit->mY)
					return;
			}
		}
	}
	else
	{
		if (aPiece->mPower == 0)
		{
			Land(aPiece);
			return;
		}
		if (aPiece->mPower != 1)
		{
			aHit->mFlags = aHit->mFlags | PF_MOVING;
			aHit->mFlags = aHit->mFlags & 0xfffd;
			aHit->mPower = aPiece->mPower - 1;
			aHit->mSpeed = aPiece->mSpeed;
			aHit->mMoveTime = aPiece->mMoveTime;
			int32_t aDirection;
			if (0.0f <= aPiece->mSpeed)
				aDirection = 1;
			else
				aDirection = -1;
			aNext = Within(theB, aDirection);
			if (aNext != 0xffffffff)
				Collide(theB, aNext);
			aPiece->mPower = 0;
			return;
		}
	}
	if ((~aPiece->mFlags & PF_TARGET) != 0 && aPiece->mPower != 0)
	{
		if (aPiece->mKind == KIND_ROCKET || aPiece->mKind == KIND_HAMMER || aPiece->mKind == KIND_MAGNET)
			PlayPieceSound(aPiece->mKind, 0, 0.0f < aPiece->mSpeed);
		else
			Play(0xa, 1);
	}
	aHit->mFlags = aHit->mFlags | (PF_MOVING | PF_TARGET);
	aHit->mPower = 0;
	float aSpeed;
	if (0.0f <= aPiece->mSpeed)
		aSpeed = (float)(double)(uint32_t)mState->mConfig.mTapSpeed;
	else
		aSpeed = (float)-(double)(uint32_t)mState->mConfig.mTapSpeed;
	aHit->mSpeed = aSpeed;
	aHit->mMoveTime = aPiece->mMoveTime;
	if ((aPiece->mFlags & PF_CHAIN) != 0 &&
		((~aHit->mFlags & PF_CHAIN) != 0 ||
			((aHit->mFlags & PF_CHAIN) != 0 && Chain(aPiece->mGroup) < Chain(aHit->mGroup))))
	{
		aHit->mGroup = aPiece->mGroup;
		aHit->mFlags = aHit->mFlags | PF_CHAIN;
	}
	uint32_t aHitIndex = IndexOf(aHit);
	uint32_t aTargetY = (aHitIndex / mColumns) * mBlockHeight;
	if (aPiece->mPower == 0)
	{
		if ((aPiece->mFlags & PF_TARGET) != 0)
		{
			if (0.0f < aPiece->mSpeed && (uint32_t)aPiece->mY < (uint32_t)aPiece->mTargetY &&
				(uint32_t)aPiece->mY < (uint32_t)aHit->mY)
			{
				uint32_t aRaised = ((uint32_t)aPiece->mTargetY >> FIXED_SHIFT) + mBlockHeight;
				if ((uint32_t)(mBlockHeight * (mRows - 1)) < aRaised)
					aRaised = mBlockHeight * (mRows - 1);
				aTargetY = aRaised;
			}
			if (aPiece->mSpeed < 0.0f && (uint32_t)aPiece->mTargetY < (uint32_t)aPiece->mY &&
				(uint32_t)aHit->mY < (uint32_t)aPiece->mY)
			{
				uint32_t aLowered = ((uint32_t)aPiece->mTargetY >> FIXED_SHIFT) - mBlockHeight;
				if ((int32_t)aLowered < 0)
					aLowered = 0;
				aTargetY = aLowered;
			}
		}
	}
	else
	{
		int32_t aKind = aPiece->mKind;
		Config& aConfig = mState->mConfig;
		if (aKind == KIND_ROCKET)
		{
			if (aPiece->mSpeed < 0.0f)
				aTargetY = aTargetY - mBlockHeight * aConfig.mRocketTapRows;
		}
		else if (aKind == KIND_EXPLODER)
		{
			int32_t anOffset;
			if (0.0f <= aPiece->mSpeed)
				anOffset = mBlockHeight * aConfig.mExploderTapRows;
			else
				anOffset = -(mBlockHeight * aConfig.mExploderTapRows);
			aTargetY = aTargetY + anOffset;
		}
		else if (aKind == KIND_HAMMER)
		{
			if (0.0f < aPiece->mSpeed)
				aTargetY = aTargetY + mBlockHeight * aConfig.mHammerTapRows;
		}
		else if (aKind == KIND_MAGNET)
		{
			aPiece->mSpeed = (float)(0.0 - (double)aPiece->mSpeed);
			aHit->mSpeed = (float)(0.0 - (double)aHit->mSpeed);
			int32_t anOffset;
			if (0.0f <= aPiece->mSpeed)
				anOffset = mBlockHeight * aConfig.mMagnetTapRows;
			else
				anOffset = -(mBlockHeight * aConfig.mMagnetTapRows);
			aTargetY = aTargetY + anOffset;
		}
		else if (aKind == KIND_RAINBOW)
		{
			if (0.0f < aPiece->mSpeed)
				aTargetY = aTargetY + mBlockHeight * aConfig.mRainbowTapRows;
		}
		else if (aKind == KIND_COLORBOMB)
		{
			if (0.0f < aPiece->mSpeed)
				aTargetY = aTargetY + mBlockHeight * aConfig.mColorbombTapRows;
		}
		else if (aKind == KIND_COLUMNNUKE)
		{
			if (0.0f < aPiece->mSpeed)
				aTargetY = aTargetY + mBlockHeight * aConfig.mColumnnukeTapRows;
		}
		else
		{
			int32_t anOffset;
			if (0.0f <= aPiece->mSpeed)
				anOffset = mBlockHeight * aConfig.mFallerTapRows;
			else
				anOffset = -(mBlockHeight * aConfig.mFloaterTapRows);
			aTargetY = aTargetY + anOffset;
		}
	}
	uint32_t aPieceTargetY;
	if ((uint32_t)aHit->mY < (uint32_t)aPiece->mY)
	{
		uint32_t aRaised = aTargetY + mBlockHeight;
		if ((uint32_t)(mBlockHeight * (mRows - 1)) < aRaised)
			aRaised = mBlockHeight * (mRows - 1);
		aPieceTargetY = aRaised;
	}
	else
	{
		uint32_t aLowered = aTargetY - mBlockHeight;
		if ((int32_t)aLowered < 0)
			aLowered = 0;
		aPieceTargetY = aLowered;
	}
	aPiece->mFlags = aPiece->mFlags | (PF_MOVING | PF_TARGET);
	aPiece->mSpeed = aHit->mSpeed;
	uint32_t aClamped;
	if ((int32_t)aTargetY < 0)
		aClamped = 0;
	else
		aClamped = aTargetY;
	uint32_t aLimited;
	if ((uint32_t)(mBlockHeight * (mRows - 1)) < aClamped)
		aLimited = mBlockHeight * (mRows - 1);
	else
		aLimited = aClamped;
	aTargetY = aLimited << FIXED_SHIFT;
	aPieceTargetY = aPieceTargetY << FIXED_SHIFT;
	if (aHit->mSpeed <= 0.0f)
	{
		if ((uint32_t)aHit->mY < aTargetY)
			aTargetY = (aHit->mY / (mBlockHeight << FIXED_SHIFT)) * mBlockHeight * FIXED_ONE;
	}
	else if (aTargetY < (uint32_t)aHit->mY)
	{
		aTargetY = ((mBlockHeight * FIXED_ONE + -1 + aHit->mY) / (mBlockHeight << FIXED_SHIFT)) * mBlockHeight * FIXED_ONE;
	}
	if (aPiece->mSpeed <= 0.0f)
	{
		if ((uint32_t)aPiece->mY < aPieceTargetY)
			aPieceTargetY = (aPiece->mY / (mBlockHeight << FIXED_SHIFT)) * mBlockHeight * FIXED_ONE;
	}
	else if (aPieceTargetY < (uint32_t)aPiece->mY)
	{
		aPieceTargetY = ((mBlockHeight * FIXED_ONE + -1 + aPiece->mY) / (mBlockHeight << FIXED_SHIFT)) * mBlockHeight * FIXED_ONE;
	}
	aHit->mTargetY = aTargetY;
	aPiece->mTargetY = aPieceTargetY;
	if (aPiece->mPower != 0)
	{
		if (aPiece->mKind == KIND_MAGNET && aPiece->mPower != 0)
			aHit->mSpeed = (float)(0.0 - (double)aHit->mSpeed);
		if (aPiece->mKind == KIND_MAGNET && aPiece->mPower != 0 && mState->mConfig.mMagnetEffect == 2)
		{
			aNext = Neighbor(theB, 0.0f <= aHit->mSpeed);
			if (aNext != 0xffffffff && (mPieces[aNext]->mFlags & PF_MOVING) != 0 &&
				(~mPieces[aNext]->mFlags & PF_TARGET) != 0 && mPieces[aNext]->mSpeed < 0.0f)
				aNext = 0xffffffff;
		}
		else
		{
			int32_t aDistance;
			if (0.0f <= aHit->mSpeed)
				aDistance = (gConfig.mTapSpeed + 1) * FIXED_ONE;
			else
				aDistance = (gConfig.mTapSpeed + 1) * -FIXED_ONE;
			aNext = Within(theB, aDistance);
		}
		if (aPiece->mKind == KIND_MAGNET && aPiece->mPower != 0)
			aHit->mSpeed = (float)(0.0 - (double)aHit->mSpeed);
		if (aNext != 0xffffffff)
		{
			if (aPiece->mKind == KIND_MAGNET && aPiece->mPower != 0)
				aHit->mSpeed = (float)(0.0 - (double)aHit->mSpeed);
			aHit->mPower = 1;
			int32_t aSavedKind = aHit->mKind;
			aHit->mKind = aPiece->mKind;
			Collide(theB, aNext);
			aHit->mKind = aSavedKind;
			aHit->mPower = 0;
		}
		aHit->mSpeed = aPiece->mSpeed;
		aPiece->mPower = 0;
	}
}

// 0x4065b2: stops the shaking of all pieces
void Field::ClearShake()
{
	for (uint32_t i = 0; i < mCount; i++)
	{
		if (mPieces[i] != nullptr)
		{
			mPieces[i]->mShakeTargetX = 0;
			mPieces[i]->mShakeTargetY = 0;
			mPieces[i]->mShakeX = 0;
			mPieces[i]->mShakeY = 0;
		}
	}
}

// 0x406634: picks where a piece shakes to next: how much depends on how near the top it is (the warning), whether it
// is one of the floaters about to enter, or being removed; the offset is random
void Field::ChooseShake(Piece* thePiece)
{
	if (thePiece->mKind < 10 || (~thePiece->mFlags & PF_PREVIEW) == 0)
	{
		uint32_t aRow = IndexOf(thePiece);
		aRow = aRow / mColumns;
		if ((mRows - 1) - aRow < 5)
			aRow = (mRows - 1) - aRow;
		uint32_t aShake;
		if (gBonusRound == 0)
		{
			uint32_t aWarning;
			if (aRow < 5)
				aWarning = (&gConfig.mWarningshake0)[aRow];
			else
				aWarning = 0;
			aShake = aWarning;
		}
		else
		{
			aShake = 0;
		}
		if ((((thePiece->mFlags & PF_MOVING) != 0 && (~thePiece->mFlags & PF_TARGET) != 0) &&
				(thePiece->mSpeed < 0.0f ||
					((thePiece->mFlags & PF_CHAIN) != 0 && Chain(thePiece->mGroup) == 1))) ||
			(thePiece->mFlags & PF_REMOVING) != 0)
		{
			aShake = 0;
		}
		if ((thePiece->mFlags & PF_PREVIEW) != 0)
			aShake = gConfig.mFloatershake;
		if ((thePiece->mFlags & PF_MATCHED) != 0 && aShake < (uint32_t)gConfig.mRemovingshake)
			aShake = gConfig.mRemovingshake;
		if (aShake == 0)
		{
			thePiece->mShakeTargetX = 0;
			thePiece->mShakeTargetY = 0;
		}
		else
		{
			int32_t aHeight = mBlockHeight;
			int32_t aWidth = mBlockWidth;
			uint32_t aRandomX = (uint32_t)Rand();
			uint32_t aRandomY = (uint32_t)Rand();
			thePiece->mShakeTargetX = (int32_t)(aShake * (aRandomX & 0x7fff)) / 0x8000;
			thePiece->mShakeTargetY =
				(int32_t)(((int32_t)(aShake * aHeight) / aWidth) * ((aRandomY & 0x7fff) - 0x4000)) / 0x8000;
		}
	}
	else
	{
		thePiece->mShakeTargetX = 0;
		thePiece->mShakeTargetY = 0;
	}
}

// 0x4067fb: moves a piece's shake toward its target (50 steps per ms of the step's size) and chooses the next target
// when the horizontal part has arrived
void Field::UpdateShake(Piece* thePiece)
{
	uint32_t aTime = GameTime();
	if (aTime < thePiece->mShakeTime)
		thePiece->mShakeTime = aTime;
	int32_t aStep = (aTime - thePiece->mShakeTime) * 0x32;
	thePiece->mShakeTime = aStep / 0x32 + thePiece->mShakeTime;
	if (0x10000 < aStep)
		aStep = 0x10000;
	int32_t aStepY = (aStep * mBlockHeight) / mBlockWidth;
	if (thePiece->mShakeTargetY * FIXED_ONE < thePiece->mShakeY)
	{
		thePiece->mShakeY = thePiece->mShakeY - aStepY;
		if (thePiece->mShakeY <= thePiece->mShakeTargetY * FIXED_ONE)
			thePiece->mShakeY = thePiece->mShakeTargetY << FIXED_SHIFT;
	}
	else
	{
		thePiece->mShakeY = thePiece->mShakeY + aStepY;
		if (thePiece->mShakeTargetY * FIXED_ONE <= thePiece->mShakeY)
			thePiece->mShakeY = thePiece->mShakeTargetY << FIXED_SHIFT;
	}
	if (thePiece->mShakeTargetX * FIXED_ONE < thePiece->mShakeX)
	{
		thePiece->mShakeX = thePiece->mShakeX - aStep;
		if (thePiece->mShakeX <= thePiece->mShakeTargetX * FIXED_ONE)
		{
			thePiece->mShakeX = thePiece->mShakeTargetX << FIXED_SHIFT;
			ChooseShake(thePiece);
		}
	}
	else
	{
		thePiece->mShakeX = thePiece->mShakeX + aStep;
		if (thePiece->mShakeTargetX * FIXED_ONE <= thePiece->mShakeX)
		{
			thePiece->mShakeX = thePiece->mShakeTargetX << FIXED_SHIFT;
			ChooseShake(thePiece);
		}
	}
}

namespace
{

// Set by Move when a piece headed for its target row is already past it; it stops all motion (0x488707, never reset)
char gMoveHalt = 0;

} // namespace

// --- motion ---

// 0x406974: moves the piece in a cell: the steps it is due (each the motion's time long), a step being its speed
// (24.8 per step) or less if a piece is in the way, in which case it collides (Collide), or the end of its motion.
// Also lands a piece that has reached its target, the top or the bottom, accelerates it, and for a dissolved piece
// (PF_GONE) starts the pieces below it, queues the floaters it earns and deletes it. Returns 0.
// The original is a maze of gotos: they stay, with the original's labels.
uint32_t Field::Move(uint32_t theIndex)
{
	Piece* aPiece;
	Piece* aBlocker;
	Piece* anOther;
	Motion* aMotion;
	uint32_t aStepTime;
	uint32_t aTime;
	uint32_t aFound;
	uint32_t aNewIndex;
	uint32_t aMagnetY;
	uint32_t aMagnetIndex;
	uint32_t aMagnetPrev;
	uint32_t aLastIndex;
	uint32_t aSearch;
	uint32_t aSearchNext;
	uint32_t aEndIndex;
	uint32_t aFrameTime;
	uint32_t aScan;
	uint32_t aHigh;
	uint32_t aLow;
	uint32_t aStop;
	uint32_t aMin;
	uint32_t aClamped;
	uint32_t aMax;
	uint32_t aTargetRow;
	int32_t aRemainder;
	int32_t aStepY;
	int32_t aGap;
	int32_t aAbsStep;
	int32_t aAbsGap;
	int32_t aMoved;
	int32_t aStopY;
	int32_t aNewY;
	int32_t aRoundedY;
	int32_t aStride;
	bool isOk;
	char aStopCenter;
	bool isFree;

	aPiece = mPieces[theIndex];
	if (aPiece != nullptr && gMoveHalt == 0)
	{
		if ((aPiece->mFlags & PF_MOVING) != 0)
		{
			if ((aPiece->mFlags & PF_TARGET) == 0 ||
				(((~aPiece->mFlags & PF_CHAIN) == 0 && ((aPiece->mFlags & PF_CHAIN) == 0 || Chain(aPiece->mGroup) != 1))))
			{
				aMotion = MotionOf(aPiece->mKind, 0.0f < aPiece->mSpeed);
				aStepTime = (uint32_t)aMotion->mTime;
			}
			else
			{
				aStepTime = mState->mConfig.mTapDelay;
			}
			aTime = GameTime();
			if (aTime < aPiece->mMoveTime)
				aPiece->mMoveTime = aTime;
			if (aPiece->mDelay == 0 || aPiece->mDelay < aTime - aPiece->mMoveTime)
			{
				aPiece->mMoveTime = aPiece->mMoveTime + aPiece->mDelay;
				aPiece->mDelay = 0;
			}
			else
			{
				aPiece->mDelay = aPiece->mDelay - (aTime - aPiece->mMoveTime);
				aPiece->mMoveTime = aTime;
			}
NextStep:
			if (aStepTime <= aTime - aPiece->mMoveTime && (aPiece->mFlags & PF_MOVING) != 0)
			{
				aRemainder = 0;
				aPiece->mMoveTime = aPiece->mMoveTime + aStepTime;
				if (aTime < aPiece->mMoveTime)
					aPiece->mMoveTime = aTime;
StepWithRemainder:
				aMoved = (int32_t)Ftol((double)aPiece->mSpeed * 256.0);
				if (0.0f <= aPiece->mSpeed)
				{
					aNewY = aMoved - aRemainder;
					if (aNewY < 0)
						aNewY = 0;
					aStepY = aNewY;
				}
				else
				{
					aNewY = aMoved + aRemainder;
					if (0 < aNewY)
						aNewY = 0;
					aStepY = aNewY;
				}
				if ((aPiece->mFlags & PF_TARGET) != 0)
				{
					aGap = aPiece->mTargetY - aPiece->mY;
					if (aStepY < 1)
						aAbsStep = -aStepY;
					else
						aAbsStep = aStepY;
					aAbsGap = aGap;
					if (aGap < 1)
						aAbsGap = -aGap;
					if (aAbsGap < aAbsStep)
						aStepY = aGap;
					if ((0.0f < aPiece->mSpeed && (uint32_t)aPiece->mTargetY < (uint32_t)aPiece->mY) ||
						(aPiece->mSpeed < 0.0f && (uint32_t)aPiece->mY < (uint32_t)aPiece->mTargetY))
					{
						gMoveHalt = 1;
						return 0;
					}
				}
				aFound = Within(theIndex, aStepY);
				if (aFound != 0xffffffff)
				{
					aBlocker = mPieces[aFound];
					if (0.0f <= aPiece->mSpeed)
					{
						aGap = (aBlocker->mY + mBlockHeight * -FIXED_ONE) - aPiece->mY;
						aPiece->mY = aBlocker->mY + mBlockHeight * -FIXED_ONE;
					}
					else
					{
						aGap = aPiece->mY - (aBlocker->mY + mBlockHeight * FIXED_ONE);
						aPiece->mY = mBlockHeight * FIXED_ONE + aBlocker->mY;
					}
					aRemainder = aRemainder + aGap;
					aNewIndex = IndexOf(aPiece);
					if (aNewIndex != theIndex)
					{
						mPieces[aNewIndex] = aPiece;
						mPieces[theIndex] = nullptr;
						theIndex = aNewIndex;
					}
					if ((~aBlocker->mFlags & PF_REMOVING) != 0)
					{
						if ((aPiece->mKind == KIND_MAGNET && aPiece->mPower != 0) && aPiece->mCrossed == 0)
						{
							aMagnetY = ((uint32_t)aPiece->mY >> FIXED_SHIFT) + gConfig.mBlockgridY;
							aMagnetIndex = aFound;
							do
							{
								aMagnetPrev = aMagnetIndex;
								if (mState->mConfig.mMagnetEffect == 0)
								{
									aMagnetIndex = Within(aMagnetIndex, (gConfig.mTapSpeed + 1) * FIXED_ONE);
								}
								else
								{
									aMagnetIndex = Neighbor(aMagnetIndex, 1);
									if (((aMagnetIndex != 0xffffffff) && ((mPieces[aMagnetIndex]->mFlags & PF_MOVING) != 0)) &&
										(((~mPieces[aMagnetIndex]->mFlags & PF_TARGET) != 0 &&
											(mPieces[aMagnetIndex]->mSpeed < 0.0f))))
									{
										aMagnetIndex = 0xffffffff;
									}
								}
							} while (aMagnetIndex != 0xffffffff);
							if (aMagnetY < ((uint32_t)mPieces[aMagnetPrev]->mY >> FIXED_SHIFT) + gConfig.mBlockHeight + gConfig.mBlockgridY)
							{
								gApp.Nothing();
								aPiece->mCrossed = 1;
							}
						}
						if (((aPiece->mKind == KIND_MAGNET && aPiece->mPower != 0)) &&
							(aSearch = aFound, aLastIndex = aFound, mState->mConfig.mMagnetEffect == 1))
						{
							do
							{
								aSearchNext = Within(aSearch, (gConfig.mTapSpeed + 1) * FIXED_ONE);
								if (aSearchNext == 0xffffffff)
								{
									aSearchNext = Neighbor(aSearch, 1);
									if (aSearchNext == 0xffffffff)
										goto MagnetCollision;
									if ((((~mPieces[aSearchNext]->mFlags & PF_MOVING) != 0) ||
											((mPieces[aSearchNext]->mFlags & PF_TARGET) != 0)) ||
										(0.0f <= mPieces[aSearchNext]->mSpeed))
									{
										aLastIndex = aSearchNext;
									}
								}
								aSearch = aSearchNext;
							} while (true);
						}
						Collide(theIndex, aFound);
					}
					goto AfterCollision;
				}
				aNewY = aPiece->mY + aStepY;
				if ((aPiece->mFlags & PF_TARGET) != 0 &&
					((~aPiece->mFlags & PF_CHAIN) != 0 ||
						((aPiece->mFlags & PF_CHAIN) != 0 && Chain(aPiece->mGroup) == 1)))
					goto ApplyNewY;
				if (mHighRow == -1 && mLowRow != -1)
					aHigh = mRows - 1;
				else
					aHigh = mHighRow;
				if (mLowRow == -1 && mHighRow != -1)
					aLow = 0;
				else
					aLow = mLowRow;
				if (0.0f <= aPiece->mSpeed)
				{
					if ((((aPiece->mFlags & PF_CHAIN) != 0 && Chain(aPiece->mGroup) == 1)) && mState->mConfig.mDisableStops != 0)
						goto ApplyNewY;
					if (gBonusRound == 0)
					{
						if (aHigh != 0xffffffff)
						{
							if ((aPiece->mFlags & PF_CHAIN) == 0 || Chain(aPiece->mGroup) != 1)
								aStopCenter = mState->mConfig.mSecondaryStopCenter;
							else
								aStopCenter = mState->mConfig.mPrimaryStopCenter;
							if (aStopCenter == 0)
								goto StopAtHighest;
						}
						if (CfgStopRow(mState->mConfig) == -1)
						{
							if (aHigh == aLow)
								aStop = (mRows >> 1) + CfgStartRowsAdd(mState->mConfig);
							else
								aStop = (int32_t)(aHigh + aLow) / 2 + CfgStopRowAdd(mState->mConfig);
						}
						else
						{
							aStop = CfgStopRow(mState->mConfig) + CfgStopRowAdd(mState->mConfig);
						}
					}
					else
					{
StopAtHighest:
						aStop = aHigh;
					}
				}
				else
				{
					if (mState->mConfig.mDisableStops != 0)
						goto ApplyNewY;
					if ((aLow == 0xffffffff || mState->mConfig.mPrimaryStopCenter != 0) && gBonusRound == 0)
					{
						if (CfgStopRow(mState->mConfig) == -1)
						{
							if (aHigh == aLow)
								aStop = (mRows >> 1) + CfgStartRowsAdd(mState->mConfig);
							else
								aStop = (int32_t)(aHigh + aLow) / 2 + CfgStopRowAdd(mState->mConfig);
						}
						else
						{
							aStop = CfgStopRow(mState->mConfig) + CfgStopRowAdd(mState->mConfig);
						}
					}
					else
					{
						aStop = aLow;
					}
				}
				if (aLow == aHigh || aLow == 0)
					aMin = 1;
				else
					aMin = aLow;
				if ((int32_t)aStop < (int32_t)aMin)
				{
					if (aLow == aHigh || aLow == 0)
						aClamped = 1;
					else
						aClamped = aLow;
				}
				else
				{
					aClamped = aStop;
				}
				if (aLow == aHigh || aHigh == gConfig.mNumRows - 1U)
					aMax = gConfig.mNumRows - 2;
				else
					aMax = aHigh;
				if (aMax < aClamped)
				{
					if (aLow == aHigh || aHigh == gConfig.mNumRows - 1U)
						aTargetRow = gConfig.mNumRows - 2;
					else
						aTargetRow = aHigh;
				}
				else
				{
					aTargetRow = aClamped;
				}
				aStopY = aTargetRow * mBlockHeight * FIXED_ONE;
				if (0.0f <= aPiece->mSpeed)
					isOk = aStopY <= aNewY;
				else
					isOk = aNewY <= aStopY;
				if (!isOk)
					goto ApplyNewY;
				aScan = theIndex;
				goto ScanForStop;
			}
		}
		if ((aPiece->mFlags & PF_GONE) == 0)
		{
			if ((~aPiece->mFlags & PF_REMOVING) != 0)
			{
				aFrameTime = GameTime();
				if (aFrameTime < aPiece->mSeed)
					aFrameTime = aFrameTime + 0x7fff;
				aPiece->mFrame = (aFrameTime - aPiece->mSeed) / 0x7d;
				aPiece->mFrame = (uint32_t)aPiece->mFrame % 4;
				if (aPiece->mFrame != 0)
					aPiece->mFrame = aPiece->mFrame + 5;
			}
			UpdateShake(aPiece);
		}
		else
		{
			isFree = true;
			aEndIndex = theIndex;
			do
			{
				aEndIndex = aEndIndex + mColumns;
				if (mCount <= aEndIndex)
					goto AboveScanned;
			} while (SkipAbove(mPieces[aEndIndex]));
			isFree = false;
AboveScanned:
			if (((~aPiece->mFlags & PF_NOFALL) != 0) || !isFree)
				StartSecondaryFall(theIndex);
			if (((~aPiece->mFlags & PF_NOFLOATS) != 0) && (&gConfig.mFloaterdeterminationNormal)[gGameMode] != 2)
			{
				mFloaterCredit2 = (float)((double)mFloaterCredit2 + (double)CfgFloatsPerRemoval(mState->mConfig));
				while (1.0 <= (double)mFloaterCredit2)
				{
					QueueSpawn();
					mFloaterCredit2 = (float)((double)mFloaterCredit2 - 1.0);
				}
			}
			DeletePiece(theIndex);
		}
	}
	return 0;
MagnetCollision:
	Collide(theIndex, aLastIndex);
	if (aLastIndex != aFound)
		Land(aPiece);
AfterCollision:
	if (mPieces[theIndex] == nullptr)
		return 0;
	aFound = Within(theIndex, (uint32_t)(-1 < aStepY) * 2 + -1);
	if (aFound != 0xffffffff || (aPiece->mFlags & PF_MOVING) == 0)
		goto AfterMove;
	goto StepWithRemainder;
ScanForStop:
	if (0.0f <= aPiece->mSpeed)
		aStride = mColumns;
	else
		aStride = -(int32_t)mColumns;
	aScan = aScan + aStride;
	if (((int32_t)aScan < 0) || (mCount <= aScan))
	{
		if (aPiece->mSpeed <= 0.0f)
		{
			aRoundedY = ((uint32_t)aPiece->mY / (uint32_t)(mBlockHeight << FIXED_SHIFT)) * mBlockHeight * FIXED_ONE;
			if (aStopY < aRoundedY)
				aRoundedY = aStopY;
			if (aRoundedY < aNewY)
				goto ApplyNewY;
		}
		else
		{
			aRoundedY = ((aPiece->mY + -1 + mBlockHeight * FIXED_ONE) / (mBlockHeight << FIXED_SHIFT)) * mBlockHeight * FIXED_ONE;
			if (aRoundedY < aStopY)
				aRoundedY = aStopY;
			if (aNewY < aRoundedY)
				goto ApplyNewY;
		}
		aStepY = aRoundedY - aPiece->mY;
		aNewY = aRoundedY;
		if ((((aPiece->mFlags & PF_CHAIN) != 0) && (Chain(aPiece->mGroup) == 1)) && (0.0f < aPiece->mSpeed))
		{
			while (mPendingFloaters != 0)
			{
				QueueSpawn();
				mPendingFloaters = mPendingFloaters - 1;
			}
		}
		Land(aPiece);
	}
	else
	{
		anOther = mPieces[aScan];
		if (anOther == nullptr || (~anOther->mFlags & PF_REMOVING) == 0)
			goto ScanForStop;
		if (((~anOther->mFlags & PF_MOVING) != 0) || ((anOther->mFlags & (PF_MOVING | PF_TARGET)) == 3))
			goto ApplyNewY;
		if (((~anOther->mFlags & PF_TARGET) == 0) || ((anOther->mFlags & PF_MOVING) == 0))
			goto ScanForStop;
		if (aPiece->mSpeed <= 0.0f)
			isOk = 0.0f <= anOther->mSpeed;
		else
			isOk = anOther->mSpeed <= 0.0f;
		if (isOk)
			goto ScanForStop;
	}
ApplyNewY:
	if (aNewY < 0)
	{
		aPiece->mY = 0;
		Land(aPiece);
		if (mState->mConfig.mDisableStops != 0)
		{
			Piece::StartRemoval(aPiece, 0x60, KIND_NONE, 0);
			aPiece->mGroup = (int32_t)mNextChain;
			Chain(aPiece->mGroup) = 0;
			mNextChain = mNextChain + 1;
			if (mNextChain == CHAIN_COUNT)
				mNextChain = 0;
		}
	}
	else
	{
		aPiece->mY = aNewY;
	}
	if ((uint32_t)(mBlockHeight * (mRows - 1) * FIXED_ONE) < (uint32_t)aPiece->mY)
	{
		aPiece->mY = mBlockHeight * (mRows - 1) * FIXED_ONE;
		Land(aPiece);
		if ((((aPiece->mFlags & PF_CHAIN) != 0) && (Chain(aPiece->mGroup) == 1)) && mState->mConfig.mDisableStops != 0)
		{
			Piece::StartRemoval(aPiece, 0x60, KIND_NONE, 0);
			aPiece->mGroup = (int32_t)mNextChain;
			Chain(aPiece->mGroup) = 0;
			mNextChain = mNextChain + 1;
			if (mNextChain == CHAIN_COUNT)
				mNextChain = 0;
			while (mPendingFloaters != 0)
			{
				QueueSpawn();
				mPendingFloaters = mPendingFloaters - 1;
			}
		}
	}
	aNewIndex = IndexOf(aPiece);
	if (aNewIndex != theIndex)
	{
		mPieces[aNewIndex] = aPiece;
		mPieces[theIndex] = nullptr;
		theIndex = aNewIndex;
	}
AfterMove:
	if (((aPiece->mFlags & (PF_MOVING | PF_TARGET)) == 3) && (aPiece->mY == aPiece->mTargetY))
		Land(aPiece);
	if (((aPiece->mFlags & PF_MOVING) != 0 && (~aPiece->mFlags & PF_TARGET) != 0) ||
		(((aPiece->mFlags & (PF_TARGET | PF_CHAIN)) == (PF_TARGET | PF_CHAIN)) && (Chain(aPiece->mGroup) != 1)))
	{
		if (aPiece->mSpeed <= 0.0f)
		{
			aMotion = MotionOf(aPiece->mKind, 0);
			aPiece->mSpeed = (float)((double)aPiece->mSpeed - (double)aMotion->mParam[1]);
			aMotion = MotionOf(aPiece->mKind, 0);
			if ((double)aPiece->mSpeed < -(double)aMotion->mParam[2])
			{
				aMotion = MotionOf(aPiece->mKind, 0);
				aPiece->mSpeed = -aMotion->mParam[2];
			}
		}
		else if ((aPiece->mFlags & PF_CHAIN) == 0 || Chain(aPiece->mGroup) != 1)
		{
			aPiece->mSpeed = (float)((double)gConfig.mSecondaryfallerMotion.mParam[1] + (double)aPiece->mSpeed);
			if (gConfig.mSecondaryfallerMotion.mParam[2] < aPiece->mSpeed)
				aPiece->mSpeed = gConfig.mSecondaryfallerMotion.mParam[2];
		}
		else
		{
			aMotion = MotionOf(aPiece->mKind, 1);
			aPiece->mSpeed = (float)((double)aPiece->mSpeed + (double)aMotion->mParam[1]);
			aMotion = MotionOf(aPiece->mKind, 1);
			if (aMotion->mParam[2] < aPiece->mSpeed)
			{
				aMotion = MotionOf(aPiece->mKind, 1);
				aPiece->mSpeed = aMotion->mParam[2];
			}
		}
	}
	goto NextStep;
}

// --- the update ---

// 0x407cc3: one update of the field: the clocks, whether anything moves (and so the locks), the matches of the chain
// pieces, the moves, the blocks that enter, and in the timed game the meter running down (faster the longer it has
// been played).
void Field::Update()
{
	int32_t aSpawnDelay;
	int32_t aState;
	uint32_t aNow;
	uint32_t anElapsed;
	float aRate;
	if (gMoveHalt == 0)
	{
		mRowDelay = mRowDelay - gConfig.mSecondaryfallRowdelay / 10;
		int32_t aRowDelay;
		if (mRowDelay < 0)
			aRowDelay = 0;
		else
			aRowDelay = mRowDelay;
		mRowDelay = aRowDelay;
		if (mClock == 0)
			mClock = GameTime();
		aNow = GameTime();
		if (aNow < mClock)
			mClock = aNow;
		mTotalTime = (aNow - mClock) + mTotalTime;
		mLevelTime = (aNow - mClock) + mLevelTime;
		aState = 2;
		for (uint32_t i = 0; i < mCount; i++)
		{
			if (mPieces[i] != nullptr)
			{
				if ((mPieces[i]->mFlags & (PF_MOVING | PF_REMOVING | PF_CHAIN)) != 0)
					aState = 1;
				if ((((mPieces[i]->mFlags & PF_MOVING) != 0) &&
						((((~mPieces[i]->mFlags & PF_TARGET) == 0 || (mPieces[i]->mSpeed <= 0.0f)) ||
							((mPieces[i]->mFlags & PF_CHAIN) == 0)) ||
							(Chain(mPieces[i]->mGroup) != 1))) &&
					(((~mPieces[i]->mFlags & PF_TARGET) == 0 || (0.0f <= mPieces[i]->mSpeed))))
				{
					aState = 0;
					break;
				}
			}
		}
		if (mPendingSpawns == 0 && aState == 2)
			mLockFall = mLockFall & 0xfffffffe;
		if (aState == 2)
		{
			mLockSpawn = mLockSpawn & 0xfffffffe;
			mLockSpawn = mLockSpawn & 0xfffffffb;
			while (mPendingFloaters != 0)
			{
				QueueSpawn();
				mPendingFloaters = mPendingFloaters - 1;
			}
			if (mPendingSpawns == 0)
				mLockFall = mLockFall & 0xfffffffb;
		}
		mIdle = aState == 2;
		if (aState == 1)
		{
			for (uint32_t i = 0; i < mCount; i++)
			{
				if (mPieces[i] != nullptr && (~mPieces[i]->mFlags & PF_CHAIN) == 0)
				{
					if (mState->mConfig.mMaxMatchOrdinal == 0 ||
						(uint32_t)(int32_t)Chain(mPieces[i]->mGroup) <= (uint32_t)mState->mConfig.mMaxMatchOrdinal)
					{
						CheckGroup(i, 1);
					}
					else
					{
						mPieces[i]->mFlags = mPieces[i]->mFlags & 0xff8f;
					}
				}
			}
		}
		for (uint32_t aColumn = 0; aColumn < mColumns; aColumn = (aColumn - mCount) + 1)
		{
			for (; aColumn < mCount; aColumn = aColumn + mColumns)
				Move(aColumn);
		}
		UpdateRows();
		aSpawnDelay = mSpawnDelay;
		while (mPendingSpawns != 0)
		{
			if ((int32_t)(aNow - mClock) < aSpawnDelay)
			{
				aSpawnDelay = aSpawnDelay - (aNow - mClock);
				break;
			}
			aSpawnDelay = aSpawnDelay - (aNow - mClock);
			mPendingSpawns = mPendingSpawns - 1;
			int32_t aSpawned = (int32_t)Spawn();
			if (aSpawned != 0)
				break;
			if (mPendingSpawns != 0)
			{
				uint32_t aRandom = (uint32_t)Rand();
				aSpawnDelay = aSpawnDelay + aRandom % (mState->mConfig.mDequeueFloatDelay + 1U);
			}
		}
		int32_t aStored;
		if (aSpawnDelay < 0)
			aStored = 0;
		else
			aStored = aSpawnDelay;
		mSpawnDelay = aStored;
		if (mLockFall == 0)
		{
			mPlayTime = (aNow - mClock) + mPlayTime;
			mPlayTime2 = (aNow - mClock) + mPlayTime2;
		}
		if (gGameMode == 1 && mLockFall == 0)
		{
			if (aNow < mState->mMeterTime)
				mState->mMeterTime = aNow;
			anElapsed = aNow - mState->mMeterTime;
			if (0x13 < anElapsed)
			{
				mState->mMeterTime = aNow;
				aRate = CfgMeterLoss(mState->mConfig);
				if ((uint32_t)CfgMeterDoublingTime(mState->mConfig) < mPlayTime)
				{
					double aInverse = 1.0 / (double)(uint32_t)CfgMeterDoublingTime(mState->mConfig);
					double aPlayed = (double)(uint32_t)(mPlayTime - CfgMeterDoublingTime(mState->mConfig));
					double aPower = pow(2.0, aInverse * aPlayed);
					aRate = (float)(aPower * (double)aRate);
				}
				mState->mMeter = (float)((double)mState->mMeter - ((double)anElapsed * (double)aRate) / 1000.0);
			}
		}
		mClock = aNow;
	}
}

// 0x4082d3: the blocks that are left to clear in the level
uint32_t Field::GetBlocksLeft()
{
	return mState->mBlocksLeft;
}

// 0x4082e8: the score
uint32_t Field::GetScore()
{
	return mScore;
}

// 0x4082f7: the points of the level so far
int32_t Field::GetLevelScore()
{
	return mScore - mLevelStartScore;
}

// 0x40830c: the points of a group of theCount blocks as the theChain-th of a chain (the configuration's table of 10 by
// 10: groups of 3 to 12 or more, chains of 1 to 10 or more)
uint32_t Field::ScorePoints(int theCount, uint32_t theChain)
{
	uint32_t aResult;
	if (theCount < 3)
	{
		aResult = 0;
	}
	else
	{
		theCount = theCount + -3;
		if (theChain != 0)
			theChain = theChain - 1;
		if (9 < theCount)
			theCount = 9;
		if (9 < theChain)
			theChain = 9;
		aResult = mState->mConfig.mScoretable[theCount + theChain * 10];
	}
	return aResult;
}

// 0x40836c: the same for what a group takes off the goal and adds to the meter
uint32_t Field::MeterPoints(int theCount, uint32_t theChain)
{
	uint32_t aResult;
	if (theCount < 3)
	{
		aResult = 0;
	}
	else
	{
		theCount = theCount + -3;
		if (theChain != 0)
			theChain = theChain - 1;
		if (9 < theCount)
			theCount = 9;
		if (9 < theChain)
			theChain = 9;
		aResult = mState->mConfig.mBlreducetable[theCount + theChain * 10];
	}
	return aResult;
}

// 0x4083cc: one step of the clearing animation at the end of a game: two sweeps, from the bottom and from the top,
// that fill the empty rows with dissolving blocks, row after row in zigzag; each empty row it passes scores. Returns 1
// when it is done.
uint32_t Field::ClearStep()
{
	uint32_t aResult;
	uint32_t aNow;
	uint32_t aPoints;
	if (mClearStart != 0)
	{
		mClearRowA = 0;
		mClearRowB = mRows - 1;
		mClearIndexA = 0;
		mClearIndexB = mCount - 1;
		mClearActiveA = 1;
		mClearActiveB = 1;
		mClearInterval = 0x32;
		mClearTime = GameTime();
		mClearPhaseB = 0;
		mClearStart = 0;
		mClearRight = 1;
		mClearReady = 1;
	}
	aNow = GameTime();
	if (mClearActiveB == 0 && mClearActiveA == 0)
	{
		mClearStart = 1;
		aResult = 1;
	}
	else
	{
		if (aNow < mClearTime)
			mClearTime = aNow;
		if (aNow - mClearTime < (uint32_t)mClearInterval)
		{
			aResult = 0;
		}
		else
		{
			mClearTime = mClearTime + mClearInterval;
			if (mClearPhaseB == 0 && mClearReady != 0)
			{
				mClearReady = 0;
				int32_t aStart;
				if (mClearRight == 0)
					aStart = mColumns - 1;
				else
					aStart = 0;
				mClearIndexA = mClearRowA * mColumns + aStart;
				if ((mRows + 1U) >> 1 <= (uint32_t)mClearRowA)
				{
					mClearActiveA = 0;
					mClearPhaseB = 1;
					mClearReady = 1;
					return 0;
				}
				for (uint32_t aColumn = 0; aColumn < mColumns; aColumn++)
				{
					int32_t aCell = mClearRowA * mColumns + aColumn;
					if (mPieces[aCell] != nullptr &&
						((~mPieces[aCell]->mFlags & PF_MOVING) != 0 || (mPieces[aCell]->mFlags & (PF_MOVING | PF_TARGET)) == 3))
					{
						mClearActiveA = 0;
						mClearPhaseB = 1;
						mClearReady = 1;
						return 0;
					}
				}
				if (mClearActiveA != 0)
				{
					aPoints = mState->mConfig.mScoreEmptyrowsInitial +
						mState->mConfig.mScoreEmptyrowsCoefficient * (mState->mLevel - 1);
					mScore = mScore + aPoints;
					Play(6, 1);
					gApp.ScoreMessage((int16_t)((int16_t)((mColumns * mBlockWidth) >> 1) + (int16_t)gConfig.mBlockgridX),
						(int16_t)((int16_t)mClearRowA * (int16_t)mBlockHeight + (int16_t)gConfig.mBlockgridY), aPoints, 0, 1);
				}
			}
			if (mClearPhaseB != 0 && mClearReady != 0)
			{
				mClearReady = 0;
				int32_t aStart;
				if (mClearRight == 0)
					aStart = mColumns - 1;
				else
					aStart = 0;
				mClearIndexB = mClearRowB * mColumns + aStart;
				if ((uint32_t)mClearRowB <= (mRows - 1U) >> 1)
				{
					mClearActiveB = 0;
					mClearPhaseB = 0;
					mClearReady = 1;
					return 0;
				}
				for (uint32_t aColumn = 0; aColumn < mColumns; aColumn++)
				{
					int32_t aCell = mClearRowB * mColumns + aColumn;
					if (mPieces[aCell] != nullptr &&
						((~mPieces[aCell]->mFlags & PF_MOVING) != 0 || (mPieces[aCell]->mFlags & (PF_MOVING | PF_TARGET)) == 3))
					{
						mClearActiveB = 0;
						mClearPhaseB = 0;
						mClearReady = 1;
						return 0;
					}
				}
				if (mClearActiveB != 0)
				{
					aPoints = mState->mConfig.mScoreEmptyrowsInitial +
						mState->mConfig.mScoreEmptyrowsCoefficient * (mState->mLevel - 1);
					mScore = mScore + aPoints;
					Play(6, 1);
					gApp.ScoreMessage((int16_t)((int16_t)((mColumns * mBlockWidth) >> 1) + (int16_t)gConfig.mBlockgridX),
						(int16_t)((int16_t)mClearRowB * (int16_t)mBlockHeight + (int16_t)gConfig.mBlockgridY), aPoints, 0, 1);
				}
			}
			if (mClearPhaseB == 0 && mClearActiveA != 0)
			{
				uint32_t anIndex = mClearIndexA;
				uint32_t aColumns = mColumns;
				Piece* aPiece = new Piece();
				aPiece->mX = (anIndex % aColumns) * mBlockWidth;
				aPiece->mY = mClearRowA * mBlockHeight * FIXED_ONE;
				aPiece->mKind = (uint32_t)mClearRowA % (uint32_t)CfgNumColors(mState->mConfig);
				aPiece->mFlags = PF_REMOVING;
				aPiece->mEffectKind = KIND_NONE;
				aPiece->mSeed = GameTime();
				aPiece->mSpeed = 0;
				mPieces[mClearIndexA] = aPiece;
			}
			if (mClearPhaseB != 0 && mClearActiveB != 0)
			{
				uint32_t anIndex = mClearIndexB;
				uint32_t aColumns = mColumns;
				Piece* aPiece = new Piece();
				aPiece->mX = (anIndex % aColumns) * mBlockWidth;
				aPiece->mY = mClearRowB * mBlockHeight * FIXED_ONE;
				aPiece->mKind = (uint32_t)mClearRowB % (uint32_t)CfgNumColors(mState->mConfig);
				aPiece->mFlags = PF_REMOVING;
				aPiece->mEffectKind = KIND_NONE;
				aPiece->mSeed = GameTime();
				aPiece->mSpeed = 0;
				mPieces[mClearIndexB] = aPiece;
			}
			if (mClearPhaseB != 0 && mClearActiveB != 0)
			{
				if (mClearRight == 0)
				{
					mClearIndexB = mClearIndexB - 1;
					if ((uint32_t)mClearIndexB % mColumns == mColumns - 1U)
					{
						mClearRowB = mClearRowB - 1;
						mClearPhaseB = 0;
						mClearRight = 1;
						mClearReady = 1;
					}
				}
				else
				{
					mClearIndexB = mClearIndexB + 1;
					if ((uint32_t)mClearIndexB % mColumns == 0)
					{
						mClearRowB = mClearRowB - 1;
						mClearPhaseB = 0;
						mClearRight = 0;
						mClearReady = 1;
					}
				}
			}
			if (mClearPhaseB == 0 && mClearActiveA != 0)
			{
				if (mClearRight == 0)
				{
					mClearIndexA = mClearIndexA - 1;
					if ((uint32_t)mClearIndexA % mColumns == mColumns - 1U)
					{
						mClearRowA = mClearRowA + 1;
						mClearPhaseB = 1;
						mClearRight = 1;
						mClearReady = 1;
					}
				}
				else
				{
					mClearIndexA = mClearIndexA + 1;
					if ((uint32_t)mClearIndexA % mColumns == 0)
					{
						mClearRowA = mClearRowA + 1;
						mClearPhaseB = 1;
						mClearRight = 0;
						mClearReady = 1;
					}
				}
			}
			aResult = 0;
		}
	}
	return aResult;
}

// 0x408ceb: the end of a level: every block that is not dissolving flies off the screen, row after row. Returns 1 if
// there was none (else 0, and the app is held until they are gone).
uint32_t Field::BlowAway()
{
	bool isAny = false;
	int32_t aDelay = 0;
	uint32_t aLastRow = 1000;
	uint32_t aTime = GameTime();
	uint32_t anIndex = mCount;
	uint32_t aResult;
	do
	{
		anIndex = anIndex - 1;
		if (mPieces[anIndex] != nullptr && (~mPieces[anIndex]->mFlags & PF_REMOVING) != 0)
		{
			if (anIndex / mColumns != aLastRow)
			{
				aLastRow = anIndex / mColumns;
				aDelay = aDelay + 100;
			}
			isAny = true;
			mPieces[anIndex]->mFlags = mPieces[anIndex]->mFlags | PF_LEAVING;
			mPieces[anIndex]->mFlyTime = aTime;
			int32_t aRandom = Rand();
			mPieces[anIndex]->mFlyDelay = aDelay + aRandom % 200;
		}
	} while (anIndex != 0);
	if (isAny)
	{
		Play(0x26, 1);
		NewHandler(CounterProc, gApp.mLayer, nullptr, (uintptr_t)&gBusy, (uintptr_t)&gApp.mCounted[0]);
		aResult = 0;
	}
	else
	{
		aResult = 1;
	}
	return aResult;
}

// 0x408e41: whether a piece has all of the flags
uint32_t Field::HasFlags(uint16_t theFlags)
{
	uint32_t anIndex = 0;
	while (true)
	{
		if (mCount <= anIndex)
			return 0;
		if (mPieces[anIndex] != nullptr && (mPieces[anIndex]->mFlags & theFlags) == theFlags)
			break;
		anIndex = anIndex + 1;
	}
	return 1;
}

// 0x408ea1: deletes all pieces and the grid
void Field::FreePieces()
{
	if (mPieces != nullptr)
	{
		for (uint32_t i = 0; i < mCount; i++)
		{
			if (mPieces[i] != nullptr)
			{
				Piece* aPiece = mPieces[i];
				if (aPiece != nullptr)
					delete aPiece;  // 0x403bbb
				mPieces[i] = nullptr;
			}
		}
		free(mPieces);
		mPieces = nullptr;
	}
}

// 0x408f49: clears the board's lock bit (2) on the player and on the floaters
void Field::ClearLockBit()
{
	mLockFall = mLockFall & 0xfffffffd;
	mLockSpawn = mLockSpawn & 0xfffffffd;
}

// 0x408f70: sets it
void Field::SetLockBit()
{
	mLockFall = mLockFall | 2;
	mLockSpawn = mLockSpawn | 2;
}

// 0x408f97: whether nothing moves or dissolves
uint8_t Field::IsIdle()
{
	return mIdle;
}

// 0x40947b: whether a kind of block is pushed up or down a number of rows by what it hits (the configuration's
// *_TAP_ROWS of it; plain blocks have one for the player's block, theFaller, and one for the others)
uint32_t Field::HasTapRows(int theKind, char theFaller)
{
	Config& aConfig = mState->mConfig;
	if (theKind == KIND_ROCKET)
	{
		if (aConfig.mRocketTapRows != 0)
			return 1;
	}
	else if (theKind == KIND_EXPLODER)
	{
		if (aConfig.mExploderTapRows != 0)
			return 1;
	}
	else if (theKind == KIND_HAMMER)
	{
		if (aConfig.mHammerTapRows != 0)
			return 1;
	}
	else if (theKind == KIND_MAGNET)
	{
		if (aConfig.mMagnetTapRows != 0)
			return 1;
	}
	else if (theKind == KIND_RAINBOW)
	{
		if (aConfig.mRainbowTapRows != 0)
			return 1;
	}
	else if (theKind == KIND_COLORBOMB)
	{
		if (aConfig.mColorbombTapRows != 0)
			return 1;
	}
	else if (theKind == KIND_COLUMNNUKE)
	{
		if (aConfig.mColumnnukeTapRows != 0)
			return 1;
	}
	else if (theFaller == 0)
	{
		if (aConfig.mFloaterTapRows != 0)
			return 1;
	}
	else
	{
		if (aConfig.mFallerTapRows != 0)
			return 1;
	}
	return 0;
}

// 0x40959b: whether a column has no block at rest: nothing in it but moving ones
uint32_t Field::ColumnClear(int theColumn)
{
	uint32_t aRow = 0;
	while (true)
	{
		if (mRows <= aRow)
			return 1;
		Piece* aPiece = mPieces[aRow * mColumns + theColumn];
		if (aPiece != nullptr && ((aPiece->mFlags & PF_MOVING) == 0 || (~aPiece->mFlags & PF_TARGET) == 0))
			break;
		aRow = aRow + 1;
	}
	return 0;
}

// 0x40962d: restarts the clocks (after a pause)
void Field::ResetClocks()
{
	mState->mMeterTime = GameTime();
	mClearTime = GameTime();
	mClock = 0;
}

// 0x40965f: lets a block enter when the level's threshold of play time has passed (not in the queue mode)
void Field::UpdateSpawnTimer()
{
	if (CfgSpawnThreshold(mState->mConfig) != 0 && (uint32_t)CfgSpawnThreshold(mState->mConfig) < mPlayTime2)
	{
		if ((&gConfig.mFloaterdeterminationNormal)[gGameMode] != 2)
			Spawn();
		mPlayTime2 = mPlayTime2 - CfgSpawnThreshold(mState->mConfig);
	}
}

// --- the save file ---

// 0x4096ce: writes the field to a savegame.dat field by field, in the original's order (WriteField: 'D' bytes as they
// are, 'B' a piece); the grid last, each piece as its cell index and itself, an empty cell as -1. Returns 1 on a failed
// write, else 0.
uint32_t Field::Save(FILE* theFile, uint32_t theNow)
{
	auto put = [&](char theType, size_t theSize, void* theData) -> bool
	{
		return (WriteField(theType, theSize, theData, theFile, theNow) & 0xff) != 0;
	};
	if (put('D', 4, &mScore) || put('D', 4, &mLevelStartScore) || put('D', 4, &mLockFall) || put('D', 4, &mLockSpawn) ||
		put('B', 0x5c, &mNext) || put('D', 4, &mTotalTime) || put('D', 4, &mLevelTime) || put('D', 4, &mBestChain) ||
		put('D', 4, &mLevelBestChain) || put('D', 4, &mBestMatch) || put('D', 4, &mLevelBestMatch))
		return 1;
	for (uint32_t i = 0; i < 0x40; i++)
	{
		if (put('D', 1, &mQueue[i]))
			return 1;
	}
	if (put('D', 4, &mQueueHead) || put('D', 4, &mQueueCount) || put('D', 4, &mLowRow) || put('D', 4, &mHighRow) ||
		put('D', 4, &mPendingSpawns) || put('D', 4, &mFloaterCredit) || put('D', 4, &mPendingFloaters) ||
		put('D', 4, &mSpawnDelay) || put('D', 1, &mIdle) || put('D', 4, &mCountdown[0]) || put('D', 4, &mCountdown[1]) ||
		put('D', 4, &mCountdown[2]) || put('D', 4, &mCountdown[3]) || put('D', 4, &mCountdown[4]) ||
		put('D', 4, &mCountdown[5]) || put('D', 4, &mCountdown[6]) || put('D', 4, &mCountdown[7]))
		return 1;
	for (int32_t i = 0; i < CHAIN_COUNT; i++)
	{
		if (put('D', 2, &mChains[i]))
			return 1;
	}
	if (put('D', 4, &mNextChain) || put('D', 4, &mRowDelay) || put('D', 1, &mClearStart) || put('D', 4, &mPlayTime) ||
		put('D', 4, &mPlayTime2) || put('D', 4, &mColumnCounter) || put('D', 4, &mFloaterCredit2) ||
		put('D', 4, &mClearRowA) || put('D', 4, &mClearRowB) || put('D', 4, &mClearIndexA) || put('D', 4, &mClearIndexB) ||
		put('D', 1, &mClearActiveA) || put('D', 1, &mClearActiveB) || put('D', 4, &mClearInterval) ||
		put('D', 1, &mClearPhaseB) || put('D', 1, &mClearRight) || put('D', 1, &mClearReady))
		return 1;
	uint32_t anIndex = 0;
	while (true)
	{
		if (mCount <= anIndex)
			return 0;
		bool isFailed;
		if (mPieces[anIndex] == nullptr)
		{
			uint32_t aNone = 0xffffffff;
			isFailed = put('D', 4, &aNone);
		}
		else
		{
			if (put('D', 4, &anIndex))
				return 1;
			isFailed = put('B', 0x5c, mPieces[anIndex]);
		}
		if (isFailed)
			break;
		anIndex = anIndex + 1;
	}
	return 1;
}

// 0x409f18: reads the field from a savegame.dat the way Save writes it. Makes the grid and its pieces. Returns 1 on
// a failed read, else 0.
uint32_t Field::Load(FILE* theFile, uint32_t theNow)
{
	auto get = [&](char theType, size_t theSize, void* theData) -> bool
	{
		return (ReadField(theType, theSize, theData, theFile, theNow) & 0xff) != 0;
	};
	if (get('D', 4, &mScore) || get('D', 4, &mLevelStartScore) || get('D', 4, &mLockFall) || get('D', 4, &mLockSpawn) ||
		get('B', 0x5c, &mNext) || get('D', 4, &mTotalTime) || get('D', 4, &mLevelTime) || get('D', 4, &mBestChain) ||
		get('D', 4, &mLevelBestChain) || get('D', 4, &mBestMatch) || get('D', 4, &mLevelBestMatch))
		return 1;
	for (int32_t i = 0; i < 0x40; i++)
	{
		if (get('D', 1, &mQueue[i]))
			return 1;
	}
	if (get('D', 4, &mQueueHead) || get('D', 4, &mQueueCount) || get('D', 4, &mLowRow) || get('D', 4, &mHighRow) ||
		get('D', 4, &mPendingSpawns) || get('D', 4, &mFloaterCredit) || get('D', 4, &mPendingFloaters) ||
		get('D', 4, &mSpawnDelay) || get('D', 1, &mIdle) || get('D', 4, &mCountdown[0]) || get('D', 4, &mCountdown[1]) ||
		get('D', 4, &mCountdown[2]) || get('D', 4, &mCountdown[3]) || get('D', 4, &mCountdown[4]) ||
		get('D', 4, &mCountdown[5]) || get('D', 4, &mCountdown[6]) || get('D', 4, &mCountdown[7]))
		return 1;
	for (int32_t i = 0; i < CHAIN_COUNT; i++)
	{
		if (get('D', 2, &mChains[i]))
			return 1;
	}
	if (get('D', 4, &mNextChain) || get('D', 4, &mRowDelay) || get('D', 1, &mClearStart) || get('D', 4, &mPlayTime) ||
		get('D', 4, &mPlayTime2) || get('D', 4, &mColumnCounter) || get('D', 4, &mFloaterCredit2) ||
		get('D', 4, &mClearRowA) || get('D', 4, &mClearRowB) || get('D', 4, &mClearIndexA) || get('D', 4, &mClearIndexB) ||
		get('D', 1, &mClearActiveA) || get('D', 1, &mClearActiveB) || get('D', 4, &mClearInterval) ||
		get('D', 1, &mClearPhaseB) || get('D', 1, &mClearRight) || get('D', 1, &mClearReady))
		return 1;
	mPieces = (Piece**)malloc(mColumns * mRows * 4);
	for (uint32_t i = 0; i < mCount; i++)
		mPieces[i] = nullptr;
	uint32_t anIndex = 0;
	while (true)
	{
		if (mCount <= anIndex)
			return 0;
		uint32_t aStored;
		if (get('D', 4, &aStored))
			break;
		if (aStored != 0xffffffff)
		{
			Piece* aPiece = new Piece();
			mPieces[anIndex] = aPiece;
			if (get('B', 0x5c, mPieces[anIndex]))
				return 1;
		}
		anIndex = anIndex + 1;
	}
	return 1;
}

// --- Board ---

// 0x40ad4f: the board of theColumns by theRows blocks with its top left at (theX, theY) on the screen. Dies (the
// original throws an exception that nothing catches) if its images or memory fail.
Board::Board(int theX, int theY, int theColumns, int theRows)
{
	// mNextBlocks are made by Piece::Piece (0x401000, four times)
	int32_t aResult = Init(theX, theY, theColumns, theRows);
	if (aResult < 0)
		Throw();
}

// 0x40ade3: the destructor (0x40adba is its deleting form: delete). Frees the images, the field and the glow buffers.
Board::~Board()
{
	FreeSurface(mArrows);
	FreeSurface(mBlocks);
	FreeSurface(mGlowTop);
	FreeSurface(mTopCover);
	FreeSurface(mBottomCover);
	if (mField != nullptr)
	{
		if (mField != nullptr)
			delete mField;  // through its virtual destructor
		mField = nullptr;
	}
	if (mGlowTable != nullptr)
		free(mGlowTable);
	if (mGlowBuffer != nullptr)
		free(mGlowBuffer);
	if (mGlowBufferA != nullptr)
		free(mGlowBufferA);
	if (mGlowBufferB != nullptr)
		free(mGlowBufferB);
	if (mGlowAlpha != nullptr)
		free(mGlowAlpha);
	FreeSounds();
}

// 0x40af30: sets the board's size and place, loads its images and makes the profile of the glow (a table of
// (2 * radius + 1) by (radius + 256) bytes: the glow of a column, 255 at the middle falling off with the squared
// distance from the column, and over the first rows of its end). Returns 0, or E_FAIL if a size is zero or an image
// could not be loaded.
int32_t Board::Init(int theX, int theY, int theColumns, int theRows)
{
	int32_t aResult;
	if (theColumns == 0)
	{
		aResult = kFailure;
	}
	else if (theRows == 0)
	{
		aResult = kFailure;
	}
	else
	{
		mGlowTable = nullptr;
		mGlowAlpha = nullptr;  // the original clears the 8 bytes at +0x1650 and the 0x3c at +0x1610
		mGlowAlphaPitch = 0;
		mGlowPitch = 0;
		mGlowBuffer = nullptr;
		mGlowBufferUnused = 0;
		mGlowBufferA = nullptr;
		mGlowValidA = 0;
		mGlowBufferB = nullptr;
		mGlowValidB = 0;
		mGlowColumn = 0;
		mGlowLength = 0;
		mGlowColumnB = 0;
		mGlowLengthB = 0;
		mGlowLowRow = 0;
		mGlowHighRow = 0;
		mGlowBrightness = 0;
		mGlowTime = 0;
		mBlockWidth = gConfig.mBlockWidth;
		mBlockHeight = gConfig.mBlockHeight;
		mWidth = mBlockWidth * theColumns;
		mHeight = mBlockHeight * theRows;
		mX = theX;
		mRight = mX + mWidth;
		mY = theY;
		mBottom = mY + mHeight;
		mColumns = theColumns;
		mRows = theRows;
		mGameOver = 0;
		mField = nullptr;
		mLastColumn = -1;
		if (LoadSurface(0x24, 9, &mBlocks) == 0)
		{
			if (LoadSurface(0x25, 1, &mGlowTop) == 0)
			{
				if (LoadSurface(0x23, 9, &mArrows) == 0)
				{
					if (LoadSurface(0x42, 9, &mTopCover) == 0)
					{
						if (LoadSurface(0x43, 9, &mBottomCover) == 0)
						{
							uint32_t aRadius = gConfig.mBkgcolGlowradius;
							uint32_t aWidth = gConfig.mBkgcolGlowradius * 2 + 1;
							uint32_t aHeight = gConfig.mBkgcolGlowradius + 0x100;
							mGlowTable = (uint8_t*)malloc(aWidth * aHeight);
							if (mGlowTable == nullptr)
							{
								NoMemory();
							}
							for (uint32_t y = 0; y < aHeight; y++)
							{
								for (uint32_t x = 0; x < aWidth; x++)
								{
									uint32_t anAlpha;
									if (y < aRadius)
										anAlpha = 0xff;
									else
										anAlpha = (aHeight - 1) - y;
									double aDY;
									if (y < aRadius)
										aDY = (double)(uint32_t)(aRadius - y);
									else
										aDY = 0.0;
									double aDX;
									if (x < aRadius)
										aDX = (double)(uint32_t)(aRadius - x);
									else
										aDX = (double)(uint32_t)(x - aRadius);
									double aRatio = 1.0 - (aDX * aDX + aDY * aDY) / (double)(uint64_t)((aRadius + 1) * (aRadius + 1));
									double aValue = (double)anAlpha * aRatio;
									int32_t aClamped;
									if ((int32_t)Ftol(aValue) < 0)
										aClamped = 0;
									else
										aClamped = (int32_t)Ftol(aValue);
									uint8_t aByte;
									if (aClamped < 0xff)
									{
										if ((int32_t)Ftol(aValue) < 0)
											aByte = 0;
										else
											aByte = (uint8_t)Ftol(aValue);
									}
									else
									{
										aByte = 0xff;
									}
									mGlowTable[x + y * aWidth] = aByte;
								}
							}
							aResult = 0;
						}
						else
						{
							aResult = kFailure;
						}
					}
					else
					{
						aResult = kFailure;
					}
				}
				else
				{
					aResult = kFailure;
				}
			}
			else
			{
				aResult = kFailure;
			}
		}
		else
		{
			aResult = kFailure;
		}
	}
	return aResult;
}

// 0x40b296: sets the board's copy of the configuration (the original's 0x12a4 bytes at gConfig)
int32_t Board::SetConfig(const Config* theConfig)
{
	memcpy(&mState.mConfig, theConfig, 0x12a4);
	return 0;
}

// 0x40b2bc: starts a game: a new field, the first level
void Board::Start()
{
	if (mField != nullptr)
	{
		if (mField != nullptr)
			delete mField;
		mField = nullptr;
	}
	mState.mLevel = mState.mConfig.mStartLevel - 1;
	mField = new Field(mColumns, mRows, &mBlockWidth);
	mField->SetState(&mState);
	gBonusRound = 0;
	gHighScoreEntered = 0;
	NextLevel();
}

// 0x40b3ad: raises a level value for the levels past the end of the list: *theValue times theRamp to the power of the
// levels past it, at most 2^31
void Board::Ramp(uint32_t* theValue, float theRamp)
{
	*theValue = (uint32_t)Ftol(pow((double)theRamp, (double)(uint32_t)((mState.mLevel + 1) - gLevelListCount)) *
		(double)*theValue);
	if (0x80000000 < *theValue)
		*theValue = 0x80000000;
}

// 0x40b41c: checks whether the level that is coming is a bonus round (every so many levels: the mode's
// BONUSMODULUS) and if so reads it from the text resource 0x13 of group 3 and sets it up. The resource lists
// "@BONUS" and then for each kind of game (N, T, S, R) and level a record: the letter and level, the seconds, the
// points of a block, the kinds the preview blocks take (as digits and the letters b e h m n r; ':' ends the list) and
// the 0x80 cells of the layout (digits are kinds, '-' empty). Returns true if a bonus round was started.
bool Board::CheckBonusRound()
{
	if (mState.mLevel == 0)
		return false;
	if (gBonusRound == 1)
	{
		gBonusRound = 0;
		return false;
	}
	uint32_t aRemainder = mState.mLevel % (uint32_t)(&gConfig.mNormalBonusmodulus)[gGameMode];
	char aMode;
	if (gGameMode == 0)
	{
		aMode = 'N';
	}
	else
	{
		char aLetter;
		if (gGameMode == 1)
			aLetter = 'T';
		else
			aLetter = (gGameMode == 2) + 'R';
		aMode = aLetter;
	}
	if (res::Load(3, 1, 0x13) == 0)
	{
		const char* aText = (const char*)res::Get(3, 0x13);
		do
		{
			do
			{
				if (SkipToToken(&aText, 0) != 0)
				{
					res::Free(3, 1, 0x13);
					return false;
				}
				ReadToken(&aText);
			} while (strcmp(aText, GTC(0x46c354)) == 0);
			while (SkipToToken(&aText, 0), '@' < *aText && *aText < '[')
			{
				if (*aText == aMode)
				{
					aText = aText + 1;
					ReadToken(&aText);
					uint32_t aNumber = (uint32_t)TokenInt();
					if (aNumber == aRemainder)
					{
						gBonusRound = 1;
						mQueueIndex = 0;
						while (SkipToToken(&aText, 0), '@' < *aText && *aText < '[')
						{
							ReadToken(&aText);
						}
						ReadToken(&aText);
						gConfig.mBonusTime = TokenInt();
						SkipToToken(&aText, 0);
						ReadToken(&aText);
						gConfig.mBonusScore = TokenInt();
						SkipToToken(&aText, 0);
						gApp.mBonusElapsed = 0;
						gApp.mBonusClockStopped = 0;
						int32_t anIndex = 0;
						do
						{
							if (0x43 < anIndex)
							{
ReadBonusLayout:
								for (anIndex = 0; anIndex < 0x80; anIndex = anIndex + 1)
								{
									if (*aText < '0' || '9' < *aText)
									{
										if (*aText == '-')
											gConfig.mBonusLayout[anIndex] = 0x11;
									}
									else
									{
										gConfig.mBonusLayout[anIndex] = *aText - '0';
									}
									aText = aText + 1;
									SkipToToken(&aText, 0);
								}
								mState.mMeterTime = GameTime();
								mField->FreePieces();
								StartLevel();
								FillQueue();
								Play(5, 1);
								res::Free(3, 1, 0x13);
								return true;
							}
							if (*aText == ':')
							{
								aText = aText + 1;
								SkipToToken(&aText, 0);
								for (; anIndex < 0x44; anIndex = anIndex + 1)
									gConfig.mBonusQueue[anIndex] = 0x11;
								goto ReadBonusLayout;
							}
							if (*aText < '0' || '9' < *aText)
							{
								char aChar = *aText;
								if (aChar < 'c')
								{
									if (aChar == 'b' || aChar == 'B')
									{
										gConfig.mBonusQueue[anIndex] = 0xf;
									}
									else
									{
										if (aChar == 'E')
											goto QueueExploder;
										if (aChar == 'H')
											goto QueueHammer;
										if (aChar == 'M')
											goto QueueMagnet;
										if (aChar == 'N')
											goto QueueColumnnuke;
										if (aChar == 'R')
											goto QueueRainbow;
QueueDefaultKind:
										gConfig.mBonusQueue[anIndex] = 0;
									}
								}
								else if (aChar == 'e')
								{
QueueExploder:
									gConfig.mBonusQueue[anIndex] = 0xb;
								}
								else if (aChar == 'h')
								{
QueueHammer:
									gConfig.mBonusQueue[anIndex] = 0xc;
								}
								else if (aChar == 'm')
								{
QueueMagnet:
									gConfig.mBonusQueue[anIndex] = 0xd;
								}
								else if (aChar == 'n')
								{
QueueColumnnuke:
									gConfig.mBonusQueue[anIndex] = 0x10;
								}
								else
								{
									if (aChar != 'r')
										goto QueueDefaultKind;
QueueRainbow:
									gConfig.mBonusQueue[anIndex] = 0xe;
								}
							}
							else
							{
								gConfig.mBonusQueue[anIndex] = *aText - '0';
							}
							aText = aText + 1;
							SkipToToken(&aText, 0);
							anIndex = anIndex + 1;
						} while (true);
					}
				}
				else
				{
					ReadToken(&aText);
				}
			}
		} while (true);
	}
	return false;
}

// 0x40b82c: starts the next level: resets the end flags and the preview blocks; unless it is a bonus round
// (CheckBonusRound) copies the level's record from the mode's list (the last one for levels past the end, with the
// values raised: Ramp) into the board's configuration, sets the goal and the meter, empties the field, starts it (Reset)
// and fills the preview blocks; plays the sound of the first level or of a new one.
void Board::NextLevel()
{
	mGameOver = 0;
	mLevelComplete = 0;
	mEnding = 0;
	mNextBlocks[0].mFlags = 0;
	mNextBlocks[1].mFlags = 0;
	mNextBlocks[2].mFlags = 0;
	mNextBlocks[3].mFlags = 0;
	if (!CheckBonusRound())
	{
		Config& aConfig = mState.mConfig;
		uint32_t aLevelIndex;
		if (mState.mLevel < gLevelListCount)
			aLevelIndex = mState.mLevel;
		else
			aLevelIndex = gLevelListCount - 1;
		const Level* aLevel = &((const Level*)gLevelList)[aLevelIndex];
		CfgStartRows(aConfig) = aLevel->mStartRows;
		CfgStartRowsAdd(aConfig) = aLevel->mStartRowsAdd;
		CfgStopRow(aConfig) = aLevel->mStopRow;
		CfgStopRowAdd(aConfig) = aLevel->mStopRowAdd;
		CfgFloatsPerRemoval(aConfig) = aLevel->mFloatsPerRemoval;
		CfgFloatsPerTap(aConfig) = aLevel->mFloatsPerTap;
		CfgGoal(aConfig) = aLevel->mGoal;
		for (int i = 0; i < 16; i++)
			CfgRange(aConfig, i) = aLevel->mRange[i];
		CfgNumColors(aConfig) = aLevel->mNumColors;
		CfgSpawnThreshold(aConfig) = aLevel->mSpawnThreshold;
		CfgMeterStart(aConfig) = aLevel->mMeterStart;
		CfgMeterLoss(aConfig) = aLevel->mMeterLoss;
		CfgMeterDoublingTime(aConfig) = aLevel->mMeterDoublingTime * 1000;
		if (CfgStopRow(aConfig) == -2)
		{
			CfgStopRow(aConfig) = (mRows - 1U) >> 1;
			CfgStopRow(aConfig) = CfgStopRow(aConfig) + CfgStartRowsAdd(aConfig);
		}
		if (CfgStopRow(aConfig) == -3)
			CfgStopRow(aConfig) = (mRows - 1U) >> 1;
		if (CfgRange(aConfig, 1) != 0)
		{
			float aHalf = (float)((double)(uint32_t)(CfgRange(aConfig, 0) + CfgRange(aConfig, 1)) / 2.0);
			float aFactor = (float)(((double)(uint32_t)(CfgRange(aConfig, 0) + CfgRange(aConfig, 1)) / 2.0 + 1.0) / (double)aHalf);
			CfgFloatsPerRemoval(aConfig) = (float)((double)aFactor * (double)CfgFloatsPerRemoval(aConfig));
			if (aConfig.mFallfloatAdjust != 0)
				CfgFloatsPerTap(aConfig) = (float)((double)aFactor * (double)CfgFloatsPerTap(aConfig));
		}
		if (CfgRange(aConfig, 5) != 0)
		{
			float aHalf = (float)((double)(uint32_t)(CfgRange(aConfig, 4) + CfgRange(aConfig, 5)) / 2.0);
			float aFactor = (float)(((double)(uint32_t)(CfgRange(aConfig, 4) + CfgRange(aConfig, 5)) / 2.0 + 1.0) / (double)aHalf);
			CfgFloatsPerRemoval(aConfig) = (float)((double)aFactor * (double)CfgFloatsPerRemoval(aConfig));
			if (aConfig.mFallfloatAdjust != 0)
				CfgFloatsPerTap(aConfig) = (float)((double)aFactor * (double)CfgFloatsPerTap(aConfig));
		}
		if (gLevelListCount <= mState.mLevel)
		{
			// The levels past the end of the list: each special's interval and the spawn threshold grow by the
			// mode's ramps, to the power of the levels past the end
			const float* aRamps;
			float aRndfltRamp;
			uint32_t aRndfltMin;
			if (gGameMode == 0)
			{
				aRamps = &aConfig.mNormalRocketRamp;
				aRndfltRamp = aConfig.mNormalRndfltRamp;
				aRndfltMin = aConfig.mNormalRndfltMin;
			}
			else if (gGameMode == 1)
			{
				aRamps = &aConfig.mTimedRocketRamp;
				aRndfltRamp = aConfig.mTimedRndfltRamp;
				aRndfltMin = aConfig.mTimedRndfltMin;
			}
			else if (gGameMode == 2)
			{
				aRamps = &aConfig.mSpeedRocketRamp;
				aRndfltRamp = aConfig.mSpeedRndfltRamp;
				aRndfltMin = aConfig.mSpeedRndfltMin;
			}
			else
			{
				aRamps = &aConfig.mStrategyRocketRamp;
				aRndfltRamp = aConfig.mStrategyRndfltRamp;
				aRndfltMin = aConfig.mStrategyRndfltMin;
			}
			if (gGameMode <= 3)
			{
				for (int i = 0; i < 8; i++)
				{
					Ramp((uint32_t*)&CfgRange(aConfig, i * 2), aRamps[i]);
					Ramp((uint32_t*)&CfgRange(aConfig, i * 2 + 1), aRamps[i]);
				}
				if ((uint32_t)CfgSpawnThreshold(aConfig) != 0)
				{
					Ramp((uint32_t*)&CfgSpawnThreshold(aConfig), aRndfltRamp);
					if ((uint32_t)CfgSpawnThreshold(aConfig) < aRndfltMin)
						CfgSpawnThreshold(aConfig) = aRndfltMin;
				}
			}
			if (gGameMode == 1)
			{
				CfgMeterLoss(aConfig) = (float)(pow((double)aConfig.mTimedPlpsecRamp,
					(double)(uint32_t)((mState.mLevel + 1) - gLevelListCount)) * (double)CfgMeterLoss(aConfig));
			}
		}
		mState.mLevel = mState.mLevel + 1;
		mState.mBlocksLeft = CfgGoal(aConfig);
		mState.mMeter = (float)(double)(uint32_t)CfgMeterStart(aConfig);
		mState.mMeterTime = GameTime();
		mField->FreePieces();
		StartLevel();
		FillQueue();
		if (mState.mLevel == (uint32_t)aConfig.mStartLevel)
			Play(4, 1);
		else
			Play(5, 1);
	}
}

// 0x40c6ec: lets the field start a block that the level's play time asks for
void Board::UpdateSpawnTimer()
{
	mField->UpdateSpawnTimer();
}

// 0x40c703: refills the four preview blocks in a bonus round: with the next of the round's queue (the first block when
// the queue is shifted up by a tap), or the first four when none is in yet. Empty (0x11) first block: the queue has
// ended and the block keeps its place.
void Board::FillQueueBonus()
{
	if (((((~mNextBlocks[0].mFlags & PF_PREVIEW) == 0) || ((~mNextBlocks[1].mFlags & PF_PREVIEW) == 0)) ||
			((~mNextBlocks[2].mFlags & PF_PREVIEW) == 0)) ||
		((~mNextBlocks[3].mFlags & PF_PREVIEW) == 0))
	{
		if (mNextBlocks[0].mKind == KIND_NONE)
		{
			mNextBlocks[0].mFlags = mNextBlocks[0].mFlags | PF_PREVIEW;
		}
		else
		{
			mQueueShifting = 1;
			mNextBlocks[0] = mNextBlocks[1];
			mNextBlocks[0].mY = 0;
			mNextBlocks[0].mMoveTime = GameTime();
			mNextBlocks[1] = mNextBlocks[2];
			mNextBlocks[1].mY = 0;
			mNextBlocks[2] = mNextBlocks[3];
			mNextBlocks[2].mY = mBlockHeight << FIXED_SHIFT;
			mNextBlocks[3].mX = 0;
			mNextBlocks[3].mY = mBlockHeight << 9;
			mNextBlocks[3].mKind = gConfig.mBonusQueue[mQueueIndex];
			mNextBlocks[3].mFlags = PF_PREVIEW;
			Motion* aMotion = MotionOf(mNextBlocks[3].mKind, 1);
			mNextBlocks[3].mSpeed = aMotion->mParam[0];
			mNextBlocks[3].mSeed = (uint32_t)Rand();
			mQueueIndex = mQueueIndex + 1;
		}
	}
	else
	{
		mNextBlocks[0].mX = 0;
		mNextBlocks[0].mY = 0;
		mNextBlocks[0].mKind = gConfig.mBonusQueue[0];
		mNextBlocks[0].mFlags = PF_PREVIEW;
		Motion* aMotion = MotionOf(mNextBlocks[0].mKind, 1);
		mNextBlocks[0].mSpeed = aMotion->mParam[0];
		mNextBlocks[0].mMoveTime = GameTime();
		mNextBlocks[0].mSeed = (uint32_t)Rand();
		mNextBlocks[1].mX = 0;
		mNextBlocks[1].mY = 0;
		mNextBlocks[1].mKind = gConfig.mBonusQueue[1];
		mNextBlocks[1].mFlags = PF_PREVIEW;
		aMotion = MotionOf(mNextBlocks[1].mKind, 1);
		mNextBlocks[1].mSpeed = aMotion->mParam[0];
		mNextBlocks[1].mSeed = (uint32_t)Rand();
		mNextBlocks[2].mX = 0;
		mNextBlocks[2].mY = mBlockHeight << FIXED_SHIFT;
		mNextBlocks[2].mKind = gConfig.mBonusQueue[2];
		mNextBlocks[2].mFlags = PF_PREVIEW;
		aMotion = MotionOf(mNextBlocks[2].mKind, 1);
		mNextBlocks[2].mSpeed = aMotion->mParam[0];
		mNextBlocks[2].mSeed = (uint32_t)Rand();
		mNextBlocks[3].mX = 0;
		mNextBlocks[3].mY = mBlockHeight << 9;
		mNextBlocks[3].mKind = gConfig.mBonusQueue[3];
		mNextBlocks[3].mFlags = PF_PREVIEW;
		aMotion = MotionOf(mNextBlocks[3].mKind, 1);
		mNextBlocks[3].mSpeed = aMotion->mParam[0];
		mNextBlocks[3].mSeed = (uint32_t)Rand();
		mQueueIndex = 4;
	}
}

// 0x40ca07: refills the four preview blocks (the player's next blocks, the first at the bottom of the display): with a
// new one at the end after a tap (the others move up), or all four when none is set; in a bonus round from its queue.
void Board::FillQueue()
{
	if (((((mNextBlocks[0].mFlags & PF_PREVIEW) == 0) || ((mNextBlocks[1].mFlags & PF_PREVIEW) == 0)) ||
			((mNextBlocks[2].mFlags & PF_PREVIEW) == 0)) ||
		((mNextBlocks[3].mFlags & PF_PREVIEW) == 0))
	{
		if (gBonusRound == 0)
		{
			if (((((~mNextBlocks[0].mFlags & PF_PREVIEW) == 0) || ((~mNextBlocks[1].mFlags & PF_PREVIEW) == 0)) ||
					((~mNextBlocks[2].mFlags & PF_PREVIEW) == 0)) ||
				((~mNextBlocks[3].mFlags & PF_PREVIEW) == 0))
			{
				mQueueShifting = 1;
				mNextBlocks[0] = mNextBlocks[1];
				mNextBlocks[0].mY = 0;
				mNextBlocks[0].mMoveTime = GameTime();
				mNextBlocks[1] = mNextBlocks[2];
				mNextBlocks[1].mY = 0;
				mNextBlocks[2] = mNextBlocks[3];
				mNextBlocks[2].mY = mBlockHeight << FIXED_SHIFT;
				mNextBlocks[3].mX = 0;
				mNextBlocks[3].mY = mBlockHeight << 9;
				mNextBlocks[3].mKind = mField->RandomKind(1, KIND_NONE, KIND_NONE);
				mNextBlocks[3].mFlags = PF_PREVIEW;
				Motion* aMotion = MotionOf(mNextBlocks[3].mKind, 1);
				mNextBlocks[3].mSpeed = aMotion->mParam[0];
				mNextBlocks[3].mSeed = (uint32_t)Rand();
			}
			else
			{
				mNextBlocks[0].mX = 0;
				mNextBlocks[0].mY = 0;
				mNextBlocks[0].mKind = mField->RandomKind(1, KIND_NONE, KIND_NONE);
				mNextBlocks[0].mFlags = PF_PREVIEW;
				Motion* aMotion = MotionOf(mNextBlocks[0].mKind, 1);
				mNextBlocks[0].mSpeed = aMotion->mParam[0];
				mNextBlocks[0].mMoveTime = GameTime();
				mNextBlocks[0].mSeed = (uint32_t)Rand();
				mNextBlocks[1].mX = 0;
				mNextBlocks[1].mY = 0;
				mNextBlocks[1].mKind = mField->RandomKind(1, KIND_NONE, KIND_NONE);
				mNextBlocks[1].mFlags = PF_PREVIEW;
				aMotion = MotionOf(mNextBlocks[1].mKind, 1);
				mNextBlocks[1].mSpeed = aMotion->mParam[0];
				mNextBlocks[1].mSeed = (uint32_t)Rand();
				mNextBlocks[2].mX = 0;
				mNextBlocks[2].mY = mBlockHeight << FIXED_SHIFT;
				mNextBlocks[2].mKind = mField->RandomKind(1, KIND_NONE, KIND_NONE);
				mNextBlocks[2].mFlags = PF_PREVIEW;
				aMotion = MotionOf(mNextBlocks[2].mKind, 1);
				mNextBlocks[2].mSpeed = aMotion->mParam[0];
				mNextBlocks[2].mSeed = (uint32_t)Rand();
				mNextBlocks[3].mX = 0;
				mNextBlocks[3].mY = mBlockHeight << 9;
				mNextBlocks[3].mKind = mField->RandomKind(1, KIND_NONE, KIND_NONE);
				mNextBlocks[3].mFlags = PF_PREVIEW;
				aMotion = MotionOf(mNextBlocks[3].mKind, 1);
				mNextBlocks[3].mSpeed = aMotion->mParam[0];
				mNextBlocks[3].mSeed = (uint32_t)Rand();
			}
		}
		else
		{
			FillQueueBonus();
		}
	}
}

// 0x40cd63: writes a column of the glow into the alpha buffer: from the profile table's column theX, theLength rows
// of its middle between the two ends (each theRadius rows), scaled by theBrightness (0-256), as 255 minus the
// alpha, into the buffer at row theY with theBytesPerPixel + 1 bytes between its pixels
void Board::GlowColumn(int theX, int theY, uint32_t theLength, char theBytesPerPixel, int theBrightness)
{
	uint32_t aRadius = gConfig.mBkgcolGlowradius;
	uint8_t aStep = (uint8_t)(theBytesPerPixel + 1);
	uint32_t aTableIndex = theX * (gConfig.mBkgcolGlowradius * 2 + 1);
	uint32_t anOut = theY * mGlowAlphaPitch;
	uint32_t i;
	for (i = 0; i < aRadius; i++)
	{
		mGlowAlpha[anOut] = (uint8_t)(-1 - (char)((uint32_t)mGlowTable[aTableIndex] * theBrightness >> 8));
		anOut = anOut + aStep;
		aTableIndex = aTableIndex + 1;
	}
	for (i = 0; i < theLength; i++)
	{
		mGlowAlpha[anOut] = (uint8_t)(-1 - (char)((uint32_t)mGlowTable[aTableIndex] * theBrightness >> 8));
		anOut = anOut + aStep;
	}
	for (i = 0; aTableIndex = aTableIndex + 1, i < aRadius; i++)
	{
		mGlowAlpha[anOut] = (uint8_t)(-1 - (char)((uint32_t)mGlowTable[aTableIndex] * theBrightness >> 8));
		anOut = anOut + aStep;
	}
}

// 0x40cea5: draws the glow behind the field: a lit column in the background (the slider's column, and the preview
// block's if it is dropping) as long as the field's blocks (BKGCOL_LENGTH of the mode), pulsing with the sine of the
// time. The area is copied from the screen into the board's buffers, darkened with the glow's alpha (GlowColumn) and
// the result added back, then drawn to the screen. Redrawn only when something it depends on changed.
void Board::DrawGlow(lowos::Surface* theScreen)
{
	int32_t aRadius = gConfig.mBkgcolGlowradius;
	lowos::PixelFormat aFormat;
	uint8_t aBytesPerPixel = lowos::GetDisplayFormat(&aFormat, nullptr, nullptr);
	int32_t aHighRow = mField->GetHighRow();
	int32_t aLowRow = mField->GetLowRow();
	if (aLowRow == -1 && aHighRow != -1)
		aLowRow = 0;
	if (aHighRow == -1 && aLowRow != -1)
		aHighRow = mRows - 1;
	uint32_t aStop;
	if (CfgStopRow(mState.mConfig) == -1)
	{
		if (aHighRow == aLowRow)
			aStop = (mRows >> 1) + CfgStartRowsAdd(mState.mConfig);
		else
			aStop = (int32_t)(aHighRow + aLowRow) / 2 + CfgStopRowAdd(mState.mConfig);
	}
	else
	{
		aStop = CfgStopRow(mState.mConfig) + CfgStopRowAdd(mState.mConfig);
	}
	uint32_t aMin;
	if (aLowRow == aHighRow || aLowRow == 0)
		aMin = 1;
	else
		aMin = aLowRow;
	uint32_t aClamped;
	if ((int32_t)aStop < (int32_t)aMin)
	{
		if (aLowRow == aHighRow || aLowRow == 0)
			aClamped = 1;
		else
			aClamped = aLowRow;
	}
	else
	{
		aClamped = aStop;
	}
	uint32_t aMax;
	if (aLowRow == aHighRow || aHighRow == gConfig.mNumRows - 1U)
		aMax = gConfig.mNumRows - 2;
	else
		aMax = aHighRow;
	uint32_t aStopRow;
	if (aMax < aClamped)
	{
		if (aLowRow == aHighRow || aHighRow == gConfig.mNumRows - 1U)
			aStopRow = gConfig.mNumRows - 2;
		else
			aStopRow = aHighRow;
	}
	else
	{
		aStopRow = aClamped;
	}
	uint32_t aStopY = aStopRow * mBlockHeight;
	uint32_t aHalfHeight = gConfig.mBlockHeight * gConfig.mNumRows >> 1;
	int32_t aColumn = (int32_t)Send(gApp.mColumnCursor, MSG_CURSOR_COLUMN, 0, 0);
	int32_t anYOffset = gConfig.mBkgcolyoffset;
	uint32_t aSecondColumn;
	if ((mField->mNext.mFlags & PF_PREVIEW) == 0)
		aSecondColumn = 0;
	else
		aSecondColumn = (uint32_t)mField->mNext.mX / gConfig.mBlockWidth;
	int32_t aBottomY = gConfig.mNumRows * gConfig.mBlockHeight - gConfig.mBkgcolyoffset;
	int32_t aLengthMode = (&gConfig.mBkgcolLengthNormal)[gGameMode];
	uint32_t aLength = 0;
	uint32_t aLengthB = 0;
	if (aLengthMode == 0)
	{
		aLength = 0;
	}
	else if (aLengthMode == 1)
	{
		uint32_t anEdge = mField->EdgeRow(aColumn, 1);
		if (anEdge == 0xffffffff)
			anEdge = aStopY;
		int32_t aLimit = anEdge + (gConfig.mBlockHeight >> 1);
		uint32_t aValue;
		if ((int32_t)(gConfig.mBlockHeight * 3) < aLimit - anYOffset)
			aValue = aLimit - anYOffset;
		else
			aValue = gConfig.mBlockHeight * 3;
		aLength = aValue;
		anEdge = mField->EdgeRow(aSecondColumn, 0);
		if (anEdge == 0xffffffff)
			anEdge = aStopY;
		anYOffset = anEdge + (gConfig.mBlockHeight >> 1);
		if ((int32_t)(gConfig.mBlockHeight * 3) < aBottomY - anYOffset)
			aValue = aBottomY - anYOffset;
		else
			aValue = gConfig.mBlockHeight * 3;
		aLengthB = aValue;
	}
	else if (aLengthMode == 2)
	{
		int32_t aMiddle = aStopY + (gConfig.mBlockHeight >> 1);
		uint32_t aValue;
		if ((int32_t)(gConfig.mBlockHeight * 3) < aMiddle - gConfig.mBkgcolyoffset)
			aValue = aMiddle - gConfig.mBkgcolyoffset;
		else
			aValue = gConfig.mBlockHeight * 3;
		aLength = aValue;
		if ((int32_t)(gConfig.mBlockHeight * 3) < aBottomY - aMiddle)
			aValue = aBottomY - aMiddle;
		else
			aValue = gConfig.mBlockHeight * 3;
		aLengthB = aValue;
	}
	else if (aLengthMode == 3)
	{
		uint32_t aValue;
		if ((int32_t)(gConfig.mBlockHeight * 3) < (int32_t)(aHalfHeight - gConfig.mBkgcolyoffset))
			aValue = aHalfHeight - gConfig.mBkgcolyoffset;
		else
			aValue = gConfig.mBlockHeight * 3;
		aLength = aValue;
		if ((int32_t)(gConfig.mBlockHeight * 3) < (int32_t)(aBottomY - aHalfHeight))
			aHalfHeight = aBottomY - aHalfHeight;
		else
			aHalfHeight = gConfig.mBlockHeight * 3;
		aLengthB = aHalfHeight;
	}
	if (aLength != 0)
	{
		int32_t aFull = gConfig.mBlockHeight * gConfig.mNumRows + gConfig.mBkgcolyoffset * -2;
		uint32_t aLimited;
		if (aFull + gConfig.mBlockHeight * -3 < aLength)
			aLimited = aFull + gConfig.mBlockHeight * -3;
		else
			aLimited = aLength;
		aLength = aLimited;
		uint32_t aLimitedB;
		if (aFull + gConfig.mBlockHeight * -3 < aLengthB)
			aLimitedB = aFull + gConfig.mBlockHeight * -3;
		else
			aLimitedB = aLengthB;
		uint32_t aRest;
		if (aFull - aLimited < aLimitedB)
			aRest = aFull - aLimited;
		else
			aRest = aLimitedB;
		aLengthB = aRest;
	}
	if (((aColumn != mGlowColumn) || (mGlowValidA == 0)) ||
		((((mField->mNext.mFlags & PF_PREVIEW) != 0 && (aLength != 0)) &&
			((aSecondColumn != (uint32_t)mGlowColumnB || (mGlowValidB == 0))))))
	{
		mGlowTime = GameTime();
	}
	uint32_t aNow = GameTime();
	uint32_t aLastTime = aNow;
	if (mGlowTime <= aNow)
		aLastTime = mGlowTime;
	mGlowTime = aLastTime;
	uint32_t anElapsed = aNow - mGlowTime;
	uint32_t aPhase = (anElapsed / gConfig.mBkgcolResolutiontime) * gConfig.mBkgcolResolutiontime;
	uint32_t aRange = gConfig.mBkgcolMaxbrightness - gConfig.mBkgcolMinbrightness;
	int32_t aBase;
	{
		double aWave = (sin(((double)aPhase * 6.2832) / (double)(uint32_t)gConfig.mBkgcolCycletime) + 1.0) / 2.0 * (double)aRange;
		if ((int32_t)Ftol(aWave) < 0)
			aBase = 0;
		else
			aBase = (int32_t)Ftol(aWave);
		if (aBase < 0x100)
		{
			if ((int32_t)Ftol(aWave) < 0)
				aBase = 0;
			else
				aBase = (int32_t)Ftol(aWave);
		}
		else
		{
			aBase = 0x100;
		}
	}
	uint32_t aBrightness;
	if ((uint32_t)(gConfig.mBkgcolMinbrightness + aBase) < 0x100)
		aBrightness = gConfig.mBkgcolMinbrightness + aBase;
	else
		aBrightness = 0x100;
	if (aLength != 0)
	{
		if ((uint32_t)(aBrightness + gConfig.mBkgcolAddbrightness) < 0x100)
			aBrightness = aBrightness + gConfig.mBkgcolAddbrightness;
		else
			aBrightness = 0x100;
	}
	int16_t aShortRadius = (int16_t)aRadius;
	uint32_t aGlowWidth;
	int32_t aTop;
	int32_t aBottom;
	int16_t aX;
	int16_t aY;
	uint16_t aW;
	uint16_t aH;
	if (((((int32_t)aLength != mGlowLength) || (aColumn != mGlowColumn)) || (aLowRow != mGlowLowRow)) ||
		((aHighRow != mGlowHighRow || (mGlowValidA == 0)) || ((int32_t)aBrightness != mGlowBrightness)))
	{
		aGlowWidth = gConfig.mBkgcoldx[aColumn] + gConfig.mBkgcolShrink * -2;
		if (aLowRow == -1)
			aTop = 0;
		else
			aTop = aLowRow * mBlockHeight;
		aTop = aTop + gConfig.mBlockgridY;
		if (aHighRow == -1)
			aBottom = mBlockHeight * mRows;
		else
			aBottom = (aHighRow + 1) * mBlockHeight;
		aBottom = aBottom + gConfig.mBlockgridY;
		aX = (int16_t)(((int16_t)gConfig.mBkgcolx[aColumn] - aShortRadius) + (int16_t)gConfig.mBkgcolShrink + (int16_t)gConfig.mBlockgridX);
		aY = (int16_t)(((int16_t)gConfig.mBlockgridY + (int16_t)gConfig.mBkgcolyoffset) - aShortRadius);
		aW = (uint16_t)((int16_t)aGlowWidth + aShortRadius * 2);
		if (aLength == 0)
			aH = (uint16_t)((int16_t)gConfig.mBlockHeight * (int16_t)gConfig.mNumRows + (int16_t)gConfig.mBkgcolyoffset * -2 + aShortRadius * 2);
		else
			aH = (uint16_t)(aShortRadius + (int16_t)aLength);
		uint32_t aScreenWidth = lowos::SurfaceWidth(theScreen);
		int32_t aSourcePitch = (aScreenWidth & 0xffff) * aBytesPerPixel;
		lowos::BlitPixels(&aFormat, &aFormat, mGlowBuffer,
			lowos::SurfaceBits(theScreen) + aBytesPerPixel * (int32_t)aX + aSourcePitch * aY, aH, aW, mGlowPitch,
			aSourcePitch, 0, aBytesPerPixel, aBytesPerPixel, 0, 0xff);
		int16_t aDestX = (int16_t)gConfig.mBkghix;
		if (aLowRow != aHighRow)
		{
			int16_t aSourceX = 0;
			int16_t aSourceY = (int16_t)((int16_t)aTop - (int16_t)gConfig.mBkghiy);
			uint32_t aGlowWidth = lowos::SurfaceWidth(mGlowTop);
			uint16_t aClipW = (uint16_t)aGlowWidth;
			uint32_t aGlowHeight;
			if (aBottom == aTop || aBottom - aTop < 0)
				aGlowHeight = 0;
			else
				aGlowHeight = aBottom - aTop;
			uint16_t aClipH = (uint16_t)aGlowHeight;
			aDestX = (int16_t)(aDestX - aX);
			int16_t aDestY = (int16_t)((int16_t)aTop - aY);
			if ((((int32_t)aDestX < (int32_t)(uint32_t)aW) && ((int32_t)aDestY < (int32_t)(uint32_t)aH)) &&
				((0 < (int32_t)((int32_t)aDestX + (aGlowWidth & 0xffff))) &&
					(0 < (int32_t)((int32_t)aDestY + (aGlowHeight & 0xffff)))))
			{
				if (aDestX < 0)
				{
					aClipW = (uint16_t)(aClipW + aDestX);
					aSourceX = (int16_t)-aDestX;
					aDestX = 0;
				}
				if (aDestY < 0)
				{
					aClipH = (uint16_t)((uint16_t)aGlowHeight + aDestY);
					aSourceY = (int16_t)(aSourceY - aDestY);
					aDestY = 0;
				}
				if ((int32_t)(uint32_t)aW < (int32_t)((int32_t)aDestX + (uint32_t)aClipW))
					aClipW = (uint16_t)(aW - aDestX);
				if ((int32_t)(uint32_t)aH < (int32_t)((int32_t)aDestY + (uint32_t)aClipH))
					aClipH = (uint16_t)(aH - aDestY);
				aScreenWidth = lowos::SurfaceWidth(mGlowTop);
				aSourcePitch = (aScreenWidth & 0xffff) * aBytesPerPixel;
				lowos::BlitPixels(&aFormat, &aFormat, mGlowBuffer + mGlowPitch * aDestY + aBytesPerPixel * (int32_t)aDestX,
					lowos::SurfaceBits(mGlowTop) + aBytesPerPixel * (int32_t)aSourceX + aSourcePitch * aSourceY, aClipH,
					aClipW, mGlowPitch, aSourcePitch, 0, aBytesPerPixel, aBytesPerPixel, 0, 0xff);
			}
		}
		memcpy(mGlowBufferA, mGlowBuffer, mGlowPitch * (uint32_t)aH);
		uint32_t i;
		for (i = 0; (int32_t)i < aRadius; i = i + 1)
			GlowColumn(i, i, aGlowWidth, aBytesPerPixel, aBrightness);
		if (aLength == 0)
		{
			for (i = aRadius; (int32_t)i < (int32_t)((uint32_t)aH - aRadius); i = i + 1)
				GlowColumn(aRadius, i, aGlowWidth, aBytesPerPixel, aBrightness);
			for (i = 0; (int32_t)i < aRadius; i = i + 1)
				GlowColumn((aRadius - 1) - i, i + ((uint32_t)aH - aRadius), aGlowWidth, aBytesPerPixel, aBrightness);
		}
		else
		{
			for (i = 0; i < aLength; i = i + 1)
				GlowColumn(aRadius + (i << 8) / aLength, i + aRadius, aGlowWidth, aBytesPerPixel, aBrightness);
		}
		lowos::BlitPixels(&aFormat, &aFormat, mGlowBuffer, mGlowAlpha, aH, aW, mGlowPitch, mGlowAlphaPitch, 2,
			aBytesPerPixel, aBytesPerPixel, 0, 0xff);
		lowos::BlitPixels(&aFormat, &aFormat, mGlowBufferA, mGlowBuffer, aH, aW, mGlowPitch, mGlowPitch, 0x10,
			aBytesPerPixel, aBytesPerPixel, 0, 0xff);
		mGlowLength = aLength;
		mGlowColumn = aColumn;
		mGlowValidA = 1;
	}
	if ((((mField->mNext.mFlags & PF_PREVIEW) != 0) && (aLength != 0)) &&
		(((aLengthB != (uint32_t)mGlowLengthB ||
				(((aSecondColumn != (uint32_t)mGlowColumnB || (aLowRow != mGlowLowRow)) || (aHighRow != mGlowHighRow)))) ||
			((mGlowValidB == 0 || ((int32_t)aBrightness != mGlowBrightness))))))
	{
		aGlowWidth = gConfig.mBkgcoldx[aSecondColumn] + gConfig.mBkgcolShrink * -2;
		if (aLowRow == -1)
			aTop = 0;
		else
			aTop = aLowRow * mBlockHeight;
		aTop = aTop + gConfig.mBlockgridY;
		if (aHighRow == -1)
			aBottom = mBlockHeight * mRows;
		else
			aBottom = (aHighRow + 1) * mBlockHeight;
		aBottom = aBottom + gConfig.mBlockgridY;
		aX = (int16_t)(((int16_t)gConfig.mBkgcolx[aSecondColumn] - aShortRadius) + (int16_t)gConfig.mBkgcolShrink + (int16_t)gConfig.mBlockgridX);
		aW = (uint16_t)((int16_t)aGlowWidth + aShortRadius * 2);
		if (aLengthB == 0)
			aH = (uint16_t)((int16_t)gConfig.mBlockHeight * (int16_t)gConfig.mNumRows + (int16_t)gConfig.mBkgcolyoffset * -2 + aShortRadius * 2);
		else
			aH = (uint16_t)(aShortRadius + (int16_t)aLengthB);
		aY = (int16_t)((((int16_t)gConfig.mBlockgridY + (int16_t)gConfig.mBlockHeight * (int16_t)gConfig.mNumRows) -
				(int16_t)gConfig.mBkgcolyoffset + aShortRadius) - (int16_t)aH);
		uint32_t aScreenWidth = lowos::SurfaceWidth(theScreen);
		int32_t aSourcePitch = (aScreenWidth & 0xffff) * aBytesPerPixel;
		lowos::BlitPixels(&aFormat, &aFormat, mGlowBuffer,
			lowos::SurfaceBits(theScreen) + aBytesPerPixel * (int32_t)aX + aSourcePitch * aY, aH, aW, mGlowPitch,
			aSourcePitch, 0, aBytesPerPixel, aBytesPerPixel, 0, 0xff);
		int16_t aDestX = (int16_t)gConfig.mBkghix;
		if (aLowRow != aHighRow)
		{
			int16_t aSourceX = 0;
			int16_t aSourceY = (int16_t)((int16_t)aTop - (int16_t)gConfig.mBkghiy);
			uint32_t aGlowWidth = lowos::SurfaceWidth(mGlowTop);
			uint16_t aClipW = (uint16_t)aGlowWidth;
			uint16_t aClipH;
			if (aBottom == aTop || aBottom - aTop < 0)
				aClipH = 0;
			else
				aClipH = (uint16_t)(aBottom - aTop);
			aDestX = (int16_t)(aDestX - aX);
			int16_t aDestY = (int16_t)((int16_t)aTop - aY);
			uint16_t aGlowHeight = aClipH;
			if ((((int32_t)aDestX < (int32_t)(uint32_t)aW) && ((int32_t)aDestY < (int32_t)(uint32_t)aH)) &&
				((0 < (int32_t)((int32_t)aDestX + (aGlowWidth & 0xffff))) && (0 < (int32_t)((int32_t)aDestY + (uint32_t)aGlowHeight))))
			{
				if (aDestX < 0)
				{
					aClipW = (uint16_t)(aClipW + aDestX);
					aSourceX = (int16_t)-aDestX;
					aDestX = 0;
				}
				if (aDestY < 0)
				{
					aClipH = (uint16_t)(aClipH + aDestY);
					aSourceY = (int16_t)(aSourceY - aDestY);
					aDestY = 0;
				}
				if ((int32_t)(uint32_t)aW < (int32_t)((int32_t)aDestX + (uint32_t)aClipW))
					aClipW = (uint16_t)(aW - aDestX);
				if ((int32_t)(uint32_t)aH < (int32_t)((int32_t)aDestY + (uint32_t)aClipH))
					aClipH = (uint16_t)(aH - aDestY);
				aScreenWidth = lowos::SurfaceWidth(mGlowTop);
				aSourcePitch = (aScreenWidth & 0xffff) * aBytesPerPixel;
				lowos::BlitPixels(&aFormat, &aFormat, mGlowBuffer + mGlowPitch * aDestY + aBytesPerPixel * (int32_t)aDestX,
					lowos::SurfaceBits(mGlowTop) + aBytesPerPixel * (int32_t)aSourceX + aSourcePitch * aSourceY, aClipH,
					aClipW, mGlowPitch, aSourcePitch, 0, aBytesPerPixel, aBytesPerPixel, 0, 0xff);
			}
		}
		memcpy(mGlowBufferB, mGlowBuffer, mGlowPitch * (uint32_t)aH);
		uint32_t i;
		for (i = 0; (int32_t)i < aRadius; i = i + 1)
			GlowColumn((aRadius - 1) - i, i + ((uint32_t)aH - aRadius), aGlowWidth, aBytesPerPixel, aBrightness);
		if (aLengthB == 0)
		{
			for (i = aRadius; (int32_t)i < (int32_t)((uint32_t)aH - aRadius); i = i + 1)
				GlowColumn(aRadius, i, aGlowWidth, aBytesPerPixel, aBrightness);
			for (i = 0; (int32_t)i < aRadius; i = i + 1)
				GlowColumn(i, i, aGlowWidth, aBytesPerPixel, aBrightness);
		}
		else
		{
			for (i = 0; i < aLengthB; i = i + 1)
				GlowColumn((aRadius + 0xff) - (i << 8) / aLengthB, i, aGlowWidth, aBytesPerPixel, aBrightness);
		}
		lowos::BlitPixels(&aFormat, &aFormat, mGlowBuffer, mGlowAlpha, aH, aW, mGlowPitch, mGlowAlphaPitch, 2,
			aBytesPerPixel, aBytesPerPixel, 0, 0xff);
		lowos::BlitPixels(&aFormat, &aFormat, mGlowBufferB, mGlowBuffer, aH, aW, mGlowPitch, mGlowPitch, 0x10,
			aBytesPerPixel, aBytesPerPixel, 0, 0xff);
		mGlowLengthB = aLengthB;
		mGlowColumnB = aSecondColumn;
		mGlowValidB = 1;
	}
	mGlowLowRow = aLowRow;
	mGlowHighRow = aHighRow;
	mGlowBrightness = aBrightness;
	if (aLength == 0)
		aH = (uint16_t)((int16_t)gConfig.mBlockHeight * (int16_t)gConfig.mNumRows + (int16_t)gConfig.mBkgcolyoffset * -2 + aShortRadius * 2);
	else
		aH = (uint16_t)(aShortRadius + (int16_t)aLength);
	lowos::Blit((uint16_t)((int16_t)gConfig.mBkgcoldx[aColumn] + aShortRadius * 2 + (int16_t)gConfig.mBkgcolShrink * -2), aH,
		(int16_t)(((int16_t)gConfig.mBkgcolx[aColumn] - aShortRadius) + (int16_t)gConfig.mBkgcolShrink + (int16_t)gConfig.mBlockgridX),
		(int16_t)(((int16_t)gConfig.mBlockgridY + (int16_t)gConfig.mBkgcolyoffset) - aShortRadius), 0, 0, mGlowPitch,
		mGlowBufferA, 1, 0xff);
	if (((mField->mNext.mFlags & PF_PREVIEW) != 0) && (aLength != 0))
	{
		if (aLengthB == 0)
			aH = (uint16_t)((int16_t)gConfig.mBlockHeight * (int16_t)gConfig.mNumRows + (int16_t)gConfig.mBkgcolyoffset * -2 + aShortRadius * 2);
		else
			aH = (uint16_t)(aShortRadius + (int16_t)aLengthB);
		lowos::Blit((uint16_t)((int16_t)gConfig.mBkgcoldx[aSecondColumn] + aShortRadius * 2 + (int16_t)gConfig.mBkgcolShrink * -2), aH,
			(int16_t)(((int16_t)gConfig.mBkgcolx[aSecondColumn] - aShortRadius) + (int16_t)gConfig.mBkgcolShrink +
				(int16_t)gConfig.mBlockgridX),
			(int16_t)(((((int16_t)gConfig.mBlockgridY + (int16_t)gConfig.mBlockHeight * (int16_t)gConfig.mNumRows) -
						(int16_t)gConfig.mBkgcolyoffset) + aShortRadius) - (int16_t)aH),
			0, 0, mGlowPitch, mGlowBufferB, 1, 0xff);
	}
}

namespace
{

// The game clock of the board's frames, in 35 ms ticks: the arrows over the columns move one row per tick (0x477050)
uint32_t gTickClock = 0xffffffff;

} // namespace

// 0x40e766: the board's frame. Updates the field (unless it is paused, held or over) and decides whether the game or the
// level is over; draws the background (theBackground, with the lit area of the field) and the glow, the arrows over the
// columns the player can drop in, the pieces from the middle row out (a dissolving one with its effect's fade, the
// ones flying in or out with their motion), the preview blocks, the covers and plays the shake sounds.
void Board::Draw(lowos::Surface* theBackground)
{
	uint32_t aNow = GameTime();
	int32_t aTicks = 0;
	uint32_t aBytesPerPixel = lowos::GetDisplayFormat(nullptr, nullptr, nullptr);
	if (aNow < gTickClock)
		gTickClock = aNow;
	for (; 0x22 < aNow - gTickClock; gTickClock = gTickClock + 0x23)
	{
		aTicks = aTicks + 1;
	}
	Nothing();
	if (gBusy != 0)
		ResetClocks();
	if (mEnding == 0 && gBusy == 0)
	{
		UpdateSpawnTimer();
		mField->Update();
		mField->ClearLockBit();
		if (gBonusRound == 0)
		{
			if (mField->IsIdle() == 0)
			{
				if (gGameMode == 1)
				{
					double aMeter = mField->GetMeter();
					if (aMeter <= 0.0 ||
						(double)(uint32_t)CfgGoal(mState.mConfig) <= mField->GetMeter() ||
						mField->GetLowRow() == -1 || mField->GetHighRow() == -1)
					{
						mField->SetLockBit();
					}
				}
				else
				{
					if (mField->GetLowRow() == -1 || mField->GetHighRow() == -1 || mField->GetBlocksLeft() == 0)
						mField->SetLockBit();
				}
			}
			else if (gGameMode == 1)
			{
				double aMeter = mField->GetMeter();
				if (aMeter > 0.0)
				{
					if (mState.mConfig.mWinningPrecedence == 0)
					{
						if (mField->GetLowRow() == -1 || mField->GetHighRow() == -1)
						{
							mGameOver = 1;
						}
						else
						{
							aMeter = mField->GetMeter();
							if ((double)(uint32_t)CfgGoal(mState.mConfig) <= aMeter)
								mLevelComplete = 1;
						}
					}
					else
					{
						aMeter = mField->GetMeter();
						if ((double)(uint32_t)CfgGoal(mState.mConfig) <= aMeter)
						{
							mLevelComplete = 1;
						}
						else if (mField->GetLowRow() == -1 || mField->GetHighRow() == -1)
						{
							mGameOver = 1;
						}
					}
				}
				else
				{
					mGameOver = 1;
					gApp.SetCrossed();
				}
			}
			else if (mState.mConfig.mWinningPrecedence == 0)
			{
				if (mField->GetLowRow() == -1 || mField->GetHighRow() == -1)
				{
					mGameOver = 1;
				}
				else if (mField->GetBlocksLeft() == 0)
				{
					mLevelComplete = 1;
				}
			}
			else
			{
				if (mField->GetBlocksLeft() == 0)
				{
					mLevelComplete = 1;
				}
				else if (mField->GetLowRow() == -1 || mField->GetHighRow() == -1)
				{
					mGameOver = 1;
				}
			}
		}
		else if (mField->IsIdle() == 0)
		{
			if (mField->GetLowRow() != -1 && mField->GetHighRow() != -1)
			{
				int32_t aLowRow = mField->GetLowRow();
				int32_t aHighRow = mField->GetHighRow();
				if (aLowRow != aHighRow && mNextBlocks[0].mKind != KIND_NONE && gApp.mBonusElapsed / 1000 < (uint32_t)gConfig.mBonusTime)
					goto AfterBonusChecks;
			}
			mField->SetLockBit();
		}
		else
		{
			int32_t aLowRow = mField->GetLowRow();
			int32_t aHighRow = mField->GetHighRow();
			if (aLowRow == aHighRow)
			{
				mLevelComplete = 1;
				gApp.mBonusClockStopped = 1;
			}
			else
			{
				if (mField->GetLowRow() == -1 || mField->GetHighRow() == -1)
				{
					mGameOver = 1;
					gApp.mBonusClockStopped = 1;
				}
				else if (mNextBlocks[0].mKind == KIND_NONE)
				{
					gApp.mBonusBlocksRanOut = 1;
					mGameOver = 1;
					gApp.mBonusClockStopped = 1;
				}
				else if (gApp.mBonusElapsed / 1000 >= (uint32_t)gConfig.mBonusTime)
				{
					gApp.mCrossed = 1;
					mGameOver = 1;
					gApp.mBonusClockStopped = 1;
				}
			}
		}
	}
AfterBonusChecks:
	{
		int32_t aTopY;
		int32_t aBottomY;
		int32_t aFirstRow = mField->GetLowRow();
		if (aFirstRow == -1)
			aFirstRow = 0;
		else
			aFirstRow = aFirstRow * mBlockHeight;
		aTopY = aFirstRow + mY;
		int32_t aLastRow = mField->GetHighRow();
		if (aLastRow == -1)
			aBottomY = mBlockHeight * mRows;
		else
			aBottomY = (aLastRow + 1) * mBlockHeight;
		aBottomY = aBottomY + mY;
		if (mField->GetLowRow() == mField->GetHighRow())
		{
			lowos::BlitRegistered(GAME_WIDTH, GAME_HEIGHT, 0, 0, 0, 0, theBackground);
		}
		else
		{
			if (aTopY != 0)
				lowos::BlitRegistered(GAME_WIDTH, aTopY, 0, 0, 0, 0, theBackground);
			uint32_t aGlowWidth = lowos::SurfaceWidth(mGlowTop);
			lowos::BlitRegistered(aGlowWidth & 0xffff, aBottomY - aTopY, gConfig.mBkghix, aTopY, 0, aTopY - gConfig.mBkghiy,
				mGlowTop);
			if (gConfig.mBkghix != 0)
				lowos::BlitRegistered(gConfig.mBkghix, aBottomY - aTopY, 0, aTopY, 0, aTopY, theBackground);
			aGlowWidth = lowos::SurfaceWidth(mGlowTop);
			if (gConfig.mBkghix + (aGlowWidth & 0xffff) < GAME_WIDTH)
			{
				uint32_t aWidth = lowos::SurfaceWidth(mGlowTop);
				int32_t aRight = gConfig.mBkghix + (aWidth & 0xffff);
				aWidth = lowos::SurfaceWidth(mGlowTop);
				lowos::BlitRegistered(GAME_WIDTH - (gConfig.mBkghix + (aWidth & 0xffff)), aBottomY - aTopY, aRight, aTopY, aRight,
					aTopY, theBackground);
			}
			if (aBottomY != GAME_HEIGHT)
				lowos::BlitRegistered(GAME_WIDTH, GAME_HEIGHT - aBottomY, 0, aBottomY, 0, aBottomY, theBackground);
		}
	}
	if (gBusy == 0 && mEnding == 0)
		DrawGlow(theBackground);
	int32_t anEnterCount = 0;
	int32_t anEnteredCount = 0;
	int32_t aLeaveCount = 0;
	int32_t aLeftCount = 0;
	uint32_t aPieceIndex = 0;
	do
	{
		uint32_t aSize = (uint32_t)mField->GetSize();
		if (aSize <= aPieceIndex || (gPauseFlags != 0 && gApp.mScreen == SCREEN_GAME))
		{
			if (mState.mConfig.mDisableStops == 0 && gBonusRound == 0 &&
				((mField->GetLowRow() == -1 || mState.mConfig.mPrimaryStopCenter != 0) && gBusy == 0 &&
					gApp.mScreen == SCREEN_GAME))
			{
				for (uint32_t aColumn = 0; aColumn < mColumns; aColumn++)
				{
					uint32_t aClear = mField->ColumnClear(aColumn);
					if ((aClear & 0xff) == 0)
					{
						mArrowY[aColumn] = -1;
					}
					else
					{
						uint32_t aHigh = mField->GetHighRow();
						uint32_t aLow = mField->GetLowRow();
						if (aLow == 0xffffffff && aHigh != 0xffffffff)
							aLow = 0;
						if (aHigh == 0xffffffff && aLow != 0xffffffff)
							aHigh = mRows - 1;
						uint32_t aStop;
						if (CfgStopRow(mState.mConfig) == -1)
						{
							if (aHigh == aLow)
								aStop = (mRows >> 1) + CfgStartRowsAdd(mState.mConfig);
							else
								aStop = (int32_t)(aHigh + aLow) / 2 + CfgStopRowAdd(mState.mConfig);
						}
						else
						{
							aStop = CfgStopRow(mState.mConfig) + CfgStopRowAdd(mState.mConfig);
						}
						uint32_t aMin;
						if (aLow == aHigh || aLow == 0)
							aMin = 1;
						else
							aMin = aLow;
						uint32_t aClamped;
						if ((int32_t)aStop < (int32_t)aMin)
						{
							if (aLow == aHigh || aLow == 0)
								aClamped = 1;
							else
								aClamped = aLow;
						}
						else
						{
							aClamped = aStop;
						}
						uint32_t aMax;
						if (aLow == aHigh || aHigh == gConfig.mNumRows - 1U)
							aMax = gConfig.mNumRows - 2;
						else
							aMax = aHigh;
						uint32_t aTargetRow;
						if (aMax < aClamped)
						{
							if (aLow == aHigh || aHigh == gConfig.mNumRows - 1U)
								aTargetRow = gConfig.mNumRows - 2;
							else
								aTargetRow = aHigh;
						}
						else
						{
							aTargetRow = aClamped;
						}
						aStop = aTargetRow * mBlockHeight;
						if (mArrowY[aColumn] == -1)
						{
							mArrowY[aColumn] = aStop;
						}
						else if (mArrowY[aColumn] < (int32_t)aStop)
						{
							mArrowY[aColumn] = mArrowY[aColumn] + aTicks;
							if ((int32_t)aStop < mArrowY[aColumn])
								mArrowY[aColumn] = aStop;
						}
						else
						{
							mArrowY[aColumn] = mArrowY[aColumn] - aTicks;
							if (mArrowY[aColumn] < (int32_t)aStop)
								mArrowY[aColumn] = aStop;
						}
						uint64_t aWideTime = aNow;
						uint32_t aArrowWidth = lowos::SurfaceWidth(mArrows);
						uint32_t aSourceX = (int32_t)((aWideTime / 0x32) % 4) * (aArrowWidth & 0xffff) >> 2;
						aArrowWidth = lowos::SurfaceWidth(mArrows);
						int32_t aSourceEnd = aSourceX + (aArrowWidth & 0xffff) / 4;
						int32_t aSourceY = 0;
						uint16_t anArrowHeight = lowos::SurfaceHeight(mArrows);
						uint32_t aSourceBottom = anArrowHeight;
						uint8_t* aBits = lowos::SurfaceBits(mArrows);
						aArrowWidth = lowos::SurfaceWidth(mArrows);
						int32_t aPitch = (aArrowWidth & 0xffff) * aBytesPerPixel;
						anArrowHeight = lowos::SurfaceHeight(mArrows);
						int32_t aDestY = (mY + mArrowY[aColumn] + mBlockHeight / 2) - anArrowHeight / 2;
						aArrowWidth = lowos::SurfaceWidth(mArrows);
						lowos::Blit(aSourceEnd - aSourceX, aSourceBottom - aSourceY,
							(mX + aColumn * mBlockWidth + mBlockWidth / 2) - (aArrowWidth & 0xffff) / 8, aDestY, aSourceX,
							aSourceY, aPitch, aBits, 0x11, 0xff);
					}
				}
			}
			if (mEnding == 0)
			{
				uint32_t aStartDelay;
				uint32_t aSlideEnd;
				uint32_t aSlideStart;
				if (mQueueShifting == 1)
				{
					aStartDelay = gConfig.mRemoveDelay * 6;
					aSlideEnd = aStartDelay +
						(uint32_t)(gConfig.mTapDelay * gConfig.mBlockHeight) / (uint32_t)gConfig.mTapSpeed;
					aSlideStart = aSlideEnd + aStartDelay;
				}
				else
				{
					aStartDelay = 0;
					aSlideEnd = 0;
					aSlideStart = 0;
				}
				uint32_t aPreviewTime = GameTime();
				if (aPreviewTime < mNextBlocks[0].mMoveTime)
					mNextBlocks[0].mMoveTime = aPreviewTime;
				uint32_t anAge = aPreviewTime - mNextBlocks[0].mMoveTime;
				for (uint32_t aSlot = 0; aSlot < 4; aSlot++)
				{
					Piece aCopy(mNextBlocks[aSlot]);
					if (aCopy.mKind == KIND_NONE)
					{
					}
					else
					{
						if (aPreviewTime < aCopy.mSeed)
							aPreviewTime = aPreviewTime + 0x7fff;
						aCopy.mFrame = ((aPreviewTime - aCopy.mSeed) / 0x7d) % 4;
						if (aCopy.mFrame != 0)
							aCopy.mFrame = aCopy.mFrame + 5;
						if (aSlot == 0 && aStartDelay <= anAge)
						{
						}
						else if (aSlot == 3 && anAge < aSlideEnd)
						{
						}
						else
						{
							uint32_t aSourceX;
							if (aSlot == 3 && anAge < aSlideStart)
								aSourceX = mBlockWidth * (5 - (anAge - aSlideEnd) / gConfig.mRemoveDelay);
							else if (aSlot == 0 && anAge < aStartDelay)
								aSourceX = mBlockWidth * (anAge / gConfig.mRemoveDelay);
							else
								aSourceX = mBlockWidth * aCopy.mFrame;
							int32_t aSourceEnd = aSourceX + mBlockWidth;
							int32_t aSourceY = aCopy.mKind * mBlockHeight;
							uint32_t aSourceBottom = aSourceY + mBlockHeight;
							int32_t aRise = 0;
							if (aStartDelay <= anAge && anAge < aSlideEnd)
								aRise = mBlockHeight - (int32_t)(((anAge - aStartDelay) * (uint32_t)gConfig.mTapSpeed) /
										(uint32_t)gConfig.mTapDelay);
							if (anAge < aStartDelay && aSlot != 0)
								aRise = mBlockHeight;
							lowos::BlitRegistered(aSourceEnd - aSourceX, aSourceBottom - aSourceY,
								aCopy.mX + gConfig.mIngameNextblocksx, aRise + ((uint32_t)aCopy.mY >> FIXED_SHIFT) + gConfig.mIngameNextblocksy,
								aSourceX, aSourceY, mBlocks);
						}
					}
				}
				if (gBusy == 0 && mNextBlocks[0].mKind != KIND_NONE)
				{
					aPreviewTime = GameTime();
					if (aPreviewTime < mNextBlocks[0].mSeed)
						aPreviewTime = aPreviewTime + 0x7fff;
					mNextBlocks[0].mFrame = (aPreviewTime - mNextBlocks[0].mSeed) / 0x7d;
					mNextBlocks[0].mFrame = mNextBlocks[0].mFrame % 4;
					if (mNextBlocks[0].mFrame != 0)
						mNextBlocks[0].mFrame = mNextBlocks[0].mFrame + 5;
					int32_t aDropColumn = (int32_t)Send(gApp.mColumnCursor, MSG_CURSOR_COLUMN, 0, 0);
					int32_t aDestX = aDropColumn * mBlockWidth;
					int32_t aDestY = 0;
					if (mField->CanPlace(aDropColumn) != 0)
					{
						uint32_t aSourceX;
						if (anAge < (uint32_t)(gConfig.mRemoveDelay * 6))
							aSourceX = mBlockWidth * (5 - anAge / gConfig.mRemoveDelay);
						else
							aSourceX = mBlockWidth * mNextBlocks[0].mFrame;
						int32_t aSourceEnd = aSourceX + mBlockWidth;
						int32_t aSourceY = mNextBlocks[0].mKind * mBlockHeight;
						uint32_t aSourceBottom = aSourceY + mBlockHeight;
						if ((((mLastColumn != aDropColumn) && ((mField->mNext.mFlags & PF_PREVIEW) != 0)) &&
								((&gConfig.mFloaterdeterminationNormal)[gGameMode] == 2)) &&
							(mField->mQueueCount == 0))
						{
							mField->mNext.mX = ((gConfig.mNumColumns - 1) - aDropColumn) * mBlockWidth;
						}
						if (mLastColumn == -1)
						{
							mLastColumn = aDropColumn;
						}
						else if (mLastColumn != aDropColumn)
						{
							mLastColumn = aDropColumn;
							Play(9, 1);
						}
						lowos::BlitRegistered(aSourceEnd - aSourceX, aSourceBottom - aSourceY, aDestX + mX, aDestY + mY,
							aSourceX, aSourceY, mBlocks);
					}
					if ((mField->mNext.mFlags & PF_PREVIEW) != 0)
					{
						Field* aField = mField;
						aPreviewTime = GameTime();
						if (aPreviewTime < aField->mNext.mSeed)
							aPreviewTime = aPreviewTime + 0x7fff;
						anAge = aPreviewTime - aField->mNext.mMoveTime;
						aField->mNext.mFrame = (aPreviewTime - aField->mNext.mSeed) / 0x7d;
						aField->mNext.mFrame = aField->mNext.mFrame % 4;
						if (aField->mNext.mFrame != 0)
							aField->mNext.mFrame = aField->mNext.mFrame + 5;
						uint32_t aSourceX = mBlockWidth * aField->mNext.mFrame;
						int32_t aSourceEnd = aSourceX + mBlockWidth;
						int32_t aSourceY = aField->mNext.mKind * mBlockHeight;
						uint32_t aSourceBottom = aSourceY + mBlockHeight;
						uint32_t aCanSpawn = mField->CanSpawn((uint32_t)aField->mNext.mX / mBlockWidth);
						if (aCanSpawn != 0)
						{
							uint32_t aBlink = (anAge % gConfig.mNextfloaterBlinkperiod << 8) / gConfig.mNextfloaterBlinkperiod;
							if (gConfig.mNextfloaterBlinkperiod <= anAge && aBlink < 0x80)
								aBlink = 0xff - aBlink;
							if ((&gConfig.mBkgcolLengthNormal)[gGameMode] != 0)
								aBlink = (aBlink & 0xffffff00) | 0xff;
							lowos::BlitLevel(aSourceEnd - aSourceX, aSourceBottom - aSourceY, aField->mNext.mX + mX,
								((uint32_t)aField->mNext.mY >> FIXED_SHIFT) + mY, aSourceX, aSourceY, mBlocks, (uint8_t)aBlink);
						}
					}
				}
			}
			if (anEnterCount != 0)
			{
				lowos::BlitRegistered(lowos::SurfaceWidth(mTopCover) & 0xffff, lowos::SurfaceHeight(mTopCover),
					gConfig.mTopcoverX, gConfig.mTopcoverY, 0, 0, mTopCover);
			}
			if (aLeaveCount != 0)
			{
				lowos::BlitRegistered(lowos::SurfaceWidth(mBottomCover) & 0xffff, lowos::SurfaceHeight(mBottomCover),
					gConfig.mBottomcoverX, gConfig.mBottomcoverY, 0, 0, mBottomCover);
			}
			if (anEnterCount != 0 && anEnterCount == anEnteredCount)
				Command(nullptr, CMD_LEVEL_START_SHOWN);
			if (aLeaveCount != 0 && aLeaveCount == aLeftCount)
			{
				FreeHandler(gApp.mCounted[0]);
				AppNextSummaryEvent(&gApp);
			}
			int32_t aRowFromLow = mField->GetLowRow();
			uint32_t aFromLow = -(uint32_t)(aRowFromLow != -1) & (aRowFromLow + 1U);
			int32_t aRowFromHigh = mField->GetHighRow();
			uint32_t aFromHigh;
			if (aRowFromHigh == -1)
				aFromHigh = 0;
			else
				aFromHigh = mRows - aRowFromHigh;
			uint32_t aNearest = aFromHigh;
			if ((int32_t)aFromLow < (int32_t)aFromHigh)
				aNearest = aFromLow;
			if (((((int32_t)aNearest < 5) && (gApp.mScreen == SCREEN_GAME)) && (mGameOver == 0)) &&
				(((mLevelComplete == 0 && (gBusy == 0)) && (gBonusRound == 0))))
			{
				for (uint32_t aSound = 0; aSound < 5; aSound++)
				{
					int32_t anIndex = 0;
					bool isLooped = false;
					while (true)
					{
						if (((int32_t)aNearest <= anIndex) &&
							((uint32_t)(&gConfig.mNormalShakesounds[0])[anIndex + gGameMode * 5] == aSound))
						{
							Play(0x27 + aSound, 2);
							isLooped = true;
							break;
						}
						if (anIndex == 4)
							break;
						anIndex = anIndex + 1;
					}
					if (!isLooped)
						Play(0x27 + aSound, 0);
				}
			}
			else
			{
				for (uint32_t aSound = 0; aSound < 5; aSound++)
					Play(0x27 + aSound, 0);
			}
			return;
		}
		uint8_t aLevel = 0xff;
		uint32_t aCell;
		if (((mRows >> 1) * mColumns < aPieceIndex || gApp.mScreen != SCREEN_GAME) || mGameOver != 0 || mLevelComplete != 0)
			aCell = aPieceIndex;
		else
			aCell = (mRows >> 1) * mColumns - aPieceIndex;
		Piece* aPiece = mField->PieceAt(aCell);
		bool isDeleteAfter = false;
		if (aPiece != nullptr)
		{
			if ((aPiece->mFlags & PF_REMOVING) != 0)
			{
				if (aNow < aPiece->mSeed)
					aPiece->mSeed = aNow;
				uint8_t anEffectFlashCount = 0;
				int32_t anEffectKind = aPiece->mEffectKind;
				if (anEffectKind == KIND_COLORBOMB)
				{
					anEffectFlashCount = gConfig.mColorbombEffects.mFlashCount;
				}
				else if (anEffectKind == KIND_COLUMNNUKE)
				{
					anEffectFlashCount = gConfig.mColumnnukeEffects.mFlashCount;
				}
				else if (anEffectKind == KIND_NONE)
				{
					uint32_t anElapsed = aNow - aPiece->mSeed;
					if (aPiece->mEffectDelay <= anElapsed)
					{
						anElapsed = anElapsed - aPiece->mEffectDelay;
						aPiece->mFrame = anElapsed / mState.mConfig.mRemoveDelay;
						aPiece->mFrame = aPiece->mFrame + 1;
						if (5 < aPiece->mFrame)
						{
							aPiece->mFlags = aPiece->mFlags | PF_GONE;
							aPiece->mFlags = aPiece->mFlags & 0xfe7f;
							goto NextPiece;
						}
					}
				}
				if (aPiece->mEffectKind != KIND_NONE)
				{
					uint32_t anElapsed = aNow - aPiece->mSeed;
					uint8_t aFade = (uint8_t)((uint64_t)(anElapsed % FLASH_TIME << 8) / FLASH_TIME);
					aLevel = aFade;
					if (anElapsed < (uint32_t)anEffectFlashCount * FLASH_TIME + aPiece->mEffectDelay)
					{
						if (aFade < 0x80)
							aLevel = -1 - aFade;
					}
					else
					{
						aPiece->mEffectDelay = 0;
						aPiece->mEffectKind = KIND_NONE;
						aPiece->mSeed = aNow;
						aLevel = 0xff;
					}
				}
			}
			uint32_t aSourceX = mBlockWidth * aPiece->mFrame;
			int32_t aSourceEnd = aSourceX + mBlockWidth;
			int32_t aSourceY = aPiece->mKind * mBlockHeight;
			uint32_t aSourceBottom = aSourceY + mBlockHeight;
			int32_t anOffsetY = 0;
			if ((aPiece->mFlags & PF_ENTERING) != 0)
			{
				anEnterCount = anEnterCount + 1;
				if (aNow < aPiece->mFlyTime)
				{
					if (gModal == 0)
					{
						aPiece->mFlags = aPiece->mFlags & 0xff7f;
						anEnteredCount = anEnteredCount + 1;
					}
				}
				else
				{
					uint32_t anElapsed = aNow - aPiece->mFlyTime;
					if (anElapsed < aPiece->mFlyDelay)
					{
						anOffsetY = -0x280;
					}
					else
					{
						anElapsed = anElapsed - aPiece->mFlyDelay;
						if (anElapsed < 0x2ee)
						{
							anOffsetY = ((((uint32_t)aPiece->mY >> FIXED_SHIFT) + mY + 10 + mBlockHeight) -
								(anElapsed * anElapsed * (((uint32_t)aPiece->mY >> 8) + mY + 10 + mBlockHeight)) / 0x89544);
						}
						else if (gModal == 0)
						{
							aPiece->mFlags = aPiece->mFlags & 0xff7f;
							anEnteredCount = anEnteredCount + 1;
						}
					}
				}
			}
			if ((aPiece->mFlags & PF_LEAVING) != 0)
			{
				aLeaveCount = aLeaveCount + 1;
				if (aNow < aPiece->mFlyTime)
				{
					aPiece->mFlags = aPiece->mFlags & 0xfeff;
					isDeleteAfter = true;  // the original deletes it here and draws it from the freed memory
					aLeftCount = aLeftCount + 1;
				}
				else
				{
					uint32_t anElapsed = aNow - aPiece->mFlyTime;
					if (anElapsed < aPiece->mFlyDelay)
					{
						anOffsetY = 0;
					}
					else
					{
						anElapsed = anElapsed - aPiece->mFlyDelay;
						if (0x2ed < anElapsed)
						{
							aPiece->mFlags = aPiece->mFlags & 0xfeff;
							mField->DeletePiece(aCell);
							aLeftCount = aLeftCount + 1;
							goto NextPiece;
						}
						anOffsetY = -(int32_t)((anElapsed * anElapsed * 0x1e0) / 0x89544);
					}
				}
			}
			lowos::BlitLevel(aSourceEnd - aSourceX, aSourceBottom - aSourceY, aPiece->mX + mX + aPiece->mShakeX / FIXED_ONE,
				(((uint32_t)aPiece->mY >> 8) + mY) - anOffsetY + aPiece->mShakeY / FIXED_ONE, aSourceX, aSourceY, mBlocks,
				aLevel);
			if (isDeleteAfter)
				mField->DeletePiece(aCell);
		}
NextPiece:
		aPieceIndex = aPieceIndex + 1;
	} while (true);
}

// 0x40e742: the row below the blocks at rest (-1: the bottom row has one)
int32_t Field::GetLowRow()
{
	return mLowRow;
}

// 0x40e754: the row above the highest block at rest (-1: past the top row)
int32_t Field::GetHighRow()
{
	return mHighRow;
}

// 0x4103c7: the number of cells
int32_t Field::GetSize()
{
	return mRows * mColumns;
}

// 0x4103dd: the meter (the timed game's time bar)
float Field::GetMeter()
{
	return mState->mMeter;
}

// 0x4103f2: starts a level on the field: the end flags clear, the field is set up (Reset)
void Board::StartLevel()
{
	mGameOver = 0;
	mLevelComplete = 0;
	mLastColumn = -1;
	mField->Reset();
}

// 0x410427: the player taps: drops the first of the preview blocks into a column (theX in pixels from the field's left, or
// -1 for the column the slider is on), if that column's bottom is free and the field does not lock the player. The
// block moves up from there, and a new one comes into the queue.
void Board::Tap(uint32_t theX)
{
	if (mNextBlocks[0].mKind != KIND_NONE)
	{
		if (theX == 0xffffffff)
		{
			int32_t aColumn = (int32_t)Send(gApp.mColumnCursor, MSG_CURSOR_COLUMN, 0, 0);
			theX = aColumn * mBlockWidth;
		}
		if ((uint32_t)(mColumns * mBlockWidth) <= theX)
			theX = (mColumns - 1) * mBlockWidth;
		uint32_t aCanPlace = mField->CanPlace((int32_t)theX / mBlockWidth);
		if (aCanPlace != 0 && mField->mLockFall == 0)
		{
			mNextBlocks[0].mFlags = mNextBlocks[0].mFlags | PF_MOVING;
			mNextBlocks[0].mMoveTime = GameTime();
			int32_t aKind = mNextBlocks[0].mKind;
			BoardState* aState = mField->GetState();
			if (aKind == KIND_EXPLODER)
				mNextBlocks[0].mPower = aState->mConfig.mExploderTapRows != 0;
			else if (aKind == KIND_HAMMER)
				mNextBlocks[0].mPower = aState->mConfig.mHammerTapRows != 0;
			else if (aKind == KIND_MAGNET)
				mNextBlocks[0].mPower = aState->mConfig.mMagnetTapRows != 0;
			else if (aKind == KIND_RAINBOW)
				mNextBlocks[0].mPower = aState->mConfig.mRainbowTapRows != 0;
			else if (aKind == KIND_COLORBOMB)
				mNextBlocks[0].mPower = aState->mConfig.mColorbombTapRows != 0;
			else if (aKind == KIND_COLUMNNUKE)
				mNextBlocks[0].mPower = aState->mConfig.mColumnnukeTapRows != 0;
			else
				mNextBlocks[0].mPower = aState->mConfig.mFallerTapRows != 0;
			Motion* aMotion = MotionOf(mNextBlocks[0].mKind, 1);
			mNextBlocks[0].mSpeed = aMotion->mParam[0];
			mNextBlocks[0].mX = ((int32_t)theX / mBlockWidth) * mBlockWidth;
			mField->Place(&mNextBlocks[0]);
			mNextBlocks[0].mFlags = mNextBlocks[0].mFlags & 0xfffb;
			FillQueue();
		}
	}
}

// 0x410651: the score
uint32_t Board::GetScore()
{
	return mField->GetScore();
}

// 0x410668: the blocks left to clear
uint32_t Board::GetBlocksLeft()
{
	return mField->GetBlocksLeft();
}

// 0x41067f: the game is over: from now on the field only clears (ClearStep)
void Board::EndGame()
{
	mEnding = 1;
}

// 0x410695: whether the game is over (EndGame)
uint32_t Board::IsEnding()
{
	return mEnding;
}

// 0x4106a7: one step of the field's clearing animation; 1 when it is done
uint32_t Board::ClearStep()
{
	return mField->ClearStep();
}

// 0x4106be: restarts the field's clocks (after a pause)
void Board::ResetClocks()
{
	mField->ResetClocks();
}

// 0x4106d5: after the queue has moved up: restarts its clock, and prepares the next block to enter
void Board::ResumeQueue()
{
	mNextBlocks[0].mMoveTime = GameTime();
	mQueueShifting = 0;
	if (gBonusRound == 0)
		mField->PrepareNext();
}

// 0x41070c: writes the board's part of the savegame, then the field's (Field::Save). Returns 1 on a failed write, else 0.
uint32_t Board::Save(FILE* theFile, uint32_t theNow)
{
	auto put = [&](char theType, size_t theSize, void* theData) -> bool
	{
		return (WriteField(theType, theSize, theData, theFile, theNow) & 0xff) != 0;
	};
	for (int i = 0; i < 4; i++)
	{
		if (put('B', 0x5c, &mNextBlocks[i]))
			return 1;
	}
	Config& aConfig = mState.mConfig;
	if (put('D', 4, &mLastColumn) || put('D', 4, &mQueueShifting) || put('D', 4, &mQueueIndex) ||
		put('D', 4, &mLevelComplete) || put('D', 4, &mGameOver) || put('D', 4, &mEnding) || put('D', 4, &mState.mLevel) ||
		put('D', 4, &mState.mBlocksLeft) || put('D', 4, &mState.mMeter) || put('D', 4, &CfgStartRows(aConfig)) ||
		put('D', 4, &CfgStartRowsAdd(aConfig)) || put('D', 4, &CfgStopRow(aConfig)) || put('D', 4, &CfgStopRowAdd(aConfig)) ||
		put('D', 4, &CfgFloatsPerRemoval(aConfig)) || put('D', 4, &CfgFloatsPerTap(aConfig)) ||
		put('D', 4, &CfgGoal(aConfig)))
		return 1;
	for (int i = 0; i < 16; i++)
	{
		if (put('D', 4, &CfgRange(aConfig, i)))
			return 1;
	}
	if (put('D', 4, &CfgNumColors(aConfig)) || put('D', 4, &CfgSpawnThreshold(aConfig)) ||
		put('D', 4, &CfgMeterStart(aConfig)) || put('D', 4, &CfgMeterLoss(aConfig)) || put('D', 4, &CfgMeterDoublingTime(aConfig)))
		return 1;
	return mField->Save(theFile, theNow);
}

// 0x410d35: reads the board's part of the savegame, makes a field and reads its part (Field::Load). Returns 1 on a
// failed read, else 0.
uint32_t Board::Load(FILE* theFile, uint32_t theNow)
{
	auto get = [&](char theType, size_t theSize, void* theData) -> bool
	{
		return (ReadField(theType, theSize, theData, theFile, theNow) & 0xff) != 0;
	};
	for (int i = 0; i < 4; i++)
	{
		if (get('B', 0x5c, &mNextBlocks[i]))
			return 1;
	}
	Config& aConfig = mState.mConfig;
	if (get('D', 4, &mLastColumn) || get('D', 4, &mQueueShifting) || get('D', 4, &mQueueIndex) ||
		get('D', 4, &mLevelComplete) || get('D', 4, &mGameOver) || get('D', 4, &mEnding) || get('D', 4, &mState.mLevel) ||
		get('D', 4, &mState.mBlocksLeft) || get('D', 4, &mState.mMeter) || get('D', 4, &CfgStartRows(aConfig)) ||
		get('D', 4, &CfgStartRowsAdd(aConfig)) || get('D', 4, &CfgStopRow(aConfig)) || get('D', 4, &CfgStopRowAdd(aConfig)) ||
		get('D', 4, &CfgFloatsPerRemoval(aConfig)) || get('D', 4, &CfgFloatsPerTap(aConfig)) ||
		get('D', 4, &CfgGoal(aConfig)))
		return 1;
	for (int i = 0; i < 16; i++)
	{
		if (get('D', 4, &CfgRange(aConfig, i)))
			return 1;
	}
	if (get('D', 4, &CfgNumColors(aConfig)) || get('D', 4, &CfgSpawnThreshold(aConfig)) ||
		get('D', 4, &CfgMeterStart(aConfig)) || get('D', 4, &CfgMeterLoss(aConfig)) || get('D', 4, &CfgMeterDoublingTime(aConfig)))
		return 1;
	for (int i = 0; i < 100; i++)
		mArrowY[i] = -1;
	mField = new Field(mColumns, mRows, &mBlockWidth);
	mField->SetState(&mState);
	return mField->Load(theFile, theNow);
}

} // namespace tiptop
