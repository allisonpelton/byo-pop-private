// EffectMgr.cpp: see EffectMgr.h.

#include "EffectMgr.h"

#include "Board.h"
#include "Common.h"
#include "NotTranslated.h"
#include "PhysObjFactory.h"

#include <cmath>

using namespace Sexy;

static const int MIN_PRIORITY = -1;				// the lowest and the highest priority an effect can have; its list is the priority plus 1
static const int MAX_PRIORITY = 3;
static const int EFFECT_DEFAULT_LIFE = 100;		// the updates an effect lasts until the one that makes it says otherwise
static const float EFFECT_MAX_FALL_SPEED = 10.0f;

// the effects of the shot's burst at the cannon's mouth (0x44efe0)
static const int EFFECT_TYPE_BURST_GLOW = 6;
static const int EFFECT_TYPE_BURST_SPARK = 0x28;
static const int EFFECT_TYPE_BURST_SMOKE = 0x29;

static const int BURST_DROP = 4;				// the burst is put this far below where the ball is (ModValInt, EffectMgr.cpp 701, 4009)
static const int BURST_GLOW_COUNT = 40;			// (702, 4011)
static const int BURST_SPARK_COUNT = 35;		// (717, 4036)
static const int BURST_SMOKE_COUNT = 35;		// (734, 4055)
static const double QUARTER_TURN = 1.5707963705062866; // the aim is measured from straight up
static const float BURST_GLOW_SPEED_BASE = 0.3f;	// (703, 4015)
static const float BURST_GLOW_SPEED_RANGE = 1.5f;	// (704, 4015)
static const double BURST_GLOW_SPREAD = 0.5;		// (705, 4016)
static const float BURST_GLOW_SPREAD_SPEED = 0.20000000298023224f; // how much a fast one's spread narrows
static const int BURST_GLOW_RADIUS = 10;			// (706, 4017)
static const int BURST_GLOW_JITTER_X = 10;			// (707, 4020)
static const int BURST_GLOW_SHIFT_X = 5;			// (708, 4020)
static const int BURST_GLOW_JITTER_Y = 10;			// (709, 4021)
static const int BURST_GLOW_SHIFT_Y = 5;			// (710, 4021)
static const int BURST_GLOW_PRIORITY = 2;			// (711, 4022)
static const int BURST_GLOW_COLOR = 0x994411;		// (712, 4024)
static const double BURST_GLOW_DRIFT_Y = -0.0;		// (713, 4030)
static const int BURST_GLOW_LIFE_BASE = 5;			// (714, 4032)
static const int BURST_GLOW_LIFE_RANGE = 30;		// (715, 4032)
static const float BURST_GLOW_GRAVITY = 0.1f;		// (716, 4033)
static const float GRAVITY_SCALE = 0.05f;			// (a constant of the game: every burst's gravity is its number times this)
static const double BURST_SPARK_SPREAD = 0.7;		// (718, 4039)
static const int BURST_SPARK_RADIUS = 20;			// (719, 4042)
static const int BURST_SPARK_RADIUS_BACK = 10;		// (720, 4042)
static const float BURST_SPARK_SPEED_BASE = 0.3f;	// (721, 4043)
static const float BURST_SPARK_SPEED_RANGE = 1.5f;	// (722, 4043)
static const int BURST_SPARK_JITTER_X = 10;			// (723, 4045)
static const int BURST_SPARK_SHIFT_X = 5;			// (724, 4045)
static const int BURST_SPARK_JITTER_Y = 10;			// (725, 4046)
static const int BURST_SPARK_SHIFT_Y = 5;			// (726, 4046)
static const int BURST_SPARK_PRIORITY = 2;			// (727, 4046)
static const int BURST_SPARK_COLOR = 0xaaaaaa;		// (728, 4048)
static const double BURST_SPARK_DRIFT_Y = -0.2;		// (729, 4050)
static const int BURST_SPARK_LIFE_RANGE = 10;		// (731, 4051)
static const int BURST_SPARK_LIFE_BASE = 30;		// (730, 4051)
static const int BURST_SPARK_LIFE_SPEED = 8;		// (732, 4051): the faster a spark, the shorter it lasts
static const float BURST_SPARK_GRAVITY = 0.1f;		// (733, 4052)
static const double BURST_SMOKE_SPREAD = 0.4;		// (735, 4058)
static const int BURST_SMOKE_RADIUS = 20;			// (736, 4061)
static const int BURST_SMOKE_RADIUS_BACK = 10;		// (737, 4061)
static const float BURST_SMOKE_SPEED_BASE = 0.1f;	// (738, 4062)
static const float BURST_SMOKE_SPEED_RANGE = 2.0f;	// (739, 4062)
static const int BURST_SMOKE_JITTER_X = 10;			// (740, 4064)
static const int BURST_SMOKE_SHIFT_X = 5;			// (741, 4064)
static const int BURST_SMOKE_JITTER_Y = 10;			// (742, 4065)
static const int BURST_SMOKE_SHIFT_Y = 5;			// (743, 4065)
static const int BURST_SMOKE_PRIORITY = 3;			// (744, 4065)
static const int BURST_SMOKE_COLOR_DARK = 0x404040;	// (745, 4071)
static const int BURST_SMOKE_COLOR_MID = 0x808080;	// (746, 4074)
static const int BURST_SMOKE_COLOR_LIGHT = 0xf0f0f0; // (747, 4077)
static const double BURST_SMOKE_DRIFT_Y = -0.2;		// (748, 4081)
static const int BURST_SMOKE_LIFE_RANGE = 10;		// (750, 4082)
static const int BURST_SMOKE_LIFE_BASE = 30;		// (749, 4082)
static const int BURST_SMOKE_LIFE_SPEED = 4;		// (751, 4082)
static const float BURST_SMOKE_GRAVITY = 0.1f;		// (752, 4083)

// 0x43e540 (the clearing is 0x432e00)
Effect::Effect()
{
	mLayer = 0;
	mType = 0;
	mAge = 0;
	mLife = 0;
	mX = 0;
	mY = 0;
	mVelocityX = 0;
	mVelocityY = 0;
	mGravity = 0;
	mUnknown28 = 0;
	mUnknown2C = 0;
	mUnknown30 = 0;
	mColor = 0;
	mUnknown38 = 0;
	mUnknown3C = 0;
	mUnknown40 = false;
	mUnknown44 = nullptr;
	mUnknown48 = nullptr;
	mUnknown4C = nullptr;
	mUnknown50 = nullptr;
	mUnknown54 = nullptr;
}

// 0x45b650: one update. The kinds that have an update of their own are not translated yet; the others only move, and fall when they have a gravity.
bool Effect::Update(bool theSlowMo)
{
	if (theSlowMo && mUnknown40)
		return true;
	mAge++;
	if (mAge < 0)
	{
		if (mType == 0x1d)
			NOT_TRANSLATED("Effect::UpdateExtremeFever");
		return true;
	}
	if (mLife < mAge && mLife != 0)
		return false;
	switch (mType)
	{
	case 4:
	case 7:
	case 8:
	case 9:
	case 0xe:
	case 0xf:
	case 0x11:
	case 0x14:
	case 0x16:
	case 0x1b:
	case 0x1d:
	case 0x1e:
	case 0x1f:
	case 0x20:
	case 0x24:
	case 0x2b:
	case 0x2e:
	case 0x2f:
	case 0x31:
		NOT_TRANSLATED("Effect::Update of a kind that has its own update");
		break;
	case 0x12:
		if (theSlowMo)
			return true;
		NOT_TRANSLATED("Effect::UpdateFeverSpark");
		break;
	}
	if (!theSlowMo)
	{
		mX = mVelocityX + mX;
		mY = mVelocityY + mY;
		if (0.0f < mGravity)
		{
			mVelocityY = mGravity + mVelocityY;
			if (EFFECT_MAX_FALL_SPEED < mVelocityY)
				mVelocityY = EFFECT_MAX_FALL_SPEED;
		}
	}
	return true;
}

// 0x45b870: an update in slow motion (theSubFrame of a normal one)
bool Effect::UpdateSlowMo(float theSubFrame)
{
	if (mUnknown40)
		return Update(false);
	if (-1 < mAge)
	{
		if (mType == 0x12)
			NOT_TRANSLATED("Effect::UpdateFeverSpark");
		mX = mX + theSubFrame * mVelocityX;
		mY = theSubFrame * mVelocityY + mY;
		if (0.0f < mGravity)
		{
			mVelocityY = theSubFrame * mGravity + mVelocityY;
			if (EFFECT_MAX_FALL_SPEED < mVelocityY)
				mVelocityY = EFFECT_MAX_FALL_SPEED;
		}
	}
	return true;
}

// 0x4471e0
EffectMgr::EffectMgr()
{
}

// 0x44de40
Effect& EffectMgr::AddEffect(int theType, float theX, float theY, int thePriority, bool theAtFront)
{
	int aPriority = thePriority < MIN_PRIORITY ? MIN_PRIORITY : (MAX_PRIORITY < thePriority ? MAX_PRIORITY : thePriority);
	std::list<Effect>& aList = mLists[aPriority + 1];
	std::list<Effect>::iterator anItr = aList.insert(theAtFront ? aList.begin() : aList.end(), Effect());
	anItr->mLayer = aPriority;
	anItr->mType = theType;
	anItr->mX = theX;
	anItr->mY = theY;
	anItr->mLife = EFFECT_DEFAULT_LIFE;
	return *anItr;
}

// 0x441650
Effect* EffectMgr::GetEffectByType(int theType)
{
	for (int aList = 0; aList < NUM_EFFECT_LISTS; aList++)
		for (std::list<Effect>::iterator anItr = mLists[aList].begin(); anItr != mLists[aList].end(); ++anItr)
			if (anItr->mType == theType)
				return &*anItr;
	return nullptr;
}

// 0x443720
void EffectMgr::Clear()
{
	for (int i = 0; i < NUM_EFFECT_LISTS; i++)
		mLists[i].clear();
}

// 0x45bf10
void EffectMgr::Update(bool theSlowMo)
{
	for (int aList = 0; aList < NUM_EFFECT_LISTS; aList++)
		for (std::list<Effect>::iterator anItr = mLists[aList].begin(); anItr != mLists[aList].end();)
		{
			if (anItr->Update(theSlowMo))
				++anItr;
			else
				anItr = mLists[aList].erase(anItr);
		}
}

// 0x45bfb0
void EffectMgr::UpdateSlowMo(float theSubFrame)
{
	for (int aList = 0; aList < NUM_EFFECT_LISTS; aList++)
		for (std::list<Effect>::iterator anItr = mLists[aList].begin(); anItr != mLists[aList].end();)
		{
			if (anItr->UpdateSlowMo(theSubFrame))
				++anItr;
			else
				anItr = mLists[aList].erase(anItr);
		}
}

// 0x443780
int EffectMgr::EraseAllOfType(int theType)
{
	int aNumErased = 0;
	for (int aList = 0; aList < NUM_EFFECT_LISTS; aList++)
		for (std::list<Effect>::iterator anItr = mLists[aList].begin(); anItr != mLists[aList].end();)
		{
			if (anItr->mType == theType)
			{
				aNumErased++;
				anItr = mLists[aList].erase(anItr);
			}
			else
				++anItr;
		}
	return aNumErased;
}

// 0x44efe0: the cannon's mouth throws out three kinds of sparks along the aim: glows, sparks and smoke. Every one takes its direction
// and speed from the random numbers, in the order the original draws them. The intermediate sums are kept at the original's precision.
void Sexy::AddShotBurst(float theX, float theY, float theAngle)
{
	Board* aBoard = GetBoard();
	if (aBoard == nullptr)
		return;
	EffectMgr* anEffects = aBoard->mEffectMgr.get();
	theY = (float)((double)BURST_DROP + theY);
	double aBase = (double)((double)theAngle + QUARTER_TURN);

	for (int i = 0; i < BURST_GLOW_COUNT; i++)
	{
		double aSpeedPart = (double)((double)(Rand() % 1000) * (double)BURST_GLOW_SPEED_RANGE / 1000.0);
		float aSpeed = (float)((double)BURST_GLOW_SPEED_BASE + aSpeedPart);
		double aSpread = (double)(1.0 - (double)(Rand() % 1000) / 500.0);
		float anAngle = (float)(((double)BURST_GLOW_SPREAD * (1.0 - (double)aSpeed * BURST_GLOW_SPREAD_SPEED)) * aSpread + aBase);
		float aRadius = (float)(Rand() % BURST_GLOW_RADIUS);
		float aCos = (float)std::cos((double)anAngle);
		double aBaseY = (double)((double)(Rand() % BURST_GLOW_JITTER_Y) + ((double)aCos * aRadius + theY));
		float aSin = (float)std::sin((double)anAngle);
		double aBaseX = (double)((double)(Rand() % BURST_GLOW_JITTER_X) + ((double)aSin * aRadius + theX));
		float aY = (float)((double)aBaseY - BURST_GLOW_SHIFT_Y);
		float aX = (float)((double)aBaseX - BURST_GLOW_SHIFT_X);
		Effect& anEffect = anEffects->AddEffect(EFFECT_TYPE_BURST_GLOW, aX, aY, BURST_GLOW_PRIORITY);
		anEffect.mColor = BURST_GLOW_COLOR;
		anEffect.mUnknown38 = 3;
		anEffect.mVelocityX = (float)((double)aSin * aSpeed);
		anEffect.mVelocityY = (float)(BURST_GLOW_DRIFT_Y + (double)aCos * aSpeed);
		anEffect.mLife = BURST_GLOW_LIFE_BASE + Rand() % BURST_GLOW_LIFE_RANGE;
		anEffect.mGravity = (float)((double)BURST_GLOW_GRAVITY * GRAVITY_SCALE);
	}

	for (int i = 0; i < BURST_SPARK_COUNT; i++)
	{
		double aSpread = (double)(1.0 - (double)(Rand() % 1000) / 500.0);
		float anAngle = (float)((double)BURST_SPARK_SPREAD * aSpread + aBase);
		float aSin = (float)std::sin((double)anAngle);
		float aCos = (float)std::cos((double)anAngle);
		float aRadius = (float)((Rand() % BURST_SPARK_RADIUS) - BURST_SPARK_RADIUS_BACK);
		double aSpeedPart = (double)((double)(Rand() % 1000) / 1000.0);
		aSpeedPart = (double)((double)BURST_SPARK_SPEED_RANGE * aSpeedPart);
		float aSpeed = (float)((double)BURST_SPARK_SPEED_BASE + aSpeedPart);
		double aBaseY = (double)((double)(Rand() % BURST_SPARK_JITTER_Y) + ((double)aRadius * aCos + theY));
		double aBaseX = (double)((double)(Rand() % BURST_SPARK_JITTER_X) + ((double)aRadius * aSin + theX));
		float aY = (float)((double)aBaseY - BURST_SPARK_SHIFT_Y);
		float aX = (float)((double)aBaseX - BURST_SPARK_SHIFT_X);
		Effect& anEffect = anEffects->AddEffect(EFFECT_TYPE_BURST_SPARK, aX, aY, BURST_SPARK_PRIORITY);
		anEffect.mUnknown38 = 1;
		anEffect.mColor = BURST_SPARK_COLOR;
		anEffect.mVelocityX = (float)((double)aSpeed * aSin);
		anEffect.mVelocityY = (float)(BURST_SPARK_DRIFT_Y + (double)aSpeed * aCos);
		int aLifeSum = BURST_SPARK_LIFE_BASE + Rand() % BURST_SPARK_LIFE_RANGE;
		anEffect.mLife = (int)((double)aLifeSum - (double)BURST_SPARK_LIFE_SPEED * aSpeed);
		anEffect.mGravity = (float)((double)BURST_SPARK_GRAVITY * GRAVITY_SCALE);
	}

	for (int i = 0; i < BURST_SMOKE_COUNT; i++)
	{
		double aSpread = (double)(1.0 - (double)(Rand() % 1000) / 500.0);
		float anAngle = (float)((double)BURST_SMOKE_SPREAD * aSpread + aBase);
		float aSin = (float)std::sin((double)anAngle);
		float aCos = (float)std::cos((double)anAngle);
		float aRadius = (float)((Rand() % BURST_SMOKE_RADIUS) - BURST_SMOKE_RADIUS_BACK);
		double aSpeedPart = (double)((double)(Rand() % 1000) / 1000.0);
		aSpeedPart = (double)((double)BURST_SMOKE_SPEED_RANGE * aSpeedPart);
		float aSpeed = (float)((double)BURST_SMOKE_SPEED_BASE + aSpeedPart);
		double aBaseY = (double)((double)(Rand() % BURST_SMOKE_JITTER_Y) + ((double)aRadius * aCos + theY));
		double aBaseX = (double)((double)(Rand() % BURST_SMOKE_JITTER_X) + ((double)aRadius * aSin + theX));
		float aY = (float)((double)aBaseY - BURST_SMOKE_SHIFT_Y);
		float aX = (float)((double)aBaseX - BURST_SMOKE_SHIFT_X);
		Effect& anEffect = anEffects->AddEffect(EFFECT_TYPE_BURST_SMOKE, aX, aY, BURST_SMOKE_PRIORITY);
		anEffect.mUnknown38 = Rand() % 2;
		anEffect.mUnknown3C = Rand() % 16;
		int aShade = Rand() % 3;
		anEffect.mColor = aShade == 0 ? BURST_SMOKE_COLOR_DARK : (aShade == 1 ? BURST_SMOKE_COLOR_MID : BURST_SMOKE_COLOR_LIGHT);
		anEffect.mVelocityX = (float)((double)aSpeed * aSin);
		anEffect.mVelocityY = (float)(BURST_SMOKE_DRIFT_Y + (double)aSpeed * aCos);
		int aLifeSum = BURST_SMOKE_LIFE_BASE + Rand() % BURST_SMOKE_LIFE_RANGE;
		anEffect.mLife = (int)((double)aLifeSum - (double)BURST_SMOKE_LIFE_SPEED * aSpeed);
		anEffect.mGravity = (float)((double)BURST_SMOKE_GRAVITY * GRAVITY_SCALE);
	}
}
