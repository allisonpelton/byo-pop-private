// LevelFile.cpp: see LevelFile.h.

#include "LevelFile.h"

#include "Buffer.h"
#include "DataSync.h"
#include "PhysObj.h"
#include "PhysObjFactory.h"
#include "SexyAppBase.h"

#include <stdexcept>

using namespace Sexy;

const int Sexy::LEVEL_FILE_MAX_VERSION = 0x42;

// 0x422330 and 0x4204c0
void Sexy::SyncPieceList(DataSync& theSync, PieceList& thePieces, std::vector<size_t>* thePieceStarts)
{
	if (!theSync.IsReading())
	{
		int aCount = (int)thePieces.size();
		theSync.SyncLong(aCount);
		for (PieceList::iterator anItr = thePieces.begin(); anItr != thePieces.end(); ++anItr)
		{
			if (thePieceStarts != nullptr)
				thePieceStarts->push_back(theSync.GetWrittenData().size());
			theSync.SyncSharedPolymorphic(*anItr, MakePhysObj);
		}
	}
	else
	{
		thePieces.clear();
		int aCount = 0;
		theSync.SyncLong(aCount);
		for (int i = 0; i < aCount; i++)
		{
			thePieces.push_back(SharedPtr<PhysObj>());
			if (thePieceStarts != nullptr)
				thePieceStarts->push_back(theSync.GetBytesRead());
			theSync.SyncSharedPolymorphic(thePieces.back(), MakePhysObj);
		}
	}
}

// 0x4227c0
bool Sexy::ReadLevelFile(const std::string& theFileName, PieceList& thePieces)
{
	Buffer aBuffer;
	if (!gSexyAppBase->ReadBufferFromFile(theFileName, &aBuffer))
		return false;
	ReadLevelData(aBuffer.GetDataPtr(), aBuffer.GetDataLen(), thePieces);
	return true;
}

void Sexy::ReadLevelData(const void* theData, size_t theSize, PieceList& thePieces, int* theVersion, bool* theFlag,
						 size_t* theBytesUsed, std::vector<size_t>* thePieceStarts)
{
	DataSync aSync;
	aSync.StartReadMemory(theData, theSize);
	int aVersion = 0;
	aSync.SyncLong(aVersion);
	if (aVersion > LEVEL_FILE_MAX_VERSION)
		throw std::runtime_error("Save game version is too new.");
	aSync.mVersion = aVersion;
	bool aFlag = false;
	aSync.SyncBool(aFlag);
	gSyncMode = aFlag ? SYNC_LEVEL_FLAGGED : SYNC_LEVEL;
	SyncPieceList(aSync, thePieces, thePieceStarts);
	aSync.ForgetObjects(); // the original's SyncPointers (0x4bb3e0): no class of the level keeps a pointer slot, so all it does is this
	if (theVersion != nullptr)
		*theVersion = aVersion;
	if (theFlag != nullptr)
		*theFlag = aFlag;
	if (theBytesUsed != nullptr)
		*theBytesUsed = aSync.GetBytesRead();
}

std::vector<uint8_t> Sexy::WriteLevelData(int theVersion, bool theFlag, PieceList& thePieces, std::vector<size_t>* thePieceStarts)
{
	DataSync aSync;
	aSync.StartWriteMemory();
	int aVersion = theVersion;
	aSync.SyncLong(aVersion);
	aSync.mVersion = theVersion;
	aSync.SyncBool(theFlag);
	gSyncMode = theFlag ? SYNC_LEVEL_FLAGGED : SYNC_LEVEL;
	SyncPieceList(aSync, thePieces, thePieceStarts);
	aSync.ForgetObjects();
	return aSync.GetWrittenData();
}
