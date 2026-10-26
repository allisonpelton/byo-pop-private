#ifndef __COSMICBUGS_CBLEVEL_H__
#define __COSMICBUGS_CBLEVEL_H__

// Levels (0x412050..0x413d10, in progress: only the members the translated code uses are named yet)

#include "CBEmitter.h"

namespace cb
{

// 0x478 bytes (0x41e880 allocates one, 0x413620 constructs it)
struct Level
{
	Level* next;                     // +0x000
	Level* prev;                     // +0x004
	uint8_t unk008[0x3f8];           // +0x008
	List<ParticleSystem> systems;    // +0x400
	uint8_t unk40c[0xc];             // +0x40c
	List<Obj> enemies;               // +0x418  the current one is the one the game file's tags go to
	uint8_t unk424[0x54];            // +0x424
};
static_assert(sizeof(Level) == 0x478, "Level");

// 0x473138: the game file's levels; the current one is the level being played (the code's gLevel)
extern List<Level> gLevels;
// 0x473144: the game file's enemies outside any level
extern List<Obj> gEnemyDefs;

} // namespace cb

#endif
