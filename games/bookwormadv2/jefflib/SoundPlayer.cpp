#include "SoundPlayer.h"

#include "SexyAppBase.h"
#include "SoundInstance.h"
#include "SoundManager.h"

using namespace JeffLib;

// 0x6af5a0. port: the original released only the cached instances; the chained and looping samples' are the
// player's too
SoundPlayer::~SoundPlayer()
{
	for (Sexy::SoundInstance*& anInstance : mCached) {
		if (anInstance != nullptr)
			anInstance->Release();
		anInstance = nullptr;
	}
	for (ChainedSample& aSample : mChained)
		aSample.mInstance->Release();
	mChained.clear();
	for (auto& aLoop : mLooping)
		aLoop.second.mInstance->Release();
	mLooping.clear();
}

void SoundPlayer::Update()
{
	mUpdateCount++;

	for (size_t i = 0; i < mDelayed.size(); i++) {
		DelayedSample& aSample = mDelayed[i];
		if (aSample.mCount-- <= 0) {
			PlayPitchShiftedSample(aSample.mId, aSample.mPitch, 0);
			mDelayed.erase(mDelayed.begin() + i);
			i--;
		}
	}

	for (size_t i = 0; i < mChained.size(); i++) {
		ChainedSample& aSample = mChained[i];
		if (!aSample.mInstance->IsPlaying() && --aSample.mCount <= 0) {
			aSample.mInstance->Release();
			PlaySampleMinTime(aSample.mNextId, 0);
			mChained.erase(mChained.begin() + i);
			i--;
		}
	}

	for (auto anIt = mLooping.begin(); anIt != mLooping.end(); ++anIt) {
		LoopedSample& aLoop = anIt->second;
		if (0.0f < aLoop.mFade) {
			double aVolume = aLoop.mInstance->GetVolume() - aLoop.mFade;
			if (!(aVolume > 0.0)) {
				aLoop.mInstance->Release();
				mLooping.erase(anIt);
				return;
			}
			aLoop.mInstance->SetVolume(aVolume);
		}
	}
}

bool SoundPlayer::PlayedWithin(int theId, int theMinUpdates) const
{
	auto anIt = mLastPlayed.find(theId);
	return anIt != mLastPlayed.end() && mUpdateCount - anIt->second <= theMinUpdates;
}

void SoundPlayer::PlaySampleMinTime(int theId, int theMinUpdates)
{
	if (theId < 0 || theId >= MAX_SAMPLES || PlayedWithin(theId, theMinUpdates))
		return;
	bool aNew = mCached[theId] == nullptr;
	Sexy::SoundInstance* anInstance = aNew ? mApp->mSoundManager->GetSoundInstance(theId) : mCached[theId];
	if (anInstance != nullptr)
		anInstance->Play(false, aNew);
	mLastPlayed[theId] = mUpdateCount;
}

void SoundPlayer::PlaySamplePan(int theId, int thePan)
{
	if (theId < 0 || theId >= MAX_SAMPLES)
		return;
	auto anIt = mLastPlayed.find(theId);
	if (anIt != mLastPlayed.end() && mUpdateCount - anIt->second < 6)
		return;
	bool aNew = mCached[theId] == nullptr;
	Sexy::SoundInstance* anInstance = aNew ? mApp->mSoundManager->GetSoundInstance(theId) : mCached[theId];
	if (anInstance != nullptr) {
		anInstance->SetPan(thePan);
		anInstance->Play(false, aNew);
	}
	mLastPlayed[theId] = mUpdateCount;
}

void SoundPlayer::PlayCachedSample(int theId, int thePan, int theMinUpdates)
{
	if (theId < 0 || theId >= MAX_SAMPLES || PlayedWithin(theId, theMinUpdates))
		return;
	if (mCached[theId] == nullptr)
		mCached[theId] = mApp->mSoundManager->GetSoundInstance(theId);
	if (Sexy::SoundInstance* anInstance = mCached[theId]) {
		if (thePan != 0)
			anInstance->SetPan(thePan);
		anInstance->Play(false, false);
	}
	mLastPlayed[theId] = mUpdateCount;
}

bool SoundPlayer::IsCachedSamplePlaying(int theId) const
{
	return theId >= 0 && theId < MAX_SAMPLES && mCached[theId] != nullptr && mCached[theId]->IsPlaying();
}

void SoundPlayer::PlayDelayedSample(int theId, int theDelay, int thePitch)
{
	mDelayed.push_back({theId, theDelay, thePitch});
}

void SoundPlayer::PlayChainedSample(int theId, int theNextId, int theDelay, int thePitch)
{
	if (mApp->mSoundManager == nullptr)
		return;
	Sexy::SoundInstance* anInstance = mApp->mSoundManager->GetSoundInstance(theId);
	if (anInstance == nullptr)
		return;
	if (thePitch != 0)
		anInstance->AdjustPitch(thePitch);
	anInstance->Play(false, false);
	mChained.push_back({theId, theNextId, theDelay, anInstance});
}

void SoundPlayer::LoopSample(int theId)
{
	if (mLooping.find(theId) != mLooping.end())
		return;
	if (Sexy::SoundInstance* anInstance = mApp->mSoundManager->GetSoundInstance(theId)) {
		anInstance->Play(true, false);
		mLooping[theId] = {anInstance, 0.0f};
	}
}

void SoundPlayer::StopSample(int theId)
{
	auto anIt = mLooping.find(theId);
	if (anIt != mLooping.end()) {
		anIt->second.mInstance->Release();
		mLooping.erase(anIt);
	}
}

void SoundPlayer::FadeOutSample(int theId, float theFade)
{
	auto anIt = mLooping.find(theId);
	if (anIt != mLooping.end())
		anIt->second.mFade = theFade;
}

void SoundPlayer::PlayPitchShiftedSample(int theId, int thePitch, int theMinUpdates)
{
	if (mApp->mSoundManager == nullptr)
		return;
	if (thePitch == 0) {
		PlaySampleMinTime(theId, theMinUpdates);
		return;
	}
	Sexy::SoundInstance* anInstance = mApp->mSoundManager->GetSoundInstance(theId);
	if (anInstance == nullptr)
		return;
	auto anIt = mLastPlayed.find(theId);
	if (anIt == mLastPlayed.end() || mUpdateCount - anIt->second > theMinUpdates) {
		anInstance->AdjustPitch(thePitch);
		anInstance->Play(false, true);
		mLastPlayed[theId] = mUpdateCount;
	} else {
		anInstance->Release();
	}
}

void SoundPlayer::PauseLooping(bool thePause)
{
	mPauseCount += (thePause ? 1 : 0) * 2 - 1;
	if (mPauseCount < 0)
		mPauseCount = 0;
	if (thePause || mPauseCount == 0) {
		for (auto anIt = mLooping.begin(); anIt != mLooping.end(); ++anIt)
			anIt->second.mInstance->SetVolume(thePause ? 0.0 : 1.0);
	}
}
