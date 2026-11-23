// Sexy::HiddenObjectGoal: see the header
#include "HiddenObjectGoal.h"

#include "Bej3App.h"
#include "Board.h"
#include "Common.h"
#include "Graphics.h"
#include "QuestBoard.h"
#include "X86Math.h"

#include <cstdlib>
#include <cstring>
#include <utility>

namespace Sexy {

namespace {

int Scaled(int theMeasure) { return gApp->mArtRes * theMeasure / 1200; }

// the exe's random_shuffle (0x4215a0, the CRT's): each element from the second on is swapped with one of the elements up to and including itself, chosen by the app's Rand()
// modulo the number of those (0x5866b0: the draw modulo the number)
void ShuffleWithAppRand(std::vector<int>& theValues)
{
	unsigned int aRange = 2;
	for (size_t aNext = 1; aNext < theValues.size(); aNext++, aRange++) {
		const size_t aOther = (size_t)((unsigned int)Rand() % aRange);
		if (aNext != aOther)
			std::swap(theValues[aNext], theValues[aOther]);
	}
}

}  // namespace

// the ten records of 0x6aec80 (the shape's width and height, its picture and its place of the drawing are not all kept: the picture and the offsets of its drawing are the exe's looks). Only the
// record with the pattern "X  X  XXX" (the string at 0x7d5190) has one: the others fill their rectangle
const HiddenObjectGoal::Shape HiddenObjectGoal::SHAPES[HiddenObjectGoal::SHAPE_COUNT] = {
	{1, 3, ""}, {2, 2, ""}, {2, 3, ""}, {4, 2, ""}, {2, 2, ""}, {2, 3, ""}, {2, 2, ""}, {3, 3, "X  X  XXX"}, {3, 2, ""}, {2, 4, ""}};

// 0x6afb60 (bejeweled3): the refill time is 10,000,000 (never counts down) and the board's quest clock starts at 550 and waits 200 (0x226 and 200 at +0x318c and +0x3190). port: the two effect
// lists the exe makes (+0x1f58 and +0x1f5c) are not
HiddenObjectGoal::HiddenObjectGoal(QuestBoard* theBoard)
{
	mBoard = theBoard;
	mBoard->mQuestClockStart = CLOCK_START;
	mBoard->mQuestClockWait = CLOCK_WAIT;
}

// slot 53 (0xd4), 0x6af970 (bejeweled3): ItemSet is read; RefillRateSec, when the quest has it, is a float of seconds taken to ticks (times 100); the board's flag at +0x269c is cleared (not
// read by anything translated); then the objects are placed (0x6aec80)
void HiddenObjectGoal::Load()
{
	mItemSet = mBoard->SettingInt("ItemSet");
	if (mBoard->HasSetting("RefillRateSec"))
		mRefillTicks = Ftol(mBoard->SettingDouble("RefillRateSec") * 100.0);
	mNeverAllowCascades = mBoard->SettingText("NeverAllowCascades") == "TRUE";
	PlaceObjects();
}

// 0x6aec80 (bejeweled3): up to 15 layouts are made, each by trying the shapes of the item set (ids 0 to 2, 3 to 5 or 6 to 9) in the order the shuffle (0x4215a0) gives them, last first. A shape
// tries 499 places: a draw for the top row, from 0 to 7 less its height (the grid's last row is never used), and one for the left column, from 1 to 6 less its width less 1 (the first and last
// columns of the rows 0 and 7 are corners that no shape may cover); a place is taken when each cell of the shape (the pattern's X, or all) is free and not a corner. The shape is then an object (with a draw
// modulo 1000) and its cells hold its index. A shape that is placed or that finds no place is taken off the list. The layout with the greatest sum, over its objects, of the least squared
// distance between the object's middle and another's (the cell of the grid's last row each take 2 off: never any) is kept (the first of equals), and its cells are the tiles
void HiddenObjectGoal::PlaceObjects()
{
	int aFirstShape = 0;
	int aLastShape = -1;
	if (mItemSet == 1) {
		aFirstShape = 0;
		aLastShape = 2;
	}
	else if (mItemSet == 2) {
		aFirstShape = 3;
		aLastShape = 5;
	}
	else if (mItemSet == 3) {
		aFirstShape = 6;
		aLastShape = 9;
	}

	std::vector<int> aGrids[LAYOUT_COUNT];            // the layouts' cells (64 object indexes each)
	std::vector<HiddenObject> aLayouts[LAYOUT_COUNT];   // the layouts' objects
	int aLayoutCount = 0;
	for (int aTry = 0; aTry < 100; aTry++) {
		if (aLayoutCount > LAYOUT_COUNT - 1)
			break;
		std::vector<int>& aGrid = aGrids[aLayoutCount];
		std::vector<HiddenObject>& anObjects = aLayouts[aLayoutCount];
		aGrid.assign(CELL_COUNT, -1);
		anObjects.clear();
		std::vector<int> aCandidates;
		for (int aShape = aFirstShape; aShape <= aLastShape; aShape++)
			aCandidates.push_back(aShape);
		if (!aCandidates.empty())
			ShuffleWithAppRand(aCandidates);
		while (!aCandidates.empty()) {
			const Shape& aShape = SHAPES[aCandidates.back()];
			const bool hasPattern = aShape.mPattern[0] != 0;
			const int aEdge = 1;   // the record's byte at +0xc is 0 for every shape: 1 less row and a first column of 1
			bool isBlocked = true;
			int aTop = 0;
			int aLeft = 0;
			for (int aTriesLeft = PLACE_TRIES; aTriesLeft > 0; aTriesLeft--) {
				aTop = (int)Rand() % (9 - aShape.mHeight - aEdge);
				aLeft = (int)Rand() % (9 - 2 - aShape.mWidth) + aEdge;
				isBlocked = false;
				for (int aRow = aTop; aRow < aTop + aShape.mHeight && !isBlocked; aRow++) {
					for (int aCol = aLeft; aCol < aLeft + aShape.mWidth && !isBlocked; aCol++) {
						const bool isCorner = (aRow == 0 || aRow == ROWS - 1) && (aCol == 0 || aCol == COLS - 1);
						const bool isTaken = aGrid[(size_t)(aCol + aRow * COLS)] != -1 || isCorner;
						if (!isTaken)
							continue;
						// the cell blocks when it belongs to the shape: no pattern, or an X
						if (!hasPattern || aShape.mPattern[(aRow - aTop) * aShape.mWidth + (aCol - aLeft)] == 'X')
							isBlocked = true;
					}
				}
				if (!isBlocked)
					break;
			}
			if (!isBlocked) {
				HiddenObject anObject;
				anObject.mCol = aLeft;
				anObject.mRow = aTop;
				anObject.mWidth = aShape.mWidth;
				anObject.mHeight = aShape.mHeight;
				anObject.mShape = aCandidates.back();
				anObject.mPattern = aShape.mPattern;
				anObject.mSeed = (int)Rand() % 1000;
				anObjects.push_back(anObject);
				for (int aRow = aTop; aRow < aTop + aShape.mHeight; aRow++) {
					for (int aCol = aLeft; aCol < aLeft + aShape.mWidth; aCol++) {
						if (!hasPattern || aShape.mPattern[(aRow - aTop) * aShape.mWidth + (aCol - aLeft)] == 'X')
							aGrid[(size_t)(aCol + aRow * COLS)] = (int)anObjects.size() - 1;
					}
				}
			}
			aCandidates.pop_back();
		}
		aLayoutCount++;
	}

	int aBestLayout = -1;
	float aBestScore = 0.0f;
	for (int aLayout = 0; aLayout < aLayoutCount; aLayout++) {
		const std::vector<HiddenObject>& anObjects = aLayouts[aLayout];
		float aScore = 0.0f;
		for (size_t i = 0; i < anObjects.size(); i++) {
			float aLeast = 0.0f;
			for (size_t j = 0; j < anObjects.size(); j++) {
				if (i == j)
					continue;
				const float aDeltaX = ((float)anObjects[i].mWidth * 0.5f + (float)anObjects[i].mCol) - ((float)anObjects[j].mWidth * 0.5f + (float)anObjects[j].mCol);
				const float aDeltaY = ((float)anObjects[i].mHeight * 0.5f + (float)anObjects[i].mRow) - ((float)anObjects[j].mHeight * 0.5f + (float)anObjects[j].mRow);
				const float aDistance = aDeltaX * aDeltaX + aDeltaY * aDeltaY;
				if (aLeast == 0.0f || aDistance < aLeast)
					aLeast = aDistance;
			}
			aScore += aLeast;
		}
		for (int aCol = 0; aCol < COLS; aCol++) {
			if (aGrids[aLayout][(size_t)(aCol + (ROWS - 1) * COLS)] != -1)
				aScore -= 2.0f;
		}
		if (aBestLayout == -1 || aBestScore < aScore) {
			aBestScore = aScore;
			aBestLayout = aLayout;
		}
	}
	if (aBestLayout < 0)
		return;
	for (int aRow = 0; aRow < ROWS; aRow++) {
		for (int aCol = 0; aCol < COLS; aCol++) {
			mTiles[aRow][aCol].mRefill = 0;
			mTiles[aRow][aCol].mObject = aGrids[aBestLayout][(size_t)(aCol + aRow * COLS)];
		}
	}
	mObjects = aLayouts[aBestLayout];
}

// 0x6b7a50 (bejeweled3): a piece id that is not in the map yet is put there as true and the map is run (0x6b75e0)
void HiddenObjectGoal::QueueTile(int thePieceId)
{
	if (mQueued.find(thePieceId) == mQueued.end()) {
		mQueued[thePieceId] = true;
		RunQueue();
	}
}

// 0x6b75e0 (bejeweled3): each entry that is true is looked up as a piece; one that is still on the board uncovers the tile it is on (0x6b7400); the entry becomes false
void HiddenObjectGoal::RunQueue()
{
	for (std::pair<const int, bool>& anEntry : mQueued) {
		if (!anEntry.second)
			continue;
		const Piece* aPiece = mBoard->FindPieceById(anEntry.first);
		if (aPiece != nullptr)
			UncoverTile(aPiece->mCol, aPiece->mRow);
		anEntry.second = false;
	}
}

// 0x6b7400 (bejeweled3): the tile's countdown is the refill time. port: the sound that the exe plays (once for the updates 5 apart) and the effect it makes are not
void HiddenObjectGoal::UncoverTile(int theCol, int theRow)
{
	if (theCol < 0 || theCol >= COLS || theRow < 0 || theRow >= ROWS)
		return;
	mTiles[theRow][theCol].mRefill = mRefillTicks;
}

// slot 16 (0x40), 0x6b9a80 (bejeweled3): a cell in the grid that holds a piece queues the piece (0x6b7a50 with its id)
void HiddenObjectGoal::OnCellHit(int theCol, int theRow)
{
	if (theCol < 0 || theCol >= COLS || theRow < 0 || theRow >= ROWS)
		return;
	const Piece* aPiece = mBoard->mGrid[theRow][theCol];
	if (aPiece != nullptr)
		QueueTile(aPiece->mId);
}

// slot 17 (0x44), 0x6b7b80 (bejeweled3): for a cluster of the run: when the setting NeverAllowCascades is off, the cluster counts only when one of its pieces is a piece of a swap; each piece of a cluster
// that counts is queued (0x6b7a50 with its id), and the queue is run. port: the exe goes through the list of the clusters and runs the queue once at its end; the port is given one cluster at a time
void HiddenObjectGoal::OnRunFound(const std::vector<Piece*>& thePieces)
{
	bool isCounted = mNeverAllowCascades;
	if (!isCounted) {
		for (const Piece* aPiece : thePieces) {
			for (const SwapRecord& aSwap : mBoard->mSwaps) {
				if (aPiece == aSwap.mFirst || aPiece == aSwap.mSecond) {
					isCounted = true;
					break;
				}
			}
			if (isCounted)
				break;
		}
	}
	if (isCounted) {
		for (const Piece* aPiece : thePieces)
			QueueTile(aPiece->mId);
	}
	RunQueue();
}

// 0x6baff0 (bejeweled3): an object that is not found yet is found when each of its cells (the pattern's X, or all) is fully uncovered; the score and the progress then go up. port: the exe also waits while an
// effect of the goal's list is still young (the list at +0x1f5c) and makes an effect for the find
void HiddenObjectGoal::CheckObjects()
{
	for (HiddenObject& anObject : mObjects) {
		if (anObject.mFound)
			continue;
		bool isWhole = true;
		for (int aCol = anObject.mCol; aCol < anObject.mCol + anObject.mWidth && isWhole; aCol++) {
			for (int aRow = anObject.mRow; aRow < anObject.mRow + anObject.mHeight; aRow++) {
				const bool isOnShape = anObject.mPattern[0] == 0 || anObject.mPattern[(aRow - anObject.mRow) * anObject.mWidth + (aCol - anObject.mCol)] == 'X';
				if (isOnShape && mTiles[aRow][aCol].mReveal < 1.0f) {
					isWhole = false;
					break;
				}
			}
		}
		if (isWhole) {
			anObject.mFound = true;
			mBoard->mScorePoints++;
			mBoard->mLevelProgress++;
		}
	}
}

// slot 54 (0xd8), 0x6c24c0 (bejeweled3): a tile that is uncovered and below the never-count limit counts down by one; a tile with time left is uncovered by a step of its curve and one at 0 is
// covered again; the objects are looked at (0x6baff0); with the board settled, the queue of the pieces is emptied. port: the curves are a step of 0.01 an update (the exe's run 100 updates; the cover
// of a tile that is falling waits for the others there: see NOTES.md), the counter at +0x108 and the effect lists are not kept
void HiddenObjectGoal::Update()
{
	for (int aRow = 0; aRow < ROWS; aRow++) {
		for (int aCol = 0; aCol < COLS; aCol++) {
			Tile& aTile = mTiles[aRow][aCol];
			if (aTile.mRefill > 0 && aTile.mRefill < REFILL_NEVER)
				aTile.mRefill--;
			if (aTile.mRefill > 0)
				aTile.mReveal = aTile.mReveal + REVEAL_STEP > 1.0f ? 1.0f : aTile.mReveal + REVEAL_STEP;
			else
				aTile.mReveal = aTile.mReveal - REVEAL_STEP < 0.0f ? 0.0f : aTile.mReveal - REVEAL_STEP;
		}
	}
	CheckObjects();
	if (mBoard->IsSettled())
		mQueued.clear();
}

int HiddenObjectGoal::FoundCount() const
{
	int aFound = 0;
	for (const HiddenObject& anObject : mObjects)
		aFound += anObject.mFound ? 1 : 0;
	return aFound;
}

// port: the objects' tiles are drawn as coloured blocks (a colour for each shape), covered by sand that fades as the tile is uncovered; a found object has a white frame
void HiddenObjectGoal::DrawBehindPieces(Graphics* g)
{
	static const int SHAPE_COLORS[SHAPE_COUNT][3] = {{220, 60, 60}, {60, 200, 90}, {70, 110, 230}, {230, 200, 60}, {200, 90, 220}, {60, 210, 210}, {240, 140, 50}, {150, 230, 90}, {240, 110, 170}, {140, 140, 240}};
	for (int aRow = 0; aRow < ROWS; aRow++) {
		for (int aCol = 0; aCol < COLS; aCol++) {
			const Tile& aTile = mTiles[aRow][aCol];
			const int aX = mBoard->GetGridX() + aCol * Board::CELL;
			const int aY = mBoard->GetGridY() + aRow * Board::CELL;
			if (aTile.mObject >= 0 && aTile.mObject < (int)mObjects.size()) {
				const int* aColor = SHAPE_COLORS[mObjects[(size_t)aTile.mObject].mShape];
				g->SetColor(Color(aColor[0], aColor[1], aColor[2], 255));
				g->FillRect(Scaled(aX + 6), Scaled(aY + 6), Scaled(Board::CELL - 12), Scaled(Board::CELL - 12));
				if (mObjects[(size_t)aTile.mObject].mFound) {
					g->SetColor(Color(255, 255, 255, 255));
					g->FillRect(Scaled(aX + 6), Scaled(aY + 6), Scaled(Board::CELL - 12), Scaled(8));
				}
			}
			const int aSand = (int)((1.0f - aTile.mReveal) * 235.0f);
			if (aSand > 0) {
				g->SetColor(Color(196, 160, 100, aSand));
				g->FillRect(Scaled(aX + 2), Scaled(aY + 2), Scaled(Board::CELL - 4), Scaled(Board::CELL - 4));
			}
		}
	}
	g->SetColor(Color(255, 255, 255, 255));
}

// the keys: the objects (column.row.width.height, the object found marked with an f), the cover (a digit for each tile, 0 covered to 9 uncovered, row after row), the refill time and the objects found
std::string HiddenObjectGoal::DescribeState() const
{
	std::string anObjects;
	for (const HiddenObject& anObject : mObjects) {
		anObjects += (anObjects.empty() ? "" : ";") + std::to_string(anObject.mCol) + "." + std::to_string(anObject.mRow) + "." + std::to_string(anObject.mWidth) + "." +
			std::to_string(anObject.mHeight) + (anObject.mFound ? "f" : "");
	}
	std::string aCover;
	for (int aRow = 0; aRow < ROWS; aRow++) {
		for (int aCol = 0; aCol < COLS; aCol++)
			aCover += (char)('0' + (int)(mTiles[aRow][aCol].mReveal * 9.0f + 0.5f));
		aCover += aRow < ROWS - 1 ? " " : "";
	}
	return ",\"hiddenobjects\":\"" + anObjects + "\",\"hiddenfound\":" + std::to_string(FoundCount()) + ",\"cover\":\"" + aCover + "\",\"refill\":" + std::to_string(mRefillTicks) +
		",\"itemset\":" + std::to_string(mItemSet);
}

}  // namespace Sexy
