#include "GnomeChest.h"

#include <climits>

#include "Application.h"
#include "Coins.h"
#include "Floater.h"
#include "GameScreen.h"
#include "Image.h"
#include "Res.h"
#include "Stats.h"
#include "ValueList.h"

using namespace Sexy;

// 0x437790
GnomeChest::GnomeChest() :
	Creature(KIND_CHEST),
	mX(0.0),
	mY(0.0),
	mOpenX(0.0),
	mOpenY(0.0),
	mStage(STAGE_CLOSED),
	mTimer(0.0)
{
}

// 0x4377f0
GnomeChest::~GnomeChest()
{
}

// 0x4378d0: the first cel while it is closed, the second when it is open (a stage out of range gives the original's
// leftover value)
void GnomeChest::GetSheetCell(double theCell[2]) const
{
	int aCol;
	if (mStage == 0)
		aCol = 0;
	else if (mStage == 1 || mStage == 2)
		aCol = 1;
	else
		aCol = INT_MIN;

	int aRow;
	if (mStage == 0 || mStage == 1 || mStage == 2)
		aRow = 0;
	else
		aRow = INT_MIN;

	theCell[0] = (double)aRow;
	theCell[1] = (double)aCol;
}

// 0x437950
void GnomeChest::ThrowCoin()
{
	GetCoins()->AddCoins(1);

	int aCoinWidth = IMAGE_COIN->GetCelWidth();
	int aCoinHeight = IMAGE_COIN->GetCelHeight();
	double aCounter[2];
	GetCoinsPos(aCounter);
	double aCounterAgain[2];
	GetCoinsPos(aCounterAgain);

	MakeFloaterCoin(mOpenX - (double)(aCoinWidth / 2), (mOpenY - (mAnchorY - mHeadY) * 0.5) - (double)aCoinHeight, aCounter[0],
		aCounterAgain[1], 0.0, 0.0, 0, 0, 0, 0);
	gApp->PlaySample(SOUND_COIN_SINGLE_PICKED);
	GetStats()->AddInt(STAT_COINS_COLLECTED, 1);
	GetStats()->SetInt(STAT_LEVEL_OF_LAST_COIN, gApp->mGameScreen->mLevel);
}

// 0x437a70
void GnomeChest::Update(const double thePos[2])
{
	Creature::Update(thePos);
	mX = thePos[0];
	mY = thePos[1];

	if (mTimer > 0.0)
	{
		mTimer = mTimer - 0.01;
		if (mTimer <= 0.0)
		{
			mTimer = 0.0;
			if (mStage == STAGE_THROWING)
			{
				mStage = STAGE_CLOSING;
				mTimer = 0.5;
				mSpent = true;
			}
			else if (mStage == STAGE_CLOSING)
				mFinished = true;
		}

		// a coin every twenty hundredths of the time left
		if (mStage == STAGE_THROWING)
		{
			int aTime = (int)(mTimer * 100.0);
			if (aTime == 180 || aTime == 160 || aTime == 140 || aTime == 120 || aTime == 100 || aTime == 80 || aTime == 60 ||
				aTime == 40 || aTime == 20)
				ThrowCoin();
		}
	}
}

// 0x437820: a chest with a time on it opens when the time is out
void GnomeChest::UpdateWithoutCubby(const double thePos[2])
{
	Creature::Update(thePos);

	if (mTimer > 0.0)
	{
		double aTimer = mTimer - 0.01;
		if (aTimer < 0.0)
			aTimer = 0.0;
		mTimer = aTimer;
		if (aTimer == 0.0 && mStage == STAGE_CLOSED)
		{
			mStage = STAGE_THROWING;
			mTimer = 3.0;
			gApp->PlaySample(SOUND_TREASURE_CHEST_OPENING);
		}
	}
}

// 0x437b60: the first hit that breaks it opens it instead
void GnomeChest::Remove()
{
	mUntouchable = true;
	mTimer = 2.0;
	mStage = STAGE_THROWING;
	mOpenX = mX;
	mOpenY = mY;
	ThrowCoin();
	gApp->PlaySample(SOUND_TREASURE_CHEST_OPENING);
}

// 0x437bc0
std::string GnomeChest::SaveExtra()
{
	ValueList aList;
	aList.Add(mX);
	aList.Add(mY);
	aList.AddInt(mStage);
	aList.Add(mTimer);
	aList.Add(mOpenX);
	aList.Add(mOpenY);
	return aList.ToString();
}

// 0x437cb0
void GnomeChest::LoadExtra(const std::string& theData)
{
	ValueList aList(theData);
	mX = aList.NextDouble();
	mY = aList.NextDouble();
	mStage = aList.NextInt();
	mTimer = aList.NextDouble();
	mOpenX = aList.NextDouble();
	mOpenY = aList.NextDouble();
}
