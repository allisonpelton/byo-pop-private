#include "Projectile.h"
#include "Board.h"
#include "HeavyWeaponApp.h"
#include "Graphics.h"
#include "Sprite.h"

using namespace Sexy;

// 0x447760
Projectile::Projectile(Board* theBoard)
{
	mBoard = theBoard;
	mVelX = 0.0;
	mVelY = 0.0;
	mIsShootable = true;
	mIsDestructible = true;
	mFrame = 0.0;
	mMirrored = false;
	mHealth = 0.0;
	mHitFlash = 0;
	mIsInBackground = false;
	theBoard->mApp->mStats[HeavyWeaponApp::STAT_PROJECTILES_FIRED]++;
	mBoard->mProjectilesFired++;
	mNode = mBoard->mProjectiles.Append(this);
	mPoints = 10;
}

// 0x447810
Projectile::~Projectile()
{
}

// 0x4476b0
void Projectile::Draw(Graphics* g)
{
	if (mHitFlash != 0)
	{
		g->SetColorizeImages(true);
		g->SetColor(Color(255, 255 - mHitFlash, 255 - mHitFlash, 255));
	}
	mSprite->Draw(g, (int)mX, (int)mY, true, (int)mFrame, 0, mMirrored);
	g->SetColorizeImages(false);
}

// 0x447890: while the tank has a shield (or its last hit still glows) a projectile touching the shield bursts there
// and costs a shield level; the glow lasts 30 updates, during which more hits cost nothing
void Projectile::Update()
{
	mHitFlash -= 24;
	if (mHitFlash < 0)
		mHitFlash = 0;

	if (mBoard->mTankDeadTime != 0)
		return;
	HeavyWeaponApp* anApp = mBoard->mApp;
	if (anApp->mPowerUps[HeavyWeaponApp::POWERUP_SHIELD] == 0 && !(mBoard->mShieldHitGlow > 0.0))
		return;
	if (!mSprite->Collides((int)mX, (int)mY, (int)mFrame, 0, mMirrored, anApp->mSprites[IMAGE_SHIELD],
			(int)mBoard->mTankX, mBoard->mTankY + 2, (int)mBoard->mShieldFrame, 0, false, true))
		return;

	mBoard->AddExplosion(mX, mY, mSprite->mCelWidth + 40, mSprite->mCelHeight + 40, -1.0, 0.0, 0.0);
	if (mBoard->mShieldHitGlow <= 0.0)
	{
		mBoard->mApp->mPowerUps[HeavyWeaponApp::POWERUP_SHIELD]--;
		if (mBoard->mApp->mPowerUps[HeavyWeaponApp::POWERUP_SHIELD] == 0)
			mBoard->mApp->PlaySample(SOUND_SHIELDDOWN, (int)(mBoard->mTankX * 3.0));
		mBoard->mShieldHitGlow = 9.0;
		mBoard->mApp->PlaySample(SOUND_SPARKING, (int)(mBoard->mTankX * 3.0));
	}
	mBoard->mProjectiles.Remove(mNode);	// deletes this
}

// 0x447730
Sprite* Projectile::GetSprite()
{
	return mSprite;
}

// 0x447740
int Projectile::GetCelWidth()
{
	return mSprite->mCelWidth;
}

// 0x447750
int Projectile::GetCelHeight()
{
	return mSprite->mCelHeight;
}

// 0x447840
void Projectile::Remove()
{
	mBoard->mProjectiles.Remove(mNode);
}

// 0x447860
void Projectile::HitGround()
{
	mBoard->AddCrater((int)mX, 0);
	mBoard->mProjectiles.Remove(mNode);
}
