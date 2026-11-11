// Test hooks: the autoplay bot (-autoplay=1). It plays whole games by calling the board's mouse handlers, so
// everything the game does for a player runs: pairs are linked, scored and taken in, rows rise, levels and the
// bonus round come and go, the game ends and the score goes to the high-score list. The bot has its own
// random numbers (the game's use the C library generator, seeded by -seed), so the same -seed and -autoplay
// always make the same game. It writes a log of what happened (-autoplaylog=<file>) that tests compare.
//
//   -autoplay=<seed>      play with the bot (its own seed)
//   -autoplaymode=action|strategy|puzzle   the game type (default action)
//   -autoplaydiff=0|4|8   the difficulty (easy, medium, hard)
//   -autoplaygames=N      play N games, then quit
//   -autoplaycap=N        quit after N updates in any case
//   -autoplaystop=N       the bot makes no more moves after update N (the run goes on: the flood, the game over and the name entry
//                         it answers still happen)
//   -autoplaymenu         the bot does not start games: a -script clicks through the menus and the bot plays the
//                         games it starts (its clicks are in the log, for replay in the original)
//   -autoplaylog=<file>   the log
//   -autoplayshots=<prefix>  screenshots at each level-up and game over

#include "NoahApp.h"
#include "Board.h"
#include "Bot.h"
#include "Screens.h"
#include "Dialogs.h"
#include "Graphics.h"
#include "WidgetManager.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <cstdarg>
#include <vector>

using namespace Sexy;

namespace Sexy
{

struct AutoplayState
{
	unsigned int	mSeed;
	int				mTick;
	int				mGames;             // games started
	int				mWait;              // updates to wait before the next action
	int				mLevelsSeen;
	int				mShots;
	bool			mGameOverLogged;
	bool			mLevelLogged;
	int				mMoves;             // moves made in this game
	int				mWalking;           // animals walking at the last update
	int				mLastLevel;
	FILE*			mLog;

	AutoplayState() : mSeed(1), mTick(0), mGames(0), mWait(0), mLevelsSeen(0), mShots(0), mGameOverLogged(false),
		mLevelLogged(false), mMoves(0), mWalking(0), mLastLevel(0), mLog(NULL) {}

	int Rand(int theRange)
	{
		mSeed = mSeed * 1103515245u + 12345u;
		return (int) ((mSeed >> 16) % (unsigned int) theRange);
	}
};

}

namespace
{
struct ResyncEntry { char mType; int mLevel; unsigned int mState; int mUpdate; };
}

namespace Sexy
{
struct ResyncState
{
	std::vector<ResyncEntry>	mEntries;
	size_t						mNext[2];                // the next entry to look at, for the level starts and for the get-ready ends
	ResyncState() { mNext[0] = mNext[1] = 0; }
};
}

// A start of a level ('n', right after its level counter went up) or the end of its get-ready ('r'): the original's state
// there, if the -resync file has one
void NoahApp::ResyncEvent(char theType)
{
	if (mResync.empty())
		return;
	if (mResyncState == NULL)
	{
		mResyncState = new ResyncState();
		FILE* aFile = fopen(mResync.c_str(), "r");
		char aKind;
		int aLevel;
		unsigned int aSeedState;
		int aUpdate;
		while ((aFile != NULL) && (fscanf(aFile, " %c %d %u %d", &aKind, &aLevel, &aSeedState, &aUpdate) == 4))
		{
			ResyncEntry anEntry = { aKind, aLevel, aSeedState, aUpdate };
			mResyncState->mEntries.push_back(anEntry);
		}
		if (aFile != NULL)
			fclose(aFile);
	}
	ResyncState* aResync = mResyncState;
	size_t& aNext = aResync->mNext[theType == 'r' ? 1 : 0];
	for (size_t i = aNext; i < aResync->mEntries.size(); i++)
	{
		if ((aResync->mEntries[i].mType == theType) && (aResync->mEntries[i].mLevel == mLevel))
		{
			CrtSRand(aResync->mEntries[i].mState);
			aNext = i + 1;
			return;
		}
	}
}

// The get-ready may not end before the update the original's did (the original's lasts a little longer: the bot's clicks are
// timed by update)
bool NoahApp::ResyncHold()
{
	if (mResync.empty())
		return false;
	ResyncEvent(0);
	ResyncState* r = mResyncState;
	for (size_t i = r->mNext[1]; i < r->mEntries.size(); i++)
		if ((r->mEntries[i].mType == 'r') && (r->mEntries[i].mLevel == mLevel))
			return mUpdateCount < r->mEntries[i].mUpdate - 2;
	return false;
}

void NoahApp::HandleCmdLineParam(const std::string& theParamName, const std::string& theParamValue)
{
	if (theParamName == "-autoplay")
	{
		mAutoplay = true;
		mAutoplayState = new AutoplayState();
		mAutoplayState->mSeed = (unsigned int) atoi(theParamValue.c_str());
		mScriptedInput = true;   // the bot's input only, as in a -script run
	}
	else if (theParamName == "-autoplaymode")
		mAutoplayMode = (theParamValue == "strategy") ? GAMEMODE_STRATEGY : ((theParamValue == "puzzle") ? GAMEMODE_PUZZLE : GAMEMODE_ACTION);
	else if (theParamName == "-autoplaydiff")
		mAutoplayDiff = atoi(theParamValue.c_str());
	else if (theParamName == "-autoplaygames")
		mAutoplayGames = atoi(theParamValue.c_str());
	else if (theParamName == "-autoplaycap")
		mAutoplayCap = atoi(theParamValue.c_str());
	else if (theParamName == "-autoplaystop")
		mAutoplayStop = atoi(theParamValue.c_str());
	else if (theParamName == "-autoplaylog")
		mAutoplayLog = theParamValue;
	else if (theParamName == "-autoplaymenu")
		mAutoplayMenu = true;
	else if (theParamName == "-statelogcells")
		mStateLogCells = theParamValue;
	else if (theParamName == "-resync")
		mResync = theParamValue;
	else if (theParamName == "-statelog")
		mStateLog = theParamValue;
	else if (theParamName == "-autoplayshots")
		mAutoplayShots = theParamValue;
	else
		SexyApp::HandleCmdLineParam(theParamName, theParamValue);
}

namespace
{

void Log(AutoplayState* theState, const char* theFormat, ...)
{
	if (theState->mLog == NULL)
		return;
	va_list args;
	va_start(args, theFormat);
	vfprintf(theState->mLog, theFormat, args);
	va_end(args);
	fputc('\n', theState->mLog);
	fflush(theState->mLog);
}

const char* ModeName(int theMode)
{
	switch (theMode)
	{
	case GAMEMODE_ACTION: return "action";
	case GAMEMODE_STRATEGY: return "strategy";
	case GAMEMODE_PUZZLE: return "puzzle";
	case GAMEMODE_STAMPEDE: return "stampede";
	}
	return "?";
}

// -autoplaymenu: a click on a widget of the app (not the board), logged like the board's so that it can be replayed
void WidgetClick(NoahApp* theApp, Widget* theWidget)
{
	int aX = theWidget->mX + theWidget->mWidth / 2;
	int aY = theWidget->mY + theWidget->mHeight / 2;
	Log(theApp->mAutoplayState, "click at update %d: %d %d", theApp->mUpdateCount, aX, aY);
	theApp->mWidgetManager->MouseMove(aX, aY);
	theApp->mWidgetManager->MouseDown(aX, aY, 1);
	theApp->mWidgetManager->MouseUp(aX, aY, 1);
}

void Click(Board* theBoard, int x, int y)
{
	int aX = Board::CellLeft(x) + 25;
	int aY = Board::CellTop(y) + 25;
	// logged with the game's update count so that the same clicks can be replayed in the original (tools/origrun_script.py)
	Log(theBoard->mApp->mAutoplayState, "click at update %d: %d %d", theBoard->mApp->mUpdateCount, aX, aY);
	theBoard->MouseMove(aX, aY);
	theBoard->MouseDown(aX, aY, 1);
	theBoard->MouseUp(aX, aY, 1);
}

}

void NoahApp::AutoplayUpdate()
{
	AutoplayState* aState = mAutoplayState;
	aState->mTick++;
	if ((aState->mLog == NULL) && !mAutoplayLog.empty())
	{
		aState->mLog = fopen(mAutoplayLog.c_str(), "w");
		Log(aState, "autoplay seed %u mode %s diff %d games %d", aState->mSeed, ModeName(mAutoplayMode), mAutoplayDiff, mAutoplayGames);
	}
	// a pair that starts walking (a match was made): where the two animals are, for screenshots of the walk
	if (mBoard != NULL)
	{
		int aMoving = 0, aFirstX = -1, aFirstY = -1;
		for (int x = 0; x < 9; x++)
			for (int y = 0; y < 9; y++)
				if ((mBoard->mGrid[x][y] != NULL) && mBoard->mGrid[x][y]->mMoving)
				{
					if (aMoving == 0)
					{
						aFirstX = x;
						aFirstY = y;
					}
					aMoving++;
				}
		if ((aMoving > 0) && (aState->mWalking == 0))
			Log(aState, "pair joined at update %d: first animal at column %d row %d", aState->mTick, aFirstX, aFirstY);
		aState->mWalking = aMoving;
	}
	if (aState->mTick > mAutoplayCap)
	{
		Log(aState, "cap reached at update %d: games %d", aState->mTick, aState->mGames);
		if (aState->mLog != NULL)
			fclose(aState->mLog);
		Shutdown();
		return;
	}
	if (!mAutoplayMenu)
		mShowTutorial = false;

	if (mTitleScreen != NULL)
	{
		if (mLoaded && (aState->mTick > 60) && !mAutoplayMenu)
			TitleScreenDone();
		return;
	}
	if (aState->mWait > 0)
	{
		aState->mWait--;
		return;
	}

	if (mHighScoreWidget != NULL)
	{
		if (!aState->mGameOverLogged)
		{
			aState->mGameOverLogged = true;
			Log(aState, "game %d over at update %d: score %d level %d moves %d", aState->mGames, aState->mTick, mGameScore, mLevel, aState->mMoves);
			if (!mAutoplayShots.empty())
				SaveScreenshot(StrFormat("%s-over%d.png", mAutoplayShots.c_str(), aState->mGames));
		}
		Dialog* aDialog = GetDialog(DIALOG_RECORD_SCORE);
		if (aDialog != NULL)
		{
			if (mAutoplayMenu)
			{
				if (((RecordScoreDialog*) aDialog)->mNameEdit->mString.empty())
				{
					Log(aState, "text at update %d: BOT", mUpdateCount);
					// typed like a player (the original is given the characters too), so nothing is selected
					for (const char* aChar = "BOT"; *aChar != 0; aChar++)
						((RecordScoreDialog*) aDialog)->mNameEdit->KeyChar((SexyChar) *aChar);
					aState->mWait = 5;
				}
				else
					WidgetClick(this, aDialog->mYesButton);
				aState->mWait = 5;
				return;
			}
			((RecordScoreDialog*) aDialog)->mNameEdit->SetText("BOT");
			ButtonDepress(2000 + DIALOG_RECORD_SCORE);
			aState->mWait = 5;
			return;
		}
		if (mHighScoreWidget->mFade >= 255.0)
		{
			Log(aState, "high scores: %d %d %d %d %d", mHighScoreWidget->mScores[0], mHighScoreWidget->mScores[1],
				mHighScoreWidget->mScores[2], mHighScoreWidget->mScores[3], mHighScoreWidget->mScores[4]);
			if (mAutoplayMenu)
				WidgetClick(this, mHighScoreWidget->mMenuLink);
			else
				mHighScoreWidget->ButtonDepress(1);
			aState->mWait = 10;
		}
		return;
	}
	if (mIntroWidget != NULL)
	{
		if (mAutoplayMenu)
			return;
		if (aState->mGames >= mAutoplayGames)
		{
			Log(aState, "done after %d games at update %d", aState->mGames, aState->mTick);
			if (aState->mLog != NULL)
				fclose(aState->mLog);
			Shutdown();
			return;
		}
		aState->mGames++;
		aState->mGameOverLogged = false;
		aState->mMoves = 0;
		aState->mLastLevel = 0;
		mGameMode = mAutoplayMode;
		mDifficulty = mAutoplayDiff;
		DeleteSavedGame();
		Log(aState, "game %d starts at update %d", aState->mGames, aState->mTick);
		StartGame(false);
		return;
	}
	if (mNextLevelWidget != NULL)
	{
		if (!aState->mLevelLogged)
		{
			aState->mLevelLogged = true;
			Log(aState, "level %d done at update %d: score %d mode %s", mLevel, aState->mTick, mGameScore, ModeName(mGameMode));
			if (!mAutoplayShots.empty())
				SaveScreenshot(StrFormat("%s-level%d-%d.png", mAutoplayShots.c_str(), aState->mGames, mLevel));
			aState->mWait = 30;
			return;
		}
		if (mAutoplayMenu)
			WidgetClick(this, mNextLevelWidget->mNextLink);
		else
			mNextLevelWidget->ButtonDepress(0);
		aState->mLevelLogged = false;
		aState->mWait = 10;
		return;
	}
	if (mBoard == NULL)
		return;
	if (GetDialog(DIALOG_MODE_INTRO) != NULL)
	{
		if (mAutoplayMenu)
			return;                 // a -script clicks it, as in the original
		ButtonDepress(2000 + DIALOG_MODE_INTRO);
		aState->mWait = 5;
		return;
	}
	if ((GetDialog(DIALOG_TUTORIAL) != NULL) || (GetDialog(DIALOG_QUIT) != NULL))
		return;                     // a script answers these dialogs; the bot plays on the board only
	Board* aBoard = mBoard;
	if (aBoard->mPaused)
		aBoard->mPaused = false;
	if (aBoard->mState != BOARD_PLAYING)
		return;
	if (aBoard->mUpdateCnt % 4 != 0)
		return;
	if (!BotBoardSettled(aBoard))
		return;
	if ((mAutoplayStop > 0) && (aState->mTick > mAutoplayStop))
		return;                     // the bot has stopped moving (a game it leaves alone is lost); the run goes on

	// the bot's move (Bot.cpp, which the test command "move" gives a driver too); its clicks are logged, so that the
	// original can replay them
	BotMove aMove;
	if (!BotChooseMove(aBoard, mGameMode, aState->mSeed, aMove))
		return;
	for (int i = 0; i < aMove.mCount; i++)
		Click(aBoard, aMove.mCellX[i], aMove.mCellY[i]);
	aState->mMoves++;
	if ((mGameMode != GAMEMODE_STAMPEDE) && (mGameMode != GAMEMODE_PUZZLE) && (mLevel != aState->mLastLevel))
	{
		aState->mLastLevel = mLevel;
		Log(aState, "level %d at update %d", mLevel, aState->mTick);
	}
}

// -statelog=<file>: a line for each update at which the board's state, the score, the level or the mode changed,
// and the nine by nine field's animal types when the state changed ("grid" lines: a digit or letter per cell, columns
// left to right within each row, '.' empty, 'X' block, 'Y' crate; animals 0-9, A-D), so a run can be compared with the original's
// (origrun --watch of [[0x4d125c]+0x408]+0x310, +0x474, +0x43c, +0x444).
namespace Sexy
{
struct StateLogState
{
	FILE*			mFile;
	int				mState, mScore, mLevel, mMode;
	int				mCellX[4], mCellY[4], mCellType[4], mCells;
	int				mWater;                 // -statelogcells=w: the water level (the board's mWaterY) at every change
	bool			mLogWater;
	unsigned int	mRng;                   // -statelogcells=r: the C generator's state after every 512th draw
	bool			mLogRng;
	unsigned int	mRngCount;              // -statelogcells=c: the number of draws made, at every update that made some
	bool			mLogRngCount;
	StateLogState() : mFile(NULL), mState(-2), mScore(-2), mLevel(-2), mMode(-2), mCells(0), mWater(-99999), mLogWater(false), mRng(0), mLogRng(false), mRngCount(0), mLogRngCount(false) {}
};
}

void NoahApp::StateLogUpdate()
{
	if (mStateLogState == NULL)
	{
		mStateLogState = new StateLogState();
		mStateLogState->mFile = fopen(mStateLog.c_str(), "w");
		const char* aSpec = mStateLogCells.c_str();
		int x, y, n;
		while ((*aSpec == 'w') || (*aSpec == 'r') || (*aSpec == 'c'))
		{
			if (*aSpec == 'w')
				mStateLogState->mLogWater = true;
			else if (*aSpec == 'r')
				mStateLogState->mLogRng = true;
			else
				mStateLogState->mLogRngCount = true;
			aSpec++;
			if (*aSpec == ';')
				aSpec++;
		}
		while ((mStateLogState->mCells < 4) && (sscanf(aSpec, "%d,%d%n", &x, &y, &n) == 2))
		{
			mStateLogState->mCellX[mStateLogState->mCells] = x;
			mStateLogState->mCellY[mStateLogState->mCells] = y;
			mStateLogState->mCellType[mStateLogState->mCells++] = -2;
			aSpec += n;
			if (*aSpec == ';')
				aSpec++;
		}
	}
	StateLogState* aLog = mStateLogState;
	if (aLog->mFile == NULL)
		return;
	if (aLog->mLogWater && (mBoard != NULL) && (mBoard->mWaterY != aLog->mWater))
	{
		fprintf(aLog->mFile, "water %d -> %d at update %d\n", aLog->mWater, mBoard->mWaterY, mUpdateCount);
		aLog->mWater = mBoard->mWaterY;
	}
	if (aLog->mLogRngCount && (CrtRandCount() != aLog->mRngCount))
	{
		fprintf(aLog->mFile, "rngc %u at update %d\n", CrtRandCount(), mUpdateCount);
		aLog->mRngCount = CrtRandCount();
	}
	if (aLog->mLogRng && (CrtRandSnapshot() != aLog->mRng))
	{
		fprintf(aLog->mFile, "rng %u -> %u at update %d\n", aLog->mRng, CrtRandSnapshot(), mUpdateCount);
		aLog->mRng = CrtRandSnapshot();
	}
	for (int i = 0; (i < aLog->mCells) && (mBoard != NULL); i++)
	{
		int aType = mBoard->mGrid[aLog->mCellX[i]][aLog->mCellY[i]]->mType;
		if (aType != aLog->mCellType[i])
		{
			fprintf(aLog->mFile, "cell %d,%d: %d -> %d at update %d\n", aLog->mCellX[i], aLog->mCellY[i], aLog->mCellType[i], aType, mUpdateCount);
			aLog->mCellType[i] = aType;
		}
	}
	int aState = (mBoard != NULL) ? (int) mBoard->mState : -1;
	if ((aState == aLog->mState) && (mGameScore == aLog->mScore) && (mLevel == aLog->mLevel) && (mGameMode == aLog->mMode))
		return;
	fprintf(aLog->mFile, "update %d: state %d score %d level %d mode %d\n", mUpdateCount, aState, mGameScore, mLevel, mGameMode);
	if ((aState != aLog->mState) && (mBoard != NULL))
	{
		for (int y = 0; y < 9; y++)
		{
			char aRow[10];
			for (int x = 0; x < 9; x++)
			{
				Piece* aPiece = mBoard->mGrid[x][y];
				int aType = (aPiece == NULL) ? PIECE_EMPTY : aPiece->mType;
				aRow[x] = (aType == PIECE_EMPTY) ? '.' : (aType == PIECE_CRATE) ? 'X' : (aType == PIECE_BLOCK) ? 'Y' : (char) ((aType < 10) ? ('0' + aType) : ('A' + aType - 10));
			}
			aRow[9] = 0;
			fprintf(aLog->mFile, "grid %d %s\n", y, aRow);
		}
	}
	fflush(aLog->mFile);
	aLog->mState = aState;
	aLog->mScore = mGameScore;
	aLog->mLevel = mLevel;
	aLog->mMode = mGameMode;
}
