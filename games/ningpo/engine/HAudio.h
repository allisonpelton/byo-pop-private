#ifndef __NINGPO_HAUDIO_H__
#define __NINGPO_HAUDIO_H__

// The sound: PGSoundManager (0x40c910..0x40cef0, at PGApp+0x3a18) keeps up to 64 sounds decoded from the archive
// (Ogg Vorbis 0x40cd70, WAV 0x40cef0) and plays them on HMixer (vtable 0x463e54, 0x424e6d..0x425277, at
// PGApp+0x6930), a software mixer of 8 voices plus the music stream (an Ogg Vorbis file decoded as it plays,
// 0x425296, looping from a given sample) into a DirectSound buffer of 22050 Hz, 16 bits, stereo. A voice is scaled
// by its volume byte (sample * volume >> 8); its pan byte is kept but never applied. port: the voices and the music
// are voices of the platform's mixer (platform/Audio.h), which resamples to the device and mixes in floats.

#include <cstdint>
#include <memory>
#include <string>

namespace Platform
{
struct SoundData;
}

namespace pg
{

class HFileSystem;

class HMixer
{
public:
	struct Voice
	{
		int mVoice = -1;           // the platform mixer's voice, -1 when free
		const void* mSound = nullptr;
	};
	Voice mVoices[8];              // +0x10, 0x28 bytes each
	std::shared_ptr<Platform::SoundData> mMusic;   // +0x150 the music stream, null when off
	int mMusicVoice = -1;
	uint8_t mMusicVolume = 0xff;   // +0x154

	// 0x424ef6: 0, or -1 when all 8 voices are busy; theLoops 0 plays once, -1 for ever
	int play(int* theVoiceOut, const std::shared_ptr<Platform::SoundData>& theSound, uint8_t theVolume, uint8_t thePan,
		int theLoops);
	// 0x425277: a voice plays this sound
	bool isPlaying(const void* theSound);
	// 0x42525f
	void stopAll();
	// 0x42521d: the music on, from where it was
	void setMusic(const std::shared_ptr<Platform::SoundData>& theMusic, uint8_t theVolume);
	// 0x425245
	void setMusicVolume(uint8_t theVolume);
	// 0x425252: the music off
	void stopMusic();
	// 0x4253b4: the music from its start
	void restartMusic();

private:
	void updateVoices();
};

class PGSoundManager
{
public:
	struct Sound
	{
		char mName[128];                                   // +0x04
		std::shared_ptr<Platform::SoundData> mData;        // +0x84 {pcm, channels, bits, rate, size}
	};
	Sound mSounds[64];
	const HFileSystem* mFileSystem = nullptr;   // +0x2504
	HMixer* mMixer = nullptr;                   // +0x2508
	int mChannels = 0;                          // +0x250c
	int mBits = 0;                              // +0x2510
	int mRate = 0;                              // +0x2514
	bool mEnabled = true;                       // +0x2518

	// 0x40c9b0
	bool setFormat(int theRate, int theChannels, int theBits);
	void setFileSystem(const HFileSystem* theFileSystem) { mFileSystem = theFileSystem; }  // 0x40ca30
	void setMixer(HMixer* theMixer) { mMixer = theMixer; }                                  // 0x40cd20
	// 0x40c9e0
	void freeAll();
	// 0x40cb00: the sound, decoded and kept the first time
	bool load(const char* theName, std::shared_ptr<Platform::SoundData>* theData);
	// 0x40cc10
	bool isPlaying(const char* theName);
	// 0x40cc70: without theOverlap a sound that is still playing is not started again
	bool play(int* theVoiceOut, const char* theName, int theLoops, uint8_t theVolume, uint8_t thePan, bool theOverlap);

private:
	Sound* find(const char* theName);                                            // 0x40ca40
	bool add(const char* theName, const std::shared_ptr<Platform::SoundData>& theData);  // 0x40ca90
};

} // namespace pg

#endif
