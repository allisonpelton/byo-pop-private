// port(variant): the match rules on a board (games/bejeweled/docs/CODE_MODS_DESIGN.md 4.6 and 4.7, stage 9): the
// variant paths of FindSets, HasSet and FindMove, on MatchFinder's groups. Not part of the original game.
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

// HasSet (0x4c9cf5) under the match rules: the fill rerolls a colour while the grid has any group
bool Board::HasSetVariant()
{
	MatchFinder::ColorGrid aGrid;
	FillColorGrid(&aGrid);
	return MatchFinder::AnyGroup(aGrid, mMatchSpec);
}

// FindMove (0x4c9dda) under the match and move rules: MoveSet's search (MoveSet.h), the swaps first in the
// original's order and test, each swap found from both of its cells, so at default rules the n-th move is the
// original's. theMove keeps FindMove's {col, row, col2, row2}, read as "the gem at (col2, row2) moves into (col, row),
// where it lands in a group": a swap's two cells, or the cell a twist or a slide brings that gem into and the cell
// it comes from (TryShowHint sparkles it there). FindVariantMove gives the move itself (BoardMoves.cpp).
bool Board::FindMoveVariant(int* theMove, int theMoveNum, bool horizontal, bool vertical)
{
	MatchFinder::ColorGrid aGrid;
	FillColorGrid(&aGrid);
	MoveSet::Found aFound;
	if (!MoveSet::FindMove(aGrid, mLayout, mMatchSpec, MoveOptions(horizontal, vertical), theMoveNum, &aFound))
		return false;
	if (theMove != NULL)
	{
		theMove[0] = aFound.mTo.first;
		theMove[1] = aFound.mTo.second;
		theMove[2] = aFound.mFrom.first;
		theMove[3] = aFound.mFrom.second;
	}
	return true;
}

// FindSets (0x4cd287)'s scoring block for one group: the original's for a vertical or horizontal run, plus
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

	ScorePopup aPopup = {anAvgX + ScorePopup::OFFSET_X, anAvgY + ScorePopup::OFFSET_Y, ScorePopup::START_LIFE, -1, aPoints};
	mScorePopups.push_back(aPopup);
	mGemsCleared += aCount;
	int aScoreBefore = mScore;
	mScore += aPoints;
	// A negative match.bonus, as the hint penalty: never below 0. Without one the points add as the original adds them,
	// so a score past the int's range wraps or turns INT_MIN as there (mods/stage4/overflow)
	if ((aBonus < 0) && (mScore < 0))
		mScore = 0;
	if (mStats != NULL) // the Stats page (GameStats.h): a shape counts with the runs of 5 or more
		mStats->OnRun(theGroup.IsRun() ? theGroup.mRunLength : std::max(aCount, 5),
			mBoard[theGroup.mCells[0].second][theGroup.mCells[0].first]->mColor, mScore - aScoreBefore);
	if (mVariant.mDirt)
		ClearDirtNextToCells(theGroup.mCells);
	if (mMoveMatchCount == 1)
	{
		mHintGlowAlpha = 0.0;
		mHintGlowTimer = 0;
		mMoveScore = 0;
		mClearedAtMoveStart = 0;
	}
	AddMoveScore(aPoints);
}

// ScoreGroup's end (0x4cd287's, for a run), and a blast's (BoardSpecials.cpp): the move's score so far, EXCELLENT when
// it passes the mark, the score timer
void Board::AddMoveScore(int thePoints)
{
	int anExcellentScore = (int)(mPointMultiplier * gGameConfig.mExcellentFactor);
	if ((mMoveScore < anExcellentScore) && (mMoveScore + thePoints >= anExcellentScore))
	{
		DoBouncyText(mApp->GetString("EXCELLENT"), 200);
		mApp->PlaySample(SOUND_EXCELLENT);
	}
	mMoveScore += thePoints;
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

// FindSets (0x4cd287) under the match rules: MatchFinder's groups in its order, each scored (ScoreGroup) and
// its gems marked dying, then the original's sounds with a group as a run (its gems as the run's length)
bool Board::FindSetsVariant()
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
	// Stage 12: the groups' cells turn gold (BoardGold.cpp)
	if (mVariant.mGold && (mRules.mGold.mGild != GameRules::GILD_BLASTS))
	{
		for (const MatchFinder::Group& aGroup : aGroups)
			GildCells(aGroup.mCells);
	}
	// Stage 11: the specials the groups make and set off (BoardSpecials.cpp)
	if (mVariant.mSpecials && !aGroups.empty())
		ResolveSpecials(aGroups);
	// Stage 18: the arrows the groups and blasts took are cleared (BoardArrows.cpp)
	if (mVariant.mArrows && !aGroups.empty())
		ClearMatchedArrows();

	int aNumMatches = (int) aGroups.size();
	if (aNumMatches > 0)
	{
		int aPan = GetPanPosition(anAvgXSum / aNumMatches + 24);
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
