#include "NukeBlast.h"
#include "Board.h"
#include "HeavyWeaponApp.h"
#include "Graphics.h"
#include "Sprite.h"
#include "DDImage.h"

#include <math.h>

using namespace Sexy;

// The smoke's tint for each boss's level (HeavyWeaponApp::GetBossIndex; the original's table at 0x561a90): red, green
// and blue, each times the puff's brightness
static const double NUKE_SMOKE_TINTS[HeavyWeaponApp::BOSS_SURVIVAL + 1][3] =
{
	{ 0.73, 0.87, 1.0 },
	{ 0.85, 0.9, 1.0 },
	{ 1.0, 0.8, 0.85 },
	{ 0.85, 1.0, 0.95 },
	{ 0.7, 0.8, 1.0 },
	{ 0.2, 0.35, 0.5 },
	{ 0.85, 0.9, 1.0 },
	{ 0.85, 0.9, 1.0 },
	{ 1.0, 0.9, 0.8 },
	{ 0.8, 0.85, 0.9 },
	{ 0.8, 0.7, 0.65 }
};

// 0x4282e0
NukeBlast::NukeBlast(Board* theBoard)
{
	mBoard = theBoard;
	mX = 0.0;
	mApp = theBoard->mApp;
	mFade = 1024.0;

	// The stem: four puffs rising
	int aY = 128;
	do
	{
		NukeSmoke* aSmoke = new NukeSmoke;
		int aRand = Rand() % 32;
		aSmoke->mVelX = 0.0;
		aSmoke->mVelY = -1.5;
		aSmoke->mBrightness = 160;
		aSmoke->mX = (double)(aRand - 16);
		aSmoke->mY = (double)aY;
		aSmoke->mFrame = Rand() % 3;
		mSmoke.Append(aSmoke);
		aY += 64;
	}
	while (aY < 360);

	// The cap: two rings of puffs, the outer one with more
	double aRing = 2.0;
	do
	{
		double aRadius = 5.0 * aRing;
		double aRadiusY = aRadius * 0.25;
		double anAngleStep = 3.0 - aRing;
		double anAngle = 3.141;
		do
		{
			NukeSmoke* aSmoke = new NukeSmoke;
			double aCos = cos(anAngle);
			aSmoke->mX = ((double)(Rand() % 16) + aCos * aRadius) - 8.0;
			double aSin = sin(anAngle);
			aSmoke->mY = (((double)(Rand() % 16) + aRadiusY * aSin) - 8.0) - 40.0;
			aSmoke->mVelX = aCos * aRing * 0.3333333333333333;
			aSmoke->mVelY = aSin * aRing * 0.25 - 1.0;
			aSmoke->mBrightness = (int)(223.0 - aSin * 32.0);
			aSmoke->mFrame = Rand() % 3;
			mSmoke.Append(aSmoke);
			anAngle = anAngleStep + anAngle;
		}
		while (anAngle < 9.423);
		aRing = aRing - 1.0;
	}
	while (aRing > 0.0);

	// The cap's base: two wider rings, a fire behind each puff
	aRing = 2.0;
	do
	{
		double aRadius = 10.0 * aRing;
		double aRadiusY = aRadius * 0.25;
		double anAngleStep = 3.0 - aRing;
		double anAngle = 3.141;
		do
		{
			NukeSmoke* aSmoke = new NukeSmoke;
			double aCos = cos(anAngle);
			aSmoke->mX = ((double)(Rand() % 16) + aCos * aRadius) - 8.0;
			double aSin = sin(anAngle);
			aSmoke->mY = ((double)(Rand() % 16) + aRadiusY * aSin) - 8.0;
			aSmoke->mVelX = aCos * aRing * 0.25;
			aSmoke->mVelY = aSin * aRing * 0.14285714285714285 - 0.3;
			aSmoke->mBrightness = (int)(223.0 - aSin * 32.0);
			aSmoke->mFrame = Rand() % 3;
			mSmoke.Append(aSmoke);

			NukeFire* aFire = new NukeFire;
			aFire->mX = aSmoke->mX;
			aFire->mY = aSmoke->mY;
			aFire->mVelX = aSmoke->mVelX;
			aFire->mVelY = aSmoke->mVelY;
			aFire->mFrame = Rand() % 3;
			aFire->mHeat = (double)(Rand() % 128 + 383);
			mFires.Append(aFire);
			anAngle = anAngleStep + anAngle;
		}
		while (anAngle < 9.423);
		aRing = aRing - 1.0;
	}
	while (aRing > 0.0);

	mNode = theBoard->mNukeBlasts.Append(this);
}

// 0x428770
void NukeBlast::Draw(Graphics* g)
{
	g->SetColorizeImages(true);
	int anAlpha = (int)mFade;
	if (anAlpha > 255)
		anAlpha = 255;
	int aBoss = mApp->GetBossIndex(mApp->mLevel);
	if (mApp->mHelpScreen != NULL)
		aBoss = 0;

	for (mSmoke.mCurrent = mSmoke.mHead; mSmoke.mCurrent->mNext != NULL; )
	{
		mSmoke.mCurrent = mSmoke.mCurrent->mNext;
		NukeSmoke* aSmoke = mSmoke.Current();

		double aBrightness = (double)aSmoke->mBrightness;
		g->SetColor(Color((int)(aBrightness * NUKE_SMOKE_TINTS[aBoss][0]), (int)(aBrightness * NUKE_SMOKE_TINTS[aBoss][1]),
			(int)(aBrightness * NUKE_SMOKE_TINTS[aBoss][2]), anAlpha));
		Sprite* aSprite = mApp->mSprites[IMAGE_MUSHSMOKE];
		if (mApp->Is3DAccelerated())
		{
			g->DrawImageF(aSprite->mImage, (float)((mX + aSmoke->mX) - 48.0), (float)((aSmoke->mY + 240.0) - 48.0),
				Rect(aSmoke->mFrame * 96, 0, 96, 96));
		}
		else
		{
			aSprite->Draw(g, (int)(mX + aSmoke->mX), (int)(aSmoke->mY + 240.0), true, aSmoke->mFrame, 0, false);
		}
	}

	g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
	for (mFires.mCurrent = mFires.mHead; mFires.mCurrent->mNext != NULL; )
	{
		mFires.mCurrent = mFires.mCurrent->mNext;
		NukeFire* aFire = mFires.Current();
		if (aFire->mHeat == 0.0)
			continue;

		int aRed = (int)aFire->mHeat;
		int aGreen = (int)(aFire->mHeat * 0.5);
		int aBlue = (int)(aFire->mHeat * 0.25);
		if (aRed > 255)
			aRed = 255;
		if (aGreen > 255)
			aGreen = 255;
		if (aBlue > 255)
			aBlue = 255;
		g->SetColor(Color(aRed, aGreen, aBlue));
		Sprite* aSprite = mApp->mSprites[IMAGE_MUSHFIRE];
		if (mApp->Is3DAccelerated())
		{
			g->DrawImageF(aSprite->mImage, (float)((mX + aFire->mX) - 64.0), (float)((aFire->mY + 240.0) - 64.0),
				Rect(aFire->mFrame * 128, 0, 128, 128));
		}
		else
		{
			aSprite->Draw(g, (int)(mX + aFire->mX), (int)(aFire->mY + 240.0), true, aFire->mFrame, 0, false);
		}
	}
	g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
	g->SetColorizeImages(false);
}

// 0x428ac0
void NukeBlast::Update()
{
	mX = mX - mBoard->mScrollSpeed * 0.3;
	mFade = mFade - 1.0;
	if (mFade < 0.0)
		mFade = 0.0;
	if (mX < -520.0 || mFade == 0.0)
	{
		mBoard->mNukeBlasts.Remove(mNode);	// deletes this
		return;
	}

	for (mSmoke.mCurrent = mSmoke.mHead; mSmoke.mCurrent->mNext != NULL; )
	{
		mSmoke.mCurrent = mSmoke.mCurrent->mNext;
		NukeSmoke* aSmoke = mSmoke.Current();
		aSmoke->mX = aSmoke->mX + aSmoke->mVelX;
		aSmoke->mY = aSmoke->mY + aSmoke->mVelY;
		aSmoke->mVelX = aSmoke->mVelX * 0.99;
		aSmoke->mVelY = aSmoke->mVelY * 0.99;
	}

	for (mFires.mCurrent = mFires.mHead; mFires.mCurrent->mNext != NULL; )
	{
		mFires.mCurrent = mFires.mCurrent->mNext;
		NukeFire* aFire = mFires.Current();
		aFire->mX = aFire->mX + aFire->mVelX;
		aFire->mY = aFire->mVelY + aFire->mY;
		aFire->mVelX = aFire->mVelX * 0.99;
		aFire->mVelY = aFire->mVelY * 0.99;
		aFire->mHeat = aFire->mHeat - 0.75;
		if (aFire->mHeat < 0.0)
			aFire->mHeat = 0.0;
	}
}
