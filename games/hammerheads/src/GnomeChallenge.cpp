#include "GnomeChallenge.h"

#include "Application.h"
#include "Avatar.h"
#include "Image.h"
#include "Res.h"
#include "ValueList.h"

using namespace Sexy;

// 0x437500
GnomeChallenge::GnomeChallenge() :
	Creature(KIND_CHALLENGE),
	mX(0.0),
	mY(0.0)
{
}

// 0x437530
GnomeChallenge::~GnomeChallenge()
{
}

// 0x4375a0: the cel changes every ten steps of its age
void GnomeChallenge::GetSheetCell(double theCell[2]) const
{
	theCell[0] = 0.0;
	theCell[1] = (double)((mAge / 10) % IMAGE_CHALLENGE->mNumCols);
}

// 0x437540: the sign buzzes once in every cycle of its cels, at the fortieth step
void GnomeChallenge::Update(const double thePos[2])
{
	Creature::Update(thePos);
	mX = thePos[0];
	mY = thePos[1];
	if (mAge % (IMAGE_CHALLENGE->mNumCols * 10) == 0x28)
		gApp->PlaySample(SOUND_MISC_NEON_BUZZ);
}

// 0x43b2b0 (one function for six gnomes): the same as Update
void GnomeChallenge::UpdateWithoutCubby(const double thePos[2])
{
	Update(thePos);
}

// 0x437620 (one function for both): the gnome is gone, and spent rather than finished
void GnomeChallenge::Remove()
{
	mUntouchable = true;
	mSpent = true;
}

// 0x4375e0
bool GnomeChallenge::Hit()
{
	bool aHit = Creature::Hit();
	if (aHit)
	{
		GetAvatar()->ClearHammerFade();
		gApp->DoChallengeScreen();
		gApp->PlaySample(SOUND_HAMMER_HIT_ROCK);
	}
	return aHit;
}

// 0x437650
std::string GnomeChallenge::SaveExtra()
{
	ValueList aList;
	aList.Add(mX);
	aList.Add(mY);
	return aList.ToString();
}

// 0x4376f0
void GnomeChallenge::LoadExtra(const std::string& theData)
{
	ValueList aList(theData);
	mX = aList.NextDouble();
	mY = aList.NextDouble();
}
