#include "Pea.h"

#include "Anim.h"
#include "BugObj.h"
#include "Emitter.h"
#include "GameApp.h"
#include "Res.h"
#include "SoundQueue.h"

using namespace Sexy;

// The tracks a pea has
enum PeaTrack
{
	PEA_TRACK_WAITING = 0,
	PEA_TRACK_FLYING = 1,
};

// The key event of the pea's arrival
const int PEA_EVENT_ARRIVED = 99;

// The pea's cel of the particles picture
const int PEA_FRAME = 6;

// A pea flies this many design pixels a frame, but never for fewer frames than the least; the original's speed is so
// high that the least always wins
const double PEA_SPEED = 550.0;
const float PEA_LEAST_FRAMES = 40.0f;

// Where a pea with no bug flies: off the design space's left edge, at a height of up to 300 from the middle
const float PEA_MISS_X = -960.0f;
const int PEA_MISS_HEIGHT_RANGE = 601;
const int PEA_MISS_HEIGHT_LOW = -300;

// 0x455aa0: a cel of the particles picture with a short swell behind it, waiting
Pea::Pea()
{
	Emitter::sParticlesAnim->AddRef();
	mAnim = Emitter::sParticlesAnim;
	SetStateInt(STATE_FRAME, PEA_FRAME);
	mTarget = nullptr;
	mEmitters = new EmitterSet(this);
	mEmitters->MakeShortSwell(-0.5f, 0x40);
	BuildTracks();
	StartTrack(PEA_TRACK_WAITING);
	Deactivate(this);
}

// 0x455b70: at the bug, the bug is marked and zapped, and the pea goes back to waiting at the origin
void Pea::Notify(ObjBase* theObj)
{
	if (theObj->mCurrentKey != nullptr && theObj->mCurrentKey->mEventId == PEA_EVENT_ARRIVED) {
		if (mTarget != nullptr) {
			mTarget->MarkPeaHit();
			mTarget->Zap(1, 1, 0);
			mTarget = nullptr;
		}
		SetStateInt(STATE_X, 0);
		SetStateInt(STATE_Y, 0);
		StartTrack(PEA_TRACK_WAITING);
		Deactivate(this);
	}
}

// 0x455be0: a puff every third frame, and the flight's end follows the bug
bool Pea::Update()
{
	if (!ObjBase::Update())
		return false;
	if (sFrame % 3 == 0)
		mEmitters->BurstShortSwell(0.0f, 0.0f);
	if (mTarget != nullptr) {
		mArrivalKey->mValue[STATE_X] = mTarget->mScreenX;
		mArrivalKey->mValue[STATE_Y] = mTarget->mScreenY;
	}
	return true;
}

// 0x455c60: a pea already flying is not fired again, but it forgets its bug
void Pea::Fire(float theX, float theY)
{
	mTarget = nullptr;
	if (IsActive())
		return;
	mTarget = BugObj::PickPeaTarget();
	Activate(this, false);
	ComputeDrawn();
	mFlightKey->mValue[STATE_X] = theX;
	mFlightKey->mValue[STATE_Y] = theY;

	float aTargetX;
	float aTargetY;
	if (mTarget != nullptr) {
		aTargetX = mTarget->mScreenX;
		aTargetY = mTarget->mScreenY;
	} else {
		aTargetX = PEA_MISS_X;
		aTargetY = (float)(int)(GameRandomNext() % PEA_MISS_HEIGHT_RANGE + PEA_MISS_HEIGHT_LOW);
	}
	// The original squares and adds on the FPU stack, rounding only the sum
	double aDeltaX = (double)theX - aTargetX;
	double aDeltaY = (double)theY - aTargetY;
	float aDistance = FloatSqrt((float)(aDeltaX * aDeltaX + aDeltaY * aDeltaY));
	float aFrames = (float)(aDistance / PEA_SPEED);
	if (PEA_LEAST_FRAMES > aFrames)
		aFrames = PEA_LEAST_FRAMES;
	mFlightKey->SetDuration((int)aFrames);
	mArrivalKey->mValue[STATE_X] = aTargetX;
	mArrivalKey->mValue[STATE_Y] = aTargetY;
	StartTrack(PEA_TRACK_FLYING);
	int aPitchRange = (int)(GameRandomNext() % 3);
	sApp->mSoundQueue->Request(SOUND_PEA, aPitchRange, false);
}

// 0x455db0: waiting holds at the origin; the flight runs toward its end, which raises the arrival event
void Pea::BuildTracks()
{
	AnimKey* aWait = new AnimKey();
	aWait->SetDuration(10);
	aWait->AddValue(STATE_X, 0.0f, MOD_HOLD);
	aWait->AddValue(STATE_Y, 0.0f, MOD_HOLD);
	AnimTrack* aWaitTrack = new AnimTrack();
	aWaitTrack->AddKey(aWait, aWait);
	AddTrack(aWaitTrack, nullptr);

	AnimKey* aFlight = new AnimKey();
	aFlight->SetDuration(999);
	aFlight->AddValue(STATE_X, 0.0f, MOD_TOWARD_NEXT);
	aFlight->AddValue(STATE_Y, 0.0f, MOD_TOWARD_NEXT);
	mFlightKey = aFlight;
	AnimKey* anArrival = new AnimKey();
	anArrival->SetDuration(1);
	anArrival->mEventId = PEA_EVENT_ARRIVED;
	anArrival->AddValue(STATE_X, 0.0f, MOD_HOLD);
	anArrival->AddValue(STATE_Y, 0.0f, MOD_HOLD);
	mArrivalKey = anArrival;
	AnimTrack* aFlightTrack = new AnimTrack();
	aFlightTrack->AddKey(aFlight, nullptr);
	aFlightTrack->AddKey(anArrival, nullptr);
	AddTrack(aFlightTrack, nullptr);
}
