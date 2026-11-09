// The sound manager and the mixer (see HAudio.h).

#include "HAudio.h"
#include "HFileSystem.h"
#include "Pg.h"

#include "platform/Audio.h"

#include <cstring>
#include <vector>

namespace pg
{

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HMixer

// The voices that ended are free again (the original frees a voice in its mixing loop, 0x424d15)
void HMixer::updateVoices()
{
	Platform::Mixer& aMixer = Platform::Mixer::Get();
	for (Voice& aVoice : mVoices)
	{
		if (aVoice.mVoice >= 0 && !aMixer.IsVoicePlaying(aVoice.mVoice))
		{
			aVoice.mVoice = -1;
			aVoice.mSound = nullptr;
		}
	}
}

int HMixer::play(int* theVoiceOut, const std::shared_ptr<Platform::SoundData>& theSound, uint8_t theVolume,
	uint8_t thePan, int theLoops)
{
	(void)thePan;
	if (!theSound || theLoops < -1)
		return -2;
	updateVoices();
	for (int i = 0; i < 8; i++)
	{
		Voice& aVoice = mVoices[i];
		if (aVoice.mVoice < 0)
		{
			float aGain = theVolume / 256.0f;
			// port: the game only plays sounds once (theLoops 0) or for ever (-1)
			aVoice.mVoice = Platform::Mixer::Get().StartVoice(theSound, aGain, aGain, 1.0, theLoops == -1);
			if (aVoice.mVoice < 0)
				return -1;
			aVoice.mSound = theSound.get();
			if (theVoiceOut != nullptr)
				*theVoiceOut = i;
			return 0;
		}
	}
	return -1;
}

bool HMixer::isPlaying(const void* theSound)
{
	updateVoices();
	for (Voice& aVoice : mVoices)
		if (aVoice.mVoice >= 0 && aVoice.mSound == theSound)
			return true;
	return false;
}

void HMixer::stopAll()
{
	for (Voice& aVoice : mVoices)
	{
		if (aVoice.mVoice >= 0)
			Platform::Mixer::Get().StopVoice(aVoice.mVoice);
		aVoice.mVoice = -1;
		aVoice.mSound = nullptr;
	}
}

// port: the music is a looping voice, kept at gain 0 while it is off so that it goes on from where it was
void HMixer::setMusic(const std::shared_ptr<Platform::SoundData>& theMusic, uint8_t theVolume)
{
	if (mMusic != theMusic)
	{
		if (mMusicVoice >= 0)
			Platform::Mixer::Get().StopVoice(mMusicVoice);
		mMusicVoice = -1;
		mMusic = theMusic;
		mMusicVolume = theVolume;
	}
	if (!mMusic)
		return;
	float aGain = mMusicVolume / 256.0f;
	if (mMusicVoice < 0 || !Platform::Mixer::Get().IsVoicePlaying(mMusicVoice))
		mMusicVoice = Platform::Mixer::Get().StartVoice(mMusic, aGain, aGain, 1.0, true);
	else
		Platform::Mixer::Get().SetVoiceGain(mMusicVoice, aGain, aGain);
}

void HMixer::setMusicVolume(uint8_t theVolume)
{
	mMusicVolume = theVolume;
	if (mMusic && mMusicVoice >= 0)
	{
		float aGain = theVolume / 256.0f;
		Platform::Mixer::Get().SetVoiceGain(mMusicVoice, aGain, aGain);
	}
}

void HMixer::stopMusic()
{
	if (mMusicVoice >= 0)
		Platform::Mixer::Get().SetVoiceGain(mMusicVoice, 0.0f, 0.0f);
	mMusic.reset();
}

void HMixer::restartMusic()
{
	if (mMusicVoice >= 0)
	{
		Platform::Mixer::Get().StopVoice(mMusicVoice);
		mMusicVoice = -1;
	}
	if (mMusic)
	{
		float aGain = mMusicVolume / 256.0f;
		mMusicVoice = Platform::Mixer::Get().StartVoice(mMusic, aGain, aGain, 1.0, true);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// PGSoundManager

bool PGSoundManager::setFormat(int theRate, int theChannels, int theBits)
{
	mRate = theRate;
	mBits = theBits;
	mChannels = theChannels;
	return true;
}

void PGSoundManager::freeAll()
{
	for (Sound& aSound : mSounds)
	{
		if (aSound.mData)
		{
			aSound.mData.reset();
			aSound.mName[0] = 0;
		}
	}
	mMixer = nullptr;
}

PGSoundManager::Sound* PGSoundManager::find(const char* theName)
{
	for (Sound& aSound : mSounds)
		if (HStricmp(theName, aSound.mName) == 0)
			return &aSound;
	return nullptr;
}

bool PGSoundManager::add(const char* theName, const std::shared_ptr<Platform::SoundData>& theData)
{
	for (Sound& aSound : mSounds)
	{
		if (aSound.mName[0] == 0 && !aSound.mData)
		{
			HStrncpy(aSound.mName, theName, 0x80);
			aSound.mData = theData;
			return true;
		}
	}
	return false;
}

bool PGSoundManager::load(const char* theName, std::shared_ptr<Platform::SoundData>* theData)
{
	if (mFileSystem == nullptr)
		return false;
	Sound* aSound = find(theName);
	if (aSound != nullptr && aSound->mData)
	{
		*theData = aSound->mData;
		return true;
	}
	if (strstr(theName, ".wav") == nullptr && strstr(theName, ".ogg") == nullptr)
		return false;
	std::vector<uint8_t> aFile;
	if (!mFileSystem->readFile(theName, aFile))
		return false;
	std::shared_ptr<Platform::SoundData> aData = Platform::Mixer::Get().LoadSoundData(aFile, theName);
	if (!aData)
		return false;
	add(theName, aData);
	*theData = aData;
	return true;
}

bool PGSoundManager::isPlaying(const char* theName)
{
	if (mMixer == nullptr)
		return false;
	for (Sound& aSound : mSounds)
		if (HStricmp(aSound.mName, theName) == 0 && aSound.mData && mMixer->isPlaying(aSound.mData.get()))
			return true;
	return false;
}

bool PGSoundManager::play(int* theVoiceOut, const char* theName, int theLoops, uint8_t theVolume, uint8_t thePan,
	bool theOverlap)
{
	if (theName == nullptr || mFileSystem == nullptr || mMixer == nullptr || !mEnabled)
		return false;
	std::shared_ptr<Platform::SoundData> aData;
	if (!load(theName, &aData))
		return false;
	if (!theOverlap && isPlaying(theName))
		return false;
	if (aData)
	{
		int aVoice = -1;
		mMixer->play(&aVoice, aData, theVolume, thePan, theLoops);
		if (theVoiceOut != nullptr && aVoice >= 0)
			*theVoiceOut = aVoice;
		GetHost()->SoundStarted(theName, aVoice);
	}
	return true;
}

} // namespace pg
