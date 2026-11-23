// The gem silhouette table of the exe (bejeweled3, 0x83a548) and its reader 0x71fbd0. See GemSilhouettes.h.
#include "GemSilhouettes.h"

#include "platform/VFS.h"

#include <SDL.h>
#include <cstdint>
#include <cstring>
#include <vector>

namespace Sexy {

namespace GemSilhouettes {

namespace {

const uint32_t TABLE_ADDRESS = 0x83a548;   // the first entry of frame 0, colour 0
const int FRAME_COUNT = 20;
const int COLOR_COUNT = 7;
const int ANGLE_COUNT = 256;
const int TABLE_FLOATS = FRAME_COUNT * COLOR_COUNT * ANGLE_COUNT;
const float TWO_PI = 6.2831854820251465f;   // 0x880e68, a double of the exe, kept as the float it was made from

// port: when the table cannot be read, every gem's edge is this far from its middle: the table's values lie between 47 and 56
const float FALLBACK_RADIUS = 52.0f;

std::vector<float> gTable;   // empty until Load has read it

uint32_t ReadU32(const std::vector<uint8_t>& theData, size_t theOffset)
{
	uint32_t aValue = 0;
	if (theOffset + 4 <= theData.size())
		memcpy(&aValue, &theData[theOffset], 4);
	return aValue;
}

uint16_t ReadU16(const std::vector<uint8_t>& theData, size_t theOffset)
{
	uint16_t aValue = 0;
	if (theOffset + 2 <= theData.size())
		memcpy(&aValue, &theData[theOffset], 2);
	return aValue;
}

// The file offset of a virtual address of a 32-bit PE file, found through its section table; 0 when no section's file data holds it
size_t FileOffset(const std::vector<uint8_t>& theData, uint32_t theAddress)
{
	if (theData.size() < 0x40 || theData[0] != 'M' || theData[1] != 'Z')
		return 0;
	size_t aHeader = ReadU32(theData, 0x3c);
	if (aHeader + 0x18 > theData.size() || memcmp(&theData[aHeader], "PE\0\0", 4) != 0)
		return 0;
	int aSectionCount = ReadU16(theData, aHeader + 6);
	size_t anOptionalSize = ReadU16(theData, aHeader + 20);
	uint32_t anImageBase = ReadU32(theData, aHeader + 24 + 28);
	size_t aSection = aHeader + 24 + anOptionalSize;
	for (int i = 0; i < aSectionCount; i++, aSection += 40) {
		uint32_t aStart = anImageBase + ReadU32(theData, aSection + 12);
		uint32_t aFileSize = ReadU32(theData, aSection + 16);
		uint32_t aFilePointer = ReadU32(theData, aSection + 20);
		if (theAddress >= aStart && theAddress < aStart + aFileSize)
			return (size_t)aFilePointer + (theAddress - aStart);
	}
	return 0;
}

}  // namespace

void Load()
{
	gTable.clear();
	std::vector<uint8_t> aFile;
	if (!Platform::VFS::Get().ReadAll("bejeweled3.exe", aFile)) {
		SDL_Log("GemSilhouettes: bejeweled3.exe is not in the game folder; the time gems' bolts use a constant radius");
		return;
	}
	size_t anOffset = FileOffset(aFile, TABLE_ADDRESS);
	if (anOffset == 0 || anOffset + (size_t)TABLE_FLOATS * 4 > aFile.size()) {
		SDL_Log("GemSilhouettes: the gem table is not where the exe of this port's version has it; the time gems' bolts use a constant radius");
		return;
	}
	gTable.resize(TABLE_FLOATS);
	memcpy(&gTable[0], &aFile[anOffset], (size_t)TABLE_FLOATS * 4);
}

// 0x71fbd0 (bejeweled3): the angle is brought into 0 to 2 pi, becomes a place in the 256 entries (256 times it over 2 pi) and the two
// entries about it are blended; below the first entry and from the last the edge entries are returned
float RadiusAt(float theAngle, int theFrame, int theColor)
{
	if (gTable.empty())
		return FALLBACK_RADIUS;
	while (theAngle >= TWO_PI)
		theAngle = (float)((double)theAngle - (double)TWO_PI);
	while (theAngle < 0.0f)
		theAngle = (float)((double)theAngle + (double)TWO_PI);
	float aPlace = (float)((double)theAngle * 256.0 / (double)TWO_PI);
	int aStart = (theFrame * COLOR_COUNT + theColor) * ANGLE_COUNT;
	int anIndex = (int)aPlace;
	if (anIndex < 0)
		return gTable[aStart];
	if (anIndex >= ANGLE_COUNT - 1)
		return gTable[aStart + ANGLE_COUNT - 1];
	float aFraction = aPlace - (float)anIndex;
	return (float)((double)gTable[aStart + anIndex] * (1.0 - (double)aFraction) + (double)aFraction * (double)gTable[aStart + anIndex + 1]);
}

}  // namespace GemSilhouettes

}  // namespace Sexy
