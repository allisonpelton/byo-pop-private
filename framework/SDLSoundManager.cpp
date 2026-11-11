#include "SDLSoundManager.h"
#include "../platform/Audio.h"
#include "../platform/VFS.h"
#include "../platform/Placeholders.h"

#include "SexyAppBase.h"

#include <algorithm>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>

using namespace Sexy;
using Platform::Mixer;

static FILE* gSoundLog = nullptr;
static int gSoundLogState = 0;  // 0 not looked at yet, 1 on, -1 off

bool Sexy::SoundLogOn()
{
	if (gSoundLogState == 0)
	{
		const char* aPath = getenv("BYOPOP_SOUNDLOG");
		gSoundLogState = aPath && *aPath ? 1 : -1;
	}
	return gSoundLogState > 0;
}

void Sexy::SoundLog(const char* theFormat, ...)
{
	if (!SoundLogOn())
		return;
	if (!gSoundLog)
	{
		gSoundLog = fopen(getenv("BYOPOP_SOUNDLOG"), "w");
		if (!gSoundLog)
		{
			gSoundLogState = -1;
			return;
		}
	}
	fprintf(gSoundLog, "%d ", gSexyAppBase ? gSexyAppBase->mUpdateCount : 0);
	va_list anArgs;
	va_start(anArgs, theFormat);
	vfprintf(gSoundLog, theFormat, anArgs);
	va_end(anArgs);
	fputc('\n', gSoundLog);
	fflush(gSoundLog);
}

double SDLSoundManager::VolumeToGain(double theVolume)
{
	// DSoundManager::VolumeToDB: hundredths of dB = (log10(1 + v*9) - 1) * 2333, muted below -2000.
	if (theVolume <= 0.0)
		return 0.0;
	double centiDB = (std::log10(1.0 + theVolume * 9.0) - 1.0) * 2333.0;
	if (centiDB < -2000.0)
		return 0.0;
	// A volume above 1 gives a positive value, which DirectSound refuses (DSBVOLUME_MAX is 0): the buffer stays at
	// full volume, never louder (Bookworm Adventures' CROWD_CHEER has volume="10").
	if (centiDB > 0.0)
		return 1.0;
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

// The volume as DSoundInstance hands it to DirectSound (DSoundManager::VolumeToDB), for the sound log
int SDLSoundInstance::LogVolume() const
{
	if (mManager->mDBPerFactor)
	{
		if (!(mManager->mMasterVolume > 0.015))
			return -10000;
		int aSum = 0;
		for (double aFactor : {mManager->mMasterVolume, mBaseVolume, mVolume})
		{
			if (!(aFactor > 0.0) || !std::isfinite(aFactor))
				return -10000;
			int aDB = (int)((std::log10(1.0 + aFactor * 9.0) - 1.0) * 2333.0);
			aSum += aDB < -2331 ? -10000 : aDB;
		}
		return aSum < -10000 ? -10000 : aSum;
	}
	double aVolume = mBaseVolume * mVolume * mManager->mMasterVolume;
	if (!(aVolume > 0.0) || !std::isfinite(aVolume))   // silent, or not a number the cast below could take
		return -10000;
	int aDB = (int)((std::log10(1.0 + aVolume * 9.0) - 1.0) * 2333.0);
	return aDB < -2000 ? -10000 : aDB;
}

void SDLSoundInstance::RehupVolume()
{
	double gain;
	if (mManager->mDBPerFactor)
	{
		int aDB = LogVolume();   // above 0 DirectSound refuses it, as in VolumeToGain
		gain = aDB <= -10000 ? 0.0 : aDB >= 0 ? 1.0 : std::pow(10.0, aDB / 2000.0);
	}
	else
		gain = SDLSoundManager::VolumeToGain(mBaseVolume * mVolume * mManager->mMasterVolume);
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
	if (SoundLogOn())   // DSoundInstance::RehupVolume sets the buffer's volume
		SoundLog("volume %d #%d %d", mSfxID, mLogNum, LogVolume());
	RehupVolume();
}

void SDLSoundInstance::SetBasePan(int theBasePan)
{
	mBasePan = theBasePan;
	if (SoundLogOn())   // and RehupPan its pan
		SoundLog("pan %d #%d %d", mSfxID, mLogNum, mBasePan + mPan);
	RehupVolume();
}

void SDLSoundInstance::AdjustPitch(double theNumSteps)
{
	mPitch = std::pow(2.0, theNumSteps / 12.0);
	if (mManager->mDBPerFactor)
	{
		double aRate = mData ? mData->mRate : 44100.0;
		double aFreq = std::min(std::max(aRate * mPitch, 100.0), 100000.0);
		mPitch = std::trunc(aFreq) / aRate;   // whole Hz, cut down (39288.9 Hz plays at 39288 in its runs)
	}
	SoundLog("pitch %d #%d %.4f", mSfxID, mLogNum, mPitch);
	if (mVoice)
		Mixer::Get().SetVoicePitch(mVoice, mPitch);
}

void SDLSoundInstance::SetVolume(double theVolume)
{
	mVolume = theVolume;
	if (SoundLogOn())
		SoundLog("volume %d #%d %d", mSfxID, mLogNum, LogVolume());
	RehupVolume();
}

void SDLSoundInstance::SetPan(int thePosition)
{
	mPan = thePosition;
	SoundLog("pan %d #%d %d", mSfxID, mLogNum, mBasePan + mPan);
	RehupVolume();
}

bool SDLSoundInstance::Play(bool looping, bool autoRelease)
{
	Stop();
	mHasPlayed = true;
	mAutoRelease = autoRelease;
	if (SoundLogOn())
		SoundLog("play %d #%d vol=%d pan=%d pitch=%.4f loop=%d", mSfxID, mLogNum, LogVolume(), mBasePan + mPan, mPitch,
			looping ? 1 : 0);
	mVoice = Mixer::Get().StartVoice(mData, 0.0f, 0.0f, mPitch, looping);
	RehupVolume();
	return mVoice != 0;
}

void SDLSoundInstance::Stop()
{
	if (mVoice)
	{
		SoundLog("stop %d #%d", mSfxID, mLogNum);
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
	if ((!data) && (Platform::Placeholders()))
	{
		// port: -placeholders (platform/Placeholders.h): a silent frame, so the id stays taken and plays nothing
		data = std::make_shared<Platform::SoundData>();
		data->mSamples.assign(1, 0);
		data->mFrames = 1;
		Platform::NoteMissing(theFilename);
	}
	if (!data)
		return false;
	SoundLog("load %d %s frames=%d rate=%d ch=%d", theSfxID, theFilename.c_str(), (int)data->mFrames, data->mRate,
		data->mChannels);
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
	SoundLog("master volume %.3f", theVolume);
	for (int i = 0; i < MAX_CHANNELS; i++)
		if (mInstances[i])
		{
			// DSoundManager::SetVolume sets each live instance's buffer volume again, playing or not
			if (SoundLogOn())
				SoundLog("volume %d #%d %d", mInstances[i]->mSfxID, mInstances[i]->mLogNum, mInstances[i]->LogVolume());
			mInstances[i]->RehupVolume();
		}
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
	static int sLogNum = 0;
	mInstances[ch]->mSfxID = theSfxID;
	mInstances[ch]->mLogNum = ++sLogNum;
	SoundLog("instance %d #%d", theSfxID, sLogNum);
	if (mDefaultPitch != 0.0)
		mInstances[ch]->AdjustPitch(mDefaultPitch);
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
	SoundLog("stop all");
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

size_t SDLSoundManager::GetSoundBytes() const
{
	size_t n = 0;
	for (int i = 0; i < MAX_SOURCE_SOUNDS; i++)
		if (mSounds[i])
			n += mSounds[i]->Bytes();
	return n;
}

int SDLSoundManager::GetNumSounds()
{
	int n = 0;
	for (int i = 0; i < MAX_SOURCE_SOUNDS; i++)
		if (mSounds[i])
			n++;
	return n;
}

size_t SDLSoundManager::GetSoundBytes(int* theCount) const
{
	size_t aBytes = 0;
	*theCount = 0;
	for (int i = 0; i < MAX_SOURCE_SOUNDS; i++)
	{
		if (mSounds[i] == NULL)
			continue;
		aBytes += mSounds[i]->Bytes();
		(*theCount)++;
	}
	return aBytes;
}
