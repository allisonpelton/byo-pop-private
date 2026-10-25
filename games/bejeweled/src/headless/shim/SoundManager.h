#ifndef __SOUNDMANAGER_H__
#define __SOUNDMANAGER_H__

// port(variant): headless shim of framework/SoundManager.h for the verifier
// (games/bejeweled/docs/CODE_MODS_DESIGN.md 6.9). Not part of the original game.

#include "Common.h"

namespace Sexy
{

class SoundInstance;

class SoundManager
{
public:
	virtual ~SoundManager() {}
	virtual SoundInstance* GetSoundInstance(unsigned int theSfxID) { return NULL; }
	void SetDefaultPitch(double theNumSteps) {}
};

}

#endif
