#include "GameText.h"
#include "SexyAppBase.h"
#include "platform/VFS.h"

#include <SDL.h>
#include <cstdio>
#include <cstring>
#include <map>
#include <string>
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

std::map<uint32_t, std::string>& Texts()
{
	static std::map<uint32_t, std::string> sTexts;
	return sTexts;
}

uint32_t TextHash(const char* theText, size_t theLength)
{
	uint32_t h = 2166136261u;
	for (size_t i = 0; i < theLength; i++)
	{
		h ^= (uint8_t)theText[i];
		h *= 16777619u;
	}
	return h;
}

uint32_t ReadU32(const std::vector<uint8_t>& theData, size_t theOffset)
{
	if (theOffset + 4 > theData.size())
		return 0;
	return theData[theOffset] | (theData[theOffset + 1] << 8) | (theData[theOffset + 2] << 16) |
		((uint32_t)theData[theOffset + 3] << 24);
}

uint16_t ReadU16(const std::vector<uint8_t>& theData, size_t theOffset)
{
	if (theOffset + 2 > theData.size())
		return 0;
	return (uint16_t)(theData[theOffset] | (theData[theOffset + 1] << 8));
}

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

	size_t Offset(uint32_t theVA) const
	{
		for (const Section& s : mSections)
			if (theVA >= s.mVA && theVA < s.mVA + s.mRawSize)
				return s.mRawPtr + (theVA - s.mVA);
		return 0;
	}
};

} // namespace

bool tiptop::LoadGameTexts()
{
	std::map<uint32_t, std::string>& aTexts = Texts();
	aTexts.clear();

	std::vector<uint8_t> aData;
	bool aRead = Platform::VFS::Get().ReadAll("TipTop.exe", aData) && !aData.empty();

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
				if (anOffset != 0 && anOffset + e.mLength < aData.size() && aData[anOffset + e.mLength] == 0 &&
					TextHash((const char*)&aData[anOffset], e.mLength) == e.mHash)
				{
					aTexts[e.mVA].assign((const char*)&aData[anOffset], e.mLength);
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
			SDL_Log("GameText: TipTop.exe is not a PE executable");
		}
	}
	else
	{
		SDL_Log("GameText: TipTop.exe not found in the game folder; the game's texts are read from it");
	}
	if (aMismatched > 0)
		SDL_Log("GameText: %d of %d strings do not match this executable (TipTop Deluxe expected)", aMismatched,
			NUM_GAME_TEXTS);

	// Overrides from string properties (mods)
	if (Sexy::gSexyAppBase != nullptr)
	{
		for (int anId = 0; anId < NUM_GAME_TEXTS; anId++)
		{
			char aName[32];
			snprintf(aName, sizeof(aName), "GT_%08x", gGameTextEntries[anId].mVA);
			uint32_t aVA = gGameTextEntries[anId].mVA;
			aTexts[aVA] = Sexy::gSexyAppBase->GetString(aName, aTexts[aVA]);
		}
	}
	return aMatched > 0;
}

const char* tiptop::GameText(uint32_t theVA)
{
	std::map<uint32_t, std::string>& aTexts = Texts();
	std::map<uint32_t, std::string>::const_iterator anItr = aTexts.find(theVA);
	return anItr == aTexts.end() ? "" : anItr->second.c_str();
}
