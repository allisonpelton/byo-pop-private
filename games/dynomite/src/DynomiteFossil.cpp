#include "DynomiteFossil.h"
#include "BinFile.h"
#include "Effects.h"
#include "Egg.h"
#include "EggGrid.h"
#include "FirePrompt.h"
#include "Fossil.h"
#include "FossilizedApp.h"
#include "GameText.h"
#include "Graphics.h"
#include "HighScores.h"
#include "ImageFont.h"
#include "Kroney.h"
#include "Launcher.h"
#include "MenuTalker.h"
#include "Museum.h"
#include "MusicInterface.h"
#include "SideBar.h"
#include "SoundInstance.h"
#include "Sprite.h"
#include "Steggy.h"
#include "Tank.h"
#include "Thunderbolt.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>

using namespace Sexy;

std::vector<std::string> Sexy::gFossilRoundNames;
std::vector<std::string> Sexy::gFossilNames;

// 0x52b1c0 and 0x52b108: each round's level string (three to a fossil) in the exe, for the strategic kind and the
// panic kind; the entry after the 45th is an empty string
static const unsigned int gStrategicLevels[45] =
{
	0x4f84c0, 0x4f8470, 0x4f8428, 0x4f83d0, 0x4f8380, 0x4f8330, 0x4f82d0, 0x4f8280, 0x4f8230, 0x4f81d8, 0x4f8180,
	0x4f8130, 0x4f80e0, 0x4f8098, 0x4f8050, 0x4f7ff8, 0x4f7fa8, 0x4f7f50, 0x4f7ef8, 0x4f7ea0, 0x4f7e48, 0x4f7df0,
	0x4f7d98, 0x4f7d40, 0x4f7ce8, 0x4f7c88, 0x4f7c28, 0x4f7bd0, 0x4f7b78, 0x4f7b20, 0x4f7ac8, 0x4f7a70, 0x4f7a18,
	0x4f79c8, 0x4f7970, 0x4f7928, 0x4f78d0, 0x4f7878, 0x4f7820, 0x4f77c8, 0x4f7770, 0x4f7718, 0x4f76c0, 0x4f7660,
	0x4f7600
};
static const unsigned int gPanicLevels[45] =
{
	0x4f9970, 0x4f9900, 0x4f9898, 0x4f9820, 0x4f97b0, 0x4f9738, 0x4f96b8, 0x4f9648, 0x4f95d8, 0x4f9560, 0x4f94e8,
	0x4f9478, 0x4f93f8, 0x4f9390, 0x4f9328, 0x4f92b0, 0x4f9240, 0x4f91c8, 0x4f9150, 0x4f90d8, 0x4f9060, 0x4f8fe8,
	0x4f8f70, 0x4f8ef8, 0x4f8e80, 0x4f8e00, 0x4f8d80, 0x4f8d08, 0x4f8c90, 0x4f8c18, 0x4f8ba0, 0x4f8b28, 0x4f8ab0,
	0x4f8a40, 0x4f89c8, 0x4f8958, 0x4f88e0, 0x4f8868, 0x4f87f0, 0x4f8778, 0x4f8700, 0x4f8688, 0x4f8610, 0x4f8590,
	0x4f8518
};

// 0x4094b0's cases: where each fragment's cell is in its fossil's picture (x, y), by fossil and round
static const int gFragmentPos[15][3][2] =
{
	{{0x68, 0x70}, {0x24, 0x6c}, {0x47, 0x2a}},
	{{0x36, 0x69}, {0x99, 0x69}, {0x67, 0x8b}},
	{{0x52, 0xcd}, {0x84, 0xee}, {0x73, 0x8b}},
	{{0x3c, 0x132}, {0xc0, 0xae}, {0x2c, 0xcf}},
	{{0x9b, 0xe3}, {0x7a, 0xa1}, {0x59, 0xa1}},
	{{0x85, 0xf0}, {0x53, 0xcf}, {0x74, 0x8d}},
	{{99, 0x12e}, {0x21, 0xec}, {0x21, 0xaa}},
	{{0x89, 0x114}, {0xfd, 0xb1}, {0x79, 0xb1}},
	{{0x59, 0x10e}, {0x28, 0xaa}, {0x130, 0xec}},
	{{0x31, 0x12a}, {0xd6, 0x12a}, {0xb5, 0xa6}},
	{{0x8e, 0x11d}, {0x123, 0xfc}, {0xc0, 0xba}},
	{{0xd1, 0x127}, {0x5e, 0xc4}, {0x103, 0xc4}},
	{{0x94, 0x152}, {0x31, 0x8b}, {0x149, 0xee}},
	{{0xf8, 0x14c}, {0xf8, 200}, {99, 0xa7}},
	{{0x2f, 0xc9}, {0xf5, 0x14c}, {0x19a, 0xc9}}
};

// The level strings are words of KEY:value. The three readers copy the string, find the key and read the word after
// its colon. port: the copy is bounded and a key with nothing after its colon reads as missing; the original copies
// the whole string and reads past the end.
static char* FindLevelValue(char* theBuffer, const char* theLevel, const char* theKey)
{
	strncpy(theBuffer, theLevel, 255);
	theBuffer[255] = '\0';
	char* aFound = strstr(theBuffer, theKey);
	if (aFound == NULL)
		return NULL;
	return strtok(aFound + strlen(theKey) + 1, " ");
}

// 0x4089e0: 0 when the key is missing
static int LevelInt(const char* theLevel, const char* theKey)
{
	char aBuffer[256];
	char* aValue = FindLevelValue(aBuffer, theLevel, theKey);
	if (aValue == NULL)
		return 0;
	return atol(aValue);
}

// 0x408a60: 0 when the key is missing
static double LevelFloat(const char* theLevel, const char* theKey)
{
	char aBuffer[256];
	char* aValue = FindLevelValue(aBuffer, theLevel, theKey);
	if (aValue == NULL)
		return 0.0;
	return atof(aValue);
}

// 0x408ae0: "" when the key is missing
static void LevelWord(const char* theLevel, const char* theKey, char* theWord, int theSize)
{
	char aBuffer[256];
	char* aValue = FindLevelValue(aBuffer, theLevel, theKey);
	if (aValue == NULL)
	{
		*theWord = '\0';
		return;
	}
	// port: bounded by theSize
	strncpy(theWord, aValue, theSize - 1);
	theWord[theSize - 1] = '\0';
}

// 0x408e40
DynomiteFossil::DynomiteFossil()
{
	mFossilX = 0;
	mFossilY = 0;
	mLevel = 1;
	mRound = 1;
	// port: the original also builds and destroys a Dynomite on the stack here, which changes nothing
	mGameType = 2;
	mKick = 0.0f;
	strcpy(mHighScoreTitle, GTC(0x4f99f0));
	mHighScores = new HighScores(2);
	mFossilX = -1;
	mShownFossil = -1;
	mShownChannels = -1;
	mResizing = false;
	mKroney = NULL;
	mMuseum = NULL;
	mMuseumScroll = 0;
	mFossilsAssembled = 0;
	mScoreRanks[0] = 0;
	mSong = 0;
	mSongOffset = 0;
	mPanicKind = false;
	mRoundCount = 1;
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
	mScoreRanks[17] = 150000;
	mScoreRanks[18] = 180000;
	mScoreRanks[19] = 200000;
	// port: the original leaves these to Init and StartRound
	mFossil = NULL;
	mFirstLevel = 1;
	mPushes = 0;
	mFirstShotFade = 0;
	mFossilFell = false;
	mFallAlpha = 0;
	mFallTimer = 0;
	mWidth = 0;
	mFragmentText[0] = '\0';
	mTextAlpha = 0;
	mTextTimer = 0;
	mGlowAlpha = 0x80;
	mGlowStep = 2;
	mFallSpeed = 0.0f;
	mFallSpeedUp = 0.0f;
}

// 0x409070
DynomiteFossil::~DynomiteFossil()
{
	if (mKroney != NULL)
	{
		delete mKroney;
		mKroney = NULL;
	}
	if (mMuseum != NULL)
	{
		mMuseum->mKeepMusic = true;
		if (mMuseum != NULL)
			delete mMuseum;
		mMuseum = NULL;
	}
	// port: the original never deletes the fossil
	if (mFossil != NULL)
		delete mFossil;
}

// 0x407fa0
void DynomiteFossil::Blasted()
{
	mKick = 3.0f;
}

// 0x407fb0
void DynomiteFossil::DrawPiece(int theColor, int x, int y)
{
	if (theColor == 0x32)
	{
		mFossilX = x;
		mFossilY = y;
	}
}

// 0x407fd0: the walls stop, Kroney's sand is set to a thousand seconds and the chain ends
void DynomiteFossil::PieceFell(int theColor)
{
	if ((theColor == 0x32) && (mLosing == false))
	{
		mFossilFell = true;
		mFallAlpha = 0;
		mFireBlocked = true;
		mKroney->Start(1000.0f);
		mChain = -1;
	}
}

// 0x408020
void DynomiteFossil::AddPoints(int x, int y, int thePoints, int theMultiplier, bool isChain)
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
	if (mFossilFell == true)
	{
		aPopup->mSpeed = 0.0f;
		aPopup->mLife = 200;
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

// 0x4084b0
Egg* DynomiteFossil::NextEgg()
{
	Egg* anEgg = Dynomite::NextEgg();
	if ((mNextEggKind != 0) && (anEgg->mColor < 0xf))
	{
		if (mNextEggKind == 0x14)
		{
			if ((0x118 < mGrid->mBottom) && (mApp->mPicker.Random(3, false) == 0))
				mNextEggKind = 5;
			if (mNextEggKind == 0x14)
			{
				if (0x154 < mGrid->mBottom)
					mNextEggKind = 5;
				if (mNextEggKind == 0x14)
				{
					if (mApp->mPicker.Random(6, false) == 2)
						mNextEggKind = 1;
					if (mNextEggKind == 0x14)
						mNextEggKind = mApp->mPicker.Random(3, false) + 8;
				}
			}
		}
		anEgg->mKind = mNextEggKind;
		mNextEggKind = 0;
	}
	return anEgg;
}

// 0x4085a0
void DynomiteFossil::GetPowerUpCenter(int* x, int* y)
{
	*x = mPlayMiddleX - 0xc + mPlayHalfWidth;
	*y = (int) ((double) mKroney->mY - 4.0);
}

// 0x4085d0: any button; unlike the board's it fires while paused
void DynomiteFossil::MouseDown(int x, int y, int theClickCount)
{
	if ((mThunderbolt == NULL) && (mFireBlocked != true) && (mLosing == false) && (mGameOver == false))
	{
		mLauncher->Fire();
		if (gBoardGlobal5304a0 == 0)
		{
			mApp->mMusicInterface->FadeIn(mSong, mSongOffset, 0.002, false);
			mApp->mMusicInterface->PlayMusic(4, 0x54, false);
			mApp->mMusicInterface->SetSongVolume(4, 0.0);
			gBoardGlobal5304a0 = 1;
		}
		mFiredThisRound = true;
		mPlaying = true;
	}
}

// 0x408bf0
void DynomiteFossil::Draw(Graphics* g)
{
	if (mMuseum != NULL)
	{
		// port: the original draws into the Graphics of the widget drawn last, which is this frame's at the same
		// address; here it is set
		SetDrawGraphics(g);
		SetDrawOffset(Point(0, 0));
		mMuseum->Draw();
		mApp->mPicMenuoverlap->Draw(0, 0);
		return;
	}
	if (((mBonusBits & 0x70) == 0) && (mApp->mGrindInstance->IsPlaying() != true))
	{
		SetDrawOffset(Point(mShakeX, mShakeY));
		mShakeX = mShakeX / 2;
		mShakeY = mShakeY / 2;
	}
	else
	{
		int aShakeX = mApp->mRand.NextNoAssert() % 3 - 1;
		int aShakeY = mApp->mRand.NextNoAssert() % 3 - 1;
		SetDrawOffset(Point(aShakeX, aShakeY));
	}
	Dynomite::Draw(g);
	if ((mGameOver == true) && (mGameOverShown == false))
	{
		Graphics* aGraphics = GetDrawGraphics();
		aGraphics->SetColorizeImages(true);
		aGraphics->SetColor(Color(0, 0, 0, mGameOverAlpha));
		Pic* aPic = mApp->mPicGameover;
		aPic->Draw((mPlayMiddleX - (aPic->mWidth >> 1)) + 3, 0xf3 - (aPic->mHeight >> 1));
		aGraphics->SetColor(Color(0xff, 0xff, 0xff, mGameOverAlpha));
		aPic = mApp->mPicGameover;
		aPic->Draw(mPlayMiddleX - (aPic->mWidth >> 1), 0xf0 - (aPic->mHeight >> 1));
		aGraphics->SetColorizeImages(false);
	}
	SetDrawOffset(Point(0, 0));
}

// 0x4091f0
void DynomiteFossil::SetupRound(int theLevel, int theRound)
{
	int anIndex = theLevel * 3 - 4 + theRound;
	const char* aLevel;
	if (mPanicKind == true)
		aLevel = GTC(gPanicLevels[anIndex]);
	else
		aLevel = GTC(gStrategicLevels[anIndex]);
	mWidth = LevelInt(aLevel, "WIDTH") << 4;
	mRoundName = gFossilRoundNames[theLevel];
	// port: the copies are bounded; the original copies the whole string
	if (theRound == 1)
		strncpy(mFragmentText, mApp->GetString("FRAGMENT_NUM_1").c_str(), sizeof(mFragmentText) - 1);
	else if (theRound == 2)
		strncpy(mFragmentText, mApp->GetString("FRAGMENT_NUM_2").c_str(), sizeof(mFragmentText) - 1);
	else if (theRound == 3)
		strncpy(mFragmentText, mApp->GetString("FRAGMENT_NUM_3").c_str(), sizeof(mFragmentText) - 1);
	mFragmentText[sizeof(mFragmentText) - 1] = '\0';
	mFiredThisRound = false;
	mTextAlpha = 0;
	mTextTimer = 500;
}

// 0x4094b0
void DynomiteFossil::SetFossil(int theLevel, int theRound)
{
	mFossil->Free();
	mShownFossil = theLevel;
	if (theRound == 1)
		mShownChannels = 1;
	else if (theRound == 2)
		mShownChannels = 3;
	else if (theRound == 3)
		mShownChannels = 7;
	mFossil->SetFossil(theLevel);
	// port: the original is a switch on the level with the positions and the glow's colour in its cases
	if ((theLevel >= 1) && (theLevel <= 15))
	{
		if ((theRound >= 1) && (theRound <= 3))
		{
			mFossil->mFragment->AlphaFromColour(1 << (theRound - 1));
			mFossil->SetPos(gFragmentPos[theLevel - 1][theRound - 1][0], gFragmentPos[theLevel - 1][theRound - 1][1]);
		}
		Color aColor;
		if (theLevel == 1)
			aColor = Color(0xff, 0xc0, 0xc0, 0xff);
		else if (theLevel == 3)
			aColor = Color(0xc0, 0xff, 0xc0, 0xff);
		else if (theLevel == 4)
			aColor = Color(0xff, 0xc0, 0xff, 0xff);
		else
			aColor = Color(0xc0, 0xc0, 0xff, 0xff);
		mFossil->MakeGlowFor(aColor);
	}
	mFossilName = gFossilNames[theLevel];
	mGlowAlpha = 0x80;
	mGlowStep = 2;
}

// 0x408680
void DynomiteFossil::Fill(int theColors, int theRows)
{
	int i = 0;
	mGrid->mMaxColor = theColors;
	if (0 < theRows)
	{
		do
		{
			mGrid->AddRow(i);
			i++;
		} while (i < theRows);
	}
	mGrid->ResetCeiling();
}

// 0x4086d0: under the launcher's middle
void DynomiteFossil::Pipe(int theColors, int theWidth, int theRows)
{
	mGrid->mMaxColor = theColors;
	int aRow = 0;
	mGrid->mCeiling = 0.0f;
	GridPoint aPoint;
	mGrid->ScreenToGrid(mPlayMiddleX + 0x10, 100, &aPoint);
	GridPoint aCell;
	EggGrid::PixelToCell(aPoint.mX, aPoint.mY, &aCell);
	int aWidth = mPlayWidth;
	if (0 < theRows)
	{
		do
		{
			int aCol = aCell.mX - theWidth / 2;
			if (0 < theWidth)
			{
				int i = theWidth;
				do
				{
					if ((-1 < aCol) && (aCol < aWidth / 0x21))
					{
						EggGrid* aGrid = mGrid;
						unsigned int aRandom = mApp->mRand.NextNoAssert();
						aGrid->Place(aCol, aRow, aRandom % (unsigned int) aGrid->mMaxColor + 1, 0);
					}
					aCol++;
					i--;
				} while (i != 0);
			}
			aRow++;
		} while (aRow < theRows);
	}
	mGrid->ResetCeiling();
}

// 0x4087d0: from the top, a cell wider each row and half a cell further left every other row
void DynomiteFossil::Triangle(int theColors, int theWidth, int theRows)
{
	mGrid->mMaxColor = theColors;
	GridPoint aPoint;
	mGrid->ScreenToGrid(mPlayMiddleX, 0, &aPoint);
	GridPoint aCell;
	EggGrid::PixelToCell(aPoint.mX, aPoint.mY, &aCell);
	int aWidth = mPlayWidth;
	int aStart = aCell.mX - theWidth / 2;
	int aRow = 0;
	if (0 < theRows)
	{
		do
		{
			int aCount = theWidth + aRow;
			int aCol = aStart;
			if (0 < aCount)
			{
				do
				{
					if ((-1 < aCol) && (aCol < aWidth / 0x21))
					{
						EggGrid* aGrid = mGrid;
						unsigned int aRandom = mApp->mRand.NextNoAssert();
						aGrid->Place(aCol, aRow, aRandom % (unsigned int) aGrid->mMaxColor + 1, 0);
					}
					aCount--;
					aCol++;
				} while (aCount != 0);
			}
			if (aRow % 2 == 0)
				aStart--;
			aRow++;
		} while (aRow < theRows);
	}
	mGrid->ResetCeiling();
}

// 0x4088e0: the same from the bottom row up
void DynomiteFossil::ITriangle(int theColors, int theWidth, int theRows)
{
	mGrid->mMaxColor = theColors;
	GridPoint aPoint;
	mGrid->ScreenToGrid(mPlayMiddleX, 100, &aPoint);
	GridPoint aCell;
	EggGrid::PixelToCell(aPoint.mX, aPoint.mY, &aCell);
	int aWidth = mPlayWidth;
	int aStart = aCell.mX - theWidth / 2;
	int aCount = theWidth;
	while (--theRows >= 0)
	{
		int aCol = aStart;
		if (0 < aCount)
		{
			int i = aCount;
			do
			{
				if ((-1 < aCol) && (aCol < aWidth / 0x21))
				{
					EggGrid* aGrid = mGrid;
					unsigned int aRandom = mApp->mRand.NextNoAssert();
					aGrid->Place(aCol, theRows, aRandom % (unsigned int) aGrid->mMaxColor + 1, 0);
				}
				aCol++;
				i--;
			} while (i != 0);
		}
		if (theRows % 2 == 0)
			aStart--;
		aCount++;
	}
	mGrid->ResetCeiling();
}

// 0x40aed0
void DynomiteFossil::StartRound(int theLevel, int theRound)
{
	mFossilFell = false;
	mFallAlpha = 0;
	mFireBlocked = false;
	// the fragments found so far show on the museum's picture of the last fossil
	if (mShownFossil != -1)
		mApp->mFossils[mShownFossil - 1]->AlphaFromColour((unsigned char) mShownChannels);
	mFirstShotFade = 0;
	mPlaying = false;
	mBonusText = "";
	mResizing = false;
	SetFossil(theLevel, theRound);
	mGrid->mCeiling = 0.0f;
	mPlayWidth = mWidth;
	mPlayHalfWidth = mWidth / 2;
	mCeilingGoal = 0;
	mCeilingAtGoal = false;
	mFirePrompt->mAlpha = 1;
	mGrid->Clear();
	mFallSpeed = 0.02f;
	mFallSpeedUp = 0.01f;
	int anIndex = theLevel * 3 - 4 + theRound;
	char aSetup[28];
	int aBonus;
	if (mPanicKind == false)
	{
		const char* aLevel = GTC(gStrategicLevels[anIndex]);
		LevelWord(aLevel, "SETUP", aSetup, sizeof(aSetup));
		int aColors = LevelInt(aLevel, "COLORS");
		int anEggWidth = LevelInt(aLevel, "EGGWIDTH");
		int aRows = LevelInt(aLevel, "ROWS");
		aBonus = LevelInt(aLevel, "BONUS");
		int aKroney = LevelInt(aLevel, "KRONEY");
		int aFX = LevelInt(aLevel, "FX");
		int aFY = LevelInt(aLevel, "FY");
		if (strcmp(aSetup, "PIPE") == 0)
			Pipe(aColors, anEggWidth, aRows);
		if (strcmp(aSetup, "FILL") == 0)
			Fill(aColors, aRows);
		if (strcmp(aSetup, "TRIANGLE") == 0)
			Triangle(aColors, anEggWidth, aRows);
		if (strcmp(aSetup, "ITRIANGLE") == 0)
			ITriangle(aColors, anEggWidth, aRows);
		mKroney->Start((float) aKroney);
		mFossil->Place(mGrid, aFX, aFY);
	}
	else
	{
		const char* aLevel = GTC(gPanicLevels[anIndex]);
		LevelWord(aLevel, "SETUP", aSetup, sizeof(aSetup));
		int aColors = LevelInt(aLevel, "COLORS");
		int anEggWidth = LevelInt(aLevel, "EGGWIDTH");
		int aRows = LevelInt(aLevel, "ROWS");
		aBonus = LevelInt(aLevel, "BONUS");
		int aKroney = LevelInt(aLevel, "KRONEY");
		int aFX = LevelInt(aLevel, "FX");
		int aFY = LevelInt(aLevel, "FY");
		double aFallSpeed = LevelFloat(aLevel, "FALLSPEED");
		double aFallSpeedUp = LevelFloat(aLevel, "FALLSPEEDUP");
		if (strcmp(aSetup, "PIPE") == 0)
			Pipe(aColors, anEggWidth, aRows);
		if (strcmp(aSetup, "FILL") == 0)
			Fill(aColors, aRows);
		if (strcmp(aSetup, "TRIANGLE") == 0)
			Triangle(aColors, anEggWidth, aRows);
		if (strcmp(aSetup, "ITRIANGLE") == 0)
			ITriangle(aColors, anEggWidth, aRows);
		mKroney->Start((float) aKroney);
		mFossil->Place(mGrid, aFX, aFY);
		mFallSpeed = (float) aFallSpeed;
		mFallSpeedUp = (float) aFallSpeedUp;
	}
	mNextEggKind = aBonus;
	mGrid->mCeilingDrawn = false;
	mGrid->ResetCeiling();
	EggGrid* aGrid = mGrid;
	int aFactor = 8 - aGrid->mMaxColor;
	if (aFactor < 1)
		aFactor = 1;
	aGrid->mBabyEggChance = aGrid->mBaseBabyEggChance * aFactor;
}

// 0x40c5f0
void DynomiteFossil::Init(FossilizedApp* theApp, int theDifficulty)
{
	Dynomite::Init(theApp, 2);
	gFossilRoundNames = mApp->GetStringVector("gFossilRoundNames");
	gFossilNames = mApp->GetStringVector("gFossilNames");
	if (theDifficulty == 1)
		mPanicKind = true;
	mFirePrompt->mOwnLine1 = mApp->GetString("BREAK_THE_FOSSIL_OUT");
	mFirePrompt->mOwnLine2 = mApp->GetString("OF_THE_EGGS_TO_ADVANCE");
	// every fossil hidden in the museum until its fragments are found
	int i = 0;
	do
	{
		if (mApp->mFossils[i] != NULL)
			mApp->mFossils[i]->AlphaFromColour(0);
		i++;
	} while (i < 20);
	mFirstLevel = 1;
	mPlayWidth = 0x17f;
	mPlayHalfWidth = 0xbf;
	mFossil = new Fossil(mApp);
	mKroney = new Kroney();
	mKroney->mHurry = true;
	mKroney->mBoard = this;
	mKroney->mApp = theApp;
	mKroney->Start(60.0f);
	SetupRound(mLevel, mRound);
	StartRound(mLevel, mRound);
	int aBonus = mNextEggKind;
	Egg* anEgg = NextEgg();
	mLauncher->Load(anEgg->mColor);
	delete anEgg;
	mPushes = 0;
	mNextEggKind = aBonus;
}

// 0x409df0: the fragment where the grid drew its cell, in the scenery's offset; the rest at (0, 0)
void DynomiteFossil::DrawScenery()
{
	if (mFossilX != -1)
	{
		Graphics* g = GetDrawGraphics();
		mFossil->mFragment->Draw(mFossilX - mFossil->mX, mFossilY - mFossil->mY);
		g->SetColorizeImages(true);
		g->SetColor(Color(0xff, 0xff, 0xff, mGlowAlpha));
		mFossil->mGlow->Draw(mFossilX - mFossil->mX, mFossilY - mFossil->mY);
		g->SetColorizeImages(false);
	}
	mFossilX = -1;
	Point anOffset = GetDrawOffset();
	mApp->mPicLeftwall->Draw((mPlayMiddleX - mPlayHalfWidth) - 0x38, 0);
	mApp->mPicRightwall->Draw(mPlayHalfWidth - 0x1e + mPlayMiddleX, 0);
	mSteggy->Draw();
	mTank->Draw();
	mApp->mPicEggpilesheet_c1->Draw(mLauncherOffsetX - 0x110 + mPlayMiddleX, 0x185);
	mApp->mPicEggpilesheet_c1->Draw(mLauncherOffsetX - 0x1a6 + mPlayMiddleX, 0x185);
	mApp->mPicEggpilesheet_c0->Draw(mLauncherOffsetX + 0x50 + mPlayMiddleX, 400);
	mApp->mPicEggpilesheet_c0->Draw(mLauncherOffsetX + 0xe6 + mPlayMiddleX, 400);
	mPowerUps.ResetIter();
	PowerUp* aPowerUp = mPowerUps.Next();
	while (aPowerUp != NULL)
	{
		aPowerUp->Draw(mPowerUpSpin);
		aPowerUp = mPowerUps.Next();
	}
	if (mThunderbolt != NULL)
		mThunderbolt->Draw();
	SetDrawOffset(Point(0, 0));
	mKroney->Draw();
	Graphics* g = GetDrawGraphics();
	if (mFallAlpha != 0)
	{
		g->SetColorizeImages(true);
		g->SetColor(Color(0, 0, 0, mFallAlpha));
		Pic* aPic = mApp->mPicCleared;
		aPic->Draw((mPlayMiddleX + 3) - (aPic->mWidth >> 1), 0xb2 - (aPic->mHeight >> 1));
		g->SetColor(Color(0xff, 0xff, 0xff, mFallAlpha));
		aPic = mApp->mPicCleared;
		aPic->Draw(mPlayMiddleX - (aPic->mWidth >> 1), 0xaf - (aPic->mHeight >> 1));
		g->SetColorizeImages(false);
		if (mBonusText.compare("") != 0)
		{
			// centred by the width of the whole bonus line
			int aWidth = mApp->mAnnounceFont1928->Width(mApp->GetString("FALLEN_FOSSIL_BONUS").c_str());
			g->SetColorizeImages(true);
			g->SetColor(Color(0, 0, 0, 0xff));
			mApp->mAnnounceFont1928->Draw(mBonusText.c_str(), (mPlayMiddleX - aWidth / 2) + 4, 0xdb);
			g->SetColor(Color(0xff, 0xff, 0, 0xff));
			mApp->mAnnounceFont1928->Draw(mBonusText.c_str(), (mPlayMiddleX - aWidth / 2) + 4, 0xd7);
			g->SetColorizeImages(false);
		}
	}
	if ((mTextAlpha != 0) && (mApp->mInTutorial == false))
	{
		g->SetColorizeImages(true);
		int aNameWidth = mApp->mAnnounceFont1928->Width(mRoundName.c_str());
		int aFragmentWidth = mApp->mAnnounceFont1928->Width(mFragmentText);
		g->SetColor(Color(0, 0, 0, mTextAlpha));
		mApp->mAnnounceFont1928->Draw(mRoundName.c_str(), (mPlayMiddleX - (aNameWidth >> 1)) + 3, 0x71);
		mApp->mAnnounceFont1928->Draw(mFragmentText, (mPlayMiddleX - (aFragmentWidth >> 1)) + 3, 0x8a);
		g->SetColor(Color(0xff, 0xff, 0xff, mTextAlpha));
		mApp->mAnnounceFont1928->Draw(mRoundName.c_str(), mPlayMiddleX - (aNameWidth >> 1), 0x6e);
		mApp->mAnnounceFont1928->Draw(mFragmentText, mPlayMiddleX - (aFragmentWidth >> 1), 0x87);
		g->SetColorizeImages(false);
	}
	mApp->mPicMenuoverlap->Draw(0, 0);
	SetDrawOffset(anOffset);
}

// 0x40a770: the panic kind's ceiling, the kick, Kroney's push, the prompt's fade
void DynomiteFossil::UpdateRound()
{
	mBonusText = "";
	if ((mPanicKind == true) && (mFiredThisRound == true) && (mCeilingAtGoal == true) && (mGrid->mPastLineFrames == 0) &&
		((mBonusBits & 0x10) == 0))
	{
		mGrid->mCeiling = (float) ((double) mFallSpeed + mGrid->mCeiling);
		if (mGrid->mCeiling > -10.0f)
		{
			mGrid->PushDown();
			mGrid->mCeiling = (float) ((double) mGrid->mCeiling - 66.0f);
		}
	}
	if (mGrid->mAlarmDue == true)
	{
		mGrid->mAlarmDue = false;
		mApp->mSndAooga->Play();
	}
	if (mKick != 0.0f)
	{
		mGrid->mCeiling = (float) ((double) mGrid->mCeiling - mKick);
		double aKick = (double) mKick * (double) 0.95f;
		mKick = (float) aKick;
		if (aKick < 1.0)
			mKick = 0.0f;
	}
	if (0 < mPushes)
	{
		EggGrid* aGrid = mGrid;
		if (aGrid->mPastLineFrames == 0)
		{
			aGrid->mCeiling = (float) ((double) aGrid->mCeiling + 1.0f);
			if (mGrid->mCeiling > -10.0f)
			{
				mGrid->PushDown();
				mGrid->mCeiling = (float) ((double) mGrid->mCeiling - 66.0f);
			}
		}
		mPushes--;
	}
	if (mFiredThisRound == false)
	{
		mFirstShotFade = mFirstShotFade + 3;
		if (0xff < mFirstShotFade)
			mFirstShotFade = 0xff;
	}
	else if (0 < mFirstShotFade)
	{
		mFirstShotFade = mFirstShotFade - 3;
		if (mFirstShotFade < 0)
			mFirstShotFade = 0;
	}
}

// 0x40a410: the eggs run off; "cleared" fades in, then the bonus line by line, the points at 200 frames, and the
// museum at 400
void DynomiteFossil::FossilFell()
{
	if (mFallAlpha == 0)
	{
		mApp->mSndChime->Play();
		mFallTimer = 0;
		if (mRound == 3)
		{
			// a fossil's last round takes the power-ups away
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
		}
	}
	if (0xfa < mFallAlpha)
		mFallTimer++;
	mBonusText = "";
	if ((0x31 < mFallTimer) && (mMuseum == NULL))
	{
		if (mFallTimer < 0x82)
		{
			if (mFallTimer < 0x5a)
				mBonusText = mApp->GetString("FALLEN");
			else
				mBonusText = mApp->GetString("FALLEN_FOSSIL");
		}
		else
			mBonusText = mApp->GetString("FALLEN_FOSSIL_BONUS");
		if ((mFallTimer == 0x32) || (mFallTimer == 0x5a) || (mFallTimer == 0x82))
			mApp->mSndInchypop->Play();
		if (mFallTimer == 200)
		{
			AddPoints(mPlayMiddleX, 0x10e, (mRound == 3) ? 200 : 100, 0, true);
			mApp->mSndInchypop->Play();
		}
	}
	if (mMuseum == NULL)
	{
		mFallAlpha = mFallAlpha + 5;
		if (0xff < mFallAlpha)
			mFallAlpha = 0xff;
	}
	else
	{
		mFallAlpha = mFallAlpha - 5;
		if (mFallAlpha < 1)
			mFallAlpha = 1;
	}
	if (-0x42 < mGrid->mBottom)
		mGrid->mCeiling = (float) ((double) mGrid->mCeiling - 2.0f);
	if (mFallTimer == 400)
	{
		mMuseum = new Museum(mApp, this);
		mMuseum->Start();
	}
}

// 0x40b360: "cleared" fades out while the walls grind to the next round's width
void DynomiteFossil::Resizing()
{
	if (0 < mFallAlpha)
	{
		mFallAlpha = mFallAlpha - 5;
		if (mFallAlpha < 0)
			mFallAlpha = 0;
	}
	int aWidth = mPlayWidth;
	int aTarget = mWidth;
	if (aWidth != aTarget)
	{
		if (aTarget < aWidth)
			mPlayWidth = aWidth - 1;
		if (mPlayWidth < aTarget)
			mPlayWidth = mPlayWidth + 1;
		if (mApp->mGrindInstance->IsPlaying() == false)
			mApp->mGrindInstance->Play(true, false);
		mPlayHalfWidth = mPlayWidth / 2;
		return;
	}
	if (mFallAlpha == 0)
	{
		if (mApp->mGrindInstance->IsPlaying() == true)
			mApp->mGrindInstance->Stop();
		StartRound(mLevel, mRound);
	}
}

// 0x40a930: after the last fragment the game is won; after any other the next round sets up
void DynomiteFossil::NextRound()
{
	if (mMuseum == NULL)
		return;
	if (mMuseum->mRoundEnd == true)
	{
		if ((mLevel == 0xf) && (mRound == 3))
		{
			mGameOver = true;
			mGameOverDelay = 1;
			mApp->mMusicInterface->StopMusic(mSong);
			mApp->mMusicInterface->SetSongVolume(4, 1.0);
			mApp->mMusicInterface->PlayMusic(4, 0x40, false);
			mGameOverAlpha = 0;
			mTalker = new MenuTalker(mApp, this, 2);
		}
		else
		{
			mResizing = true;
			mBonusText = "";
			mRound++;
			if (3 < mRound)
			{
				mFossilsAssembled++;
				mRound = 1;
				mLevel++;
			}
			mRoundCount++;
			mGrid->Clear();
			SetupRound(mLevel, mRound);
		}
	}
	Museum* aMuseum = mMuseum;
	mMuseumScroll = aMuseum->mScroll;
	if (aMuseum != NULL)
		delete aMuseum;
	mMuseum = NULL;
}

// 0x40ad90: the museum opens straight up; it closes unless the tyrannosaurus is out (at a round's end closing it
// goes on to the next round)
void DynomiteFossil::ToggleMuseum()
{
	Museum* aMuseum = mMuseum;
	if (aMuseum == NULL)
	{
		mMuseum = new Museum(mApp, this);
		mMuseum->mY = 0;
		mMuseum->SetScroll(mMuseumScroll);
		MarkDirty();
	}
	else if (aMuseum->mRex == NULL)
	{
		if (aMuseum->mRoundEnd == true)
		{
			NextRound();
			return;
		}
		mMuseumScroll = aMuseum->mScroll;
		delete aMuseum;
		mMuseum = NULL;
	}
}

// 0x40ae70: the board's keys without its own call
void DynomiteFossil::KeyDown(KeyCode theKey)
{
	if ((theKey == KEYCODE_SPACE) || (theKey == KEYCODE_ESCAPE))
		mApp->mSideBar->SetPauseState(0);
	if ((theKey == KEYCODE_F1) || (theKey == (KeyCode) 'P'))
		mApp->mSideBar->SetPauseState(0);
	// the original tests 0x6d, which as a key code is the keypad's minus, and M
	if ((theKey == KEYCODE_SUBTRACT) || (theKey == (KeyCode) 'M'))
		ToggleMuseum();
}

// 0x40b420
void DynomiteFossil::Update()
{
	if (mApp->mActive == false)
		return;
	if (mMuseum != NULL)
		mMuseum->Update();
	if (mGameOverShown == true)
	{
		UpdateGameOver();
		return;
	}
	if (0 < mPauseCount)
		return;
	if (mMuseum != NULL)
		return;
	if (((mBonusBits & 0x70) == 0) && (mPlayWidth == mWidth))
	{
		if (mApp->mGrindInstance->IsPlaying() == true)
			mApp->mGrindInstance->Stop();
	}
	else if (mApp->mGrindInstance->IsPlaying() == false)
		mApp->mGrindInstance->Play(true, false);
	EggGrid* aGrid = mGrid;
	if (((mBonusBits & 0x10) != 0) && (aGrid->mBottom > -0x2b))
		aGrid->mCeiling = (float) ((double) aGrid->mCeiling - (double) ((aGrid->mCeiling < 0.0f) ? 0.05f : 0.1f));
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
	int aGlow = mGlowAlpha;
	mGlowAlpha = aGlow + mGlowStep;
	if (0xfe < aGlow + mGlowStep)
	{
		mGlowAlpha = 0xff;
		mGlowStep = -2;
	}
	if (mGlowAlpha < 0x81)
		mGlowStep = 2;
	// the round's name shows for 500 frames before the first shot
	if (mFiredThisRound == false)
	{
		mTextTimer--;
		if (0 < mTextTimer)
		{
			if (mTextAlpha < 0xff)
			{
				mTextAlpha = mTextAlpha + 5;
				if (0xff < mTextAlpha)
					mTextAlpha = 0xff;
			}
		}
		else
		{
			mTextTimer = 0;
			mTextAlpha = mTextAlpha - 1;
			if (mTextAlpha < 0)
				mTextAlpha = 0;
		}
	}
	else if (0 < mTextAlpha)
	{
		mTextAlpha = mTextAlpha - 5;
		if (mTextAlpha < 0)
			mTextAlpha = 0;
	}
	if (mGameOverDelay == 0)
	{
		SetPaused(true);
		if (mHighScores->IsHighScore(mApp->mSideBar->mScore, (int) mStartTicks) == true)
			mApp->ShowHighScoreDialog(mHighScores);
	}
	if (mKroney->Update() == true)
	{
		// the sand ran out: the eggs come down a row, or in the panic kind the ceiling speeds up
		GameSound* aSound;
		if (mPanicKind == false)
		{
			mPushes = 0x21;
			aSound = mApp->mSndFootfall;
		}
		else
		{
			mFallSpeed = (float) ((double) mFallSpeedUp + mFallSpeed);
			aSound = mApp->mSndSpeedup;
		}
		aSound->Play();
	}
	if (mLosing != false)
	{
		if (mGameOver == false)
		{
			// the eggs run down off the board, then "game over"
			mGrid->mCeiling = (float) ((double) mGrid->mCeiling + 32.0f);
			if (mGrid->mCeiling > 496.0f)
			{
				mGrid->mCeiling = 496.0f;
				mGameOver = true;
				mShakeY = 0x3c;
				mApp->mSndFootlands->Play();
				mApp->mMusicInterface->StopMusic(mSong);
				mApp->mMusicInterface->SetSongVolume(4, 1.0);
				mApp->mMusicInterface->PlayMusic(4, 0x23, false);
				mGameOverAlpha = 0;
				mTalker = new MenuTalker(mApp, this, 2);
				return;
			}
		}
		else
		{
			mGameOverAlpha = mGameOverAlpha + 2;
			if (0xff < mGameOverAlpha)
				mGameOverAlpha = 0xff;
		}
		return;
	}
	if (mResizing == true)
	{
		Resizing();
		return;
	}
	if (mFossilFell != true)
	{
		UpdateRound();
		return;
	}
	FossilFell();
}

// 0x40b8f0: each line has a black shadow two pixels down and right; the numbers are right-aligned
void DynomiteFossil::DrawGameOver(Graphics* g)
{
	std::vector<std::string> aComboNames = mApp->GetStringVector("ComboNames");
	Dynomite::DrawGameOver(g);
	g->SetFont(mApp->mWhiteFontSmall);
	char aCombo[32];
	strcpy(aCombo, aComboNames[mLongestChain].c_str());
	int aComboWidth = g->GetFont()->StringWidth(aCombo);
	char aScore[16];
	sprintf(aScore, "%d", mApp->mSideBar->mScore);
	int aScoreWidth = g->GetFont()->StringWidth(aScore);
	char aFossils[16];
	sprintf(aFossils, "%d", mFossilsAssembled);
	int aFossilsWidth = g->GetFont()->StringWidth(aFossils);
	char aBabies[16];
	sprintf(aBabies, "%d", mBabies);
	int aBabiesWidth = g->GetFont()->StringWidth(aBabies);
	g->SetColor(Color(0, 0, 0));
	g->DrawString(mApp->GetString("BEST_COMBO"), 0xe3, mPanelTop + 0x57);
	g->DrawString(mApp->GetString("BABIES_HATCHED"), 0xe3, mPanelTop + 0x6b);
	g->DrawString(mApp->GetString("FOSSILS_ASSEMBLED"), 0xe3, mPanelTop + 0x7f);
	g->DrawString(mApp->GetString("FINAL_SCORE"), 0xe3, mPanelTop + 0x93);
	g->DrawString(aCombo, 0x1b7 - aComboWidth, mPanelTop + 0x57);
	g->DrawString(aFossils, 0x1b7 - aFossilsWidth, mPanelTop + 0x7f);
	g->DrawString(aScore, 0x1b7 - aScoreWidth, mPanelTop + 0x93);
	g->DrawString(aBabies, 0x1b7 - aBabiesWidth, mPanelTop + 0x6b);
	g->SetColor(Color(0xe6, 0xe6, 0xff));
	g->DrawString(mApp->GetString("BEST_COMBO"), 0xe1, mPanelTop + 0x55);
	g->DrawString(aCombo, 0x1b5 - aComboWidth, mPanelTop + 0x55);
	g->DrawString(mApp->GetString("BABIES_HATCHED"), 0xe1, mPanelTop + 0x69);
	g->DrawString(mApp->GetString("FOSSILS_ASSEMBLED"), 0xe1, mPanelTop + 0x7d);
	g->DrawString(aFossils, 0x1b5 - aFossilsWidth, mPanelTop + 0x7d);
	g->DrawString(aBabies, 0x1b5 - aBabiesWidth, mPanelTop + 0x69);
	g->SetColor(Color(0xff, 0xff, 0xff));
	g->DrawString(mApp->GetString("FINAL_SCORE"), 0xe1, mPanelTop + 0x91);
	g->DrawString(aScore, 0x1b5 - aScoreWidth, mPanelTop + 0x91);
}

// 0x408330: the ceiling's pushes and the rounds played are written twice, as the original does
void DynomiteFossil::Save()
{
	Dynomite::Save();
	mBinFile.WriteFloat(mKick);
	mBinFile.WriteInt(mFirstLevel);
	mBinFile.WriteInt(mLevel);
	mBinFile.WriteInt(mRound);
	mBinFile.WriteInt(mFossilsAssembled);
	mBinFile.WriteInt(mShownFossil);
	mBinFile.WriteInt(mShownChannels);
	mKroney->SaveState(&mBinFile);
	mBinFile.WriteInt(mFirstShotFade);
	mBinFile.WriteInt(mPushes);
	mBinFile.WriteInt(mRoundCount);
	mBinFile.WriteInt(mPushes);
	mBinFile.WriteInt(mRoundCount);
	mBinFile.WriteBool(mFossilFell);
	mBinFile.WriteInt(mFallAlpha);
	mBinFile.WriteInt(mFallTimer);
	mBinFile.WriteBool(mResizing);
	mBinFile.WriteInt(mWidth);
	mBinFile.WriteInt(mMuseumScroll);
	mBinFile.WriteBool(mPanicKind);
	mBinFile.WriteFloat(mFallSpeed);
	mBinFile.WriteFloat(mFallSpeedUp);
	if (mMuseum == NULL)
	{
		mBinFile.WriteBool(false);
		return;
	}
	mBinFile.WriteBool(true);
	mMuseum->SaveState(&mBinFile);
}

// 0x40aae0: song 4 starts silent at the museum's part, as with the first shot (MouseDown); an open museum comes
// back, and the fossils before this one are shown whole
void DynomiteFossil::Load()
{
	Dynomite::Load();
	mTextTimer = 500;
	if (gBoardGlobal5304a0 == 1)
	{
		mApp->mMusicInterface->PlayMusic(4, 0x54, false);
		mApp->mMusicInterface->SetSongVolume(4, 0.0);
	}
	mKick = mBinFile.ReadFloat();
	mFirstLevel = mBinFile.ReadInt();
	mLevel = mBinFile.ReadInt();
	mRound = mBinFile.ReadInt();
	mFossilsAssembled = mBinFile.ReadInt();
	mShownFossil = mBinFile.ReadInt();
	mShownChannels = mBinFile.ReadInt();
	mKroney->LoadState(&mBinFile);
	mFirstShotFade = mBinFile.ReadInt();
	mPushes = mBinFile.ReadInt();
	mRoundCount = mBinFile.ReadInt();
	mPushes = mBinFile.ReadInt();
	mRoundCount = mBinFile.ReadInt();
	mFossilFell = mBinFile.ReadBool();
	mFallAlpha = mBinFile.ReadInt();
	mFallTimer = mBinFile.ReadInt();
	mResizing = mBinFile.ReadBool();
	mWidth = mBinFile.ReadInt();
	SetFossil(mLevel, mRound);
	mMuseumScroll = mBinFile.ReadInt();
	mPanicKind = mBinFile.ReadBool();
	mFallSpeed = mBinFile.ReadFloat();
	mFallSpeedUp = mBinFile.ReadFloat();
	if (mBinFile.ReadBool() == true)
	{
		mApp->mMusicInterface->StopAllMusic();
		mApp->mMusicInterface->PlayMusic(4, 0x54, false);
		mApp->mMusicInterface->SetSongVolume(4, 0.0);
		mMuseum = new Museum(mApp, this);
		mMuseum->LoadState(&mBinFile);
		mApp->mMusicInterface->SetSongVolume(mSong, 0.0);
	}
	int aFossil = mFirstLevel;
	if (aFossil < mLevel)
	{
		do
		{
			mApp->mFossils[aFossil - 1]->AlphaFromColour(7);
			aFossil++;
		} while (aFossil < mLevel);
	}
	int aChannels = 0;
	if (mRound == 2)
		aChannels = 1;
	if (mRound == 3)
		aChannels = mRound;
	mApp->mFossils[mLevel - 1]->AlphaFromColour(aChannels);
}
