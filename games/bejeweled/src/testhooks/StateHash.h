#ifndef __STATEHASH_H__
#define __STATEHASH_H__

// port: test hook for -tracehash=<file> (see WinDMApp.h). Not part of the original game.

#include <cstdint>

namespace Sexy
{

class WinDMApp;

// 64-bit FNV-1a hashes of the game state after an update; the field lists are in StateHash.cpp.
// theLogicHash: the state play depends on; theFullHash: everything, raw Piece bytes included.
void HashGameState(WinDMApp* theApp, uint64_t* theLogicHash, uint64_t* theFullHash);

}

#endif
