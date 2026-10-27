#include "DynomiteStomped.h"
#include "BinFile.h"
#include "Effects.h"
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
#include "SoundInstance.h"
#include "Sprite.h"
#include "Steggy.h"
#include "Tank.h"
#include "Thunderbolt.h"
#include <cstdio>
#include <cstring>

using namespace Sexy;

// port: the original reads a name's whole length (the pack's string with its end) into the field; here it is cut
// to the field and always ended
static void ReadName(BinFile* theFile, char* theName, unsigned int theSize)
{
	unsigned int aLength = (unsigned int) theFile->ReadInt();
	unsigned int aRead = aLength;
	if (aRead > theSize - 1)
		aRead = theSize - 1;
	memset(theName, 0, theSize);
	theFile->Read(theName, aRead);
	if (aLength > aRead)
	{
		unsigned int aRest = aLength - aRead;
		unsigned int aLeft = (unsigned int) (theFile->Length() - theFile->Tell());
		theFile->Skip((long) ((aRest < aLeft) ? aRest : aLeft));
	}
}

// 0x40ea50
DynomiteStomped::DynomiteStomped()
{
	mGameType = 0;
	mKick = 0.0f;
	strcpy(mHighScoreTitle, GTC(0x4f9e48));
	mHighScores = new HighScores(0);
	mEggIndex = 0;
	mEggCount = 0;
	mMeterTarget = 0;
	mMeter = 0;
	mCleared = false;
	mClearedAlpha = 0;
	mClearedFrames = 0;
	mField23c = 0;
	mScoreRanks[0] = 0;
	mFirstRound = true;
	mSong = 1;
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
	mScoreRanks[12] = 60000;
	mScoreRanks[13] = 70000;
	mScoreRanks[14] = 80000;
	mScoreRanks[15] = 90000;
	mScoreRanks[16] = 100000;
	mScoreRanks[17] = 115000;
	mScoreRanks[18] = 130000;
	mScoreRanks[19] = 150000;
	// port: zeroed; the original leaves these as the allocation had them until they are first set (the round's
	// bonus only by a lone egg or the bonus itself)
	mWarnTimer = 0;
	mStartEggs = 0;
	mShots = 0;
	mRoundBonus = 0;
	mPackPasses = 0;
	mPackPos = 0;
	mFirstPos = 0;
	mPuzzleName[0] = 0;
	mRoundText[0] = 0;
}

// 0x40ec00
DynomiteStomped::~DynomiteStomped()
{
	mPack.Close();
}

// 0x40ec70
Egg* DynomiteStomped::NextEgg()
{
	Egg* anEgg;
	if (mEggCount < mEggIndex)
	{
		anEgg = Dynomite::NextEgg();
		if ((mField108 != 0) && (anEgg->mColor < 0xf))
		{
			if (mField108 == 0x14)
			{
				if ((0x168 < mGrid->mBottom) && (mApp->mPicker.Random(2, false) == 0))
					mField108 = 5;
				if (mField108 == 0x14)
				{
					if (0x178 < mGrid->mBottom)
						mField108 = 5;
					if ((mField108 == 0x14) && (mApp->mPicker.Random(6, false) == 2))
						mField108 = 1;
				}
				if (mField108 == 0x14)
					mField108 = mApp->mPicker.Random(3, false) + 8;
			}
			anEgg->mKind = mField108;
			mField108 = 0;
		}
		return anEgg;
	}
	if (mColors[mEggIndex] == 0x14)
		anEgg = Dynomite::NextEgg();
	else
	{
		anEgg = new Egg();
		anEgg->Init(mApp, this, (int) mColors[mEggIndex]);
	}
	int aKind = (int) mKinds[mEggIndex];
	mEggIndex++;
	if (aKind != 0)
	{
		anEgg->mKind = aKind;
		return anEgg;
	}
	if ((mField108 != 0) && (anEgg->mColor < 0xf))
	{
		if (mField108 == 0x14)
		{
			if ((300 < mGrid->mBottom) && (mApp->mPicker.Random(3, false) == 0))
				mField108 = 5;
			if (mField108 == 0x14)
			{
				if (mApp->mPicker.Random(6, false) == 2)
					mField108 = 1;
				if (mField108 == 0x14)
					mField108 = mApp->mPicker.Random(3, false) + 8;
			}
		}
		anEgg->mKind = mField108;
		mField108 = 0;
	}
	return anEgg;
}

// 0x40eef0: the grid again from the round's place in the pack; in the tutorial rows 4 to 9 of the first ten columns
// are emptied. The ceiling then comes down from above to where the puzzle has it.
void DynomiteStomped::LoadGrid(int theUnused)
{
	mPack.Open("STOMPED.TMP", true);
	mPack.Seek(mPackPos);
	mGrid->Clear();
	mGrid->mMaxColor = 8;
	mGrid->Load(&mPack);
	mPackPos = mPack.Tell();
	mPack.Close();
	if (mApp->mInTutorial == true)
	{
		int aRow = 4;
		do
		{
			int aCol = 0;
			do
			{
				if (mGrid->mDynamite[aCol][aRow] != NULL)
				{
					mGrid->mDynamites.Remove(mGrid->mDynamite[aCol][aRow]);
					if (mGrid->mDynamite[aCol][aRow] != NULL)
						delete mGrid->mDynamite[aCol][aRow];
					mGrid->mDynamite[aCol][aRow] = NULL;
				}
				mGrid->Remove(aCol, aRow, false);
				aCol++;
			} while (aCol < 10);
			aRow++;
		} while (aRow < 10);
	}
	mCeilingTarget = mGrid->mCeiling;
	mGrid->ResetCeiling();
	mShotsLeft = mShotsPerDrop;
	mCeilingMoving = true;
	mSteggy->mField28 = true;
	mShots = 0;
	mStartEggs = 0;
}

// 0x40f030
void DynomiteStomped::EggLanded()
{
	mShotsLeft--;
	mShots++;
	if (mShotsLeft < 1)
	{
		mShotsLeft = mShotsPerDrop;
		mCeilingMoving = true;
		mCeilingTarget = (float) ((double) mGrid->mCeiling + 33.0f);
		mApp->mSndFootfall->Play();
	}
	mMeterTarget = (int) ((double) 132.0f - (double) (mShotsLeft - 1) / (double) (mShotsPerDrop - 1) * 132.0f);
}

// 0x40f0c0
void DynomiteStomped::AddPoints(int x, int y, int thePoints, int theMultiplier, bool isChain)
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
		thePoints = thePoints / 2;
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
		aScore = thePoints * 0x14;
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
	if (mCleared == true)
	{
		aPopup->mSpeed = 0.0f;
		aPopup->mLife = 200;
		if ((499 < mRoundBonus) || (mRound == 1))
			aPopup->mLife = 0x113;
	}
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

// 0x40f3f0
void DynomiteStomped::EggAlone(int theColor)
{
	if (mGrid->HasColor(theColor) == true)
	{
		mRoundBonus = mRoundBonus - 0x19;
		if (mRoundBonus < 0)
			mRoundBonus = 0;
	}
}

// 0x40f420
void DynomiteStomped::GetPowerUpCenter(int* x, int* y)
{
	*x = mField70 + mField6c;
	*y = 0xbe;
}

// 0x40f8f0. port: STOMPED.TMP is kept in memory (BinFile::SetMemoryFile): the port writes nothing into the game's
// folder, which the web build does not have
void DynomiteStomped::CopyPack(const char* theFile)
{
	BinFile aPack;
	aPack.Open(theFile, true);
	BinFile::SetMemoryFile("STOMPED.TMP", aPack.mData);
	aPack.Close();
}

// 0x40f9a0: past the skill's first puzzles
void DynomiteStomped::SkipPuzzles(int theCount)
{
	char aBuffer[256];
	int i = 0;
	if (theCount < 1)
	{
		mPackPasses = 0;
		return;
	}
	do
	{
		if (mPack.ReadBool() == false)
			break;
		mPack.ReadInt();
		mPack.ReadInt();
		mPack.ReadInt();
		mPack.ReadFloat();
		ReadName(&mPack, mPuzzleName, sizeof(mPuzzleName));
		mPack.Read(aBuffer, 0x100);
		mPack.Read(aBuffer, 0x100);
		mGrid->Load(&mPack);
		i++;
	} while (i < theCount);
	mPackPasses = 0;
}

// 0x40fa80
void DynomiteStomped::Draw(Graphics* g)
{
	if (mField131 == true)
	{
		SetDrawOffset(Point(mField138, mField13c));
		mField138 = mField138 / 2;
		mField13c = mField13c / 2;
	}
	else if ((mApp->mGrindInstance->IsPlaying() == true) || (mShotsLeft == 1))
	{
		int aShakeX = (int) (mApp->mRand.NextNoAssert() & 1) - 1;
		int aShakeY = (int) (mApp->mRand.NextNoAssert() & 1) - 1;
		SetDrawOffset(Point(aShakeX, aShakeY));
	}
	Dynomite::Draw(g);
	if ((mField131 == true) && (mField132 == false))
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
	SetDrawOffset(Point(0, 0));
}

// 0x40fc30: a puzzle is its width, its launcher's place, its shots a row, its baby eggs' chance (the eggs' frames
// between babies are 100 plus 10,000 times one less it), its name, its first eggs' colours and kinds (256 each) and
// its grid. The grid is read here and again by LoadGrid once the walls are in place.
void DynomiteStomped::NextPuzzle(int theUnused)
{
	char aColors[256];
	char aKinds[256];
	mPack.Open("STOMPED.TMP", true);
	mPack.Seek(mPackPos);
	bool hasPuzzle = mPack.ReadBool();
	while (hasPuzzle == false)
	{
		mPackPasses++;
		mApp->mSideBar->MarkDirty();
		mPack.Seek(mFirstPos);
		hasPuzzle = mPack.ReadBool();
		// port: a pack with no puzzle at the skill's first would loop for ever
		if (hasPuzzle == false)
			break;
	}
	int aWidth = mPack.ReadInt();
	int aLauncherX = mPack.ReadInt();
	int aShotsPerDrop = mPack.ReadInt();
	float aBabyChance = mPack.ReadFloat();
	mGrid->mBabyEggChance = (int) ((1.0 - (double) aBabyChance) * 10000.0f + 100.0f);
	memset(aColors, 0x14, sizeof(aColors));
	memset(aKinds, 0, sizeof(aKinds));
	ReadName(&mPack, mPuzzleName, sizeof(mPuzzleName));
	mPack.Read(aColors, 0x100);
	mPack.Read(aKinds, 0x100);
	mShotsPerDrop = aShotsPerDrop - mPackPasses * 2;
	if (mShotsPerDrop < 3)
		mShotsPerDrop = 3;
	mFirePrompt->Reset();
	mGrid->Clear();
	mGrid->mCeiling = -100.0f;
	mCeilingTarget = -100.0f;
	mRound++;
	mLauncherX = aLauncherX;
	mField104 = -100;
	mField16c = false;
	mRoundLoaded = false;
	mWidth = aWidth;
	// port: bounded (the field is 21 bytes)
	snprintf(mRoundText, sizeof(mRoundText), mApp->GetString("ROUND_D").c_str(), mRound);
	mTextAlpha = 0;
	mTextTimer = 500;
	mTextHold = 10;
	mSettingUp = true;
	mFirePrompt->mField1c = true;
	mEggIndex = 0;
	mStartEggs = 0;
	mShots = 0;
	mClearedFrames = 0;
	mEggCount = 0;
	int aLast = 0xff;
	do
	{
		if (aColors[aLast] != 0x14)
			break;
		aLast--;
	} while (0 < aLast);
	int aLastKind = 0xff;
	do
	{
		// the original tests the colours' last place each time, not the kinds'
		if (aKinds[aLast] != 0)
			break;
		aLastKind--;
	} while (0 < aLastKind);
	mEggCount = aLast;
	if (aLast < aLastKind + 1)
		mEggCount = aLastKind;
	int i = 0;
	do
	{
		mColors[i] = aColors[i];
		mKinds[i] = aKinds[i];
		i++;
	} while (i < 0x100);
	mPackPos = mPack.Tell();
	mGrid->mMaxColor = 8;
	mGrid->Load(&mPack);
	mGrid->mField46e5 = true;
	mGrid->ResetCeiling();
	mCeilingMoving = false;
	mField104 = (int) mGrid->mCeiling;
	mPack.Close();
}

// 0x40ffc0
void DynomiteStomped::DrawScenery()
{
	mApp->mPicLeftwall->Draw((mField70 - mField6c) - 0x38, 0);
	mApp->mPicRightwall->Draw(mField6c - 0x1e + mField70, 0);
	mApp->mPicMamameter_c1->Draw(mField6c - 7 + mField70, 0x107);
	mApp->mPicMamameter_c0->DrawPart(mField6c - 7 + mField70, 0x107, 0, 0, 0x27, 0x8a - mMeter);
	Pic* aPic = mApp->mPicMamaportrait;
	aPic->Draw((mField6c + 0x14 + mField70) - (aPic->mWidth >> 1), 0xd2 - (aPic->mHeight >> 1));
	mSteggy->Draw();
	mTank->Draw();
	mApp->mPicEggpilesheet_c1->Draw(mField158 - 0x110 + mField70, 0x185);
	mApp->mPicEggpilesheet_c1->Draw(mField158 - 0x1a6 + mField70, 0x185);
	mApp->mPicEggpilesheet_c0->Draw(mField158 + 0x50 + mField70, 400);
	mApp->mPicEggpilesheet_c0->Draw(mField158 + 0xe6 + mField70, 400);
	if ((mShotsLeft < 4) && (0 < mShotsLeft))
	{
		// the glow fades from each warning's beat
		float anAlpha = (float) ((double) mWarnTimer * (double) 0.008f * (double) 255.0f);
		Graphics* g = GetDrawGraphics();
		g->SetColorizeImages(true);
		g->SetColor(Color(0xff, 0xff, 0xff, (int) anAlpha));
		aPic = mApp->mPicThermoglow;
		aPic->Draw((mField6c + 7 + mField70) - (aPic->mWidth >> 1), 0x19a - (aPic->mHeight >> 1));
		g->SetColorizeImages(false);
	}
	mPowerUps.ResetIter();
	PowerUp* aPowerUp = mPowerUps.Next();
	while (aPowerUp != NULL)
	{
		aPowerUp->Draw(mField128);
		aPowerUp = mPowerUps.Next();
	}
	Graphics* g = GetDrawGraphics();
	if (0 < mClearedAlpha)
	{
		g->SetColorizeImages(true);
		g->SetColor(Color(0, 0, 0, mClearedAlpha));
		aPic = mApp->mPicCleared;
		aPic->Draw((mField70 + 3) - (aPic->mWidth >> 1), 0xb2 - (aPic->mHeight >> 1));
		g->SetColor(Color(0xff, 0xff, 0xff, mClearedAlpha));
		aPic = mApp->mPicCleared;
		aPic->Draw(mField70 - (aPic->mWidth >> 1), 0xaf - (aPic->mHeight >> 1));
		g->SetColorizeImages(false);
	}
	if (0x32 < mClearedFrames)
	{
		// "round", "round cleared", "round cleared bonus", each placed where the last one is centred
		std::string aText;
		if (mClearedFrames > 0x82)
			aText = mApp->GetString("ROUND_CLEARED_BONUS");
		else if (mClearedFrames > 0x5a)
			aText = mApp->GetString("ROUND_CLEARED");
		else
			aText = mApp->GetString("ROUND");
		PicFont* aFont = mApp->mAnnounceFont1928;
		int aWidth = aFont->Width(mApp->GetString("ROUND_CLEARED_BONUS").c_str());
		g->SetColorizeImages(true);
		g->SetColor(Color(0, 0, 0, 0xff));
		aFont->Draw(aText.c_str(), (mField70 - aWidth / 2) + 4, 0xdb);
		g->SetColor(Color(0xff, 0xff, 0, 0xff));
		aFont->Draw(aText.c_str(), (mField70 - aWidth / 2) + 4, 0xd7);
		g->SetColorizeImages(false);
	}
	if (mThunderbolt != NULL)
		mThunderbolt->Draw();
	Point anOffset = GetDrawOffset();
	SetDrawOffset(Point(0, 0));
	mApp->mPicMenuoverlap->Draw(0, 0);
	SetDrawOffset(anOffset);
	if ((0 < mTextAlpha) && (mApp->mInTutorial == false))
	{
		g = GetDrawGraphics();
		g->SetColorizeImages(true);
		PicFont* aFont = mApp->mAnnounceFont;
		int aRoundWidth = aFont->Width(mRoundText);
		int aNameWidth = aFont->Width(mPuzzleName);
		g->SetColor(Color(0, 0, 0, mTextAlpha));
		aFont->Draw(mRoundText, (mField70 - (aRoundWidth >> 1)) + 3, 0x71);
		aFont->Draw(mPuzzleName, (mField70 - (aNameWidth >> 1)) + 3, 0x8a);
		g->SetColor(Color(0xff, 0xff, 0xff, mTextAlpha));
		aFont->Draw(mRoundText, mField70 - (aRoundWidth >> 1), 0x6e);
		aFont->Draw(mPuzzleName, mField70 - (aNameWidth >> 1), 0x87);
		g->SetColorizeImages(false);
	}
}

// 0x4107f0: the launcher's egg is swapped for the next puzzle's first unless they are alike, and Steggy drops his
void DynomiteStomped::NextRound()
{
	mRoundsCleared++;
	mCleared = false;
	mCeilingTarget = 25.0f;
	mCeilingMoving = true;
	NextPuzzle(1);
	mFieldB0 = true;
	Egg* anEgg = NextEgg();
	Egg* aLoaded = mLauncher->mLoaded;
	if ((aLoaded->mColor == anEgg->mColor) && (aLoaded->mKind == anEgg->mKind))
	{
		delete anEgg;
		mSteggy->mDropEgg = true;
		return;
	}
	mLauncher->Freeze(anEgg);
	mSteggy->mDropEgg = true;
}

// 0x410880: each line has a black shadow two pixels down and right; the numbers are right-aligned. The labels and
// the combos' names are the exe's own.
void DynomiteStomped::DrawGameOver(Graphics* g)
{
	const char* aComboNames[9];
	aComboNames[0] = GTC(0x4fa03c);
	aComboNames[1] = GTC(0x4fa03c);
	aComboNames[2] = GTC(0x4fa034);
	aComboNames[3] = GTC(0x4fa02c);
	aComboNames[4] = GTC(0x4fa020);
	aComboNames[5] = GTC(0x4fa014);
	aComboNames[6] = GTC(0x4fa008);
	aComboNames[7] = GTC(0x4f9ffc);
	aComboNames[8] = GTC(0x4f9ff0);
	Dynomite::DrawGameOver(g);
	g->SetFont(mApp->mWhiteFontSmall);
	char aCombo[32];
	strcpy(aCombo, aComboNames[mLongestChain]);
	int aComboWidth = g->GetFont()->StringWidth(aCombo);
	char aScore[16];
	sprintf(aScore, "%d", mApp->mSideBar->mScore);
	int aScoreWidth = g->GetFont()->StringWidth(aScore);
	char aRounds[16];
	sprintf(aRounds, "%d", mRound);
	int aRoundsWidth = g->GetFont()->StringWidth(aRounds);
	char aBabies[16];
	sprintf(aBabies, "%d", mBabies);
	int aBabiesWidth = g->GetFont()->StringWidth(aBabies);
	g->SetColor(Color(0, 0, 0));
	g->DrawString(GTC(0x4f9fe4), 0xe3, mField1c8 + 0x57);
	g->DrawString(GTC(0x4f9fd4), 0xe3, mField1c8 + 0x6b);
	g->DrawString(GTC(0x4f9fc4), 0xe3, mField1c8 + 0x7f);
	g->DrawString(GTC(0x4f9fb4), 0xe3, mField1c8 + 0x93);
	g->DrawString(aCombo, 0x1b7 - aComboWidth, mField1c8 + 0x57);
	g->DrawString(aRounds, 0x1b7 - aRoundsWidth, mField1c8 + 0x7f);
	g->DrawString(aScore, 0x1b7 - aScoreWidth, mField1c8 + 0x93);
	g->DrawString(aBabies, 0x1b7 - aBabiesWidth, mField1c8 + 0x6b);
	g->SetColor(Color(0xe6, 0xe6, 0xff));
	g->DrawString(GTC(0x4f9fe4), 0xe1, mField1c8 + 0x55);
	g->DrawString(aCombo, 0x1b5 - aComboWidth, mField1c8 + 0x55);
	g->DrawString(GTC(0x4f9fd4), 0xe1, mField1c8 + 0x69);
	g->DrawString(GTC(0x4f9fc4), 0xe1, mField1c8 + 0x7d);
	g->DrawString(aRounds, 0x1b5 - aRoundsWidth, mField1c8 + 0x7d);
	g->DrawString(aBabies, 0x1b5 - aBabiesWidth, mField1c8 + 0x69);
	g->SetColor(Color(0xff, 0xff, 0xff));
	g->DrawString(GTC(0x4f9fb4), 0xe1, mField1c8 + 0x91);
	g->DrawString(aScore, 0x1b5 - aScoreWidth, mField1c8 + 0x91);
}

// 0x411370: the pack's header is its name, the puzzles each skill skips (-1 none) and its picture
void DynomiteStomped::Init(FossilizedApp* theApp, int theDifficulty)
{
	char aPackName[100];
	Dynomite::Init(theApp, theDifficulty);
	mFirePrompt->mOwnLine1 = mApp->GetString("CLEAR_ALL_THE");
	mFirePrompt->mOwnLine2 = mApp->GetString("EGGS_TO_ADVANCE");
	mGrid->mBabyEggChance = mGrid->mBabyEggChance * 5;
	CopyPack(mApp->mPackFile);
	mPack.Open("STOMPED.TMP", true);
	ReadName(&mPack, aPackName, sizeof(aPackName));
	int anEasy = mPack.ReadInt();
	int aMedium = mPack.ReadInt();
	int aHard = mPack.ReadInt();
	mPack.Skip(0xb2e0);
	if (theDifficulty == 1)
		SkipPuzzles(anEasy);
	else if (theDifficulty == 2)
		SkipPuzzles(aMedium);
	else if (theDifficulty == 3)
		SkipPuzzles(aHard);
	int aPos = mPack.Tell();
	mFirstPos = aPos;
	mPackPos = aPos;
	mPack.Close();
	mRound = 0;
	mRoundsCleared = 0;
	mGrid->mMaxColor = 5;
	NextPuzzle(1);
	mLauncher->SetLoaded(NextEgg());
	mGrid->mField46d0 = true;
	mGrid->ResetCeiling();
}

// 0x411630: "cleared" fades in while the eggs go up; once they are off the top the texts come at 50, 90 and 130
// frames, the bonus at 200 and the next puzzle at 400
void DynomiteStomped::UpdateCleared()
{
	if (mClearedAlpha < 0xff)
		mClearedAlpha = mClearedAlpha + 5;
	if (0xff < mClearedAlpha)
		mClearedAlpha = 0xff;
	EggGrid* aGrid = mGrid;
	if (-0x1e < aGrid->mBottom)
	{
		aGrid->mCeiling = (float) ((double) aGrid->mCeiling - 2.0f);
		aGrid = mGrid;
		mField104 = (int) aGrid->mCeiling;
	}
	if ((mClearedAlpha == 0xff) && (aGrid->mCeiling < -20.0f))
	{
		mClearedFrames++;
		if ((mClearedFrames == 0x32) || (mClearedFrames == 0x5a) || (mClearedFrames == 0x82))
			mApp->mSndInchypop->Play();
		if (mClearedFrames == 200)
		{
			mRoundBonus = 100;
			AddPoints(mField70, 0x10e, 100, 0, true);
			mApp->mSndInchypop->Play();
		}
		if (mClearedFrames == 400)
			NextRound();
	}
}

// 0x411740
void DynomiteStomped::Update()
{
	if (mApp->mActive == false)
		return;
	if (mField132 == true)
	{
		UpdateGameOver();
		return;
	}
	if (0 < mPauseCount)
		return;
	if (mMeter != mMeterTarget)
	{
		if (mMeterTarget < mMeter)
			mMeter--;
		if (mMeter < mMeterTarget)
			mMeter++;
	}
	if (mSettingUp == true)
	{
		// the walls and the launcher move a pixel a frame to the puzzle's, grinding; then the eggs come in
		mField80 = true;
		mLauncher->Update();
		mSteggy->Update();
		mTank->Update();
		mGrid->Update();
		UpdateBoard();
		bool isMoving = false;
		if (mFirstRound == true)
		{
			mField68 = mWidth;
			mField6c = mWidth >> 1;
			mField158 = mLauncherX;
			mGrindFrames = 0;
		}
		if (mField68 != mWidth)
		{
			isMoving = true;
			if (mWidth < mField68)
				mField68--;
			if (mField68 < mWidth)
				mField68++;
			mField6c = mField68 >> 1;
		}
		if (mField158 != mLauncherX)
		{
			isMoving = true;
			if (mLauncherX < mField158)
				mField158--;
			if (mField158 < mLauncherX)
				mField158++;
		}
		if (isMoving == true)
		{
			mGrindFrames = 0x32;
			if (mApp->mGrindInstance->IsPlaying() == false)
				mApp->mGrindInstance->Play(true, false);
		}
		else
			mApp->mGrindInstance->Stop();
		mGrindFrames--;
		if (0 < mGrindFrames)
			isMoving = true;
		if (mTextAlpha < 0xff)
		{
			mTextAlpha = mTextAlpha + 3;
			if (0xff < mTextAlpha)
				mTextAlpha = 0xff;
			mFirePrompt->mField1c = true;
		}
		else if (0 < mTextHold)
		{
			mTextHold--;
			mFirePrompt->mField1c = true;
		}
		if (isMoving == false)
		{
			mField80 = false;
			mApp->mGrindInstance->Stop();
			if (mRoundLoaded == false)
			{
				LoadGrid(1);
				mRoundLoaded = true;
			}
			mFirstRound = false;
			mSettingUp = false;
			mFieldB0 = true;
		}
		if (0 < mTextHold)
		{
			mSettingUp = true;
			mFieldB0 = false;
		}
		if (0 < mClearedAlpha)
		{
			mClearedAlpha = mClearedAlpha - 5;
			if (mClearedAlpha < 0)
				mClearedAlpha = 0;
		}
		if ((mCeilingMoving == true) && (mRoundLoaded == true))
		{
			if (mGrid->mCeiling < mCeilingTarget)
				mGrid->mCeiling = (float) ((double) mGrid->mCeiling + 2.0f);
			else
				mCeilingMoving = false;
		}
		MarkDirty();
	}
	else if (mCleared == true)
	{
		mField80 = true;
		if (0 < mTextAlpha)
		{
			mTextAlpha = mTextAlpha - 2;
			if (mTextAlpha < 0)
				mTextAlpha = 0;
		}
		mFieldB0 = false;
		mLauncher->Update();
		mSteggy->Update();
		mTank->Update();
		UpdateBoard();
		UpdateCleared();
		MarkDirty();
	}
	else
	{
		Dynomite::Update();
		mField80 = false;
		if ((mBonusBits & 0x70) == 0)
		{
			if (mApp->mGrindInstance->IsPlaying() == true)
				mApp->mGrindInstance->Stop();
		}
		else if (mApp->mGrindInstance->IsPlaying() == false)
			mApp->mGrindInstance->Play(true, false);
		if (((mBonusBits & 0x10) != 0) && (-0x2b < mGrid->mBottom))
		{
			EggGrid* aGrid = mGrid;
			if (aGrid->mCeiling < 0.0f)
				aGrid->mCeiling = (float) ((double) aGrid->mCeiling - 0.05f);
			else
				aGrid->mCeiling = (float) ((double) aGrid->mCeiling - 0.1f);
		}
		if (mField16c == true)
		{
			if (0 < mTextAlpha)
			{
				mTextAlpha = mTextAlpha - 2;
				if (mTextAlpha < 0)
					mTextAlpha = 0;
			}
		}
		else
		{
			mTextTimer--;
			if (mTextTimer < 1)
			{
				mTextTimer = 0;
				mTextAlpha = mTextAlpha - 1;
				if (mTextAlpha < 0)
					mTextAlpha = 0;
			}
		}
		if (0 < mClearedAlpha)
		{
			mClearedAlpha = mClearedAlpha - 5;
			if (mClearedAlpha < 0)
				mClearedAlpha = 0;
		}
		if (mStartEggs == 0)
			mStartEggs = mGrid->mEggCount;
		if ((mGrid->mEggCount < 1) && (mField16c == true) && (mField130 == false) && (mField131 == false))
		{
			// the round is cleared
			mPowerUps.ResetIter();
			PowerUp* aPowerUp = mPowerUps.Next();
			while (aPowerUp != NULL)
			{
				mPowerUps.Remove(aPowerUp);
				if (aPowerUp != NULL)
					delete aPowerUp;
				aPowerUp = mPowerUps.Next();
			}
			mBonusBits = 0;
			mSteggy->mField28 = false;
			mShotsLeft = mShotsPerDrop;
			mCleared = true;
			mMeterTarget = (int) ((double) 132.0f -
				(double) (mShotsLeft - 1) / (double) (mShotsPerDrop - 1) * 132.0f);
			mApp->mSndChime->Play();
			mWarnTimer = 0;
			mTextAlpha = 0;
			mTextTimer = 500;
			mFirePrompt->mAlpha = 0;
			mShotsLeft = 100;
			mChain = -1;
			mClearedFrames = 0;
			mField108 = 0;
			mField16c = false;
		}
		if (mFieldB0 == true)
		{
			if (mField130 == false)
			{
				EggGrid* aGrid = mGrid;
				if (aGrid->mField4691 == true)
				{
					aGrid->mField4691 = false;
					mApp->mSndAooga->Play();
				}
				aGrid = mGrid;
				if (aGrid->mPastLineFrames == 0)
				{
					if (mKick != 0.0f)
					{
						aGrid->mCeiling = (float) ((double) aGrid->mCeiling - mKick);
						double aKick = (double) mKick * (double) 0.95f;
						mKick = (float) aKick;
						if (aKick < 1.0)
							mKick = 0.0f;
					}
					else if (mCeilingMoving == true)
					{
						if (aGrid->mCeiling < mCeilingTarget)
							aGrid->mCeiling = (float) ((double) aGrid->mCeiling + 2.0f);
						else
							mCeilingMoving = false;
					}
				}
				if ((mShotsLeft < 4) && (0 < mShotsLeft))
				{
					mWarnTimer--;
					if (mWarnTimer < 1)
					{
						mApp->mSndWarning->Play();
						mWarnTimer = 0x7d;
					}
				}
				else
					mWarnTimer = 2;
				if (mShotsLeft == 1)
				{
					if (mApp->mFootfallRumbleInstance->IsPlaying() == false)
						mApp->mFootfallRumbleInstance->Play(true, false);
				}
				else
					mApp->mFootfallRumbleInstance->Stop();
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
					mWarnTimer = 0;
					mApp->mSndFootlands->Play();
					mApp->mMusicInterface->StopMusic(mSong);
					mApp->mMusicInterface->SetSongVolume(4, 1.0);
					mApp->mMusicInterface->PlayMusic(4, 0x23, false);
					mField140 = 0;
					mTalker = new MenuTalker(mApp, this, 2);
				}
			}
			else
			{
				if (mApp->mFootfallRumbleInstance->IsPlaying() == true)
					mApp->mFootfallRumbleInstance->Stop();
				if (mApp->mGrindInstance->IsPlaying() == true)
					mApp->mGrindInstance->Stop();
				mField140 = mField140 + 2;
				if (0xff < mField140)
					mField140 = 0xff;
			}
		}
	}
	if ((mApp->mIsRegistered == false) && (mField134 == 1))
	{
		if (mHighScores->IsHighScore(mApp->mSideBar->mScore, (int) mStartTicks) == true)
			mApp->ShowRegisterDialog(GTC(0x4fa068));
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
}

// 0x40f440: the pack (STOMPED.TMP) goes into the saved game whole, after its length and the places in it
void DynomiteStomped::Save()
{
	Dynomite::Save();
	mBinFile.WriteFloat(mKick);
	mBinFile.WriteFloat(mCeilingTarget);
	mBinFile.WriteBool(mCeilingMoving);
	mBinFile.WriteInt(mWidth);
	mBinFile.WriteInt(mLauncherX);
	mBinFile.WriteInt(mGrindFrames);
	mBinFile.WriteBool(mSettingUp);
	mBinFile.WriteBool(mRoundLoaded);
	mBinFile.WriteBool(mFirstRound);
	mBinFile.WriteInt(mShotsPerDrop);
	mBinFile.WriteInt(mShotsLeft);
	mBinFile.Write(mColors, 0x100);
	mBinFile.WriteInt(mEggIndex);
	mBinFile.WriteInt(mEggCount);
	mBinFile.WriteInt(mWarnTimer);
	mBinFile.WriteInt(mMeter);
	mBinFile.WriteInt(mMeterTarget);
	mBinFile.WriteInt(mStartEggs);
	mBinFile.WriteInt(mShots);
	mBinFile.WriteInt(mRound);
	mBinFile.WriteInt(mRoundsCleared);
	mBinFile.WriteInt(mRoundBonus);
	mBinFile.WriteString(mRoundText);
	mBinFile.WriteString(mPuzzleName);
	mBinFile.WriteInt(mTextAlpha);
	mBinFile.WriteInt(mTextHold);
	mBinFile.WriteBool(mCleared);
	mBinFile.WriteInt(mClearedAlpha);
	mBinFile.WriteInt(mClearedFrames);
	BinFile aPack;
	aPack.Open("STOMPED.TMP", true);
	mBinFile.WriteInt(aPack.Length());
	mBinFile.WriteInt(mPackPos);
	mBinFile.WriteInt(mFirstPos);
	mBinFile.CopyFrom(&aPack, aPack.Length());
	aPack.Close();
	mPack.Close();
}

// 0x40f6b0: the pack is written back to STOMPED.TMP
void DynomiteStomped::Load()
{
	Dynomite::Load();
	mKick = mBinFile.ReadFloat();
	mCeilingTarget = mBinFile.ReadFloat();
	mCeilingMoving = mBinFile.ReadBool();
	mWidth = mBinFile.ReadInt();
	mLauncherX = mBinFile.ReadInt();
	mGrindFrames = mBinFile.ReadInt();
	mSettingUp = mBinFile.ReadBool();
	mRoundLoaded = mBinFile.ReadBool();
	mFirstRound = mBinFile.ReadBool();
	mShotsPerDrop = mBinFile.ReadInt();
	mShotsLeft = mBinFile.ReadInt();
	mBinFile.Read(mColors, 0x100);
	mEggIndex = mBinFile.ReadInt();
	mEggCount = mBinFile.ReadInt();
	mWarnTimer = mBinFile.ReadInt();
	mMeter = mBinFile.ReadInt();
	mMeterTarget = mBinFile.ReadInt();
	mStartEggs = mBinFile.ReadInt();
	mShots = mBinFile.ReadInt();
	mRound = mBinFile.ReadInt();
	mRoundsCleared = mBinFile.ReadInt();
	mRoundBonus = mBinFile.ReadInt();
	mBinFile.ReadString(mRoundText);
	mBinFile.ReadString(mPuzzleName);
	mTextAlpha = mBinFile.ReadInt();
	mTextHold = mBinFile.ReadInt();
	mCleared = mBinFile.ReadBool();
	mClearedAlpha = mBinFile.ReadInt();
	mClearedFrames = mBinFile.ReadInt();
	int aLength = mBinFile.ReadInt();
	int aPackPos = mBinFile.ReadInt();
	mFirstPos = mBinFile.ReadInt();
	mTextTimer = 500;
	BinFile aPack;
	aPack.Create("STOMPED.TMP");
	aPack.CopyFrom(&mBinFile, aLength);
	aPack.Close();
	mPack.Close();
	mPackPos = aPackPos;
}
