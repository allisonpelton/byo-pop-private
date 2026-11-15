// BoardUpdate.cpp: the board's updates of the things on the field (explosions, craters, shots, sparks and the rest),
// the scrolling, and the small functions that add them. The tank, the waves, the collisions and the level's course are
// in Board.cpp.

#include "Board.h"
#include "Banner.h"
#include "BossDeath.h"
#include "Enemy.h"
#include "FloatingText.h"
#include "HeavyWeaponApp.h"
#include "MegaLaserPart.h"
#include "NukeBlast.h"
#include "PauseScreen.h"
#include "Projectile.h"
#include "Res.h"
#include "Sprite.h"
#include "TankShots.h"
#include "SoundInstance.h"
#include "SoundManager.h"
#include "WidgetManager.h"
#include "X87.h"

#include <math.h>
#include <stdlib.h>

using namespace Sexy;

namespace
{
	const double DEG_TO_RAD = 0.017453292520882225;	// the original's pi / 180
	const double GUN_LENGTH = 36.0;					// from the turret's pivot to the muzzle
	const double GUN_SHOT_SPEED = -5.0;				// the gun's bullets, along the barrel
	const double QUARTER_TURN = 1.5707963268794;	// the original's pi / 2, a little over
	const int GUN_DIRECTIONS = 20;					// the turret's frames: 9 degrees each, -94 to +86
	const int MAX_GUN_ROW = 4;						// the bullet sprite's rows, one a gun power level
	const double PAN_SCALE = 3.0;					// a sound's pan: 3 times its x from the centre
	const int EXPLOSION_FRAMES = 20;
	const int GROUND_Y = 460;						// where craters throw up their smoke and rocks
	const int CRATER_SPACING = 20;
	const int CRATER_LIFE = 1000;
	const int CRATER_FRAMES = 5;
	const int GUN_HEIGHT = 12;						// the turret's pivot above the tank's y
	const int BULLET_POINTS = 10;					// an enemy bullet shot down by an orb or the mega laser
	const double ORB_RADIUS_X = 50.0;				// the orbs' ellipse round the tank
	const double ORB_RADIUS_Y = 40.0;
	const double ORB_DAMAGE = 1000.0;				// to a projectile an orb touches
	const double MEGA_LASER_DAMAGE = 10000.0;		// to a projectile the mega laser touches
	const double BEAM_START = 35.0;					// the lasers' beams start this far from the gun's pivot
	const double BEAM_STEP = 5.0;					// and are tested every 5 pixels
	const double BEAM_END = 800.0;
	const int LASER_BURN_END = 50;					// the lasers burn while their delay (100 when they fire) is above this
	const int BOSS_DEATH_EXPLOSIONS = 20;			// the boss's last explosions before the field clears
	const double TANK_STOP_DISTANCE = 40.0;			// the tank stops with the mouse this close to it
	const double TANK_SLOW_DISTANCE = 80.0;			// and slows down within this
	const int TREAD_FRAMES = 9;
	const int TANK_RANGE = 290;						// the tank's x either side of the centre
	const int TANK_BLOCKED_RANGE_CUT = 95;			// less on a side something blocks (mBlockedLeft, mBlockedRight)
	const int NUM_GUN_SOUNDS = 5;					// SOUND_TANKFIRE and the four after it, by gun power
	const int STATIC_STRIKE_DELAY = 50;				// updates between each special weapon's shots
	const int FLAK_DELAY = 50;
	const int LASER_DELAY = 100;
	const int ROCKET_DELAY = 40;
}

// 0x40fe00: the screen shakes by up to a 16th of mShake each way, the y drawn first
void Board::UpdateShake()
{
	if (mShake <= 0)
		return;

	int aY = Rand() % (mShake / 16 + 1) - mShake / 32;
	int aX = Rand() % (mShake / 16 + 1) - mShake / 32;
	Move(aX, aY);
	mShake -= 2;
	if (mShake <= 0)
	{
		mShake = 0;
		Move(0, 0);
	}
}

// 0x410860: the mega laser fires theAmount updates of its charge; each of the last five seconds' starts beeps. Empty,
// it stops firing.
void Board::UseMegaLaser(int theAmount)
{
	if (theAmount <= 0)
		return;

	int i = 0;
	do
	{
		if (mMegaLaserTime == 0)
		{
			mMegaLaserFiring = 0;
			return;
		}
		int aTime = mMegaLaserTime - 1;
		mMegaLaserTime = aTime;
		if (aTime < 500 && (double)(aTime / 100) == (double)aTime * 0.01)
			mApp->PlaySample(SOUND_MEGALASER_OUT, 0);
		i++;
	}
	while (i < theAmount);
}

// 0x410bd0
void Board::UpdateBanners()
{
	for (mBanners.mCurrent = mBanners.mHead; mBanners.mCurrent->mNext != NULL; )
	{
		mBanners.mCurrent = mBanners.mCurrent->mNext;
		mBanners.Current()->Update();	// may delete it
	}
}

// 0x410cf0
void Board::UpdateTankShots()
{
	for (mTankShots.mCurrent = mTankShots.mHead; mTankShots.mCurrent->mNext != NULL; )
	{
		mTankShots.mCurrent = mTankShots.mCurrent->mNext;
		mTankShots.Current()->Update();	// may delete it
	}
}

// 0x410d40: a bullet of the gun, theSideOffset to the side of the barrel and theAngleOffset degrees off it (the
// spread shot's), its cel the gun's direction and power
void Board::AddGunShot(double theGunPower, int theSideOffset, int theAngleOffset)
{
	Bullet* aBullet = new Bullet();
	mBullets.Append(aBullet);
	mApp->mStats[HeavyWeaponApp::STAT_SHOTS_FIRED]++;
	mShotsFired++;
	mShotsThisPress++;
	aBullet->mIsEnemy = false;
	aBullet->mGravity = 0.0;
	aBullet->mSprite = mApp->mSprites[IMAGE_BULLETS];

	double anAngle = (mTurretFrame * 9.0 - 4.0) * DEG_TO_RAD;
	aBullet->mColumn = (int)((double)(theAngleOffset / 9) + mTurretFrame);
	if (aBullet->mColumn > GUN_DIRECTIONS)
		aBullet->mColumn -= GUN_DIRECTIONS;
	if (aBullet->mColumn < 0)
		aBullet->mColumn += GUN_DIRECTIONS;

	double aSideAngle = QUARTER_TURN + anAngle;
	double aSide = (double)theSideOffset;
	aBullet->mX = (mTankX - X87Cos(anAngle) * GUN_LENGTH) - X87Cos(aSideAngle) * aSide;
	aBullet->mY = ((double)(mTankY - GUN_HEIGHT) - X87Sin(anAngle) * GUN_LENGTH) - X87Sin(aSideAngle) * aSide;
	double aShotAngle = anAngle + (double)theAngleOffset * DEG_TO_RAD;
	aBullet->mVelX = X87Cos(aShotAngle) * GUN_SHOT_SPEED;
	aBullet->mVelY = X87Sin(aShotAngle) * GUN_SHOT_SPEED;
	aBullet->mDamage = theGunPower + 1.0;
	aBullet->mRow = (int)theGunPower;
	if (aBullet->mRow > MAX_GUN_ROW)
		aBullet->mRow = MAX_GUN_ROW;
}

// 0x410ec0: with the tank dead the enemies speed up off the field; the boss is touchable again unless it says so this
// update
void Board::UpdateEnemies()
{
	for (mEnemies.mCurrent = mEnemies.mHead; mEnemies.mCurrent->mNext != NULL; )
	{
		mEnemies.mCurrent = mEnemies.mCurrent->mNext;
		Enemy* anEnemy = mEnemies.Current();
		if (mTankDeadTime != 0 && anEnemy != mBoss)
			anEnemy->mVelX *= 1.01;
		if (anEnemy == mBoss && anEnemy->mIsAlive)
			mBossUntouchable = false;
		anEnemy->Update();	// may delete it
	}
}

// 0x410f40: the explosion throws sparks and smoke as the particle option allows; theFrameSpeed (or, at 0, a speed by
// its size) is overwritten in the original, so every explosion plays at 0.4 frames an update
void Board::AddExplosion(double x, double y, int theWidth, int theHeight, double theVelX, double theVelY,
	double theFrameSpeed)
{
	Explosion* anExplosion = new Explosion();
	mExplosions.Append(anExplosion);
	anExplosion->mX = x;
	anExplosion->mY = y;
	anExplosion->mVelX = theVelX;
	anExplosion->mVelY = theVelY;
	anExplosion->mWidth = theWidth;
	anExplosion->mHeight = theHeight;
	anExplosion->mFrame = 0.0;
	anExplosion->mFrameSpeed = 0.4;

	int aSize = (theWidth + theHeight) / 6;
	if (aSize > 100)
		aSize = 100;
	if (mApp->mParticles != 0.0 && aSize != 0)
	{
		double aVelX = theVelX * 0.5;
		mParticles.Emit(mApp->mSprites[IMAGE_SPARK], x, y, aVelX, theVelY, (int)((double)aSize * mApp->mParticles),
			3.0, 0.05, 3.0, 100, true, -1);
		if (aSize >= 2)
			mParticles.Emit(mApp->mSprites[IMAGE_SMOKE], x, y, aVelX, theVelY,
				(int)((double)(aSize / 2) * mApp->mParticles), 0.5, -0.01, -0.5, 200, true, -1);
	}

	if (theWidth + theHeight > 140)
		mApp->PlaySample(SOUND_BIGEXPLODE, (int)(x * PAN_SCALE));
	else
		mApp->PlaySample(SOUND_SMALLEXPLODE, (int)(x * PAN_SCALE));
}

// 0x4111a0
void Board::UpdateExplosions()
{
	for (mExplosions.mCurrent = mExplosions.mHead; mExplosions.mCurrent->mNext != NULL; )
	{
		mExplosions.mCurrent = mExplosions.mCurrent->mNext;
		Explosion* anExplosion = mExplosions.Current();
		anExplosion->mX = anExplosion->mVelX + anExplosion->mX;
		anExplosion->mY = anExplosion->mY + anExplosion->mVelY;
		anExplosion->mVelX = anExplosion->mVelX * 0.99;
		anExplosion->mVelY = anExplosion->mVelY * 0.99;
		anExplosion->mFrame = anExplosion->mFrameSpeed + anExplosion->mFrame;
		if ((int)anExplosion->mFrame >= EXPLOSION_FRAMES)
			mExplosions.RemoveCurrent();
	}
}

// 0x4112a0
void Board::UpdateProjectiles()
{
	for (mProjectiles.mCurrent = mProjectiles.mHead; mProjectiles.mCurrent->mNext != NULL; )
	{
		mProjectiles.mCurrent = mProjectiles.mCurrent->mNext;
		mProjectiles.Current()->Update();	// may delete it
	}
}

// 0x4112f0: the ground's speed eases up to 1, slows while the boss is out, and runs down to a stop over the last of
// mScrollDistance; the four planes scroll at their fractions of it and wrap, and the craters ride the ground and fade.
// theFrac is the part of an update this frame shows.
void Board::UpdateScroll(float theFrac)
{
	if (mScrollDistance < 10.0)
		mScrollSpeed = mScrollDistance * 0.1;
	else if (mBoss != NULL)
		mScrollSpeed = mScrollSpeed * 0.99;
	else
	{
		mScrollSpeed = mScrollSpeed + 0.01;
		if (mScrollSpeed > 1.0)
			mScrollSpeed = 1.0;
	}

	double aDistance = (double)theFrac * mScrollSpeed;
	double aScroll = (double)mSkyScroll - aDistance * 0.1;
	mSkyScroll = (float)aScroll;
	if (aScroll < -640.0)
		mSkyScroll = (float)(aScroll + 640.0);
	aScroll = (double)mFarBackgroundScroll - aDistance * 0.2;
	mFarBackgroundScroll = (float)aScroll;
	if (aScroll < -960.0)
		mFarBackgroundScroll = (float)(aScroll + 960.0);
	aScroll = (double)mBackgroundScroll - aDistance * 0.3;
	mBackgroundScroll = (float)aScroll;
	if (aScroll < -960.0)
		mBackgroundScroll = (float)(aScroll + 960.0);
	aScroll = (double)mGroundScroll - aDistance;
	mGroundScroll = (float)aScroll;
	if (aScroll < -640.0)
		mGroundScroll = (float)(aScroll + 640.0);
	if (mScrollDistance < 1000.0)
		mScrollDistance = mScrollDistance - aDistance;

	for (mCraters.mCurrent = mCraters.mHead; mCraters.mCurrent->mNext != NULL; )
	{
		mCraters.mCurrent = mCraters.mCurrent->mNext;
		Crater* aCrater = mCraters.Current();
		aCrater->mX = aCrater->mX - (double)theFrac * mScrollSpeed;
		aCrater->mLife--;
		if ((double)(-320 - mApp->mSprites[IMAGE_CRATER]->mCelWidth / 2) > aCrater->mX || aCrater->mLife < 0)
			mCraters.RemoveCurrent();
	}
}

// 0x411540
void Board::AddCrater(int x, int theSpread)
{
	for (int i = -theSpread; i <= theSpread; i++)
	{
		int aX = x + i * CRATER_SPACING;
		Crater* aCrater = new Crater();
		aCrater->mLife = CRATER_LIFE;
		mCraters.Append(aCrater);
		aCrater->mX = (double)aX;
		aCrater->mFrame = Rand() % CRATER_FRAMES;
		mParticles.Emit(mApp->mSprites[IMAGE_SMOKE], (double)aX, (double)GROUND_Y, -1.0, -1.0,
			(int)(mApp->mParticles * 10.0), 1.0, -0.1, -0.5, 100, true, -1);
		mParticles.Emit(mApp->mSprites[IMAGE_ROCK], (double)aX, (double)GROUND_Y, -1.0, -2.0,
			(int)(mApp->mParticles * 20.0), 1.0, 0.05, 3.0, 200, false, -1);
	}
}

// 0x4116b0: the stretch under the tank grows to cover it; the others ride the ground off the left side (their left
// edge held within 4 of -323, so the tread pattern stays in step)
void Board::UpdateTreadMarks()
{
	bool isUnderTank = false;
	for (mTreadMarks.mCurrent = mTreadMarks.mHead; mTreadMarks.mCurrent->mNext != NULL; )
	{
		mTreadMarks.mCurrent = mTreadMarks.mCurrent->mNext;
		TreadMark* aMark = mTreadMarks.Current();
		double aLeft = aMark->mLeft - mScrollSpeed;
		aMark->mLeft = aLeft;
		if (aLeft < -323.0)
			aMark->mLeft = aLeft + 4.0;
		if (!aMark->mIsUnderTank)
		{
			double aRight = aMark->mRight - mScrollSpeed;
			aMark->mRight = aRight;
			if (aRight < -319.0)
				mTreadMarks.RemoveCurrent();
		}
		else
		{
			if (mTankX - 20.0 < aMark->mLeft)
				aMark->mLeft = mTankX - 20.0;
			double aRight = mTankX + 20.0;
			if (aRight <= aMark->mRight)
				aRight = aMark->mRight - mScrollSpeed;
			aMark->mRight = aRight;
			isUnderTank = true;
		}
	}

	if (!isUnderTank && mTankDeadTime == 0)
	{
		TreadMark* aMark = new TreadMark();
		mTreadMarks.Append(aMark);
		aMark->mLeft = mTankX;
		aMark->mIsUnderTank = true;
		aMark->mRight = mTankX;
	}
}

// 0x411800
void Board::UpdateNukeBlasts()
{
	for (mNukeBlasts.mCurrent = mNukeBlasts.mHead; mNukeBlasts.mCurrent->mNext != NULL; )
	{
		mNukeBlasts.mCurrent = mNukeBlasts.mCurrent->mNext;
		mNukeBlasts.Current()->Update();	// may delete it
	}
}

// 0x411850: a bullet of an enemy's gun at (x, y), theAngle radians from straight left (0 to the left, rising
// clockwise), theSpeed pixels an update (0 means 2.5)
void Board::AddEnemyBullet(int x, int y, double theAngle, double theSpeed)
{
	if (mTankDeadTime != 0 || mBossDeath != NULL)
		return;

	Bullet* aBullet = new Bullet();
	mBullets.Append(aBullet);
	aBullet->mIsEnemy = true;
	aBullet->mGravity = 0.0;
	aBullet->mSprite = mApp->mSprites[IMAGE_BOMBFRAG];
	aBullet->mX = (double)x;
	aBullet->mY = (double)y;
	if (theSpeed == 0.0)
		theSpeed = 2.5;
	aBullet->mColumn = 0;
	aBullet->mRow = 0;
	aBullet->mVelX = -(X87Cos(theAngle) * theSpeed);
	aBullet->mVelY = X87Sin(theAngle) * theSpeed;
	mApp->PlaySample(SOUND_ENEMYFIRE, (int)((double)x * PAN_SCALE));
}

// 0x411930
void Board::UpdateMegaLaserParts()
{
	for (mMegaLaserParts.mCurrent = mMegaLaserParts.mHead; mMegaLaserParts.mCurrent->mNext != NULL; )
	{
		mMegaLaserParts.mCurrent = mMegaLaserParts.mCurrent->mNext;
		mMegaLaserParts.Current()->Update();	// may delete it
	}
}

// 0x411980: the casings fall (at most 2 an update) and go once off the left or bottom
void Board::UpdateShellCasings()
{
	for (mShellCasings.mCurrent = mShellCasings.mHead; mShellCasings.mCurrent->mNext != NULL; )
	{
		mShellCasings.mCurrent = mShellCasings.mCurrent->mNext;
		ShellCasing* aCasing = mShellCasings.Current();
		aCasing->mX = aCasing->mVelX + aCasing->mX;
		aCasing->mY = aCasing->mY + aCasing->mVelY;
		aCasing->mVelX = aCasing->mVelX * 0.99;
		double aVelY = aCasing->mVelY + 0.06;
		aCasing->mVelY = aVelY;
		if (aVelY > 2.0)
			aCasing->mVelY = 2.0;
		if (aCasing->mX < -325.0 || aCasing->mY > 485.0)
			mShellCasings.RemoveCurrent();
	}
}

// 0x411ab0
void Board::AddMuzzleFlash(double x, double y, double theVelX, double theVelY)
{
	MuzzleFlash* aFlash = new MuzzleFlash();
	aFlash->mX = x;
	aFlash->mY = y;
	aFlash->mVelX = theVelX;
	aFlash->mVelY = theVelY;
	aFlash->mLife = 5.0;
	mMuzzleFlashes.Append(aFlash);
}

// 0x411b40
void Board::UpdateMuzzleFlashes()
{
	for (mMuzzleFlashes.mCurrent = mMuzzleFlashes.mHead; mMuzzleFlashes.mCurrent->mNext != NULL; )
	{
		mMuzzleFlashes.mCurrent = mMuzzleFlashes.mCurrent->mNext;
		MuzzleFlash* aFlash = mMuzzleFlashes.Current();
		aFlash->mX = aFlash->mVelX + aFlash->mX;
		aFlash->mY = aFlash->mVelY + aFlash->mY;
		aFlash->mLife = aFlash->mLife - 1.0;
		if (aFlash->mLife < 1.0)
			mMuzzleFlashes.RemoveCurrent();
	}
}

// 0x411c30: each spark flickers (a new brightness and frame every update) until its life runs out
void Board::UpdateSparks()
{
	for (mSparks.mCurrent = mSparks.mHead; mSparks.mCurrent->mNext != NULL; )
	{
		mSparks.mCurrent = mSparks.mCurrent->mNext;
		Spark* aSpark = mSparks.Current();
		aSpark->mX = aSpark->mVelX + aSpark->mX;
		aSpark->mY = aSpark->mVelY + aSpark->mY;
		aSpark->mBrightness = Rand() % 32 + 32;
		aSpark->mFrame = Rand() % 5;
		if (aSpark->mLife == 0)
			mSparks.RemoveCurrent();
		else
			aSpark->mLife--;
	}
}

// 0x411d40
void Board::Pause()
{
	if (mPaused || mApp->mDialogMap.size() != 0 || mApp->mHelpScreen != NULL)
		return;

	mPaused = true;
	PauseScreen* aPauseScreen = new PauseScreen(this);
	mApp->mWidgetManager->AddWidget(aPauseScreen);
	mApp->mWidgetManager->SetFocus(aPauseScreen);
}

// 0x411de0
void Board::Suspend()
{
	mSuspended = true;
	if (mMegaLaserSound != NULL)
		mMegaLaserSound->SetVolume(0.0);
	for (mEnemies.mCurrent = mEnemies.mHead; mEnemies.mCurrent->mNext != NULL; )
	{
		mEnemies.mCurrent = mEnemies.mCurrent->mNext;
		mEnemies.Current()->SilenceSounds();
	}
}

// 0x411e40
void Board::Resume()
{
	mSuspended = false;
	if (mMegaLaserSound != NULL)
		mMegaLaserSound->SetVolume(1.0);
	for (mEnemies.mCurrent = mEnemies.mHead; mEnemies.mCurrent->mNext != NULL; )
	{
		mEnemies.mCurrent = mEnemies.mCurrent->mNext;
		mEnemies.Current()->RestoreSounds();
	}
}

// 0x411eb0: the flashes ride the ground and fade (type 0 by 3 an update, others by 5)
void Board::UpdateGroundFlashes(float theFrac)
{
	for (mGroundFlashes.mCurrent = mGroundFlashes.mHead; mGroundFlashes.mCurrent->mNext != NULL; )
	{
		mGroundFlashes.mCurrent = mGroundFlashes.mCurrent->mNext;
		GroundFlash* aFlash = mGroundFlashes.Current();
		aFlash->mX = aFlash->mX - (double)theFrac * mScrollSpeed;
		if (aFlash->mType == 0)
			aFlash->mAlpha -= 3;
		else
			aFlash->mAlpha -= 5;
		if (aFlash->mAlpha < 1)
			mGroundFlashes.RemoveCurrent();
	}
}

// 0x411fa0
void Board::AddGroundFlash(int x, int theType)
{
	GroundFlash* aFlash = new GroundFlash();
	aFlash->mX = (double)x;
	aFlash->mAlpha = 255;
	aFlash->mType = theType;
	mGroundFlashes.Append(aFlash);
}

// 0x417390: the first destructible projectile in reach takes theDamage and flashes; below 0 health it is destroyed
// for its points, otherwise it sparks. The original passes the projectile's mirroring for theSprite's too, kept.
bool Board::HitProjectiles(Sprite* theSprite, int x, int y, int theColumn, int theRow, double theDamage)
{
	if (mTankDeadTime != 0 && theSprite == mApp->mSprites[IMAGE_TANK])
		return false;

	for (mProjectiles.mCurrent = mProjectiles.mHead; mProjectiles.mCurrent->mNext != NULL; )
	{
		mProjectiles.mCurrent = mProjectiles.mCurrent->mNext;
		Projectile* aProjectile = mProjectiles.Current();
		if (!aProjectile->mIsDestructible || !aProjectile->mIsShootable || aProjectile->mIsInBackground)
			continue;
		if (!theSprite->Collides(x, y, theColumn, theRow, aProjectile->mMirrored, aProjectile->GetSprite(),
				(int)aProjectile->mX, (int)aProjectile->mY, (int)aProjectile->mFrame, 0, aProjectile->mMirrored, true))
			continue;

		aProjectile->mHitFlash = 255;
		aProjectile->mHealth = aProjectile->mHealth - theDamage;
		if (aProjectile->mHealth < 0.0)
		{
			AddScore(aProjectile->mPoints, (int)aProjectile->mX, (int)aProjectile->mY);
			AddExplosion(aProjectile->mX, aProjectile->mY, aProjectile->GetCelWidth() + 40,
				aProjectile->GetCelHeight() + 40, aProjectile->mVelX, aProjectile->mVelY - 0.5, 0.0);
			aProjectile->Remove();
			mApp->mStats[HeavyWeaponApp::STAT_PROJECTILES_DESTROYED]++;
			mProjectilesDestroyed++;
			return true;
		}

		mApp->PlaySample(SOUND_BULLETHIT, x * 3);
		mParticles.Emit(mApp->mSprites[IMAGE_SPARK], (double)x, (double)y, aProjectile->mVelX, aProjectile->mVelY,
			(int)(mApp->mParticles * 10.0), 2.0, 0.05, 3.0, 30, true, -1);
		return true;
	}
	return false;
}

// 0x4138e0: the boss's last explosions; at the first the shots in the air vanish, after the twentieth everything on
// the field is killed and cleared with a flash, and the ground rolls on to the next sign
void Board::UpdateBossDeath()
{
	mBossDeath->Update();
	if (mBossDeath->mExplosions == 1)
	{
		mProjectiles.Clear();
		mBullets.Clear();
	}
	if (mBossDeath->mExplosions <= BOSS_DEATH_EXPLOSIONS)
		return;

	for (mEnemies.mCurrent = mEnemies.mHead; mEnemies.mCurrent->mNext != NULL; )
	{
		mEnemies.mCurrent = mEnemies.mCurrent->mNext;
		mEnemies.Current()->Killed();
	}
	mEnemies.Clear();
	mBoss = NULL;
	mShake = 254;
	mNukeFlash = 255.0;
	for (mBullets.mCurrent = mBullets.mHead; mBullets.mCurrent->mNext != NULL; )
	{
		mBullets.mCurrent = mBullets.mCurrent->mNext;
		if (mBullets.Current()->mIsEnemy)
			mBullets.RemoveCurrent();
	}
	mProjectiles.Clear();
	mApp->PlaySample(SOUND_BOSSBLAST, 0);
	delete mBossDeath;
	mBossDeath = NULL;
	mScrollDistance = 480.0;
	mLevelTime++;
	mSignFrame = Rand() % NUM_SIGNS;
}

// 0x414a70: the texts count down and go; rainbow ones cycle their hue. Then a text that overlaps another one, lower
// down or level with it, pushes that one up 2.
void Board::UpdateFloatingTexts()
{
	for (mFloatingTexts.mCurrent = mFloatingTexts.mHead; mFloatingTexts.mCurrent->mNext != NULL; )
	{
		mFloatingTexts.mCurrent = mFloatingTexts.mCurrent->mNext;
		FloatingText* aText = mFloatingTexts.Current();
		if (aText->mIsRainbow)
		{
			aText->mHue = (aText->mHue + 16) % 256;
			ulong aColor = mApp->HSLToRGB(aText->mHue, 255, 192);
			aText->mColor.mRed = (aColor >> 16) & 0xff;
			aText->mColor.mGreen = (aColor >> 8) & 0xff;
			aText->mColor.mBlue = aColor & 0xff;
		}
		aText->mAlpha--;
		if (aText->mAlpha == 0)
			mFloatingTexts.RemoveCurrent();
	}

	if (mFloatingTexts.mCount == 0)
		return;
	for (ObjectList<FloatingText>::Node* aNode = mFloatingTexts.mHead->mNext; aNode != NULL; aNode = aNode->mNext)
	{
		FloatingText* aText = aNode->mItem;
		for (mFloatingTexts.mCurrent = mFloatingTexts.mHead; mFloatingTexts.mCurrent->mNext != NULL; )
		{
			mFloatingTexts.mCurrent = mFloatingTexts.mCurrent->mNext;
			FloatingText* anOther = mFloatingTexts.Current();
			if (aText == anOther)
				continue;
			int aY = aText->mAlpha / 2 + aText->mY;
			int anOtherY = anOther->mY + anOther->mAlpha / 2;
			if (aText->mX < anOther->mWidth + anOther->mX && aY - 148 < anOtherY - 128 &&
				anOther->mX < aText->mWidth + aText->mX && anOtherY - 148 < aY - 128 && anOtherY - 128 <= aY - 128)
				anOther->mY -= 2;
		}
	}
}

// 0x418770: the orbs circle the tank (10 - 2n degrees an update, n of them), 120 degrees apart or, two of them, 180;
// each hits the projectiles it touches as a 1000-damage shot and stops the enemy bullets it touches
void Board::UpdateOrbs()
{
	mOrbAngle = (double)(10 - 2 * mApp->mWeapons[HeavyWeaponApp::WEAPON_ORBS]) + mOrbAngle;
	if (mOrbAngle > 360.0)
		mOrbAngle = mOrbAngle - 360.0;

	for (int i = 0; i < mApp->mWeapons[HeavyWeaponApp::WEAPON_ORBS]; i++)
	{
		DefenseOrb& anOrb = mOrbs[i];
		anOrb.mFlash = anOrb.mFlash - 0.02;
		if (anOrb.mFlash < 0.0)
			anOrb.mFlash = 0.0;
		int anOffset = mApp->mWeapons[HeavyWeaponApp::WEAPON_ORBS] == 2 ? i * 180 : i * 120;
		anOrb.mX = X87Cos(((double)anOffset + mOrbAngle) * DEG_TO_RAD) * ORB_RADIUS_X + mTankX;
		anOrb.mY = X87Sin(((double)anOffset + mOrbAngle) * DEG_TO_RAD) * ORB_RADIUS_Y + (double)mTankY;
		if (HitProjectiles(mApp->mSprites[IMAGE_ORB], (int)anOrb.mX, (int)anOrb.mY, 0, 0, ORB_DAMAGE))
		{
			anOrb.mFlash = 1.0;
			mApp->PlaySample(SOUND_ORBHIT, (int)(anOrb.mX * PAN_SCALE));
		}

		for (mBullets.mCurrent = mBullets.mHead; mBullets.mCurrent->mNext != NULL; )
		{
			mBullets.mCurrent = mBullets.mCurrent->mNext;
			Bullet* aBullet = mBullets.Current();
			if (!aBullet->mIsEnemy)
				continue;
			if (!mApp->mSprites[IMAGE_ORB]->Collides((int)anOrb.mX, (int)anOrb.mY, 0, 0, false, aBullet->mSprite,
					(int)aBullet->mX, (int)aBullet->mY, aBullet->mColumn, aBullet->mRow, false, true))
				continue;
			anOrb.mFlash = 1.0;
			AddScore(BULLET_POINTS, (int)aBullet->mX, (int)aBullet->mY);
			mBullets.RemoveCurrent();
			AddExplosion(anOrb.mX, anOrb.mY, 50, 50, 0.0, 0.0, 0.0);
			mApp->PlaySample(SOUND_ORBHIT, (int)(anOrb.mX * PAN_SCALE));
		}
	}
}

// 0x418a50: for the first 50 updates after the lasers fire, the beam reaches from the gun towards the mouse, 5 pixels
// further each step up to 800, and stops at the first enemy or projectile it touches, burning 0.2 health a laser level
// off an enemy (0.25 a level off a projectile). The field's edges stop it only while an enemy is out: they are checked
// in the walk over the enemies.
void Board::UpdateLasers()
{
	if (mWeaponDelays[HeavyWeaponApp::WEAPON_LASERS] < LASER_BURN_END)
		return;

	int aTargetY = mMouseY;
	if (aTargetY > mTankY - GUN_HEIGHT)
		aTargetY = mTankY - GUN_HEIGHT;
	mLaserLength = BEAM_START;
	mLaserAngle = X87Atan2((double)(mMouseX - 320) - mTankX, (double)((mTankY - aTargetY) - GUN_HEIGHT)) - 1.57;
	do
	{
		double aReach = mLaserLength + BEAM_STEP;
		double x = X87Cos(mLaserAngle) * aReach + mTankX;
		double y = X87Sin(mLaserAngle) * aReach + (double)(mTankY - GUN_HEIGHT);
		for (mEnemies.mCurrent = mEnemies.mHead; mEnemies.mCurrent->mNext != NULL; )
		{
			mEnemies.mCurrent = mEnemies.mCurrent->mNext;
			Enemy* anEnemy = mEnemies.Current();
			if (x < -330.0 || x > 330.0 || y < 0.0)
				return;
			if (!anEnemy->mIsAlive)
				continue;
			if (!mApp->mSprites[IMAGE_ROCK]->Collides((int)x, (int)y, 0, 0, false, anEnemy->GetSprite(),
					(int)anEnemy->mX, (int)anEnemy->mY, (int)anEnemy->mFrame, 0, anEnemy->mMirrored, true))
				continue;
			anEnemy->mHitFlash = 255;
			anEnemy->mHealth = anEnemy->mHealth - (double)mApp->mWeapons[HeavyWeaponApp::WEAPON_LASERS] * 0.2;
			if (anEnemy->mHealth >= 0.0)
				return;
			anEnemy->Killed();
			if (anEnemy != mBoss)
				anEnemy->Remove();
			return;
		}
		if (HitProjectiles(mApp->mSprites[IMAGE_ROCK], (int)x, (int)y, 0, 0,
				(double)mApp->mWeapons[HeavyWeaponApp::WEAPON_LASERS] * 0.25))
			return;
		mLaserLength = mLaserLength + BEAM_STEP;
	}
	while (mLaserLength < BEAM_END);
}

// 0x418c90: the mega laser spends two updates of its charge, and its beam reaches from the gun towards the mouse to
// the field's edge, killing every enemy (not the player's units), projectile and enemy bullet it touches
void Board::UpdateMegaLaser()
{
	int aTargetY = mMouseY;
	if (aTargetY > mTankY - GUN_HEIGHT)
		aTargetY = mTankY - GUN_HEIGHT;
	mLaserAngle = X87Atan2((double)(mMouseX - 320) - mTankX, (double)((mTankY - aTargetY) - GUN_HEIGHT)) - 1.57;
	UseMegaLaser(2);
	mLaserLength = BEAM_START;
	do
	{
		double x = X87Cos(mLaserAngle) * mLaserLength + mTankX;
		double y = X87Sin(mLaserAngle) * mLaserLength + (double)(mTankY - GUN_HEIGHT);
		if (x < -320.0 || x > 320.0 || y < 0.0)
			break;

		Sprite* aBeam = mApp->mSprites[IMAGE_BEAM];
		for (mEnemies.mCurrent = mEnemies.mHead; mEnemies.mCurrent->mNext != NULL; )
		{
			mEnemies.mCurrent = mEnemies.mCurrent->mNext;
			Enemy* anEnemy = mEnemies.Current();
			if (anEnemy->mIsFriendly || !anEnemy->mIsAlive)
				continue;
			if (!aBeam->Collides((int)x, (int)y, 0, 0, false, anEnemy->GetSprite(), (int)anEnemy->mX,
					(int)anEnemy->mY, (int)anEnemy->mFrame, 0, anEnemy->mMirrored, true))
				continue;
			anEnemy->Killed();
			anEnemy->Remove();
		}

		HitProjectiles(aBeam, (int)x, (int)y, 0, 0, MEGA_LASER_DAMAGE);
		for (mBullets.mCurrent = mBullets.mHead; mBullets.mCurrent->mNext != NULL; )
		{
			mBullets.mCurrent = mBullets.mCurrent->mNext;
			Bullet* aBullet = mBullets.Current();
			if (!aBullet->mIsEnemy)
				continue;
			if (!aBeam->Collides((int)x, (int)y, 0, 0, false, aBullet->mSprite, (int)aBullet->mX, (int)aBullet->mY,
					aBullet->mColumn, aBullet->mRow, false, true))
				continue;
			AddScore(BULLET_POINTS, (int)aBullet->mX, (int)aBullet->mY);
			mBullets.RemoveCurrent();
		}
		mLaserLength = mLaserLength + BEAM_STEP;
	}
	while (mLaserLength < BEAM_END);

	if (mMegaLaserSound != NULL)
		mMegaLaserSound->SetPan((int)(mTankX * PAN_SCALE));
}

// 0x418f50: each background animation steps through its frames at each frame's own speed (to the end and back, or
// looping; a nuked one holds its last frame), drifts by its own speed and scrolls with its plane, and goes once it is
// off the left side
void Board::UpdateAnims(float theFrac)
{
	for (int aPlane = 0; aPlane < NUM_ANIM_PLANES; aPlane++)
	{
		BackgroundAnim* anAnim = mAnims[aPlane];
		if (anAnim == NULL)
			continue;

		int aFrameCount = anAnim->mSprite->mColumns;
		int anEnd = aFrameCount;
		if (anAnim->mHasNukedFrame)
			anEnd = aFrameCount - 1;
		if (!anAnim->mIsNuked)
		{
			anAnim->mFrame = (double)anAnim->mDirection * anAnim->mFrameDelays[(int)anAnim->mFrame] + anAnim->mFrame;
			if ((double)anEnd <= anAnim->mFrame)
			{
				if (anAnim->mPlaysOnce == 1)
				{
					anAnim->mDirection = -1;
					anAnim->mFrame = (double)(anEnd - 1);
				}
				else
					anAnim->mFrame = anAnim->mFrame - (double)anEnd;
			}
			else if (anAnim->mFrame < 0.0)
			{
				anAnim->mDirection = 1;
				anAnim->mFrame = 1.0;
			}
		}
		else
			anAnim->mFrame = (double)(aFrameCount - 1);

		anAnim->mX = (double)theFrac * anAnim->mMoveX * mScrollSpeed + anAnim->mX;
		switch (anAnim->mPlane)
		{
		case PLANE_GROUND:
			anAnim->mX = anAnim->mX - (double)theFrac * mScrollSpeed;
			break;
		case PLANE_BACKGROUND:
			anAnim->mX = anAnim->mX - (double)theFrac * mScrollSpeed * 0.3;
			break;
		case PLANE_FAR_BACKGROUND:
			anAnim->mX = anAnim->mX - (double)theFrac * mScrollSpeed * 0.2;
			break;
		case PLANE_SKY:
			anAnim->mX = anAnim->mX - (double)theFrac * mScrollSpeed * 0.1;
			break;
		}

		if ((double)-anAnim->mSprite->mCelWidth > anAnim->mX)
		{
			delete mAnims[aPlane];
			mAnims[aPlane] = NULL;
		}
	}
}

// 0x419eb0: a free plane starts one of the level's animations one update in 200 (a rare one only one time in 20 of
// those), if the one drawn belongs to that plane, until 2000 updates before the boss; it comes in off the right side
void Board::StartAnims()
{
	for (int aPlane = 0; aPlane < NUM_ANIM_PLANES; aPlane++)
	{
		if (mAnims[aPlane] != NULL)
			continue;
		std::vector<BackgroundAnimInfo>& anInfos = mApp->mAnims[mApp->GetBossIndex(mApp->mLevel)];
		if (anInfos.size() == 0)
			continue;
		if (mLevelTime >= mLevelLength - 2000 && !mIsSurvival)
			continue;
		if (Rand() % 200 != 0)
			continue;
		BackgroundAnimInfo& anInfo = anInfos[(unsigned int)Rand() % anInfos.size()];
		if (anInfo.mRare && Rand() % 20 != 0)
			continue;
		if (anInfo.mPlane != aPlane)
			continue;

		BackgroundAnim* anAnim = new BackgroundAnim();
		mAnims[aPlane] = anAnim;
		anAnim->mFrame = 0.0;
		for (int aFrame = 0; aFrame < anInfo.mSprite->mColumns; aFrame++)
			anAnim->mFrameDelays.push_back(anInfo.mFrameDelays[aFrame]);
		anAnim->mMoveX = anInfo.mMoveX;
		anAnim->mIsNuked = false;
		anAnim->mHasNukedFrame = anInfo.mHasNukedFrame;
		anAnim->mRare = anInfo.mRare;
		anAnim->mPlane = anInfo.mPlane;
		anAnim->mSprite = anInfo.mSprite;
		anAnim->mX = anInfo.mOffset;
		anAnim->mY = anInfo.mY;
		anAnim->mDirection = 1;
		anAnim->mPlaysOnce = anInfo.mPlaysOnce ? 1 : 0;
		if (aPlane == PLANE_BACKGROUND)
			anAnim->mX = (double)mBackgroundScroll + anAnim->mX;
		if (aPlane == PLANE_FAR_BACKGROUND)
			anAnim->mX = (double)mFarBackgroundScroll + anAnim->mX;
		if (anAnim->mX < 640.0)
			anAnim->mX = anAnim->mX + 960.0;
		if (anAnim->mX < 640.0)
			anAnim->mX = anAnim->mX + 960.0;
	}
}

// port: the original works out each power-up's effect inline, three times in UpdateTank: (level + 1) to the power
// 2 / ln(level + 6), 1 at level 0 and growing ever more slowly
static double PowerUpFactor(int theLevel)
{
	return pow((double)theLevel + 1.0, 2.0 / X87Log((double)theLevel + 6.0));
}

// 0x412dc0: the tank's update. The gun turns towards the mouse (at most 90 degrees from upright) and the tank drives
// towards it, faster the further the gun leans, slowing within 80 pixels and stopping within 40; a new tank first
// drives in from the left. While the left button is held the gun fires (a shot more for each spread level, its
// sound a blend of the five gun sounds by gun power) and so does every special weapon that is ready.
void Board::UpdateTank()
{
	if ((mBossDeath != NULL || mLevelTime > mLevelLength) && !mIsSurvival)
	{
		mMouseDown = false;
		mMouseX = 320;
		mMouseY = 320;
		mWeaponDelays[HeavyWeaponApp::WEAPON_LASERS] = 0;
	}
	if (mShieldHitGlow > 0.0)
		mShieldDropDelay = mIsSurvival ? 1000 : mLevelLength / 8;
	if (mMegaLaserTime != 0)
		UseMegaLaser(1);
	else
	{
		if (mMegaLaserSound != NULL)
			mMegaLaserSound->Release();
		mMegaLaserSound = NULL;
	}
	mShieldFrame = mShieldFrame + 0.1;
	if (mShieldFrame >= 10.0)
		mShieldFrame = mShieldFrame - 10.0;

	double aMouseX = (double)(mMouseX - 320);
	double anAngle = X87Atan2(aMouseX - mTankX, (double)(mTankY - mMouseY - GUN_HEIGHT)) * 57.29577951;
	if (anAngle < -90.0)
		anAngle = -90.0;
	else if (anAngle > 90.0)
		anAngle = 90.0;
	mTurretFrame = (anAngle + 94.0) * 0.1111111111111111;

	double aMaxSpeed = PowerUpFactor(mApp->mPowerUps[HeavyWeaponApp::POWERUP_SPEED]) * 0.25 + 1.75;
	double aGunAngle = anAngle + 90.0;
	double aSpeed = (aGunAngle * 0.1111111111111111 - 10.0) * 0.5;
	double aDistance;
	if (mMouseY < mTankY)
	{
		// the original's pow(d, 2.0), which is d * d
		double aDeltaY = (double)(mMouseY - mTankY);
		double aDeltaX = aMouseX - mTankX;
		aDistance = sqrt(aDeltaY * aDeltaY + aDeltaX * aDeltaX);
	}
	else
		aDistance = (double)abs((int)(aMouseX - mTankX));
	if (aDistance < TANK_STOP_DISTANCE)
		aSpeed = 0.0;
	else if (aDistance < TANK_SLOW_DISTANCE)
		aSpeed = (aDistance - TANK_STOP_DISTANCE) * aSpeed * 0.025;
	if (aSpeed < -aMaxSpeed)
		aSpeed = -aMaxSpeed;
	if (aSpeed > aMaxSpeed)
		aSpeed = aMaxSpeed;

	mTreadFrame = mScrollSpeed * 0.3 + aSpeed * 0.3333333333333333 + mTreadFrame;
	if (mTreadFrame > (double)TREAD_FRAMES)
		mTreadFrame = mTreadFrame - (double)TREAD_FRAMES;
	if (mTreadFrame < 0.0)
		mTreadFrame = mTreadFrame + (double)TREAD_FRAMES;

	// a new tank drives in from the left until it is on the field, unless the mouse is well away from it (and not
	// at the window's left edge)
	if ((mMouseX >= 30 && aDistance >= TANK_SLOW_DISTANCE) || mTankX >= (double)-TANK_RANGE)
	{
		mTankX = aSpeed + mTankX;
		double aLeft = (double)(mBlockedLeft ? -TANK_RANGE + TANK_BLOCKED_RANGE_CUT : -TANK_RANGE);
		if (mTankX < aLeft && aSpeed < 0.0)
			mTankX = aLeft;
		double aRight = (double)(mBlockedRight ? TANK_RANGE - TANK_BLOCKED_RANGE_CUT : TANK_RANGE);
		if (aRight < mTankX)
			mTankX = aRight;
	}
	else
		mTankX = mTankX + 2.0;

	if (mGunDelay != 0)
		mGunDelay--;
	for (int i = 0; i < NUM_WEAPON_DELAYS; i++)
	{
		if (mWeaponDelays[i] != 0)
			mWeaponDelays[i]--;
	}

	if (mMouseDown && mMegaLaserTime == 0)
	{
		if (mGunDelay == 0)
		{
			double aSpread;
			double aGunPower;
			double aFireRate;
			if (mApp->mHelpScreen != NULL)
			{
				aFireRate = 0.0;
				aSpread = 0.0;
				aGunPower = 0.0;
			}
			else
			{
				aSpread = (double)mApp->mPowerUps[HeavyWeaponApp::POWERUP_SPREAD];
				aGunPower = PowerUpFactor(mApp->mPowerUps[HeavyWeaponApp::POWERUP_GUN_POWER]);
				aFireRate = PowerUpFactor(mApp->mPowerUps[HeavyWeaponApp::POWERUP_FIRE_RATE]) - 1.0;
			}
			mGunDelay = (int)((15.0 - aFireRate) + (aSpread + aSpread));

			// The gun's sound: gun power 1 plays the first of the five gun sounds, each 1.5 more the next; between two
			// both play, each the quieter the further it is
			double aSound = (aGunPower - 1.0) * 0.16666666666666666;
			if (aSound > 1.0)
				aSound = 1.0;
			aSound = aSound * (double)(NUM_GUN_SOUNDS - 1);
			if ((double)(int)aSound == aSound)
				mApp->PlaySample((int)(aSound + 1.0), (int)(mTankX * PAN_SCALE));
			else
			{
				int aLower = (int)floor(aSound);
				int anUpper = (int)ceil(aSound);
				if (aLower >= 0)
				{
					int aSoundId = SOUND_TANKFIRE + aLower;
					mApp->mSoundManager->SetBaseVolume(aSoundId,
						(1.0 - (aSound - (double)aLower)) * gSoundInfo[aSoundId].mBaseVolume);
					mApp->PlaySample(aSoundId, (int)(mTankX * PAN_SCALE));
					mApp->mSoundManager->SetBaseVolume(aSoundId, gSoundInfo[aSoundId].mBaseVolume);
				}
				if (anUpper < NUM_GUN_SOUNDS)
				{
					int aSoundId = SOUND_TANKFIRE + anUpper;
					mApp->mSoundManager->SetBaseVolume(aSoundId,
						(1.0 - ((double)anUpper - aSound)) * gSoundInfo[aSoundId].mBaseVolume);
					mApp->PlaySample(aSoundId, (int)(mTankX * PAN_SCALE));
					mApp->mSoundManager->SetBaseVolume(aSoundId, gSoundInfo[aSoundId].mBaseVolume);
				}
			}

			double aRadians = aGunAngle * DEG_TO_RAD;
			double aFlashY = (double)(mTankY - GUN_HEIGHT) - X87Sin(aRadians) * GUN_LENGTH;
			double aFlashX = mTankX - X87Cos(aRadians) * GUN_LENGTH;
			AddMuzzleFlash(aFlashX, aFlashY, aSpeed, 0.0);
			for (int i = 0; i <= aSpread; i++)
				AddGunShot(aGunPower, 0, (int)((double)(i * 10) - aSpread * 5.0));

			ShellCasing* aCasing = new ShellCasing();
			mShellCasings.Append(aCasing);
			aCasing->mX = mTankX - 5.0;
			aCasing->mVelY = -1.5;
			aCasing->mY = (double)(mTankY - 10);
			aCasing->mVelX = aSpeed - 1.5;
		}

		if (mApp->mHelpScreen == NULL)
		{
			if (mApp->mWeapons[HeavyWeaponApp::WEAPON_STATIC] != 0 &&
				mWeaponDelays[HeavyWeaponApp::WEAPON_STATIC] == 0)
			{
				mWeaponDelays[HeavyWeaponApp::WEAPON_STATIC] = STATIC_STRIKE_DELAY;
				new StaticStrike(this);
			}
			int aHomingLevel = mApp->mWeapons[HeavyWeaponApp::WEAPON_HOMING];
			if (aHomingLevel != 0 && mWeaponDelays[HeavyWeaponApp::WEAPON_HOMING] == 0)
			{
				mWeaponDelays[HeavyWeaponApp::WEAPON_HOMING] = 85 - 15 * aHomingLevel;
				new HomingMissile(this, mTankX, (double)mTankY);
			}
			if (mApp->mWeapons[HeavyWeaponApp::WEAPON_FLAK] != 0 && mWeaponDelays[HeavyWeaponApp::WEAPON_FLAK] == 0)
			{
				mWeaponDelays[HeavyWeaponApp::WEAPON_FLAK] = FLAK_DELAY;
				new FlakBurst(this);
			}
			if (mApp->mWeapons[HeavyWeaponApp::WEAPON_LASERS] != 0 &&
				mWeaponDelays[HeavyWeaponApp::WEAPON_LASERS] == 0)
			{
				mWeaponDelays[HeavyWeaponApp::WEAPON_LASERS] = LASER_DELAY;
				mApp->PlaySample(SOUND_TANKLASER, (int)(mTankX * PAN_SCALE));
			}
			if (mApp->mWeapons[HeavyWeaponApp::WEAPON_ROCKETS] != 0 &&
				mWeaponDelays[HeavyWeaponApp::WEAPON_ROCKETS] == 0)
			{
				mWeaponDelays[HeavyWeaponApp::WEAPON_ROCKETS] = ROCKET_DELAY;
				for (int i = 0; i < mApp->mWeapons[HeavyWeaponApp::WEAPON_ROCKETS]; i++)
				{
					// the pods fire 90 degrees apart split between them, the fan centred near upright
					int aPods = mApp->mWeapons[HeavyWeaponApp::WEAPON_ROCKETS];
					int anAngleOffset = 90 / aPods * i;
					if (aPods > 1)
						anAngleOffset += -22 - (aPods == 3 ? 8 : 0);
					new Rocket(this, mTankX, (double)mTankY, anAngleOffset);
				}
				mApp->PlaySample(SOUND_SMALLMISSILE, (int)(mTankX * PAN_SCALE));
			}
		}
	}

	if (mMegaLaserFiring != 0 && mMegaLaserSound == NULL)
	{
		mMegaLaserSound = mApp->mSoundManager->GetSoundInstance(SOUND_MEGALASER);
		if (mMegaLaserSound != NULL)
		{
			mMegaLaserSound->SetVolume(mApp->GetSfxVolume());
			mMegaLaserSound->Play(true, true);
		}
	}
	if (mMegaLaserFiring == 0)
	{
		if (mMegaLaserSound != NULL)
			mMegaLaserSound->Release();
		mMegaLaserSound = NULL;
	}
}
