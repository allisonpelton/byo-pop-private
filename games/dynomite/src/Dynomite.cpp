#include "Dynomite.h"
#include "Effects.h"
#include "Egg.h"
#include "EggGrid.h"
#include "FossilizedApp.h"
#include "MusicInterface.h"
#include "PortCompat.h"
#include "SideBar.h"
#include "SoundInstance.h"
#include "WidgetManager.h"
#include <cstdio>
#include <cstring>

using namespace Sexy;

int Sexy::gBoardGlobal5304a0 = 0;

// 0x404030
Dynomite::Dynomite()
{
	gBoardGlobal5304a0 = 0;
	mInitialized = false;
	mFieldB0 = false;
	mField74 = 0x160;
	mField80 = false;
	mApp = NULL;
	mField104 = 0;
	mField108 = 0;
	mPowerUpAngle = 0;
	mField130 = false;
	mField131 = false;
	mField13c = 0;
	mField138 = 0;
	mPowerUps.Clear();
	// TODO: +0x144 = 0
	mField148 = true;
	mChain = -1;
	mPauseCount = 0;
	// TODO: +0x154 (the dynamite) = 0
	mField134 = 0x15e;
	mField132 = false;
	mField1c8 = -0x1e0;
	mField1cc = 0;
	// TODO: +0x1d8 = 0
	mBabies = 0;
	mField1d0 = 0;
	// TODO: +0x204 (a widget) = 0
	mField149 = true;
	mField158 = 0;
	mBonusBits = 0;
	mField15c = -1;
	mField160 = -1;
	mField16c = false;
	mRank = 0;
	mRankChanged = false;
	mField100 = false;
	mSong = 0;
	mField23c = 0;
	mField244 = false;
	mStartTicks = GetTickCount();
	mFieldB4 = 0;
	// port: the fields the original constructor leaves to Init (slot 46)
	mBoard5c = NULL;
	mGrid = NULL;
	mDifficulty = 0;
	mGameType = 0;
}

// 0x4041e0
Dynomite::~Dynomite()
{
	if (mApp != NULL)
	{
		mApp->mMusicInterface->FadeOut(mSong, true, 0.004);
		mApp->mMusicInterface->FadeOut(4, true, 0.004);
	}
	// TODO: +0x204 (removed from the widget manager and safe-deleted), +0x1d8 and +0x154 (the dynamite)
	if (mInitialized == true)
	{
		if (mApp->mFootfallRumbleInstance->IsPlaying() == true)
			mApp->mFootfallRumbleInstance->Stop();
		if (mApp->mGrindInstance->IsPlaying() == true)
			mApp->mGrindInstance->Stop();
		if (mApp->mTicktockInstance->IsPlaying() == true)
			mApp->mTicktockInstance->Stop();
		mFlyingEggs.ResetIter();
		FlyingEgg* anEgg = mFlyingEggs.Next();
		while (anEgg != NULL)
		{
			delete anEgg;
			anEgg = mFlyingEggs.Next();
		}
		mEffects.ResetIter();
		Effect* anEffect = mEffects.Next();
		while (anEffect != NULL)
		{
			delete anEffect;
			anEffect = mEffects.Next();
		}
		mPopups.ResetIter();
		anEffect = mPopups.Next();
		while (anEffect != NULL)
		{
			delete anEffect;
			anEffect = mPopups.Next();
		}
		mPowerUps.ResetIter();
		PowerUp* aPowerUp = mPowerUps.Next();
		while (aPowerUp != NULL)
		{
			delete aPowerUp;
			aPowerUp = mPowerUps.Next();
		}
		if (mGrid != NULL)
			delete mGrid;
		// TODO: +0x5c (the launcher), +0x60, +0x84 and +0x88
		mInitialized = false;
	}
	// TODO: +0x144
}

// 0x404fb0
void Dynomite::SetPaused(bool isPaused)
{
	if (isPaused == true)
		mPauseCount++;
	else
		mPauseCount--;
	if (mPauseCount < 0)
		mPauseCount = 0;
	SetActive(mPauseCount == 0);
}

// 0x4047e0 (TODO)
void Dynomite::SetActive(bool isActive)
{
}

// 0x4060c0: the next one 20 degrees on round the circle
void Dynomite::AddPowerUp(int theKind)
{
	PowerUp* aPowerUp = new PowerUp(this, theKind, mPowerUpAngle);
	mPowerUps.Add(aPowerUp);
	int anAngle = mPowerUpAngle + 0x14;
	if (0x167 < anAngle)
		anAngle = mPowerUpAngle - 0x154;
	if (anAngle < 0)
		anAngle = anAngle + 0x168;
	mPowerUpAngle = anAngle;
}

// 0x404f30
void Dynomite::GetPowerUpCenter(int* x, int* y)
{
}

// 0x404c30
void Dynomite::AddPoints(int x, int y, int thePoints, int theMultiplier, bool isChain)
{
	char aText[100];
	if (mApp->mInTutorial == true)
		return;
	if (0 < theMultiplier)
		thePoints = thePoints * theMultiplier;
	if ((1 < mChain) && (isChain == true))
	{
		int aBonus = 0;
		switch (mChain)
		{
		case 2:
			aBonus = 3;
			break;
		case 3:
			aBonus = 4;
			break;
		case 4:
			aBonus = 6;
			break;
		case 5:
			aBonus = 8;
			break;
		case 6:
			aBonus = 10;
			break;
		case 7:
			aBonus = 0xf;
			break;
		}
		thePoints = thePoints + aBonus;
	}
	if (mDifficulty == 1)
	{
		thePoints = thePoints / 5;
		if (thePoints < 1)
			thePoints = 1;
	}
	else if (mDifficulty == 3)
		thePoints = thePoints * 2;
	int aScore = thePoints * 10;
	sprintf(aText, "+%d", aScore);
	if ((mBonusBits & 2) != 0)
	{
		strcat(aText, "x2");
		aScore = aScore * 2;
	}
	if ((mBonusBits & 4) != 0)
	{
		strcat(aText, "x3");
		aScore = aScore * 3;
	}
	if ((mBonusBits & 8) != 0)
	{
		strcat(aText, "x4");
		aScore = aScore << 2;
	}
	ScorePopup* aPopup = new ScorePopup();
	aPopup->Init(x, y, aText, mApp);
	mPopups.Add(aPopup);
	mApp->mSideBar->mScore = mApp->mSideBar->mScore + aScore;
	int anOldRank = mRank;
	int* aRank = &mScoreRanks[1];
	int i = 2;
	int aNext;
	do
	{
		if (aRank[-1] < mApp->mSideBar->mScore)
			mRank = i - 2;
		if (aRank[0] < mApp->mSideBar->mScore)
			mRank = i - 1;
		if (aRank[1] < mApp->mSideBar->mScore)
			mRank = i;
		if (aRank[2] < mApp->mSideBar->mScore)
			mRank = i + 1;
		if (aRank[3] < mApp->mSideBar->mScore)
			mRank = i + 2;
		if (aRank[4] < mApp->mSideBar->mScore)
			mRank = i + 3;
		if (aRank[5] < mApp->mSideBar->mScore)
			mRank = i + 4;
		if (aRank[6] < mApp->mSideBar->mScore)
			mRank = i + 5;
		if (aRank[7] < mApp->mSideBar->mScore)
			mRank = i + 6;
		if (aRank[8] < mApp->mSideBar->mScore)
			mRank = i + 7;
		aNext = i + 8;
		aRank = aRank + 10;
		i = i + 10;
	} while (aNext < 0x14);
	if (mRank != anOldRank)
		mRankChanged = true;
}

// 0x405010 (TODO: the 0x1c-byte dynamite, 0x407e00, at +0x154)
void Dynomite::MakeDynamite()
{
}

// 0x4050d0
void Dynomite::EggAlone(int theColor)
{
}

// 0x4b1930
void Dynomite::EggLanded()
{
}

// 0x40ad90 (TODO)
void Dynomite::Hint()
{
}
