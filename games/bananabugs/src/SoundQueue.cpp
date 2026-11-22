#include "SoundQueue.h"

#include "ObjBase.h"
#include "SexyAppBase.h"
#include "SoundInstance.h"
#include "SoundManager.h"

using namespace Sexy;

// 0x46c7e0
SoundQueue::SoundQueue(SexyAppBase* theApp)
{
	mApp = theApp;
	mFirstId = 1;
	mLastId = 0x60;
	for (int i = 0; i < SOUND_QUEUE_SLOTS; i++) {
		if (i < mFirstId || mLastId < i) {
			mCues[i] = nullptr;
		} else {
			SoundCue* aCue = new SoundCue();
			mCues[i] = aCue;
			aCue->mSoundId = i;
			aCue->mPending = false;
			aCue->mCooldown = 0;
			aCue->mPitchRange = 0;
			aCue->mDelay = 0;
			aCue->mDelayReset = 0;
			aCue->mLoop = false;
			aCue->mLoopInstance = nullptr;
		}
	}
	// mFrame is left as operator new gave it in the original; nothing reads it
	mFrame = 0;
}

// The original never deletes the queue (it lives until the process ends); the port frees the cues with it
SoundQueue::~SoundQueue()
{
	for (int i = 0; i < SOUND_QUEUE_SLOTS; i++)
		delete mCues[i];
}

// 0x46c850
void SoundQueue::Request(int theSoundId, int thePitchRange, bool loop)
{
	SoundCue* aCue = mCues[theSoundId];
	if (aCue != nullptr) {
		aCue->mPending = true;
		aCue->mPitchRange = thePitchRange;
		aCue->mLoop = loop;
	}
}

// 0x46c870
void SoundQueue::StopLoop(int theSoundId)
{
	SoundCue* aCue = mCues[theSoundId];
	if (aCue != nullptr && aCue->mLoopInstance != nullptr) {
		aCue->mLoopInstance->Release();
		aCue->mLoopInstance = nullptr;
	}
}

// 0x46c890: sound 16 (SOUND_FILL1; the ids count from SOUND_FXSTART, 0) waits five frames before each play
void SoundQueue::DelayFillSound()
{
	if (mCues[16] != nullptr) {
		mCues[16]->mDelay = 5;
		mCues[16]->mDelayReset = 5;
	}
}

// 0x46c8b0
bool SoundQueue::Update()
{
	mFrame++;
	if (mApp->mSoundManager == nullptr)
		return false;
	for (int anId = mFirstId; anId <= mLastId; anId++) {
		SoundCue* aCue = mCues[anId];
		if (0 < aCue->mCooldown)
			aCue->mCooldown--;
		if (!aCue->mPending)
			continue;
		if (0 < aCue->mDelay) {
			aCue->mDelay--;
			continue;
		}
		aCue->mPending = false;
		if (0 < aCue->mCooldown)
			continue;
		aCue->mDelay = aCue->mDelayReset;
		SoundInstance* anInstance;
		bool autoRelease;
		if (aCue->mLoop) {
			anInstance = aCue->mLoopInstance;
			if (anInstance == nullptr) {
				anInstance = mApp->mSoundManager->GetSoundInstance(anId);
				aCue->mLoopInstance = anInstance;
			}
			autoRelease = false;
		} else {
			anInstance = mApp->mSoundManager->GetSoundInstance(anId);
			autoRelease = true;
		}
		if (anInstance == nullptr)
			continue;
		if (0 < aCue->mPitchRange) {
			unsigned int aRandom = GameRandomNext();
			anInstance->AdjustPitch((double)(int)(aRandom % (unsigned int)(aCue->mPitchRange + 1)));
		}
		anInstance->Play(aCue->mLoop, autoRelease);
		aCue->mCooldown = 5;
	}
	return true;
}
