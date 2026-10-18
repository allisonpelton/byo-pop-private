#include "SDLSoundManager.h"
#include "../platform/Audio.h"
#include "../platform/VFS.h"

#include <cmath>

using namespace Sexy;
using Platform::Mixer;

double SDLSoundManager::VolumeToGain(double theVolume)
{
	// DSoundManager::VolumeToDB: hundredths of dB = (log10(1 + v*9) - 1) * 2333, muted below -2000.
	if (theVolume <= 0.0)
		return 0.0;
	double centiDB = (std::log10(1.0 + theVolume * 9.0) - 1.0) * 2333.0;
	if (centiDB < -2000.0)
		return 0.0;
	return std::pow(10.0, centiDB / 2000.0);
}

// ---------------------------------------------------------------------------------------------

SDLSoundInstance::SDLSoundInstance(SDLSoundManager* theManager, std::shared_ptr<Platform::SoundData> theData, double theBaseVolume, int theBasePan)
	: mManager(theManager), mData(std::move(theData)), mBaseVolume(theBaseVolume), mBasePan(theBasePan)
{
}

SDLSoundInstance::~SDLSoundInstance()
{
	if (mVoice)
		Mixer::Get().StopVoice(mVoice);
}

void SDLSoundInstance::RehupVolume()
{
	double gain = SDLSoundManager::VolumeToGain(mBaseVolume * mVolume * mManager->mMasterVolume);
	int pan = mBasePan + mPan;
	if (pan < -10000) pan = -10000;
	if (pan > 10000) pan = 10000;
	// DirectSound pan attenuates the opposite channel by |pan| hundredths of a dB.
	double left = gain, right = gain;
	if (pan > 0)
		left *= std::pow(10.0, -pan / 2000.0);
	else if (pan < 0)
		right *= std::pow(10.0, pan / 2000.0);
	if (mVoice)
		Mixer::Get().SetVoiceGain(mVoice, (float)left, (float)right);
}

void SDLSoundInstance::Release()
{
	Stop();
	mReleased = true;
}

void SDLSoundInstance::SetBaseVolume(double theBaseVolume)
{
	mBaseVolume = theBaseVolume;
	RehupVolume();
}

void SDLSoundInstance::SetBasePan(int theBasePan)
{
	mBasePan = theBasePan;
	RehupVolume();
}

void SDLSoundInstance::AdjustPitch(double theNumSteps)
{
	mPitch = std::pow(2.0, theNumSteps / 12.0);
	if (mVoice)
		Mixer::Get().SetVoicePitch(mVoice, mPitch);
}

void SDLSoundInstance::SetVolume(double theVolume)
{
	mVolume = theVolume;
	RehupVolume();
}

void SDLSoundInstance::SetPan(int thePosition)
{
	mPan = thePosition;
	RehupVolume();
}

bool SDLSoundInstance::Play(bool looping, bool autoRelease)
{
	Stop();
	mHasPlayed = true;
	mAutoRelease = autoRelease;
	mVoice = Mixer::Get().StartVoice(mData, 0.0f, 0.0f, mPitch, looping);
	RehupVolume();
	return mVoice != 0;
}

void SDLSoundInstance::Stop()
{
	if (mVoice)
	{
		Mixer::Get().StopVoice(mVoice);
		mVoice = 0;
	}
	mAutoRelease = false;
}

bool SDLSoundInstance::IsPlaying()
{
	return mVoice && Mixer::Get().IsVoicePlaying(mVoice);
}

bool SDLSoundInstance::IsReleased()
{
	if (!mReleased && mAutoRelease && mHasPlayed && !IsPlaying())
		Release();
	return mReleased;
}

double SDLSoundInstance::GetVolume()
{
	return mVolume;
}

// ---------------------------------------------------------------------------------------------

SDLSoundManager::SDLSoundManager()
{
	for (int i = 0; i < MAX_SOURCE_SOUNDS; i++)
	{
		mBaseVolumes[i] = 1.0;
		mBasePans[i] = 0;
	}
	for (int i = 0; i < MAX_CHANNELS; i++)
		mInstances[i] = nullptr;
	Mixer::Get().Init();
}

SDLSoundManager::~SDLSoundManager()
{
	ReleaseChannels();
	ReleaseSounds();
}

bool SDLSoundManager::Initialized()
{
	return Mixer::Get().IsInitialized();
}

static const char* kSoundExtensions[] = {".ogg", ".wav", ".au", ".mp3", nullptr};

bool SDLSoundManager::LoadSound(unsigned int theSfxID, const std::string& theFilename)
{
	if (theSfxID >= MAX_SOURCE_SOUNDS)
		return false;
	ReleaseSound(theSfxID);
	std::shared_ptr<Platform::SoundData> data;
	std::string name = theFilename;
	size_t dot = name.find_last_of('.');
	size_t slash = name.find_last_of("/\\");
	bool hasExt = dot != std::string::npos && (slash == std::string::npos || dot > slash);
	if (hasExt)
		data = Mixer::Get().LoadSound(name);
	// The framework probes extensions when the name has none (and mods may swap formats).
	std::string stem = hasExt ? name.substr(0, dot) : name;
	for (int i = 0; !data && kSoundExtensions[i]; i++)
	{
		if (Platform::VFS::Get().Exists(stem + kSoundExtensions[i]))
			data = Mixer::Get().LoadSound(stem + kSoundExtensions[i]);
	}
	if (!data)
		return false;
	mSounds[theSfxID] = data;
	return true;
}

int SDLSoundManager::LoadSound(const std::string& theFilename)
{
	int id = GetFreeSoundId();
	if (id < 0 || !LoadSound((unsigned)id, theFilename))
		return -1;
	return id;
}

void SDLSoundManager::ReleaseSound(unsigned int theSfxID)
{
	if (theSfxID < MAX_SOURCE_SOUNDS)
		mSounds[theSfxID].reset();
}

void SDLSoundManager::SetVolume(double theVolume)
{
	mMasterVolume = theVolume;
	for (int i = 0; i < MAX_CHANNELS; i++)
		if (mInstances[i])
			mInstances[i]->RehupVolume();
}

bool SDLSoundManager::SetBaseVolume(unsigned int theSfxID, double theBaseVolume)
{
	if (theSfxID >= MAX_SOURCE_SOUNDS)
		return false;
	mBaseVolumes[theSfxID] = theBaseVolume;
	return true;
}

bool SDLSoundManager::SetBasePan(unsigned int theSfxID, int theBasePan)
{
	if (theSfxID >= MAX_SOURCE_SOUNDS)
		return false;
	mBasePans[theSfxID] = theBasePan;
	return true;
}

void SDLSoundManager::ReleaseFinishedChannels()
{
	for (int i = 0; i < MAX_CHANNELS; i++)
	{
		if (mInstances[i] && mInstances[i]->IsReleased())
		{
			delete mInstances[i];
			mInstances[i] = nullptr;
		}
	}
}

int SDLSoundManager::FindFreeChannel()
{
	ReleaseFinishedChannels();
	for (int i = 0; i < MAX_CHANNELS; i++)
		if (!mInstances[i])
			return i;
	return -1;
}

SoundInstance* SDLSoundManager::GetSoundInstance(unsigned int theSfxID)
{
	if (theSfxID >= MAX_SOURCE_SOUNDS || !mSounds[theSfxID])
		return nullptr;
	int ch = FindFreeChannel();
	if (ch < 0)
		return nullptr;
	mInstances[ch] = new SDLSoundInstance(this, mSounds[theSfxID], mBaseVolumes[theSfxID], mBasePans[theSfxID]);
	return mInstances[ch];
}

void SDLSoundManager::ReleaseSounds()
{
	for (int i = 0; i < MAX_SOURCE_SOUNDS; i++)
		mSounds[i].reset();
}

void SDLSoundManager::ReleaseChannels()
{
	for (int i = 0; i < MAX_CHANNELS; i++)
	{
		delete mInstances[i];
		mInstances[i] = nullptr;
	}
}

double SDLSoundManager::GetMasterVolume()
{
	return mMasterVolume;
}

void SDLSoundManager::SetMasterVolume(double theVolume)
{
	SetVolume(theVolume);
}

void SDLSoundManager::Flush()
{
}

void SDLSoundManager::SetCooperativeWindow(HWND, bool)
{
}

void SDLSoundManager::StopAllSounds()
{
	for (int i = 0; i < MAX_CHANNELS; i++)
		if (mInstances[i])
			mInstances[i]->Stop();
}

int SDLSoundManager::GetFreeSoundId()
{
	for (int i = 0; i < MAX_SOURCE_SOUNDS; i++)
		if (!mSounds[i])
			return i;
	return -1;
}

int SDLSoundManager::GetNumSounds()
{
	int n = 0;
	for (int i = 0; i < MAX_SOURCE_SOUNDS; i++)
		if (mSounds[i])
			n++;
	return n;
}
