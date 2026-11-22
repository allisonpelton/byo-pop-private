#ifndef __SPROUT2_HOST_SIMPLESOUNDDX_H__
#define __SPROUT2_HOST_SIMPLESOUNDDX_H__

// The port's simplesounddx: the engine's sound system over the platform's mixer (platform/Audio.h) instead of
// DirectSound. A source is the decoded sound (a FileStreamSource's file decoded as it plays, a WaveBufferSource's
// decoded whole), a handle one voice of it. What the original does in its own update, every 100 ms from the window's
// timer, stays there: a handle sees its sound's end and moves its fade on only then, so isPlaying and the fades keep
// the original's steps. Under -fast the voices move on the game's clock (SexyAppBase), and the app calls the update
// every 100 ms of the game's time.

#include "SimpleSound.h"

#include <map>
#include <memory>
#include <string>
#include <vector>

namespace Platform
{
struct SoundData;
}

namespace simplesounddx
{

class SimpleSoundDX;

// A source registered under a name (simplesounddx::SoundDataSource): a stream (FileStreamSource, vtable 0x5612e4
// (feedingfrenzy2)) plays through one handle at a time, a buffer (WaveBufferSource) through any number
struct SoundSource
{
	std::shared_ptr<Platform::SoundData> mData;
	bool mStream = false;
	bool mTaken = false;  // a stream's handle made (the FileStreamSource's slot 0x24 refuses a second)
};

// A handle (SoundHandleDX, its vtable 0x5610f4 (feedingfrenzy2)), over one voice of the mixer at a time
class SoundHandleDX : public simplesound::SoundHandle
{
public:
	SoundHandleDX(SimpleSoundDX* theSystem, SoundSource* theSource);
	~SoundHandleDX() override;

	void play() override;
	void stop() override;
	void setVolume(float theVolume) override;
	float getVolume() override { return mVolume; }
	void setLooping(bool theLooping) override { mLooping = theLooping; }
	bool isPlaying() override { return mPlaying; }
	void setFadeInTime(float theSeconds) override;
	void setFadeOutTime(float theSeconds) override;
	void setCategory(int theCategory) override { mCategory = theCategory; }
	void update(int theMilliseconds) override;
	void applyVolume(float theVolume) override;

private:
	enum FadeStep
	{
		FADE_IN = 0,
		FADE_NONE = 1,
		FADE_OUT = 2,
		FADE_OUT_AGAIN = 3  // a step the fade treats as the fade-out (not yet seen set)
	};

	// 0x0c (0x477c20 (feedingfrenzy2)): stopped, and its sound put back to the start
	void rewind();
	// 0x68 (0x436390 (feedingfrenzy2)): the original's refill of the DirectSound buffer from the stream, which sees
	// the stream's end there; port: the voice's end seen, and the handle stopped (slot 0x08) once
	void checkEnd();
	// 0x64 (0x436030 (feedingfrenzy2)): the fade moved on by the milliseconds, its volume applied
	void updateFade(int theMilliseconds);
	// The end of a fade-out (0x4ea965 (feedingfrenzy2)): stopped, and put back to the start
	void endFadeOut();

	SimpleSoundDX* mSystem;
	SoundSource* mSource;        // +0x0c
	int mVoice;                  // +0x10: the mixer's voice, 0 for none (the DirectSound buffer)
	int mCategory;               // +0x08
	float mVolume;               // +0x2c
	bool mLooping;               // +0x34
	bool mPlaying;               // +0x35
	bool mEnded;                 // +0x36: the sound ran out and the handle has not been stopped yet
	float mVolumeHeard;          // +0x48: the last volume applied, master's and category's included
	float mFadeInTime;           // +0x4c
	float mFadeOutTime;          // +0x50
	float mFadeTimeLeft;         // +0x54
	int mFadeStep;               // +0x58
};

// The sound system (SimpleSoundDX, vtable 0x56100c (feedingfrenzy2); its constructor 0x4a27f0 (feedingfrenzy2))
class SimpleSoundDX : public simplesound::SimpleSound
{
public:
	SimpleSoundDX();
	~SimpleSoundDX() override;

	bool isRegistered(const std::string& theName) override;
	bool registerSound(const std::string& theName, engine::InputStream* theStream) override;
	bool registerStream(const std::string& theName, engine::InputStream* theStream) override;
	simplesound::SoundHandle* getHandle(const std::string& theName, int theSetupValue, int theCategory) override;
	void setMasterVolume(float theVolume) override;
	float getMasterVolume() override { return mMasterVolume; }
	void stopAll() override;
	void update(int theMilliseconds) override;
	void applyVolumes() override;

	// port: the window's timer (WM_TIMER 1, 0x424780 (feedingfrenzy2)) on the game's clock: an update for each 100 ms
	// the milliseconds bring
	void advanceTimer(double theMilliseconds);
	// The handle's own list (+0x114), kept by its constructor and destructor
	void addHandle(SoundHandleDX* theHandle) { mHandles.push_back(theHandle); }
	void removeHandle(SoundHandleDX* theHandle);

private:
	bool registerSource(const std::string& theName, engine::InputStream* theStream, bool theDecodedAsPlayed);

	std::map<std::string, SoundSource> mSources;
	std::vector<SoundHandleDX*> mHandles;         // +0x114
	float mMasterVolume;                          // +0x130
	double mTimerLeft;                            // port: the game's milliseconds since the timer's last update
};

} // namespace simplesounddx

#endif
