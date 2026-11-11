// The saved game: savegame.dat holds a game in progress (see Board.h). It is written when the game is left with
// progress, read when the player chooses to continue, and deleted after it was read.

#include "Board.h"
#include "WinAlchApp.h"
#include "Piece.h"
#include "Fixes.h"
#include "Sidebar.h"
#include "WidgetManager.h"
#include "Buffer.h"
#include "PortCompat.h"
#include "Compat2004.h"
#include <string.h>

using namespace Sexy;

static const int SAVEGAME_VERSION = 1;

template <typename T> static void WriteRaw(Buffer* theBuffer, const T& theValue)
{
	theBuffer->WriteBytes((const uchar*) &theValue, (int) sizeof(T));
}

// As the original's fread: a field the file does not hold (a damaged file) keeps its value
template <typename T> static void ReadRaw(Buffer* theBuffer, T* theValue)
{
	if (theBuffer->mReadBitPos / 8 + (int) sizeof(T) > theBuffer->GetDataLen())
		return;
	theBuffer->ReadBytes((uchar*) theValue, (int) sizeof(T));
}

static void WriteBool(Buffer* theBuffer, bool theValue)
{
	uchar aByte = theValue ? 1 : 0;
	theBuffer->WriteBytes(&aByte, 1);
}

static void ReadBool(Buffer* theBuffer, bool* theValue)
{
	if (theBuffer->mReadBitPos / 8 + 1 > theBuffer->GetDataLen())
		return;
	uchar aByte = 0;
	theBuffer->ReadBytes(&aByte, 1);
	*theValue = (aByte != 0);
}

static void WriteText(Buffer* theBuffer, const std::string& theText)
{
	int aLength = (int) theText.length();
	WriteRaw(theBuffer, aLength);
	theBuffer->WriteBytes((const uchar*) theText.c_str(), aLength);
}

static std::string ReadText(Buffer* theBuffer)
{
	int aLength = 0;
	ReadRaw(theBuffer, &aLength);
	if (aLength < 0 || theBuffer->mReadBitPos / 8 + aLength > theBuffer->GetDataLen())
		return "";
	std::string aText(aLength, ' ');
	if (aLength > 0)
		theBuffer->ReadBytes((uchar*) &aText[0], aLength);
	return aText;
}

std::string Board::GetSaveGameFileName()
{
	return "userdata\\savegame.dat";
}

// 0x402a44
bool Board::SaveGame()
{
	Pause(false);
	Unpause(false);

	std::string aData = SerializeGame();
	return mApp->WriteBytesToFile(GetSaveGameFileName(), aData.data(), (unsigned long) aData.size());
}

// The bytes SaveGame writes (also the web build's backup, WinAlchApp::GetShutdownSaves: it does not touch the clock)
std::string Board::SerializeGame()
{
	Buffer aBuffer;

	WriteRaw(&aBuffer, SAVEGAME_VERSION);
	WriteRaw(&aBuffer, mApp->mSkillLevel);
	WriteBool(&aBuffer, mApp->mTimeTrial);
	WriteRaw(&aBuffer, mState);
	WriteText(&aBuffer, mRand.Serialize());
	WriteText(&aBuffer, mEncodedGame.ToWebString());
	WriteRaw(&aBuffer, mSavedZero);

	WriteBool(&aBuffer, mCurPiece != NULL);
	if (mCurPiece != NULL)
	{
		WriteRaw(&aBuffer, mCurPiece->mColor);
		WriteRaw(&aBuffer, mCurPiece->mShape);
		WriteBool(&aBuffer, mCurPiece->mBomb);
		WriteBool(&aBuffer, mCurPiece->mSkull);
	}
	WriteBool(&aBuffer, mAnimateLights);
	WriteBool(&aBuffer, mTimeTrial);
	WriteBool(&aBuffer, mPristine);
	WriteRaw(&aBuffer, mNextBoardFade);

	for (int aRow = 0; aRow < ROWS; aRow++)
		for (int aCol = 0; aCol < COLS; aCol++)
		{
			Piece* aPiece = mGrid[aRow][aCol];
			WriteBool(&aBuffer, aPiece != NULL);
			if (aPiece != NULL)
			{
				WriteRaw(&aBuffer, aPiece->mCol);
				WriteRaw(&aBuffer, aPiece->mRow);
				WriteRaw(&aBuffer, aPiece->mColor);
				WriteRaw(&aBuffer, aPiece->mShape);
				WriteBool(&aBuffer, aPiece->mSkull);
			}
			WriteBool(&aBuffer, mLit[aRow][aCol]);
			WriteBool(&aBuffer, mFlash[aRow][aCol]);
			WriteBool(&aBuffer, mValid[aRow][aCol]);
		}

	// the seconds so far, with the part of the game that is running now
	int anElapsed = mElapsed;
	if (mPauseCount == 0)
		anElapsed += (int) ((GameTickCount() - mStartTick) / 1000);

	WriteRaw(&aBuffer, mNumValid);
	WriteRaw(&aBuffer, mProgress);
	WriteRaw(&aBuffer, mScore);
	WriteRaw(&aBuffer, mProgRate);
	WriteRaw(&aBuffer, mProgDecay);
	WriteRaw(&aBuffer, mAnimDir);
	WriteRaw(&aBuffer, mAnimPct);
	WriteRaw(&aBuffer, mClearRow);
	WriteRaw(&aBuffer, mClearCol);
	WriteBool(&aBuffer, mLightOn);
	WriteBool(&aBuffer, mLightFading);
	WriteRaw(&aBuffer, mLightFade);
	WriteRaw(&aBuffer, mLightVariant);
	WriteRaw(&aBuffer, mLightCol);
	WriteRaw(&aBuffer, mLightRow);
	WriteRaw(&aBuffer, mLightPos);
	WriteRaw(&aBuffer, mLightMix);
	WriteRaw(&aBuffer, mLightMixStep);
	WriteRaw(&aBuffer, mSkillBase);
	WriteRaw(&aBuffer, mNumShapes);
	WriteRaw(&aBuffer, mNumColors);
	WriteRaw(&aBuffer, mBombChance);
	WriteRaw(&aBuffer, mSkullChance);
	WriteRaw(&aBuffer, anElapsed);
	WriteRaw(&aBuffer, mRun);
	WriteRaw(&aBuffer, mLongestRun);
	WriteRaw(&aBuffer, mBoardsCleared);
	WriteRaw(&aBuffer, mRate);
	WriteRaw(&aBuffer, mLastShape);
	WriteRaw(&aBuffer, mLastColor);
	WriteRaw(&aBuffer, mApp->mSidebar->mScoreAnim);

	return std::string((const char*) aBuffer.GetDataPtr(), (size_t) aBuffer.GetDataLen());
}

// 0x40589a: the saved game goes on where it was left (the app started the game before, so the piece to place exists)
bool Board::LoadGame()
{
	Buffer aBuffer;
	if (!mApp->ReadBufferFromFile(GetSaveGameFileName(), &aBuffer))
		return false;
	aBuffer.SeekFront();

	int aVersion = 0;
	ReadRaw(&aBuffer, &aVersion);
	if (aVersion != SAVEGAME_VERSION)   // 0x4058dd: the game is applied only when the first int is 1
		return false;
	ReadRaw(&aBuffer, &mApp->mSkillLevel);
	ReadBool(&aBuffer, &mApp->mTimeTrial);
	ReadRaw(&aBuffer, &mState);
	mRand.SRand(ReadText(&aBuffer));
	mEncodedGame.FromWebString(ReadText(&aBuffer));
	ReadRaw(&aBuffer, &mSavedZero);

	bool hasPiece = false;
	ReadBool(&aBuffer, &hasPiece);
	if (hasPiece)
	{
		Piece aSaved(mApp);
		ReadRaw(&aBuffer, &aSaved.mColor);
		ReadRaw(&aBuffer, &aSaved.mShape);
		ReadBool(&aBuffer, &aSaved.mBomb);
		ReadBool(&aBuffer, &aSaved.mSkull);
		if (mCurPiece != NULL)
		{
			mCurPiece->mColor = aSaved.mColor;
			mCurPiece->mShape = aSaved.mShape;
			mCurPiece->mBomb = aSaved.mBomb;
			mCurPiece->mSkull = aSaved.mSkull;
		}
	}
	ReadBool(&aBuffer, &mAnimateLights);
	ReadBool(&aBuffer, &mTimeTrial);
	ReadBool(&aBuffer, &mPristine);
	ReadRaw(&aBuffer, &mNextBoardFade);

	for (int aRow = 0; aRow < ROWS; aRow++)
		for (int aCol = 0; aCol < COLS; aCol++)
		{
			bool hasGridPiece = false;
			ReadBool(&aBuffer, &hasGridPiece);
			delete mGrid[aRow][aCol];
			mGrid[aRow][aCol] = NULL;
			if (hasGridPiece)
			{
				Piece* aPiece = new Piece(mApp);
				ReadRaw(&aBuffer, &aPiece->mCol);
				ReadRaw(&aBuffer, &aPiece->mRow);
				ReadRaw(&aBuffer, &aPiece->mColor);
				ReadRaw(&aBuffer, &aPiece->mShape);
				ReadBool(&aBuffer, &aPiece->mSkull);
				aPiece->mShrink = 0;
				aPiece->mShrinkDir = 0;
				// 0x405b39: the original has no board offset here (a piece's position is the board's, not the screen's, until
				// the next Resize); the end-of-game shrink of a continued game draws from it, as the original's does:
				// 150 pixels left of the cells, out of the board's picture (fix "continuedEnding" gives the offset)
				int aOffsetX = FixEnabled("continuedEnding") ? mX : 0;
				int aOffsetY = FixEnabled("continuedEnding") ? mY : 0;
				aPiece->Resize(GetColX(aPiece->mCol) + aOffsetX, GetRowY(aPiece->mRow) + aOffsetY, 0x4b, 0x4b);
				mGrid[aRow][aCol] = aPiece;
			}
			ReadBool(&aBuffer, &mLit[aRow][aCol]);
			ReadBool(&aBuffer, &mFlash[aRow][aCol]);
			ReadBool(&aBuffer, &mValid[aRow][aCol]);
		}

	ReadRaw(&aBuffer, &mNumValid);
	ReadRaw(&aBuffer, &mProgress);
	ReadRaw(&aBuffer, &mScore);
	ReadRaw(&aBuffer, &mProgRate);
	ReadRaw(&aBuffer, &mProgDecay);
	ReadRaw(&aBuffer, &mAnimDir);
	ReadRaw(&aBuffer, &mAnimPct);
	ReadRaw(&aBuffer, &mClearRow);
	ReadRaw(&aBuffer, &mClearCol);
	ReadBool(&aBuffer, &mLightOn);
	ReadBool(&aBuffer, &mLightFading);
	ReadRaw(&aBuffer, &mLightFade);
	ReadRaw(&aBuffer, &mLightVariant);
	ReadRaw(&aBuffer, &mLightCol);
	ReadRaw(&aBuffer, &mLightRow);
	ReadRaw(&aBuffer, &mLightPos);
	ReadRaw(&aBuffer, &mLightMix);
	ReadRaw(&aBuffer, &mLightMixStep);
	ReadRaw(&aBuffer, &mSkillBase);
	ReadRaw(&aBuffer, &mNumShapes);
	ReadRaw(&aBuffer, &mNumColors);
	ReadRaw(&aBuffer, &mBombChance);
	ReadRaw(&aBuffer, &mSkullChance);
	ReadRaw(&aBuffer, &mElapsed);
	ReadRaw(&aBuffer, &mRun);
	ReadRaw(&aBuffer, &mLongestRun);
	ReadRaw(&aBuffer, &mBoardsCleared);
	ReadRaw(&aBuffer, &mRate);
	ReadRaw(&aBuffer, &mLastShape);
	ReadRaw(&aBuffer, &mLastColor);
	ReadRaw(&aBuffer, &mApp->mSidebar->mScoreAnim);

	// a light that was dissolving in when the game was saved has no pictures: it is over
	mLightOn = false;
	mPristine = false;
	mStartTick = GameTickCount();
	DetermineValidMoves();
	RedrawBackground();

	if (mState == STATE_BOARD_CLEARED)
	{
		GetGameStats();
		if (mCurPiece != NULL)
		{
			mCurPiece->SetVisible(false);
			mCurPiece->mFollowMouse = false;
		}
		ShowBoardComplete();
	}
	return true;
}
