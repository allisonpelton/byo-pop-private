// Test hooks: the autoplay bot (-autoplay=<seed>). It plays puzzles the way a player would: it makes a player, presses
// play, walks from the island to a temple and into the first puzzle that is not solved, throws and pulls the tiles of the
// best known solution (levels/maxedscores.txt) through the board's own handlers, and presses the tablet's button when the
// puzzle is done. So everything the game does for a player runs: the screens come and go, the results go into the
// player's record and into the table of the best solutions, temples open. The bot has its own random numbers (the
// game's use the C library generator, seeded by -seed); the same -seed and -autoplay always make the same run. It writes
// a log of what happened (-autoplaylog=<file>) that tests compare.
//
//   -autoplay=<seed>         play with the bot (its own seed)
//   -autoplaypuzzles=N       solve N puzzles, then quit (default 3)
//   -autoplaycap=N           quit after N updates in any case
//   -autoplaylog=<file>      the log
//   -autoplayshots=<prefix>  screenshots of each screen the bot comes to

#include "PixelusApp.h"
#include "Board.h"
#include "IslandMap.h"
#include "TempleScreen.h"
#include "PuzzleCompleted.h"
#include "StoryScreen.h"
#include "MainMenu.h"
#include "LoadingScreen.h"
#include "Temples.h"
#include "LevelData.h"
#include "ProgressManager.h"
#include "UserDirectory.h"
#include "UserDialogs.h"
#include "Grid.h"
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
	int				mWait;				// updates to wait before the next action
	int				mSolved;			// puzzles finished by the bot
	int				mMoveIndex;			// the next move of the solution
	int				mScreenTicks;		// updates on the screen the bot is waiting on
	std::vector<Grid::Move>	mSolution;
	std::string		mUuid;
	unsigned int	mScreens;			// bit per kind of screen: its screenshot was taken
	FILE*			mLog;

	AutoplayState() : mSeed(1), mTick(0), mWait(0), mSolved(0), mMoveIndex(-1), mScreenTicks(0), mScreens(0), mLog(NULL) {}
};

}

void PixelusApp::HandleCmdLineParam(const std::string& theParamName, const std::string& theParamValue)
{
	if (theParamName == "-autoplay")
	{
		mAutoplay = true;
		mAutoplayState = new AutoplayState();
		mAutoplayState->mSeed = (unsigned int) atoi(theParamValue.c_str());
		mScriptedInput = true;   // the bot's input only, as in a -script run
	}
	else if (theParamName == "-autoplaypuzzles")
		mAutoplayPuzzles = atoi(theParamValue.c_str());
	else if (theParamName == "-autoplaycap")
		mAutoplayCap = atoi(theParamValue.c_str());
	else if (theParamName == "-autoplaylog")
		mAutoplayLog = theParamValue;
	else if (theParamName == "-autoplayshots")
		mAutoplayShots = theParamValue;
	else
		NcApp::HandleCmdLineParam(theParamName, theParamValue);
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

// a screenshot, the first time a kind of screen comes up
void Shot(PixelusApp* theApp, AutoplayState* s, int theKind, const char* theName)
{
	if (theApp->mAutoplayShots.empty() || (s->mScreens & (1u << theKind)))
		return;
	s->mScreens |= 1u << theKind;
	theApp->AutoplayShot(theApp->mAutoplayShots + "-" + theName + ".png");
}

enum { SHOT_MENU, SHOT_ISLAND, SHOT_TEMPLE, SHOT_BOARD, SHOT_BOARD_END, SHOT_COMPLETED, SHOT_COMPLETED_2 };

// the first temple that is open and has a puzzle without a result; -1 when there is none
int FindTemple(PixelusApp* theApp, UserProfile* theUser)
{
	for (int i = 0; i < NUM_TEMPLES; i++)
	{
		if (IsTempleLocked(theApp, i) || !IsTempleOpen(theApp, i))
			continue;
		if (theUser->CountSolved(gTemples[i].mName) < theApp->mLevelData->GetPuzzleCount(gTemples[i].mName))
			return i;
	}
	return -1;
}

// the first puzzle of a temple without a result (the tutorial's come in order, the mystery one is the last)
int FindPuzzle(PixelusApp* theApp, UserProfile* theUser, const std::string& theTemple)
{
	const LevelData::Level* aLevel = theApp->mLevelData->GetLevel(theTemple);
	if (aLevel == NULL)
		return -1;
	for (size_t i = 0; i < aLevel->mPuzzles.size(); i++)
		if (theUser->GetRecord(aLevel->mPuzzles[i].mUuid) == NULL)
			return (int) i;
	return -1;
}

}

// the bot, once per update
void PixelusApp::AutoplayUpdate()
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
	if (s->mWait > 0)
	{
		s->mWait--;
		return;
	}
	// the loading screen: wait until the bar is full, then press its play link
	if (mLoadingScreen != NULL)
	{
		if (mLoadingScreen->mLoadDone && ++s->mScreenTicks > 150)
		{
			s->mScreenTicks = 0;
			PlayClicked();
			s->mWait = 20;
		}
		return;
	}

	// the dialogs: the one for a new player gets a name, the others their first button
	if (GetDialog(0x13) != NULL)
	{
		((NewUserDialog*) GetDialog(0x13))->SetName("Bot");
		ButtonDepress(2019);
		s->mWait = 10;
		return;
	}
	if (GetDialogCount() > 0)
	{
		for (int anId = 0; anId < 64; anId++)
			if (GetDialog(anId) != NULL)
			{
				ButtonDepress(2000 + anId);
				break;
			}
		s->mWait = 10;
		return;
	}

	UserProfile* aUser = mUserDirectory != NULL ? mUserDirectory->GetCurrent() : NULL;
	if (mMainMenu != NULL)
	{
		if (aUser == NULL)
			return;
		if (++s->mScreenTicks < 150)
			return;
		Shot(this, s, SHOT_MENU, "menu");
		Log(s, "play");
		s->mScreenTicks = 0;
		PlayGame();
		s->mWait = 20;
		return;
	}
	if (mStoryScreen != NULL)
	{
		// every speech has come in by now: photograph, then press the button
		s->mScreenTicks++;
		if (s->mScreenTicks == 700 && !mAutoplayShots.empty())
		{
			char aName[32];
			snprintf(aName, sizeof(aName), "-story%d.png", s->mSolved);
			AutoplayShot(mAutoplayShots + aName);
		}
		if (s->mScreenTicks < 750)
			return;
		s->mScreenTicks = 0;
		Log(s, "story with %d speeches", (int) mStoryScreen->mSpeeches.size());
		mStoryScreen->ButtonDepress(StoryScreen::ID_CONTINUE);
		s->mWait = 20;
		return;
	}

	if (mPuzzleCompleted != NULL)
	{
		s->mScreenTicks++;
		if (s->mScreenTicks == 150 && !mAutoplayShots.empty() && (s->mSolved == 0 || s->mSolved == 8 || s->mSolved == 9 || s->mSolved == 10 || s->mSolved == 11))
		{
			char aName[32];
			snprintf(aName, sizeof(aName), "-completed%d.png", s->mSolved);
			AutoplayShot(mAutoplayShots + aName);
		}
		if (s->mScreenTicks < 220)
			return;
		s->mScreenTicks = 0;
		s->mSolved++;
		Log(s, "completed %s moves %d time %d record %d maxed %d opened %d temple %d tutorial %d", s->mUuid.c_str(),
			mPuzzleCompleted->mMoves, mPuzzleCompleted->mTime, mPuzzleCompleted->mWorldRecord, mPuzzleCompleted->mWasMaxed,
			mPuzzleCompleted->mOpenedTemple, mPuzzleCompleted->mTempleDone, mPuzzleCompleted->mTutorialDone);
		mPuzzleCompleted->ButtonDepress(PuzzleCompletedScreen::ID_CONTINUE);
		s->mMoveIndex = -1;
		s->mWait = 20;
		return;
	}

	if (mBoard != NULL)
	{
		Board* b = mBoard;
		if (s->mMoveIndex < 0)
		{
			// the board has just come: read the best known solution
			s->mUuid = b->mUuid;
			s->mSolution.clear();
			const ProgressManager::Best* aBest = mProgress->FindBest(b->mUuid);
			if (aBest != NULL)
				for (size_t i = 0; i + 6 <= aBest->mSolution.size(); i += 6)
				{
					Grid::Move aMove;
					if (Grid::ParseMove(aBest->mSolution.substr(i, 6), &aMove))
						s->mSolution.push_back(aMove);
				}
			Log(s, "board %s %s, solution of %d moves", b->mLevelName.c_str(), b->mUuid.c_str(), (int) s->mSolution.size());
			s->mMoveIndex = 0;
			s->mWait = 120;
			Shot(this, s, SHOT_BOARD, "board");
			return;
		}
		if (b->mGrid->mDone)
			return;
		if (s->mMoveIndex >= (int) s->mSolution.size())
		{
			// out of moves and not done: the log says so and the run stops
			Log(s, "solution ended without a finished puzzle (%d moves)", b->mGrid->mMoveCount);
			Shutdown();
			return;
		}
		const Grid::Move& aMove = s->mSolution[s->mMoveIndex++];
		b->Aim(aMove.mSide, aMove.mCursor);
		if (aMove.mRemove)
			b->DoPull();
		else
			b->DoThrow();
		if (s->mMoveIndex == (int) s->mSolution.size() - 1)
			Shot(this, s, SHOT_BOARD_END, "board-end");
		s->mWait = 4;
		return;
	}

	if (mTempleScreen != NULL)
	{
		if (++s->mScreenTicks < 120)
			return;
		Shot(this, s, SHOT_TEMPLE, "temple");
		s->mScreenTicks = 0;
		if (s->mSolved >= mAutoplayPuzzles)
		{
			Log(s, "done: %d puzzles", s->mSolved);
			Shutdown();
			return;
		}
		int aPuzzle = FindPuzzle(this, aUser, mLevelData->mCurrentLevel);
		if (aPuzzle < 0)
		{
			Log(s, "temple %s done", mLevelData->mCurrentLevel.c_str());
			BackToIsland();
			s->mWait = 20;
			return;
		}
		mLevelData->mCurrentPuzzle = aPuzzle;
		s->mMoveIndex = -1;
		StartPuzzle(true);
		s->mWait = 20;
		return;
	}

	if (mIslandMap != NULL)
	{
		if (++s->mScreenTicks < 150)
			return;
		Shot(this, s, SHOT_ISLAND, "island");
		s->mScreenTicks = 0;
		if (s->mSolved >= mAutoplayPuzzles)
		{
			Log(s, "done: %d puzzles", s->mSolved);
			Shutdown();
			return;
		}
		int aTemple = FindTemple(this, aUser);
		if (aTemple < 0)
		{
			Log(s, "done: no temple left");
			Shutdown();
			return;
		}
		Log(s, "temple %s", gTemples[aTemple].mName);
		EnterTemple(aTemple);
		s->mWait = 20;
		return;
	}
}
