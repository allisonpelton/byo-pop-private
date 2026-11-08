#include "DynomiteTimeTrial.h"
#include "BinFile.h"
#include "Egg.h"
#include "EggGrid.h"
#include "FirePrompt.h"
#include "FossilizedApp.h"
#include "GameText.h"
#include "Graphics.h"
#include "HighScores.h"
#include "ImageFont.h"
#include "Launcher.h"
#include "MenuTalker.h"
#include "MusicInterface.h"
#include "SideBar.h"
#include "Sprite.h"
#include "Steggy.h"
#include <cstdio>
#include <cstring>

using namespace Sexy;

// 0x411ff0
DynomiteTimeTrial::DynomiteTimeTrial()
{
	// port: the original also builds and destroys a Dynomite on the stack here, which changes nothing
	mGameType = 3;
	strcpy(mHighScoreTitle, GTC(0x4fa0e4));
	mHighScores = new HighScores(3);
	mField149 = false;
	mField148 = false;
	mRowsCleared = 0;
	mEggsBroken = 0;
	mEggsDropped = 0;
	mSong = 0;
	mField74 = 0xfa;
	// the score is minus the time in hundredths: 12 minutes down to 2 and a half
	mScoreRanks[0] = -72000;
	mScoreRanks[1] = -69000;
	mScoreRanks[2] = -66000;
	mScoreRanks[3] = -63000;
	mScoreRanks[4] = -60000;
	mScoreRanks[5] = -57000;
	mScoreRanks[6] = -54000;
	mScoreRanks[7] = -51000;
	mScoreRanks[8] = -48000;
	mScoreRanks[9] = -45000;
	mScoreRanks[10] = -42000;
	mScoreRanks[11] = -39000;
	mScoreRanks[12] = -36000;
	mScoreRanks[13] = -33000;
	mScoreRanks[14] = -30000;
	mScoreRanks[15] = -27000;
	mScoreRanks[16] = -24000;
	mScoreRanks[17] = -21000;
	mScoreRanks[18] = -18000;
	mScoreRanks[19] = -15000;
	mField23c = 0x2b;
}

// 0x4121a0
DynomiteTimeTrial::~DynomiteTimeTrial()
{
}

// 0x4121b0
Egg* DynomiteTimeTrial::NextEgg()
{
	unsigned char aPresent[20];
	memset(aPresent, 0, sizeof(aPresent));
	EggGrid* aGrid = mGrid;
	int aFirstRow = (int) ((double) aGrid->mRows - 6.0);
	if (aFirstRow < 0)
		aFirstRow = 0;
	int aCol = 0;
	int i = 0xf;
	do
	{
		int aRow = aFirstRow;
		if ((float) aFirstRow <= aGrid->mRows)
		{
			do
			{
				unsigned int aBits = aGrid->mCells[aCol][aRow].mBits & 0xff;
				if (aBits != 0)
					aPresent[aBits] = 1;
				aRow++;
			} while ((float) aRow <= aGrid->mRows);
		}
		aCol++;
		i--;
	} while (i != 0);
	int aTries = 0;
	aPresent[0] = 0;
	int aColor;
	while (true)
	{
		aColor = mApp->mPicker.Random(0x14, false);
		if (aPresent[aColor] == 0)
			aColor = 0;
		aTries++;
		if (aTries > 0x32)
		{
			// past 50 tries, any colour
			do
				aColor = mApp->mPicker.Random(mGrid->mMaxColor, false) + 1;
			while (aColor == 0);
			break;
		}
		if (aColor != 0)
			break;
	}
	Egg* anEgg = new Egg();
	anEgg->Init(mApp, this, aColor);
	return anEgg;
}

// 0x412320
void DynomiteTimeTrial::Update()
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
	Dynomite::Update();
	if (mField134 == 0)
	{
		SetPaused(true);
		if (mHighScores->IsHighScore(mApp->mSideBar->mScore, (int) mStartTicks) == true)
			mApp->ShowHighScoreDialog(mHighScores);
	}
	EggGrid* aGrid = mGrid;
	if (aGrid->mCeiling < 30.0f)
		aGrid->mCeiling = (float) ((double) aGrid->mCeiling + 1.0f);
	aGrid = mGrid;
	if (mField74 < aGrid->mBottom)
	{
		aGrid->mCeiling = (float) ((double) aGrid->mCeiling - (double) ((aGrid->mBottom - mField74) / 3));
		mField104 = (int) aGrid->mCeiling;
	}
	if ((mFieldB0 == true) && (mField131 == false))
		mApp->mSideBar->mScore--;
	if (mGrid->mEggCount == 0)
	{
		if (mField131 == false)
		{
			// every egg is gone: "cleared", Inchy and the end's music
			mTalker = new MenuTalker(mApp, this, 2);
			mField131 = true;
			mField140 = 0;
			mApp->mMusicInterface->SetSongVolume(0, 1.0);
			mApp->mMusicInterface->PlayMusic(0, 0x27, false);
			int aRank = 0;
			do
			{
				if (mScoreRanks[aRank] < mApp->mSideBar->mScore)
					mRank = aRank;
				aRank++;
			} while (aRank < 0x14);
			return;
		}
		mField140 = mField140 + 2;
		if (mField140 > 0xff)
			mField140 = 0xff;
	}
}

// 0x412680
void DynomiteTimeTrial::AddPoints(int x, int y, int thePoints, int theMultiplier, bool isChain)
{
	mEggsBroken = mEggsBroken + thePoints;
	mEggsDropped = mEggsDropped + theMultiplier;
}

// 0x412750
void DynomiteTimeTrial::Draw(Graphics* g)
{
	Dynomite::Draw(g);
	if ((mField131 == true) && (mField132 == false))
	{
		Graphics* aGraphics = GetDrawGraphics();
		aGraphics->SetColorizeImages(true);
		aGraphics->SetColor(Color(0, 0, 0, mField140));
		Pic* aPic = mApp->mPicCleared;
		aPic->Draw((mField70 - (aPic->mWidth >> 1)) + 3, 0xdf - (aPic->mHeight >> 1));
		aGraphics->SetColor(Color(0xff, 0xff, 0xff, mField140));
		aPic = mApp->mPicCleared;
		aPic->Draw(mField70 - (aPic->mWidth >> 1), 0xdc - (aPic->mHeight >> 1));
		aGraphics->SetColorizeImages(false);
	}
}

// 0x412840
void DynomiteTimeTrial::Init(FossilizedApp* theApp, int theDifficulty)
{
	Dynomite::Init(theApp, theDifficulty);
	mFirePrompt->mOwnLine1 = mApp->GetString("CLEAR_ALL_THE_EGGS");
	mFirePrompt->mOwnLine2 = mApp->GetString("AS_FAST_AS_YOU_CAN");
	mSteggy->mField28 = false;
	mField68 = 0x1a0;
	mField6c = 0xd0;
	mApp->mSideBar->mShowTime = true;
	mLauncher->Load(1);
	int aRows;
	if (mDifficulty == 1)
	{
		mGrid->mMaxColor = 5;
		aRows = 0x3c;
	}
	else if (mDifficulty == 2)
	{
		mGrid->mMaxColor = 7;
		aRows = 0x1e;
	}
	else
	{
		mGrid->mMaxColor = 9;
		aRows = 0xf;
	}
	mGrid->mCeiling = (float) (aRows * -0x21);
	mField104 = (int) mGrid->mCeiling;
	int i = 0;
	if (aRows != 0)
	{
		do
		{
			mGrid->AddRow(i);
			i++;
		} while (i < aRows);
	}
	mGrid->ResetCeiling();
	mGrid->mField46d0 = true;
	mGrid->mBabyEggsOn = false;
}

// 0x412ad0
void DynomiteTimeTrial::DrawScenery()
{
	Dynomite::DrawScenery();
	mApp->mPicRowstogo->Draw(0x192, mField74 + 0x17);
	Graphics* g = GetDrawGraphics();
	g->SetFont(mApp->mWhiteFontSmall);
	g->SetColor(Color(0xff, 0xff, 0xff));
	char aText[16];
	if (mField131 == true)
		sprintf(aText, "0");
	else
	{
		sprintf(aText, "%.0f", (double) mGrid->mRows + 1.0);
		double aRowsToGo = (double) mGrid->mRows + 1.0;
		if ((double) mRowsCleared < aRowsToGo)
			mRowsCleared = (int) aRowsToGo;
	}
	g->DrawString(aText, 0x1cd - mApp->mWhiteFontSmall->StringWidth(aText), mField74 + 0x52);
}

// 0x412cb0: each line has a black shadow two pixels down and right; the numbers are right-aligned
void DynomiteTimeTrial::DrawGameOver(Graphics* g)
{
	Dynomite::DrawGameOver(g);
	g->SetFont(mApp->mWhiteFontSmall);
	char aRows[16];
	sprintf(aRows, "%d", mRowsCleared);
	char aBroken[16];
	sprintf(aBroken, "%d", mEggsBroken);
	char aDropped[16];
	sprintf(aDropped, "%d", mEggsDropped);
	// the score is minus the time in hundredths
	int aSeconds = mApp->mSideBar->mScore / -100;
	char aTime[16];
	sprintf(aTime, "%d:%.2d", aSeconds / 60, aSeconds % 60);
	int aRowsWidth = g->GetFont()->StringWidth(aRows);
	int aBrokenWidth = g->GetFont()->StringWidth(aBroken);
	int aDroppedWidth = g->GetFont()->StringWidth(aDropped);
	int aTimeWidth = g->GetFont()->StringWidth(aTime);
	g->SetColor(Color(0, 0, 0));
	g->DrawString(mApp->GetString("ROWS_CLEARED"), 0xe3, mField1c8 + 0x57);
	g->DrawString(mApp->GetString("EGGS_BROKEN"), 0xe3, mField1c8 + 0x6b);
	g->DrawString(mApp->GetString("EGGS_DROPPED"), 0xe3, mField1c8 + 0x7f);
	g->DrawString(mApp->GetString("FINAL_SCORE"), 0xe3, mField1c8 + 0x93);
	g->DrawString(aRows, 0x1b7 - aRowsWidth, mField1c8 + 0x57);
	g->DrawString(aBroken, 0x1b7 - aBrokenWidth, mField1c8 + 0x6b);
	g->DrawString(aDropped, 0x1b7 - aDroppedWidth, mField1c8 + 0x7f);
	g->DrawString(aTime, 0x1b7 - aTimeWidth, mField1c8 + 0x93);
	g->SetColor(Color(0xe6, 0xe6, 0xff));
	g->DrawString(mApp->GetString("ROWS_CLEARED"), 0xe1, mField1c8 + 0x55);
	g->DrawString(mApp->GetString("EGGS_BROKEN"), 0xe1, mField1c8 + 0x69);
	g->DrawString(mApp->GetString("EGGS_DROPPED"), 0xe1, mField1c8 + 0x7d);
	g->DrawString(aRows, 0x1b5 - aRowsWidth, mField1c8 + 0x55);
	g->DrawString(aBroken, 0x1b5 - aBrokenWidth, mField1c8 + 0x69);
	g->DrawString(aDropped, 0x1b5 - aDroppedWidth, mField1c8 + 0x7d);
	g->SetColor(Color(0xff, 0xff, 0xff));
	g->DrawString(mApp->GetString("FINAL_SCORE"), 0xe1, mField1c8 + 0x91);
	g->DrawString(aTime, 0x1b5 - aTimeWidth, mField1c8 + 0x91);
}

// 0x4126b0
void DynomiteTimeTrial::Save()
{
	Dynomite::Save();
	mBinFile.WriteInt(mEggsBroken);
	mBinFile.WriteInt(mEggsDropped);
	mBinFile.WriteInt(mRowsCleared);
}

// 0x4126f0
void DynomiteTimeTrial::Load()
{
	Dynomite::Load();
	mEggsBroken = mBinFile.ReadInt();
	mEggsDropped = mBinFile.ReadInt();
	mRowsCleared = mBinFile.ReadInt();
}
