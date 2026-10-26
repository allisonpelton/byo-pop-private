#include "DynomitePanic.h"
#include "Egg.h"
#include "EggGrid.h"
#include "FirePrompt.h"
#include "FossilizedApp.h"
#include "GameText.h"
#include "Graphics.h"
#include "HighScores.h"
#include "Kroney.h"
#include "Launcher.h"
#include "MenuTalker.h"
#include "MusicInterface.h"
#include "SideBar.h"
#include "SoundInstance.h"
#include "Sprite.h"
#include "Thunderbolt.h"
#include "Whirley.h"
#include <cstring>

using namespace Sexy;

// 0x5304d4: the walls move every other update
static int gPanicWallFrame = 0;

// 0x40ca40
DynomitePanic::DynomitePanic()
{
	// port: the original also builds and destroys a Dynomite on the stack here, which changes nothing
	mGameType = 1;
	mKick = 0.0f;
	strcpy(mHighScoreTitle, GTC(0x4f9cc4));
	mHighScores = new HighScores(1);
	mWhirleysHit = 0;
	mScoreRanks[0] = 0;
	mSong = 0;
	mScoreRanks[1] = 1500;
	mScoreRanks[2] = 2500;
	mScoreRanks[3] = 5000;
	mScoreRanks[4] = 8000;
	mScoreRanks[5] = 12000;
	mScoreRanks[6] = 16000;
	mScoreRanks[7] = 20000;
	mScoreRanks[8] = 25000;
	mScoreRanks[9] = 30000;
	mScoreRanks[10] = 37000;
	mScoreRanks[11] = 50000;
	mScoreRanks[12] = 65000;
	mScoreRanks[13] = 70000;
	mScoreRanks[14] = 85000;
	mScoreRanks[15] = 100000;
	mScoreRanks[16] = 120000;
	mScoreRanks[17] = 145000;
	mScoreRanks[18] = 170000;
	mScoreRanks[19] = 200000;
	mField23c = 0x47;
	// port: the original leaves these to Init
	mKroney = NULL;
	mCeilingSpeed = NULL;
	mWhirleyLevel = 0;
}

// 0x40cbf0
DynomitePanic::~DynomitePanic()
{
	// port: the original runs the board's destructor a second time after the list's; once is enough here
	if (mKroney != NULL)
		delete mKroney;
	if (mCeilingSpeed != NULL)
		delete mCeilingSpeed;
	mWhirleys.ResetIter();
	Whirley* aWhirley = mWhirleys.Next();
	while (aWhirley != NULL)
	{
		delete aWhirley;
		aWhirley = mWhirleys.Next();
	}
}

// 0x40d050
void DynomitePanic::GetPowerUpCenter(int* x, int* y)
{
	*x = mField70 - 0xc + mField6c;
	*y = (int) ((double) mKroney->mY - 4.0);
}

// 0x40d080
void DynomitePanic::Blasted()
{
	mKick = 7.0f;
	mCeilingSpeed->mSpeed = mCeilingSpeed->mMinSpeed;
}

// 0x40d0a0
void DynomitePanic::DrawScenery()
{
	Dynomite::DrawScenery();
	mKroney->Draw();
}

// 0x40cca0
void DynomitePanic::DrawMode()
{
	mWhirleys.ResetIter();
	Whirley* aWhirley = mWhirleys.Next();
	while (aWhirley != NULL)
	{
		aWhirley->Draw();
		aWhirley = mWhirleys.Next();
	}
}

// 0x40cce0
Egg* DynomitePanic::NextEgg()
{
	int aColor = mApp->mPicker.Random(mGrid->mMaxColor, false) + 1;
	if ((((4 - mGrid->mMaxColor) * 0x10 + mField74) <= mGrid->mBottom) &&
		(mApp->mRand.NextNoAssert() % 3 != 1))
	{
		unsigned char aPresent[20];
		memset(aPresent, 0, sizeof(aPresent));
		int aFirstRow = (int) (mGrid->mRows - 1.0f);
		if (aFirstRow < 0)
			aFirstRow = 0;
		int aCol = 0;
		int i = 0xf;
		do
		{
			int aRow = aFirstRow;
			if ((float) aFirstRow <= mGrid->mRows)
			{
				do
				{
					unsigned int aBits = mGrid->mCells[aCol][aRow].mBits & 0xff;
					if ((aBits != 0) && (aBits < 0x12))
						aPresent[aBits] = 1;
					aRow++;
				} while ((float) aRow <= mGrid->mRows);
			}
			aCol++;
			i--;
		} while (i != 0);
		int aTries = 0;
		aColor = 0;
		aPresent[0] = 0;
		while (true)
		{
			if (aColor == 0)
			{
				do
				{
					aColor = mApp->mPicker.Random(0x14, false);
					if (aPresent[aColor] == 0)
						aColor = 0;
					if (aColor == mField15c)
						aColor = 0;
					aTries++;
				} while ((aTries <= 0x32) && (aColor == 0));
			}
			if (aTries <= 0x32)
				break;
			// past 50 tries the colour fired last is allowed once more; then any colour
			if (mField15c == -1)
			{
				aColor = mApp->mPicker.Random(mGrid->mMaxColor, false) + 1;
				break;
			}
			mField15c = -1;
		}
		while (aColor == 0)
			aColor = mApp->mRand.NextNoAssert() % (unsigned int) mGrid->mMaxColor + 1;
	}
	Egg* anEgg = new Egg();
	anEgg->Init(mApp, this, aColor);
	if (mField108 != 0)
	{
		if (mField108 == 0x14)
		{
			if ((0x118 < mGrid->mBottom) && (mApp->mPicker.Random(3, false) == 0))
				mField108 = 5;
			if ((mField108 == 0x14) && (0x154 < mGrid->mBottom))
				mField108 = 5;
			if (mField108 == 0x14)
			{
				if (mApp->mPicker.Random(5, false) == 2)
				{
					int aKind = mApp->mPicker.Random(3, false);
					if (aKind == 0)
						mField108 = 1;
					else if (aKind == 1)
						mField108 = 6;
					else if (aKind == 2)
						mField108 = 7;
				}
				if (mField108 == 0x14)
				{
					mField108 = mApp->mPicker.Random(3, false) + 2;
					if (mApp->mPicker.Random(2, false) == 0)
						mField108 = mField108 + 6;
				}
			}
		}
		anEgg->mKind = mField108;
		mField108 = 0;
	}
	return anEgg;
}

// 0x40d2a0
void DynomitePanic::Draw(Graphics* g)
{
	if ((mBonusBits & 0x70) == 0)
	{
		SetDrawOffset(Point(mField138, mField13c));
		mField138 = mField138 / 2;
		mField13c = mField13c / 2;
	}
	else
	{
		int aShakeX = mApp->mRand.NextNoAssert() % 9 - 4;
		int aShakeY = mApp->mRand.NextNoAssert() % 9 - 4;
		SetDrawOffset(Point(aShakeX, aShakeY));
	}
	Dynomite::Draw(g);
	if ((mField131 == true) && (mField132 == false) && (mApp->mInTutorial == false))
	{
		Graphics* aGraphics = GetDrawGraphics();
		aGraphics->SetColorizeImages(true);
		aGraphics->SetColor(Color(0, 0, 0, mField140));
		Pic* aPic = mApp->mPicGameover;
		aPic->Draw((mField70 - (aPic->mWidth >> 1)) + 3, 0xf3 - (aPic->mHeight >> 1));
		aGraphics->SetColor(Color(0xff, 0xff, 0xff, mField140));
		aPic = mApp->mPicGameover;
		aPic->Draw(mField70 - (aPic->mWidth >> 1), 0xf0 - (aPic->mHeight >> 1));
		aGraphics->SetColorizeImages(false);
	}
}

// 0x40d420
void DynomitePanic::Init(FossilizedApp* theApp, int theDifficulty)
{
	Dynomite::Init(theApp, theDifficulty);
	mFirePrompt->mOwnLine1 = mApp->GetString("KEEP_THE_EGGS_FROM");
	mFirePrompt->mOwnLine2 = mApp->GetString("REACHING_THE_BOTTOM");
	if (theDifficulty == 1)
		mGrid->mMaxColor = 3;
	else if (theDifficulty == 2)
		mGrid->mMaxColor = 4;
	else if (theDifficulty == 3)
		mGrid->mMaxColor = 6;
	EggGrid* aGrid = mGrid;
	float aFactor;
	switch (aGrid->mMaxColor)
	{
	case 3:
		aFactor = 10.0f;
		break;
	case 4:
		aFactor = 5.0f;
		break;
	case 5:
		aFactor = 3.0f;
		break;
	case 6:
		aFactor = 2.0f;
		break;
	case 7:
		aFactor = 1.0f;
		break;
	default:
		aFactor = 0.5f;
		break;
	}
	aGrid->mBabyEggChance = (int) ((double) aGrid->mField46b4 * aFactor);
	mLauncher->Load(1);
	mKroney = new Kroney();
	mKroney->mBoard = this;
	mKroney->mApp = theApp;
	mKroney->Start(60.0f);
	mCeilingSpeed = new CeilingSpeed(mApp, this, theDifficulty);
	mGrid->AddRow(0);
	mGrid->AddRow(1);
	mGrid->AddRow(2);
	mGrid->AddRow(3);
	mGrid->AddRow(4);
	mGrid->AddRow(5);
	mGrid->mField46d0 = false;
	mGrid->ResetCeiling();
	mWhirleys.Clear();
	mWhirleyLevel = 0;
	if (theDifficulty != 3)
		mField108 = 1;
}

// 0x40d7b0
void DynomitePanic::Update()
{
	if (mApp->mActive == false)
		return;
	if (mField132 == true)
	{
		UpdateGameOver();
		return;
	}
	if (mPauseCount > 0)
		return;
	if (mThunderbolt != NULL)
	{
		if (mThunderbolt->Update() == false)
		{
			if (mThunderbolt != NULL)
				delete mThunderbolt;
			mThunderbolt = NULL;
		}
		MarkDirty();
		return;
	}
	Dynomite::Update();
	if ((mApp->mIsRegistered == false) && (mField134 == 1))
	{
		if (mHighScores->IsHighScore(mApp->mSideBar->mScore, (int) mStartTicks) == true)
			mApp->ShowRegisterDialog(mApp->GetString("CONGRATS_HS_DYNOMITE_UNREGISTERED").c_str());
		else
			mApp->ShowBuyDialog();
	}
	if (mField134 == 0)
	{
		SetPaused(true);
		if ((mHighScores->IsHighScore(mApp->mSideBar->mScore, (int) mStartTicks) == true) &&
			(mApp->mIsRegistered == true))
			mApp->ShowHighScoreDialog(mHighScores);
	}
	if (mWhirleyLevel < 1)
		mWhirleyLevel = 1;
	mCeilingSpeed->Update();
	if ((mBonusBits & 0x70) != 0)
	{
		if (mApp->mGrindInstance->IsPlaying() == false)
			mApp->mGrindInstance->Play(true, false);
	}
	else if (mApp->mGrindInstance->IsPlaying() == true)
		mApp->mGrindInstance->Stop();
	int aWallFrame = gPanicWallFrame;
	if (aWallFrame == 0)
	{
		if ((mBonusBits & 0x20) != 0)
		{
			mField68++;
			if (mField68 > 0x1b5)
				mField68 = 0x1b5;
			mField6c = mField68 >> 1;
		}
		if ((mBonusBits & 0x40) != 0)
		{
			mField68--;
			if (mField68 < 0xca)
				mField68 = 0xca;
			mField6c = mField68 >> 1;
		}
	}
	gPanicWallFrame = 1 - aWallFrame;
	if (((mBonusBits & 0x10) != 0) && (mGrid->mBottom > -0x2b))
	{
		mCeilingSpeed->mSpeed = 0.02f;
		mGrid->mCeiling = (float) ((double) mGrid->mCeiling - mCeilingSpeed->mSpeed);
		mGrid->mCeiling = (float) ((double) mGrid->mCeiling - 0.5);
	}
	mWhirleys.ResetIter();
	Whirley* aWhirley = mWhirleys.Next();
	while (aWhirley != NULL)
	{
		if (aWhirley->Update() == false)
		{
			mWhirleys.Remove(aWhirley);
			if (aWhirley != NULL)
				delete aWhirley;
		}
		aWhirley = mWhirleys.Next();
	}
	if (mKroney->Update() == true)
		mWhirleys.Add(new Whirley(mApp, this, mGrid, mWhirleyLevel, mGrid->mMaxColor + 1));
	if (mFieldB0 != true)
		return;
	if (mField130 == false)
	{
		EggGrid* aGrid = mGrid;
		if (aGrid->mField4691 == true)
		{
			aGrid->mField4691 = false;
			mApp->mSndAooga->Play();
		}
		aGrid = mGrid;
		if (aGrid->mPastLineFrames != 0)
			return;
		if (mKick != 0.0f)
		{
			aGrid->mCeiling = (float) ((double) aGrid->mCeiling - mKick);
			double aKick = (double) mKick * (double) 0.95f;
			mKick = (float) aKick;
			if (aKick < 1.0)
				mKick = 0.0f;
		}
		else if (mField100 == true)
			aGrid->mCeiling = (float) ((double) mCeilingSpeed->mSpeed + aGrid->mCeiling);
		aGrid = mGrid;
		if ((aGrid->mCeiling > -10.0f) && (mField100 == true))
		{
			aGrid->PushDown();
			aGrid->mCeiling = (float) ((double) aGrid->mCeiling - 66.0f);
		}
	}
	else if (mField131 == false)
	{
		// the eggs run down off the board, then "game over"
		mGrid->mCeiling = (float) ((double) mGrid->mCeiling + 32.0f);
		EggGrid* aGrid = mGrid;
		if (aGrid->mCeiling > 496.0f)
		{
			aGrid->mCeiling = 496.0f;
			mField131 = true;
			mField13c = 0x3c;
			mApp->mSndFootlands->Play();
			if (mApp->mInTutorial == false)
			{
				mApp->mMusicInterface->StopMusic(mSong);
				mApp->mMusicInterface->SetSongVolume(4, 1.0);
				mApp->mMusicInterface->PlayMusic(4, 0x23, false);
			}
			mField140 = 0;
			if (mApp->mInTutorial == false)
				mTalker = new MenuTalker(mApp, this, 2);
		}
	}
	else
	{
		mField140 = mField140 + 2;
		if (mField140 > 0xff)
			mField140 = 0xff;
	}
}
