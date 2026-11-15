#include "Enemy.h"
#include "Board.h"
#include "HeavyWeaponApp.h"
#include "MegaLaserPart.h"
#include "Graphics.h"
#include "Sprite.h"

#include <stdlib.h>

using namespace Sexy;

// 0x431550
Enemy::Enemy(Board* theBoard)
{
	mHitFlash = 0;
	mMirrored = false;
	mFrame = 0.0;
	// port: the original leaves the place unset, and the robot reads its own before it sets it, to place its arms and
	// launchers (RobotBoss::RobotBoss); the port starts it at 0
	mX = 0.0;
	mY = 0.0;
	mVelX = 0.0;
	mVelY = 0.0;
	mBoard = theBoard;
	theBoard->mApp->mStats[HeavyWeaponApp::STAT_ENEMIES_SPAWNED]++;
	mBoard->mEnemiesSpawned++;
	mNode = mBoard->mEnemies.Append(this);
	mIsFriendly = false;
	mMaxHealth = 0.0;
	mSprite = NULL;
	mIsAlive = true;
	mApp = theBoard->mApp;
}

// 0x431600
Enemy::~Enemy()
{
}

// 0x431260
void Enemy::Draw(Graphics* g)
{
	if (mHitFlash != 0)
	{
		g->SetColorizeImages(true);
		g->SetColor(Color(255, 255 - mHitFlash, 255 - mHitFlash, 255));
	}
	mSprite->Draw(g, (int)mX, (int)mY, true, (int)mFrame, 0, mMirrored);
	g->SetColorizeImages(false);
}

// 0x4312e0
void Enemy::Update()
{
	mHitFlash -= 24;
	if (mHitFlash < 0)
		mHitFlash = 0;
}

// 0x471be0
void Enemy::UpdateF(float theFrac)
{
}

// 0x431330: counts the kill; one enemy in 20 drops a mega laser part, at most one every 500 updates, while the laser
// is not charged, over the field and not in the level's last 4000 updates
void Enemy::Killed()
{
	if (mIsFriendly)
	{
		mBoard->mApp->mStats[HeavyWeaponApp::STAT_FRIENDLIES_DESTROYED]++;
		mBoard->mFriendliesDestroyed++;
		return;
	}

	mBoard->mApp->mStats[HeavyWeaponApp::STAT_ENEMIES_DESTROYED]++;
	mBoard->mEnemiesDestroyed++;
	if (Rand() % 20 != 0)
		return;
	if (mBoard->mLevelTime >= mBoard->mLevelLength - 4000 && !mBoard->mIsSurvival)
		return;
	if (mY <= 280.0 && mX > -320.0 && mX < 320.0 && mBoard->mMegaLaserTime == 0 && mBoard->mMegaLaserPartDelay == 0)
	{
		new MegaLaserPart(mBoard, (int)mX, (int)mY);
		mBoard->mMegaLaserPartDelay = 500;
	}
}

// 0x431480: an explosion a quarter bigger than its cel
void Enemy::Explode()
{
	int aSize = GetCelWidth();
	if (GetCelHeight() > aSize)
		aSize = GetCelHeight();
	aSize = (int)((aSize + 20) * 1.25);
	mBoard->AddExplosion(mX, mY, aSize, aSize, mVelX, mVelY, 0.0);
}

// 0x4011a0
void Enemy::SilenceSounds()
{
}

// 0x4011a0
void Enemy::RestoreSounds()
{
}

// 0x431650: which side it comes in from. With a friendly unit out it comes from the side away from the first one,
// unless that one is near the middle; otherwise either side.
void Enemy::ChooseSide()
{
	if (mBoard->mFriendlyCount == 0)
	{
		mMirrored = Rand() % 2 != 0;
		return;
	}

	ObjectList<Enemy>& anEnemies = mBoard->mEnemies;
	ObjectList<Enemy>::Node* aWalk = anEnemies.mCurrent;
	anEnemies.mCurrent = anEnemies.mHead;
	Enemy* aFriendly;
	do
	{
		if (anEnemies.mCurrent->mNext == NULL)
		{
			anEnemies.mCurrent = aWalk;
			return;
		}
		anEnemies.mCurrent = anEnemies.mCurrent->mNext;
		aFriendly = anEnemies.Current();
	}
	while (!aFriendly->mIsFriendly);

	if (abs((int)aFriendly->mX) < 160)
		mMirrored = Rand() % 2 != 0;
	else if (aFriendly->mX < 0.0)
		mMirrored = true;
	else
		mMirrored = false;
	anEnemies.mCurrent = aWalk;
}

// 0x431300
Sprite* Enemy::GetSprite()
{
	return mSprite;
}

// 0x431310
int Enemy::GetCelWidth()
{
	return mSprite->mCelWidth;
}

// 0x431320
int Enemy::GetCelHeight()
{
	return mSprite->mCelHeight;
}

// 0x431630
void Enemy::Remove()
{
	mBoard->mEnemies.Remove(mNode);
}
