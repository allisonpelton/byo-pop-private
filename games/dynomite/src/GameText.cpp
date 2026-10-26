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
	uint32_t h = 2166136261u;
	for (size_t i = 0; i < theLength; i++)
	{
		h ^= (uint8_t) theText[i];
		h *= 16777619u;
	}
	return h;
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

// A PE image starting at mBase in the file, mapped from virtual addresses to file offsets
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
	std::vector<Section> mSections;

	bool Parse(const std::vector<uint8_t>& theData, size_t theBase)
	{
		mBase = theBase;
		mSections.clear();
		if (theBase + 0x40 > theData.size() || theData[theBase] != 'M' || theData[theBase + 1] != 'Z')
			return false;
		size_t aPE = theBase + ReadU32(theData, theBase + 0x3c);
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
		for (const Section& s : mSections)
			if ((theVA >= s.mVA) && (theVA < s.mVA + s.mRawSize))
				return mBase + s.mRawPtr + (theVA - s.mVA);
		return 0;
	}
};

// The texts of the image at theBase that match the table; theTexts gets them when theTexts is not NULL
int MatchTexts(const std::vector<uint8_t>& theData, const PEImage& theImage,
	std::map<unsigned int, SexyString>* theTexts)
{
	int aMatched = 0;
	for (int anId = 0; anId < NUM_GAME_TEXTS; anId++)
	{
		const GameTextEntry& e = gGameTextEntries[anId];
		size_t anOffset = theImage.Offset(e.mVA);
		if ((anOffset != 0) && (anOffset + e.mLength < theData.size()) && (theData[anOffset + e.mLength] == 0) &&
			(TextHash((const char*) &theData[anOffset], e.mLength) == e.mHash))
		{
			if (theTexts != NULL)
				(*theTexts)[e.mVA].assign((const char*) &theData[anOffset], e.mLength);
			aMatched++;
		}
	}
	return aMatched;
}

}

bool Sexy::LoadGameTexts()
{
	std::map<unsigned int, SexyString>& aTexts = Texts();
	aTexts.clear();

	// The retail Dynomite.exe, or the game image on its own
	std::vector<uint8_t> aData;
	bool aRead = Platform::VFS::Get().ReadAll("dynomite.exe", aData) && !aData.empty();

	int aMatched = 0;
	if (aRead)
	{
		// the image whose texts match best
		PEImage aBest;
		int aBestCount = 0;
		for (size_t i = 0; i + 0x40 < aData.size(); i++)
		{
			if ((aData[i] != 'M') || (aData[i + 1] != 'Z'))
				continue;
			PEImage anImage;
			if (!anImage.Parse(aData, i))
				continue;
			int aCount = MatchTexts(aData, anImage, NULL);
			if (aCount > aBestCount)
			{
				aBest = anImage;
				aBestCount = aCount;
			}
		}
		if (aBestCount > 0)
			aMatched = MatchTexts(aData, aBest, &aTexts);
		if (aMatched < NUM_GAME_TEXTS)
			SDL_Log("GameText: %d of %d texts do not match this executable (Dynomite Deluxe expected)",
				NUM_GAME_TEXTS - aMatched, NUM_GAME_TEXTS);
	}
	else
	{
		SDL_Log("GameText: Dynomite.exe not found in the game folder; a few of the game's texts are read from it");
	}

	// Overrides from string properties (mods)
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
