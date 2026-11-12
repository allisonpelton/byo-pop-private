// The board's panel (0x417d40..0x4183b0), the power-ups (0x4183b0..0x419780) and the weapons some of them give
// (0x419790..0x41aec0). A power-up falls from where an enemy died; when the ship touches it, it shows its name and
// changes the ship or the game, or gives the ship a weapon to orbit it. A click launches the first orbiting weapon.

#include "CBLevel.h"
#include "R64Sys.h"

#include <cstdio>
#include <cstring>
#include <new>

namespace cb
{

namespace
{

int gPanelPercent = -1;            // 0x460a38  what the panel last drew
int gPanelLives = -1;              // 0x460a34
int gPanelScore = -1;              // 0x460a30
int gPowerupKinds[0x78];           // 0x6e5c00  a power-up's kind by a random index
int gRebuildKinds = 1;             // 0x460a48  (never cleared: each power-up builds the table again)

// 0x417e60: a new item at the tail, made current
void Panel_AppendItem(List<Obj>* theList)
{
	Obj* anItem = (Obj*)::operator new(sizeof(Obj));
	Obj_Init(anItem);
	List_Append(theList, anItem);
}

// 0x4183b0: ten entries for each kind from 0 to 9, 0 for the indexes past 99 but six more of 10 at 110..115
void Powerup_BuildKinds()
{
	for (uint32_t i = 0; (int)i < 0x78; i++)
	{
		gPowerupKinds[i] = i / 10;
		if (99 < (int)i)
			gPowerupKinds[i] = 0;
	}
	gPowerupKinds[110] = 10;
	gPowerupKinds[111] = 10;
	gPowerupKinds[112] = 10;
	gPowerupKinds[113] = 10;
	gPowerupKinds[114] = 10;
	gPowerupKinds[115] = 10;
}

// The effects. 0x418440: a faster beam (by the beams' starting speed)
void Powerup_FastShot()
{
	Shooter* aShip = gLevels.mCurrent->mShooters.mCurrent;
	aShip->mBeamSpeed = (float)(((double)gBeamSpeed - (double)gDifficulty * (double)0.05f) + (double)aShip->mBeamSpeed);
}

// 0x418470: half the speed, but not under half the starting speed
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

// 0x4184c0
void Powerup_ExtraLife()
{
	gLives = gLives + 1;
}

// 0x4184d0: a cell scores one more
void Powerup_Multiplier()
{
	Shooter* aShip = gLevels.mCurrent->mShooters.mCurrent;
	aShip->mCellPoints = aShip->mCellPoints + 1;
}

// 0x4184f0
void Powerup_SlowMotion()
{
	Shooter* aShip = gLevels.mCurrent->mShooters.mCurrent;
	aShip->mFlags = aShip->mFlags | 1;
	gLevels.mCurrent->mShooters.mCurrent->mSlowMotionTime = 5000;
}

// 0x418520
void Powerup_HyperMotion()
{
	Shooter* aShip = gLevels.mCurrent->mShooters.mCurrent;
	aShip->mFlags = aShip->mFlags | 2;
	gLevels.mCurrent->mShooters.mCurrent->mHyperMotionTime = 5000;
}

// 0x418550
void Powerup_Sweepers()
{
	Shooter* aShip = gLevels.mCurrent->mShooters.mCurrent;
	aShip->mFlags = aShip->mFlags | 4;
}

// 0x418570
void Powerup_FlipFlopper()
{
	Shooter* aShip = gLevels.mCurrent->mShooters.mCurrent;
	aShip->mFlags = aShip->mFlags | 8;
}

// 0x418610
void Powerup_Invincibubble()
{
	Shooter* aShip = gLevels.mCurrent->mShooters.mCurrent;
	aShip->mFlags = aShip->mFlags | 0x10;
}

// 0x418630: a new weapon at the tail, made current; 0x40abe0 (a weapon's constructor, which only calls Obj_Init) is
// inlined here
void Weapon_Append(List<Weapon>* theList)
{
	Weapon* aWeapon = (Weapon*)::operator new(sizeof(Weapon));
	Obj_Init(aWeapon); // 0x40abe0
	List_Append(theList, aWeapon);
}

void Weapon_Setup(Weapon* theWeapon, float x, float y, float theRadius, int theKind);

// 0x418720, 0x4187c0, 0x418810, 0x418860: a weapon where the power-up is
void Powerup_GiveWeapon(Powerup* thePowerup, float theRadius, int theKind)
{
	Weapon_Append(&gLevels.mCurrent->mWeapons);
	Weapon_Setup(gLevels.mCurrent->mWeapons.mCurrent, thePowerup->mX, thePowerup->mY, theRadius, theKind);
}

// 0x419420: shows its name (removing any long or main_font text still up) and takes effect
int Powerup_Collect(Powerup* thePowerup)
{
	gProfiles[gProfile].mPowerupsCaught = gProfiles[gProfile].mPowerupsCaught + 1;
	List<Text>* aTexts = &gLevels.mCurrent->mTexts;
	List_Each(aTexts, [&](Text* theText) {
		if (0xd < std::strlen(theText->mText) || theText->mFont == Font_Find("main_font"))
			List_RemoveCurrent(aTexts);
	});
	Text_Append(aTexts);
	Text_Make(aTexts->mCurrent, thePowerup->mText, Font_Find("main_font"), SCREEN_CENTRE_X, 0xe6, 0, 200, 2000, 200, 0, 0);
	const char* aSound;
	switch (thePowerup->mKind)
	{
	case POWERUP_SLOW_MOTION:
		Powerup_SlowMotion();
		break;
	case POWERUP_MULTIPLIER:
		Powerup_Multiplier();
		break;
	case POWERUP_SLOW_SHOT:
		Powerup_SlowShot();
		aSound = "audio\\powerdn.wav";
		goto play;
	case POWERUP_TIME_BOMB:
		Powerup_GiveWeapon(thePowerup, 50.0f, WEAPON_TIME_BOMB);
		break;
	case POWERUP_HYPER_MOTION:
		Powerup_HyperMotion();
		aSound = "audio\\powerdn.wav";
		goto play;
	case POWERUP_MISSILE:
		Powerup_GiveWeapon(thePowerup, 40.0f, WEAPON_MISSILE);
		break;
	case POWERUP_FREEZE:
		Powerup_GiveWeapon(thePowerup, 40.0f, WEAPON_ICE_SHOT);
		break;
	case POWERUP_MINE:
		Powerup_GiveWeapon(thePowerup, 50.0f, WEAPON_MINE);
		break;
	case POWERUP_SWEEPERS:
		Powerup_Sweepers();
		break;
	case POWERUP_EXTRA_LIFE:
		Powerup_ExtraLife();
		break;
	default:
		Powerup_FastShot();
		break;
	case POWERUP_FLIP_FLOPPER:
		Powerup_FlipFlopper();
		aSound = "audio\\powerdn.wav";
		goto play;
	case POWERUP_INVINCIBUBBLE:
		Powerup_Invincibubble();
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

// 0x419660: falls; collected when it touches the ship, gone below the screen
int Powerup_Fall(Powerup* thePowerup)
{
	thePowerup->mVy = thePowerup->mGravity + thePowerup->mVy;
	Obj_Update(thePowerup);
	Shooter* aShip = gLevels.mCurrent->mShooters.mCurrent;
	if (aShip != nullptr && aShip->mState == SHIP_CONTROLLED &&
		CircleOverlap(thePowerup->mX, thePowerup->mY, (float)(thePowerup->mImage->mH / 2), aShip->mX, aShip->mY,
			(float)(aShip->mImage->mH / 4)) != 0.0)
	{
		Powerup_Collect(thePowerup);
		return 0;
	}
	float y = thePowerup->mY;
	int h = thePowerup->mImage->mH;
	Obj_UpdateEmitters(thePowerup);
	return y <= (float)(h + SCREEN_HEIGHT);
}

// The weapons. 0x4199c0: a missile's animation by the way it points
void Missile_Face(Weapon* theWeapon)
{
	if (theWeapon->mKind == WEAPON_MISSILE)
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

// 0x419e70 (a mine or a missile): blows up the enemies it touches; 0 when it did
int Weapon_Blast(Weapon* theWeapon)
{
	int aResult = 1;
	List_Each(&gLevels.mCurrent->mEnemies, [&](Enemy* theEnemy) {
		if (theEnemy->mState == ENEMY_ALIVE &&
			CircleOverlap(theWeapon->mX, theWeapon->mY, (float)(theWeapon->mImage->mW / 3), theEnemy->mX, theEnemy->mY,
				(float)(theEnemy->mImage->mW / 3)) != 0.0)
		{
			theEnemy->mDieAnim = Animation_Find("second_explo", &theEnemy->mDieFrameCount, &theEnemy->mDieFrameTime);
			Enemy_Kill(theEnemy, 500);
			aResult = 0;
		}
	});
	return aResult;
}

// 0x419fb0 (an ice shot): freezes the enemies it touches (0 when it did), shedding the odd blue sparkle
int Ice_Freeze(Weapon* theWeapon)
{
	int aResult = 1;
	List_Each(&gLevels.mCurrent->mEnemies, [&](Enemy* theEnemy) {
		if (theEnemy->mState == ENEMY_ALIVE &&
			CircleOverlap(theWeapon->mX, theWeapon->mY, (float)(theWeapon->mImage->mW / 3), theEnemy->mX, theEnemy->mY,
				(float)(theEnemy->mImage->mW / 3)) != 0.0)
		{
			aResult = 0;
			Enemy_Freeze(theEnemy);
		}
	});
	if (theWeapon->mKind == WEAPON_ICE_SHOT && RandRange(1, 0x23) == 1)
	{
		ParticleSystem_New();
		int aParticle = theWeapon->mParticle;
		int aLifetime = RandRange(600, 800);
		float aVy = (float)((double)RandRange(-0x32, 0x32) / 400.0);
		float aVx = (float)((double)RandRange(-0x32, 0x32) / 400.0);
		float y = (float)RandRange(-5, 5) + theWeapon->mY;
		float x = (float)RandRange(-5, 5) + theWeapon->mX;
		ParticleSystem_Add(gLevels.mCurrent->mSystems.mCurrent, x, y, aVx, aVy, aParticle, aParticle, 0, 200, 200, 0xff,
			0x58, 0x58, 0xff, 400, 10, aLifetime, 0.999f, 0.0002f, 5);
	}
	return aResult;
}

// 0x41a1d0 (a time bomb): sparks fly off the top of the fuse, which burns down, hissing every 230 updates; at the
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
		List_Each(&gLevels.mCurrent->mEnemies, [&](Enemy* theEnemy) {
			if (theEnemy->mState == ENEMY_ALIVE &&
				CircleOverlap(theWeapon->mX, theWeapon->mY, 100.0f, theEnemy->mX, theEnemy->mY,
					(float)(theEnemy->mImage->mW / 3)) != 0.0)
			{
				theEnemy->mDieAnim = Animation_Find("second_explo", &theEnemy->mDieFrameCount, &theEnemy->mDieFrameTime);
				Enemy_Kill(theEnemy, 500);
			}
		});
		Obj_Explode(theWeapon, 0.0f, 0.0f);
		theWeapon->mLifetime = 0.0f;
		theWeapon->mAge = 0.0f;
	}
	return 1;
}

// 0x41a490: a click sends it straight out at its speed, the way it is from the ship (unless an enemy is too close)
void Weapon_Launch(Weapon* theWeapon)
{
	if (theWeapon->mTooClose == 0)
	{
		theWeapon->mVx = (float)Sign(theWeapon->mVx) * theWeapon->mLaunchSpeed;
		theWeapon->mState = WEAPON_LAUNCHED;
		theWeapon->mVy = (float)Sign(theWeapon->mVy) * theWeapon->mLaunchSpeed;
		theWeapon->mAge = 0.0f;
		int aKind = theWeapon->mKind;
		// (each sound is panned by x: the port's Sound_Play takes no pan)
		if (aKind != WEAPON_MISSILE)
		{
			const char* aSound;
			if (aKind == WEAPON_ICE_SHOT)
				aSound = "audio\\ice_launch.wav";
			else if (aKind == WEAPON_MINE)
				aSound = "audio\\mine.wav";
			else
			{
				if (aKind == WEAPON_TIME_BOMB)
				{
					Sound_Play(Sound_Find("audio\\mine.wav"));
					theWeapon->mLifetime = 2000.0f;
				}
				return;
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

// 0x41a7a0 (launched): slows by its drag and does its work; 0 when it is done or off the screen
int Weapon_Move(Weapon* theWeapon)
{
	int aResult = 1;
	theWeapon->mVx = theWeapon->mDrag * theWeapon->mVx;
	theWeapon->mVy = theWeapon->mVy * theWeapon->mDrag;
	switch (theWeapon->mKind)
	{
	case WEAPON_MINE:
		aResult = Weapon_Blast(theWeapon);
		break;
	case WEAPON_MISSILE:
		aResult = Weapon_Blast(theWeapon);
		break;
	case WEAPON_ICE_SHOT:
		aResult = Ice_Freeze(theWeapon);
		break;
	case WEAPON_TIME_BOMB:
		aResult = TimeBomb_Update(theWeapon);
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

// 0x41a8b0: a new weapon orbits opposite the first one already there; any second one goes. It ends current.
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
	List_Each(aWeapons, [&](Weapon* theOther) {
		if (theOther->mState == WEAPON_ORBITING && theOther != theWeapon)
		{
			if (aCount == 0)
				theWeapon->mAngle = (theOther->mAngle + 0xb4) % 0x168;
			else
				List_RemoveCurrent(aWeapons);
			aCount = aCount + 1;
		}
	});
	aWeapons->mCurrent = theWeapon;
}

// 0x41a990
void Weapon_Setup(Weapon* theWeapon, float x, float y, float theRadius, int theKind)
{
	theWeapon->mX = x;
	theWeapon->mTooClose = 0;
	theWeapon->mY = y;
	theWeapon->mKind = theKind;
	theWeapon->mState = WEAPON_ORBITING;
	theWeapon->mRadius = theRadius;
	theWeapon->mDepth = 3;
	theWeapon->mShadow = 1;
	theWeapon->mAlpha = 0xff;
	theWeapon->mAcross = gLevels.mCurrent->mShooters.mCurrent->mBeamAcross;
	switch (theKind)
	{
	case 0:
		theWeapon->mAnim = Animation_Find("shooter_mine", &theWeapon->mFrameCount, &theWeapon->mFrameTime);
		theWeapon->mLaunchSpeed = 0.15f;
		theWeapon->mDrag = 0.996f;
		break;
	case 1:
		Missile_Face(theWeapon);
		theWeapon->mLaunchSpeed = 0.15f;
		theWeapon->mDrag = 1.002f;
		break;
	case 3:
		theWeapon->mAnim = Animation_Find("ice_shot", &theWeapon->mFrameCount, &theWeapon->mFrameTime);
		theWeapon->mLaunchSpeed = 0.2f;
		theWeapon->mDrag = 1.001f;
		theWeapon->mParticle = Bank_IndexOf(&gArtBank, "particle_0");
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
	}
	theWeapon->mDirty = 1;
	theWeapon->mDrawKind = 4;
	theWeapon->mMidHandle = 1;
	Weapon_TakeSlot(theWeapon);
}

// 0x41ac00 (orbiting): at its angle and radius about the ship, a quarter turn more each time the ship turns;
// faint while an enemy touches it or when it is not the first, which a click launches
void Weapon_Orbit(Weapon* theWeapon)
{
	theWeapon->mTooClose = 0;
	theWeapon->mAlpha = 0xff;
	List_Each(&gLevels.mCurrent->mEnemies, [&](Enemy* theEnemy) {
		if (CircleOverlap(theWeapon->mX, theWeapon->mY, (float)(theWeapon->mImage->mW / 2), theEnemy->mX, theEnemy->mY,
				(float)(theEnemy->mImage->mW / 2)) != 0.0 &&
			theWeapon->mKind != WEAPON_TIME_BOMB)
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

void Panel_FromXml(Panel* thePanel, const char* theCursor, const XmlTag* theTag)
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
	Obj_FromXml(thePanel, theCursor, theTag);
	thePanel->mDirty = 0;
}

// The panel a level without a panel tag gets: the panel image along the bottom, the logo at its right
void Panel_Setup(Panel* thePanel)
{
	thePanel->mPercentX = 0x44;
	thePanel->mPercentY = 0x220;
	thePanel->mLivesX = 0x19b;
	thePanel->mLivesY = 0x220;
	thePanel->mScoreX = 0xa8;
	thePanel->mScoreY = 0x220;
	thePanel->mLevelX = 0x2c4;
	thePanel->mLevelY = 0x230;
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
	anItems->mCurrent->mAnim = Animation_Find("logo_o", &anItems->mCurrent->mFrameCount, &anItems->mCurrent->mFrameTime);
	anItems->mCurrent->mX = 540.0f;
	anItems->mCurrent->mY = 542.0f;
	anItems->mCurrent->mMidHandle = 1;
	anItems->mCurrent->mDrawKind = 1;
	thePanel->mDrawKind = 0;
}

// (it returns 1)
void Panel_Update(Panel* thePanel)
{
	List_Each(&thePanel->mItems, [](Obj* theItem) { Obj_Update(theItem); });
}

// Into the back buffer when theAll is set or a number changed (the lives shown stop at 9, and so do the lives);
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
		if (99 < aPercent)
			aPercent = 99;
	}
	Obj_Update(thePanel);
	if (theAll != 0 || aPercent != gPanelPercent || gLives != gPanelLives || gScore != gPanelScore)
	{
		Image* anImage = thePanel->mImage;
		Image_BlitRect(anImage, gBackBuffer.mBits, gBackBuffer.mPitch, &gBackBuffer.mFmt, (int)thePanel->mX,
			(int)thePanel->mY, 0, 0, anImage->mW, anImage->mH);
		std::sprintf(aText, "%02d", aPercent);
		Font_SetTarget(thePanel->mPercentFont, gBackBuffer.mBits, gBackBuffer.mPitch, &gBackBuffer.mFmt, gBackBuffer.mW,
			gBackBuffer.mH);
		Font_Printf(thePanel->mPercentFont, aRect, thePanel->mPercentX, thePanel->mPercentY, aText);
		std::sprintf(aText, "%03d", gLevels.mCurrent->mNumber);
		Font_SetTarget(thePanel->mLevelFont, gBackBuffer.mBits, gBackBuffer.mPitch, &gBackBuffer.mFmt, gBackBuffer.mW,
			gBackBuffer.mH);
		Font_Printf(thePanel->mLevelFont, aRect, thePanel->mLevelX, thePanel->mLevelY, aText);
		int aScore = gScore;
		if (999999 < gScore)
			aScore = 999999;
		std::sprintf(aText, "%06d", aScore);
		Font_SetTarget(thePanel->mScoreFont, gBackBuffer.mBits, gBackBuffer.mPitch, &gBackBuffer.mFmt, gBackBuffer.mW,
			gBackBuffer.mH);
		Font_Printf(thePanel->mScoreFont, aRect, thePanel->mScoreX, thePanel->mScoreY, aText);
		if (8 < gLives)
			gLives = 9;
		std::sprintf(aText, "%01d", gLives);
		Font_SetTarget(thePanel->mLivesFont, gBackBuffer.mBits, gBackBuffer.mPitch, &gBackBuffer.mFmt, gBackBuffer.mW,
			gBackBuffer.mH);
		Font_Printf(thePanel->mLivesFont, aRect, thePanel->mLivesX, thePanel->mLivesY, aText);
		Dirty_AddClippedShown(&gDirty, 0, (int)thePanel->mY, thePanel->mImage->mW, thePanel->mImage->mH, SCREEN_CLIP);
		gPanelLives = gLives;
		gPanelScore = gScore;
		gPanelPercent = aPercent;
	}
	List_Each(&thePanel->mItems, [](Obj* theItem) {
		if (theItem->mImage != nullptr)
		{
			Obj_Draw(theItem);
			Dirty_AddClippedImage(&gDirty, (int)theItem->mX, (int)theItem->mY, theItem->mImage, SCREEN_CLIP);
		}
	});
}

void Powerup_Draw(Powerup* thePowerup)
{
	Obj_Draw(thePowerup);
}

void List_DeleteNode(Powerup* thePowerup)
{
	// 0x4186c0
	Obj_Destroy(thePowerup);
	::operator delete(thePowerup);
}

// A kind by the level (the later the level, the more kinds), and its text, animation and three emitters of sparkles.
// The keys 0..9 and A..D, held while it is made, choose kinds 0..13 (the developers' test). The table gives kinds
// 0..10 (0 a faster beam), so only those keys make 12 and 13. The original's kinds 14..17 (an anchor, two
// explode-alls and a magnet) nothing makes, and the port leaves them out (docs/README.md, "The unneeded code").
Powerup* Powerup_Init(Powerup* thePowerup)
{
	Obj_Init(thePowerup);
	if (gRebuildKinds != 0)
		Powerup_BuildKinds();
	int aRange = gLevels.mCurrent->mNumber / 2;
	if (0xb < aRange)
		aRange = 0xb;
	aRange = aRange * 10 + 9;
	thePowerup->mKind = gPowerupKinds[RandRange(0, aRange)];
	std::sprintf(thePowerup->mText, "^%d", RandRange(1, 8));
	if (Key_Down('0') != 0)
		thePowerup->mKind = POWERUP_FAST_SHOT;
	if (Key_Down('1') != 0)
		thePowerup->mKind = POWERUP_SLOW_MOTION;
	if (Key_Down('2') != 0)
		thePowerup->mKind = POWERUP_MULTIPLIER;
	if (Key_Down('3') != 0)
		thePowerup->mKind = POWERUP_SLOW_SHOT;
	if (Key_Down('4') != 0)
		thePowerup->mKind = POWERUP_TIME_BOMB;
	if (Key_Down('5') != 0)
		thePowerup->mKind = POWERUP_HYPER_MOTION;
	if (Key_Down('6') != 0)
		thePowerup->mKind = POWERUP_MISSILE;
	if (Key_Down('7') != 0)
		thePowerup->mKind = POWERUP_FREEZE;
	if (Key_Down('8') != 0)
		thePowerup->mKind = POWERUP_MINE;
	if (Key_Down('9') != 0)
		thePowerup->mKind = POWERUP_SWEEPERS;
	if (Key_Down('A') != 0)
		thePowerup->mKind = POWERUP_EXTRA_LIFE;
	if (Key_Down('B') != 0)
		thePowerup->mKind = 11;
	if (Key_Down('C') != 0)
		thePowerup->mKind = POWERUP_FLIP_FLOPPER;
	if (Key_Down('D') != 0)
		thePowerup->mKind = POWERUP_INVINCIBUBBLE;
	// The texts and names are the exe's (each strcat'd from its .rdata)
	uint32_t aText;
	const char* aName;
	switch (thePowerup->mKind)
	{
	case POWERUP_SLOW_MOTION:
		aText = GTA(0x4598d0);
		aName = "powerup_slowmo";
		break;
	case POWERUP_MULTIPLIER:
		aText = GTA(0x459854);
		aName = "powerup_2x";
		break;
	case POWERUP_SLOW_SHOT:
		aText = GTA(0x459894);
		aName = "powerup_slow";
		break;
	case POWERUP_TIME_BOMB:
		aText = GTA(0x4597b8);
		aName = "powerup_timebomb";
		break;
	case POWERUP_HYPER_MOTION:
		aText = GTA(0x4598b0);
		aName = "powerup_hypermo";
		break;
	case POWERUP_MISSILE:
		aText = GTA(0x459808);
		aName = "powerup_missile";
		break;
	case POWERUP_FREEZE:
		aText = GTA(0x4597d4);
		aName = "powerup_freeze";
		break;
	case POWERUP_MINE:
		aText = GTA(0x45983c);
		aName = "powerup_mine";
		break;
	case POWERUP_SWEEPERS:
		aText = GTA(0x4597ec);
		aName = "powerup_sweeper";
		break;
	case POWERUP_EXTRA_LIFE:
		aText = GTA(0x459878);
		aName = "powerup_plusone";
		break;
	default:
		aText = GTA(0x4596d0);
		aName = "powerup_speed";
		break;
	case POWERUP_FLIP_FLOPPER:
		aText = GTA(0x459794);
		aName = "powerup_flip_flopper";
		break;
	case POWERUP_INVINCIBUBBLE:
		aText = GTA(0x45970c);
		aName = "powerup_invincibubble";
		break;
	}
	std::strcat(thePowerup->mText, GT(aText));
	thePowerup->mState = POWERUP_FALLING;
	thePowerup->mDepth = 1;
	thePowerup->mX = gLevels.mCurrent->mEnemies.mCurrent->mX;
	thePowerup->mY = gLevels.mCurrent->mEnemies.mCurrent->mY;
	Animation* anAnim = Animation_Find(aName, &thePowerup->mFrameCount, &thePowerup->mFrameTime);
	thePowerup->mAnim = anAnim;
	thePowerup->mImage = anAnim->mFrames[0];
	thePowerup->mDrawKind = 1;
	thePowerup->mDirty = 1;
	thePowerup->mMidHandle = 1;
	int aSide = Sign(thePowerup->mX - (float)SCREEN_CENTRE_X);
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
	// (the original then calls 0x40fe10, which is empty, while the flag at 0x460a40 is set)
	return thePowerup;
}

int Powerup_Update(Powerup* thePowerup)
{
	if (thePowerup->mState == POWERUP_FALLING)
		return Powerup_Fall(thePowerup);
	return 0;
}

void Weapon_Draw(Weapon* theWeapon)
{
	Obj_Draw(theWeapon);
	if (theWeapon->mKind == WEAPON_TIME_BOMB && theWeapon->mState == WEAPON_LAUNCHED)
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
	std::strcpy(aText, GT(0x459918));
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
	if (gLevels.mCurrent->mShooters.mCurrent->mState == SHIP_DYING)
		return 0;
	float x = theWeapon->mX;
	float y = theWeapon->mY;
	int aResult = Obj_Update(theWeapon);
	if (theWeapon->mState == WEAPON_ORBITING)
	{
		theWeapon->mX = x;
		theWeapon->mY = y;
		Weapon_Orbit(theWeapon);
	}
	else if (theWeapon->mState == WEAPON_LAUNCHED)
	{
		int aCol = (int)theWeapon->mX / CELL_SIZE;
		if (-1 < aCol && aCol < GRID_COLUMNS)
		{
			int aRow = (int)theWeapon->mY / CELL_SIZE;
			if (-1 < aRow && aRow < GRID_ROWS)
				return aResult & Weapon_Move(theWeapon);
		}
		return 0;
	}
	return aResult;
}

} // namespace cb
