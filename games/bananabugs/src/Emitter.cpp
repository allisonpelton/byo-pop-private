#include "Emitter.h"

#include "Anim.h"
#include "GameApp.h"
#include "SoundQueue.h"

using namespace Sexy;

ObjBase* Emitter::sLayer = nullptr;
Anim* Emitter::sParticlesAnim = nullptr;

// A whole number from -theRange to theRange, as the original draws it
static int RandomPlusMinus(int theRange)
{
	return (int)(GameRandomNext() % (unsigned int)(theRange * 2 + 1)) - theRange;
}

// 0x427b60
Emitter::Emitter(ObjBase* theParent, bool theFlag)
{
	mUnused2a8 = 0;
	mRandomFrames = 0;
	mDepth = 0;
	mNextParticle = 0;
	mDelay = 0;
	mEmitOnCall = false;
	mType = OBJ_TYPE_EMITTER;
	mAngleMode = EMITTER_ANGLE_SPREAD;
	mSound = -1;
	mMadeWithFlag = theFlag;
	// the original leaves these unset until Setup
	mParticleCount = 0;
	mRate = 1;
	mBurst = 0;
	mFrameCount = 0;
	mJitterX = 0;
	mJitterY = 0;
	mAngleJitter = 0;
	mX = 0.0f;
	mY = 0.0f;
	for (int i = 0; i < EMITTER_MAX_PARTICLES; i++)
		mParticles[i] = nullptr;
	SetParent(theParent, nullptr);
}

// 0x427bf0. The particles, wherever they are, are its children again (the original's call does nothing for those
// still under it) and go with it.
Emitter::~Emitter()
{
	for (int i = 0; i < mParticleCount; i++) {
		if (mParticles[i] != nullptr)
			mParticles[i]->SetParent(this, this);
	}
}

// 0x427c40: a particle that ends comes back under its emitter
void Emitter::Deactivate(ObjBase* theObj)
{
	theObj->ObjBase::Deactivate(theObj);
	if (theObj != this)
		theObj->SetParent(this, this);
}

// 0x427c60
bool Emitter::Update()
{
	if (sLayer == nullptr)
		return false;
	if (mDelay > 0) {
		mDelay--;
		if (mDelay == 0 && mSound >= 0)
			sApp->mSoundQueue->Request(mSound, 0, false);
		return true;
	}
	if (mEmitOnCall)
		return true;
	if (!ObjBase::Update())
		return false;
	Emit(mX, mY, (float)RandomPlusMinus(mAngleJitter));
	return true;
}

// 0x427d20
void Emitter::SetDepth(Emitter* theEmitter, float theDepth)
{
	theEmitter->mDepth = (int)theDepth;
}

// 0x427d80 (0x427d50 calls it): a burst, on the frames the rate allows. An emitter that emits when told moves to the
// place given first.
void Emitter::Emit(float theX, float theY, float theAngle)
{
	if (sLayer == nullptr)
		return;
	float aAngleGiven = theAngle;
	if (mFrameCount % mRate == 0) {
		if (mEmitOnCall) {
			SetState(STATE_X, theX);
			SetState(STATE_Y, theY);
			mState[STATE_X] = theX;
			mState[STATE_Y] = theY;
			ComputeDrawn();
		}
		// the original keeps the angle in the x argument's place, so a mode it does not know reuses the last one
		float aParticleAngle = theX;
		for (int i = 0; i < mBurst; i++) {
			ObjBase* aParticle = mParticles[mNextParticle];
			mNextParticle = (mNextParticle + 1) % mParticleCount;
			aParticle->Init();
			aParticle->Activate(aParticle, false);
			aParticle->SetStateInt(STATE_RED, 0);
			aParticle->SetStateInt(STATE_GREEN, 0);
			aParticle->SetStateInt(STATE_BLUE, 0);
			aParticle->SetStateInt(STATE_ALPHA, 0);
			aParticle->SetStateInt(STATE_SCALE_X, 0);
			aParticle->SetStateInt(STATE_SCALE_Y, 0);
			if (mRandomFrames > 0)
				aParticle->SetStateInt(STATE_FRAME, (int)(GameRandomNext() % (unsigned int)mRandomFrames));
			if (mAngleMode == EMITTER_ANGLE_OWN) {
				aParticleAngle = mState[STATE_ORBIT_ANGLE];
			} else if (mAngleMode == EMITTER_ANGLE_SPREAD) {
				theAngle = (float)(360.0 / (double)mBurst + theAngle);
				aParticleAngle = (float)((double)theAngle * PI_F / 180.0 + mState[STATE_ORBIT_ANGLE]);
			} else if (mAngleMode == EMITTER_ANGLE_JITTER) {
				theAngle = (float)((double)RandomPlusMinus(mAngleJitter) + aAngleGiven);
				aParticleAngle = (float)((double)theAngle * PI_F / 180.0 + mState[STATE_ORBIT_ANGLE]);
			}
			aParticle->SetState(STATE_ORBIT_ANGLE, aParticleAngle);
			aParticle->SetState(STATE_ROTATION, aParticleAngle);
			aParticle->SetParent(sLayer, nullptr);
			aParticle->SetState(STATE_X, (float)((double)RandomPlusMinus(mJitterX) + mScreenX));
			aParticle->SetState(STATE_Y, (float)((double)RandomPlusMinus(mJitterY) + mScreenY));
			aParticle->SetState(STATE_Z, (float)((double)mDepth + mDrawn[STATE_Z] + (double)i / 10000.0));
			aParticle->ComputeDrawn();
		}
	}
	mFrameCount++;
}

// 0x428040: theParticleCount copies of this emitter's picture and tracks (512 at most), which this emitter then gives
// up; it is shown hidden, at the place given
void Emitter::Setup(int theParticleCount, int theRate, int theBurst, int theX, int theY, int theJitterX,
	int theJitterY, int theAngleJitter)
{
	if (theParticleCount < 0)
		return;
	if (theParticleCount > EMITTER_MAX_PARTICLES)
		theParticleCount = EMITTER_MAX_PARTICLES;
	mRate = theRate;
	mBurst = theBurst;
	mX = (float)theX;
	mY = (float)theY;
	mJitterX = theJitterX;
	mJitterY = theJitterY;
	mAngleJitter = theAngleJitter;
	mParticleCount = theParticleCount;
	mFrameCount = 0;
	mRandomFrames = 0;
	SetStateInt(STATE_X, theX);
	SetStateInt(STATE_Y, theY);
	SetStateInt(STATE_SCALE_X, 1);
	SetStateInt(STATE_SCALE_Y, 1);
	SetStateInt(STATE_RED, 1);
	SetStateInt(STATE_GREEN, 1);
	SetStateInt(STATE_BLUE, 1);
	SetStateInt(STATE_ALPHA, 1);
	for (int i = 0; i < mParticleCount; i++) {
		ObjBase* aParticle = new ObjBase();
		mParticles[i] = aParticle;
		aParticle->CopyFrom(this);
		aParticle->SetParent(this, this);
		aParticle->mType = OBJ_TYPE_OWNED_ELSEWHERE;
	}
	ClearTracks();
	Activate(this, true);
}

// 0x428190
void Emitter::CreateLayer()
{
	sLayer = new ObjBase();
	sLayer->SetParent(nullptr, nullptr);
	sLayer->SetStateInt(STATE_X, 0);
	sLayer->SetStateInt(STATE_Y, 0);
	sLayer->SetState(STATE_Z, -0.5f);
	sLayer->SetStateInt(STATE_SCALE_X, 1);
	sLayer->SetStateInt(STATE_SCALE_Y, 1);
	sLayer->SetStateInt(STATE_RED, 1);
	sLayer->SetStateInt(STATE_GREEN, 1);
	sLayer->SetStateInt(STATE_BLUE, 1);
	sLayer->SetStateInt(STATE_ALPHA, 1);
	sLayer->Activate(sLayer, false);
}

// 0x4346e0
void Emitter::LoadParticles()
{
	sParticlesAnim = Anim::Get("images/particles", ANIM_SCALE_FAST, false, 0x31, 0x31, 8, false);
}

// The presets: each emitter's picture, keys and numbers

// 0x430790: a dim glow rising 96 pixels as it turns orange
static void PresetFlame(Emitter* theEmitter)
{
	Emitter::sParticlesAnim->AddRef();
	theEmitter->mAnim = Emitter::sParticlesAnim;
	theEmitter->SetStateInt(STATE_FRAME, 0);
	theEmitter->SetAdditive();
	AnimKey* aKey1 = new AnimKey();
	aKey1->SetDuration(10);
	aKey1->mDrawMode = 1;
	aKey1->AddValue(STATE_X, 0.0f, MOD_TOWARD_NEXT);
	aKey1->AddValue(STATE_Y, 0.0f, MOD_TOWARD_NEXT);
	aKey1->AddValue(STATE_SCALE_X, 0.5f, MOD_TOWARD_NEXT);
	aKey1->AddValue(STATE_SCALE_Y, 0.5f, MOD_TOWARD_NEXT);
	aKey1->AddValue(STATE_RED, 0.1f, MOD_TOWARD_NEXT);
	aKey1->AddValue(STATE_GREEN, 0.1f, MOD_TOWARD_NEXT);
	aKey1->AddValue(STATE_BLUE, 0.1f, MOD_TOWARD_NEXT);
	aKey1->AddValue(STATE_ALPHA, 0.0f, MOD_TOWARD_NEXT);
	AnimKey* aKey2 = new AnimKey();
	aKey2->SetDuration(30);
	aKey2->mDrawMode = 1;
	aKey2->AddValue(STATE_X, 0.0f, MOD_TOWARD_NEXT);
	aKey2->AddValue(STATE_Y, 0.0f, MOD_TOWARD_NEXT);
	aKey2->AddValue(STATE_SCALE_X, 1.0f, MOD_TOWARD_NEXT);
	aKey2->AddValue(STATE_SCALE_Y, 1.3f, MOD_TOWARD_NEXT);
	aKey2->AddValue(STATE_RED, 0.3f, MOD_TOWARD_NEXT);
	aKey2->AddValue(STATE_GREEN, 0.2f, MOD_TOWARD_NEXT);
	aKey2->AddValue(STATE_BLUE, 0.2f, MOD_TOWARD_NEXT);
	aKey2->AddValue(STATE_ALPHA, 1.0f, MOD_TOWARD_NEXT);
	AnimKey* aKey3 = new AnimKey();
	aKey3->mDrawMode = 1;
	aKey3->SetDuration(10);
	aKey3->AddValue(STATE_X, 0.0f, MOD_TOWARD_NEXT);
	aKey3->AddValue(STATE_Y, -96.0f, MOD_TOWARD_NEXT);
	aKey3->AddValue(STATE_SCALE_X, 0.5f, MOD_TOWARD_NEXT);
	aKey3->AddValue(STATE_SCALE_Y, 0.5f, MOD_TOWARD_NEXT);
	aKey3->AddValue(STATE_RED, 1.0f, MOD_TOWARD_NEXT);
	aKey3->AddValue(STATE_GREEN, 0.5f, MOD_TOWARD_NEXT);
	aKey3->AddValue(STATE_BLUE, 0.1f, MOD_TOWARD_NEXT);
	aKey3->AddValue(STATE_ALPHA, 0.0f, MOD_TOWARD_NEXT);
	AnimTrack* aTrack = new AnimTrack();
	aTrack->AddKey(aKey1, nullptr);
	aTrack->AddKey(aKey2, nullptr);
	aTrack->AddKey(aKey3, nullptr);
	theEmitter->AddTrack(aTrack, nullptr);
	theEmitter->Setup(64, 1, 1, 0, 0, 4, 4, 0);
	theEmitter->Deactivate(theEmitter);
}

// 0x430200: ten stars a burst going out in a ring and turning
static void PresetStarRing(Emitter* theEmitter, int theCount)
{
	Emitter::sParticlesAnim->AddRef();
	theEmitter->mAnim = Emitter::sParticlesAnim;
	theEmitter->SetStateInt(STATE_FRAME, 2);
	theEmitter->SetAdditive();
	AnimKey* aKey1 = new AnimKey();
	aKey1->SetDuration(28);
	aKey1->mDrawMode = 1;
	aKey1->AddValue(STATE_X, 0.0f, MOD_ORBIT_COS);
	aKey1->AddValue(STATE_Y, 0.0f, MOD_ORBIT_SIN);
	aKey1->AddValue(STATE_SCALE_X, 0.1f, MOD_TOWARD_NEXT);
	aKey1->AddValue(STATE_SCALE_Y, 0.1f, MOD_TOWARD_NEXT);
	aKey1->AddValue(STATE_RED, 0.5f, MOD_HOLD);
	aKey1->AddValue(STATE_GREEN, 0.5f, MOD_HOLD);
	aKey1->AddValue(STATE_BLUE, 0.6f, MOD_HOLD);
	aKey1->AddValue(STATE_ALPHA, 0.3f, MOD_TOWARD_NEXT);
	aKey1->AddValue(STATE_ORBIT_RADIUS, 4.0f, MOD_TOWARD_NEXT);
	AnimKey* aKey2 = new AnimKey();
	aKey2->SetDuration(60);
	aKey2->mDrawMode = 1;
	aKey2->AddValue(STATE_X, 0.0f, MOD_ORBIT_COS);
	aKey2->AddValue(STATE_Y, 0.0f, MOD_ORBIT_SIN);
	aKey2->AddValue(STATE_SCALE_X, 0.5f, MOD_TOWARD_NEXT);
	aKey2->AddValue(STATE_SCALE_Y, 0.5f, MOD_TOWARD_NEXT);
	aKey2->AddValue(STATE_RED, 1.0f, MOD_TOWARD_NEXT);
	aKey2->AddValue(STATE_GREEN, 1.0f, MOD_TOWARD_NEXT);
	aKey2->AddValue(STATE_BLUE, 1.0f, MOD_TOWARD_NEXT);
	aKey2->AddValue(STATE_ALPHA, 1.0f, MOD_TOWARD_NEXT);
	aKey2->AddValue(STATE_ORBIT_RADIUS, 0.0f, MOD_TOWARD_NEXT);
	aKey2->AddValue(STATE_ROTATION, 0.0f, MOD_TOWARD_NEXT);
	AnimKey* aKey3 = new AnimKey();
	aKey3->SetDuration(1);
	aKey3->mDrawMode = 1;
	aKey3->AddValue(STATE_X, 0.0f, MOD_ORBIT_COS);
	aKey3->AddValue(STATE_Y, 0.0f, MOD_ORBIT_SIN);
	aKey3->AddValue(STATE_SCALE_X, 0.3f, MOD_TOWARD_NEXT);
	aKey3->AddValue(STATE_SCALE_Y, 0.3f, MOD_TOWARD_NEXT);
	aKey3->AddValue(STATE_RED, 0.4f, MOD_HOLD);
	aKey3->AddValue(STATE_GREEN, 0.4f, MOD_HOLD);
	aKey3->AddValue(STATE_BLUE, 0.5f, MOD_HOLD);
	aKey3->AddValue(STATE_ALPHA, 0.0f, MOD_TOWARD_NEXT);
	aKey3->AddValue(STATE_ORBIT_RADIUS, 0.0f, MOD_TOWARD_NEXT);
	aKey3->AddValue(STATE_ROTATION, 1.5707964f, MOD_HOLD);
	AnimTrack* aTrack = new AnimTrack();
	aTrack->AddKey(aKey1, nullptr);
	aTrack->AddKey(aKey2, nullptr);
	aTrack->AddKey(aKey3, nullptr);
	theEmitter->AddTrack(aTrack, nullptr);
	theEmitter->Setup(theCount, 1, 10, 0, 0, 0, 0, 180);
	theEmitter->SetEmitOnCall();
}

// 0x430c60: a white glow swelling to twice its size over 65 frames
static void PresetSwell(Emitter* theEmitter)
{
	Emitter::sParticlesAnim->AddRef();
	theEmitter->mAnim = Emitter::sParticlesAnim;
	theEmitter->SetStateInt(STATE_FRAME, 5);
	theEmitter->SetAdditive();
	AnimKey* aKey1 = new AnimKey();
	aKey1->SetDuration(4);
	aKey1->AddValue(STATE_SCALE_X, 0.0f, MOD_TOWARD_NEXT);
	aKey1->AddValue(STATE_SCALE_Y, 0.0f, MOD_TOWARD_NEXT);
	aKey1->AddValue(STATE_RED, 1.0f, MOD_HOLD);
	aKey1->AddValue(STATE_GREEN, 1.0f, MOD_HOLD);
	aKey1->AddValue(STATE_BLUE, 1.0f, MOD_HOLD);
	aKey1->AddValue(STATE_ALPHA, 0.0f, MOD_TOWARD_NEXT);
	AnimKey* aKey2 = new AnimKey();
	aKey2->SetDuration(65);
	aKey2->AddValue(STATE_SCALE_X, 1.9f, MOD_TOWARD_NEXT);
	aKey2->AddValue(STATE_SCALE_Y, 1.9f, MOD_TOWARD_NEXT);
	aKey2->AddValue(STATE_RED, 1.0f, MOD_HOLD);
	aKey2->AddValue(STATE_GREEN, 1.0f, MOD_HOLD);
	aKey2->AddValue(STATE_BLUE, 1.0f, MOD_HOLD);
	aKey2->AddValue(STATE_ALPHA, 0.6f, MOD_TOWARD_NEXT);
	AnimKey* aKey3 = new AnimKey();
	aKey3->SetDuration(1);
	aKey3->AddValue(STATE_SCALE_X, 2.1f, MOD_TOWARD_NEXT);
	aKey3->AddValue(STATE_SCALE_Y, 2.1f, MOD_TOWARD_NEXT);
	aKey3->AddValue(STATE_RED, 1.0f, MOD_HOLD);
	aKey3->AddValue(STATE_GREEN, 1.0f, MOD_HOLD);
	aKey3->AddValue(STATE_BLUE, 1.0f, MOD_HOLD);
	aKey3->AddValue(STATE_ALPHA, 0.0f, MOD_TOWARD_NEXT);
	AnimTrack* aTrack = new AnimTrack();
	aTrack->AddKey(aKey1, nullptr);
	aTrack->AddKey(aKey2, nullptr);
	aTrack->AddKey(aKey3, nullptr);
	theEmitter->AddTrack(aTrack, nullptr);
	theEmitter->Setup(200, 1, 1, 0, 0, 0, 0, 180);
	theEmitter->SetEmitOnCall();
}

// 0x431030: pale specks drifting from about the place, two every other frame
static void PresetDust(Emitter* theEmitter, int theCount)
{
	Emitter::sParticlesAnim->AddRef();
	theEmitter->mAnim = Emitter::sParticlesAnim;
	theEmitter->SetStateInt(STATE_FRAME, 2);
	theEmitter->SetAdditive();
	AnimKey* aKey1 = new AnimKey();
	aKey1->SetDuration(25);
	aKey1->mDrawMode = 1;
	aKey1->AddValue(STATE_SCALE_X, 0.1f, MOD_TOWARD_NEXT);
	aKey1->AddValue(STATE_SCALE_Y, 0.1f, MOD_TOWARD_NEXT);
	aKey1->AddValue(STATE_RED, 0.3f, MOD_HOLD);
	aKey1->AddValue(STATE_GREEN, 0.3f, MOD_HOLD);
	aKey1->AddValue(STATE_BLUE, 0.4f, MOD_HOLD);
	aKey1->AddValue(STATE_ALPHA, 0.1f, MOD_TOWARD_NEXT);
	aKey1->AddValue(STATE_ROTATION, 0.0f, MOD_TOWARD_NEXT);
	AnimKey* aKey2 = new AnimKey();
	aKey2->SetDuration(70);
	aKey2->mDrawMode = 1;
	aKey2->AddValue(STATE_SCALE_X, 0.32f, MOD_TOWARD_NEXT);
	aKey2->AddValue(STATE_SCALE_Y, 0.32f, MOD_TOWARD_NEXT);
	aKey2->AddValue(STATE_RED, 0.8f, MOD_HOLD);
	aKey2->AddValue(STATE_GREEN, 0.8f, MOD_HOLD);
	aKey2->AddValue(STATE_BLUE, 0.7f, MOD_HOLD);
	aKey2->AddValue(STATE_ALPHA, 0.6f, MOD_TOWARD_NEXT);
	aKey2->AddValue(STATE_ROTATION, 1.5707964f, MOD_TOWARD_NEXT);
	AnimKey* aKey3 = new AnimKey();
	aKey3->SetDuration(0);
	aKey3->mDrawMode = 1;
	aKey3->AddValue(STATE_SCALE_X, 0.02f, MOD_TOWARD_NEXT);
	aKey3->AddValue(STATE_SCALE_Y, 0.02f, MOD_TOWARD_NEXT);
	aKey3->AddValue(STATE_RED, 0.3f, MOD_HOLD);
	aKey3->AddValue(STATE_GREEN, 0.3f, MOD_HOLD);
	aKey3->AddValue(STATE_BLUE, 0.4f, MOD_HOLD);
	aKey3->AddValue(STATE_ALPHA, 0.0f, MOD_TOWARD_NEXT);
	aKey3->AddValue(STATE_ROTATION, 0.0f, MOD_TOWARD_NEXT);
	AnimTrack* aTrack = new AnimTrack();
	aTrack->AddKey(aKey1, nullptr);
	aTrack->AddKey(aKey2, nullptr);
	aTrack->AddKey(aKey3, nullptr);
	theEmitter->AddTrack(aTrack, nullptr);
	theEmitter->Setup(theCount, 2, 2, 0, 0, 20, 16, 180);
	theEmitter->SetEmitOnCall();
}

// 0x4314b0: a white glow swelling to two and a half times its size over 100 frames
static void PresetSlowSwell(Emitter* theEmitter)
{
	Emitter::sParticlesAnim->AddRef();
	theEmitter->mAnim = Emitter::sParticlesAnim;
	theEmitter->SetStateInt(STATE_FRAME, 5);
	theEmitter->SetAdditive();
	AnimKey* aKey1 = new AnimKey();
	aKey1->SetDuration(5);
	aKey1->AddValue(STATE_SCALE_X, 0.0f, MOD_TOWARD_NEXT);
	aKey1->AddValue(STATE_SCALE_Y, 0.0f, MOD_TOWARD_NEXT);
	aKey1->AddValue(STATE_RED, 1.0f, MOD_HOLD);
	aKey1->AddValue(STATE_GREEN, 1.0f, MOD_HOLD);
	aKey1->AddValue(STATE_BLUE, 1.0f, MOD_HOLD);
	aKey1->AddValue(STATE_ALPHA, 0.0f, MOD_TOWARD_NEXT);
	AnimKey* aKey2 = new AnimKey();
	aKey2->SetDuration(100);
	aKey2->AddValue(STATE_SCALE_X, 1.8f, MOD_TOWARD_NEXT);
	aKey2->AddValue(STATE_SCALE_Y, 1.8f, MOD_TOWARD_NEXT);
	aKey2->AddValue(STATE_RED, 1.0f, MOD_HOLD);
	aKey2->AddValue(STATE_GREEN, 1.0f, MOD_HOLD);
	aKey2->AddValue(STATE_BLUE, 1.0f, MOD_HOLD);
	aKey2->AddValue(STATE_ALPHA, 0.5f, MOD_TOWARD_NEXT);
	AnimKey* aKey3 = new AnimKey();
	aKey3->SetDuration(1);
	aKey3->AddValue(STATE_SCALE_X, 2.5f, MOD_TOWARD_NEXT);
	aKey3->AddValue(STATE_SCALE_Y, 2.5f, MOD_TOWARD_NEXT);
	aKey3->AddValue(STATE_RED, 1.0f, MOD_HOLD);
	aKey3->AddValue(STATE_GREEN, 1.0f, MOD_HOLD);
	aKey3->AddValue(STATE_BLUE, 1.0f, MOD_HOLD);
	aKey3->AddValue(STATE_ALPHA, 0.0f, MOD_TOWARD_NEXT);
	AnimTrack* aTrack = new AnimTrack();
	aTrack->AddKey(aKey1, nullptr);
	aTrack->AddKey(aKey2, nullptr);
	aTrack->AddKey(aKey3, nullptr);
	theEmitter->AddTrack(aTrack, nullptr);
	theEmitter->Setup(200, 1, 1, 0, 0, 0, 0, 180);
	theEmitter->SetEmitOnCall();
}

// 0x431880: a white glow swelling as it rises 16 pixels
static void PresetRisingSwell(Emitter* theEmitter, int theCount)
{
	Emitter::sParticlesAnim->AddRef();
	theEmitter->mAnim = Emitter::sParticlesAnim;
	theEmitter->SetStateInt(STATE_FRAME, 5);
	AnimKey* aKey1 = new AnimKey();
	aKey1->SetDuration(4);
	aKey1->AddValue(STATE_SCALE_X, 0.0f, MOD_TOWARD_NEXT);
	aKey1->AddValue(STATE_SCALE_Y, 0.0f, MOD_TOWARD_NEXT);
	aKey1->AddValue(STATE_RED, 1.0f, MOD_HOLD);
	aKey1->AddValue(STATE_GREEN, 1.0f, MOD_HOLD);
	aKey1->AddValue(STATE_BLUE, 1.0f, MOD_HOLD);
	aKey1->AddValue(STATE_ALPHA, 0.0f, MOD_TOWARD_NEXT);
	AnimKey* aKey2 = new AnimKey();
	aKey2->SetDuration(100);
	aKey2->AddValue(STATE_Y, 0.0f, MOD_TOWARD_NEXT);
	aKey2->AddValue(STATE_SCALE_X, 1.3f, MOD_TOWARD_NEXT);
	aKey2->AddValue(STATE_SCALE_Y, 1.3f, MOD_TOWARD_NEXT);
	aKey2->AddValue(STATE_RED, 1.0f, MOD_HOLD);
	aKey2->AddValue(STATE_GREEN, 1.0f, MOD_HOLD);
	aKey2->AddValue(STATE_BLUE, 1.0f, MOD_HOLD);
	aKey2->AddValue(STATE_ALPHA, 0.45f, MOD_TOWARD_NEXT);
	AnimKey* aKey3 = new AnimKey();
	aKey3->SetDuration(1);
	aKey3->AddValue(STATE_Y, -16.0f, MOD_TOWARD_NEXT);
	aKey3->AddValue(STATE_SCALE_X, 1.7f, MOD_TOWARD_NEXT);
	aKey3->AddValue(STATE_SCALE_Y, 1.7f, MOD_TOWARD_NEXT);
	aKey3->AddValue(STATE_RED, 1.0f, MOD_HOLD);
	aKey3->AddValue(STATE_GREEN, 1.0f, MOD_HOLD);
	aKey3->AddValue(STATE_BLUE, 1.0f, MOD_HOLD);
	aKey3->AddValue(STATE_ALPHA, 0.0f, MOD_TOWARD_NEXT);
	AnimTrack* aTrack = new AnimTrack();
	aTrack->AddKey(aKey1, nullptr);
	aTrack->AddKey(aKey2, nullptr);
	aTrack->AddKey(aKey3, nullptr);
	theEmitter->AddTrack(aTrack, nullptr);
	theEmitter->Setup(theCount, 1, 1, 0, 0, 0, 0, 180);
	theEmitter->SetEmitOnCall();
}

// 0x431c90: a white glow swelling a little over 60 frames
static void PresetShortSwell(Emitter* theEmitter, int theCount)
{
	Emitter::sParticlesAnim->AddRef();
	theEmitter->mAnim = Emitter::sParticlesAnim;
	theEmitter->SetStateInt(STATE_FRAME, 5);
	AnimKey* aKey1 = new AnimKey();
	aKey1->SetDuration(60);
	aKey1->AddValue(STATE_SCALE_X, 1.0f, MOD_TOWARD_NEXT);
	aKey1->AddValue(STATE_SCALE_Y, 1.0f, MOD_TOWARD_NEXT);
	aKey1->AddValue(STATE_RED, 1.0f, MOD_HOLD);
	aKey1->AddValue(STATE_GREEN, 1.0f, MOD_HOLD);
	aKey1->AddValue(STATE_BLUE, 1.0f, MOD_HOLD);
	aKey1->AddValue(STATE_ALPHA, 0.45f, MOD_TOWARD_NEXT);
	AnimKey* aKey2 = new AnimKey();
	aKey2->SetDuration(1);
	aKey2->AddValue(STATE_SCALE_X, 1.4f, MOD_TOWARD_NEXT);
	aKey2->AddValue(STATE_SCALE_Y, 1.4f, MOD_TOWARD_NEXT);
	aKey2->AddValue(STATE_RED, 1.0f, MOD_HOLD);
	aKey2->AddValue(STATE_GREEN, 1.0f, MOD_HOLD);
	aKey2->AddValue(STATE_BLUE, 1.0f, MOD_HOLD);
	aKey2->AddValue(STATE_ALPHA, 0.0f, MOD_TOWARD_NEXT);
	AnimTrack* aTrack = new AnimTrack();
	aTrack->AddKey(aKey1, nullptr);
	aTrack->AddKey(aKey2, nullptr);
	theEmitter->AddTrack(aTrack, nullptr);
	theEmitter->Setup(theCount, 1, 1, 0, 0, 0, 0, 180);
	theEmitter->SetEmitOnCall();
}

// 0x431f40: blue sparks orbiting outward, two a frame
static void PresetBlueSparks(Emitter* theEmitter)
{
	Emitter::sParticlesAnim->AddRef();
	theEmitter->mAnim = Emitter::sParticlesAnim;
	theEmitter->SetStateInt(STATE_FRAME, 2);
	theEmitter->SetAdditive();
	AnimKey* aKey1 = new AnimKey();
	aKey1->SetDuration(1);
	aKey1->mDrawMode = 1;
	aKey1->AddValue(STATE_X, 0.0f, MOD_ORBIT_COS);
	aKey1->AddValue(STATE_Y, 0.0f, MOD_ORBIT_SIN);
	aKey1->AddValue(STATE_SCALE_X, 0.0f, MOD_TOWARD_NEXT);
	aKey1->AddValue(STATE_SCALE_Y, 0.0f, MOD_TOWARD_NEXT);
	aKey1->AddValue(STATE_ORBIT_RADIUS, 0.0f, MOD_HOLD);
	aKey1->AddValue(STATE_RED, 1.0f, MOD_TOWARD_NEXT);
	aKey1->AddValue(STATE_GREEN, 1.0f, MOD_TOWARD_NEXT);
	aKey1->AddValue(STATE_BLUE, 1.0f, MOD_TOWARD_NEXT);
	aKey1->AddValue(STATE_ALPHA, 0.0f, MOD_TOWARD_NEXT);
	AnimKey* aKey2 = new AnimKey();
	aKey2->SetDuration(65);
	aKey2->mDrawMode = 1;
	aKey2->AddValue(STATE_X, 0.0f, MOD_ORBIT_COS);
	aKey2->AddValue(STATE_Y, 0.0f, MOD_ORBIT_SIN);
	aKey2->AddValue(STATE_SCALE_X, 0.2f, MOD_TOWARD_NEXT);
	aKey2->AddValue(STATE_SCALE_Y, 0.2f, MOD_TOWARD_NEXT);
	aKey2->AddValue(STATE_ORBIT_RADIUS, 0.9f, MOD_TOWARD_NEXT);
	aKey2->AddValue(STATE_RED, 0.6f, MOD_TOWARD_NEXT);
	aKey2->AddValue(STATE_GREEN, 0.7f, MOD_TOWARD_NEXT);
	aKey2->AddValue(STATE_BLUE, 1.0f, MOD_TOWARD_NEXT);
	aKey2->AddValue(STATE_ALPHA, 0.45f, MOD_TOWARD_NEXT);
	AnimKey* aKey3 = new AnimKey();
	aKey3->SetDuration(5);
	aKey3->mDrawMode = 1;
	aKey3->AddValue(STATE_X, 0.0f, MOD_ORBIT_COS);
	aKey3->AddValue(STATE_Y, 0.0f, MOD_ORBIT_SIN);
	aKey3->AddValue(STATE_SCALE_X, 0.15f, MOD_TOWARD_NEXT);
	aKey3->AddValue(STATE_SCALE_Y, 0.15f, MOD_TOWARD_NEXT);
	aKey3->AddValue(STATE_ORBIT_RADIUS, 0.0f, MOD_HOLD);
	aKey3->AddValue(STATE_RED, 0.3f, MOD_TOWARD_NEXT);
	aKey3->AddValue(STATE_GREEN, 0.4f, MOD_TOWARD_NEXT);
	aKey3->AddValue(STATE_BLUE, 1.0f, MOD_TOWARD_NEXT);
	aKey3->AddValue(STATE_ALPHA, 0.35f, MOD_TOWARD_NEXT);
	AnimKey* aKey4 = new AnimKey();
	aKey4->SetDuration(1);
	aKey4->mDrawMode = 1;
	aKey4->AddValue(STATE_X, 0.0f, MOD_ORBIT_COS);
	aKey4->AddValue(STATE_Y, 0.0f, MOD_ORBIT_SIN);
	aKey4->AddValue(STATE_SCALE_X, 0.0f, MOD_TOWARD_NEXT);
	aKey4->AddValue(STATE_SCALE_Y, 0.0f, MOD_TOWARD_NEXT);
	aKey4->AddValue(STATE_ORBIT_RADIUS, 0.0f, MOD_HOLD);
	aKey4->AddValue(STATE_RED, 1.0f, MOD_HOLD);
	aKey4->AddValue(STATE_GREEN, 1.0f, MOD_HOLD);
	aKey4->AddValue(STATE_BLUE, 1.0f, MOD_HOLD);
	aKey4->AddValue(STATE_ALPHA, 0.0f, MOD_TOWARD_NEXT);
	AnimTrack* aTrack = new AnimTrack();
	aTrack->AddKey(aKey1, nullptr);
	aTrack->AddKey(aKey2, nullptr);
	aTrack->AddKey(aKey3, nullptr);
	aTrack->AddKey(aKey4, nullptr);
	theEmitter->AddTrack(aTrack, nullptr);
	theEmitter->Setup(96, 1, 2, 0, 0, 2, 2, 180);
	theEmitter->SetEmitOnCall();
}

// 0x4325e0: orange sparks reddening as they orbit outward, two a frame
static void PresetRedSparks(Emitter* theEmitter)
{
	Emitter::sParticlesAnim->AddRef();
	theEmitter->mAnim = Emitter::sParticlesAnim;
	theEmitter->SetStateInt(STATE_FRAME, 2);
	theEmitter->SetAdditive();
	AnimKey* aKey1 = new AnimKey();
	aKey1->SetDuration(1);
	aKey1->mDrawMode = 1;
	aKey1->AddValue(STATE_X, 0.0f, MOD_ORBIT_COS);
	aKey1->AddValue(STATE_Y, 0.0f, MOD_ORBIT_SIN);
	aKey1->AddValue(STATE_SCALE_X, 0.0f, MOD_TOWARD_NEXT);
	aKey1->AddValue(STATE_SCALE_Y, 0.0f, MOD_TOWARD_NEXT);
	aKey1->AddValue(STATE_ORBIT_RADIUS, 0.0f, MOD_HOLD);
	aKey1->AddValue(STATE_RED, 1.0f, MOD_TOWARD_NEXT);
	aKey1->AddValue(STATE_GREEN, 1.0f, MOD_TOWARD_NEXT);
	aKey1->AddValue(STATE_BLUE, 1.0f, MOD_TOWARD_NEXT);
	aKey1->AddValue(STATE_ALPHA, 0.0f, MOD_TOWARD_NEXT);
	AnimKey* aKey2 = new AnimKey();
	aKey2->SetDuration(65);
	aKey2->mDrawMode = 1;
	aKey2->AddValue(STATE_X, 0.0f, MOD_ORBIT_COS);
	aKey2->AddValue(STATE_Y, 0.0f, MOD_ORBIT_SIN);
	aKey2->AddValue(STATE_SCALE_X, 0.2f, MOD_TOWARD_NEXT);
	aKey2->AddValue(STATE_SCALE_Y, 0.2f, MOD_TOWARD_NEXT);
	aKey2->AddValue(STATE_ORBIT_RADIUS, 0.9f, MOD_TOWARD_NEXT);
	aKey2->AddValue(STATE_RED, 1.0f, MOD_TOWARD_NEXT);
	aKey2->AddValue(STATE_GREEN, 0.7f, MOD_TOWARD_NEXT);
	aKey2->AddValue(STATE_BLUE, 0.5f, MOD_TOWARD_NEXT);
	aKey2->AddValue(STATE_ALPHA, 0.6f, MOD_TOWARD_NEXT);
	AnimKey* aKey3 = new AnimKey();
	aKey3->SetDuration(5);
	aKey3->mDrawMode = 1;
	aKey3->AddValue(STATE_X, 0.0f, MOD_ORBIT_COS);
	aKey3->AddValue(STATE_Y, 0.0f, MOD_ORBIT_SIN);
	aKey3->AddValue(STATE_SCALE_X, 0.14f, MOD_TOWARD_NEXT);
	aKey3->AddValue(STATE_SCALE_Y, 0.14f, MOD_TOWARD_NEXT);
	aKey3->AddValue(STATE_ORBIT_RADIUS, 0.0f, MOD_HOLD);
	aKey3->AddValue(STATE_RED, 1.0f, MOD_TOWARD_NEXT);
	aKey3->AddValue(STATE_GREEN, 0.1f, MOD_TOWARD_NEXT);
	aKey3->AddValue(STATE_BLUE, 0.1f, MOD_TOWARD_NEXT);
	aKey3->AddValue(STATE_ALPHA, 0.35f, MOD_TOWARD_NEXT);
	AnimKey* aKey4 = new AnimKey();
	aKey4->SetDuration(1);
	aKey4->mDrawMode = 1;
	aKey4->AddValue(STATE_X, 0.0f, MOD_ORBIT_COS);
	aKey4->AddValue(STATE_Y, 0.0f, MOD_ORBIT_SIN);
	aKey4->AddValue(STATE_SCALE_X, 0.0f, MOD_TOWARD_NEXT);
	aKey4->AddValue(STATE_SCALE_Y, 0.0f, MOD_TOWARD_NEXT);
	aKey4->AddValue(STATE_ORBIT_RADIUS, 0.0f, MOD_HOLD);
	aKey4->AddValue(STATE_RED, 1.0f, MOD_HOLD);
	aKey4->AddValue(STATE_GREEN, 1.0f, MOD_HOLD);
	aKey4->AddValue(STATE_BLUE, 1.0f, MOD_HOLD);
	aKey4->AddValue(STATE_ALPHA, 0.0f, MOD_TOWARD_NEXT);
	AnimTrack* aTrack = new AnimTrack();
	aTrack->AddKey(aKey1, nullptr);
	aTrack->AddKey(aKey2, nullptr);
	aTrack->AddKey(aKey3, nullptr);
	aTrack->AddKey(aKey4, nullptr);
	theEmitter->AddTrack(aTrack, nullptr);
	theEmitter->Setup(96, 1, 2, 0, 0, 2, 2, 180);
	theEmitter->SetEmitOnCall();
}

// 0x432c80: grey smoke rising 96 pixels, every fifth frame
static void PresetSmoke(Emitter* theEmitter)
{
	Emitter::sParticlesAnim->AddRef();
	theEmitter->mAnim = Emitter::sParticlesAnim;
	theEmitter->SetStateInt(STATE_FRAME, 2);
	theEmitter->SetAdditive();
	AnimKey* aKey1 = new AnimKey();
	aKey1->SetDuration(5);
	aKey1->mDrawMode = 1;
	aKey1->AddValue(STATE_X, 0.0f, MOD_TOWARD_NEXT);
	aKey1->AddValue(STATE_Y, 0.0f, MOD_TOWARD_NEXT);
	aKey1->AddValue(STATE_SCALE_X, 0.1f, MOD_TOWARD_NEXT);
	aKey1->AddValue(STATE_SCALE_Y, 0.1f, MOD_TOWARD_NEXT);
	aKey1->AddValue(STATE_RED, 0.3f, MOD_TOWARD_NEXT);
	aKey1->AddValue(STATE_GREEN, 0.3f, MOD_TOWARD_NEXT);
	aKey1->AddValue(STATE_BLUE, 0.3f, MOD_TOWARD_NEXT);
	aKey1->AddValue(STATE_ALPHA, 0.0f, MOD_TOWARD_NEXT);
	AnimKey* aKey2 = new AnimKey();
	aKey2->SetDuration(50);
	aKey2->mDrawMode = 1;
	aKey2->AddValue(STATE_X, 0.0f, MOD_TOWARD_NEXT);
	aKey2->AddValue(STATE_Y, 0.0f, MOD_TOWARD_NEXT);
	aKey2->AddValue(STATE_SCALE_X, 0.3f, MOD_TOWARD_NEXT);
	aKey2->AddValue(STATE_SCALE_Y, 0.3f, MOD_TOWARD_NEXT);
	aKey2->AddValue(STATE_RED, 0.3f, MOD_TOWARD_NEXT);
	aKey2->AddValue(STATE_GREEN, 0.4f, MOD_TOWARD_NEXT);
	aKey2->AddValue(STATE_BLUE, 0.5f, MOD_TOWARD_NEXT);
	aKey2->AddValue(STATE_ALPHA, 1.0f, MOD_TOWARD_NEXT);
	AnimKey* aKey3 = new AnimKey();
	aKey3->mDrawMode = 1;
	aKey3->SetDuration(1);
	aKey3->AddValue(STATE_X, 0.0f, MOD_TOWARD_NEXT);
	aKey3->AddValue(STATE_Y, -96.0f, MOD_TOWARD_NEXT);
	aKey3->AddValue(STATE_SCALE_X, 0.2f, MOD_TOWARD_NEXT);
	aKey3->AddValue(STATE_SCALE_Y, 0.2f, MOD_TOWARD_NEXT);
	aKey3->AddValue(STATE_RED, 0.1f, MOD_TOWARD_NEXT);
	aKey3->AddValue(STATE_GREEN, 0.1f, MOD_TOWARD_NEXT);
	aKey3->AddValue(STATE_BLUE, 0.2f, MOD_TOWARD_NEXT);
	aKey3->AddValue(STATE_ALPHA, 0.0f, MOD_TOWARD_NEXT);
	AnimTrack* aTrack = new AnimTrack();
	aTrack->AddKey(aKey1, nullptr);
	aTrack->AddKey(aKey2, nullptr);
	aTrack->AddKey(aKey3, nullptr);
	theEmitter->AddTrack(aTrack, nullptr);
	theEmitter->Setup(64, 5, 1, 0, 0, 32, 32, 0);
	theEmitter->SetEmitOnCall();
	theEmitter->Deactivate(theEmitter);
}

// 0x433160: ten grey specks going out in a ring
static void PresetGreyRing(Emitter* theEmitter)
{
	Emitter::sParticlesAnim->AddRef();
	theEmitter->mAnim = Emitter::sParticlesAnim;
	theEmitter->SetStateInt(STATE_FRAME, 2);
	theEmitter->SetAdditive();
	AnimKey* aKey1 = new AnimKey();
	aKey1->SetDuration(23);
	aKey1->mDrawMode = 1;
	aKey1->AddValue(STATE_X, 0.0f, MOD_ORBIT_COS);
	aKey1->AddValue(STATE_Y, 0.0f, MOD_ORBIT_SIN);
	aKey1->AddValue(STATE_SCALE_X, 0.1f, MOD_TOWARD_NEXT);
	aKey1->AddValue(STATE_SCALE_Y, 0.1f, MOD_TOWARD_NEXT);
	aKey1->AddValue(STATE_RED, 0.1f, MOD_TOWARD_NEXT);
	aKey1->AddValue(STATE_GREEN, 0.1f, MOD_TOWARD_NEXT);
	aKey1->AddValue(STATE_BLUE, 0.6f, MOD_TOWARD_NEXT);
	aKey1->AddValue(STATE_ALPHA, 0.2f, MOD_TOWARD_NEXT);
	aKey1->AddValue(STATE_ORBIT_RADIUS, 3.7f, MOD_TOWARD_NEXT);
	AnimKey* aKey2 = new AnimKey();
	aKey2->SetDuration(80);
	aKey2->mDrawMode = 1;
	aKey2->AddValue(STATE_X, 0.0f, MOD_ORBIT_COS);
	aKey2->AddValue(STATE_Y, 0.0f, MOD_ORBIT_SIN);
	aKey2->AddValue(STATE_SCALE_X, 0.35f, MOD_TOWARD_NEXT);
	aKey2->AddValue(STATE_SCALE_Y, 0.35f, MOD_TOWARD_NEXT);
	aKey2->AddValue(STATE_RED, 0.6f, MOD_TOWARD_NEXT);
	aKey2->AddValue(STATE_GREEN, 0.6f, MOD_TOWARD_NEXT);
	aKey2->AddValue(STATE_BLUE, 0.6f, MOD_TOWARD_NEXT);
	aKey2->AddValue(STATE_ALPHA, 0.5f, MOD_TOWARD_NEXT);
	aKey2->AddValue(STATE_ORBIT_RADIUS, 0.0f, MOD_TOWARD_NEXT);
	aKey2->AddValue(STATE_ROTATION, 0.0f, MOD_TOWARD_NEXT);
	AnimKey* aKey3 = new AnimKey();
	aKey3->SetDuration(18);
	aKey3->mDrawMode = 1;
	aKey3->AddValue(STATE_X, 0.0f, MOD_ORBIT_COS);
	aKey3->AddValue(STATE_Y, 0.0f, MOD_ORBIT_SIN);
	aKey3->AddValue(STATE_SCALE_X, 0.3f, MOD_TOWARD_NEXT);
	aKey3->AddValue(STATE_SCALE_Y, 0.3f, MOD_TOWARD_NEXT);
	aKey3->AddValue(STATE_RED, 0.6f, MOD_HOLD);
	aKey3->AddValue(STATE_GREEN, 0.6f, MOD_HOLD);
	aKey3->AddValue(STATE_BLUE, 0.6f, MOD_HOLD);
	aKey3->AddValue(STATE_ALPHA, 0.3f, MOD_HOLD);
	aKey3->AddValue(STATE_ORBIT_RADIUS, 0.0f, MOD_HOLD);
	aKey3->AddValue(STATE_ROTATION, 1.5707964f, MOD_HOLD);
	aKey3->AddValue(STATE_ORBIT_ANGLE, 1.5707964f, MOD_HOLD);
	AnimKey* aKey4 = new AnimKey();
	aKey4->SetDuration(1);
	aKey4->mDrawMode = 1;
	aKey4->AddValue(STATE_X, 0.0f, MOD_ORBIT_COS);
	aKey4->AddValue(STATE_Y, 0.0f, MOD_ORBIT_SIN);
	aKey4->AddValue(STATE_SCALE_X, 0.0f, MOD_TOWARD_NEXT);
	aKey4->AddValue(STATE_SCALE_Y, 0.0f, MOD_TOWARD_NEXT);
	aKey4->AddValue(STATE_RED, 0.0f, MOD_HOLD);
	aKey4->AddValue(STATE_GREEN, 0.0f, MOD_HOLD);
	aKey4->AddValue(STATE_BLUE, 0.0f, MOD_HOLD);
	aKey4->AddValue(STATE_ALPHA, 0.0f, MOD_HOLD);
	aKey4->AddValue(STATE_ORBIT_RADIUS, 0.0f, MOD_HOLD);
	aKey4->AddValue(STATE_ROTATION, 1.5707964f, MOD_HOLD);
	aKey4->AddValue(STATE_ORBIT_ANGLE, 1.5707964f, MOD_HOLD);
	AnimTrack* aTrack = new AnimTrack();
	aTrack->AddKey(aKey1, nullptr);
	aTrack->AddKey(aKey2, nullptr);
	aTrack->AddKey(aKey3, nullptr);
	aTrack->AddKey(aKey4, nullptr);
	theEmitter->AddTrack(aTrack, nullptr);
	theEmitter->Setup(10, 1, 10, 0, 0, 0, 0, 180);
	theEmitter->SetEmitOnCall();
}

// 0x4338f0: 24 pale glows going out in a ring and darkening
static void PresetPaleBurst(Emitter* theEmitter)
{
	Emitter::sParticlesAnim->AddRef();
	theEmitter->mAnim = Emitter::sParticlesAnim;
	theEmitter->SetStateInt(STATE_FRAME, 0);
	theEmitter->SetAdditive();
	AnimKey* aKey1 = new AnimKey();
	aKey1->SetDuration(1);
	aKey1->mDrawMode = 1;
	aKey1->AddValue(STATE_X, 0.0f, MOD_ORBIT_COS);
	aKey1->AddValue(STATE_Y, 0.0f, MOD_ORBIT_SIN);
	aKey1->AddValue(STATE_SCALE_X, 0.2f, MOD_TOWARD_NEXT);
	aKey1->AddValue(STATE_SCALE_Y, 0.2f, MOD_TOWARD_NEXT);
	aKey1->AddValue(STATE_RED, 0.5f, MOD_HOLD);
	aKey1->AddValue(STATE_GREEN, 0.5f, MOD_HOLD);
	aKey1->AddValue(STATE_BLUE, 0.5f, MOD_HOLD);
	aKey1->AddValue(STATE_ALPHA, 0.1f, MOD_TOWARD_NEXT);
	aKey1->AddValue(STATE_ORBIT_RADIUS, 0.1f, MOD_TOWARD_NEXT);
	AnimKey* aKey2 = new AnimKey();
	aKey2->SetDuration(50);
	aKey2->mDrawMode = 1;
	aKey2->AddValue(STATE_X, 0.0f, MOD_ORBIT_COS);
	aKey2->AddValue(STATE_Y, 0.0f, MOD_ORBIT_SIN);
	aKey2->AddValue(STATE_SCALE_X, 1.0f, MOD_TOWARD_NEXT);
	aKey2->AddValue(STATE_SCALE_Y, 1.0f, MOD_TOWARD_NEXT);
	aKey2->AddValue(STATE_RED, 0.7f, MOD_HOLD);
	aKey2->AddValue(STATE_GREEN, 0.7f, MOD_HOLD);
	aKey2->AddValue(STATE_BLUE, 0.7f, MOD_HOLD);
	aKey2->AddValue(STATE_ALPHA, 1.0f, MOD_TOWARD_NEXT);
	aKey2->AddValue(STATE_ORBIT_RADIUS, 2.6f, MOD_TOWARD_NEXT);
	AnimKey* aKey3 = new AnimKey();
	aKey3->SetDuration(10);
	aKey3->mDrawMode = 1;
	aKey3->AddValue(STATE_X, 0.0f, MOD_ORBIT_COS);
	aKey3->AddValue(STATE_Y, 0.0f, MOD_ORBIT_SIN);
	aKey3->AddValue(STATE_SCALE_X, 0.3f, MOD_TOWARD_NEXT);
	aKey3->AddValue(STATE_SCALE_Y, 0.3f, MOD_TOWARD_NEXT);
	aKey3->AddValue(STATE_RED, 0.3f, MOD_HOLD);
	aKey3->AddValue(STATE_GREEN, 0.1f, MOD_HOLD);
	aKey3->AddValue(STATE_BLUE, 0.1f, MOD_HOLD);
	aKey3->AddValue(STATE_ALPHA, 0.0f, MOD_TOWARD_NEXT);
	aKey3->AddValue(STATE_ORBIT_RADIUS, 0.0f, MOD_TOWARD_NEXT);
	AnimTrack* aTrack = new AnimTrack();
	aTrack->AddKey(aKey1, nullptr);
	aTrack->AddKey(aKey2, nullptr);
	aTrack->AddKey(aKey3, nullptr);
	theEmitter->AddTrack(aTrack, nullptr);
	theEmitter->Setup(24, 1, 24, 0, 0, 0, 0, 180);
	theEmitter->SetEmitOnCall();
}

// 0x433e30: 32 gold glows going out in a ring
static void PresetGoldBurst(Emitter* theEmitter)
{
	Emitter::sParticlesAnim->AddRef();
	theEmitter->mAnim = Emitter::sParticlesAnim;
	theEmitter->SetStateInt(STATE_FRAME, 0);
	theEmitter->SetAdditive();
	AnimKey* aKey1 = new AnimKey();
	aKey1->SetDuration(1);
	aKey1->mDrawMode = 1;
	aKey1->AddValue(STATE_X, 0.0f, MOD_ORBIT_COS);
	aKey1->AddValue(STATE_Y, 0.0f, MOD_ORBIT_SIN);
	aKey1->AddValue(STATE_SCALE_X, 0.2f, MOD_TOWARD_NEXT);
	aKey1->AddValue(STATE_SCALE_Y, 0.2f, MOD_TOWARD_NEXT);
	aKey1->AddValue(STATE_RED, 0.5f, MOD_HOLD);
	aKey1->AddValue(STATE_GREEN, 0.5f, MOD_HOLD);
	aKey1->AddValue(STATE_BLUE, 0.5f, MOD_HOLD);
	aKey1->AddValue(STATE_ALPHA, 0.1f, MOD_TOWARD_NEXT);
	aKey1->AddValue(STATE_ORBIT_RADIUS, 0.1f, MOD_TOWARD_NEXT);
	AnimKey* aKey2 = new AnimKey();
	aKey2->SetDuration(60);
	aKey2->mDrawMode = 1;
	aKey2->AddValue(STATE_X, 0.0f, MOD_ORBIT_COS);
	aKey2->AddValue(STATE_Y, 0.0f, MOD_ORBIT_SIN);
	aKey2->AddValue(STATE_SCALE_X, 1.0f, MOD_TOWARD_NEXT);
	aKey2->AddValue(STATE_SCALE_Y, 1.0f, MOD_TOWARD_NEXT);
	aKey2->AddValue(STATE_RED, 0.9f, MOD_HOLD);
	aKey2->AddValue(STATE_GREEN, 0.7f, MOD_HOLD);
	aKey2->AddValue(STATE_BLUE, 0.3f, MOD_HOLD);
	aKey2->AddValue(STATE_ALPHA, 1.0f, MOD_TOWARD_NEXT);
	aKey2->AddValue(STATE_ORBIT_RADIUS, 2.6f, MOD_TOWARD_NEXT);
	AnimKey* aKey3 = new AnimKey();
	aKey3->SetDuration(10);
	aKey3->mDrawMode = 1;
	aKey3->AddValue(STATE_X, 0.0f, MOD_ORBIT_COS);
	aKey3->AddValue(STATE_Y, 0.0f, MOD_ORBIT_SIN);
	aKey3->AddValue(STATE_SCALE_X, 0.3f, MOD_TOWARD_NEXT);
	aKey3->AddValue(STATE_SCALE_Y, 0.3f, MOD_TOWARD_NEXT);
	aKey3->AddValue(STATE_RED, 0.3f, MOD_HOLD);
	aKey3->AddValue(STATE_GREEN, 0.1f, MOD_HOLD);
	aKey3->AddValue(STATE_BLUE, 0.0f, MOD_HOLD);
	aKey3->AddValue(STATE_ALPHA, 0.0f, MOD_TOWARD_NEXT);
	aKey3->AddValue(STATE_ORBIT_RADIUS, 0.0f, MOD_TOWARD_NEXT);
	AnimTrack* aTrack = new AnimTrack();
	aTrack->AddKey(aKey1, nullptr);
	aTrack->AddKey(aKey2, nullptr);
	aTrack->AddKey(aKey3, nullptr);
	theEmitter->AddTrack(aTrack, nullptr);
	theEmitter->Setup(32, 1, 32, 0, 0, 0, 0, 180);
	theEmitter->SetEmitOnCall();
}


// 0x42fd80
EmitterSet::EmitterSet(ObjBase* theParent)
{
	mParent = theParent;
	mFlame = nullptr;
	mStarRing = nullptr;
	mSwell = nullptr;
	mDust = nullptr;
	mSlowSwell = nullptr;
	mRisingSwell = nullptr;
	mShortSwell = nullptr;
	mBlueSparks = nullptr;
	mRedSparks = nullptr;
	mSmoke = nullptr;
	mGreyRing = nullptr;
	mPaleBurst = nullptr;
	mGoldBurst = nullptr;
}

// 0x42fdb0
EmitterSet::~EmitterSet()
{
	if (mFlame != nullptr)
		delete mFlame;
	if (mStarRing != nullptr)
		delete mStarRing;
	if (mSwell != nullptr)
		delete mSwell;
	if (mDust != nullptr)
		delete mDust;
	if (mSlowSwell != nullptr)
		delete mSlowSwell;
	if (mRisingSwell != nullptr)
		delete mRisingSwell;
	if (mShortSwell != nullptr)
		delete mShortSwell;
	if (mBlueSparks != nullptr)
		delete mBlueSparks;
	if (mRedSparks != nullptr)
		delete mRedSparks;
	if (mSmoke != nullptr)
		delete mSmoke;
	if (mGreyRing != nullptr)
		delete mGreyRing;
	if (mPaleBurst != nullptr)
		delete mPaleBurst;
	if (mGoldBurst != nullptr)
		delete mGoldBurst;
}

// 0x42fed0
void EmitterSet::MakeFlame(float theDepth)
{
	mFlame = new Emitter(mParent, true);
	PresetFlame(mFlame);
	Emitter::SetDepth(mFlame, theDepth);
}

// 0x42fe80
void EmitterSet::MakeStarRing(float theDepth, int theCount)
{
	mStarRing = new Emitter(mParent, true);
	PresetStarRing(mStarRing, theCount);
	Emitter::SetDepth(mStarRing, theDepth);
}

// 0x42ff10
void EmitterSet::MakeSwell(float theDepth)
{
	mSwell = new Emitter(mParent, true);
	PresetSwell(mSwell);
	Emitter::SetDepth(mSwell, theDepth);
}

// 0x42ff50
void EmitterSet::MakeDust(float theDepth, int theCount)
{
	mDust = new Emitter(mParent, true);
	PresetDust(mDust, theCount);
	Emitter::SetDepth(mDust, theDepth);
}

// 0x42ffa0
void EmitterSet::MakeSlowSwell(float theDepth)
{
	mSlowSwell = new Emitter(mParent, true);
	PresetSlowSwell(mSlowSwell);
	Emitter::SetDepth(mSlowSwell, theDepth);
}

// 0x42ffe0
void EmitterSet::MakeRisingSwell(float theDepth, int theCount)
{
	mRisingSwell = new Emitter(mParent, true);
	PresetRisingSwell(mRisingSwell, theCount);
	Emitter::SetDepth(mRisingSwell, theDepth);
}

// 0x430030
void EmitterSet::MakeShortSwell(float theDepth, int theCount)
{
	mShortSwell = new Emitter(mParent, true);
	PresetShortSwell(mShortSwell, theCount);
	Emitter::SetDepth(mShortSwell, theDepth);
}

// 0x430080
void EmitterSet::MakeBlueSparks(float theDepth)
{
	mBlueSparks = new Emitter(mParent, true);
	PresetBlueSparks(mBlueSparks);
	Emitter::SetDepth(mBlueSparks, theDepth);
}

// 0x4300c0
void EmitterSet::MakeRedSparks(float theDepth)
{
	mRedSparks = new Emitter(mParent, true);
	PresetRedSparks(mRedSparks);
	Emitter::SetDepth(mRedSparks, theDepth);
}

// 0x430100
void EmitterSet::MakeSmoke(float theDepth)
{
	mSmoke = new Emitter(mParent, true);
	PresetSmoke(mSmoke);
	Emitter::SetDepth(mSmoke, theDepth);
}

// 0x430140
void EmitterSet::MakeGreyRing(float theDepth)
{
	mGreyRing = new Emitter(mParent, true);
	PresetGreyRing(mGreyRing);
	Emitter::SetDepth(mGreyRing, theDepth);
}

// 0x430180
void EmitterSet::MakePaleBurst(float theDepth)
{
	mPaleBurst = new Emitter(mParent, true);
	PresetPaleBurst(mPaleBurst);
	Emitter::SetDepth(mPaleBurst, theDepth);
}

// 0x4301c0
void EmitterSet::MakeGoldBurst(float theDepth)
{
	mGoldBurst = new Emitter(mParent, true);
	PresetGoldBurst(mGoldBurst);
	Emitter::SetDepth(mGoldBurst, theDepth);
}

// 0x4343d0: shown, hidden, first
void EmitterSet::BurstFlame(float theX, float theY)
{
	if (mFlame == nullptr)
		return;
	mFlame->Activate(mFlame, true);
	mFlame->Emit(theX, theY, 0.0f);
}

// 0x4343a0
void EmitterSet::BurstStarRing(float theX, float theY)
{
	if (mStarRing == nullptr)
		return;
	mStarRing->Emit(theX, theY, 0.0f);
}

// 0x434410
void EmitterSet::BurstSwell(float theX, float theY)
{
	if (mSwell == nullptr)
		return;
	mSwell->Emit(theX, theY, (float)(GameRandomNext() % 360));
}

// 0x434370
void EmitterSet::BurstDust(float theX, float theY)
{
	if (mDust == nullptr)
		return;
	mDust->Emit(theX, theY, 0.0f);
}

// 0x434460
void EmitterSet::BurstSlowSwell(float theX, float theY)
{
	if (mSlowSwell == nullptr)
		return;
	mSlowSwell->Emit(theX, theY, (float)(GameRandomNext() % 360));
}

// 0x4344b0
void EmitterSet::BurstRisingSwell(float theX, float theY)
{
	if (mRisingSwell == nullptr)
		return;
	mRisingSwell->Emit(theX, theY, (float)(GameRandomNext() % 360));
}

// 0x434500
void EmitterSet::BurstShortSwell(float theX, float theY)
{
	if (mShortSwell == nullptr)
		return;
	mShortSwell->Emit(theX, theY, (float)(GameRandomNext() % 360));
}

// 0x434550
void EmitterSet::BurstBlueSparks(ObjBase* theParent, float theX, float theY)
{
	if (mBlueSparks == nullptr)
		return;
	if (theParent != nullptr)
		mBlueSparks->SetParent(theParent, nullptr);
	mBlueSparks->Emit(theX, theY, (float)(GameRandomNext() % 360));
}

// 0x4345b0
void EmitterSet::BurstRedSparks(float theX, float theY)
{
	if (mRedSparks == nullptr)
		return;
	mRedSparks->Emit(theX, theY, (float)(GameRandomNext() % 360));
}

// 0x434600
void EmitterSet::BurstSmoke(float theX, float theY)
{
	if (mSmoke == nullptr)
		return;
	mSmoke->Emit(theX, theY, (float)(GameRandomNext() % 360));
}

// 0x434650
void EmitterSet::BurstGreyRing(float theX, float theY)
{
	if (mGreyRing == nullptr)
		return;
	mGreyRing->Emit(theX, theY, 0.0f);
}

// 0x434680
void EmitterSet::BurstPaleBurst(float theX, float theY)
{
	if (mPaleBurst == nullptr)
		return;
	mPaleBurst->Emit(theX, theY, 0.0f);
}

// 0x4346b0
void EmitterSet::BurstGoldBurst(float theX, float theY)
{
	if (mGoldBurst == nullptr)
		return;
	mGoldBurst->Emit(theX, theY, 0.0f);
}
