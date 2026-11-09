// port(variant): the match rules' finder (MatchFinder.h). Not part of the original game.

#include "MatchFinder.h"

#include <algorithm>

using namespace Sexy;
using namespace Sexy::MatchFinder;

namespace
{

// The runs of at least theMinRun cells of one colour along the line from (theCol, theRow) in steps of
// (theStepCol, theStepRow), in scan order
void ScanLine(const ColorGrid& theGrid, int theCol, int theRow, int theStepCol, int theStepRow, int theMinRun, GroupKind theKind,
	std::vector<Group>* theRuns)
{
	int aColor = NO_GEM;
	int aCount = 0;
	int aStartCol = theCol;
	int aStartRow = theRow;
	for (int aCol = theCol, aRow = theRow; ; aCol += theStepCol, aRow += theStepRow)
	{
		bool isInside = (aCol >= 0) && (aRow >= 0) && (aCol < theGrid.mCols) && (aRow < theGrid.mRows);
		int aHere = isInside ? theGrid.mColor[aRow][aCol] : (int) NO_GEM;
		if ((aHere != NO_GEM) && (aHere == aColor))
		{
			aCount++;
			continue;
		}
		if ((aColor != NO_GEM) && (aCount >= theMinRun))
		{
			Group aRun;
			aRun.mKind = theKind;
			aRun.mColor = aColor;
			aRun.mRunLength = aCount;
			for (int i = 0; i < aCount; i++)
				aRun.mCells.push_back(Cell(aStartCol + i * theStepCol, aStartRow + i * theStepRow));
			theRuns->push_back(aRun);
		}
		if (!isInside)
			break;
		aColor = aHere;
		aCount = 1;
		aStartCol = aCol;
		aStartRow = aRow;
	}
}

// Every straight run the rules form, in the finder's order
void FindRuns(const ColorGrid& theGrid, const Spec& theSpec, std::vector<Group>* theRuns)
{
	for (int aCol = 0; aCol < theGrid.mCols; aCol++)
		ScanLine(theGrid, aCol, 0, 0, 1, theSpec.mMinRun, GROUP_VERTICAL, theRuns);
	for (int aRow = 0; aRow < theGrid.mRows; aRow++)
		ScanLine(theGrid, 0, aRow, 1, 0, theSpec.mMinRun, GROUP_HORIZONTAL, theRuns);
	if (!theSpec.mDiagonal)
		return;
	// "\" lines from their top-left end, "/" lines from their top-right end: the top row's cells left to right,
	// then the edge column's cells top to bottom
	for (int aCol = 0; aCol < theGrid.mCols; aCol++)
		ScanLine(theGrid, aCol, 0, 1, 1, theSpec.mDiagonalMinRun, GROUP_DIAGONAL_DOWN, theRuns);
	for (int aRow = 1; aRow < theGrid.mRows; aRow++)
		ScanLine(theGrid, 0, aRow, 1, 1, theSpec.mDiagonalMinRun, GROUP_DIAGONAL_DOWN, theRuns);
	for (int aCol = 0; aCol < theGrid.mCols; aCol++)
		ScanLine(theGrid, aCol, 0, -1, 1, theSpec.mDiagonalMinRun, GROUP_DIAGONAL_UP, theRuns);
	for (int aRow = 1; aRow < theGrid.mRows; aRow++)
		ScanLine(theGrid, theGrid.mCols - 1, aRow, -1, 1, theSpec.mDiagonalMinRun, GROUP_DIAGONAL_UP, theRuns);
}

int IndexOf(const Group& theGroup, const Cell& theCell)
{
	for (size_t i = 0; i < theGroup.mCells.size(); i++)
	{
		if (theGroup.mCells[i] == theCell)
			return (int) i;
	}
	return -1;
}

int Root(std::vector<int>* theParents, int theIndex)
{
	while ((*theParents)[theIndex] != theIndex)
		theIndex = (*theParents)[theIndex] = (*theParents)[(*theParents)[theIndex]];
	return theIndex;
}

// match.crossings "merge": the runs that share a cell (and so a colour) as one group each, at its first run's place
void MergeRuns(const std::vector<Group>& theRuns, std::vector<Group>* theGroups)
{
	std::vector<int> aParents(theRuns.size());
	for (size_t i = 0; i < theRuns.size(); i++)
		aParents[i] = (int) i;
	int aFirst[MAX_ROWS][MAX_COLS];
	for (int r = 0; r < MAX_ROWS; r++)
		std::fill(aFirst[r], aFirst[r] + MAX_COLS, -1);
	for (size_t i = 0; i < theRuns.size(); i++)
	{
		for (const Cell& c : theRuns[i].mCells)
		{
			int& aRun = aFirst[c.second][c.first];
			if (aRun < 0)
				aRun = (int) i;
			else
				aParents[Root(&aParents, (int) i)] = Root(&aParents, aRun);
		}
	}

	std::vector<int> aGroupOf(theRuns.size(), -1);
	std::vector<std::vector<int> > aMembers;
	size_t aBase = theGroups->size();
	for (size_t i = 0; i < theRuns.size(); i++)
	{
		int aRoot = Root(&aParents, (int) i);
		if (aGroupOf[aRoot] < 0)
		{
			aGroupOf[aRoot] = (int) aMembers.size();
			aMembers.push_back(std::vector<int>());
			theGroups->push_back(theRuns[i]);
		}
		else
		{
			Group& aGroup = (*theGroups)[aBase + aGroupOf[aRoot]];
			for (const Cell& c : theRuns[i].mCells)
			{
				if (IndexOf(aGroup, c) < 0)
					aGroup.mCells.push_back(c);
			}
		}
		aMembers[aGroupOf[aRoot]].push_back((int) i);
	}

	// A group of two runs or more: where they cross. Each pair that shares a cell crosses at an end of both (L), at an
	// end of one (T) or inside both (plus); the group takes the largest of its crossings.
	for (size_t g = 0; g < aMembers.size(); g++)
	{
		const std::vector<int>& aRuns = aMembers[g];
		if (aRuns.size() < 2)
			continue;
		int aRank = 0;
		for (size_t a = 0; a < aRuns.size(); a++)
		{
			const Group& aRunA = theRuns[aRuns[a]];
			for (size_t b = a + 1; b < aRuns.size(); b++)
			{
				const Group& aRunB = theRuns[aRuns[b]];
				for (size_t i = 0; i < aRunA.mCells.size(); i++)
				{
					int j = IndexOf(aRunB, aRunA.mCells[i]);
					if (j < 0)
						continue;
					bool isEndA = (i == 0) || (i + 1 == aRunA.mCells.size());
					bool isEndB = (j == 0) || (j + 1 == (int) aRunB.mCells.size());
					aRank = std::max(aRank, (isEndA && isEndB) ? 1 : (isEndA || isEndB) ? 2 : 3);
				}
			}
		}
		Group& aGroup = (*theGroups)[aBase + g];
		aGroup.mKind = (aRank == 3) ? GROUP_PLUS : (aRank == 2) ? GROUP_T : GROUP_L;
		aGroup.mRunLength = 0;
	}
}

// Whether the orientation placed at (theCol, theRow) lies on cells of one colour; that colour, else NO_GEM
int PlacedColor(const ColorGrid& theGrid, const std::vector<Cell>& theCells, int theCol, int theRow)
{
	int aColor = theGrid.At(theCol + theCells[0].first, theRow + theCells[0].second);
	if (aColor == NO_GEM)
		return NO_GEM;
	for (size_t i = 1; i < theCells.size(); i++)
	{
		if (theGrid.At(theCol + theCells[i].first, theRow + theCells[i].second) != aColor)
			return NO_GEM;
	}
	return aColor;
}

// The length of the line of theCol, theRow's colour through it along (theStepCol, theStepRow), both ways
int LineThrough(const ColorGrid& theGrid, int theCol, int theRow, int theStepCol, int theStepRow)
{
	int aColor = theGrid.At(theCol, theRow);
	int aCount = 1;
	for (int i = 1; theGrid.At(theCol + i * theStepCol, theRow + i * theStepRow) == aColor; i++)
		aCount++;
	for (int i = 1; theGrid.At(theCol - i * theStepCol, theRow - i * theStepRow) == aColor; i++)
		aCount++;
	return aCount;
}

}

bool Group::Contains(int theCol, int theRow) const
{
	for (const Cell& c : mCells)
	{
		if ((c.first == theCol) && (c.second == theRow))
			return true;
	}
	return false;
}

Spec Spec::FromRules(const GameRules& theRules)
{
	Spec aSpec;
	const GameRules::MatchRules& aMatch = theRules.mMatch;
	aSpec.mMinRun = aMatch.mMinRun;
	aSpec.mDiagonal = aMatch.mDiagonal;
	aSpec.mDiagonalMinRun = aMatch.mDiagonalMinRun;
	aSpec.mMerge = aMatch.mMergeCrossings;
	for (std::map<std::string, ShapePattern>::const_iterator it = aMatch.mShapes.begin(); it != aMatch.mShapes.end(); ++it)
	{
		Shape aShape;
		aShape.mId = it->first;
		aShape.mOrientations = GameRules::ShapeOrientations(it->second);
		if (!aShape.mOrientations.empty() && !aShape.mOrientations[0].empty())
			aSpec.mShapes.push_back(aShape);
	}
	return aSpec;
}

bool Spec::IsDefault() const
{
	return (mMinRun == 3) && !mDiagonal && !mMerge && mShapes.empty();
}

void MatchFinder::FindGroups(const ColorGrid& theGrid, const Spec& theSpec, std::vector<Group>* theGroups)
{
	theGroups->clear();
	std::vector<Group> aRuns;
	FindRuns(theGrid, theSpec, &aRuns);
	if (theSpec.mMerge)
		MergeRuns(aRuns, theGroups);
	else
		theGroups->swap(aRuns);

	for (size_t aShapeIndex = 0; aShapeIndex < theSpec.mShapes.size(); aShapeIndex++)
	{
		const Spec::Shape& aShape = theSpec.mShapes[aShapeIndex];
		for (int aRow = 0; aRow < theGrid.mRows; aRow++)
		{
			for (int aCol = 0; aCol < theGrid.mCols; aCol++)
			{
				for (const std::vector<Cell>& anOrientation : aShape.mOrientations)
				{
					int aColor = PlacedColor(theGrid, anOrientation, aCol, aRow);
					if (aColor == NO_GEM)
						continue;
					bool isInOne = false;
					for (size_t g = 0; (g < theGroups->size()) && !isInOne; g++)
					{
						const Group& aGroup = (*theGroups)[g];
						isInOne = (aGroup.mColor == aColor);
						for (size_t i = 0; (i < anOrientation.size()) && isInOne; i++)
							isInOne = aGroup.Contains(aCol + anOrientation[i].first, aRow + anOrientation[i].second);
					}
					if (isInOne)
						continue;
					Group anInstance;
					anInstance.mKind = GROUP_SHAPE;
					anInstance.mShape = (int) aShapeIndex;
					anInstance.mColor = aColor;
					for (const Cell& c : anOrientation)
						anInstance.mCells.push_back(Cell(aCol + c.first, aRow + c.second));
					theGroups->push_back(anInstance);
				}
			}
		}
	}
}

bool MatchFinder::AnyGroup(const ColorGrid& theGrid, const Spec& theSpec)
{
	// Every group holds a run or a shape instance (merging joins runs; a skipped instance lies in a group), so the
	// grid has a group when any cell starts a run or anchors an instance. An anchor is the corner of an
	// orientation's box, not always one of its cells (an L's), so a hole anchors instances too.
	for (int aRow = 0; aRow < theGrid.mRows; aRow++)
	{
		for (int aCol = 0; aCol < theGrid.mCols; aCol++)
		{
			int aColor = theGrid.mColor[aRow][aCol];
			static const int kSteps[4][2] = {{1, 0}, {0, 1}, {1, 1}, {-1, 1}};
			for (int d = 0; (d < (theSpec.mDiagonal ? 4 : 2)) && (aColor != NO_GEM); d++)
			{
				int aNeed = (d < 2) ? theSpec.mMinRun : theSpec.mDiagonalMinRun;
				int aCount = 1;
				while ((aCount < aNeed) && (theGrid.At(aCol + aCount * kSteps[d][0], aRow + aCount * kSteps[d][1]) == aColor))
					aCount++;
				if (aCount >= aNeed)
					return true;
			}
			for (const Spec::Shape& aShape : theSpec.mShapes)
			{
				for (const std::vector<Cell>& anOrientation : aShape.mOrientations)
				{
					if (PlacedColor(theGrid, anOrientation, aCol, aRow) != NO_GEM)
						return true;
				}
			}
		}
	}
	return false;
}

bool MatchFinder::InGroup(const ColorGrid& theGrid, const Spec& theSpec, int theCol, int theRow, bool theHorizontal, bool theVertical)
{
	if (theGrid.At(theCol, theRow) == NO_GEM)
		return false;
	if (theHorizontal && (LineThrough(theGrid, theCol, theRow, 1, 0) >= theSpec.mMinRun))
		return true;
	if (theVertical && (LineThrough(theGrid, theCol, theRow, 0, 1) >= theSpec.mMinRun))
		return true;
	if (!theHorizontal || !theVertical)
		return false;
	if (theSpec.mDiagonal && ((LineThrough(theGrid, theCol, theRow, 1, 1) >= theSpec.mDiagonalMinRun) ||
		(LineThrough(theGrid, theCol, theRow, -1, 1) >= theSpec.mDiagonalMinRun)))
		return true;
	for (const Spec::Shape& aShape : theSpec.mShapes)
	{
		for (const std::vector<Cell>& anOrientation : aShape.mOrientations)
		{
			for (const Cell& c : anOrientation)
			{
				if (PlacedColor(theGrid, anOrientation, theCol - c.first, theRow - c.second) != NO_GEM)
					return true;
			}
		}
	}
	return false;
}

const char* MatchFinder::BonusKey(const Group& theGroup, const Spec& theSpec)
{
	switch (theGroup.mKind)
	{
	case GROUP_DIAGONAL_DOWN:
	case GROUP_DIAGONAL_UP:
		return "diagonal";
	case GROUP_L:
		return "L";
	case GROUP_T:
		return "T";
	case GROUP_PLUS:
		return "plus";
	case GROUP_SHAPE:
		return theSpec.mShapes[theGroup.mShape].mId.c_str();
	default:
		return NULL;
	}
}
