#ifndef __WORDHARMONY_SOUNDHOST_H__
#define __WORDHARMONY_SOUNDHOST_H__

// The port's sound system: the engine's SimpleSoundDX (0x443980, engine/Sound.h) over the platform's mixer. Sounds are
// decoded whole when they are loaded (the original streamed the music's clips from their files); the mixer cannot
// seek, so a voice halted and played again starts over.

#include "engine/Sound.h"
#include "platform/Audio.h"

#include <cstdio>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace wordharmony
{

class HostSoundSystem;

class HostSound : public engine::Sound
{
public:
	HostSound(HostSoundSystem* theSystem, const std::shared_ptr<Platform::SoundData>& theData);
	~HostSound() override;

	void play() override;
	void halt();  // 0x443dd0: stops where it is (the original's slot 4; only the sound's own functions call it)
	void stop() override;
	void rewind() override;
	void close() override;
	void setPitch(float thePitch) override;
	float getPitch() override { return mPitch; }
	void setVolume(float theVolume) override;
	float getVolume() override { return mVolume; }
	// 0x443fb0
	void setLoop(bool theLoop) override { mLoop = theLoop; }
	bool isLoop() override { return mLoop; }
	bool isPlaying() override;
	// 0x4444f0
	void setFadeIn(float theSeconds) override { mFadeIn = theSeconds; }
	// 0x444520
	void setFadeOut(float theSeconds) override { mFadeOut = theSeconds; }
	void setCategory(int theCategory) override { mCategory = theCategory; }
	int getCategory() override { return mCategory; }
	void update(int theMs) override;

	// The gains the mixer plays the voice at (0x444730: category x master x the faded volume; the pan is always the middle)
	void applyGain();

private:
	HostSoundSystem* mSystem;
	std::shared_ptr<Platform::SoundData> mData;
	int mVoice;
	bool mPlaying;
	bool mLoop;
	float mVolume;
	float mPitch;
	float mFadeIn;
	float mFadeOut;
	int mCategory;
	int mFadeState;     // +0x54: 0 fading in, 1 playing, 2 and 3 fading out
	float mFadeLeft;    // +0x50: seconds
	float mFadeVolume;  // the fade's factor
	double mPosition;   // seconds of the sound played
	uint32_t mStartTime;  // port: the engine's time at play()

public:
	// port: with -fast the sounds end by the engine's clock (the original's DirectSound play cursor, which origrun's
	// virtual clock moves), not by the mixer's, which runs on the wall clock
	static bool sGameClock;
	// port: -soundlog=<file> (a test run's): each start and stop, with the frame it came in and the sound's length, to
	// line up with origrun's DirectSound log of the original (IDENTICAL_PLAY.md)
	static FILE* sLog;
	static int sLogFrame;
};

class HostSoundSystem : public engine::SoundSystem
{
public:
	HostSoundSystem();
	~HostSoundSystem() override;

	bool load(const char* theName, const char* theFile) override;
	// 0x443b60
	bool loadStream(const char* theName, const char* theFile) override { return load(theName, theFile); }
	engine::Sound* createSound(const char* theName, bool theStatic, int theCategory) override;
	void setVolume(float theVolume) override;
	// 0x443a20
	float getVolume() override { return mVolume; }
	void setCategoryVolume(int theCategory, float theVolume) override;
	float getCategoryVolume(int theCategory) override;
	void update(int theMs) override;

	void removeSound(HostSound* theSound);
	// port (the "memreport" test command): the sounds held and the bytes of their decoded samples
	int getSoundMemory(size_t* theBytes) const;

private:
	std::map<std::string, std::shared_ptr<Platform::SoundData>> mData;  // +0x124
	std::vector<HostSound*> mSounds;                                     // +0x118
	float mVolume;                                                       // +0x130
	float mCategoryVolumes[64];
};

} // namespace wordharmony

#endif
