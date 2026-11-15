#include "Board.h"
#include "Bubbles.h"
#include "TileMaster.h"
#include "PixelusApp.h"
#include "UserDirectory.h"
#include "GameText.h"
#include "Res.h"
#include <string>

// Lesson.cpp: what teaches in the tutorial puzzles, and what answers a click that did nothing (the original's Board
// functions 0x4026a0-0x402fe0 and 0x406050-0x407aa0). Each tutorial puzzle has a step function that the update calls every
// fifth update (0x407aa0): the first call puts up the puzzle's note and starts the glow on the cells it speaks of; the
// later calls, while no note is up, look at the cells and point at the one that matters with a speech bubble, after the
// bubble's timer has run down. The cells the steps name are those of the shipped puzzles.

using namespace Sexy;

// the 16 texts of the bubbles (0x53d138); the first eight are the lessons', the rest answer clicks that did nothing
static const unsigned int sBubbleTexts[16] =
{
	0x50c22c, 0x50c1f8, 0x50c1c4, 0x50c18c, 0x50c148, 0x50c104, 0x50c0d0, 0x50c0ac,
	0x50c070, 0x50c030, 0x50bff8, 0x50bfb8, 0x50bf90, 0x50bf64, 0x50bf24, 0x50bef8
};

// 0x50b860: the updates (counted in fifths) a bubble's timer waits; the bubbles with 0 show at once
static const int sTimerInit[Board::NUM_TIMERS] =
{
	60, 60, 0, 60, 0, 60, 60, 60, 0, 0, 0, 0, 0, 0, 60, 60, 60, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

// port: a helper of the port (Board::InitLesson)
void Board::InitLesson()
{
	mHelper = new HintBubbles(mApp);
	mBubbleId = -1;
	mBubbleX = mBubbleY = 0;
	mLessonFast = false;
	mClickX = mClickY = -1;
	mFirstAfterTutorial = false;
	mFinishedOnce = false;
	ResetLessonTimers();
}

// 0x402700
void Board::ResetLessonTimers()
{
	for (int i = 0; i < NUM_TIMERS; i++)
		mLessonTimers[i] = sTimerInit[i];
}

// 0x4026a0: counts the timer down; true when it has run out (it starts again)
bool Board::TickLessonTimer(int theIndex)
{
	int aLimit = mLessonFast ? 0x19 : 0;
	if (--mLessonTimers[theIndex] <= aLimit)
	{
		mLessonTimers[theIndex] = sTimerInit[theIndex];
		return true;
	}
	return false;
}

// 0x402d90: the bubble goes, the pointer with it, and the timers start again
void Board::ClearBubble()
{
	if (mBubbleId != -1)
	{
		mHelper->Remove(mBubbleId);
		mBubbleId = -1;
		mGrid->HidePointer();
		ResetLessonTimers();
	}
}

// 0x402de0: the corner of the bubble's picture that points at the cell, chosen so that the bubble stays on the screen and
// clear of the Tile Master's side
int Board::BubbleCorner(int theRow, int theCol)
{
	int aSide = mTileMaster->GetSide();
	if (aSide == Grid::SIDE_DOWN)
	{
		if (theRow > 12)
			return theCol < 13 ? 1 : 0;
		return 3 - (theCol < 13 ? 1 : 0);
	}
	if (aSide == Grid::SIDE_UP)
	{
		if (theRow < 3)
			return 3 - (theCol < 13 ? 1 : 0);
		return theCol < 13 ? 1 : 0;
	}
	int aLimit = aSide == Grid::SIDE_RIGHT ? 13 : 3;
	if (theCol < aLimit)
		return theRow <= 2 ? 2 : 1;
	return theRow > 2 ? 0 : 3;
}

// 0x406050: the bubble with the text for the place, unless it is up already; theTick runs the timer down here, theFire
// says it has run down
void Board::ShowBubble(int theX, int theY, int theCorner, int theIndex, bool theFlagged, bool theTick, bool theFire)
{
	if (mUpdateCnt <= 200)
		return;
	const std::string aText = GT(sBubbleTexts[theIndex]);
	if (!(mBubbleId == -1 || theX != mBubbleX || theY != mBubbleY || aText != mBubbleText))
		return;
	ClearBubble();
	if ((theTick && TickLessonTimer(theIndex)) || theFire)
	{
		mLessonFast = sTimerInit[theIndex] < 1;
		mBubbleId = mHelper->Add(theX, theY, theCorner, aText, theFlagged);
		mBubbleX = theX;
		mBubbleY = theY;
		mBubbleText = aText;
	}
}

// 0x406140: the bubble points at the cell; the timer for the text runs down by one for each call
void Board::PointAtCell(int theRow, int theCol, int theCorner, int theIndex, bool theFlagged)
{
	if (mUpdateCnt <= 200)
		return;
	int aLimit = mLessonFast ? 0x19 : 0;
	bool aFire = --mLessonTimers[theIndex] <= aLimit;
	if (aFire)
		mLessonTimers[theIndex] = sTimerInit[theIndex];
	ShowBubble(mGridX + theCol * Grid::CELL_SIZE + Grid::CELL_SIZE / 2, mGridY + theRow * Grid::CELL_SIZE + Grid::CELL_SIZE / 2,
		theCorner, theIndex, theFlagged, false, aFire);
	if (aFire)
		mGrid->ShowPointer(theRow, theCol);
}

// 0x4221f0 then 0x402de0 and 0x406140: points at the first loose tile that has an open side; false when there is none
bool Board::PointAtStray(int theSkipRow, int theSkipCol)
{
	int aRow = 0, aCol = 0;
	bool aFound = theSkipRow < 0 ? mGrid->FindStray(&aRow, &aCol) : mGrid->FindStrayFrom(&aRow, &aCol, theSkipRow, theSkipCol);
	if (aFound)
		PointAtCell(aRow, aCol, BubbleCorner(aRow, aCol), 1, false);
	return aFound;
}

// 0x403930: the cell holds the tile thrown last (it stayed on its line, and no tile is between it and the thrower)
bool Board::IsLastThrownTile(int theRow, int theCol)
{
	if (mUndo.empty())
		return false;
	const UndoMove& aLast = mUndo.back();
	if (aLast.mType != 0)
		return false;
	for (int aDir = 0; aDir < 4; aDir++)
	{
		int aRow = theRow, aCol = theCol;
		bool aBlocked = false;
		for (;;)
		{
			switch (aDir)
			{
			case 0: aRow++; break;
			case 1: aCol--; break;
			case 2: aRow--; break;
			case 3: aCol++; break;
			}
			if (!mGrid->InGrid(aRow, aCol))
				break;
			if (mGrid->HasObject(aRow, aCol))
			{
				aBlocked = true;
				break;
			}
		}
		if (aBlocked)
			continue;
		if (aLast.mSide == aDir)
		{
			int aCursor = (aDir == Grid::SIDE_LEFT || aDir == Grid::SIDE_RIGHT) ? theRow : theCol;
			if (aLast.mCursor == aCursor)
				return true;
		}
	}
	return false;
}

// the note of a tutorial puzzle (0x40f160): where its middle lies is given by the step (0x196 and 0xf0 are the middle of the
// screen's board part)
void Board::ShowLessonNote(int thePuzzle)
{
	static const int aWidths[9] = { 0xfa, 0xfa, 0xfa, 0xdc, 0xfa, 0xdc, 0xfa, 0xdc, 300 };
	static const unsigned int aTexts[8] = { 0x50be48, 0x50bdd8, 0x50bd68, 0x50bd08, 0x50bca8, 0x50bc50, 0x50bc00, 0x50bbd0 };
	// x and y from the middle (the sixth and seventh arguments of 0x40f160)
	static const int aOffsets[9][2] = { { 0x16, -0x50 }, { 0x3c, 0x34 }, { 0, 0 }, { 0x33, 0x36 }, { -0x22, -9 }, { -0x33, 0x2d },
		{ 0, 0x8d }, { -7, -0x72 }, { 0, 0 } };
	if (thePuzzle < 8)
		mApp->ShowHint(aWidths[thePuzzle], -1, "", GT(aTexts[thePuzzle]), aOffsets[thePuzzle][0], aOffsets[thePuzzle][1]);
	else
	{
		UserProfile* aUser = mApp->mUserDirectory->GetCurrent();
		bool anExpert = aUser != NULL && aUser->mExpert;
		mApp->ShowHint(aWidths[8], -1, "", anExpert ? GT(0x50b9a8) : GT(0x50bab8), 0, 0);	// 0x402f80
	}
}

// 0x4061f0
void Board::LessonStep0()
{
	if (!mLessonShown)
	{
		mLessonShown = true;
		ShowLessonNote(0);
		mGrid->StartGlow(9, 8, 0);
		mGrid->StartGlow(9, 9, 0xe4);
	}
	if (mApp->GetDialog(0x1f) != NULL)
		return;
	if (mGrid->mStrayTiles < 1)
	{
		if (!mGrid->HasObject(9, 8))
		{
			PointAtCell(9, 8, 0, 0, false);
			return;
		}
		if (!mGrid->HasObject(9, 9))
			PointAtCell(9, 9, 2, 3, false);
	}
	else
		PointAtStray(-1, -1);
}

// 0x406310
void Board::LessonStep1()
{
	if (!mLessonShown)
	{
		mLessonShown = true;
		ShowLessonNote(1);
		mGrid->StartGlow(5, 9, 0);
		mGrid->StartGlow(5, 12, 0xe4);
	}
	if (mApp->GetDialog(0x1f) != NULL)
		return;
	if (mGrid->mStrayTiles < 1)
	{
		if (mGrid->mEmptyWanted == 2)
		{
			PointAtCell(5, 9, 0, 0, false);
			return;
		}
		if (mGrid->mEmptyWanted == 1)
		{
			if (mGrid->HasObject(5, 12))
			{
				PointAtCell(5, 12, 2, 4, false);
				return;
			}
			if (mGrid->HasObject(5, 9))
				PointAtCell(5, 12, 2, 3, false);
		}
	}
	else
		PointAtStray(-1, -1);
}

// 0x406470
void Board::LessonStep2()
{
	if (!mLessonShown)
	{
		mLessonShown = true;
		ShowLessonNote(2);
		mGrid->StartGlow(2, 4, 0);
	}
	if (mApp->GetDialog(0x1f) != NULL)
		return;
	int aStray = mGrid->mStrayTiles;
	if (aStray < 1)
	{
		if (mGrid->mEmptyWanted == 2)
		{
			PointAtCell(2, 4, 3, 7, false);
			return;
		}
		if (mGrid->HasObject(2, 5) && !mGrid->HasObject(2, 0xb))
		{
			PointAtCell(2, 0xb, 2, 3, false);
			return;
		}
		if (!mGrid->HasObject(2, 5) && mGrid->HasObject(2, 0xb))
			PointAtCell(2, 0xb, 2, 4, false);
		return;
	}
	if (!mGrid->HasObject(2, 4))
	{
		PointAtStray(-1, -1);
		return;
	}
	if (aStray < 2)
	{
		if (mGrid->HasObject(2, 5))
		{
			PointAtCell(2, 4, 3, 6, false);
			return;
		}
		if (!mGrid->HasObject(2, 0xb))
		{
			PointAtCell(2, 5, 2, 5, false);
			return;
		}
		PointAtCell(2, 0xb, 3, 4, false);
		return;
	}
	PointAtStray(2, 4);
}

// 0x4066b0
void Board::LessonStep3()
{
	if (!mLessonShown)
	{
		mLessonShown = true;
		ShowLessonNote(3);
	}
	if (mApp->GetDialog(0x1f) != NULL || mGrid->IsSettled())
		return;
	Grid* g = mGrid;
	if (g->HasObject(9, 3))
	{
		if (g->HasOpenSide(9, 3))
		{
			PointAtCell(9, 3, 2, 0xe, false);
			return;
		}
		PointAtStray(-1, -1);
		return;
	}
	if (g->HasObject(4, 0xb))
	{
		PointAtCell(4, 0xb, 3, g->HasOpenSide(4, 0xb) ? 1 : 0xf, false);
		return;
	}
	bool aChain = false;
	if (!g->HasObject(2, 0xb))
	{
		if (g->HasObject(3, 0xb))
		{
			if (g->HasObject(0, 0xb))
			{
				PointAtCell(0, 0xb, 3, 0x10, false);
				return;
			}
			if (g->HasObject(1, 0xb))
			{
				PointAtCell(1, 0xb, 3, 0x10, false);
				return;
			}
			if (PointAtStray(3, 0xb))
				return;
			PointAtCell(2, 0xb, 3, 5, false);
			return;
		}
		aChain = true;
	}
	else
	{
		if (!g->HasObject(3, 0xb))
		{
			if (!g->HasObject(3, 0xd))
			{
				if (g->HasObject(3, 0xc))
				{
					if (PointAtStray(3, 0xc))
						return;
					PointAtCell(3, 0xd, 3, 5, false);
					return;
				}
				if (g->HasObject(2, 0xc))
				{
					PointAtCell(2, 0xc, 3, 0x10, false);
					return;
				}
				PointAtCell(3, 0xc, 3, 7, false);
				return;
			}
			if (g->HasObject(3, 0xc))
			{
				PointAtCell(3, 0xc, 3, 6, false);
				return;
			}
			if (PointAtStray(-1, -1))
				return;
			PointAtCell(9, 3, 3, 3, false);
			return;
		}
		aChain = true;
	}
	if (aChain)
	{
		// the row 3 from the right: the first tile out there is the one to take back
		for (int aCol = 0xf; aCol >= 0xd; aCol--)
			if (g->HasObject(3, aCol))
			{
				PointAtCell(3, aCol, 3, 0x10, false);
				return;
			}
		if (!g->HasObject(3, 0xc))
		{
			// the two chains differ here: the one that had no tile at (2, 11) asks for the tile at (3, 11) (text 7), the
			// other for the one at (2, 11) not yet being in (text 6)
			PointAtCell(3, 0xb, 3, g->HasObject(2, 0xb) ? 6 : 7, false);
			return;
		}
		PointAtCell(3, 0xc, 3, 0x10, false);
	}
}

// 0x406b10
void Board::LessonStep4()
{
	if (!mLessonShown)
	{
		mLessonShown = true;
		ShowLessonNote(4);
		mGrid->StartGlow(0xd, 7, 0);
		mGrid->StartGlow(0xd, 6, 0xe4);
	}
	if (mApp->GetDialog(0x1f) != NULL || mGrid->IsSettled())
		return;
	Grid* g = mGrid;
	if (g->HasObject(0xd, 0xb))
	{
		PointAtCell(0xd, 0xb, 0, 0xe, false);
		return;
	}
	int aMask = (g->HasObject(0xd, 7) ? 1 : 0) | (g->HasObject(0xd, 6) ? 2 : 0) | (g->HasObject(0xb, 5) ? 4 : 0) |
		(g->HasObject(0xc, 5) ? 8 : 0) | (g->HasObject(0xd, 5) ? 0x10 : 0);
	// the jump table at 0x406e04 (the index table at 0x406e38 gives the case of each mask): the cell and the text for the
	// case, the corner when no other tile is pointed at (the original leaves it at 1; 0 for the case 9), and the cell of
	// the tile to skip when another loose tile comes first
	struct Rule { int mRow, mCol, mIndex, mCorner, mSkipRow, mSkipCol; bool mFind; };
	static const Rule aRules[13] =
	{
		{ 0xd, 7, 7, 1, -1, -1, true },
		{ 0xd, 6, 7, 1, 0xd, 7, true },
		{ 0xd, 5, 5, 1, 0xd, 6, true },
		{ 0xd, 7, 6, 1, 0, 0, false },
		{ 0xc, 5, 7, 1, 0xb, 5, true },
		{ 0xb, 5, 1, 1, 0, 0, false },
		{ 0xd, 5, 5, 1, 0xc, 5, true },
		{ 0xd, 7, 1, 1, 0, 0, false },
		{ 0xb, 5, 6, 1, 0, 0, false },
		{ 0xd, 0xb, 3, 0, -1, -1, true },
		{ 0xd, 6, 6, 1, 0, 0, false },
		{ 0xc, 5, 6, 1, 0, 0, false },
		{ 1, 1, 1, 1, 0, 0, false }		// no mask leads here that the puzzle can reach: the original points at cell (1, 1)
	};
	static const unsigned char aCase[25] = { 0, 1, 2, 3, 4, 5, 5, 12, 6, 7, 12, 12, 8, 12, 12, 12, 9, 12, 10, 12, 12, 12, 12, 12, 11 };
	// the jump table's own order is 0, 1, 2, 3 ... 12 for these cases of the table at 0x406e04 listed above by address
	const Rule& aRule = aRules[aMask <= 0x18 ? aCase[aMask] : 12];
	if (aRule.mFind && PointAtStray(aRule.mSkipRow, aRule.mSkipCol))
		return;
	PointAtCell(aRule.mRow, aRule.mCol, aRule.mCorner, aRule.mIndex, false);
}

// 0x406e60
void Board::LessonStep5()
{
	if (!mLessonShown)
	{
		mLessonShown = true;
		ShowLessonNote(5);
	}
	if (mApp->GetDialog(0x1f) != NULL)
		return;
	Grid* g = mGrid;
	int aMask = (g->HasObject(5, 10) ? 1 : 0) | (g->HasObject(6, 11) ? 2 : 0) | (g->HasObject(7, 12) ? 4 : 0) |
		(g->HasObject(5, 11) ? 8 : 0) | (g->HasObject(6, 12) ? 0x10 : 0) | (g->HasObject(5, 12) ? 0x20 : 0);
	// the cases of the table at 0x4070e4 (the index table at 0x407118 gives the case of each mask): the cell and the text
	// (-1: none) and, when another loose tile than the one in (mSkipRow, mSkipCol) comes first, that one
	struct Rule { int mRow, mCol, mIndex, mSkipRow, mSkipCol; bool mFind; };
	static const Rule aRules[13] =
	{
		{ 5, 10, 7, 5, 10, true },
		{ 5, 11, 5, 5, 10, true },
		{ 5, 11, 5, 6, 11, true },
		{ 6, 12, 5, 7, 12, true },
		{ 5, 12, 0, -1, -1, true },
		{ 5, 10, 6, 0, 0, false },
		{ 6, 11, 6, 0, 0, false },
		{ 0, 0, -1, -1, -1, true },
		{ 7, 12, 6, 0, 0, false },
		{ 5, 12, 3, -1, -1, true },
		{ 5, 12, 0x10, 0, 0, false },
		{ 6, 12, 3, -1, -1, true },
		{ 0, 0, -1, 0, 0, false }
	};
	static const unsigned char aCase[53] =
	{
		0, 1, 2, 1, 3, 1, 3, 1, 4, 5, 6, 6, 3, 12, 12, 12, 0, 1, 7, 12, 8, 12, 12, 12, 9, 5, 12, 12, 8, 12, 12, 12,
		10, 10, 10, 12, 10, 10, 10, 12, 11, 7, 7, 12, 7, 12, 12, 12, 10, 10, 10, 12, 10
	};
	// (the table's order is by case: the cases 3 to 11 of the original's table are the rules in the order given here)
	if (aMask > 0x34)
		return;
	// the original's table lists the cases 0 1 2 3 4 5 6 7 8 9 10 11 12 at 0x406f66 0x406f80 0x406f9a 0x406ff6 0x406fd9 0x406fb4
	// 0x406fc8 0x40703a 0x407010 0x407021 0x40704c 0x406fef 0x40705d, which the rules above follow
	const Rule& aRule = aRules[aCase[aMask]];
	if (aRule.mFind && PointAtStray(aRule.mSkipRow, aRule.mSkipCol))
		return;
	if (aRule.mIndex >= 0)
		PointAtCell(aRule.mRow, aRule.mCol, 3, aRule.mIndex, false);
}

// 0x407150
void Board::LessonStep6()
{
	if (!mLessonShown)
	{
		mLessonShown = true;
		ShowLessonNote(6);
		mGrid->StartGlow(4, 7, 0);
		mGrid->StartGlow(0xb, 8, 0xe4);
	}
	if (mApp->GetDialog(0x1f) != NULL)
		return;
	Grid* g = mGrid;
	int aRow = 0, aCol = 0, aIndex = -1, aCorner = 3;
	bool aFind = false;
	int aSkipRow = -1, aSkipCol = -1;
	if (g->HasObject(10, 10))
	{
		aRow = 10; aCol = 10; aIndex = 1;
	}
	else if (g->HasObject(5, 8))
	{
		aRow = 5; aCol = 8; aIndex = 0xf; aCorner = 1;
	}
	else if (g->HasObject(6, 9))
	{
		aRow = 6; aCol = 9; aIndex = 0xf; aCorner = 1;
	}
	else if (g->HasObject(9, 7))
	{
		aRow = 9; aCol = 7; aIndex = 0xf;
	}
	else
	{
		int aMask = (g->HasObject(4, 7) ? 1 : 0) | (g->HasObject(10, 7) ? 2 : 0) | (g->HasObject(10, 8) ? 4 : 0) |
			(g->HasObject(10, 9) ? 8 : 0) | (g->HasObject(0xb, 8) ? 0x10 : 0);
		// the index table at 0x4074e4 gives the case of each mask, the table at 0x4074b8 the code of each case
		static const unsigned char aCase[29] = { 0, 1, 2, 1, 3, 3, 3, 1, 4, 5, 6, 1, 7, 7, 6, 10, 8, 1, 10, 1, 8, 8, 8, 10, 9, 10, 10, 10, 8 };
		switch (aMask <= 0x1c ? aCase[aMask] : 10)
		{
		case 0: aRow = 10; aCol = 7; aIndex = 7; aFind = true; break;
		case 1: aRow = 4; aCol = 7; aIndex = 0x10; aCorner = 1; break;
		case 2: aRow = 10; aCol = 8; aIndex = 7; aFind = true; aSkipRow = 10; aSkipCol = 7; break;
		case 3: aRow = 10; aCol = 9; aIndex = 5; break;
		case 4: aRow = 0xb; aCol = 8; aIndex = 0; aFind = true; break;
		case 5: aRow = 0xb; aCol = 8; aIndex = 3; aFind = true; break;
		case 6: aRow = 10; aCol = 7; aIndex = 6; break;
		case 7: aRow = 10; aCol = 8; aIndex = 6; aCorner = 0; break;
		case 8: aRow = 0xb; aCol = 8; aIndex = 0x10; break;
		case 9: aRow = 4; aCol = 7; aIndex = 3; aFind = true; aCorner = 1; break;
		default: break;
		}
	}
	if (aFind && PointAtStray(aSkipRow, aSkipCol))
		return;
	if (aIndex >= 0)
		PointAtCell(aRow, aCol, aCorner, aIndex, false);
}

// 0x407510
void Board::LessonStep7()
{
	if (!mLessonShown)
	{
		mLessonShown = true;
		ShowLessonNote(7);
		mGrid->StartGlow(6, 6, 0);
		mGrid->StartGlow(6, 7, 0);
		mGrid->StartGlow(6, 8, 0);
		mGrid->StartGlow(5, 7, 0xe4);
	}
	if (mApp->GetDialog(0x1f) != NULL || mGrid->IsSettled())
		return;
	Grid* g = mGrid;
	// the cells that decide first, each with its text: 1 when the tile can be taken out (an open side), else 0xf
	static const int aFirst[5][2] = { { 10, 7 }, { 8, 7 }, { 7, 6 }, { 7, 7 }, { 7, 8 } };
	for (int i = 0; i < 5; i++)
		if (g->HasObject(aFirst[i][0], aFirst[i][1]))
		{
			int aIndex = i == 0 ? 0xe : g->HasOpenSide(aFirst[i][0], aFirst[i][1]) ? 1 : 0xf;
			PointAtCell(aFirst[i][0], aFirst[i][1], 3, aIndex, false);
			return;
		}
	int aMask = (g->HasObject(5, 6) ? 1 : 0) | (g->HasObject(5, 7) ? 2 : 0) | (g->HasObject(5, 8) ? 4 : 0) |
		(g->HasObject(6, 5) ? 8 : 0) | (g->HasObject(6, 6) ? 0x10 : 0) | (g->HasObject(6, 7) ? 0x20 : 0) |
		(g->HasObject(6, 8) ? 0x40 : 0) | (g->HasObject(6, 9) ? 0x80 : 0);
	// the masks of each case of the table at 0x40795c (the index table at 0x4079ac gives the case of each mask): the cell
	// and the text, and when another loose tile than the one in (mSkipRow, mSkipCol) comes first, that one
	struct Rule { int mRow, mCol, mIndex, mSkipRow, mSkipCol; bool mFind; int mMasks[6]; };
	static const Rule aRules[19] =
	{
		{ 6, 5, 7, -1, -1, true, { 0, 1, 4, -1 } },
		{ 6, 6, 5, 6, 5, true, { 8, 9, 0xc, -1 } },
		{ 6, 7, 7, -1, -1, true, { 0x10, 0x11, 0x14, 0x40, -1 } },
		{ 5, 7, 0x10, 6, 7, true, { 0x12, 0x22, 0x32, 0x42, 0x62, 0x72 } },
		{ 6, 5, 6, 0, 0, false, { 0x18, 0x19, 0x1c, -1 } },
		{ 6, 6, 5, 6, 7, true, { 0x20, 0x60, -1 } },
		{ 6, 8, 5, 6, 7, true, { 0x30, 0x31, 0x34, -1 } },
		{ 5, 6, 7, -1, -1, true, { 0x50, 0x58, 0xd0, -1 } },
		{ 5, 7, 5, 5, 6, true, { 0x51, 0x59, 0xd1, -1 } },
		{ 10, 7, 3, -1, -1, true, { 0x52, -1 } },
		{ 5, 6, 6, 0, 0, false, { 0x53, -1 } },
		{ 5, 7, 5, 5, 8, true, { 0x54, 0x5c, 0xd4, -1 } },
		{ 5, 8, 1, 0, 0, false, { 0x55, -1 } },
		{ 5, 8, 6, 0, 0, false, { 0x56, -1 } },
		{ 6, 5, 1, 0, 0, false, { 0x68, -1 } },
		{ 6, 7, 6, 0, 0, false, { 0x70, 0x71, 0x74, 0x78, 0xf0, -1 } },
		{ 6, 8, 5, 6, 9, true, { 0x80, -1 } },
		{ 6, 9, 1, 0, 0, false, { 0xb0, -1 } },
		{ 6, 9, 6, 0, 0, false, { 0xc0, -1 } }
	};
	for (int i = 0; i < 19; i++)
		for (int j = 0; j < 6 && aRules[i].mMasks[j] >= 0; j++)
			if (aRules[i].mMasks[j] == aMask)
			{
				if (aRules[i].mFind && PointAtStray(aRules[i].mSkipRow, aRules[i].mSkipCol))
					return;
				PointAtCell(aRules[i].mRow, aRules[i].mCol, 3, aRules[i].mIndex, false);
				return;
			}
	PointAtStray(6, 7);		// any other state: the first loose tile but the one in (6, 7)
}

// 0x407aa0: every fifth update, while the board has no editor, the hints are on and the game is not paused. In the tutorial
// puzzles the step of the puzzle runs; in the others the bubble is cleared, and the first one after the tutorial is noted
void Board::HintCheck()
{
	if (mApp->GetHintsDisabled() || mApp->IsPaused())
		return;
	if (mTutorial && mPuzzleIndex >= 0 && mPuzzleIndex <= 8)
	{
		switch (mPuzzleIndex)
		{
		case 0: LessonStep0(); break;
		case 1: LessonStep1(); break;
		case 2: LessonStep2(); break;
		case 3: LessonStep3(); break;
		case 4: LessonStep4(); break;
		case 5: LessonStep5(); break;
		case 6: LessonStep6(); break;
		case 7: LessonStep7(); break;
		default: LessonStep8(); break;
		}
		return;
	}
	if (!mFirstAfterTutorial)
	{
		UserProfile* aUser = mApp->mUserDirectory->GetCurrent();
		if (aUser != NULL && !aUser->mFlags[3] && aUser->mFlags[0] && aUser->mFlags[1] && aUser->mFlags[2])
		{
			mFirstAfterTutorial = true;
			return;
		}
	}
	ClearBubble();
}

// 0x402f80: the last puzzle of the tutorial has only its note
void Board::LessonStep8()
{
	if (!mLessonShown)
	{
		mLessonShown = true;
		ShowLessonNote(8);
	}
}
