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
	theEnemy->lastShot = 0.0f;
	theEnemy->shield = nullptr;
	theEnemy->phase = 0;
	int aTime = RandRange(2000, 3000);
	theEnemy->lastTurn = 0;
	theEnemy->countdown = 0;
	theEnemy->shotDelay = aTime;
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
	theEnemy->acceleration = (float)Xml_GetFloat(theTag, "acceleration");
	theEnemy->decay = (float)Xml_GetFloat(theTag, "decay");
	theEnemy->turnSpeed = (float)Xml_GetFloat(theTag, "turn_speed");
	if (IsType(theTag, "roller"))
		theEnemy->kind = 0x11;
	else if (IsType(theTag, "burst_roller"))
		theEnemy->kind = 1;
	else if (IsType(theTag, "jellyfish_roller"))
	{
		theEnemy->kind = 3;
		theEnemy->firstParticle = Bank_IndexOf(&gArtBank, "bubble01");
		theEnemy->lastParticle = Bank_IndexOf(&gArtBank, "bubble02");
	}
	else if (IsType(theTag, "teleporter_roller"))
	{
		theEnemy->kind = 5;
		theEnemy->firstParticle = Bank_IndexOf(&gArtBank, "bubble01");
		theEnemy->lastParticle = Bank_IndexOf(&gArtBank, "bubble02");
	}
	else if (IsType(theTag, "spin_roller"))
	{
		theEnemy->kind = 4;
		theEnemy->firstParticle = Bank_IndexOf(&gArtBank, "particle_0");
		theEnemy->lastParticle = Bank_IndexOf(&gArtBank, "particle_0");
	}
	else if (IsType(theTag, "turn_roller"))
		theEnemy->kind = 6;
	else if (IsType(theTag, "bump_roller"))
	{
		theEnemy->kind = 2;
		theEnemy->firstParticle = Bank_IndexOf(&gArtBank, "particle_0");
		theEnemy->lastParticle = Bank_IndexOf(&gArtBank, "particle_0");
	}
	else if (IsType(theTag, "snake_shooter"))
	{
		theEnemy->firstParticle = Bank_IndexOf(&gArtBank, "exp_0");
		theEnemy->lastParticle = Bank_IndexOf(&gArtBank, "exp_4");
		theEnemy->kind = 0x12;
	}
	else if (IsType(theTag, "line_walker"))
	{
		theEnemy->kind = 0x10;
		theEnemy->shadow = 1;
	}
	else if (IsType(theTag, "scurry_walker"))
	{
		theEnemy->kind = 8;
		theEnemy->shadow = 1;
	}
	else if (IsType(theTag, "smart_walker"))
	{
		theEnemy->kind = 0x14;
		theEnemy->shadow = 1;
	}
	else if (IsType(theTag, "cannon_walker"))
	{
		theEnemy->firstParticle = Bank_IndexOf(&gArtBank, "smoke_0001");
		theEnemy->lastParticle = Bank_IndexOf(&gArtBank, "smoke_0009");
		theEnemy->kind = 0xb;
		theEnemy->shadow = 1;
	}
	else if (IsType(theTag, "laser_walker"))
	{
		theEnemy->kind = 10;
		theEnemy->shadow = 1;
		theEnemy->firstParticle = Bank_IndexOf(&gArtBank, "particle_4");
		theEnemy->lastParticle = Bank_IndexOf(&gArtBank, "particle_4");
	}
	else if (IsType(theTag, "crab_walker"))
	{
		theEnemy->kind = 0xc;
		theEnemy->shadow = 1;
		theEnemy->firstParticle = Bank_IndexOf(&gArtBank, "particle_4");
		theEnemy->lastParticle = Bank_IndexOf(&gArtBank, "particle_4");
	}
	else if (IsType(theTag, "lobster_walker"))
	{
		theEnemy->kind = 7;
		theEnemy->shadow = 1;
		theEnemy->firstParticle = Bank_IndexOf(&gArtBank, "particle_4");
		theEnemy->lastParticle = Bank_IndexOf(&gArtBank, "particle_4");
	}
	else if (IsType(theTag, "turtle_walker"))
	{
		theEnemy->kind = 9;
		theEnemy->shadow = 1;
		theEnemy->firstParticle = Bank_IndexOf(&gArtBank, "exp_2");
		theEnemy->lastParticle = Bank_IndexOf(&gArtBank, "exp_4");
	}
	else if (IsType(theTag, "frog"))
		theEnemy->kind = 0;
	if (Xml_GetString(theTag, "left_animation") != nullptr)
	{
		theEnemy->leftAnim = Animation_Find(Xml_GetString(theTag, "left_animation"), &theEnemy->frameCount,
			&theEnemy->frameTime);
		theEnemy->anim = theEnemy->leftAnim;
	}
	if (Xml_GetString(theTag, "right_animation") != nullptr)
		theEnemy->rightAnim = Animation_Find(Xml_GetString(theTag, "right_animation"), &theEnemy->frameCount,
			&theEnemy->frameTime);
	if (Xml_GetString(theTag, "up_animation") != nullptr)
		theEnemy->upAnim = Animation_Find(Xml_GetString(theTag, "up_animation"), &theEnemy->frameCount,
			&theEnemy->frameTime);
	if (Xml_GetString(theTag, "down_animation") != nullptr)
		theEnemy->downAnim = Animation_Find(Xml_GetString(theTag, "down_animation"), &theEnemy->frameCount,
			&theEnemy->frameTime);
	theEnemy->baseSpeed = theEnemy->speed;
	theEnemy->state = 0;
	theEnemy->depth = 3;
	theEnemy->baseFrameTime = (float)theEnemy->frameTime;
	theEnemy->baseAcceleration = theEnemy->acceleration;
}

void Enemy_Draw(Enemy* theEnemy)
{
	if (theEnemy->shield != nullptr)
		Obj_Draw(theEnemy->shield);
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
	int aCol = (int)theEnemy->x / 16;
	int aRow = (int)theEnemy->y / 16;
	if (gLevels.current->columns[aCol][aRow].kind == 0)
	{
		Image* anImage = theEnemy->image;
		int anAhead = (int)((double)((anImage->w * Sign(theEnemy->vx)) / 3) + (double)theEnemy->x);
		Cell* aCell = &gLevels.current->columns[anAhead / 16][aRow];
		if (aCell->kind != 0)
		{
			theEnemy->hitCell = aCell;
			theEnemy->vx = -theEnemy->vx;
		}
		anAhead = (int)((double)((anImage->h * Sign(theEnemy->vy)) / 3) + (double)theEnemy->y);
		aCell = &gLevels.current->columns[aCol][anAhead / 16];
		if (aCell->kind != 0)
		{
			theEnemy->hitCell = aCell;
			theEnemy->vy = -theEnemy->vy;
		}
	}
	if (gLevels.current->columns[aCol][aRow].kind != 0)
	{
		theEnemy->vx = -theEnemy->vx;
		theEnemy->vy = -theEnemy->vy;
	}
	return 0;
}

// Speeds up over frames 6 to 10 of its animation, the decay slowing it the rest of the time
int BurstRoller_Move(Enemy* theEnemy)
{
	theEnemy->acceleration = (theEnemy->frame < 6 || 10 < theEnemy->frame) ? 1.0002f : 1.0141f;
	float aDecayed = theEnemy->decay * theEnemy->vx;
	theEnemy->vx = aDecayed * theEnemy->acceleration;
	aDecayed = theEnemy->decay * theEnemy->vy;
	theEnemy->vy = aDecayed * theEnemy->acceleration;
	return Enemy_Bounce(theEnemy);
}

// Sinks, and over frames 40 to 65 rises, now and then letting a bubble go
int Jellyfish_Move(Enemy* theEnemy)
{
	if (theEnemy->frame < 0x28 || 0x41 < theEnemy->frame)
		theEnemy->acceleration = 0.00058f;
	else
	{
		theEnemy->acceleration = -0.001f;
		if (RandRange(0, 0x13) == 0)
		{
			ParticleSystem_New();
			int aSign = Sign(theEnemy->vx);
			int aFirst = theEnemy->firstParticle;
			int aLifetime = RandRange(600, 800);
			float aVy = (float)((double)RandRange(-30, 30) / 200.0);
			float aVx = (float)((double)RandRange(-10, 10) / 200.0);
			float aY = (float)RandRange(-20, 0) + theEnemy->y;
			float aX = (float)((double)RandRange(-10, 10) + (double)theEnemy->x + (double)(aSign * 3));
			ParticleSystem_Add(gLevels.current->systems.current, aX, aY, aVx, aVy, aFirst, aFirst, 0, 0xff, 0xff, 0xff,
				0xff, 0xff, 0xff, 0x104, 10, aLifetime, 0.998f, -0.0003f, 5);
		}
	}
	float aDecayed = theEnemy->decay * theEnemy->vy;
	theEnemy->vy = aDecayed + theEnemy->acceleration;
	return Enemy_Bounce(theEnemy);
}

// 0x405a20 has this twice: a bounce in x off a beam's line (kind 2) pushes the line's column one cell on into the
// filled cells behind it, a bounce in y its row. The line has to have open cells on the roller's side and two filled
// ones behind it all along, and end on four cells of one kind at both ends.
static void BumpRoller_Push(Cell* theHit, int theD, bool theInX)
{
	int aLine = theInX ? theHit->col : theHit->row;
	int aStart = theInX ? theHit->row : theHit->col;
	auto C = [&](int theAlong, int theOff) -> Cell* {
		if (theInX)
			return &gLevels.current->columns[aLine + theOff][theAlong];
		return &gLevels.current->columns[theAlong][aLine + theOff];
	};
	if (C(aStart, theD)->kind != 4)
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
			if (C(anAlong, 0)->state == 5)
				anOk = false;
			aBefore = C(anAlong, -theD)->kind;
			anOn = C(anAlong, 0)->kind;
			aBehind = C(anAlong, theD)->kind;
			aBehind2 = C(anAlong, theD * 2)->kind;
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
		gLevels.current->filledCells = gLevels.current->filledCells + -1;
		C(anAlong, 0)->kind = 0;
		C(anAlong, 0)->image = gLevels.current->empty;
		Cell* aCell = C(anAlong, 0);
		int aY = (int)aCell->y;
		int aX = (int)aCell->x;
		Image* aBack = gLevels.current->background;
		Copy16(gBackBuffer.bits, gBackBuffer.pitch, aX, aY, aBack->bits, aBack->pitch, aX, aY, 0x10, 0x10);
		Cell_Redraw(C(anAlong, 0));
		C(anAlong, theD)->kind = 2;
		ParticleSystem_New();
		int aLifetime = RandRange(700, 0x44c);
		int aLast = Bank_IndexOf(&gArtBank, "exp_4");
		int aFirst = Bank_IndexOf(&gArtBank, "exp_0");
		aFirst = aFirst + RandRange(0, 3);
		float aVy = (float)((double)RandRange(-100, 100) / 1000.0);
		float aVx = (float)((double)RandRange(-100, 100) / 1000.0);
		float aPy = (float)RandRange(-10, 10) + C(anAlong, 0)->y;
		float aPx = (float)RandRange(-10, 10) + C(anAlong, 0)->x;
		ParticleSystem_Add(gLevels.current->systems.current, aPx, aPy, aVx, aVy, aFirst, aLast, 0, 0xff, 0xff, 0xff,
			0xe7, 0xa6, 0x39, 0x21c, 10, aLifetime, 1.0f, 0.0f, 5);
	}
	for (int anAlong = aMin; anAlong <= aMax; anAlong++)
	{
		Cell_PickImage(C(anAlong, theD));
		Cell_Redraw(C(anAlong, theD));
		Cell* aCell = C(anAlong, theD);
		Dirty_AddClippedShown(&gDirty, (int)aCell->x, (int)aCell->y, 0x10, 0x10, kScreenClip);
		if (anAlong == aMin || anAlong == aMax)
		{
			Cell_PickImage(C(anAlong, 0));
			Cell_Redraw(C(anAlong, 0));
		}
		aCell = C(anAlong, 0);
		Dirty_AddClippedShown(&gDirty, (int)aCell->x, (int)aCell->y, 0x10, 0x10, kScreenClip);
	}
}

// A bounce that turned it back in x (else in y) off a beam's line pushes the line
int BumpRoller_Move(Enemy* theEnemy)
{
	int aResult = Enemy_Bounce(theEnemy);
	Cell* aHit = theEnemy->hitCell;
	if (aHit != nullptr)
	{
		if (1.0f < theEnemy->age && Sign(theEnemy->vx) != Sign(theEnemy->prevVx))
		{
			if (aHit->kind == 2)
				BumpRoller_Push(aHit, Sign(theEnemy->prevVx), true);
		}
		else if (1.0f < theEnemy->age && Sign(theEnemy->vy) != Sign(theEnemy->prevVy))
		{
			if (aHit->kind == 2)
				BumpRoller_Push(aHit, Sign(theEnemy->prevVy), false);
		}
	}
	theEnemy->prevVx = theEnemy->vx;
	theEnemy->prevVy = theEnemy->vy;
	return aResult;
}

// Winds up (its frame time falls to 3), rolls with sparks until it is fast, then winds down again
void SpinRoller_Move(Enemy* theEnemy)
{
	int aPhase = theEnemy->phase;
	if (aPhase == 0)
	{
		theEnemy->acceleration = 1.0f;
		if ((int)theEnemy->age % 100 == 0)
			theEnemy->frameTime = theEnemy->frameTime + -1;
		if (theEnemy->frameTime < 4)
			theEnemy->phase = 1;
	}
	else if (aPhase == 1)
	{
		theEnemy->acceleration = theEnemy->baseAcceleration;
		if (RandRange(1, 10) == 1)
		{
			ParticleSystem_New();
			int aLifetime = RandRange(400, 500);
			int aLast = theEnemy->lastParticle;
			int aFirst = theEnemy->firstParticle;
			float aVy = (float)((double)RandRange(-50, 50) / 200.0);
			float aVx = (float)((double)RandRange(-50, 50) / 200.0);
			float aY = (float)RandRange(-5, 5) + theEnemy->y;
			float aX = (float)RandRange(-5, 5) + theEnemy->x;
			ParticleSystem_Add(gLevels.current->systems.current, aX, aY, aVx, aVy, aFirst, aLast, 0, 0xff, 0xff, 0xff,
				0xff, 0xff, 0, 400, 10, aLifetime, 0.999f, 0.0003f, 5);
		}
		double aVx2 = (double)theEnemy->vx * (double)theEnemy->vx;
		double aVy2 = (double)theEnemy->vy * (double)theEnemy->vy;
		if (0.6 < aVx2 + aVy2)
			theEnemy->phase = 2;
	}
	else if (aPhase == 2)
	{
		theEnemy->acceleration = 1.0f;
		if ((int)theEnemy->age % 0x50 == 0)
			theEnemy->frameTime = theEnemy->frameTime + 1;
		if (theEnemy->baseFrameTime <= (float)theEnemy->frameTime)
			theEnemy->phase = 0;
	}
	float aDecayed = theEnemy->decay * theEnemy->vx;
	theEnemy->vx = aDecayed * theEnemy->acceleration;
	aDecayed = theEnemy->decay * theEnemy->vy;
	theEnemy->vy = aDecayed * theEnemy->acceleration;
	Enemy_Bounce(theEnemy);
}

// Hops along its row (frames 19 on of its animation are the hop); two cells short of anything not open it lands,
// turned round, sits for 1000 updates and jumps again
int Frog_Move(Enemy* theEnemy)
{
	int aPhase = theEnemy->phase;
	if (aPhase != 0)
	{
		if (aPhase == 1)
		{
			theEnemy->frame = 0;
			theEnemy->frameTimer = 0;
			if (theEnemy->age <= 1000.0f)
				return 1;
			theEnemy->phase = 2;
			Sound_Play(Sound_Find("audio\\frog_jump.wav"));
			return 1;
		}
		if (aPhase == 2 && 8 <= theEnemy->frame)
			theEnemy->phase = 0;
		return 1;
	}
	if (theEnemy->frame < 2 ||
		(theEnemy->frame == theEnemy->frameCount + -1 && theEnemy->frameTimer == theEnemy->frameTime + -1))
	{
		theEnemy->frame = 0x13;
		theEnemy->frameTimer = 0;
	}
	if (theEnemy->anim == theEnemy->leftAnim)
		theEnemy->vx = -theEnemy->speed;
	else if (theEnemy->anim == theEnemy->rightAnim)
		theEnemy->vx = theEnemy->speed;
	int aSx = Sign(theEnemy->vx);
	int aSy = Sign(theEnemy->vy);
	int aRow = (int)theEnemy->y / 16;
	int aCol = (int)theEnemy->x / 16;
	if (gLevels.current->columns[aCol + aSx * 2][aRow + aSy * 2].kind == 0)
		return 1;
	if (theEnemy->vx < 0.0f)
	{
		theEnemy->x = theEnemy->x - 8.0f;
		theEnemy->anim = theEnemy->rightAnim;
	}
	else if (0.0f < theEnemy->vx)
	{
		theEnemy->anim = theEnemy->leftAnim;
		theEnemy->x = theEnemy->x + 8.0f;
	}
	theEnemy->vx = 0.0f;
	theEnemy->frameTimer = 0;
	theEnemy->vy = 0.0f;
	theEnemy->frame = 0;
	theEnemy->phase = 1;
	theEnemy->age = 1.0f;
	Sound_Play(Sound_Find("audio\\frog_land.wav"));
	return 1;
}

// Kind 0xe, a laser walker's shot: bursts in a ring inside a cell that is not open (0: done)
int LaserShot_Move(Enemy* theEnemy)
{
	int aCol = (int)theEnemy->x / 16;
	int aRow = (int)theEnemy->y / 16;
	if (gLevels.current->columns[aCol][aRow].kind != 0)
	{
		for (int n = 0x12; n != 0; n--)
		{
			int anAngle = RandRange(0, 0x167);
			float aVx = (float)((double)CosDeg(anAngle) * 0.11999999731779099);
			float aVy = (float)((double)SinDeg(anAngle) * 0.11999999731779099);
			ParticleSystem_New();
			ParticleSystem_Add(gLevels.current->systems.current, theEnemy->x, theEnemy->y, aVx, aVy,
				theEnemy->firstParticle, theEnemy->lastParticle, 0, 0xff, 0x80, 0x80, 0xff, 0x80, 0x80, 0xf0, 10, 600,
				0.999f, 0.0005f, 5);
		}
		return 0;
	}
	return 1;
}

// A cell around it is being captured
int Enemy_NearCapture(Enemy* theEnemy)
{
	int aCol = (int)theEnemy->x / 16;
	int aRow = (int)theEnemy->y / 16;
	for (int r = aRow - 1; r <= aRow + 1; r++)
	{
		for (int k = aCol - 1; k <= aCol + 1; k++)
		{
			if ((uint32_t)k < 0x32 && (uint32_t)r < 0x25 && gLevels.current->columns[k][r].state == 5)
				return 1;
		}
	}
	return 0;
}

// Its animation by the way it goes
void Enemy_Face(Enemy* theEnemy)
{
	if (theEnemy->vx < 0.0f)
	{
		theEnemy->anim = theEnemy->leftAnim;
		return;
	}
	if (0.0f < theEnemy->vx)
	{
		theEnemy->anim = theEnemy->rightAnim;
		return;
	}
	if (theEnemy->vy < 0.0f)
	{
		theEnemy->anim = theEnemy->downAnim;
		return;
	}
	if (0.0f < theEnemy->vy)
		theEnemy->anim = theEnemy->upAnim;
}

void Enemy_UpdateShield(Enemy* theEnemy)
{
	if (theEnemy->shield != nullptr)
	{
		theEnemy->shield->x = theEnemy->x;
		theEnemy->shield->y = theEnemy->y;
		if (Obj_Update(theEnemy->shield) == 0)
		{
			if (theEnemy->shield != nullptr)
				Obj_Delete(theEnemy->shield);
			theEnemy->shield = nullptr;
		}
	}
}

// A shield flash where a beam touched it (again from the start if it is showing)
void Enemy_Shield(Enemy* theEnemy)
{
	if (theEnemy->shield == nullptr)
	{
		Obj* aShield = (Obj*)::operator new(sizeof(Obj));
		Obj_Init(aShield);
		theEnemy->shield = aShield;
		aShield->x = theEnemy->x;
		// (sic: the original's y is the enemy's x, until the next Enemy_UpdateShield)
		theEnemy->shield->y = theEnemy->x;
		theEnemy->shield->vx = 0.0f;
		theEnemy->shield->vy = 0.0f;
		theEnemy->shield->anim = Animation_Find("enemy_shield", &theEnemy->shield->frameCount,
			&theEnemy->shield->frameTime);
		theEnemy->shield->midHandle = 1;
		theEnemy->shield->drawKind = 2;
		theEnemy->shield->dirty = 1;
		theEnemy->shield->oneShot = 1;
		return;
	}
	theEnemy->shield->frame = 0;
}

// 0x407070: No other roller (kind 0x11 or 0x12) within 120 pixels
int Enemy_ClearOfRollers(Enemy* theEnemy)
{
	int aResult = 1;
	for (Enemy* anEnemy = gLevels.current->enemies.head; anEnemy != nullptr; anEnemy = Node_Next(anEnemy))
	{
		if (anEnemy != theEnemy && 0x10 < anEnemy->kind && anEnemy->kind < 0x13)
		{
			double dy = (double)theEnemy->y - (double)anEnemy->y;
			double dx = (double)theEnemy->x - (double)anEnemy->x;
			if ((int)(dx * dx + dy * dy) < 0x3840)
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
	for (Enemy* anEnemy = gLevels.current->enemies.head; anEnemy != nullptr; anEnemy = Node_Next(anEnemy))
	{
		if (anEnemy != theEnemy && 0xf < anEnemy->kind && anEnemy->kind < 0xb)
		{
			double dy = (double)theEnemy->y - (double)anEnemy->y;
			double dx = (double)theEnemy->x - (double)anEnemy->x;
			if ((int)(dx * dx + dy * dy) < 10000)
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
					if (gLevels.current->columns[c][r].kind != 0)
						aBlocked = true;
				}
			}
			if (!aBlocked)
			{
				theEnemy->x = (float)(aCol * 0x10 + 8);
				theEnemy->y = (float)(aRow * 0x10 + 8);
				if (Enemy_ClearOfRollers(theEnemy) != 0)
				{
					int anAngle = RandRange(0, 0x167);
					float aSquare = (float)((double)theEnemy->vx * theEnemy->vx + (double)theEnemy->vy * theEnemy->vy);
					float aSpeed = (float)std::sqrt((double)aSquare);
					theEnemy->vx = CosDeg(anAngle) * aSpeed;
					theEnemy->vy = SinDeg(anAngle) * aSpeed;
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
	if (theEnemy->shield != nullptr)
		Obj_Delete(theEnemy->shield);
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
	Shooter* aShip = gLevels.current->shooters.current;
	if (aShip != nullptr)
	{
		float aSquare = (float)((double)theEnemy->vx * theEnemy->vx + (double)theEnemy->vy * theEnemy->vy);
		float aSpeed = (float)std::sqrt((double)aSquare);
		float aX = (float)(((double)aShip->x - theEnemy->x) / 12000.0 + (double)theEnemy->turnSpeed * theEnemy->vx);
		float aY = (float)(((double)aShip->y - theEnemy->y) / 12000.0 + (double)theEnemy->turnSpeed * theEnemy->vy);
		List_Each(&gLevels.current->enemies, [&](Enemy* anOther) {
			if (anOther != theEnemy && anOther->kind == 3)
			{
				float dx = anOther->x - theEnemy->x;
				float dy = anOther->y - theEnemy->y;
				float aSum = (float)((double)dx * dx + (double)dy * dy);
				float aDist = std::fabs(aSum) * 70.0f;
				if (aDist != 0.0f)
				{
					float aT = (float)((double)aX - (double)dx / aDist);
					aX = (float)((double)aT - (double)dy / aDist);
				}
			}
		});
		gLevels.current->enemies.current = theEnemy;
		float anAngle = (float)std::atan2((double)aY, (double)aX);
		float aCos = (float)std::cos((double)anAngle);
		theEnemy->vx = aCos * aSpeed;
		float aSin = (float)std::sin((double)anAngle);
		theEnemy->vy = aSin * aSpeed;
	}
	Enemy_Bounce(theEnemy);
}

// 0x478608: the order Walker_Turn goes round the ways in (0 right, 1 left, 2 down, 3 up); 0x460a0c: not set up yet
static int sWays[4];
static int sWaysUnset = 1;

// Stops, then tries up to four ways round from a random one for the first along an edge or a beam's line
void Walker_Turn(Enemy* theEnemy)
{
	if (sWaysUnset != 0)
	{
		sWays[0] = 0;
		sWays[1] = 2;
		sWays[2] = 1;
		sWays[3] = 3;
		sWaysUnset = 0;
	}
	theEnemy->vx = 0.0f;
	theEnemy->vy = 0.0f;
	int aCol = (int)theEnemy->x / 16;
	int aRow = (int)theEnemy->y / 16;
	int aWay = RandRange(0, 0x27) / 10;
	int aTry = 0;
	do
	{
		if (theEnemy->vx != 0.0f || theEnemy->vy != 0.0f)
			return;
		aWay = aWay + 1;
		if (3 < aWay)
			aWay = 0;
		int aKind;
		switch (sWays[aWay])
		{
		case 0:
			if (aCol < 0x31)
			{
				aKind = gLevels.current->columns[aCol + 1][aRow].kind;
				if (aKind == 3 || aKind == 2)
					theEnemy->vx = theEnemy->speed;
			}
			break;
		case 1:
			if (0 < aCol)
			{
				aKind = gLevels.current->columns[aCol - 1][aRow].kind;
				if (aKind == 3 || aKind == 2)
					theEnemy->vx = -theEnemy->speed;
			}
			break;
		case 2:
			if (aRow < 0x24)
			{
				aKind = gLevels.current->columns[aCol][aRow + 1].kind;
				if (aKind == 3 || aKind == 2)
					theEnemy->vy = theEnemy->speed;
			}
			break;
		case 3:
			if (0 < aRow)
			{
				aKind = gLevels.current->columns[aCol][aRow - 1].kind;
				if (aKind == 3 || aKind == 2)
					theEnemy->vy = -theEnemy->speed;
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
	List_Each(&gLevels.current->shooters.current->beams, [&](Beam* aBeam) {
		int aLength = (int)(std::fabs((double)aBeam->startX - aBeam->x) + std::fabs((double)aBeam->startY - aBeam->y));
		for (int i = 0; i < aLength; i++)
		{
			int anAngle = RandRange(0, 0x167);
			float aCos = CosDeg(anAngle);
			float aSin = SinDeg(anAngle);
			int aSx = Sign(aBeam->vx);
			int aSy = Sign(aBeam->vy);
			ParticleSystem_New();
			Cell* aCell = gLevels.current->columns[0];
			ParticleSystem_Add(gLevels.current->systems.current, (float)(aSx * i) + aBeam->startX,
				(float)(aSy * i) + aBeam->startY, (float)((double)aCos * 0.11999999731779099),
				(float)((double)aSin * 0.11999999731779099), aCell->firstParticle, aCell->lastParticle, 0, 0xff, 0xff,
				0xff, 0xff, 0xff, 0xff, 0x21c, 10, 700, 0.999f, 0.0001f, 5);
		}
	});
	Shooter_DropBeams(gLevels.current->shooters.current);
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
			if (gLevels.current->columns[aCol][aRow].kind == 3)
			{
				theEnemy->x = (float)(aCol * 0x10 + 8);
				theEnemy->y = (float)(aRow * 0x10 + 8);
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
		ParticleSystem_Add(gLevels.current->systems.current, theEnemy->x, theEnemy->y, aVx, aVy, 0, 0, 0, 0xff, 0xff,
			0xff, 0xff, 0xff, 0xff, 0xff, 0, aLifetime, 1.0f, 0.0f, 4);
		Particle* aParticle = gLevels.current->systems.current->particles.current;
		int aFrameTime;
		aParticle->anim = Animation_Find("sparkles3", &aParticle->frameCount, &aFrameTime);
		aParticle->tint = RandRange(0, 7);
	}
}

// Kind 0x10: loops the first 32 frames of its animation for 5000 updates, its children circling it for the first
// 2000; then plays on, and past frame 48 vanishes in sparkles to come back somewhere else, a trail of them between
int Teleporter_Move(Enemy* theEnemy)
{
	if (theEnemy->phase == 0)
	{
		if (theEnemy->age <= 5000.0f)
		{
			if (0x1f < theEnemy->frame)
			{
				theEnemy->frame = 0;
				theEnemy->frameTimer = 0;
			}
		}
		else if (0x1f < theEnemy->frame)
			theEnemy->phase = 1;
		if (2000.0f <= theEnemy->age || gLevels.current->age <= theEnemy->age)
		{
			bool aMore;
			do
				aMore = List_RemoveCurrent(&theEnemy->children);
			while (aMore);
			return Enemy_Bounce(theEnemy);
		}
		double anAlpha = (double)theEnemy->age * 0.25;
		if (255.0 < anAlpha)
			anAlpha = 255.0;
		theEnemy->alpha = (int)anAlpha;
		int anOffset = 0;
		List_Each(&theEnemy->children, [&](Obj* aChild) {
			int anAngle = ((int)((double)theEnemy->age * 0.25) % 0x168 + anOffset) % 0x168;
			aChild->x = (float)((double)CosDeg(anAngle) * 40.0 + (double)theEnemy->x);
			anOffset = anOffset + 0x1e;
			aChild->y = (float)((double)SinDeg(anAngle) * 40.0 + (double)theEnemy->y);
			theEnemy->drawKind = 4;
		});
	}
	else if (0x30 < theEnemy->frame)
	{
		Teleporter_Sparkles(theEnemy);
		float anOldX = theEnemy->x;
		float anOldY = theEnemy->y;
		Enemy_Place(theEnemy);
		theEnemy->phase = 0;
		theEnemy->age = 1.0f;
		theEnemy->frame = 0;
		theEnemy->frameTimer = 0;
		Teleporter_Sparkles(theEnemy);
		double dy = (double)anOldY - theEnemy->y;
		double dx = (double)anOldX - theEnemy->x;
		float aSquare = (float)(dx * dx + dy * dy);
		float aDist = (float)std::sqrt((double)aSquare);
		float aSteps = (float)((double)aDist / 15.0);
		for (float t = 0.0f; t < aSteps; t = t + 1.0f)
		{
			float f = t / aSteps;
			float aX = (float)(((double)theEnemy->x - anOldX) * f + anOldX);
			float aY = (float)(((double)theEnemy->y - anOldY) * f + anOldY);
			ParticleSystem_New();
			int aLifetime = RandRange(400, 900);
			ParticleSystem_Add(gLevels.current->systems.current, aX, aY, 0.0f, 0.0f, 0, 0, 0, 0xff, 0xff, 0xff, 0xff,
				0xff, 0xff, 0xff, 0, aLifetime, 1.0f, 0.0f, 4);
			Particle* aParticle = gLevels.current->systems.current->particles.current;
			int aFrameTime;
			aParticle->anim = Animation_Find("sparkles3", &aParticle->frameCount, &aFrameTime);
		}
	}
	return Enemy_Bounce(theEnemy);
}

// 5000 updates after its last turn there: on crossing a cell's middle beside a way off to the side (an edge, or a
// line of theKind), turns that way half the time
void Walker_TurnAtJunction(Enemy* theEnemy, int theKind)
{
	if ((float)(theEnemy->lastTurn + 5000) <= theEnemy->age)
	{
		int aCol = (int)theEnemy->x / 16;
		float aMidX = (float)(aCol * 0x10 + 8);
		int aRow = (int)theEnemy->y / 16;
		float aMidY = (float)(aRow * 0x10 + 8);
		if (theEnemy->vx == 0.0f)
		{
			if (((aMidY < theEnemy->y && theEnemy->oldY < aMidY) || (theEnemy->y < aMidY && aMidY < theEnemy->oldY)) &&
				RandRange(0, 1) != 0 && (uint32_t)(aCol - 1) < 0x30)
			{
				int aRight = gLevels.current->columns[aCol + 1][aRow].kind;
				int aLeft = gLevels.current->columns[aCol - 1][aRow].kind;
				if (aRight == 3 || aRight == theKind || aLeft == 3 || aLeft == theKind)
				{
					theEnemy->x = (float)(((int)theEnemy->x / 16) * 0x10 + 8);
					theEnemy->y = (float)(((int)theEnemy->y / 16) * 0x10 + 8);
					while (theEnemy->vx == 0.0f)
					{
						theEnemy->vx = 0.0f;
						theEnemy->vy = 0.0f;
						Walker_Turn(theEnemy);
					}
					// (the original keeps the last turn's age only for turns off a horizontal way)
				}
			}
		}
		else if (((aMidX < theEnemy->x && theEnemy->oldX < aMidX) || (theEnemy->x < aMidX && aMidX < theEnemy->oldX)) &&
			RandRange(0, 1) != 0 && (uint32_t)(aRow - 1) < 0x23)
		{
			int aBelow = gLevels.current->columns[aCol][aRow + 1].kind;
			int anAbove = gLevels.current->columns[aCol][aRow - 1].kind;
			if (aBelow == 3 || aBelow == theKind || anAbove == 3 || anAbove == theKind)
			{
				theEnemy->x = (float)(((int)theEnemy->x / 16) * 0x10 + 8);
				theEnemy->y = (float)(((int)theEnemy->y / 16) * 0x10 + 8);
				while (theEnemy->vy == 0.0f)
				{
					theEnemy->vx = 0.0f;
					theEnemy->vy = 0.0f;
					Walker_Turn(theEnemy);
				}
				theEnemy->lastTurn = (int)theEnemy->age;
			}
		}
	}
}

// Where the cell half a cell ahead is no edge or line of theKind (or is on the board's rim), stops in the middle of
// its cell and turns, trying up to 21 times for a way across the one it had
void Walker_Follow(Enemy* theEnemy, int theKind)
{
	if (theEnemy->vx == 0.0f)
	{
		int aCol = (int)theEnemy->x / 16;
		int aSy = Sign(theEnemy->vy);
		int aRow = (int)((double)theEnemy->vy + (double)theEnemy->y + (double)((aSy * 0x10) / 2)) / 16;
		bool aBlocked = 0x22 < (uint32_t)(aRow - 1);
		if (!aBlocked)
		{
			int aKind = gLevels.current->columns[aCol][aRow].kind;
			aBlocked = aKind != 3 && aKind != theKind;
		}
		if (aBlocked)
		{
			theEnemy->x = (float)(aCol * 0x10 + 8);
			theEnemy->y = (float)(((int)theEnemy->y / 16) * 0x10 + 8);
			Walker_Turn(theEnemy);
			int aTry = 0;
			do
			{
				if (theEnemy->vx != 0.0f)
					return;
				Walker_Turn(theEnemy);
				aTry = aTry + 1;
			} while (aTry < 0x14);
		}
	}
	else
	{
		int aSx = Sign(theEnemy->vx);
		int aCol = (int)((double)theEnemy->x + (double)theEnemy->vx + (double)((aSx * 0x10) / 2)) / 16;
		int aRow = (int)theEnemy->y / 16;
		bool aBlocked = 0x2f < (uint32_t)(aCol - 1);
		if (!aBlocked)
		{
			int aKind = gLevels.current->columns[aCol][aRow].kind;
			aBlocked = aKind != 3 && aKind != theKind;
		}
		if (aBlocked)
		{
			theEnemy->x = (float)(((int)theEnemy->x / 16) * 0x10 + 8);
			theEnemy->y = (float)(aRow * 0x10 + 8);
			Walker_Turn(theEnemy);
			int aTry = 0;
			while (theEnemy->vy == 0.0f)
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
	theEnemy->leftAnim = theDef->leftAnim;
	theEnemy->rightAnim = theDef->rightAnim;
	theEnemy->upAnim = theDef->upAnim;
	theEnemy->downAnim = theDef->downAnim;
	theEnemy->phase = theDef->phase;
	theEnemy->firstParticle = theDef->firstParticle;
	theEnemy->lastParticle = theDef->lastParticle;
	theEnemy->baseSpeed = theDef->baseSpeed;
	theEnemy->baseAcceleration = theDef->baseAcceleration;
	theEnemy->baseFrameTime = theDef->baseFrameTime;
	theEnemy->acceleration = theDef->acceleration;
	theEnemy->decay = theDef->decay;
	theEnemy->turnSpeed = theDef->turnSpeed;
	theEnemy->lastShot = theDef->lastShot;
	List_Each(&theDef->emitters, [&](Obj* anEmitter) {
		Emitter_Append(&theEnemy->emitters);
		Emitter_Copy(static_cast<Emitter*>(theEnemy->emitters.current), static_cast<Emitter*>(anEmitter));
		static_cast<Emitter*>(theEnemy->emitters.current)->parent = theEnemy;
	});
}

int gKillCount = 0; // 0x478608: the kills since a power-up was last allowed (every second and fourth of five)

// 0x408a30: Dies, exploding where it is. A walker or roller scores 100 for each enemy killed while this capture fills (at
// most 900; thePoints instead when given) times the ship's multiplier, and now and then leaves a power-up while the
// board is short of its quota.
void Enemy_Kill(Enemy* theEnemy, int thePoints)
{
	gProfiles[gProfile].kills = gProfiles[gProfile].kills + 1;
	Obj_Explode(theEnemy, 0.0f, 0.0f);
	int aKind = theEnemy->kind;
	if (aKind != 0x13 && aKind != 0xd && aKind != 0xe)
	{
		int aPoints = gLevels.current->chain * 100;
		if (thePoints != 0)
			aPoints = thePoints;
		char aSound[64];
		std::sprintf(aSound, "audio\\exp%d.wav", aPoints);
		Sound_Play(Sound_Find(aSound));
		if (900 < aPoints)
			aPoints = 900;
		char aText[64];
		std::sprintf(aText, "%d", aPoints);
		Shooter* aShip = gLevels.current->shooters.current;
		if (aShip == nullptr || aShip->cellPoints < 2)
			gScore = gScore + aPoints;
		else
		{
			gScore = gScore + aShip->cellPoints * aPoints;
			char aTimes[16];
			std::sprintf(aTimes, "^5X%d", gLevels.current->shooters.current->cellPoints);
			Text_Append(&gLevels.current->texts);
			Text_Make(gLevels.current->texts.current, aTimes, Font_Find("bonus_font"),
				(int)((double)theEnemy->x + 44.0), (int)theEnemy->y, 0, 0x28, 2000, 300, 0, 0);
			gLevels.current->texts.current->vy = -0.02f;
		}
		Text_Append(&gLevels.current->texts);
		Text_Make(gLevels.current->texts.current, aText, Font_Find("bonus_font2"), (int)theEnemy->x, (int)theEnemy->y,
			0, 0x28, 2000, 300, 0, 0);
		gLevels.current->texts.current->vy = -0.02f;
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
			Text_Append(&gLevels.current->texts);
			Text_Make(gLevels.current->texts.current, aCombo, Font_Find("bonus_font"), (int)theEnemy->x,
				(int)((double)theEnemy->y + 10.0), 0, 0x28, 2500, 300, 0, 0);
			gLevels.current->texts.current->vy = 0.0f;
		}
		gLevels.current->chain = gLevels.current->chain + 1;
		RandRange(1, 10);
		gKillCount = gKillCount + 1;
		if (4 < gKillCount)
		{
			gKillCount = 0;
			return;
		}
		if ((gKillCount == 2 || gKillCount == 4) &&
			((gLevels.current->unk3c0 + gLevels.current->filledCells) * 100) / gLevels.current->totalCells <
				gLevels.current->fillQuota)
			Powerup_Insert(&gLevels.current->powerups);
	}
}

// Kind 0x12: shotDelay updates after its last shot fires a snake (kind 0x13) from 50 pixels out, at an angle its
// animation's frame and a random quarter turn give, when the cell there is open
int SnakeShooter_Move(Enemy* theEnemy)
{
	int aResult = Enemy_Bounce(theEnemy);
	if ((double)theEnemy->shotDelay + (double)theEnemy->lastShot < (double)theEnemy->age)
	{
		int aCount = theEnemy->frameCount;
		int anAngle = RandRange(0, 3) * 0x5a + (theEnemy->frame * 0x5a) / aCount + 0x168 / aCount;
		if (0x167 < anAngle)
			anAngle = anAngle + -0x168;
		int aX = (int)((double)CosDeg(anAngle) * 50.0 + (double)theEnemy->x);
		int aY = (int)((double)SinDeg(anAngle) * 50.0 + (double)theEnemy->y);
		if (gLevels.current->columns[aX / 16][aY / 16].kind == 0)
		{
			Sound_Play(Sound_Find("audio\\canon.wav"));
			theEnemy->lastShot = theEnemy->age;
			Enemy_Append(&gLevels.current->enemies);
			Enemy* aShot = gLevels.current->enemies.current;
			Enemy_CopyDef(aShot, theEnemy);
			aShot->age = 0.0f;
			aShot->lifetime = 0.0f;
			aShot->kind = 0x13;
			aShot->state = 3;
			aShot->anim = Animation_Find("snake", &aShot->frameCount, &aShot->frameTime);
			aShot->dieAnim = Animation_Find("small_explo", &aShot->dieFrameCount, &aShot->dieFrameTime);
			aShot->depth = theEnemy->depth + 1;
			aShot->midHandle = 1;
			aShot->vx = (float)((double)CosDeg(anAngle) * 0.3499999940395355);
			aShot->vy = (float)((double)SinDeg(anAngle) * 0.3499999940395355);
			aShot->x = (float)((double)CosDeg(anAngle) * 50.0 + (double)theEnemy->x);
			aShot->y = (float)((double)SinDeg(anAngle) * 50.0 + (double)theEnemy->y);
			Emitter_Append(&aShot->emitters);
			Emitter_Setup(static_cast<Emitter*>(aShot->emitters.current), aShot, 0, 0, 0.0f, 0.0f, 0x28, 0, "spark", 2,
				255.0f, 0.0f, 2000.0f);
			for (int n = 0x28; n != 0; n--)
			{
				ParticleSystem_New();
				float aDrag = (float)((double)RandRange(-100, 0) / 1000.0 + 1.0);
				int aLifetime = RandRange(400, 0x514);
				int aLast = theEnemy->lastParticle;
				int aFirst = theEnemy->firstParticle;
				float aVy = (float)((double)RandRange(-50, 50) / 1200.0 + (double)aShot->vy);
				float aVx = (float)((double)RandRange(-50, 50) / 1200.0 + (double)aShot->vx);
				float aPy = (float)RandRange(-5, 5) + aShot->y;
				float aPx = (float)RandRange(-5, 5) + aShot->x;
				ParticleSystem_Add(gLevels.current->systems.current, aPx, aPy, aVx, aVy, aFirst, aLast, 0, 0xff, 0xff,
					0xe6, 100, 0x32, 0, 0xff, 10, aLifetime, aDrag, 0.0f, 5);
			}
			gLevels.current->enemies.current = theEnemy;
			for (int i = 0; i < aShot->anim->count; i++)
				Image_SetMidHandle(aShot->anim->frames[i]);
		}
	}
	return aResult;
}

// Kind 0x13, a snake shooter's snake: bursts on hitting anything not open, holing the board 3 cells round
int Snake_Move(Enemy* theEnemy)
{
	theEnemy->hitCell = nullptr;
	Enemy_Bounce(theEnemy);
	if (theEnemy->hitCell != nullptr)
	{
		Enemy_Kill(theEnemy, 0);
		Level_Blast((int)theEnemy->x / 16, (int)theEnemy->y / 16, 3);
	}
	if (gLevels.current->columns[(int)theEnemy->x / 16][(int)theEnemy->y / 16].kind != 0)
		Enemy_Kill(theEnemy, 0);
	return 1;
}

// Kind 0xd, a cannon walker's ball: rises and falls through its animation over its lifetime, then bursts, holing
// the board 3 cells round (0: done)
int CannonBall_Move(Enemy* theEnemy)
{
	if (theEnemy->age < theEnemy->lifetime)
	{
		float aSin = SinDeg((int)((double)theEnemy->age * 180.0 / (double)theEnemy->lifetime));
		Animation* anAnim = theEnemy->anim;
		int aCount = anAnim->count;
		int aFrame = (int)((double)aCount * (double)aSin);
		if (aCount <= aFrame)
			aFrame = aCount + -1;
		theEnemy->image = Animation_Frame(anAnim, aFrame);
		return 1;
	}
	Enemy_Kill(theEnemy, 0);
	Level_Blast((int)theEnemy->x / 16, (int)theEnemy->y / 16, 3);
	return 0;
}

// A walker, along the edges and the lines of theKind: dies in the open after its first 2000 updates, and turns where
// it stands still, now and then at junctions, and where its way ends
int Walker_Move(Enemy* theEnemy, int theKind)
{
	if (2000.0f < theEnemy->age &&
		gLevels.current->columns[(int)theEnemy->x / 16][(int)theEnemy->y / 16].kind == 0)
	{
		Enemy_Kill(theEnemy, 0);
		return 1;
	}
	if (theEnemy->vx == 0.0f && theEnemy->vy == 0.0f)
		Walker_Turn(theEnemy);
	Walker_TurnAtJunction(theEnemy, theKind);
	Walker_Follow(theEnemy, theKind);
	return 0;
}

// Kind 8: speeds up (to three times its speed) while the ship has beams out, and slows back down without
void ScurryWalker_Move(Enemy* theEnemy)
{
	Shooter* aShip = gLevels.current->shooters.current;
	float aSpeed;
	if (aShip == nullptr || aShip->beams.current == nullptr)
	{
		aSpeed = theEnemy->decay * theEnemy->speed;
		if (aSpeed < theEnemy->baseSpeed)
			aSpeed = theEnemy->baseSpeed;
	}
	else
	{
		float aMax = theEnemy->baseSpeed * 7.0f;
		aSpeed = theEnemy->acceleration * theEnemy->speed;
		if (aMax < aSpeed)
			aSpeed = aMax;
	}
	theEnemy->speed = aSpeed;
	theEnemy->frameTime = (int)((double)theEnemy->baseFrameTime * theEnemy->baseSpeed / theEnemy->speed);
	theEnemy->vx = (float)Sign(theEnemy->vx) * theEnemy->speed;
	theEnemy->vy = (float)Sign(theEnemy->vy) * theEnemy->speed;
	Walker_Move(theEnemy, 2);
}

// Kind 0x14: 1000 updates after it last did, turns towards the ship when that is within 150 pixels; slows (to a third
// of its speed) while a beam across its way is that near, and speeds back up without
void SmartWalker_Move(Enemy* theEnemy)
{
	Shooter* aShip = gLevels.current->shooters.current;
	if ((double)theEnemy->lastShot + 1000.0 < (double)theEnemy->age && aShip != nullptr)
	{
		double dy = (double)aShip->y - theEnemy->y;
		double dx = (double)aShip->x - theEnemy->x;
		if (dx * dx + dy * dy < 22500.0)
		{
			theEnemy->lastShot = theEnemy->age;
			theEnemy->vx = std::fabs(theEnemy->vx) * (float)Sign(gLevels.current->shooters.current->x - theEnemy->x);
			theEnemy->vy = std::fabs(theEnemy->vy) * (float)Sign(gLevels.current->shooters.current->y - theEnemy->y);
			Enemy_Face(theEnemy);
		}
	}
	bool aNear = false;
	aShip = gLevels.current->shooters.current;
	if (aShip != nullptr && aShip->beams.current != nullptr)
	{
		List_Each(&aShip->beams, [&](Beam* aBeam) {
			double dy = (double)theEnemy->y - aBeam->y;
			double dx = (double)theEnemy->x - aBeam->x;
			if (dx * dx + dy * dy < 22500.0 &&
				((aBeam->vx != 0.0f && theEnemy->vy != 0.0f) || (aBeam->vy != 0.0f && theEnemy->vx != 0.0f)))
				aNear = true;
		});
	}
	float aSpeed;
	if (aNear)
	{
		float aMin = (float)((double)theEnemy->baseSpeed / 3.0);
		aSpeed = theEnemy->decay * theEnemy->speed;
		if (aSpeed < aMin)
			aSpeed = aMin;
	}
	else
	{
		aSpeed = theEnemy->acceleration * theEnemy->speed;
		if (theEnemy->baseSpeed < aSpeed)
			aSpeed = theEnemy->baseSpeed;
	}
	theEnemy->speed = aSpeed;
	theEnemy->frameTime = (int)((double)theEnemy->baseFrameTime * theEnemy->baseSpeed / theEnemy->speed);
	theEnemy->vx = (float)Sign(theEnemy->vx) * theEnemy->speed;
	theEnemy->vy = (float)Sign(theEnemy->vy) * theEnemy->speed;
	Walker_Move(theEnemy, 2);
}

// Kind 0xb: 4000 updates after its last shot, when the 9 by 9 cells 16 cells ahead of it hold 29 filled ones or
// lines, fires a cannon ball (kind 0xd) that way in a puff of smoke
void CannonWalker_Move(Enemy* theEnemy)
{
	if ((double)theEnemy->lastShot + 4000.0 < (double)theEnemy->age)
	{
		int aCount = 1;
		int aCol = (int)theEnemy->x / 16 + Sign(theEnemy->vx) * 0x10;
		int aRow = (int)theEnemy->y / 16 + Sign(theEnemy->vy) * 0x10;
		Level* aLevel = gLevels.current;
		for (int r = aRow - 4; r <= aRow + 4; r++)
		{
			for (int c = aCol - 4; c <= aCol + 4; c++)
			{
				if (aCount != 0 && -1 < c && c < aLevel->cols && -1 < r && r < aLevel->rows &&
					(aLevel->columns[c][r].kind == 4 || aLevel->columns[c][r].kind == 2))
					aCount = aCount + 1;
				// (sic: the original's reset can never happen)
				else if (c < 0 && aLevel->cols <= c && r < 0 && aLevel->rows <= r)
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
				int aSy = Sign(theEnemy->vy);
				int aSx = Sign(theEnemy->vx);
				int aLifetime = RandRange(200, 300);
				int aLast = theEnemy->lastParticle;
				int aFirst = theEnemy->firstParticle;
				float aVy = (float)((double)theEnemy->vy + theEnemy->vy + (double)RandRange(-50, 50) / 1200.0);
				float aVx = (float)((double)theEnemy->vx + theEnemy->vx + (double)RandRange(-50, 50) / 1200.0);
				float aY = (float)((double)RandRange(-5, 5) + theEnemy->y + (double)aSy * 15.0);
				float aX = (float)((double)RandRange(-5, 5) + theEnemy->x + (double)aSx * 15.0);
				ParticleSystem_Add(gLevels.current->systems.current, aX, aY, aVx, aVy, aFirst, aLast, 0, 0xff, 0xff,
					0xe6, 100, 0x32, 0, 0xff, 10, aLifetime, 0.999f, 0.0f, 5);
			}
			theEnemy->lastShot = theEnemy->age;
			Enemy_Append(&gLevels.current->enemies);
			Enemy* aBall = gLevels.current->enemies.current;
			Enemy_CopyDef(aBall, theEnemy);
			aBall->kind = 0xd;
			aBall->state = 3;
			aBall->x = (float)((double)Sign(theEnemy->vx) * 16.0 + (double)theEnemy->x);
			aBall->y = (float)((double)Sign(theEnemy->vy) * 16.0 + (double)theEnemy->y);
			aBall->age = 0.0f;
			aBall->lifetime = 1500.0f;
			aBall->vx = (float)((double)Sign(theEnemy->vx) * 240.0 / 1500.0);
			aBall->vy = (float)((double)Sign(theEnemy->vy) * 240.0 / 1500.0);
			aBall->anim = Animation_Find("cannon_ball", &aBall->frameCount, &aBall->frameTime);
			aBall->depth = theEnemy->depth + 1;
			aBall->midHandle = 1;
			aBall->dieAnim = Animation_Find("small_explo", &aBall->dieFrameCount, &aBall->dieFrameTime);
			gLevels.current->enemies.current = theEnemy;
			for (int i = 0; i < aBall->anim->count; i++)
				Image_SetMidHandle(aBall->anim->frames[i]);
		}
	}
	Walker_Move(theEnemy, 2);
}

// Kind 0xb: faces the ship across its way; 4000 updates after its last shot, while it is within the length of a
// beam going its way, shoots a laser (kind 0xd) at it
void LaserWalker_Move(Enemy* theEnemy)
{
	Animation* anAnim = theEnemy->downAnim;
	theEnemy->anim = anAnim;
	Shooter* aShip = gLevels.current->shooters.current;
	float aDx;
	float aDy;
	if (theEnemy->vx == 0.0f)
	{
		if (aShip == nullptr || aShip->x <= theEnemy->x)
			aDx = -1.0f;
		else
		{
			anAnim = theEnemy->upAnim;
			aDx = 1.0f;
		}
		aDy = 0.0f;
		theEnemy->anim = anAnim;
	}
	else if (aShip != nullptr && aShip->y < theEnemy->y)
	{
		aDx = 0.0f;
		aDy = -1.0f;
		theEnemy->anim = theEnemy->leftAnim;
	}
	else
	{
		aDx = 0.0f;
		theEnemy->anim = theEnemy->rightAnim;
		aDy = 1.0f;
	}
	if ((double)theEnemy->age <= (double)theEnemy->lastShot + 4000.0)
	{
		Walker_Move(theEnemy, 2);
		return;
	}
	bool aLinedUp = false;
	aShip = gLevels.current->shooters.current;
	if (aShip != nullptr && aShip->beams.current != nullptr)
	{
		List_Each(&aShip->beams, [&](Beam* aBeam) {
			if (aBeam->vx == 0.0f || theEnemy->vx == 0.0f)
			{
				if (aBeam->vy != 0.0f && theEnemy->vy != 0.0f &&
					((aBeam->startY < theEnemy->y && theEnemy->y < aBeam->y) ||
						(theEnemy->y < aBeam->startY && aBeam->y < theEnemy->y)))
					aLinedUp = true;
			}
			else if ((aBeam->startX < theEnemy->x && theEnemy->x < aBeam->x) ||
				(theEnemy->x < aBeam->startX && aBeam->x < theEnemy->x))
				aLinedUp = true;
		});
		if (aLinedUp)
		{
			// (the original works out a pan from x for a play call that takes none)
			Sound_Play(Sound_Find("audio\\laser.wav"));
			theEnemy->lastShot = theEnemy->age;
			Enemy_Append(&gLevels.current->enemies);
			Enemy* aShot = gLevels.current->enemies.current;
			Enemy_CopyDef(aShot, theEnemy);
			aShot->age = 0.0f;
			aShot->state = 3;
			aShot->lifetime = 0.0f;
			aShot->kind = 0xe;
			aShot->midHandle = 1;
			aShot->depth = theEnemy->depth + 1;
			aShot->x = (float)((double)aDx * (double)theEnemy->image->w * 0.5 + (double)theEnemy->x);
			aShot->y = (float)((double)aDy * (double)theEnemy->image->h * 0.5 + (double)theEnemy->y);
			aShot->vx = (float)((double)aDx * 0.2);
			aShot->vy = (float)((double)aDy * 0.2);
			const char* aName;
			if (0.0f < aShot->vy)
				aName = "laser_shot_up";
			else if (aShot->vy < 0.0f)
				aName = "laser_shot_down";
			else if (0.0f <= aShot->vx)
				aName = "laser_shot_right";
			else
				aName = "laser_shot_left";
			aShot->anim = Animation_Find(aName, &aShot->frameCount, &aShot->frameTime);
			aShot->frame = 0;
			gLevels.current->enemies.current = theEnemy;
			for (int i = 0; i < aShot->anim->count; i++)
				Image_SetMidHandle(aShot->anim->frames[i]);
		}
	}
	Walker_Move(theEnemy, 2);
}

// Kind 0x13: walks the edges alone (kind 3), facing the ship across its way; at frame 12 of its animation it nips
// the cell two ahead of where it faces, holing the board 2 cells round when that is filled or a line
int CrabWalker_Move(Enemy* theEnemy)
{
	Animation* anAnim = theEnemy->downAnim;
	theEnemy->anim = anAnim;
	Shooter* aShip = gLevels.current->shooters.current;
	int aDx;
	int aDy;
	if (theEnemy->vx == 0.0f)
	{
		aDy = 0;
		if (aShip == nullptr || aShip->x <= theEnemy->x)
			aDx = -1;
		else
		{
			anAnim = theEnemy->upAnim;
			aDx = 1;
		}
		theEnemy->anim = anAnim;
	}
	else
	{
		aDx = 0;
		if (aShip == nullptr || theEnemy->y <= aShip->y)
		{
			theEnemy->anim = theEnemy->rightAnim;
			aDy = 1;
		}
		else
		{
			theEnemy->anim = theEnemy->leftAnim;
			aDy = -1;
		}
	}
	int aResult = Walker_Move(theEnemy, 3);
	if (theEnemy->frameTimer == 0 && theEnemy->frame == 0xc)
	{
		int aCol = (int)theEnemy->x / 16 + aDx * 2;
		int aRow = (int)theEnemy->y / 16 + aDy * 2;
		if (0x2d < (uint32_t)(aCol - 2) || 0x20 < (uint32_t)(aRow - 2))
			return aResult;
		int aKind = gLevels.current->columns[aCol][aRow].kind;
		if (aKind == 2 || aKind == 4)
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
				Cell* aCell = &gLevels.current->columns[aCol][aRow];
				float aY = (float)RandRange(-0x19, 0x19) + aCell->y;
				float aX = (float)RandRange(-0x19, 0x19) + gLevels.current->columns[aCol][aRow].x;
				ParticleSystem_Add(gLevels.current->systems.current, aX, aY, aVx, aVy, aFirst, aLast, 0, 0xff, 0xff,
					0xff, 0xe7, 0xa6, 0x39, 0x21c, 10, aLifetime, 1.0f, 0.0f, 4);
			}
		}
	}
	return aResult;
}

// Kind 0x15: its speed swings with the sine of an eighth of its age, its animation keeping step
int LobsterWalker_Move(Enemy* theEnemy)
{
	int aResult = Walker_Move(theEnemy, 2);
	float aSin = SinDeg((int)((double)theEnemy->age * 0.125) % 0x168);
	float aSpeed = (float)(((double)aSin + aSin + 1.0) / 20.0 + (double)theEnemy->baseSpeed);
	theEnemy->speed = aSpeed;
	theEnemy->vx = (float)Sign(theEnemy->vx) * aSpeed;
	theEnemy->vy = (float)Sign(theEnemy->vy) * aSpeed;
	theEnemy->frameTime = (int)(((double)theEnemy->baseSpeed * 3.0 - (double)aSpeed / 1.3) * 200.0);
	return aResult;
}

// A puff of smoke behind a turtle walker, fading to the colour given (0x40a360 has this twice)
static void Turtle_Puff(Enemy* theEnemy, int theEndR, int theEndG, int theEndB)
{
	ParticleSystem_New();
	int aSy = Sign(theEnemy->vy);
	int aSx = Sign(theEnemy->vx);
	double aY = theEnemy->y;
	double aX = theEnemy->x;
	int aLifetime = RandRange(100, 0xfa);
	int aLast = theEnemy->lastParticle;
	int aFirst = theEnemy->firstParticle;
	float aVy = (float)((double)RandRange(-100, 100) / 20000.0 + (double)theEnemy->vy * 0.25);
	float aVx = (float)((double)RandRange(-100, 100) / 10000.0 + (double)theEnemy->vx * 0.25);
	float aPy = (float)(aY - (double)RandRange(-0x1e, 0x1e) / 20.0 - (double)((theEnemy->image->h * aSy) / 3));
	float aPx = (float)(aX - (double)RandRange(-0x1e, 0x1e) / 20.0 - (double)((theEnemy->image->w * aSx) / 3));
	ParticleSystem_Add(gLevels.current->systems.current, aPx, aPy, aVx, aVy, aFirst, aLast, 0, 0xff, 0xff, 0xff,
		theEndR, theEndG, theEndB, 0x154, 10, aLifetime, 1.0f, 0.0f, 5);
}

// Kind 0x14: on turning round stops for 2000 updates (its age set to 2000), playing frames 0..28 at half rate;
// walking it plays frames 29 on and puffs smoke behind it
int TurtleWalker_Move(Enemy* theEnemy)
{
	int aResult = Walker_Move(theEnemy, 2);
	if (Sign(theEnemy->prevVx) != Sign(theEnemy->vx))
		theEnemy->age = 2000.0f;
	theEnemy->prevVx = theEnemy->vx;
	if (theEnemy->age < 2000.0f || 4000.0f < theEnemy->age)
	{
		if (RandRange(0, 3) == 0)
			Turtle_Puff(theEnemy, 0xe7, 0xa6, 0x39);
		if (RandRange(0, 10) == 0)
			Turtle_Puff(theEnemy, 100, 100, 100);
		theEnemy->frameTime = 0xf;
		if (theEnemy->frame < 0x1d)
			theEnemy->frame = 0x1d;
	}
	else
	{
		theEnemy->frameTime = 0x1e;
		theEnemy->x = theEnemy->x - theEnemy->vx;
		theEnemy->y = theEnemy->y - theEnemy->vy;
		if (0x1c < theEnemy->frame)
			theEnemy->frame = 0;
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
	if (theEnemy->image != nullptr &&
		(theEnemy->x <= 0.0f || 800.0f <= theEnemy->x || theEnemy->y <= 0.0f || 592.0f < theEnemy->y))
		return 0;
	if (theEnemy->countdown != 0)
	{
		theEnemy->countdown = theEnemy->countdown + -1;
		if (theEnemy->countdown == 1)
		{
			Enemy_Kill(theEnemy, 0);
			return 1;
		}
	}
	if (gLevels.current->shooters.current != nullptr && (gLevels.current->shooters.current->flags & 2) != 0)
		aPasses = 3;
	for (int aPass = 0; aPass < aPasses; aPass++)
	{
		Shooter* aShip = gLevels.current->shooters.current;
		if (aShip != nullptr && (int)theEnemy->age % 4 != 1 && (aShip->flags & 1) != 0)
		{
			theEnemy->age = theEnemy->age + 1.0f;
			theEnemy->oldX = theEnemy->x;
			theEnemy->oldY = theEnemy->y;
			return 1;
		}
		aResult = aResult | Obj_Update(theEnemy);
		switch (theEnemy->kind)
		{
		case 0:
			Frog_Move(theEnemy);
			break;
		case 1:
			BurstRoller_Move(theEnemy);
			break;
		case 2:
			BumpRoller_Move(theEnemy);
			break;
		case 3:
			Jellyfish_Move(theEnemy);
			break;
		case 4:
			SpinRoller_Move(theEnemy);
			break;
		case 5:
			Teleporter_Move(theEnemy);
			break;
		case 6:
			TurnRoller_Move(theEnemy);
			break;
		case 7:
			LobsterWalker_Move(theEnemy);
			break;
		case 8:
			ScurryWalker_Move(theEnemy);
			break;
		case 9:
			TurtleWalker_Move(theEnemy);
			break;
		case 10:
			LaserWalker_Move(theEnemy);
			break;
		case 0xb:
			CannonWalker_Move(theEnemy);
			break;
		case 0xc:
			CrabWalker_Move(theEnemy);
			break;
		case 0xd:
			CannonBall_Move(theEnemy);
			break;
		case 0xe:
			aResult = aResult & LaserShot_Move(theEnemy);
			break;
		case 0x10:
			Walker_Move(theEnemy, 2);
			break;
		case 0x11:
			Enemy_Bounce(theEnemy);
			break;
		case 0x12:
			SnakeShooter_Move(theEnemy);
			break;
		case 0x13:
			Snake_Move(theEnemy);
			break;
		case 0x14:
			SmartWalker_Move(theEnemy);
			break;
		}
		Obj_UpdateEmitters(theEnemy);
		Beam* aBeam = Obj_TouchedBeam(theEnemy, theEnemy->kind == 0 ? 2 : 3, theEnemy->kind == 0 ? 5 : 3);
		if (aBeam == theEnemy->lastBeam || theEnemy->kind == 0x13 || theEnemy->kind == 0xd ||
			theEnemy->kind == 0xe)
		{
			if (aBeam != nullptr)
			{
				if (theEnemy->kind == 0xe)
					aResult = aResult & LaserShot_HitBeams(theEnemy);
				theEnemy->lastBeam = aBeam;
			}
		}
		else if (aBeam != nullptr)
		{
			bool aRiding = false;
			List_Each(&aBeam->riders, [&](Rider* aRider) {
				if (aRider->enemy == theEnemy)
					aRiding = true;
			});
			if (!aRiding)
			{
				Rider_Append(&aBeam->riders);
				Rider_Setup(aBeam->riders.current, aBeam, theEnemy->x, theEnemy->y, theEnemy);
				if ((float)(theEnemy->lastZap + 1000) < theEnemy->age && gLevels.current->shooters.current->state == 5)
				{
					// (the original works out a pan from x for a play call that takes none)
					Sound_Play(Sound_Find("audio\\electricity.wav"));
					theEnemy->lastZap = (int)theEnemy->age;
				}
			}
			Enemy_Shield(theEnemy);
			theEnemy->lastBeam = aBeam;
		}
		int aKind = theEnemy->kind;
		bool aDies = false;
		if (aKind != 0xd)
		{
			int aCell = gLevels.current->columns[(int)theEnemy->x / 16][(int)theEnemy->y / 16].kind;
			aDies = aCell == 4 || (-1 < aKind && aKind < 7 && aCell == 2);
		}
		if (!aDies)
			aDies = 6 < aKind && aKind < 0xd && Enemy_NearCapture(theEnemy) != 0;
		if (aDies)
		{
			if (theEnemy->shield != nullptr)
				Obj_Delete(theEnemy->shield);
			theEnemy->shield = nullptr;
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
	theEnemy->age = theEnemy->age + 1.0f;
	if (theEnemy->drawKind != 0)
	{
		theEnemy->drawKind = 4;
		int anAlpha = gLevels.current->columns[(int)theEnemy->x / 16][(int)theEnemy->y / 16].alpha;
		theEnemy->alpha = anAlpha;
		if (0xfe < anAlpha && Enemy_NearCapture(theEnemy) == 0)
		{
			theEnemy->state = 3;
			theEnemy->drawKind = 1;
		}
	}
	return 1;
}

int Enemy_Update(Enemy* theEnemy)
{
	int aResult = 0;
	switch (theEnemy->state)
	{
	case 0:
		aResult = Enemy_UpdateAppearing(theEnemy);
		break;
	case 1:
		theEnemy->anim = theEnemy->dieAnim;
		theEnemy->vx = (float)((double)theEnemy->vx * 0.96);
		theEnemy->vy = (float)((double)theEnemy->vy * 0.96);
		[[fallthrough]];
	case 2:
		aResult = Obj_Update(theEnemy);
		break;
	case 3:
		aResult = Enemy_Step(theEnemy);
		break;
	}
	Enemy_UpdateShield(theEnemy);
	return aResult;
}

} // namespace wb
