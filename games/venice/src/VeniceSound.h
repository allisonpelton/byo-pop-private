// The game's sounds and music: samples loaded by name and played on the mixer. The original uses the BASS library; its sample
// handle and channel handle become the mixer's decoded sound and voice here.

#ifndef VENICE_SOUND_H
#define VENICE_SOUND_H

#include "CursorList.h"
#include "platform/Audio.h"

#include <memory>

// A sound or a piece of music (the list node is 0x3c bytes in the original). One object plays a sample again and again: its
// channel is the voice of the latest Play, and the volume calls reach that voice only.
class Sound
{
public:
	Sound* mNext = nullptr;
	Sound* mPrev = nullptr;
	int mChannel = 0; // the voice of the latest Play (0xc); the music fades set it from a level's saved channel
	std::shared_ptr<Platform::SoundData> mData; // the sample (0x10), decoded at the first play
	bool mLoop = false; // 0x14
	bool mIsMusic = false; // 0x18: music plays at the music volume and starts silent
	char mName[64] = {}; // 0x1c

	~Sound();
	bool Load(const char* theName, bool theLoop, bool theIsMusic); // 0x4437e0: the sample is decoded when it first plays
	void Play(int theFrequency); // 0x4438e0: a frequency of -1 keeps the sample's own
	void SetVolume(float thePercent); // 0x443a60: a negative percent stops the channel
};

extern int gMusicVolume; // 0x5b51d0: 0 to 100, the player's setting
extern int gSoundVolume; // 0x5b51d4
extern CursorList<Sound> gSounds; // 0xa2ac58

// 0x443b50: the sound of this name, null when none was loaded under it
Sound* FindSound(const char* theName);
// 0x443bc0: the sound of this name, made the first time it is asked for
Sound* LoadSound(const char* theName, bool theLoop, bool theIsMusic);
// 0x443c50: takes the sound out of the list and frees it, which silences the voice it plays
void FreeSound(Sound* theSound);
// the find and the Play that the game's callers write out (a name that was never loaded plays nothing)
void PlaySoundByName(const char* theName, int theFrequency);
// 0x443bc0 and then Play: the sound is made when it was not loaded yet
void LoadAndPlaySound(const char* theName, int theFrequency = -1);
void PauseSounds(); // 0x443f30: the window lost the focus
void ResumeSounds(); // 0x443f40
void SetVolumes(int theMusicVolume, int theSoundVolume); // 0x443790: a volume of -1 stays

#endif
