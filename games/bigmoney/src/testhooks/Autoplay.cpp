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
#include <algorithm>
#include <climits>
#include <cstring>
#include <vector>

using namespace Sexy;

// -autoplaylook: the board as the bot's one-click lookahead sees it (EMPTY, SPECIAL or a coin colour)
namespace
{
enum { LOOK_EMPTY = -1, LOOK_SPECIAL = -2 };

struct LookGrid
{
	int8_t mCell[Board::BOARD_ROWS][Board::BOARD_COLS];
	int mCols;
	int mRows;
	int mDirs;
};

void FillLookGrid(Board* theBoard, LookGrid* theGrid)
{
	memset(theGrid->mCell, LOOK_EMPTY, sizeof(theGrid->mCell));
	theGrid->mCols = theBoard->mCols;
	theGrid->mRows = theBoard->mRows;
	theGrid->mDirs = theBoard->GroupDirs();
	for (int r = 0; r < theBoard->mRows; r++)
	{
		for (int c = 0; c < theBoard->mCols; c++)
		{
			Piece* aPiece = theBoard->mGrid[r][c];
			if (aPiece != NULL)
				theGrid->mCell[r][c] = aPiece->mIsSpecial ? LOOK_SPECIAL : (int8_t) aPiece->mColor;
		}
	}
}

// The cells (r * BOARD_COLS + c) of the coin group at (r, c), marking them in theSeen
void LookGroup(const LookGrid& theGrid, int theRow, int theCol, bool* theSeen, std::vector<int>* theCells)
{
	theCells->clear();
	int8_t aColor = theGrid.mCell[theRow][theCol];
	theCells->push_back(theRow * Board::BOARD_COLS + theCol);
	theSeen[theRow * Board::BOARD_COLS + theCol] = true;
	for (size_t i = 0; i < theCells->size(); i++)
	{
		int r0 = (*theCells)[i] / Board::BOARD_COLS;
		int c0 = (*theCells)[i] % Board::BOARD_COLS;
		for (int d = 0; d < theGrid.mDirs; d++)
		{
			int r = r0 + gGroupDirs[d][1];
			int c = c0 + gGroupDirs[d][0];
			if ((r < 0) || (r >= theGrid.mRows) || (c < 0) || (c >= theGrid.mCols) || (theSeen[r * Board::BOARD_COLS + c]) ||
				(theGrid.mCell[r][c] != aColor))
				continue;
			theSeen[r * Board::BOARD_COLS + c] = true;
			theCells->push_back(r * Board::BOARD_COLS + c);
		}
	}
}

// After a click: the group's coins go, so do bags right above them (banked or on their way out), and the
// pieces above fall (the sideways closing of empty bottom cells is left out)
void LookRemove(LookGrid* theGrid, const std::vector<int>& theCells)
{
	for (int aCell : theCells)
	{
		int r = aCell / Board::BOARD_COLS;
		int c = aCell % Board::BOARD_COLS;
		theGrid->mCell[r][c] = LOOK_EMPTY;
		if ((r > 0) && (theGrid->mCell[r - 1][c] == LOOK_SPECIAL))
			theGrid->mCell[r - 1][c] = LOOK_EMPTY;
	}
	for (int c = 0; c < theGrid->mCols; c++)
	{
		int aDest = theGrid->mRows - 1;
		for (int r = theGrid->mRows - 1; r >= 0; r--)
		{
			int8_t v = theGrid->mCell[r][c];
			if (v == LOOK_EMPTY)
				continue;
			theGrid->mCell[r][c] = LOOK_EMPTY;
			theGrid->mCell[aDest--][c] = v;
		}
	}
}

// What the board offers next: (size - 1)^2 summed over its coin groups of two or more, less theHeightCost
// per row of its tallest column
int LookPotential(const LookGrid& theGrid, int theHeightCost)
{
	bool aSeen[Board::BOARD_ROWS * Board::BOARD_COLS] = {};
	std::vector<int> aCells;
	int aSum = 0;
	int aTop = theGrid.mRows;
	for (int r = 0; r < theGrid.mRows; r++)
	{
		for (int c = 0; c < theGrid.mCols; c++)
		{
			int8_t v = theGrid.mCell[r][c];
			if (v != LOOK_EMPTY)
				aTop = std::min(aTop, r);
			if ((v < 0) || (aSeen[r * Board::BOARD_COLS + c]))
				continue;
			LookGroup(theGrid, r, c, aSeen, &aCells);
			int n = (int) aCells.size() - 1;
			aSum += n * n;
		}
	}
	return aSum - theHeightCost * (theGrid.mRows - aTop);
}
}

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

// A clickable group (same colour, connected as the board's groups are, not special, at least mMinGroup
// coins), chosen by the bot's own flood fill over the grid (the game's FindGroups marks are not touched):
// mostly the largest, sometimes a random one. With mBags the best pick is the largest group with a bag right
// above one of its coins (double points, a bag on its way, and no move used in Strategy), if there is one.
// Returns the screen point of one of its coins.
bool AutoplayBot::PickGroup(Board* theBoard, int* theX, int* theY, int* theSize)
{
	int aMark[Board::BOARD_ROWS][Board::BOARD_COLS];
	memset(aMark, 0, sizeof(aMark));
	struct Group { int mRow, mCol, mSize; bool mUnderBag; };
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
			bool isUnderBag = false;
			std::vector<std::pair<int, int>> aStack(1, std::make_pair(aRow, aCol));
			aMark[aRow][aCol] = aNextMark;
			while (!aStack.empty())
			{
				std::pair<int, int> aCell = aStack.back();
				aStack.pop_back();
				aSize++;
				Piece* anAbove = (aCell.first > 0) ? theBoard->mGrid[aCell.first - 1][aCell.second] : NULL;
				isUnderBag |= (anAbove != NULL) && (anAbove->mIsSpecial);
				for (int d = 0; d < theBoard->GroupDirs(); d++)
				{
					int r = aCell.first + gGroupDirs[d][1];
					int c = aCell.second + gGroupDirs[d][0];
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
				aGroups.push_back({aRow, aCol, aSize, isUnderBag});
		}
	}
	if (aGroups.empty())
		return false;
	size_t aPick = 0;
	bool isRandom = (mBest < 0) ? (mRand() % 4 == 0) : ((int) (mRand() % 100) >= mBest);
	if (isRandom)
		aPick = mRand() % aGroups.size();
	else if (mLook)
	{
		// One click ahead: its points (doubled under a bag, as the game does), a bag freed (in Strategy no
		// move used), and what the board offers after it
		LookGrid aGrid;
		FillLookGrid(theBoard, &aGrid);
		int aBest = INT_MIN;
		for (size_t i = 0; i < aGroups.size(); i++)
		{
			bool aSeen[Board::BOARD_ROWS * Board::BOARD_COLS] = {};
			std::vector<int> aCells;
			LookGroup(aGrid, aGroups[i].mRow, aGroups[i].mCol, aSeen, &aCells);
			LookGrid anAfter = aGrid;
			LookRemove(&anAfter, aCells);
			int aScore = theBoard->mTunables.GroupPoints(aGroups[i].mSize) * (aGroups[i].mUnderBag ? 2 : 1) +
				(aGroups[i].mUnderBag ? mLookBag : 0) + LookPotential(anAfter, mLookHeight);
			if (aScore > aBest)
			{
				aBest = aScore;
				aPick = i;
			}
		}
	}
	else
	{
		for (size_t i = 1; i < aGroups.size(); i++)
		{
			bool isBetterBag = (mBags) && (aGroups[i].mUnderBag != aGroups[aPick].mUnderBag);
			if (isBetterBag ? aGroups[i].mUnderBag : (aGroups[i].mSize > aGroups[aPick].mSize))
				aPick = i;
		}
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
		mNextAction = aNow + ClickDelay();
		return;
	}

	if ((!mCheckPending) && (UsePickpocket(aBoard, aNow)))
		return;

	if (mCheckPending)
	{
		int aVerdict = HoverVerdict(aBoard);
		if ((aVerdict < 0) && (aNow - mCheckSince < 3000))
			return;
		Queue("click", mCheckArgs);
		if (mLog != NULL)
			fprintf(mLog, "# %s, checked: %d\n", mCheckWhat.c_str(), aVerdict);
		mCheckPending = false;
		mNextAction = aNow + ClickDelay();
		return;
	}

	int aX, aY, aSize;
	// (not while a won Puzzle level plays on for extra bags: the solver is done with it)
	if ((mFollowSolver > 0) && (aBoard->mSolver != NULL) && (!aBoard->PuzzlePlaysOn()) &&
		((mFollowSolver >= 100) || ((int) (mRand() % 100) < mFollowSolver)))
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
			Click(Board::ColX(aMove.mCol) + 14, Board::RowY(aMove.mRow - aBoard->SolverRowOffset()) + 14, "solver's click");
			mNextAction = aNow + 20;
			return;
		}
	}
	if ((mCheckClicks) && (aBoard->mSolverCheck != NULL) && (!isAtRest))
		return;
	if ((mRand() % 25 == 0) || (!PickGroup(aBoard, &aX, &aY, &aSize)))
	{
		// A random cell: often an invalid click (single coin, a bag, an empty cell)
		aX = Board::ColX(mRand() % aBoard->mCols) + 14;
		aY = Board::RowY(mRand() % aBoard->mRows) + 14;
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
	mNextAction = aNow + ClickDelay();
}

// The updates until the bot's next click (mPace)
int AutoplayBot::ClickDelay()
{
	if (mPace <= 0)
		return 20 + (int) (mRand() % 40);
	return mPace / 2 + (int) (mRand() % (unsigned long) mPace);
}

// -autoplaypocket: a Pickpocket on a random small group, in mUsePockets% of the turns and whenever no group can
// be clicked (not while a Puzzle board moves)
bool AutoplayBot::UsePickpocket(Board* theBoard, int theNow)
{
	if (((mUsePockets <= 0) && (!mPocketBags)) || (!theBoard->mPickpocket) || (theBoard->mPickpockets <= 0) ||
		((theBoard->mPiecesMoving) && (mApp->mGameType == GAMETYPE_PUZZLE)))
		return false;
	int aX, aY, aSize;
	std::vector<Piece*> aTargets;
	if (mPocketBags)
	{
		// A small group with a bag right above it; any small group only when nothing can be clicked
		for (int aRow = 1; aRow < Board::BOARD_ROWS; aRow++)
		{
			for (int aCol = 0; aCol < Board::BOARD_COLS; aCol++)
			{
				Piece* anAbove = theBoard->mGrid[aRow - 1][aCol];
				if ((anAbove != NULL) && (anAbove->mIsSpecial) && (theBoard->IsPickpocketTarget(theBoard->mGrid[aRow][aCol])))
					aTargets.push_back(theBoard->mGrid[aRow][aCol]);
			}
		}
		if ((aTargets.empty()) && (PickGroup(theBoard, &aX, &aY, &aSize)))
			return false;
	}
	else if (((int) (mRand() % 100) >= mUsePockets) && (PickGroup(theBoard, &aX, &aY, &aSize)))
		return false;
	if (aTargets.empty())
	{
		for (int aRow = 0; aRow < Board::BOARD_ROWS; aRow++)
		{
			for (int aCol = 0; aCol < Board::BOARD_COLS; aCol++)
			{
				if (theBoard->IsPickpocketTarget(theBoard->mGrid[aRow][aCol]))
					aTargets.push_back(theBoard->mGrid[aRow][aCol]);
			}
		}
	}
	if (aTargets.empty())
		return false;
	Piece* aPiece = aTargets[mRand() % aTargets.size()];
	aX = theBoard->mX + (int) aPiece->mX + 14;
	aY = theBoard->mY + (int) aPiece->mY + 14;
	char anArgs[32];
	snprintf(anArgs, sizeof(anArgs), "%d %d", aX, aY);
	if (mPocketsUsed++ % 2 == 1)
	{
		if (!theBoard->mPickpocketArmed)
			Queue("char", "p");
		Click(aX, aY, "pickpocket: p and a click");
	}
	else
	{
		Queue("move", anArgs);
		Queue("rclick", anArgs);
		if (mLog != NULL)
			fprintf(mLog, "# pickpocket: right click\n");
	}
	mNextAction = theNow + ClickDelay();
	return true;
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
			int aKey = theBoard->SolverGroupKey(aCol, aRow + theBoard->SolverRowOffset());
			if ((aKey < 0) || (theBoard->mSolverVerdict[aKey] == PuzzleClickCheck::VERDICT_NONE))
				return -1;
			return theBoard->mSolverVerdict[aKey];
		}
	}
	return -1;
}
