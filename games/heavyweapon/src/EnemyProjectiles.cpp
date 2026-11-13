#include "EnemyProjectiles.h"
#include "Board.h"
#include "DDImage.h"
#include "Enemy.h"
#include "Graphics.h"
#include "HeavyWeaponApp.h"
#include "Particles.h"
#include "Res.h"
#include "Sprite.h"

#include <math.h>
#include <stdlib.h>

using namespace Sexy;

namespace
{
	const int FIELD_EDGE_X = 320;				// a projectile goes when it is more than half its width past this
	const int GROUND_Y = 460;					// it lands when it passes this
	const int BLAST_SIZE = 70;					// the explosion where a bomb or missile bursts
	const int BOULDER_BLAST_SIZE = 90;
	const double DEGREES_TO_RADIANS = 0.017453292520882225;
	const double HEADING_DEGREES_PER_FRAME = 17.143;	// a missile's frame is its heading: this many degrees a frame
	const double HEADING_FRAME_CENTRE = 10.0;			// its frame 10 is straight on
	const double HEADING_OFFSET = 1.5705;				// the heading's angle is measured from the vertical
	const double FRAGMENT_GRAVITY = 0.05;				// a bullet thrown from a bomb falls this fast

	// port: the bomb's fall, shared by the dumb bomb, the iron bomb and the frag bomb: it moves, slows sideways and
	// speeds up falling (to 2), and its frame advances (to 9)
	void FallBomb(Projectile* theBomb)
	{
		theBomb->mX = theBomb->mX + theBomb->mVelX;
		theBomb->mY = theBomb->mVelY + theBomb->mY;
		theBomb->mVelX = theBomb->mVelX * 0.995;
		if (theBomb->mVelY < 2.0)
			theBomb->mVelY = theBomb->mVelY + 0.05;
		if (theBomb->mFrame < 9.0)
			theBomb->mFrame = theBomb->mFrame + 0.1;
	}

	// port: is it past the field's sides (the original writes this test out in each Update)
	bool IsPastSides(Projectile* theProjectile)
	{
		return !((double)(-FIELD_EDGE_X - theProjectile->GetCelWidth() / 2) <= theProjectile->mX &&
			theProjectile->mX <= (double)(theProjectile->GetCelWidth() / 2 + FIELD_EDGE_X));
	}

	// port: does it touch the tank (the original writes this out in each Update)
	bool TouchesTank(Projectile* theProjectile)
	{
		Board* aBoard = theProjectile->mBoard;
		return theProjectile->mSprite->Collides((int)theProjectile->mX, (int)theProjectile->mY, (int)theProjectile->mFrame,
			0, theProjectile->mMirrored, aBoard->mApp->mSprites[IMAGE_TANK], (int)aBoard->mTankX, aBoard->mTankY,
			(int)aBoard->mTreadFrame, 0, false, true);
	}

	// port: the end of most projectiles' Update, which the original writes out in each of them: off the sides it is
	// gone; on the ground it bursts and leaves a crater; on the tank it bursts and kills it; else the shield's hit
	void FinishUpdate(Projectile* theProjectile, int theBlastSize)
	{
		Board* aBoard = theProjectile->mBoard;
		if (IsPastSides(theProjectile))
		{
			theProjectile->Remove();
			return;
		}
		if (GROUND_Y < theProjectile->mY)
		{
			aBoard->AddExplosion(theProjectile->mX, theProjectile->mY, theBlastSize, theBlastSize, -1.0, 0.0, 0.0);
			theProjectile->HitGround();
			return;
		}
		if (aBoard->mTankDeadTime == 0 && TouchesTank(theProjectile))
		{
			aBoard->AddExplosion(theProjectile->mX, theProjectile->mY, theBlastSize, theBlastSize, -1.0, 0.0, 0.0);
			aBoard->KillTank();
			theProjectile->Remove();
			return;
		}
		theProjectile->Projectile::Update();
	}

	// port: the smoke a missile leaves as it is made
	void EmitLaunchSmoke(Projectile* theMissile)
	{
		Board* aBoard = theMissile->mBoard;
		aBoard->mParticles.Emit(aBoard->mApp->mSprites[IMAGE_SMOKE], theMissile->mX, theMissile->mY, 0.0, 0.0, 4, 1.0, -0.05,
			-0.5, 40, true, -1);
	}

	// port: the sparks behind a missile that moves by (theVelX, theVelY) a frame, from 5 frames behind it
	void EmitTrail(Projectile* theMissile, double theTrailX, double theTrailY, double theVelX, double theVelY)
	{
		Board* aBoard = theMissile->mBoard;
		aBoard->mParticles.Emit(aBoard->mApp->mSprites[IMAGE_SPARK], theTrailX, theTrailY, theVelX, theVelY, 1, 1.0, 0.0, 0.0,
			30, true, -1);
	}

	// port: a bullet thrown from a bomb or missile, falling, that harms the tank
	Bullet* AddFragmentBullet(Board* theBoard, double x, double y, double theVelX, double theVelY)
	{
		Bullet* aBullet = new Bullet();
		theBoard->mBullets.Append(aBullet);
		aBullet->mX = x;
		aBullet->mY = y;
		aBullet->mGravity = FRAGMENT_GRAVITY;
		aBullet->mIsEnemy = true;
		aBullet->mVelX = theVelX;
		aBullet->mVelY = theVelY;
		aBullet->mSprite = theBoard->mApp->mSprites[IMAGE_BOMBFRAG];
		aBullet->mColumn = 0;
		aBullet->mRow = 0;
		return aBullet;
	}
}

//
// AirMine
//

// 0x447a30
AirMine::AirMine(Board* theBoard, double x, double y, double theVelX, double theVelY) : Projectile(theBoard)
{
	mX = x;
	mY = y;
	mVelX = theVelX;
	mVelY = theVelY;
	mHealth = 20.0;
	mPoints = 40;
	mSprite = mBoard->mApp->mSprites[IMAGE_AIRMINE];
}

// 0x447ab0
AirMine::~AirMine()
{
}

// 0x447ac0: it falls faster to 3 pixels an update, and sways toward the tank's x, harder the nearer the ground
void AirMine::Update()
{
	mX = mX + mVelX;
	mY = mVelY + mY;
	if (mVelY < 3.0)
		mVelY = mVelY + 0.02;
	double aSway = 0.1 - cos(mY * 0.0032710416666666665) * 0.1;
	if (mBoard->mTankX <= mX)
		mVelX = mVelX - aSway;
	else
		mVelX = aSway + mVelX;
	if (mVelX <= 3.0)
	{
		if (mVelX < -3.0)
			mVelX = -3.0;
	}
	else
		mVelX = 3.0;
	mFrame = mFrame + 0.5;
	if (5.0 <= mFrame)
		mFrame = mFrame - 5.0;
	FinishUpdate(this, BLAST_SIZE);
}

//
// Boulder
//

// 0x447e00
Boulder::Boulder(Board* theBoard, double x, double y, double theVelX, double theVelY, bool isDark) : Projectile(theBoard)
{
	mX = x;
	mY = y;
	mVelX = theVelX;
	mVelY = theVelY;
	mFrame = (double)(Rand() % 5);
	mSprite = mBoard->mApp->mSprites[IMAGE_BOULDER];
	mPoints = 0;
	mHealth = 20.0;
	mIsDark = isDark;
}

// 0x447ec0
Boulder::~Boulder()
{
}

// 0x447ed0: red when hit; a dark one is grey, and redder when hit
void Boulder::Draw(Graphics* g)
{
	if (!mIsDark)
	{
		if (mHitFlash != 0)
		{
			g->SetColorizeImages(true);
			g->SetColor(Color(255, 255 - mHitFlash, 255 - mHitFlash, 255));
		}
	}
	else
	{
		g->SetColorizeImages(true);
		if (mHitFlash != 0)
			g->SetColor(Color(0x80, 0x80 - mHitFlash / 2, 0x80 - mHitFlash / 2, 0xff));
		else
			g->SetColor(Color(0x80, 0x80, 0x80));
	}
	mSprite->Draw(g, (int)mX, (int)mY, true, (int)mFrame, 0, false);
	g->SetColorizeImages(false);
}

// 0x447fa0
void Boulder::Update()
{
	mX = mVelX + mX;
	mY = mVelY + mY;
	if (mVelY < 2.0)
		mVelY = mVelY + 0.05;
	FinishUpdate(this, BOULDER_BLAST_SIZE);
}

//
// BurstRocket
//

// 0x448130
BurstRocket::BurstRocket(Board* theBoard, double x, double y, double theVelX, bool isMirrored, int theBulletCount) :
	Projectile(theBoard)
{
	mX = x;
	mY = y;
	mVelX = theVelX;
	mVelY = 0.0;
	mFrame = 0.0;
	mHealth = 80.0;
	mMirrored = isMirrored;
	mBulletCount = theBulletCount;
	mSprite = mBoard->mApp->mSprites[IMAGE_BURSTROCKET];
	mPoints = 100;
}

// 0x4481b0
BurstRocket::~BurstRocket()
{
}

// 0x449fa0: mBulletCount bullets round a circle, starting straight up, thrown out at 3 pixels an update
void BurstRocket::Burst()
{
	double anAngle = 1.5708;
	do
	{
		double aCos = cos(anAngle);
		AddFragmentBullet(mBoard, mX, mY, aCos * 3.0, sin(anAngle) * -3.0);
		anAngle = 6.282 / (double)mBulletCount + anAngle;
	}
	while (anAngle < 7.8539);
}

// 0x44b010: it flies level; when it is within 20 pixels of the tank's x (with the tank alive) it bursts
void BurstRocket::Update()
{
	mX = mX + mVelX;
	mFrame = mFrame + 0.2;
	if (8.0 <= mFrame)
		mFrame = 0.0;

	if (!IsPastSides(this))
	{
		Board* aBoard = mBoard;
		if (aBoard->mTankX + 20.0 <= mX || mX <= aBoard->mTankX - 20.0 || aBoard->mTankDeadTime != 0)
		{
			Projectile::Update();
			return;
		}
		aBoard->AddExplosion(mX, mY, 140, 70, mVelX, 0.0, 0.0);
		Burst();
	}
	Remove();
}

//
// DumbBomb
//

// 0x4481c0
DumbBomb::DumbBomb(Board* theBoard, double x, double y, double theVelX, double theVelY, bool isMirrored) :
	Projectile(theBoard)
{
	mX = x;
	mY = y;
	mVelX = theVelX;
	mVelY = theVelY;
	mFrame = 0.0;
	mMirrored = isMirrored;
	mSprite = mBoard->mApp->mSprites[IMAGE_DUMBBOMB];
	mBoard->mApp->PlaySample(SOUND_BOMBFALL, (int)(x * 3.0));
}

// 0x448280
DumbBomb::~DumbBomb()
{
}

// 0x449010
void DumbBomb::Update()
{
	FallBomb(this);
	FinishUpdate(this, BLAST_SIZE);
}

//
// FatBoy
//

// 0x448290: isArmed makes it start with its frame at 9 (its light on)
FatBoy::FatBoy(Board* theBoard, double x, double y, double theVelX, double theVelY, bool isMirrored, bool isArmed) :
	Projectile(theBoard)
{
	mX = x;
	mY = y;
	mVelX = theVelX;
	mVelY = theVelY;
	mMirrored = isMirrored;
	mHealth = 60.0;
	mIsDetonating = false;
	mFlareTime = 0.2;
	mPoints = 200;
	mFrame = isArmed ? 9.0 : 0.0;
	mSprite = mBoard->mApp->mSprites[IMAGE_FATBOY];
	mBoard->mApp->PlaySample(SOUND_BOMBFALL, (int)(x * 3.0));
}

// 0x448380
FatBoy::~FatBoy()
{
}

// 0x44a0e0: while it goes off, a flare added over it that grows each update and dims
void FatBoy::Draw(Graphics* g)
{
	if (!mIsDetonating)
	{
		Projectile::Draw(g);
		return;
	}

	Sprite* aFlare = mBoard->mApp->mSprites[IMAGE_FATBOYFLASH];
	int aWidth = (int)((double)aFlare->mCelWidth * mFlareTime);
	int aHeight = (int)((double)aFlare->mCelHeight * mFlareTime);
	g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
	g->SetColorizeImages(true);
	int aShade = (int)(512.0 - mFlareTime * 84.0);
	if (aShade > 255)
		aShade = 255;
	g->SetColor(Color(aShade, aShade, aShade));
	Rect aDestRect((int)(mX - (double)(aWidth / 2)), (int)(mY - (double)(aHeight / 2)), aWidth, aHeight);
	Rect aSrcRect(0, 0, 200, 80);
	g->DrawImage(aFlare->mImage, aDestRect, aSrcRect);
	g->SetColorizeImages(false);
	g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
}

// 0x448390: it falls like a bomb, and where it lands or touches the tank or its shield it goes off: the screen flashes
// white, and 29 updates later the tank dies, shield or none
void FatBoy::Update()
{
	if (!mIsDetonating)
	{
		FallBomb(this);
		if (!IsPastSides(this))
		{
			if (GROUND_Y < mY)
			{
				mIsDetonating = true;
				mIsShootable = false;
				mBoard->mNukeFlash = 128.0;
				mBoard->AddExplosion(mX, mY, 200, 200, 0.0, 0.0, 0.05);
				mBoard->AddCrater((int)mX, 0);
				return;
			}
			Board* aBoard = mBoard;
			if (aBoard->mTankDeadTime != 0)
			{
				Projectile::Update();
				return;
			}
			if (!TouchesTank(this))
			{
				if (!mSprite->Collides((int)mX, (int)mY, (int)mFrame, 0, mMirrored, aBoard->mApp->mSprites[IMAGE_SHIELD],
					(int)aBoard->mTankX, aBoard->mTankY + 2, (int)aBoard->mShieldFrame, 0, false, true))
				{
					Projectile::Update();
					return;
				}
			}
			mIsDetonating = true;
			mIsShootable = false;
			aBoard->mNukeFlash = 128.0;
			aBoard->AddExplosion(mX, mY, 200, 200, 0.0, 0.0, 0.05);
			aBoard->KillTank();
			return;
		}
	}
	else
	{
		mFlareTime = mFlareTime + 0.2;
		if (mFlareTime < 6.0)
			return;
		mBoard->KillTank();
	}
	Remove();
}

//
// FragBomb
//

// 0x448680
FragBomb::FragBomb(Board* theBoard, double x, double y, double theVelX, double theVelY, bool isMirrored) :
	Projectile(theBoard)
{
	mX = x;
	mY = y;
	mVelX = theVelX;
	mVelY = theVelY;
	mFrame = 0.0;
	mMirrored = isMirrored;
	mSprite = mBoard->mApp->mSprites[IMAGE_FRAGBOMB];
	mPoints = 15;
	mBoard->mApp->PlaySample(SOUND_BOMBFALL, (int)(x * 3.0));
}

// 0x448740
FragBomb::~FragBomb()
{
}

// 0x44a330: 4 bullets up and out of the ground, at 45, 75, 105 and 135 degrees (0.79 to 2.35 radians), 4 a frame
void FragBomb::ThrowFragments()
{
	double anAngle = 0.79;
	do
	{
		double aCos = cos(anAngle);
		AddFragmentBullet(mBoard, mX, (double)GROUND_Y, aCos * 4.0, sin(anAngle) * -4.0);
		anAngle = anAngle + 0.52;
	}
	while (anAngle < 2.37);
}

// 0x44b350: like a dumb bomb, but the fragments fly when it lands
void FragBomb::Update()
{
	FallBomb(this);
	if (IsPastSides(this))
	{
		Remove();
		return;
	}
	if (GROUND_Y < mY)
	{
		ThrowFragments();
		mBoard->AddExplosion(mX, mY, BLAST_SIZE, BLAST_SIZE, -1.0, 0.0, 0.0);
		HitGround();
		return;
	}
	if (mBoard->mTankDeadTime == 0 && TouchesTank(this))
	{
		mBoard->AddExplosion(mX, mY, BLAST_SIZE, BLAST_SIZE, -1.0, 0.0, 0.0);
		mBoard->KillTank();
		Remove();
		return;
	}
	Projectile::Update();
}

//
// HeadMissile
//

// 0x448750
HeadMissile::HeadMissile(Board* theBoard, double x, double y, bool isLowFrame) : Projectile(theBoard)
{
	mX = x;
	mY = y;
	int aRand = Rand();
	mHealth = 20.0;
	mPoints = 50;
	mFrame = isLowFrame ? 3.0 : 17.0;
	mTurnSpeed = (double)(aRand % 150) * 0.001 + 0.05;
	mIsLocked = false;
	mSprite = mBoard->mApp->mSprites[IMAGE_HEADMISSILE];
	mBoard->mApp->PlaySample(SOUND_MISSILE, (int)(x * 3.0));
	EmitLaunchSmoke(this);
}

// 0x448880
HeadMissile::~HeadMissile()
{
}

// 0x448890: it flies at 3 pixels an update along its heading (the frame), turning the frame by mTurnSpeed toward the
// tank's bearing until it has turned as far as it, then it flies straight
void HeadMissile::Update()
{
	double anAngle = (mFrame - HEADING_FRAME_CENTRE) * HEADING_DEGREES_PER_FRAME * DEGREES_TO_RADIANS;
	double aHeading = anAngle + HEADING_OFFSET;
	mVelX = cos(aHeading) * 3.0;
	mVelY = sin(aHeading) * 3.0;
	mX = mX - mVelX;
	mY = mVelY + mY;
	EmitTrail(this, mVelX * 5.0 + mX, mY - mVelY * 5.0, mVelX * 0.5, mVelY * -0.5);

	if (!mIsLocked)
	{
		double aBearing = atan2(mBoard->mTankX - mX, (double)mBoard->mTankY - mY);
		if (anAngle <= aBearing)
		{
			mFrame = mTurnSpeed + mFrame;
			if (aBearing <= mTurnSpeed * HEADING_DEGREES_PER_FRAME * DEGREES_TO_RADIANS + anAngle)
				mIsLocked = true;
		}
		else
		{
			mFrame = mFrame - mTurnSpeed;
			double aLowest = anAngle - mTurnSpeed * HEADING_DEGREES_PER_FRAME * DEGREES_TO_RADIANS;
			if (aLowest <= aBearing)
				mIsLocked = true;
		}
	}
	FinishUpdate(this, BLAST_SIZE);
}

//
// Hellfire
//

// 0x448b20: it starts 20 pixels to the side of x away from the tank, its frame (heading) turned that way
Hellfire::Hellfire(Board* theBoard, double x, double y, double theVelX, double theVelY, double theScale) :
	Projectile(theBoard)
{
	mY = y;
	mVelX = theVelX;
	mVelY = theVelY;
	mScale = theScale;
	int aRand = Rand();
	mPoints = 20;
	mTurnSpeed = ((double)(aRand % 150) * 0.001 + 0.1) * mScale;
	if (mBoard->mTankX <= x)
	{
		mFrame = 0.0;
		mX = x - 20.0;
	}
	else
	{
		mFrame = 20.0;
		mX = x + 20.0;
	}
	mIsLocked = false;
	mSprite = mBoard->mApp->mSprites[IMAGE_HELLFIRE];
	mBoard->mApp->PlaySample(SOUND_MISSILE, (int)(x * 3.0));
	EmitLaunchSmoke(this);
}

// 0x448c80
Hellfire::~Hellfire()
{
}

// 0x448c90: as the head missile, at 4 times its scale in pixels an update
void Hellfire::Update()
{
	double anAngle = (mFrame - HEADING_FRAME_CENTRE) * HEADING_DEGREES_PER_FRAME * DEGREES_TO_RADIANS;
	double aSpeed = mScale * 4.0;
	double aCos = cos(anAngle + HEADING_OFFSET);
	mVelX = aCos * aSpeed;
	double aSin = sin(anAngle + HEADING_OFFSET);
	mVelY = aSin * aSpeed;
	mX = mX - mVelX;
	mY = mVelY + mY;
	EmitTrail(this, mVelX * 5.0 + mX, mY - mVelY * 5.0, mVelX * 0.5, mVelY * -0.5);

	if (!mIsLocked)
	{
		double aBearing = atan2(mBoard->mTankX - mX, (double)mBoard->mTankY - mY);
		if (anAngle <= aBearing)
		{
			mFrame = mTurnSpeed + mFrame;
			if (aBearing <= mTurnSpeed * HEADING_DEGREES_PER_FRAME * DEGREES_TO_RADIANS + anAngle)
				mIsLocked = true;
		}
		else
		{
			mFrame = mFrame - mTurnSpeed;
			double aLowest = anAngle - mTurnSpeed * HEADING_DEGREES_PER_FRAME * DEGREES_TO_RADIANS;
			if (aLowest <= aBearing)
				mIsLocked = true;
		}
	}
	FinishUpdate(this, BLAST_SIZE);
}

//
// IronBomb
//

// 0x448f30
IronBomb::IronBomb(Board* theBoard, double x, double y, double theVelX, double theVelY, bool isMirrored) :
	Projectile(theBoard)
{
	mX = x;
	mY = y;
	mVelX = theVelX;
	mVelY = theVelY;
	mFrame = 0.0;
	mHealth = 10.0;
	mMirrored = isMirrored;
	mSprite = mBoard->mApp->mSprites[IMAGE_IRONBOMB];
	mPoints = 20;
	mBoard->mApp->PlaySample(SOUND_BOMBFALL, (int)(x * 3.0));
}

// 0x449000
IronBomb::~IronBomb()
{
}

// 0x449010
void IronBomb::Update()
{
	FallBomb(this);
	FinishUpdate(this, BLAST_SIZE);
}

//
// LGB
//

// 0x4491c0
LGB::LGB(Board* theBoard, double x, double y, double theVelX, double theVelY, bool isMirrored) : Projectile(theBoard)
{
	mX = x;
	mY = y;
	mVelX = theVelX;
	mVelY = theVelY;
	mFrame = 0.0;
	mPoints = 15;
	if (!isMirrored)
	{
		mAngle = 90.0;
		mFrame = 20.0;
	}
	else
	{
		mAngle = -90.0;
		mFrame = 0.0;
	}
	mMirrored = false;
	mSprite = mBoard->mApp->mSprites[IMAGE_LGB];
	mBoard->mApp->PlaySample(SOUND_BOMBFALL, (int)(x * 3.0));
}

// 0x4492a0
LGB::~LGB()
{
}

// 0x4492b0: it falls slowing sideways; its heading keeps within 45 degrees of straight down, and inside that it
// steers toward the tank's x (2 degrees an update, with 20 pixels of slack), flying 2 pixels an update along it
void LGB::Update()
{
	mX = mX + mVelX;
	mY = mVelY + mY;
	mVelX = mVelX * 0.95;
	if (mVelY < 1.0)
		mVelY = mVelY + 0.05;
	if (45.0 <= mAngle)
		mAngle = mAngle - 2.0;
	if (mAngle <= -45.0)
		mAngle = mAngle + 2.0;

	if (abs((int)mAngle) < 45)
	{
		if (mX < mBoard->mTankX - 20.0)
			mAngle = mAngle + 2.0;
		if (mBoard->mTankX + 20.0 < mX)
			mAngle = mAngle - 2.0;
		double aSteer = (mAngle - 90.0) * DEGREES_TO_RADIANS;
		mX = cos(aSteer) + cos(aSteer) + mX;
		mY = mY - sin(aSteer);
	}
	mFrame = (mAngle + 90.0) * (1.0 / 9.0);
	FinishUpdate(this, BLAST_SIZE);
}

//
// Meteor
//

// 0x449510: it starts above the field, 500 to 1500 pixels from a point on the ground within 320 of the middle, along a
// direction from 68 to 90 degrees, and falls toward that point at 3 to 4 pixels an update
Meteor::Meteor(Board* theBoard) : Projectile(theBoard)
{
	int aGroundRand = Rand();
	double anAngle = ((double)(Rand() % 800) * 0.001 + 1.5701) - 0.4;
	double aDistance = (double)(Rand() % 1000) + 500.0;
	int aSpeedRand = Rand();
	mIsShootable = false;
	double aSpeed = (double)(aSpeedRand % 100) * 0.01 + 3.0;
	double aCos = cos(anAngle);
	mX = aDistance * aCos + (double)(aGroundRand % 640 - FIELD_EDGE_X);
	double aSin = sin(anAngle);
	mY = (double)GROUND_Y - aSin * aDistance;
	mVelX = -(aCos * aSpeed);
	mVelY = aSin * aSpeed;
	mSprite = mBoard->mApp->mSprites[IMAGE_METEORITE];
}

// 0x449620
Meteor::~Meteor()
{
}

// 0x449630: a sound as it comes onto the screen, and a puff of its own picture every third update
void Meteor::Update()
{
	mX = mVelX + mX;
	mY = mVelY + mY;
	if (0.0 <= mY && mY - mVelY < 0.0)
		mBoard->mApp->PlaySample(SOUND_METEOR, (int)(mX * 3.0));
	if (mBoard->mApp->mUpdateCount % 3 == 0)
		mBoard->mParticles.Emit(mBoard->mApp->mSprites[IMAGE_METEORITE], mX, mY, 0.0, 0.0, 1, 0.0, 0.0, 0.0, 60, true, -1);
	FinishUpdate(this, BLAST_SIZE);
}

//
// Rpg
//

// 0x449930
Rpg::Rpg(Board* theBoard, double x, double y, bool isMirrored, double theFrameStep) : Projectile(theBoard)
{
	mX = x;
	mY = y;
	mFrameStep = theFrameStep;
	mMirrored = isMirrored;
	mPoints = 20;
	mSprite = mBoard->mApp->mSprites[IMAGE_RPG];
	mBoard->mApp->PlaySample(SOUND_MISSILE, (int)(x * 3.0));
	EmitLaunchSmoke(this);
}

// 0x449a30
Rpg::~Rpg()
{
}

// 0x449a40: it flies at 3 pixels an update, its frame (0..9) its heading from 30 degrees down 66 degrees
void Rpg::Update()
{
	mFrame = mFrameStep + mFrame;
	if (9.0 < mFrame)
		mFrame = 9.0;
	double anAngle = (30.0 - mFrame * 6.6) * DEGREES_TO_RADIANS;
	double aCos = cos(anAngle);
	mVelX = aCos * (mMirrored ? -3.0 : 3.0);
	double aSin = sin(anAngle) * -3.0;
	mVelY = aSin;
	mX = mVelX + mX;
	mY = aSin + mY;
	EmitTrail(this, mX - mVelX * 5.0, mY - aSin * 5.0, mVelX * 0.5, aSin * 0.5);
	FinishUpdate(this, BLAST_SIZE);
}

//
// BombFragment
//

// 0x44b950
BombFragment::BombFragment(Board* theBoard, double x, double y, int theKind, int theDrift) : Projectile(theBoard)
{
	mX = x;
	mY = y;
	mDrift = theDrift;
	mVelX = 0.0;
	mKind = theKind;
	mSwayAccel = (double)theDrift * 0.1;
	mIsShootable = false;
	mPoints = 500;
	mHasLanded = false;
	mSprite = mBoard->mApp->mSprites[IMAGE_BOMBFRAG];
}

// 0x44b870
BombFragment::~BombFragment()
{
}

// 0x44a990: the points of its trail above the ground, added
void BombFragment::Draw(Graphics* g)
{
	g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
	for (unsigned int i = 0; i < mTrailX.size(); i++)
	{
		if (mTrailY[i] < GROUND_Y + 12)
			mBoard->mApp->mSprites[IMAGE_BOMBFRAG]->Draw(g, mTrailX[i], mTrailY[i], true, 0, 0, false);
	}
	g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
}

// 0x44ba10
void BombFragment::Update()
{
	if (mKind == KIND_STRAIGHT)
	{
		mY = mY + 3.5;
	}
	else if (mKind == KIND_DRIFT)
	{
		int aShift = mDrift * 3;
		mX = (double)aShift + mX;
		mY = mY + 3.0;
	}
	else if (mKind == KIND_SWAY)
	{
		mX = mVelX + mX;
		mVelX = mVelX + mSwayAccel;
		if (mVelX < -5.0)
			mSwayAccel = 0.1;
		if (5.0 < mVelX)
			mSwayAccel = -0.1;
		mY = mY + 2.0;
	}

	if ((double)GROUND_Y < mY && !mHasLanded)
	{
		mHasLanded = true;
		GroundFlash* aFlash = new GroundFlash();
		aFlash->mX = mX;
		aFlash->mAlpha = GROUND_FLASH_ALPHA;
		aFlash->mType = 0;
		mBoard->mGroundFlashes.Append(aFlash);
	}

	// it goes once the oldest point of its path is below the ground
	if (mTrailY.empty() || mTrailY[0] <= GROUND_Y)
	{
		mTrailX.push_back((int)mX);
		mTrailY.push_back((int)mY);
		if (mTrailX.size() > TRAIL_LENGTH)
		{
			mTrailX.erase(mTrailX.begin());
			mTrailY.erase(mTrailY.begin());
		}
		if (mBoard->mTankDeadTime == 0)
		{
			for (unsigned int i = 0; i < mTrailX.size(); i++)
			{
				if (mSprite->Collides(mTrailX[i], mTrailY[i], 0, 0, false, mBoard->mApp->mSprites[IMAGE_TANK],
					(int)mBoard->mTankX, mBoard->mTankY, (int)mBoard->mTreadFrame, 0, false, true))
					mBoard->KillTank();
			}
		}
	}
	else
	{
		Remove();
	}
}

//
// BigMissile
//

// 0x431d20: it starts on the ground at x and climbs out of sight (in the background, out of reach); its health is
// 7 a level (to 14 levels), at most 40
BigMissile::BigMissile(Board* theBoard, double x) : Projectile(theBoard)
{
	mX = x;
	mY = (double)GROUND_Y;
	mRising = 1.0;
	mFrame = 0.0;
	int aLevel;
	if (!mBoard->mIsSurvival)
		aLevel = mBoard->mApp->mLevel;
	else
		aLevel = mBoard->mLevelTime / 12000;
	if (aLevel > 14)
		aLevel = 14;
	mHealth = (double)((aLevel + 1) * HEALTH_PER_LEVEL);
	if (mHealth > (double)MAX_HEALTH)
		mHealth = (double)MAX_HEALTH;
	mSprite = mBoard->mApp->mSprites[IMAGE_BIGMISSILE];
	mIsShootable = false;
	mIsInBackground = true;
	mTankDied = false;
	mPoints = 500;
	mBoard->mApp->PlaySample(SOUND_AIRRAID, 0);
}

// 0x431e20
BigMissile::~BigMissile()
{
}

// 0x432030: its row is whether it climbs (1) or falls (0)
void BigMissile::Draw(Graphics* g)
{
	if (mHitFlash != 0)
	{
		g->SetColorizeImages(true);
		g->SetColor(Color(255, 255 - mHitFlash, 255 - mHitFlash, 255));
	}
	mSprite->Draw(g, (int)mX, (int)mY, true, (int)mFrame, (int)mRising, false);
	g->SetColorizeImages(false);
}

// 0x431e30: it climbs 1.5 an update until it is off the top, then comes down at 3 and can be shot (it is gone if the
// tank died meanwhile); falling it bursts on the ground, or on the tank
void BigMissile::Update()
{
	if (mBoard->mTankDeadTime != 0)
		mTankDied = true;
	if (mRising == 1.0)
		mY = mY - 1.5;
	else
		mY = mY + 3.0;
	if (mY < (double)-GetCelHeight() && mRising == 1.0)
	{
		mRising = 0.0;
		mIsShootable = true;
		mIsInBackground = false;
		if (mTankDied)
		{
			Remove();
			return;
		}
	}

	mFrame = mFrame + 0.15;
	if (10.0 <= mFrame)
		mFrame = mFrame - 10.0;
	if ((double)GROUND_Y < mY)
	{
		mBoard->AddExplosion(mX, mY, mSprite->mCelWidth + 40, mSprite->mCelHeight + 40, -1.0, 0.0, 0.0);
		HitGround();
		return;
	}
	if (mBoard->mTankDeadTime == 0 && mRising == 0.0 && TouchesTank(this))
	{
		mBoard->AddExplosion(mX, mY, mSprite->mCelWidth + 40, mSprite->mCelHeight + 40, -1.0, 0.0, 0.0);
		mBoard->KillTank();
		Remove();
		return;
	}
	if (mRising != 0.0)
	{
		mHitFlash -= 24;
		if (mHitFlash < 0)
			mHitFlash = 0;
		return;
	}
	Projectile::Update();
}
