#ifndef __AUDIOCAPTURE_H__
#define __AUDIOCAPTURE_H__

// port: test hook for -audiocapture=<file.wav> (WinTwistApp.h; the same hook as Bejeweled's, games/bejeweled/src/testhooks). Not part of the original game.

#include <cstdio>
#include <string>
#include <vector>

namespace Sexy
{

// The game's sound and music as a 16-bit stereo WAV at the mixer's rate, 1/100 s of it per update, so that it keeps
// time with the updates (and a -script's screenshots) however fast the game runs: update n's sounds start at
// (n - 1) / 100 s. While it captures it holds the mixer's lock, so the audio device's callback (rundesk's dummy
// driver) never mixes and only Update advances the voices and songs; Finish releases it before the device closes.
class AudioCapture
{
public:
	explicit AudioCapture(const std::string& theFileName);
	~AudioCapture();

	void				Update();   // after every update: the next 1/100 s
	void				Finish();   // the WAV header's sizes, the file closed, the lock released

private:
	FILE*				mFile;
	bool				mLocked;
	int					mRate;
	long long			mUpdates;
	long long			mFrames;
	std::vector<float>	mMix;
	std::vector<short>	mPcm;
};

}

#endif
