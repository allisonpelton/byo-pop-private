#include "GnomeShop.h"

#include "Application.h"
#include "Avatar.h"
#include "Res.h"
#include "ValueList.h"

using namespace Sexy;

// 0x43d3d0
GnomeShop::GnomeShop() :
	Creature(KIND_SHOP),
	mCenterX(0.0),
	mCenterY(0.0)
{
}

// 0x43d400
GnomeShop::~GnomeShop()
{
}

// 0x43d460: the sign lights for 170 steps in 200 (after the buzz)
void GnomeShop::GetSheetCell(double theCell[2]) const
{
	theCell[0] = 0.0;
	theCell[1] = (double)(29 < mAge % 200);
}

// 0x43d410
void GnomeShop::Update(const double thePos[2])
{
	Creature::Update(thePos);
	mCenterX = thePos[0];
	mCenterY = thePos[1];
	if (mAge % 200 == 30)
		gApp->PlaySample(SOUND_MISC_NEON_BUZZ);
}

// 0x43b2b0 (one function for six gnomes): the same as Update
void GnomeShop::UpdateWithoutCubby(const double thePos[2])
{
	Update(thePos);
}

// 0x437620 (one function for both): the gnome is gone, and spent rather than finished
void GnomeShop::Remove()
{
	mUntouchable = true;
	mSpent = true;
}

// 0x43d490
bool GnomeShop::Hit()
{
	bool aHit = Creature::Hit();
	if (aHit)
	{
		GetAvatar()->ClearHammerFade();
		gApp->DoShopScreen();
		gApp->PlaySample(SOUND_HAMMER_HIT_ROCK);
	}
	return aHit;
}

// 0x43d4f0
std::string GnomeShop::SaveExtra()
{
	ValueList aList;
	aList.Add(mCenterX);
	aList.Add(mCenterY);
	return aList.ToString();
}

// 0x43d590
void GnomeShop::LoadExtra(const std::string& theData)
{
	ValueList aList(theData);
	mCenterX = aList.NextDouble();
	mCenterY = aList.NextDouble();
}
