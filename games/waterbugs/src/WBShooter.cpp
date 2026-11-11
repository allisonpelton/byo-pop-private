// The player's ship (0x41b340..0x41e800): it comes in, follows the mouse, fires a pair of beams along its row or
// column and, when both have stopped, fills the smaller side; the invincibubble is its extra.
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
	List_Each(&gLevels.mCurrent->mWeapons, [&](Weapon* theAWeapon) {
		if (theAWeapon->mState == 3)
			aCount = aCount + 1;
	});
	return aCount;
}

// 0x41b7f0: the beams turn between across and up and down
void Shooter_Turn(Shooter* theShooter)
{
	if (theShooter->mBeamAcross == 0)
	{
		theShooter->mAnim = theShooter->mHorizAnim;
		theShooter->mBeamAcross = 1;
		return;
	}
	theShooter->mAnim = theShooter->mVertAnim;
	theShooter->mBeamAcross = 0;
}

// 0x41d370 (state 4): four sparkles circle in towards the middle of the screen while the ship comes in, and a burst
// of sparkles marks its arrival
void Shooter_Appear(Shooter* theShooter)
{
	int aFrameTimes[2];
	Sys_SetMousePos(400, 300);
	theShooter->mX = 400.0f;
	theShooter->mY = 300.0f;
	if (theShooter->mAge == 1.0f)
		Level_ShowStart(gLevels.mCurrent);
	aFrameTimes[1] = 0;
	if (theShooter->mAppearTime <= theShooter->mAge)
	{
		if (theShooter->mAppearTime == theShooter->mAge)
		{
			theShooter->mDrawKind = 4;
			int aSparksLeft = 0x78;
			do
			{
				int anAngle = RandRange(0, 0x167);
				int aSpeed = RandRange(0, 100);
				float aStep = (float)((double)aSpeed / 360.0);
				float aVx = (float)((double)CosDeg(anAngle) * (double)aStep);
				float aVy = (float)((double)SinDeg(anAngle) * (double)aStep);
				ParticleSystem_New();
				int aLifetime = RandRange(600, 0x4b0);
				ParticleSystem_Add(gLevels.mCurrent->mSystems.mCurrent, theShooter->mX, theShooter->mY, aVx, aVy, 0, 0,
					0, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0, aLifetime, 1.0f, 0.0f, 4);
				Particle* aParticle = gLevels.mCurrent->mSystems.mCurrent->mParticles.mCurrent;
				aParticle->mAnim = Animation_Find("sparkles3", &aParticle->mFrameCount, &aFrameTimes[1]);
				aParticle->mTint = RandRange(0, 7);
				aSparksLeft = aSparksLeft - 1;
			} while (aSparksLeft != 0);
			theShooter->mAlpha = 0xff;
			theShooter->mState = 3;
			while (List_RemoveCurrent(&theShooter->mChildren))
				;
		}
		return;
	}
	if (theShooter->mAge == 1.0f)
	{
		List<Obj>* aChildren = &theShooter->mChildren;
		int aSparklesLeft = 4;
		do
		{
			Panel_AppendItem(aChildren);
			aChildren->mCurrent->mAnim =
				Animation_Find("sparkles1", &aChildren->mCurrent->mFrameCount, &aChildren->mCurrent->mFrameTime);
			aChildren->mCurrent->mMidHandle = 1;
			aChildren->mCurrent->mDirty = 1;
			aChildren->mCurrent->mDrawKind = 4;
			aSparklesLeft = aSparklesLeft - 1;
			aChildren->mCurrent->mAlpha = 0xff;
		} while (aSparklesLeft != 0);
	}
	float aRatio = theShooter->mAge / theShooter->mAppearTime;
	theShooter->mAlpha = (int)aRatio;
	int aSpin = (int)((double)theShooter->mAge / 3.0);
	aSpin = aSpin % 0x168;
	if (theShooter->mChildren.mCurrent != nullptr)
	{
		Obj* aChild = theShooter->mChildren.mHead;
		Obj* aCurrent;
		double aRadius = (double)(float)(1.0 - (double)aRatio * (double)aRatio) * 160.0;
		while (true)
		{
			theShooter->mChildren.mCurrent = aChild;
			theShooter->mChildren.mCurrent->mX =
				(float)((double)CosDeg(aSpin % 0x168) * aRadius + (double)theShooter->mX);
			theShooter->mChildren.mCurrent->mY =
				(float)((double)SinDeg(aSpin % 0x168) * aRadius + (double)theShooter->mY);
			if (RandRange(1, 0x10) == 5)
			{
				ParticleSystem_New();
				float aY = (float)RandRange(-4, 4) + theShooter->mChildren.mCurrent->mY;
				float aX = (float)RandRange(-4, 4) + theShooter->mChildren.mCurrent->mX;
				ParticleSystem_Add(gLevels.mCurrent->mSystems.mCurrent, aX, aY, 0.0f, 0.0f, 0, 0, 0, 0xff, 0xff, 0xff,
					0xff, 0xff, 0xff, 0x21c, 10, 400, 0.999f, 0.0002f, 5);
				Particle* aParticle = gLevels.mCurrent->mSystems.mCurrent->mParticles.mCurrent;
				aParticle->mAnim = Animation_Find("sparkles3", &aParticle->mFrameCount, &aFrameTimes[0]);
			}
			aCurrent = theShooter->mChildren.mCurrent;
			aSpin = aSpin + 0x5a;
			if (aCurrent == nullptr)
				return;
			if (aCurrent->mNext == nullptr)
				break;
			aChild = aCurrent->mNext;
		}
		if (aCurrent != nullptr)
			theShooter->mChildren.mCurrent = theShooter->mChildren.mHead;
	}
}

// 0x41b830: starts filling the open cells of theCell's row (beams across) or column (beams up and down) between
// the beams' ends, on the side theCell is
void Shooter_Fill(Shooter* theShooter, Cell* theCell)
{
	// RandMod(1) is always 0 (the level's one fill image), but it takes a number from the generator
	if (theShooter->mBeamAcross == 0)
	{
		int aTail = (int)theShooter->mBeams.mTail->mY / 16;
		int aHead = (int)theShooter->mBeams.mHead->mY / 16;
		int aLo = aHead < aTail ? aHead : aTail;
		int aHi = aHead > aTail ? aHead : aTail;
		for (int i = theCell->mRow; i <= aHi; i++)
		{
			Cell* aCell = &gLevels.mCurrent->mColumns[theCell->mCol][i];
			if (aCell->mKind == 0 && aCell->mUnmarked != 0)
			{
				int anImage = (int)RandMod(1);
				Cell_StartCapture(aCell, (&gLevels.mCurrent->mFillImage)[anImage], 0, 4);
				Cell_Flood(aCell);
			}
		}
		for (int i = theCell->mRow; i >= aLo; i--)
		{
			Cell* aCell = &gLevels.mCurrent->mColumns[theCell->mCol][i];
			if (aCell->mKind == 0 && aCell->mUnmarked != 0)
			{
				int anImage = (int)RandMod(1);
				Cell_StartCapture(aCell, (&gLevels.mCurrent->mFillImage)[anImage], 0, 4);
				Cell_Flood(aCell);
			}
		}
		return;
	}
	int aTail = (int)theShooter->mBeams.mTail->mX / 16;
	int aHead = (int)theShooter->mBeams.mHead->mX / 16;
	int aLo = aHead < aTail ? aHead : aTail;
	int aHi = aHead > aTail ? aHead : aTail;
	for (int i = theCell->mCol; i <= aHi; i++)
	{
		Cell* aCell = &gLevels.mCurrent->mColumns[i][theCell->mRow];
		if (aCell->mKind == 0 && aCell->mUnmarked != 0)
		{
			int anImage = (int)RandMod(1);
			Cell_StartCapture(aCell, (&gLevels.mCurrent->mFillImage)[anImage], 0, 4);
			Cell_Flood(aCell);
		}
	}
	for (int i = theCell->mCol; i >= aLo; i--)
	{
		Cell* aCell = &gLevels.mCurrent->mColumns[i][theCell->mRow];
		if (aCell->mKind == 0 && aCell->mUnmarked != 0)
		{
			int anImage = (int)RandMod(1);
			Cell_StartCapture(aCell, (&gLevels.mCurrent->mFillImage)[anImage], 0, 4);
			Cell_Flood(aCell);
		}
	}
}

// 0x41bb40: the open span about the ship along its row (theLeft to theRight) and its column (theTop to theBottom):
// each end walks 16 pixels a step to the first cell not open, then back a pixel a step onto an open one. All four
// are 0 when the ship is within 16 pixels of the screen's edge. Every walk's
// guard checks the left end's column and the top end's row, as the original's do.
void Shooter_Extents(Shooter* theShooter, float* theLeft, float* theTop, float* theRight, float* theBottom)
{
	if (16.0f < theShooter->mX && theShooter->mX < 784.0f && 16.0f < theShooter->mY && theShooter->mY < 584.0f)
	{
		float aLeft = 0.0f <= theShooter->mX ? theShooter->mX : 0.0f;
		float aRight = theShooter->mX < 799.0f ? theShooter->mX : 799.0f;
		float aTop = theShooter->mY < 0.0f ? 0.0f : theShooter->mY;
		float aBottom = theShooter->mY <= 599.0 ? theShooter->mY : 599.0f;
		Cell** aColumns = gLevels.mCurrent->mColumns;
		int aMoved = 0;
		while ((int)aLeft / 16 < 50 && (int)aTop / 16 < 37 && aColumns[(int)aLeft / 16][(int)aTop / 16].mKind == 0)
		{
			aLeft = aLeft - 16.0f;
			aMoved = 1;
		}
		while ((int)aLeft / 16 < 50 && (int)aTop / 16 < 37 && aMoved != 0 &&
			aColumns[(int)aLeft / 16][(int)aTop / 16].mKind != 0)
			aLeft = aLeft + 1.0f;
		aMoved = 0;
		while ((int)aLeft / 16 < 50 && (int)aTop / 16 < 37 && aColumns[(int)aRight / 16][(int)aTop / 16].mKind == 0)
		{
			aRight = aRight + 16.0f;
			aMoved = 1;
		}
		while ((int)aLeft / 16 < 50 && (int)aTop / 16 < 37 && aMoved != 0 &&
			aColumns[(int)aRight / 16][(int)aTop / 16].mKind != 0)
			aRight = aRight - 1.0f;
		aMoved = 0;
		while ((int)aLeft / 16 < 50 && (int)aTop / 16 < 37 &&
			aColumns[(int)theShooter->mX / 16][(int)aTop / 16].mKind == 0)
		{
			aTop = aTop - 16.0f;
			aMoved = 1;
		}
		while ((int)aLeft / 16 < 50 && (int)aTop / 16 < 37 && aMoved != 0 &&
			aColumns[(int)theShooter->mX / 16][(int)aTop / 16].mKind != 0)
			aTop = aTop + 1.0f;
		aMoved = 0;
		while ((int)aLeft / 16 < 50 && (int)aTop / 16 < 37 &&
			aColumns[(int)theShooter->mX / 16][(int)aBottom / 16].mKind == 0)
		{
			aBottom = aBottom + 16.0f;
			aMoved = 1;
		}
		while ((int)aLeft / 16 < 50 && (int)aTop / 16 < 37 && aMoved != 0 &&
			aColumns[(int)theShooter->mX / 16][(int)aBottom / 16].mKind != 0)
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
	List_Each(&gLevels.mCurrent->mEnemies, [&](Enemy* theAnEnemy) {
		if (CircleOverlap(theShooter->mX, theShooter->mY, (float)(theShooter->mImage->mH / 3), theAnEnemy->mX,
			theAnEnemy->mY, (float)(theAnEnemy->mImage->mH / 3)) != 0.0)
			aCount = aCount + 1;
	});
	return aCount;
}

// 0x41c230: counts the open region about (theCol, theRow) not visited yet, its cells, the enemies in them and the
// sum of their x (rounded)
int Region_Count(int theCol, int theRow, int* theCells, int* theEnemies, int* theSumX)
{
	if (gLevels.mCurrent->mColumns[theCol][theRow].mKind != 0 || gVisited[theCol][theRow] != 0)
		return 0;
	List_Each(&gLevels.mCurrent->mEnemies, [&](Enemy* theAnEnemy) {
		if (theCol == (int)theAnEnemy->mX / 16 && theRow == (int)theAnEnemy->mY / 16)
		{
			*theEnemies = *theEnemies + 1;
			*theSumX = *theSumX + (int)theAnEnemy->mX;
		}
	});
	*theCells = *theCells + 1;
	gVisited[theCol][theRow] = 1;
	for (int c = theCol - 1; c <= theCol + 1; c++)
	{
		for (int r = theRow - 1; r <= theRow + 1; r++)
		{
			if ((c == theCol || r == theRow) && 0 <= c && c < gLevels.mCurrent->mCols && 0 <= r &&
				r < gLevels.mCurrent->mRows && gLevels.mCurrent->mColumns[c][r].mKind == 0 && gVisited[c][r] == 0)
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
	if (theShooter->mBeamAcross == 0)
	{
		int aTail = (int)theShooter->mBeams.mTail->mY / 16;
		int aHead = (int)theShooter->mBeams.mHead->mY / 16;
		int aLo = aHead < aTail ? aHead : aTail;
		int aHi = aHead > aTail ? aHead : aTail;
		for (int i = theCell->mRow; i <= aHi; i++)
		{
			if (gVisited[theCell->mCol][i] == 0)
				Region_Count(theCell->mCol, i, &aCells, theEnemies, theSumX);
		}
		for (int i = theCell->mRow; aLo <= i; i--)
		{
			if (gVisited[theCell->mCol][i] == 0)
				Region_Count(theCell->mCol, i, &aCells, theEnemies, theSumX);
		}
		return aCells;
	}
	int aTail = (int)theShooter->mBeams.mTail->mX / 16;
	int aHead = (int)theShooter->mBeams.mHead->mX / 16;
	int aLo = aHead < aTail ? aHead : aTail;
	int aHi = aHead > aTail ? aHead : aTail;
	for (int i = theCell->mCol; i <= aHi; i++)
	{
		if (gVisited[i][theCell->mRow] == 0)
			Region_Count(i, theCell->mRow, &aCells, theEnemies, theSumX);
	}
	for (int i = theCell->mCol; aLo <= i; i--)
	{
		if (gVisited[i][theCell->mRow] == 0)
			Region_Count(i, theCell->mRow, &aCells, theEnemies, theSumX);
	}
	return aCells;
}

// 0x41cd90: the aiming guides along the open span, 11 lines across the beams' path, fainter at the edges
void Shooter_Guides(Shooter* theShooter)
{
	int anAlpha = 5;
	if (theShooter->mState != 3 || Weapons_Orbiting() != 0)
		return;
	if (theShooter->mBeamAcross == 1)
	{
		int aLo = (int)((double)theShooter->mY - 5.0);
		int aHi = (int)((double)theShooter->mY + 5.0);
		for (; aLo <= aHi; aLo++, aHi--, anAlpha += 10)
		{
			Draw_Line((int)theShooter->mExtLeft, aLo, (int)theShooter->mExtRight, aLo, 0xff, 0xff, 0xff, anAlpha);
			if (aLo != aHi)
				Draw_Line((int)theShooter->mExtLeft, aHi, (int)theShooter->mExtRight, aHi, 0xff, 0xff, 0xff, anAlpha);
		}
		Dirty_AddClippedShown(&gDirty, (int)theShooter->mExtLeft, (int)((double)theShooter->mY - 5.0),
			(int)((double)theShooter->mExtRight - (double)theShooter->mExtLeft + 1.0), 0xb, SCREEN_CLIP);
		return;
	}
	int aLo = (int)((double)theShooter->mX - 5.0);
	int aHi = (int)((double)theShooter->mX + 5.0);
	for (; aLo <= aHi; aLo++, aHi--, anAlpha += 10)
	{
		Draw_Line(aLo, (int)theShooter->mExtTop, aLo, (int)theShooter->mExtBottom, 0xff, 0xff, 0xff, anAlpha);
		if (aLo != aHi)
			Draw_Line(aHi, (int)theShooter->mExtTop, aHi, (int)theShooter->mExtBottom, 0xff, 0xff, 0xff, anAlpha);
	}
	Dirty_AddClippedShown(&gDirty, (int)((double)theShooter->mX - 5.0), (int)theShooter->mExtTop, 0xb,
		(int)((double)theShooter->mExtBottom - (double)theShooter->mExtTop + 1.0), SCREEN_CLIP);
}

// 0x41d2e0
void Shooter_UpdateExtras(Shooter* theShooter)
{
	if ((theShooter->mFlags & 0x10) != 0)
	{
		theShooter->mBubble.mX = theShooter->mX;
		theShooter->mBubble.mDrawKind = 1;
		theShooter->mBubble.mDirty = 1;
		theShooter->mBubble.mY = theShooter->mY;
		theShooter->mBubble.mMidHandle = 1;
		Obj_Update(&theShooter->mBubble);
	}
}

// 0x41d340
void Shooter_DrawExtras(Shooter* theShooter)
{
	if ((theShooter->mFlags & 0x10) != 0)
		Obj_Draw(&theShooter->mBubble);
}

// 0x41d7a0 (state 6): once the explosion has played, the next life comes in, or the game is over
void Shooter_Dying(Shooter* theShooter)
{
	while (List_RemoveCurrent(&theShooter->mBeams))
		;
	theShooter->mFlags = 0;
	theShooter->mBeamSpeed = (float)((double)gBeamSpeed - (double)gDifficulty * (double)0.05f);
	if ((double)(theShooter->mFrameCount * theShooter->mFrameTime) < (double)theShooter->mAge)
	{
		if (0 < gLives)
		{
			Animation* anAnim = Animation_Find(theShooter->mVertName, &theShooter->mFrameCount,
				&theShooter->mFrameTime);
			theShooter->mVertAnim = anAnim;
			theShooter->mAnim = anAnim;
			theShooter->mWingsH.mAnim = Animation_Find("wings_horizontal", &theShooter->mWingsH.mFrameCount,
				&theShooter->mWingsH.mFrameTime);
			theShooter->mWingsH.mFrame = 0;
			anAnim = Animation_Find("wings_vertical", &theShooter->mWingsV.mFrameCount,
				&theShooter->mWingsV.mFrameTime);
			theShooter->mAge = 0.0f;
			theShooter->mWingsV.mFrame = 0;
			theShooter->mFrame = 0;
			theShooter->mOneShot = 0;
			theShooter->mBeamAcross = 0;
			theShooter->mDepth = 0;
			theShooter->mWingsV.mAnim = anAnim;
			theShooter->mCellPoints = 1;
			theShooter->mState = 4;
			SaveProfiles();
			return;
		}
		Level_GameOver(gLevels.mCurrent);
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
	Sys_SetMousePos((int)theShooter->mX, (int)theShooter->mY);
	Shooter_UpdateExtras(theShooter);
	if (theShooter->mBeams.mCurrent != nullptr)
	{
		List_Each(&theShooter->mBeams, [&](Beam* theABeam) {
			if ((int)((double)theShooter->mAge - (double)theShooter->mFireAge) % 0x42 == 0)
				Sound_Play(theShooter->mDrillSound);
			aGoing = aGoing | Beam_Update(theShooter->mBeams.mCurrent);
		});
		if (aGoing != 0)
			return;
	}
	Beam_DrawTo(theShooter->mBeams.mHead, gBackBuffer.mBits, gBackBuffer.mPitch);
	Beam_DrawTo(theShooter->mBeams.mTail, gBackBuffer.mBits, gBackBuffer.mPitch);
	// up and down, the sides are the cells left and right of the ship's; across, above and below it
	Level* aLevel = gLevels.mCurrent;
	Cell* aSide1;
	Cell* aSide2;
	if (theShooter->mBeams.mCurrent->mVx == 0.0f)
	{
		int aCol = (int)theShooter->mX / 16;
		int aRow = (int)theShooter->mY / 16;
		aSide1 = &aLevel->mColumns[aCol - 1][aRow];
		aSide2 = &aLevel->mColumns[aCol + 1][aRow];
		for (int i = (int)theShooter->mBeams.mTail->mY / 16 - 1; i <= (int)theShooter->mBeams.mHead->mY / 16 + 1; i++)
		{
			Cell* aCell = &aLevel->mColumns[(int)theShooter->mX / 16][i];
			if (aCell->mKind != 3)
			{
				aCell->mKind = 1;
				aCell->mUnmarked = 0;
			}
		}
		Cell_StartCapture(&aLevel->mColumns[(int)theShooter->mX / 16][(int)theShooter->mY / 16], aLevel->mBeamV, 1, 2);
	}
	else
	{
		Cell* aCell = &aLevel->mColumns[(int)theShooter->mX / 16][(int)theShooter->mY / 16];
		aSide1 = aCell - 1;
		aSide2 = aCell + 1;
		for (int i = (int)theShooter->mBeams.mTail->mX / 16 - 1; i <= (int)theShooter->mBeams.mHead->mX / 16 + 1; i++)
		{
			Cell* aBeamCell = &aLevel->mColumns[i][(int)theShooter->mY / 16];
			if (aBeamCell->mKind != 3)
			{
				aBeamCell->mKind = 1;
				aBeamCell->mUnmarked = 0;
			}
		}
		Cell_StartCapture(&aLevel->mColumns[(int)theShooter->mX / 16][(int)theShooter->mY / 16], aLevel->mBeamH, 1, 2);
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
		if (gLevels.mCurrent->mNumber > 2 && anEnemies1 == 0 && anEnemies2 == 0)
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
		else if (gLevels.mCurrent->mNumber <= 2)
			Shooter_Fill(theShooter, aSide1);
		Sound_Play(gLevels.mCurrent->mFillSound);
	}
	gLevels.mCurrent->mLines = gLevels.mCurrent->mLines + 1;
	Shooter_DropBeams(theShooter);
}

// 0x41e110: a click fires a beam each way from the middle of the cell under the ship, if it is open and not yet
// reached by a flood; with an enemy on the ship it buzzes instead; once the filled share is past the quota, a click
// does nothing
void Shooter_Fire(Shooter* theShooter)
{
	if (Weapons_Orbiting() != 0)
		return;
	Level* aLevel = gLevels.mCurrent;
	if (aLevel->mClicked == 0)
		return;
	if (aLevel->mFillQuota < (aLevel->mFilledCells * 100) / aLevel->mTotalCells || Shooter_EnemiesNear(theShooter) != 0)
	{
		aLevel = gLevels.mCurrent;
		if (aLevel->mClicked != 0 && (aLevel->mFilledCells * 100) / aLevel->mTotalCells <= aLevel->mFillQuota &&
			Shooter_EnemiesNear(theShooter) != 0)
		{
			// (the original works out a pan from x for a play call that takes none)
			Sound_Play(Sound_Find("audio\\buzzer.wav"));
		}
		return;
	}
	{
		int aRow = (int)theShooter->mY / 16;
		if (aRow < gLevels.mCurrent->mRows)
		{
			int aCol = (int)theShooter->mX / 16;
			Cell* aCell = &gLevels.mCurrent->mColumns[aCol][aRow];
			if (aCell->mKind == 0 && aCell->mUnmarked != 0)
			{
				theShooter->mX = (float)(aCol * 0x10 + 8);
				List<Beam>* aBeams = &theShooter->mBeams;
				theShooter->mY = (float)(aRow * 0x10 + 8);
				gLevels.mCurrent->mShots = gLevels.mCurrent->mShots + 1;
				theShooter->mState = 5;
				gProfiles[gProfile].mBeamsFired = gProfiles[gProfile].mBeamsFired + 1;
				Beam_Insert(aBeams);
				aBeams->mCurrent->mX = theShooter->mX;
				aBeams->mCurrent->mY = theShooter->mY;
				Image* anImage;
				if (theShooter->mBeamAcross == 1)
				{
					Beam_Setup(aBeams->mCurrent, theShooter, theShooter->mX, theShooter->mY, -theShooter->mBeamSpeed,
						0.0f);
					anImage = gLevels.mCurrent->mBeamH;
				}
				else
				{
					Beam_Setup(aBeams->mCurrent, theShooter, theShooter->mX, theShooter->mY, 0.0f,
						-theShooter->mBeamSpeed);
					anImage = gLevels.mCurrent->mBeamV;
				}
				aBeams->mCurrent->mImage = anImage;
				Beam_Insert(aBeams);
				aBeams->mCurrent->mX = theShooter->mX;
				aBeams->mCurrent->mY = theShooter->mY;
				if (theShooter->mBeamAcross == 1)
				{
					Beam_Setup(aBeams->mCurrent, theShooter, theShooter->mX, theShooter->mY, theShooter->mBeamSpeed,
						0.0f);
					aBeams->mCurrent->mImage = gLevels.mCurrent->mBeamH;
					theShooter->mAnim = theShooter->mHorizShootAnim;
					theShooter->mWingsH.mAnim = Animation_Find("wings_fire_horizontal",
						&theShooter->mWingsH.mFrameCount, &theShooter->mWingsH.mFrameTime);
					theShooter->mWingsH.mFrame = 0;
				}
				else
				{
					Beam_Setup(aBeams->mCurrent, theShooter, theShooter->mX, theShooter->mY, 0.0f,
						theShooter->mBeamSpeed);
					aBeams->mCurrent->mImage = gLevels.mCurrent->mBeamV;
					theShooter->mAnim = theShooter->mVertShootAnim;
					theShooter->mWingsV.mAnim = Animation_Find("wings_fire_vertical", &theShooter->mWingsV.mFrameCount,
						&theShooter->mWingsV.mFrameTime);
					theShooter->mWingsV.mFrame = 0;
				}
				if (List_Count(aBeams) < 3)
				{
					theShooter->mFireAge = (int)theShooter->mAge;
					Sound_Play(theShooter->mShootSound);
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
		theShooter->mNoAutoTurn = 1;
	int aTurn = gLevels.mCurrent->mRightClicked;
	double aX = ((double)gMouseX + (double)theShooter->mX) * 0.5;
	theShooter->mX = (float)(799.0 < aX ? 799.0 : (aX < 0.0 ? 0.0 : aX));
	double aY = ((double)gMouseY + (double)theShooter->mY) * 0.5;
	theShooter->mY = (float)(599.0 < aY ? 599.0 : (aY < 0.0 ? 0.0 : aY));
	Shooter_Fire(theShooter);
	Shooter_UpdateExtras(theShooter);
	Shooter_Extents(theShooter, &theShooter->mExtLeft, &theShooter->mExtTop, &theShooter->mExtRight,
		&theShooter->mExtBottom);
	if ((theShooter->mFlags & 8) != 0)
	{
		if ((int)theShooter->mAge % 500 == 300)
			Shooter_Turn(theShooter);
		return;
	}
	if (theShooter->mNoAutoTurn == 0 && Weapons_Orbiting() == 0 &&
		2.0 < (double)theShooter->mExtBottom - (double)theShooter->mExtTop &&
		2.0 < (double)theShooter->mExtRight - (double)theShooter->mExtLeft)
	{
		int aDown = (int)theShooter->mExtBottom / 16 - (int)theShooter->mExtTop / 16;
		int anAcross = (int)theShooter->mExtRight / 16 - (int)theShooter->mExtLeft / 16;
		if (theShooter->mBeamAcross == 1 && aDown < anAcross)
			Shooter_Turn(theShooter);
		else if (theShooter->mBeamAcross == 0 && anAcross < aDown)
			Shooter_Turn(theShooter);
	}
	if (aTurn != 0)
	{
		theShooter->mNoAutoTurn = 1;
		Shooter_Turn(theShooter);
	}
}

} // namespace

void Shooter_FromXml(Shooter* theShooter, const char* p, const XmlTag* theTag)
{
	Obj_FromXml(theShooter, p, theTag);
	theShooter->mVertAnim = Animation_Find(Xml_GetString(theTag, "vertical_animation"), &theShooter->mFrameCount,
		&theShooter->mFrameTime);
	theShooter->mHorizAnim = Animation_Find(Xml_GetString(theTag, "horizontal_animation"), &theShooter->mFrameCount,
		&theShooter->mFrameTime);
	theShooter->mVertShootAnim = Animation_Find(Xml_GetString(theTag, "vertical_shoot_animation"),
		&theShooter->mFrameCount, &theShooter->mFrameTime);
	theShooter->mHorizShootAnim = Animation_Find(Xml_GetString(theTag, "horizontal_shoot_animation"),
		&theShooter->mFrameCount, &theShooter->mFrameTime);
	std::strcpy(theShooter->mVertName, Xml_GetString(theTag, "vertical_animation"));
	// the original copies the vertical animation's name here too
	std::strcpy(theShooter->mHorizName, Xml_GetString(theTag, "vertical_animation"));
	theShooter->mShootSound = Sound_Load(Xml_GetString(theTag, "shoot_sound"));
	theShooter->mDrillSound = Sound_Load(Xml_GetString(theTag, "drill_sound"));
	theShooter->mHitSound = Sound_Load(Xml_GetString(theTag, "clank_sound"));
	if (std::strcmp(Xml_GetString(theTag, "orientation"), "vertical") == 0)
	{
		theShooter->mBeamAcross = 0;
		theShooter->mAnim = theShooter->mVertAnim;
	}
	else
	{
		theShooter->mBeamAcross = 1;
		theShooter->mAnim = theShooter->mHorizAnim;
	}
	Obj_Copy(&theShooter->mWingsV, theShooter);
	Obj_Copy(&theShooter->mWingsH, theShooter);
	theShooter->mWingsV.mAnim = Animation_Find("wings_vertical", &theShooter->mWingsV.mFrameCount,
		&theShooter->mWingsV.mFrameTime);
	theShooter->mWingsH.mAnim = Animation_Find("wings_horizontal", &theShooter->mWingsH.mFrameCount,
		&theShooter->mWingsH.mFrameTime);
	theShooter->mWingsV.mDepth = 5;
	theShooter->mWingsH.mDepth = 5;
	theShooter->mShadow = 1;
}

void Shooter_Setup(Shooter* theShooter)
{
	theShooter->mMidHandle = 1;
	theShooter->mDirty = 1;
	theShooter->mDrawKind = 4;
	theShooter->mDieAnim = Animation_Find("generic_explo", &theShooter->mDieFrameCount, &theShooter->mDieFrameTime);
	theShooter->mVertAnim = Animation_Find("shooter_vertical", &theShooter->mFrameCount, &theShooter->mFrameTime);
	theShooter->mHorizAnim = Animation_Find("shooter_horizontal", &theShooter->mFrameCount, &theShooter->mFrameTime);
	theShooter->mVertShootAnim = Animation_Find("shooter_fire_vertical", &theShooter->mFrameCount,
		&theShooter->mFrameTime);
	theShooter->mHorizShootAnim = Animation_Find("shooter_fire_horizontal", &theShooter->mFrameCount,
		&theShooter->mFrameTime);
	std::strcpy(theShooter->mVertName, "shooter_vertical");
	std::strcpy(theShooter->mHorizName, "shooter_horizontal");
	theShooter->mShootSound = Sound_Load("audio\\drill_shoot.wav");
	theShooter->mDrillSound = Sound_Load("audio\\drill.wav");
	theShooter->mHitSound = Sound_Load("audio\\clank.wav");
	theShooter->mExploSound = Sound_Load("audio\\explosion2.wav");
	theShooter->mBeamAcross = 0;
	Obj_Copy(&theShooter->mWingsV, theShooter);
	Obj_Copy(&theShooter->mWingsH, theShooter);
	theShooter->mWingsV.mAnim = Animation_Find("wings_vertical", &theShooter->mWingsV.mFrameCount,
		&theShooter->mWingsV.mFrameTime);
	theShooter->mWingsH.mAnim = Animation_Find("wings_horizontal", &theShooter->mWingsH.mFrameCount,
		&theShooter->mWingsH.mFrameTime);
	theShooter->mWingsV.mDepth = 5;
	theShooter->mWingsH.mDepth = 5;
	theShooter->mShadow = 1;
	theShooter->mBubble.mAnim = Animation_Find("invincibubble", &theShooter->mBubble.mFrameCount,
		&theShooter->mBubble.mFrameTime);
	theShooter->mBubble.mDepth = 10;
	theShooter->mAnim = theShooter->mVertAnim;
	if (theShooter->mVertAnim != nullptr)
		theShooter->mImage = Animation_Frame(theShooter->mVertAnim, 0);
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
		float aY = (float)RandRange(-10, 10) + theShooter->mY;
		float aX = (float)RandRange(-10, 10) + theShooter->mX;
		ParticleSystem_Add(gLevels.mCurrent->mSystems.mCurrent, aX, aY, aVx, aVy, aFirst, aLast, 0, 0xff, 0xff, 0xff,
			0xe7, 0xa6, 0x39, 0x21c, 10, aLifetime, 1.0f, 0.0f, 5);
	}
}

void Shooter_DropBeams(Shooter* theShooter)
{
	while (List_RemoveCurrent(&theShooter->mBeams))
		;
	theShooter->mState = 3;
	theShooter->mFrame = 0;
	theShooter->mWingsH.mAnim = Animation_Find("wings_horizontal", &theShooter->mWingsH.mFrameCount,
		&theShooter->mWingsH.mFrameTime);
	theShooter->mWingsH.mFrame = 0;
	theShooter->mWingsV.mAnim = Animation_Find("wings_vertical", &theShooter->mWingsV.mFrameCount,
		&theShooter->mWingsV.mFrameTime);
	theShooter->mWingsV.mFrame = 0;
	Sys_SetMousePos((int)theShooter->mX, (int)theShooter->mY);
	theShooter->mNoAutoTurn = 0;
	// turned twice: the orientation stays, and the ship's animation is its plain one again
	Shooter_Turn(theShooter);
	Shooter_Turn(theShooter);
}

void Shooter_Draw(Shooter* theShooter)
{
	if (Level_IsPlaying(gLevels.mCurrent) == 0 || gLevels.mCurrent->mTotalCells == 0)
		return;
	Shooter_Guides(theShooter);
	Shooter_DrawExtras(theShooter);
	List_Each(&theShooter->mBeams, [](Beam* theABeam) { Beam_Draw(theABeam); });
	Obj_Draw(theShooter);
	if ((theShooter->mState == 3 || theShooter->mState == 5) && Weapons_Orbiting() == 0)
	{
		theShooter->mWingsV.mX = theShooter->mX;
		theShooter->mWingsH.mX = theShooter->mX;
		theShooter->mWingsV.mY = theShooter->mY;
		theShooter->mWingsH.mY = theShooter->mY;
		if (theShooter->mBeamAcross == 1)
		{
			theShooter->mWingsH.mAlpha = theShooter->mAlpha;
			Obj_Draw(&theShooter->mWingsH);
			return;
		}
		theShooter->mWingsV.mAlpha = theShooter->mAlpha;
		Obj_Draw(&theShooter->mWingsV);
	}
}

Shooter* Shooter_Init(Shooter* theShooter)
{
	Obj_Init(theShooter);
	List_Init(&theShooter->mBeams);
	Obj_Init(&theShooter->mWingsH);
	Obj_Init(&theShooter->mWingsV);
	Obj_Init(&theShooter->mIdleObj);
	Obj_Init(&theShooter->mBubble);
	theShooter->mAppearTime = 700.0f;
	theShooter->mBeamAcross = 1;
	theShooter->mState = 4;
	theShooter->mDepth = 4;
	theShooter->mFlags = 0;
	theShooter->mNoAutoTurn = 0;
	theShooter->mCellPoints = 1;
	theShooter->mBeamSpeed = (float)((double)gBeamSpeed - (double)gDifficulty * (double)0.05f);
	return theShooter;
}

// 0x412360: the four Objs it holds are destroyed, the beams still in the air are deleted (the list steps from its
// head), then the base
void Shooter_Destroy(Shooter* theShooter)
{
	Obj_Destroy(&theShooter->mBubble);
	Obj_Destroy(&theShooter->mIdleObj);
	Obj_Destroy(&theShooter->mWingsV);
	Obj_Destroy(&theShooter->mWingsH);
	if (theShooter->mBeams.mCurrent != nullptr)
	{
		theShooter->mBeams.mCurrent = theShooter->mBeams.mHead;
		Beams_RemoveAll(&theShooter->mBeams);
	}
	Obj_Destroy(theShooter);
}

void List_DeleteNode(Shooter* theShooter)
{
	Shooter_Destroy(theShooter);
	::operator delete(theShooter);
}

int Shooter_Update(Shooter* theShooter)
{
	if (Level_IsPlaying(gLevels.mCurrent) == 0 || gLevels.mCurrent->mTotalCells == 0)
		return 1;
	theShooter->mFrameCount = theShooter->mAnim->mCount;
	int aResult = Obj_Update(theShooter);
	switch (theShooter->mState)
	{
	case 1:
		theShooter->mState = 6;
		theShooter->mAge = 0.0f;
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
	Obj_Update(&theShooter->mWingsH);
	Obj_Update(&theShooter->mWingsV);
	return aResult;
}

// 0x41e800: a bubble leaves by the right edge to come in at the left, and by the top to start again below
int Star_Update(Obj* theStar)
{
	Obj_Update(theStar);
	if ((float)gBackBuffer.mW <= theStar->mX)
		theStar->mX = -2.0f;
	if (theStar->mY < -2.0f)
		theStar->mY = (float)((double)gBackBuffer.mH + 12.0);
	theStar->mDepth = 999;
	return 1;
}

// 0x41e850: a bubble sways sideways as it rises, within the board while it has a panel
void Star_Draw(Obj* theStar)
{
	Level* aLevel = gLevels.mCurrent;
	Image* anImage = theStar->mImage;
	if (anImage == nullptr)
		return;
	int aClip[4];
	aClip[1] = 0;
	aClip[0] = 0;
	aClip[2] = aLevel->mCols * 0x10 + -1;
	aClip[3] = aLevel->mRows * 0x10 + -1;
	int aSway = (int)((double)theStar->mY * 5.0);
	float aSwayX = (float)((double)SinDeg(aSway % 0x168) * 10.0);
	const int* aDirtyClip;
	if (aLevel->mPanels.mCurrent == nullptr)
	{
		DrawList_Add(anImage, (uint32_t)(uintptr_t)theStar, (int)((double)theStar->mX + (double)aSwayX),
			(int)theStar->mY, theStar->mDepth, theStar->mDrawKind, gScreenBits, gScreenPitch, theStar->mR, theStar->mG,
			theStar->mB, theStar->mAlpha, &theStar->mAnim->mPalette, nullptr);
		anImage = theStar->mImage;
		aDirtyClip = SCREEN_CLIP;
	}
	else
	{
		DrawList_Add(anImage, (uint32_t)(uintptr_t)theStar, (int)((double)theStar->mX + (double)aSwayX),
			(int)theStar->mY, theStar->mDepth, theStar->mDrawKind, gScreenBits, gScreenPitch, theStar->mR, theStar->mG,
			theStar->mB, theStar->mAlpha, &theStar->mAnim->mPalette, aClip);
		anImage = theStar->mImage;
		aDirtyClip = aClip;
	}
	Dirty_AddClippedImage(&gDirty, (int)((double)theStar->mX + (double)aSwayX), (int)theStar->mY, anImage, aDirtyClip);
}

} // namespace wb
