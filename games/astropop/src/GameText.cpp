#include "GameText.h"
#include "SexyAppBase.h"
#include "platform/VFS.h"

#include <SDL.h>
#include <cstdio>
#include <cstring>
#include <map>
#include <set>
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
	size_t mBase = 0; // where the PE image starts in the file
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

// The texts that are not at their address in this image (another build of the game, such as the PopCap Game
// Collection's AstroPop.exe of 2007: the framework and the compiler are newer, the game's texts are the same and sit
// at other addresses), found by their content: every zero-terminated run of bytes in the image and every tail of
// it (the compiler pools texts that end the same) is looked up by length and hash. theFound says which entries are
// already read; the others are filled in.
int FindTextsByContent(const std::vector<uint8_t>& theData, const PEImage& theImage, std::vector<bool>& theFound,
	std::map<unsigned int, SexyString>& theTexts)
{
	std::map<std::pair<uint32_t, uint32_t>, std::vector<int> > aWanted;
	std::set<uint32_t> aLengths;
	for (int anId = 0; anId < NUM_GAME_TEXTS; anId++)
	{
		if (!theFound[anId])
		{
			aWanted[std::make_pair(gGameTextEntries[anId].mLength, gGameTextEntries[anId].mHash)].push_back(anId);
			aLengths.insert(gGameTextEntries[anId].mLength);
		}
	}
	int aFoundNow = 0;
	for (const PEImage::Section& aSection : theImage.mSections)
	{
		size_t aStart = theImage.mBase + aSection.mRawPtr;
		size_t anEnd = aStart + aSection.mRawSize;
		if (anEnd > theData.size())
			anEnd = theData.size();
		size_t aRunStart = aStart;
		for (size_t i = aStart; i < anEnd; i++)
		{
			if (theData[i] != 0)
				continue;
			// the run [aRunStart, i) ends in a zero byte: each of its tails of a wanted length is a candidate
			size_t aRunLength = i - aRunStart;
			for (uint32_t aLength : aLengths)
			{
				if (aLength > aRunLength)
					break;
				const char* aText = (const char*) &theData[i - aLength];
				std::map<std::pair<uint32_t, uint32_t>, std::vector<int> >::iterator anIt =
					aWanted.find(std::make_pair(aLength, TextHash(aText, aLength)));
				if (anIt == aWanted.end())
					continue;
				for (int anId : anIt->second)
				{
					if (!theFound[anId])
					{
						theFound[anId] = true;
						theTexts[gGameTextEntries[anId].mVA].assign(aText, aLength);
						aFoundNow++;
					}
				}
			}
			aRunStart = i + 1;
		}
	}
	return aFoundNow;
}

}

// port: the game's texts, loaded from the table that tools/make_gametext.py writes
bool Sexy::LoadGameTexts()
{
	std::map<unsigned int, SexyString>& aTexts = Texts();
	aTexts.clear();

	// The Steam copy's launcher (WinAP.exe), a carved game image (as tools/extract_game_exe.py writes it) or the PopCap
	// Game Collection's AstroPop.exe (the game itself, no launcher)
	std::vector<uint8_t> aData;
	bool aRead = false;
	const char* aNames[] = {"WinAP.exe", "AstroPopGame.exe", "AstroPop.exe"};
	for (const char* aName : aNames)
	{
		if (Platform::VFS::Get().ReadAll(aName, aData) && !aData.empty())
		{
			aRead = true;
			break;
		}
	}

	int aMatched = 0;
	int aMismatched = 0;
	if (aRead)
	{
		size_t aBase = 0;
		const char* aMarkers[] = {"!popcapdrmprotect!", "!popcapcdprotect!"};
		for (const char* aMarker : aMarkers)
		{
			size_t aMarkerLen = strlen(aMarker);
			bool aFoundMarker = false;
			for (size_t i = 0; i + aMarkerLen < aData.size(); i++)
			{
				if ((aData[i] == '!') && (memcmp(&aData[i], aMarker, aMarkerLen) == 0))
				{
					aBase = i + aMarkerLen;
					while ((aBase + 1 < aData.size()) && !((aData[aBase] == 'M') && (aData[aBase + 1] == 'Z')))
						aBase++;
					aFoundMarker = true;
					break;
				}
			}
			if (aFoundMarker)
				break;
		}
		PEImage anImage;
		if (anImage.Parse(aData, aBase))
		{
			std::vector<bool> aFound(NUM_GAME_TEXTS, false);
			for (int anId = 0; anId < NUM_GAME_TEXTS; anId++)
			{
				const GameTextEntry& e = gGameTextEntries[anId];
				size_t anOffset = anImage.Offset(e.mVA);
				if ((anOffset != 0) && (anOffset + e.mLength < aData.size()) && (aData[anOffset + e.mLength] == 0) &&
					(TextHash((const char*) &aData[anOffset], e.mLength) == e.mHash))
				{
					aTexts[e.mVA].assign((const char*) &aData[anOffset], e.mLength);
					aFound[anId] = true;
					aMatched++;
				}
				else
				{
					aMismatched++;
				}
			}
			if (aMismatched > 0)
			{
				// another build of the game (the PopCap Game Collection's): the texts are the same, at other addresses
				int aFoundByContent = FindTextsByContent(aData, anImage, aFound, aTexts);
				SDL_Log("GameText: %d of %d texts are not at their address in this executable; %d found by their content", aMismatched,
					NUM_GAME_TEXTS, aFoundByContent);
				aMatched += aFoundByContent;
				aMismatched -= aFoundByContent;
			}
		}
		else
		{
			SDL_Log("GameText: the executable of the game folder is not an AstroPop executable");
		}
	}
	else
	{
		SDL_Log("GameText: no WinAP.exe or AstroPop.exe in the game folder; the game's texts are read from it");
	}
	if (aMismatched > 0)
		SDL_Log("GameText: %d of %d texts are not in this executable (AstroPop Deluxe 1.0's or the PopCap Game Collection's expected)",
			aMismatched, NUM_GAME_TEXTS);

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

// port: the game's text by its id
const SexyString& Sexy::GameText(unsigned int theVA)
{
	static const SexyString sEmpty;
	std::map<unsigned int, SexyString>& aTexts = Texts();
	std::map<unsigned int, SexyString>::const_iterator anItr = aTexts.find(theVA);
	return anItr == aTexts.end() ? sEmpty : anItr->second;
}
