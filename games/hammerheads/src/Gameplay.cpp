#include "Gameplay.h"

#include "Application.h"
#include "Creature.h"
#include "Cubby.h"
#include "Floater.h"
#include "GameScreen.h"
#include "GnomeBouncing.h"
#include "GnomeFlying.h"
#include "Hearts.h"
#include "LevelRules.h"
#include "Res.h"
#include "Stats.h"
#include "Timer.h"

using namespace Sexy;

static ObjectRef gGameplayRef;		// 0x597f40

// 0x435460
Gameplay::Gameplay() :
	Object(0),
	mPhase(PHASE_PLAYING),
	mPhaseTimer(0.0),
	mNextPopTimer(0.0),
	mCloverTimer(0.0),
	mStrawberryTimer(0.0),
	mDoubleHitTimer(0.0),
	mFiveTimesTimer(0.0),
	mPopNow(true),
	mFirstCubbyStarted(false),
	mChallengeShown(false),
	mChallengeDone(false),
	mFlagCC(false),
	mCloverDelay(0.0),
	mFiveTimesDelay(0.0),
	mDoubleHitDelay(0.0),
	mStrawberryDelay(0.0),
	mBackdrop(nullptr),
	mCubbyBack(nullptr),
	mPlatform(nullptr),
	mCrosshair(nullptr)
{
}

// 0x435570: the original's own factory names the object with a blank name
static Gameplay* MakeGameplay()
{
	Gameplay* aGameplay = new Gameplay();
	aGameplay->SetName("");
	gGameplayRef.Set(aGameplay);
	return aGameplay;
}

// port: the object when it has been made, else null (the state log of phase one looks without making it)
Gameplay* Sexy::PeekGameplay()
{
	return gGameplayRef.IsValid() ? static_cast<Gameplay*>(gGameplayRef.Get()) : nullptr;
}

// 0x435650
Gameplay* Sexy::GetGameplay()
{
	if (!gGameplayRef.IsValid())
		MakeGameplay()->SetTheme();
	return static_cast<Gameplay*>(gGameplayRef.Get());
}

// 0x434fc0
Image* Gameplay::GetBackdrop() const
{
	return mBackdrop;
}

// 0x434fd0
Image* Gameplay::GetCubbyBackImage() const
{
	return mCubbyBack;
}

// 0x434fe0
Image* Gameplay::GetPlatformImage() const
{
	return mPlatform;
}

// 0x434ff0
Image* Gameplay::GetCrosshair() const
{
	if (gApp->IsChallengeOpen() || gApp->IsShopOpen())
		return IMAGE_CROSSHAIR;
	return mCrosshair;
}

// 0x434de0: a level's season, by its number; the levels of the Classic mode go through seven in turn from the first
// (spring), those of Tough Cookie start at the fifth (the tarmac); Marathon has its own
void Gameplay::SetTheme()
{
	int aMode = gApp->mGameScreen->mMode;
	int aLevel = gApp->mGameScreen->mLevel;
	if (aMode == 0)
		aLevel = aLevel - 1;
	else
	{
		if (aMode != 1)
		{
			mBackdrop = IMAGE_MARATHON_BACKGROUND;
			mCubbyBack = IMAGE_MARATHON_CUBBY_BACK;
			mPlatform = IMAGE_MARATHON_CUBBY_PLATFORM;
			mCrosshair = IMAGE_CROSSHAIR_SNOW;
			return;
		}
		aLevel = aLevel + 4;
	}

	switch (aLevel % 7)
	{
	case 0:
		mBackdrop = IMAGE_BACKDROP_SPRING_1;
		mCubbyBack = IMAGE_CUBBY_BACK;
		mPlatform = IMAGE_CUBBY_PLATFORM;
		mCrosshair = IMAGE_CROSSHAIR;
		break;
	case 1:
		mBackdrop = IMAGE_BACKDROP_SUMMER_1;
		mCubbyBack = IMAGE_CUBBY_BACK_SUMMER;
		mPlatform = IMAGE_CUBBY_PLATFORM_SUMMER;
		mCrosshair = IMAGE_CROSSHAIR_SNOW;
		break;
	case 2:
		mBackdrop = IMAGE_BACKDROP_DESERT;
		mCubbyBack = IMAGE_CUBBY_BACK_SUMMER;
		mPlatform = IMAGE_CUBBY_PLATFORM_DESERT;
		mCrosshair = IMAGE_CROSSHAIR;
		break;
	case 3:
		mBackdrop = IMAGE_BACKDROP_AUTUMN_1;
		mCubbyBack = IMAGE_CUBBY_BACK;
		mPlatform = IMAGE_CUBBY_PLATFORM_AUTUMN;
		mCrosshair = IMAGE_CROSSHAIR;
		break;
	case 4:
		mBackdrop = IMAGE_BACKDROP_TARMAC;
		mCubbyBack = IMAGE_CUBBY_BACK_TARMAC;
		mPlatform = IMAGE_CUBBY_PLATFORM_TARMAC;
		mCrosshair = IMAGE_CROSSHAIR;
		break;
	case 5:
		mBackdrop = IMAGE_BACKDROP_WINTER_1;
		mCubbyBack = IMAGE_CUBBY_BACK_WINTER;
		mPlatform = IMAGE_CUBBY_PLATFORM_WINTER;
		mCrosshair = IMAGE_CROSSHAIR_SNOW;
		break;
	case 6:
		mBackdrop = IMAGE_BACKDROP_LIVINGROOM;
		mCubbyBack = IMAGE_CUBBY_BACK_LIVINGROOM;
		mPlatform = IMAGE_CUBBY_PLATFORM_LIVINGROOM;
		mCrosshair = IMAGE_CROSSHAIR;
		break;
	}
}

// 0x42d660
bool Gameplay::IsStrawberryActive() const
{
	return mStrawberryTimer > 0.0;
}

// 0x434c80
bool Gameplay::IsPlaying() const
{
	return mPhase == PHASE_PLAYING || mPhase == PHASE_DRAINING;
}

// 0x435030
void Gameplay::StartEnding()
{
	if (gApp->mGameMode == GAME_MODE_MARATHON)
	{
		mPhase = PHASE_PAUSING;
		mPhaseTimer = 0.0;
		return;
	}
	if (gApp->mGameMode == GAME_MODE_CLASSIC || gApp->mGameMode == GAME_MODE_TOUGH_COOKIE)
		mPhase = PHASE_DRAINING;
}

// 0x435090
void Gameplay::SetPlaying()
{
	mPhase = PHASE_PLAYING;
}

// 0x4350a0
bool Gameplay::IsChallengeDue() const
{
	if (gApp->mGameScreen->mMode != 0 && gApp->mGameScreen->mMode != 1)
		return false;
	if (gApp->mGameScreen->mLevel % CHALLENGE_LEVEL_INTERVAL == 0)
	{
		if (!GetRulesSheet()->IsLastRow(gApp->mGameScreen->mLevel - 1))
			return true;
	}
	return false;
}

// 0x435120
bool Gameplay::IsChallengeWaiting() const
{
	return !mChallengeDone && mChallengeShown;
}

// 0x435140
void Gameplay::SetChallengeDone()
{
	mChallengeDone = true;
}

// 0x4347c0
Cubby* Gameplay::GetCubby(int theIndex) const
{
	return dynamic_cast<Cubby*>(mCubbies[theIndex].Get());
}

// 0x4347f0
int Gameplay::CountCubbiesWithKind(int theKind) const
{
	int aCount = 0;
	for (int i = 0; i < (int)mCubbies.size(); i++)
	{
		Cubby* aCubby = GetCubby(i);
		if (aCubby->mCreature != nullptr && GetCubby(i)->mCreature->mKind == theKind)
			aCount++;
	}
	return aCount;
}

// 0x434950
int Gameplay::CountReady() const
{
	int aCount = 0;
	for (int i = 0; i < (int)mCubbies.size(); i++)
	{
		if (GetCubby(i)->mState == Cubby::STATE_EMPTY)
			aCount++;
		else if (GetCubby(i)->mState == Cubby::STATE_SINKING && GetCubby(i)->mCreature == nullptr)
			aCount++;
	}
	return aCount;
}

// 0x434a10
int Gameplay::CountInState(int theState) const
{
	int aCount = 0;
	for (int i = 0; i < (int)mCubbies.size(); i++)
	{
		if (GetCubby(i)->mState == theState)
			aCount++;
	}
	return aCount;
}

// 0x434a80
int Gameplay::CountEmpty() const
{
	return CountInState(Cubby::STATE_EMPTY);
}

// 0x434a90
int Gameplay::PickEmptyCubby() const
{
	std::vector<int> aEmpty;
	for (int i = 0; i < (int)mCubbies.size(); i++)
	{
		if (GetCubby(i)->mState == Cubby::STATE_EMPTY)
			aEmpty.push_back(i);
	}
	if (aEmpty.empty())
		return -1;
	return aEmpty[Rand() % (int)aEmpty.size()];
}

// 0x434ca0
void Gameplay::PopRandomCubby()
{
	int anIndex = PickEmptyCubby();
	if (anIndex != -1)
	{
		GetCubby(anIndex)->SetState(Cubby::STATE_RISING, -1);
		if (0.0 < GetStats()->Get(STAT_FREEZE_TIMER))
		{
			if (GetCubby(anIndex)->mCreature != nullptr)
				Cubby::FreezeOrdinary(GetCubby(anIndex)->mCreature);
		}
	}
}

// 0x434d60
void Gameplay::PopCubbies(int theCount)
{
	while (0 < theCount)
	{
		PopRandomCubby();
		theCount--;
	}
}

// 0x434d10
bool Gameplay::NoBouncingOrFlying() const
{
	int aStart = 0;
	if (ObjectManager::Get()->FindByType<GnomeBouncing>(&aStart) != nullptr)
		return false;
	aStart = 0;
	return ObjectManager::Get()->FindByType<GnomeFlying>(&aStart) == nullptr;
}

// 0x4351b0
int Gameplay::CountLimited() const
{
	int aCount = 0;
	for (int i = 0; i < (int)mCubbies.size(); i++)
	{
		if (GetCubby(i)->mState != Cubby::STATE_EMPTY)
		{
			if (GetCubby(i)->mCreature != nullptr && GetCubby(i)->mCreature->mCountsTowardLimit)
				aCount++;
		}
	}
	return aCount;
}

// 0x435210
Creature* Gameplay::FindCreatureOfKind(int theKind) const
{
	for (int i = 0; i < (int)mCubbies.size(); i++)
	{
		if (GetCubby(i)->mState != Cubby::STATE_EMPTY)
		{
			Creature* aCreature = GetCubby(i)->mCreature;
			if (aCreature != nullptr && aCreature->mKind == theKind)
				return aCreature;
		}
	}
	return nullptr;
}

// 0x435270
bool Gameplay::AllReady() const
{
	return CountReady() == (int)mCubbies.size();
}

// 0x435350
void Gameplay::FixedUpdate()
{
	Object::FixedUpdate();

	// a cubby that was deleted is taken off the list
	for (int i = 0; i < (int)mCubbies.size(); i++)
	{
		if (!mCubbies[i].IsValid())
		{
			mCubbies.erase(mCubbies.begin() + i);
			i--;
		}
	}

	if ((mTickCount % 20 == 0 && mNextPopTimer <= 0.0) || mPopNow)
	{
		int aLimit = gApp->mGameScreen->GetMaxCreatures();
		int aRoom = aLimit - CountLimited();
		int anEmpty = CountEmpty();
		if (anEmpty < aRoom)
			aRoom = anEmpty;
		if (0 < aRoom)
		{
			if (!mPopNow)
				mNextPopTimer = gApp->mGameScreen->GetPopDelay();
			else
				mNextPopTimer = 1e-05;
		}
		mPopNow = false;
	}

	if (mTickCount % 100 == 0)
		CheckTrophies(0);
}

// 0x435680
void Gameplay::GameOver()
{
	mPhase = PHASE_OVER;
	gApp->FadeOutMusic();
	MakeFloater(4, (double)(gSexyAppBase->mWidth / 2), (double)(gSexyAppBase->mHeight / 2), "Game Over", 0, "");
	gApp->mGameScreen->UpdateHover();
}

// port: Takes a timer that has a delay in front of it a step down: the delay runs first, then the timer
static void StepTimer(double& theTimer, double& theDelay, double theStep)
{
	if (theDelay <= 0.0)
	{
		double aValue = theTimer - theStep;
		if (aValue < 0.0)
			aValue = 0.0;
		theTimer = aValue;
	}
	else
	{
		double aValue = theDelay - theStep;
		if (aValue < 0.0)
			aValue = 0.0;
		theDelay = aValue;
	}
}

// 0x435770
void Gameplay::Update(float theFrac)
{
	Object::Update(theFrac);

	if (mPhase == PHASE_PLAYING || mPhase == PHASE_DRAINING)
	{
		if (gApp->mGameScreen->IsLevelRunning())
		{
			double aStep = (double)theFrac * 0.01;
			StepTimer(mCloverTimer, mCloverDelay, aStep);
			StepTimer(mStrawberryTimer, mStrawberryDelay, aStep);
			StepTimer(mDoubleHitTimer, mDoubleHitDelay, aStep);
			StepTimer(mFiveTimesTimer, mFiveTimesDelay, aStep);

			double aFreeze = GetStats()->Get(STAT_FREEZE_TIMER);
			if (0.0 < aFreeze)
			{
				aFreeze = aFreeze - aStep;
				if (aFreeze < 0.0)
					aFreeze = 0.0;
				GetStats()->Set(STAT_FREEZE_TIMER, aFreeze);
			}
		}
		if (GetPlayerHealth() == 0.0)
			GameOver();
	}

	if (mPhase == PHASE_DRAINING)
	{
		if (!mChallengeShown && IsChallengeDue() && CountReady() == (int)mCubbies.size())
		{
			mChallengeShown = true;
			PopRandomCubby();
		}
		if (CountReady() == (int)mCubbies.size() && (!IsChallengeDue() || mChallengeDone) && NoBouncingOrFlying())
		{
			mPhase = PHASE_PAUSING;
			mPhaseTimer = 1.0;
		}
	}
	else if (mPhase == PHASE_PAUSING)
	{
		mPhaseTimer = mPhaseTimer - (double)theFrac * 0.01;
		if (mPhaseTimer <= 0.0)
		{
			mPhase = PHASE_ENDED;
			gApp->LevelFinished();
		}
	}
	else if (mPhase == PHASE_PLAYING && !IsWaveTail() && 0.0 < mNextPopTimer)
	{
		mNextPopTimer = mNextPopTimer - (double)theFrac * 0.01;
		if (mNextPopTimer <= 0.0)
		{
			mNextPopTimer = 0.0;
			if (!mFirstCubbyStarted && gApp->mGameScreen->mLevel == 1)
			{
				mFirstCubbyStarted = true;
				GetCubby(0)->SetState(Cubby::STATE_RISING, 0);
			}
			else
				PopRandomCubby();
		}
	}

	if (mPhase == PHASE_PLAYING)
	{
		if (GetTimer()->GetProgress() == 1.0)
			StartEnding();
	}
}
