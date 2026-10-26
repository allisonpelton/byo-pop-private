#ifndef __WORDHARMONY_SOUNDHOST_H__
#define __WORDHARMONY_SOUNDHOST_H__

// The port's sound system: the engine's SimpleSoundDX (0x443980, engine/Sound.h) over the platform's mixer. Sounds are
// decoded whole when they are loaded (the original streamed the music's clips from their files); the mixer cannot
// seek, so a voice halted and played again starts over.

#include "engine/Sound.h"
#include "platform/Audio.h"

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
	void halt() override;
	void stop() override;
	void rewind() override;
	void close() override;
	void setPitch(float thePitch) override;
	float getPitch() override { return mPitch; }
	void setPan(float thePan) override;
	float getPan() override { return mPan; }
	void setVolume(float theVolume) override;
	float getVolume() override { return mVolume; }
	void setLoop(bool theLoop) override { mLoop = theLoop; }
	bool isLoop() override { return mLoop; }
	bool isPlaying() override;
	void setFadeIn(float theSeconds) override { mFadeIn = theSeconds; }
	void setFadeOut(float theSeconds) override { mFadeOut = theSeconds; }
	void setCategory(int theCategory) override { mCategory = theCategory; }
	int getCategory() override { return mCategory; }
	void update(int theMs) override;

	// The gains the mixer plays the voice at (0x444730: category x master x the faded volume, and the pan)
	void applyGain();
	const std::shared_ptr<Platform::SoundData>& getData() const { return mData; }

private:
	HostSoundSystem* mSystem;
	std::shared_ptr<Platform::SoundData> mData;
	int mVoice;
	bool mPlaying;
	bool mLoop;
	float mVolume;
	float mPan;
	float mPitch;
	float mFadeIn;
	float mFadeOut;
	int mCategory;
	int mFadeState;     // +0x54: 0 fading in, 1 playing, 2 and 3 fading out
	float mFadeLeft;    // +0x50: seconds
	float mFadeVolume;  // the fade's factor
	double mPosition;   // seconds of the sound played
};

class HostSoundSystem : public engine::SoundSystem
{
public:
	HostSoundSystem();
	~HostSoundSystem() override;

	bool load(const char* theName, const char* theFile) override;
	bool loadStream(const char* theName, const char* theFile) override { return load(theName, theFile); }
	void unloadUnused() override;
	void unload(const char* theName) override;
	engine::Sound* createSound(const char* theName, int theArg1, int theArg2) override;
	void setVolume(float theVolume) override;
	float getVolume() override { return mVolume; }
	void setCategoryVolume(int theCategory, float theVolume) override;
	float getCategoryVolume(int theCategory) override;
	void update(int theMs) override;

	void removeSound(HostSound* theSound);

private:
	std::map<std::string, std::shared_ptr<Platform::SoundData>> mData;  // +0x124
	std::vector<HostSound*> mSounds;                                     // +0x118
	float mVolume;                                                       // +0x130
	float mCategoryVolumes[64];
};

} // namespace wordharmony

#endif
