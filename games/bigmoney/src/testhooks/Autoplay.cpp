// port: test hook (see Autoplay.h). Not part of the original game.

#include "Autoplay.h"
#include "WinBMApp.h"
#include "Board.h"
#include "Piece.h"
#include "PuzzleSolver.h"
#include "TitleScreen.h"
#include "GameSelector.h"
#include "HelpWidget.h"
#include "Res.h"

#include "HyperlinkWidget.h"
#include "DialogButton.h"
#include "Dialog.h"

#include <SDL.h>
#include <cstring>
#include <vector>

using namespace Sexy;

AutoplayBot::AutoplayBot(WinBMApp* theApp, unsigned long theSeed, int theMode, int theSkill, int theGames,
	int theUpdateCap, const std::string& theLogFileName, const std::string& theShotPrefix) :
	mApp(theApp), mRand(theSeed), mSeed(theSeed), mMode(theMode), mSkill(theSkill), mGamesWanted(theGames),
	mUpdateCap(theUpdateCap), mLog(NULL), mShotPrefix(theShotPrefix)
{
	if ((mMode < GAMETYPE_ACTION) || (mMode > GAMETYPE_PUZZLE))
		mMode = GAMETYPE_ACTION;
	if ((mSkill < 1) || (mSkill > 3))
		mSkill = 1;
	if (mGamesWanted < 1)
		mGamesWanted = 1;
	mNextAction = 0;
	mGamesOver = 0;
	mLevelsDone = 0;
	mLastBoardState = -1;
	mLastBoard = NULL;
	mQuitAt = -1;
	mDone = false;
	if (!theLogFileName.empty())
	{
		mLog = fopen(theLogFileName.c_str(), "wb");
		if (mLog != NULL)
			fprintf(mLog, "# autoplay seed %lu mode %d skill %d games %d\n", mSeed, mMode, mSkill, mGamesWanted);
		else
			SDL_Log("autoplay: cannot write %s", theLogFileName.c_str());
	}
}

AutoplayBot::~AutoplayBot()
{
	if (mLog != NULL)
		fclose(mLog);
}

// Commands run before the next update, like a -script line for mUpdateCount + 1
void AutoplayBot::Queue(const std::string& theCommand, const std::string& theArgs)
{
	int anUpdate = mApp->mUpdateCount + 1;
	mApp->AddScriptCommand(anUpdate, theCommand, theArgs);
	if (mLog != NULL)
		fprintf(mLog, "%d %s %s\n", anUpdate, theCommand.c_str(), theArgs.c_str());
}

void AutoplayBot::Click(int theX, int theY, const char* theWhat)
{
	char anArgs[32];
	snprintf(anArgs, sizeof(anArgs), "%d %d", theX, theY);
	// A move first, so hover-sensitive widgets (HyperlinkWidget) see the mouse over them on the press
	Queue("move", anArgs);
	Queue("click", anArgs);
	if ((mLog != NULL) && (theWhat != NULL))
		fprintf(mLog, "# %s\n", theWhat);
}

// <prefix>-<game number>-<event>.png
void AutoplayBot::Shot(int theGame, const char* theEvent)
{
	if (mShotPrefix.empty())
		return;
	char aName[64];
	snprintf(aName, sizeof(aName), "-%d-%s.png", theGame, theEvent);
	Queue("screenshot", mShotPrefix + aName);
}

// A clickable group (same colour, 4-connected, not special, at least mMinGroup coins), chosen by the bot's
// own flood fill over the grid (the game's FindGroups marks are not touched): mostly the largest, sometimes
// a random one. Returns the screen point of one of its coins.
bool AutoplayBot::PickGroup(Board* theBoard, int* theX, int* theY, int* theSize)
{
	int aMark[Board::BOARD_ROWS][Board::BOARD_COLS];
	memset(aMark, 0, sizeof(aMark));
	struct Group { int mRow, mCol, mSize; };
	std::vector<Group> aGroups;
	int aNextMark = 1;
	for (int aRow = 0; aRow < Board::BOARD_ROWS; aRow++)
	{
		for (int aCol = 0; aCol < Board::BOARD_COLS; aCol++)
		{
			Piece* aPiece = theBoard->mGrid[aRow][aCol];
			if ((aPiece == NULL) || (aPiece->mIsSpecial) || (aMark[aRow][aCol] != 0))
				continue;
			int aSize = 0;
			std::vector<std::pair<int, int>> aStack(1, std::make_pair(aRow, aCol));
			aMark[aRow][aCol] = aNextMark;
			while (!aStack.empty())
			{
				std::pair<int, int> aCell = aStack.back();
				aStack.pop_back();
				aSize++;
				static const int kDR[4] = {0, 0, -1, 1};
				static const int kDC[4] = {-1, 1, 0, 0};
				for (int d = 0; d < 4; d++)
				{
					int r = aCell.first + kDR[d];
					int c = aCell.second + kDC[d];
					if ((r < 0) || (r >= Board::BOARD_ROWS) || (c < 0) || (c >= Board::BOARD_COLS) || (aMark[r][c] != 0))
						continue;
					Piece* aNeighbour = theBoard->mGrid[r][c];
					if ((aNeighbour == NULL) || (aNeighbour->mIsSpecial) || (aNeighbour->mColor != aPiece->mColor))
						continue;
					aMark[r][c] = aNextMark;
					aStack.push_back(std::make_pair(r, c));
				}
			}
			aNextMark++;
			if (aSize >= theBoard->mMinGroup)
				aGroups.push_back({aRow, aCol, aSize});
		}
	}
	if (aGroups.empty())
		return false;
	size_t aPick = 0;
	if (mRand() % 4 == 0)
		aPick = mRand() % aGroups.size();
	else
	{
		for (size_t i = 1; i < aGroups.size(); i++)
			if (aGroups[i].mSize > aGroups[aPick].mSize)
				aPick = i;
	}
	Piece* aPiece = theBoard->mGrid[aGroups[aPick].mRow][aGroups[aPick].mCol];
	*theX = theBoard->mX + (int) aPiece->mX + 14;
	*theY = theBoard->mY + (int) aPiece->mY + 14;
	*theSize = aGroups[aPick].mSize;
	return true;
}

void AutoplayBot::Update()
{
	if (mDone)
		return;
	int aNow = mApp->mUpdateCount;
	if ((mQuitAt >= 0 && aNow >= mQuitAt) || (aNow >= mUpdateCap))
	{
		Queue("quit", "");
		mDone = true;
		return;
	}
	if (aNow < mNextAction)
		return;

	// Title screen: the play link once loading is done
	if (mApp->mTitleScreen != NULL)
	{
		HyperlinkWidget* aLink = mApp->mTitleScreen->mPlayLink;
		if ((mApp->mTitleScreen->mLoadDone) && (aLink != NULL) && (aLink->mVisible) && (aLink->mWidth > 0))
		{
			Click(aLink->mX + aLink->mWidth / 2, aLink->mY + aLink->mHeight / 2, "title: play");
			mNextAction = aNow + 100;
		}
		return;
	}

	// "Continue your last game?" (a save from an earlier run): NO, the bot always starts fresh
	if (mApp->GetDialog(DIALOG_CONTINUE_GAME) != NULL)
	{
		Dialog* aDialog = mApp->GetDialog(DIALOG_CONTINUE_GAME);
		if (aDialog->mNoButton != NULL)
		{
			DialogButton* aNo = aDialog->mNoButton;
			Click(aDialog->mX + aNo->mX + aNo->mWidth / 2, aDialog->mY + aNo->mY + aNo->mHeight / 2, "continue: no");
		}
		mNextAction = aNow + 60;
		return;
	}

	// High score name entry: type a name and press Enter
	if (mApp->GetDialog(DIALOG_RECORD_SCORE) != NULL)
	{
		Queue("key", "BACK");
		Queue("char", "B");
		Queue("char", "o");
		Queue("char", "t");
		Queue("key", "RETURN");
		if (mLog != NULL)
			fprintf(mLog, "# record score: name Bot\n");
		mNextAction = aNow + 60;
		return;
	}
	// Any other dialog (e.g. the "enter a name" notice): its default button by Enter
	if (mApp->GetDialogCount() > 0)
	{
		Queue("key", "RETURN");
		mNextAction = aNow + 60;
		return;
	}

	// Game selector: type, skill, start (only while it is in place)
	if (mApp->mGameSelector != NULL)
	{
		GameSelector* aSel = mApp->mGameSelector;
		if ((aSel->mSlidingIn) || (aSel->mSlidingOut))
			return;
		if (aSel->mGameType != mMode)
			Click(178 + 154 * mMode + 77, 190, "selector: type");
		else if (aSel->mSkill != mSkill)
		{
			static const int kSkillX[3] = {29, 186, 339};
			Click(GameSelector::PANEL_X + kSkillX[mSkill - 1] + 56, 385, "selector: skill");
		}
		else
			Click(400, 448, "selector: start");
		mNextAction = aNow + 40;
		return;
	}

	// Help screen: "click here to start game"
	if (mApp->mHelpWidget != NULL)
	{
		HyperlinkWidget* aLink = mApp->mHelpWidget->mStartLink;
		if ((aLink != NULL) && (aLink->mWidth > 0))
			Click(mApp->mHelpWidget->mX + aLink->mX + aLink->mWidth / 2, mApp->mHelpWidget->mY + aLink->mY + aLink->mHeight / 2,
				"help: start game");
		mNextAction = aNow + 60;
		return;
	}

	Board* aBoard = mApp->mBoard;
	if (aBoard == NULL)
		return;

	// Events, logged once per board and state
	if ((aBoard != mLastBoard) || (aBoard->mState != mLastBoardState))
	{
		if (aBoard->mState == STATE_LEVEL_DONE)
		{
			mLevelsDone++;
			if (mLog != NULL)
				fprintf(mLog, "# level %d done, score %d\n", mLevelsDone, aBoard->mScore);
			Shot(mGamesOver + 1, "level");
		}
		else if (aBoard->mState == STATE_GAME_OVER_SCORES)
		{
			mGamesOver++;
			if (mLog != NULL)
				fprintf(mLog, "# game %d over, score %d\n", mGamesOver, aBoard->mScore);
			Shot(mGamesOver, "gameover");
			if (mGamesOver >= mGamesWanted)
				mQuitAt = aNow + 700;  // after the score slide and the name entry
			mNextAction = aNow + 450;
		}
		mLastBoard = aBoard;
		mLastBoardState = aBoard->mState;
	}

	if (aBoard->mPauseCount > 0)
	{
		// A click anywhere resumes (Board::MouseDown)
		Click(400, 240, "unpause");
		mNextAction = aNow + 30;
		return;
	}

	DialogButton* aButton = aBoard->mContinueButton;
	if (((aBoard->mState == STATE_LEVEL_DONE) || (aBoard->mState == STATE_GAME_OVER_SCORES)) && (aButton != NULL) &&
		(aButton->mWidth > 0))
	{
		if ((aBoard->mState == STATE_GAME_OVER_SCORES) && (mGamesOver >= mGamesWanted))
			return;
		Click(aButton->mX + aButton->mWidth / 2, aButton->mY + aButton->mHeight / 2,
			aBoard->mState == STATE_LEVEL_DONE ? "next level" : "play again");
		mNextAction = aNow + 100;
		return;
	}

	if (aBoard->mState != STATE_PLAYING)
		return;

	// Columns still sliding sideways do not count as moving, and a click finds its coin by where it is drawn
	// (Board::PieceAt): the solver's tests wait until every coin rests in its cell
	bool isAtRest = !aBoard->mPiecesMoving;
	for (int aRow = 0; aRow < Board::BOARD_ROWS; aRow++)
	{
		for (int aCol = 0; aCol < Board::BOARD_COLS; aCol++)
		{
			Piece* aPiece = aBoard->mGrid[aRow][aCol];
			if ((aPiece != NULL) && ((aPiece->mX != (double) Board::ColX(aCol)) || (aPiece->mY != (double) Board::RowY(aRow))))
				isAtRest = false;
		}
	}

	if ((mPressUndo > 0) && (aBoard->mSolver != NULL) && (aBoard->mSolverUndo) && (!aBoard->mSolverUndos.empty()) &&
		((int) (mRand() % 100) < mPressUndo))
	{
		Queue("char", "u");
		if (mLog != NULL)
			fprintf(mLog, "# undo\n");
		mNextAction = aNow + 20 + (int) (mRand() % 40);
		return;
	}

	if (mCheckPending)
	{
		int aVerdict = HoverVerdict(aBoard);
		if ((aVerdict < 0) && (aNow - mCheckSince < 3000))
			return;
		Queue("click", mCheckArgs);
		if (mLog != NULL)
			fprintf(mLog, "# %s, checked: %d\n", mCheckWhat.c_str(), aVerdict);
		mCheckPending = false;
		mNextAction = aNow + 20 + (int) (mRand() % 40);
		return;
	}

	int aX, aY, aSize;
	if ((mFollowSolver > 0) && (aBoard->mSolver != NULL) && ((mFollowSolver >= 100) || ((int) (mRand() % 100) < mFollowSolver)))
	{
		PuzzleSolver* aSolver = aBoard->mSolver;
		if ((!isAtRest) || (aSolver->mStatus == PuzzleSolver::STATUS_THINKING) ||
			((aSolver->mStatus == PuzzleSolver::STATUS_WINNABLE) && (aSolver->mPlan.empty())))
			return; // wait for the board to settle and the solver to answer
		if ((aSolver->mStatus == PuzzleSolver::STATUS_WINNABLE) && (mPressHints) && (aBoard->mSolverHintBoard == 0) &&
			(aBoard->mSolverHints) && (!aBoard->mSolverHintWanted) && (aBoard->mSolverMessageTicks == 0))
		{
			Queue("char", "h");
			mNextAction = aNow + 5;
			return;
		}
		if (aSolver->mStatus == PuzzleSolver::STATUS_WINNABLE)
		{
			const PuzzleSolver::Move& aMove = aSolver->mPlan[0];
			Click(Board::ColX(aMove.mCol) + 14, Board::RowY(aMove.mRow) + 14, "solver's click");
			mNextAction = aNow + 20;
			return;
		}
	}
	if ((mCheckClicks) && (aBoard->mSolverCheck != NULL) && (!isAtRest))
		return;
	if ((mRand() % 25 == 0) || (!PickGroup(aBoard, &aX, &aY, &aSize)))
	{
		// A random cell: often an invalid click (single coin, a bag, an empty cell)
		aX = Board::ColX(mRand() % Board::BOARD_COLS) + 14;
		aY = Board::RowY(mRand() % Board::BOARD_ROWS) + 14;
		Click(aX, aY, "random cell");
	}
	else
	{
		char aWhat[32];
		snprintf(aWhat, sizeof(aWhat), "group of %d", aSize);
		if ((mCheckClicks) && (aBoard->mSolverCheck != NULL))
		{
			char anArgs[32];
			snprintf(anArgs, sizeof(anArgs), "%d %d", aX, aY);
			Queue("move", anArgs);
			mCheckPending = true;
			mCheckArgs = anArgs;
			mCheckWhat = aWhat;
			mCheckSince = aNow;
			mNextAction = aNow + 2;
			return;
		}
		Click(aX, aY, aWhat);
	}
	mNextAction = aNow + 20 + (int) (mRand() % 40);
}

// -autoplaycheck: the solver's verdict (PuzzleClickCheck::Verdict) on the click on the hovered group, -1 while
// it has none; VERDICT_NONE when no check comes (a lost level)
int AutoplayBot::HoverVerdict(Board* theBoard)
{
	PuzzleSolver* aSolver = theBoard->mSolver;
	if ((theBoard->mSolverCheck == NULL) || (aSolver->mStatus == PuzzleSolver::STATUS_LOST))
		return PuzzleClickCheck::VERDICT_NONE;
	if ((theBoard->mPiecesMoving) || (aSolver->mStatus == PuzzleSolver::STATUS_THINKING) ||
		(theBoard->mSolverVerdictBoard != aSolver->GetBoardHash()))
		return -1;
	for (int aRow = 0; aRow < Board::BOARD_ROWS; aRow++)
	{
		for (int aCol = 0; aCol < Board::BOARD_COLS; aCol++)
		{
			Piece* aPiece = theBoard->mGrid[aRow][aCol];
			if ((aPiece == NULL) || (!aPiece->mHighlighted))
				continue;
			int aKey = theBoard->SolverGroupKey(aCol, aRow);
			if ((aKey < 0) || (theBoard->mSolverVerdict[aKey] == PuzzleClickCheck::VERDICT_NONE))
				return -1;
			return theBoard->mSolverVerdict[aKey];
		}
	}
	return -1;
}
