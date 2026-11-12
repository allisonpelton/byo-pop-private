#ifndef __BONNIES_SOUND_H__
#define __BONNIES_SOUND_H__

// The sound system (0x41e760-0x41f390, the object 0x47c764): FMOD 3's samples and one module for the music, through
// the host (Host.h).

#include "Str.h"

namespace bonnies
{

// A loaded sample (0x41ee80: an 8-byte block whose first word is FMOD's sample)
struct Sample
{
	void* mHandle = nullptr;
	int mSpare = 0;           // never used
};

class SoundSystem
{
public:
	// 0x41e760: FMOD at 44.1 kHz, 64 channels; the music module Sound/music.xm, .mod or .oxm
	SoundSystem();

	// 0x41e9c0: plays theSample with theVolume (times the options' sound volume) and thePitch (a frequency factor, used
	// outside 0.99-1.01); with theDelay > 0 it is queued and played theDelay ms later, then at pitch 1. The result is the
	// channel, -1 when queued, 0 when it was not played.
	int play(Sample* theSample, float theVolume, int thePassedOn, int theDelay, float thePitch);
	// 0x41eb40: the same with the sample named (Sound/<name>.ogg through the resources)
	int play(const Str& theName, float theVolume, int thePassedOn, int theDelay, float thePitch);
	// 0x41ec60: the music's volume, the options' music volume times theFactor
	void setMusicVolume(float theFactor);
	// 0x41ece0
	bool isMusicPlaying();
	// 0x41ed30: stops the stream and the music
	void stopMusic();
	// 0x41edc0, 0x41edf0, 0x41ee10: pausing (the window lost its focus) and resuming
	void pause();
	void pauseMusicOnly();
	void resume();
	// 0x41ee40: stops everything and closes FMOD
	void shutdown();
	// 0x41ee80: loads Sound/<name>.ogg
	static Sample* loadSample(const Str& theName);
	// 0x41e990
	static void freeSample(Sample* theSample);
	// 0x41ef90: plays the music module from theStartOrder; it is stopped once it passes theStopOrder (after 10 s)
	void playMusic(int theBegin, int theEnd, int theStartOrder, int theStopOrder, float theFactor);
	// 0x41f060: plays Sound/<name>: a stream (.ogg, .mp3, .wav, played as a sample) or another module
	void playStream(const Str& theName, float theFactor);
	// 0x41f2b0: each frame: ends a finished stream, stops the music past its stop order, plays the queued samples due
	void update();

	int mPaused;          // +0x00 (play() plays only while it is below 1)
	int mLastChannel;     // +0x04: (-1) set at the start; nothing reads it
	int mFocusPaused;     // +0x08
	struct Delayed
	{
		Sample* mSample;
		int mPassedOn;           // passed along with the sample and never read
		float mVolume;
		uint32_t mDue;
		Delayed* mNext;
	};
	Delayed* mDelayed;    // +0x0c
	uint32_t mMusicStart; // +0x10
	int mMusicLength;     // +0x14
	int mStopOrder;       // +0x18
};

extern SoundSystem* gSound;  // 0x47c764

} // namespace bonnies

#endif
