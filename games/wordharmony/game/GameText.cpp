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
	const char* aNames[] = {"WordHarmony.exe", "WordHarmonyGame.exe"};
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
		aHost->log("GameText: WordHarmony.exe not found in the game folder; the game's texts are read from it\n");
		return false;
	}
	PEImage anImage;
	if (!anImage.parse(aData, findGameImage(aData)))
	{
		std::snprintf(aLine, sizeof(aLine), "GameText: %s is not a Word Harmony executable\n", aFound.c_str());
		aHost->log(aLine);
		return false;
	}
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
	return aMatched > 0;
}

const std::string& gameText(unsigned int theVA)
{
	static const std::string sEmpty;
	std::map<uint32_t, std::string>& aTexts = texts();
	std::map<uint32_t, std::string>::const_iterator it = aTexts.find(theVA);
	return it == aTexts.end() ? sEmpty : it->second;
}
