// The board's cells (0x4034f0..0x404d10): 16x16 squares in 50 columns. Captured cells slide into place one by one
// from a cell next to them; the board's wave fades the cells in at the start and out when the level is won.

#include "CBLevel.h"
#include "R64Blit.h"

#include <cstdlib>

namespace cb
{

// The original reads one row past a column's end (Cell_SameNeighbours and the like test row <= rows): the port's
// columns have a 38th cell there, a solid one, where the original reads whatever the heap holds
Cell* Cells_New()
{
	Cell* aCells = (Cell*)::operator new(sizeof(Cell) * 38);
	for (int i = 0; i < 38; i++)
		Cell_Init(&aCells[i]);
	return aCells;
}

void Cells_Delete(Cell* theCells)
{
	for (int i = 38; i > 0; i--)
		Obj_Destroy(&theCells[i - 1]);
	::operator delete(theCells);
}

Cell* Cell_Init(Cell* theCell)
{
	Obj_Init(theCell);
	theCell->mStep = 0;
	theCell->mShadowRight = 0;
	theCell->mShadowBelow = 0;
	theCell->mUnmarked = 1;
	theCell->mDrawKind = 1;
	theCell->mSteps = 0xb4;
	theCell->mKind = CELL_SOLID;
	theCell->mAlpha = 0xff;
	return theCell;
}

static Cell* At(int theCol, int theRow)
{
	return &gLevels.mCurrent->mColumns[theCol][theRow];
}

// Moving cells go on the screen; still ones into the back buffer, with the halves of their filled neighbours
// (around edges and beams) and the shadows they cast to the right and down
void Cell_Redraw(Cell* theCell)
{
	if (theCell->mImage == nullptr)
		return;
	Image* anImage = theCell->mImage;
	int aX = (int)theCell->mX;
	int aSx = aX % anImage->mW;
	int aY = (int)theCell->mY;
	int aSy = aY % anImage->mH;
	int aState = theCell->mState;
	if (aState == CELL_CAPTURING)
	{
		int aStep = theCell->mStep;
		int aSteps = theCell->mSteps;
		int aDx = (int)(((double)theCell->mOffX + (double)theCell->mX) - (double)((theCell->mOffX * aStep) / aSteps));
		int aDy = (int)(((double)theCell->mOffY + (double)theCell->mY) - (double)((theCell->mOffY * aStep) / aSteps));
		if (-1 < aDx && -1 < aDy && aDx + 0x10 < 800 && aDy + 0x10 < 600)
		{
			Image* anImg = theCell->mImage;
			Alpha16MaskFade(gScreenBits, gScreenPitch, &gBackBuffer.mFmt, aDx, aDy, anImg->mBits, anImg->mPitch, aSx, aSy,
				anImg->mMask->mBits, anImg->mMask->mPitch, 0, 0, 0x10, 0x10, theCell->mAlpha);
			Dirty_AddClippedShown(&gDirty, aDx, aDy, 0x10, 0x10, SCREEN_CLIP);
			return;
		}
	}
	else if (aState == CELL_IN_WAVE)
	{
		if (theCell->mKind != CELL_SOLID)
		{
			float t = (float)((double)theCell->mStep / (double)theCell->mSteps);
			if (gLevels.mCurrent->mFilledCells == 0)
				t = (float)(1.0 - (double)t);
			t = t * t;
			double anOffX = (double)t * (double)theCell->mOffX;
			float aFx = (float)(anOffX + (double)theCell->mX);
			double anOffY = (double)t * (double)theCell->mOffY;
			float aFy = (float)(anOffY + (double)theCell->mY);
			if (0.0f <= aFx && 0.0f <= aFy && (double)aFx + 16.0 < 800.0 && 16.0 + (double)aFy < 600.0)
			{
				Image* anImg = theCell->mImage;
				int aDy = (int)aFy;
				int aDx = (int)aFx;
				Alpha16MaskFade(gScreenBits, gScreenPitch, &gBackBuffer.mFmt, aDx, aDy, anImg->mBits, anImg->mPitch, aSx, aSy,
					anImg->mMask->mBits, anImg->mMask->mPitch, 0, 0, 0x10, 0x10, theCell->mAlpha);
				Dirty_AddClippedShown(&gDirty, aDx, aDy, 0x10, 0x10, SCREEN_CLIP);
				return;
			}
		}
	}
	else if (aState != CELL_WAITING)
	{
		Image* aFill = gLevels.mCurrent->mFillImage;
		int aFsx = aX % aFill->mW;
		int aFsy = aY % aFill->mH;
		uint8_t* aBack = gBackBuffer.mBits;
		int aPitch = gBackBuffer.mPitch;
		const PixelFormat* aFmt = &gBackBuffer.mFmt;
		int aCol = theCell->mCol;
		int aRow = theCell->mRow;
		if (theCell->mKind == CELL_EDGE || theCell->mKind == CELL_LINE)
		{
			// the filled neighbours' halves next to this cell
			if (At(aCol - 1, aRow)->mKind == CELL_FILLED)
				Alpha16Mask(aBack, aPitch, aFmt, aX, aY, aFill->mBits, aFill->mPitch, aFsx, aFsy, aFill->mMask->mBits,
					aFill->mMask->mPitch, 0, 0, 8, 0x10);
			if (At(aCol + 1, aRow)->mKind == CELL_FILLED)
				Alpha16Mask(aBack, aPitch, aFmt, (int)((double)theCell->mX + 8.0), (int)theCell->mY, aFill->mBits,
					aFill->mPitch, aFsx + 8, aFsy, aFill->mMask->mBits, aFill->mMask->mPitch, 0, 0, 8, 0x10);
			if (At(aCol, aRow + 1)->mKind == CELL_FILLED)
				Alpha16Mask(aBack, aPitch, aFmt, (int)theCell->mX, (int)((double)theCell->mY + 8.0), aFill->mBits,
					aFill->mPitch, aFsx, aFsy + 8, aFill->mMask->mBits, aFill->mMask->mPitch, 0, 0, 0x10, 8);
			if (At(aCol, aRow - 1)->mKind == CELL_FILLED)
				Alpha16Mask(aBack, aPitch, aFmt, (int)theCell->mX, (int)theCell->mY, aFill->mBits, aFill->mPitch, aFsx, aFsy,
					aFill->mMask->mBits, aFill->mMask->mPitch, 0, 0, 0x10, 8);
			// the beam neighbours' halves
			Cell* aNext = At(aCol - 1, aRow);
			if (aNext->mKind == CELL_LINE)
			{
				Image* anImg = aNext->mImage;
				Alpha16Mask(aBack, aPitch, aFmt, (int)theCell->mX, (int)theCell->mY, anImg->mBits, anImg->mPitch, 0, 0,
					anImg->mMask->mBits, anImg->mMask->mPitch, 0, 0, 8, 0x10);
			}
			aNext = At(aCol + 1, aRow);
			if (aNext->mKind == CELL_LINE)
			{
				Image* anImg = aNext->mImage;
				Alpha16Mask(aBack, aPitch, aFmt, (int)((double)theCell->mX + 8.0), (int)theCell->mY, anImg->mBits,
					anImg->mPitch, 8, 0, anImg->mMask->mBits, anImg->mMask->mPitch, 0, 0, 8, 0x10);
			}
			aNext = At(aCol, aRow + 1);
			if (aNext->mKind == CELL_LINE)
			{
				Image* anImg = aNext->mImage;
				Alpha16Mask(aBack, aPitch, aFmt, (int)theCell->mX, (int)((double)theCell->mY + 8.0), anImg->mBits,
					anImg->mPitch, 0, 8, anImg->mMask->mBits, anImg->mMask->mPitch, 0, 0, 0x10, 8);
			}
			aNext = At(aCol, aRow - 1);
			if (aNext->mKind == CELL_LINE)
			{
				Image* anImg = aNext->mImage;
				Alpha16Mask(aBack, aPitch, aFmt, (int)theCell->mX, (int)theCell->mY, anImg->mBits, anImg->mPitch, 0, 0,
					anImg->mMask->mBits, anImg->mMask->mPitch, 0, 0, 0x10, 8);
			}
		}
		if (theCell->mKind == CELL_LINE || theCell->mKind == CELL_EDGE)
		{
			// its shadow, once, on filled cells to the right and below
			Cell* aRight = At(aCol + 1, aRow);
			if (aRight->mKind == CELL_FILLED && aRight->mShadowRight == 0)
			{
				Image* aMask = theCell->mImage->mMask;
				Fill16MaskAlpha(aBack, aPitch, aFmt, (int)((double)theCell->mX + 5.0), (int)theCell->mY, aSx, aSy,
					aMask->mBits, aMask->mPitch, 0, 0, 0x10, 0x10, 0, 0x5a);
				At(aCol + 1, aRow)->mShadowRight = 1;
			}
			Cell* aBelow = At(aCol, aRow + 1);
			if (aBelow->mKind == CELL_FILLED && aBelow->mShadowBelow == 0)
			{
				Image* aMask = theCell->mImage->mMask;
				Fill16MaskAlpha(aBack, aPitch, aFmt, (int)theCell->mX, (int)((double)theCell->mY + 5.0), aSx, aSy,
					aMask->mBits, aMask->mPitch, 0, 0, 0x10, 0x10, 0, 0x5a);
				At(aCol, aRow + 1)->mShadowBelow = 1;
			}
		}
		Image* anImg = theCell->mImage;
		Alpha16Mask(aBack, aPitch, aFmt, (int)theCell->mX, (int)theCell->mY, anImg->mBits, anImg->mPitch, aSx, aSy,
			anImg->mMask->mBits, anImg->mMask->mPitch, 0, 0, 0x10, 0x10);
		Dirty_AddClippedShown(&gDirty, (int)theCell->mX, (int)theCell->mY, 0x18, 0x18, SCREEN_CLIP);
	}
}

uint32_t Cell_SameNeighbours(Cell* theCell)
{
	return Cell_Touches(theCell, theCell->mKind);
}

uint32_t Cell_Touches(Cell* theCell, int theKind)
{
	int aCol = theCell->mCol;
	int aRow = theCell->mRow;
	uint32_t aMask = 0;
	for (int r = aRow - 1; r <= aRow + 1; r++)
	{
		for (int k = aCol - 1; k <= aCol + 1; k++)
		{
			if (-1 < k && k < gLevels.mCurrent->mCols && -1 < r && r <= gLevels.mCurrent->mRows &&
				At(k, r)->mKind == theKind)
				aMask = aMask | 1u << (((r - aRow) * 3 + (k - aCol) + 4) & 0x1f);
		}
	}
	return aMask;
}

void Cell_Flood(Cell* theCell)
{
	int aCol = theCell->mCol;
	theCell->mUnmarked = 0;
	int aRow = theCell->mRow;
	gLevels.mCurrent->mFloodedCells = gLevels.mCurrent->mFloodedCells + 1;
	for (int r = aRow - 1; r <= aRow + 1; r++)
	{
		for (int k = aCol - 1; k <= aCol + 1; k++)
		{
			if ((k == aCol || r == aRow) && -1 < k && k < gLevels.mCurrent->mCols && -1 < r &&
				r <= gLevels.mCurrent->mRows)
			{
				Cell* aNext = At(k, r);
				if (aNext->mKind == CELL_OPEN && aNext->mUnmarked == 1)
					Cell_Flood(aNext);
			}
		}
	}
}

// Edges by their edge neighbours (bits 1 up, 3 left, 5 right, 7 down): the level's patterns along a side, its
// corners; beams by theirs
int Cell_PickImage(Cell* theCell)
{
	uint32_t aSameNeighbours = Cell_SameNeighbours(theCell);
	Level* aLevel = gLevels.mCurrent;
	int aKind = theCell->mKind;
	if (aKind == CELL_OPEN)
	{
		theCell->mImage = aLevel->mEmpty;
		return 1;
	}
	if (aKind == CELL_EDGE)
	{
		uint32_t aLeft = aSameNeighbours & 8;
		if (aLeft != 0 && (aSameNeighbours & 0x20) != 0)
		{
			theCell->mImage = Animation_Frame(aLevel->mHTiles, aLevel->mHPattern[theCell->mCol % aLevel->mHPatternCount]);
			return 1;
		}
		uint32_t anUp = aSameNeighbours & 2;
		if (anUp != 0 && (int8_t)aSameNeighbours < 0)
		{
			theCell->mImage = Animation_Frame(aLevel->mVTiles, aLevel->mVPattern[theCell->mRow % aLevel->mVPatternCount]);
			return 1;
		}
		if ((aSameNeighbours & 0x20) != 0 && (int8_t)aSameNeighbours < 0)
		{
			theCell->mImage = Animation_Frame(aLevel->mCornerTiles, theCell->mCol % aLevel->mCornerTileCount);
			return 1;
		}
		if (aLeft != 0 && (int8_t)aSameNeighbours < 0)
		{
			theCell->mImage = Animation_Frame(aLevel->mCornerTiles, 0);
			return 1;
		}
		if ((aSameNeighbours & 0x20) != 0 && anUp != 0)
		{
			theCell->mImage = Animation_Frame(aLevel->mCornerTiles, 0);
			return 1;
		}
		if (aLeft != 0 && anUp != 0)
		{
			theCell->mImage = Animation_Frame(aLevel->mCornerTiles, 0);
			return 1;
		}
	}
	else if (aKind == CELL_BEAM || aKind == CELL_LINE)
	{
		uint32_t aDown = aSameNeighbours & 0x80;
		if (aDown == 0 || (aSameNeighbours & 8) == 0)
		{
			uint32_t anUp = aSameNeighbours & 2;
			if (anUp != 0 && (aSameNeighbours & 8) != 0)
			{
				theCell->mImage = aLevel->mBeamMiddle;
				return 1;
			}
			uint32_t aRight = aSameNeighbours & 0x20;
			if (aRight != 0 && anUp != 0)
			{
				theCell->mImage = aLevel->mBeamMiddle;
				return 1;
			}
			if (aRight == 0 || aDown == 0)
			{
				if (anUp == 0 && aDown == 0)
				{
					if ((aSameNeighbours & 8) == 0 && aRight == 0)
						return 1;
					theCell->mImage = aLevel->mBeamH;
					return 1;
				}
				theCell->mImage = aLevel->mBeamV;
				return 1;
			}
		}
		theCell->mImage = aLevel->mBeamMiddle;
		return 1;
	}
	return 1;
}

// It joins the level's moving cells and slides in from a random direction, 18 pixels out
void Cell_StartCapture(Cell* theCell, Image* theImage, int theFrom, int theTo)
{
	List_Append(&gLevels.mCurrent->mFills, theCell);
	theCell->mStep = theCell->mSteps / 10;
	theCell->mFromKind = theFrom;
	theCell->mToKind = theTo;
	theCell->mKind = theTo;
	theCell->mState = CELL_CAPTURING;
	theCell->mImage = theImage;
	int anAngle = RandRange(0, 0x167);
	theCell->mOffX = (int)((double)CosDeg(anAngle) * 18.0);
	theCell->mOffY = (int)((double)SinDeg(anAngle) * 18.0);
	Cell_PickImage(theCell);
}

void Cell_StartFill(Cell* theCell)
{
	List_Append(&gLevels.mCurrent->mFills, theCell);
	RandRange(0, 0x167);
	if (gLevels.mCurrent->mFilledCells != 0)
	{
		// taken down: each cell drops away
		theCell->mOffX = 0;
		theCell->mOffY = 0x32;
		theCell->mStep = 0;
		theCell->mState = CELL_IN_WAVE;
		return;
	}
	theCell->mOffX = RandRange(-40, 40);
	theCell->mOffY = RandRange(-40, 40);
	theCell->mStep = 0;
	theCell->mState = CELL_IN_WAVE;
}

// ' ' solid, ':' open, 'X' edge, '$' filled; any other code is a key (Level_AddKey) on an edge cell when the cell
// to its left or above is one, else on an open one
void Cell_Set(Cell* theCell, char theCode, int theCol, int theRow)
{
	while (true)
	{
		switch (theCode)
		{
		case ' ':
			theCell->mKind = CELL_SOLID;
			goto place;
		default:
			Level_AddKey(gLevels.mCurrent, theCode, theCol, theRow);
			if (At(theCol - 1, theRow)->mKind == CELL_EDGE || At(theCol, theRow - 1)->mKind == CELL_EDGE)
				theCode = 'X';
			else
				theCode = ':';
			break;
		case '$':
			theCell->mKind = CELL_OPEN;
			Cell_StartCapture(theCell, gLevels.mCurrent->mFillImage, CELL_OPEN, CELL_FILLED);
			goto place;
		case ':':
			theCell->mKind = CELL_OPEN;
			gLevels.mCurrent->mTotalCells = gLevels.mCurrent->mTotalCells + 1;
			theCell->mImage = gLevels.mCurrent->mEmpty;
			goto place;
		case 'X':
			theCell->mImage = Bank_FindImage2(&gArtBank, "cell_base");
			theCell->mKind = CELL_EDGE;
			theCell->mAlpha = 0;
			goto place;
		}
	}
place:
	theCell->mCol = theCol;
	theCell->mRow = theRow;
	theCell->mX = (float)(theCol << 4);
	theCell->mY = (float)(theRow << 4);
	theCell->mFirstParticle = Bank_IndexOf(&gArtBank, "particle_0");
	theCell->mLastParticle = Bank_IndexOf(&gArtBank, "particle_0");
}

// Right, else left; then down, else up
void Cell_FillNext(Cell* theCell)
{
	Cell* aNext = At(theCell->mCol + 1, theCell->mRow);
	if (aNext->mState == CELL_WAITING || (aNext = At(theCell->mCol - 1, theCell->mRow), aNext->mState == CELL_WAITING))
		Cell_StartFill(aNext);
	Cell* aCell = At(theCell->mCol, theCell->mRow);
	if (aCell[1].mState == CELL_WAITING)
	{
		Cell_StartFill(&aCell[1]);
		return;
	}
	if (aCell[-1].mState == CELL_WAITING)
	{
		Cell_StartFill(&aCell[-1]);
		return;
	}
}

// To the right, left, down and up: the marked neighbours of the kind the capture takes
void Cell_SpreadCapture(Cell* theCell)
{
	Cell* aNext = At(theCell->mCol + 1, theCell->mRow);
	if (aNext->mKind == theCell->mFromKind && aNext->mUnmarked == 0 && aNext->mState != CELL_CAPTURING)
		Cell_StartCapture(aNext, theCell->mImage, theCell->mFromKind, theCell->mToKind);
	aNext = At(theCell->mCol - 1, theCell->mRow);
	if (aNext->mKind == theCell->mFromKind && aNext->mUnmarked == 0 && aNext->mState != CELL_CAPTURING)
		Cell_StartCapture(aNext, theCell->mImage, theCell->mFromKind, theCell->mToKind);
	Cell* aCell = At(theCell->mCol, theCell->mRow);
	if (aCell[1].mKind == theCell->mFromKind && aCell[1].mUnmarked == 0 && aCell[1].mState != CELL_CAPTURING)
		Cell_StartCapture(&aCell[1], theCell->mImage, theCell->mFromKind, theCell->mToKind);
	aCell = At(theCell->mCol, theCell->mRow);
	if (aCell[-1].mKind == theCell->mFromKind && aCell[-1].mUnmarked == 0 && aCell[-1].mState != CELL_CAPTURING)
		Cell_StartCapture(&aCell[-1], theCell->mImage, theCell->mFromKind, theCell->mToKind);
}

// A third of the way it spreads; three quarters of the way it may puff; at the end it scores, lands in the back
// buffer and redraws the edges and beams around it
int Cell_UpdateCapture(Cell* theCell)
{
	int aSteps = theCell->mSteps;
	theCell->mStep = theCell->mStep + 1;
	int aStep = theCell->mStep;
	if (aStep <= aSteps)
	{
		if (aStep != aSteps / 3)
		{
			if (aStep == (aSteps * 3) / 4 && RandRange(1, 2) == 2)
			{
				ParticleSystem_New();
				int aLifetime = RandRange(400, 500);
				int aLast = theCell->mLastParticle;
				int aFirst = theCell->mFirstParticle;
				float aVy = (float)((double)RandRange(-160, -50) / 1000.0);
				float aVx = (float)((double)RandRange(-50, 50) / 1000.0);
				ParticleSystem_Add(gLevels.mCurrent->mSystems.mCurrent, theCell->mX + 5.0f, theCell->mY + 5.0f, aVx, aVy,
					aFirst, aLast, 0, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x21c, 10, aLifetime, 1.0f, 0.0006f, 5);
			}
			return 1;
		}
		Cell_SpreadCapture(theCell);
		return 1;
	}
	gScore = gScore + gLevels.mCurrent->mShooters.mCurrent->mCellPoints;
	theCell->mUnmarked = 1;
	theCell->mState = CELL_STILL;
	gLevels.mCurrent->mFilledCells = gLevels.mCurrent->mFilledCells + 1;
	Cell_Redraw(theCell);
	int anAlpha = (theCell->mStep * 0x142) / theCell->mSteps;
	theCell->mShadowRight = 0;
	theCell->mShadowBelow = 0;
	if (0xff < anAlpha)
		anAlpha = 0xff;
	theCell->mAlpha = anAlpha;
	for (int aDy = -1; aDy < 2; aDy++)
	{
		for (int aDx = -1; aDx < 2; aDx++)
		{
			if ((aDx == 0 && aDy != 0) || (aDx != 0 && aDy == 0))
			{
				Cell* aNext = At(theCell->mCol + aDx, theCell->mRow + aDy);
				int aKind = aNext->mKind;
				if ((aKind == CELL_EDGE || aKind == CELL_LINE) && (theCell->mKind == CELL_FILLED || aKind == CELL_EDGE))
					Cell_Redraw(aNext);
			}
		}
	}
	return 0;
}

int Cell_UpdateFill(Cell* theCell)
{
	int aResult = 1;
	if (theCell->mKind != CELL_SOLID)
	{
		float t = (float)((double)theCell->mStep / (double)theCell->mSteps);
		if (gLevels.mCurrent->mFilledCells == 0)
			t = (float)(1.0 - (double)t);
		int anAlpha = (int)((double)t * 255.0);
		if (0xff < anAlpha)
			anAlpha = 0xff;
		theCell->mAlpha = 0xff - anAlpha;
	}
	if (theCell->mStep == 0)
	{
		// the background under it
		int aY = (int)theCell->mY;
		int aX = (int)theCell->mX;
		Image* aBack = gLevels.mCurrent->mBackground;
		Copy16(gBackBuffer.mBits, gBackBuffer.mPitch, aX, aY, aBack->mBits, aBack->mPitch, aX, aY, 0x10, 0x10);
	}
	theCell->mStep = theCell->mStep + 1;
	if (theCell->mSteps < theCell->mStep)
	{
		theCell->mState = CELL_STILL;
		aResult = 0;
		if (gLevels.mCurrent->mFilledCells == 0)
		{
			if (theCell->mKind != CELL_SOLID && CELL_OPEN < theCell->mKind)
			{
				for (int i = 0; i < 3; i++)
				{
					ParticleSystem_New();
					int aLifetime = RandRange(400, 500);
					int aLast = theCell->mLastParticle;
					int aFirst = theCell->mFirstParticle;
					float aVy = (float)((double)RandRange(-50, 50) / 500.0);
					float aVx = (float)((double)RandRange(-50, 50) / 500.0);
					float aY = (float)((double)theCell->mY + 5.0 + (double)RandRange(-5, 5));
					float aX = (float)((double)theCell->mX + 5.0 + (double)RandRange(-5, 5));
					ParticleSystem_Add(gLevels.mCurrent->mSystems.mCurrent, aX, aY, aVx, aVy, aFirst, aLast, 0, 0xff, 0xff,
						0xff, 0xff, 0xff, 0xff, 400, 10, aLifetime, 0.999f, 0.0002f, 5);
				}
			}
			Cell_Redraw(theCell);
			return 0;
		}
		theCell->mKind = CELL_SOLID;
		float anOffX = (float)theCell->mOffX;
		int aDx = (int)(((double)theCell->mX + (double)anOffX) - (double)anOffX);
		float anOffY = (float)theCell->mOffY;
		int aDy = (int)(((double)theCell->mY + (double)anOffY) - (double)anOffY);
		if (-1 < aDx && -1 < aDy && aDx + 0x10 < 800 && aDy + 0x10 < 600)
		{
			Dirty_AddClippedShown(&gDirty, aDx, aDy, 0x10, 0x10, SCREEN_CLIP);
			return 0;
		}
	}
	else if (theCell->mStep == theCell->mSteps / 0x32)
		Cell_FillNext(theCell);
	return aResult;
}

int Fill_Update(Cell* theCell)
{
	if (theCell->mState == CELL_CAPTURING)
		return Cell_UpdateCapture(theCell);
	if (theCell->mState == CELL_IN_WAVE)
		return Cell_UpdateFill(theCell);
	return 0;
}


// The background under a cell, back in the back buffer
static void Cell_Uncover(Cell* theCell)
{
	int aY = (int)theCell->mY;
	int aX = (int)theCell->mX;
	Image* aBack = gLevels.mCurrent->mBackground;
	Copy16(gBackBuffer.mBits, gBackBuffer.mPitch, aX, aY, aBack->mBits, aBack->mPitch, aX, aY, 0x10, 0x10);
}

static bool InBoard(int theCol, int theRow)
{
	return -1 < theCol && theCol < gLevels.mCurrent->mCols && -1 < theRow && theRow < gLevels.mCurrent->mRows;
}

// The filled cells and lines of the square of theRadius round a cell that the last flood did not reach open up,
// the square's outer ring becoming lines. Then each row of the square (the original runs the tidy-up inside the
// row loop) redraws its lines, throws fire from each cell when theRadius is 3 or more, and opens the lines with
// open or edge cells on three of their four sides until none is left.
void Level_Blast(int theCol, int theRow, int theRadius)
{
	for (int aRow = theRow - theRadius; aRow <= theRow + theRadius; aRow++)
	{
		for (int aCol = theCol - theRadius; aCol <= theCol + theRadius; aCol++)
		{
			if (InBoard(aCol, aRow))
			{
				Cell* aCell = At(aCol, aRow);
				if ((aCell->mKind == CELL_FILLED || aCell->mKind == CELL_LINE) && aCell->mUnmarked != 0)
				{
					aCell->mKind = CELL_OPEN;
					aCell->mState = CELL_STILL;
					aCell->mImage = gLevels.mCurrent->mEmpty;
					Cell_Uncover(aCell);
					Cell_Redraw(aCell);
					if (theRadius + -1 < std::abs(aCol - theCol) || theRadius + -1 < std::abs(aRow - theRow))
						aCell->mKind = CELL_LINE;
					else
						gLevels.mCurrent->mFilledCells = gLevels.mCurrent->mFilledCells + -1;
				}
			}
		}
	}
	for (int aRow = theRow - theRadius; aRow <= theRow + theRadius; aRow++)
	{
		for (int aCol = theCol - theRadius; aCol <= theCol + theRadius; aCol++)
		{
			if (InBoard(aCol, aRow))
			{
				Cell* aCell = At(aCol, aRow);
				if (aCell->mKind == CELL_LINE)
				{
					Cell_PickImage(aCell);
					Cell_Redraw(aCell);
				}
				Dirty_AddClippedShown(&gDirty, (int)aCell->mX, (int)aCell->mY, 0x10, 0x10, SCREEN_CLIP);
				for (int i = 1; i < theRadius; i++)
				{
					if (theRadius < 3)
						break;
					ParticleSystem_New();
					int aLifetime = RandRange(700, 0x44c);
					int aLast = Bank_IndexOf(&gArtBank, "exp_4");
					int aFirst = Bank_IndexOf(&gArtBank, "exp_0");
					aFirst = aFirst + RandRange(0, 3);
					float aVy = (float)((double)RandRange(-100, 100) / 1000.0);
					float aVx = (float)((double)RandRange(-100, 100) / 1000.0);
					float aY = (float)RandRange(-10, 10) + aCell->mY;
					float aX = (float)RandRange(-10, 10) + aCell->mX;
					ParticleSystem_Add(gLevels.mCurrent->mSystems.mCurrent, aX, aY, aVx, aVy, aFirst, aLast, 0, 0xff, 0xff,
						0xff, 0xe7, 0xa6, 0x39, 0x21c, 10, aLifetime, 1.0f, 0.0f, 5);
				}
			}
		}
		bool aChanged;
		do
		{
			aChanged = false;
			for (int aRow2 = theRow - theRadius; aRow2 <= theRow + theRadius; aRow2++)
			{
				for (int aCol2 = theCol - theRadius; aCol2 <= theCol + theRadius; aCol2++)
				{
					if (InBoard(aCol2, aRow2) && At(aCol2, aRow2)->mKind == CELL_LINE)
					{
						Cell* aCell = At(aCol2, aRow2);
						// the four sides' bits of Cell_Touches: left 8, right 0x20, up 2, down 0x80
						uint32_t aSides = Cell_Touches(aCell, CELL_OPEN);
						int aCount = (aSides & 8) != 0;
						if ((aSides & 0x20) != 0)
							aCount = aCount + 1;
						if ((aSides & 2) != 0)
							aCount = aCount + 1;
						if ((aSides & 0x80) != 0)
							aCount = aCount + 1;
						aSides = Cell_Touches(aCell, CELL_EDGE);
						if ((aSides & 8) != 0)
							aCount = aCount + 1;
						if ((aSides & 0x20) != 0)
							aCount = aCount + 1;
						if ((aSides & 2) != 0)
							aCount = aCount + 1;
						if ((aSides & 0x80) != 0)
							aCount = aCount + 1;
						if (2 < aCount)
						{
							aCell->mKind = CELL_OPEN;
							aCell->mImage = gLevels.mCurrent->mEmpty;
							Cell_Uncover(aCell);
							Cell_Redraw(aCell);
							gLevels.mCurrent->mFilledCells = gLevels.mCurrent->mFilledCells + -1;
							aChanged = true;
						}
					}
				}
			}
		} while (aChanged);
	}
}

} // namespace cb
