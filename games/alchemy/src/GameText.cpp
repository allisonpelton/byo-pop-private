// The texts read from the player's own executable (see GameText.h).

#include "GameText.h"
#include "platform/VFS.h"

#include <SDL.h>
#include <stdint.h>
#include <string.h>
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

// 0x40bad1: the names of the strategic table, then of the time trial table, in the order the score table's constructor
// takes them. Address in the game image, length and FNV-1a hash of each (no text).
const GameTextEntry gGameTextEntries[] =
{
	{0x004e4148, 3, 0x9ed4f4c8},
	{0x004e4140, 4, 0x4e00443c},
	{0x004e4138, 5, 0x7dcf225c},
	{0x004e4130, 5, 0xbbcf1633},
	{0x004e412c, 3, 0xbfb3574a},
	{0x004e4124, 7, 0x2d1f32b6},
	{0x004e411c, 4, 0xf15e5ec7},
	{0x004e4114, 5, 0xfabe499f},
	{0x004e410c, 5, 0xe60ffad0},
	{0x004e4108, 3, 0x6b825e40}
};

struct GameTableEntry
{
	uint32_t	mVA;
	uint32_t	mCount;		// 32-bit values
	uint32_t	mHash;		// FNV-1a of their bytes
};

// 0x4de400: the order in which the pixels of a newly lit tile light up (Board::Update's dissolve, BoardUpdate.cpp)
const GameTableEntry gGameTableEntries[] =
{
	{0x004de400, 2749, 0x87741464}
};

const int NUM_GAME_TABLES = (int) (sizeof(gGameTableEntries) / sizeof(gGameTableEntries[0]));

const int NUM_GAME_TEXTS = (int) (sizeof(gGameTextEntries) / sizeof(gGameTextEntries[0]));

std::map<unsigned int, std::vector<int> >& Tables()
{
	static std::map<unsigned int, std::vector<int> > sTables;
	return sTables;
}

std::map<unsigned int, std::string>& Texts()
{
	static std::map<unsigned int, std::string> sTexts;
	return sTexts;
}

uint32_t TextHash(const unsigned char* theText, size_t theLength)
{
	uint32_t h = 2166136261u;
	for (size_t i = 0; i < theLength; i++)
	{
		h ^= theText[i];
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

// A PE image inside the file, with its sections mapped from virtual addresses to file offsets
struct PEImage
{
	struct Section
	{
		uint32_t mVA;
		uint32_t mRawSize;
		uint32_t mRawPtr;
	};
	size_t mBase = 0; // where the image starts in the file
	std::vector<Section> mSections;

	bool Parse(const std::vector<uint8_t>& theData, size_t theBase)
	{
		mBase = theBase;
		if (theData.size() < theBase + 0x40 || theData[theBase] != 'M' || theData[theBase + 1] != 'Z')
			return false;
		size_t aPE = theBase + ReadU32(theData, theBase + 0x3c);
		if (aPE + 0xf8 > theData.size() || memcmp(&theData[aPE], "PE\0\0", 4) != 0)
			return false;
		int aNumSections = ReadU16(theData, aPE + 6);
		int anOptSize = ReadU16(theData, aPE + 20);
		uint32_t anImageBase = ReadU32(theData, aPE + 24 + 28);
		size_t aSection = aPE + 24 + anOptSize;
		for (int i = 0; i < aNumSections; i++, aSection += 40)
		{
			Section aSectionInfo;
			aSectionInfo.mVA = anImageBase + ReadU32(theData, aSection + 12);
			aSectionInfo.mRawSize = ReadU32(theData, aSection + 16);
			aSectionInfo.mRawPtr = ReadU32(theData, aSection + 20);
			mSections.push_back(aSectionInfo);
		}
		return !mSections.empty();
	}

	// File offset of a virtual address, or 0 when it is outside every section's raw data
	size_t Offset(uint32_t theVA) const
	{
		for (size_t i = 0; i < mSections.size(); i++)
			if ((theVA >= mSections[i].mVA) && (theVA < mSections[i].mVA + mSections[i].mRawSize))
				return mBase + mSections[i].mRawPtr + (theVA - mSections[i].mVA);
		return 0;
	}
};

// The start of the game's image: after the launcher's marker when the file is the launcher, else the file's own start
size_t FindImage(const std::vector<uint8_t>& theData)
{
	static const char* const aMarkers[] = {"!popcapdrmprotect!", "!popcapcdprotect!"};
	for (size_t aMarker = 0; aMarker < sizeof(aMarkers) / sizeof(aMarkers[0]); aMarker++)
	{
		size_t aMarkerLen = strlen(aMarkers[aMarker]);
		for (size_t i = 0; i + aMarkerLen < theData.size(); i++)
		{
			if ((theData[i] == '!') && (memcmp(&theData[i], aMarkers[aMarker], aMarkerLen) == 0))
			{
				size_t aBase = i + aMarkerLen;
				while ((aBase + 1 < theData.size()) && !((theData[aBase] == 'M') && (theData[aBase + 1] == 'Z')))
					aBase++;
				return aBase;
			}
		}
	}
	return 0;
}

}

bool Sexy::LoadGameTexts()
{
	std::map<unsigned int, std::string>& aTexts = Texts();
	aTexts.clear();
	Tables().clear();

	std::vector<uint8_t> aData;
	bool aRead = false;
	static const char* const aNames[] = {"Alchemy.exe", "AlchemyGame.exe"};
	for (size_t i = 0; i < sizeof(aNames) / sizeof(aNames[0]); i++)
	{
		if (Platform::VFS::Get().ReadAll(aNames[i], aData) && !aData.empty())
		{
			aRead = true;
			break;
		}
	}
	if (!aRead)
	{
		SDL_Log("GameText: Alchemy.exe not found in the game folder; the score tables start with placeholder names");
		return false;
	}

	PEImage anImage;
	if (!anImage.Parse(aData, FindImage(aData)))
	{
		SDL_Log("GameText: Alchemy.exe is not an Alchemy executable");
		return false;
	}

	int aMatched = 0;
	for (int anId = 0; anId < NUM_GAME_TEXTS; anId++)
	{
		const GameTextEntry& anEntry = gGameTextEntries[anId];
		size_t anOffset = anImage.Offset(anEntry.mVA);
		if ((anOffset != 0) && (anOffset + anEntry.mLength < aData.size()) && (aData[anOffset + anEntry.mLength] == 0) &&
			(TextHash(&aData[anOffset], anEntry.mLength) == anEntry.mHash))
		{
			aTexts[anEntry.mVA].assign((const char*) &aData[anOffset], anEntry.mLength);
			aMatched++;
		}
	}
	for (int anId = 0; anId < NUM_GAME_TABLES; anId++)
	{
		const GameTableEntry& anEntry = gGameTableEntries[anId];
		size_t anOffset = anImage.Offset(anEntry.mVA);
		if ((anOffset != 0) && (anOffset + anEntry.mCount * 4 <= aData.size()) && (TextHash(&aData[anOffset], anEntry.mCount * 4) == anEntry.mHash))
		{
			std::vector<int>& aTable = Tables()[anEntry.mVA];
			for (uint32_t i = 0; i < anEntry.mCount; i++)
				aTable.push_back((int) ReadU32(aData, anOffset + i * 4));
		}
		else
			SDL_Log("GameText: the table at 0x%x does not match this executable", anEntry.mVA);
	}
	if (aMatched < NUM_GAME_TEXTS)
		SDL_Log("GameText: %d of %d texts do not match this executable (Alchemy Deluxe 1.6 expected)",
			NUM_GAME_TEXTS - aMatched, NUM_GAME_TEXTS);
	return aMatched > 0;
}

const std::string& Sexy::GameText(unsigned int theVA)
{
	static const std::string sEmpty;
	std::map<unsigned int, std::string>& aTexts = Texts();
	std::map<unsigned int, std::string>::const_iterator anItr = aTexts.find(theVA);
	return anItr == aTexts.end() ? sEmpty : anItr->second;
}

const std::vector<int>& Sexy::GameTable(unsigned int theVA)
{
	static const std::vector<int> sEmpty;
	std::map<unsigned int, std::vector<int> >::const_iterator anItr = Tables().find(theVA);
	return anItr == Tables().end() ? sEmpty : anItr->second;
}
