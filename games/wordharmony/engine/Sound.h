#ifndef __WORDHARMONY_ENGINE_SOUND_H__
#define __WORDHARMONY_ENGINE_SOUND_H__

// The sound system the engine talks to: the Windows layer's SimpleSoundDX (0x134 bytes, constructor 0x443980, vtable
// 0x485190, made by 0x443ca0 from the window) keeps the loaded sounds by name and the voices it made from them. The
// application keeps it in a global (0x49bfc0) that App::getSoundSystem returns. The port's host implements it over the
// framework's sound (the slots below are the ones the engine and the game call).

namespace engine
{

// A voice (0x58 bytes, constructor 0x443fc0; 0x5c bytes with vtable 0x4850f8 for sounds of two seconds or less)
class Sound
{
public:
	virtual ~Sound() {}
	virtual void play() = 0;  // slot 0x00
};

class SoundSystem
{
public:
	virtual ~SoundSystem() {}
	// slot 0x00: 0x443a30, loads a sound file under a name; true when it is (or already was) loaded
	virtual bool load(const char* theName, const char* theFile) = 0;
	// slot 0x14: 0x4435e0, a new voice of a loaded sound (the system keeps it); null when the name is not loaded
	virtual Sound* createSound(const char* theName, int theArg1, int theArg2) = 0;
};

} // namespace engine

#endif
