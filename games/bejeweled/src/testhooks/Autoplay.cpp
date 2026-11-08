// port: test hook for -autoplay=<seed> (see Autoplay.h and WinDMApp.h). Not part of the original game.

#include "Autoplay.h"
#include "WinDMApp.h"
#include "Board.h"
#include "TitleScreen.h"
#include "GameConfig.h"
#include "WidgetManager.h"
#include "HyperlinkWidget.h"

#include <SDL.h>
#include <cmath>
#include <cstdarg>

using namespace Sexy;

static const char* const gStateNames[] =
{
	"falling", "playing", "swapping", "swapping back", "clearing", "game over", "game over done",
	"vortex", "level wipe", "fall out", "twisting", "twisting back", "sliding", "sliding back", "arrow stepping"
};

static const char* StateName(int theState)
{
	return ((theState >= 0) && (theState < (int) (sizeof(gStateNames) / sizeof(gStateNames[0])))) ? gStateNames[theState] : "?";
}

AutoplayBot::AutoplayBot(WinDMApp* theApp, unsigned long theSeed, bool timedMode, int theUpdateCap, const std::string& theLogFileName,
	const std::string& theShotFileName) :
	mApp(theApp),
	mRand((std::mt19937::result_type) theSeed),
	mSeed(theSeed),
	mWantTimed(timedMode),
	mUpdateCap(theUpdateCap),
	mLog(NULL),
	mShotFileName(theShotFileName),
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

	// port(variant): a seed the player chose (-gameseed) is part of the run, too
	std::string aGameSeed = (gGameConfig.mPendingSeed != 0) ? " -gameseed=" + std::to_string(gGameConfig.mPendingSeed) : "";
	Note("-autoplay=%lu -seed=%lu%s%s -autoplaycap=%d: the bot's input as a -script file", mSeed, (unsigned long) mApp->mRandSeed,
		aGameSeed.c_str(), mWantTimed ? " -autoplaytimed" : "", mUpdateCap);
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
	Queue(theCommand, (theCommand != "quit") ? std::to_string(theX) + " " + std::to_string(theY) : std::string());
}

void AutoplayBot::Queue(const std::string& theCommand, const std::string& theArgs)
{
	int anUpdate = mApp->mUpdateCount + 1;
	mApp->AddScriptCommand(anUpdate, theCommand, theArgs);

	if (mLog != NULL)
		fprintf(mLog, "%d %s%s%s\n", anUpdate, theCommand.c_str(), theArgs.empty() ? "" : " ", theArgs.c_str());
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

// A board position (Board::GetColX, Piece::mX) in the window: on a scaled board (stage 7) scaled about the board
// window's corner, as BoardTrace prints its clicks; else itself
int AutoplayBot::ToScreenX(WinDMApp* theApp, int theX)
{
	Board* aBoard = theApp->mBoard;
	return aBoard->mVariant.mScaled ? (int) floor(aBoard->mLayout.ToScreenX(theX) + 0.5) : theX;
}

int AutoplayBot::ToScreenY(WinDMApp* theApp, int theY)
{
	Board* aBoard = theApp->mBoard;
	return aBoard->mVariant.mScaled ? (int) floor(aBoard->mLayout.ToScreenY(theY) + 0.5) : theY;
}

// A click at that board position reaches the board (not a dialog or a button over it)
bool AutoplayBot::ClickReachesBoard(WinDMApp* theApp, int theX, int theY)
{
	return theApp->mWidgetManager->GetWidgetAt(ToScreenX(theApp, theX), ToScreenY(theApp, theY), NULL, NULL) == theApp->mBoard;
}

int AutoplayBot::ScreenX(int theX)
{
	return ToScreenX(mApp, theX);
}

int AutoplayBot::ScreenY(int theY)
{
	return ToScreenY(mApp, theY);
}

bool AutoplayBot::OnBoard(int theX, int theY)
{
	return ClickReachesBoard(mApp, theX, theY);
}

// The valid swaps whose two gems take a click (the first game's instructions cover the top rows)
void AutoplayBot::ValidClickableMoves(WinDMApp* theApp, std::vector<int>* theMoves)
{
	Board* aBoard = theApp->mBoard;
	theMoves->clear();
	int aMove[4];
	for (int i = 0; aBoard->FindMove(aMove, i, true, true); i++)
		theMoves->push_back(((aMove[0] * 16 + aMove[1]) * 16 + aMove[2]) * 16 + aMove[3]);
	for (size_t i = 0; i < theMoves->size(); )
	{
		int aPacked = (*theMoves)[i];
		if (ClickReachesBoard(theApp, aBoard->GetColX(aPacked >> 12) + 24, aBoard->GetRowY((aPacked >> 8) & 15) + 24) &&
			ClickReachesBoard(theApp, aBoard->GetColX((aPacked >> 4) & 15) + 24, aBoard->GetRowY(aPacked & 15) + 24))
			i++;
		else
			theMoves->erase(theMoves->begin() + i);
	}
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

// Arrow gems (stage 18): whether the swap of (theCol, theRow) and (theCol2, theRow2) puts the arrow at (theArrowCol,
// theArrowRow) in a line of three of its colour
static bool SwapMatchesArrow(Board* theBoard, int theCol, int theRow, int theCol2, int theRow2, int theArrowCol, int theArrowRow)
{
	auto aColorAt = [&](int aCol, int aRow) {
		if ((aCol == theCol) && (aRow == theRow))
		{
			aCol = theCol2;
			aRow = theRow2;
		}
		else if ((aCol == theCol2) && (aRow == theRow2))
		{
			aCol = theCol;
			aRow = theRow;
		}
		Piece* aPiece = theBoard->mBoard[aRow][aCol];
		return (aPiece != NULL) ? aPiece->mColor : -1;
	};
	int aCol = theArrowCol, aRow = theArrowRow;
	if ((aCol == theCol) && (aRow == theRow))
	{
		aCol = theCol2;
		aRow = theRow2;
	}
	else if ((aCol == theCol2) && (aRow == theRow2))
	{
		aCol = theCol;
		aRow = theRow;
	}
	int aColor = aColorAt(aCol, aRow);
	for (int anAxis = 0; anAxis < 2; anAxis++)
	{
		int dc = (anAxis == 0) ? 1 : 0, dr = 1 - dc;
		int aRun = 1;
		for (int c = aCol + dc, r = aRow + dr; (c < theBoard->mCols) && (r < theBoard->mRows) && (aColorAt(c, r) == aColor); c += dc, r += dr)
			aRun++;
		for (int c = aCol - dc, r = aRow - dr; (c >= 0) && (r >= 0) && (aColorAt(c, r) == aColor); c -= dc, r -= dr)
			aRun++;
		if (aRun >= 3)
			return true;
	}
	return false;
}

// Of the valid moves (packed as PlanMove packs them), the ones that clear the arrow closest to its edge, else any arrow;
// all of them if none does
static void PreferArrowMoves(Board* theBoard, std::vector<int>* theMoves)
{
	int aBest = -1;
	std::vector<int> aBestMoves, anyMoves;
	for (int aRow = 0; aRow < theBoard->mRows; aRow++)
	{
		for (int aCol = 0; aCol < theBoard->mCols; aCol++)
		{
			Piece* aPiece = theBoard->mBoard[aRow][aCol];
			if ((aPiece == NULL) || (aPiece->mArrow == 0))
				continue;
			int aDistance = theBoard->ArrowDistance(aCol, aRow, aPiece->mArrow - 1);
			std::vector<int> aClearing;
			for (int aMove : *theMoves)
			{
				if (SwapMatchesArrow(theBoard, aMove >> 12, (aMove >> 8) & 15, (aMove >> 4) & 15, aMove & 15, aCol, aRow))
					aClearing.push_back(aMove);
			}
			anyMoves.insert(anyMoves.end(), aClearing.begin(), aClearing.end());
			if (!aClearing.empty() && ((aBest < 0) || (aDistance < aBest)))
			{
				aBest = aDistance;
				aBestMoves = aClearing;
			}
		}
	}
	if (!aBestMoves.empty())
		*theMoves = aBestMoves;
}

// STATE_PLAYING, nothing selected: a valid move (88%), a random swap (9%, usually illegal) or a hint (3%). With arrow
// gems a valid move clears the arrow closest to its edge when one can
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
	if (aBoard->mVariant.mMoves && (aBoard->mRules.mMoves.mTypes != GameRules::MOVE_SWAP))
	{
		PlanVariantMove(aKind < 12, aThink);
		return;
	}

	// Only moves whose two gems take a click (the first game's instructions cover the top rows). A move is packed
	// 4 bits a coordinate; the board's own size and its playable cells (stage 7: on the original 8x8 the same list)
	std::vector<int> aMoves;
	const char* aKindName;
	if (aKind < 12)
	{
		static const int aDirs[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
		for (int aCol = 0; aCol < aBoard->mCols; aCol++)
		{
			for (int aRow = 0; aRow < aBoard->mRows; aRow++)
			{
				for (int aDir = 0; aDir < 4; aDir++)
				{
					int aCol2 = aCol + aDirs[aDir][0];
					int aRow2 = aRow + aDirs[aDir][1];
					if ((aCol2 >= 0) && (aCol2 < aBoard->mCols) && (aRow2 >= 0) && (aRow2 < aBoard->mRows) &&
						aBoard->mLayout.IsPlayable(aCol, aRow) && aBoard->mLayout.IsPlayable(aCol2, aRow2))
						aMoves.push_back(((aCol * 16 + aRow) * 16 + aCol2) * 16 + aRow2);
				}
			}
		}
		aKindName = "random";
	}
	else
	{
		ValidClickableMoves(mApp, &aMoves); // the list the test command "move" chooses from, too (TestCommands.cpp)
		aKindName = "valid";
	}

	for (size_t i = 0; i < aMoves.size(); )
	{
		int aMove = aMoves[i];
		if ((OnBoard(aBoard->GetColX(aMove >> 12) + 24, aBoard->GetRowY((aMove >> 8) & 15) + 24)) &&
			(OnBoard(aBoard->GetColX((aMove >> 4) & 15) + 24, aBoard->GetRowY(aMove & 15) + 24)))
			i++;
		else
			aMoves.erase(aMoves.begin() + i);
	}
	if (aMoves.empty())
		return;
	if (aBoard->mVariant.mArrows && (aKind >= 12))
		PreferArrowMoves(aBoard, &aMoves);

	int aPacked = aMoves[Random((int) aMoves.size())];
	int aMove[4] = {aPacked >> 12, (aPacked >> 8) & 15, (aPacked >> 4) & 15, aPacked & 15};

	int aStyle = Random(10);
	Note("move %d: (%d,%d)->(%d,%d) %s, %s", ++mMoveNum, aMove[0], aMove[1], aMove[2], aMove[3], aKindName,
		(aStyle < 6) ? "click" : (aStyle < 8) ? "zone click" : "drag");
	AddTargetSteps(aMove[0], aMove[1], aMove[2], aMove[3], aStyle, aThink);
}

// The input points of a move take a click: a swap's two gems, a twist's point where its four gems meet, a slide's
// drag from a cell's centre k cells on (or length - k back, when that is shorter), as BoardTrace prints them
bool AutoplayBot::VariantMoveOnBoard(const MoveSet::Move& theMove)
{
	Board* aBoard = mApp->mBoard;
	const MoveSet::Move& m = theMove;
	if (m.mType == GameRules::MOVE_SWAP)
		return OnBoard(aBoard->GetColX(m.mA) + 24, aBoard->GetRowY(m.mB) + 24) && OnBoard(aBoard->GetColX(m.mC) + 24, aBoard->GetRowY(m.mD) + 24);
	if (m.mType == GameRules::MOVE_TWIST)
		return OnBoard(aBoard->GetColX(m.mA + 1) - 1, aBoard->GetRowY(m.mB + 1) - 1);
	bool isRow = (m.mA == MoveSet::AXIS_ROW);
	bool isBack = (m.mD - m.mE < m.mE);
	int aFrom = isBack ? m.mC + m.mD - 1 : m.mC;
	int aTo = isBack ? aFrom - (m.mD - m.mE) : aFrom + m.mE;
	return OnBoard(aBoard->GetColX(isRow ? aFrom : m.mB) + 24, aBoard->GetRowY(isRow ? m.mB : aFrom) + 24) &&
		OnBoard(aBoard->GetColX(isRow ? aTo : m.mB) + 24, aBoard->GetRowY(isRow ? m.mB : aTo) + 24);
}

// PlanMove on a board with twists or slides (stage 10): a valid move of any type (MoveSet's search), or a random one
// of a random type the rules have (usually without a group). A swap as AddTargetSteps makes it (not as a drag where a
// drag slides); a twist as a click within 8 px of the point where its gems meet (13 px take it), a right click for
// one anticlockwise; a slide as a press within 8 px of a cell's centre and a drag of 1 to 3 moves along the line to
// k cells on, give or take 20 px (26 px would round to another k)
void AutoplayBot::PlanVariantMove(bool isRandom, int theDelay)
{
	Board* aBoard = mApp->mBoard;
	int aTypes = aBoard->mRules.mMoves.mTypes;
	std::vector<MoveSet::Move> aMoves;
	const char* aKindName = isRandom ? "random" : "valid";
	if (isRandom)
	{
		std::vector<int> aTypeList;
		for (int aType = GameRules::MOVE_SWAP; aType <= GameRules::MOVE_SLIDE; aType <<= 1)
		{
			if (aTypes & aType)
				aTypeList.push_back(aType);
		}
		int aType = aTypeList[Random((int) aTypeList.size())];
		for (int aRow = 0; aRow < aBoard->mRows; aRow++)
		{
			for (int aCol = 0; aCol < aBoard->mCols; aCol++)
			{
				MoveSet::Move aMove;
				aMove.mType = aType;
				if ((aType == GameRules::MOVE_SWAP) && aBoard->mLayout.IsPlayable(aCol, aRow))
				{
					aMove.mA = aCol;
					aMove.mB = aRow;
					if (aBoard->mLayout.IsPlayable(aCol + 1, aRow))
					{
						aMove.mC = aCol + 1;
						aMove.mD = aRow;
						aMoves.push_back(aMove);
					}
					if (aBoard->mLayout.IsPlayable(aCol, aRow + 1))
					{
						aMove.mC = aCol;
						aMove.mD = aRow + 1;
						aMoves.push_back(aMove);
					}
				}
				else if ((aType == GameRules::MOVE_TWIST) && aBoard->mLayout.IsPlayable(aCol, aRow) && aBoard->mLayout.IsPlayable(aCol + 1, aRow) &&
					aBoard->mLayout.IsPlayable(aCol, aRow + 1) && aBoard->mLayout.IsPlayable(aCol + 1, aRow + 1))
				{
					aMove.mA = aCol;
					aMove.mB = aRow;
					aMove.mC = (aBoard->mRules.mMoves.mTwistBoth && (Random(2) == 0)) ? -1 : 1;
					aMoves.push_back(aMove);
				}
				else if (aType == GameRules::MOVE_SLIDE)
				{
					// each segment once, from its first cell
					for (int anAxis = MoveSet::AXIS_ROW; anAxis <= MoveSet::AXIS_COL; anAxis++)
					{
						int aStart = 0;
						int aLength = 0;
						if (MoveSet::SegmentAt(aBoard->mLayout, anAxis, aCol, aRow, &aStart, &aLength) && (aLength >= 2) &&
							(aStart == ((anAxis == MoveSet::AXIS_ROW) ? aCol : aRow)))
						{
							aMove.mA = anAxis;
							aMove.mB = (anAxis == MoveSet::AXIS_ROW) ? aRow : aCol;
							aMove.mC = aStart;
							aMove.mD = aLength;
							aMove.mE = 1 + Random(aLength - 1);
							aMoves.push_back(aMove);
						}
					}
				}
			}
		}
	}
	else
	{
		std::vector<MoveSet::Found> aFound;
		aBoard->FindVariantMoves(&aFound);
		for (size_t i = 0; i < aFound.size(); i++)
			aMoves.push_back(aFound[i].mMove);
	}
	for (size_t i = 0; i < aMoves.size(); )
	{
		if (VariantMoveOnBoard(aMoves[i]))
			i++;
		else
			aMoves.erase(aMoves.begin() + i);
	}
	if (aMoves.empty())
		return;

	const MoveSet::Move aPickedMove = aMoves[Random((int) aMoves.size())];
	if (aPickedMove.mType == GameRules::MOVE_SWAP)
	{
		int aStyle = Random((aTypes & GameRules::MOVE_SLIDE) ? 8 : 10);
		Note("move %d: (%d,%d)->(%d,%d) %s, %s", ++mMoveNum, aPickedMove.mA, aPickedMove.mB, aPickedMove.mC, aPickedMove.mD, aKindName,
			(aStyle < 6) ? "click" : (aStyle < 8) ? "zone click" : "drag");
		AddTargetSteps(aPickedMove.mA, aPickedMove.mB, aPickedMove.mC, aPickedMove.mD, aStyle, theDelay);
	}
	else if (aPickedMove.mType == GameRules::MOVE_TWIST)
	{
		Note("move %d: twist (%d,%d) %s, %s", ++mMoveNum, aPickedMove.mA, aPickedMove.mB, (aPickedMove.mC > 0) ? "cw" : "ccw", aKindName);
		Step aClick = {STEP_CLICK_AT, ROLE_SELECT, (aPickedMove.mC > 0) ? 0 : 1, 0, aBoard->GetColX(aPickedMove.mA + 1) - 1 + Random(17) - 8,
			aBoard->GetRowY(aPickedMove.mB + 1) - 1 + Random(17) - 8, theDelay};
		mPlan.push_back(aClick);
	}
	else
	{
		bool isRow = (aPickedMove.mA == MoveSet::AXIS_ROW);
		bool isBack = (aPickedMove.mD - aPickedMove.mE < aPickedMove.mE);
		int aFrom = isBack ? aPickedMove.mC + aPickedMove.mD - 1 : aPickedMove.mC;
		int aDistance = (isBack ? -(aPickedMove.mD - aPickedMove.mE) : aPickedMove.mE) * Board::CELL_SIZE + Random(41) - 20;
		Note("move %d: slide %s %d from %d length %d by %d, %s", ++mMoveNum, isRow ? "row" : "col", aPickedMove.mB, aPickedMove.mC, aPickedMove.mD, aPickedMove.mE, aKindName);
		Step aDown = {STEP_DOWN_AT, ROLE_SELECT, 0, 0, aBoard->GetColX(isRow ? aFrom : aPickedMove.mB) + 24 + Random(17) - 8,
			aBoard->GetRowY(isRow ? aPickedMove.mB : aFrom) + 24 + Random(17) - 8, theDelay};
		mPlan.push_back(aDown);
		int aParts = 1 + Random(3);
		int aDone = 0;
		for (int i = 1; i <= aParts; i++)
		{
			int aPart = aDistance * i / aParts - aDone;
			aDone += aPart;
			Step aMove = {STEP_DRAG_MOVE, ROLE_ANY, 0, 0, isRow ? aPart : Random(5) - 2, isRow ? Random(5) - 2 : aPart, 1 + Random(4)};
			mPlan.push_back(aMove);
		}
		Step anUp = {STEP_DRAG_UP, ROLE_ANY, 0, 0, 0, 0, 1 + Random(3)};
		mPlan.push_back(anUp);
	}
}

// While the board falls, clears or falls out: select a random gem (a fall-out leaves it stale)
void AutoplayBot::PlanFallingSelect()
{
	Board* aBoard = mApp->mBoard;
	std::vector<int> aCells;
	for (int aRow = 0; aRow < aBoard->mRows; aRow++)
	{
		for (int aCol = 0; aCol < aBoard->mCols; aCol++)
		{
			if (aBoard->mBoard[aRow][aCol] != NULL)
				aCells.push_back(aRow * Board::MAX_COLS + aCol);
		}
	}
	if (aCells.empty())
		return;

	int aCell = aCells[Random((int) aCells.size())];
	Note("select (%d,%d) while %s", aCell % Board::MAX_COLS, aCell / Board::MAX_COLS, StateName(aBoard->mState));
	Step aSelect = {STEP_CLICK_CELL, ROLE_SELECT_ANY, aCell % Board::MAX_COLS, aCell / Board::MAX_COLS, 0, 0, 1};
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
				aTargets.push_back(aMove[3] * Board::MAX_COLS + aMove[2]);
		}
	}
	if (aTargets.empty())
	{
		int aDir = Random(4);
		for (int i = 0; i < 4; i++, aDir = (aDir + 1) % 4)
		{
			int aCol2 = aCol + aDirs[aDir][0];
			int aRow2 = aRow + aDirs[aDir][1];
			if ((aCol2 >= 0) && (aCol2 < aBoard->mCols) && (aRow2 >= 0) && (aRow2 < aBoard->mRows) && aBoard->mLayout.IsPlayable(aCol2, aRow2))
				aTargets.push_back(aRow2 * Board::MAX_COLS + aCol2);
		}
	}

	for (size_t i = 0; i < aTargets.size(); )
	{
		int aCell = aTargets[i];
		if (OnBoard(aBoard->GetColX(aCell % Board::MAX_COLS) + 24, aBoard->GetRowY(aCell / Board::MAX_COLS) + 24))
			i++;
		else
			aTargets.erase(aTargets.begin() + i);
	}

	int aTarget[2] = {aCol, aRow};
	if (!aTargets.empty())
	{
		int aCell = aTargets[Random((int) aTargets.size())];
		aTarget[0] = aCell % Board::MAX_COLS;
		aTarget[1] = aCell / Board::MAX_COLS;
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
				Queue("down", ScreenX(x), ScreenY(y));
			}
			else
				Queue("click", ScreenX(x), ScreenY(y));
		}
		return true;

	case STEP_CLICK_ZONE:
		{
			Piece* aSelected = aBoard->mSelectedPiece;
			int x = (int) aSelected->mX + 24 + theStep.mDX;
			int y = (int) aSelected->mY + 24 + theStep.mDY;
			if (!OnBoard(x, y))
				return false;
			Queue("click", ScreenX(x), ScreenY(y));
		}
		return true;

	case STEP_CLICK_AT:
	case STEP_DOWN_AT:
		if (!OnBoard(theStep.mDX, theStep.mDY))
			return false;
		if (theStep.mKind == STEP_DOWN_AT)
		{
			mDragX = theStep.mDX;
			mDragY = theStep.mDY;
			Queue("down", ScreenX(mDragX), ScreenY(mDragY));
		}
		else
			Queue((theStep.mCol == 1) ? "rclick" : "click", ScreenX(theStep.mDX), ScreenY(theStep.mDY));
		return true;

	case STEP_DRAG_MOVE:
		mDragX += theStep.mDX;
		mDragY += theStep.mDY;
		Queue("move", ScreenX(mDragX), ScreenY(mDragY));
		return true;

	case STEP_DRAG_UP:
		Queue("up", ScreenX(mDragX), ScreenY(mDragY));
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
		// -autoplayshot: as the panel is about to stop, before a record-score dialog covers the middle
		if ((!mShotFileName.empty()) && (aNext == mGameOverUpdate + 59))
			Queue("screenshot", mShotFileName);
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
