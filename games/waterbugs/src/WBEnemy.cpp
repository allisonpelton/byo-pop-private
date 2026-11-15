// The enemies (0x405070..0x40ace0): rollers bounce about the open cells, walkers keep to the edges; a beam an enemy
// touches gets a beamrider that runs back along it to the ship

#include "WBLevel.h"
#include "R64Blit.h"

#include <cmath>
#include <cstdio>
#include <cstring>

namespace wb
{

Enemy* Enemy_Init(Enemy* theEnemy)
{
	Obj_Init(theEnemy);
	theEnemy->mLastShot = 0.0f;
	theEnemy->mShield = nullptr;
	theEnemy->mPhase = 0;
	int aTime = RandRange(2000, 3000);
	theEnemy->mLastTurn = 0;
	theEnemy->mCountdown = 0;
	theEnemy->mShotDelay = aTime;
	return theEnemy;
}

static bool IsType(const XmlTag* theTag, const char* theType)
{
	return std::strcmp(Xml_GetString(theTag, "type"), theType) == 0;
}

// 0x4050f0: an enemy type from its tag; Water Bugs numbers the kinds differently from Cosmic Bugs (the roller is 0x11)
void Enemy_FromXml(Enemy* theEnemy, const char* p, const XmlTag* theTag)
{
	Obj_FromXml(theEnemy, p, theTag);
	theEnemy->mAcceleration = (float)Xml_GetFloat(theTag, "acceleration");
	theEnemy->mDecay = (float)Xml_GetFloat(theTag, "decay");
	theEnemy->mTurnSpeed = (float)Xml_GetFloat(theTag, "turn_speed");
	if (IsType(theTag, "roller"))
		theEnemy->mKind = ENEMY_ROLLER;
	else if (IsType(theTag, "burst_roller"))
		theEnemy->mKind = ENEMY_BURST_ROLLER;
	else if (IsType(theTag, "jellyfish_roller"))
	{
		theEnemy->mKind = ENEMY_JELLYFISH_ROLLER;
		theEnemy->mFirstParticle = Bank_IndexOf(&gArtBank, "bubble01");
		theEnemy->mLastParticle = Bank_IndexOf(&gArtBank, "bubble02");
	}
	else if (IsType(theTag, "teleporter_roller"))
	{
		theEnemy->mKind = ENEMY_TELEPORTER_ROLLER;
		theEnemy->mFirstParticle = Bank_IndexOf(&gArtBank, "bubble01");
		theEnemy->mLastParticle = Bank_IndexOf(&gArtBank, "bubble02");
	}
	else if (IsType(theTag, "spin_roller"))
	{
		theEnemy->mKind = ENEMY_SPIN_ROLLER;
		theEnemy->mFirstParticle = Bank_IndexOf(&gArtBank, "particle_0");
		theEnemy->mLastParticle = Bank_IndexOf(&gArtBank, "particle_0");
	}
	else if (IsType(theTag, "turn_roller"))
		theEnemy->mKind = ENEMY_TURN_ROLLER;
	else if (IsType(theTag, "bump_roller"))
	{
		theEnemy->mKind = ENEMY_BUMP_ROLLER;
		theEnemy->mFirstParticle = Bank_IndexOf(&gArtBank, "particle_0");
		theEnemy->mLastParticle = Bank_IndexOf(&gArtBank, "particle_0");
	}
	else if (IsType(theTag, "snake_shooter"))
	{
		theEnemy->mFirstParticle = Bank_IndexOf(&gArtBank, "exp_0");
		theEnemy->mLastParticle = Bank_IndexOf(&gArtBank, "exp_4");
		theEnemy->mKind = ENEMY_SNAKE_SHOOTER;
	}
	else if (IsType(theTag, "line_walker"))
	{
		theEnemy->mKind = ENEMY_LINE_WALKER;
		theEnemy->mShadow = 1;
	}
	else if (IsType(theTag, "scurry_walker"))
	{
		theEnemy->mKind = ENEMY_SCURRY_WALKER;
		theEnemy->mShadow = 1;
	}
	else if (IsType(theTag, "smart_walker"))
	{
		theEnemy->mKind = ENEMY_SMART_WALKER;
		theEnemy->mShadow = 1;
	}
	else if (IsType(theTag, "cannon_walker"))
	{
		theEnemy->mFirstParticle = Bank_IndexOf(&gArtBank, "smoke_0001");
		theEnemy->mLastParticle = Bank_IndexOf(&gArtBank, "smoke_0009");
		theEnemy->mKind = ENEMY_CANNON_WALKER;
		theEnemy->mShadow = 1;
	}
	else if (IsType(theTag, "laser_walker"))
	{
		theEnemy->mKind = ENEMY_LASER_WALKER;
		theEnemy->mShadow = 1;
		theEnemy->mFirstParticle = Bank_IndexOf(&gArtBank, "particle_4");
		theEnemy->mLastParticle = Bank_IndexOf(&gArtBank, "particle_4");
	}
	else if (IsType(theTag, "crab_walker"))
	{
		theEnemy->mKind = ENEMY_CRAB_WALKER;
		theEnemy->mShadow = 1;
		theEnemy->mFirstParticle = Bank_IndexOf(&gArtBank, "particle_4");
		theEnemy->mLastParticle = Bank_IndexOf(&gArtBank, "particle_4");
	}
	else if (IsType(theTag, "lobster_walker"))
	{
		theEnemy->mKind = ENEMY_LOBSTER_WALKER;
		theEnemy->mShadow = 1;
		theEnemy->mFirstParticle = Bank_IndexOf(&gArtBank, "particle_4");
		theEnemy->mLastParticle = Bank_IndexOf(&gArtBank, "particle_4");
	}
	else if (IsType(theTag, "turtle_walker"))
	{
		theEnemy->mKind = ENEMY_TURTLE_WALKER;
		theEnemy->mShadow = 1;
		theEnemy->mFirstParticle = Bank_IndexOf(&gArtBank, "exp_2");
		theEnemy->mLastParticle = Bank_IndexOf(&gArtBank, "exp_4");
	}
	else if (IsType(theTag, "frog"))
		theEnemy->mKind = ENEMY_FROG;
	if (Xml_GetString(theTag, "left_animation") != nullptr)
	{
		theEnemy->mLeftAnim = Animation_Find(Xml_GetString(theTag, "left_animation"), &theEnemy->mFrameCount,
			&theEnemy->mFrameTime);
		theEnemy->mAnim = theEnemy->mLeftAnim;
	}
	if (Xml_GetString(theTag, "right_animation") != nullptr)
		theEnemy->mRightAnim = Animation_Find(Xml_GetString(theTag, "right_animation"), &theEnemy->mFrameCount,
			&theEnemy->mFrameTime);
	if (Xml_GetString(theTag, "up_animation") != nullptr)
		theEnemy->mUpAnim = Animation_Find(Xml_GetString(theTag, "up_animation"), &theEnemy->mFrameCount,
			&theEnemy->mFrameTime);
	if (Xml_GetString(theTag, "down_animation") != nullptr)
		theEnemy->mDownAnim = Animation_Find(Xml_GetString(theTag, "down_animation"), &theEnemy->mFrameCount,
			&theEnemy->mFrameTime);
	theEnemy->mBaseSpeed = theEnemy->mSpeed;
	theEnemy->mState = ENEMY_APPEARING;
	theEnemy->mDepth = 3;
	theEnemy->mBaseFrameTime = (float)theEnemy->mFrameTime;
	theEnemy->mBaseAcceleration = theEnemy->mAcceleration;
}

void Enemy_Draw(Enemy* theEnemy)
{
	if (theEnemy->mShield != nullptr)
		Obj_Draw(theEnemy->mShield);
	Obj_Draw(theEnemy);
}

static int Sign(float theValue)
{
	if (theValue == 0.0f)
		return 0;
	if (theValue <= 0.0f)
		return -1;
	return 1;
}

// A third of its width ahead in x, and a third of its height ahead in y: a cell there that is not open turns it
// back on that axis; inside one it turns back on both
int Enemy_Bounce(Enemy* theEnemy)
{
	int aCol = (int)theEnemy->mX / CELL_SIZE;
	int aRow = (int)theEnemy->mY / CELL_SIZE;
	if (gLevels.mCurrent->mColumns[aCol][aRow].mKind == CELL_OPEN)
	{
		Image* anImage = theEnemy->mImage;
		int anAhead = (int)((double)((anImage->mW * Sign(theEnemy->mVx)) / 3) + (double)theEnemy->mX);
		Cell* aCell = &gLevels.mCurrent->mColumns[anAhead / CELL_SIZE][aRow];
		if (aCell->mKind != CELL_OPEN)
		{
			theEnemy->mHitCell = aCell;
			theEnemy->mVx = -theEnemy->mVx;
		}
		anAhead = (int)((double)((anImage->mH * Sign(theEnemy->mVy)) / 3) + (double)theEnemy->mY);
		aCell = &gLevels.mCurrent->mColumns[aCol][anAhead / CELL_SIZE];
		if (aCell->mKind != CELL_OPEN)
		{
			theEnemy->mHitCell = aCell;
			theEnemy->mVy = -theEnemy->mVy;
		}
	}
	if (gLevels.mCurrent->mColumns[aCol][aRow].mKind != CELL_OPEN)
	{
		theEnemy->mVx = -theEnemy->mVx;
		theEnemy->mVy = -theEnemy->mVy;
	}
	return 0;
}

// Speeds up over frames 6 to 10 of its animation, the decay slowing it the rest of the time
int BurstRoller_Move(Enemy* theEnemy)
{
	theEnemy->mAcceleration = (theEnemy->mFrame < 6 || 10 < theEnemy->mFrame) ? 1.0002f : 1.0141f;
	float aDecayed = theEnemy->mDecay * theEnemy->mVx;
	theEnemy->mVx = aDecayed * theEnemy->mAcceleration;
	aDecayed = theEnemy->mDecay * theEnemy->mVy;
	theEnemy->mVy = aDecayed * theEnemy->mAcceleration;
	return Enemy_Bounce(theEnemy);
}

// Sinks, and over frames 40 to 65 rises, now and then letting a bubble go
int Jellyfish_Move(Enemy* theEnemy)
{
	if (theEnemy->mFrame < 0x28 || 0x41 < theEnemy->mFrame)
		theEnemy->mAcceleration = 0.00058f;
	else
	{
		theEnemy->mAcceleration = -0.001f;
		if (RandRange(0, 0x13) == 0)
		{
			ParticleSystem_New();
			int aSign = Sign(theEnemy->mVx);
			int aFirst = theEnemy->mFirstParticle;
			int aLifetime = RandRange(600, 800);
			float aVy = (float)((double)RandRange(-30, 30) / 200.0);
			float aVx = (float)((double)RandRange(-10, 10) / 200.0);
			float aY = (float)RandRange(-20, 0) + theEnemy->mY;
			float aX = (float)((double)RandRange(-10, 10) + (double)theEnemy->mX + (double)(aSign * 3));
			ParticleSystem_Add(gLevels.mCurrent->mSystems.mCurrent, aX, aY, aVx, aVy, aFirst, aFirst, 0, 0xff, 0xff,
				0xff, 0xff, 0xff, 0xff, 0x104, 10, aLifetime, 0.998f, -0.0003f, 5);
		}
	}
	float aDecayed = theEnemy->mDecay * theEnemy->mVy;
	theEnemy->mVy = aDecayed + theEnemy->mAcceleration;
	return Enemy_Bounce(theEnemy);
}

// 0x405a20 has this twice: a bounce in x off a beam's line (kind 2) pushes the line's column one cell on into the
// filled cells behind it, a bounce in y its row. The line has to have open cells on the roller's side and two filled
// ones behind it all along, and end on four cells of one kind at both ends.
static void BumpRoller_Push(Cell* theHit, int theD, bool theInX)
{
	int aLine = theInX ? theHit->mCol : theHit->mRow;
	int aStart = theInX ? theHit->mRow : theHit->mCol;
	auto aCellAt = [&](int theAlong, int theOff) -> Cell* {
		if (theInX)
			return &gLevels.mCurrent->mColumns[aLine + theOff][theAlong];
		return &gLevels.mCurrent->mColumns[theAlong][aLine + theOff];
	};
	if (aCellAt(aStart, theD)->mKind != CELL_FILLED)
		return;
	bool anOk = true;
	int aMin = 999;
	int aMax = -999;
	for (int aDir = -1; aDir < 2; aDir = aDir + 2)
	{
		int anAlong = aStart;
		int aBefore, anOn, aBehind, aBehind2;
		while (true)
		{
			if (aCellAt(anAlong, 0)->mState == 5)
				anOk = false;
			aBefore = aCellAt(anAlong, -theD)->mKind;
			anOn = aCellAt(anAlong, 0)->mKind;
			aBehind = aCellAt(anAlong, theD)->mKind;
			aBehind2 = aCellAt(anAlong, theD * 2)->mKind;
			if (aBefore != 0 || anOn != 2 || aBehind != 4 || aBehind2 != 4)
				break;
			anAlong = anAlong + aDir;
		}
		if (anAlong < aMin)
			aMin = anAlong;
		if (aMax < anAlong)
			aMax = anAlong;
		if (aBefore != anOn || anOn != aBehind || aBehind != aBehind2)
			anOk = false;
	}
	if (!(aMin + 1 < aMax + -1 && anOk))
		return;
	// (the original works out a pan from x for a play call that takes none)
	Sound_Play(Sound_Find("audio\\bump.wav"));
	for (int anAlong = aMin + 1; anAlong <= aMax + -1; anAlong++)
	{
		gLevels.mCurrent->mFilledCells = gLevels.mCurrent->mFilledCells + -1;
		aCellAt(anAlong, 0)->mKind = CELL_OPEN;
		aCellAt(anAlong, 0)->mImage = gLevels.mCurrent->mEmpty;
		Cell* aCell = aCellAt(anAlong, 0);
		int aY = (int)aCell->mY;
		int aX = (int)aCell->mX;
		Image* aBack = gLevels.mCurrent->mBackground;
		Copy16(gBackBuffer.mBits, gBackBuffer.mPitch, aX, aY, aBack->mBits, aBack->mPitch, aX, aY, 0x10, 0x10);
		Cell_Redraw(aCellAt(anAlong, 0));
		aCellAt(anAlong, theD)->mKind = CELL_LINE;
		ParticleSystem_New();
		int aLifetime = RandRange(700, 0x44c);
		int aLast = Bank_IndexOf(&gArtBank, "exp_4");
		int aFirst = Bank_IndexOf(&gArtBank, "exp_0");
		aFirst = aFirst + RandRange(0, 3);
		float aVy = (float)((double)RandRange(-100, 100) / 1000.0);
		float aVx = (float)((double)RandRange(-100, 100) / 1000.0);
		float aPy = (float)RandRange(-10, 10) + aCellAt(anAlong, 0)->mY;
		float aPx = (float)RandRange(-10, 10) + aCellAt(anAlong, 0)->mX;
		ParticleSystem_Add(gLevels.mCurrent->mSystems.mCurrent, aPx, aPy, aVx, aVy, aFirst, aLast, 0, 0xff, 0xff, 0xff,
			0xe7, 0xa6, 0x39, 0x21c, 10, aLifetime, 1.0f, 0.0f, 5);
	}
	for (int anAlong = aMin; anAlong <= aMax; anAlong++)
	{
		Cell_PickImage(aCellAt(anAlong, theD));
		Cell_Redraw(aCellAt(anAlong, theD));
		Cell* aCell = aCellAt(anAlong, theD);
		Dirty_AddClippedShown(&gDirty, (int)aCell->mX, (int)aCell->mY, 0x10, 0x10, SCREEN_CLIP);
		if (anAlong == aMin || anAlong == aMax)
		{
			Cell_PickImage(aCellAt(anAlong, 0));
			Cell_Redraw(aCellAt(anAlong, 0));
		}
		aCell = aCellAt(anAlong, 0);
		Dirty_AddClippedShown(&gDirty, (int)aCell->mX, (int)aCell->mY, 0x10, 0x10, SCREEN_CLIP);
	}
}

// A bounce that turned it back in x (else in y) off a beam's line pushes the line
int BumpRoller_Move(Enemy* theEnemy)
{
	int aResult = Enemy_Bounce(theEnemy);
	Cell* aHit = theEnemy->mHitCell;
	if (aHit != nullptr)
	{
		if (1.0f < theEnemy->mAge && Sign(theEnemy->mVx) != Sign(theEnemy->mPrevVx))
		{
			if (aHit->mKind == CELL_LINE)
				BumpRoller_Push(aHit, Sign(theEnemy->mPrevVx), true);
		}
		else if (1.0f < theEnemy->mAge && Sign(theEnemy->mVy) != Sign(theEnemy->mPrevVy))
		{
			if (aHit->mKind == CELL_LINE)
				BumpRoller_Push(aHit, Sign(theEnemy->mPrevVy), false);
		}
	}
	theEnemy->mPrevVx = theEnemy->mVx;
	theEnemy->mPrevVy = theEnemy->mVy;
	return aResult;
}

// Winds up (its frame time falls to 3), rolls with sparks until it is fast, then winds down again
void SpinRoller_Move(Enemy* theEnemy)
{
	int aPhase = theEnemy->mPhase;
	if (aPhase == 0)
	{
		theEnemy->mAcceleration = 1.0f;
		if ((int)theEnemy->mAge % 100 == 0)
			theEnemy->mFrameTime = theEnemy->mFrameTime + -1;
		if (theEnemy->mFrameTime < 4)
			theEnemy->mPhase = 1;
	}
	else if (aPhase == 1)
	{
		theEnemy->mAcceleration = theEnemy->mBaseAcceleration;
		if (RandRange(1, 10) == 1)
		{
			ParticleSystem_New();
			int aLifetime = RandRange(400, 500);
			int aLast = theEnemy->mLastParticle;
			int aFirst = theEnemy->mFirstParticle;
			float aVy = (float)((double)RandRange(-50, 50) / 200.0);
			float aVx = (float)((double)RandRange(-50, 50) / 200.0);
			float aY = (float)RandRange(-5, 5) + theEnemy->mY;
			float aX = (float)RandRange(-5, 5) + theEnemy->mX;
			ParticleSystem_Add(gLevels.mCurrent->mSystems.mCurrent, aX, aY, aVx, aVy, aFirst, aLast, 0, 0xff, 0xff,
				0xff, 0xff, 0xff, 0, 400, 10, aLifetime, 0.999f, 0.0003f, 5);
		}
		double aVx2 = (double)theEnemy->mVx * (double)theEnemy->mVx;
		double aVy2 = (double)theEnemy->mVy * (double)theEnemy->mVy;
		if (0.6 < aVx2 + aVy2)
			theEnemy->mPhase = 2;
	}
	else if (aPhase == 2)
	{
		theEnemy->mAcceleration = 1.0f;
		if ((int)theEnemy->mAge % 0x50 == 0)
			theEnemy->mFrameTime = theEnemy->mFrameTime + 1;
		if (theEnemy->mBaseFrameTime <= (float)theEnemy->mFrameTime)
			theEnemy->mPhase = 0;
	}
	float aDecayed = theEnemy->mDecay * theEnemy->mVx;
	theEnemy->mVx = aDecayed * theEnemy->mAcceleration;
	aDecayed = theEnemy->mDecay * theEnemy->mVy;
	theEnemy->mVy = aDecayed * theEnemy->mAcceleration;
	Enemy_Bounce(theEnemy);
}

// Hops along its row (frames 19 on of its animation are the hop); two cells short of anything not open it lands,
// turned round, sits for 1000 updates and jumps again
int Frog_Move(Enemy* theEnemy)
{
	int aPhase = theEnemy->mPhase;
	if (aPhase != 0)
	{
		if (aPhase == 1)
		{
			theEnemy->mFrame = 0;
			theEnemy->mFrameTimer = 0;
			if (theEnemy->mAge <= 1000.0f)
				return 1;
			theEnemy->mPhase = 2;
			Sound_Play(Sound_Find("audio\\frog_jump.wav"));
			return 1;
		}
		if (aPhase == 2 && 8 <= theEnemy->mFrame)
			theEnemy->mPhase = 0;
		return 1;
	}
	if (theEnemy->mFrame < 2 ||
		(theEnemy->mFrame == theEnemy->mFrameCount + -1 && theEnemy->mFrameTimer == theEnemy->mFrameTime + -1))
	{
		theEnemy->mFrame = 0x13;
		theEnemy->mFrameTimer = 0;
	}
	if (theEnemy->mAnim == theEnemy->mLeftAnim)
		theEnemy->mVx = -theEnemy->mSpeed;
	else if (theEnemy->mAnim == theEnemy->mRightAnim)
		theEnemy->mVx = theEnemy->mSpeed;
	int aSx = Sign(theEnemy->mVx);
	int aSy = Sign(theEnemy->mVy);
	int aRow = (int)theEnemy->mY / CELL_SIZE;
	int aCol = (int)theEnemy->mX / CELL_SIZE;
	if (gLevels.mCurrent->mColumns[aCol + aSx * 2][aRow + aSy * 2].mKind == CELL_OPEN)
		return 1;
	if (theEnemy->mVx < 0.0f)
	{
		theEnemy->mX = theEnemy->mX - 8.0f;
		theEnemy->mAnim = theEnemy->mRightAnim;
	}
	else if (0.0f < theEnemy->mVx)
	{
		theEnemy->mAnim = theEnemy->mLeftAnim;
		theEnemy->mX = theEnemy->mX + 8.0f;
	}
	theEnemy->mVx = 0.0f;
	theEnemy->mFrameTimer = 0;
	theEnemy->mVy = 0.0f;
	theEnemy->mFrame = 0;
	theEnemy->mPhase = 1;
	theEnemy->mAge = 1.0f;
	Sound_Play(Sound_Find("audio\\frog_land.wav"));
	return 1;
}

// Kind 0xe, a laser walker's shot: bursts in a ring inside a cell that is not open (0: done)
int LaserShot_Move(Enemy* theEnemy)
{
	int aCol = (int)theEnemy->mX / CELL_SIZE;
	int aRow = (int)theEnemy->mY / CELL_SIZE;
	if (gLevels.mCurrent->mColumns[aCol][aRow].mKind != CELL_OPEN)
	{
		for (int n = 0x12; n != 0; n--)
		{
			int anAngle = RandRange(0, 0x167);
			float aVx = (float)((double)CosDeg(anAngle) * 0.11999999731779099);
			float aVy = (float)((double)SinDeg(anAngle) * 0.11999999731779099);
			ParticleSystem_New();
			ParticleSystem_Add(gLevels.mCurrent->mSystems.mCurrent, theEnemy->mX, theEnemy->mY, aVx, aVy,
				theEnemy->mFirstParticle, theEnemy->mLastParticle, 0, 0xff, 0x80, 0x80, 0xff, 0x80, 0x80, 0xf0, 10, 600,
				0.999f, 0.0005f, 5);
		}
		return 0;
	}
	return 1;
}

// A cell around it is being captured
int Enemy_NearCapture(Enemy* theEnemy)
{
	int aCol = (int)theEnemy->mX / CELL_SIZE;
	int aRow = (int)theEnemy->mY / CELL_SIZE;
	for (int r = aRow - 1; r <= aRow + 1; r++)
	{
		for (int k = aCol - 1; k <= aCol + 1; k++)
		{
			if ((uint32_t)k < GRID_COLUMNS && (uint32_t)r < GRID_ROWS && gLevels.mCurrent->mColumns[k][r].mState == 5)
				return 1;
		}
	}
	return 0;
}

// Its animation by the way it goes
void Enemy_Face(Enemy* theEnemy)
{
	if (theEnemy->mVx < 0.0f)
	{
		theEnemy->mAnim = theEnemy->mLeftAnim;
		return;
	}
	if (0.0f < theEnemy->mVx)
	{
		theEnemy->mAnim = theEnemy->mRightAnim;
		return;
	}
	if (theEnemy->mVy < 0.0f)
	{
		theEnemy->mAnim = theEnemy->mDownAnim;
		return;
	}
	if (0.0f < theEnemy->mVy)
		theEnemy->mAnim = theEnemy->mUpAnim;
}

void Enemy_UpdateShield(Enemy* theEnemy)
{
	if (theEnemy->mShield != nullptr)
	{
		theEnemy->mShield->mX = theEnemy->mX;
		theEnemy->mShield->mY = theEnemy->mY;
		if (Obj_Update(theEnemy->mShield) == 0)
		{
			if (theEnemy->mShield != nullptr)
				Obj_Delete(theEnemy->mShield);
			theEnemy->mShield = nullptr;
		}
	}
}

// A shield flash where a beam touched it (again from the start if it is showing)
void Enemy_Shield(Enemy* theEnemy)
{
	if (theEnemy->mShield == nullptr)
	{
		Obj* aShield = (Obj*)::operator new(sizeof(Obj));
		Obj_Init(aShield);
		theEnemy->mShield = aShield;
		aShield->mX = theEnemy->mX;
		// (sic: the original's y is the enemy's x, until the next Enemy_UpdateShield)
		theEnemy->mShield->mY = theEnemy->mX;
		theEnemy->mShield->mVx = 0.0f;
		theEnemy->mShield->mVy = 0.0f;
		theEnemy->mShield->mAnim = Animation_Find("enemy_shield", &theEnemy->mShield->mFrameCount,
			&theEnemy->mShield->mFrameTime);
		theEnemy->mShield->mMidHandle = 1;
		theEnemy->mShield->mDrawKind = 2;
		theEnemy->mShield->mDirty = 1;
		theEnemy->mShield->mOneShot = 1;
		return;
	}
	theEnemy->mShield->mFrame = 0;
}

// 0x407070: No other roller (kind 0x11 or 0x12) within 120 pixels
int Enemy_ClearOfRollers(Enemy* theEnemy)
{
	int aResult = 1;
	for (Enemy* anEnemy = gLevels.mCurrent->mEnemies.mHead; anEnemy != nullptr; anEnemy = Node_Next(anEnemy))
	{
		if (anEnemy != theEnemy && 0x10 < anEnemy->mKind && anEnemy->mKind < 0x13)
		{
			double aDy = (double)theEnemy->mY - (double)anEnemy->mY;
			double aDx = (double)theEnemy->mX - (double)anEnemy->mX;
			if ((int)(aDx * aDx + aDy * aDy) < 0x3840)
				aResult = 0;
		}
	}
	return aResult;
}

// No other walker within 100 pixels (Water Bugs' kind test can never hold: the walkers' kinds were renumbered and this
// function was not)
int Enemy_ClearOfWalkers(Enemy* theEnemy)
{
	int aResult = 1;
	for (Enemy* anEnemy = gLevels.mCurrent->mEnemies.mHead; anEnemy != nullptr; anEnemy = Node_Next(anEnemy))
	{
		if (anEnemy != theEnemy && 0xf < anEnemy->mKind && anEnemy->mKind < 0xb)
		{
			double aDy = (double)theEnemy->mY - (double)anEnemy->mY;
			double aDx = (double)theEnemy->mX - (double)anEnemy->mX;
			if ((int)(aDx * aDx + aDy * aDy) < 10000)
				aResult = 0;
		}
	}
	return aResult;
}

// The way to the board's middle (theMiddle) from theValue; away from it on the middle line
static int Towards(int theValue, int theMiddle)
{
	int aDir = theMiddle - theValue == 0 ? 0 : (0 < theMiddle - theValue ? 1 : -1);
	if (aDir == 0)
		aDir = -1;
	return aDir;
}

// A roller somewhere with only open cells within 3 and no other roller near, heading off at random at its speed:
// up to 30 random cells, each walked towards the middle in x or in y (0: no room)
int Enemy_Place(Enemy* theEnemy)
{
	int aTry = 0;
	do
	{
		int aCol = RandRange(6, 0x2b);
		int aRow = RandRange(6, 0x19);
		int aDx = Towards(aCol, 0x19);
		int aDy = Towards(aRow, 0x12);
		if (RandRange(0, 99) < 0x32)
			aDy = 0;
		else
			aDx = 0;
		while (6 < aCol)
		{
			if (0x2a < aCol || 0x11 < (uint32_t)(aRow - 3 - 4))
				break;
			bool aBlocked = false;
			for (int r = aRow - 3; r <= aRow + 3; r++)
			{
				for (int c = aCol - 3; c < aCol + 3; c++)
				{
					if (gLevels.mCurrent->mColumns[c][r].mKind != CELL_OPEN)
						aBlocked = true;
				}
			}
			if (!aBlocked)
			{
				theEnemy->mX = (float)(aCol * CELL_SIZE + CELL_HALF);
				theEnemy->mY = (float)(aRow * CELL_SIZE + CELL_HALF);
				if (Enemy_ClearOfRollers(theEnemy) != 0)
				{
					int anAngle = RandRange(0, 0x167);
					float aSquare = (float)((double)theEnemy->mVx * theEnemy->mVx + (double)theEnemy->mVy *
						theEnemy->mVy);
					float aSpeed = (float)std::sqrt((double)aSquare);
					theEnemy->mVx = CosDeg(anAngle) * aSpeed;
					theEnemy->mVy = SinDeg(anAngle) * aSpeed;
					return 1;
				}
			}
			aCol = aCol + aDx;
			aRow = aRow + aDy;
		}
		aTry = aTry + 1;
	} while (aTry <= 0x1d);
	return 0;
}

void Enemy_Append(List<Enemy>* theList)
{
	Enemy* anEnemy = (Enemy*)::operator new(sizeof(Enemy));
	List_Append(theList, Enemy_Init(anEnemy));
}

void Powerup_Insert(List<Powerup>* theList)
{
	Powerup* aPowerup = (Powerup*)::operator new(sizeof(Powerup));
	List_InsertBefore(theList, Powerup_Init(aPowerup));
}

void Enemy_Destroy(Enemy* theEnemy)
{
	if (theEnemy->mShield != nullptr)
		Obj_Delete(theEnemy->mShield);
	Obj_Destroy(theEnemy);
}

void List_DeleteNode(Enemy* theEnemy)
{
	Enemy_Destroy(theEnemy);
	::operator delete(theEnemy);
}

// Steers towards the ship (turn_speed keeps it on its way) and away from the other turn rollers, at its speed
void TurnRoller_Move(Enemy* theEnemy)
{
	Shooter* aShip = gLevels.mCurrent->mShooters.mCurrent;
	if (aShip != nullptr)
	{
		float aSquare = (float)((double)theEnemy->mVx * theEnemy->mVx + (double)theEnemy->mVy * theEnemy->mVy);
		float aSpeed = (float)std::sqrt((double)aSquare);
		float aX = (float)(((double)aShip->mX - theEnemy->mX) / 12000.0 + (double)theEnemy->mTurnSpeed * theEnemy->mVx);
		float aY = (float)(((double)aShip->mY - theEnemy->mY) / 12000.0 + (double)theEnemy->mTurnSpeed * theEnemy->mVy);
		List_Each(&gLevels.mCurrent->mEnemies, [&](Enemy* theAnOther) {
			if (theAnOther != theEnemy && theAnOther->mKind == ENEMY_JELLYFISH_ROLLER)
			{
				float aDx = theAnOther->mX - theEnemy->mX;
				float aDy = theAnOther->mY - theEnemy->mY;
				float aSum = (float)((double)aDx * aDx + (double)aDy * aDy);
				float aDist = std::fabs(aSum) * 70.0f;
				if (aDist != 0.0f)
				{
					float aT = (float)((double)aX - (double)aDx / aDist);
					aX = (float)((double)aT - (double)aDy / aDist);
				}
			}
		});
		gLevels.mCurrent->mEnemies.mCurrent = theEnemy;
		float anAngle = (float)std::atan2((double)aY, (double)aX);
		float aCos = (float)std::cos((double)anAngle);
		theEnemy->mVx = aCos * aSpeed;
		float aSin = (float)std::sin((double)anAngle);
		theEnemy->mVy = aSin * aSpeed;
	}
	Enemy_Bounce(theEnemy);
}

// 0x478608: the order Walker_Turn goes round the ways in (0 right, 1 left, 2 down, 3 up); 0x460a0c: not set up yet
static int gSWays[4];
static int gSWaysUnset = 1;

// Stops, then tries up to four ways round from a random one for the first along an edge or a beam's line
void Walker_Turn(Enemy* theEnemy)
{
	if (gSWaysUnset != 0)
	{
		gSWays[0] = 0;
		gSWays[1] = 2;
		gSWays[2] = 1;
		gSWays[3] = 3;
		gSWaysUnset = 0;
	}
	theEnemy->mVx = 0.0f;
	theEnemy->mVy = 0.0f;
	int aCol = (int)theEnemy->mX / CELL_SIZE;
	int aRow = (int)theEnemy->mY / CELL_SIZE;
	int aWay = RandRange(0, 0x27) / 10;
	int aTry = 0;
	do
	{
		if (theEnemy->mVx != 0.0f || theEnemy->mVy != 0.0f)
			return;
		aWay = aWay + 1;
		if (3 < aWay)
			aWay = 0;
		int aKind;
		switch (gSWays[aWay])
		{
		case 0:
			if (aCol < 0x31)
			{
				aKind = gLevels.mCurrent->mColumns[aCol + 1][aRow].mKind;
				if (aKind == CELL_EDGE || aKind == CELL_LINE)
					theEnemy->mVx = theEnemy->mSpeed;
			}
			break;
		case 1:
			if (0 < aCol)
			{
				aKind = gLevels.mCurrent->mColumns[aCol - 1][aRow].mKind;
				if (aKind == CELL_EDGE || aKind == CELL_LINE)
					theEnemy->mVx = -theEnemy->mSpeed;
			}
			break;
		case 2:
			if (aRow < 0x24)
			{
				aKind = gLevels.mCurrent->mColumns[aCol][aRow + 1].mKind;
				if (aKind == CELL_EDGE || aKind == CELL_LINE)
					theEnemy->mVy = theEnemy->mSpeed;
			}
			break;
		case 3:
			if (0 < aRow)
			{
				aKind = gLevels.mCurrent->mColumns[aCol][aRow - 1].mKind;
				if (aKind == CELL_EDGE || aKind == CELL_LINE)
					theEnemy->mVy = -theEnemy->mSpeed;
			}
			break;
		}
		Enemy_Face(theEnemy);
		aTry = aTry + 1;
	} while (aTry <= 3);
}

// Kind 0xe (a laser walker's shot) on a beam: the ship's beams all burst into sparks along their length, and it
// loses them
int LaserShot_HitBeams(Enemy* theEnemy)
{
	(void)theEnemy;
	// (the original works out a pan from x for a play call that takes none)
	Sound_Play(Sound_Find("audio\\beam_hit.wav"));
	List_Each(&gLevels.mCurrent->mShooters.mCurrent->mBeams, [&](Beam* theABeam) {
		int aLength = (int)(std::fabs((double)theABeam->mStartX - theABeam->mX) +
			std::fabs((double)theABeam->mStartY - theABeam->mY));
		for (int i = 0; i < aLength; i++)
		{
			int anAngle = RandRange(0, 0x167);
			float aCos = CosDeg(anAngle);
			float aSin = SinDeg(anAngle);
			int aSx = Sign(theABeam->mVx);
			int aSy = Sign(theABeam->mVy);
			ParticleSystem_New();
			Cell* aCell = gLevels.mCurrent->mColumns[0];
			ParticleSystem_Add(gLevels.mCurrent->mSystems.mCurrent, (float)(aSx * i) + theABeam->mStartX,
				(float)(aSy * i) + theABeam->mStartY, (float)((double)aCos * 0.11999999731779099),
				(float)((double)aSin * 0.11999999731779099), aCell->mFirstParticle, aCell->mLastParticle, 0, 0xff, 0xff,
				0xff, 0xff, 0xff, 0xff, 0x21c, 10, 700, 0.999f, 0.0001f, 5);
		}
	});
	Shooter_DropBeams(gLevels.mCurrent->mShooters.mCurrent);
	return 0;
}

// A walker somewhere on an edge cell (kind 3) with no other walker near: up to 10 random cells, each walked towards
// the middle in x or in y (0: no room)
int Enemy_PlaceFirst(Enemy* theEnemy)
{
	int aTry = 0;
	do
	{
		int aCol = RandRange(0, 0x31);
		int aRow = RandRange(0, 0x24);
		int aDx = Towards(aCol, 0x19);
		int aDy = Towards(aRow, 0x12);
		if (RandRange(0, 99) < 0x32)
			aDy = 0;
		else
			aDx = 0;
		while (0 < aCol)
		{
			if (0x31 < aCol || 0x23 < (uint32_t)(aRow - 1))
				break;
			if (gLevels.mCurrent->mColumns[aCol][aRow].mKind == CELL_EDGE)
			{
				theEnemy->mX = (float)(aCol * CELL_SIZE + CELL_HALF);
				theEnemy->mY = (float)(aRow * CELL_SIZE + CELL_HALF);
				if (Enemy_ClearOfWalkers(theEnemy) != 0)
					return 1;
			}
			aCol = aCol + aDx;
			aRow = aRow + aDy;
		}
		aTry = aTry + 1;
	} while (aTry <= 9);
	return 0;
}

void Text_Append(List<Text>* theList)
{
	Text* aText = (Text*)::operator new(sizeof(Text));
	List_Append(theList, Text_Init(aText));
}

// 60 sparkles flying off from where it is (0x407e90 has this twice)
static void Teleporter_Sparkles(Enemy* theEnemy)
{
	for (int n = 0x3c; n != 0; n--)
	{
		int anAngle = RandRange(0, 0x167);
		float aScale = (float)((double)RandRange(0, 100) / 560.0);
		float aVx = CosDeg(anAngle) * aScale;
		float aVy = SinDeg(anAngle) * aScale;
		ParticleSystem_New();
		int aLifetime = RandRange(400, 900);
		ParticleSystem_Add(gLevels.mCurrent->mSystems.mCurrent, theEnemy->mX, theEnemy->mY, aVx, aVy, 0, 0, 0, 0xff,
			0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0, aLifetime, 1.0f, 0.0f, 4);
		Particle* aParticle = gLevels.mCurrent->mSystems.mCurrent->mParticles.mCurrent;
		int aFrameTime;
		aParticle->mAnim = Animation_Find("sparkles3", &aParticle->mFrameCount, &aFrameTime);
		aParticle->mTint = RandRange(0, 7);
	}
}

// Kind 0x10: loops the first 32 frames of its animation for 5000 updates, its children circling it for the first
// 2000; then plays on, and past frame 48 vanishes in sparkles to come back somewhere else, a trail of them between
int Teleporter_Move(Enemy* theEnemy)
{
	if (theEnemy->mPhase == 0)
	{
		if (theEnemy->mAge <= 5000.0f)
		{
			if (0x1f < theEnemy->mFrame)
			{
				theEnemy->mFrame = 0;
				theEnemy->mFrameTimer = 0;
			}
		}
		else if (0x1f < theEnemy->mFrame)
			theEnemy->mPhase = 1;
		if (2000.0f <= theEnemy->mAge || gLevels.mCurrent->mAge <= theEnemy->mAge)
		{
			bool aMore;
			do
				aMore = List_RemoveCurrent(&theEnemy->mChildren);
			while (aMore);
			return Enemy_Bounce(theEnemy);
		}
		double anAlpha = (double)theEnemy->mAge * 0.25;
		if (255.0 < anAlpha)
			anAlpha = 255.0;
		theEnemy->mAlpha = (int)anAlpha;
		int anOffset = 0;
		List_Each(&theEnemy->mChildren, [&](Obj* theAChild) {
			int anAngle = ((int)((double)theEnemy->mAge * 0.25) % 0x168 + anOffset) % 0x168;
			theAChild->mX = (float)((double)CosDeg(anAngle) * 40.0 + (double)theEnemy->mX);
			anOffset = anOffset + 0x1e;
			theAChild->mY = (float)((double)SinDeg(anAngle) * 40.0 + (double)theEnemy->mY);
			theEnemy->mDrawKind = 4;
		});
	}
	else if (0x30 < theEnemy->mFrame)
	{
		Teleporter_Sparkles(theEnemy);
		float anOldX = theEnemy->mX;
		float anOldY = theEnemy->mY;
		Enemy_Place(theEnemy);
		theEnemy->mPhase = 0;
		theEnemy->mAge = 1.0f;
		theEnemy->mFrame = 0;
		theEnemy->mFrameTimer = 0;
		Teleporter_Sparkles(theEnemy);
		double aDy = (double)anOldY - theEnemy->mY;
		double aDx = (double)anOldX - theEnemy->mX;
		float aSquare = (float)(aDx * aDx + aDy * aDy);
		float aDist = (float)std::sqrt((double)aSquare);
		float aSteps = (float)((double)aDist / 15.0);
		for (float t = 0.0f; t < aSteps; t = t + 1.0f)
		{
			float f = t / aSteps;
			float aX = (float)(((double)theEnemy->mX - anOldX) * f + anOldX);
			float aY = (float)(((double)theEnemy->mY - anOldY) * f + anOldY);
			ParticleSystem_New();
			int aLifetime = RandRange(400, 900);
			ParticleSystem_Add(gLevels.mCurrent->mSystems.mCurrent, aX, aY, 0.0f, 0.0f, 0, 0, 0, 0xff, 0xff, 0xff, 0xff,
				0xff, 0xff, 0xff, 0, aLifetime, 1.0f, 0.0f, 4);
			Particle* aParticle = gLevels.mCurrent->mSystems.mCurrent->mParticles.mCurrent;
			int aFrameTime;
			aParticle->mAnim = Animation_Find("sparkles3", &aParticle->mFrameCount, &aFrameTime);
		}
	}
	return Enemy_Bounce(theEnemy);
}

// 5000 updates after its last turn there: on crossing a cell's middle beside a way off to the side (an edge, or a
// line of theKind), turns that way half the time
void Walker_TurnAtJunction(Enemy* theEnemy, int theKind)
{
	if ((float)(theEnemy->mLastTurn + 5000) <= theEnemy->mAge)
	{
		int aCol = (int)theEnemy->mX / CELL_SIZE;
		float aMidX = (float)(aCol * CELL_SIZE + CELL_HALF);
		int aRow = (int)theEnemy->mY / CELL_SIZE;
		float aMidY = (float)(aRow * CELL_SIZE + CELL_HALF);
		if (theEnemy->mVx == 0.0f)
		{
			if (((aMidY < theEnemy->mY && theEnemy->mOldY < aMidY) ||
				(theEnemy->mY < aMidY && aMidY < theEnemy->mOldY)) &&
				RandRange(0, 1) != 0 && (uint32_t)(aCol - 1) < 0x30)
			{
				int aRight = gLevels.mCurrent->mColumns[aCol + 1][aRow].mKind;
				int aLeft = gLevels.mCurrent->mColumns[aCol - 1][aRow].mKind;
				if (aRight == CELL_EDGE || aRight == theKind || aLeft == CELL_EDGE || aLeft == theKind)
				{
					theEnemy->mX = (float)(((int)theEnemy->mX / CELL_SIZE) * CELL_SIZE + CELL_HALF);
					theEnemy->mY = (float)(((int)theEnemy->mY / CELL_SIZE) * CELL_SIZE + CELL_HALF);
					while (theEnemy->mVx == 0.0f)
					{
						theEnemy->mVx = 0.0f;
						theEnemy->mVy = 0.0f;
						Walker_Turn(theEnemy);
					}
					// (the original keeps the last turn's age only for turns off a horizontal way)
				}
			}
		}
		else if (((aMidX < theEnemy->mX && theEnemy->mOldX < aMidX) ||
			(theEnemy->mX < aMidX && aMidX < theEnemy->mOldX)) && RandRange(0, 1) != 0 && (uint32_t)(aRow - 1) < 0x23)
		{
			int aBelow = gLevels.mCurrent->mColumns[aCol][aRow + 1].mKind;
			int anAbove = gLevels.mCurrent->mColumns[aCol][aRow - 1].mKind;
			if (aBelow == CELL_EDGE || aBelow == theKind || anAbove == CELL_EDGE || anAbove == theKind)
			{
				theEnemy->mX = (float)(((int)theEnemy->mX / CELL_SIZE) * CELL_SIZE + CELL_HALF);
				theEnemy->mY = (float)(((int)theEnemy->mY / CELL_SIZE) * CELL_SIZE + CELL_HALF);
				while (theEnemy->mVy == 0.0f)
				{
					theEnemy->mVx = 0.0f;
					theEnemy->mVy = 0.0f;
					Walker_Turn(theEnemy);
				}
				theEnemy->mLastTurn = (int)theEnemy->mAge;
			}
		}
	}
}

// Where the cell half a cell ahead is no edge or line of theKind (or is on the board's rim), stops in the middle of
// its cell and turns, trying up to 21 times for a way across the one it had
void Walker_Follow(Enemy* theEnemy, int theKind)
{
	if (theEnemy->mVx == 0.0f)
	{
		int aCol = (int)theEnemy->mX / CELL_SIZE;
		int aSy = Sign(theEnemy->mVy);
		int aRow = (int)((double)theEnemy->mVy + (double)theEnemy->mY + (double)((aSy * 0x10) / 2)) / CELL_SIZE;
		bool aBlocked = 0x22 < (uint32_t)(aRow - 1);
		if (!aBlocked)
		{
			int aKind = gLevels.mCurrent->mColumns[aCol][aRow].mKind;
			aBlocked = aKind != CELL_EDGE && aKind != theKind;
		}
		if (aBlocked)
		{
			theEnemy->mX = (float)(aCol * CELL_SIZE + CELL_HALF);
			theEnemy->mY = (float)(((int)theEnemy->mY / CELL_SIZE) * CELL_SIZE + CELL_HALF);
			Walker_Turn(theEnemy);
			int aTry = 0;
			do
			{
				if (theEnemy->mVx != 0.0f)
					return;
				Walker_Turn(theEnemy);
				aTry = aTry + 1;
			} while (aTry < 0x14);
		}
	}
	else
	{
		int aSx = Sign(theEnemy->mVx);
		int aCol = (int)((double)theEnemy->mX + (double)theEnemy->mVx + (double)((aSx * 0x10) / 2)) / CELL_SIZE;
		int aRow = (int)theEnemy->mY / CELL_SIZE;
		bool aBlocked = 0x2f < (uint32_t)(aCol - 1);
		if (!aBlocked)
		{
			int aKind = gLevels.mCurrent->mColumns[aCol][aRow].mKind;
			aBlocked = aKind != CELL_EDGE && aKind != theKind;
		}
		if (aBlocked)
		{
			theEnemy->mX = (float)(((int)theEnemy->mX / CELL_SIZE) * CELL_SIZE + CELL_HALF);
			theEnemy->mY = (float)(aRow * CELL_SIZE + CELL_HALF);
			Walker_Turn(theEnemy);
			int aTry = 0;
			while (theEnemy->mVy == 0.0f)
			{
				Walker_Turn(theEnemy);
				aTry = aTry + 1;
				if (0x13 < aTry)
					return;
			}
		}
	}
}

// A level's enemy from a default: the object's fields and the enemy's own (not the shot timer, the turn's age, the
// countdown, the shield or the capsule), and copies of its emitters
void Enemy_CopyDef(Enemy* theEnemy, Enemy* theDef)
{
	Obj_Copy(theEnemy, theDef);
	theEnemy->mLeftAnim = theDef->mLeftAnim;
	theEnemy->mRightAnim = theDef->mRightAnim;
	theEnemy->mUpAnim = theDef->mUpAnim;
	theEnemy->mDownAnim = theDef->mDownAnim;
	theEnemy->mPhase = theDef->mPhase;
	theEnemy->mFirstParticle = theDef->mFirstParticle;
	theEnemy->mLastParticle = theDef->mLastParticle;
	theEnemy->mBaseSpeed = theDef->mBaseSpeed;
	theEnemy->mBaseAcceleration = theDef->mBaseAcceleration;
	theEnemy->mBaseFrameTime = theDef->mBaseFrameTime;
	theEnemy->mAcceleration = theDef->mAcceleration;
	theEnemy->mDecay = theDef->mDecay;
	theEnemy->mTurnSpeed = theDef->mTurnSpeed;
	theEnemy->mLastShot = theDef->mLastShot;
	List_Each(&theDef->mEmitters, [&](Obj* theAnEmitter) {
		Emitter_Append(&theEnemy->mEmitters);
		Emitter_Copy(static_cast<Emitter*>(theEnemy->mEmitters.mCurrent), static_cast<Emitter*>(theAnEmitter));
		static_cast<Emitter*>(theEnemy->mEmitters.mCurrent)->mParent = theEnemy;
	});
}

int gKillCount = 0; // 0x478608: the kills since a power-up was last allowed (every second and fourth of five)

// 0x408a30: Dies, exploding where it is. A walker or roller scores 100 for each enemy killed while this capture fills (at
// most 900; thePoints instead when given) times the ship's multiplier, and now and then leaves a power-up while the
// board is short of its quota.
void Enemy_Kill(Enemy* theEnemy, int thePoints)
{
	gProfiles[gProfile].mKills = gProfiles[gProfile].mKills + 1;
	Obj_Explode(theEnemy, 0.0f, 0.0f);
	int aKind = theEnemy->mKind;
	if (aKind != ENEMY_SNAKE && aKind != ENEMY_CANNON_BALL && aKind != ENEMY_LASER_SHOT)
	{
		int aPoints = gLevels.mCurrent->mChain * 100;
		if (thePoints != 0)
			aPoints = thePoints;
		char aSound[64];
		std::sprintf(aSound, "audio\\exp%d.wav", aPoints);
		Sound_Play(Sound_Find(aSound));
		if (900 < aPoints)
			aPoints = 900;
		char aText[64];
		std::sprintf(aText, "%d", aPoints);
		Shooter* aShip = gLevels.mCurrent->mShooters.mCurrent;
		if (aShip == nullptr || aShip->mCellPoints < 2)
			gScore = gScore + aPoints;
		else
		{
			gScore = gScore + aShip->mCellPoints * aPoints;
			char aTimes[16];
			std::sprintf(aTimes, "^5X%d", gLevels.mCurrent->mShooters.mCurrent->mCellPoints);
			Text_Append(&gLevels.mCurrent->mTexts);
			Text_Make(gLevels.mCurrent->mTexts.mCurrent, aTimes, Font_Find("bonus_font"),
				(int)((double)theEnemy->mX + 44.0), (int)theEnemy->mY, 0, 0x28, 2000, 300, 0, 0);
			gLevels.mCurrent->mTexts.mCurrent->mVy = -0.02f;
		}
		Text_Append(&gLevels.mCurrent->mTexts);
		Text_Make(gLevels.mCurrent->mTexts.mCurrent, aText, Font_Find("bonus_font2"), (int)theEnemy->mX,
			(int)theEnemy->mY, 0, 0x28, 2000, 300, 0, 0);
		gLevels.mCurrent->mTexts.mCurrent->mVy = -0.02f;
		if (aPoints == (aPoints / 300) * 300)
		{
			// a chain of 300, 600 or 900 points names itself above the score
			const char* aCombo = "";
			if (aPoints == 300)
				aCombo = GT(0x458884);
			else if (aPoints == 600)
				aCombo = GT(0x45887c);
			else if (aPoints == 900)
				aCombo = GT(0x458870);
			Text_Append(&gLevels.mCurrent->mTexts);
			Text_Make(gLevels.mCurrent->mTexts.mCurrent, aCombo, Font_Find("bonus_font"), (int)theEnemy->mX,
				(int)((double)theEnemy->mY + 10.0), 0, 0x28, 2500, 300, 0, 0);
			gLevels.mCurrent->mTexts.mCurrent->mVy = 0.0f;
		}
		gLevels.mCurrent->mChain = gLevels.mCurrent->mChain + 1;
		RandRange(1, 10);
		gKillCount = gKillCount + 1;
		if (4 < gKillCount)
		{
			gKillCount = 0;
			return;
		}
		if ((gKillCount == 2 || gKillCount == 4) &&
			((gLevels.mCurrent->mFloodedCells + gLevels.mCurrent->mFilledCells) * 100) / gLevels.mCurrent->mTotalCells <
				gLevels.mCurrent->mFillQuota)
			Powerup_Insert(&gLevels.mCurrent->mPowerups);
	}
}

// Kind 0x12: mShotDelay updates after its last shot fires a snake (kind 0x13) from 50 pixels out, at an angle its
// animation's frame and a random quarter turn give, when the cell there is open
int SnakeShooter_Move(Enemy* theEnemy)
{
	int aResult = Enemy_Bounce(theEnemy);
	if ((double)theEnemy->mShotDelay + (double)theEnemy->mLastShot < (double)theEnemy->mAge)
	{
		int aCount = theEnemy->mFrameCount;
		int anAngle = RandRange(0, 3) * 0x5a + (theEnemy->mFrame * 0x5a) / aCount + 0x168 / aCount;
		if (0x167 < anAngle)
			anAngle = anAngle + -0x168;
		int aX = (int)((double)CosDeg(anAngle) * 50.0 + (double)theEnemy->mX);
		int aY = (int)((double)SinDeg(anAngle) * 50.0 + (double)theEnemy->mY);
		if (gLevels.mCurrent->mColumns[aX / CELL_SIZE][aY / CELL_SIZE].mKind == CELL_OPEN)
		{
			Sound_Play(Sound_Find("audio\\canon.wav"));
			theEnemy->mLastShot = theEnemy->mAge;
			Enemy_Append(&gLevels.mCurrent->mEnemies);
			Enemy* aShot = gLevels.mCurrent->mEnemies.mCurrent;
			Enemy_CopyDef(aShot, theEnemy);
			aShot->mAge = 0.0f;
			aShot->mLifetime = 0.0f;
			aShot->mKind = ENEMY_SNAKE;
			aShot->mState = ENEMY_ALIVE;
			aShot->mAnim = Animation_Find("snake", &aShot->mFrameCount, &aShot->mFrameTime);
			aShot->mDieAnim = Animation_Find("small_explo", &aShot->mDieFrameCount, &aShot->mDieFrameTime);
			aShot->mDepth = theEnemy->mDepth + 1;
			aShot->mMidHandle = 1;
			aShot->mVx = (float)((double)CosDeg(anAngle) * 0.3499999940395355);
			aShot->mVy = (float)((double)SinDeg(anAngle) * 0.3499999940395355);
			aShot->mX = (float)((double)CosDeg(anAngle) * 50.0 + (double)theEnemy->mX);
			aShot->mY = (float)((double)SinDeg(anAngle) * 50.0 + (double)theEnemy->mY);
			Emitter_Append(&aShot->mEmitters);
			Emitter_Setup(static_cast<Emitter*>(aShot->mEmitters.mCurrent), aShot, 0, 0, 0.0f, 0.0f, 0x28, 0, "spark",
				2, 255.0f, 0.0f, 2000.0f);
			for (int n = 0x28; n != 0; n--)
			{
				ParticleSystem_New();
				float aDrag = (float)((double)RandRange(-100, 0) / 1000.0 + 1.0);
				int aLifetime = RandRange(400, 0x514);
				int aLast = theEnemy->mLastParticle;
				int aFirst = theEnemy->mFirstParticle;
				float aVy = (float)((double)RandRange(-50, 50) / 1200.0 + (double)aShot->mVy);
				float aVx = (float)((double)RandRange(-50, 50) / 1200.0 + (double)aShot->mVx);
				float aPy = (float)RandRange(-5, 5) + aShot->mY;
				float aPx = (float)RandRange(-5, 5) + aShot->mX;
				ParticleSystem_Add(gLevels.mCurrent->mSystems.mCurrent, aPx, aPy, aVx, aVy, aFirst, aLast, 0, 0xff,
					0xff, 0xe6, 100, 0x32, 0, 0xff, 10, aLifetime, aDrag, 0.0f, 5);
			}
			gLevels.mCurrent->mEnemies.mCurrent = theEnemy;
			for (int i = 0; i < aShot->mAnim->mCount; i++)
				Image_SetMidHandle(aShot->mAnim->mFrames[i]);
		}
	}
	return aResult;
}

// Kind 0x13, a snake shooter's snake: bursts on hitting anything not open, holing the board 3 cells round
int Snake_Move(Enemy* theEnemy)
{
	theEnemy->mHitCell = nullptr;
	Enemy_Bounce(theEnemy);
	if (theEnemy->mHitCell != nullptr)
	{
		Enemy_Kill(theEnemy, 0);
		Level_Blast((int)theEnemy->mX / CELL_SIZE, (int)theEnemy->mY / CELL_SIZE, 3);
	}
	if (gLevels.mCurrent->mColumns[(int)theEnemy->mX / CELL_SIZE][(int)theEnemy->mY / CELL_SIZE].mKind != CELL_OPEN)
		Enemy_Kill(theEnemy, 0);
	return 1;
}

// Kind 0xd, a cannon walker's ball: rises and falls through its animation over its lifetime, then bursts, holing
// the board 3 cells round (0: done)
int CannonBall_Move(Enemy* theEnemy)
{
	if (theEnemy->mAge < theEnemy->mLifetime)
	{
		float aSin = SinDeg((int)((double)theEnemy->mAge * 180.0 / (double)theEnemy->mLifetime));
		Animation* anAnim = theEnemy->mAnim;
		int aCount = anAnim->mCount;
		int aFrame = (int)((double)aCount * (double)aSin);
		if (aCount <= aFrame)
			aFrame = aCount + -1;
		theEnemy->mImage = Animation_Frame(anAnim, aFrame);
		return 1;
	}
	Enemy_Kill(theEnemy, 0);
	Level_Blast((int)theEnemy->mX / CELL_SIZE, (int)theEnemy->mY / CELL_SIZE, 3);
	return 0;
}

// A walker, along the edges and the lines of theKind: dies in the open after its first 2000 updates, and turns where
// it stands still, now and then at junctions, and where its way ends
int Walker_Move(Enemy* theEnemy, int theKind)
{
	if (2000.0f < theEnemy->mAge &&
		gLevels.mCurrent->mColumns[(int)theEnemy->mX / CELL_SIZE][(int)theEnemy->mY / CELL_SIZE].mKind == CELL_OPEN)
	{
		Enemy_Kill(theEnemy, 0);
		return 1;
	}
	if (theEnemy->mVx == 0.0f && theEnemy->mVy == 0.0f)
		Walker_Turn(theEnemy);
	Walker_TurnAtJunction(theEnemy, theKind);
	Walker_Follow(theEnemy, theKind);
	return 0;
}

// Kind 8: speeds up (to three times its speed) while the ship has beams out, and slows back down without
void ScurryWalker_Move(Enemy* theEnemy)
{
	Shooter* aShip = gLevels.mCurrent->mShooters.mCurrent;
	float aSpeed;
	if (aShip == nullptr || aShip->mBeams.mCurrent == nullptr)
	{
		aSpeed = theEnemy->mDecay * theEnemy->mSpeed;
		if (aSpeed < theEnemy->mBaseSpeed)
			aSpeed = theEnemy->mBaseSpeed;
	}
	else
	{
		float aMax = theEnemy->mBaseSpeed * 7.0f;
		aSpeed = theEnemy->mAcceleration * theEnemy->mSpeed;
		if (aMax < aSpeed)
			aSpeed = aMax;
	}
	theEnemy->mSpeed = aSpeed;
	theEnemy->mFrameTime = (int)((double)theEnemy->mBaseFrameTime * theEnemy->mBaseSpeed / theEnemy->mSpeed);
	theEnemy->mVx = (float)Sign(theEnemy->mVx) * theEnemy->mSpeed;
	theEnemy->mVy = (float)Sign(theEnemy->mVy) * theEnemy->mSpeed;
	Walker_Move(theEnemy, CELL_LINE);
}

// Kind 0x14: 1000 updates after it last did, turns towards the ship when that is within 150 pixels; slows (to a third
// of its speed) while a beam across its way is that near, and speeds back up without
void SmartWalker_Move(Enemy* theEnemy)
{
	Shooter* aShip = gLevels.mCurrent->mShooters.mCurrent;
	if ((double)theEnemy->mLastShot + 1000.0 < (double)theEnemy->mAge && aShip != nullptr)
	{
		double aDy = (double)aShip->mY - theEnemy->mY;
		double aDx = (double)aShip->mX - theEnemy->mX;
		if (aDx * aDx + aDy * aDy < 22500.0)
		{
			theEnemy->mLastShot = theEnemy->mAge;
			theEnemy->mVx = std::fabs(theEnemy->mVx) *
				(float)Sign(gLevels.mCurrent->mShooters.mCurrent->mX - theEnemy->mX);
			theEnemy->mVy = std::fabs(theEnemy->mVy) *
				(float)Sign(gLevels.mCurrent->mShooters.mCurrent->mY - theEnemy->mY);
			Enemy_Face(theEnemy);
		}
	}
	bool aNear = false;
	aShip = gLevels.mCurrent->mShooters.mCurrent;
	if (aShip != nullptr && aShip->mBeams.mCurrent != nullptr)
	{
		List_Each(&aShip->mBeams, [&](Beam* theABeam) {
			double aDy = (double)theEnemy->mY - theABeam->mY;
			double aDx = (double)theEnemy->mX - theABeam->mX;
			if (aDx * aDx + aDy * aDy < 22500.0 &&
				((theABeam->mVx != 0.0f && theEnemy->mVy != 0.0f) || (theABeam->mVy != 0.0f && theEnemy->mVx != 0.0f)))
				aNear = true;
		});
	}
	float aSpeed;
	if (aNear)
	{
		float aMin = (float)((double)theEnemy->mBaseSpeed / 3.0);
		aSpeed = theEnemy->mDecay * theEnemy->mSpeed;
		if (aSpeed < aMin)
			aSpeed = aMin;
	}
	else
	{
		aSpeed = theEnemy->mAcceleration * theEnemy->mSpeed;
		if (theEnemy->mBaseSpeed < aSpeed)
			aSpeed = theEnemy->mBaseSpeed;
	}
	theEnemy->mSpeed = aSpeed;
	theEnemy->mFrameTime = (int)((double)theEnemy->mBaseFrameTime * theEnemy->mBaseSpeed / theEnemy->mSpeed);
	theEnemy->mVx = (float)Sign(theEnemy->mVx) * theEnemy->mSpeed;
	theEnemy->mVy = (float)Sign(theEnemy->mVy) * theEnemy->mSpeed;
	Walker_Move(theEnemy, CELL_LINE);
}

// Kind 0xb: 4000 updates after its last shot, when the 9 by 9 cells 16 cells ahead of it hold 29 filled ones or
// lines, fires a cannon ball (kind 0xd) that way in a puff of smoke
void CannonWalker_Move(Enemy* theEnemy)
{
	if ((double)theEnemy->mLastShot + 4000.0 < (double)theEnemy->mAge)
	{
		int aCount = 1;
		int aCol = (int)theEnemy->mX / CELL_SIZE + Sign(theEnemy->mVx) * 0x10;
		int aRow = (int)theEnemy->mY / CELL_SIZE + Sign(theEnemy->mVy) * 0x10;
		Level* aLevel = gLevels.mCurrent;
		for (int r = aRow - 4; r <= aRow + 4; r++)
		{
			for (int c = aCol - 4; c <= aCol + 4; c++)
			{
				if (aCount != 0 && -1 < c && c < aLevel->mCols && -1 < r && r < aLevel->mRows &&
					(aLevel->mColumns[c][r].mKind == CELL_FILLED || aLevel->mColumns[c][r].mKind == CELL_LINE))
					aCount = aCount + 1;
				// (sic: the original's reset can never happen)
				else if (c < 0 && aLevel->mCols <= c && r < 0 && aLevel->mRows <= r)
					aCount = 0;
			}
		}
		if (0x1d < aCount)
		{
			// (the original works out a pan from x for a play call that takes none)
			Sound_Play(Sound_Find("audio\\canon.wav"));
			for (int n = 0x14; n != 0; n--)
			{
				ParticleSystem_New();
				int aSy = Sign(theEnemy->mVy);
				int aSx = Sign(theEnemy->mVx);
				int aLifetime = RandRange(200, 300);
				int aLast = theEnemy->mLastParticle;
				int aFirst = theEnemy->mFirstParticle;
				float aVy = (float)((double)theEnemy->mVy + theEnemy->mVy + (double)RandRange(-50, 50) / 1200.0);
				float aVx = (float)((double)theEnemy->mVx + theEnemy->mVx + (double)RandRange(-50, 50) / 1200.0);
				float aY = (float)((double)RandRange(-5, 5) + theEnemy->mY + (double)aSy * 15.0);
				float aX = (float)((double)RandRange(-5, 5) + theEnemy->mX + (double)aSx * 15.0);
				ParticleSystem_Add(gLevels.mCurrent->mSystems.mCurrent, aX, aY, aVx, aVy, aFirst, aLast, 0, 0xff, 0xff,
					0xe6, 100, 0x32, 0, 0xff, 10, aLifetime, 0.999f, 0.0f, 5);
			}
			theEnemy->mLastShot = theEnemy->mAge;
			Enemy_Append(&gLevels.mCurrent->mEnemies);
			Enemy* aBall = gLevels.mCurrent->mEnemies.mCurrent;
			Enemy_CopyDef(aBall, theEnemy);
			aBall->mKind = ENEMY_CANNON_BALL;
			aBall->mState = ENEMY_ALIVE;
			aBall->mX = (float)((double)Sign(theEnemy->mVx) * 16.0 + (double)theEnemy->mX);
			aBall->mY = (float)((double)Sign(theEnemy->mVy) * 16.0 + (double)theEnemy->mY);
			aBall->mAge = 0.0f;
			aBall->mLifetime = 1500.0f;
			aBall->mVx = (float)((double)Sign(theEnemy->mVx) * 240.0 / 1500.0);
			aBall->mVy = (float)((double)Sign(theEnemy->mVy) * 240.0 / 1500.0);
			aBall->mAnim = Animation_Find("cannon_ball", &aBall->mFrameCount, &aBall->mFrameTime);
			aBall->mDepth = theEnemy->mDepth + 1;
			aBall->mMidHandle = 1;
			aBall->mDieAnim = Animation_Find("small_explo", &aBall->mDieFrameCount, &aBall->mDieFrameTime);
			gLevels.mCurrent->mEnemies.mCurrent = theEnemy;
			for (int i = 0; i < aBall->mAnim->mCount; i++)
				Image_SetMidHandle(aBall->mAnim->mFrames[i]);
		}
	}
	Walker_Move(theEnemy, CELL_LINE);
}

// Kind 0xb: faces the ship across its way; 4000 updates after its last shot, while it is within the length of a
// beam going its way, shoots a laser (kind 0xd) at it
void LaserWalker_Move(Enemy* theEnemy)
{
	Animation* anAnim = theEnemy->mDownAnim;
	theEnemy->mAnim = anAnim;
	Shooter* aShip = gLevels.mCurrent->mShooters.mCurrent;
	float aDx;
	float aDy;
	if (theEnemy->mVx == 0.0f)
	{
		if (aShip == nullptr || aShip->mX <= theEnemy->mX)
			aDx = -1.0f;
		else
		{
			anAnim = theEnemy->mUpAnim;
			aDx = 1.0f;
		}
		aDy = 0.0f;
		theEnemy->mAnim = anAnim;
	}
	else if (aShip != nullptr && aShip->mY < theEnemy->mY)
	{
		aDx = 0.0f;
		aDy = -1.0f;
		theEnemy->mAnim = theEnemy->mLeftAnim;
	}
	else
	{
		aDx = 0.0f;
		theEnemy->mAnim = theEnemy->mRightAnim;
		aDy = 1.0f;
	}
	if ((double)theEnemy->mAge <= (double)theEnemy->mLastShot + 4000.0)
	{
		Walker_Move(theEnemy, CELL_LINE);
		return;
	}
	bool aLinedUp = false;
	aShip = gLevels.mCurrent->mShooters.mCurrent;
	if (aShip != nullptr && aShip->mBeams.mCurrent != nullptr)
	{
		List_Each(&aShip->mBeams, [&](Beam* theABeam) {
			if (theABeam->mVx == 0.0f || theEnemy->mVx == 0.0f)
			{
				if (theABeam->mVy != 0.0f && theEnemy->mVy != 0.0f &&
					((theABeam->mStartY < theEnemy->mY && theEnemy->mY < theABeam->mY) ||
						(theEnemy->mY < theABeam->mStartY && theABeam->mY < theEnemy->mY)))
					aLinedUp = true;
			}
			else if ((theABeam->mStartX < theEnemy->mX && theEnemy->mX < theABeam->mX) ||
				(theEnemy->mX < theABeam->mStartX && theABeam->mX < theEnemy->mX))
				aLinedUp = true;
		});
		if (aLinedUp)
		{
			// (the original works out a pan from x for a play call that takes none)
			Sound_Play(Sound_Find("audio\\laser.wav"));
			theEnemy->mLastShot = theEnemy->mAge;
			Enemy_Append(&gLevels.mCurrent->mEnemies);
			Enemy* aShot = gLevels.mCurrent->mEnemies.mCurrent;
			Enemy_CopyDef(aShot, theEnemy);
			aShot->mAge = 0.0f;
			aShot->mState = ENEMY_ALIVE;
			aShot->mLifetime = 0.0f;
			aShot->mKind = ENEMY_LASER_SHOT;
			aShot->mMidHandle = 1;
			aShot->mDepth = theEnemy->mDepth + 1;
			aShot->mX = (float)((double)aDx * (double)theEnemy->mImage->mW * 0.5 + (double)theEnemy->mX);
			aShot->mY = (float)((double)aDy * (double)theEnemy->mImage->mH * 0.5 + (double)theEnemy->mY);
			aShot->mVx = (float)((double)aDx * 0.2);
			aShot->mVy = (float)((double)aDy * 0.2);
			const char* aName;
			if (0.0f < aShot->mVy)
				aName = "laser_shot_up";
			else if (aShot->mVy < 0.0f)
				aName = "laser_shot_down";
			else if (0.0f <= aShot->mVx)
				aName = "laser_shot_right";
			else
				aName = "laser_shot_left";
			aShot->mAnim = Animation_Find(aName, &aShot->mFrameCount, &aShot->mFrameTime);
			aShot->mFrame = 0;
			gLevels.mCurrent->mEnemies.mCurrent = theEnemy;
			for (int i = 0; i < aShot->mAnim->mCount; i++)
				Image_SetMidHandle(aShot->mAnim->mFrames[i]);
		}
	}
	Walker_Move(theEnemy, CELL_LINE);
}

// Kind 0x13: walks the edges alone (kind 3), facing the ship across its way; at frame 12 of its animation it nips
// the cell two ahead of where it faces, holing the board 2 cells round when that is filled or a line
int CrabWalker_Move(Enemy* theEnemy)
{
	Animation* anAnim = theEnemy->mDownAnim;
	theEnemy->mAnim = anAnim;
	Shooter* aShip = gLevels.mCurrent->mShooters.mCurrent;
	int aDx;
	int aDy;
	if (theEnemy->mVx == 0.0f)
	{
		aDy = 0;
		if (aShip == nullptr || aShip->mX <= theEnemy->mX)
			aDx = -1;
		else
		{
			anAnim = theEnemy->mUpAnim;
			aDx = 1;
		}
		theEnemy->mAnim = anAnim;
	}
	else
	{
		aDx = 0;
		if (aShip == nullptr || theEnemy->mY <= aShip->mY)
		{
			theEnemy->mAnim = theEnemy->mRightAnim;
			aDy = 1;
		}
		else
		{
			theEnemy->mAnim = theEnemy->mLeftAnim;
			aDy = -1;
		}
	}
	int aResult = Walker_Move(theEnemy, CELL_EDGE);
	if (theEnemy->mFrameTimer == 0 && theEnemy->mFrame == 0xc)
	{
		int aCol = (int)theEnemy->mX / CELL_SIZE + aDx * 2;
		int aRow = (int)theEnemy->mY / CELL_SIZE + aDy * 2;
		if (0x2d < (uint32_t)(aCol - 2) || 0x20 < (uint32_t)(aRow - 2))
			return aResult;
		int aKind = gLevels.mCurrent->mColumns[aCol][aRow].mKind;
		if (aKind == CELL_LINE || aKind == CELL_FILLED)
		{
			// (the original works out a pan from x for a play call that takes none)
			Sound_Play(Sound_Find("audio\\crab.wav"));
			Level_Blast(aCol, aRow, 2);
			for (int n = 0x14; n != 0; n--)
			{
				ParticleSystem_New();
				int aLifetime = RandRange(700, 0x44c);
				int aLast = Bank_IndexOf(&gArtBank, "bubble03");
				int aFirst = Bank_IndexOf(&gArtBank, "bubble01");
				aFirst = aFirst + RandRange(0, 2);
				float aVy = (float)((double)RandRange(-100, 100) / 1000.0);
				float aVx = (float)((double)RandRange(-100, 100) / 1000.0);
				Cell* aCell = &gLevels.mCurrent->mColumns[aCol][aRow];
				float aY = (float)RandRange(-0x19, 0x19) + aCell->mY;
				float aX = (float)RandRange(-0x19, 0x19) + gLevels.mCurrent->mColumns[aCol][aRow].mX;
				ParticleSystem_Add(gLevels.mCurrent->mSystems.mCurrent, aX, aY, aVx, aVy, aFirst, aLast, 0, 0xff, 0xff,
					0xff, 0xe7, 0xa6, 0x39, 0x21c, 10, aLifetime, 1.0f, 0.0f, 4);
			}
		}
	}
	return aResult;
}

// Kind 0x15: its speed swings with the sine of an eighth of its age, its animation keeping step
int LobsterWalker_Move(Enemy* theEnemy)
{
	int aResult = Walker_Move(theEnemy, CELL_LINE);
	float aSin = SinDeg((int)((double)theEnemy->mAge * 0.125) % 0x168);
	float aSpeed = (float)(((double)aSin + aSin + 1.0) / 20.0 + (double)theEnemy->mBaseSpeed);
	theEnemy->mSpeed = aSpeed;
	theEnemy->mVx = (float)Sign(theEnemy->mVx) * aSpeed;
	theEnemy->mVy = (float)Sign(theEnemy->mVy) * aSpeed;
	theEnemy->mFrameTime = (int)(((double)theEnemy->mBaseSpeed * 3.0 - (double)aSpeed / 1.3) * 200.0);
	return aResult;
}

// A puff of smoke behind a turtle walker, fading to the colour given (0x40a360 has this twice)
static void Turtle_Puff(Enemy* theEnemy, int theEndR, int theEndG, int theEndB)
{
	ParticleSystem_New();
	int aSy = Sign(theEnemy->mVy);
	int aSx = Sign(theEnemy->mVx);
	double aY = theEnemy->mY;
	double aX = theEnemy->mX;
	int aLifetime = RandRange(100, 0xfa);
	int aLast = theEnemy->mLastParticle;
	int aFirst = theEnemy->mFirstParticle;
	float aVy = (float)((double)RandRange(-100, 100) / 20000.0 + (double)theEnemy->mVy * 0.25);
	float aVx = (float)((double)RandRange(-100, 100) / 10000.0 + (double)theEnemy->mVx * 0.25);
	float aPy = (float)(aY - (double)RandRange(-0x1e, 0x1e) / 20.0 - (double)((theEnemy->mImage->mH * aSy) / 3));
	float aPx = (float)(aX - (double)RandRange(-0x1e, 0x1e) / 20.0 - (double)((theEnemy->mImage->mW * aSx) / 3));
	ParticleSystem_Add(gLevels.mCurrent->mSystems.mCurrent, aPx, aPy, aVx, aVy, aFirst, aLast, 0, 0xff, 0xff, 0xff,
		theEndR, theEndG, theEndB, 0x154, 10, aLifetime, 1.0f, 0.0f, 5);
}

// Kind 0x14: on turning round stops for 2000 updates (its age set to 2000), playing frames 0..28 at half rate;
// walking it plays frames 29 on and puffs smoke behind it
int TurtleWalker_Move(Enemy* theEnemy)
{
	int aResult = Walker_Move(theEnemy, CELL_LINE);
	if (Sign(theEnemy->mPrevVx) != Sign(theEnemy->mVx))
		theEnemy->mAge = 2000.0f;
	theEnemy->mPrevVx = theEnemy->mVx;
	if (theEnemy->mAge < 2000.0f || 4000.0f < theEnemy->mAge)
	{
		if (RandRange(0, 3) == 0)
			Turtle_Puff(theEnemy, 0xe7, 0xa6, 0x39);
		if (RandRange(0, 10) == 0)
			Turtle_Puff(theEnemy, 100, 100, 100);
		theEnemy->mFrameTime = 0xf;
		if (theEnemy->mFrame < 0x1d)
			theEnemy->mFrame = 0x1d;
	}
	else
	{
		theEnemy->mFrameTime = 0x1e;
		theEnemy->mX = theEnemy->mX - theEnemy->mVx;
		theEnemy->mY = theEnemy->mY - theEnemy->mVy;
		if (0x1c < theEnemy->mFrame)
			theEnemy->mFrame = 0;
	}
	return aResult;
}

// The update of an enemy about (state 3, and while it appears): three passes while the ship has time slowed (flags
// 2); while it has time frozen (flags 1) all but every fourth update only age it. A beam it touches gets a
// beamrider running back to the ship (and its shield flashes); it dies on a filled cell (a roller on a line too),
// and a walker beside a capture.
int Enemy_Step(Enemy* theEnemy)
{
	int aResult = 0;
	int aPasses = 1;
	if (theEnemy->mImage != nullptr &&
		(theEnemy->mX <= 0.0f || (float)BOARD_WIDTH <= theEnemy->mX || theEnemy->mY <= 0.0f || (float)BOARD_HEIGHT <
			theEnemy->mY))
		return 0;
	if (theEnemy->mCountdown != 0)
	{
		theEnemy->mCountdown = theEnemy->mCountdown + -1;
		if (theEnemy->mCountdown == 1)
		{
			Enemy_Kill(theEnemy, 0);
			return 1;
		}
	}
	if (gLevels.mCurrent->mShooters.mCurrent != nullptr && (gLevels.mCurrent->mShooters.mCurrent->mFlags & 2) != 0)
		aPasses = 3;
	for (int aPass = 0; aPass < aPasses; aPass++)
	{
		Shooter* aShip = gLevels.mCurrent->mShooters.mCurrent;
		if (aShip != nullptr && (int)theEnemy->mAge % 4 != 1 && (aShip->mFlags & 1) != 0)
		{
			theEnemy->mAge = theEnemy->mAge + 1.0f;
			theEnemy->mOldX = theEnemy->mX;
			theEnemy->mOldY = theEnemy->mY;
			return 1;
		}
		aResult = aResult | Obj_Update(theEnemy);
		switch (theEnemy->mKind)
		{
		case ENEMY_FROG:
			Frog_Move(theEnemy);
			break;
		case ENEMY_BURST_ROLLER:
			BurstRoller_Move(theEnemy);
			break;
		case ENEMY_BUMP_ROLLER:
			BumpRoller_Move(theEnemy);
			break;
		case ENEMY_JELLYFISH_ROLLER:
			Jellyfish_Move(theEnemy);
			break;
		case ENEMY_SPIN_ROLLER:
			SpinRoller_Move(theEnemy);
			break;
		case ENEMY_TELEPORTER_ROLLER:
			Teleporter_Move(theEnemy);
			break;
		case ENEMY_TURN_ROLLER:
			TurnRoller_Move(theEnemy);
			break;
		case ENEMY_LOBSTER_WALKER:
			LobsterWalker_Move(theEnemy);
			break;
		case ENEMY_SCURRY_WALKER:
			ScurryWalker_Move(theEnemy);
			break;
		case ENEMY_TURTLE_WALKER:
			TurtleWalker_Move(theEnemy);
			break;
		case ENEMY_LASER_WALKER:
			LaserWalker_Move(theEnemy);
			break;
		case ENEMY_CANNON_WALKER:
			CannonWalker_Move(theEnemy);
			break;
		case ENEMY_CRAB_WALKER:
			CrabWalker_Move(theEnemy);
			break;
		case ENEMY_CANNON_BALL:
			CannonBall_Move(theEnemy);
			break;
		case ENEMY_LASER_SHOT:
			aResult = aResult & LaserShot_Move(theEnemy);
			break;
		case ENEMY_LINE_WALKER:
			Walker_Move(theEnemy, CELL_LINE);
			break;
		case ENEMY_ROLLER:
			Enemy_Bounce(theEnemy);
			break;
		case ENEMY_SNAKE_SHOOTER:
			SnakeShooter_Move(theEnemy);
			break;
		case ENEMY_SNAKE:
			Snake_Move(theEnemy);
			break;
		case ENEMY_SMART_WALKER:
			SmartWalker_Move(theEnemy);
			break;
		}
		Obj_UpdateEmitters(theEnemy);
		Beam* aBeam = Obj_TouchedBeam(theEnemy, theEnemy->mKind == ENEMY_FROG ? 2 : 3,
			theEnemy->mKind == ENEMY_FROG ? 5 : 3);
		if (aBeam == theEnemy->mLastBeam || theEnemy->mKind == ENEMY_SNAKE || theEnemy->mKind == ENEMY_CANNON_BALL ||
			theEnemy->mKind == ENEMY_LASER_SHOT)
		{
			if (aBeam != nullptr)
			{
				if (theEnemy->mKind == ENEMY_LASER_SHOT)
					aResult = aResult & LaserShot_HitBeams(theEnemy);
				theEnemy->mLastBeam = aBeam;
			}
		}
		else if (aBeam != nullptr)
		{
			bool aRiding = false;
			List_Each(&aBeam->mRiders, [&](Rider* theARider) {
				if (theARider->mEnemy == theEnemy)
					aRiding = true;
			});
			if (!aRiding)
			{
				Rider_Append(&aBeam->mRiders);
				Rider_Setup(aBeam->mRiders.mCurrent, aBeam, theEnemy->mX, theEnemy->mY, theEnemy);
				if ((float)(theEnemy->mLastZap + 1000) < theEnemy->mAge &&
					gLevels.mCurrent->mShooters.mCurrent->mState == 5)
				{
					// (the original works out a pan from x for a play call that takes none)
					Sound_Play(Sound_Find("audio\\electricity.wav"));
					theEnemy->mLastZap = (int)theEnemy->mAge;
				}
			}
			Enemy_Shield(theEnemy);
			theEnemy->mLastBeam = aBeam;
		}
		int aKind = theEnemy->mKind;
		bool aDies = false;
		if (aKind != ENEMY_CANNON_BALL)
		{
			int aCell = gLevels.mCurrent->mColumns[(int)theEnemy->mX / CELL_SIZE][(int)theEnemy->mY / CELL_SIZE].mKind;
			aDies = aCell == CELL_FILLED || (-1 < aKind && aKind < 7 && aCell == CELL_LINE);
		}
		if (!aDies)
			aDies = 6 < aKind && aKind < 0xd && Enemy_NearCapture(theEnemy) != 0;
		if (aDies)
		{
			if (theEnemy->mShield != nullptr)
				Obj_Delete(theEnemy->mShield);
			theEnemy->mShield = nullptr;
			Enemy_Kill(theEnemy, 0);
		}
	}
	return aResult;
}

// Fades in with the cell under it (drawn with the cell's alpha), and is about once that is whole and nothing near
// is being captured
int Enemy_UpdateAppearing(Enemy* theEnemy)
{
	Enemy_Step(theEnemy);
	theEnemy->mAge = theEnemy->mAge + 1.0f;
	if (theEnemy->mDrawKind != 0)
	{
		theEnemy->mDrawKind = 4;
		int anAlpha = gLevels.mCurrent->mColumns[(int)theEnemy->mX / CELL_SIZE][(int)theEnemy->mY / CELL_SIZE].mAlpha;
		theEnemy->mAlpha = anAlpha;
		if (0xfe < anAlpha && Enemy_NearCapture(theEnemy) == 0)
		{
			theEnemy->mState = ENEMY_ALIVE;
			theEnemy->mDrawKind = 1;
		}
	}
	return 1;
}

int Enemy_Update(Enemy* theEnemy)
{
	int aResult = 0;
	switch (theEnemy->mState)
	{
	case ENEMY_APPEARING:
		aResult = Enemy_UpdateAppearing(theEnemy);
		break;
	case ENEMY_DYING:
		theEnemy->mAnim = theEnemy->mDieAnim;
		theEnemy->mVx = (float)((double)theEnemy->mVx * 0.96);
		theEnemy->mVy = (float)((double)theEnemy->mVy * 0.96);
		[[fallthrough]];
	case ENEMY_FADING:
		aResult = Obj_Update(theEnemy);
		break;
	case ENEMY_ALIVE:
		aResult = Enemy_Step(theEnemy);
		break;
	}
	Enemy_UpdateShield(theEnemy);
	return aResult;
}

} // namespace wb
