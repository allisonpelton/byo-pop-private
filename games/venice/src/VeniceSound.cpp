// The sound layer (0x4436a0-0x444000); see VeniceSound.h.

#include "VeniceSound.h"

#include "SDLSoundManager.h"
#include "VeniceApp.h"
#include "platform/VFS.h"

#include <cstring>
#include <vector>

int gMusicVolume = 50; // 0x5b51d0
int gSoundVolume = 55; // 0x5b51d4
CursorList<Sound> gSounds; // 0xa2ac58

// the times of the latest twelve starts; another sound starts only when one of the twelve is over 200 ms old
static unsigned int sRecentStarts[12]; // 0xa2ac18
static int sRecentStartIndex = 0; // 0xa2ac48

// port: the sound's voice is stopped as the record goes (the mixer is the port's)
Sound::~Sound()
{
	Platform::Mixer::Get().StopVoice(mChannel);
}

// 0x4437e0: the original reads the file here and fails when it is missing; the port decodes it at the first play
bool Sound::Load(const char* theName, bool theLoop, bool theIsMusic)
{
	mLoop = theLoop;
	mIsMusic = theIsMusic;
	strcpy(mName, theName);
	if (Sexy::SoundLogOn()) {
		// a test run compares the sounds with the original's by the file's checksum (tools/idplay/soundcmp.py)
		std::vector<uint8_t> aFile;
		unsigned int aCrc = 0xffffffff;
		if (Platform::VFS::Get().ReadAll(mName, aFile)) {
			for (uint8_t aByte : aFile) {
				aCrc ^= aByte;
				for (int aBit = 0; aBit < 8; aBit++)
					aCrc = (aCrc >> 1) ^ (0xedb88320 & (0 - (aCrc & 1)));
			}
		}
		Sexy::SoundLog("load %s loop=%d crc %08x len %zu", mName, theLoop ? 1 : 0, ~aCrc, aFile.size());
	}
	return true;
}

// 0x4438e0
void Sound::Play(int theFrequency)
{
	unsigned int aNow = gApp->GetClockTime();
	bool aFree = false;
	for (int i = 0; i < 12; i++) {
		if (aNow - sRecentStarts[i] > 200) {
			aFree = true;
			break;
		}
	}
	if (!aFree)
		return;
	sRecentStarts[sRecentStartIndex] = aNow;
	sRecentStartIndex = sRecentStartIndex + 1 < 12 ? sRecentStartIndex + 1 : 0;
	Sexy::SoundLog("play %s freq=%d vol=%d", mName, theFrequency, mIsMusic ? 0 : gSoundVolume);
	if (mData == nullptr) {
		mData = Platform::Mixer::Get().LoadSound(mName);
		if (mData == nullptr)
			return;
	}
	float aGain = mIsMusic ? 0.0f : (float)gSoundVolume / 100.0f;
	double aPitch = theFrequency < 0 ? 1.0 : (double)theFrequency / (double)mData->mRate;
	mChannel = Platform::Mixer::Get().StartVoice(mData, aGain, aGain, aPitch, mLoop);
}

// 0x443a60: the volume of a channel that is playing, the sound's own (or the music's) volume times the percent
void Sound::SetVolume(float thePercent)
{
	if (!Platform::Mixer::Get().IsVoicePlaying(mChannel))
		return;
	if (thePercent < 0.0f) {
		Sexy::SoundLog("stop %s", mName);
		Platform::Mixer::Get().StopVoice(mChannel);
		return;
	}
	int aVolume = ((int)thePercent * (mIsMusic ? gMusicVolume : gSoundVolume)) / 100;
	Sexy::SoundLog("volume %s %d", mName, aVolume);
	float aGain = (float)aVolume / 100.0f;
	Platform::Mixer::Get().SetVoiceGain(mChannel, aGain, aGain);
}

// 0x443b50
Sound* FindSound(const char* theName)
{
	bool aMore = gSounds.Begin();
	while (aMore) {
		if (strcmp(theName, gSounds.mCursor->mName) == 0)
			return gSounds.mCursor;
		aMore = gSounds.Next();
	}
	return nullptr;
}

// 0x443bc0
Sound* LoadSound(const char* theName, bool theLoop, bool theIsMusic)
{
	Sound* aSound = FindSound(theName);
	if (aSound != nullptr) {
		aSound->mLoop = theLoop;
		aSound->mIsMusic = theIsMusic;
		return aSound;
	}
	gSounds.Append(new Sound());
	gSounds.mCursor->Load(theName, theLoop, theIsMusic);
	return gSounds.mCursor;
}

// 0x443c50
void FreeSound(Sound* theSound)
{
	bool aMore = gSounds.Begin();
	while (aMore) {
		if (gSounds.mCursor == theSound) {
			gSounds.RemoveCursor();
			return;
		}
		aMore = gSounds.Next();
	}
}

// port: the original calls the finder (0x443b50) and then the player (0x4438e0) at each place that plays a sound by name
void PlaySoundByName(const char* theName, int theFrequency)
{
	Sound* aSound = FindSound(theName);
	if (aSound != nullptr)
		aSound->Play(theFrequency);
}

// port: the original calls the loader (0x443bc0) and then the player (0x4438e0) at each place that plays a new sound
void LoadAndPlaySound(const char* theName, int theFrequency)
{
	LoadSound(theName, false, false)->Play(theFrequency);
}

// BASS_Pause and BASS_Start stop and start the whole output; the mixer has no pause, so the voices are silenced and keep
// their place (a deviation: they play on, unheard, while the window has no focus)
void PauseSounds()
{
	Sexy::SoundLog("pause");
	Platform::Mixer::Get().SetSfxMasterGain(0.0f);
}

void ResumeSounds()
{
	Sexy::SoundLog("start");
	Platform::Mixer::Get().SetSfxMasterGain(1.0f);
}

void SetVolumes(int theMusicVolume, int theSoundVolume)
{
	if (theMusicVolume != -1)
		gMusicVolume = theMusicVolume;
	if (theSoundVolume != -1)
		gSoundVolume = theSoundVolume;
}
