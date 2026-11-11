// The effects of the earth and air elements (see Effects.h).

#include "Effects.h"
#include "AtomicaApp.h"
#include "Atom.h"
#include "Board.h"
#include "Grid.h"
#include "Graphics.h"

#include <cmath>

using namespace Sexy;

// ---------------------------------------------------------------------------------------------------------
// EarthBolt

// 0x421680: a direction that is not still
EarthBolt::EarthBolt(AtomicaApp* theApp) :
	Effect(theApp),
	mDX(0),
	mDY(0),
	mDirTimer(0),
	mStepTimer(0)
{
	do
	{
		if (mDY != 0.0f)
			break;
		mDX = (float) mApp->mRand.Get(2, true);
		mDY = (float) mApp->mRand.Get(2, true);
	} while (mDX == 0.0f);
}

// 0x421770: a spark every update, and every fourth the bolt takes a step of 50 pixels, taking the atom of the cell it
// reaches; it ends when it leaves the field
bool EarthBolt::Update()
{
	mStepTimer--;
	if (mStepTimer <= 0)
	{
		mX = mDX * 50.0f + mX;
		mY = mDY * 50.0f + mY;
		Grid* aGrid = mApp->mBoard->mGrid;
		int aCell[2] = {0, 0};
		Grid::PixelToCell((int) (mX - (float) aGrid->mX), (int) (mY - (float) aGrid->mY), aCell);
		if ((aCell[0] >= 0) && (aCell[0] < Grid::COLS) && (aCell[1] >= 0) && (aCell[1] < Grid::ROWS) &&
			(aGrid->mCells[aCell[0]][aCell[1]] != NULL))
		{
			int aCount = aGrid->Collect(aCell[0], aCell[1], aCell[0], aCell[1]);
			if (mApp->mMode == 1)
				mApp->mBoard->mMeter.Hit(aCount * 5);
		}
		mStepTimer = 4;
		bool anInside = ((float) aGrid->mX <= mX) && (mX <= (float) (aGrid->mX + aGrid->mWidth)) && ((float) aGrid->mY <= mY) &&
			(mY <= (float) (aGrid->mY + aGrid->mHeight));
		if (!anInside)
			return false;
		mDirTimer--;
		if (mDirTimer < 1)
		{
			mDirTimer = 0;
			if (mApp->mRand.Get(2, false) == 0)
			{
				mDX = 0;
				mDY = 0;
				do
				{
					if (mDY != 0.0f)
						break;
					mDX = (float) mApp->mRand.Get(2, true);
					mDY = (float) mApp->mRand.Get(2, true);
				} while (mDX == 0.0f);
			}
		}
	}
	// a spark that flies off
	Particle* aSpark = new Particle(mApp, mApp->mMiniSpark);
	aSpark->mX = mX;
	aSpark->mY = mY;
	int anAngle = mApp->mRand.Get(0x168, false);
	float aSpeed = (float) (mApp->mRand.Get(10, false) + 1) * 0.05f;
	aSpark->mVX = aSpeed * SinDeg(anAngle);
	aSpark->mVY = aSpeed * NegCosDeg(anAngle);
	mApp->mBoard->mEffects.Add(aSpark);
	return true;
}

void EarthBolt::Draw(Graphics* g)
{
	(void) g;
}

// ---------------------------------------------------------------------------------------------------------
// Vortex

// 0x431300
Vortex::Vortex(AtomicaApp* theApp, int theX, int theY) :
	Effect(theApp),
	mLife(2000),
	mAngleSpeed(0),
	mAngleSpeedTarget(0),
	mTimer(0),
	mFirst(NULL),
	mMoving(false)
{
	float aRadius = 10.0f;
	for (int i = 0; i < 0x32; i++)
	{
		for (int j = 0; j < 2; j++)
		{
			Mote* aParticle = new Mote();
			aParticle->mRadius = aRadius;
			aParticle->mAge = (float) i;
			aParticle->mAngle = (float) mApp->mRand.Get(0x168, false);
			aParticle->mSpeed = (float) (mApp->mRand.Get(5, false) + 5);
			mParticles.Add(aParticle);
			if ((i == 0) && (j == 0))
				mFirst = aParticle;
		}
		aRadius = aRadius * 1.0085f;
	}
	mX = (float) theX;
	mY = (float) theY;
	Grid* aGrid = mApp->mBoard->mGrid;
	mCenterX = aGrid->mWidth / 2 + aGrid->mX;
	mCenterY = aGrid->mHeight / 2 + aGrid->mY;
	int aDX = mCenterX - theX;
	int aDY = mCenterY - theY;
	mDistance = sqrtf((float) (aDX * aDX + aDY * aDY));
	mDistanceTarget = mDistance;
	mAngle = (float) CompassAngle(mCenterX, mCenterY, theX, theY);
	mX = SinDeg((int) mAngle) * mDistance + (float) mCenterX;
	mY = (float) mCenterY + NegCosDeg((int) mAngle) * mDistance;
}

Vortex::~Vortex()
{
	mParticles.Reset();
	Mote* aParticle = mParticles.Next();
	while (aParticle != NULL)
	{
		delete aParticle;
		aParticle = mParticles.Next();
	}
	mParticles.Clear();
}

// 0x431510: the particles spiral out and are born again at the middle until the vortex's time is up; it wanders (new
// distance and speed from time to time) and the atom under it flies away
bool Vortex::Update()
{
	if (mParticles.Size() == 0)
		return false;
	mLife--;
	if (mLife < 0)
		mLife = 0;
	if (mApp->mBoard->mMeter.mFull && (mApp->mMode == 0))
		mLife = 0;
	mParticles.Reset();
	Mote* aParticle = mParticles.Next();
	while (aParticle != NULL)
	{
		int anAngle = (int) (aParticle->mAngle + aParticle->mSpeed);
		if (anAngle > 0x167)
			anAngle -= 0x168;
		if (anAngle < 0)
			anAngle += 0x168;
		aParticle->mAngle = (float) anAngle;
		aParticle->mAge += 0.25f;
		aParticle->mRadius *= 1.0085f;
		if (aParticle->mAge >= 50.0f)
		{
			if (mLife < 1)
			{
				mParticles.Remove(aParticle);
				delete aParticle;
			}
			else
			{
				aParticle->mRadius = 10.0f;
				aParticle->mAge = 0;
				aParticle->mAngle = (float) mApp->mRand.Get(0x168, false);
				aParticle->mSpeed = (float) (mApp->mRand.Get(2, false) + 1);
				if (aParticle == mFirst)
					mMoving = true;
			}
		}
		aParticle = mParticles.Next();
	}
	if (mMoving)
	{
		mTimer--;
		if (mTimer < 1)
		{
			mAngleSpeedTarget = (mApp->mRand.Get(2, false) == 0 ? 1.0f : -1.0f) * (float) (mApp->mRand.Get(7, false) + 1) * 0.02f;
			mDistanceTarget = (float) mApp->mRand.Get(mApp->mBoard->mGrid->mHeight / 2, false);
			mTimer = mApp->mRand.Get(500, false) + 100;
		}
		mAngle += mAngleSpeed;
		if (mAngleSpeedTarget < mAngleSpeed)
			mAngleSpeed -= 0.0005f;
		if (mAngleSpeed < mAngleSpeedTarget)
			mAngleSpeed += 0.0005f;
		if (mDistanceTarget < mDistance)
			mDistance -= 0.1f;
		if (mDistance < mDistanceTarget)
			mDistance += 0.1f;
		int aWhole = (int) mAngle;
		mX = sinf((float) aWhole * 0.017453292f) * mDistance + (float) mCenterX;
		mY = (float) mCenterY - cosf((float) aWhole * 0.017453292f) * mDistance;
	}
	if (0 < mLife)
	{
		Grid* aGrid = mApp->mBoard->mGrid;
		int aCell[2] = {0, 0};
		Grid::PixelToCell((int) (mX - (float) aGrid->mX), (int) (mY - (float) aGrid->mY), aCell);
		if ((aCell[0] > -1) && (aCell[1] > -1) && (aCell[0] < 9) && (aCell[1] < 8))
		{
			Atom* anAtom = aGrid->mCells[aCell[0]][aCell[1]];
			if ((anAtom != NULL) && !anAtom->mPre)
			{
				anAtom->FlyAway(mApp->mBoard->LastTarget());
				mApp->mSndWhoosh->Play();
				if (mApp->mMode == 1)
					mApp->mBoard->mMeter.Hit(10);
			}
		}
	}
	return true;
}

// 0x431850: three copies of every particle, each fainter and a little behind the one before
void Vortex::Draw(Graphics* g)
{
	g->SetColorizeImages(true);
	mParticles.Reset();
	Mote* aParticle = mParticles.Next();
	while (aParticle != NULL)
	{
		int anAlpha = 0xff;
		int anAngle = (int) aParticle->mAngle;
		for (int i = 0; i < 3; i++)
		{
			g->SetColor(Color(0xff, 0xff, 0xff, anAlpha));
			Sprite* aDot = mApp->mElectron;
			float aX = SinDeg(anAngle) * aParticle->mRadius + mX;
			float aY = NegCosDeg(anAngle) * aParticle->mRadius + mY;
			aDot->Draw(g, (int) aX - (aDot->mWidth >> 1), (int) aY - (aDot->mHeight >> 1));
			anAlpha -= 0x55;
			anAngle -= 8;
			if (anAngle > 0x167)
				anAngle -= 0x170;
			if (anAngle < 0)
				anAngle += 0x168;
		}
		aParticle = mParticles.Next();
	}
	g->SetColorizeImages(false);
}
