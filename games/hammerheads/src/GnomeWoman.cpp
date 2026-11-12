#include "GnomeWoman.h"

#include "Application.h"
#include "Cubby.h"
#include "DrawUtil.h"
#include "Graphics.h"
#include "Res.h"
#include "ValueList.h"

using namespace Sexy;

// 0x43f5f0
GnomeWoman::GnomeWoman() :
	Creature(KIND_WOMAN),
	mFrame(0.0),
	mFrameSpeed(0.0),
	mStage(STAGE_CALM),
	mSmokeTime(0.0),
	mHasRisen(false)
{
}

// 0x43f640
GnomeWoman::~GnomeWoman()
{
}

// 0x43f8d0
void GnomeWoman::GetSheetCell(double theCell[2]) const
{
	if (mStage == STAGE_ANGRY)
	{
		theCell[0] = 1.0;
		theCell[1] = (double)(int)mFrame;
		return;
	}
	theCell[0] = 0.0;
	theCell[1] = 0.0;
}

// 0x43f650
void GnomeWoman::Update(const double thePos[2])
{
	Creature::Update(thePos);

	if (mStage == STAGE_ANGRY)
	{
		mSmokeTime = mSmokeTime + 0.05;
		double aFrame = mFrame + mFrameSpeed;
		mFrame = aFrame;
		if ((0.0 < mFrameSpeed && 10.0 <= aFrame) || (mFrameSpeed < 0.0 && aFrame <= 5.0))
		{
			double aSpeed = -mFrameSpeed;
			mFrameSpeed = aSpeed;
			mFrame = aSpeed + aSpeed + mFrame;
		}
	}
	if (!mHasRisen && mCubby->mState == Cubby::STATE_UP)
		mHasRisen = true;
}

// 0x43f7d0: the gnome, and the smoke over her ears while she is angry
void GnomeWoman::Draw(Graphics* g, double theX, double theY, int theHover, double theFlashTime)
{
	Creature::Draw(g, theX, theY, theHover, theFlashTime);
	if (mStage == STAGE_ANGRY)
	{
		DrawImageCelF(g, IMAGE_GNOME_FEMALE_SMOKE, (theX - mAnchorX) + 10.0, ((theY - mAnchorY) - mHeight) + 30.0,
			(int)mSmokeTime % 6, 0);
	}
}

// 0x43f860
bool GnomeWoman::Hit()
{
	bool aHit = Creature::Hit();
	if (mHealth == 1)
	{
		mStage = STAGE_ANGRY;
		mFrame = 0.0;
		mFrameSpeed = 0.18;
		gApp->PlaySample(SOUND_HAMMER_HIT_WOOD);
		gApp->PlaySample(SOUND_VOICE_WOMAN_HELABA);
	}
	return aHit;
}

// 0x43f940
std::string GnomeWoman::SaveExtra()
{
	ValueList aList;
	aList.Add(mFrame);
	aList.Add(mFrameSpeed);
	aList.AddInt(mStage);
	aList.Add(mSmokeTime);
	aList.AddBool(mHasRisen);
	return aList.ToString();
}

// 0x43fa10
void GnomeWoman::LoadExtra(const std::string& theData)
{
	ValueList aList(theData);
	mFrame = aList.NextDouble();
	mFrameSpeed = aList.NextDouble();
	mStage = aList.NextInt();
	mSmokeTime = aList.NextDouble();
	mHasRisen = aList.NextBool();
}
