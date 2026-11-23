// Sexy::WallBlastGoal, GoldLinesGoal and GoldLinesFlipGoal: see the header
#include "WallGoldGoals.h"

#include "Bej3App.h"
#include "Board.h"
#include "Common.h"
#include "Graphics.h"
#include "QuestBoard.h"
#include "X86Math.h"

#include <cstdint>
#include <cstdio>

namespace Sexy {

namespace {

int Scaled(int theMeasure) { return gApp->mArtRes * theMeasure / 1200; }

// 0x744e00 with its second argument set (bejeweled3, which Wall Blast's Load gives): the digits of the text are the cells of a grid, 64 to a grid, row after row; a letter
// is no cell. A grid with fewer than 64 digits has the cells past its last digit at 0 (Wall Blast's first quest has 56)
std::vector<std::vector<int>> ParseWallGrids(const std::string& theText)
{
	std::vector<std::vector<int>> aGrids;
	int aCellIndex = 0;
	for (char aCharacter : theText) {
		if (aCharacter < '0' || aCharacter > '9')
			continue;
		if (aGrids.empty() || aCellIndex > WallBlastGoal::CELL_COUNT - 1) {
			aCellIndex = 0;
			aGrids.push_back(std::vector<int>(WallBlastGoal::CELL_COUNT, 0));
		}
		aGrids.back()[(size_t)aCellIndex] = aCharacter - '0';
		aCellIndex++;
	}
	return aGrids;
}

// the cell a piece's position falls in (0x6b1380: the position through the x87 conversion and a division by the cell's 128; -1 outside the grid's 1024)
int CellOfPosition(float thePosition)
{
	const int aPixels = Ftol((double)thePosition);
	if (aPixels < 0 || aPixels >= GoldLinesGoal::GRID_SPAN)
		return -1;
	return aPixels / GoldLinesGoal::CELL_SIZE;
}

void FillCell(Graphics* g, const QuestBoard* theBoard, int theCol, int theRow)
{
	g->FillRect(Scaled(theBoard->GetGridX() + theCol * Board::CELL + 2), Scaled(theBoard->GetGridY() + theRow * Board::CELL + 2), Scaled(Board::CELL - 4), Scaled(Board::CELL - 4));
}

}  // namespace

// slot 53 (0xd4), 0x6b0fb0 (bejeweled3): DropThreshold is read; the setting Grids is cut into grids (0x744e00) and one is drawn by the app's Rand(); then for each of the 64 cells
// (row after row), a digit other than 0 makes a wall cell, with two draws of Rand() for its look's flags (an even draw sets a flag) and the count of the wall goes up. The drop
// countdown (+0x218) is DropThreshold. port: the tutorial's text and the hint (+0x250) are not made
void WallBlastGoal::Load()
{
	mWallCount = 0;
	mDropThreshold = mBoard->SettingInt("DropThreshold");
	std::vector<std::vector<int>> aGrids;
	auto aGridsIter = mBoard->mSettings.find("Grids");
	if (aGridsIter != mBoard->mSettings.end())
		aGrids = ParseWallGrids(aGridsIter->second);
	mDropCountdown = mDropThreshold;
	if (aGrids.empty())
		return;
	const size_t aChosen = (size_t)((uint32_t)Rand() % (uint32_t)aGrids.size());
	for (int aIndex = 0; aIndex < CELL_COUNT; aIndex++) {
		WallCell& aCell = mCells[aIndex];
		if (aGrids[aChosen][(size_t)aIndex] == 0)
			aCell.mPresent = false;
		else {
			aCell.mPresent = true;
			aCell.mFlipA = (Rand() & 1) == 0;
			aCell.mFlipB = (Rand() & 1) == 0;
			mWallCount++;
		}
		aCell.mWasPresent = aCell.mPresent;
	}
}

// 0x6ad740 (bejeweled3): the wall at the cell (index col + 8 * row) goes: the score and the progress go up
void WallBlastGoal::BreakCell(int theCol, int theRow)
{
	if (theCol < 0 || theCol >= COLS || theRow < 0 || theRow >= ROWS)
		return;
	WallCell& aCell = mCells[theCol + theRow * COLS];
	if (!aCell.mPresent)
		return;
	aCell.mPresent = false;
	mBoard->mScorePoints++;
	mBoard->mLevelProgress++;
}

// slot 16 (0x40), 0x6afcb0 (bejeweled3): a jump to 0x6ad740 with the cell
void WallBlastGoal::OnCellHit(int theCol, int theRow)
{
	BreakCell(theCol, theRow);
}

// slot 39 (0x9c), 0x6ad9d0 (bejeweled3): a piece that is cleared (the flag set), when its match was timed (+0x178 above 0) or it was destroyed (+0x17d), and is in the grid (row 0 or more),
// breaks the wall at its cell
void WallBlastGoal::OnPieceClearing(Piece* thePiece, bool theFlag)
{
	if (theFlag && (thePiece->mMatchTimer > 0.0f || thePiece->mDestroyed) && thePiece->mRow >= 0)
		BreakCell(thePiece->mCol, thePiece->mRow);
}

int WallBlastGoal::CellsLeft() const
{
	int aLeft = 0;
	for (const WallCell& aCell : mCells)
		aLeft += aCell.mPresent ? 1 : 0;
	return aLeft;
}

// port: the wall is drawn as a grey block over each of its cells (the exe's slot 59, 0x6afe80, draws its stones and their cracks)
void WallBlastGoal::DrawBehindPieces(Graphics* g)
{
	for (int aRow = 0; aRow < ROWS; aRow++) {
		for (int aCol = 0; aCol < COLS; aCol++) {
			if (!mCells[aCol + aRow * COLS].mPresent)
				continue;
			g->SetColor(Color(120, 120, 130, 255));
			FillCell(g, mBoard, aCol, aRow);
		}
	}
	g->SetColor(Color(255, 255, 255, 255));
}

std::string WallBlastGoal::DescribeState() const
{
	return ",\"wallcount\":" + std::to_string(mWallCount) + ",\"wallleft\":" + std::to_string(CellsLeft()) + ",\"dropthreshold\":" + std::to_string(mDropThreshold);
}

// slot 53 (0xd4), 0x6b04f0 (bejeweled3): every cell is made level 0 with no flips waiting; Squares and OnlyCheckSwaps (its text is TRUE) are read
void GoldLinesGoal::Load()
{
	for (int aRow = 0; aRow < ROWS; aRow++) {
		for (int aCol = 0; aCol < COLS; aCol++) {
			mCells[aRow][aCol].mLevel = 0;
			mCells[aRow][aCol].mDelays.clear();
		}
	}
	mSquares = mBoard->SettingInt("Squares");
	mOnlyCheckSwaps = mBoard->SettingText("OnlyCheckSwaps") == "TRUE";
}

// slot 53 (0xd4), 0x6b0f20 (bejeweled3): as Gold Lines' (0x6b04f0), then every cell is level -1
void GoldLinesFlipGoal::Load()
{
	GoldLinesGoal::Load();
	mIsFlip = true;
	for (int aRow = 0; aRow < ROWS; aRow++) {
		for (int aCol = 0; aCol < COLS; aCol++)
			mCells[aRow][aCol].mLevel = -1;
	}
}

// slot 10 (0x28), 0x6aaa00 (bejeweled3): the game may end only when no cell has a flip waiting. port: Gold Lines Flip's (0x6ab050) also waits for its effect (+0xdc), which is not made
bool GoldLinesGoal::CanEndGame()
{
	for (int aRow = 0; aRow < ROWS; aRow++) {
		for (int aCol = 0; aCol < COLS; aCol++) {
			if (!mCells[aRow][aCol].mDelays.empty())
				return false;
		}
	}
	return true;
}

// slot 11 (0x2c), 0x6abeb0 (bejeweled3): for each cell with flips waiting, the first one's ticks go down by one; at 0 it is taken out and the cell is flipped. port: the
// sound's counter (+0x93c) is not kept
void GoldLinesGoal::UpdateTick()
{
	for (int aRow = 0; aRow < ROWS; aRow++) {
		for (int aCol = 0; aCol < COLS; aCol++) {
			std::vector<int>& aDelays = mCells[aRow][aCol].mDelays;
			if (aDelays.empty())
				continue;
			aDelays.front()--;
			if (aDelays.front() == 0) {
				aDelays.erase(aDelays.begin());
				FlipCell(aRow, aCol);
			}
		}
	}
}

// slot 26 (0x68), 0x6aa1d0 (bejeweled3): when the score has reached Squares the board's slot 156 is run (the level is full) and the answer is true
bool GoldLinesGoal::IsLevelFull() const
{
	if (mSquares <= mBoard->mScorePoints) {
		mBoard->OnLevelFull();
		return true;
	}
	return false;
}

int GoldLinesGoal::GoldCount() const
{
	int aCount = 0;
	for (int aRow = 0; aRow < ROWS; aRow++) {
		for (int aCol = 0; aCol < COLS; aCol++)
			aCount += mCells[aRow][aCol].mLevel > 0 ? 1 : 0;
	}
	return aCount;
}

// slot 72 (0x120), 0x6b78f0 (bejeweled3): a cell below level 1 goes up one level; one then above 0 adds 1 to the score (+0xd24) and the progress (+0xe00). port: the effect
// the exe makes for the cell and the sound's counter are not made. Gold Lines Flip's slot 72 is a jump here
void GoldLinesGoal::FlipCell(int theRow, int theCol)
{
	GoldCell& aCell = mCells[theRow][theCol];
	if (aCell.mLevel >= 1)
		return;
	aCell.mLevel++;
	if (aCell.mLevel > 0) {
		mBoard->mScorePoints++;
		mBoard->mLevelProgress++;
	}
}

// slot 40 (0xa0), 0x6aa220 (bejeweled3): a piece that was destroyed (+0x17d) flips the cell it was in
void GoldLinesGoal::OnPieceCleared(Piece* thePiece)
{
	if (thePiece->mDestroyed && thePiece->mRow >= 0 && thePiece->mRow < ROWS && thePiece->mCol >= 0 && thePiece->mCol < COLS)
		FlipCell(thePiece->mRow, thePiece->mCol);
}

// slot 17 (0x44), 0x6b1380 (bejeweled3): a run of matched gems. The cell of each gem is its position over 128 (-1 outside the grid). With the first two gems in one column the run is
// vertical: the middle is the sum of its gems' rows over their count; for a distance from 0 to 7 the rows the middle less it and the middle plus it (only one at 0) that are in the
// grid are flipped: the middle's at once, the others after 10 updates for each step of distance. The same along the row for two gems in one row. The wave's count of cells
// that were not gold yet (the exe's +0x138 of each cell) only feeds a sound. port: the setting OnlyCheckSwaps (+0x118), which would count only the runs that contain a gem of a swap, is
// false in every quest, and the list of the runs seen (+0x928) is never filled by Gold Lines, so neither is made
void GoldLinesGoal::OnRunFound(const std::vector<Piece*>& thePieces)
{
	if (thePieces.size() < 2)
		return;
	const Piece* aFirst = thePieces[0];
	const Piece* aSecond = thePieces[1];
	const int aFirstCol = CellOfPosition(aFirst->mX);
	const int aFirstRow = CellOfPosition(aFirst->mY);
	const int aSecondCol = CellOfPosition(aSecond->mX);
	const int aSecondRow = CellOfPosition(aSecond->mY);
	const int aCount = (int)thePieces.size();
	if (aFirstCol == aSecondCol && aFirstCol >= 0) {
		int aRowSum = 0;
		for (const Piece* aPiece : thePieces)
			aRowSum += CellOfPosition(aPiece->mY);
		const int aMiddle = aRowSum / aCount;
		for (int aStep = 0; aStep < ROWS; aStep++) {
			for (int aSide = 0; aSide < 2; aSide++) {
				if (aSide == 1 && aStep == 0)
					continue;
				const int aRow = aSide == 0 ? aMiddle - aStep : aMiddle + aStep;
				if ((unsigned int)aRow >= (unsigned int)ROWS)
					continue;
				if (aStep == 0)
					FlipCell(aRow, aFirstCol);
				else
					mCells[aRow][aFirstCol].mDelays.push_back(aStep * TICKS_PER_STEP);
			}
		}
	}
	else if (aFirstRow == aSecondRow && aFirstRow >= 0) {
		int aColumnSum = 0;
		for (const Piece* aPiece : thePieces)
			aColumnSum += CellOfPosition(aPiece->mX);
		const int aMiddle = aColumnSum / aCount;
		for (int aStep = 0; aStep < COLS; aStep++) {
			for (int aSide = 0; aSide < 2; aSide++) {
				if (aSide == 1 && aStep == 0)
					continue;
				const int aCol = aSide == 0 ? aMiddle - aStep : aMiddle + aStep;
				if ((unsigned int)aCol >= (unsigned int)COLS)
					continue;
				if (aStep == 0)
					FlipCell(aFirstRow, aCol);
				else
					mCells[aFirstRow][aCol].mDelays.push_back(aStep * TICKS_PER_STEP);
			}
		}
	}
}

// port: a gold cell is tinted gold under the gems, and a lead cell of Gold Lines Flip grey (the exe's slot 59 draws the plates and their flips)
void GoldLinesGoal::DrawBehindPieces(Graphics* g)
{
	for (int aRow = 0; aRow < ROWS; aRow++) {
		for (int aCol = 0; aCol < COLS; aCol++) {
			const int aLevel = mCells[aRow][aCol].mLevel;
			if (aLevel < 0 || (aLevel == 0 && !mIsFlip))
				continue;
			g->SetColor(aLevel > 0 ? Color(240, 196, 40, 255) : Color(130, 130, 140, 255));
			FillCell(g, mBoard, aCol, aRow);
		}
	}
	g->SetColor(Color(255, 255, 255, 255));
}

std::string GoldLinesGoal::DescribeState() const
{
	return ",\"squares\":" + std::to_string(mSquares) + ",\"gold\":" + std::to_string(GoldCount());
}

}  // namespace Sexy
