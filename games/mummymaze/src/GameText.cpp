#include "GameText.h"
#include "SexyAppBase.h"
#include "PEImage.h"
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

}

bool Sexy::LoadGameTexts()
{
	std::map<unsigned int, SexyString>& aTexts = Texts();
	aTexts.clear();

	// The original's executable (version 1.1, 2002-06-21) or a copy of it
	std::vector<uint8_t> aData;
	bool aRead = Platform::VFS::Get().ReadAll("WinMM.exe", aData) && !aData.empty();

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
			SDL_Log("GameText: WinMM.exe is not a PE executable");
		}
	}
	else
	{
		SDL_Log("GameText: WinMM.exe not found in the game folder; the game's texts are read from it");
	}
	if (aMismatched > 0)
		SDL_Log("GameText: %d of %d texts do not match this executable (Mummy Maze Deluxe 1.1 expected)", aMismatched,
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
