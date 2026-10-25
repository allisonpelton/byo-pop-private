// port(variant): board sizes and shapes (games/bejeweled/docs/CODE_MODS_DESIGN.md 4.3 and section 5), Board members.
// Not part of the original game. Stage 7: a board of the rules' board.cols/rows/mask, or one shape per level
// (board.levels), simulated in logical 51-px space with the original constants (BoardLayout.h). The translated
// functions dispatch here on a shape board (VariantFlags::mShape): FillBoard to FillBoardVariant, UpdateClearing's
// refill to RefillMasked, SetupLevel's tile loop to DrawVariantChecker; the constructor's tutorial loop gives way to
// FillBoardWithMove where the tutorial move cannot apply. Each is the original algorithm over the layout's cells, so
// on the original 8x8 it draws the same Rand() values in the same order (-forcevariant runs it there).
//
// Level shapes: SetupLevel (inside UpdateBlackHole's done-block, before its deletion loop) sets up the next level's
// layout in mPendingLayout and draws its checkerboard; the deletion loop runs with the old bounds, then
// ApplyPendingLayout switches. A loaded save brings its level's shape (BoardSave.cpp, SetLoadedLayout), and the
// background is rebuilt for its level and shape (RebuildLoadedBackground; the original keeps level 0's, 5.3).

#include "Board.h"
#include "WinDMApp.h"
#include "GameConfig.h"
#include "Res.h"
#include "Graphics.h"
#include "MemoryImage.h"
#include "DDImage.h"

#include <SDL.h>
#include <algorithm>
#include <cmath>
#include <cstring>

using namespace Sexy;

// FillBoardVariant's backstop (3.6): a fill that has rerolled this often starts again, still drawing from the
// board's RNG, so a combination of rules the static fill minimum did not foresee cannot hang it
static const int MAX_FILL_REROLLS = 1000;

// FillBoard (0x4cb182) on a shape board (5.5): the old pieces parked as FillBoard parks them, then rows bottom-up
// and columns left to right over the cells only, each gem 65..304 px above the nearest cell below it (already
// filled) and its colour rolled until the board has no match
void Board::FillBoardVariant()
{
	mState = STATE_FALLING;
	mIdleTicks = 0;

	for (int i = 0; i < MAX_ROWS * MAX_COLS; i++)
	{
		Piece*& aCell = mBoard[i / MAX_COLS][i % MAX_COLS];
		if ((aCell != NULL) && (aCell != mSelectedPiece))
			mOrphanedPieces.push_back(aCell);
	}
	memset(mBoard, 0, sizeof(mBoard));

	for (;;)
	{
		int aRerolls = 0;
		bool isCapped = false;
		for (int aRow = mRows - 1; (aRow >= 0) && !isCapped; aRow--)
		{
			for (int aCol = 0; (aCol < mCols) && !isCapped; aCol++)
			{
				if (!mLayout.IsPlayable(aCol, aRow))
					continue;

				Piece* aPiece = new Piece();
				aPiece->mRow = aRow;
				aPiece->mCol = aCol;
				aPiece->mX = GetColX(aCol);
				int aBelowY = 0;
				for (int aBelow = aRow + 1; aBelow < mRows; aBelow++)
				{
					if (mLayout.IsPlayable(aCol, aBelow))
					{
						aBelowY = (int)mBoard[aBelow][aCol]->mY;
						break;
					}
				}
				aPiece->mY = aBelowY - Rand() % 240 - 65;
				mBoard[aRow][aCol] = aPiece;
				for (;;)
				{
					aPiece->mColor = Rand() % gGameConfig.mGemColors;
					if (!HasMatch())
						break;
					if (++aRerolls >= MAX_FILL_REROLLS)
					{
						isCapped = true;
						break;
					}
				}
			}
		}
		if (!isCapped)
			return;

		SDL_Log("board: the fill rerolled %d times; filling again", MAX_FILL_REROLLS);
		for (int i = 0; i < MAX_ROWS * MAX_COLS; i++)
		{
			delete mBoard[i / MAX_COLS][i % MAX_COLS];
			mBoard[i / MAX_COLS][i % MAX_COLS] = NULL;
		}
	}
}

// UpdateClearing's refill (0x4ca9ed) over each column's cells V, top to bottom (5.4): every gem above the column's
// lowest gap moves one cell down, through holes, and one new gem spawns over the column's top cell, 69 px above its
// highest gem; passes repeat until no column has a gap. On a full column V is 0..rows-1, the original pass.
void Board::RefillMasked()
{
	int aCells[MAX_ROWS];
	bool aChanged;
	do
	{
		aChanged = false;
		for (int aCol = 0; aCol < mCols; aCol++)
		{
			int aCount = mLayout.PlayableRows(aCol, aCells);
			if (aCount == 0)
				continue;

			int aMinY = GetRowY(aCells[0]);
			bool aGap = false;
			for (int k = aCount - 1; k >= 0; k--)
			{
				Piece* aPiece = mBoard[aCells[k]][aCol];
				if (aPiece == NULL)
				{
					if (aGap)
						mBoard[aCells[k + 1]][aCol] = NULL;
					aGap = true;
				}
				else
				{
					if (aPiece->mY < aMinY)
						aMinY = (int)aPiece->mY;
					if (aGap)
					{
						aPiece->mRow = aCells[k + 1];
						aPiece->mVelY = 1.0;
						mBoard[aCells[k + 1]][aCol] = aPiece;
					}
				}
			}

			if (aGap)
			{
				aChanged = true;
				Piece* aPiece = new Piece();
				aPiece->mVelY = 0.3;
				aPiece->mRow = aCells[0];
				aPiece->mCol = aCol;
				aPiece->mX = GetColX(aCol);
				aPiece->mY = aMinY - 69;
				aPiece->mColor = Rand() % gGameConfig.mGemColors;
				mBoard[aCells[0]][aCol] = aPiece;
			}
		}
	}
	while (aChanged);
}

// The constructor's fill (0x4cb5bc) on a board where the tutorial move (a horizontal run to the right in rows 4..6,
// columns 1..6) cannot apply (5.5): boards are filled until one has a move, at most 100 times. The tutorial
// arrows stay off (mTutorialCol -1); the instructions dialog still opens at tick 150 as in the original (2.5).
void Board::FillBoardWithMove()
{
	int aFills = 0;
	do
		FillBoard();
	while (!FindMove(NULL, 0, true, true) && (++aFills < 100));
	mTutorialCol = -1;
}

// A mod's void tile (images/voidtile), looked up once; NULL without one
static Image* ModVoidTile(WinDMApp* theApp)
{
	static bool isLookedUp = false;
	static Image* aVoidTile = NULL;
	if (!isLookedUp)
	{
		isLookedUp = true;
		aVoidTile = theApp->GetImage("images\\voidtile", true);
	}
	return aVoidTile;
}

// SetupLevel's tile loop (0x4cae29) on a shape board (5.6): the checkerboard of the level SetupLevel sets up, which
// becomes the pending layout. The window is first filled with the void tile (a mod's images/voidtile, or the level's
// texture darkened); then the board's cells and a ring of one cell around them get tile A or B by (r + c) % 2, as the
// original's 10x10 cells from (151, -35) do, and holes the void tile. On the original 8x8 the tiles cover the whole
// window and no void tile is drawn: the original loop's draws exactly (the textures are not all opaque, so a void tile
// under them would show through where the original shows the last level's background). A scaled board's tiles are
// stretched between rounded screen edges, so that neighbours leave no seams.
void Board::DrawVariantChecker(Graphics* g, Image* theTileA, Image* theTileB)
{
	mPendingLayout = mRules.LayoutForLevel(mLevel);
	const BoardLayout& aLayout = mPendingLayout;

	Image* aVoidTile = ModVoidTile(mApp);
	DDImage* aDarkTile = NULL;
	if (aVoidTile == NULL)
	{
		MemoryImage aTexture(mApp);
		aTexture.Create(CELL_SIZE, CELL_SIZE);
		Graphics aTextureG(&aTexture);
		aTextureG.DrawImage(mApp->mImages[IMAGE_CHECKERTEXTURES], 0, 0, Rect((mLevel % 5) * 102, 0, CELL_SIZE, CELL_SIZE));
		aDarkTile = mApp->CreateColorizedImage(&aTexture, Color(40, 32, 36));
		aVoidTile = aDarkTile;
	}
	Rect aVoidRect(0, 0, std::min((int) CELL_SIZE, aVoidTile->GetWidth()), std::min((int) CELL_SIZE, aVoidTile->GetHeight()));

	if (!aLayout.IsOriginal())
	{
		for (int aY = -35; aY < 425; aY += CELL_SIZE)
		{
			for (int aX = 151; aX < 611; aX += CELL_SIZE)
				g->DrawImage(aVoidTile, aX, aY, aVoidRect);
		}
	}

	bool isScaled = (aLayout.mScale != 1.0);
	for (int aRow = -1; aRow <= aLayout.mRows; aRow++)
	{
		for (int aCol = -1; aCol <= aLayout.mCols; aCol++)
		{
			bool isHole = aLayout.InBounds(aCol, aRow) && !aLayout.IsPlayable(aCol, aRow);
			Image* aTile = isHole ? aVoidTile : (((aRow + aCol) % 2 == 0) ? theTileA : theTileB);
			int aX = aCol * CELL_SIZE + aLayout.mOriginX - 1;
			int aY = aRow * CELL_SIZE + aLayout.mOriginY;
			if (!isScaled)
			{
				// As the original draws them: tile B whole (102 px, its right half empty)
				if (isHole)
					g->DrawImage(aTile, aX, aY, aVoidRect);
				else
					g->DrawImage(aTile, aX, aY);
				continue;
			}
			int aLeft = (int)floor(aLayout.ToScreenX(aX) + 0.5);
			int aRight = (int)floor(aLayout.ToScreenX(aX + CELL_SIZE) + 0.5);
			int aTop = (int)floor(aLayout.ToScreenY(aY) + 0.5);
			int aBottom = (int)floor(aLayout.ToScreenY(aY + CELL_SIZE) + 0.5);
			g->DrawImage(aTile, Rect(aLeft, aTop, aRight - aLeft, aBottom - aTop), isHole ? aVoidRect : Rect(0, 0, CELL_SIZE, CELL_SIZE));
		}
	}

	delete aDarkTile;
}

// UpdateBlackHole's done-block (0x4cb931), after its deletion loop: the level SetupLevel set up takes its shape
void Board::ApplyPendingLayout()
{
	mLayout = mPendingLayout;
	mCols = mLayout.mCols;
	mRows = mLayout.mRows;
	int aShapes = mRules.ShapeCount();
	mLevelShape = ((mLevel % aShapes) + aShapes) % aShapes;
}

// ReadVariantHeader, once the save has passed its checks: the shape of the save's level. LoadGame then replaces the
// cells inside the new bounds; the pieces outside them go here.
void Board::SetLoadedLayout(const BoardLayout& theLayout, int theLevelShape)
{
	for (int aRow = 0; aRow < MAX_ROWS; aRow++)
	{
		for (int aCol = 0; aCol < MAX_COLS; aCol++)
		{
			if ((aRow < theLayout.mRows) && (aCol < theLayout.mCols))
				continue;
			if (mBoard[aRow][aCol] == mSelectedPiece)
				mSelectedPiece = NULL;
			delete mBoard[aRow][aCol];
			mBoard[aRow][aCol] = NULL;
		}
	}
	mLayout = theLayout;
	mPendingLayout = theLayout;
	mCols = mLayout.mCols;
	mRows = mLayout.mRows;
	mLevelShape = theLevelShape;
}

// GameLoadedVariant on a shape board: the background of the loaded level and its shape, where the original keeps
// the level-0 background it was made with (5.3). SetupLevel draws it; the level parameters it would set are the
// save's and stay.
void Board::RebuildLoadedBackground()
{
	bool aBackgroundChanged = mBackgroundChanged;
	double aTimeDrain = mTimeDrain;
	double aPointMultiplier = mPointMultiplier;
	int aLevelPoints = mLevelPoints;
	double aTimeDrainRate = mTimeDrainRate;
	double aTimeDrainAccel = mTimeDrainAccel;
	SetupLevel();
	mBackgroundChanged = aBackgroundChanged;
	mTimeDrain = aTimeDrain;
	mPointMultiplier = aPointMultiplier;
	mLevelPoints = aLevelPoints;
	mTimeDrainRate = aTimeDrainRate;
	mTimeDrainAccel = aTimeDrainAccel;

	Graphics g(mBackgroundImage);
	g.DrawImage(mNextBackgroundImage, 0, 0);
}
