#include "FireBall.h"

#include "Anim.h"
#include "Emitter.h"
#include "Pending.h"
#include "GameApp.h"
#include "Player.h"
#include "Res.h"
#include "SoundQueue.h"
#include "StarStream.h"

using namespace Sexy;

Anim* FireBall::sFireBallAnim = nullptr;

// The tracks a fireball has
enum FireBallTrack
{
	FIREBALL_TRACK_WAITING = 0,
	FIREBALL_TRACK_FLYING = 1,
	FIREBALL_TRACK_BURST = 2,
};

// The key events of a fireball's tracks
enum FireBallEvent
{
	FIREBALL_EVENT_ARRIVED = 99,
	FIREBALL_EVENT_BURST_DONE = 100,
};

// A fireball flies this many design pixels a frame, but never for fewer frames than the least
const double FIREBALL_SPEED = 400.0;
const float FIREBALL_LEAST_FRAMES = 60.0f;

// The kind of star stream a fireball's burst is made of
const int FIREBALL_STAR_KIND = 5;

// 0x42ad60
FireBall::FireBall()
{
	sFireBallAnim->AddRef();
	mAnim = sFireBallAnim;
	mBurst = false;
	mEmitters = new EmitterSet(this);
	mEmitters->MakeShortSwell(-0.5f, 0x40);
	BuildTracks();
	StartTrack(FIREBALL_TRACK_WAITING);
	for (int i = 0; i < FIREBALL_STAR_COUNT; i++) {
		mStars[i] = new StarStream(FIREBALL_STAR_KIND);
		mStars[i]->SetParent(this, nullptr);
	}
	Deactivate(this);
}

// 0x42ae80: nothing happens once the level has reached its target or has no bugs left: the ball stays where it landed,
// and even its burst's end is not heard, so it is never put away
void FireBall::Notify(ObjBase* theObj)
{
	AnimKey* aKey = theObj->mCurrentKey;
	if (aKey == nullptr)
		return;
	if (Player::sCurrent->mPercent >= Player::GetTargetPercent() || BugObj::sAlive < 1)
		return;
	if (aKey->mEventId == FIREBALL_EVENT_ARRIVED) {
		if (mTarget != nullptr) {
			mTarget->MarkPeaHit();
			mTarget->Zap(1, 1, 0);
			mTarget = nullptr;
		}
		// The original clears the rotation and the orbit angle, not the position
		SetStateInt(STATE_ROTATION, 0);
		SetStateInt(STATE_ORBIT_ANGLE, 0);
		for (int i = 0; i < FIREBALL_STAR_COUNT; i++)
			mStars[i]->Burst(0.0f, 0.0f, (int)(GameRandomNext() % 360));
		SetHidden(true);
		mBurst = true;
		StartTrack(FIREBALL_TRACK_BURST);
	} else if (aKey->mEventId == FIREBALL_EVENT_BURST_DONE) {
		Deactivate(this);
	}
}

// 0x42af70: a puff every third frame while it flies
bool FireBall::Update()
{
	if (!ObjBase::Update())
		return false;
	if (!mBurst && sFrame % 3 == 0)
		mEmitters->BurstShortSwell(0.0f, 0.0f);
	return true;
}

// 0x42afc0: a ball already flying is not thrown again; with no bug to pick the ball goes. Unlike a pea's, the ball's
// end stays where the bug was.
void FireBall::Throw(float theX, float theY)
{
	mBurst = false;
	if (IsActive())
		return;
	SetHidden(false);
	mTarget = BugObj::PickPeaTarget();
	if (mTarget == nullptr) {
		Deactivate(this);
		return;
	}
	float aTargetX = mTarget->mScreenX;
	float aTargetY = mTarget->mScreenY;
	if (!IsActive())
		Activate(this, false);
	ComputeDrawn();
	mFlightKey->mValue[STATE_X] = theX;
	mFlightKey->mValue[STATE_Y] = theY;
	double aDeltaY = (double)theY - aTargetY;
	SetState(STATE_ROTATION, FloatAtan2((float)aDeltaY, (float)((double)aTargetX - theX)));
	// The original squares and adds on the FPU stack, rounding only the sum
	double aDeltaX = (double)theX - aTargetX;
	float aDistance = FloatSqrt((float)(aDeltaX * aDeltaX + aDeltaY * aDeltaY));
	float aFrames = (float)(aDistance / FIREBALL_SPEED);
	if (aFrames < FIREBALL_LEAST_FRAMES)
		aFrames = FIREBALL_LEAST_FRAMES;
	mFlightKey->SetDuration((int)aFrames);
	mArrivalKey->mValue[STATE_X] = aTargetX;
	mArrivalKey->mValue[STATE_Y] = aTargetY;
	StartTrack(FIREBALL_TRACK_FLYING);
	int aPitchRange = (int)(GameRandomNext() % 3);
	sApp->mSoundQueue->Request(SOUND_PEA, aPitchRange, false);
}

// 0x42b150: waiting holds at the origin; the flight runs toward its end, which raises the arrival event; the burst
// waits 400 frames and raises its own
void FireBall::BuildTracks()
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
	anArrival->mEventId = FIREBALL_EVENT_ARRIVED;
	anArrival->AddValue(STATE_X, 0.0f, MOD_HOLD);
	anArrival->AddValue(STATE_Y, 0.0f, MOD_HOLD);
	mArrivalKey = anArrival;
	AnimTrack* aFlightTrack = new AnimTrack();
	aFlightTrack->AddKey(aFlight, nullptr);
	aFlightTrack->AddKey(anArrival, nullptr);
	AddTrack(aFlightTrack, nullptr);

	AnimKey* aBurst = new AnimKey();
	aBurst->AddValue(STATE_ORBIT_RADIUS, 0.0f, MOD_HOLD);
	aBurst->SetDuration(400);
	AnimKey* aBurstDone = new AnimKey();
	aBurstDone->mEventId = FIREBALL_EVENT_BURST_DONE;
	aBurstDone->AddValue(STATE_ORBIT_RADIUS, 0.0f, MOD_HOLD);
	aBurstDone->SetDuration(1);
	AnimTrack* aBurstTrack = new AnimTrack();
	aBurstTrack->AddKey(aBurst, nullptr);
	aBurstTrack->AddKey(aBurstDone, nullptr);
	AddTrack(aBurstTrack, nullptr);
}

// 0x42b3f0
void Sexy::PreloadFireBall()
{
	FireBall::sFireBallAnim = Anim::Get("images/fireball", ANIM_SCALE_SMOOTH, false, 0, 0, 1, false);
}
