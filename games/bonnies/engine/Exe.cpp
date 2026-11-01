// The game's exe as data (see Exe.h): its sections mapped at their addresses.

#include "Exe.h"

#include <cstring>

namespace bonnies
{

namespace
{
const uint32_t kImageBase = 0x400000;
std::vector<uint8_t> gImage;  // the image as loaded, from the image base
const uint8_t kZeros[64] = {};

uint32_t rd32(const std::vector<uint8_t>& theData, size_t theOffset)
{
	if (theOffset + 4 > theData.size())
		return 0;
	return theData[theOffset] | theData[theOffset + 1] << 8 | theData[theOffset + 2] << 16 |
		(uint32_t)theData[theOffset + 3] << 24;
}

uint16_t rd16(const std::vector<uint8_t>& theData, size_t theOffset)
{
	if (theOffset + 2 > theData.size())
		return 0;
	return (uint16_t)(theData[theOffset] | theData[theOffset + 1] << 8);
}
}

bool loadExe(const std::vector<uint8_t>& theData)
{
	if (theData.size() < 0x40 || theData[0] != 'M' || theData[1] != 'Z')
		return false;
	uint32_t aPe = rd32(theData, 0x3c);
	if (rd32(theData, aPe) != 0x4550)
		return false;
	int aSections = rd16(theData, aPe + 6);
	int anOptSize = rd16(theData, aPe + 20);
	uint32_t anImageSize = rd32(theData, aPe + 24 + 56);
	if (rd32(theData, aPe + 24 + 28) != kImageBase || anImageSize > 0x2000000)
		return false;
	gImage.assign(anImageSize, 0);
	for (int i = 0; i < aSections; i++)
	{
		size_t aHeader = aPe + 24 + anOptSize + 40 * i;
		uint32_t aVirtualSize = rd32(theData, aHeader + 8);
		uint32_t anAddress = rd32(theData, aHeader + 12);
		uint32_t aRawSize = rd32(theData, aHeader + 16);
		uint32_t aRawPos = rd32(theData, aHeader + 20);
		uint32_t aSize = aRawSize < aVirtualSize ? aRawSize : aVirtualSize;
		if (anAddress + aSize > anImageSize || aRawPos + aSize > theData.size())
			return false;
		std::memcpy(gImage.data() + anAddress, theData.data() + aRawPos, aSize);
	}
	return true;
}

const uint8_t* exePtr(uint32_t theAddress)
{
	if (theAddress < kImageBase || theAddress - kImageBase + 64 > gImage.size())
		return kZeros;
	return gImage.data() + (theAddress - kImageBase);
}

uint32_t exeU32(uint32_t theAddress)
{
	uint32_t aValue;
	std::memcpy(&aValue, exePtr(theAddress), 4);
	return aValue;
}

int32_t exeI32(uint32_t theAddress)
{
	return (int32_t)exeU32(theAddress);
}

float exeFloat(uint32_t theAddress)
{
	float aValue;
	std::memcpy(&aValue, exePtr(theAddress), 4);
	return aValue;
}

const char* exeString(uint32_t theAddress)
{
	return (const char*)exePtr(theAddress);
}

std::vector<const char*> exeStringArray(uint32_t theAddress, int theCount)
{
	std::vector<const char*> aResult;
	for (int i = 0; i < theCount; i++)
		aResult.push_back(exeString(exeU32(theAddress + 4 * i)));
	return aResult;
}

} // namespace bonnies
