#include "GnomeBomb.h"

#include "Application.h"
#include "GameScreen.h"
#include "Hearts.h"
#include "Popper.h"
#include "Res.h"
#include "Stats.h"
#include "ValueList.h"

using namespace Sexy;

// 0x435b70
GnomeBomb::GnomeBomb() :
	Creature(KIND_BOMB),
	mPulse(0.0),
	mCenterX(0.0),
	mCenterY(0.0),
	mFuse(0.0)
{
}

// 0x435bc0
GnomeBomb::~GnomeBomb()
{
}

// 0x435c80: the four cels of the first row, one after the other
void GnomeBomb::GetSheetCell(double theCell[2]) const
{
	theCell[0] = 0.0;
	theCell[1] = (double)((int)mPulse % 4);
}

// 0x435bd0
void GnomeBomb::Update(const double thePos[2])
{
	Creature::Update(thePos);

	mCenterX = thePos[0];
	mCenterY = thePos[1] - mHeight;
	mPulse = mPulse + 0.5;

	double aFuse = mFuse - 0.01;
	if (aFuse < 0.0)
		aFuse = 0.0;
	mFuse = aFuse;
	if (aFuse == 0.0 && !IsFrozen())
	{
		gApp->PlaySample(SOUND_BOMB_FACE_FUSE);
		mFuse = 1.0;
	}
}

// 0x435cc0: a bomb that is hit costs the player health (mVariant of it) and breaks up; a frozen one is worth points
// instead and costs nothing
bool GnomeBomb::Hit()
{
	bool aDestroyed = Creature::Hit();
	if (!IsFrozen())
	{
		GetStats()->AddInt(STAT_BOMBS_HIT, 1);
		GetStats()->SetInt(STAT_LEVEL_OF_LAST_BOMB, gApp->mGameScreen->mLevel);
		if (!aDestroyed)
			return aDestroyed;

		Hearts* aHearts = GetHearts();
		if (mVariant != 0 && aHearts != nullptr && aHearts->mActive)
			aHearts->ChangeHealth(HEALTH_ESCAPE, (double)mVariant);

		double aDirection[2] = { 0.0, 1.0 };
		MakePopper(mCenterX, mCenterY - (mAnchorY - mHeadY) * 0.5, 6, aDirection);
		mShatters = true;
	}

	if (aDestroyed && IsFrozen())
	{
		mScore = 500;
		return true;
	}
	return aDestroyed;
}

// 0x435e90
std::string GnomeBomb::SaveExtra()
{
	ValueList aList;
	aList.Add(mPulse);
	aList.Add(mCenterX);
	aList.Add(mCenterY);
	return aList.ToString();
}

// 0x435de0
void GnomeBomb::LoadExtra(const std::string& theData)
{
	ValueList aList(theData);
	mPulse = aList.NextDouble();
	mCenterX = aList.NextDouble();
	mCenterY = aList.NextDouble();
}
