#include "TileMaster.h"
#include "PixelusApp.h"
#include "Grid.h"
#include "Graphics.h"
#include "Image.h"
#include <math.h>

using namespace Sexy;

// how far the middle of his sprite stands from the grid's edge
static const int STAND_VERTICAL = 36;
static const int STAND_HORIZONTAL = 34;

namespace
{
	// the rectangle he walks along: left edge (side 1) going up, top (side 2) going right, right edge (side 3)
	// going down, bottom (side 0) going left
	struct Path
	{
		int		mLeft, mRight, mTop, mBottom;	// the rectangle's lines
		int		mLeftLength;					// bottom to top
		int		mTopLength;						// left to right
		double	mTotal;
	};

	Path MakePath(int theGridX, int theGridY, int theCols, int theRows)
	{
		Path aPath;
		aPath.mLeft = theGridX - STAND_HORIZONTAL;
		aPath.mRight = theGridX + theCols * Grid::CELL_SIZE + STAND_HORIZONTAL;
		aPath.mTop = theGridY - STAND_VERTICAL;
		aPath.mBottom = theGridY + theRows * Grid::CELL_SIZE + STAND_VERTICAL;
		aPath.mLeftLength = aPath.mBottom - aPath.mTop;
		aPath.mTopLength = aPath.mRight - aPath.mLeft;
		aPath.mTotal = 2.0 * (aPath.mLeftLength + aPath.mTopLength);
		return aPath;
	}
}

TileMaster::TileMaster(PixelusApp* theApp, int theGridX, int theGridY, int theCols, int theRows)
{
	mApp = theApp;
	mGridX = theGridX;
	mGridY = theGridY;
	mCols = theCols;
	mRows = theRows;
	mSide = Grid::SIDE_UP;
	mCursor = 0;
	mThrowTimer = 0;
	mPull = false;
	mWalkPhase = 0;
	mU = mTargetU = ToPath(mSide, mCursor);
}

// the distance along the path of the place he stands at for a side and a position along it
double TileMaster::ToPath(int theSide, int theCursor) const
{
	Path aPath = MakePath(mGridX, mGridY, mCols, mRows);
	int aCol = mGridX + theCursor * Grid::CELL_SIZE + Grid::CELL_SIZE / 2;
	int aRow = mGridY + theCursor * Grid::CELL_SIZE + Grid::CELL_SIZE / 2;
	switch (theSide)
	{
	case Grid::SIDE_RIGHT:	// he stands left of the grid and throws right
		return aPath.mBottom - aRow;
	case Grid::SIDE_DOWN:	// above the grid
		return aPath.mLeftLength + (aCol - aPath.mLeft);
	case Grid::SIDE_LEFT:	// right of the grid
		return aPath.mLeftLength + aPath.mTopLength + (aRow - aPath.mTop);
	default:				// below the grid
		return 2.0 * aPath.mLeftLength + aPath.mTopLength + (aPath.mRight - aCol);
	}
}

// the middle of his sprite and the view (a row of the sprite sheet) at a place on the path
void TileMaster::FromPath(double theU, double* theX, double* theY, int* theFacing) const
{
	Path aPath = MakePath(mGridX, mGridY, mCols, mRows);
	while (theU < 0)
		theU += aPath.mTotal;
	while (theU >= aPath.mTotal)
		theU -= aPath.mTotal;

	if (theU < aPath.mLeftLength)
	{
		*theX = aPath.mLeft;
		*theY = aPath.mBottom - theU;
		*theFacing = 1;
	}
	else if (theU < aPath.mLeftLength + aPath.mTopLength)
	{
		*theX = aPath.mLeft + (theU - aPath.mLeftLength);
		*theY = aPath.mTop;
		*theFacing = 0;
	}
	else if (theU < 2 * aPath.mLeftLength + aPath.mTopLength)
	{
		*theX = aPath.mRight;
		*theY = aPath.mTop + (theU - aPath.mLeftLength - aPath.mTopLength);
		*theFacing = 2;
	}
	else
	{
		*theX = aPath.mRight - (theU - 2 * aPath.mLeftLength - aPath.mTopLength);
		*theY = aPath.mBottom;
		*theFacing = 3;
	}
}

// 0x42aec0: aim at a place; the walk goes the shorter way round. theJump puts him there at once (a new puzzle)
void TileMaster::SetTarget(int theSide, int theCursor, bool theJump)
{
	mSide = theSide;
	mCursor = theCursor;
	mTargetU = ToPath(theSide, theCursor);
	if (theJump)
		mU = mTargetU;
}

// 0x42b010: the throwing pose, which 0x42af80 and the walk end
void TileMaster::Throw()
{
	mThrowTimer = 12;
	mPull = false;
}

void TileMaster::Pull()
{
	mThrowTimer = 12;
	mPull = true;
}

// 0x42bf50
void TileMaster::Update()
{
	if (mThrowTimer > 0)
		mThrowTimer--;

	if (mU == mTargetU)
		return;

	Path aPath = MakePath(mGridX, mGridY, mCols, mRows);
	double aDelta = mTargetU - mU;
	if (aDelta > aPath.mTotal / 2)
		aDelta -= aPath.mTotal;
	else if (aDelta < -aPath.mTotal / 2)
		aDelta += aPath.mTotal;

	mWalkPhase++;
	if (fabs(aDelta) <= WALK_SPEED)
		mU = mTargetU;
	else
		mU += aDelta > 0 ? WALK_SPEED : -WALK_SPEED;
	if (mU < 0)
		mU += aPath.mTotal;
	else if (mU >= aPath.mTotal)
		mU -= aPath.mTotal;
}

void TileMaster::Draw(Graphics* g, int theAlpha)
{
	double aX, aY;
	int aFacing;
	FromPath(mU, &aX, &aY, &aFacing);

	// the sprite sheets list the four views as standing: front (above the grid), right, left and back (below)
	int aRow = aFacing;
	int aBob = 0;
	if (mU != mTargetU)
		aBob = (mWalkPhase / 3) % 2;

	Image* aShadow = mApp->mImages[IMAGE_TILEMASTER_SHADOW];
	int aLeft = (int) aX - FRAME_WIDTH / 2;
	int aTop = (int) aY - FRAME_HEIGHT / 2 - aBob;

	g->SetColorizeImages(theAlpha < 255);
	g->SetColor(Color(255, 255, 255, theAlpha));
	g->DrawImage(aShadow, (int) aX - aShadow->GetWidth() / 2, (int) aY + 10);

	if (mThrowTimer > 0)
	{
		Image* aThrow = mApp->mImages[IMAGE_TILEMASTERTHROW];
		// the pull is the pose with the hand drawn back, the throw the one that has let go
		int aFrame = mPull ? 0 : (mThrowTimer > 6 ? 0 : 1);
		if (mPull)
			aFrame = mThrowTimer > 6 ? 1 : 0;
		g->DrawImage(aThrow, aLeft, aTop, Rect(aFrame * FRAME_WIDTH, aRow * FRAME_HEIGHT, FRAME_WIDTH, FRAME_HEIGHT));
	}
	else
	{
		Image* aStand = mApp->mImages[IMAGE_TILEMASTER];
		g->DrawImage(aStand, aLeft, aTop, Rect(0, aRow * FRAME_HEIGHT, FRAME_WIDTH, FRAME_HEIGHT));
	}
	g->SetColorizeImages(false);
}
