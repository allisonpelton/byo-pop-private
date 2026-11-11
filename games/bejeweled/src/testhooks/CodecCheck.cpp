// port(variant): test hook for -codeccheck=<file>; see CodecCheck.h. Not part of the original game.

#include "CodecCheck.h"
#include "WinDMApp.h"
#include "Board.h"
#include "StateCodec.h"
#include "Replay.h"
#include "WatchController.h"
#include "Buffer.h"
#include "MTRand.h"

#include <SDL.h>
#include <cstring>

using namespace Sexy;

CodecCheck::CodecCheck(WinDMApp* theApp, const std::string& theFileName)
{
	mApp = theApp;
	mFile = fopen(theFileName.c_str(), "wb");
	if (mFile == NULL)
		SDL_Log("codeccheck: cannot write %s", theFileName.c_str());
	mScratch = NULL;
	mSynthetic = NULL;
	mLastBoard = NULL;
	mLastState = -1;
	mLastUndos = 0;
	mChecked = 0;
	mSettles = 0;
	mFailures = 0;
}

CodecCheck::~CodecCheck()
{
	if (mFile != NULL)
	{
		fprintf(mFile, "codeccheck: %d updates checked, %d settles, %d failures\n", mChecked, mSettles, mFailures);
		fclose(mFile);
	}
	// mScratch is kept to the end on purpose (CodecCheck.h)
}

void CodecCheck::AfterUpdate()
{
	Board* aBoard = mApp->mBoard;
	if ((mFile == NULL) || (aBoard == NULL))
		return;
	if (mScratch == NULL)
		mScratch = new Board(mApp, Board::SCRATCH_BOARD);

	// The settles -trace prints (TraceBoard, BoardTrace.h): a watched replay's last board has none once the replay has ended
	bool isEnded = (mApp->mWatch != NULL) && mApp->mWatch->IsEndedBoard(aBoard);
	bool isSettle = !isEnded && (aBoard->mState == STATE_PLAYING) &&
		((aBoard != mLastBoard) || (mLastState != STATE_PLAYING) || (aBoard->mUndosUsed != mLastUndos));
	mLastBoard = aBoard;
	mLastState = aBoard->mState;
	mLastUndos = aBoard->mUndosUsed;
	mChecked++;
	std::string aDiff = StateCodec::RoundTrip(aBoard, mScratch);
	// In play, the undo snapshot a move would take now restores into the copy and reads back the same
	if (aDiff.empty() && aBoard->mVariant.mUndo && (aBoard->mState == STATE_PLAYING))
		aDiff = StateCodec::UndoRoundTrip(aBoard, mScratch);
	// The save the board would make now passes the check a replay's LDSV gets (Replay::CheckSave), unless the
	// game is over (no save then); a variant board's v2 save with its rules' RulesId, and its
	// header, field sequence and chunks read back as written (Replay::ReadVariantSave)
	if (aDiff.empty() && (aBoard->mState != STATE_GAME_OVER) && (aBoard->mState != STATE_GAME_OVER_DONE))
	{
		Buffer aSave;
		aBoard->SaveGameSnapshot(&aSave);
		std::string aBytes((const char*) aSave.GetDataPtr(), (size_t) aSave.GetDataLen());
		std::string aWhy;
		uint8_t anId[Replay::RULES_ID_SIZE] = {};
		if (aBoard->mVariant.mAny)
			aBoard->mRules.RulesId(anId);
		std::vector<BoardLayout> aShapes;
		for (int i = 0; i < aBoard->mRules.ShapeCount(); i++)
			aShapes.push_back(aBoard->mRules.LayoutForLevel(i));
		if (!Replay::CheckSave(aBytes, aBoard->mCols, aBoard->mRows, &aWhy, aBoard->mVariant.mAny ? anId : NULL, aBoard->mVariant.mAny ? &aShapes : NULL,
			GameConfig::GemColors(aBoard->mRules)))
			aDiff = "the save: " + aWhy;
		else if (aBoard->mVariant.mAny)
		{
			Replay::VariantSave aRead;
			Replay::VariantSave aWritten;
			aBoard->GetVariantSave(&aWritten);
			if (!Replay::ReadVariantSave(aBytes, &aRead, &aWhy))
				aDiff = "the v2 save: " + aWhy;
			else if ((aRead.mMoves != aWritten.mMoves) || (aRead.mInvalidMoves != aWritten.mInvalidMoves) ||
				(aRead.mUndosUsed != aWritten.mUndosUsed) || (aRead.mRng.mSeed != aWritten.mRng.mSeed) ||
				(memcmp(aRead.mRng.mMt, aWritten.mRng.mMt, sizeof(aRead.mRng.mMt)) != 0) || (aRead.mRng.mMti != aWritten.mRng.mMti))
				aDiff = "the v2 save's chunks are not the board's";
		}
	}
	if (!aDiff.empty())
	{
		mFailures++;
		fprintf(mFile, "FAIL update %d tick %d state %d: %s\n", mApp->mUpdateCount, aBoard->mUpdateCnt, aBoard->mState, aDiff.c_str());
	}
	else if (isSettle)
		fprintf(mFile, "settle %d tick %d state %d: ok, %d bytes\n", mApp->mUpdateCount, aBoard->mUpdateCnt, aBoard->mState,
			(int) StateCodec::Encode(aBoard, StateCodec::PROFILE_SNAPSHOT).size());
	if (isSettle)
		mSettles++;
	if (isSettle && (mSynthetic == NULL))
		CheckSynthetic(aBoard);
}

// Once, at the first settle: a copy of the board (a second scratch board) given what play reaches only
// rarely, round-tripped into the first scratch board: a piece outside the grid that is both the selection
// and the first swap piece (it must stay one piece), score popups, an own RNG that has drawn, a pending
// seed, an empty cell
void CodecCheck::CheckSynthetic(Board* theBoard)
{
	mSynthetic = new Board(mApp, Board::SCRATCH_BOARD);
	Board* aBoard = mSynthetic;
	std::string aDiff = StateCodec::RoundTrip(theBoard, aBoard);
	if (aDiff.empty())
	{
		Piece* anOutside = new Piece();
		anOutside->mColor = 3;
		anOutside->mCol = 2;
		anOutside->mRow = 5;
		anOutside->mX = 305.5;
		aBoard->mOrphanedPieces.push_back(anOutside);
		aBoard->mSelectedPiece = anOutside;
		aBoard->mSwapPiece1 = anOutside;
		aBoard->mSwapPiece2 = aBoard->mBoard[0][0];
		delete aBoard->mBoard[aBoard->mRows - 1][aBoard->mCols - 1];
		aBoard->mBoard[aBoard->mRows - 1][aBoard->mCols - 1] = NULL;
		ScorePopup aPopup = {100, 200, 50, -1, 30};
		aBoard->mScorePopups.push_back(aPopup);
		aPopup.mPoints = 45;
		aBoard->mScorePopups.push_back(aPopup);
		delete aBoard->mOwnRng;
		aBoard->mOwnRng = new MTRand(77);
		for (int i = 0; i < 700; i++)
			aBoard->mOwnRng->Next();
		aBoard->mGameSeed = 77;
		aBoard->mUserSeeded = true;
		aBoard->mTakesPendingSeed = true;
		aBoard->mBoardDraws = 700;
		// A variant board's move counts (a board of the original rules has no rows for them)
		aBoard->mMoveCount = 17;
		aBoard->mInvalidMoves = 5;
		aBoard->mUndosUsed = 2;
		aDiff = StateCodec::RoundTrip(aBoard, mScratch);
	}
	if (!aDiff.empty())
	{
		mFailures++;
		fprintf(mFile, "FAIL synthetic: %s\n", aDiff.c_str());
	}
	else
		fprintf(mFile, "synthetic: ok, %d bytes\n", (int) StateCodec::Encode(aBoard, StateCodec::PROFILE_SNAPSHOT).size());
}
