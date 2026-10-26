// The board's cells (0x403580..0x404dc0): 16x16 squares in 50 columns. Captured cells slide into place one by one
// from a cell next to them; the board's wave fades the cells in at the start and out when the level is won.

#include "WBLevel.h"
#include "R64Blit.h"

#include <cstdlib>

namespace wb
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
	theCell->step = 0;
	theCell->shadowRight = 0;
	theCell->shadowBelow = 0;
	theCell->unmarked = 1;
	theCell->drawKind = 1;
	theCell->steps = 0xb4;
	theCell->kind = 5;
	theCell->alpha = 0xff;
	return theCell;
}

static Cell* At(int theCol, int theRow)
{
	return &gLevels.current->columns[theCol][theRow];
}

// Moving cells go on the screen; still ones into the back buffer, with the halves of their filled neighbours
// (around edges and beams) and the shadows they cast to the right and down
void Cell_Redraw(Cell* theCell)
{
	if (theCell->image == nullptr)
		return;
	Image* anImage = theCell->image;
	int aX = (int)theCell->x;
	int sx = aX % anImage->w;
	int aY = (int)theCell->y;
	int sy = aY % anImage->h;
	int aState = theCell->state;
	if (aState == 5)
	{
		int aStep = theCell->step;
		int aSteps = theCell->steps;
		int dx = (int)(((double)theCell->offX + (double)theCell->x) - (double)((theCell->offX * aStep) / aSteps));
		int dy = (int)(((double)theCell->offY + (double)theCell->y) - (double)((theCell->offY * aStep) / aSteps));
		if (-1 < dx && -1 < dy && dx + 0x10 < 800 && dy + 0x10 < 600)
		{
			Image* img = theCell->image;
			Alpha16MaskFade(gScreenBits, gScreenPitch, &gBackBuffer.fmt, dx, dy, img->bits, img->pitch, sx, sy,
				img->mask->bits, img->mask->pitch, 0, 0, 0x10, 0x10, theCell->alpha);
			Dirty_AddClippedShown(&gDirty, dx, dy, 0x10, 0x10, kScreenClip);
			return;
		}
	}
	else if (aState == 7)
	{
		if (theCell->kind != 5)
		{
			float t = (float)((double)theCell->step / (double)theCell->steps);
			if (gLevels.current->filledCells == 0)
				t = (float)(1.0 - (double)t);
			t = t * t;
			double anOffX = (double)t * (double)theCell->offX;
			float fx = (float)(anOffX + (double)theCell->x);
			double anOffY = (double)t * (double)theCell->offY;
			float fy = (float)(anOffY + (double)theCell->y);
			if (0.0f <= fx && 0.0f <= fy && (double)fx + 16.0 < 800.0 && 16.0 + (double)fy < 600.0)
			{
				Image* img = theCell->image;
				int dy = (int)fy;
				int dx = (int)fx;
				Alpha16MaskFade(gScreenBits, gScreenPitch, &gBackBuffer.fmt, dx, dy, img->bits, img->pitch, sx, sy,
					img->mask->bits, img->mask->pitch, 0, 0, 0x10, 0x10, theCell->alpha);
				Dirty_AddClippedShown(&gDirty, dx, dy, 0x10, 0x10, kScreenClip);
				return;
			}
		}
	}
	else if (aState != 6)
	{
		Image* aFill = gLevels.current->fillImage;
		int fsx = aX % aFill->w;
		int fsy = aY % aFill->h;
		uint8_t* aBack = gBackBuffer.bits;
		int aPitch = gBackBuffer.pitch;
		const PixelFormat* aFmt = &gBackBuffer.fmt;
		int aCol = theCell->col;
		int aRow = theCell->row;
		if (theCell->kind == 3 || theCell->kind == 2)
		{
			// the filled neighbours' halves next to this cell
			if (At(aCol - 1, aRow)->kind == 4)
				Alpha16Mask(aBack, aPitch, aFmt, aX, aY, aFill->bits, aFill->pitch, fsx, fsy, aFill->mask->bits,
					aFill->mask->pitch, 0, 0, 8, 0x10);
			if (At(aCol + 1, aRow)->kind == 4)
				Alpha16Mask(aBack, aPitch, aFmt, (int)((double)theCell->x + 8.0), (int)theCell->y, aFill->bits,
					aFill->pitch, fsx + 8, fsy, aFill->mask->bits, aFill->mask->pitch, 0, 0, 8, 0x10);
			if (At(aCol, aRow + 1)->kind == 4)
				Alpha16Mask(aBack, aPitch, aFmt, (int)theCell->x, (int)((double)theCell->y + 8.0), aFill->bits,
					aFill->pitch, fsx, fsy + 8, aFill->mask->bits, aFill->mask->pitch, 0, 0, 0x10, 8);
			if (At(aCol, aRow - 1)->kind == 4)
				Alpha16Mask(aBack, aPitch, aFmt, (int)theCell->x, (int)theCell->y, aFill->bits, aFill->pitch, fsx, fsy,
					aFill->mask->bits, aFill->mask->pitch, 0, 0, 0x10, 8);
			// the beam neighbours' halves
			Cell* aNext = At(aCol - 1, aRow);
			if (aNext->kind == 2)
			{
				Image* img = aNext->image;
				Alpha16Mask(aBack, aPitch, aFmt, (int)theCell->x, (int)theCell->y, img->bits, img->pitch, 0, 0,
					img->mask->bits, img->mask->pitch, 0, 0, 8, 0x10);
			}
			aNext = At(aCol + 1, aRow);
			if (aNext->kind == 2)
			{
				Image* img = aNext->image;
				Alpha16Mask(aBack, aPitch, aFmt, (int)((double)theCell->x + 8.0), (int)theCell->y, img->bits,
					img->pitch, 8, 0, img->mask->bits, img->mask->pitch, 0, 0, 8, 0x10);
			}
			aNext = At(aCol, aRow + 1);
			if (aNext->kind == 2)
			{
				Image* img = aNext->image;
				Alpha16Mask(aBack, aPitch, aFmt, (int)theCell->x, (int)((double)theCell->y + 8.0), img->bits,
					img->pitch, 0, 8, img->mask->bits, img->mask->pitch, 0, 0, 0x10, 8);
			}
			aNext = At(aCol, aRow - 1);
			if (aNext->kind == 2)
			{
				Image* img = aNext->image;
				Alpha16Mask(aBack, aPitch, aFmt, (int)theCell->x, (int)theCell->y, img->bits, img->pitch, 0, 0,
					img->mask->bits, img->mask->pitch, 0, 0, 0x10, 8);
			}
		}
		if (theCell->kind == 2 || theCell->kind == 3)
		{
			// its shadow, once, on filled cells to the right and below
			Cell* aRight = At(aCol + 1, aRow);
			if (aRight->kind == 4 && aRight->shadowRight == 0)
			{
				Image* aMask = theCell->image->mask;
				Fill16MaskAlpha(aBack, aPitch, aFmt, (int)((double)theCell->x + 5.0), (int)theCell->y, sx, sy,
					aMask->bits, aMask->pitch, 0, 0, 0x10, 0x10, 0, 0x5a);
				At(aCol + 1, aRow)->shadowRight = 1;
			}
			Cell* aBelow = At(aCol, aRow + 1);
			if (aBelow->kind == 4 && aBelow->shadowBelow == 0)
			{
				Image* aMask = theCell->image->mask;
				Fill16MaskAlpha(aBack, aPitch, aFmt, (int)theCell->x, (int)((double)theCell->y + 5.0), sx, sy,
					aMask->bits, aMask->pitch, 0, 0, 0x10, 0x10, 0, 0x5a);
				At(aCol, aRow + 1)->shadowBelow = 1;
			}
		}
		Image* img = theCell->image;
		Alpha16Mask(aBack, aPitch, aFmt, (int)theCell->x, (int)theCell->y, img->bits, img->pitch, sx, sy,
			img->mask->bits, img->mask->pitch, 0, 0, 0x10, 0x10);
		Dirty_AddClippedShown(&gDirty, (int)theCell->x, (int)theCell->y, 0x18, 0x18, kScreenClip);
	}
}

uint32_t Cell_SameNeighbours(Cell* theCell)
{
	return Cell_Touches(theCell, theCell->kind);
}

uint32_t Cell_Touches(Cell* theCell, int theKind)
{
	int aCol = theCell->col;
	int aRow = theCell->row;
	uint32_t aMask = 0;
	for (int r = aRow - 1; r <= aRow + 1; r++)
	{
		for (int k = aCol - 1; k <= aCol + 1; k++)
		{
			if (-1 < k && k < gLevels.current->cols && -1 < r && r <= gLevels.current->rows &&
				At(k, r)->kind == theKind)
				aMask = aMask | 1u << (((r - aRow) * 3 + (k - aCol) + 4) & 0x1f);
		}
	}
	return aMask;
}

void Cell_Flood(Cell* theCell)
{
	int aCol = theCell->col;
	theCell->unmarked = 0;
	int aRow = theCell->row;
	gLevels.current->unk3c0 = gLevels.current->unk3c0 + 1;
	for (int r = aRow - 1; r <= aRow + 1; r++)
	{
		for (int k = aCol - 1; k <= aCol + 1; k++)
		{
			if ((k == aCol || r == aRow) && -1 < k && k < gLevels.current->cols && -1 < r &&
				r <= gLevels.current->rows)
			{
				Cell* aNext = At(k, r);
				if (aNext->kind == 0 && aNext->unmarked == 1)
					Cell_Flood(aNext);
			}
		}
	}
}

// Edges by their edge neighbours (bits 1 up, 3 left, 5 right, 7 down): the level's patterns along a side, its
// corners; beams by theirs
int Cell_PickImage(Cell* theCell)
{
	uint32_t m = Cell_SameNeighbours(theCell);
	Level* aLevel = gLevels.current;
	int aKind = theCell->kind;
	if (aKind == 0)
	{
		theCell->image = aLevel->empty;
		return 1;
	}
	if (aKind == 3)
	{
		uint32_t aLeft = m & 8;
		if (aLeft != 0 && (m & 0x20) != 0)
		{
			theCell->image = Animation_Frame(aLevel->hTiles, aLevel->hPattern[theCell->col % aLevel->hPatternCount]);
			return 1;
		}
		uint32_t anUp = m & 2;
		if (anUp != 0 && (int8_t)m < 0)
		{
			theCell->image = Animation_Frame(aLevel->vTiles, aLevel->vPattern[theCell->row % aLevel->vPatternCount]);
			return 1;
		}
		if ((m & 0x20) != 0 && (int8_t)m < 0)
		{
			theCell->image = Animation_Frame(aLevel->cornerTiles, theCell->col % aLevel->cornerTileCount);
			return 1;
		}
		if (aLeft != 0 && (int8_t)m < 0)
		{
			theCell->image = Animation_Frame(aLevel->cornerTiles, 0);
			return 1;
		}
		if ((m & 0x20) != 0 && anUp != 0)
		{
			theCell->image = Animation_Frame(aLevel->cornerTiles, 0);
			return 1;
		}
		if (aLeft != 0 && anUp != 0)
		{
			theCell->image = Animation_Frame(aLevel->cornerTiles, 0);
			return 1;
		}
	}
	else if (aKind == 1 || aKind == 2)
	{
		uint32_t aDown = m & 0x80;
		if (aDown == 0 || (m & 8) == 0)
		{
			uint32_t anUp = m & 2;
			if (anUp != 0 && (m & 8) != 0)
			{
				theCell->image = aLevel->beamMiddle;
				return 1;
			}
			uint32_t aRight = m & 0x20;
			if (aRight != 0 && anUp != 0)
			{
				theCell->image = aLevel->beamMiddle;
				return 1;
			}
			if (aRight == 0 || aDown == 0)
			{
				if (anUp == 0 && aDown == 0)
				{
					if ((m & 8) == 0 && aRight == 0)
						return 1;
					theCell->image = aLevel->beamH;
					return 1;
				}
				theCell->image = aLevel->beamV;
				return 1;
			}
		}
		theCell->image = aLevel->beamMiddle;
		return 1;
	}
	return 1;
}

// It joins the level's moving cells and slides in from a random direction, 18 pixels out
void Cell_StartCapture(Cell* theCell, Image* theImage, int theFrom, int theTo)
{
	List_Append(&gLevels.current->fills, theCell);
	theCell->step = theCell->steps / 10;
	theCell->fromKind = theFrom;
	theCell->toKind = theTo;
	theCell->kind = theTo;
	theCell->state = 5;
	theCell->image = theImage;
	int anAngle = RandRange(0, 0x167);
	theCell->offX = (int)((double)CosDeg(anAngle) * 18.0);
	theCell->offY = (int)((double)SinDeg(anAngle) * 18.0);
	Cell_PickImage(theCell);
}

// 0x404400
void Cell_StartFill(Cell* theCell)
{
	List_Append(&gLevels.current->fills, theCell);
	int anAngle = RandRange(0, 0x167);
	if (gLevels.current->filledCells == 0)
	{
		theCell->offX = RandRange(-40, 40);
		theCell->offY = RandRange(-40, 40);
	}
	else
	{
		// (both offsets are the cosine, as in the original: the cells fly out along a diagonal; the first cell's
		// random offsets are flown in instead, see Cell_Redraw's state 7)
		theCell->offX = (int)((double)CosDeg(anAngle) * 38.0);
		theCell->offY = (int)((double)CosDeg(anAngle) * 38.0);
	}
	theCell->step = 0;
	theCell->state = 7;
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
			theCell->kind = 5;
			goto place;
		default:
			Level_AddKey(gLevels.current, theCode, theCol, theRow);
			if (At(theCol - 1, theRow)->kind == 3 || At(theCol, theRow - 1)->kind == 3)
				theCode = 'X';
			else
				theCode = ':';
			break;
		case '$':
			theCell->kind = 0;
			Cell_StartCapture(theCell, gLevels.current->fillImage, 0, 4);
			goto place;
		case ':':
			theCell->kind = 0;
			gLevels.current->totalCells = gLevels.current->totalCells + 1;
			theCell->image = gLevels.current->empty;
			goto place;
		case 'X':
			theCell->image = Bank_FindImage2(&gArtBank, "cell_base");
			theCell->kind = 3;
			theCell->alpha = 0;
			goto place;
		}
	}
place:
	theCell->col = theCol;
	theCell->row = theRow;
	theCell->x = (float)(theCol << 4);
	theCell->y = (float)(theRow << 4);
	theCell->firstParticle = Bank_IndexOf(&gArtBank, "particle_0");
	theCell->lastParticle = Bank_IndexOf(&gArtBank, "particle_0");
}

// Right, else left; then down, else up
void Cell_FillNext(Cell* theCell)
{
	Cell* aNext = At(theCell->col + 1, theCell->row);
	if (aNext->state == 6 || (aNext = At(theCell->col - 1, theCell->row), aNext->state == 6))
		Cell_StartFill(aNext);
	Cell* aCell = At(theCell->col, theCell->row);
	if (aCell[1].state == 6)
	{
		Cell_StartFill(&aCell[1]);
		return;
	}
	if (aCell[-1].state == 6)
	{
		Cell_StartFill(&aCell[-1]);
		return;
	}
}

// To the right, left, down and up: the marked neighbours of the kind the capture takes
void Cell_SpreadCapture(Cell* theCell)
{
	Cell* aNext = At(theCell->col + 1, theCell->row);
	if (aNext->kind == theCell->fromKind && aNext->unmarked == 0 && aNext->state != 5)
		Cell_StartCapture(aNext, theCell->image, theCell->fromKind, theCell->toKind);
	aNext = At(theCell->col - 1, theCell->row);
	if (aNext->kind == theCell->fromKind && aNext->unmarked == 0 && aNext->state != 5)
		Cell_StartCapture(aNext, theCell->image, theCell->fromKind, theCell->toKind);
	Cell* aCell = At(theCell->col, theCell->row);
	if (aCell[1].kind == theCell->fromKind && aCell[1].unmarked == 0 && aCell[1].state != 5)
		Cell_StartCapture(&aCell[1], theCell->image, theCell->fromKind, theCell->toKind);
	aCell = At(theCell->col, theCell->row);
	if (aCell[-1].kind == theCell->fromKind && aCell[-1].unmarked == 0 && aCell[-1].state != 5)
		Cell_StartCapture(&aCell[-1], theCell->image, theCell->fromKind, theCell->toKind);
}

// A third of the way it spreads; three quarters of the way it may puff; at the end it scores, lands in the back
// buffer and redraws the edges and beams around it
int Cell_UpdateCapture(Cell* theCell)
{
	int aSteps = theCell->steps;
	theCell->step = theCell->step + 1;
	int aStep = theCell->step;
	if (aStep <= aSteps)
	{
		if (aStep != aSteps / 3)
		{
			if (aStep == (aSteps * 3) / 4 && RandRange(1, 2) == 2)
			{
				ParticleSystem_New();
				int aLifetime = RandRange(400, 500);
				int aLast = theCell->lastParticle;
				int aFirst = theCell->firstParticle;
				float aVy = (float)((double)RandRange(-160, -50) / 1000.0);
				float aVx = (float)((double)RandRange(-50, 50) / 1000.0);
				ParticleSystem_Add(gLevels.current->systems.current, theCell->x + 5.0f, theCell->y + 5.0f, aVx, aVy,
					aFirst, aLast, 0, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x21c, 10, aLifetime, 1.0f, 0.0006f, 5);
			}
			return 1;
		}
		Cell_SpreadCapture(theCell);
		return 1;
	}
	gScore = gScore + gLevels.current->shooters.current->cellPoints;
	theCell->unmarked = 1;
	theCell->state = 3;
	gLevels.current->filledCells = gLevels.current->filledCells + 1;
	Cell_Redraw(theCell);
	int anAlpha = (theCell->step * 0x142) / theCell->steps;
	theCell->shadowRight = 0;
	theCell->shadowBelow = 0;
	if (0xff < anAlpha)
		anAlpha = 0xff;
	theCell->alpha = anAlpha;
	for (int dy = -1; dy < 2; dy++)
	{
		for (int dx = -1; dx < 2; dx++)
		{
			if ((dx == 0 && dy != 0) || (dx != 0 && dy == 0))
			{
				Cell* aNext = At(theCell->col + dx, theCell->row + dy);
				int aKind = aNext->kind;
				if ((aKind == 3 || aKind == 2) && (theCell->kind == 4 || aKind == 3))
					Cell_Redraw(aNext);
			}
		}
	}
	return 0;
}

int Cell_UpdateFill(Cell* theCell)
{
	int aResult = 1;
	if (theCell->kind != 5)
	{
		float t = (float)((double)theCell->step / (double)theCell->steps);
		if (gLevels.current->filledCells == 0)
			t = (float)(1.0 - (double)t);
		int anAlpha = (int)((double)t * 255.0);
		if (0xff < anAlpha)
			anAlpha = 0xff;
		theCell->alpha = 0xff - anAlpha;
	}
	if (theCell->step == 0)
	{
		// the background under it
		int aY = (int)theCell->y;
		int aX = (int)theCell->x;
		Image* aBack = gLevels.current->background;
		Copy16(gBackBuffer.bits, gBackBuffer.pitch, aX, aY, aBack->bits, aBack->pitch, aX, aY, 0x10, 0x10);
	}
	theCell->step = theCell->step + 1;
	if (theCell->steps < theCell->step)
	{
		theCell->state = 3;
		aResult = 0;
		if (gLevels.current->filledCells == 0)
		{
			if (theCell->kind != 5 && 0 < theCell->kind)
			{
				for (int i = 0; i < 3; i++)
				{
					ParticleSystem_New();
					int aLifetime = RandRange(400, 500);
					int aLast = theCell->lastParticle;
					int aFirst = theCell->firstParticle;
					float aVy = (float)((double)RandRange(-50, 50) / 500.0);
					float aVx = (float)((double)RandRange(-50, 50) / 500.0);
					float aY = (float)((double)theCell->y + 5.0 + (double)RandRange(-5, 5));
					float aX = (float)((double)theCell->x + 5.0 + (double)RandRange(-5, 5));
					ParticleSystem_Add(gLevels.current->systems.current, aX, aY, aVx, aVy, aFirst, aLast, 0, 0xff, 0xff,
						0xff, 0xff, 0xff, 0xff, 400, 10, aLifetime, 0.999f, 0.0002f, 5);
				}
			}
			Cell_Redraw(theCell);
			return 0;
		}
		theCell->kind = 5;
		float anOffX = (float)theCell->offX;
		int dx = (int)(((double)theCell->x + (double)anOffX) - (double)anOffX);
		float anOffY = (float)theCell->offY;
		int dy = (int)(((double)theCell->y + (double)anOffY) - (double)anOffY);
		if (-1 < dx && -1 < dy && dx + 0x10 < 800 && dy + 0x10 < 600)
		{
			Dirty_AddClippedShown(&gDirty, dx, dy, 0x10, 0x10, kScreenClip);
			return 0;
		}
	}
	else if (theCell->step == theCell->steps / 0x32)
		Cell_FillNext(theCell);
	return aResult;
}

int Fill_Update(Cell* theCell)
{
	if (theCell->state == 5)
		return Cell_UpdateCapture(theCell);
	if (theCell->state == 7)
		return Cell_UpdateFill(theCell);
	return 0;
}


// The background under a cell, back in the back buffer
static void Cell_Uncover(Cell* theCell)
{
	int aY = (int)theCell->y;
	int aX = (int)theCell->x;
	Image* aBack = gLevels.current->background;
	Copy16(gBackBuffer.bits, gBackBuffer.pitch, aX, aY, aBack->bits, aBack->pitch, aX, aY, 0x10, 0x10);
}

static bool InBoard(int theCol, int theRow)
{
	return -1 < theCol && theCol < gLevels.current->cols && -1 < theRow && theRow < gLevels.current->rows;
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
				if ((aCell->kind == 4 || aCell->kind == 2) && aCell->unmarked != 0)
				{
					aCell->kind = 0;
					aCell->state = 3;
					aCell->image = gLevels.current->empty;
					Cell_Uncover(aCell);
					Cell_Redraw(aCell);
					if (theRadius + -1 < std::abs(aCol - theCol) || theRadius + -1 < std::abs(aRow - theRow))
						aCell->kind = 2;
					else
						gLevels.current->filledCells = gLevels.current->filledCells + -1;
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
				if (aCell->kind == 2)
				{
					Cell_PickImage(aCell);
					Cell_Redraw(aCell);
				}
				Dirty_AddClippedShown(&gDirty, (int)aCell->x, (int)aCell->y, 0x10, 0x10, kScreenClip);
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
					float aY = (float)RandRange(-10, 10) + aCell->y;
					float aX = (float)RandRange(-10, 10) + aCell->x;
					ParticleSystem_Add(gLevels.current->systems.current, aX, aY, aVx, aVy, aFirst, aLast, 0, 0xff, 0xff,
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
					if (InBoard(aCol2, aRow2) && At(aCol2, aRow2)->kind == 2)
					{
						Cell* aCell = At(aCol2, aRow2);
						// the four sides' bits of Cell_Touches: left 8, right 0x20, up 2, down 0x80
						uint32_t aSides = Cell_Touches(aCell, 0);
						int aCount = (aSides & 8) != 0;
						if ((aSides & 0x20) != 0)
							aCount = aCount + 1;
						if ((aSides & 2) != 0)
							aCount = aCount + 1;
						if ((aSides & 0x80) != 0)
							aCount = aCount + 1;
						aSides = Cell_Touches(aCell, 3);
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
							aCell->kind = 0;
							aCell->image = gLevels.current->empty;
							Cell_Uncover(aCell);
							Cell_Redraw(aCell);
							gLevels.current->filledCells = gLevels.current->filledCells + -1;
							aChanged = true;
						}
					}
				}
			}
		} while (aChanged);
	}
}

} // namespace wb
