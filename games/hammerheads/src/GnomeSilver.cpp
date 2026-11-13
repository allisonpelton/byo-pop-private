#include "GnomeSilver.h"

#include "Application.h"
#include "Cubby.h"
#include "Graphics.h"
#include "Res.h"
#include "ValueList.h"

using namespace Sexy;

// 0x43d630
GnomeSilver::GnomeSilver() :
	Creature(KIND_SILVER),
	mStage(0),
	mCenterX(0.0),
	mCenterY(0.0),
	mHasRisen(false),
	mFade(1.0)
{
}

// 0x43d680
GnomeSilver::~GnomeSilver()
{
}

// 0x43d7e0
bool GnomeSilver::IsFading() const
{
	return mStage == 6;
}

// 0x43d7f0
void GnomeSilver::GetSheetCell(double theCell[2]) const
{
	theCell[0] = 0.0;
	theCell[1] = (double)mStage;
}

// 0x43d840
void GnomeSilver::Update(const double thePos[2])
{
	Creature::Update(thePos);
	mCenterX = thePos[0];
	mCenterY = thePos[1];

	if (mStage == 6 && 0.0 < mFade)
	{
		double aFade = mFade - 0.1;
		mFade = aFade;
		if (aFade <= 0.0)
		{
			mFinished = true;
			mFade = 0.0;
		}
	}
	if (mCubby->mState == Cubby::STATE_UP && !mHasRisen)
		mHasRisen = true;
}

// 0x43d8e0: there is no shadow; the last stage is drawn with the fade as its alpha
void GnomeSilver::Draw(Graphics* g, double theX, double theY, int theHover, double theFlashTime)
{
	Graphics aG(*g);
	double aTopLeftX = theX - mAnchorX;
	double aTopLeftY = (theY - mAnchorY) - mHeight;
	double aAlpha = 1.0;
	if (mStage == 6)
	{
		aG.SetColorizeImages(true);
		aAlpha = mFade;
	}
	int aFlash = FlashAlpha(theFlashTime);
	DrawBody(&aG, aTopLeftX, aTopLeftY, mStage, 0, aFlash, aAlpha);
	DrawHealth(&aG, theX, theY, mHealth);
}

// 0x43d690: each hit moves the spot where the next one lands, to where the armor that is left is
bool GnomeSilver::Hit()
{
	bool aHit = Creature::Hit();
	switch (mHealth)
	{
	case 0:
		gApp->PlaySample(SOUND_IRON_GNOME_STRIKE_5);
		break;
	case 1:
		mHeadX = 49.0;
		mHeadY = 95.0;
		gApp->PlaySample(SOUND_IRON_GNOME_STRIKE_4);
		break;
	case 2:
		mHeadX = 57.0;
		mHeadY = 69.0;
		gApp->PlaySample(SOUND_IRON_GNOME_STRIKE_3);
		break;
	case 3:
		mHeadX = 46.0;
		mHeadY = 45.0;
		gApp->PlaySample(SOUND_IRON_GNOME_STRIKE_2);
		break;
	case 4:
		mHeadX = 62.0;
		mHeadY = 25.0;
		gApp->PlaySample(SOUND_IRON_GNOME_STRIKE_1);
		break;
	case 5:
		mHeadX = 38.0;
		mHeadY = 15.0;
		gApp->PlaySample(SOUND_IRON_GNOME_STRIKE_0);
		break;
	}
	mStage = 6 - mHealth;
	return aHit;
}

// 0x43d7b0: it is not finished at once; unless it is frozen the last stage fades it out first
void GnomeSilver::Remove()
{
	if (IsFrozen())
	{
		mUntouchable = true;
		mFinished = true;
		return;
	}
	mUntouchable = true;
}

// 0x43d9d0
std::string GnomeSilver::SaveExtra()
{
	ValueList aList;
	aList.Add(mCenterX);
	aList.Add(mCenterY);
	aList.AddInt(mStage);
	aList.AddBool(mHasRisen);
	return aList.ToString();
}

// 0x43da90
void GnomeSilver::LoadExtra(const std::string& theData)
{
	ValueList aList(theData);
	mCenterX = aList.NextDouble();
	mCenterY = aList.NextDouble();
	mStage = aList.NextInt();
	mHasRisen = aList.NextBool();
}
