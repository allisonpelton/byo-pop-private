// Test hooks: the autoplay bot (-autoplay=<seed>), kept apart from the game code. It plays whole classic games through
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
		mStartChamber(0), mHeldKey(0), mLastKey(0), mTutorialShots(0), mTutorial(false), mSaveAt(0), mContinue(false), mAdventure(false), mMapShot(false), mDiverged(false), mReplan(false), mReplans(0), mPlanned(false), mUseSolver(true), mRandomMoves(0), mPathIndex(0),
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

void MummyApp::HandleAutoplayParam(const std::string& theParamName, const std::string& theParamValue)
{
	if (mAutoplayState == NULL)
		mAutoplayState = new AutoplayState();
	if (theParamName == "-autoplay")
		mAutoplayState->mSeed = (unsigned int) atoi(theParamValue.c_str());
	else if (theParamName == "-autoplaylevels")
		mAutoplayState->mLevelsPerGame = atoi(theParamValue.c_str());
	else if (theParamName == "-autoplaysave")
		mAutoplayState->mSaveAt = atoi(theParamValue.c_str());
	else if (theParamName == "-autoplaytrace")
		mAutoplayState->mTrace = true;
	else if (theParamName == "-autoplaycontinue")
		mAutoplayState->mContinue = true;
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
	if (s->mTrace && s->mLog != NULL)
	{
		// the values phase one of the identical-play pass compares with the original's (origrun --watch of the same
		// expressions: the app's score, chamber and pyramid, the board's maze, file and state, the explorer, the
		// mummies and the scorpion by cell); a number is logged as the original's 32 bits, "-" for a missing object
		Board* b = mBoard;
		struct { const char* mName; bool mValid; int mValue; } aValues[] = {
			{"board", true, b != NULL ? 1 : 0},
			{"score", true, mScore},
			{"chamber", true, mClassicChamber},
			{"pyramid", true, mClassicPyramid},
			{"maze", b != NULL, b != NULL ? b->mMaze : 0},
			{"file", b != NULL, b != NULL ? b->mFile : 0},
			{"state", b != NULL, b != NULL ? (int) b->mState : 0},
			{"death2", b != NULL, b != NULL ? b->mDeathTimer2 : 0},
			{"ex", b != NULL && b->mExplorer != NULL, b != NULL && b->mExplorer != NULL ? b->mExplorer->mX : 0},
			{"ey", b != NULL && b->mExplorer != NULL, b != NULL && b->mExplorer != NULL ? b->mExplorer->mY : 0},
			{"m0x", b != NULL && b->mMummies[0] != NULL, b != NULL && b->mMummies[0] != NULL ? b->mMummies[0]->mX : 0},
			{"m0y", b != NULL && b->mMummies[0] != NULL, b != NULL && b->mMummies[0] != NULL ? b->mMummies[0]->mY : 0},
			{"m1x", b != NULL && b->mMummies[1] != NULL, b != NULL && b->mMummies[1] != NULL ? b->mMummies[1]->mX : 0},
			{"m1y", b != NULL && b->mMummies[1] != NULL, b != NULL && b->mMummies[1] != NULL ? b->mMummies[1]->mY : 0},
			{"sx", b != NULL && b->mScorpion != NULL, b != NULL && b->mScorpion != NULL ? b->mScorpion->mX : 0},
			{"sy", b != NULL && b->mScorpion != NULL, b != NULL && b->mScorpion != NULL ? b->mScorpion->mY : 0},
			{"mode", true, mGameMode},
			{"tbase", b != NULL, b != NULL ? (int) b->mTimeBase : 0},
			{"ebase", true, mElapsedBase},
			{"undo", b != NULL, b != NULL ? (int) b->mUndo.size() : 0},
			{"rng", true, (int) CrtRandState()},
		};
		for (int i = 0; i < 21; i++)
		{
			unsigned int aNow = aValues[i].mValid ? (unsigned int) aValues[i].mValue : 0xfffffffdu;
			if (aNow == s->mPrev[i])
				continue;
			if (s->mPrev[i] == 0xfffffffe)
				s->mPrev[i] = aNow == 0xfffffffdu ? 0xfffffffdu : 0;
			if (aNow == s->mPrev[i])
				continue;
			if (aNow == 0xfffffffdu)
				fprintf(s->mLog, "T %d %s -\n", mUpdateCount, aValues[i].mName);
			else
				fprintf(s->mLog, "T %d %s %u\n", mUpdateCount, aValues[i].mName, aNow);
			s->mPrev[i] = aNow;
		}
	}
	if (s->mSaveAt > 0 && mBoard != NULL && mBoard->mUpdateCnt >= s->mSaveAt)
	{
		Board* b = mBoard;
		Log(s, "saved: %d updates in, time base %d, saved %d", (int) b->mUpdateCnt, (int) b->mTimeBase, (int) (b->mUpdateCnt - b->mTimeBase));
		b->SaveClassic();
		Shutdown();
		return;
	}
	if (s->mTick >= mAutoplayCap)
	{
		Log(s, "cap reached at update %d", s->mTick);
		Shutdown();
		return;
	}
	// a watchdog: when the bot has done nothing for a long time, say where the game stands and stop
	if (s->mTick - s->mLastAction > 30000)
	{
		Board* b = mBoard;
		if (b != NULL)
			Log(s, "stalled: state %d, explorer %d,%d (target %d,%d, progress %.1f), mummies %d,%d and %d,%d, scorpion %d, death timers %d %d, "
				"fade2 %.1f, path %d of %d, dialogs %d, panel %d, complete %d", b->mState, b->mExplorer->mX, b->mExplorer->mY,
				b->mExplorer->mTargetX, b->mExplorer->mTargetY, b->mExplorer->mProgress,
				b->mMummies[0] ? b->mMummies[0]->mX : -9, b->mMummies[0] ? b->mMummies[0]->mY : -9,
				b->mMummies[1] ? b->mMummies[1]->mX : -9, b->mMummies[1] ? b->mMummies[1]->mY : -9, b->mScorpion != NULL ? 1 : 0,
				b->mDeathTimer, b->mDeathTimer2, b->mFade2, (int) s->mPathIndex, (int) s->mPath.size(), GetDialogCount(),
				mGamePanel != NULL ? 1 : 0, mLevelComplete != NULL ? 1 : 0);
		else
			Log(s, "stalled: no board");
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
				Log(s, "  text %s at app update %d", aName, mUpdateCount);
			}
			if (anId == DIALOG_NEW_ADVENTURE_NAME)
			{
				Log(s, "  text Bot at app update %d", mUpdateCount);
				((NewAdventureDialog*) aDialog)->mNameEdit->SetText("Bot");
			}
			if (anId == DIALOG_ADVENTURE_NEW)
			{
				// the saved adventures: the first one goes on
				Log(s, "adventure: continue slot 0");
				((AdventureSlotsDialog*) aDialog)->ButtonDepress(0);
			}
			else if (anId == DIALOG_CONTINUE)
				ButtonDepress((s->mContinue ? 2000 : 3000) + anId);
			else
			{
				if (aDialog->mYesButton != NULL)
					LogClick(s, mUpdateCount, aDialog->mYesButton);
				ButtonDepress(2000 + anId);
			}
			s->mWait = 5;
			return;
		}
	}
	// the adventure's world map: the first pyramid that is open, then into it
	if (mWorldMap != NULL)
	{
		WorldMap* m = mWorldMap;
		if (m->mStep >= 0 || m->mCurrent != m->mTarget)
			return;
		int aPyramid = -1;
		for (int i = 0; i < 15 && aPyramid < 0; i++)
			if (m->mProgress[i] >= 0 && m->mProgress[i] < 15)
				aPyramid = i;
		if (aPyramid < 0 || s->mLevels >= s->mLevelsPerGame)
		{
			int aSolved = 0;
			for (int i = 0; i < 15; i++)
				if (m->mProgress[i] == 15)
					aSolved++;
			Log(s, "adventure: %d chambers played, %d pyramids solved, caught %d times", s->mLevels, aSolved, mDeaths);
			if (!mAutoplayShots.empty())
				SaveScreenshot(StrFormat("%s-map.png", mAutoplayShots.c_str()));
			Shutdown();
			return;
		}
		if (!mAutoplayShots.empty() && !s->mMapShot)
		{
			s->mMapShot = true;
			s->mWait = 100;
			return;
		}
		Log(s, "adventure: map, marker on pyramid %d, going to %d (progress %d)", m->mCurrent, aPyramid, m->mProgress[aPyramid]);
		s->mPlanned = false;
		LogClick(s, mUpdateCount, m->mPyramids[aPyramid]);
		m->ButtonDepress(aPyramid + 1);
		s->mWait = 10;
		return;
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
		if (mAutoplayMode == "tutorial")
		{
			Log(s, "tutorial");
			s->mTutorial = true;
			LogClick(s, mUpdateCount, mMainMenu->mLinks[3], "menuclick");
			mMainMenu->ButtonDepress(3);
			s->mWait = 5;
			return;
		}
		s->mLevels = 0;
		s->mGivenUp = false;
		s->mGameOverLogged = false;
		s->mPlanned = false;
		if (mAutoplayMode == "adventure")
		{
			Log(s, "game %d: adventure", s->mGames);
			s->mAdventure = true;
			LogClick(s, mUpdateCount, mMainMenu->mLinks[1], "menuclick");
			mMainMenu->ButtonDepress(1);
			s->mWait = 5;
			return;
		}
		Log(s, "game %d: classic from pyramid %d chamber %d", s->mGames, s->mStartPyramid, s->mStartChamber);
		for (int i = 0; i < 4; i++)
			Log(s, "  menu link %d at %d,%d size %d x %d", i, mMainMenu->mLinks[i]->mX, mMainMenu->mLinks[i]->mY, mMainMenu->mLinks[i]->mWidth, mMainMenu->mLinks[i]->mHeight);
		LogClick(s, mUpdateCount, mMainMenu->mLinks[0], "menuclick");
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
			int aPlace = 0;	// where the bot's score stands in the list (0: not in it)
			for (int i = 4; i >= 0; i--)
				if (mHighScores->mScores[i] == mScore)
					aPlace = i + 1;
			Log(s, "game over: score %d, list %d %d %d %d %d, the bot is number %d", mScore, mHighScores->mScores[0], mHighScores->mScores[1],
				mHighScores->mScores[2], mHighScores->mScores[3], mHighScores->mScores[4], aPlace);
			if (!mAutoplayShots.empty())
				SaveScreenshot(StrFormat("%s-gameover%d.png", mAutoplayShots.c_str(), s->mGames));
			mHighScores->ButtonDepress(1);
			s->mWait = 20;
		}
		return;
	}
	// the tutorial finished: a classic game has begun
	if (s->mTutorial && mGameMode == MODE_CLASSIC)
	{
		Log(s, "tutorial done at update %d", s->mTick);
		Shutdown();
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
		Log(s, "level time %d seconds, total %.0f", mLevelComplete->mSeconds, mTotalTime);
		if (mGameMode == MODE_ADVENTURE)
			Log(s, "chamber done: pyramid %d chamber %d", mAdvPyramid, mAdvChamber);
		else
			Log(s, "chamber done: pyramid %d chamber %d, score %d", BOT_PYR, BOT_CHM, mScore);
		s->mLevels++;
		s->mPlanned = false;
		LogClick(s, mUpdateCount, mLevelComplete->mNextLink);
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
			Log(s, "abandon hope at pyramid %d chamber %d", BOT_PYR, BOT_CHM);
			LogClick(s, mUpdateCount, mGamePanel->mLinks[3]);
			mGamePanel->ButtonDepress(3);
		}
		else
		{
			Log(s, "caught at pyramid %d chamber %d", BOT_PYR, BOT_CHM);
			LogClick(s, mUpdateCount, mGamePanel->mLinks[1]);
			mGamePanel->ButtonDepress(1);
		}
		s->mWait = 5;
		return;
	}
	// the victory screen: look at it, then back to the menu
	if (mVictory != NULL)
	{
		if (mVictory->mFade >= 255.0 && mVictory->mNameAsked && !s->mGameOverLogged)
		{
			s->mGameOverLogged = true;
			Log(s, "victory: score %d, caught %d times", mScore, mDeaths);
			if (!mAutoplayShots.empty())
				SaveScreenshot(StrFormat("%s-victory.png", mAutoplayShots.c_str()));
			mVictory->ButtonDepress(1);
			s->mWait = 20;
		}
		return;
	}
	if (mBoard == NULL)
		return;

	// the tutorial: the move the table allows, or the button that is showing
	if (mGameMode >= MODE_TUTORIAL && mGameMode <= 8)
	{
		Board* b = mBoard;
		if (b->mState != STATE_READY || b->mDeathTimer2 >= 0 || b->mFade2 < 1.0)
			return;
		for (int i = 0; i < 2; i++)
			if (b->mMummies[i] != NULL && b->mMummies[i]->mFighting)
				return;
		if (b->mPlayNowButton0->mVisible || b->mPlayNowButton->mVisible)
		{
			int anId = b->mPlayNowButton0->mVisible ? b->mPlayNowButton0->mId : b->mPlayNowButton->mId;
			Log(s, "tutorial part %d step %d: button %d", mGameMode, b->mTutorialStep, anId);
			if (!mAutoplayShots.empty() && !(s->mTutorialShots & (1 << b->mTutorialStep % 24)) && b->mTutorialStep % 10 == 2)
			{
				s->mTutorialShots |= 1 << b->mTutorialStep % 24;
				SaveScreenshot(StrFormat("%s-step%d.png", mAutoplayShots.c_str(), b->mTutorialStep));
			}
			LogClick(s, mUpdateCount, b->mPlayNowButton0->mVisible ? (Widget*) b->mPlayNowButton0 : (Widget*) b->mPlayNowButton);
			b->ButtonDepress(anId);
			s->mWait = 10;
			return;
		}
		int aMove = kTutorialMove[b->mTutorialStep];
		static const int kKeys[5] = {KEYCODE_UP, KEYCODE_LEFT, KEYCODE_RIGHT, KEYCODE_DOWN, KEYCODE_SPACE};
		if (aMove < 0 || aMove > 4)
			return;
		Log(s, "tutorial part %d step %d: move %d", mGameMode, b->mTutorialStep, aMove);
		Log(s, "  key %d at app update %d, board update %d, state %d", kKeys[aMove], mUpdateCount, b->mUpdateCnt, (int) b->mState);
		mWidgetManager->KeyDown((KeyCode) kKeys[aMove]);
		s->mHeldKey = kKeys[aMove];
		s->mWait = 1;
		return;
	}

	// the scene at the end of a pyramid: the explorer takes the treasure, then the link on to the next pyramid
	if (mGameMode == MODE_CLASSIC_DONE || mGameMode == MODE_ADVENTURE_DONE)
	{
		if (mBoard->mContinueLink != NULL && mBoard->mContinueLink->mVisible)
		{
			if (mGameMode == MODE_ADVENTURE_DONE)
				Log(s, "pyramid %d solved, treasure scene", mAdvPyramid);
			else
				Log(s, "pyramid %d done, score %d", mClassicPyramid, mScore);
			if (!mAutoplayShots.empty() && mClassicPyramid == 0)
				SaveScreenshot(StrFormat("%s-pyramid0.png", mAutoplayShots.c_str()));
			s->mPlanned = false;
			LogClick(s, mUpdateCount, mBoard->mContinueLink);
			mBoard->ButtonDepress(3);
			s->mWait = 20;
		}
		return;
	}

	Board* b = mBoard;
	if (b->mState != STATE_READY || b->mDeathTimer2 >= 0 || b->mFade2 < 1.0 || b->mShowingSolution)
		return;
	// a mummy that has just won a fight with the other one dances first, and the explorer waits for it
	for (int i = 0; i < 2; i++)
		if (b->mMummies[i] != NULL && b->mMummies[i]->mFighting)
			return;
	if (mGameMode != MODE_CLASSIC && mGameMode != MODE_ADVENTURE)
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
	if (s->mLevels >= s->mLevelsPerGame && !s->mGivenUp && mGameMode == MODE_CLASSIC)
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
		s->mDiverged = false;
		s->mReplans = 0;
		s->mUseSolver = s->mRandomMoves == 0 && s->Rand(10) < 7;
		s->mRandomMoves = s->mUseSolver ? 0 : 2 + s->Rand(6);
		if (s->mUseSolver)
		{
			// Solve takes every move back first (Retry): the original is given the click on RESET MAZE
			LogClick(s, mUpdateCount, mSidePanel->mButtons[1]);
			b->Solve();
			s->mPath = b->mSolution;
			s->mPathIndex = 0;
			b->mShowingSolution = false;
			b->ClearSolution();
			Log(s, "level pyramid %d chamber %d (maze %d of file %d, %d x %d): solver, %d moves", BOT_PYR,
				BOT_CHM, b->mMaze, b->mFile, b->mSize, b->mSize, (int) s->mPath.size());
			s->mWait = 30;
			return;
		}
		Log(s, "level pyramid %d chamber %d (maze %d of file %d, %d x %d): %d random moves", BOT_PYR,
			BOT_CHM, b->mMaze, b->mFile, b->mSize, b->mSize, s->mRandomMoves);
	}

	// the next key
	Explorer* e = b->mExplorer;
	int aKey = 0;
	if (s->mUseSolver)
	{
		// the board must be where the solver said it would be after the last move: a difference is a bug in one of them
		if (s->mPathIndex > 0 && s->mPathIndex <= s->mPath.size() && !s->mDiverged)
		{
			ChaseState aWant = s->mPath[s->mPathIndex - 1];
			ChaseState aHave = b->GetChaseState();
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
				Log(s, "  step %d: explorer %d,%d (want %d,%d) mummy %d,%d (want %d,%d) gate %d (want %d); key at %d,%d gate at %d,%d, traps %d,%d",
					(int) s->mPathIndex, aHave.mExplorerX, aHave.mExplorerY, aWant.mExplorerX, aWant.mExplorerY,
					aHave.mMummyX[0], aHave.mMummyY[0], aWant.mMummyX[0], aWant.mMummyY[0], aHave.mGateClosed ? 1 : 0,
					aWant.mGateClosed ? 1 : 0, b->mKeyX, b->mKeyY, b->mGateX, b->mGateY, b->mTrapX[0], b->mTrapY[0]);
			if (!(aWant == aHave))
			{
				s->mDiverged = true;
				s->mReplan = true;
				Log(s, "DIVERGED after move %d: want explorer %d,%d mummies %d,%d%s %d,%d%s scorpion %d,%d%s gate %d; "
					"have explorer %d,%d mummies %d,%d%s %d,%d%s scorpion %d,%d%s gate %d", (int) s->mPathIndex,
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
		if (s->mReplan)
		{
			s->mReplan = false;
			std::vector<ChaseState> aPath;
			if (++s->mReplans <= 8 && b->FindWay(aPath) && !aPath.empty())
			{
				s->mPath = aPath;
				s->mPathIndex = 0;
				s->mDiverged = false;
				Log(s, "replanned from the explorer's place: %d moves", (int) aPath.size());
			}
			else
				s->mUseSolver = false, s->mRandomMoves = 3;
			s->mWait = 5;
			return;
		}
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
			LogClick(s, mUpdateCount, mSidePanel->mButtons[0]);
			b->Undo();
			s->mWait = 5;
			return;
		}
	}
	Log(s, "  key %d at app update %d, board update %d, state %d", aKey, mUpdateCount, b->mUpdateCnt, (int) b->mState);
	mWidgetManager->KeyDown((KeyCode) aKey);
	s->mLastKey = aKey;
	s->mLastAction = s->mTick;
	s->mHeldKey = aKey;
	s->mWait = 1;
}
