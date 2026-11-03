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
//   -autoplaymenu         the bot does not start games: a -script clicks through the menus and the bot plays the
//                         games it starts (its clicks are in the log, for replay in the original)
//   -autoplaylog=<file>   the log
//   -autoplayshots=<prefix>  screenshots at each level-up and game over

#include "NoahApp.h"
#include "Board.h"
#include "Screens.h"
#include "Dialogs.h"
#include "Graphics.h"
#include "WidgetManager.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
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
	else if (theParamName == "-autoplaylog")
		mAutoplayLog = theParamValue;
	else if (theParamName == "-autoplaymenu")
		mAutoplayMenu = true;
	else if (theParamName == "-statelogcells")
		mStateLogCells = theParamValue;
	else if (theParamName == "-statelog")
		mStateLog = theParamValue;
	else if (theParamName == "-autoplayshots")
		mAutoplayShots = theParamValue;
	else
		SexyApp::HandleCmdLineParam(theParamName, theParamValue);
}

namespace
{

void Log(AutoplayState* s, const char* theFormat, ...)
{
	if (s->mLog == NULL)
		return;
	va_list args;
	va_start(args, theFormat);
	vfprintf(s->mLog, theFormat, args);
	va_end(args);
	fputc('\n', s->mLog);
	fflush(s->mLog);
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

bool Quiet(Piece* p)
{
	return (p->mType < 0xd) && !p->mRemoved && !p->mFading && !p->mMoving && (p->mOffsetX == 0) && (p->mOffsetY == 0);
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

void Click(Board* b, int x, int y)
{
	int aX = Board::CellLeft(x) + 25;
	int aY = Board::CellTop(y) + 25;
	// logged with the game's update count so that the same clicks can be replayed in the original (tools/origrun_script.py)
	Log(b->mApp->mAutoplayState, "click at update %d: %d %d", b->mApp->mUpdateCount, aX, aY);
	b->MouseMove(aX, aY);
	b->MouseDown(aX, aY, 1);
	b->MouseUp(aX, aY, 1);
}

}

void NoahApp::AutoplayUpdate()
{
	AutoplayState* s = mAutoplayState;
	s->mTick++;
	if ((s->mLog == NULL) && !mAutoplayLog.empty())
	{
		s->mLog = fopen(mAutoplayLog.c_str(), "w");
		Log(s, "autoplay seed %u mode %s diff %d games %d", s->mSeed, ModeName(mAutoplayMode), mAutoplayDiff, mAutoplayGames);
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
		if ((aMoving > 0) && (s->mWalking == 0))
			Log(s, "pair joined at update %d: first animal at column %d row %d", s->mTick, aFirstX, aFirstY);
		s->mWalking = aMoving;
	}
	if (s->mTick > mAutoplayCap)
	{
		Log(s, "cap reached at update %d: games %d", s->mTick, s->mGames);
		if (s->mLog != NULL)
			fclose(s->mLog);
		Shutdown();
		return;
	}
	if (!mAutoplayMenu)
		mShowTutorial = false;

	if (mTitleScreen != NULL)
	{
		if (mLoaded && (s->mTick > 60) && !mAutoplayMenu)
			TitleScreenDone();
		return;
	}
	if (s->mWait > 0)
	{
		s->mWait--;
		return;
	}

	if (mHighScoreWidget != NULL)
	{
		if (!s->mGameOverLogged)
		{
			s->mGameOverLogged = true;
			Log(s, "game %d over at update %d: score %d level %d moves %d", s->mGames, s->mTick, mGameScore, mLevel, s->mMoves);
			if (!mAutoplayShots.empty())
				SaveScreenshot(StrFormat("%s-over%d.png", mAutoplayShots.c_str(), s->mGames));
		}
		Dialog* aDialog = GetDialog(DIALOG_RECORD_SCORE);
		if (aDialog != NULL)
		{
			if (mAutoplayMenu)
			{
				if (((RecordScoreDialog*) aDialog)->mNameEdit->mString.empty())
				{
					Log(s, "text at update %d: BOT", mUpdateCount);
					((RecordScoreDialog*) aDialog)->mNameEdit->SetText("BOT");
					s->mWait = 5;
				}
				else
					WidgetClick(this, aDialog->mYesButton);
				s->mWait = 5;
				return;
			}
			((RecordScoreDialog*) aDialog)->mNameEdit->SetText("BOT");
			ButtonDepress(2000 + DIALOG_RECORD_SCORE);
			s->mWait = 5;
			return;
		}
		if (mHighScoreWidget->mFade >= 255.0)
		{
			Log(s, "high scores: %d %d %d %d %d", mHighScoreWidget->mScores[0], mHighScoreWidget->mScores[1],
				mHighScoreWidget->mScores[2], mHighScoreWidget->mScores[3], mHighScoreWidget->mScores[4]);
			if (mAutoplayMenu)
				WidgetClick(this, mHighScoreWidget->mMenuLink);
			else
				mHighScoreWidget->ButtonDepress(1);
			s->mWait = 10;
		}
		return;
	}
	if (mIntroWidget != NULL)
	{
		if (mAutoplayMenu)
			return;
		if (s->mGames >= mAutoplayGames)
		{
			Log(s, "done after %d games at update %d", s->mGames, s->mTick);
			if (s->mLog != NULL)
				fclose(s->mLog);
			Shutdown();
			return;
		}
		s->mGames++;
		s->mGameOverLogged = false;
		s->mMoves = 0;
		s->mLastLevel = 0;
		mGameMode = mAutoplayMode;
		mDifficulty = mAutoplayDiff;
		DeleteSavedGame();
		Log(s, "game %d starts at update %d", s->mGames, s->mTick);
		StartGame(false);
		return;
	}
	if (mNextLevelWidget != NULL)
	{
		if (!s->mLevelLogged)
		{
			s->mLevelLogged = true;
			Log(s, "level %d done at update %d: score %d mode %s", mLevel, s->mTick, mGameScore, ModeName(mGameMode));
			if (!mAutoplayShots.empty())
				SaveScreenshot(StrFormat("%s-level%d-%d.png", mAutoplayShots.c_str(), s->mGames, mLevel));
			s->mWait = 30;
			return;
		}
		if (mAutoplayMenu)
			WidgetClick(this, mNextLevelWidget->mNextLink);
		else
			mNextLevelWidget->ButtonDepress(0);
		s->mLevelLogged = false;
		s->mWait = 10;
		return;
	}
	if (mBoard == NULL)
		return;
	if (GetDialog(DIALOG_MODE_INTRO) != NULL)
	{
		if (mAutoplayMenu)
			return;                 // a -script clicks it, as in the original
		ButtonDepress(2000 + DIALOG_MODE_INTRO);
		s->mWait = 5;
		return;
	}
	if (GetDialog(DIALOG_TUTORIAL) != NULL)
		return;
	Board* b = mBoard;
	if (b->mPaused)
		b->mPaused = false;
	if (b->mState != BOARD_PLAYING)
		return;
	if (b->mUpdateCnt % 4 != 0)
		return;
	if (!b->mLink.empty())
		return;
	for (int x = 0; x < 9; x++)
		for (int y = 0; y < 9; y++)
			if (b->mGrid[x][y]->mMoving || ((b->mGrid[x][y]->mOffsetY != 0) && (b->mGrid[x][y]->mType < 0xd)) || b->mGrid[x][y]->mRemoved)
				return;

	if (mGameMode == GAMEMODE_STAMPEDE)
	{
		// every group of three or more, one of them at random
		std::vector<std::pair<int, int> > aGroups;
		for (int x = 0; x < 9; x++)
			for (int y = 0; y < 9; y++)
			{
				Piece* p = b->mGrid[x][y];
				if (!Quiet(p))
					continue;
				b->HighlightGroup(x, y);
				if (b->mLink.size() >= 3)
					aGroups.push_back(std::make_pair(x, y));
				b->ClearLinkList();
			}
		if (aGroups.empty())
			return;
		std::pair<int, int> aPick = aGroups[s->Rand((int) aGroups.size())];
		int aX = Board::CellLeft(aPick.first) + 25;
		int aY = Board::CellTop(aPick.second) + 25;
		b->MouseMove(aX, aY);
		b->MouseDown(aX, aY, 1);
		b->MouseUp(aX, aY, 1);
		s->mMoves++;
		return;
	}
	if (mGameMode == GAMEMODE_PUZZLE)
	{
		static const int aDirs[2][2] = {{1, 0}, {0, 1}};
		std::vector<int> aSwaps;
		for (int x = 0; x < 9; x++)
			for (int y = 0; y < 9; y++)
				for (int d = 0; d < 2; d++)
				{
					int nx = x + aDirs[d][0], ny = y + aDirs[d][1];
					if ((nx > 8) || (ny > 8) || !Quiet(b->mGrid[x][y]) || !Quiet(b->mGrid[nx][ny]))
						continue;
					std::swap(b->mGrid[x][y]->mType, b->mGrid[nx][ny]->mType);
					bool aMatch = false;
					for (int c = 0; c < 2; c++)
					{
						int cx = c ? nx : x, cy = c ? ny : y;
						int aType = b->mGrid[cx][cy]->mType;
						int aRun = 1;
						for (int i = cx - 1; i >= 0 && b->mGrid[i][cy]->mType == aType; i--) aRun++;
						for (int i = cx + 1; i < 9 && b->mGrid[i][cy]->mType == aType; i++) aRun++;
						if (aRun >= 3) aMatch = true;
						aRun = 1;
						for (int i = cy - 1; i >= 0 && b->mGrid[cx][i]->mType == aType; i--) aRun++;
						for (int i = cy + 1; i < 9 && b->mGrid[cx][i]->mType == aType; i++) aRun++;
						if (aRun >= 3) aMatch = true;
					}
					std::swap(b->mGrid[x][y]->mType, b->mGrid[nx][ny]->mType);
					if (aMatch)
						aSwaps.push_back(((x * 9 + y) << 8) | (nx * 9 + ny));
				}
		if (aSwaps.empty())
			return;
		int aPick = aSwaps[s->Rand((int) aSwaps.size())];
		int a = aPick >> 8, c = aPick & 0xff;
		Click(b, a / 9, a % 9);
		Click(b, c / 9, c % 9);
		s->mMoves++;
		return;
	}

	// action and strategy: a random pair that can be joined
	std::vector<std::pair<Piece*, Piece*> > aPairs;
	for (int x = 0; x < 9; x++)
		for (int y = 0; y < 9; y++)
		{
			Piece* p = b->mGrid[x][y];
			if (!Quiet(p))
				continue;
			for (int x2 = x; x2 < 9; x2++)
				for (int y2 = (x2 == x) ? y + 1 : 0; y2 < 9; y2++)
				{
					Piece* q = b->mGrid[x2][y2];
					if (Quiet(q) && (q->mType == p->mType) && b->Reachable(p, q))
						aPairs.push_back(std::make_pair(p, q));
				}
		}
	if (aPairs.empty())
		return;
	std::pair<Piece*, Piece*> aPick = aPairs[s->Rand((int) aPairs.size())];
	Click(b, aPick.first->mX, aPick.first->mY);
	Click(b, aPick.second->mX, aPick.second->mY);
	s->mMoves++;
	if (mLevel != s->mLastLevel)
	{
		s->mLastLevel = mLevel;
		Log(s, "level %d at update %d", mLevel, s->mTick);
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
	StateLogState() : mFile(NULL), mState(-2), mScore(-2), mLevel(-2), mMode(-2), mCells(0), mWater(-99999), mLogWater(false) {}
};
}

void NoahApp::StateLogUpdate()
{
	if (mStateLogState == NULL)
	{
		mStateLogState = new StateLogState();
		mStateLogState->mFile = fopen(mStateLog.c_str(), "w");
		const char* c = mStateLogCells.c_str();
		int x, y, n;
		if (*c == 'w')
		{
			mStateLogState->mLogWater = true;
			c++;
			if (*c == ';')
				c++;
		}
		while ((mStateLogState->mCells < 4) && (sscanf(c, "%d,%d%n", &x, &y, &n) == 2))
		{
			mStateLogState->mCellX[mStateLogState->mCells] = x;
			mStateLogState->mCellY[mStateLogState->mCells] = y;
			mStateLogState->mCellType[mStateLogState->mCells++] = -2;
			c += n;
			if (*c == ';')
				c++;
		}
	}
	StateLogState* s = mStateLogState;
	if (s->mFile == NULL)
		return;
	if (s->mLogWater && (mBoard != NULL) && (mBoard->mWaterY != s->mWater))
	{
		fprintf(s->mFile, "water %d -> %d at update %d\n", s->mWater, mBoard->mWaterY, mUpdateCount);
		s->mWater = mBoard->mWaterY;
	}
	for (int i = 0; (i < s->mCells) && (mBoard != NULL); i++)
	{
		int t = mBoard->mGrid[s->mCellX[i]][s->mCellY[i]]->mType;
		if (t != s->mCellType[i])
		{
			fprintf(s->mFile, "cell %d,%d: %d -> %d at update %d\n", s->mCellX[i], s->mCellY[i], s->mCellType[i], t, mUpdateCount);
			s->mCellType[i] = t;
		}
	}
	int aState = (mBoard != NULL) ? (int) mBoard->mState : -1;
	if ((aState == s->mState) && (mGameScore == s->mScore) && (mLevel == s->mLevel) && (mGameMode == s->mMode))
		return;
	fprintf(s->mFile, "update %d: state %d score %d level %d mode %d\n", mUpdateCount, aState, mGameScore, mLevel, mGameMode);
	if ((aState != s->mState) && (mBoard != NULL))
	{
		for (int y = 0; y < 9; y++)
		{
			char aRow[10];
			for (int x = 0; x < 9; x++)
			{
				Piece* p = mBoard->mGrid[x][y];
				int t = (p == NULL) ? PIECE_EMPTY : p->mType;
				aRow[x] = (t == PIECE_EMPTY) ? '.' : (t == PIECE_BLOCK) ? 'X' : (t == PIECE_CRATE) ? 'Y' : (char) ((t < 10) ? ('0' + t) : ('A' + t - 10));
			}
			aRow[9] = 0;
			fprintf(s->mFile, "grid %d %s\n", y, aRow);
		}
	}
	fflush(s->mFile);
	s->mState = aState;
	s->mScore = mGameScore;
	s->mLevel = mLevel;
	s->mMode = mGameMode;
}
