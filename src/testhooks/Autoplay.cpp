// port: test hook for -autoplay=<seed> (see Autoplay.h and WinDMApp.h). Not part of the original game.

#include "Autoplay.h"
#include "game/WinDMApp.h"
#include "game/Board.h"
#include "game/TitleScreen.h"
#include "WidgetManager.h"
#include "HyperlinkWidget.h"

#include <SDL.h>
#include <cstdarg>

using namespace Sexy;

static const char* const gStateNames[] =
{
	"falling", "playing", "swapping", "swapping back", "clearing", "game over", "game over done",
	"vortex", "level wipe", "fall out"
};

static const char* StateName(int theState)
{
	return ((theState >= 0) && (theState < (int) (sizeof(gStateNames) / sizeof(gStateNames[0])))) ? gStateNames[theState] : "?";
}

AutoplayBot::AutoplayBot(WinDMApp* theApp, unsigned long theSeed, bool timedMode, int theUpdateCap, const std::string& theLogFileName) :
	mApp(theApp),
	mRand((std::mt19937::result_type) theSeed),
	mSeed(theSeed),
	mWantTimed(timedMode),
	mUpdateCap(theUpdateCap),
	mLog(NULL),
	mDone(false),
	mTitleClicked(false),
	mTimedClicked(false),
	mPlanPos(0),
	mNextStepUpdate(0),
	mDragX(0),
	mDragY(0),
	mGameOverUpdate(-1),
	mMoveNum(0),
	mLastBoard(NULL),
	mLastState(-1),
	mLastLevel(-1)
{
	if (!theLogFileName.empty())
	{
		mLog = fopen(theLogFileName.c_str(), "wb");
		if (mLog == NULL)
			SDL_Log("autoplay: cannot write %s", theLogFileName.c_str());
	}

	Note("-autoplay=%lu -seed=%lu%s -autoplaycap=%d: the bot's input as a -script file", mSeed, (unsigned long) mApp->mRandSeed,
		mWantTimed ? " -autoplaytimed" : "", mUpdateCap);
}

AutoplayBot::~AutoplayBot()
{
	if (mLog != NULL)
		fclose(mLog);
}

int AutoplayBot::Random(int theRange)
{
	return (int) (mRand() % (unsigned) theRange);
}

// A comment line in the log (the state after the current update)
void AutoplayBot::Note(const char* theFormat, ...)
{
	if (mLog == NULL)
		return;

	va_list anArgs;
	va_start(anArgs, theFormat);
	fprintf(mLog, "# update %d: ", mApp->mUpdateCount);
	vfprintf(mLog, theFormat, anArgs);
	fprintf(mLog, "\n");
	va_end(anArgs);
}

// Queues one script command for the next update, and logs it as a script line
void AutoplayBot::Queue(const std::string& theCommand, int theX, int theY)
{
	int anUpdate = mApp->mUpdateCount + 1;
	std::string anArgs;
	if (theCommand != "quit")
		anArgs = std::to_string(theX) + " " + std::to_string(theY);
	mApp->AddScriptCommand(anUpdate, theCommand, anArgs);

	if (mLog != NULL)
		fprintf(mLog, "%d %s%s%s\n", anUpdate, theCommand.c_str(), anArgs.empty() ? "" : " ", anArgs.c_str());
}

void AutoplayBot::LogChanges()
{
	Board* aBoard = mApp->mBoard;
	if (aBoard != mLastBoard)
	{
		if (aBoard != NULL)
			Note("new board (%s)", aBoard->mTimedMode ? "timed" : "normal");
		mLastBoard = aBoard;

		// A plan made on the old board is void (a drag in progress still ends)
		if ((mPlanPos < mPlan.size()) && (mPlan[mPlanPos].mKind != STEP_DRAG_MOVE) && (mPlan[mPlanPos].mKind != STEP_DRAG_UP))
		{
			mPlan.clear();
			mPlanPos = 0;
		}
		mLastState = -1;
		mLastLevel = -1;
	}

	if ((aBoard != NULL) && ((aBoard->mState != mLastState) || (aBoard->mDisplayLevel != mLastLevel)))
	{
		Note("%s -> %s, level %d, score %d, board tick %d, Rand() draws %u", StateName(mLastState), StateName(aBoard->mState),
			aBoard->mDisplayLevel, aBoard->mScore, aBoard->mUpdateCnt, GetRandCount());
		mLastState = aBoard->mState;
		mLastLevel = aBoard->mDisplayLevel;
	}
}

// The board takes no click now: paused, the input delay after an unpause, or a dialog in front
// (the non-modal first-game instructions do not count, OnBoard handles them)
bool AutoplayBot::InputBlocked()
{
	Board* aBoard = mApp->mBoard;
	if ((aBoard == NULL) || (aBoard->mPauseCount > 0) || (aBoard->mUserPauseCount > 0) || (aBoard->mInputDelay > 0))
		return true;

	for (DialogMap::iterator anItr = mApp->mDialogMap.begin(); anItr != mApp->mDialogMap.end(); ++anItr)
	{
		if (anItr->first != DIALOG_INSTRUCTIONS)
			return true;
	}
	return false;
}

// A click there reaches the board (not a dialog or a button over it)
bool AutoplayBot::OnBoard(int theX, int theY)
{
	return mApp->mWidgetManager->GetWidgetAt(theX, theY, NULL, NULL) == mApp->mBoard;
}

void AutoplayBot::AddTargetSteps(int theCol, int theRow, int theCol2, int theRow2, int theStyle, int theDelay)
{
	int aDirX = theCol2 - theCol;
	int aDirY = theRow2 - theRow;

	Step aSelect = {STEP_CLICK_CELL, ROLE_SELECT, theCol, theRow, Random(17) - 8, Random(17) - 8, theDelay};
	if (theStyle < 6)
	{
		// click both gems
		Step aTarget = {STEP_CLICK_CELL, ROLE_TARGET, theCol2, theRow2, Random(17) - 8, Random(17) - 8, 3 + Random(20)};
		mPlan.push_back(aSelect);
		mPlan.push_back(aTarget);
	}
	else if (theStyle < 8)
	{
		// the second click 22..61 px from the selected gem's centre toward the other gem (MouseDown's zone)
		int aMajor = 22 + Random(40);
		int aMinor = Random(21) - 10;
		Step aTarget = {STEP_CLICK_ZONE, ROLE_TARGET, 0, 0, aDirX * aMajor + aDirY * aMinor, aDirY * aMajor + aDirX * aMinor, 3 + Random(20)};
		mPlan.push_back(aSelect);
		mPlan.push_back(aTarget);
	}
	else
	{
		// drag 16..45 px toward the other gem (MouseDrag)
		int aMajor = 16 + Random(30);
		int aMinor = Random(11) - 5;
		aSelect.mKind = STEP_DOWN_CELL;
		Step aMove = {STEP_DRAG_MOVE, ROLE_ANY, 0, 0, aDirX * aMajor + aDirY * aMinor, aDirY * aMajor + aDirX * aMinor, 1 + Random(4)};
		Step anUp = {STEP_DRAG_UP, ROLE_ANY, 0, 0, 0, 0, 1 + Random(3)};
		mPlan.push_back(aSelect);
		mPlan.push_back(aMove);
		mPlan.push_back(anUp);
	}
}

// STATE_PLAYING, nothing selected: a valid move (88%), a random swap (9%, usually illegal) or a hint (3%)
void AutoplayBot::PlanMove()
{
	Board* aBoard = mApp->mBoard;
	int aKind = Random(100);
	int aThink = 5 + Random(40);

	if (aKind < 3)
	{
		Note("move %d: hint", ++mMoveNum);
		Step aHint = {STEP_CLICK_HINT, ROLE_ANY, 0, 0, 0, 0, aThink};
		mPlan.push_back(aHint);
		return;
	}

	// Only moves whose two gems take a click (the first game's instructions cover the top rows)
	std::vector<int> aMoves;
	const char* aKindName;
	if (aKind < 12)
	{
		static const int aDirs[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
		for (int aCol = 0; aCol < Board::BOARD_COLS; aCol++)
		{
			for (int aRow = 0; aRow < Board::BOARD_ROWS; aRow++)
			{
				for (int aDir = 0; aDir < 4; aDir++)
				{
					int aCol2 = aCol + aDirs[aDir][0];
					int aRow2 = aRow + aDirs[aDir][1];
					if ((aCol2 >= 0) && (aCol2 < Board::BOARD_COLS) && (aRow2 >= 0) && (aRow2 < Board::BOARD_ROWS))
						aMoves.push_back(((aCol * 8 + aRow) * 8 + aCol2) * 8 + aRow2);
				}
			}
		}
		aKindName = "random";
	}
	else
	{
		int aMove[4];
		for (int i = 0; aBoard->FindMove(aMove, i, true, true); i++)
			aMoves.push_back(((aMove[0] * 8 + aMove[1]) * 8 + aMove[2]) * 8 + aMove[3]);
		aKindName = "valid";
	}

	for (size_t i = 0; i < aMoves.size(); )
	{
		int aMove = aMoves[i];
		if ((OnBoard(Board::GetColX(aMove >> 9) + 24, Board::GetRowY((aMove >> 6) & 7) + 24)) &&
			(OnBoard(Board::GetColX((aMove >> 3) & 7) + 24, Board::GetRowY(aMove & 7) + 24)))
			i++;
		else
			aMoves.erase(aMoves.begin() + i);
	}
	if (aMoves.empty())
		return;

	int aPacked = aMoves[Random((int) aMoves.size())];
	int aMove[4] = {aPacked >> 9, (aPacked >> 6) & 7, (aPacked >> 3) & 7, aPacked & 7};

	int aStyle = Random(10);
	Note("move %d: (%d,%d)->(%d,%d) %s, %s", ++mMoveNum, aMove[0], aMove[1], aMove[2], aMove[3], aKindName,
		(aStyle < 6) ? "click" : (aStyle < 8) ? "zone click" : "drag");
	AddTargetSteps(aMove[0], aMove[1], aMove[2], aMove[3], aStyle, aThink);
}

// While the board falls, clears or falls out: select a random gem (a fall-out leaves it stale)
void AutoplayBot::PlanFallingSelect()
{
	Board* aBoard = mApp->mBoard;
	std::vector<int> aCells;
	for (int aRow = 0; aRow < Board::BOARD_ROWS; aRow++)
	{
		for (int aCol = 0; aCol < Board::BOARD_COLS; aCol++)
		{
			if (aBoard->mBoard[aRow][aCol] != NULL)
				aCells.push_back(aRow * Board::BOARD_COLS + aCol);
		}
	}
	if (aCells.empty())
		return;

	int aCell = aCells[Random((int) aCells.size())];
	Note("select (%d,%d) while %s", aCell % Board::BOARD_COLS, aCell / Board::BOARD_COLS, StateName(aBoard->mState));
	Step aSelect = {STEP_CLICK_CELL, ROLE_SELECT_ANY, aCell % Board::BOARD_COLS, aCell / Board::BOARD_COLS, 0, 0, 1};
	mPlan.push_back(aSelect);
}

// STATE_PLAYING with a selection left from before: click a gem next to its cell, a valid move when
// there is one from that cell (60%). A stale selection is not in the grid (and its gem fell off the
// screen), so the click lands on the target gem itself. With no neighbour that takes a click, the
// selected gem is clicked again, which deselects it.
void AutoplayBot::PlanTarget(Piece* theSelected)
{
	static const int aDirs[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
	Board* aBoard = mApp->mBoard;
	int aCol = theSelected->mCol;
	int aRow = theSelected->mRow;
	bool isStale = aBoard->mBoard[aRow][aCol] != theSelected;

	std::vector<int> aTargets;
	if ((Random(10) < 6) && (!isStale))
	{
		int aMove[4];
		for (int i = 0; aBoard->FindMove(aMove, i, true, true); i++)
		{
			if ((aMove[0] == aCol) && (aMove[1] == aRow))
				aTargets.push_back(aMove[3] * Board::BOARD_COLS + aMove[2]);
		}
	}
	if (aTargets.empty())
	{
		int aDir = Random(4);
		for (int i = 0; i < 4; i++, aDir = (aDir + 1) % 4)
		{
			int aCol2 = aCol + aDirs[aDir][0];
			int aRow2 = aRow + aDirs[aDir][1];
			if ((aCol2 >= 0) && (aCol2 < Board::BOARD_COLS) && (aRow2 >= 0) && (aRow2 < Board::BOARD_ROWS))
				aTargets.push_back(aRow2 * Board::BOARD_COLS + aCol2);
		}
	}

	for (size_t i = 0; i < aTargets.size(); )
	{
		int aCell = aTargets[i];
		if (OnBoard(Board::GetColX(aCell % Board::BOARD_COLS) + 24, Board::GetRowY(aCell / Board::BOARD_COLS) + 24))
			i++;
		else
			aTargets.erase(aTargets.begin() + i);
	}

	int aTarget[2] = {aCol, aRow};
	if (!aTargets.empty())
	{
		int aCell = aTargets[Random((int) aTargets.size())];
		aTarget[0] = aCell % Board::BOARD_COLS;
		aTarget[1] = aCell / Board::BOARD_COLS;
	}
	else if (isStale)
		return;

	Note("move %d: selected (%d,%d)%s -> (%d,%d)", ++mMoveNum, aCol, aRow, isStale ? " (stale)" : "", aTarget[0], aTarget[1]);
	Step aStep = {STEP_CLICK_CELL, ROLE_TARGET, aTarget[0], aTarget[1], Random(17) - 8, Random(17) - 8, 3 + Random(30)};
	mPlan.push_back(aStep);
}

// Queues the input of a due step, or returns false if the game has moved on and the plan is void
bool AutoplayBot::RunStep(const Step& theStep)
{
	Board* aBoard = mApp->mBoard;
	if (aBoard == NULL)
		return false;

	int aState = aBoard->mState;
	bool hasSelection = aBoard->mSelectedPiece != NULL;
	switch (theStep.mRole)
	{
	case ROLE_SELECT:
		if ((aState != STATE_PLAYING) || (hasSelection))
			return false;
		break;
	case ROLE_SELECT_ANY:
		if ((aState == STATE_GAME_OVER) || (aState == STATE_GAME_OVER_DONE) || (aState == STATE_BLACK_HOLE) ||
			(aState == STATE_LEVEL_WIPE) || (hasSelection))
			return false;
		break;
	case ROLE_TARGET:
		if ((aState != STATE_PLAYING) || (!hasSelection))
			return false;
		break;
	}

	switch (theStep.mKind)
	{
	case STEP_CLICK_CELL:
	case STEP_DOWN_CELL:
		{
			Piece* aPiece = aBoard->mBoard[theStep.mRow][theStep.mCol];
			if (aPiece == NULL)
				return false;
			int x = (int) aPiece->mX + 24 + theStep.mDX;
			int y = (int) aPiece->mY + 24 + theStep.mDY;
			if (!OnBoard(x, y))
				return false;

			if (theStep.mKind == STEP_DOWN_CELL)
			{
				mDragX = x;
				mDragY = y;
				Queue("down", x, y);
			}
			else
				Queue("click", x, y);
		}
		return true;

	case STEP_CLICK_ZONE:
		{
			Piece* aSelected = aBoard->mSelectedPiece;
			int x = (int) aSelected->mX + 24 + theStep.mDX;
			int y = (int) aSelected->mY + 24 + theStep.mDY;
			if (!OnBoard(x, y))
				return false;
			Queue("click", x, y);
		}
		return true;

	case STEP_DRAG_MOVE:
		mDragX += theStep.mDX;
		mDragY += theStep.mDY;
		Queue("move", mDragX, mDragY);
		return true;

	case STEP_DRAG_UP:
		Queue("up", mDragX, mDragY);
		return true;

	case STEP_CLICK_HINT:
		if (aState != STATE_PLAYING)
			return false;
		for (std::list<ButtonWidget*>::iterator anItr = mApp->mButtonList.begin(); anItr != mApp->mButtonList.end(); ++anItr)
		{
			if ((*anItr)->mId == BUTTON_HINT)
			{
				Queue("click", (*anItr)->mX + (*anItr)->mWidth / 2, (*anItr)->mY + (*anItr)->mHeight / 2);
				return true;
			}
		}
		return false;
	}

	return false;
}

void AutoplayBot::Update()
{
	LogChanges();
	if (mDone)
		return;

	Board* aBoard = mApp->mBoard;
	int aNext = mApp->mUpdateCount + 1;

	if (aNext >= mUpdateCap)
	{
		Note("update cap");
		Queue("quit", 0, 0);
		mDone = true;
		return;
	}

	// Game over: quit once the high-score screen has been up for a second
	if ((aBoard != NULL) && (aBoard->mState == STATE_GAME_OVER_DONE))
	{
		if (mGameOverUpdate < 0)
			mGameOverUpdate = mApp->mUpdateCount;
		if (mApp->mUpdateCount >= mGameOverUpdate + 100)
		{
			Queue("quit", 0, 0);
			mDone = true;
		}
		return;
	}

	if (mPlanPos >= mPlan.size())
	{
		mPlan.clear();
		mPlanPos = 0;

		if (mApp->mTitleScreen != NULL)
		{
			HyperlinkWidget* aLink = mApp->mTitleScreen->mPlayLink;
			if ((!mTitleClicked) && (mApp->mTitleScreen->mLoadingComplete) && (aLink->mVisible))
			{
				Queue("click", aLink->mX + aLink->mWidth / 2, aLink->mY + aLink->mHeight / 2);
				mTitleClicked = true;
			}
			return;
		}

		if ((aBoard == NULL) || (InputBlocked()))
			return;

		if ((mWantTimed) && (!aBoard->mTimedMode))
		{
			// A new game switches without a confirmation
			ButtonWidget* aButton = mApp->mTimedModeButton;
			if ((!mTimedClicked) && (aBoard->mIsNewGame) && (aButton != NULL))
			{
				Queue("click", aButton->mX + aButton->mWidth / 2, aButton->mY + aButton->mHeight / 2);
				mTimedClicked = true;
			}
			return;
		}

		if (aBoard->mState == STATE_PLAYING)
		{
			if (aBoard->mSelectedPiece != NULL)
				PlanTarget(aBoard->mSelectedPiece);
			else
				PlanMove();
		}
		else if (((aBoard->mState == STATE_FALLING) || (aBoard->mState == STATE_CLEARING) || (aBoard->mState == STATE_FALL_OUT)) &&
			(aBoard->mSelectedPiece == NULL) && (Random(200) == 0))
			PlanFallingSelect();

		if (mPlan.empty())
			return;
		mNextStepUpdate = aNext + mPlan[0].mDelay;
	}

	if (aNext < mNextStepUpdate)
		return;

	// A drag in progress always finishes; anything else waits while the board takes no input
	const Step& aStep = mPlan[mPlanPos];
	if ((aStep.mKind != STEP_DRAG_MOVE) && (aStep.mKind != STEP_DRAG_UP) && (InputBlocked()))
		return;

	if (RunStep(aStep))
	{
		mPlanPos++;
		if (mPlanPos < mPlan.size())
			mNextStepUpdate = aNext + mPlan[mPlanPos].mDelay;
	}
	else
	{
		Note("plan dropped (%s)", StateName((aBoard != NULL) ? aBoard->mState : -1));
		mPlan.clear();
		mPlanPos = 0;
	}
}
