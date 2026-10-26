#include "SoundPlayer.h"

#include "SexyAppBase.h"
#include "SoundInstance.h"
#include "SoundManager.h"

using namespace JeffLib;

SoundPlayer::~SoundPlayer()
{
	for (Sexy::SoundInstance*& anInstance : mCached) {
		if (anInstance != nullptr)
			anInstance->Release();
		anInstance = nullptr;
	}
}

void SoundPlayer::Update()
{
	mUpdateCount++;
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
