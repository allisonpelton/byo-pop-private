#include "GnomeHeart.h"

#include "Application.h"
#include "Floater.h"
#include "GameScreen.h"
#include "Hearts.h"
#include "Res.h"
#include "Stats.h"
#include "ValueList.h"

using namespace Sexy;

// 0x43ad60
GnomeHeart::GnomeHeart() :
	Creature(KIND_HEART),
	mFrame(0.0),
	mFrameSpeed(0.55),
	mUnusedInt(0),
	mUnusedDouble(0.0),
	mHeartX(0.0),
	mHeartY(0.0)
{
	if (!gApp->mQuietSpawn)
		gApp->PlaySample(SOUND_HEART_APPEAR);
}

// 0x43ae00: a heart that goes down unhit is a missed one
GnomeHeart::~GnomeHeart()
{
	if (0 < mHealth)
		GetStats()->AddInt(STAT_HEARTS_MISSED, 1);
}

// 0x43aee0
void GnomeHeart::GetSheetCell(double theCell[2]) const
{
	theCell[0] = 0.0;
	theCell[1] = (double)((int)mFrame % 11);
}

// 0x43ae70
void GnomeHeart::Update(const double thePos[2])
{
	Creature::Update(thePos);
	mHeartX = thePos[0];
	mHeartY = thePos[1] - mHeight;
	mFrame = mFrame + mFrameSpeed;
	double aSpeed = mFrameSpeed - 0.001;
	if (aSpeed < 0.15)
		aSpeed = 0.15;
	mFrameSpeed = aSpeed;
}

// 0x43af10: a hit heart flies to the end of the health in the row, where it is added
bool GnomeHeart::Hit()
{
	bool aHit = Creature::Hit();
	if (aHit)
	{
		GetHearts()->ChangeHealth(HEALTH_GAIN, 1.0);

		double aEnd[2];
		GetHearts()->GetHealthEndPos(aEnd);
		MakeFloaterHeart(mHeartX, mHeartY - (mAnchorY - mHeadY) * 0.5, aEnd[0], aEnd[1], mFrame);

		if (GetStats()->GetInt(STAT_HEARTS_COLLECTED) == 0)
			GetStats()->SetInt(STAT_LEVEL_OF_FIRST_HEART, gApp->mGameScreen->mLevel);
		GetStats()->AddInt(STAT_HEARTS_COLLECTED, 1);
		GetStats()->SetInt(STAT_LEVEL_OF_LAST_HEART, gApp->mGameScreen->mLevel);
		gApp->PlaySample(SOUND_HEART_PICKED);
	}
	return aHit;
}

// 0x43b040
std::string GnomeHeart::SaveExtra()
{
	ValueList aList;
	aList.Add(mFrame);
	aList.Add(mFrameSpeed);
	aList.AddInt(mUnusedInt);
	aList.Add(mUnusedDouble);
	aList.Add(mHeartX);
	aList.Add(mHeartY);
	return aList.ToString();
}

// 0x43b130
void GnomeHeart::LoadExtra(const std::string& theData)
{
	ValueList aList(theData);
	mFrame = aList.NextDouble();
	mFrameSpeed = aList.NextDouble();
	mUnusedInt = aList.NextInt();
	mUnusedDouble = aList.NextDouble();
	mHeartX = aList.NextDouble();
	mHeartY = aList.NextDouble();
}
