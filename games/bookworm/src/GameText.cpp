#include "GameText.h"
#include "SexyAppBase.h"
#include "platform/VFS.h"

#include <SDL.h>
#include <cstring>
#include <vector>

using namespace Sexy;

namespace
{

struct GameTextEntry
{
	const char*	mName;
	uint32_t	mVA;
	uint32_t	mLength;
	uint32_t	mHash;
};

const GameTextEntry gGameTextEntries[] =
{
#define GAMETEXT(theId, theVA, theLength, theHash) {#theId, theVA, theLength, theHash},
#include "GameTextTable.inc"
#undef GAMETEXT
	{"", 0, 0, 0}
};

std::vector<SexyString>& Texts()
{
	static std::vector<SexyString> sTexts(NUM_GAME_TEXTS);
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
	std::vector<Section> mSections;

	bool Parse(const std::vector<uint8_t>& theData, size_t theBase)
	{
		mBase = theBase;
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

const char* const gMarker = "!popcapcdprotect!";

}

// port: the original keeps its texts in the exe; the port reads each from the player's own exe by address (GameText.h)
bool Sexy::LoadGameTexts()
{
	std::vector<SexyString>& aTexts = Texts();
	for (SexyString& s : aTexts)
		s.clear();

	// The retail launcher (Bookworm.exe) or a carved game image (as tools/extract_game_exe.py writes it)
	std::vector<uint8_t> aData;
	const char* aNames[] = {"Bookworm.exe", "BookwormGame.exe"};
	std::string aFound;
	for (const char* aName : aNames)
	{
		if (Platform::VFS::Get().ReadAll(aName, aData) && !aData.empty())
		{
			aFound = aName;
			break;
		}
	}

	int aMatched = 0;
	int aMismatched = 0;
	if (!aFound.empty())
	{
		size_t aBase = 0;
		size_t aMarkerLen = strlen(gMarker);
		for (size_t i = 0; i + aMarkerLen < aData.size(); i++)
		{
			if ((aData[i] == '!') && (memcmp(&aData[i], gMarker, aMarkerLen) == 0))
			{
				aBase = i + aMarkerLen;
				while ((aBase + 1 < aData.size()) && !((aData[aBase] == 'M') && (aData[aBase + 1] == 'Z')))
					aBase++;
				break;
			}
		}
		PEImage anImage;
		if (anImage.Parse(aData, aBase))
		{
			for (int anId = 0; anId < NUM_GAME_TEXTS; anId++)
			{
				const GameTextEntry& e = gGameTextEntries[anId];
				size_t anOffset = anImage.Offset(e.mVA);
				if ((anOffset != 0) && (anOffset + e.mLength < aData.size()) && (aData[anOffset + e.mLength] == 0) &&
					(TextHash((const char*) &aData[anOffset], e.mLength) == e.mHash))
				{
					aTexts[anId].assign((const char*) &aData[anOffset], e.mLength);
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
			SDL_Log("GameText: %s is not a BookWorm executable", aFound.c_str());
		}
	}
	else
	{
		SDL_Log("GameText: Bookworm.exe not found in the game folder; the game's texts are read from it");
	}
	if (aMismatched > 0)
		SDL_Log("GameText: %d of %d texts do not match this executable (BookWorm Deluxe 1.03 expected)", aMismatched,
			(int) NUM_GAME_TEXTS);

	// Overrides from string properties (properties/texts.xml, mods)
	if (gSexyAppBase != NULL)
	{
		for (int anId = 0; anId < NUM_GAME_TEXTS; anId++)
			aTexts[anId] = gSexyAppBase->GetString(gGameTextEntries[anId].mName, aTexts[anId]);
	}
	return aMatched > 0;
}

// port: a game text by id
const SexyString& Sexy::GameText(GameTextId theId)
{
	static const SexyString sEmpty;
	if ((theId < 0) || (theId >= NUM_GAME_TEXTS))
		return sEmpty;
	return Texts()[theId];
}

// port: a game text by id, as a C string
const char* Sexy::GameTextC(GameTextId theId)
{
	return GameText(theId).c_str();
}

// port: the name of a game text's entry
const char* Sexy::GameTextName(GameTextId theId)
{
	if ((theId < 0) || (theId >= NUM_GAME_TEXTS))
		return "";
	return gGameTextEntries[theId].mName;
}
