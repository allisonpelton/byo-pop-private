#include "GameText.h"
#include "Host.h"

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <map>
#include <vector>

namespace
{

struct GameTextEntry
{
	uint32_t mVA;
	uint32_t mLength;
	uint32_t mHash;
};

const GameTextEntry gGameTextEntries[] =
{
#define GAMETEXT(theVA, theLength, theHash) {theVA, theLength, theHash},
#include "GameTextTable.inc"
#undef GAMETEXT
	{0, 0, 0}
};

const int NUM_GAME_TEXTS = (int)(sizeof(gGameTextEntries) / sizeof(gGameTextEntries[0])) - 1;

std::map<uint32_t, std::string>& texts()
{
	static std::map<uint32_t, std::string> sTexts;
	return sTexts;
}

std::map<uint32_t, std::string>& strings()
{
	static std::map<uint32_t, std::string> sStrings;
	return sStrings;
}

// FNV-1a over the text's bytes (tools/make_gametext.py computes the same)
uint32_t textHash(const char* theText, size_t theLength)
{
	uint32_t h = 2166136261u;
	for (size_t i = 0; i < theLength; i++)
	{
		h ^= (uint8_t)theText[i];
		h *= 16777619u;
	}
	return h;
}

uint32_t readU32(const std::vector<uint8_t>& theData, size_t theOffset)
{
	if (theOffset + 4 > theData.size())
		return 0;
	return theData[theOffset] | (theData[theOffset + 1] << 8) | (theData[theOffset + 2] << 16) |
		((uint32_t)theData[theOffset + 3] << 24);
}

uint16_t readU16(const std::vector<uint8_t>& theData, size_t theOffset)
{
	if (theOffset + 2 > theData.size())
		return 0;
	return (uint16_t)(theData[theOffset] | (theData[theOffset + 1] << 8));
}

// A PE image inside theData at theBase, mapped from virtual addresses to file offsets
struct PEImage
{
	struct Section
	{
		uint32_t mVA;
		uint32_t mRawSize;
		uint32_t mRawPtr;
	};
	size_t mBase = 0;
	uint32_t mImageBase = 0;
	uint32_t mResourceRVA = 0;  // the resource directory (data directory 2)
	std::vector<Section> mSections;

	bool parse(const std::vector<uint8_t>& theData, size_t theBase)
	{
		mBase = theBase;
		if (theBase + 0x40 > theData.size() || theData[theBase] != 'M' || theData[theBase + 1] != 'Z')
			return false;
		size_t aPE = theBase + readU32(theData, theBase + 0x3c);
		if (aPE + 0xf8 > theData.size() || std::memcmp(&theData[aPE], "PE\0\0", 4) != 0)
			return false;
		int aNumSections = readU16(theData, aPE + 6);
		int anOptSize = readU16(theData, aPE + 20);
		uint32_t anImageBase = readU32(theData, aPE + 24 + 28);
		mImageBase = anImageBase;
		mResourceRVA = readU32(theData, aPE + 24 + 96 + 2 * 8);
		size_t aSection = aPE + 24 + anOptSize;
		for (int i = 0; i < aNumSections; i++, aSection += 40)
		{
			Section s;
			s.mVA = anImageBase + readU32(theData, aSection + 12);
			s.mRawSize = readU32(theData, aSection + 16);
			s.mRawPtr = readU32(theData, aSection + 20);
			mSections.push_back(s);
		}
		return !mSections.empty();
	}

	// The file offset of a virtual address, or 0 outside every section's raw data
	size_t offset(uint32_t theVA) const
	{
		for (const Section& s : mSections)
			if (theVA >= s.mVA && theVA < s.mVA + s.mRawSize)
				return mBase + s.mRawPtr + (theVA - s.mVA);
		return 0;
	}
};

// The string table (RT_STRING, type 6): block n of the resource directory holds the ids 16 (n - 1) to 16 n - 1, each a
// length and that many UTF-16 characters (the game's are all Latin-1)
void readStringTable(const std::vector<uint8_t>& theData, const PEImage& theImage,
                     std::map<uint32_t, std::string>& theStrings)
{
	if (theImage.mResourceRVA == 0)
		return;
	uint32_t aRoot = theImage.mImageBase + theImage.mResourceRVA;
	size_t aRootOffset = theImage.offset(aRoot);
	if (aRootOffset == 0)
		return;
	// a directory's entries: its named entries' count at +0x0c, its ids' at +0x0e, then 8 bytes each
	auto entries = [&](size_t theDirectory, std::vector<std::pair<uint32_t, uint32_t>>& theOut) {
		int aCount = readU16(theData, theDirectory + 12) + readU16(theData, theDirectory + 14);
		for (int i = 0; i < aCount; i++)
			theOut.emplace_back(readU32(theData, theDirectory + 16 + 8 * i), readU32(theData, theDirectory + 20 + 8 * i));
	};
	std::vector<std::pair<uint32_t, uint32_t>> aTypes;
	entries(aRootOffset, aTypes);
	for (const auto& aType : aTypes)
	{
		if (aType.first != 6 || (aType.second & 0x80000000) == 0)
			continue;
		std::vector<std::pair<uint32_t, uint32_t>> aBlocks;
		entries(aRootOffset + (aType.second & 0x7fffffff), aBlocks);
		for (const auto& aBlock : aBlocks)
		{
			if ((aBlock.second & 0x80000000) == 0)
				continue;
			std::vector<std::pair<uint32_t, uint32_t>> aLanguages;
			entries(aRootOffset + (aBlock.second & 0x7fffffff), aLanguages);
			if (aLanguages.empty() || (aLanguages[0].second & 0x80000000) != 0)
				continue;
			size_t aDataEntry = aRootOffset + aLanguages[0].second;
			size_t anOffset = theImage.offset(theImage.mImageBase + readU32(theData, aDataEntry));
			uint32_t aSize = readU32(theData, aDataEntry + 4);
			if (anOffset == 0 || anOffset + aSize > theData.size())
				continue;
			size_t aPos = anOffset;
			for (uint32_t anId = (aBlock.first - 1) * 16; anId < aBlock.first * 16 && aPos + 2 <= anOffset + aSize; anId++)
			{
				int aLength = readU16(theData, aPos);
				aPos += 2;
				if (aLength == 0)
					continue;
				std::string aText;
				for (int i = 0; i < aLength && aPos + 2 <= theData.size(); i++, aPos += 2)
					aText += (char)(uint8_t)readU16(theData, aPos);
				theStrings[anId] = aText;
			}
		}
	}
}

// The game image follows the retail launcher's marker (or the file is the carved image itself)
size_t findGameImage(const std::vector<uint8_t>& theData)
{
	const char* aMarkers[] = {"!popcapdrmprotect!", "!popcapcdprotect!"};
	for (const char* aMarker : aMarkers)
	{
		size_t aLength = std::strlen(aMarker);
		for (size_t i = 0; i + aLength < theData.size(); i++)
		{
			if (theData[i] == '!' && std::memcmp(&theData[i], aMarker, aLength) == 0)
			{
				size_t aBase = i + aLength;
				while (aBase + 1 < theData.size() && !(theData[aBase] == 'M' && theData[aBase + 1] == 'Z'))
					aBase++;
				return aBase;
			}
		}
	}
	return 0;
}

}

bool loadGameTexts()
{
	std::map<uint32_t, std::string>& aTexts = texts();
	aTexts.clear();
	engine::Host* aHost = engine::getHost();
	std::vector<uint8_t> aData;
	const char* aNames[] = {"PizzaFrenzy.exe", "PizzaFrenzyGame.exe"};
	std::string aFound;
	for (const char* aName : aNames)
	{
		if (aHost->readGameFile(aName, aData) && !aData.empty())
		{
			aFound = aName;
			break;
		}
	}
	char aLine[256];
	if (aFound.empty())
	{
		aHost->log("GameText: PizzaFrenzy.exe not found in the game folder; the game's texts are read from it\n");
		return false;
	}
	PEImage anImage;
	if (!anImage.parse(aData, findGameImage(aData)))
	{
		std::snprintf(aLine, sizeof(aLine), "GameText: %s is not a Pizza Frenzy executable\n", aFound.c_str());
		aHost->log(aLine);
		return false;
	}
	readStringTable(aData, anImage, strings());
	int aMatched = 0;
	for (int i = 0; i < NUM_GAME_TEXTS; i++)
	{
		const GameTextEntry& e = gGameTextEntries[i];
		size_t anOffset = anImage.offset(e.mVA);
		if (anOffset != 0 && anOffset + e.mLength < aData.size() && aData[anOffset + e.mLength] == 0 &&
			textHash((const char*)&aData[anOffset], e.mLength) == e.mHash)
		{
			aTexts[e.mVA].assign((const char*)&aData[anOffset], e.mLength);
			aMatched++;
		}
	}
	if (aMatched < NUM_GAME_TEXTS)
	{
		std::snprintf(aLine, sizeof(aLine), "GameText: %d of %d texts do not match this executable\n",
			NUM_GAME_TEXTS - aMatched, NUM_GAME_TEXTS);
		aHost->log(aLine);
	}
	return aMatched > 0 || !strings().empty();
}

const std::string& gameText(unsigned int theVA)
{
	static const std::string sEmpty;
	std::map<uint32_t, std::string>& aTexts = texts();
	std::map<uint32_t, std::string>::const_iterator it = aTexts.find(theVA);
	return it == aTexts.end() ? sEmpty : it->second;
}

const std::string& stringResource(unsigned int theId)
{
	static const std::string sEmpty;
	std::map<uint32_t, std::string>::const_iterator it = strings().find(theId);
	return it == strings().end() ? sEmpty : it->second;
}
