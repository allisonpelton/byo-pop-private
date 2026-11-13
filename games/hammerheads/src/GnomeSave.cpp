#include "GnomeSave.h"

#include "Application.h"
#include "Avatar.h"
#include "Floater.h"
#include "GameScreen.h"
#include "Res.h"
#include "Stats.h"
#include "UserManager.h"
#include "ValueList.h"

using namespace Sexy;

// 0x43d000
GnomeSave::GnomeSave() :
	Creature(KIND_SAVE),
	mCenterX(0.0),
	mCenterY(0.0),
	mTimer(0.0)
{
}

// 0x43d040
GnomeSave::~GnomeSave()
{
}

// 0x43d050
void GnomeSave::Update(const double thePos[2])
{
	Creature::Update(thePos);
	mCenterX = thePos[0];
	mCenterY = thePos[1] - mHeight;
	if (0.0 < mTimer)
	{
		mTimer = mTimer - 0.01;
		if (!(mTimer != mTimer) && (mTimer < 0.0) != (mTimer == 0.0))
			mSpent = true;
	}
}

// 0x430030 (one function for both): the creature's own update, without the gnome's
void GnomeSave::UpdateWithoutCubby(const double thePos[2])
{
	Creature::Update(thePos);
}

// 0x43d0c0
void GnomeSave::Remove()
{
	mUntouchable = true;
	mTimer = 1.0;
}

// 0x426700: the photograph for a level; the five pictures cover five levels each (the exe's table goes on into other
// pictures from level 25 on, and no board has a photographer there)
static Image* PhotoImage(int theIndex)
{
	switch (theIndex)
	{
	case 0: return IMAGE_PHOTO01;
	case 1: return IMAGE_PHOTO02;
	case 2: return IMAGE_PHOTO03;
	case 3: return IMAGE_PHOTO04;
	default: return IMAGE_PHOTO05;
	}
}

// 0x43d260
bool GnomeSave::Hit()
{
	bool aHit = Creature::Hit();
	if (aHit)
	{
		GetAvatar()->ClearHammerFade();
		gApp->mUserManager->GetCurrentUser()->AddRestartPoint();
		MakeFloaterPhoto(mCenterX - 10.0, mCenterY + 25.0, PhotoImage(gApp->mGameScreen->mLevel / 5));
		MakeFloater(FLOATER_FLASH, mCenterX + 15.0, mCenterY - 30.0, nullptr, 0, "");
		gApp->PlaySample(SOUND_MISC_CAMERA_FLASH);
		GetStats()->ClearLevelCounts();
	}
	return aHit;
}

// 0x43d100
std::string GnomeSave::SaveExtra()
{
	ValueList aList;
	aList.Add(mCenterX);
	aList.Add(mCenterY);
	aList.Add(mTimer);
	return aList.ToString();
}

// 0x43d1b0
void GnomeSave::LoadExtra(const std::string& theData)
{
	ValueList aList(theData);
	mCenterX = aList.NextDouble();
	mCenterY = aList.NextDouble();
	mTimer = aList.NextDouble();
}
