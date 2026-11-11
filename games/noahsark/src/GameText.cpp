#include "GameText.h"
#include "SexyAppBase.h"
#include "platform/VFS.h"

#include <SDL.h>
#include <cstdio>
#include <cstring>
#include <map>
#include <vector>

using namespace Sexy;

namespace
{

struct GameTextEntry
{
	uint32_t	mVA;
	uint32_t	mLength;
	uint32_t	mHash;
};

const GameTextEntry gGameTextEntries[] =
{
#define GAMETEXT(theVA, theLength, theHash) {theVA, theLength, theHash},
#include "GameTextTable.inc"
#undef GAMETEXT
	{0, 0, 0}
};

const int NUM_GAME_TEXTS = (int) (sizeof(gGameTextEntries) / sizeof(gGameTextEntries[0])) - 1;

std::map<unsigned int, SexyString>& Texts()
{
	static std::map<unsigned int, SexyString> sTexts;
	return sTexts;
}

// FNV-1a over the text's bytes (make_gametext.py computes the same)
uint32_t TextHash(const char* theText, size_t theLength)
{
	uint32_t aHash = 2166136261u;
	for (size_t i = 0; i < theLength; i++)
	{
		aHash ^= (uint8_t) theText[i];
		aHash *= 16777619u;
	}
	return aHash;
}

uint32_t ReadU32(const std::vector<uint8_t>& theData, size_t theOffset)
{
	if (theOffset + 4 > theData.size())
		return 0;
	return theData[theOffset] | (theData[theOffset + 1] << 8) | (theData[theOffset + 2] << 16) |
		((uint32_t) theData[theOffset + 3] << 24);
}

uint16_t ReadU16(const std::vector<uint8_t>& theData, size_t theOffset)
{
	if (theOffset + 2 > theData.size())
		return 0;
	return (uint16_t) (theData[theOffset] | (theData[theOffset + 1] << 8));
}

// A PE image, mapped from virtual addresses to file offsets
struct PEImage
{
	struct Section
	{
		uint32_t mVA;
		uint32_t mRawSize;
		uint32_t mRawPtr;
	};
	uint32_t mImageBase = 0;
	std::vector<Section> mSections;

	bool Parse(const std::vector<uint8_t>& theData)
	{
		if (theData.size() < 0x40 || theData[0] != 'M' || theData[1] != 'Z')
			return false;
		size_t aPE = ReadU32(theData, 0x3c);
		if (aPE + 0xf8 > theData.size() || memcmp(&theData[aPE], "PE\0\0", 4) != 0)
			return false;
		int aNumSections = ReadU16(theData, aPE + 6);
		int anOptSize = ReadU16(theData, aPE + 20);
		mImageBase = ReadU32(theData, aPE + 24 + 28);
		size_t aSection = aPE + 24 + anOptSize;
		for (int i = 0; i < aNumSections; i++, aSection += 40)
		{
			Section s;
			s.mVA = mImageBase + ReadU32(theData, aSection + 12);
			s.mRawSize = ReadU32(theData, aSection + 16);
			s.mRawPtr = ReadU32(theData, aSection + 20);
			mSections.push_back(s);
		}
		return !mSections.empty();
	}

	// File offset of a virtual address, or 0 when it is outside every section's raw data
	size_t Offset(uint32_t theVA) const
	{
		for (const Section& aSection : mSections)
			if ((theVA >= aSection.mVA) && (theVA < aSection.mVA + aSection.mRawSize))
				return aSection.mRawPtr + (theVA - aSection.mVA);
		return 0;
	}
};

}

bool Sexy::LoadGameTexts()
{
	std::map<unsigned int, SexyString>& aTexts = Texts();
	aTexts.clear();

	// The original's executable (version 1.1, 2002-10-17) or a copy of it
	std::vector<uint8_t> aData;
	bool aRead = Platform::VFS::Get().ReadAll("WinNoah.exe", aData) && !aData.empty();

	int aMatched = 0;
	int aMismatched = 0;
	if (aRead)
	{
		PEImage anImage;
		if (anImage.Parse(aData))
		{
			for (int anId = 0; anId < NUM_GAME_TEXTS; anId++)
			{
				const GameTextEntry& e = gGameTextEntries[anId];
				size_t anOffset = anImage.Offset(e.mVA);
				if ((anOffset != 0) && (anOffset + e.mLength < aData.size()) && (aData[anOffset + e.mLength] == 0) &&
					(TextHash((const char*) &aData[anOffset], e.mLength) == e.mHash))
				{
					aTexts[e.mVA].assign((const char*) &aData[anOffset], e.mLength);
					aMatched++;
				}
				else
				{
					aMismatched++;
				}
			}
		}
		else
		{
			SDL_Log("GameText: WinNoah.exe is not a PE executable");
		}
	}
	else
	{
		SDL_Log("GameText: WinNoah.exe not found in the game folder; the game's texts are read from it");
	}
	if (aMismatched > 0)
		SDL_Log("GameText: %d of %d texts do not match this executable (Noah's Ark Deluxe 1.1 expected)", aMismatched,
			NUM_GAME_TEXTS);

	// Overrides from string properties (properties/texts.xml, mods)
	if (gSexyAppBase != NULL)
	{
		for (int anId = 0; anId < NUM_GAME_TEXTS; anId++)
		{
			char aName[32];
			snprintf(aName, sizeof(aName), "GT_%08x", gGameTextEntries[anId].mVA);
			unsigned int aVA = gGameTextEntries[anId].mVA;
			aTexts[aVA] = gSexyAppBase->GetString(aName, aTexts[aVA]);
		}
	}
	return aMatched > 0;
}

const SexyString& Sexy::GameText(unsigned int theVA)
{
	static const SexyString sEmpty;
	std::map<unsigned int, SexyString>& aTexts = Texts();
	std::map<unsigned int, SexyString>::const_iterator anItr = aTexts.find(theVA);
	return anItr == aTexts.end() ? sEmpty : anItr->second;
}
