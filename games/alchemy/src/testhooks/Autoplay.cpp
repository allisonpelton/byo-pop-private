// Test hooks: the autoplay bot (see Autoplay.h).

#include "Autoplay.h"
#include "AlchemyApp.h"
#include "Board.h"
#include "Piece.h"
#include "HighScores.h"
#include "Dialogs.h"
#include "TitleScreen.h"
#include "Selector.h"
#include "WidgetManager.h"
#include "ButtonWidget.h"
#include "Compat2004.h"

#include <cstdarg>
#include <cstdlib>
#include <cstring>
#include <vector>

using namespace Sexy;

AutoplayBot::AutoplayBot(AlchemyApp* theApp, unsigned int theSeed)
{
	mApp = theApp;
	mSeed = theSeed;
	mTick = 0;
	mGames = 0;
	mWait = 0;
	mMoves = 0;
	mForges = 0;
	mShots = 0;
	mLastState = -1;
	mLastBoards = 0;
	mClickX = -1;
	mClickY = -1;
	mOverLogged = false;
	mFinished = false;
	mLog = NULL;
	if (!theApp->mAutoplayLogFileName.empty())
		mLog = fopen(theApp->mAutoplayLogFileName.c_str(), "w");
}

AutoplayBot::~AutoplayBot()
{
	if (mLog != NULL)
		fclose(mLog);
}

int AutoplayBot::Rand(int theRange)
{
	mSeed = mSeed * 1103515245u + 12345u;
	return (int) ((mSeed >> 16) % (unsigned int) theRange);
}

void AutoplayBot::Log(const char* theFormat, ...)
{
	if (mLog == NULL)
		return;
	va_list args;
	va_start(args, theFormat);
	vfprintf(mLog, theFormat, args);
	va_end(args);
	fputc('\n', mLog);
	fflush(mLog);
}

void AutoplayBot::Shot(const char* theName, int theNumber)
{
	if (mApp->mAutoplayShotPrefix.empty() || mShots >= 40)
		return;
	mShots++;
	mApp->AutoplayShot(StrFormat("%s-%s%d.png", mApp->mAutoplayShotPrefix.c_str(), theName, theNumber));
}

void AutoplayBot::Finish()
{
	if (mFinished)
		return;
	mFinished = true;
	Log("done after %d games at update %d", mGames, mTick);
	if (mLog != NULL)
	{
		fclose(mLog);
		mLog = NULL;
	}
	mApp->Shutdown();
}

// A player's move: the piece follows the mouse to a cell, then a click puts it down
void AutoplayBot::PlayBoard(Board* theBoard)
{
	if (theBoard->mState != Board::STATE_PLAYING || theBoard->mPause > 0 || theBoard->mTrialPause > 0)
		return;

	// a tutorial step's text waits for the next button
	if (theBoard->mTutorial && theBoard->mTutWaiting)
	{
		Log("tutorial step %d at update %d", theBoard->mTutStep, mTick);
		Shot("tutorial", theBoard->mTutStep);
		theBoard->ButtonDepress(2);
		mWait = 5;
		return;
	}
	// the last step's button ends the tutorial
	if (theBoard->mTutorial && theBoard->mCurPiece == NULL && theBoard->mButton != NULL)
	{
		Log("tutorial button %d at update %d", theBoard->mButton->mId, mTick);
		Shot("tutorialend", theBoard->mButton->mId);
		theBoard->ButtonDepress(theBoard->mButton->mId);
		mWait = 5;
		return;
	}
	if (theBoard->mCurPiece == NULL)
		return;

	if (mClickX >= 0)
	{
		int aX = mClickX;
		int aY = mClickY;
		mClickX = -1;
		mApp->mWidgetManager->MouseDown(aX, aY, 1);
		mApp->mWidgetManager->MouseUp(aX, aY, 1);
		mMoves++;
		mWait = 1;
		return;
	}

	Piece* aPiece = theBoard->mCurPiece;
	std::vector<int> aCells;
	std::vector<int> aScores;
	int aBest = -1;
	for (int aRow = 0; aRow < Board::ROWS; aRow++)
		for (int aCol = 0; aCol < Board::COLS; aCol++)
		{
			int aReason;
			if ((theBoard->mGrid[aRow][aCol] != NULL && !aPiece->mBomb) || !theBoard->CanPlace(aPiece, aCol, aRow, &aReason))
				continue;
			// a cell that completes a row or a column is best (the tutorial asks for it too); otherwise the more
			// neighbours a cell has, the sooner a row or column fills
			int aRowCount = 0;
			for (int i = 0; i < Board::COLS; i++)
				if (theBoard->mGrid[aRow][i] != NULL)
					aRowCount++;
			int aColCount = 0;
			for (int i = 0; i < Board::ROWS; i++)
				if (theBoard->mGrid[i][aCol] != NULL)
					aColCount++;
			int aNeighbors = (aRowCount == Board::COLS - 1 || aColCount == Board::ROWS - 1) ? 10 : 0;
			if (aRow > 0 && theBoard->mGrid[aRow - 1][aCol] != NULL) aNeighbors++;
			if (aRow < Board::ROWS - 1 && theBoard->mGrid[aRow + 1][aCol] != NULL) aNeighbors++;
			if (aCol > 0 && theBoard->mGrid[aRow][aCol - 1] != NULL) aNeighbors++;
			if (aCol < Board::COLS - 1 && theBoard->mGrid[aRow][aCol + 1] != NULL) aNeighbors++;
			aCells.push_back(aRow * Board::COLS + aCol);
			aScores.push_back(aNeighbors);
			if (aNeighbors > aBest)
				aBest = aNeighbors;
		}

	if (aCells.empty())
	{
		// nowhere to put it: the forge
		mForges++;
		Log("forge %d at update %d (piece shape %d color %d)", mForges, mTick, aPiece->mShape, aPiece->mColor);
		theBoard->MouseDown(0, 0, -1);
		mWait = 3;
		return;
	}

	int aPick = -1;
	if (Rand(10) == 0)
		aPick = aCells[Rand((int) aCells.size())];
	else
	{
		int aCount = 0;
		for (size_t i = 0; i < aCells.size(); i++)
			if (aScores[i] == aBest && Rand(++aCount) == 0)
				aPick = aCells[i];
	}

	// the piece follows the mouse (Piece::Update puts it 0x30 pixels up and left of the pointer): the pointer goes there
	// now and the click comes with the next action, when the piece has caught up
	mClickX = AlchemyApp::BOARD_X + Board::ColToX(aPick % Board::COLS) + 0x30;
	mClickY = Board::RowToY(aPick / Board::COLS) + 0x30;
	mApp->mWidgetManager->MouseMove(mClickX, mClickY);
	mWait = 1;
}

void AutoplayBot::Update()
{
	mTick++;
	if (mFinished)
		return;
	if (mTick >= mApp->mAutoplayCap)
	{
		Log("cap reached at update %d: games %d moves %d", mTick, mGames, mMoves);
		Finish();
		return;
	}
	if (mWait > 0)
	{
		mWait--;
		return;
	}

	// the loading screen's link
	if (mApp->mTitleScreen != NULL)
	{
		if (mApp->mTitleScreen->mDone)
		{
			Log("title done at update %d", mTick);
			Shot("title", 0);
			mApp->mTitleScreen->ButtonDepress(1);
			mWait = 5;
		}
		return;
	}

	// the wizard's messages
	if (mApp->GetDialog(DIALOG_HINT) != NULL)
	{
		Log("hint dialog closed at update %d", mTick);
		Shot("hint", mTick);
		mApp->ButtonDepress(DIALOG_HINT + 2000);
		mWait = 5;
		return;
	}

	// the saved game of an earlier run: yes loads it, and that game is the bot's first
	if (mApp->GetDialog(DIALOG_LOAD_GAME) != NULL)
	{
		Log("saved game loaded at update %d", mTick);
		Shot("loaddialog", mTick);
		mApp->ButtonDepress(DIALOG_LOAD_GAME + 2000);
		mGames = 1;
		mWait = 20;
		return;
	}

	// a name for the score table
	if (mApp->GetDialog(DIALOG_NAME_ENTRY) != NULL)
	{
		Log("name entered at update %d", mTick);
		Shot("name", mGames);
		((NameEntryDialog*) mApp->GetDialog(DIALOG_NAME_ENTRY))->mEdit->SetText("Bot");
		mApp->ButtonDepress(DIALOG_NAME_ENTRY + 2000);
		mWait = 5;
		return;
	}

	// the score table after a game
	if (mApp->mHighScores != NULL)
	{
		if (mApp->mHighScores->mReveal < mApp->mHighScores->mHeight)
			return;
		if (!mOverLogged)
			return;
		Shot("scores", mGames);
		mApp->mHighScores->ButtonDepress(1);
		mWait = 5;
		return;
	}

	// the selector: the next game
	if (mApp->mSelector != NULL)
	{
		if (mGames >= mApp->mAutoplayGames)
		{
			Finish();
			return;
		}
		mGames++;
		mMoves = 0;
		mForges = 0;
		mOverLogged = false;
		mLastState = -1;
		mLastBoards = 0;
		Shot("selector", mGames);
		mApp->mTutorialMode = (mApp->mAutoplayMode == "tutorial");
		mApp->SetTimeTrial(mApp->mAutoplayMode == "timetrial");
		Log("game %d (%s) starts at update %d", mGames, mApp->mAutoplayMode.c_str(), mTick);
		mApp->StartGame();
		mWait = 20;
		return;
	}

	Board* aBoard = mApp->mBoard;
	if (aBoard == NULL)
		return;

	if (aBoard->mState != mLastState)
	{
		mLastState = aBoard->mState;
		Log("board state %d at update %d: score %d moves %d", aBoard->mState, mTick, aBoard->mScore, aBoard->mMoves);
	}

	if (aBoard->mState == Board::STATE_GAME_OVER)
	{
		if (!mOverLogged)
		{
			mOverLogged = true;
			Log("game %d over at update %d: score %d pieces %d forged %d boards %d", mGames, mTick, aBoard->mScore,
				aBoard->mMoves, aBoard->mDiscards, aBoard->mBoardsCleared);
			Shot("over", mGames);
		}
		return;
	}
	if (aBoard->mState == Board::STATE_BOARD_CLEARED)
	{
		if (aBoard->mBoardsCleared != mLastBoards)
		{
			mLastBoards = aBoard->mBoardsCleared;
			Log("board %d cleared at update %d: score %d", mLastBoards, mTick, aBoard->mScore);
			Shot("cleared", mLastBoards);
		}
		aBoard->ButtonDepress(0);
		mWait = 5;
		return;
	}

	if (mTick % 500 == 0)
	{
		Log("update %d: state %d piece %d pause %d trialpause %d helpshown %d newpiecedelay %d moves %d forged %d", mTick, aBoard->mState,
			aBoard->mCurPiece != NULL, aBoard->mPause, aBoard->mTrialPause, aBoard->mHelpShown, aBoard->mNewPieceDelay, mMoves, mForges);
		if (mTick % 2000 == 0)
			Shot("tick", mTick);
	}

	PlayBoard(aBoard);
}
