// The board's panel (0x418050..0x418810), the power-ups (0x418810..0x419c00) and the weapons some of them give
// (0x419c10..0x41b340). A power-up falls from where an enemy died; when the ship touches it, it shows its name and
// changes the ship or the game, or gives the ship a weapon to orbit it. A click launches the first orbiting weapon.

#include "WBLevel.h"
#include "R64Sys.h"

#include <cstdio>
#include <cstring>
#include <new>

namespace wb
{

namespace
{

int gPanelPercent = -1;            // 0x460a38  what the panel last drew
int gPanelLives = -1;              // 0x460a34
int gPanelScore = -1;              // 0x460a30
int gPowerupKinds[0x96];           // 0x6e5c08  a power-up's kind by a random index
int gRebuildKinds = 1;             // 0x460a48  (never cleared: each power-up builds the table again)

// 0x418810: ten entries for each kind from 0 to 12 (the even ones of 8 and 9 are 0), 0 from index 130, and the extra life
// (13) in the last six places, 140..145; no kind past 13 is in it
void Powerup_BuildKinds()
{
	for (uint32_t i = 0; (int)i < 0x96; i++)
	{
		uint32_t aKind = i / 10;
		gPowerupKinds[i] = aKind;
		if (0x81 < (int)i)
			gPowerupKinds[i] = 0;
		if ((aKind == 9 || aKind == 8) && (i & 1) == 0)
			gPowerupKinds[i] = 0;
	}
	gPowerupKinds[140] = 0xd;
	gPowerupKinds[141] = 0xd;
	gPowerupKinds[142] = 0xd;
	gPowerupKinds[143] = 0xd;
	gPowerupKinds[144] = 0xd;
	gPowerupKinds[145] = 0xd;
}

// The effects. 0x4188a0: a faster beam (by the beams' starting speed)
void Powerup_FastShot()
{
	Shooter* aShip = gLevels.mCurrent->mShooters.mCurrent;
	aShip->mBeamSpeed = (float)(((double)gBeamSpeed - (double)gDifficulty * (double)0.05f) + (double)aShip->mBeamSpeed);
}

// 0x4188d0: half the speed, but not under half the starting speed
void Powerup_SlowShot()
{
	Shooter* aShip = gLevels.mCurrent->mShooters.mCurrent;
	float aMin = (float)(((double)gBeamSpeed - (double)gDifficulty * (double)0.05f) * 0.5);
	float aHalf = (float)((double)aShip->mBeamSpeed * 0.5);
	if (aHalf < aMin)
	{
		aShip->mBeamSpeed = aMin;
		return;
	}
	aShip->mBeamSpeed = aHalf;
}

// 0x418920
void Powerup_ExtraLife()
{
	gLives = gLives + 1;
}

// 0x418930: a cell scores one more
void Powerup_Multiplier()
{
	Shooter* aShip = gLevels.mCurrent->mShooters.mCurrent;
	aShip->mCellPoints = aShip->mCellPoints + 1;
}

// 0x418950
void Powerup_SlowMotion()
{
	Shooter* aShip = gLevels.mCurrent->mShooters.mCurrent;
	aShip->mFlags = aShip->mFlags | 1;
	gLevels.mCurrent->mShooters.mCurrent->mSlowMotionTime = 5000;
}

// 0x418980
void Powerup_HyperMotion()
{
	Shooter* aShip = gLevels.mCurrent->mShooters.mCurrent;
	aShip->mFlags = aShip->mFlags | 2;
	gLevels.mCurrent->mShooters.mCurrent->mHyperMotionTime = 5000;
}

// 0x4189d0
void Powerup_FlipFlopper()
{
	Shooter* aShip = gLevels.mCurrent->mShooters.mCurrent;
	aShip->mFlags = aShip->mFlags | 8;
}

// 0x4189f0: explode all floaters: the countdowns of the enemies of kinds 0..6 are set to 300, 600 and so on
void Powerup_ExplodeFloaters()
{
	int aDelay = 300;
	for (Enemy* anEnemy = gLevels.mCurrent->mEnemies.mHead; anEnemy != nullptr; anEnemy = Node_Next(anEnemy))
	{
		if (-1 < anEnemy->mKind && anEnemy->mKind < 7)
		{
			anEnemy->mCountdown = aDelay;
			aDelay = aDelay + 300;
		}
	}
}

// 0x418a30: explode all walkers (kinds 7..0xc)
void Powerup_ExplodeWalkers()
{
	int aDelay = 300;
	for (Enemy* anEnemy = gLevels.mCurrent->mEnemies.mHead; anEnemy != nullptr; anEnemy = Node_Next(anEnemy))
	{
		if (6 < anEnemy->mKind && anEnemy->mKind < 0xd)
		{
			anEnemy->mCountdown = aDelay;
			aDelay = aDelay + 300;
		}
	}
}

// 0x418a70
void Powerup_Invincibubble()
{
	Shooter* aShip = gLevels.mCurrent->mShooters.mCurrent;
	aShip->mFlags = aShip->mFlags | 0x10;
}

// 0x418a90: a new weapon at the tail, made current
// 0x40ad50 (a weapon's constructor, which only calls Obj_Init, is inlined here)
void Weapon_Append(List<Weapon>* theList)
{
	Weapon* aWeapon = (Weapon*)::operator new(sizeof(Weapon));
	Obj_Init(aWeapon); // 0x40ad50
	List_Append(theList, aWeapon);
}

void Weapon_Setup(Weapon* theWeapon, float x, float y, float theRadius, int theKind);

// 0x418b80, 0x418bd0, 0x418c20, 0x418c70, 0x418cc0, 0x418d10: a weapon where the power-up is
void Powerup_GiveWeapon(Powerup* thePowerup, float theRadius, int theKind)
{
	Weapon_Append(&gLevels.mCurrent->mWeapons);
	Weapon_Setup(gLevels.mCurrent->mWeapons.mCurrent, thePowerup->mX, thePowerup->mY, theRadius, theKind);
}

// 0x4198a0: shows its name (removing any long or main_font text still up) and takes effect
int Powerup_Collect(Powerup* thePowerup)
{
	gProfiles[gProfile].mPowerupsCaught = gProfiles[gProfile].mPowerupsCaught + 1;
	List<Text>* aTexts = &gLevels.mCurrent->mTexts;
	List_Each(aTexts, [&](Text* theAText) {
		if (0xd < std::strlen(theAText->mText) || theAText->mFont == Font_Find("main_font"))
			List_RemoveCurrent(aTexts);
	});
	Text_Append(aTexts);
	Text_Make(aTexts->mCurrent, thePowerup->mText, Font_Find("main_font"), 400, 0xe6, 0, 200, 2000, 200, 0, 0);
	const char* aSound;
	switch (thePowerup->mKind)
	{
	case 1:
		Powerup_FlipFlopper();
		aSound = "audio\\powerdn.wav";
		goto play;
	case 2:
		Powerup_SlowMotion();
		break;
	case 3:
		Powerup_GiveWeapon(thePowerup, 50.0f, 5);
		break;
	case 4:
		Powerup_GiveWeapon(thePowerup, 50.0f, 4);
		break;
	case 5:
		Powerup_SlowShot();
		aSound = "audio\\powerdn.wav";
		goto play;
	case 6:
		Powerup_Invincibubble();
		break;
	case 7:
		Powerup_GiveWeapon(thePowerup, 50.0f, 6);
		break;
	case 8:
		Powerup_ExplodeWalkers();
		break;
	case 9:
		Powerup_ExplodeFloaters();
		break;
	case 10:
		Powerup_Multiplier();
		break;
	case 0xb:
		Powerup_HyperMotion();
		aSound = "audio\\powerdn.wav";
		goto play;
	case 0xc:
		Powerup_GiveWeapon(thePowerup, 40.0f, 1);
		break;
	case 0xd:
		Powerup_ExtraLife();
		break;
	default:
		Powerup_FastShot();
		break;
	}
	aSound = "audio\\powerup.wav";
play:
	Sound_Play(Sound_Find(aSound));
	return 1;
}

// Powerup_Init's emitters, about its middle
void Powerup_AddSparkles(Powerup* thePowerup, const char* theAnim)
{
	Emitter_Append(&thePowerup->mEmitters);
	int y = RandRange(-8, 8) + thePowerup->mImage->mH / 2;
	int x = RandRange(-8, 8) + thePowerup->mImage->mW / 2;
	Emitter_Setup(static_cast<Emitter*>(thePowerup->mEmitters.mCurrent), thePowerup, x, y, 30.0f, 30.0f, 0x5a, 0,
		theAnim, 1, 255.0f, 255.0f, 1200.0f);
}

// 0x419ae0: falls; collected when it touches the ship, gone below the screen
int Powerup_Fall(Powerup* thePowerup)
{
	thePowerup->mVy = thePowerup->mGravity + thePowerup->mVy;
	Obj_Update(thePowerup);
	if (thePowerup->mAttached != nullptr)
	{
		Obj_Update(thePowerup->mAttached);
		Obj* anAttached = thePowerup->mAttached;
		double aShift = (double)anAttached->mImage->mW * (double)anAttached->mVx;
		anAttached->mX = (float)((aShift + aShift) / 3.0 + (double)thePowerup->mX);
		thePowerup->mAttached->mY = thePowerup->mY;
	}
	Shooter* aShip = gLevels.mCurrent->mShooters.mCurrent;
	if (aShip != nullptr && aShip->mState == 3 &&
		CircleOverlap(thePowerup->mX, thePowerup->mY, (float)(thePowerup->mImage->mH / 2), aShip->mX, aShip->mY,
			(float)(aShip->mImage->mH / 4)) != 0.0)
	{
		Powerup_Collect(thePowerup);
		return 0;
	}
	float y = thePowerup->mY;
	int h = thePowerup->mImage->mH;
	Obj_UpdateEmitters(thePowerup);
	return y <= (float)(h + 600);
}

// The weapons. 0x419e40: a missile's animation by the way it points
void Missile_Face(Weapon* theWeapon)
{
	if (theWeapon->mKind == 1)
	{
		if (theWeapon->mAcross == 0)
		{
			if (0.0f < theWeapon->mVy)
			{
				theWeapon->mAnim = Animation_Find("missile_down", &theWeapon->mFrameCount, &theWeapon->mFrameTime);
				return;
			}
			theWeapon->mAnim = Animation_Find("missile_up", &theWeapon->mFrameCount, &theWeapon->mFrameTime);
			return;
		}
		if (0.0f < theWeapon->mVx)
		{
			theWeapon->mAnim = Animation_Find("missile_left", &theWeapon->mFrameCount, &theWeapon->mFrameTime);
			return;
		}
		theWeapon->mAnim = Animation_Find("missile_right", &theWeapon->mFrameCount, &theWeapon->mFrameTime);
	}
}

int Sign(float x)
{
	if (x == 0.0f)
		return 0;
	if (x <= 0.0f)
		return -1;
	return 1;
}

// 0x419ee0 (the magnet): off cells that are not open, a third of its size ahead; it slows only over open cells
int Weapon_Bounce(Weapon* theWeapon)
{
	int aCol = (int)theWeapon->mX / 16;
	int aRow = (int)theWeapon->mY / 16;
	Level* aLevel = gLevels.mCurrent;
	if (aLevel->mColumns[aCol][aRow].mKind == 0)
	{
		Image* anImage = theWeapon->mImage;
		int x = (int)((double)(anImage->mW * Sign(theWeapon->mVx) / 3) + (double)theWeapon->mX);
		if (aLevel->mColumns[x / 16][aRow].mKind != 0)
		{
			theWeapon->mVx = -theWeapon->mVx;
			aLevel = gLevels.mCurrent;
		}
		int y = (int)((double)(anImage->mH * Sign(theWeapon->mVy) / 3) + (double)theWeapon->mY);
		if (aLevel->mColumns[aCol][y / 16].mKind != 0)
			theWeapon->mVy = -theWeapon->mVy;
		theWeapon->mDrag = 0.996f;
		// (its own cell was open above, so this never holds)
		if (gLevels.mCurrent->mColumns[aCol][aRow].mKind != 0)
		{
			theWeapon->mVx = -theWeapon->mVx;
			theWeapon->mVy = -theWeapon->mVy;
			return 0;
		}
	}
	else
		theWeapon->mDrag = 1.0f;
	return 0;
}

// 0x41a060 (the anchor): settles in the middle of its cell and makes that cell and the four beside it edges
int Anchor_Drop(Weapon* theWeapon)
{
	int aCol = (int)theWeapon->mX / 16;
	int aRow = (int)theWeapon->mY / 16;
	theWeapon->mX = (float)(aCol * 0x10 + 8);
	theWeapon->mY = (float)(aRow * 0x10 + 8);
	for (int aDc = -1; aDc < 2; aDc++)
	{
		for (int aDr = -1; aDr < 2; aDr++)
		{
			if ((aDr == 0 || aDc == 0) && 0 < aDc + aCol && aDc + aCol < 0x31 && 0 < aDr + aRow && aDr + aRow < 0x24)
			{
				Cell* aCell = &gLevels.mCurrent->mColumns[aCol + aDc][aRow + aDr];
				if (aCell->mKind == 0)
					gLevels.mCurrent->mTotalCells = gLevels.mCurrent->mTotalCells - 1;
				aCell->mKind = 3;
				aCell->mImage = Animation_Frame(gLevels.mCurrent->mCornerTiles, 0);
				Cell_Redraw(aCell);
			}
		}
	}
	return 0;
}

// 0x41a1a0 (the magnet): fades out over its last 1000 updates, pulling the moving enemies it touches a 300th of
// the way to it each update
int Magnet_Pull(Weapon* theWeapon)
{
	if (9000.0f < theWeapon->mAge)
	{
		if (theWeapon->mAge >= 10000.0f)
			return 0;
		theWeapon->mAlpha = (int)((10000.0 - (double)theWeapon->mAge) * 255.0 / 1000.0);
	}
	List_Each(&gLevels.mCurrent->mEnemies, [&](Enemy* theAnEnemy) {
		if (theAnEnemy->mState == 3 &&
			CircleOverlap(theWeapon->mX, theWeapon->mY, (float)theWeapon->mImage->mW, theAnEnemy->mX, theAnEnemy->mY,
				(float)theAnEnemy->mImage->mW) != 0.0)
		{
			if (theAnEnemy->mVx != 0.0f)
				theAnEnemy->mX = (float)(((double)theWeapon->mX - (double)theAnEnemy->mX) / 300.0 + (double)theAnEnemy->mX);
			if (theAnEnemy->mVy != 0.0f)
				theAnEnemy->mY = (float)(((double)theWeapon->mY - (double)theAnEnemy->mY) / 300.0 + (double)theAnEnemy->mY);
		}
	});
	return 1;
}

// 0x41a2f0 (a mine or a missile): blows up the enemies it touches; 0 when it did
int Weapon_Blast(Weapon* theWeapon)
{
	int aResult = 1;
	List_Each(&gLevels.mCurrent->mEnemies, [&](Enemy* theAnEnemy) {
		if (theAnEnemy->mState == 3 &&
			CircleOverlap(theWeapon->mX, theWeapon->mY, (float)(theWeapon->mImage->mW / 3), theAnEnemy->mX, theAnEnemy->mY,
				(float)(theAnEnemy->mImage->mW / 3)) != 0.0)
		{
			theAnEnemy->mDieAnim = Animation_Find("second_explo", &theAnEnemy->mDieFrameCount, &theAnEnemy->mDieFrameTime);
			Enemy_Kill(theAnEnemy, 500);
			aResult = 0;
		}
	});
	return aResult;
}

// 0x41a650 (a time bomb): sparks fly off the top of the fuse, which burns down, hissing every 230 updates; at the
// end it blows up the enemies within 100 pixels and then itself
int TimeBomb_Update(Weapon* theWeapon)
{
	double anEnd = (double)theWeapon->mLifetime - 1.0;
	if ((double)theWeapon->mAge < anEnd)
	{
		if (RandRange(1, 10) == 1)
		{
			double aBase = (double)theWeapon->mY - (double)(theWeapon->mImage->mH / 2);
			int aFuse = (int)(9.0 - (double)theWeapon->mAge * 8.0 / (double)theWeapon->mLifetime);
			int aTop = (int)(aBase - (double)aFuse);
			ParticleSystem_New();
			int aParticle = theWeapon->mParticle;
			int aLifetime = RandRange(400, 500);
			float aVy = (float)((double)RandRange(-0x32, 0x32) / 1200.0);
			float aVx = (float)((double)RandRange(-0x32, 0x32) / 1200.0);
			ParticleSystem_Add(gLevels.mCurrent->mSystems.mCurrent, (float)((double)theWeapon->mX - 0.0), (float)aTop, aVx,
				aVy, aParticle, aParticle, 0, 0xff, 0xff, 0xff, 0xff, 0xff, 0, 400, 10, aLifetime, 0.999f, 0.0003f, 5);
		}
		if ((int)theWeapon->mAge % 0xe6 == 0)
		{
			Sound_Play(theWeapon->mFuseSound);
			return 1;
		}
	}
	else if ((double)theWeapon->mAge == anEnd)
	{
		List_Each(&gLevels.mCurrent->mEnemies, [&](Enemy* theAnEnemy) {
			if (theAnEnemy->mState == 3 &&
				CircleOverlap(theWeapon->mX, theWeapon->mY, 100.0f, theAnEnemy->mX, theAnEnemy->mY,
					(float)(theAnEnemy->mImage->mW / 3)) != 0.0)
			{
				theAnEnemy->mDieAnim = Animation_Find("second_explo", &theAnEnemy->mDieFrameCount, &theAnEnemy->mDieFrameTime);
				Enemy_Kill(theAnEnemy, 500);
			}
		});
		Obj_Explode(theWeapon, 0.0f, 0.0f);
		theWeapon->mLifetime = 0.0f;
		theWeapon->mAge = 0.0f;
	}
	return 1;
}

// 0x41a910: a click sends it straight out at its speed, the way it is from the ship (unless an enemy is too close)
void Weapon_Launch(Weapon* theWeapon)
{
	if (theWeapon->mTooClose == 0)
	{
		theWeapon->mVx = (float)Sign(theWeapon->mVx) * theWeapon->mLaunchSpeed;
		theWeapon->mState = 4;
		theWeapon->mVy = (float)Sign(theWeapon->mVy) * theWeapon->mLaunchSpeed;
		theWeapon->mAge = 0.0f;
		int aKind = theWeapon->mKind;
		// (each sound is panned by x: the port's Sound_Play takes no pan)
		if (aKind != 1)
		{
			const char* aSound;
			if (aKind == 5)
				aSound = "audio\\mine.wav";
			else
			{
				if (aKind == 4)
				{
					Sound_Play(Sound_Find("audio\\mine.wav"));
					theWeapon->mLifetime = 2000.0f;
					return;
				}
				if (aKind != 6)
					return;
				aSound = "audio\\anchor.wav";
			}
			Sound_Play(Sound_Find(aSound));
			return;
		}
		Sound_Play(Sound_Find("audio\\missile.wav"));
		List<Obj>* anEmitters = &theWeapon->mEmitters;
		Emitter_Append(anEmitters);
		Emitter* anEmitter = static_cast<Emitter*>(anEmitters->mCurrent);
		if (0.0f < theWeapon->mVx)
		{
			Emitter_Setup(anEmitter, theWeapon, -6, 0x15, 0.0f, 11.0f, 0x14, 0, "smoke", 1, 100.0f, 10.0f, 400.0f);
			return;
		}
		if (theWeapon->mVx < 0.0f)
		{
			Emitter_Setup(anEmitter, theWeapon, 0x2a, 0x15, 0.0f, 11.0f, 0x14, 0, "smoke", 1, 100.0f, 10.0f, 400.0f);
			return;
		}
		if (0.0f < theWeapon->mVy)
		{
			Emitter_Setup(anEmitter, theWeapon, 0x15, -6, 11.0f, 0.0f, 0x14, 0, "smoke", 1, 100.0f, 10.0f, 400.0f);
			return;
		}
		if (theWeapon->mVy < 0.0f)
		{
			Emitter_Setup(anEmitter, theWeapon, 0x15, 0x2a, 11.0f, 0.0f, 0x14, 0, "smoke", 1, 100.0f, 10.0f, 400.0f);
			return;
		}
	}
}

// 0x41ac20 (launched): slows by its drag and does its work; 0 when it is done or off the screen
int Weapon_Move(Weapon* theWeapon)
{
	int aResult = 1;
	theWeapon->mVx = theWeapon->mDrag * theWeapon->mVx;
	theWeapon->mVy = theWeapon->mVy * theWeapon->mDrag;
	switch (theWeapon->mKind)
	{
	case 1:
		aResult = Weapon_Blast(theWeapon);
		break;
	case 4:
		aResult = TimeBomb_Update(theWeapon);
		break;
	case 5:
		Weapon_Bounce(theWeapon);
		aResult = Magnet_Pull(theWeapon);
		break;
	case 6:
		aResult = Anchor_Drop(theWeapon);
		break;
	}
	Obj_UpdateEmitters(theWeapon);
	int w = theWeapon->mImage->mW;
	if ((float)(SCREEN_CLIP[0] - w) <= theWeapon->mX && theWeapon->mX <= (float)(w + SCREEN_CLIP[2]))
	{
		int h = theWeapon->mImage->mH;
		if ((float)(SCREEN_CLIP[1] - h) <= theWeapon->mY && theWeapon->mY <= (float)(h + SCREEN_CLIP[3]))
			return aResult;
	}
	return 0;
}

// 0x41ad30: a new weapon orbits opposite the first one already there; any second one goes. It ends current.
void Weapon_TakeSlot(Weapon* theWeapon)
{
	int aCount = 0;
	if (gLevels.mCurrent->mShooters.mCurrent->mBeamAcross == 0)
		theWeapon->mAngle = 0x5a;
	else
		theWeapon->mAngle = 0;
	List<Weapon>* aWeapons = &gLevels.mCurrent->mWeapons;
	if (aWeapons->mCurrent == nullptr)
	{
		aWeapons->mCurrent = theWeapon;
		return;
	}
	List_Each(aWeapons, [&](Weapon* theAWeapon) {
		if (theAWeapon->mState == 3 && theAWeapon != theWeapon)
		{
			if (aCount == 0)
				theWeapon->mAngle = (theAWeapon->mAngle + 0xb4) % 0x168;
			else
				List_RemoveCurrent(aWeapons);
			aCount = aCount + 1;
		}
	});
	aWeapons->mCurrent = theWeapon;
}

// 0x41ae10
void Weapon_Setup(Weapon* theWeapon, float x, float y, float theRadius, int theKind)
{
	theWeapon->mX = x;
	theWeapon->mTooClose = 0;
	theWeapon->mY = y;
	theWeapon->mKind = theKind;
	theWeapon->mState = 3;
	theWeapon->mRadius = theRadius;
	theWeapon->mDepth = 3;
	theWeapon->mShadow = 1;
	theWeapon->mAlpha = 0xff;
	theWeapon->mAcross = gLevels.mCurrent->mShooters.mCurrent->mBeamAcross;
	switch (theKind)
	{
	case 1:
		Missile_Face(theWeapon);
		theWeapon->mLaunchSpeed = 0.15f;
		theWeapon->mDrag = 1.002f;
		break;
	case 4:
	{
		Shooter* aShip = gLevels.mCurrent->mShooters.mCurrent;
		theWeapon->mAnim = Animation_Find("shooter_timebomb", &theWeapon->mFrameCount, &theWeapon->mFrameTime);
		theWeapon->mLaunchSpeed = 0.15f;
		theWeapon->mDrag = 0.996f;
		theWeapon->mDieAnim = aShip->mDieAnim;
		theWeapon->mDieFrameCount = aShip->mDieFrameCount;
		theWeapon->mDieFrameTime = aShip->mDieFrameTime;
		theWeapon->mParticle = Bank_IndexOf(&gArtBank, "particle_0");
		theWeapon->mExploSound = Sound_Find("audio\\explosion1.wav");
		theWeapon->mFuseSound = Sound_Find("audio\\fuse.wav");
		break;
	}
	case 5:
		theWeapon->mAnim = Animation_Find("shooter_magnet", &theWeapon->mFrameCount, &theWeapon->mFrameTime);
		theWeapon->mLaunchSpeed = 0.35f;
		theWeapon->mDrag = 0.996f;
		break;
	case 6:
		theWeapon->mAnim = Animation_Find("anchor", &theWeapon->mFrameCount, &theWeapon->mFrameTime);
		theWeapon->mLaunchSpeed = 0.0f;
		theWeapon->mDrag = 1.001f;
		theWeapon->mParticle = Bank_IndexOf(&gArtBank, "particle_0");
		theWeapon->mShadow = 0;
		break;
	}
	theWeapon->mDirty = 1;
	theWeapon->mDrawKind = 4;
	theWeapon->mMidHandle = 1;
	Weapon_TakeSlot(theWeapon);
}

// 0x41b080 (orbiting): at its angle and radius about the ship, a quarter turn more each time the ship turns;
// faint while an enemy touches it or when it is not the first, which a click launches
void Weapon_Orbit(Weapon* theWeapon)
{
	theWeapon->mTooClose = 0;
	theWeapon->mAlpha = 0xff;
	List_Each(&gLevels.mCurrent->mEnemies, [&](Enemy* theAnEnemy) {
		if (CircleOverlap(theWeapon->mX, theWeapon->mY, (float)(theWeapon->mImage->mW / 2), theAnEnemy->mX, theAnEnemy->mY,
				(float)(theAnEnemy->mImage->mW / 2)) != 0.0 &&
			theWeapon->mKind != 4)
			theWeapon->mTooClose = 1;
	});
	Shooter* aShip = gLevels.mCurrent->mShooters.mCurrent;
	if (theWeapon->mTooClose == 1 || gFirstWeapon == 0)
		theWeapon->mAlpha = 0x4b;
	else
		theWeapon->mAlpha = aShip->mAlpha;
	if (theWeapon->mAcross != aShip->mBeamAcross)
		theWeapon->mAngle = (theWeapon->mAngle + 0x5a) % 0x168;
	theWeapon->mVx = CosDeg(theWeapon->mAngle) * theWeapon->mRadius;
	float aVy = SinDeg(theWeapon->mAngle) * theWeapon->mRadius;
	theWeapon->mVy = aVy;
	theWeapon->mY = aVy + gLevels.mCurrent->mShooters.mCurrent->mY;
	theWeapon->mX = gLevels.mCurrent->mShooters.mCurrent->mX + theWeapon->mVx;
	Missile_Face(theWeapon);
	if (gLevels.mCurrent->mClicked != 0 && gFirstWeapon != 0)
	{
		Weapon_Launch(theWeapon);
		gLevels.mCurrent->mClicked = 0;
	}
	gFirstWeapon = 0;
	theWeapon->mAcross = gLevels.mCurrent->mShooters.mCurrent->mBeamAcross;
}

} // namespace

// 0x418170: a new item at the tail, made current
void Panel_AppendItem(List<Obj>* theList)
{
	Obj* anItem = (Obj*)::operator new(sizeof(Obj));
	Obj_Init(anItem);
	List_Append(theList, anItem);
}

void Panel_FromXml(Panel* thePanel, const char* theXmlCursor, const XmlTag* theTag)
{
	thePanel->mPercentX = Xml_GetInt(theTag, "percent_x");
	thePanel->mPercentY = Xml_GetInt(theTag, "percent_y");
	thePanel->mLivesX = Xml_GetInt(theTag, "lives_x");
	thePanel->mLivesY = Xml_GetInt(theTag, "lives_y");
	thePanel->mScoreX = Xml_GetInt(theTag, "score_x");
	thePanel->mScoreY = Xml_GetInt(theTag, "score_y");
	thePanel->mLevelX = Xml_GetInt(theTag, "level_x");
	thePanel->mLevelY = Xml_GetInt(theTag, "level_y");
	thePanel->mPercentFont = Font_Find(Xml_GetString(theTag, "percent_font"));
	thePanel->mLivesFont = Font_Find(Xml_GetString(theTag, "lives_font"));
	thePanel->mScoreFont = Font_Find(Xml_GetString(theTag, "score_font"));
	thePanel->mLevelFont = Font_Find(Xml_GetString(theTag, "level_font"));
	Obj_FromXml(thePanel, theXmlCursor, theTag);
	thePanel->mDirty = 0;
}

// 0x4182a0: the panel a level without a panel tag gets: the panel image along the bottom, the logo at its right and,
// in the level's menus, a quit button
void Panel_Setup(Panel* thePanel)
{
	thePanel->mPercentX = 0x50;
	thePanel->mPercentY = 0x224;
	thePanel->mLivesX = 0x1de;
	thePanel->mLivesY = 0x224;
	thePanel->mScoreX = 0xbd;
	thePanel->mScoreY = 0x224;
	thePanel->mLevelX = 0x163;
	thePanel->mLevelY = 0x224;
	thePanel->mPercentFont = Font_Find("percent_font");
	thePanel->mLivesFont = Font_Find("lives_font");
	thePanel->mScoreFont = Font_Find("score_font");
	thePanel->mLevelFont = Font_Find("level_font");
	thePanel->mX = 0.0f;
	thePanel->mY = 520.0f;
	thePanel->mDirty = 0;
	thePanel->mAnim = Animation_Find("panel", &thePanel->mFrameCount, &thePanel->mFrameTime);
	List<Obj>* anItems = &thePanel->mItems;
	Panel_AppendItem(anItems);
	anItems->mCurrent->mAnim = Animation_Find("water_panel", &anItems->mCurrent->mFrameCount, &anItems->mCurrent->mFrameTime);
	anItems->mCurrent->mX = 656.0f;
	anItems->mCurrent->mY = 531.0f;
	anItems->mCurrent->mMidHandle = 0;
	anItems->mCurrent->mDrawKind = 1;
	thePanel->mDrawKind = 0;
	Menu_Insert(&gLevels.mCurrent->mMenus);
	MenuItem_Insert(&gLevels.mCurrent->mMenus.mCurrent->mItems);
	MenuItem* aQuit = gLevels.mCurrent->mMenus.mCurrent->mItems.mCurrent;
	aQuit->mX = 549.0f;
	std::strcpy(aQuit->mName, "aaa");
	aQuit->mY = 567.0f;
	aQuit->mFont = Font_Find("main_font");
	aQuit->mAnim = Animation_Find("quit", &aQuit->mFrameCount, &aQuit->mFrameTime);
	aQuit->mMidHandle = 1;
	aQuit->mDrawKind = 1;
	aQuit->mDirty = 1;
	aQuit->mKind = 0x21;
	aQuit->mFirstParticle = Bank_IndexOf(&gArtBank, "bubble01");
	aQuit->mLastParticle = Bank_IndexOf(&gArtBank, "bubble02");
}

// (it returns 1)
void Panel_Update(Panel* thePanel)
{
	List_Each(&thePanel->mItems, [](Obj* theAnItem) { Obj_Update(theAnItem); });
}

// 0x4184e0: into the back buffer when theAll is set or a number changed (the lives shown stop at 99, and so do the lives);
// its items onto the screen
void Panel_Draw(Panel* thePanel, int theAll)
{
	char aText[32];
	int aRect[4];
	int aPercent;
	Level* aLevel = gLevels.mCurrent;
	if (aLevel->mTotalCells == 0)
		aPercent = 0;
	else
	{
		aPercent = (aLevel->mFilledCells * 100) / aLevel->mTotalCells;
		if (100 < aPercent)
			aPercent = 100;
	}
	Obj_Update(thePanel);
	if (theAll != 0 || aPercent != gPanelPercent || gLives != gPanelLives || gScore != gPanelScore)
	{
		Image* anImage = thePanel->mImage;
		Image_BlitRect(anImage, gBackBuffer.mBits, gBackBuffer.mPitch, &gBackBuffer.mFmt, (int)thePanel->mX,
			(int)thePanel->mY, 0, 0, anImage->mW, anImage->mH);
		std::sprintf(aText, "%03d", aPercent);
		Font_SetTarget(thePanel->mPercentFont, gBackBuffer.mBits, gBackBuffer.mPitch, &gBackBuffer.mFmt, gBackBuffer.mW,
			gBackBuffer.mH);
		Font_Printf(thePanel->mPercentFont, aRect, thePanel->mPercentX, thePanel->mPercentY, aText);
		std::sprintf(aText, "%03d", gLevels.mCurrent->mNumber);
		Font_SetTarget(thePanel->mLevelFont, gBackBuffer.mBits, gBackBuffer.mPitch, &gBackBuffer.mFmt, gBackBuffer.mW,
			gBackBuffer.mH);
		Font_Printf(thePanel->mLevelFont, aRect, thePanel->mLevelX, thePanel->mLevelY, aText);
		int aScore = gScore;
		if (9999999 < gScore)
			aScore = 9999999;
		std::sprintf(aText, "%07d", aScore);
		Font_SetTarget(thePanel->mScoreFont, gBackBuffer.mBits, gBackBuffer.mPitch, &gBackBuffer.mFmt, gBackBuffer.mW,
			gBackBuffer.mH);
		Font_Printf(thePanel->mScoreFont, aRect, thePanel->mScoreX, thePanel->mScoreY, aText);
		if (0x62 < gLives)
			gLives = 99;
		std::sprintf(aText, "%02d", gLives);
		Font_SetTarget(thePanel->mLivesFont, gBackBuffer.mBits, gBackBuffer.mPitch, &gBackBuffer.mFmt, gBackBuffer.mW,
			gBackBuffer.mH);
		Font_Printf(thePanel->mLivesFont, aRect, thePanel->mLivesX, thePanel->mLivesY, aText);
		Dirty_AddClippedShown(&gDirty, 0, (int)thePanel->mY, thePanel->mImage->mW, thePanel->mImage->mH, SCREEN_CLIP);
		gPanelLives = gLives;
		gPanelScore = gScore;
		gPanelPercent = aPercent;
	}
	List_Each(&thePanel->mItems, [](Obj* theAnItem) {
		if (theAnItem->mImage != nullptr)
		{
			Obj_Draw(theAnItem);
			Dirty_AddClippedImage(&gDirty, (int)theAnItem->mX, (int)theAnItem->mY, theAnItem->mImage, SCREEN_CLIP);
		}
	});
}

void Powerup_Draw(Powerup* thePowerup)
{
	Obj_Draw(thePowerup);
	if (thePowerup->mAttached != nullptr && thePowerup->mState == 4)
		Obj_Draw(thePowerup->mAttached);
}

void List_DeleteNode(Powerup* thePowerup)
{
	// 0x418b20
	if (thePowerup->mAttached != nullptr)
	{
		Obj_Destroy(thePowerup->mAttached);
		::operator delete(thePowerup->mAttached);
	}
	Obj_Destroy(thePowerup);
	::operator delete(thePowerup);
}

// 0x418d60: a kind by the level (the later the level, the more kinds), and its text, animation and three emitters of sparkles. The keys 0..9 and A..D,
// held while it is made, choose kinds 0..13 (the developers' test). The table gives kinds 0..13 (0 a faster beam; the
// extra life is in its last six places), so nothing makes the original's water mine or freeze (kinds 16 and 17); the
// port leaves them out.
Powerup* Powerup_Init(Powerup* thePowerup)
{
	Obj_Init(thePowerup);
	if (gRebuildKinds != 0)
		Powerup_BuildKinds();
	int aRange = gLevels.mCurrent->mNumber / 4;
	if (0xe < aRange)
		aRange = 0xe;
	aRange = aRange * 10 + 9;
	thePowerup->mKind = gPowerupKinds[RandRange(0, aRange)];
	std::sprintf(thePowerup->mText, "^%d", RandRange(1, 8));
	if (Key_Down('0') != 0)
		thePowerup->mKind = 0;
	if (Key_Down('1') != 0)
		thePowerup->mKind = 1;
	if (Key_Down('2') != 0)
		thePowerup->mKind = 2;
	if (Key_Down('3') != 0)
		thePowerup->mKind = 3;
	if (Key_Down('4') != 0)
		thePowerup->mKind = 4;
	if (Key_Down('5') != 0)
		thePowerup->mKind = 5;
	if (Key_Down('6') != 0)
		thePowerup->mKind = 6;
	if (Key_Down('7') != 0)
		thePowerup->mKind = 7;
	if (Key_Down('8') != 0)
		thePowerup->mKind = 8;
	if (Key_Down('9') != 0)
		thePowerup->mKind = 9;
	if (Key_Down('A') != 0)
		thePowerup->mKind = 10;
	if (Key_Down('B') != 0)
		thePowerup->mKind = 11;
	if (Key_Down('C') != 0)
		thePowerup->mKind = 12;
	if (Key_Down('D') != 0)
		thePowerup->mKind = 13;
	// The texts and names are the exe's (each strcat'd from its .rdata)
	uint32_t aText;
	const char* aName;
	switch (thePowerup->mKind)
	{
	case 1:
		aText = GTA(0x45992c);
		aName = "powerup_flip_flopper";
		break;
	case 2:
		aText = GTA(0x459a68);
		aName = "powerup_slowmo";
		break;
	case 3:
		aText = GTA(0x4599bc);
		aName = "powerup_magnet";
		break;
	case 4:
		aText = GTA(0x459950);
		aName = "powerup_timebomb";
		break;
	case 5:
		aText = GTA(0x459a2c);
		aName = "powerup_slow";
		break;
	case 6:
		aText = GTA(0x4598a4);
		aName = "powerup_invincibubble";
		break;
	case 7:
		aText = GTA(0x459884);
		aName = "powerup_anchor";
		break;
	case 8:
		aText = GTA(0x4598cc);
		aName = "powerup_destroywalkers";
		break;
	case 9:
		aText = GTA(0x4598fc);
		aName = "powerup_destroyrollers";
		break;
	case 10:
		aText = GTA(0x4599ec);
		aName = "powerup_2x";
		break;
	case 0xb:
		aText = GTA(0x459a48);
		aName = "powerup_hypermo";
		break;
	case 0xc:
		aText = GTA(0x4599a0);
		aName = "powerup_missile";
		break;
	case 0xd:
		aText = GTA(0x459a10);
		aName = "powerup_plusone";
		break;
	default:
		aText = GTA(0x459868);
		aName = "powerup_speed";
		break;
	}

	std::strcat(thePowerup->mText, GT(aText));
	thePowerup->mState = 4;
	thePowerup->mDepth = 1;
	thePowerup->mX = gLevels.mCurrent->mEnemies.mCurrent->mX;
	thePowerup->mY = gLevels.mCurrent->mEnemies.mCurrent->mY;
	Animation* anAnim = Animation_Find(aName, &thePowerup->mFrameCount, &thePowerup->mFrameTime);
	thePowerup->mAnim = anAnim;
	thePowerup->mImage = anAnim->mFrames[0];
	thePowerup->mDrawKind = 1;
	thePowerup->mDirty = 1;
	thePowerup->mMidHandle = 1;
	int aSide = Sign(thePowerup->mX - 400.0f);
	thePowerup->mShadow = 1;
	thePowerup->mAlpha = 0xff;
	thePowerup->mVx = (float)aSide * -0.1f;
	thePowerup->mVy = -0.2f;
	thePowerup->mGravity = 0.0002f;
	List<Obj>* anEmitters = &thePowerup->mEmitters;
	Powerup_AddSparkles(thePowerup, "sparkles1");
	anEmitters->mCurrent->mDrawKind = 4;
	Powerup_AddSparkles(thePowerup, "sparkles3");
	Powerup_AddSparkles(thePowerup, "sparkles2");
	Emitter* aLast = static_cast<Emitter*>(anEmitters->mCurrent);
	aLast->mDrawKind = 5;
	aLast->mStartR = 200;
	aLast->mStartG = 200;
	aLast->mStartB = 0xff;
	aLast->mEndR = 200;
	aLast->mEndG = 200;
	aLast->mEndB = 0xff;
	aLast->mVariance = 0;
	thePowerup->mAttached = nullptr;
	// (the original then calls 0x401410, which is empty, while the flag at 0x460a40 is set)
	return thePowerup;
}

int Powerup_Update(Powerup* thePowerup)
{
	if (thePowerup->mState == 4)
		return Powerup_Fall(thePowerup);
	return 0;
}

void Weapon_Draw(Weapon* theWeapon)
{
	Obj_Draw(theWeapon);
	if (theWeapon->mKind == 4 && theWeapon->mState == 4)
	{
		int aFuse = (int)(9.0 - (double)theWeapon->mAge * 8.0 / (double)theWeapon->mLifetime);
		double aBase = (double)theWeapon->mY - (double)(theWeapon->mImage->mH / 2);
		int aTop = (int)(aBase - (double)aFuse);
		int aBottom = (int)aBase;
		int x = (int)theWeapon->mX;
		Draw_Line(x, aTop, x, aBottom, 0x96, 0x96, 0x96, 0xff);
		x = (int)((double)theWeapon->mX - 1.0);
		Draw_Line(x, aTop, x, aBottom, 0xdc, 0xdc, 0xdc, 0xff);
		Dirty_AddClippedShown(&gDirty, (int)((double)theWeapon->mX - 1.0), aTop, 2, aFuse, SCREEN_CLIP);
	}
}

// To the weapon's right, or above it while the beams go across
void Weapon_DrawTooClose(Weapon* theWeapon)
{
	char aText[32];
	int aRect[4];
	// (the original sprintfs the text, which has no %)
	std::strcpy(aText, GT(0x459ab0));
	if (theWeapon->mTooClose != 0)
	{
		Font* aFont = Font_Find("micro_font");
		Font_SetEscapes(aFont, 1);
		Font_SetTarget(aFont, gScreenBits, gScreenPitch, &gBackBuffer.mFmt, gBackBuffer.mW, gBackBuffer.mH);
		Font_SetPalette(aFont, nullptr);
		aFont->mAlpha = 0xff;
		aFont->mAlign = 0;
		int x;
		int y;
		if (gLevels.mCurrent->mShooters.mCurrent->mBeamAcross == 0)
		{
			x = (int)((double)theWeapon->mX + 20.0);
			y = (int)theWeapon->mY;
		}
		else
		{
			x = (int)((double)theWeapon->mX - (double)(Font_StringWidth(aFont, aText) / 2));
			y = (int)((double)theWeapon->mY - 20.0);
		}
		Font_DrawOutlined(aFont, aRect, x, y, aText);
		int aHeight = Font_StringHeight(aFont, aText) + 2;
		Dirty_AddClippedShown(&gDirty, x - 1, y - 1, Font_StringWidth(aFont, aText) + 2, aHeight, SCREEN_CLIP);
	}
}

// 0 when it is done, and whenever the ship is dying
int Weapon_Update(Weapon* theWeapon)
{
	if (gLevels.mCurrent->mShooters.mCurrent->mState == 6)
		return 0;
	float x = theWeapon->mX;
	float y = theWeapon->mY;
	int aResult = Obj_Update(theWeapon);
	if (theWeapon->mState == 3)
	{
		theWeapon->mX = x;
		theWeapon->mY = y;
		Weapon_Orbit(theWeapon);
	}
	else if (theWeapon->mState == 4)
	{
		int aCol = (int)theWeapon->mX / 16;
		if (-1 < aCol && aCol < 0x32)
		{
			int aRow = (int)theWeapon->mY / 16;
			if (-1 < aRow && aRow < 0x25)
				return aResult & Weapon_Move(theWeapon);
		}
		return 0;
	}
	return aResult;
}

} // namespace wb
