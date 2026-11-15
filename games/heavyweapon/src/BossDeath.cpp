#include "BossDeath.h"
#include "Board.h"
#include "Enemy.h"
#include "HeavyWeaponApp.h"
#include "MusicInterface.h"
#include "Sprite.h"

using namespace Sexy;

// 0x41bf20
BossDeath::BossDeath(Board* theBoard)
{
	mBoard = theBoard;
	mApp = theBoard->mApp;
	mTimer = 20;
	mExplosions = 0;
	mApp->SaveUsers();
}

// 0x41bf50
BossDeath::~BossDeath()
{
	mBoard->mApp->mMusicInterface->FadeOut(SONG_ATOMIC_TANK_42, true, 0.004);
}

// 0x41bf70
void BossDeath::Update()
{
	mTimer++;
	if (mTimer <= 20)
		return;

	mTimer = Rand() % 10;
	mExplosions++;

	// A random point in the boss's cel that is on the boss itself (a bullet there would hit it)
	int aX;
	int aY;
	Enemy* aBoss;
	do
	{
		aBoss = mBoard->mBoss;
		int aWidth = aBoss->GetCelWidth();
		int anOffsetX = Rand() % aWidth - mBoard->mBoss->GetCelWidth() / 2;
		aX = (int)(anOffsetX + aBoss->mX);
		aBoss = mBoard->mBoss;
		int aHeight = aBoss->GetCelHeight();
		int anOffsetY = Rand() % aHeight - mBoard->mBoss->GetCelHeight() / 2;
		aY = (int)(anOffsetY + aBoss->mY);
	}
	while (!aBoss->GetSprite()->Collides((int)aBoss->mX, (int)aBoss->mY, 0, 0, aBoss->mMirrored,
		mBoard->mApp->mSprites[IMAGE_BULLETS], aX, aY, 0, 0, aBoss->mMirrored, true));

	mBoard->AddExplosion((double)aX, (double)aY, 160, 160, 0.0, 0.0, 0.0);
}
