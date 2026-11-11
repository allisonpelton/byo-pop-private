// port: test hooks: the autoplay bot (-autoplay=<seed>), kept apart from the game code. It plays whole classic games through
// the keys the board reads and the buttons of the screens, so everything the game does for a player runs: the
// explorer walks, the mummies and the scorpion answer, chambers and pyramids come and go, the score and the high-score
// list are kept. The bot has its own random numbers (the game's use the C library generator, seeded by -seed), so the
// same -seed and -autoplay always make the same game. It writes a log of what happened (-autoplaylog=<file>) that
// tests compare.
//
//   -autoplay=<seed>         play with the bot (its own seed)
//   -autoplaymode=tutorial   play the tutorial instead of a classic game (every step by the table of allowed moves)
//   -autoplaygames=N         play N games, then quit
//   -autoplaylevels=N        levels played in a game before the bot gives up with Abandon Hope (default 3)
//   -autoplaystart=P,C       the first game starts at pyramid P (0 to 14), chamber C (0 to 14)
//   -autoplaysave=N          save the classic game N updates into its first board (before the clock starts), then quit
//   -autoplaycontinue        answer the Continue question with yes (a classic.sav in the user folder is loaded)
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

#define BOT_PYR ((mGameMode == MODE_ADVENTURE) ? mAdvPyramid : mClassicPyramid)
#define BOT_CHM ((mGameMode == MODE_ADVENTURE) ? mAdvChamber : mClassicChamber)

#include "GameLevels.h"
#include "Tables.inc"

namespace Sexy
{

struct AutoplayState
{
	unsigned int		mSeed;
	int					mTick;
	int					mLastAction;        // the update of the last log line or key
	int					mGames;             // games started
	int					mWait;              // updates to wait before the next action
	int					mLevels;            // levels finished in this game
	int					mLevelsPerGame;
	int					mStartPyramid, mStartChamber;
	int					mHeldKey;           // the key that is down, or 0
	int					mLastKey;           // the last key pressed
	unsigned int		mTutorialShots;     // the tutorial steps that were photographed
	bool				mTutorial;          // the tutorial is being played
	int					mSaveAt;            // save and quit when the board is this many updates old (0: never)
	int					mQuitAt;            // quit to the main menu through the side panel's button and its dialog when the board is this old (0: never); the game saves
	int					mQuitTick;          // the tick of that click (0: not yet)
	bool				mContinue;          // the Continue question is answered with yes
	bool				mAdventure;         // an adventure is being played
	bool				mMapShot;           // the world map was photographed
	bool				mDiverged;          // the board left the solver's path (logged once per plan)
	bool				mReplan;            // plan again from where the explorer stands
	int					mReplans;           // how often in this level
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
	bool				mTrace;             // -autoplaytrace: every change of the values the original is watched on
	unsigned int		mPrev[24];

	AutoplayState() : mSeed(1), mTick(0), mLastAction(0), mGames(0), mWait(0), mLevels(0), mLevelsPerGame(3), mStartPyramid(0),
		mStartChamber(0), mHeldKey(0), mLastKey(0), mTutorialShots(0), mTutorial(false), mSaveAt(0), mQuitAt(0), mQuitTick(0), mContinue(false), mAdventure(false), mMapShot(false), mDiverged(false), mReplan(false), mReplans(0), mPlanned(false), mUseSolver(true), mRandomMoves(0), mPathIndex(0),
		mGameOverLogged(false), mGivenUp(false), mOptionsStage(0), mDialogShots(0), mDialogSeen(0), mShotWait(0), mLog(NULL), mTrace(false)
	{
		for (int i = 0; i < 24; i++)
			mPrev[i] = 0xfffffffe;
	}

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
	s->mLastAction = s->mTick;
	if (s->mLog == NULL)
		return;
	va_list args;
	va_start(args, theFormat);
	vfprintf(s->mLog, theFormat, args);
	va_end(args);
	fputc('\n', s->mLog);
	fflush(s->mLog);
}

// the click the original is given for a button the bot depresses: its middle, in the window (identical.py's input)
void LogClick(AutoplayState* s, int theUpdate, Widget* theWidget, const char* theKind = "click")
{
	Point aPos = theWidget->GetAbsPos();
	Log(s, "  %s %d %d at app update %d", theKind, aPos.mX + theWidget->mWidth / 2, aPos.mY + theWidget->mHeight / 2, theUpdate);
}

}

void MMApp::HandleAutoplayParam(const std::string& theParamName, const std::string& theParamValue)
{
	if (mAutoplayState == NULL)
		mAutoplayState = new AutoplayState();
	if (theParamName == "-autoplay")
		mAutoplayState->mSeed = (unsigned int) atoi(theParamValue.c_str());
	else if (theParamName == "-autoplaylevels")
		mAutoplayState->mLevelsPerGame = atoi(theParamValue.c_str());
	else if (theParamName == "-autoplaysave")
		mAutoplayState->mSaveAt = atoi(theParamValue.c_str());
	else if (theParamName == "-autoplayquit")
		mAutoplayState->mQuitAt = atoi(theParamValue.c_str());
	else if (theParamName == "-autoplaytrace")
		mAutoplayState->mTrace = true;
	else if (theParamName == "-autoplaycontinue")
		mAutoplayState->mContinue = true;
	else if (theParamName == "-autoplaystart")
		sscanf(theParamValue.c_str(), "%d,%d", &mAutoplayState->mStartPyramid, &mAutoplayState->mStartChamber);
}

// the bot, once per update
void MMApp::AutoplayUpdate()
{
	AutoplayState* aState = mAutoplayState;
	if (aState == NULL)
		return;
	if (aState->mLog == NULL && !mAutoplayLog.empty())
		aState->mLog = fopen(mAutoplayLog.c_str(), "w");
	aState->mTick++;
	if (aState->mTrace && aState->mLog != NULL)
	{
		// the values the identical-play comparison reads beside the original's (origrun --watch of the same
		// expressions: the app's score, chamber and pyramid, the board's maze, file and state, the explorer, the
		// mummies and the scorpion by cell); a number is logged as the original's 32 bits, "-" for a missing object
		Board* aBoard = mBoard;
		struct { const char* mName; bool mValid; int mValue; } aValues[] = {
			{"board", true, aBoard != NULL ? 1 : 0},
			{"score", true, mScore},
			{"chamber", true, mClassicChamber},
			{"pyramid", true, mClassicPyramid},
			{"maze", aBoard != NULL, aBoard != NULL ? aBoard->mMaze : 0},
			{"file", aBoard != NULL, aBoard != NULL ? aBoard->mFile : 0},
			{"state", aBoard != NULL, aBoard != NULL ? (int) aBoard->mState : 0},
			{"death2", aBoard != NULL, aBoard != NULL ? aBoard->mStungTimer : 0},
			{"ex", aBoard != NULL && aBoard->mExplorer != NULL, aBoard != NULL && aBoard->mExplorer != NULL ? aBoard->mExplorer->mX : 0},
			{"ey", aBoard != NULL && aBoard->mExplorer != NULL, aBoard != NULL && aBoard->mExplorer != NULL ? aBoard->mExplorer->mY : 0},
			{"m0x", aBoard != NULL && aBoard->mMummies[0] != NULL, aBoard != NULL && aBoard->mMummies[0] != NULL ? aBoard->mMummies[0]->mX : 0},
			{"m0y", aBoard != NULL && aBoard->mMummies[0] != NULL, aBoard != NULL && aBoard->mMummies[0] != NULL ? aBoard->mMummies[0]->mY : 0},
			{"m1x", aBoard != NULL && aBoard->mMummies[1] != NULL, aBoard != NULL && aBoard->mMummies[1] != NULL ? aBoard->mMummies[1]->mX : 0},
			{"m1y", aBoard != NULL && aBoard->mMummies[1] != NULL, aBoard != NULL && aBoard->mMummies[1] != NULL ? aBoard->mMummies[1]->mY : 0},
			{"sx", aBoard != NULL && aBoard->mScorpion != NULL, aBoard != NULL && aBoard->mScorpion != NULL ? aBoard->mScorpion->mX : 0},
			{"sy", aBoard != NULL && aBoard->mScorpion != NULL, aBoard != NULL && aBoard->mScorpion != NULL ? aBoard->mScorpion->mY : 0},
			{"mode", true, mGameMode},
			{"tbase", aBoard != NULL, aBoard != NULL ? (int) aBoard->mTimeBase : 0},
			{"ebase", true, mElapsedBase},
			{"undo", aBoard != NULL, aBoard != NULL ? (int) aBoard->mUndo.size() : 0},
			{"rng", true, (int) CrtRandState()},
		};
		for (int i = 0; i < 21; i++)
		{
			unsigned int aNow = aValues[i].mValid ? (unsigned int) aValues[i].mValue : 0xfffffffdu;
			if (aNow == aState->mPrev[i])
				continue;
			if (aState->mPrev[i] == 0xfffffffe)
				aState->mPrev[i] = aNow == 0xfffffffdu ? 0xfffffffdu : 0;
			if (aNow == aState->mPrev[i])
				continue;
			if (aNow == 0xfffffffdu)
				fprintf(aState->mLog, "T %d %s -\n", mUpdateCount, aValues[i].mName);
			else
				fprintf(aState->mLog, "T %d %s %u\n", mUpdateCount, aValues[i].mName, aNow);
			aState->mPrev[i] = aNow;
		}
	}
	if (aState->mSaveAt > 0 && mBoard != NULL && mBoard->mUpdateCnt >= aState->mSaveAt)
	{
		Board* b = mBoard;
		Log(aState, "saved: %d updates in, time base %d, saved %d", (int) b->mUpdateCnt, (int) b->mTimeBase, (int) (b->mUpdateCnt - b->mTimeBase));
		b->SaveGame();
		Shutdown();
		return;
	}
	if (aState->mQuitAt > 0 && aState->mQuitTick == 0 && mBoard != NULL && mSidePanel != NULL && mBoard->mUpdateCnt >= aState->mQuitAt)
	{
		Log(aState, "quit to the main menu: %d updates in, time base %d", (int) mBoard->mUpdateCnt, (int) mBoard->mTimeBase);
		LogClick(aState, mUpdateCount, mSidePanel->mButtons[4]);
		mSidePanel->ButtonDepress(14);
		aState->mQuitTick = aState->mTick;
		aState->mWait = 5;	// the dialog is answered a few updates later, as a player would
	}
	if (aState->mQuitTick > 0 && aState->mTick >= aState->mQuitTick + 150)
	{
		Log(aState, "quit done at update %d", aState->mTick);
		Shutdown();
		return;
	}
	if (aState->mTick >= mAutoplayCap)
	{
		Log(aState, "cap reached at update %d", aState->mTick);
		Shutdown();
		return;
	}
	// a watchdog: when the bot has done nothing for a long time, say where the game stands and stop
	if (aState->mTick - aState->mLastAction > 30000)
	{
		Board* b = mBoard;
		if (b != NULL)
			Log(aState, "stalled: state %d, explorer %d,%d (target %d,%d, progress %.1f), mummies %d,%d and %d,%d, scorpion %d, death timers %d %d, "
				"fade2 %.1f, path %d of %d, dialogs %d, panel %d, complete %d", b->mState, b->mExplorer->mX, b->mExplorer->mY,
				b->mExplorer->mTargetX, b->mExplorer->mTargetY, b->mExplorer->mProgress,
				b->mMummies[0] ? b->mMummies[0]->mX : -9, b->mMummies[0] ? b->mMummies[0]->mY : -9,
				b->mMummies[1] ? b->mMummies[1]->mX : -9, b->mMummies[1] ? b->mMummies[1]->mY : -9, b->mScorpion != NULL ? 1 : 0,
				b->mTrapDeathTimer, b->mStungTimer, b->mRiseFade, (int) aState->mPathIndex, (int) aState->mPath.size(), GetDialogCount(),
				mGamePanel != NULL ? 1 : 0, mLevelComplete != NULL ? 1 : 0);
		else
			Log(aState, "stalled: no board");
		Shutdown();
		return;
	}
	if (aState->mHeldKey != 0)
	{
		mWidgetManager->KeyUp((KeyCode) aState->mHeldKey);
		aState->mHeldKey = 0;
	}
	if (aState->mWait > 0)
	{
		aState->mWait--;
		return;
	}

	// the title: wait for the load, then click the play link
	if (mTitleScreen != NULL)
	{
		if (mTitleScreen->mPlayShown)
		{
			mTitleScreen->ButtonDepress(1);
			aState->mWait = 20;
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
			if (!mAutoplayShots.empty() && !(aState->mDialogShots & (1u << anId)))
			{
				// seen: it is drawn and photographed once the next updates have passed
				if (!(aState->mDialogSeen & (1u << anId)))
				{
					aState->mDialogSeen |= 1u << anId;
					Log(aState, "dialog %d at %d,%d size %d x %d, visible %d, board fade %.2f", anId, aDialog->mX, aDialog->mY, aDialog->mWidth,
						aDialog->mHeight, aDialog->mVisible ? 1 : 0, mBoard != NULL ? mBoard->mFade : -1.0);
					aState->mWait = 200;
					return;
				}
				aState->mDialogShots |= 1u << anId;
				SaveScreenshot(StrFormat("%s-dialog%d.png", mAutoplayShots.c_str(), anId));
				aState->mWait = 3;
				return;
			}
			if (anId == DIALOG_OPTIONS && aState->mOptionsStage == 1)
			{
				aState->mOptionsStage = 2;
				((OptionsDialog*) aDialog)->ButtonDepress(2);
				aState->mWait = 5;
				return;
			}
			if (anId == DIALOG_OPTIONS && aState->mOptionsStage == 2)
			{
				aState->mOptionsStage = 3;
				((OptionsDialog*) aDialog)->ButtonDepress(4);
				aState->mWait = 5;
				return;
			}
			if (anId == DIALOG_RECORD_SCORE)
			{
				char aName[32];
				sprintf(aName, "Bot%d", aState->mGames);
				((RecordScoreDialog*) aDialog)->mNameEdit->SetText(aName);
				Log(aState, "high score: name %s", aName);
				Log(aState, "  text %s at app update %d", aName, mUpdateCount);
			}
			if (anId == DIALOG_NEW_ADVENTURE_NAME)
			{
				Log(aState, "  text Bot at app update %d", mUpdateCount);
				((NameDialog*) aDialog)->mNameEdit->SetText("Bot");
			}
			if (anId == DIALOG_ADVENTURE_NEW)
			{
				// the saved adventures: the first one goes on
				Log(aState, "adventure: continue slot 0");
				((AdventureDialog*) aDialog)->ButtonDepress(0);
			}
			else if (anId == DIALOG_CONTINUE)
			{
				if (aState->mContinue && aDialog->mYesButton != NULL)
					LogClick(aState, mUpdateCount, aDialog->mYesButton);
				ButtonDepress((aState->mContinue ? 2000 : 3000) + anId);
			}
			else
			{
				if (aDialog->mYesButton != NULL)
					LogClick(aState, mUpdateCount, aDialog->mYesButton);
				ButtonDepress(2000 + anId);
			}
			aState->mWait = 5;
			return;
		}
	}
	// the adventure's world map: the first pyramid that is open, then into it
	if (mWorldMap != NULL)
	{
		MapWidget* aMap = mWorldMap;
		if (aMap->mStep >= 0 || aMap->mCurrent != aMap->mTarget)
			return;
		int aPyramid = -1;
		for (int i = 0; i < 15 && aPyramid < 0; i++)
			if (aMap->mProgress[i] >= 0 && aMap->mProgress[i] < 15)
				aPyramid = i;
		if (aPyramid < 0 || aState->mLevels >= aState->mLevelsPerGame)
		{
			int aSolved = 0;
			for (int i = 0; i < 15; i++)
				if (aMap->mProgress[i] == 15)
					aSolved++;
			Log(aState, "adventure: %d chambers played, %d pyramids solved, caught %d times", aState->mLevels, aSolved, mDeaths);
			if (!mAutoplayShots.empty())
				SaveScreenshot(StrFormat("%s-map.png", mAutoplayShots.c_str()));
			Shutdown();
			return;
		}
		if (!mAutoplayShots.empty() && !aState->mMapShot)
		{
			aState->mMapShot = true;
			aState->mWait = 100;
			return;
		}
		Log(aState, "adventure: map, marker on pyramid %d, going to %d (progress %d)", aMap->mCurrent, aPyramid, aMap->mProgress[aPyramid]);
		aState->mPlanned = false;
		LogClick(aState, mUpdateCount, aMap->mPyramids[aPyramid]);
		aMap->ButtonDepress(aPyramid + 1);
		aState->mWait = 10;
		return;
	}
	// the main menu: a classic game, or the end of the test
	if (mMainMenu != NULL)
	{
		if (mMainMenu->mScrollState != 1)
			return;
		if (aState->mGames >= mAutoplayGames)
		{
			Log(aState, "done after %d games", aState->mGames);
			Shutdown();
			return;
		}
		aState->mGames++;
		if (mAutoplayMode == "tutorial")
		{
			Log(aState, "tutorial");
			aState->mTutorial = true;
			LogClick(aState, mUpdateCount, mMainMenu->mLinks[3], "menuclick");
			mMainMenu->ButtonDepress(3);
			aState->mWait = 5;
			return;
		}
		aState->mLevels = 0;
		aState->mGivenUp = false;
		aState->mGameOverLogged = false;
		aState->mPlanned = false;
		if (mAutoplayMode == "adventure")
		{
			Log(aState, "game %d: adventure", aState->mGames);
			aState->mAdventure = true;
			LogClick(aState, mUpdateCount, mMainMenu->mLinks[1], "menuclick");
			mMainMenu->ButtonDepress(1);
			aState->mWait = 5;
			return;
		}
		Log(aState, "game %d: classic from pyramid %d chamber %d", aState->mGames, aState->mStartPyramid, aState->mStartChamber);
		for (int i = 0; i < 4; i++)
			Log(aState, "  menu link %d at %d,%d size %d x %d", i, mMainMenu->mLinks[i]->mX, mMainMenu->mLinks[i]->mY, mMainMenu->mLinks[i]->mWidth, mMainMenu->mLinks[i]->mHeight);
		LogClick(aState, mUpdateCount, mMainMenu->mLinks[0], "menuclick");
		mMainMenu->ButtonDepress(0);
		mClassicPyramid = aState->mStartPyramid;
		mClassicChamber = aState->mStartChamber;
		aState->mWait = 5;
		return;
	}
	// the high scores after Abandon Hope: look at them, then back to the menu
	if (mHighScores != NULL)
	{
		if (mHighScores->mFade >= 255.0 && mHighScoreBack == NULL && mHighScores->mNameAsked && !aState->mGameOverLogged)
		{
			aState->mGameOverLogged = true;
			int aPlace = 0;	// where the bot's score stands in the list (0: not in it)
			for (int i = 4; i >= 0; i--)
				if (mHighScores->mScores[i] == mScore)
					aPlace = i + 1;
			Log(aState, "game over: score %d, list %d %d %d %d %d, the bot is number %d", mScore, mHighScores->mScores[0], mHighScores->mScores[1],
				mHighScores->mScores[2], mHighScores->mScores[3], mHighScores->mScores[4], aPlace);
			if (!mAutoplayShots.empty())
				SaveScreenshot(StrFormat("%s-gameover%d.png", mAutoplayShots.c_str(), aState->mGames));
			LogClick(aState, mUpdateCount, mHighScores->mMenuLink);
			mHighScores->ButtonDepress(1);
			aState->mWait = 20;
		}
		return;
	}
	// the tutorial finished: a classic game has begun
	if (aState->mTutorial && mGameMode == MODE_CLASSIC)
	{
		Log(aState, "tutorial done at update %d", aState->mTick);
		Shutdown();
		return;
	}
	// a chamber is done: on to the next
	if (mLevelComplete != NULL)
	{
		if (!mAutoplayShots.empty() && aState->mLevels == 0 && aState->mShotWait == 0)
		{
			// the first chamber's screen is photographed once it has settled
			aState->mShotWait = 1;
			aState->mWait = 250;
			return;
		}
		if (aState->mShotWait == 1)
		{
			aState->mShotWait = 2;
			SaveScreenshot(StrFormat("%s-chamberdone%d.png", mAutoplayShots.c_str(), aState->mGames));
		}
		Log(aState, "level time %d seconds, total %.0f", mLevelComplete->mSeconds, mTotalTime);
		if (mGameMode == MODE_ADVENTURE)
			Log(aState, "chamber done: pyramid %d chamber %d", mAdvPyramid, mAdvChamber);
		else
			Log(aState, "chamber done: pyramid %d chamber %d, score %d", BOT_PYR, BOT_CHM, mScore);
		aState->mLevels++;
		aState->mPlanned = false;
		LogClick(aState, mUpdateCount, mLevelComplete->mNextLink);
		mLevelComplete->ButtonDepress(0);
		aState->mWait = 5;
		return;
	}
	// the panel of a lost explorer: try again, or give up
	if (mGamePanel != NULL)
	{
		if (mGamePanel->mScrollState != 1)
			return;
		aState->mPlanned = false;
		if (aState->mGivenUp)
		{
			Log(aState, "abandon hope at pyramid %d chamber %d", BOT_PYR, BOT_CHM);
			LogClick(aState, mUpdateCount, mGamePanel->mLinks[3]);
			mGamePanel->ButtonDepress(3);
		}
		else
		{
			Log(aState, "caught at pyramid %d chamber %d", BOT_PYR, BOT_CHM);
			LogClick(aState, mUpdateCount, mGamePanel->mLinks[1]);
			mGamePanel->ButtonDepress(1);
		}
		aState->mWait = 5;
		return;
	}
	// the victory screen: look at it, then back to the menu
	if (mVictory != NULL)
	{
		if (mVictory->mFade >= 255.0 && mVictory->mNameAsked && !aState->mGameOverLogged)
		{
			aState->mGameOverLogged = true;
			Log(aState, "victory: score %d, caught %d times", mScore, mDeaths);
			if (!mAutoplayShots.empty())
				SaveScreenshot(StrFormat("%s-victory.png", mAutoplayShots.c_str()));
			LogClick(aState, mUpdateCount, mVictory->mMenuLink);
			mVictory->ButtonDepress(1);
			aState->mWait = 20;
		}
		return;
	}
	if (mBoard == NULL)
		return;

	// the tutorial: the move the table allows, or the button that is showing
	if (mGameMode >= MODE_TUTORIAL && mGameMode <= MODE_TUTORIAL_LAST)
	{
		Board* aBoard = mBoard;
		if (aBoard->mState != STATE_READY || aBoard->mStungTimer >= 0 || aBoard->mRiseFade < 1.0)
			return;
		for (int i = 0; i < 2; i++)
			if (aBoard->mMummies[i] != NULL && aBoard->mMummies[i]->mFighting)
				return;
		if (aBoard->mTutorialStepButton->mVisible || aBoard->mPlayNowButton->mVisible)
		{
			int anId = aBoard->mTutorialStepButton->mVisible ? aBoard->mTutorialStepButton->mId : aBoard->mPlayNowButton->mId;
			Log(aState, "tutorial part %d step %d: button %d", mGameMode, aBoard->mTutorialStep, anId);
			if (!mAutoplayShots.empty() && !(aState->mTutorialShots & (1 << aBoard->mTutorialStep % 24)) && aBoard->mTutorialStep % 10 == 2)
			{
				aState->mTutorialShots |= 1 << aBoard->mTutorialStep % 24;
				SaveScreenshot(StrFormat("%s-step%d.png", mAutoplayShots.c_str(), aBoard->mTutorialStep));
			}
			LogClick(aState, mUpdateCount, aBoard->mTutorialStepButton->mVisible ? (Widget*) aBoard->mTutorialStepButton : (Widget*) aBoard->mPlayNowButton);
			aBoard->ButtonDepress(anId);
			aState->mWait = 10;
			return;
		}
		int aMove = kTutorialMove[aBoard->mTutorialStep];
		static const int kKeys[5] = {KEYCODE_UP, KEYCODE_LEFT, KEYCODE_RIGHT, KEYCODE_DOWN, KEYCODE_SPACE};
		if (aMove < 0 || aMove > 4)
			return;
		Log(aState, "tutorial part %d step %d: move %d", mGameMode, aBoard->mTutorialStep, aMove);
		Log(aState, "  key %d at app update %d, board update %d, state %d", kKeys[aMove], mUpdateCount, aBoard->mUpdateCnt, (int) aBoard->mState);
		mWidgetManager->KeyDown((KeyCode) kKeys[aMove]);
		aState->mHeldKey = kKeys[aMove];
		aState->mWait = 1;
		return;
	}

	// the scene at the end of a pyramid: the explorer takes the treasure, then the link on to the next pyramid
	if (mGameMode == MODE_CLASSIC_DONE || mGameMode == MODE_ADVENTURE_DONE)
	{
		if (mBoard->mContinueLink != NULL && mBoard->mContinueLink->mVisible)
		{
			if (mGameMode == MODE_ADVENTURE_DONE)
				Log(aState, "pyramid %d solved, treasure scene", mAdvPyramid);
			else
				Log(aState, "pyramid %d done, score %d", mClassicPyramid, mScore);
			if (!mAutoplayShots.empty() && mClassicPyramid == 0)
				SaveScreenshot(StrFormat("%s-pyramid0.png", mAutoplayShots.c_str()));
			aState->mPlanned = false;
			LogClick(aState, mUpdateCount, mBoard->mContinueLink);
			mBoard->ButtonDepress(3);
			aState->mWait = 20;
		}
		return;
	}

	Board* aBoard = mBoard;
	if (aBoard->mState != STATE_READY || aBoard->mStungTimer >= 0 || aBoard->mRiseFade < 1.0 || aBoard->mShowingSolution)
		return;
	// a mummy that has just won a fight with the other one dances first, and the explorer waits for it
	for (int i = 0; i < 2; i++)
		if (aBoard->mMummies[i] != NULL && aBoard->mMummies[i]->mFighting)
			return;
	if (mGameMode != MODE_CLASSIC && mGameMode != MODE_ADVENTURE)
		return;

	// the options and the dialogs they lead to are photographed once
	if (!mAutoplayShots.empty() && aState->mOptionsStage == 0 && mSidePanel != NULL)
	{
		aState->mOptionsStage = 1;
		mSidePanel->ButtonDepress(12);
		aState->mWait = 5;
		return;
	}

	// enough levels: give up. The explorer walks at random until a mummy catches it, and then Abandon Hope is clicked: the
	// way a player ends a classic game, with input that the original can be given
	if (aState->mLevels >= aState->mLevelsPerGame && !aState->mGivenUp && mGameMode == MODE_CLASSIC)
	{
		aState->mGivenUp = true;
		aState->mPlanned = false;
	}

	// a new level: the dice choose the way
	if (!aState->mPlanned)
	{
		aState->mPlanned = true;
		aState->mDiverged = false;
		aState->mReplans = 0;
		aState->mUseSolver = aState->mRandomMoves == 0 && aState->Rand(10) < 7;
		aState->mRandomMoves = aState->mUseSolver ? 0 : 2 + aState->Rand(6);
		if (aState->mGivenUp)
		{
			aState->mUseSolver = false;
			aState->mRandomMoves = 1000000;
		}
		if (aState->mUseSolver)
		{
			// DoSolution takes every move back first (ResetMaze): the original is given the click on RESET MAZE
			LogClick(aState, mUpdateCount, mSidePanel->mButtons[1]);
			aBoard->DoSolution();
			aState->mPath = aBoard->mSolution;
			aState->mPathIndex = 0;
			aBoard->mShowingSolution = false;
			aBoard->ClearSolutionList();
			Log(aState, "level pyramid %d chamber %d (maze %d of file %d, %d x %d): solver, %d moves", BOT_PYR,
				BOT_CHM, aBoard->mMaze, aBoard->mFile, aBoard->mSize, aBoard->mSize, (int) aState->mPath.size());
			aState->mWait = 30;
			return;
		}
		Log(aState, "level pyramid %d chamber %d (maze %d of file %d, %d x %d): %d random moves", BOT_PYR,
			BOT_CHM, aBoard->mMaze, aBoard->mFile, aBoard->mSize, aBoard->mSize, aState->mRandomMoves);
	}

	// the next key
	Player* anExplorer = aBoard->mExplorer;
	int aKey = 0;
	if (aState->mUseSolver)
	{
		// the board must be where the solver said it would be after the last move: a difference is a bug in one of them
		if (aState->mPathIndex > 0 && aState->mPathIndex <= aState->mPath.size() && !aState->mDiverged)
		{
			ChaseState aWant = aState->mPath[aState->mPathIndex - 1];
			ChaseState aHave = aBoard->GetChaseState();
			// (where a dead mummy or scorpion lies does not matter)
			for (int i = 0; i < 2; i++)
			{
				if (!aWant.mMummyAlive[i])
					aWant.mMummyX[i] = aWant.mMummyY[i] = 0;
				if (!aHave.mMummyAlive[i])
					aHave.mMummyX[i] = aHave.mMummyY[i] = 0;
			}
			if (!aWant.mScorpionAlive)
				aWant.mScorpionX = aWant.mScorpionY = 0;
			if (!aHave.mScorpionAlive)
				aHave.mScorpionX = aHave.mScorpionY = 0;
			if (mAutoplayMode == "stopondiverge")
				Log(aState, "  step %d: explorer %d,%d (want %d,%d) mummy %d,%d (want %d,%d) gate %d (want %d); key at %d,%d gate at %d,%d, traps %d,%d",
					(int) aState->mPathIndex, aHave.mExplorerX, aHave.mExplorerY, aWant.mExplorerX, aWant.mExplorerY,
					aHave.mMummyX[0], aHave.mMummyY[0], aWant.mMummyX[0], aWant.mMummyY[0], aHave.mGateClosed ? 1 : 0,
					aWant.mGateClosed ? 1 : 0, aBoard->mKeyX, aBoard->mKeyY, aBoard->mGateX, aBoard->mGateY, aBoard->mTrapX[0], aBoard->mTrapY[0]);
			if (!(aWant == aHave))
			{
				aState->mDiverged = true;
				aState->mReplan = true;
				Log(aState, "DIVERGED after move %d: want explorer %d,%d mummies %d,%d%s %d,%d%s scorpion %d,%d%s gate %d; "
					"have explorer %d,%d mummies %d,%d%s %d,%d%s scorpion %d,%d%s gate %d", (int) aState->mPathIndex,
					aWant.mExplorerX, aWant.mExplorerY, aWant.mMummyX[0], aWant.mMummyY[0], aWant.mMummyAlive[0] ? "" : "(dead)",
					aWant.mMummyX[1], aWant.mMummyY[1], aWant.mMummyAlive[1] ? "" : "(dead)", aWant.mScorpionX, aWant.mScorpionY,
					aWant.mScorpionAlive ? "" : "(dead)", aWant.mGateClosed ? 1 : 0,
					aHave.mExplorerX, aHave.mExplorerY, aHave.mMummyX[0], aHave.mMummyY[0], aHave.mMummyAlive[0] ? "" : "(dead)",
					aHave.mMummyX[1], aHave.mMummyY[1], aHave.mMummyAlive[1] ? "" : "(dead)", aHave.mScorpionX, aHave.mScorpionY,
					aHave.mScorpionAlive ? "" : "(dead)", aHave.mGateClosed ? 1 : 0);
				if ((mAutoplayMode == "stopondiverge"))
					Shutdown();
			}
		}
		// the turn did not go as the search foresaw (the original's search leaves out a few things, and the board then
		// does them): the way out from where the explorer stands now
		if (aState->mReplan)
		{
			aState->mReplan = false;
			std::vector<ChaseState> aPath;
			if (++aState->mReplans <= 8 && aBoard->FindWay(aPath) && !aPath.empty())
			{
				aState->mPath = aPath;
				aState->mPathIndex = 0;
				aState->mDiverged = false;
				Log(aState, "replanned from the explorer's place: %d moves", (int) aPath.size());
			}
			else
				aState->mUseSolver = false, aState->mRandomMoves = 3;
			aState->mWait = 5;
			return;
		}
		if (aState->mPathIndex >= aState->mPath.size())
		{
			// the way is walked; the board takes the explorer out by itself, or the plan failed: plan again
			aState->mPlanned = false;
			return;
		}
		const ChaseState& next = aState->mPath[aState->mPathIndex++];
		int dx = next.mExplorerX - anExplorer->mX;
		int dy = next.mExplorerY - anExplorer->mY;
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
		aKey = kKeys[aState->Rand(5)];
		if (--aState->mRandomMoves <= 0)
		{
			aState->mRandomMoves = 0;
			aState->mPlanned = false;
		}
		// the random walk ends in a solver level or an undo of the last move, by the dice
		if (aState->mRandomMoves == 0 && !aState->mGivenUp && aState->Rand(2) == 0)
		{
			Log(aState, "undo");
			LogClick(aState, mUpdateCount, mSidePanel->mButtons[0]);
			aBoard->UndoMove();
			aState->mWait = 5;
			return;
		}
	}
	Log(aState, "  key %d at app update %d, board update %d, state %d", aKey, mUpdateCount, aBoard->mUpdateCnt, (int) aBoard->mState);
	mWidgetManager->KeyDown((KeyCode) aKey);
	aState->mLastKey = aKey;
	aState->mLastAction = aState->mTick;
	aState->mHeldKey = aKey;
	aState->mWait = 1;
}
