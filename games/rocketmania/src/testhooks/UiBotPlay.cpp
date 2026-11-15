// port: the bot of the identical-play runs; see UiBot.h. It decides after each update what the mouse does before the next one, runs
// that through the script (AddScriptCommand), and writes the same line to -uiscript, so the original can be given the input.

#include "UiBot.h"
#include "RocketApp.h"
#include "Board.h"
#include "Dialogs.h"
#include "Screens.h"
#include "WidgetManager.h"
#include "Dialog.h"
#include "DialogButton.h"
#include "HyperlinkWidget.h"

#include <SDL.h>
#include <cstdlib>
#include <algorithm>
#include <deque>

using namespace Sexy;

UiBot::UiBot(RocketApp* theApp, const UiBotParams& theParams) :
	mApp(theApp),
	mParams(theParams),
	mScriptOut(NULL),
	mSeed(theParams.mSeed * 2654435761u + 12345u),
	mGame(0),
	mGamesDone(0),
	mWasBoard(false),
	mLinkClicked(false),
	mQuitting(false),
	mBusyUntil(0),
	mWaitDialog(-1),
	mWaitSince(0),
	mNameTyped(false),
	mFinished(false)
{
	if (!mParams.mScriptFile.empty())
		mScriptOut = fopen(mParams.mScriptFile.c_str(), "w");
	if (mParams.mModes.empty())
		mParams.mModes.push_back(0);
	if (mParams.mDiffs.empty())
		mParams.mDiffs.push_back(2);
	mApp->mScriptedInput = true;
}

UiBot::~UiBot()
{
	if (mScriptOut != NULL)
		fclose(mScriptOut);
}

int UiBot::Next(int theRange)
{
	mSeed = mSeed * 1103515245u + 12345u;
	return (int) ((mSeed >> 16) % (unsigned int) theRange);
}

// the input that the script runs at the start of update (now + theDelay); the script file holds the number it was run at
// (origrun's "at <counter>" is that number minus one, toorig.py)
void UiBot::Input(int theDelay, const char* theCommand, const std::string& theArgs)
{
	int anUpdate = mApp->mUpdateCount + theDelay;
	mApp->AddScriptCommand(anUpdate, theCommand, theArgs);
	if (mScriptOut != NULL)
	{
		fprintf(mScriptOut, "%d %s %s\n", anUpdate, theCommand, theArgs.c_str());
		fflush(mScriptOut);
	}
}

void UiBot::ClickAt(int theX, int theY, int theButton, int theDelay)
{
	char anArgs[32];
	snprintf(anArgs, sizeof(anArgs), "%d %d", theX, theY);
	Input(theDelay, theButton < 0 ? "rclick" : "click", anArgs);
}

void UiBot::ClickWidget(Widget* theWidget, int theDelay)
{
	ClickAt(theWidget->mX + theWidget->mWidth / 2, theWidget->mY + theWidget->mHeight / 2, 1, theDelay);
}

// Handles the dialog that is up, if one is; true when there is one (nothing else is done then)
bool UiBot::Dialogs()
{
	RocketApp* anApp = mApp;
	static const int sOrder[] = { DIALOG_RECORD_SCORE, DIALOG_LEVEL_UP, DIALOG_HIGH_SCORES, DIALOG_HINT, DIALOG_CONTINUE,
		DIALOG_DIFFICULTY, DIALOG_GAME_OVER, DIALOG_QUIT };
	int aFound = -1;
	for (size_t i = 0; i < sizeof(sOrder) / sizeof(sOrder[0]); i++)
	{
		if (anApp->GetDialog(sOrder[i]) != NULL)
		{
			aFound = sOrder[i];
			break;
		}
	}
	if (aFound < 0)
	{
		if (anApp->GetDialogCount() == 0)
		{
			mWaitDialog = -1;
			return false;
		}
		// some other dialog: its yes button
		for (DialogMap::iterator anItr = anApp->mDialogMap.begin(); anItr != anApp->mDialogMap.end(); ++anItr)
		{
			if (anItr->second->mYesButton != NULL && mApp->mUpdateCount + 1 >= mBusyUntil)
			{
				ClickWidget(anItr->second->mYesButton);
				mBusyUntil = mApp->mUpdateCount + 30;
			}
		}
		return true;
	}
	int anUpdate = mApp->mUpdateCount;
	if (anUpdate + 1 < mBusyUntil)
		return true;
	if (aFound != mWaitDialog)
	{
		// a new dialog: give it a few updates before the first click
		mWaitDialog = aFound;
		mWaitSince = anUpdate;
		mBusyUntil = anUpdate + 8;
		return true;
	}
	Dialog* aDialog = anApp->GetDialog(aFound);
	Widget* aTarget = NULL;
	switch (aFound)
	{
	case DIALOG_RECORD_SCORE:
	{
		if (!mNameTyped)
		{
			mNameTyped = true;
			int aCount = (int) mParams.mName.size();
			for (int i = 0; i < aCount; i++)
				Input(1 + i, "char", std::string(1, mParams.mName[i]));
			Input(2 + aCount, "key", "13");
			mBusyUntil = anUpdate + aCount + 40;
			return true;
		}
		aTarget = aDialog->mYesButton;
		break;
	}
	case DIALOG_LEVEL_UP:
	{
		if (anUpdate - mWaitSince < mParams.mHold)
			return true;
		HyperlinkWidget* aLink = ((LevelUpDialog*) aDialog)->mButton;
		if (aLink != NULL && aLink->mVisible && !aLink->mDisabled)
			aTarget = aLink;
		break;
	}
	case DIALOG_HIGH_SCORES:
	{
		HyperlinkWidget* aLink = ((HighScoreDisplayDialog*) aDialog)->mMenuButton;
		if (aLink != NULL && aLink->mVisible && !aLink->mDisabled)
			aTarget = aLink;
		break;
	}
	case DIALOG_HINT:
		aTarget = ((HintDialog*) aDialog)->mPlayButton;
		break;
	case DIALOG_CONTINUE:
		aTarget = mParams.mContinue ? ((ContinueDialog*) aDialog)->mContinueButton : ((ContinueDialog*) aDialog)->mNewButton;
		break;
	case DIALOG_DIFFICULTY:
	{
		int aDifficulty = mParams.mDiffs[mGame % mParams.mDiffs.size()];
		aTarget = ((DifficultyDialog*) aDialog)->mButtons[aDifficulty - 1];
		break;
	}
	case DIALOG_GAME_OVER:
		aTarget = mQuitting ? aDialog->mYesButton : aDialog->mNoButton;
		break;
	case DIALOG_QUIT:
		aTarget = aDialog->mNoButton;
		break;
	}
	if (aTarget != NULL)
		ClickWidget(aTarget);
	mBusyUntil = anUpdate + 30;
	return true;
}

// Finds a path of tiles from the left edge to the right one that the tiles can be turned to make, and queues the clicks that
// turn them. A tile that is not a plain tile of the grid ends the search there.
bool UiBot::PlanPath(Board* theBoard)
{
	struct State
	{
		int mRow, mCol, mIn, mPrev;
		int mRotation;
	};
	std::vector<State> aStates;
	std::deque<int> aQueue;
	bool aSeen[9][6][16];
	for (int r = 0; r < 9; r++)
		for (int c = 0; c < 6; c++)
			for (int s = 0; s < 16; s++)
				aSeen[r][c][s] = false;
	int aFirst = Next(9);
	for (int i = 0; i < 9; i++)
	{
		int aRow = (aFirst + i) % 9;
		Piece* aPiece = theBoard->mGrid[aRow][0];
		if (aPiece == NULL || aPiece->mType != 1 || theBoard->mLeft[aRow] == NULL || theBoard->mRight[aRow] == NULL)
			continue;
		State aState = { aRow, 0, 8, -1, 0 };
		aStates.push_back(aState);
		aSeen[aRow][0][8] = true;
		aQueue.push_back((int) aStates.size() - 1);
	}
	int aGoal = -1;
	while (!aQueue.empty() && aGoal < 0)
	{
		int anIndex = aQueue.front();
		aQueue.pop_front();
		State aState = aStates[anIndex];
		Piece* aPiece = theBoard->mGrid[aState.mRow][aState.mCol];
		int aStart = Next(4);
		for (int k = 0; k < 4 && aGoal < 0; k++)
		{
			int aRotation = (aStart + k) & 3;
			unsigned int aMask = gPipeMask[aRotation + aPiece->mKind * 4];
			if ((aMask & aState.mIn) == 0)
				continue;
			for (int aStep = 0; aStep < 4 && aGoal < 0; aStep++)
			{
				const PipeStep& aDir = gPipeSteps[aStep];
				if ((aMask & aDir.mOut) == 0 || aDir.mOut == aState.mIn)
					continue;
				int aRow = aState.mRow + aDir.mRow;
				int aCol = aState.mCol + aDir.mCol;
				if (aCol == 6 && aDir.mOut == 2)
				{
					State aEnd = { aState.mRow, aState.mCol, aState.mIn, anIndex, aRotation };
					aStates.push_back(aEnd);
					aGoal = (int) aStates.size() - 1;
					break;
				}
				if (aRow < 0 || aRow >= 9 || aCol < 0 || aCol >= 6 || aSeen[aRow][aCol][aDir.mIn])
					continue;
				Piece* aNext = theBoard->mGrid[aRow][aCol];
				if (aNext == NULL || aNext->mType != 1)
					continue;
				aSeen[aRow][aCol][aDir.mIn] = true;
				State aNew = { aRow, aCol, aDir.mIn, anIndex, aRotation };
				// the rotation that is chosen is the one of the cell left, kept in the state of the cell entered
				aStates.push_back(aNew);
				aQueue.push_back((int) aStates.size() - 1);
			}
		}
	}
	if (aGoal < 0)
		return false;
	// walk back: state k holds the rotation chosen for the cell of its parent
	std::vector<Click> aClicks;
	int anIndex = aGoal;
	while (anIndex >= 0)
	{
		const State& aState = aStates[anIndex];
		int aParent = aState.mPrev;
		if (aParent >= 0)
		{
			const State& aCell = aStates[aParent];
			Piece* aPiece = theBoard->mGrid[aCell.mRow][aCell.mCol];
			int aSteps = (aState.mRotation - aPiece->mRotation) & 3;
			int anX = Board::CellX(aCell.mCol) + 21;
			int aY = Board::CellY(aCell.mRow) + 21;
			// a left click turns a tile back a quarter, a right click on a quarter
			if (aSteps == 1)
			{
				Click aClick = { anX, aY, -1 };
				aClicks.push_back(aClick);
			}
			else
			{
				for (int i = 0; i < ((aSteps == 2) ? 2 : (aSteps == 3 ? 1 : 0)); i++)
				{
					Click aClick = { anX, aY, 1 };
					aClicks.push_back(aClick);
				}
			}
		}
		anIndex = aParent;
	}
	std::reverse(aClicks.begin(), aClicks.end());
	mQueue = aClicks;
	return !mQueue.empty();
}

void UiBot::PlayBoard(Board* theBoard)
{
	RocketApp* anApp = mApp;
	int anUpdate = anApp->mUpdateCount;
	if (anUpdate + 1 < mBusyUntil)
		return;
	bool aStopNow = mParams.mStop > 0 && anUpdate + 1 >= mParams.mStop;
	if (!mQuitting && (theBoard->mUpdateCnt >= mParams.mCap || aStopNow) && theBoard->mState == 0)
		mQuitting = true;
	if (mQuitting)
	{
		if (theBoard->mState != 0 || anApp->GetDialog(DIALOG_GAME_OVER) != NULL)
			return;
		for (std::list<ButtonWidget*>::iterator anItr = anApp->mBoardButtons.begin(); anItr != anApp->mBoardButtons.end(); ++anItr)
		{
			if ((*anItr)->mId == 2)
			{
				ClickWidget(*anItr);
				break;
			}
		}
		mBusyUntil = anUpdate + 60;
		return;
	}
	if (theBoard->mState != 0 || theBoard->mBurning || theBoard->mPause != 0)
		return;
	if (mParams.mPassive > 0 && theBoard->mUpdateCnt >= mParams.mPassive)
		return;
	if (!mQueue.empty())
	{
		Click aClick = mQueue.front();
		mQueue.erase(mQueue.begin());
		ClickAt(aClick.mX, aClick.mY, aClick.mButton, 1);
		mBusyUntil = anUpdate + 3 + Next(4);
		return;
	}
	int aChoice = Next(100);
	int aMode = anApp->mMode;
	if (aChoice < 55)
	{
		if (PlanPath(theBoard))
			return;
		aChoice = 55;
	}
	if (aChoice < 75)
	{
		int aRow = Next(theBoard->mRows);
		int aColumn = Next(theBoard->mCols);
		ClickAt(Board::CellX(aColumn) + 21, Board::CellY(aRow) + 21, Next(3) == 0 ? -1 : 1, 1);
		mBusyUntil = anUpdate + 3 + Next(8);
	}
	else if (aChoice < 85 && aMode == 0)
	{
		// drag a tile into the next column: press, move, release
		int aRow = Next(theBoard->mRows);
		int aColumn = Next(theBoard->mCols);
		int anX = Board::CellX(aColumn) + 21;
		int aY = Board::CellY(aRow) + 21;
		char anArgs[32];
		snprintf(anArgs, sizeof(anArgs), "%d %d", anX, aY);
		Input(1, "down", anArgs);
		snprintf(anArgs, sizeof(anArgs), "%d %d", anX + (Next(2) == 0 ? -30 : 30), aY);
		Input(2, "move", anArgs);
		snprintf(anArgs, sizeof(anArgs), "%d %d", anX, aY);
		Input(3, "up", anArgs);
		mBusyUntil = anUpdate + 6 + Next(8);
	}
	else if (aChoice < 93 && aMode == 0)
	{
		// the strip at the top: more tiles drop
		ClickAt(300 + Next(100), 20, 1, 1);
		mBusyUntil = anUpdate + 5 + Next(10);
	}
	else
		mBusyUntil = anUpdate + 2 + Next(10);
}

void UiBot::Update()
{
	RocketApp* anApp = mApp;
	if (mFinished)
		return;
	int anUpdate = anApp->mUpdateCount;
	if (anUpdate + 1 < mBusyUntil)
		return;
	if (!mLinkClicked)
	{
		if (anApp->mLoadingScreen != NULL && anApp->mLoadingScreen->mLinkShown && anUpdate + 1 >= mParams.mStart)
		{
			ClickWidget(anApp->mLoadingScreen->mPlayLink);
			mLinkClicked = true;
			mBusyUntil = anUpdate + 100;
		}
		return;
	}
	Board* aBoard = anApp->mBoard;
	if (aBoard == NULL)
	{
		if (mWasBoard)
		{
			mWasBoard = false;
			mGamesDone++;
			mGame++;
			mQuitting = false;
			mNameTyped = false;
			mQueue.clear();
		}
	}
	else
	{
		mWasBoard = true;
		if (anApp->GetDialogCount() == 0 && aBoard->mState == 3)
			return;
	}
	if (Dialogs())
		return;
	if (aBoard != NULL)
	{
		PlayBoard(aBoard);
		return;
	}
	if (anApp->mGameSelector == NULL)
		return;
	if (mGamesDone >= mParams.mGames || (mParams.mStop > 0 && anUpdate + 1 >= mParams.mStop))
	{
		Input(100, "quit", "");
		mFinished = true;
		return;
	}
	int aMode = mParams.mModes[mGame % mParams.mModes.size()];
	ClickWidget(anApp->mGameSelector->mModeButtons[aMode]);
	mBusyUntil = anUpdate + 40;
}
