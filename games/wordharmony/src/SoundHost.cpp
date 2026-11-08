#include "SoundHost.h"

#include <cmath>
#include "engine/Host.h"

#include <algorithm>

using namespace wordharmony;

bool HostSound::sGameClock = false;
FILE* HostSound::sLog = nullptr;
int HostSound::sLogFrame = 0;

static double lengthMs(const Platform::SoundData* theData)
{
	return theData->mRate > 0 ? theData->mFrames * 1000.0 / theData->mRate : 0.0;
}

HostSound::HostSound(HostSoundSystem* theSystem, const std::shared_ptr<Platform::SoundData>& theData)
	: mSystem(theSystem), mData(theData), mVoice(0), mPlaying(false), mLoop(false), mVolume(1), mPan(0), mPitch(1),
	  mFadeIn(0), mFadeOut(0), mCategory(0), mFadeState(1), mFadeLeft(0), mFadeVolume(1), mPosition(0), mStartTime(0)
{
}

HostSound::~HostSound()
{
	halt();
	if (mSystem != nullptr)
		mSystem->removeSound(this);
}

// Slot 0: from where it is (the start, in the port), with a fade-in when it has one; nothing when it plays
void HostSound::play()
{
	if (isPlaying())
		return;
	if (0 < mFadeIn)
	{
		mFadeState = 0;
		mFadeLeft = mFadeIn;
		mFadeVolume = 0;
	}
	else
	{
		mFadeState = 1;
		mFadeVolume = 1;
	}
	mPosition = 0;
	mStartTime = engine::getHost()->timeMs();
	mVoice = Platform::Mixer::Get().StartVoice(mData, 0, 0, mPitch, mLoop);
	// on the engine's clock a sound plays its length whether or not the mixer could start a voice (a run with no audio
	// device: the web build under Node), as the original's buffer does
	mPlaying = mVoice != 0 || sGameClock;
	applyGain();
	if (sLog != nullptr)
		std::fprintf(sLog, "%d play %.0f %s vol=%.3f\n", sLogFrame, lengthMs(mData.get()), mLoop ? "looping" : "once", mVolume);
}

// Slot 4 (0x443dd0)
void HostSound::halt()
{
	if (mVoice != 0 && sLog != nullptr)
		std::fprintf(sLog, "%d stop %.0f\n", sLogFrame, lengthMs(mData.get()));
	if (mVoice != 0)
		Platform::Mixer::Get().StopVoice(mVoice);
	mVoice = 0;
	mPlaying = false;
}

// Slot 8 (0x443df0): a fade-out when it has one, else halt and rewind
void HostSound::stop()
{
	if (0 < mFadeOut && isPlaying())
	{
		mFadeState = 2;
		mFadeLeft = mFadeOut;
		return;
	}
	halt();
	rewind();
}

// Slot 0xc
void HostSound::rewind()
{
	mPosition = 0;
}

// Slot 0x10
void HostSound::close()
{
	halt();
}

void HostSound::setPitch(float thePitch)
{
	mPitch = thePitch;
	if (mVoice != 0)
		Platform::Mixer::Get().SetVoicePitch(mVoice, thePitch);
}

void HostSound::setPan(float thePan)
{
	mPan = thePan;
	applyGain();
}

void HostSound::setVolume(float theVolume)
{
	mVolume = theVolume;
	applyGain();
}

// The original plays a sound of up to 2 s from one buffer, which ends after its length; a longer one streams through a
// 2 s buffer refilled a second at a time (0x444270), and it stops only when the cursor leaves the half that took the
// last of it: at the whole seconds of its length plus one (as 0x4440f0 reads the end with the last data).
static double endMs(const Platform::SoundData* theData)
{
	double aMs = theData->mFrames * 1000.0 / theData->mRate;
	if (aMs > 2000.0)
		aMs = (floor(aMs / 1000.0) + 1) * 1000.0;
	return aMs;
}

bool HostSound::isPlaying()
{
	if (sGameClock && mPlaying && !mLoop && mData->mRate > 0 &&
		(double)(uint32_t)(engine::getHost()->timeMs() - mStartTime) * mPitch >= endMs(mData.get()))
	{
		halt();
		mPosition = 0;
	}
	else if (!sGameClock && mPlaying && !Platform::Mixer::Get().IsVoicePlaying(mVoice))
	{
		mVoice = 0;
		mPlaying = false;
		mPosition = 0;
	}
	return mPlaying;
}

// 0x444240 and its fades (0x444380)
void HostSound::update(int theMs)
{
	if (!isPlaying())
		return;
	float aSeconds = (float)theMs * 0.001f;
	mPosition += aSeconds * mPitch;
	switch (mFadeState)
	{
	case 0:
		mFadeLeft -= aSeconds;
		if (mFadeLeft < 0)
		{
			mFadeLeft = 0;
			mFadeState = 1;
		}
		mFadeVolume = (mFadeIn - mFadeLeft) / mFadeIn;
		break;
	case 1:
		mFadeVolume = 1;
		if (0 < mFadeOut && !mLoop && mData != nullptr)
		{
			double aLength = (double)mData->mFrames / Platform::Mixer::Get().GetRate();
			if (aLength - mFadeOut <= mPosition)
			{
				mFadeState = 2;
				mFadeLeft = mFadeOut;
			}
		}
		break;
	default:
		mFadeLeft -= aSeconds;
		if (mFadeLeft < 0)
			mFadeLeft = 0;
		mFadeVolume = 1 - (mFadeOut - mFadeLeft) / mFadeOut;
		break;
	}
	applyGain();
	if (mFadeState == 2 && mFadeLeft <= 0)
	{
		halt();
		rewind();
	}
}

void HostSound::applyGain()
{
	if (mVoice == 0)
		return;
	float aGain = mSystem->getCategoryVolume(mCategory) * mSystem->getVolume() * mVolume * mFadeVolume;
	float aLeft = aGain * (mPan <= 0 ? 1 : 1 - mPan);
	float aRight = aGain * (mPan >= 0 ? 1 : 1 + mPan);
	Platform::Mixer::Get().SetVoiceGain(mVoice, aLeft, aRight);
}

HostSoundSystem::HostSoundSystem()
	: mVolume(1)
{
	for (float& v : mCategoryVolumes)
		v = 1;
}

HostSoundSystem::~HostSoundSystem()
{
	for (HostSound* aSound : mSounds)
		aSound->halt();
}

// Slot 0 (0x443a30): the file decoded under the name
bool HostSoundSystem::load(const char* theName, const char* theFile)
{
	if (mData.find(theName) != mData.end())
		return true;
	std::string aPath = theFile;
	std::replace(aPath.begin(), aPath.end(), '\\', '/');
	std::shared_ptr<Platform::SoundData> aData = Platform::Mixer::Get().LoadSound(aPath);
	if (aData == nullptr)
		return false;
	mData[theName] = aData;
	return true;
}

// Slot 0xc (0x443120)
void HostSoundSystem::unloadUnused()
{
	for (std::map<std::string, std::shared_ptr<Platform::SoundData>>::iterator it = mData.begin(); it != mData.end();)
	{
		bool aUsed = false;
		for (HostSound* aSound : mSounds)
			aUsed = aUsed || aSound->getData() == it->second;
		if (aUsed)
			++it;
		else
			it = mData.erase(it);
	}
}

// Slot 0x10 (0x443190)
void HostSoundSystem::unload(const char* theName)
{
	mData.erase(theName);
}

// Slot 0x14 (0x4435e0)
engine::Sound* HostSoundSystem::createSound(const char* theName, int, int)
{
	std::map<std::string, std::shared_ptr<Platform::SoundData>>::iterator it = mData.find(theName);
	if (it == mData.end())
		return nullptr;
	HostSound* aSound = new HostSound(this, it->second);
	mSounds.push_back(aSound);
	return aSound;
}

void HostSoundSystem::setVolume(float theVolume)
{
	mVolume = theVolume;
	for (HostSound* aSound : mSounds)
		aSound->applyGain();
}

// 0x453e50
void HostSoundSystem::setCategoryVolume(int theCategory, float theVolume)
{
	if (theCategory < 0 || theCategory >= 64)
		return;
	mCategoryVolumes[theCategory] = theVolume;
	for (HostSound* aSound : mSounds)
		aSound->applyGain();
}

// 0x453ea0
float HostSoundSystem::getCategoryVolume(int theCategory)
{
	if (theCategory < 0 || theCategory >= 64)
		return 1;
	return mCategoryVolumes[theCategory];
}

// Slot 0x2c (0x442840)
void HostSoundSystem::update(int theMs)
{
	for (size_t i = 0; i < mSounds.size(); i++)
		mSounds[i]->update(theMs);
}

void HostSoundSystem::removeSound(HostSound* theSound)
{
	mSounds.erase(std::remove(mSounds.begin(), mSounds.end(), theSound), mSounds.end());
}
