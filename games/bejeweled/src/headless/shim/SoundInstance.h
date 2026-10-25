#ifndef __SOUNDINSTANCE_H__
#define __SOUNDINSTANCE_H__

// port(variant): headless shim of framework/SoundInstance.h for the verifier
// (games/bejeweled/docs/CODE_MODS_DESIGN.md 6.9). Not part of the original game. The headless SoundManager hands
// out none (as the game's does without an audio device), so Board's whirlpool sound stays NULL.

#include "Common.h"

namespace Sexy
{

class SoundInstance
{
public:
	virtual ~SoundInstance() {}
	virtual void		Release() {}
	virtual void		SetVolume(double theVolume) {}
	virtual bool		Play(bool looping, bool autoRelease) { return false; }
};

}

#endif
