#include "TileMaster.h"
#include "PixelusApp.h"
#include "Grid.h"
#include "Graphics.h"
#include "Image.h"
#include "Compat2004.h"
#include "Res.h"
#include <math.h>

using namespace Sexy;

namespace
{
	// 0x437b90: the sheets of the set "TileMaster". A grid sheet has a row per side (the frames of a row are side by side), a
	// strip is one row; mode: 0 plays once and stays on the last frame, 1 plays once and goes back to the stand, 2 repeats,
	// 4 plays forward and back, then goes back to the stand (0x436dc0)
	struct ActionInfo
	{
		int		mImage;
		int		mMode;
		int		mRows;
		int		mFrames;			// in a row
		int		mDurations[8];		// updates each frame is shown
		int		mPivotY;
	};

	const ActionInfo sActions[TileMaster::ACTION_COUNT] =
	{
		{ IMAGE_TILEMASTER,					2, 4, 1, { 10 },							TileMaster::PIVOT_Y },
		{ IMAGE_TILEMASTERTHROW,			1, 4, 2, { 10, 10 },						TileMaster::PIVOT_Y },
		{ IMAGE_CHARACTER_SCRATCHING,		1, 1, 2, { 10, 10 },						TileMaster::PIVOT_Y },
		{ IMAGE_CHARACTER_JUGGLING,			1, 1, 5, { 10, 10, 10, 10, 10 },			TileMaster::PIVOT_Y },
		{ IMAGE_JUGGLING2,					1, 1, 4, { 10, 10, 10, 10 },				TileMaster::PIVOT_Y },
		{ IMAGE_CHARACTER_SHRUGGING,		4, 1, 8, { 10, 6, 6, 6, 6, 6, 6, 3 },		TileMaster::PIVOT_Y },
		{ IMAGE_CHARACTER_CELEBRATING,		0, 1, 3, { 10, 10, 10 },					TileMaster::PIVOT_Y_CELEBRATE }
	};

	// the row of the sheets for a side: below the grid (side 0) he faces up the screen
	const int sSideRows[4] = { 3, 1, 0, 2 };

	int ReadDuration(const ActionInfo& theAction, int theFrame)
	{
		if (theFrame >= theAction.mFrames)
			theFrame = theAction.mFrames - 1;
		return theAction.mDurations[theFrame];
	}
}

// 0x42bd40: the trail starts on the top side at the third cell and runs back to the first, and he stands where 0x423b90
// puts the top left corner a cell up until the first update
// 0x437b90: the sheet table of the set above
TileMaster::TileMaster(PixelusApp* theApp, int theGridX, int theGridY, int theCols, int theRows)
{
	mApp = theApp;
	mGridX = theGridX;
	mGridY = theGridY;
	mCols = theCols;
	mRows = theRows;
	mTrailSide = Grid::SIDE_DOWN;
	mTrailPos = 2;
	mTrailTargetSide = Grid::SIDE_DOWN;
	mTrailTargetPos = 0;
	mTrailMode = 2;
	mTrailPhase = 0.0f;
	mSide = Grid::SIDE_DOWN;
	mStandSide = Grid::SIDE_DOWN;
	mCursor = 0;
	mSettleDelay = 0;
	mStandX = mGridX;
	mStandY = mGridY - Grid::CELL_SIZE;
	mIdling = false;
	mIdleAction = 0;
	mIdleRepeats = 0;
	mIdleDelay = 500;
	mFrozen = false;
	mAction = ACTION_STAND;
	mRow = 0;
	mFrame = 0;
	mTick = 0;
	mBackwards = false;
}

// 0x423b90 with the middle of the cell asked for
void TileMaster::CellCenter(int theRow, int theCol, int* theX, int* theY) const
{
	*theX = mGridX + Grid::CELL_SIZE * theCol + Grid::CELL_SIZE / 2;
	*theY = mGridY + Grid::CELL_SIZE * theRow + Grid::CELL_SIZE / 2;
}

// 0x436f30
void TileMaster::SetAction(int theAction)
{
	if (mAction != theAction)
	{
		mAction = theAction;
		mFrame = 0;
		mTick = 0;
		mBackwards = false;
	}
}

// 0x436f10
void TileMaster::SetRow(int theRow)
{
	if (mRow != theRow)
	{
		mRow = theRow;
		mFrame = 0;
		mTick = 0;
		mBackwards = false;
	}
}

// 0x436dc0: a frame is shown for its number of updates, then the next one
bool TileMaster::AdvanceAnimation()
{
	const ActionInfo& anAction = sActions[mAction];
	mTick++;
	if (mTick < ReadDuration(anAction, mFrame))
		return false;

	mTick = 0;
	int aLast = anAction.mFrames - 1;
	switch (anAction.mMode)
	{
	case 0:
		if (mFrame < aLast)
		{
			mFrame++;
			return false;
		}
		return true;
	case 1:
		if (mFrame < aLast)
		{
			mFrame++;
			return false;
		}
		mFrame = 0;
		mAction = ACTION_STAND;
		return true;
	case 2:
		mFrame++;
		if (mFrame == anAction.mFrames)
		{
			mFrame = 0;
			return true;
		}
		return false;
	default:
		if (!mBackwards)
		{
			if (mFrame < aLast)
			{
				mFrame++;
				return false;
			}
			mFrame--;
			mBackwards = true;
			return true;
		}
		if (mFrame > 0)
		{
			mFrame--;
			return false;
		}
		mAction = ACTION_STAND;
		mFrame = 0;
		return true;
	}
}

// 0x42aec0 (the place the cursor would be at, which he does not use, left out): he goes to the place one update later
void TileMaster::SetTarget(int theSide, int theCursor, bool theJump)
{
	(void) theJump;
	mCursor = theCursor;
	mSide = theSide;
	mSettleDelay = 1;
}

// 0x42af80: the idle action is over, the next comes in 400 to 599 updates
void TileMaster::EndIdle()
{
	CancelIdle();
	mIdleDelay = CrtRand() % 200 + 400;
}

// the first half of 0x42af80, 0x42afc0, 0x42b030 and 0x42bb20: back to the stand
void TileMaster::CancelIdle()
{
	if (mIdling)
	{
		SetAction(ACTION_STAND);
		SetRow(0);
		mIdling = false;
	}
}

// 0x42b8b0: he goes to the cell beside the grid for the side and place, facing it
void TileMaster::Settle()
{
	if (mFrozen)
		return;

	mStandSide = mSide;
	int aOldX = mStandX;
	int aOldY = mStandY;
	switch (mSide)
	{
	case Grid::SIDE_UP:
		CellCenter(mRows + 2, mCursor, &mStandX, &mStandY);
		break;
	case Grid::SIDE_RIGHT:
		CellCenter(mCursor, -2, &mStandX, &mStandY);
		break;
	case Grid::SIDE_DOWN:
		CellCenter(-1, mCursor, &mStandX, &mStandY);
		break;
	case Grid::SIDE_LEFT:
		CellCenter(mCursor, mCols + 1, &mStandX, &mStandY);
		break;
	}
	SetTrailTarget(mSide, mCursor);
	if (aOldX != mStandX || aOldY != mStandY)
	{
		CancelIdle();
		mIdleDelay = CrtRand() % 200 + 400;
		SetRow(sSideRows[mSide]);
	}
}

// 0x42bb20: the throw (and the pull) animation, facing the side
void TileMaster::Throw()
{
	Settle();
	CancelIdle();
	mIdleDelay = CrtRand() % 200 + 400;
	SetRow(sSideRows[mSide]);
	SetAction(ACTION_THROW);
}

// 0x42b010
void TileMaster::Celebrate()
{
	SetRow(0);
	SetAction(ACTION_CELEBRATE);
}

// 0x42b030: the tile cannot be pulled; the shrug counts as an idle action that is repeated no more
void TileMaster::Shrug()
{
	CancelIdle();
	mIdleDelay = CrtRand() % 200 + 400;
	SetRow(0);
	SetAction(ACTION_SHRUG);
	mIdling = true;
	mIdleRepeats = 0;
}

// 0x42afc0: the puzzle is done
void TileMaster::Freeze()
{
	CancelIdle();
	mIdleDelay = CrtRand() % 200 + 400;
	mFrozen = true;
}

// 0x42b7a0: when the wait is over an idle action is picked (scratch 5 in 10, juggle 3, the other juggling 2) and repeated
void TileMaster::UpdateIdle()
{
	if (mIdling)
	{
		if (mAction == ACTION_STAND)
		{
			if (--mIdleRepeats < 1)
				EndIdle();
			else
				SetAction(mIdleAction);
		}
		return;
	}

	if (--mIdleDelay > 0)
		return;

	SetRow(0);
	int aPick = CrtRand() % 10;
	if (aPick < 5)
	{
		mIdleAction = ACTION_SCRATCH;
		mIdleRepeats = CrtRand() % 3 + 3;
	}
	else if (aPick < 8)
	{
		mIdleAction = ACTION_JUGGLE;
		mIdleRepeats = CrtRand() % 2 + 2;
	}
	else
	{
		mIdleAction = ACTION_JUGGLE2;
		mIdleRepeats = CrtRand() % 3 + 2;
	}
	SetAction(mIdleAction);
	mIdling = true;
}

// 0x42ad90: which way round the trail goes to the side and place aimed at
void TileMaster::SetTrailTarget(int theSide, int theCursor)
{
	mTrailTargetSide = theSide;
	mTrailTargetPos = theCursor;
	if (mTrailSide == theSide && mTrailPos == theCursor)
		return;

	if (mTrailSide == theSide)
	{
		mTrailMode = 1;
		switch (mTrailSide)
		{
		case 0:
		case 1:
			if (theCursor <= mTrailPos)
				return;
			break;
		default:
			if (mTrailPos <= theCursor)
				return;
			break;
		}
		mTrailMode = 2;
		return;
	}

	// the other sides: the next one round counts up, the one before counts down, the one across by where it is
	bool anAcross = false;
	switch (mTrailSide)
	{
	case 0:
		if (theSide == 1)
		{
			mTrailMode = 1;
			return;
		}
		if (theSide == 3)
		{
			mTrailMode = 2;
			return;
		}
		anAcross = true;
		break;
	case 1:
		if (theSide == 2)
		{
			mTrailMode = 1;
			return;
		}
		if (theSide == 0)
		{
			mTrailMode = 2;
			return;
		}
		anAcross = true;
		break;
	case 2:
		if (theSide == 3)
		{
			mTrailMode = 1;
			return;
		}
		if (theSide == 1)
		{
			mTrailMode = 2;
			return;
		}
		mTrailMode = (mTrailPos + theCursor <= mRows / 2) + 1;
		return;
	default:
		if (theSide == 0)
		{
			mTrailMode = 1;
			return;
		}
		if (theSide == 2)
		{
			mTrailMode = 2;
			return;
		}
		mTrailMode = (mTrailPos + theCursor <= mRows / 2) + 1;
		return;
	}
	if (anAcross)
		mTrailMode = (mRows / 2 < mTrailPos + theCursor) + 1;
}

// 0x42b180: where the dot is. The sides run left (1, up), top (2, to the right), right (3, down) and bottom (0, to the left);
// at a corner the dot turns on a quarter circle of one cell's radius, and the corner at the end of the bottom and the
// left sides takes longer, as they are two cells off the grid
void TileMaster::TrailPlace(int* theX, int* theY) const
{
	const int S = Grid::CELL_SIZE;
	const int A = mRows;
	const int B = mCols;
	const float aPhase = mTrailPhase;
	int aX = 0, aY = 0;
	int aCX, aCY;

	switch (mTrailSide)
	{
	case 3:
		if (mTrailPos == A - 1 && aPhase > 2.0f)
		{
			if (aPhase <= 3.0f)
			{
				CellCenter(A + 1, B, &aCX, &aCY);
				float anAngle = (aPhase - 2.0f) * 90.0f;
				aX = (int) (cos(anAngle * 0.017453292519943295) * S + aCX);
				aY = (int) (sin(anAngle * 0.017453292519943295) * S + aCY);
			}
			else if (aPhase <= 4.0f)
			{
				CellCenter(A + 2, B, &aX, &aY);
				aX -= (int) (S * (aPhase - 3.0f));
			}
			break;
		}
		CellCenter(mTrailPos, B + 1, &aX, &aY);
		aY += (int) (S * aPhase);
		break;
	case 1:
		if (mTrailPos == 0 && aPhase > 0.0f)
		{
			if (aPhase <= 1.0f)
			{
				CellCenter(0, -1, &aCX, &aCY);
				float anAngle = aPhase * 90.0f + 180.0f;
				aX = (int) (cos(anAngle * 0.017453292519943295) * S + aCX);
				aY = (int) (sin(anAngle * 0.017453292519943295) * S + aCY);
			}
			else if (aPhase <= 2.0f)
			{
				CellCenter(-1, -1, &aX, &aY);
				aX += (int) (S * (aPhase - 1.0f));
			}
			break;
		}
		CellCenter(mTrailPos, -2, &aX, &aY);
		aY -= (int) (S * aPhase);
		break;
	case 2:
		if (mTrailPos == B - 1 && aPhase > 1.0f)
		{
			if (aPhase <= 2.0f)
			{
				CellCenter(0, B, &aCX, &aCY);
				float anAngle = (aPhase - 1.0f) * 90.0f + 270.0f;
				aX = (int) (cos(anAngle * 0.017453292519943295) * S + aCX);
				aY = (int) (sin(anAngle * 0.017453292519943295) * S + aCY);
			}
			break;
		}
		CellCenter(-1, mTrailPos, &aX, &aY);
		aX += (int) (S * aPhase);
		break;
	default:
		if (mTrailPos == 0 && aPhase > 1.0f)
		{
			if (aPhase <= 2.0f)
			{
				CellCenter(A + 1, -1, &aCX, &aCY);
				float anAngle = aPhase * 90.0f;
				aX = (int) (cos(anAngle * 0.017453292519943295) * S + aCX);
				aY = (int) (sin(anAngle * 0.017453292519943295) * S + aCY);
			}
			else if (aPhase <= 4.0f)
			{
				CellCenter(A + 1, -2, &aX, &aY);
				aY -= (int) (S * (aPhase - 2.0f));
			}
			break;
		}
		CellCenter(A + 2, mTrailPos, &aX, &aY);
		aX -= (int) (S * aPhase);
		break;
	}
	*theX = aX;
	*theY = aY;
}

// 0x42bf50: five steps of 0.3 a cell each update, each leaving a puff; then the puffs age
void TileMaster::UpdateTrail()
{
	const float aStep = 0.3f;
	const int A = mRows;
	const int B = mCols;

	if (mTrailMode == 1)
	{
		for (int aStepIndex = 0; aStepIndex < 5 && mTrailMode != 0; aStepIndex++)
		{
			mTrailPhase += aStep;
			if (mTrailSide == 1 && mTrailPos == 0)
			{
				if (mTrailPhase > 2.0f)
				{
					mTrailPos = 0;
					mTrailSide = 2;
					mTrailPhase -= 2.0f;
				}
			}
			else if (mTrailSide == 2 && mTrailPos == B - 1)
			{
				if (mTrailPhase > 2.0f)
				{
					mTrailPos = 0;
					mTrailSide = 3;
					mTrailPhase -= 2.0f;
				}
			}
			else if (mTrailSide == 3 && mTrailPos == A - 1)
			{
				if (mTrailPhase > 4.0f)
				{
					mTrailPhase -= 4.0f;
					mTrailSide = 0;
					mTrailPos = B - 1;
				}
			}
			else if (mTrailSide == 0 && mTrailPos == 0)
			{
				if (mTrailPhase > 4.0f)
				{
					mTrailPhase -= 4.0f;
					mTrailSide = 1;
					mTrailPos = A - 1;
				}
			}
			else if (mTrailPhase > 1.0f)
			{
				mTrailPhase -= 1.0f;
				mTrailPos += mTrailSide <= 1 ? -1 : 1;
			}
			Puff aPuff;
			aPuff.mLife = PUFF_LIFE;
			TrailPlace(&aPuff.mX, &aPuff.mY);
			mPuffs.push_back(aPuff);
			if (mTrailSide == mTrailTargetSide && mTrailPos == mTrailTargetPos)
				mTrailMode = 0;
		}
	}
	else if (mTrailMode == 2)
	{
		for (int aStepIndex = 0; aStepIndex < 5 && mTrailMode != 0; aStepIndex++)
		{
			mTrailPhase -= aStep;
			if (mTrailSide == 1 && mTrailPos == A + 1)
			{
				if (mTrailPhase < 0.0f)
				{
					mTrailPos = 0;
					mTrailSide = 0;
					mTrailPhase += 4.0f;
				}
			}
			else if (mTrailSide == 2 && mTrailPos == 0)
			{
				if (mTrailPhase < 0.0f)
				{
					mTrailPos = 0;
					mTrailSide = 1;
					mTrailPhase += 2.0f;
				}
			}
			else if (mTrailSide == 3 && mTrailPos == 0)
			{
				if (mTrailPhase < 0.0f)
				{
					mTrailPhase += 2.0f;
					mTrailSide = 2;
					mTrailPos = B - 1;
				}
			}
			else if (mTrailSide == 0 && mTrailPos == B - 1)
			{
				if (mTrailPhase < 0.0f)
				{
					mTrailPhase += 4.0f;
					mTrailSide = 3;
					mTrailPos = A - 1;
				}
			}
			else if (mTrailPhase < -1.0f)
			{
				mTrailPhase += 1.0f;
				mTrailPos += mTrailSide <= 1 ? 1 : -1;
			}
			Puff aPuff;
			aPuff.mLife = PUFF_LIFE;
			TrailPlace(&aPuff.mX, &aPuff.mY);
			mPuffs.push_back(aPuff);
			if (mTrailSide == mTrailTargetSide && mTrailPos == mTrailTargetPos)
				mTrailMode = 0;
		}
	}

	size_t aKept = 0;
	for (size_t i = 0; i < mPuffs.size(); i++)
	{
		if (mPuffs[i].mLife > 0)
			mPuffs[i].mLife--;
		if (mPuffs[i].mLife >= 1)
			mPuffs[aKept++] = mPuffs[i];
	}
	mPuffs.resize(aKept);
}

// 0x42c460
void TileMaster::Update()
{
	AdvanceAnimation();
	UpdateTrail();
	if (mSettleDelay > 0 && --mSettleDelay < 1)
	{
		Settle();
		return;
	}
	UpdateIdle();
}

// 0x42b6a0: the puffs, the shadow, then the frame
void TileMaster::Draw(Graphics* g, int theAlpha)
{
	Image* aPuffImage = mApp->mImages[IMAGE_PLAYERTRAILPARTICLE];
	g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
	g->SetColorizeImages(true);
	for (size_t i = 0; i < mPuffs.size(); i++)
	{
		int aLife = mPuffs[i].mLife * 255;
		int anAlpha = (aLife + ((aLife >> 31) & 7)) >> 3;
		g->SetColor(Color(255, 255, 255, anAlpha * theAlpha / 255));
		g->DrawImage(aPuffImage, mPuffs[i].mX - aPuffImage->GetWidth() / 2, mPuffs[i].mY - aPuffImage->GetHeight() / 2);
	}

	g->SetColorizeImages(theAlpha < 255);
	g->SetColor(Color(255, 255, 255, theAlpha));
	Image* aShadow = mApp->mImages[IMAGE_TILEMASTER_SHADOW];
	g->DrawImage(aShadow, mStandX - aShadow->GetWidth() / 2, mStandY - aShadow->GetHeight() / 2);

	const ActionInfo& anAction = sActions[mAction];
	int aRow = mRow >= anAction.mRows ? anAction.mRows - 1 : mRow;
	int aFrame = mFrame >= anAction.mFrames ? anAction.mFrames - 1 : mFrame;
	Image* aSheet = mApp->mImages[anAction.mImage];
	int aWidth = aSheet->GetWidth() / anAction.mFrames;
	int aHeight = aSheet->GetHeight() / anAction.mRows;
	g->DrawImage(aSheet, mStandX - PIVOT_X, mStandY - anAction.mPivotY, Rect(aFrame * aWidth, aRow * aHeight, aWidth, aHeight));
	g->SetColorizeImages(false);
}
