#ifndef __MAHJONGJAPAN_SOUND_H__
#define __MAHJONGJAPAN_SOUND_H__

// The sound object (0x436620, vtable 0x51e680), which wrapped BASS 2.0: the WAVE resources of Resource.dll as
// samples by id (+4), the MO3 music by id (+0x20) and the one playing (+0x1c), the volumes (+0x14 sounds, +0x18
// music, 0-100) and the paused states (+0x30 the app is inactive, +0x31 the music is paused). See docs/ENGINE.md,
// "The device boundary".
//
// Port: Platform::Mixer plays the samples as voices and the music through libopenmpt (MusicPlayer in Sound.cpp, which
// does BASS's volume slides and the row sync of SetMusicLoop inside the audio callback). A BASS volume v is a gain of
// v / 100.

#include "JBBase.h"

#include <map>
#include <memory>
#include <vector>

namespace Platform
{
struct SoundData;
}

namespace jb
{

class MusicPlayer;

class Sound
{
public:
	struct Sample
	{
		std::shared_ptr<Platform::SoundData> mData;
		std::vector<int> mVoices;  // the playing channels, oldest first (at most mMax)
		int mMax = 10;             // BASS_SampleLoad's max: 1 for a looping sample
		int mLast = 0;             // +4 the last channel played (the one the volume and the pauses reach)
		bool mLoop = false;        // +8
		bool mPaused = false;      // +9 paused by Pause (not by the app going inactive)
		bool mLastPaused = false;  // the last channel is paused (BASS_ACTIVE_PAUSED)
	};

	struct Music
	{
		int mSong = 0;                 // the mixer's song
		MusicPlayer* mPlayer = nullptr;  // owned by the mixer
	};

	bool mInitialized = false;     // 0x5d9364 BASS_Init's result
	std::map<int, Sample*> mSamples;  // +0x04
	int mVolume = 100;             // +0x14
	int mMusicVolume = 100;        // +0x18
	Music* mMusic = nullptr;       // +0x1c the music playing
	std::map<int, Music*> mMusics;   // +0x20
	bool mInactive = false;        // +0x30
	bool mMusicPaused = false;     // +0x31

	// 0x436620
	Sound() {}
	// 0x436860
	virtual ~Sound();

	// 0x436b20: BASS_Init at 44100 Hz
	void Init();
	// 0x436b50: BASS_Free (the samples and the music stop)
	void Shutdown();
	// 0x436b70: the WAVE resource theId of Resource.dll, once
	void Load(int theId, bool theLoop);
	// 0x436d50: not while the app is inactive
	void Play(int theId);
	// 0x436de0
	void Stop(int theId);
	// 0x436e50
	void StopAllExcept(int theId);
	// 0x436ec0: the last channel of each sample changes too, when it plays or is paused
	void SetVolume(int theVolume);
	// 0x436f60: all samples or only the looping ones, but theExcept
	void Pause(bool thePause, bool theLoopingOnly, int theExcept);
	// 0x437010: the MO3 in the WAVE resource theId, once
	void LoadMusic(int theId);
	// 0x4371a0: theId from order theOrder (0: the start), faded in over theFadeMs; the music playing goes on (or
	// resumes) when it is theId and not theRestart
	void PlayMusic(int theId, bool theRestart, int theFadeMs, int theOrder);
	// 0x437300: at the next row that is a multiple of theRowStep the music jumps to theOrder, once
	void SetMusicLoop(int theRowStep, int theOrder);
	// 0x437360
	void StopMusic();
	// 0x4373a0
	void PauseMusic(bool thePause);
	// 0x437400: every music's volume
	void SetMusicVolume(int theVolume);
	// 0x437470: to thePercent of the music volume over theMs
	void SlideMusic(int thePercent, int theMs);
	// 0x4374b0: everything pauses while the app is inactive
	void Activate(bool theActive);

private:
	// 0x437520
	void PauseSamples(bool thePause);
	void PauseVoice(int theVoice, bool thePause);
	void ApplyVolume(Sample* theSample, int theVoice);
	// BASS_ChannelIsActive of a sample's last channel: 0 stopped, 1 playing, 3 paused
	int LastState(Sample* theSample);
};

} // namespace jb

#endif
