// port(variant): the match rules on a board (games/bejeweled/docs/CODE_MODS_DESIGN.md 4.6 and 4.7, stage 9): the
// variant paths of ProcessMatches, HasMatch and FindMove, on MatchFinder's groups. Not part of the original game.
// -forcevariant runs them at default rules, where they must play exactly as the originals.

#include "Board.h"
#include "WinDMApp.h"
#include "GameConfig.h"
#include "Res.h"

#include <algorithm>

using namespace Sexy;

void Board::FillColorGrid(MatchFinder::ColorGrid* theGrid) const
{
	theGrid->mCols = mCols;
	theGrid->mRows = mRows;
	for (int aRow = 0; aRow < mRows; aRow++)
	{
		for (int aCol = 0; aCol < mCols; aCol++)
		{
			Piece* aPiece = mBoard[aRow][aCol];
			theGrid->mColor[aRow][aCol] = (aPiece != NULL) ? aPiece->mColor : (int) MatchFinder::NO_GEM;
		}
	}
}

// HasMatch (0x4c9cf5) under the match rules: the fill rerolls a colour while the grid has any group
bool Board::HasMatchVariant()
{
	MatchFinder::ColorGrid aGrid;
	FillColorGrid(&aGrid);
	return MatchFinder::AnyGroup(aGrid, mMatchSpec);
}

// FindMove (0x4c9dda) under the match rules, for swaps: the original's candidates in its order (rows, columns,
// then the directions right, left, down, up), each swap applied to a copy of the grid. A swap counts when the gem
// it brings into (aCol, aRow) lands in a group (horizontal: a horizontal run, vertical: a vertical one, both: any
// group), so each swap is found from both of its cells, as in the original, and at default rules the n-th move is
// the original's.
bool Board::FindMoveVariant(int* theMove, int theMoveNum, bool horizontal, bool vertical)
{
	static const int aDirs[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};

	MatchFinder::ColorGrid aGrid;
	FillColorGrid(&aGrid);
	int aMoveCount = 0;
	for (int aRow = 0; aRow < mRows; aRow++)
	{
		for (int aCol = 0; aCol < mCols; aCol++)
		{
			if (mBoard[aRow][aCol] == NULL)
				continue;
			for (int aDir = 0; aDir < 4; aDir++)
			{
				int aCol2 = aCol + aDirs[aDir][0];
				int aRow2 = aRow + aDirs[aDir][1];
				if ((aCol2 < 0) || (aCol2 >= mCols) || (aRow2 < 0) || (aRow2 >= mRows) || (mBoard[aRow2][aCol2] == NULL))
					continue;
				std::swap(aGrid.mColor[aRow][aCol], aGrid.mColor[aRow2][aCol2]);
				bool isMove = MatchFinder::InGroup(aGrid, mMatchSpec, aCol, aRow, horizontal, vertical);
				std::swap(aGrid.mColor[aRow][aCol], aGrid.mColor[aRow2][aCol2]);
				if (!isMove)
					continue;
				if (aMoveCount == theMoveNum)
				{
					if (theMove != NULL)
					{
						theMove[0] = aCol;
						theMove[1] = aRow;
						theMove[2] = aCol2;
						theMove[3] = aRow2;
					}
					return true;
				}
				aMoveCount++;
			}
		}
	}
	return false;
}

// ProcessMatches (0x4cd287)'s scoring block for one group: the original's for a vertical or horizontal run, plus
// match.bonus for its pattern before the multiplier; the length bonuses for straight runs only (diagonals included)
void Board::ScoreGroup(const MatchFinder::Group& theGroup, int* theAvgX)
{
	int aCount = (int) theGroup.mCells.size();
	int aSumX = 0;
	int aSumY = 0;
	for (const MatchFinder::Cell& c : theGroup.mCells)
	{
		Piece* aPiece = mBoard[c.second][c.first];
		aSumX = (int)(aSumX + aPiece->mX);
		aSumY = (int)(aSumY + aPiece->mY);
	}
	int anAvgX = aSumX / aCount;
	int anAvgY = aSumY / aCount;
	*theAvgX = anAvgX;

	mMoveMatchCount++;
	int aBasePoints = gGameConfig.SetValue(mMoveMatchCount);
	if (theGroup.IsRun())
	{
		if (theGroup.mRunLength > 3)
			aBasePoints += (theGroup.mRunLength - 3) * gGameConfig.mLengthBonus;
		if (theGroup.mRunLength > 5)
			aBasePoints += (theGroup.mRunLength - 5) * gGameConfig.mLongLengthBonus;
	}
	const char* aKey = MatchFinder::BonusKey(theGroup, mMatchSpec);
	int aBonus = 0;
	if (aKey != NULL)
	{
		std::map<std::string, int>::const_iterator it = mRules.mMatch.mBonus.find(aKey);
		if (it != mRules.mMatch.mBonus.end())
			aBonus = it->second;
	}
	aBasePoints += aBonus;
	int aPoints = (int)(aBasePoints * mPointMultiplier);

	ScorePopup aPopup = {anAvgX + 24, anAvgY + 16, 110, -1, aPoints};
	mScorePopups.push_back(aPopup);
	mGemsCleared += aCount;
	mScore += aPoints;
	// A negative match.bonus, as the hint penalty: never below 0. Without one the points add as the original adds them,
	// so a score past the int's range wraps or turns INT_MIN as there (mods/stage4/overflow)
	if ((aBonus < 0) && (mScore < 0))
		mScore = 0;
	if (mVariant.mDirt)
		ClearDirtNextToCells(theGroup.mCells);
	if (mMoveMatchCount == 1)
	{
		mHintGlowAlpha = 0.0;
		mHintGlowTimer = 0;
		mMoveScore = 0;
		mUnused218 = 0;
	}

	int anExcellentScore = (int)(mPointMultiplier * gGameConfig.mExcellentFactor);
	if ((mMoveScore < anExcellentScore) && (mMoveScore + aPoints >= anExcellentScore))
	{
		ShowText(mApp->GetString("EXCELLENT"), 200);
		mApp->PlaySample(SOUND_EXCELLENT);
	}
	mMoveScore += aPoints;
	if (mMoveScoreTimer != -1)
	{
		double aTicks = mMoveScore / mPointMultiplier * 3.0;
		if (aTicks < 300.0)
			aTicks = 300.0;
		else if (aTicks >= 1200.0)
			aTicks = 1200.0;
		mMoveScoreTimer = (int)aTicks;
	}
}

// ProcessMatches (0x4cd287) under the match rules: MatchFinder's groups in its order, each scored (ScoreGroup) and
// its gems marked dying, then the original's sounds with a group as a run (its gems as the run's length)
bool Board::ProcessMatchesVariant()
{
	MatchFinder::ColorGrid aGrid;
	FillColorGrid(&aGrid);
	std::vector<MatchFinder::Group> aGroups;
	MatchFinder::FindGroups(aGrid, mMatchSpec, &aGroups);

	int anAvgXSum = 0;
	int aMaxLength = 0;
	for (const MatchFinder::Group& aGroup : aGroups)
	{
		for (const MatchFinder::Cell& c : aGroup.mCells)
		{
			Piece* aPiece = mBoard[c.second][c.first];
			aPiece->mDyingFrame = 0;
			aPiece->mDying = true;
		}
		mGroupCounts[aGroup.mKind]++;
		int anAvgX = 0;
		ScoreGroup(aGroup, &anAvgX);
		anAvgXSum += anAvgX;
		aMaxLength = std::max(aMaxLength, (int) aGroup.mCells.size());
	}

	int aNumMatches = (int) aGroups.size();
	if (aNumMatches > 0)
	{
		int aPan = GetPanForX(anAvgXSum / aNumMatches + 24);
		mIdleTicks = 0;
		mApp->PlaySample((aMaxLength == 3) ? SOUND_GOTSET : SOUND_GOTSETBIG, aPan);
		int aCombo = mCascadeCount - 1;
		if (aCombo >= 5)
			aCombo = 4;
		mApp->PlaySample(SOUND_COMBO2 + aCombo, aPan);
		if (aNumMatches > 1)
			mApp->PlaySample(SOUND_MULTISHOT, aPan);
	}
	return aNumMatches > 0;
}
