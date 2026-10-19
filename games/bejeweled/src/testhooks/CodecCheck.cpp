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
	bool isSettle = !isEnded && (aBoard->mState == STATE_PLAYING) && ((aBoard != mLastBoard) || (mLastState != STATE_PLAYING));
	mLastBoard = aBoard;
	mLastState = aBoard->mState;
	mChecked++;
	std::string aDiff = StateCodec::RoundTrip(aBoard, mScratch);
	// The save the board would make now passes the check a replay's LDSV gets (Replay::CheckSave), unless the
	// game is over (no save then)
	if (aDiff.empty() && (aBoard->mState != STATE_GAME_OVER) && (aBoard->mState != STATE_GAME_OVER_DONE))
	{
		Buffer aSave;
		aBoard->SaveGameSnapshot(&aSave);
		std::string aWhy;
		if (!Replay::CheckSave(std::string((const char*) aSave.GetDataPtr(), (size_t) aSave.GetDataLen()), aBoard->mCols, aBoard->mRows, &aWhy))
			aDiff = "the save: " + aWhy;
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
	Board* b = mSynthetic;
	std::string aDiff = StateCodec::RoundTrip(theBoard, b);
	if (aDiff.empty())
	{
		Piece* anOutside = new Piece();
		anOutside->mColor = 3;
		anOutside->mCol = 2;
		anOutside->mRow = 5;
		anOutside->mX = 305.5;
		b->mOrphanedPieces.push_back(anOutside);
		b->mSelectedPiece = anOutside;
		b->mSwapPiece1 = anOutside;
		b->mSwapPiece2 = b->mBoard[0][0];
		delete b->mBoard[b->mRows - 1][b->mCols - 1];
		b->mBoard[b->mRows - 1][b->mCols - 1] = NULL;
		ScorePopup aPopup = {100, 200, 50, -1, 30};
		b->mScorePopups.push_back(aPopup);
		aPopup.mPoints = 45;
		b->mScorePopups.push_back(aPopup);
		delete b->mOwnRng;
		b->mOwnRng = new MTRand(77);
		for (int i = 0; i < 700; i++)
			b->mOwnRng->Next();
		b->mGameSeed = 77;
		b->mUserSeeded = true;
		b->mTakesPendingSeed = true;
		b->mBoardDraws = 700;
		aDiff = StateCodec::RoundTrip(b, mScratch);
	}
	if (!aDiff.empty())
	{
		mFailures++;
		fprintf(mFile, "FAIL synthetic: %s\n", aDiff.c_str());
	}
	else
		fprintf(mFile, "synthetic: ok, %d bytes\n", (int) StateCodec::Encode(b, StateCodec::PROFILE_SNAPSHOT).size());
}
