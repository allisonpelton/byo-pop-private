// The player's ship (0x41b340..0x41e800): it comes in, follows the mouse, fires a pair of beams along its row or
// column and, when both have stopped, fills the smaller side; the sweeper and the invincibubble are its extras.
// The weapons that orbit it (0x41a910..0x41b290) go with the power-ups.

#include "WBLevel.h"
#include "R64Sys.h"

#include <cmath>
#include <cstring>
#include <new>

namespace wb
{

namespace
{

uint8_t gVisited[50][37];          // 0x6e5e60  the open cells Region_Count has been to

// 0x41c440: the level's weapons orbiting the ship
int Weapons_Orbiting()
{
	int aCount = 0;
	List_Each(&gLevels.current->weapons, [&](Weapon* aWeapon) {
		if (aWeapon->state == 3)
			aCount = aCount + 1;
	});
	return aCount;
}

// 0x41b7f0: the beams turn between across and up and down
void Shooter_Turn(Shooter* theShooter)
{
	if (theShooter->beamAcross == 0)
	{
		theShooter->anim = theShooter->horizAnim;
		theShooter->beamAcross = 1;
		return;
	}
	theShooter->anim = theShooter->vertAnim;
	theShooter->beamAcross = 0;
}

// 0x41d370 (state 4): four sparkles circle in towards the middle of the screen while the ship comes in, and a burst
// of sparkles marks its arrival
void Shooter_Appear(Shooter* theShooter)
{
	int aDummy[2];
	if (gDemo == 0)
		Sys_SetMousePos(400, 300);
	theShooter->x = 400.0f;
	theShooter->y = 300.0f;
	if (theShooter->age == 1.0f)
		Level_ShowStart(gLevels.current);
	aDummy[1] = 0;
	if (theShooter->appearTime <= theShooter->age)
	{
		if (theShooter->appearTime == theShooter->age)
		{
			theShooter->drawKind = 4;
			int n = 0x78;
			do
			{
				int anAngle = RandRange(0, 0x167);
				int aSpeed = RandRange(0, 100);
				float aStep = (float)((double)aSpeed / 360.0);
				float aVx = (float)((double)CosDeg(anAngle) * (double)aStep);
				float aVy = (float)((double)SinDeg(anAngle) * (double)aStep);
				ParticleSystem_New();
				int aLifetime = RandRange(600, 0x4b0);
				ParticleSystem_Add(gLevels.current->systems.current, theShooter->x, theShooter->y, aVx, aVy, 0, 0, 0,
					0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0, aLifetime, 1.0f, 0.0f, 4);
				Particle* aParticle = gLevels.current->systems.current->particles.current;
				aParticle->anim = Animation_Find("sparkles3", &aParticle->frameCount, &aDummy[1]);
				aParticle->tint = RandRange(0, 7);
				n = n - 1;
			} while (n != 0);
			theShooter->alpha = 0xff;
			theShooter->state = 3;
			while (List_RemoveCurrent(&theShooter->children))
				;
		}
		return;
	}
	if (theShooter->age == 1.0f)
	{
		List<Obj>* aChildren = &theShooter->children;
		int n = 4;
		do
		{
			Panel_AppendItem(aChildren);
			aChildren->current->anim =
				Animation_Find("sparkles1", &aChildren->current->frameCount, &aChildren->current->frameTime);
			aChildren->current->midHandle = 1;
			aChildren->current->dirty = 1;
			aChildren->current->drawKind = 4;
			n = n - 1;
			aChildren->current->alpha = 0xff;
		} while (n != 0);
	}
	float aRatio = theShooter->age / theShooter->appearTime;
	theShooter->alpha = (int)aRatio;
	int aSpin = (int)((double)theShooter->age / 3.0);
	aSpin = aSpin % 0x168;
	if (theShooter->children.current != nullptr)
	{
		Obj* aChild = theShooter->children.head;
		Obj* aCurrent;
		double aRadius = (double)(float)(1.0 - (double)aRatio * (double)aRatio) * 160.0;
		while (true)
		{
			theShooter->children.current = aChild;
			theShooter->children.current->x =
				(float)((double)CosDeg(aSpin % 0x168) * aRadius + (double)theShooter->x);
			theShooter->children.current->y =
				(float)((double)SinDeg(aSpin % 0x168) * aRadius + (double)theShooter->y);
			if (RandRange(1, 0x10) == 5)
			{
				ParticleSystem_New();
				float aY = (float)RandRange(-4, 4) + theShooter->children.current->y;
				float aX = (float)RandRange(-4, 4) + theShooter->children.current->x;
				ParticleSystem_Add(gLevels.current->systems.current, aX, aY, 0.0f, 0.0f, 0, 0, 0, 0xff, 0xff, 0xff, 0xff,
					0xff, 0xff, 0x21c, 10, 400, 0.999f, 0.0002f, 5);
				Particle* aParticle = gLevels.current->systems.current->particles.current;
				aParticle->anim = Animation_Find("sparkles3", &aParticle->frameCount, &aDummy[0]);
			}
			aCurrent = theShooter->children.current;
			aSpin = aSpin + 0x5a;
			if (aCurrent == nullptr)
				return;
			if (aCurrent->next == nullptr)
				break;
			aChild = aCurrent->next;
		}
		if (aCurrent != nullptr)
			theShooter->children.current = theShooter->children.head;
	}
}

// 0x41b830: starts filling the open cells of theCell's row (beams across) or column (beams up and down) between
// the beams' ends, on the side theCell is
void Shooter_Fill(Shooter* theShooter, Cell* theCell)
{
	// RandMod(1) is always 0 (the level's one fill image), but it takes a number from the generator
	if (theShooter->beamAcross == 0)
	{
		int aTail = (int)theShooter->beams.tail->y / 16;
		int aHead = (int)theShooter->beams.head->y / 16;
		int aLo = aHead < aTail ? aHead : aTail;
		int aHi = aHead > aTail ? aHead : aTail;
		for (int i = theCell->row; i <= aHi; i++)
		{
			Cell* aCell = &gLevels.current->columns[theCell->col][i];
			if (aCell->kind == 0 && aCell->unmarked != 0)
			{
				int anImage = (int)RandMod(1);
				Cell_StartCapture(aCell, (&gLevels.current->fillImage)[anImage], 0, 4);
				Cell_Flood(aCell);
			}
		}
		for (int i = theCell->row; i >= aLo; i--)
		{
			Cell* aCell = &gLevels.current->columns[theCell->col][i];
			if (aCell->kind == 0 && aCell->unmarked != 0)
			{
				int anImage = (int)RandMod(1);
				Cell_StartCapture(aCell, (&gLevels.current->fillImage)[anImage], 0, 4);
				Cell_Flood(aCell);
			}
		}
		return;
	}
	int aTail = (int)theShooter->beams.tail->x / 16;
	int aHead = (int)theShooter->beams.head->x / 16;
	int aLo = aHead < aTail ? aHead : aTail;
	int aHi = aHead > aTail ? aHead : aTail;
	for (int i = theCell->col; i <= aHi; i++)
	{
		Cell* aCell = &gLevels.current->columns[i][theCell->row];
		if (aCell->kind == 0 && aCell->unmarked != 0)
		{
			int anImage = (int)RandMod(1);
			Cell_StartCapture(aCell, (&gLevels.current->fillImage)[anImage], 0, 4);
			Cell_Flood(aCell);
		}
	}
	for (int i = theCell->col; i >= aLo; i--)
	{
		Cell* aCell = &gLevels.current->columns[i][theCell->row];
		if (aCell->kind == 0 && aCell->unmarked != 0)
		{
			int anImage = (int)RandMod(1);
			Cell_StartCapture(aCell, (&gLevels.current->fillImage)[anImage], 0, 4);
			Cell_Flood(aCell);
		}
	}
}

// 0x41bb40: the open span about the ship along its row (theLeft to theRight) and its column (theTop to theBottom):
// each end walks 16 pixels a step to the first cell not open, then back a pixel a step onto an open one. All four
// are 0 when the ship is within 16 pixels of the screen's edge; the demo leaves them as they are. Every walk's
// guard checks the left end's column and the top end's row, as the original's do.
void Shooter_Extents(Shooter* theShooter, float* theLeft, float* theTop, float* theRight, float* theBottom)
{
	if (gDemo != 0)
		return;
	if (16.0f < theShooter->x && theShooter->x < 784.0f && 16.0f < theShooter->y && theShooter->y < 584.0f)
	{
		float aLeft = 0.0f <= theShooter->x ? theShooter->x : 0.0f;
		float aRight = theShooter->x < 799.0f ? theShooter->x : 799.0f;
		float aTop = theShooter->y < 0.0f ? 0.0f : theShooter->y;
		float aBottom = theShooter->y <= 599.0 ? theShooter->y : 599.0f;
		Cell** aColumns = gLevels.current->columns;
		int aMoved = 0;
		while ((int)aLeft / 16 < 50 && (int)aTop / 16 < 37 && aColumns[(int)aLeft / 16][(int)aTop / 16].kind == 0)
		{
			aLeft = aLeft - 16.0f;
			aMoved = 1;
		}
		while ((int)aLeft / 16 < 50 && (int)aTop / 16 < 37 && aMoved != 0 &&
			aColumns[(int)aLeft / 16][(int)aTop / 16].kind != 0)
			aLeft = aLeft + 1.0f;
		aMoved = 0;
		while ((int)aLeft / 16 < 50 && (int)aTop / 16 < 37 && aColumns[(int)aRight / 16][(int)aTop / 16].kind == 0)
		{
			aRight = aRight + 16.0f;
			aMoved = 1;
		}
		while ((int)aLeft / 16 < 50 && (int)aTop / 16 < 37 && aMoved != 0 &&
			aColumns[(int)aRight / 16][(int)aTop / 16].kind != 0)
			aRight = aRight - 1.0f;
		aMoved = 0;
		while ((int)aLeft / 16 < 50 && (int)aTop / 16 < 37 &&
			aColumns[(int)theShooter->x / 16][(int)aTop / 16].kind == 0)
		{
			aTop = aTop - 16.0f;
			aMoved = 1;
		}
		while ((int)aLeft / 16 < 50 && (int)aTop / 16 < 37 && aMoved != 0 &&
			aColumns[(int)theShooter->x / 16][(int)aTop / 16].kind != 0)
			aTop = aTop + 1.0f;
		aMoved = 0;
		while ((int)aLeft / 16 < 50 && (int)aTop / 16 < 37 &&
			aColumns[(int)theShooter->x / 16][(int)aBottom / 16].kind == 0)
		{
			aBottom = aBottom + 16.0f;
			aMoved = 1;
		}
		while ((int)aLeft / 16 < 50 && (int)aTop / 16 < 37 && aMoved != 0 &&
			aColumns[(int)theShooter->x / 16][(int)aBottom / 16].kind != 0)
			aBottom = aBottom - 1.0f;
		*theLeft = aLeft;
		*theTop = aTop;
		*theRight = aRight;
		*theBottom = aBottom;
		return;
	}
	*theBottom = 0.0f;
	*theRight = 0.0f;
	*theTop = 0.0f;
	*theLeft = 0.0f;
}

// 0x41c140: the enemies whose circles (a third of their images' heights) overlap the ship's
int Shooter_EnemiesNear(Shooter* theShooter)
{
	int aCount = 0;
	List_Each(&gLevels.current->enemies, [&](Enemy* anEnemy) {
		if (CircleOverlap(theShooter->x, theShooter->y, (float)(theShooter->image->h / 3), anEnemy->x, anEnemy->y,
			(float)(anEnemy->image->h / 3)) != 0.0)
			aCount = aCount + 1;
	});
	return aCount;
}

// 0x41c230: counts the open region about (theCol, theRow) not visited yet, its cells, the enemies in them and the
// sum of their x (rounded)
int Region_Count(int theCol, int theRow, int* theCells, int* theEnemies, int* theSumX)
{
	if (gLevels.current->columns[theCol][theRow].kind != 0 || gVisited[theCol][theRow] != 0)
		return 0;
	List_Each(&gLevels.current->enemies, [&](Enemy* anEnemy) {
		if (theCol == (int)anEnemy->x / 16 && theRow == (int)anEnemy->y / 16)
		{
			*theEnemies = *theEnemies + 1;
			*theSumX = *theSumX + (int)anEnemy->x;
		}
	});
	*theCells = *theCells + 1;
	gVisited[theCol][theRow] = 1;
	for (int c = theCol - 1; c <= theCol + 1; c++)
	{
		for (int r = theRow - 1; r <= theRow + 1; r++)
		{
			if ((c == theCol || r == theRow) && 0 <= c && c < gLevels.current->cols && 0 <= r &&
				r < gLevels.current->rows && gLevels.current->columns[c][r].kind == 0 && gVisited[c][r] == 0)
				Region_Count(c, r, theCells, theEnemies, theSumX);
		}
	}
	return 0;
}

// 0x41d0c0: the open cells of the side the line of theCell is on (between the beams' ends), with the enemies in
// them and the sum of their x
int Shooter_Measure(Shooter* theShooter, Cell* theCell, int* theEnemies, int* theSumX)
{
	int aCells = 0;
	std::memset(gVisited, 0, sizeof(gVisited));
	*theEnemies = 0;
	*theSumX = 0;
	if (theShooter->beamAcross == 0)
	{
		int aTail = (int)theShooter->beams.tail->y / 16;
		int aHead = (int)theShooter->beams.head->y / 16;
		int aLo = aHead < aTail ? aHead : aTail;
		int aHi = aHead > aTail ? aHead : aTail;
		for (int i = theCell->row; i <= aHi; i++)
		{
			if (gVisited[theCell->col][i] == 0)
				Region_Count(theCell->col, i, &aCells, theEnemies, theSumX);
		}
		for (int i = theCell->row; aLo <= i; i--)
		{
			if (gVisited[theCell->col][i] == 0)
				Region_Count(theCell->col, i, &aCells, theEnemies, theSumX);
		}
		return aCells;
	}
	int aTail = (int)theShooter->beams.tail->x / 16;
	int aHead = (int)theShooter->beams.head->x / 16;
	int aLo = aHead < aTail ? aHead : aTail;
	int aHi = aHead > aTail ? aHead : aTail;
	for (int i = theCell->col; i <= aHi; i++)
	{
		if (gVisited[i][theCell->row] == 0)
			Region_Count(i, theCell->row, &aCells, theEnemies, theSumX);
	}
	for (int i = theCell->col; aLo <= i; i--)
	{
		if (gVisited[i][theCell->row] == 0)
			Region_Count(i, theCell->row, &aCells, theEnemies, theSumX);
	}
	return aCells;
}

// One of the sweeper's sparks, 5 pixels either way about (theX, theY)
void Sweeper_Spark(Shooter* theShooter, double theX, double theY)
{
	ParticleSystem_New();
	int aParticle = theShooter->particle;
	int aLifetime = RandRange(500, 600);
	float aVy = (float)((double)RandRange(-0x32, 0x32) / 400.0);
	float aVx = (float)((double)RandRange(-0x32, 0x32) / 400.0);
	float aY = (float)((double)RandRange(-5, 5) + theY);
	float aX = (float)((double)RandRange(-5, 5) + theX);
	ParticleSystem_Add(gLevels.current->systems.current, aX, aY, aVx, aVy, aParticle, aParticle, 0, 0xff, 0xff, 0xff,
		0x80, 0x80, 0xff, 400, 10, aLifetime, 0.999f, 0.0004f, 5);
}

// 0x41c4a0: the sweeper rides the ends of the open span about the ship (16 pixels further out); an enemy in the
// span that crossed the ship's line since the last update goes back to its side of it, turned round
void Sweeper_Push(Shooter* theShooter)
{
	if (theShooter->x < 1.0f || 799.0f <= theShooter->x || theShooter->y < 1.0f || 591.0f < theShooter->y ||
		theShooter->state != 3 || Weapons_Orbiting() != 0)
		return;
	Obj_Update(&theShooter->sweeper);
	float aLeft;
	float aTop;
	float aRight;
	float aBottom;
	Shooter_Extents(theShooter, &aLeft, &aTop, &aRight, &aBottom);
	aLeft = aLeft - 16.0f;
	aTop = aTop - 16.0f;
	aRight = aRight + 16.0f;
	aBottom = aBottom + 16.0f;
	List_Each(&gLevels.current->enemies, [&](Enemy* anEnemy) {
		int aCol = (int)anEnemy->x / 16;
		int aRow = (int)anEnemy->y / 16;
		int aKind = gLevels.current->columns[aCol][aRow].kind;
		if (theShooter->beamAcross == 1)
		{
			if (aLeft < anEnemy->x && anEnemy->x < aRight)
			{
				float aLine = theShooter->y <= 568.0 ? theShooter->y : 568.0f;
				if (anEnemy->oldY < theShooter->oldY && aLine < anEnemy->y)
				{
					anEnemy->vy = -anEnemy->vy;
					while (aLine < anEnemy->y && gLevels.current->columns[aCol][(int)((double)aLine - 1.0 +
						(double)anEnemy->vy) / 16].kind == aKind)
						anEnemy->y = anEnemy->y - 1.0f;
				}
				else if (theShooter->oldY < anEnemy->oldY && anEnemy->y < aLine)
				{
					anEnemy->vy = -anEnemy->vy;
					while (anEnemy->y < aLine && gLevels.current->columns[aCol][(int)((double)aLine + 1.0 +
						(double)anEnemy->vy) / 16].kind == aKind)
						anEnemy->y = anEnemy->y + 1.0f;
				}
			}
		}
		else if (aTop < anEnemy->y && anEnemy->y < aBottom)
		{
			float aLine = theShooter->x <= 776.0 ? theShooter->x : 776.0f;
			if (anEnemy->oldX < theShooter->oldX && aLine < anEnemy->x)
			{
				anEnemy->vx = -anEnemy->vx;
				while (aLine < anEnemy->x && gLevels.current->columns[(int)((double)aLine - 1.0 +
					(double)anEnemy->vx) / 16][aRow].kind == aKind)
					anEnemy->x = anEnemy->x - 1.0f;
			}
			else if (theShooter->oldX < anEnemy->oldX && anEnemy->x < aLine)
			{
				anEnemy->vx = -anEnemy->vx;
				while (anEnemy->x < aLine && gLevels.current->columns[(int)((double)aLine + 1.0 +
					(double)anEnemy->vx) / 16][aRow].kind == aKind)
					anEnemy->x = anEnemy->x + 1.0f;
			}
		}
	});
	if (theShooter->beamAcross == 1)
	{
		if (RandRange(1, 0xf) == 1)
		{
			Sweeper_Spark(theShooter, (double)aLeft + 8.0, (double)theShooter->y);
			Sweeper_Spark(theShooter, (double)aRight - 8.0, (double)theShooter->y);
		}
	}
	else if (RandRange(1, 0xf) == 1)
	{
		Sweeper_Spark(theShooter, (double)theShooter->x, (double)aTop + 8.0);
		Sweeper_Spark(theShooter, (double)theShooter->x, (double)aBottom - 8.0);
	}
}

// 0x41cd90: the aiming guides along the open span, 11 lines across the beams' path, fainter at the edges
void Shooter_Guides(Shooter* theShooter)
{
	if (gDemo != 0)
		return;
	int anAlpha = 5;
	if ((theShooter->flags & 4) != 0)
		anAlpha = 0x1e;
	if (theShooter->state != 3 || Weapons_Orbiting() != 0)
		return;
	if (theShooter->beamAcross == 1)
	{
		int aLo = (int)((double)theShooter->y - 5.0);
		int aHi = (int)((double)theShooter->y + 5.0);
		for (; aLo <= aHi; aLo++, aHi--, anAlpha += 10)
		{
			Draw_Line((int)theShooter->extLeft, aLo, (int)theShooter->extRight, aLo, 0xff, 0xff, 0xff, anAlpha);
			if (aLo != aHi)
				Draw_Line((int)theShooter->extLeft, aHi, (int)theShooter->extRight, aHi, 0xff, 0xff, 0xff, anAlpha);
		}
		Dirty_AddClippedShown(&gDirty, (int)theShooter->extLeft, (int)((double)theShooter->y - 5.0),
			(int)((double)theShooter->extRight - (double)theShooter->extLeft + 1.0), 0xb, kScreenClip);
		return;
	}
	int aLo = (int)((double)theShooter->x - 5.0);
	int aHi = (int)((double)theShooter->x + 5.0);
	for (; aLo <= aHi; aLo++, aHi--, anAlpha += 10)
	{
		Draw_Line(aLo, (int)theShooter->extTop, aLo, (int)theShooter->extBottom, 0xff, 0xff, 0xff, anAlpha);
		if (aLo != aHi)
			Draw_Line(aHi, (int)theShooter->extTop, aHi, (int)theShooter->extBottom, 0xff, 0xff, 0xff, anAlpha);
	}
	Dirty_AddClippedShown(&gDirty, (int)((double)theShooter->x - 5.0), (int)theShooter->extTop, 0xb,
		(int)((double)theShooter->extBottom - (double)theShooter->extTop + 1.0), kScreenClip);
}

// 0x41cfb0: the sweeper at both ends of the span
void Sweeper_Draw(Shooter* theShooter)
{
	if (theShooter->state != 3 || Weapons_Orbiting() != 0)
		return;
	Dirty_AddClippedShown(&gDirty, (int)((double)theShooter->extLeft - 2.0), (int)((double)theShooter->extTop - 2.0),
		(int)((double)theShooter->extRight - (double)theShooter->extLeft + 4.0),
		(int)((double)theShooter->extBottom - (double)theShooter->extTop + 4.0), kScreenClip);
	Obj* aSweeper = &theShooter->sweeper;
	if (theShooter->beamAcross == 1)
	{
		aSweeper->y = theShooter->y;
		aSweeper->x = theShooter->extLeft - 8.0f;
		Obj_Draw(aSweeper);
		aSweeper->x = theShooter->extRight + 8.0f;
		Obj_Draw(aSweeper);
		return;
	}
	aSweeper->x = theShooter->x;
	aSweeper->y = theShooter->extTop - 8.0f;
	Obj_Draw(aSweeper);
	aSweeper->y = theShooter->extBottom + 8.0f;
	Obj_Draw(aSweeper);
}

// 0x41d2e0
void Shooter_UpdateExtras(Shooter* theShooter)
{
	if (theShooter->state == 3 && (theShooter->flags & 4) != 0)
		Sweeper_Push(theShooter);
	if ((theShooter->flags & 0x10) != 0)
	{
		theShooter->bubble.x = theShooter->x;
		theShooter->bubble.drawKind = 1;
		theShooter->bubble.dirty = 1;
		theShooter->bubble.y = theShooter->y;
		theShooter->bubble.midHandle = 1;
		Obj_Update(&theShooter->bubble);
	}
}

// 0x41d340
void Shooter_DrawExtras(Shooter* theShooter)
{
	if ((theShooter->flags & 4) != 0)
		Sweeper_Draw(theShooter);
	if ((theShooter->flags & 0x10) != 0)
		Obj_Draw(&theShooter->bubble);
}

// 0x41d7a0 (state 6): once the explosion has played, the next life comes in, or the game is over
void Shooter_Dying(Shooter* theShooter)
{
	while (List_RemoveCurrent(&theShooter->beams))
		;
	theShooter->flags = 0;
	theShooter->beamSpeed = (float)((double)gBeamSpeed - (double)gDifficulty * (double)0.05f);
	if ((double)(theShooter->frameCount * theShooter->frameTime) < (double)theShooter->age)
	{
		if (0 < gLives)
		{
			Animation* anAnim = Animation_Find(theShooter->vertName, &theShooter->frameCount, &theShooter->frameTime);
			theShooter->vertAnim = anAnim;
			theShooter->anim = anAnim;
			theShooter->wingsH.anim = Animation_Find("wings_horizontal", &theShooter->wingsH.frameCount,
				&theShooter->wingsH.frameTime);
			theShooter->wingsH.frame = 0;
			anAnim = Animation_Find("wings_vertical", &theShooter->wingsV.frameCount, &theShooter->wingsV.frameTime);
			theShooter->age = 0.0f;
			theShooter->wingsV.frame = 0;
			theShooter->frame = 0;
			theShooter->oneShot = 0;
			theShooter->beamAcross = 0;
			theShooter->depth = 0;
			theShooter->wingsV.anim = anAnim;
			theShooter->cellPoints = 1;
			theShooter->state = 4;
			SaveProfiles();
			return;
		}
		Level_GameOver(gLevels.current);
		SaveProfiles();
	}
}

// 0x41e070: a new beam before the current one, made current
void Beam_Insert(List<Beam>* theList)
{
	Beam* aBeam = (Beam*)::operator new(sizeof(Beam));
	List_InsertBefore(theList, Beam_Init(aBeam));
}

// 0x41db70 (state 5): the beams run on; when both have stopped, the cells under them become the beam's, and the
// smaller side (else the one with fewer enemies, ...) fills
void Shooter_Firing(Shooter* theShooter)
{
	int aGoing = 0;
	Sys_SetMousePos((int)theShooter->x, (int)theShooter->y);
	Shooter_UpdateExtras(theShooter);
	if (theShooter->beams.current != nullptr)
	{
		List_Each(&theShooter->beams, [&](Beam* aBeam) {
			if ((int)((double)theShooter->age - (double)theShooter->fireAge) % 0x42 == 0)
				Sound_Play(theShooter->drillSound);
			aGoing = aGoing | Beam_Update(theShooter->beams.current);
		});
		if (aGoing != 0)
			return;
	}
	Beam_DrawTo(theShooter->beams.head, gBackBuffer.bits, gBackBuffer.pitch);
	Beam_DrawTo(theShooter->beams.tail, gBackBuffer.bits, gBackBuffer.pitch);
	// up and down, the sides are the cells left and right of the ship's; across, above and below it
	Level* aLevel = gLevels.current;
	Cell* aSide1;
	Cell* aSide2;
	if (theShooter->beams.current->vx == 0.0f)
	{
		int aCol = (int)theShooter->x / 16;
		int aRow = (int)theShooter->y / 16;
		aSide1 = &aLevel->columns[aCol - 1][aRow];
		aSide2 = &aLevel->columns[aCol + 1][aRow];
		for (int i = (int)theShooter->beams.tail->y / 16 - 1; i <= (int)theShooter->beams.head->y / 16 + 1; i++)
		{
			Cell* aCell = &aLevel->columns[(int)theShooter->x / 16][i];
			if (aCell->kind != 3)
			{
				aCell->kind = 1;
				aCell->unmarked = 0;
			}
		}
		Cell_StartCapture(&aLevel->columns[(int)theShooter->x / 16][(int)theShooter->y / 16], aLevel->beamV, 1, 2);
	}
	else
	{
		Cell* aCell = &aLevel->columns[(int)theShooter->x / 16][(int)theShooter->y / 16];
		aSide1 = aCell - 1;
		aSide2 = aCell + 1;
		for (int i = (int)theShooter->beams.tail->x / 16 - 1; i <= (int)theShooter->beams.head->x / 16 + 1; i++)
		{
			Cell* aBeamCell = &aLevel->columns[i][(int)theShooter->y / 16];
			if (aBeamCell->kind != 3)
			{
				aBeamCell->kind = 1;
				aBeamCell->unmarked = 0;
			}
		}
		Cell_StartCapture(&aLevel->columns[(int)theShooter->x / 16][(int)theShooter->y / 16], aLevel->beamH, 1, 2);
	}
	int anEnemies1 = 0;
	int anEnemies2 = 0;
	int aSumX1;
	int aSumX2;
	int aCells1 = Shooter_Measure(theShooter, aSide1, &anEnemies1, &aSumX1);
	int aCells2 = Shooter_Measure(theShooter, aSide2, &anEnemies2, &aSumX2);
	if (aCells1 != 0 && aCells2 != 0)
	{
		// the sums are compared unsigned, as the original does
		if (gLevels.current->number > 2 && anEnemies1 == 0 && anEnemies2 == 0)
		{
			Shooter_Fill(theShooter, aSide1);
			Shooter_Fill(theShooter, aSide2);
		}
		else if (aCells1 < aCells2)
			Shooter_Fill(theShooter, aSide1);
		else if (aCells2 < aCells1)
			Shooter_Fill(theShooter, aSide2);
		else if (anEnemies2 < anEnemies1)
			Shooter_Fill(theShooter, aSide2);
		else if (anEnemies1 < anEnemies2)
			Shooter_Fill(theShooter, aSide1);
		else if ((unsigned)aSumX1 < (unsigned)aSumX2)
			Shooter_Fill(theShooter, aSide1);
		else if ((unsigned)aSumX2 < (unsigned)aSumX1)
			Shooter_Fill(theShooter, aSide2);
		else if (gLevels.current->number <= 2)
			Shooter_Fill(theShooter, aSide1);
		Sound_Play(gLevels.current->fillSound);
	}
	gLevels.current->lines = gLevels.current->lines + 1;
	Shooter_DropBeams(theShooter);
}

// 0x41e110: a click fires a beam each way from the middle of the cell under the ship, if it is open and not yet
// reached by a flood; with an enemy on the ship it buzzes instead; once the filled share is past the quota, a click does nothing
void Shooter_Fire(Shooter* theShooter)
{
	if (Weapons_Orbiting() != 0)
		return;
	Level* aLevel = gLevels.current;
	if (aLevel->clicked == 0)
		return;
	if (aLevel->fillQuota < (aLevel->filledCells * 100) / aLevel->totalCells || Shooter_EnemiesNear(theShooter) != 0)
	{
		aLevel = gLevels.current;
		if (aLevel->clicked != 0 && (aLevel->filledCells * 100) / aLevel->totalCells <= aLevel->fillQuota &&
			Shooter_EnemiesNear(theShooter) != 0)
		{
			// (the original works out a pan from x for a play call that takes none)
			Sound_Play(Sound_Find("audio\\buzzer.wav"));
		}
		return;
	}
	{
		int aRow = (int)theShooter->y / 16;
		if (aRow < gLevels.current->rows)
		{
			int aCol = (int)theShooter->x / 16;
			Cell* aCell = &gLevels.current->columns[aCol][aRow];
			if (aCell->kind == 0 && aCell->unmarked != 0)
			{
				theShooter->x = (float)(aCol * 0x10 + 8);
				List<Beam>* aBeams = &theShooter->beams;
				theShooter->y = (float)(aRow * 0x10 + 8);
				gLevels.current->shots = gLevels.current->shots + 1;
				theShooter->state = 5;
				gProfiles[gProfile].beamsFired = gProfiles[gProfile].beamsFired + 1;
				Beam_Insert(aBeams);
				aBeams->current->x = theShooter->x;
				aBeams->current->y = theShooter->y;
				Image* anImage;
				if (theShooter->beamAcross == 1)
				{
					Beam_Setup(aBeams->current, theShooter, theShooter->x, theShooter->y, -theShooter->beamSpeed, 0.0f);
					anImage = gLevels.current->beamH;
				}
				else
				{
					Beam_Setup(aBeams->current, theShooter, theShooter->x, theShooter->y, 0.0f, -theShooter->beamSpeed);
					anImage = gLevels.current->beamV;
				}
				aBeams->current->image = anImage;
				Beam_Insert(aBeams);
				aBeams->current->x = theShooter->x;
				aBeams->current->y = theShooter->y;
				if (theShooter->beamAcross == 1)
				{
					Beam_Setup(aBeams->current, theShooter, theShooter->x, theShooter->y, theShooter->beamSpeed, 0.0f);
					aBeams->current->image = gLevels.current->beamH;
					theShooter->anim = theShooter->horizShootAnim;
					theShooter->wingsH.anim = Animation_Find("wings_fire_horizontal", &theShooter->wingsH.frameCount,
						&theShooter->wingsH.frameTime);
					theShooter->wingsH.frame = 0;
				}
				else
				{
					Beam_Setup(aBeams->current, theShooter, theShooter->x, theShooter->y, 0.0f, theShooter->beamSpeed);
					aBeams->current->image = gLevels.current->beamV;
					theShooter->anim = theShooter->vertShootAnim;
					theShooter->wingsV.anim = Animation_Find("wings_fire_vertical", &theShooter->wingsV.frameCount,
						&theShooter->wingsV.frameTime);
					theShooter->wingsV.frame = 0;
				}
				if (List_Count(aBeams) < 3)
				{
					theShooter->fireAge = (int)theShooter->age;
					Sound_Play(theShooter->shootSound);
				}
			}
		}
	}
}

// 0x41e480 (state 3): the ship eases toward the mouse; it fires, and turns on a right click (or by itself, unless
// the player has turned it or the auto-turn option is off, toward the shorter way across the span)
void Shooter_Control(Shooter* theShooter)
{
	if (gAutoFire == 0)
		theShooter->noAutoTurn = 1;
	int aTurn = gLevels.current->rightClicked;
	if (gDemo == 0)
	{
		double aX = ((double)gMouseX + (double)theShooter->x) * 0.5;
		theShooter->x = (float)(799.0 < aX ? 799.0 : (aX < 0.0 ? 0.0 : aX));
		double aY = ((double)gMouseY + (double)theShooter->y) * 0.5;
		theShooter->y = (float)(599.0 < aY ? 599.0 : (aY < 0.0 ? 0.0 : aY));
	}
	else
	{
		theShooter->x = (float)RandRange(0, 799);
		theShooter->y = (float)RandRange(0, 599);
		RandRange(1, 1000);
	}
	Shooter_Fire(theShooter);
	Shooter_UpdateExtras(theShooter);
	Shooter_Extents(theShooter, &theShooter->extLeft, &theShooter->extTop, &theShooter->extRight,
		&theShooter->extBottom);
	if ((theShooter->flags & 8) != 0)
	{
		if ((int)theShooter->age % 500 == 300)
			Shooter_Turn(theShooter);
		return;
	}
	if (theShooter->noAutoTurn == 0 && Weapons_Orbiting() == 0 &&
		2.0 < (double)theShooter->extBottom - (double)theShooter->extTop &&
		2.0 < (double)theShooter->extRight - (double)theShooter->extLeft)
	{
		int aDown = (int)theShooter->extBottom / 16 - (int)theShooter->extTop / 16;
		int anAcross = (int)theShooter->extRight / 16 - (int)theShooter->extLeft / 16;
		if (theShooter->beamAcross == 1 && aDown < anAcross)
			Shooter_Turn(theShooter);
		else if (theShooter->beamAcross == 0 && anAcross < aDown)
			Shooter_Turn(theShooter);
	}
	if (aTurn != 0)
	{
		theShooter->noAutoTurn = 1;
		Shooter_Turn(theShooter);
	}
}

} // namespace

void Shooter_FromXml(Shooter* theShooter, const char* p, const XmlTag* theTag)
{
	Obj_FromXml(theShooter, p, theTag);
	theShooter->vertAnim = Animation_Find(Xml_GetString(theTag, "vertical_animation"), &theShooter->frameCount,
		&theShooter->frameTime);
	theShooter->horizAnim = Animation_Find(Xml_GetString(theTag, "horizontal_animation"), &theShooter->frameCount,
		&theShooter->frameTime);
	theShooter->vertShootAnim = Animation_Find(Xml_GetString(theTag, "vertical_shoot_animation"),
		&theShooter->frameCount, &theShooter->frameTime);
	theShooter->horizShootAnim = Animation_Find(Xml_GetString(theTag, "horizontal_shoot_animation"),
		&theShooter->frameCount, &theShooter->frameTime);
	std::strcpy(theShooter->vertName, Xml_GetString(theTag, "vertical_animation"));
	// the original copies the vertical animation's name here too
	std::strcpy(theShooter->horizName, Xml_GetString(theTag, "vertical_animation"));
	theShooter->shootSound = Sound_Load(Xml_GetString(theTag, "shoot_sound"));
	theShooter->drillSound = Sound_Load(Xml_GetString(theTag, "drill_sound"));
	theShooter->hitSound = Sound_Load(Xml_GetString(theTag, "clank_sound"));
	if (std::strcmp(Xml_GetString(theTag, "orientation"), "vertical") == 0)
	{
		theShooter->beamAcross = 0;
		theShooter->anim = theShooter->vertAnim;
	}
	else
	{
		theShooter->beamAcross = 1;
		theShooter->anim = theShooter->horizAnim;
	}
	Obj_Copy(&theShooter->wingsV, theShooter);
	Obj_Copy(&theShooter->wingsH, theShooter);
	theShooter->wingsV.anim = Animation_Find("wings_vertical", &theShooter->wingsV.frameCount,
		&theShooter->wingsV.frameTime);
	theShooter->wingsH.anim = Animation_Find("wings_horizontal", &theShooter->wingsH.frameCount,
		&theShooter->wingsH.frameTime);
	theShooter->wingsV.depth = 5;
	theShooter->wingsH.depth = 5;
	theShooter->shadow = 1;
	Obj_Copy(&theShooter->sweeper, theShooter);
	theShooter->sweeper.anim = Animation_Find("sweeper", &theShooter->sweeper.frameCount,
		&theShooter->sweeper.frameTime);
	theShooter->sweeper.depth = 2;
	theShooter->sweeper.alpha = 0xff;
	theShooter->particle = Bank_IndexOf(&gArtBank, "particle_0");
}

void Shooter_Setup(Shooter* theShooter)
{
	theShooter->midHandle = 1;
	theShooter->dirty = 1;
	theShooter->drawKind = 4;
	theShooter->dieAnim = Animation_Find("generic_explo", &theShooter->dieFrameCount, &theShooter->dieFrameTime);
	theShooter->vertAnim = Animation_Find("shooter_vertical", &theShooter->frameCount, &theShooter->frameTime);
	theShooter->horizAnim = Animation_Find("shooter_horizontal", &theShooter->frameCount, &theShooter->frameTime);
	theShooter->vertShootAnim = Animation_Find("shooter_fire_vertical", &theShooter->frameCount,
		&theShooter->frameTime);
	theShooter->horizShootAnim = Animation_Find("shooter_fire_horizontal", &theShooter->frameCount,
		&theShooter->frameTime);
	std::strcpy(theShooter->vertName, "shooter_vertical");
	std::strcpy(theShooter->horizName, "shooter_horizontal");
	theShooter->shootSound = Sound_Load("audio\\drill_shoot.wav");
	theShooter->drillSound = Sound_Load("audio\\drill.wav");
	theShooter->hitSound = Sound_Load("audio\\clank.wav");
	theShooter->exploSound = Sound_Load("audio\\explosion2.wav");
	theShooter->beamAcross = 0;
	Obj_Copy(&theShooter->wingsV, theShooter);
	Obj_Copy(&theShooter->wingsH, theShooter);
	theShooter->wingsV.anim = Animation_Find("wings_vertical", &theShooter->wingsV.frameCount,
		&theShooter->wingsV.frameTime);
	theShooter->wingsH.anim = Animation_Find("wings_horizontal", &theShooter->wingsH.frameCount,
		&theShooter->wingsH.frameTime);
	theShooter->wingsV.depth = 5;
	theShooter->wingsH.depth = 5;
	theShooter->shadow = 1;
	Obj_Copy(&theShooter->sweeper, theShooter);
	theShooter->sweeper.anim = Animation_Find("sweeper", &theShooter->sweeper.frameCount,
		&theShooter->sweeper.frameTime);
	theShooter->sweeper.depth = 2;
	theShooter->sweeper.alpha = 0xff;
	theShooter->bubble.anim = Animation_Find("invincibubble", &theShooter->bubble.frameCount,
		&theShooter->bubble.frameTime);
	theShooter->bubble.depth = 10;
	theShooter->particle = Bank_IndexOf(&gArtBank, "particle_0");
	theShooter->anim = theShooter->vertAnim;
	if (theShooter->vertAnim != nullptr)
		theShooter->image = Animation_Frame(theShooter->vertAnim, 0);
}

void Shooter_Hit(Shooter* theShooter, float theVx, float theVy)
{
	Obj_Explode(theShooter, theVx, theVy);
	for (int i = 0x18; i != 0; i--)
	{
		ParticleSystem_New();
		int aLifetime = RandRange(700, 0x44c);
		int aLast = Bank_IndexOf(&gArtBank, "exp_4");
		int aFirst = Bank_IndexOf(&gArtBank, "exp_0");
		aFirst = aFirst + RandRange(0, 3);
		float aVy = (float)((double)RandRange(-100, 100) / 500.0);
		float aVx = (float)((double)RandRange(-100, 100) / 500.0);
		float aY = (float)RandRange(-10, 10) + theShooter->y;
		float aX = (float)RandRange(-10, 10) + theShooter->x;
		ParticleSystem_Add(gLevels.current->systems.current, aX, aY, aVx, aVy, aFirst, aLast, 0, 0xff, 0xff, 0xff,
			0xe7, 0xa6, 0x39, 0x21c, 10, aLifetime, 1.0f, 0.0f, 5);
	}
}

void Shooter_DropBeams(Shooter* theShooter)
{
	while (List_RemoveCurrent(&theShooter->beams))
		;
	theShooter->state = 3;
	theShooter->frame = 0;
	theShooter->wingsH.anim = Animation_Find("wings_horizontal", &theShooter->wingsH.frameCount,
		&theShooter->wingsH.frameTime);
	theShooter->wingsH.frame = 0;
	theShooter->wingsV.anim = Animation_Find("wings_vertical", &theShooter->wingsV.frameCount,
		&theShooter->wingsV.frameTime);
	theShooter->wingsV.frame = 0;
	if (gDemo == 0)
		Sys_SetMousePos((int)theShooter->x, (int)theShooter->y);
	theShooter->noAutoTurn = 0;
	// turned twice: the orientation stays, and the ship's animation is its plain one again
	Shooter_Turn(theShooter);
	Shooter_Turn(theShooter);
}

void Shooter_Draw(Shooter* theShooter)
{
	if (Level_IsPlaying(gLevels.current) == 0 || gLevels.current->totalCells == 0)
		return;
	Shooter_Guides(theShooter);
	Shooter_DrawExtras(theShooter);
	List_Each(&theShooter->beams, [](Beam* aBeam) { Beam_Draw(aBeam); });
	Obj_Draw(theShooter);
	if ((theShooter->state == 3 || theShooter->state == 5) && Weapons_Orbiting() == 0)
	{
		theShooter->wingsV.x = theShooter->x;
		theShooter->wingsH.x = theShooter->x;
		theShooter->wingsV.y = theShooter->y;
		theShooter->wingsH.y = theShooter->y;
		if (theShooter->beamAcross == 1)
		{
			theShooter->wingsH.alpha = theShooter->alpha;
			Obj_Draw(&theShooter->wingsH);
			return;
		}
		theShooter->wingsV.alpha = theShooter->alpha;
		Obj_Draw(&theShooter->wingsV);
	}
}

Shooter* Shooter_Init(Shooter* theShooter)
{
	Obj_Init(theShooter);
	List_Init(&theShooter->beams);
	Obj_Init(&theShooter->wingsH);
	Obj_Init(&theShooter->wingsV);
	Obj_Init(&theShooter->sweeper);
	Obj_Init(&theShooter->unk320);
	Obj_Init(&theShooter->bubble);
	theShooter->appearTime = 700.0f;
	theShooter->beamAcross = 1;
	theShooter->state = 4;
	theShooter->depth = 4;
	theShooter->flags = 0;
	theShooter->particle = 0;
	theShooter->noAutoTurn = 0;
	theShooter->cellPoints = 1;
	theShooter->beamSpeed = (float)((double)gBeamSpeed - (double)gDifficulty * (double)0.05f);
	return theShooter;
}

int Shooter_Update(Shooter* theShooter)
{
	if (Level_IsPlaying(gLevels.current) == 0 || gLevels.current->totalCells == 0)
		return 1;
	theShooter->frameCount = theShooter->anim->count;
	int aResult = Obj_Update(theShooter);
	switch (theShooter->state)
	{
	case 1:
		theShooter->state = 6;
		theShooter->age = 0.0f;
		break;
	case 3:
		Shooter_Control(theShooter);
		break;
	case 4:
		Shooter_Appear(theShooter);
		break;
	case 5:
		Shooter_Firing(theShooter);
		break;
	case 6:
		Shooter_Dying(theShooter);
		break;
	}
	Obj_Update(&theShooter->wingsH);
	Obj_Update(&theShooter->wingsV);
	return aResult;
}

// 0x41e800: a bubble leaves by the right edge to come in at the left, and by the top to start again below
int Star_Update(Obj* theStar)
{
	Obj_Update(theStar);
	if ((float)gBackBuffer.w <= theStar->x)
		theStar->x = -2.0f;
	if (theStar->y < -2.0f)
		theStar->y = (float)((double)gBackBuffer.h + 12.0);
	theStar->depth = 999;
	return 1;
}

// 0x41e850: a bubble sways sideways as it rises, within the board while it has a panel
void Star_Draw(Obj* theStar)
{
	Level* aLevel = gLevels.current;
	Image* anImage = theStar->image;
	if (anImage == nullptr)
		return;
	int aClip[4];
	aClip[1] = 0;
	aClip[0] = 0;
	aClip[2] = aLevel->cols * 0x10 + -1;
	aClip[3] = aLevel->rows * 0x10 + -1;
	int aSway = (int)((double)theStar->y * 5.0);
	float aSwayX = (float)((double)SinDeg(aSway % 0x168) * 10.0);
	const int* aDirtyClip;
	if (aLevel->panels.current == nullptr)
	{
		DrawList_Add(anImage, (uint32_t)(uintptr_t)theStar, (int)((double)theStar->x + (double)aSwayX), (int)theStar->y,
			theStar->depth, theStar->drawKind, gScreenBits, gScreenPitch, theStar->r, theStar->g, theStar->b,
			theStar->alpha, &theStar->anim->palette, nullptr);
		anImage = theStar->image;
		aDirtyClip = kScreenClip;
	}
	else
	{
		DrawList_Add(anImage, (uint32_t)(uintptr_t)theStar, (int)((double)theStar->x + (double)aSwayX), (int)theStar->y,
			theStar->depth, theStar->drawKind, gScreenBits, gScreenPitch, theStar->r, theStar->g, theStar->b,
			theStar->alpha, &theStar->anim->palette, aClip);
		anImage = theStar->image;
		aDirtyClip = aClip;
	}
	Dirty_AddClippedImage(&gDirty, (int)((double)theStar->x + (double)aSwayX), (int)theStar->y, anImage, aDirtyClip);
}

} // namespace wb
