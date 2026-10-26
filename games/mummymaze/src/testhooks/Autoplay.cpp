// Test hooks: the autoplay bot (-autoplay=<seed>), kept apart from the game code. It plays whole classic games through
// the keys the board reads and the buttons of the screens, so everything the game does for a player runs: the
// explorer walks, the mummies and the scorpion answer, chambers and pyramids come and go, the score and the high-score
// list are kept. The bot has its own random numbers (the game's use the C library generator, seeded by -seed), so the
// same -seed and -autoplay always make the same game. It writes a log of what happened (-autoplaylog=<file>) that
// tests compare.
//
//   -autoplay=<seed>         play with the bot (its own seed)
//   -autoplaygames=N         play N games, then quit
//   -autoplaylevels=N        levels played in a game before the bot gives up with Abandon Hope (default 3)
//   -autoplaystart=P,C       the first game starts at pyramid P (0 to 14), chamber C (0 to 14)
//   -autoplaycap=N           quit after N updates in any case
//   -autoplaylog=<file>      the log
//   -autoplayshots=<prefix>  screenshots at each game over
//
// A level is played in one of two ways, by the bot's dice: with the game's own solver (the moves of the shortest way
// out, pressed as keys), or as a walk of random moves that the mummies end or that goes on until the bot solves it.

#include "MummyApp.h"
#include "Board.h"
#include "Screens.h"
#include "Dialogs.h"
#include "Graphics.h"
#include "WidgetManager.h"

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
	unsigned int		mSeed;
	int					mTick;
	int					mGames;             // games started
	int					mWait;              // updates to wait before the next action
	int					mLevels;            // levels finished in this game
	int					mLevelsPerGame;
	int					mStartPyramid, mStartChamber;
	int					mHeldKey;           // the key that is down, or 0
	bool				mPlanned;           // the board's current level has a plan
	bool				mUseSolver;
	int					mRandomMoves;       // moves still to make at random
	std::vector<ChaseState> mPath;          // the solver's way out: the explorer's places
	size_t				mPathIndex;
	bool				mGameOverLogged;
	bool				mGivenUp;           // Abandon Hope was pressed in this game
	int					mOptionsStage;      // with screenshots on: 0 not yet, 1 options up, 2 its high scores seen, 3 its credits seen
	unsigned int		mDialogShots;       // a bit per dialog id: its screenshot was taken
	unsigned int		mDialogSeen;        // a bit per dialog id: it has been up for a while
	int					mShotWait;          // the chamber-done screenshot: 0 not yet, 1 waiting, 2 taken
	FILE*				mLog;

	AutoplayState() : mSeed(1), mTick(0), mGames(0), mWait(0), mLevels(0), mLevelsPerGame(3), mStartPyramid(0),
		mStartChamber(0), mHeldKey(0), mPlanned(false), mUseSolver(true), mRandomMoves(0), mPathIndex(0),
		mGameOverLogged(false), mGivenUp(false), mOptionsStage(0), mDialogShots(0), mDialogSeen(0), mShotWait(0), mLog(NULL) {}

	int Rand(int theRange)
	{
		mSeed = mSeed * 1103515245u + 12345u;
		return (int) ((mSeed >> 16) % (unsigned int) theRange);
	}
};

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

}

void MummyApp::HandleAutoplayParam(const std::string& theParamName, const std::string& theParamValue)
{
	if (mAutoplayState == NULL)
		mAutoplayState = new AutoplayState();
	if (theParamName == "-autoplay")
		mAutoplayState->mSeed = (unsigned int) atoi(theParamValue.c_str());
	else if (theParamName == "-autoplaylevels")
		mAutoplayState->mLevelsPerGame = atoi(theParamValue.c_str());
	else if (theParamName == "-autoplaystart")
		sscanf(theParamValue.c_str(), "%d,%d", &mAutoplayState->mStartPyramid, &mAutoplayState->mStartChamber);
}

// the bot, once per update
void MummyApp::AutoplayUpdate()
{
	AutoplayState* s = mAutoplayState;
	if (s == NULL)
		return;
	if (s->mLog == NULL && !mAutoplayLog.empty())
		s->mLog = fopen(mAutoplayLog.c_str(), "w");
	s->mTick++;
	if (s->mTick >= mAutoplayCap)
	{
		Log(s, "cap reached at update %d", s->mTick);
		Shutdown();
		return;
	}
	if (s->mHeldKey != 0)
	{
		mWidgetManager->KeyUp((KeyCode) s->mHeldKey);
		s->mHeldKey = 0;
	}
	if (s->mWait > 0)
	{
		s->mWait--;
		return;
	}

	// the title: wait for the load, then click the play link
	if (mTitleScreen != NULL)
	{
		if (mTitleScreen->mPlayShown)
		{
			mTitleScreen->ButtonDepress(1);
			s->mWait = 20;
		}
		return;
	}
	// the dialogs, the newest first: each is photographed once (their sizes are checked by eye), the name for the high
	// score is typed, the options visit their high scores and credits, the others are answered with their first button
	if (GetDialogCount() > 0)
	{
		for (int anId = 31; anId >= 0; anId--)
		{
			Dialog* aDialog = GetDialog(anId);
			if (aDialog == NULL)
				continue;
			if (!mAutoplayShots.empty() && !(s->mDialogShots & (1u << anId)))
			{
				// seen: it is drawn and photographed once the next updates have passed
				if (!(s->mDialogSeen & (1u << anId)))
				{
					s->mDialogSeen |= 1u << anId;
					Log(s, "dialog %d at %d,%d size %d x %d, visible %d, board fade %.2f", anId, aDialog->mX, aDialog->mY, aDialog->mWidth,
						aDialog->mHeight, aDialog->mVisible ? 1 : 0, mBoard != NULL ? mBoard->mFade : -1.0);
					s->mWait = 200;
					return;
				}
				s->mDialogShots |= 1u << anId;
				SaveScreenshot(StrFormat("%s-dialog%d.png", mAutoplayShots.c_str(), anId));
				s->mWait = 3;
				return;
			}
			if (anId == DIALOG_OPTIONS && s->mOptionsStage == 1)
			{
				s->mOptionsStage = 2;
				((OptionsDialog*) aDialog)->ButtonDepress(2);
				s->mWait = 5;
				return;
			}
			if (anId == DIALOG_OPTIONS && s->mOptionsStage == 2)
			{
				s->mOptionsStage = 3;
				((OptionsDialog*) aDialog)->ButtonDepress(4);
				s->mWait = 5;
				return;
			}
			if (anId == DIALOG_RECORD_SCORE)
			{
				char aName[32];
				sprintf(aName, "Bot%d", s->mGames);
				((RecordScoreDialog*) aDialog)->mNameEdit->SetText(aName);
				Log(s, "high score: name %s", aName);
			}
			if (anId == DIALOG_CONTINUE)
				ButtonDepress(3000 + anId);
			else
				ButtonDepress(2000 + anId);
			s->mWait = 5;
			return;
		}
	}
	// the main menu: a classic game, or the end of the test
	if (mMainMenu != NULL)
	{
		if (mMainMenu->mScrollState != 1)
			return;
		if (s->mGames >= mAutoplayGames)
		{
			Log(s, "done after %d games", s->mGames);
			Shutdown();
			return;
		}
		s->mGames++;
		s->mLevels = 0;
		s->mGivenUp = false;
		s->mGameOverLogged = false;
		s->mPlanned = false;
		Log(s, "game %d: classic from pyramid %d chamber %d", s->mGames, s->mStartPyramid, s->mStartChamber);
		mMainMenu->ButtonDepress(0);
		mClassicPyramid = s->mStartPyramid;
		mClassicChamber = s->mStartChamber;
		s->mWait = 5;
		return;
	}
	// the high scores after Abandon Hope: look at them, then back to the menu
	if (mHighScores != NULL)
	{
		if (mHighScores->mFade >= 255.0 && mHighScoreBack == NULL && mHighScores->mNameAsked && !s->mGameOverLogged)
		{
			s->mGameOverLogged = true;
			Log(s, "game over: score %d, list %s %d, %s %d, %s %d, %s %d, %s %d", mScore,
				mHighScores->mNames[0].c_str(), mHighScores->mScores[0], mHighScores->mNames[1].c_str(), mHighScores->mScores[1],
				mHighScores->mNames[2].c_str(), mHighScores->mScores[2], mHighScores->mNames[3].c_str(), mHighScores->mScores[3],
				mHighScores->mNames[4].c_str(), mHighScores->mScores[4]);
			if (!mAutoplayShots.empty())
				SaveScreenshot(StrFormat("%s-gameover%d.png", mAutoplayShots.c_str(), s->mGames));
			mHighScores->ButtonDepress(1);
			s->mWait = 20;
		}
		return;
	}
	// a chamber is done: on to the next
	if (mLevelComplete != NULL)
	{
		if (!mAutoplayShots.empty() && s->mLevels == 0 && s->mShotWait == 0)
		{
			// the first chamber's screen is photographed once it has settled
			s->mShotWait = 1;
			s->mWait = 250;
			return;
		}
		if (s->mShotWait == 1)
		{
			s->mShotWait = 2;
			SaveScreenshot(StrFormat("%s-chamberdone%d.png", mAutoplayShots.c_str(), s->mGames));
		}
		Log(s, "chamber done: pyramid %d chamber %d, score %d", mClassicPyramid, mClassicChamber, mScore);
		s->mLevels++;
		s->mPlanned = false;
		mLevelComplete->ButtonDepress(0);
		s->mWait = 5;
		return;
	}
	// the panel of a lost explorer: try again, or give up
	if (mGamePanel != NULL)
	{
		if (mGamePanel->mScrollState != 1)
			return;
		s->mPlanned = false;
		if (s->mGivenUp)
		{
			Log(s, "abandon hope at pyramid %d chamber %d", mClassicPyramid, mClassicChamber);
			mGamePanel->ButtonDepress(3);
		}
		else
		{
			Log(s, "caught at pyramid %d chamber %d", mClassicPyramid, mClassicChamber);
			mGamePanel->ButtonDepress(1);
		}
		s->mWait = 5;
		return;
	}
	if (mBoard == NULL)
		return;

	Board* b = mBoard;
	if (b->mState != STATE_READY || b->mDeathTimer2 >= 0 || b->mFade2 < 1.0 || b->mShowingSolution)
		return;
	if (mGameMode != MODE_CLASSIC)
		return;

	// the options and the dialogs they lead to are photographed once
	if (!mAutoplayShots.empty() && s->mOptionsStage == 0 && mSidePanel != NULL)
	{
		s->mOptionsStage = 1;
		mSidePanel->ButtonDepress(12);
		s->mWait = 5;
		return;
	}

	// enough levels: give up, so that the solution plays and the high scores come
	if (s->mLevels >= s->mLevelsPerGame && !s->mGivenUp)
	{
		s->mGivenUp = true;
		ShowGamePanel();
		s->mWait = 5;
		return;
	}

	// a new level: the dice choose the way
	if (!s->mPlanned)
	{
		s->mPlanned = true;
		s->mUseSolver = s->mRandomMoves == 0 && s->Rand(10) < 7;
		s->mRandomMoves = s->mUseSolver ? 0 : 2 + s->Rand(6);
		if (s->mUseSolver)
		{
			b->Solve();
			s->mPath = b->mSolution;
			s->mPathIndex = 0;
			b->mShowingSolution = false;
			b->ClearSolution();
			Log(s, "level pyramid %d chamber %d (maze %d of file %d, %d x %d): solver, %d moves", mClassicPyramid,
				mClassicChamber, b->mMaze, b->mFile, b->mSize, b->mSize, (int) s->mPath.size());
			s->mWait = 30;
			return;
		}
		Log(s, "level pyramid %d chamber %d (maze %d of file %d, %d x %d): %d random moves", mClassicPyramid,
			mClassicChamber, b->mMaze, b->mFile, b->mSize, b->mSize, s->mRandomMoves);
	}

	// the next key
	Explorer* e = b->mExplorer;
	int aKey = 0;
	if (s->mUseSolver)
	{
		if (s->mPathIndex >= s->mPath.size())
		{
			// the way is walked; the board takes the explorer out by itself, or the plan failed: plan again
			s->mPlanned = false;
			return;
		}
		const ChaseState& next = s->mPath[s->mPathIndex++];
		int dx = next.mExplorerX - e->mX;
		int dy = next.mExplorerY - e->mY;
		if (dx < 0)
			aKey = KEYCODE_LEFT;
		else if (dx > 0)
			aKey = KEYCODE_RIGHT;
		else if (dy < 0)
			aKey = KEYCODE_UP;
		else if (dy > 0)
			aKey = KEYCODE_DOWN;
		else
			aKey = KEYCODE_SPACE;
	}
	else
	{
		static const int kKeys[5] = {KEYCODE_LEFT, KEYCODE_RIGHT, KEYCODE_UP, KEYCODE_DOWN, KEYCODE_SPACE};
		aKey = kKeys[s->Rand(5)];
		if (--s->mRandomMoves <= 0)
		{
			s->mRandomMoves = 0;
			s->mPlanned = false;
		}
		// the random walk ends in a solver level or an undo of the last move, by the dice
		if (s->mRandomMoves == 0 && s->Rand(2) == 0)
		{
			Log(s, "undo");
			b->Undo();
			s->mWait = 5;
			return;
		}
	}
	mWidgetManager->KeyDown((KeyCode) aKey);
	s->mHeldKey = aKey;
	s->mWait = 1;
}
