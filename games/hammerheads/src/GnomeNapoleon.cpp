#include "GnomeNapoleon.h"

#include "Application.h"
#include "Cubby.h"
#include "Image.h"
#include "Popper.h"
#include "Res.h"
#include "ValueList.h"

using namespace Sexy;

// 0x43db40
GnomeNapoleon::GnomeNapoleon() :
	Creature(KIND_NAPOLEON),
	mHitStage(0),
	mShownStage(0),
	mCenterX(0.0),
	mCenterY(0.0),
	mHasRisen(false)
{
}

// 0x43db80
GnomeNapoleon::~GnomeNapoleon()
{
}

// 0x43dca0
void GnomeNapoleon::GetSheetCell(double theCell[2]) const
{
	theCell[0] = 0.0;
	theCell[1] = (double)mShownStage;
}

// 0x43db90
void GnomeNapoleon::Update(const double thePos[2])
{
	Creature::Update(thePos);
	mCenterX = thePos[0];
	mCenterY = thePos[1];
	if (mCubby->mState == Cubby::STATE_UP && !mHasRisen)
		mHasRisen = true;
	if (mAge % 8 == 0 && mShownStage < mHitStage)
		mShownStage = mShownStage + 1;
}

// 0x43dc00
bool GnomeNapoleon::Hit()
{
	bool aHit = Creature::Hit();
	gApp->PlaySample(SOUND_HAMMER_HIT_WOOD2);
	gApp->PlaySample(SOUND_VOICE_NAPOLEON_UGH);
	double aDirection[2] = { 0.0, -0.1 };
	MakePopper(mCenterX, mCenterY - 100.0, 7, aDirection);
	mHitStage = (IMAGE_GNOME_NAPOLEON->mNumCols - mHealth) - 1;
	return aHit;
}

// 0x43dcf0
std::string GnomeNapoleon::SaveExtra()
{
	ValueList aList;
	aList.Add(mCenterX);
	aList.Add(mCenterY);
	aList.AddInt(mHitStage);
	aList.AddInt(mShownStage);
	aList.AddBool(mHasRisen);
	return aList.ToString();
}

// 0x43ddc0
void GnomeNapoleon::LoadExtra(const std::string& theData)
{
	ValueList aList(theData);
	mCenterX = aList.NextDouble();
	mCenterY = aList.NextDouble();
	mHitStage = aList.NextInt();
	mShownStage = aList.NextInt();
	mHasRisen = aList.NextBool();
}
