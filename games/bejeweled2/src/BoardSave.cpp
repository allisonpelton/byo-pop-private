// BoardSave.cpp: the undo states, the saved game and the lightning list of Sexy::Board.
//   SaveState 0x594e35, LoadState 0x59b957, PopUndo 0x59bad4, UndoAll 0x59bb11, ClearUndoBuffers 0x59b531,
//   ClearLightning 0x59b66d, GetSavedGameFileName 0x59c4f0, SaveGame 0x59c5ab, DeleteSavedGame 0x59caff,
//   HasSavedGame 0x5a50b3, LoadGame 0x5ae34e.
//
// Not ported (dead code, no caller anywhere in the exe): 0x59bb4f (steps back to an undo index), 0x59bbc1 (switches the
// undo preview on and off) and 0x59c390 (writes a puzzle file).

#include "Board.h"
#include "WinDMApp.h"
#include "Profile.h"
#include "Buffer.h"
#include "Common.h"

using namespace Sexy;

// the saved game writes each field raw, with the size of the original's field
#define PUT(theBuffer, theField) (theBuffer).WriteBytes((const uchar*)&(theField), sizeof(theField))
#define GET(theBuffer, theField) (theBuffer).ReadBytes((uchar*)&(theField), sizeof(theField))

// 0x594e35: the board for the undo list (the score, the solver's state and the gems)
void Board::SaveState(Buffer& theBuffer)
{
	theBuffer.Clear();
	theBuffer.WriteLong(mScore);
	theBuffer.WriteBoolean(mSolutionValid);
	theBuffer.WriteShort((short)mSolutionMoves);
	theBuffer.WriteShort((short)mSolutionNode);
	theBuffer.WriteByte((uchar)mSolutionHintIndex);
	for (int aRow = 0; aRow < 8; aRow++)
	{
		for (int aCol = 0; aCol < 8; aCol++)
		{
			Piece* aPiece = mBoard[aRow][aCol];
			if (aPiece == NULL)
			{
				theBuffer.WriteByte(0xff);
			}
			else
			{
				theBuffer.WriteByte((uchar)aPiece->mType);
				theBuffer.WriteByte(aPiece->m70 != 0);
				if (aPiece->mType == 8)
					theBuffer.WriteByte((uchar)aPiece->m58);
			}
		}
	}
}

// 0x59b957
void Board::LoadState(const Buffer& theBuffer)
{
	m15eac = -1;
	m15ea9 = false;
	theBuffer.SeekFront();
	ClearBoard();
	mScore = theBuffer.ReadLong();
	if (mIsPuzzle && mIsSecret && m1918 > 0)
		mScore = m1774;
	m15e40 = mScore;
	mSolutionValid = theBuffer.ReadBoolean();
	mSolutionMoves = theBuffer.ReadShort();
	mSolutionNode = theBuffer.ReadShort();
	mSolutionHintIndex = theBuffer.ReadByte();
	for (int aRow = 0; aRow < 8; aRow++)
	{
		for (int aCol = 0; aCol < 8; aCol++)
		{
			int aType = theBuffer.ReadByte();
			if (aType != 0xff)
			{
				uchar aFlags = theBuffer.ReadByte();
				Piece* aPiece = new Piece();
				aPiece->mType = aType;
				aPiece->mCol = aCol;
				aPiece->mRow = aRow;
				aPiece->mX = (float)GetColX(aCol);
				aPiece->mY = (float)GetRowY(aRow);
				if (aType == 8)
					aPiece->m58 = theBuffer.ReadByte();
				if ((aFlags & 1) != 0)
					aPiece->m70 = true;
				mBoard[aRow][aCol] = aPiece;
			}
		}
	}
	ClearLightning();
	mState = STATE_IDLE;
	MarkDirty();
}

// 0x59bad4: forget the newest undo state
void Board::PopUndo()
{
	std::list<Buffer>& aList = GetUndoList();
	if (!aList.empty())
	{
		int& aIndex = GetUndoIndex();
		if (aIndex > -1)
			aIndex--;
		aList.pop_front();
	}
}

// 0x59bb11: back to the start of the undo list
void Board::UndoAll()
{
	std::list<Buffer>& aList = GetUndoList();
	if (!aList.empty())
	{
		LoadState(aList.back());
		ClearUndoBuffers();
	}
}

// 0x59b531
void Board::ClearUndoBuffers()
{
	mUndoList[0].clear();
	mUndoList[1].clear();
	mUndoIndex[0] = -1;
	mUndoIndex[1] = -1;
}

// 0x59b66d
void Board::ClearLightning()
{
	for (size_t i = 0; i < mLightning.size(); i++)
		delete mLightning[i];
	mLightning.clear();
}

// 0x59c4f0: the file of the saved game of this mode in the profile's folder (static: nothing here uses the board; the
// main menu asks for it before there is one)
std::string Board::GetSavedGameFileName()
{
	static const char* const aNames[9] = {"normal.sav", "timed.sav", "puzzle.sav", "endless.sav", "s_normal.sav",
		"s_timed.sav", "s_puzzle.sav", "s_endless.sav", "twilight.sav"};
	return gApp->mProfile->GetFolder() + "/" + aNames[gApp->mGameMode];
}

// 0x59caff
bool Board::DeleteSavedGame()
{
	return gApp->EraseFile(GetSavedGameFileName());
}

// 0x5a50b3
bool Board::HasSavedGame()
{
	if (gApp->mGameMode == MODE_PUZZLE)
		return false;
	return gApp->FileExists(GetSavedGameFileName());
}

// 0x59c5ab: false for a cheated game and for a puzzle that is not the secret galaxy's
bool Board::SaveGame()
{
	if (mCheated || (mIsPuzzle && !mIsSecret))
		return false;

	std::string aFileName = GetSavedGameFileName();
	Pause(false);
	Unpause(false);

	Buffer aBuffer;
	int aVersion = 4;
	aBuffer.WriteBytes((const uchar*)&aVersion, 4);
	aBuffer.WriteString(mRand.Serialize());
	PUT(aBuffer, mBackdropNum);
	PUT(aBuffer, mLevel);
	PUT(aBuffer, mNextLevel);
	for (int aRow = 0; aRow < 8; aRow++)
	{
		for (int aCol = 0; aCol < 8; aCol++)
		{
			Piece* aPiece = mBoard[aRow][aCol];
			bool aHasPiece = aPiece != NULL;
			aBuffer.WriteBytes((const uchar*)&aHasPiece, 1);
			if (aHasPiece)
				aBuffer.WriteBytes((const uchar*)aPiece, 0xe0);
		}
	}
	StabilizeState();
	PUT(aBuffer, mState);
	PUT(aBuffer, mIsAction);
	PUT(aBuffer, mIsPuzzle);
	PUT(aBuffer, mInUndoPreview);
	PUT(aBuffer, m1723);
	PUT(aBuffer, mNoMoveYet);
	PUT(aBuffer, m1994);
	PUT(aBuffer, mPlayedSeconds);
	PUT(aBuffer, mHintCountdown);
	PUT(aBuffer, mScore);
	PUT(aBuffer, mLevelValue);
	PUT(aBuffer, m15e40);
	PUT(aBuffer, m15e44);
	PUT(aBuffer, m15e4c);
	PUT(aBuffer, mCascadeCount);
	PUT(aBuffer, m15e6c);
	PUT(aBuffer, m15e70);
	PUT(aBuffer, m15ee0);
	PUT(aBuffer, m15ee4);
	PUT(aBuffer, m15ee8);
	PUT(aBuffer, m15eec);
	PUT(aBuffer, m15ef0);
	PUT(aBuffer, m15ef8);
	PUT(aBuffer, m15f00);
	PUT(aBuffer, m15f08);
	PUT(aBuffer, m15f10);
	PUT(aBuffer, m15f1c);
	PUT(aBuffer, m15f7c);
	PUT(aBuffer, m15f84);
	PUT(aBuffer, m15f90);
	PUT(aBuffer, m15fa8);
	PUT(aBuffer, m194d);
	PUT(aBuffer, m194e);
	PUT(aBuffer, m15e30);
	PUT(aBuffer, m15e38);
	PUT(aBuffer, mSweep1);
	PUT(aBuffer, mSweep2);
	PUT(aBuffer, mSweep0);
	PUT(aBuffer, m15f68);
	PUT(aBuffer, mIsFinity);
	PUT(aBuffer, mIsSecret);
	PUT(aBuffer, mIsTwilight);
	PUT(aBuffer, mIsHyper);
	PUT(aBuffer, mFlipped);
	PUT(aBuffer, mSecretModeUnlocked);
	if (mIsPuzzle && mIsSecret)
	{
		PUT(aBuffer, m1774);
		PUT(aBuffer, mSecretPuzzleCount);
		PUT(aBuffer, mSecretSolved);
		PUT(aBuffer, mSecretRun);
		aBuffer.WriteLong((long)mSecretPuzzles.size());
		for (size_t i = 0; i < mSecretPuzzles.size(); i++)
			aBuffer.WriteString(mSecretPuzzles[i]);
		aBuffer.WriteString(mPuzzleFile);
	}
	PUT(aBuffer, m1918);
	PUT(aBuffer, m15e7c);
	PUT(aBuffer, m15e84);
	return mApp->WriteBufferToFile(aFileName, &aBuffer);
}

// 0x5ae34e: false when there is no saved game, or it is of another version, or this is an ordinary puzzle
bool Board::LoadGame()
{
	if (mIsPuzzle && !mIsSecret)
		return false;

	std::string aFileName = GetSavedGameFileName();
	Buffer aBuffer;
	if (!mApp->ReadBufferFromFile(aFileName, &aBuffer, false))
		return false;

	int aVersion;
	aBuffer.ReadBytes((uchar*)&aVersion, 4);
	if (aVersion != 4)
		return false;

	mRand.SRand(aBuffer.ReadString());
	GET(aBuffer, mBackdropNum);
	int aBackdropCount = (int)mApp->mBackdrops.size();
	mBackdropNum = (unsigned int)(aBackdropCount - 1 + mBackdropNum) % (unsigned int)mApp->mBackdrops.size();
	GET(aBuffer, mLevel);
	GET(aBuffer, mNextLevel);
	CreateFontGlowImages();
	for (int aRow = 0; aRow < 8; aRow++)
	{
		for (int aCol = 0; aCol < 8; aCol++)
		{
			delete mBoard[aRow][aCol];
			mBoard[aRow][aCol] = NULL;
			bool aHasPiece;
			aBuffer.ReadBytes((uchar*)&aHasPiece, 1);
			if (aHasPiece)
			{
				Piece* aPiece = new Piece();
				aBuffer.ReadBytes((uchar*)aPiece, 0xe0);
				aPiece->m5C = 0;
				aPiece->m60 = 0;
				mBoard[aRow][aCol] = aPiece;
			}
		}
	}
	GET(aBuffer, mState);
	if (mState == STATE_SWAPPING || mState == STATE_COMMIT_SWAP || mState == STATE_SWAP_BACK)
		mState = STATE_IDLE;
	GET(aBuffer, mIsAction);
	GET(aBuffer, mIsPuzzle);
	GET(aBuffer, mInUndoPreview);
	GET(aBuffer, m1723);
	GET(aBuffer, mNoMoveYet);
	GET(aBuffer, m1994);
	GET(aBuffer, mPlayedSeconds);
	m15e18 = mPlayedSeconds;
	GET(aBuffer, mHintCountdown);
	GET(aBuffer, mScore);
	GET(aBuffer, mLevelValue);
	GET(aBuffer, m15e40);
	GET(aBuffer, m15e44);
	m15e48 = m15e44;
	GET(aBuffer, m15e4c);
	GET(aBuffer, mCascadeCount);
	GET(aBuffer, m15e6c);
	GET(aBuffer, m15e70);
	GET(aBuffer, m15ee0);
	GET(aBuffer, m15ee4);
	GET(aBuffer, m15ee8);
	GET(aBuffer, m15eec);
	GET(aBuffer, m15ef0);
	GET(aBuffer, m15ef8);
	GET(aBuffer, m15f00);
	GET(aBuffer, m15f08);
	GET(aBuffer, m15f10);
	GET(aBuffer, m15f1c);
	GET(aBuffer, m15f7c);
	GET(aBuffer, m15f84);
	GET(aBuffer, m15f90);
	GET(aBuffer, m15fa8);
	GET(aBuffer, m194d);
	GET(aBuffer, m194e);
	GET(aBuffer, m15e30);
	GET(aBuffer, m15e38);
	GET(aBuffer, mSweep1);
	GET(aBuffer, mSweep2);
	GET(aBuffer, mSweep0);
	GET(aBuffer, m15f68);
	GET(aBuffer, mIsFinity);
	GET(aBuffer, mIsSecret);
	GET(aBuffer, mIsTwilight);
	GET(aBuffer, mIsHyper);
	GET(aBuffer, mFlipped);
	GET(aBuffer, mSecretModeUnlocked);
	if (mIsPuzzle && mIsSecret)
	{
		GET(aBuffer, m1774);
		GET(aBuffer, mSecretPuzzleCount);
		GET(aBuffer, mSecretSolved);
		GET(aBuffer, mSecretRun);
		mScore = m1774;
		m15e40 = m1774;
		long aCount = aBuffer.ReadLong();
		mSecretPuzzles.clear();
		for (long i = 0; i < aCount; i++)
			mSecretPuzzles.push_back(aBuffer.ReadString());
		mPuzzleFile = aBuffer.ReadString();
		LoadPuzzle(mPuzzleFile);
		MakePuzzleCrazy();
	}
	GET(aBuffer, m1918);
	GET(aBuffer, m15e7c);
	m15e80 = m15e7c;
	GET(aBuffer, m15e84);
	m15e88 = m15e84;
	// (the original also clears a byte of the app that nothing reads)
	for (int aRow = 0; aRow < 8; aRow++)
	{
		for (int aCol = 0; aCol < 8; aCol++)
		{
			Piece* aPiece = mBoard[aRow][aCol];
			if (aPiece != NULL)
			{
				aPiece->mX = (float)GetColX(aPiece->mCol);
				aPiece->mY = (float)GetRowY(aPiece->mRow);
			}
		}
	}
	if (mState == STATE_WHIRLPOOL)
	{
		EndWhirlpool();
		mState = STATE_NEW;
		mDropDelay = 200;
	}
	else
	{
		mDropDelay = 0;
	}
	mHintCountdown = 0;
	return true;
}
