#ifndef __SPROUT2_SIMPLESOUND_H__
#define __SPROUT2_SIMPLESOUND_H__

// The engine's sound (the namespace simplesound): named sources registered once, each played through a handle, with
// 64 categories of volume under a master volume. The original's implementation is simplesounddx's, over DirectSound
// (SimpleSoundDX, SoundHandleDX and StaticSoundHandleDX); the port's is host/SimpleSoundDX's, over the platform's
// mixer. Translated so far: what the music system's layers and the engine's slots 0x7c to 0x84 call; the other slots
// are left out until something that is translated calls them.

#include <string>
#include <vector>

namespace engine
{
class InputStream;
}

namespace simplesound
{

class SimpleSound;

// A source played (simplesound::SoundHandle; SoundHandleDX's vtable 0x5610f4 (feedingfrenzy2), 31 slots): its
// volume, looping and fades kept apart from the source's, so that each handle of a source plays it on its own
class SoundHandle
{
public:
	// 0x10 (0x4203d0 (feedingfrenzy2)): stopped and let go of by its source
	virtual ~SoundHandle() {}
	// 0x00 (0x41e1a0 (feedingfrenzy2)): played on from where it is, faded in over the fade-in time when it has one
	virtual void play() = 0;
	// 0x08 (0x477ca0 (feedingfrenzy2)): faded out over the fade-out time when it has one, else stopped at once and
	// put back to its start
	virtual void stop() = 0;
	// 0x24 (0x435270 (feedingfrenzy2)): the volume, 0 to 1, heard from the next update
	virtual void setVolume(float theVolume) = 0;
	// 0x28 (0x43c150 (feedingfrenzy2))
	virtual float getVolume() = 0;
	// 0x2c (0x4061f0 (feedingfrenzy2))
	virtual void setLooping(bool theLooping) = 0;
	// 0x34 (0x439ab0 (feedingfrenzy2)): playing, a fade-out included
	virtual bool isPlaying() = 0;
	// 0x38 (0x486930 (feedingfrenzy2)) and 0x3c (0x486910 (feedingfrenzy2)): the fades' seconds, none when not
	// above 0
	virtual void setFadeInTime(float theSeconds) = 0;
	virtual void setFadeOutTime(float theSeconds) = 0;
	// 0x48 (0x422490 (feedingfrenzy2)): the category whose volume it is played at
	virtual void setCategory(int theCategory) = 0;
	// 0x5c (0x437480 (feedingfrenzy2)): the sound's own update while it plays, every 100 ms: its end seen, and its
	// fade moved on by the milliseconds
	virtual void update(int theMilliseconds) = 0;
	// 0x78 (0x435f60 (feedingfrenzy2)): the volume heard, the master's times the category's times the one given
	virtual void applyVolume(float theVolume) = 0;
};

// The sound system (simplesound::SimpleSound, vtable 0x5612c4 (feedingfrenzy2), 16 slots; its constructor 0x4a2850
// (feedingfrenzy2)): the categories' volumes here, the rest its implementation's
class SimpleSound
{
public:
	static const int CATEGORY_COUNT = 64;

	SimpleSound();
	virtual ~SimpleSound() {}

	// 0x00 (SimpleSoundDX's 0x478260 (feedingfrenzy2)): a source is registered under the name
	virtual bool isRegistered(const std::string& theName) = 0;
	// 0x04 (SimpleSoundDX's 0x478360 (feedingfrenzy2)): the stream read whole into a buffer and registered under the
	// name (a WaveBufferSource); true when it was, or one was already
	virtual bool registerSound(const std::string& theName, engine::InputStream* theStream) = 0;
	// 0x08 (SimpleSoundDX's 0x40c290 (feedingfrenzy2)): the stream registered under the name to be read as it plays
	// (a FileStreamSource); true when it was, or one was already
	virtual bool registerStream(const std::string& theName, engine::InputStream* theStream) = 0;
	// 0x18 (SimpleSoundDX's 0x42ba70 (feedingfrenzy2)): a new handle of the source in the category, null for none.
	// The second argument is passed to the handle's set-up (slot 0x58), not yet translated; the engine passes 0
	virtual SoundHandle* getHandle(const std::string& theName, int theSetupValue, int theCategory) = 0;
	// 0x1c (SimpleSoundDX's 0x4a0b10 (feedingfrenzy2)) and 0x20 (0x436020 (feedingfrenzy2)): the master volume, 0
	// to 1, every handle's volume applied again
	virtual void setMasterVolume(float theVolume) = 0;
	virtual float getMasterVolume() = 0;
	// 0x24 (0x494930 (feedingfrenzy2)): a category's volume, 0 to 1, every handle's volume applied again; a category
	// past the last is not kept
	virtual void setCategoryVolume(int theCategory, float theVolume);
	// 0x28 (0x435f40 (feedingfrenzy2)): a category's volume, 1 past the last
	virtual float getCategoryVolume(int theCategory);
	// 0x2c (SimpleSoundDX's 0x477ce0 (feedingfrenzy2)): every handle stopped (its slot 0x08)
	virtual void stopAll() = 0;
	// 0x30 (SimpleSoundDX's 0x437400 (feedingfrenzy2)): every handle's update, from the window's timer every 100 ms
	virtual void update(int theMilliseconds) = 0;
	// 0x34 (SimpleSoundDX's 0x4360d0 (feedingfrenzy2)): every handle's volume set again and its update run for no
	// time, so the volume is applied anew
	virtual void applyVolumes() = 0;

protected:
	float mCategoryVolumes[CATEGORY_COUNT];  // +0x04
};

// The global sound system (0x5ac648 (feedingfrenzy2)), set by its implementation's constructor; null without sound
SimpleSound* getSimpleSound();
void setSimpleSound(SimpleSound* theSimpleSound);

} // namespace simplesound

#endif
