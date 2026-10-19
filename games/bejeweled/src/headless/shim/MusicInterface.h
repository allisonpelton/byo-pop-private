#ifndef __MUSICINTERFACE_H__
#define __MUSICINTERFACE_H__

// port(variant): headless shim of framework/MusicInterface.h for the verifier
// (games/bejeweled/docs/CODE_MODS_DESIGN.md 6.9). Not part of the original game.

#include "Common.h"

namespace Sexy
{

class MusicInterface
{
public:
	virtual ~MusicInterface() {}
	virtual void		FadeIn(int theSongId, int theOffset = -1, double theSpeed = 0.002, bool noLoop = false) {}
	virtual void		FadeOut(int theSongId, bool stopSong = true, double theSpeed = 0.004) {}
};

}

#endif
