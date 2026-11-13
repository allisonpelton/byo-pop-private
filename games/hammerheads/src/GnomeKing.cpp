#include "GnomeKing.h"

#include "Application.h"
#include "Cubby.h"
#include "Floater.h"
#include "Gameplay.h"
#include "Image.h"
#include "Object.h"
#include "Popper.h"
#include "Res.h"
#include "Stats.h"
#include "Timer.h"
#include "ValueList.h"

using namespace Sexy;

// 0x591b54: the kinds he may carry (-1: a bonus drawn when it is asked for)
static const int KING_ITEMS[9] = { KIND_GNOME, KIND_WOMAN, KIND_HIDE, KIND_STRONG, -1, KIND_BUBBLEHEAD, KIND_SWITCHER, KIND_SILVER, KIND_NAPOLEON };

// 0x43b840: the arrival is quiet (not counted, no timer, no sting) for the king the intro pages show
GnomeKing::GnomeKing(bool theForIntro) :
	Creature(KIND_KING),
	mFrame(0.0),
	mFrameSpeed(0.0),
	mStage(STAGE_WAITING),
	mTimer(3.0),
	mCenterX(0.0),
	mCenterY(0.0),
	mItemCount(9),
	mItemIndex(0),
	mFlash(0.0),
	mHasRisen(false)
{
	if (!theForIntro)
	{
		GetStats()->AddInt(STAT_KINGS_RELEASED, 1);
		GetTimer()->Setup(3);
		gApp->PlaySample(SOUND_STING_GNOME_KING);
	}
}

// 0x43b930
GnomeKing::~GnomeKing()
{
}

// 0x43b970: the first row, the second while he flashes
void GnomeKing::GetSheetCell(double theCell[2]) const
{
	bool aFlashing = mFlash != 0.0;
	switch (mStage)
	{
	case STAGE_OPENING:
	case STAGE_CLOSING:
		theCell[0] = (double)aFlashing;
		theCell[1] = (double)(int)mFrame;
		return;
	case STAGE_OPEN:
		theCell[0] = (double)aFlashing;
		theCell[1] = 9.0;
		return;
	default:
		theCell[0] = (double)aFlashing;
		theCell[1] = 0.0;
		return;
	}
}

// 0x43ba10
int GnomeKing::GetItemKind() const
{
	int aKind = KING_ITEMS[mItemIndex];
	if (aKind == -1)
	{
		switch (Rand() % 4)
		{
		case 0: return KIND_RABBIT;
		case 1: return KIND_BONUS_DUCK;
		case 2: return KIND_BONUS_ICE_CREAM;
		case 3: aKind = KIND_STRAWBERRY;
		}
	}
	return aKind;
}

// 0x43bca0
void GnomeKing::Update(const double thePos[2])
{
	Creature::Update(thePos);
	mCenterX = thePos[0];
	mCenterY = thePos[1];

	double aFlash = mFlash - 0.01;
	if (aFlash < 0.0)
		aFlash = 0.0;
	mFlash = aFlash;

	switch (mStage)
	{
	case STAGE_WAITING:
	{
		double aTimer = mTimer - 0.01;
		if (aTimer < 0.0)
			aTimer = 0.0;
		mTimer = aTimer;
		if (aTimer == 0.0)
		{
			mStage = STAGE_OPENING;
			mFrame = 0.0;
			gApp->PlaySample(SOUND_GNOME_KING_DRONING);
		}
		break;
	}
	case STAGE_OPENING:
	{
		double aLast = (double)(IMAGE_GNOME_KING->mNumCols - 1);
		double aNext = mFrame + 0.07;
		mFrame = (aNext <= aLast) ? aNext : aLast;
		if (mFrame == (double)(IMAGE_GNOME_KING->mNumCols - 1))
		{
			int aRand = Rand();
			mTimer = 3.0;
			mStage = STAGE_OPEN;
			mItemIndex = aRand % mItemCount;
			MakeFloater(FLOATER_CRYSTAL_GLOW, (double)(int)((thePos[0] - 41.0) - 3.0), (double)(int)((thePos[1] - 248.0) + 60.0),
				nullptr, 0, "");
			GetGameplay()->PopCubbies(7);
			gApp->PlaySample(SOUND_GNOME_KIND_FLASHING);
		}
		break;
	}
	case STAGE_OPEN:
	{
		double aTimer = mTimer - 0.01;
		if (aTimer < 0.0)
			aTimer = 0.0;
		mTimer = aTimer;
		if (aTimer == 0.0)
			mStage = STAGE_CLOSING;
		break;
	}
	case STAGE_CLOSING:
	{
		double aFrame = mFrame - 0.07;
		if (aFrame < 0.0)
			aFrame = 0.0;
		mFrame = aFrame;
		if (aFrame == 0.0)
		{
			mStage = STAGE_WAITING;
			mTimer = 3.0;
		}
		break;
	}
	}

	if (mCubby->mState == Cubby::STATE_UP && !mHasRisen)
		mHasRisen = true;
}

// 0x43bf90: a hit lights him up and sends sparks; the hit that brings him down breaks everything else up too and puts the glow out
bool GnomeKing::Hit()
{
	bool aDestroyed = Creature::Hit();
	mFlash = 1.0;
	gApp->PlaySample(SOUND_HAMMER_HIT_ROCK);

	double aDirection[2] = { 0.0, 0.1 };
	MakePopper(mCenterX, mCenterY - 140.0, 8, aDirection);

	if (aDestroyed)
	{
		MakeFloater(FLOATER_SCORE_POP, mCenterX, mCenterY, "Magnificent!", 0, "");
		ExplodeAll(this);

		int aIndex = 0;
		for (Floater* aFloater = ObjectManager::Get()->FindByType<Floater>(&aIndex); aFloater != nullptr;
			aFloater = ObjectManager::Get()->FindByType<Floater>(&aIndex))
		{
			if (aFloater->mType == FLOATER_CRYSTAL_GLOW)
				aFloater->Kill();
			aIndex++;
		}
		gApp->PlaySample(SOUND_BOMB_FACE_EXPLOSION);
		gApp->PlaySample(SOUND_BOMB_FACE_EXPLOSION);
	}
	return aDestroyed;
}

// 0x43baa0 (the item index is not saved)
std::string GnomeKing::SaveExtra()
{
	ValueList aList;
	aList.Add(mFrame);
	aList.Add(mFrameSpeed);
	aList.AddInt(mStage);
	aList.Add(mTimer);
	aList.Add(mCenterX);
	aList.Add(mCenterY);
	aList.Add(mFlash);
	aList.AddBool(mHasRisen);
	return aList.ToString();
}

// 0x43bbb0
void GnomeKing::LoadExtra(const std::string& theData)
{
	ValueList aList(theData);
	mFrame = aList.NextDouble();
	mFrameSpeed = aList.NextDouble();
	mStage = aList.NextInt();
	mTimer = aList.NextDouble();
	mCenterX = aList.NextDouble();
	mCenterY = aList.NextDouble();
	mFlash = aList.NextDouble();
	mHasRisen = aList.NextBool();
}
