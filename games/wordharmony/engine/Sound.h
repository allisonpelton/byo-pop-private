#ifndef __WORDHARMONY_ENGINE_SOUND_H__
#define __WORDHARMONY_ENGINE_SOUND_H__

// The sound system the engine talks to: the Windows layer's SimpleSoundDX (0x134 bytes, constructor 0x443980, vtable
// 0x485190, made by 0x443ca0 from the window) keeps the loaded sounds by name and the voices it made from them. The
// application keeps it in a global (0x49bfc0) that App::getSoundSystem returns. The port's host implements both
// classes over the platform's mixer (games/wordharmony/docs/WORKING_NOTES.md, "Sound as read"); only the slots the
// engine and the game call are here.

namespace engine
{

// A voice (0x58 bytes, constructor 0x443fc0, vtable 0x4851e0; 0x5c bytes with vtable 0x4850f8 for a short sound). Its
// volume is multiplied by its category's volume and the system's; it fades in when it starts and out when it stops
// (or nears its end, without a loop) when it has those times.
class Sound
{
public:
	virtual ~Sound() {}
	virtual void play() = 0;                       // 0x00: from where it is; nothing when it plays
	virtual void stop() = 0;                       // 0x08: 0x443df0, a fade-out, or halt and rewind
	virtual void rewind() = 0;                     // 0x0c: 0x443e40
	virtual void close() = 0;                      // 0x10: 0x4447b0
	virtual void setPitch(float thePitch) = 0;     // 0x14: 0x443ea0 (1 the sound's own rate)
	virtual float getPitch() = 0;                  // 0x18: 0x4444b0
	virtual void setVolume(float theVolume) = 0;   // 0x24: 0x443f70 (0 to 1)
	virtual float getVolume() = 0;                 // 0x28
	virtual void setLoop(bool theLoop) = 0;        // 0x2c: 0x443fb0
	virtual bool isLoop() = 0;                     // 0x30: 0x4444c0
	virtual bool isPlaying() = 0;                  // 0x34: 0x4444d0
	virtual void setFadeIn(float theSeconds) = 0;  // 0x38: 0x4444f0
	virtual void setFadeOut(float theSeconds) = 0; // 0x3c: 0x444520
	virtual void setCategory(int theCategory) = 0; // 0x48: 0x453e10
	virtual int getCategory() = 0;                 // 0x4c: 0x453e20
	virtual void update(int theMs) = 0;            // 0x5c: 0x444240, the fades and the end
};

class SoundSystem
{
public:
	virtual ~SoundSystem() {}
	// 0x00: 0x443a30, loads a sound file under a name; true when it is (or already was) loaded
	virtual bool load(const char* theName, const char* theFile) = 0;
	// 0x04: 0x443b60, the same for a sound played from its file as it goes (the music)
	virtual bool loadStream(const char* theName, const char* theFile) = 0;
	// 0x14: 0x4435e0, a new voice of a loaded sound (the system keeps it); null when the name is not loaded
	virtual Sound* createSound(const char* theName, int theArg1, int theArg2) = 0;
	// 0x18: 0x4425a0 (0 to 1)
	virtual void setVolume(float theVolume) = 0;
	// 0x1c: 0x443a20
	virtual float getVolume() = 0;
	// 0x20: 0x453e50, one of 64 category volumes (0 to 1)
	virtual void setCategoryVolume(int theCategory, float theVolume) = 0;
	// 0x24: 0x453ea0 (1 for a category out of range)
	virtual float getCategoryVolume(int theCategory) = 0;
	// 0x2c: 0x442840, every voice's update; the window calls it every 100 ms with 100
	virtual void update(int theMs) = 0;
};

} // namespace engine

#endif
