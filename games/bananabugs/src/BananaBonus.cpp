#include "BananaBonus.h"

#include "Anim.h"
#include "Emitter.h"
#include "GameApp.h"
#include "LevelObj.h"
#include "Player.h"

using namespace Sexy;

Anim* BananaBonus::sBunchAnim = nullptr;
Anim* BananaBonus::sCoronaAnim = nullptr;

// The key events a bunch's track raises (AnimKey::mEventId)
enum BunchEvent
{
	BUNCH_EVENT_SCATTERED = 1, // nothing listens for it
	BUNCH_EVENT_ARRIVED = 2,   // the bunch has reached the counter
};

// How far a bunch springs out from where it was won, and how far it lifts before flying
const float BUNCH_SCATTER_DISTANCE = 48.0f;
const float BUNCH_LIFT = 8.0f;

// Where the bunches fly to: the banana counter, at the top right of the design space (further in on a wide screen)
const float BUNCH_COUNTER_X = 710.0f;
const float BUNCH_COUNTER_X_WIDE = 664.0f;
const float BUNCH_COUNTER_Y = -560.0f;

// 0x4048b0: hidden itself; the bunches are made at once and wait
BananaBonus::BananaBonus()
{
	mNextBunch = 0;
	SetHidden(true);
	BuildBunches();
	Activate(this, false);
}

// 0x404950: a bunch that reaches the counter bursts in stars and its bananas are counted, the counter showing them
void BananaBonus::Notify(ObjBase* theObj)
{
	if (theObj->mCurrentKey != nullptr && theObj->mCurrentKey->mEventId == BUNCH_EVENT_ARRIVED) {
		theObj->mEmitters->BurstStarRing(0.0f, 0.0f);
		Player::AddBananas(Player::GetBananaValue());
		static_cast<LevelObj*>(sLevel)->ShowBananas(true);
	}
}

// 0x4049a0: the next bunch appears at the place, picks its direction and starts
void BananaBonus::Spawn(float theX, float theY)
{
	ObjBase* aBunch = mBunches[mNextBunch];
	int aSlot = aBunch->mHoverState;
	mStartKeys[aSlot]->mValue[STATE_X] = theX;
	mStartKeys[aSlot]->mValue[STATE_Y] = theY;

	float anAngle = (float)((double)(GameRandomNext() % 360) * 3.141592653589793 / 180.0);
	float anOffsetX = (float)(FloatCos(anAngle) * (double)BUNCH_SCATTER_DISTANCE);
	float anOffsetY = (float)(FloatSin(anAngle) * (double)BUNCH_SCATTER_DISTANCE);
	float aScatterX = anOffsetX + theX;
	mScatterKeys[aSlot]->mValue[STATE_X] = aScatterX;
	float aScatterY = anOffsetY + theY;
	mScatterKeys[aSlot]->mValue[STATE_Y] = aScatterY;
	mLiftKeys[aSlot]->mValue[STATE_X] = aScatterX;
	mLiftKeys[aSlot]->mValue[STATE_Y] = (float)(aScatterY - (double)BUNCH_LIFT);

	aBunch->StartTrack(aBunch->mButtonId);
	Activate(aBunch, false);
	ObjBase* aCorona = mCoronas[mNextBunch];
	aCorona->StartTrack(aCorona->mButtonId);
	Activate(aCorona, false);
	mNextBunch = (mNextBunch + 1) % BANANA_BUNCH_COUNT;
}

// 0x404b00: each bunch keeps its track's index in the field the menus use for a button's id (+0xe8) and its slot in
// the one for its hover state (+0xec)
void BananaBonus::BuildBunches()
{
	float aCounterX = sApp->mWidescreen ? BUNCH_COUNTER_X_WIDE : BUNCH_COUNTER_X;
	for (int i = 0; i < BANANA_BUNCH_COUNT; i++) {
		ObjBase* aBunch = new ObjBase();
		mBunches[i] = aBunch;
		sBunchAnim->AddRef();
		aBunch->mAnim = sBunchAnim;
		aBunch->SetStateInt(STATE_Z, 300);
		aBunch->SetStateInt(STATE_ALPHA, 0);
		aBunch->SetStateInt(STATE_SCALE_X, 0);
		aBunch->SetStateInt(STATE_SCALE_Y, 0);
		aBunch->SetEventTarget(this);
		aBunch->mEmitters = new EmitterSet(aBunch);
		aBunch->mEmitters->MakeStarRing(0.0f, 0x80);

		// Appearing, half seen and growing from nothing
		AnimKey* aStart = new AnimKey();
		aStart->SetDuration(15);
		aStart->AddValue(STATE_X, 0.0f, MOD_TOWARD_NEXT);
		aStart->AddValue(STATE_Y, 0.0f, MOD_TOWARD_NEXT);
		aStart->AddValue(STATE_ALPHA, 0.5f, MOD_TOWARD_NEXT);
		aStart->AddValue(STATE_SCALE_X, 0.0f, MOD_TOWARD_NEXT);
		aStart->AddValue(STATE_SCALE_Y, 0.0f, MOD_TOWARD_NEXT);
		aStart->AddValue(STATE_ROTATION, 0.0f, MOD_TOWARD_NEXT);
		mStartKeys[i] = aStart;

		// Springing out, turning half a turn
		AnimKey* aScatter = new AnimKey();
		aScatter->mEventId = BUNCH_EVENT_SCATTERED;
		aScatter->SetDuration(70);
		aScatter->AddValue(STATE_X, 0.0f, MOD_TOWARD_NEXT);
		aScatter->AddValue(STATE_Y, 0.0f, MOD_TOWARD_NEXT);
		aScatter->AddValue(STATE_ALPHA, 1.0f, MOD_TOWARD_NEXT);
		aScatter->AddValue(STATE_SCALE_X, 1.0f, MOD_TOWARD_NEXT);
		aScatter->AddValue(STATE_SCALE_Y, 1.0f, MOD_TOWARD_NEXT);
		aScatter->AddValue(STATE_ROTATION, 3.1415927f, MOD_TOWARD_NEXT);
		aScatter->mEasing = EASE_LINEAR;
		mScatterKeys[i] = aScatter;

		// Lifting and flying to the counter, gathering speed
		AnimKey* aLift = new AnimKey();
		aLift->SetDuration(65);
		aLift->AddValue(STATE_X, 0.0f, MOD_TOWARD_NEXT);
		aLift->AddValue(STATE_Y, 0.0f, MOD_TOWARD_NEXT);
		aLift->AddValue(STATE_ALPHA, 1.0f, MOD_HOLD);
		aLift->AddValue(STATE_SCALE_X, 1.0f, MOD_HOLD);
		aLift->AddValue(STATE_SCALE_Y, 1.0f, MOD_HOLD);
		aLift->AddValue(STATE_ROTATION, 6.37045f, MOD_TOWARD_NEXT);
		aLift->mEasing = EASE_IN_SQUARE;
		mLiftKeys[i] = aLift;

		// At the counter, shrinking away as it spins
		AnimKey* anArrive = new AnimKey();
		anArrive->mEventId = BUNCH_EVENT_ARRIVED;
		anArrive->SetDuration(50);
		anArrive->AddValue(STATE_X, aCounterX, MOD_HOLD);
		anArrive->AddValue(STATE_Y, BUNCH_COUNTER_Y, MOD_HOLD);
		anArrive->AddValue(STATE_ALPHA, 1.0f, MOD_HOLD);
		anArrive->AddValue(STATE_SCALE_X, 1.0f, MOD_TOWARD_NEXT);
		anArrive->AddValue(STATE_SCALE_Y, 1.0f, MOD_TOWARD_NEXT);
		anArrive->AddValue(STATE_ROTATION, 12.5664f, MOD_TOWARD_NEXT);

		// Gone. The original sets the alpha twice, 0 and then 0.1, and an orbit radius of 38 that no orbit reads.
		AnimKey* aGone = new AnimKey();
		aGone->SetDuration(1);
		aGone->AddValue(STATE_ALPHA, 0.0f, MOD_HOLD);
		aGone->AddValue(STATE_ALPHA, 0.1f, MOD_HOLD);
		aGone->AddValue(STATE_ORBIT_RADIUS, 38.0f, MOD_HOLD);
		aGone->AddValue(STATE_SCALE_X, 0.0f, MOD_HOLD);
		aGone->AddValue(STATE_SCALE_Y, 0.0f, MOD_HOLD);
		aGone->AddValue(STATE_ROTATION, -6.28319f, MOD_HOLD);

		AnimTrack* aTrack = new AnimTrack();
		aTrack->AddKey(aStart, nullptr);
		aTrack->AddKey(aScatter, nullptr);
		aTrack->AddKey(aLift, nullptr);
		aTrack->AddKey(anArrive, nullptr);
		aTrack->AddKey(aGone, nullptr);
		aBunch->mButtonId = aBunch->AddTrack(aTrack, nullptr);
		aBunch->mHoverState = i;
		Deactivate(aBunch);

		ObjBase* aCorona = new ObjBase();
		mCoronas[i] = aCorona;
		sCoronaAnim->AddRef();
		aCorona->mAnim = sCoronaAnim;
		aCorona->SetParent(aBunch, nullptr);
		aCorona->SetStateInt(STATE_Z, -1);
		aCorona->SetStateInt(STATE_RED, 1);
		aCorona->SetStateInt(STATE_GREEN, 1);
		aCorona->SetState(STATE_BLUE, 0.8f);
		aCorona->SetStateInt(STATE_ALPHA, 0);
		aCorona->SetStateInt(STATE_SCALE_X, 0);
		aCorona->SetStateInt(STATE_SCALE_Y, 0);

		// The corona glows up behind the bunch, swells and fades, all drawn additively
		AnimKey* aGlowStart = new AnimKey();
		aGlowStart->SetDuration(15);
		aGlowStart->mDrawMode = 1;
		aGlowStart->AddValue(STATE_X, 0.0f, MOD_TOWARD_NEXT);
		aGlowStart->AddValue(STATE_Y, 0.0f, MOD_TOWARD_NEXT);
		aGlowStart->AddValue(STATE_ALPHA, 0.0f, MOD_TOWARD_NEXT);
		aGlowStart->AddValue(STATE_SCALE_X, 0.5f, MOD_TOWARD_NEXT);
		aGlowStart->AddValue(STATE_SCALE_Y, 0.5f, MOD_TOWARD_NEXT);
		aGlowStart->AddValue(STATE_ROTATION, 0.0f, MOD_TOWARD_NEXT);

		AnimKey* aGlow = new AnimKey();
		aGlow->mDrawMode = 1;
		aGlow->SetDuration(80);
		aGlow->AddValue(STATE_X, 0.0f, MOD_TOWARD_NEXT);
		aGlow->AddValue(STATE_Y, 0.0f, MOD_TOWARD_NEXT);
		aGlow->AddValue(STATE_ALPHA, 0.6f, MOD_TOWARD_NEXT);
		aGlow->AddValue(STATE_SCALE_X, 1.7f, MOD_TOWARD_NEXT);
		aGlow->AddValue(STATE_SCALE_Y, 1.7f, MOD_TOWARD_NEXT);
		aGlow->AddValue(STATE_ROTATION, 1.5708f, MOD_TOWARD_NEXT);

		AnimKey* aGlowShrink = new AnimKey();
		aGlowShrink->mDrawMode = 1;
		aGlowShrink->SetDuration(10);
		aGlowShrink->AddValue(STATE_X, 0.0f, MOD_TOWARD_NEXT);
		aGlowShrink->AddValue(STATE_Y, 0.0f, MOD_TOWARD_NEXT);
		aGlowShrink->AddValue(STATE_ALPHA, 0.6f, MOD_TOWARD_NEXT);
		aGlowShrink->AddValue(STATE_SCALE_X, 1.2f, MOD_TOWARD_NEXT);
		aGlowShrink->AddValue(STATE_SCALE_Y, 1.2f, MOD_TOWARD_NEXT);
		aGlowShrink->AddValue(STATE_ROTATION, 16.7552f, MOD_TOWARD_NEXT);

		AnimKey* aGlowEnd = new AnimKey();
		aGlowEnd->mDrawMode = 1;
		aGlowEnd->SetDuration(1);
		aGlowEnd->AddValue(STATE_X, 0.0f, MOD_TOWARD_NEXT);
		aGlowEnd->AddValue(STATE_Y, 0.0f, MOD_TOWARD_NEXT);
		aGlowEnd->AddValue(STATE_ALPHA, 0.1f, MOD_TOWARD_NEXT);
		aGlowEnd->AddValue(STATE_SCALE_X, 0.0f, MOD_TOWARD_NEXT);
		aGlowEnd->AddValue(STATE_SCALE_Y, 0.0f, MOD_TOWARD_NEXT);
		aGlowEnd->AddValue(STATE_ROTATION, 0.0f, MOD_TOWARD_NEXT);

		AnimTrack* aGlowTrack = new AnimTrack();
		aGlowTrack->AddKey(aGlowStart, nullptr);
		aGlowTrack->AddKey(aGlow, nullptr);
		aGlowTrack->AddKey(aGlowShrink, nullptr);
		aGlowTrack->AddKey(aGlowEnd, nullptr);
		aCorona->mButtonId = aCorona->AddTrack(aGlowTrack, nullptr);
		aCorona->mHoverState = i;
		Deactivate(aCorona);
	}
}

// 0x405730
void Sexy::PreloadBananas()
{
	BananaBonus::sBunchAnim = Anim::Get("images/bananabunch01", ANIM_SCALE_SMOOTH, false, 0, 0, 1, false);
	BananaBonus::sCoronaAnim = Anim::Get("images/corona1", ANIM_SCALE_SMOOTH, false, 0, 0, 1, false);
}
