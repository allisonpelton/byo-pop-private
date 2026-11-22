#include "StarStream.h"

#include "Anim.h"
#include "Emitter.h"

using namespace Sexy;

// The star's cel of the particles picture
const int STAR_STREAM_FRAME = 4;

// 0x46c9d0: from nothing, with the emitter of its kind; a kind outside the five has no emitter
StarStream::StarStream(int theKind)
{
	int aTrailInterval = 3;
	SetStateInt(STATE_SCALE_X, 0);
	SetStateInt(STATE_SCALE_Y, 0);
	SetStateInt(STATE_ALPHA, 0);
	mKind = theKind;
	switch (theKind) {
	case STAR_STREAM_BLUE:
		mEmitters = new EmitterSet(this);
		mEmitters->MakeBlueSparks(32.0f);
		break;
	case STAR_STREAM_DUST:
	case STAR_STREAM_BIG_DUST:
		mEmitters = new EmitterSet(this);
		mEmitters->MakeDust(32.0f, 100);
		break;
	case STAR_STREAM_SLOW_BLUE:
		mEmitters = new EmitterSet(this);
		mEmitters->MakeBlueSparks(32.0f);
		aTrailInterval = 5;
		break;
	case STAR_STREAM_RED:
		mEmitters = new EmitterSet(this);
		mEmitters->MakeRedSparks(32.0f);
		aTrailInterval = 4;
		break;
	}
	mTrailInterval = aTrailInterval;
	BuildTrack();
	Deactivate(this);
}

// 0x46cb50
bool StarStream::Update()
{
	if (!ObjBase::Update())
		return false;
	if ((unsigned int)sFrame % (unsigned int)mTrailInterval == 0) {
		switch (mKind) {
		case STAR_STREAM_BLUE:
		case STAR_STREAM_SLOW_BLUE:
			mEmitters->BurstBlueSparks(nullptr, 0.0f, 0.0f);
			break;
		case STAR_STREAM_DUST:
		case STAR_STREAM_BIG_DUST:
			mEmitters->BurstDust(0.0f, 0.0f);
			break;
		case STAR_STREAM_RED:
			mEmitters->BurstRedSparks(0.0f, 0.0f);
			break;
		}
	}
	return true;
}

// 0x46cbf0: thrown from the place at the angle, in degrees; the speed is 2.1 to 4.5 by kind, and the flight lasts 50
// to 100 frames for the sparks and dust, 100 for big dust and 150 for slow sparks
void StarStream::Burst(float theX, float theY, int theDegrees)
{
	float anAngle = (float)((double)theDegrees * 3.141592653589793 / 180.0);
	float aSpeed;
	int aFrames;
	switch (mKind) {
	case STAR_STREAM_BLUE:
	case STAR_STREAM_DUST:
	case STAR_STREAM_RED:
		aSpeed = (float)((double)(GameRandomNext() % 25 + 21) / 10.0);
		SetState(STATE_X, theX);
		SetState(STATE_Y, theY);
		SetStateInt(STATE_Z, 200);
		SetState(STATE_ORBIT_ANGLE, anAngle);
		ComputeDrawn();
		aFrames = (int)(GameRandomNext() % 51 + 50);
		break;
	case STAR_STREAM_BIG_DUST:
		aSpeed = (float)((double)(GameRandomNext() % 11 + 31) / 10.0);
		SetState(STATE_X, theX);
		SetState(STATE_Y, theY);
		SetStateInt(STATE_Z, 200);
		SetState(STATE_ORBIT_ANGLE, anAngle);
		ComputeDrawn();
		aFrames = 100;
		break;
	default:
		aSpeed = 2.1f;
		SetState(STATE_X, theX);
		SetState(STATE_Y, theY);
		SetStateInt(STATE_Z, 200);
		SetState(STATE_ORBIT_ANGLE, anAngle);
		ComputeDrawn();
		aFrames = 150;
		break;
	}
	mFlightKey->SetDuration(aFrames);
	mFlightKey->SetDecay(STATE_X, aSpeed, FloatCos(anAngle));
	mFlightKey->SetDecay(STATE_Y, aSpeed, FloatSin(anAngle));
	Activate(this, false);
	StartTrack(0);
}

// 0x46ce00: the flight decays from a speed of 2.5 toward a random multiple of ten degrees (every burst replaces both),
// growing and showing; the last key holds the end size and alpha and raises event 99
void StarStream::BuildTrack()
{
	Emitter::sParticlesAnim->AddRef();
	mAnim = Emitter::sParticlesAnim;
	SetStateInt(STATE_FRAME, STAR_STREAM_FRAME);
	float anAngle = (float)((double)((GameRandomNext() % 36 + 1) * 10) * 3.141592653589793 / 180.0);

	float aFall;
	float aSize;
	float anAlpha;
	float anEndSize;
	float anEndAlpha;
	switch (mKind) {
	case STAR_STREAM_BLUE:
	case STAR_STREAM_DUST:
	case STAR_STREAM_RED:
		aFall = -0.04f;
		aSize = 1.5f;
		anAlpha = 1.0f;
		anEndSize = 0.8f;
		anEndAlpha = 0.2f;
		break;
	case STAR_STREAM_SLOW_BLUE:
		aFall = -0.005f;
		aSize = 1.5f;
		anAlpha = 1.0f;
		anEndSize = 0.8f;
		anEndAlpha = 0.2f;
		break;
	default:
		aFall = -0.04f;
		aSize = 3.0f;
		anAlpha = 0.7f;
		anEndSize = 1.0f;
		anEndAlpha = 0.0f;
		break;
	}

	AnimKey* aFlight = new AnimKey();
	aFlight->SetDuration(100);
	aFlight->AddDecay(STATE_X, 0.0f, 2.5f, FloatCos(anAngle), 0.0f, 1.0f);
	aFlight->AddDecay(STATE_Y, 0.0f, 2.5f, FloatSin(anAngle), aFall, 1.0f);
	aFlight->AddValue(STATE_SCALE_X, aSize, MOD_TOWARD_NEXT);
	aFlight->AddValue(STATE_SCALE_Y, aSize, MOD_TOWARD_NEXT);
	aFlight->AddValue(STATE_ALPHA, anAlpha, MOD_TOWARD_NEXT);
	mFlightKey = aFlight;

	AnimKey* anEnd = new AnimKey();
	anEnd->SetDuration(1);
	anEnd->AddValue(STATE_SCALE_X, anEndSize, MOD_HOLD);
	anEnd->AddValue(STATE_SCALE_Y, anEndSize, MOD_HOLD);
	anEnd->AddValue(STATE_ALPHA, anEndAlpha, MOD_HOLD);
	anEnd->mEventId = 99;

	AnimTrack* aTrack = new AnimTrack();
	aTrack->AddKey(aFlight, nullptr);
	aTrack->AddKey(anEnd, nullptr);
	AddTrack(aTrack, nullptr);
}
