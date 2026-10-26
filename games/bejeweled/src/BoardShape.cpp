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
//
// Gravity (stage 7c, 4.16): the falls (VariantFlags::mFall: UpdateFalling's loop, the refill, the fill, the
// fall-out and the game-over fall) run here in the direction GravityUp() gives, mirrored about the board's middle
// row under up; down is the original algorithm line for line, and -forcevariant runs it at default rules.
// "alternate" (Bejeweled 2's Twilight) starts up and flips at each settle that ends a move (OnGravitySettle).
//
// Dirt (stage 7b, 4.14): a dirt cell holds no gem and is a floor. UpdateClearing's refill becomes RefillDiagonal
// (column segments between dirt cells, then diagonal slides into the cells under dirt), a scored run takes a layer
// off each dirt cell next to it (ClearDirtNextToRun), and a cell whose last layer goes is a gem cell from then on.

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
// filled) and its colour rolled until the board has no match. Under up (4.16) the mirror: rows top-down, each gem
// 65..304 px below the nearest cell above it, or below the mirror line M = GetRowY(0) + GetRowY(rows - 1) (the
// mirror of y 0), so for one RNG state the fill is the vertical mirror of the downward one, draw for draw
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

	bool isUp = GravityUp();
	int aMirrorY = GetRowY(0) + GetRowY(mRows - 1);
	for (;;)
	{
		int aRerolls = 0;
		bool isCapped = false;
		for (int i = 0; (i < mRows) && !isCapped; i++)
		{
			int aRow = isUp ? i : mRows - 1 - i;
			for (int aCol = 0; (aCol < mCols) && !isCapped; aCol++)
			{
				if (!mLayout.IsPlayable(aCol, aRow))
					continue;

				Piece* aPiece = new Piece();
				aPiece->mRow = aRow;
				aPiece->mCol = aCol;
				aPiece->mX = GetColX(aCol);
				if (isUp)
				{
					int anAboveY = aMirrorY;
					for (int anAbove = aRow - 1; anAbove >= 0; anAbove--)
					{
						if (mLayout.IsPlayable(aCol, anAbove))
						{
							anAboveY = (int)mBoard[anAbove][aCol]->mY;
							break;
						}
					}
					aPiece->mY = anAboveY + Rand() % 240 + 65;
				}
				else
				{
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
				}
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
// highest gem; passes repeat until no column has a gap. On a full column V is 0..rows-1, the original pass. Under
// up (4.16) V runs bottom to top, gems move one cell up and the spawn goes 69 px below the column's lowest gem: the
// same draws as the downward refill of the vertically mirrored grid.
void Board::RefillMasked()
{
	bool isUp = GravityUp();
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
			if (isUp)
				std::reverse(aCells, aCells + aCount);

			// The entry side's edge: the highest gem (down), the lowest (up)
			int anEdgeY = GetRowY(aCells[0]);
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
					if (isUp ? (aPiece->mY > anEdgeY) : (aPiece->mY < anEdgeY))
						anEdgeY = (int)aPiece->mY;
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
				aPiece->mY = isUp ? anEdgeY + 69 : anEdgeY - 69;
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
//
// look.backdrop "twilight" (4.16, VariantFlags::mChecker): Bejeweled 2's Twilight backdrop, each pixel row mirrored
// left to right and each colour channel halved. BJ1 has no backdrop picture, so the checker layer takes it: every
// tile is drawn mirrored, colorized with 128 (half), at the cell mirrored about the board's middle (x 406 on 8x8,
// where the layer's x 151..661 maps onto itself), and the void tiles about the window's x 406. The frame and HUD
// (IMAGE_ALPHABACK) follow unmirrored.
void Board::DrawVariantChecker(Graphics* g, Image* theTileA, Image* theTileB)
{
	bool isTwilight = (mRules.mLook.mBackdrop == GameRules::BACKDROP_TWILIGHT);
	if (isTwilight)
	{
		g->SetColorizeImages(true);
		g->SetColor(Color(128, 128, 128));
	}
	// A tile at x theX: as drawn, or under twilight mirrored about theMirrorX / 2
	auto aDraw = [&](Image* theTile, int theX, int theY, const Rect& theSrc, int theMirrorX)
	{
		if (isTwilight)
			g->DrawImageMirror(theTile, theMirrorX - theX - theSrc.mWidth, theY, theSrc);
		else
			g->DrawImage(theTile, theX, theY, theSrc);
	};

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
				aDraw(aVoidTile, aX, aY, aVoidRect, 2 * 406);
		}
	}

	bool isScaled = (aLayout.mScale != 1.0);
	// Twice the board's middle x: the left edge of column -1's tile plus the right edge of column cols' tile
	int aMirrorX = 2 * (aLayout.mOriginX - 1) + aLayout.mCols * CELL_SIZE;
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
				// As the original draws them: tile B whole (102 px, its right half empty); mirrored, its drawn half
				if (isHole)
					aDraw(aTile, aX, aY, aVoidRect, aMirrorX);
				else if (isTwilight)
					aDraw(aTile, aX, aY, Rect(0, 0, CELL_SIZE, CELL_SIZE), aMirrorX);
				else
					g->DrawImage(aTile, aX, aY);
				continue;
			}
			int aMirroredX = isTwilight ? aMirrorX - aX - CELL_SIZE : aX;
			int aLeft = (int)floor(aLayout.ToScreenX(aMirroredX) + 0.5);
			int aRight = (int)floor(aLayout.ToScreenX(aMirroredX + CELL_SIZE) + 0.5);
			int aTop = (int)floor(aLayout.ToScreenY(aY) + 0.5);
			int aBottom = (int)floor(aLayout.ToScreenY(aY + CELL_SIZE) + 0.5);
			Rect aDest(aLeft, aTop, aRight - aLeft, aBottom - aTop);
			Rect aSrc = isHole ? aVoidRect : Rect(0, 0, CELL_SIZE, CELL_SIZE);
			if (isTwilight)
				g->DrawImageMirror(aTile, aDest, aSrc);
			else
				g->DrawImage(aTile, aDest, aSrc);
		}
	}

	if (isTwilight)
		g->SetColorizeImages(false);
	delete aDarkTile;
}

// UpdateBlackHole's done-block (0x4cb931), after its deletion loop: the level SetupLevel set up takes its shape
void Board::ApplyPendingLayout()
{
	mLayout = mPendingLayout; // dirt cells come back with all their layers (4.14)
	mDirtDust.clear();
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
	mDirtDust.clear();
	mCols = mLayout.mCols;
	mRows = mLayout.mRows;
	mLevelShape = theLevelShape;
}

// GameLoadedVariant on a shape board: the background of the loaded level and its shape, where the original keeps
// the level-0 background it was made with (5.3). SetupLevel draws it; the level parameters it would set are the
// save's and stay, and so do the gold and its time (stage 12: the GOLD and GTIM chunks, ReadVariantChunks).
void Board::RebuildLoadedBackground()
{
	bool aBackgroundChanged = mBackgroundChanged;
	double aTimeDrain = mTimeDrain;
	double aPointMultiplier = mPointMultiplier;
	int aLevelPoints = mLevelPoints;
	double aTimeDrainRate = mTimeDrainRate;
	double aTimeDrainAccel = mTimeDrainAccel;
	uint8_t aGold[MAX_ROWS][MAX_COLS];
	memcpy(aGold, mGold, sizeof(mGold));
	int aGoldTicksLeft = mGoldTicksLeft;
	int aGoldTickLimit = mGoldTickLimit;
	SetupLevel();
	mBackgroundChanged = aBackgroundChanged;
	mTimeDrain = aTimeDrain;
	mPointMultiplier = aPointMultiplier;
	mLevelPoints = aLevelPoints;
	mTimeDrainRate = aTimeDrainRate;
	mTimeDrainAccel = aTimeDrainAccel;
	memcpy(mGold, aGold, sizeof(mGold));
	mGoldTicksLeft = aGoldTicksLeft;
	mGoldTickLimit = aGoldTickLimit;

	Graphics g(mBackgroundImage);
	g.DrawImage(mNextBackgroundImage, 0, 0);
}

// ---------------------------------------------------------------------------------------------------
// Gravity (stage 7c, games/bejeweled/docs/CODE_MODS_DESIGN.md 4.16)

// "up", or "alternate" after an even number of flips (Twilight starts up). False on every board whose rules say
// "down", -forcevariant included, so the flip, the guard and the upward branches stay off there.
bool Board::GravityUp() const
{
	if (!mVariant.mGravity)
		return false;
	int aDirection = mRules.mBoard.mGravity;
	return (aDirection == GameRules::GRAVITY_UP) || ((aDirection == GameRules::GRAVITY_ALTERNATE) && ((mGravityFlips & 1) == 0));
}

// UpdateFalling's loop (0x4cd687) with board.gravity: the fall acceleration fallAccel / 1000.0 (180 is the original's
// 0.18, the same double), and under up every moving gem rises (the speeds stay positive) and lands at or above its
// rest y. Down is the original loop line for line.
void Board::FallPiecesVariant(int* theLandedCount, int* theLandedXSum, bool* isSettled)
{
	bool isUp = GravityUp();
	double anAccel = mRules.mBoard.mFallAccel / 1000.0;
	for (int aRow = 0; aRow < mRows; aRow++)
	{
		double aRestY = GetRowY(aRow);
		for (int aCol = 0; aCol < mCols; aCol++)
		{
			Piece* aPiece = mBoard[aRow][aCol];
			if (aPiece == NULL) continue;
			// A gem that slid diagonally (RefillDiagonal, 4.14) also moves over, at its fall speed, once it is no
			// further from its row than from its column: it falls straight, then glides in at 45 degrees, under the dirt
			// and not across it. Every other gem is over its column already, so this is the original loop for them
			double aRestX = GetColX(aCol);
			if ((aPiece->mY == aRestY) && (aPiece->mX == aRestX))
				aPiece->mVelY = 0.0;
			else
			{
				*isSettled = false;
				if ((aPiece->mX != aRestX) && (std::abs(aRestY - aPiece->mY) <= std::abs(aRestX - aPiece->mX)))
				{
					double aStep = std::max(aPiece->mVelY, 1.0);
					aPiece->mX = (aPiece->mX < aRestX) ? std::min(aPiece->mX + aStep, aRestX) : std::max(aPiece->mX - aStep, aRestX);
				}
				if (aPiece->mY != aRestY)
				{
					if (isUp)
						aPiece->mY -= aPiece->mVelY;
					else
						aPiece->mY += aPiece->mVelY;
					if (isUp ? (aPiece->mY <= aRestY) : (aPiece->mY >= aRestY))
					{
						(*theLandedCount)++;
						*theLandedXSum = (int)(aPiece->mX + 24.0 + *theLandedXSum);
						aPiece->mY = aRestY;
					}
				}
				// A gem that just landed keeps this velocity for one tick
				aPiece->mVelY += anAccel;
			}
		}
	}
}

// The end of UpdateFalling's no-match branch: a settle. As in Bejeweled 2's UpdateFalling, "alternate" flips when the
// settle reaches STATE_PLAYING, so a move's cascades share one direction, every fill (opening, level, fall-out) flips
// once at its settle, and a settle without a move left does not flip; a kept invalid move's settle does not either
// (mGravityNoFlip, KeepInvalidMove). A swap that goes back reaches PLAYING from UpdateSwap, with no settle.
void Board::OnGravitySettle()
{
	if ((mRules.mBoard.mGravity == GameRules::GRAVITY_ALTERNATE) && (mState == STATE_PLAYING) && !mGravityNoFlip)
		mGravityFlips++;
	mGravityNoFlip = false;
}

// UpdateFallOut (0x4cb3bf) with board.gravity: one Rand() % 15 per column per tick as there; under up the scan starts
// at the top, so the column's highest resting gem is released, gems rise, and a gem is gone once it passes the
// mirror of the screen bottom (M - (mScreenBottom + 32), -123 on 8x8). Then the next board's fill, as there.
void Board::UpdateFallOutVariant()
{
	bool isUp = GravityUp();
	int aGoneY = GetRowY(0) + GetRowY(mRows - 1) - (mLayout.mScreenBottom + 32);
	bool allGone = true;
	MarkDirty();

	for (int aCol = 0; aCol < mCols; aCol++)
	{
		bool aRelease = Rand() % 15 == 0;
		for (int i = 0; i < mRows; i++)
		{
			int aRow = isUp ? i : mRows - 1 - i;
			Piece* aPiece = mBoard[aRow][aCol];
			if (aPiece == NULL)
				continue;

			if (aPiece->mVelY == 0.0)
			{
				if (aRelease)
				{
					aPiece->mVelY = 1.0;
					aRelease = false;
				}
			}
			else
			{
				if (isUp)
					aPiece->mY -= aPiece->mVelY;
				else
					aPiece->mY += aPiece->mVelY;
				aPiece->mVelY += 0.18;
			}

			if (isUp ? (aPiece->mY > aGoneY) : (aPiece->mY < mLayout.mScreenBottom + 32))
				allGone = false;
		}
	}

	if (allGone)
	{
		mTimeDrainDelay = gGameConfig.mLevelDrainDelay;
		FillBoard();
	}
}

// UpdateGameOver's fall (0x4cab82) with board.gravity: the shake and the explosion velocities stay in UpdateGameOver;
// under up each gem moves by -mVelY, so its kick points down and it falls upwards, and it is gone once it passes the
// mirror of the screen bottom (M - (mScreenBottom + 49), -140 on 8x8). True when every gem is gone.
bool Board::GameOverFallVariant()
{
	bool isUp = GravityUp();
	int aGoneY = GetRowY(0) + GetRowY(mRows - 1) - (mLayout.mScreenBottom + GEM_SIZE);
	bool allGone = true;
	for (int aRow = 0; aRow < mRows; aRow++)
	{
		for (int aCol = 0; aCol < mCols; aCol++)
		{
			Piece* aPiece = mBoard[aRow][aCol];
			if (aPiece == NULL)
				continue;

			aPiece->mX += aPiece->mVelX;
			if (isUp)
				aPiece->mY -= aPiece->mVelY;
			else
				aPiece->mY += aPiece->mVelY;
			aPiece->mVelY += 0.15;
			if (isUp ? (aPiece->mY > aGoneY) : (aPiece->mY < mLayout.mScreenBottom + GEM_SIZE))
				allGone = false;
		}
	}
	return allGone;
}

// ---------------------------------------------------------------------------------------------------
// Dirt (stage 7b, games/bejeweled/docs/CODE_MODS_DESIGN.md 4.14)

// UpdateClearing's refill on a board with dirt: passes over the column segments (RefillSegmentsPass) until nothing
// moves, then one gem slides diagonally into a cell no vertical fall reaches (SlideDiagonal), which opens a cell on
// its own column, and the passes go on; until no gem can slide. Without dirt every column is one segment that spawns,
// so the passes are RefillMasked's, draw for draw, and no cell is left for a slide (-forcevariant runs it so).
void Board::RefillDiagonal()
{
	do
	{
		while (RefillSegmentsPass())
			;
	}
	while (SlideDiagonal());
}

// One of RefillMasked's passes over every column, split at its dirt cells into segments, each from the entry side (the
// top; the bottom under up). In a segment every gem on the entry side of its farthest gap moves one cell on. The
// entry segment, with no dirt between it and the column's entry edge, then gets a new gem over its first cell, 69 px
// beyond its edge gem, as RefillMasked's column does; a segment beyond dirt gets none, so its gaps gather under the
// dirt for SlideDiagonal. True if a gem moved or spawned.
bool Board::RefillSegmentsPass()
{
	bool isUp = GravityUp();
	bool aChanged = false;
	for (int aCol = 0; aCol < mCols; aCol++)
	{
		int aCells[BoardLayout::MAX_ROWS];
		int aCount = 0;
		bool isEntry = true;
		for (int i = 0; i <= mRows; i++)
		{
			int aRow = isUp ? mRows - 1 - i : i;
			bool isEnd = (i == mRows) || mLayout.IsDirt(aCol, aRow);
			if (!isEnd)
			{
				if (mLayout.IsPlayable(aCol, aRow))
					aCells[aCount++] = aRow;
				continue;
			}
			if (aCount > 0)
			{
				// The entry side's edge: the highest gem (down), the lowest (up)
				int anEdgeY = GetRowY(aCells[0]);
				bool aGap = false;
				bool aMoved = false;
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
						if (isUp ? (aPiece->mY > anEdgeY) : (aPiece->mY < anEdgeY))
							anEdgeY = (int)aPiece->mY;
						if (aGap)
						{
							aPiece->mRow = aCells[k + 1];
							aPiece->mVelY = 1.0;
							mBoard[aCells[k + 1]][aCol] = aPiece;
							aMoved = true;
						}
					}
				}
				if (aGap && isEntry)
				{
					aChanged = true;
					Piece* aPiece = new Piece();
					aPiece->mVelY = 0.3;
					aPiece->mRow = aCells[0];
					aPiece->mCol = aCol;
					aPiece->mX = GetColX(aCol);
					aPiece->mY = isUp ? anEdgeY + 69 : anEdgeY - 69;
					aPiece->mColor = Rand() % gGameConfig.mGemColors;
					mBoard[aCells[0]][aCol] = aPiece;
				}
				else if (aGap)
				{
					// The first cell's gem moved on (or it was empty already)
					mBoard[aCells[0]][aCol] = NULL;
					aChanged = aChanged || aMoved;
				}
			}
			aCount = 0;
			isEntry = false;
		}
	}
	return aChanged;
}

// No vertical fall reaches (theCol, theRow): the nearest cell on its entry side that is not a hole is dirt
static bool IsUnderDirt(const BoardLayout& theLayout, int theCol, int theRow, bool isUp)
{
	int aStep = isUp ? 1 : -1;
	for (int aRow = theRow + aStep; (aRow >= 0) && (aRow < theLayout.mRows); aRow += aStep)
	{
		if (theLayout.IsDirt(theCol, aRow))
			return true;
		if (theLayout.IsPlayable(theCol, aRow))
			return false;
	}
	return false;
}

// RefillDiagonal's slide: rows from the far side back (down: bottom to top), columns left to right, the first empty gem
// cell under dirt (where the passes gathered a segment's gaps) with a gem in a neighbouring column one row nearer the
// entry side. With a gem on both sides mDiagonalLeftFirst picks, and it flips after every slide. The gem takes the
// cell (FallPiecesVariant moves it over and on); false when no cell can take one. The columns keep their order under
// up, so the slides mirror vertically, as the passes do.
bool Board::SlideDiagonal()
{
	bool isUp = GravityUp();
	for (int i = 0; i < mRows; i++)
	{
		int aRow = isUp ? i : mRows - 1 - i;
		int aFromRow = isUp ? aRow + 1 : aRow - 1;
		for (int aCol = 0; aCol < mCols; aCol++)
		{
			if (!mLayout.IsPlayable(aCol, aRow) || (mBoard[aRow][aCol] != NULL) || !IsUnderDirt(mLayout, aCol, aRow, isUp))
				continue;
			Piece* aLeft = mLayout.IsPlayable(aCol - 1, aFromRow) ? mBoard[aFromRow][aCol - 1] : NULL;
			Piece* aRight = mLayout.IsPlayable(aCol + 1, aFromRow) ? mBoard[aFromRow][aCol + 1] : NULL;
			if ((aLeft == NULL) && (aRight == NULL))
				continue;
			bool isLeft = (aLeft != NULL) && ((aRight == NULL) || mDiagonalLeftFirst);
			Piece* aPiece = isLeft ? aLeft : aRight;
			mBoard[aFromRow][isLeft ? aCol - 1 : aCol + 1] = NULL;
			mBoard[aRow][aCol] = aPiece;
			aPiece->mRow = aRow;
			aPiece->mCol = aCol;
			aPiece->mVelY = 1.0;
			mDiagonalLeftFirst = !mDiagonalLeftFirst;
			return true;
		}
	}
	return false;
}

// ProcessMatches, after each scored run (theColor's gems in the rectangle, as it marks them): every dirt cell next to
// one of the run's gems (up, down, left, right) loses one layer, once for the run, so a cell next to two runs of one
// resolution loses two. board.dirtPoints times the multiplier for each layer. A cell whose last layer goes is a gem
// cell that the refill fills, under a puff of dust (DrawDirtDust).
void Board::ClearDirtNextToRun(int theStartCol, int theStartRow, int theEndCol, int theEndRow, int theColor)
{
	std::vector<std::pair<int, int> > aCells;
	for (int aRow = theStartRow; aRow <= theEndRow; aRow++)
	{
		for (int aCol = theStartCol; aCol <= theEndCol; aCol++)
		{
			Piece* aPiece = mBoard[aRow][aCol];
			if ((aPiece != NULL) && (aPiece->mColor == theColor))
				aCells.push_back(std::make_pair(aCol, aRow));
		}
	}
	ClearDirtNextToCells(aCells);
}

// ClearDirtNextToRun for any group of cells (col, row): ProcessMatchesVariant's groups, a pattern once for its group
void Board::ClearDirtNextToCells(const std::vector<std::pair<int, int> >& theCells)
{
	static const int kNeighbours[4][2] = {{0, -1}, {-1, 0}, {1, 0}, {0, 1}};
	bool aNext[BoardLayout::MAX_ROWS][BoardLayout::MAX_COLS] = {};
	bool isAny = false;
	for (const std::pair<int, int>& aCell : theCells)
	{
		for (int i = 0; i < 4; i++)
		{
			int aDirtCol = aCell.first + kNeighbours[i][0];
			int aDirtRow = aCell.second + kNeighbours[i][1];
			if (mLayout.IsDirt(aDirtCol, aDirtRow))
			{
				aNext[aDirtRow][aDirtCol] = true;
				isAny = true;
			}
		}
	}
	if (!isAny)
		return;
	ClearDirtLayers(aNext);
}

// Stage 11: the dirt cells of theCells (a blast's reach) lose a layer each, as ClearDirtNextToCells' neighbours
void Board::ClearDirtCells(const std::vector<std::pair<int, int> >& theCells)
{
	bool aCells[BoardLayout::MAX_ROWS][BoardLayout::MAX_COLS] = {};
	bool isAny = false;
	for (const std::pair<int, int>& aCell : theCells)
	{
		if (mLayout.IsDirt(aCell.first, aCell.second))
		{
			aCells[aCell.second][aCell.first] = true;
			isAny = true;
		}
	}
	if (isAny)
		ClearDirtLayers(aCells);
}

// One layer off each marked dirt cell, row-major: its points (board.dirtPoints), and its dust when it was the last
void Board::ClearDirtLayers(const bool (*theCells)[BoardLayout::MAX_COLS])
{
	for (int aRow = 0; aRow < mRows; aRow++)
	{
		for (int aCol = 0; aCol < mCols; aCol++)
		{
			if (!theCells[aRow][aCol])
				continue;
			mLayout.ClearDirtLayer(aCol, aRow);
			int aPoints = (int)(mRules.mBoard.mDirtPoints * mPointMultiplier);
			if (aPoints > 0)
			{
				mScore += aPoints;
				if (mStats != NULL) mStats->OnPoints(aPoints);
				ScorePopup aPopup = {GetColX(aCol) + 24, GetRowY(aRow) + 16, 110, -1, aPoints};
				mScorePopups.push_back(aPopup);
			}
			if (!mLayout.IsDirt(aCol, aRow))
			{
				DirtDust aDust = {aCol, aRow, mUpdateCnt};
				mDirtDust.push_back(aDust);
			}
		}
	}
	MarkDirty();
}

// A save's DIRT chunk or a snapshot's row (row-major, mCols x mRows): the layers left on the dirt cells of the level's
// mask, 0 for one cleared; any other cell must say 0. Applied to mLayout only when every cell fits
bool Board::ApplyDirtLayers(const std::vector<uint8_t>& theLayers, std::string* theWhy)
{
	BoardLayout aMask = mRules.LayoutForLevel(mLevelShape);
	if ((aMask.mCols != mCols) || (aMask.mRows != mRows) || (theLayers.size() != (size_t)(mCols * mRows)))
	{
		*theWhy = "dirt layers for " + std::to_string(theLayers.size()) + " cells on a " + std::to_string(mCols) + "x" +
			std::to_string(mRows) + " board";
		return false;
	}
	for (int aRow = 0; aRow < mRows; aRow++)
	{
		for (int aCol = 0; aCol < mCols; aCol++)
		{
			int aLayers = theLayers[aRow * mCols + aCol];
			int aMost = aMask.IsDirt(aCol, aRow) ? aMask.mDirt[aRow][aCol] : 0;
			if (aLayers > aMost)
			{
				*theWhy = std::to_string(aLayers) + " dirt layers at cell (" + std::to_string(aCol) + "," + std::to_string(aRow) +
					"), where the mask has " + std::to_string(aMost);
				return false;
			}
		}
	}
	for (int aRow = 0; aRow < mRows; aRow++)
	{
		for (int aCol = 0; aCol < mCols; aCol++)
		{
			if (aMask.IsDirt(aCol, aRow))
				mLayout.SetDirtLayers(aCol, aRow, theLayers[aRow * mCols + aCol]);
		}
	}
	return true;
}
