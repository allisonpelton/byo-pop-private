#include "FireWorks.h"

#include "Emitter.h"
#include "GameApp.h"
#include "Res.h"
#include "SoundQueue.h"
#include "StarStream.h"

using namespace Sexy;

// The key event of the rocket's burst
const int FIREWORKS_EVENT_BURST = 1;

// The kind of star stream a rocket's burst is made of
const int FIREWORKS_STAR_KIND = 1;

// The degrees between the burst's star streams
const int FIREWORKS_STAR_SPACING = 45;

// 0x42b410: a ring of stars, a rising swell and dust, and eight star streams, waiting
FireWorks::FireWorks()
{
	mEmitters = new EmitterSet(this);
	mEmitters->MakeStarRing(0.0f, 0x28);
	mEmitters->MakeRisingSwell(40.0f, 0xfa);
	mEmitters->MakeDust(0.0f, 0x40);
	BuildTrack();
	for (int i = 0; i < FIREWORKS_STAR_COUNT; i++) {
		mStars[i] = new StarStream(FIREWORKS_STAR_KIND);
		mStars[i]->SetParent(this, nullptr);
	}
	Deactivate(this);
}

// 0x42b550: the angle is 87 to 93 degrees, the speed 9.4 to 11.4, and the burst comes once the rocket is drawn above a
// height of 120 to 350 window pixels; the launch sounds two times in thirteen, the whistle two times in thirteen
void FireWorks::Launch(float theX, float theY)
{
	SetState(STATE_X, theX);
	SetState(STATE_Y, theY);
	float anAngle = (float)((double)(GameRandomNext() % 7 + 87) * 3.141592653589793 / 180.0);
	float aSpeed = (float)((double)(GameRandomNext() % 3 + 1) + 8.4);
	mRiseKey->SetDecay(STATE_X, aSpeed, -FloatCos(anAngle));
	mRiseKey->SetDecay(STATE_Y, aSpeed, FloatSin(anAngle));
	mRiseKey->mEndCondition = KEY_END_DRAWN_BELOW;
	mRiseKey->mEndChannel = STATE_Y;
	mRiseKey->mEndValue = (float)(GameRandomNext() % 231 + 120);
	mRising = true;
	Activate(this, false);
	switch (GameRandomNext() % 13) {
	case 0:
	case 1:
		sApp->mSoundQueue->Request(SOUND_FIREWORKS_LAUNCH, 0, false);
		break;
	case 3:
	case 4:
		sApp->mSoundQueue->Request(SOUND_FIREWORKS_WHISTLE, 0, false);
		break;
	}
	StartTrack(mTrackIndex);
}

// 0x42b6b0: the burst; two times in nine the louder bang and the star streams
void FireWorks::Notify(ObjBase* theObj)
{
	if (theObj->mCurrentKey == nullptr || theObj->mCurrentKey->mEventId != FIREWORKS_EVENT_BURST)
		return;
	if (GameRandomNext() % 9 < 2) {
		sApp->mSoundQueue->Request(SOUND_FIREWORKS_EXPLO2, 0, false);
		int aStar = 0;
		for (int anAngle = 0; anAngle < 360; anAngle += FIREWORKS_STAR_SPACING)
			mStars[aStar++]->Burst(0.0f, 0.0f, anAngle);
	} else {
		sApp->mSoundQueue->Request(SOUND_FIREWORKS_EXPLO1, 0, false);
	}
	mRising = false;
	mEmitters->BurstStarRing(0.0f, 0.0f);
	mEmitters->BurstRisingSwell(0.0f, 0.0f);
}

// 0x42b770: dust every frame while it rises
bool FireWorks::Update()
{
	if (!ObjBase::Update())
		return false;
	if (mRising)
		mEmitters->BurstDust(0.0f, 0.0f);
	return true;
}

// 0x42b7b0: the rise moves by decaying speeds, the upward one with a pull of -0.05, for up to 999 frames or until its
// end condition (which Launch sets) holds; then 300 frames for the burst. The decays start toward an angle of one radian,
// which every launch replaces.
void FireWorks::BuildTrack()
{
	mRiseKey = new AnimKey();
	mRiseKey->SetDuration(999);
	mRiseKey->AddDecay(STATE_X, 0.0f, 12.5f, -FloatCos(1.0f), 0.0f, 1.0f);
	mRiseKey->AddDecay(STATE_Y, 0.0f, 12.5f, FloatSin(1.0f), -0.05f, 1.0f);
	mRiseKey->mEndCondition = KEY_END_DRAWN_BELOW;
	mRiseKey->mEndChannel = STATE_Y;
	mRiseKey->mEndValue = 300.0f;

	mBurstKey = new AnimKey();
	mBurstKey->mEventId = FIREWORKS_EVENT_BURST;
	mBurstKey->SetDuration(300);

	AnimTrack* aTrack = new AnimTrack();
	aTrack->AddKey(mRiseKey, nullptr);
	aTrack->AddKey(mBurstKey, nullptr);
	mTrackIndex = AddTrack(aTrack, nullptr);
}
