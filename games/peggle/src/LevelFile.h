// LevelFile.h: reading a level's file (levels/<name>.dat): a version, a flag that picks the sync mode, and the list of its pieces.
// The level's own file and levels/_base.dat (the pieces every level shares) are read the same way.

#ifndef __LEVELFILE_H__
#define __LEVELFILE_H__

#include "SharedPtr.h"

#include <cstdint>
#include <list>
#include <string>
#include <vector>

namespace Sexy
{

class DataSync;
class PhysObj;

typedef std::list<SharedPtr<PhysObj> > PieceList;

// The newest version of a level file the game reads (a newer one gives "Save game version is too new.")
extern const int LEVEL_FILE_MAX_VERSION; // 0x42

// 0x4227c0: reads the file into thePieces; false when there is no such file. Leaves gSyncMode as the file's flag sets it.
bool ReadLevelFile(const std::string& theFileName, PieceList& thePieces);

// The part of it that reads from memory. theVersion and theFlag are the file's own; theBytesUsed is how far the reading got.
void ReadLevelData(const void* theData, size_t theSize, PieceList& thePieces, int* theVersion = nullptr, bool* theFlag = nullptr,
				   size_t* theBytesUsed = nullptr, std::vector<size_t>* thePieceStarts = nullptr);

// 0x422330 and 0x4204c0: the list of pieces, as a count and then each piece by SyncSharedPolymorphic. A test may ask for the byte each
// piece starts at.
void SyncPieceList(DataSync& theSync, PieceList& thePieces, std::vector<size_t>* thePieceStarts = nullptr);

// What the level editor writes (and the tests compare with the file): the version, the flag and the list
std::vector<uint8_t> WriteLevelData(int theVersion, bool theFlag, PieceList& thePieces, std::vector<size_t>* thePieceStarts = nullptr);

} // namespace Sexy

#endif
