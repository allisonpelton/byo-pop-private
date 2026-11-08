// Test hooks: the autoplay bot (see Autoplay.h).

#include "Autoplay.h"
#include "Bot.h"
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
#include "Dialog.h"
#include "EditWidget.h"
#include "HyperlinkWidget.h"

#include <cstdarg>
#include <algorithm>
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
	mScriptOut = NULL;
	mSelStep = 0;
	mPausedFor = 0;
	if (!theApp->mAutoplayLogFileName.empty())
		mLog = fopen(theApp->mAutoplayLogFileName.c_str(), "w");
	if (theApp->mAutoplayUi && !theApp->mAutoplayScriptFileName.empty())
		mScriptOut = fopen(theApp->mAutoplayScriptFileName.c_str(), "w");
}

AutoplayBot::~AutoplayBot()
{
	if (mLog != NULL)
		fclose(mLog);
	if (mScriptOut != NULL)
		fclose(mScriptOut);
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

// -autoplayui: the pointer and the buttons, as a -script file's lines. The bot runs after the update's widgets, a script
// line before the next update's, so a line is for the update after this one.
void AutoplayBot::Move(int theX, int theY)
{
	if (mScriptOut != NULL)
		fprintf(mScriptOut, "%d move %d %d\n", mApp->mUpdateCount + 1, theX, theY);
	mApp->TestMouseMoveTo(theX, theY);
}

void AutoplayBot::Click(int theX, int theY, int theButton)
{
	if (mScriptOut != NULL)
		fprintf(mScriptOut, "%d %s %d %d\n", mApp->mUpdateCount + 1, theButton < 0 ? "rclick" : "click", theX, theY);
	mApp->TestMouseMoveTo(theX, theY);
	mApp->mWidgetManager->MouseDown(theX, theY, theButton);
	mApp->mWidgetManager->MouseUp(theX, theY, theButton);
}

void AutoplayBot::ClickWidget(Widget* theWidget)
{
	Point aPos = theWidget->GetAbsPos();
	Click(aPos.mX + theWidget->mWidth / 2, aPos.mY + theWidget->mHeight / 2, 1);
}

void AutoplayBot::Char(char theChar)
{
	if (mScriptOut != NULL)
		fprintf(mScriptOut, "%d char %c\n", mApp->mUpdateCount + 1, theChar);
	mApp->mWidgetManager->KeyChar((SexyChar) theChar);
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
	if (mScriptOut != NULL)
	{
		fclose(mScriptOut);
		mScriptOut = NULL;
	}
	mApp->Shutdown();
}

// A player's move: the piece follows the mouse to a cell, then a click puts it down
void AutoplayBot::PlayBoard(Board* theBoard)
{
	// a key paused the game: another one goes on
	if (mApp->mAutoplayUi && mApp->mAutoplayExtra > 0 && theBoard->mState == Board::STATE_PLAYING && (theBoard->mPause > 0 || theBoard->mTrialPause > 0))
	{
		if (++mPausedFor >= 40)
		{
			mPausedFor = 0;
			Char('k');
		}
		return;
	}
	if (theBoard->mState != Board::STATE_PLAYING || theBoard->mPause > 0 || theBoard->mTrialPause > 0)
		return;

	// a tutorial step's text waits for the next button
	if (theBoard->mTutorial && theBoard->mTutWaiting)
	{
		Log("tutorial step %d at update %d", theBoard->mTutStep, mTick);
		Shot("tutorial", theBoard->mTutStep);
		if (mApp->mAutoplayUi)
		{
			if (theBoard->mButton == NULL)
				return;
			ClickWidget(theBoard->mButton);
		}
		else
			theBoard->ButtonDepress(2);
		mWait = 5;
		return;
	}
	// the last step's button ends the tutorial
	if (theBoard->mTutorial && theBoard->mCurPiece == NULL && theBoard->mButton != NULL)
	{
		Log("tutorial button %d at update %d", theBoard->mButton->mId, mTick);
		Shot("tutorialend", theBoard->mButton->mId);
		if (mApp->mAutoplayUi)
			ClickWidget(theBoard->mButton);
		else
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
		if (mApp->mAutoplayUi)
			Click(aX, aY, 1);
		else
		{
			mApp->mWidgetManager->MouseDown(aX, aY, 1);
			mApp->mWidgetManager->MouseUp(aX, aY, 1);
		}
		mMoves++;
		mWait = 1;
		return;
	}

	if (mApp->mAutoplayUi && mApp->mAutoplayExtra > 0 && Rand(mApp->mAutoplayExtra) == 0)
	{
		// the side panel's hint button, or a key (it pauses)
		bool isHint = Rand(2) == 0;
		if (isHint)
		{
			for (std::list<ButtonWidget*>::iterator anItr = mApp->mButtons.begin(); anItr != mApp->mButtons.end(); ++anItr)
				if ((*anItr)->mId == BUTTON_HINT)
				{
					ClickWidget(*anItr);
					break;
				}
		}
		else
			Char('k');
		mWait = 3;
		return;
	}

	// the choice itself is shared with the `move` test command (Bot.cpp); the bot's own random numbers are drawn in the
	// same order as before
	BotChoice aChoice = ChooseBotMove(theBoard, &mSeed, mApp->mAutoplayUi ? mApp->mAutoplayMiss : 0);
	if (aChoice.mKind == BotChoice::KIND_FORGE)
	{
		// nowhere to put it: the forge
		Piece* aPiece = theBoard->mCurPiece;
		mForges++;
		Log("forge %d at update %d (piece shape %d color %d)", mForges, mTick, aPiece->mShape, aPiece->mColor);
		if (mApp->mAutoplayUi)
			Click(320, 240, -1);
		else
			theBoard->MouseDown(0, 0, -1);
		mWait = 3;
		return;
	}
	int aPick = aChoice.mRow * Board::COLS + aChoice.mCol;

	// the piece follows the mouse (Piece::Update puts it 0x30 pixels up and left of the pointer): the pointer goes there
	// now and the click comes with the next action, when the piece has caught up
	BotCellToPointer(aPick % Board::COLS, aPick / Board::COLS, &mClickX, &mClickY);
	if (mApp->mAutoplayUi)
		Move(mClickX, mClickY);
	else
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
	if (mTick < mApp->mAutoplayStart)
		return;
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
			if (mApp->mAutoplayUi)
				ClickWidget(mApp->mTitleScreen->mPlayLink);
			else
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
		if (mApp->mAutoplayUi)
			ClickWidget(mApp->GetDialog(DIALOG_HINT)->mYesButton);
		else
			mApp->ButtonDepress(DIALOG_HINT + 2000);
		mWait = 5;
		return;
	}

	// the saved game of an earlier run: yes loads it, and that game is the bot's first
	if (mApp->GetDialog(DIALOG_LOAD_GAME) != NULL)
	{
		Log("saved game loaded at update %d", mTick);
		Shot("loaddialog", mTick);
		if (mApp->mAutoplayUi)
			ClickWidget(mApp->GetDialog(DIALOG_LOAD_GAME)->mYesButton);
		else
			mApp->ButtonDepress(DIALOG_LOAD_GAME + 2000);
		mGames = 1;
		mWait = 20;
		return;
	}

	// a name for the score table
	if (mApp->GetDialog(DIALOG_NAME_ENTRY) != NULL)
	{
		Log("name entered at update %d", mTick);
		{
			NameEntryDialog* aName = (NameEntryDialog*) mApp->GetDialog(DIALOG_NAME_ENTRY);
			Point aDialogPos = aName->GetAbsPos();
			Point anEditPos = aName->mEdit->GetAbsPos();
			Point aYesPos = aName->mYesButton->GetAbsPos();
			Point aNoPos = aName->mNoButton != NULL ? aName->mNoButton->GetAbsPos() : Point(-1, -1);
			Log("name dialog: lines %d chars, header %d chars; at (%d, %d) %d x %d; edit (%d, %d) %d x %d; yes (%d, %d) %d x %d; no (%d, %d)", (int) aName->mDialogLines.length(), (int) aName->mDialogHeader.length(), aDialogPos.mX, aDialogPos.mY,
				aName->mWidth, aName->mHeight, anEditPos.mX, anEditPos.mY, aName->mEdit->mWidth, aName->mEdit->mHeight, aYesPos.mX,
				aYesPos.mY, aName->mYesButton->mWidth, aName->mYesButton->mHeight, aNoPos.mX, aNoPos.mY);
		}
		Shot("name", mGames);
		if (mApp->mAutoplayUi)
		{
			Char('B');
			Char('o');
			Char('t');
			ClickWidget(mApp->GetDialog(DIALOG_NAME_ENTRY)->mYesButton);
		}
		else
		{
			((NameEntryDialog*) mApp->GetDialog(DIALOG_NAME_ENTRY))->mEdit->SetText("Bot");
			mApp->ButtonDepress(DIALOG_NAME_ENTRY + 2000);
		}
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
		if (mApp->mAutoplayUi)
			ClickWidget(mApp->mHighScores->mStartButton);
		else
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
		if (mApp->mAutoplayUi && mApp->mAutoplayMode == "tutorial")
		{
			// the tutorial's picture button starts it
			mGames++;
			mMoves = 0;
			mForges = 0;
			mOverLogged = false;
			mLastState = -1;
			mLastBoards = 0;
			Log("game %d (tutorial) starts at update %d", mGames, mTick);
			ClickWidget(mApp->mSelector->mTutorialPicButton);
			mWait = 20;
			return;
		}
		if (mApp->mAutoplayUi && mSelStep == 0 && mApp->mAutoplaySkill >= 0)
		{
			// the skill level's button, then the mode's, then (each two updates later) the start button
			ClickWidget(mApp->mSelector->mSkillButtons[mApp->mAutoplaySkill]);
			mSelStep = 1;
			mWait = 2;
			return;
		}
		if (mApp->mAutoplayUi && mSelStep <= 1)
		{
			bool aTrial = mApp->mAutoplayMode == "timetrial";
			ClickWidget(aTrial ? (Widget*) mApp->mSelector->mTimeTrialButton : (Widget*) mApp->mSelector->mStrategicButton);
			mSelStep = 2;
			mWait = 2;
			return;
		}
		mSelStep = 0;
		mGames++;
		mMoves = 0;
		mForges = 0;
		mOverLogged = false;
		mLastState = -1;
		mLastBoards = 0;
		Shot("selector", mGames);
		Log("game %d (%s) starts at update %d", mGames, mApp->mAutoplayMode.c_str(), mTick);
		if (mApp->mAutoplayUi)
			ClickWidget(mApp->mSelector->mStartButton);
		else
		{
			mApp->mTutorialMode = (mApp->mAutoplayMode == "tutorial");
			mApp->SetTimeTrial(mApp->mAutoplayMode == "timetrial");
			mApp->StartGame();
		}
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
		if (mApp->mAutoplayUi)
		{
			if (aBoard->mButton == NULL)
				return;
			ClickWidget(aBoard->mButton);
		}
		else
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

StateLog::StateLog(AlchemyApp* theApp)
{
	mApp = theApp;
	mFile = fopen(theApp->mStateLogFileName.c_str(), "w");
	for (int i = 0; i < NUM_FIELDS; i++)
		mLast[i] = -999999;
}

StateLog::~StateLog()
{
	if (mFile != NULL)
		fclose(mFile);
}

void StateLog::Update()
{
	if (mFile == NULL)
		return;

	static const char* const aNames[NUM_FIELDS] = {"state", "score", "moves", "discards", "boards", "shape", "color", "timetrial",
		"trialpause", "fullcount", "overtimer", "multimoves", "run", "longest", "pause", "numshapes", "numcolors", "bombchance",
		"skullchance", "elapsed", "totalseconds", "numvalid", "lastshape", "tutorial", "tutstep", "tutcount", "tutwaiting", "hinton", "hinttimer", "rand"};
	int aValues[NUM_FIELDS];
	for (int i = 0; i < NUM_FIELDS; i++)
		aValues[i] = -1000000;  // "-": no board or piece
	Board* aBoard = mApp->mBoard;
	if (aBoard != NULL)
	{
		aValues[0] = aBoard->mState;
		aValues[1] = aBoard->mScore;
		aValues[2] = aBoard->mMoves;
		aValues[3] = aBoard->mDiscards;
		aValues[4] = aBoard->mBoardsCleared;
		aValues[7] = aBoard->mTimeTrial ? 1 : 0;
		aValues[8] = aBoard->mTrialPause;
		aValues[9] = aBoard->mFullCount;
		aValues[10] = aBoard->mOverTimer;
		aValues[11] = aBoard->mMultiMoves;
		aValues[12] = aBoard->mRun;
		aValues[13] = aBoard->mLongestRun;
		aValues[14] = aBoard->mPause;
		aValues[15] = aBoard->mNumShapes;
		aValues[16] = aBoard->mNumColors;
		aValues[17] = aBoard->mBombChance;
		aValues[18] = aBoard->mSkullChance;
		aValues[19] = aBoard->mElapsed;
		aValues[20] = aBoard->mTotalSeconds;
		aValues[21] = aBoard->mNumValid;
		aValues[22] = aBoard->mLastShape;
		aValues[23] = aBoard->mTutorial ? 1 : 0;
		aValues[24] = aBoard->mTutStep;
		aValues[25] = aBoard->mTutCount;
		aValues[26] = aBoard->mTutWaiting ? 1 : 0;
		aValues[27] = aBoard->mHintOn ? 1 : 0;
		aValues[28] = aBoard->mHintTimer;
		if (aBoard->mCurPiece != NULL)
		{
			aValues[5] = aBoard->mCurPiece->mShape;
			aValues[6] = aBoard->mCurPiece->mColor;
		}
	}
	aValues[29] = (int) CrtRandState();  // the C library's rand, after this update
	for (int i = 0; i < NUM_FIELDS; i++)
	{
		if (aValues[i] == mLast[i])
			continue;
		mLast[i] = aValues[i];
		if (aValues[i] == -1000000)
			fprintf(mFile, "%d %s -\n", mApp->mUpdateCount, aNames[i]);
		else
			fprintf(mFile, "%d %s %d\n", mApp->mUpdateCount, aNames[i], aValues[i]);
	}
	fflush(mFile);
}
