#include "GnomeBonus.h"

#include "Application.h"
#include "Cubby.h"
#include "Floater.h"
#include "Res.h"
#include "Score.h"
#include "Stats.h"
#include "ValueList.h"

using namespace Sexy;

// 0x435f40
GnomeBonus::GnomeBonus(int theKind) :
	Creature(theKind),
	mDrawX(0.0),
	mDrawY(0.0)
{
}

// 0x435f80
GnomeBonus::~GnomeBonus()
{
}

// 0x435f90
void GnomeBonus::Update(const double thePos[2])
{
	Creature::Update(thePos);
	mDrawX = thePos[0] - mAnchorX;
	mDrawY = (thePos[1] - mAnchorY) - mHeight;
}

// 0x435f90 (the function of Update)
void GnomeBonus::UpdateWithoutCubby(const double thePos[2])
{
	Update(thePos);
}

// 0x435fd0
void Sexy::StartIceCreamPower(double theSeconds)
{
	FreezeAll();
	gApp->PlaySample(SOUND_VARIOUS_ICECREAM_GLOOP);
	gApp->PlaySample(SOUND_MISC_ICE_FREEZE);
	GetStats()->Set(STAT_FREEZE_TIMER, theSeconds);
}

// 0x436040
bool GnomeBonus::Hit()
{
	bool aDestroyed = Creature::Hit();
	if (aDestroyed)
	{
		if (mKind == KIND_BONUS_ICE_CREAM)
		{
			gApp->PlaySample(SOUND_VARIOUS_ICECREAM_GLOOP);
			Rect aClip = mCubby->GetEffectRect();
			ScoreDisplayPos aSign = GetIceCreamSignPos();
			MakeFloaterShrinking(mDrawX, mDrawY, aSign.mX, aSign.mY, IMAGE_GNOME_BONUS_ICECREAM, 0, 0, mExposed, aClip.mX,
				aClip.mY, aClip.mWidth, aClip.mHeight);
			StartIceCreamPower(5.0);
		}
		else if (mKind == KIND_BONUS_DUCK)
		{
			gApp->PlaySample(SOUND_VARIOUS_DUCK_QUACK);
			return true;
		}
	}
	return aDestroyed;
}

// 0x436130
std::string GnomeBonus::SaveExtra()
{
	ValueList aList;
	aList.Add(mDrawX);
	aList.Add(mDrawY);
	return aList.ToString();
}

// 0x4361d0
void GnomeBonus::LoadExtra(const std::string& theData)
{
	ValueList aList(theData);
	mDrawX = aList.NextDouble();
	mDrawY = aList.NextDouble();
}
