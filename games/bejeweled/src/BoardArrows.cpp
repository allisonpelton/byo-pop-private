// port(variant): arrow gems (games/bejeweled/docs/CODE_MODS_DESIGN.md 4.20, stage 18). Not part of the original
// game; the idea is Bejeweled 3's Butterflies, turned four ways (eight with the diagonals), with none of that mode's
// art or texts. Some gems are arrows (Piece::mArrow), each pointing up, right, down or left, or along a diagonal
// (for a diamond board, whose edges are diagonal). A move that stands (OnMoveMade, or a kept
// invalid move) makes the settle that ends it step the arrows: each, closest to its edge first, trades places with
// the next gem the way it points (holes and dirt are jumped), and waits while that gem is an arrow too. The steps
// animate in STATE_ARROW_STEPPING with the grid already traded, then the board settles again, so a step that makes
// a match clears and cascades as usual; the arrows step once per move. An arrow that has to step off the board
// escapes and the game is over. After every arrows.every moves (one fewer each level, down to arrows.everyMin), a
// gem on the edge opposite a direction the rules allow turns into an arrow, drawn from the board's RNG at that
// settle. A matched or blasted arrow is cleared: arrows.bonus times the arrows cleared so far, times the multiplier.
// The arrows and the counts are play state (hashed, saved in ARRW, undone); their art and the warning
// (BoardArrowsDraw.cpp) are cosmetic, and of the step's animation only the pieces' positions are hashed, which play
// reads no more than a fall's.

#include "Board.h"
#include "WinDMApp.h"
#include "GameConfig.h"
#include "GameStats.h"
#include "Res.h"

#include <algorithm>
#include <vector>

using namespace Sexy;

namespace
{

enum
{
	ARROW_STEP_TICKS = 24       // a step's animation
};

const int kStepCol[GameRules::NUM_ARROW_DIRECTIONS] = {0, 1, 0, -1, 1, 1, -1, -1};    // GameRules::ArrowDirection
const int kStepRow[GameRules::NUM_ARROW_DIRECTIONS] = {-1, 0, 1, 0, -1, 1, 1, -1};

}

// A move stood (OnMoveMade, KeepInvalidMove): the settle that ends it steps the arrows
void Board::OnArrowMove()
{
	if (mVariant.mArrows)
		mArrowStepDue = true;
}

bool Board::NextArrowCell(int theCol, int theRow, int theDir, int* theNextCol, int* theNextRow) const
{
	int aCol = theCol + kStepCol[theDir];
	int aRow = theRow + kStepRow[theDir];
	while ((aCol >= 0) && (aCol < mCols) && (aRow >= 0) && (aRow < mRows))
	{
		if (mBoard[aRow][aCol] != NULL)
		{
			*theNextCol = aCol;
			*theNextRow = aRow;
			return true;
		}
		aCol += kStepCol[theDir];
		aRow += kStepRow[theDir];
	}
	return false;
}

int Board::ArrowDistance(int theCol, int theRow, int theDir) const
{
	int aDistance = 0;
	int aCol = theCol, aRow = theRow;
	while (NextArrowCell(aCol, aRow, theDir, &aCol, &aRow))
		aDistance++;
	return aDistance;
}

// UpdateFalling's settle without a match, before the move's end is decided. A move made the settle due: the arrows
// step, closest to their edge first (row-major among equals), each into the next gem cell it points to unless an
// arrow holds it; the first that has nowhere to go escapes and the others stay. With any step the board animates
// them (true) and settles again after. Then, at a settle with nothing to step, a new arrow once the move count is due.
bool Board::ArrowSettle()
{
	if (mArrowStepDue)
	{
		mArrowStepDue = false;
		mArrowMoves++;

		struct Ahead
		{
			Piece*	mPiece;
			int		mDistance;
			int		mOrder;
		};
		std::vector<Ahead> anArrows;
		for (int aRow = 0; aRow < mRows; aRow++)
		{
			for (int aCol = 0; aCol < mCols; aCol++)
			{
				Piece* aPiece = mBoard[aRow][aCol];
				if ((aPiece != NULL) && (aPiece->mArrow != 0))
					anArrows.push_back({aPiece, ArrowDistance(aCol, aRow, aPiece->mArrow - 1), aRow * mCols + aCol});
			}
		}
		std::sort(anArrows.begin(), anArrows.end(), [](const Ahead& a, const Ahead& b) {
			return (a.mDistance != b.mDistance) ? (a.mDistance < b.mDistance) : (a.mOrder < b.mOrder);
		});

		mArrowSteps.clear();
		for (const Ahead& anArrow : anArrows)
		{
			Piece* aPiece = anArrow.mPiece;
			int aDir = aPiece->mArrow - 1;
			int aCol = aPiece->mCol, aRow = aPiece->mRow;
			int aNextCol, aNextRow;
			if (!NextArrowCell(aCol, aRow, aDir, &aNextCol, &aNextRow))
			{
				// it flies past the holes beyond it to the first cell off the grid
				int anOutCol = aCol + kStepCol[aDir], anOutRow = aRow + kStepRow[aDir];
				while ((anOutCol >= 0) && (anOutCol < mCols) && (anOutRow >= 0) && (anOutRow < mRows))
				{
					anOutCol += kStepCol[aDir];
					anOutRow += kStepRow[aDir];
				}
				mArrowSteps.push_back({aCol, aRow, anOutCol, anOutRow});
				break;
			}
			Piece* aGem = mBoard[aNextRow][aNextCol];
			if (aGem->mArrow != 0)
				continue;
			mBoard[aNextRow][aNextCol] = aPiece;
			mBoard[aRow][aCol] = aGem;
			aPiece->mCol = aNextCol;
			aPiece->mRow = aNextRow;
			aGem->mCol = aCol;
			aGem->mRow = aRow;
			mArrowSteps.push_back({aCol, aRow, aNextCol, aNextRow});
		}
		if (!mArrowSteps.empty())
		{
			mArrowTick = 0;
			mState = STATE_ARROW_STEPPING;
			mApp->PlaySample(SOUND_SELECT, GetPanPosition(GetColX(mArrowSteps[0].mFromCol) + 24));
			return true;
		}
	}

	if (mArrowMoves >= mRules.ArrowInterval(mLevel))
	{
		mArrowMoves = 0;
		AddNewArrow();
	}
	return false;
}

// STATE_ARROW_STEPPING: the arrows and the gems they traded with glide between their cells (the grid has them in their
// new cells already), the arrow that escapes glides a cell off the board. Then STATE_FALLING, whose next tick finds
// the board at rest and settles: the steps' matches, or the move's end. An escape ends the game instead.
void Board::UpdateArrowSteps()
{
	MarkDirty();
	mArrowTick++;
	double aStepProgress = (double) mArrowTick / ARROW_STEP_TICKS;
	aStepProgress = aStepProgress * aStepProgress * (3.0 - 2.0 * aStepProgress);
	for (const ArrowStep& aStep : mArrowSteps)
	{
		double aFromX = GetColX(aStep.mFromCol), aFromY = GetRowY(aStep.mFromRow);
		double aToX = GetColX(aStep.mToCol), aToY = GetRowY(aStep.mToRow);
		bool isEscape = (aStep.mToCol < 0) || (aStep.mToCol >= mCols) || (aStep.mToRow < 0) || (aStep.mToRow >= mRows);
		if (isEscape)
		{
			Piece* anArrow = mBoard[aStep.mFromRow][aStep.mFromCol];
			anArrow->mX = aFromX + (aToX - aFromX) * aStepProgress;
			anArrow->mY = aFromY + (aToY - aFromY) * aStepProgress;
			continue;
		}
		Piece* anArrow = mBoard[aStep.mToRow][aStep.mToCol];
		Piece* aGem = mBoard[aStep.mFromRow][aStep.mFromCol];
		anArrow->mX = aFromX + (aToX - aFromX) * aStepProgress;
		anArrow->mY = aFromY + (aToY - aFromY) * aStepProgress;
		aGem->mX = aToX + (aFromX - aToX) * aStepProgress;
		aGem->mY = aToY + (aFromY - aToY) * aStepProgress;
	}
	if (mArrowTick < ARROW_STEP_TICKS)
		return;

	bool anEscape = false;
	for (const ArrowStep& aStep : mArrowSteps)
	{
		bool isEscape = (aStep.mToCol < 0) || (aStep.mToCol >= mCols) || (aStep.mToRow < 0) || (aStep.mToRow >= mRows);
		int aCols[2] = {aStep.mFromCol, aStep.mToCol}, aRows[2] = {aStep.mFromRow, aStep.mToRow};
		for (int i = 0; i < (isEscape ? 1 : 2); i++)
		{
			Piece* aPiece = mBoard[aRows[i]][aCols[i]];
			if (isEscape)
			{
				// gone over the edge: a shot when every edge it crossed is one of arrows.shots, which scores the arrow's
				// bonus and leaves a cell to refill; otherwise an escape, and the game is over
				int anEdges = ArrowExitEdges(aStep);
				if ((anEdges & ~mRules.mArrows.mShots) == 0)
				{
					ClearArrow(aPiece);
					mApp->PlaySample(SOUND_GOTSETBIG, GetPanPosition((int) aPiece->mX + 24));
				}
				else
					anEscape = true;
				mBoard[aRows[i]][aCols[i]] = NULL;
				if (mSelectedPiece == aPiece)
					mSelectedPiece = NULL;
				delete aPiece;
				continue;
			}
			aPiece->mX = GetColX(aCols[i]);
			aPiece->mY = GetRowY(aRows[i]);
		}
	}
	mArrowSteps.clear();
	mArrowTick = 0;
	if (anEscape)
	{
		mApp->PlaySample(SOUND_BAD);
		DoBouncyText(mApp->GetString("ARROW_ESCAPED", _S("ARROW ESCAPED!")), 400);
		GameOver(200);
		return;
	}
	mState = STATE_FALLING;
}

// A quarter turn of an arrow (Piece::mArrow, its direction + 1), clockwise for theQuarters 1 and anticlockwise for -1:
// the straight four turn among themselves, and so do the diagonals (GameRules::ArrowDirection's order is clockwise)
int Board::TurnArrow(int theArrow, int theQuarters)
{
	int aDir = theArrow - 1;
	int aFirst = (aDir >= GameRules::FIRST_DIAGONAL_ARROW) ? GameRules::FIRST_DIAGONAL_ARROW : 0;
	return aFirst + (((aDir - aFirst + theQuarters) % 4) + 4) % 4 + 1;
}

int Board::ArrowExitEdges(const ArrowStep& theStep) const
{
	int anEdges = 0;
	if (theStep.mToRow < 0) anEdges |= 1 << GameRules::ARROW_EDGE_TOP;
	if (theStep.mToCol >= mCols) anEdges |= 1 << GameRules::ARROW_EDGE_RIGHT;
	if (theStep.mToRow >= mRows) anEdges |= 1 << GameRules::ARROW_EDGE_BOTTOM;
	if (theStep.mToCol < 0) anEdges |= 1 << GameRules::ARROW_EDGE_LEFT;
	return anEdges;
}

// An escape is the settle's only step: the arrows closest to their edges go first, and the first with none ahead escapes
bool Board::IsArrowEscaping() const
{
	for (const ArrowStep& s : mArrowSteps)
	{
		if ((s.mToCol < 0) || (s.mToCol >= mCols) || (s.mToRow < 0) || (s.mToRow >= mRows))
			return true;
	}
	return false;
}

// A gem on the edge opposite the direction turns into an arrow, so it has the whole board to cross: for each column
// (up, down) or row (left, right), the gem farthest from the edge the arrow points to, if it is no arrow or special
// gem already. A diagonal arrow starts on a gem with no gem behind it on its diagonal, if that gem is at least half
// as far from its edge as the farthest such gem: on a square board, not in the corners it would leave at once; on a
// diamond, anywhere along its lower or upper edge. The direction, then the gem, from the board's RNG.
void Board::AddNewArrow()
{
	int aDirs[GameRules::NUM_ARROW_DIRECTIONS];
	int aDirCount = 0;
	for (int d = 0; d < GameRules::NUM_ARROW_DIRECTIONS; d++)
	{
		if (mRules.mArrows.mDirections & (1 << d))
			aDirs[aDirCount++] = d;
	}
	if (aDirCount == 0)
		return;
	int aDir = aDirs[Rand() % aDirCount];

	std::vector<Piece*> aCandidates;
	if (aDir >= GameRules::FIRST_DIAGONAL_ARROW)
	{
		int aBehind = GameRules::FIRST_DIAGONAL_ARROW + (aDir - GameRules::FIRST_DIAGONAL_ARROW + 2) % 4;
		std::vector<Piece*> aFirsts;
		std::vector<int> aDistances;
		int aLongest = 0;
		for (int aRow = 0; aRow < mRows; aRow++)
		{
			for (int aCol = 0; aCol < mCols; aCol++)
			{
				int aBehindCol, aBehindRow;
				if ((mBoard[aRow][aCol] == NULL) || NextArrowCell(aCol, aRow, aBehind, &aBehindCol, &aBehindRow))
					continue;
				aFirsts.push_back(mBoard[aRow][aCol]);
				aDistances.push_back(ArrowDistance(aCol, aRow, aDir));
				aLongest = std::max(aLongest, aDistances.back());
			}
		}
		for (size_t i = 0; i < aFirsts.size(); i++)
		{
			if ((aFirsts[i]->mArrow == 0) && (aFirsts[i]->mKind == 0) && (2 * aDistances[i] >= aLongest))
				aCandidates.push_back(aFirsts[i]);
		}
	}
	else
	{
		bool isVertical = (aDir == GameRules::ARROW_UP) || (aDir == GameRules::ARROW_DOWN);
		int aLines = isVertical ? mCols : mRows;
		int aLength = isVertical ? mRows : mCols;
		for (int aLine = 0; aLine < aLines; aLine++)
		{
			for (int i = 0; i < aLength; i++)
			{
				// from the edge behind the arrow
				int aPos = ((aDir == GameRules::ARROW_UP) || (aDir == GameRules::ARROW_LEFT)) ? aLength - 1 - i : i;
				Piece* aPiece = isVertical ? mBoard[aPos][aLine] : mBoard[aLine][aPos];
				if (aPiece == NULL)
					continue;
				if ((aPiece->mArrow == 0) && (aPiece->mKind == 0))
					aCandidates.push_back(aPiece);
				break;
			}
		}
	}
	if (aCandidates.empty())
		return;
	Piece* aPiece = aCandidates[Rand() % (int) aCandidates.size()];
	aPiece->mArrow = aDir + 1;
	aPiece->mSpinFrame = 0;
	mApp->PlaySample(SOUND_GOTSETBIG, GetPanPosition((int) aPiece->mX + 24));
	MarkDirty();
}

void Board::ClearArrow(Piece* thePiece)
{
	if ((thePiece == NULL) || (thePiece->mArrow == 0))
		return;
	thePiece->mArrow = 0;
	mArrowsCleared++;
	int aPoints = (int) ((double) mRules.mArrows.mBonus * mArrowsCleared * mPointMultiplier);
	if (aPoints <= 0)
		return;
	ScorePopup aPopup = {(int) thePiece->mX + ScorePopup::OFFSET_X, (int) thePiece->mY + 4, ScorePopup::START_LIFE, -1, aPoints};
	mScorePopups.push_back(aPopup);
	mScore += aPoints;
	if (mStats != NULL) mStats->OnPoints(aPoints);
	AddMoveScore(aPoints);
}

// FindSetsVariant found groups (a board with arrows always takes the match rules' path): every arrow they or a
// blast took
void Board::ClearMatchedArrows()
{
	if (!mVariant.mArrows)
		return;
	for (int aRow = 0; aRow < mRows; aRow++)
	{
		for (int aCol = 0; aCol < mCols; aCol++)
		{
			Piece* aPiece = mBoard[aRow][aCol];
			if ((aPiece != NULL) && aPiece->mDying && (aPiece->mArrow != 0))
				ClearArrow(aPiece);
		}
	}
}
