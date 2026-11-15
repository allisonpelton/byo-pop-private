#include "Timer.h"

#include "Application.h"
#include "Floater.h"
#include "GameScreen.h"
#include "Gameplay.h"
#include "LevelRules.h"
#include "Stats.h"

using namespace Sexy;

static ObjectRef gTimerRef;		// 0x598060

// 0x44bd60
Timer::Timer() :
	Object(0x80),
	mTotal(0.0),
	mLeft(0.0),
	mRate(0.0),
	mPhase(TIMER_PHASE_WAVE)
{
	Setup(TIMER_SETUP_NEW_LEVEL);
	mRate = 0.01;
}

Timer::~Timer()
{
}

// 0x44bb90
bool Sexy::IsWaveResultShowing()
{
	ObjectManager* aManager = ObjectManager::Get();
	for (size_t i = 0; i < aManager->mObjects.size(); i++)
	{
		Floater* aFloater = dynamic_cast<Floater*>(aManager->mObjects[i]);
		if (aFloater != nullptr && (aFloater->mType == FLOATER_WAVE_RESULT || aFloater->mType == FLOATER_LEVEL_COMPLETE))
			return true;
	}
	return false;
}

// 0x44bbf0
void Timer::Drain()
{
	double aStep = mRate;
	if (mPhase == TIMER_PHASE_RUNNING_OUT && !IsWaveResultShowing())
		aStep = mRate * 10.0;

	mLeft = mLeft - aStep;
	if (mLeft < 0.0)
		mLeft = 0.0;
}

// 0x44bc50
Timer* Sexy::GetTimer()
{
	return dynamic_cast<Timer*>(gTimerRef.Get());
}

// 0x44bc80
void Timer::Setup(int theSetup)
{
	switch (theSetup)
	{
	case TIMER_SETUP_NEW_LEVEL:
		mTotal = (double)GetRulesSheet()->GetInt(gApp->mGameScreen->mLevel - 1, RULE_TIME);
		mLeft = mTotal;
		GetStats()->ClearBonusCounts();
		break;
	case TIMER_SETUP_RUN:
		mRate = 0.01;
		break;
	case TIMER_SETUP_STOP:
		mRate = 0.0;
		break;
	case TIMER_SETUP_ALMOST_OUT:
		mLeft = 0.001;
		break;
	}
}

// 0x44bd20
double Timer::GetProgress() const
{
	return 1.0 - mLeft / mTotal;
}

// 0x44bd40
double Timer::GetTimeUsed() const
{
	return mTotal - mLeft;
}

// 0x44be50
Timer* Sexy::MakeTimer()
{
	Timer* aTimer = new Timer();
	aTimer->SetName("");
	gTimerRef = ObjectRef(aTimer);
	return aTimer;
}

// 0x44bf30
void Timer::FixedUpdate()
{
	Object::FixedUpdate();
	if (mLeft <= 0.0)
		return;

	bool aInTail = IsWaveTail();
	if (mPhase == TIMER_PHASE_WAVE)
	{
		if (aInTail)
		{
			mPhase = TIMER_PHASE_CLEARING;
			return;
		}
	}
	else
	{
		if (mPhase == TIMER_PHASE_CLEARING)
		{
			if (!GetGameplay()->AllReady())
				return;
			if (!GetGameplay()->NoBouncingOrFlying())
				return;

			mPhase = TIMER_PHASE_RUNNING_OUT;
			if (GetStats()->GetInt(STAT_KINGS_DESTROYED) == 0)
			{
				Floater* aResult = MakeFloater(FLOATER_WAVE_RESULT, 0.0, 0.0, nullptr, 0, "");
				aResult->SetWaveResult(GetStats()->GetInt(STAT_MISSES_IN_LEVEL), GetStats()->GetInt(STAT_SWINGS_IN_LEVEL),
					GetStats()->GetInt(STAT_LEVEL_SCORE), GetWaveIndex());

				if ((gApp->mGameMode == GAME_MODE_CLASSIC || gApp->mGameMode == GAME_MODE_TOUGH_COOKIE) && GetWaveIndex() == 1)
				{
					CheckTrophies(1);
					Floater* aSummary = MakeFloater(FLOATER_LEVEL_COMPLETE, 0.0, 0.0, nullptr, 0, "");
					aSummary->SetLevelCounts(GetStats()->GetInt(STAT_ROUND_SMASHED), GetStats()->GetInt(STAT_BEST_STREAK),
						GetStats()->GetInt(STAT_ROUND_SPEED_HITS));
				}
			}
			GetStats()->ClearLevelCounts();
			return;
		}

		if (mPhase != TIMER_PHASE_RUNNING_OUT)
			return;
		if (!aInTail)
			mPhase = TIMER_PHASE_WAVE;
	}

	Drain();
}
