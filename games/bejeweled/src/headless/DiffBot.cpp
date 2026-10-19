// port(variant): the difftest's player; see DiffBot.h. Not part of the original game (test code).

#include "DiffBot.h"
#include "Board.h"

#include <cstdlib>

using namespace Sexy;

DiffBot::DiffBot(uint32_t theSeed) :
	mMoves(0),
	mHints(0),
	mPauses(0),
	mNewGames(0),
	mRand((std::mt19937::result_type) theSeed),
	mPlanPos(0),
	mWait(0),
	mDragX(0),
	mDragY(0),
	mModalOpen(false),
	mNewGameMove(-1),
	mTimedThink(0)
{
	// One game in 40 asks for New Game after a few moves (its end is then New Game's, not a game over)
	if (Random(40) == 0)
		mNewGameMove = 3 + Random(30);
	// A timed game: one in four as quick as the normal games (they reach high levels), one slower, half much
	// slower (their time runs out on the first levels)
	int aPace = Random(4);
	mTimedThink = (aPace == 0) ? 0 : (aPace == 1) ? 40 + Random(160) : 500 + Random(1000);
}

int DiffBot::Random(int theRange)
{
	return (int) (mRand() % (unsigned) theRange);
}

void DiffBot::AddTargetSteps(int theCol, int theRow, int theCol2, int theRow2, int theDelay)
{
	int aDirX = theCol2 - theCol;
	int aDirY = theRow2 - theRow;
	int aStyle = Random(10);

	Step aSelect = {STEP_CLICK_CELL, ROLE_SELECT, theCol, theRow, Random(17) - 8, Random(17) - 8, theDelay};
	if (aStyle < 6)
	{
		// a click on both gems
		Step aTarget = {STEP_CLICK_CELL, ROLE_TARGET, theCol2, theRow2, Random(17) - 8, Random(17) - 8, 3 + Random(20)};
		mPlan.push_back(aSelect);
		mPlan.push_back(aTarget);
	}
	else if (aStyle < 8)
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
		// a drag of 10..45 px toward the other gem (MouseDrag; under 16 px it only selects)
		int aMajor = 10 + Random(36);
		int aMinor = Random(11) - 5;
		aSelect.mKind = STEP_DOWN_CELL;
		Step aMove = {STEP_DRAG_MOVE, ROLE_ANY, 0, 0, aDirX * aMajor + aDirY * aMinor, aDirY * aMajor + aDirX * aMinor, 1 + Random(4)};
		Step anUp = {STEP_DRAG_UP, ROLE_ANY, 0, 0, 0, 0, 1 + Random(3)};
		mPlan.push_back(aSelect);
		mPlan.push_back(aMove);
		mPlan.push_back(anUp);
	}
}

// STATE_PLAYING, nothing selected: a valid move (88%) or a random swap (12%, usually illegal)
void DiffBot::PlanMove(Board* theBoard)
{
	static const int aDirs[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
	std::vector<int> aMoves;
	if (Random(100) < 12)
	{
		for (int aRow = 0; aRow < theBoard->mRows; aRow++)
		{
			for (int aCol = 0; aCol < theBoard->mCols; aCol++)
			{
				for (int aDir = 0; aDir < 4; aDir++)
				{
					int aCol2 = aCol + aDirs[aDir][0];
					int aRow2 = aRow + aDirs[aDir][1];
					if ((aCol2 >= 0) && (aCol2 < theBoard->mCols) && (aRow2 >= 0) && (aRow2 < theBoard->mRows))
						aMoves.push_back(((aCol * 16 + aRow) * 16 + aCol2) * 16 + aRow2);
				}
			}
		}
	}
	else
	{
		int aMove[4];
		for (int i = 0; theBoard->FindMove(aMove, i, true, true); i++)
			aMoves.push_back(((aMove[0] * 16 + aMove[1]) * 16 + aMove[2]) * 16 + aMove[3]);
	}
	if (aMoves.empty())
		return;
	int aPacked = aMoves[Random((int) aMoves.size())];
	mMoves++;
	int aThink = 5 + Random(40) + (theBoard->mTimedMode ? Random(mTimedThink + 1) : 0);
	AddTargetSteps(aPacked >> 12, (aPacked >> 8) & 15, (aPacked >> 4) & 15, aPacked & 15, aThink);
}

// STATE_PLAYING with a selection left from before (a selection made while the board fell): a gem next to its
// cell (a valid move from there when there is one), or a right click that drops it
void DiffBot::PlanTarget(Board* theBoard)
{
	static const int aDirs[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
	Piece* aSelected = theBoard->mSelectedPiece;
	int aCol = aSelected->mCol;
	int aRow = aSelected->mRow;
	if (Random(8) == 0)
	{
		Step aStep = {STEP_RCLICK_CELL, ROLE_ANY, aCol, aRow, 0, 0, 3 + Random(30)};
		mPlan.push_back(aStep);
		return;
	}

	std::vector<int> aTargets;
	bool isStale = (aCol < 0) || (aCol >= theBoard->mCols) || (aRow < 0) || (aRow >= theBoard->mRows) || (theBoard->mBoard[aRow][aCol] != aSelected);
	if (!isStale && (Random(10) < 6))
	{
		int aMove[4];
		for (int i = 0; theBoard->FindMove(aMove, i, true, true); i++)
		{
			if ((aMove[0] == aCol) && (aMove[1] == aRow))
				aTargets.push_back(aMove[3] * 16 + aMove[2]);
		}
	}
	if (aTargets.empty())
	{
		for (int aDir = 0; aDir < 4; aDir++)
		{
			int aCol2 = aCol + aDirs[aDir][0];
			int aRow2 = aRow + aDirs[aDir][1];
			if ((aCol2 >= 0) && (aCol2 < theBoard->mCols) && (aRow2 >= 0) && (aRow2 < theBoard->mRows))
				aTargets.push_back(aRow2 * 16 + aCol2);
		}
	}
	if (aTargets.empty())
		return;
	int aCell = aTargets[Random((int) aTargets.size())];
	mMoves++;
	Step aStep = {STEP_CLICK_CELL, ROLE_TARGET, aCell % 16, aCell / 16, Random(17) - 8, Random(17) - 8, 3 + Random(30)};
	mPlan.push_back(aStep);
}

void DiffBot::Plan(Board* theBoard)
{
	int aState = theBoard->mState;
	if (mModalOpen)
	{
		Step aStep = {STEP_MODAL_CLOSE, ROLE_ANY, 0, 0, 0, 0, 20 + Random(200)};
		mPlan.push_back(aStep);
		return;
	}
	if (theBoard->mUserPauseCount > 0)
	{
		Step aStep = {(Random(2) == 0) ? STEP_KEY : STEP_UNPAUSE_CLICK, ROLE_ANY, 0, 0, 0, 0, 10 + Random(100)};
		mPlan.push_back(aStep);
		return;
	}
	if ((aState == STATE_GAME_OVER) || (aState == STATE_GAME_OVER_DONE))
		return;

	if (aState == STATE_PLAYING)
	{
		if ((mNewGameMove >= 0) && (mMoves >= mNewGameMove))
		{
			mNewGameMove = -1;
			Step aStep = {STEP_NEW_GAME, ROLE_ANY, 0, 0, 0, 0, 5 + Random(40)};
			mPlan.push_back(aStep);
			return;
		}
		if (theBoard->mSelectedPiece != NULL)
		{
			PlanTarget(theBoard);
			return;
		}
		int aKind = Random(1000);
		if (aKind < 25)
		{
			Step aStep = {STEP_HINT, ROLE_ANY, 0, 0, 0, 0, 5 + Random(40)};
			mPlan.push_back(aStep);
		}
		else if (aKind < 40)
		{
			Step aStep = {STEP_KEY, ROLE_ANY, 0, 0, 0, 0, 5 + Random(40)};
			mPlan.push_back(aStep);
		}
		else if (aKind < 50)
		{
			Step aStep = {STEP_MODAL_OPEN, ROLE_ANY, 0, 0, 0, 0, 5 + Random(40)};
			mPlan.push_back(aStep);
		}
		else
			PlanMove(theBoard);
		return;
	}

	// While the board falls, clears or falls out: now and then a selection (a fall-out leaves it stale) or a pause
	if (((aState == STATE_FALLING) || (aState == STATE_CLEARING) || (aState == STATE_FALL_OUT)) && (theBoard->mSelectedPiece == NULL) &&
		(Random(150) == 0))
	{
		std::vector<int> aCells;
		for (int aRow = 0; aRow < theBoard->mRows; aRow++)
		{
			for (int aCol = 0; aCol < theBoard->mCols; aCol++)
			{
				if (theBoard->mBoard[aRow][aCol] != NULL)
					aCells.push_back(aRow * 16 + aCol);
			}
		}
		if (!aCells.empty())
		{
			int aCell = aCells[Random((int) aCells.size())];
			Step aStep = {STEP_CLICK_CELL, ROLE_SELECT_ANY, aCell % 16, aCell / 16, 0, 0, 1};
			mPlan.push_back(aStep);
		}
	}
	else if (Random(2000) == 0)
	{
		Step aStep = {STEP_KEY, ROLE_ANY, 0, 0, 0, 0, 1};
		mPlan.push_back(aStep);
	}
}

// The input of a due step, or false if the board has moved on and the plan is void
bool DiffBot::RunStep(Board* theBoard, const Step& theStep)
{
	int aState = theBoard->mState;
	bool hasSelection = theBoard->mSelectedPiece != NULL;
	switch (theStep.mRole)
	{
	case ROLE_SELECT:
		if ((aState != STATE_PLAYING) || hasSelection)
			return false;
		break;
	case ROLE_SELECT_ANY:
		if ((aState == STATE_GAME_OVER) || (aState == STATE_GAME_OVER_DONE) || (aState == STATE_BLACK_HOLE) || (aState == STATE_LEVEL_WIPE) ||
			hasSelection)
			return false;
		break;
	case ROLE_TARGET:
		if ((aState != STATE_PLAYING) || !hasSelection)
			return false;
		break;
	}

	switch (theStep.mKind)
	{
	case STEP_CLICK_CELL:
	case STEP_RCLICK_CELL:
	case STEP_DOWN_CELL:
		{
			if ((theStep.mCol >= theBoard->mCols) || (theStep.mRow >= theBoard->mRows))
				return false;
			Piece* aPiece = theBoard->mBoard[theStep.mRow][theStep.mCol];
			if (aPiece == NULL)
				return false;
			int x = (int) aPiece->mX + 24 + theStep.mDX;
			int y = (int) aPiece->mY + 24 + theStep.mDY;
			theBoard->MouseMove(x, y);
			if (theStep.mKind == STEP_DOWN_CELL)
			{
				mDragX = x;
				mDragY = y;
				theBoard->MouseDown(x, y, 1);
			}
			else
			{
				int aClickCount = (theStep.mKind == STEP_RCLICK_CELL) ? -1 : 1;
				theBoard->MouseDown(x, y, aClickCount);
				theBoard->MouseUp(x, y, aClickCount);
			}
		}
		return true;

	case STEP_CLICK_ZONE:
		{
			Piece* aSelected = theBoard->mSelectedPiece;
			int x = (int) aSelected->mX + 24 + theStep.mDX;
			int y = (int) aSelected->mY + 24 + theStep.mDY;
			theBoard->MouseMove(x, y);
			theBoard->MouseDown(x, y, 1);
			theBoard->MouseUp(x, y, 1);
		}
		return true;

	case STEP_DRAG_MOVE:
		mDragX += theStep.mDX;
		mDragY += theStep.mDY;
		theBoard->MouseDrag(mDragX, mDragY);
		return true;

	case STEP_DRAG_UP:
		theBoard->MouseUp(mDragX, mDragY, 1);
		return true;

	case STEP_HINT:
		if (aState != STATE_PLAYING)
			return false;
		mHints++;
		theBoard->ShowHint();
		return true;

	case STEP_KEY:
		mPauses++;
		theBoard->KeyChar('p');
		return true;

	case STEP_UNPAUSE_CLICK:
		theBoard->MouseDown(320, 240, 1);
		theBoard->MouseUp(320, 240, 1);
		return true;

	case STEP_MODAL_OPEN:
		mPauses++;
		mModalOpen = true;
		theBoard->Pause(theBoard->mTimedMode);
		return true;

	case STEP_MODAL_CLOSE:
		mModalOpen = false;
		theBoard->Unpause(theBoard->mTimedMode);
		return true;

	case STEP_NEW_GAME:
		if (aState != STATE_PLAYING)
			return false;
		mNewGames++;
		theBoard->RequestNewGame();
		return true;
	}
	return false;
}

void DiffBot::BeforeUpdate(Board* theBoard)
{
	if (mPlanPos >= mPlan.size())
	{
		mPlan.clear();
		mPlanPos = 0;
		Plan(theBoard);
		if (mPlan.empty())
			return;
		mWait = mPlan[0].mDelay;
	}
	if (mWait > 0)
	{
		mWait--;
		return;
	}

	// A drag in progress always finishes, a pause always ends; anything else waits while the board takes no input
	const Step& aStep = mPlan[mPlanPos];
	bool isAlways = (aStep.mKind == STEP_DRAG_MOVE) || (aStep.mKind == STEP_DRAG_UP) || (aStep.mKind == STEP_KEY) ||
		(aStep.mKind == STEP_UNPAUSE_CLICK) || (aStep.mKind == STEP_MODAL_CLOSE);
	if (!isAlways && ((theBoard->mPauseCount > 0) || (theBoard->mUserPauseCount > 0) || (theBoard->mInputDelay > 0)))
		return;

	if (RunStep(theBoard, aStep))
	{
		mPlanPos++;
		if (mPlanPos < mPlan.size())
			mWait = mPlan[mPlanPos].mDelay;
	}
	else
	{
		mPlan.clear();
		mPlanPos = 0;
	}
}
