#include "GameLevels.h"
#include "PEImage.h"
#include "platform/VFS.h"

#include <SDL.h>
#include <cstring>

using namespace Sexy;

int Sexy::kAdventureMaze[225];
int Sexy::kAdventureLevel[225];
int Sexy::kExcluded[962];
int Sexy::kExcludedResume[966];
int Sexy::kMapPos[30];
int Sexy::kMapLinks[225];
int Sexy::kMapEdge[225];
int Sexy::kMapWay[252];

namespace
{

struct LevelTable
{
	const char*	mName;
	int*		mValues;
	uint32_t	mVA;
	uint32_t	mCount;
	uint32_t	mHash;
};

const LevelTable gLevelTables[] =
{
#define GAMELEVEL(theName, theCount, theVA, theHash) {#theName, theName, theVA, theCount, theHash},
#define TUTORIALPART(theVA, theLength, theHash)
#include "GameLevelsTable.inc"
#undef TUTORIALPART
#undef GAMELEVEL
};

struct TutorialCode
{
	uint32_t	mVA;		// where the part's statements begin in 0x40f520
	uint32_t	mLength;	// the bytes of its wall statements (they end at the first test after them)
	uint32_t	mHash;
};

const TutorialCode gTutorialCode[] =
{
#define GAMELEVEL(theName, theCount, theVA, theHash)
#define TUTORIALPART(theVA, theLength, theHash) {theVA, theLength, theHash},
#include "GameLevelsTable.inc"
#undef TUTORIALPART
#undef GAMELEVEL
};

const int NUM_TUTORIAL_PARTS = (int) (sizeof(gTutorialCode) / sizeof(gTutorialCode[0]));

std::vector<TutorialWall> gTutorialWalls[5];

// FNV-1a over the bytes (make_levels.py computes the same)
uint32_t Hash(const uint8_t* theBytes, size_t theLength)
{
	uint32_t h = 2166136261u;
	for (size_t i = 0; i < theLength; i++)
	{
		h ^= theBytes[i];
		h *= 16777619u;
	}
	return h;
}

// The length of an x86 ModRM byte with its SIB byte and displacement
size_t ModRMLength(const std::vector<uint8_t>& theData, size_t theIndex)
{
	int aMod = theData[theIndex] >> 6;
	int aRM = theData[theIndex] & 7;
	size_t aLength = 1;
	if (aMod != 3 && aRM == 4)
		aLength++;
	if (aMod == 0 && aRM == 5)
		aLength += 4;
	else if (aMod == 1)
		aLength += 1;
	else if (aMod == 2)
		aLength += 4;
	return aLength;
}

// A part of the tutorial's maze is built by statements `cell |= bits` that the compiler spread over registers:
// the constants 8, 2 and 4 are in ebx, edx and ecx when the part begins, a cell is loaded from [esi + 0x300 + 4 * cell],
// OR-ed with a register or an immediate, and stored back. A statement with an XOR (the exit) is not a wall.
// Only the instructions of those statements are known; anything else ends the decoding.
bool DecodeWalls(const std::vector<uint8_t>& d, size_t theStart, size_t theLength, std::vector<TutorialWall>& theWalls)
{
	bool aHasVal[8] = {false, false, false, false, false, false, false, false};
	int aVal[8] = {0, 0, 0, 0, 0, 0, 0, 0};
	bool aHasMem[8] = {false, false, false, false, false, false, false, false};
	uint32_t aMem[8] = {0, 0, 0, 0, 0, 0, 0, 0};
	int anOr[8] = {0, 0, 0, 0, 0, 0, 0, 0};
	bool aTaint[8] = {false, false, false, false, false, false, false, false};
	aHasVal[3] = true; aVal[3] = 8;
	aHasVal[2] = true; aVal[2] = 2;
	aHasVal[1] = true; aVal[1] = 4;

	size_t i = theStart;
	size_t anEnd = theStart + theLength;
	while (i < anEnd)
	{
		if (i + 8 > d.size())
			return false;
		int anOp = d[i];
		int aModRM = d[i + 1];
		int aReg = (aModRM >> 3) & 7;
		int aMod = aModRM >> 6;
		int aRM = aModRM & 7;
		if (anOp == 0x8b && (aModRM & 0xc7) == 0x86)			// mov reg, [esi + disp32]
		{
			aHasVal[aReg] = false;
			aMem[aReg] = ReadU32(d, i + 2);
			aHasMem[aReg] = true;
			anOr[aReg] = 0;
			aTaint[aReg] = false;
			i += 6;
		}
		else if (anOp == 0x0b && aMod == 3)						// or reg, reg
		{
			if (!aHasVal[aRM])
				return false;
			anOr[aReg] |= aVal[aRM];
			i += 2;
		}
		else if (anOp == 0x83 && (aModRM & 0xf8) == 0xc8)		// or reg, imm8
		{
			anOr[aRM] |= d[i + 2];
			i += 3;
		}
		else if (anOp == 0x89 && (aModRM & 0xc7) == 0x86)		// mov [esi + disp32], reg
		{
			uint32_t aDisp = ReadU32(d, i + 2);
			if (aHasMem[aReg] && aMem[aReg] == aDisp && !aTaint[aReg] && aDisp >= 0x300)
			{
				TutorialWall aWall = {(int) ((aDisp - 0x300) / 4), anOr[aReg]};
				theWalls.push_back(aWall);
			}
			i += 6;
		}
		else if (anOp >= 0xb8 && anOp <= 0xbf)					// mov reg, imm32
		{
			aHasVal[anOp - 0xb8] = true;
			aVal[anOp - 0xb8] = (int) ReadU32(d, i + 1);
			i += 5;
		}
		else if (anOp == 0x33 && aMod == 3)						// xor reg, reg
		{
			aTaint[aReg] = true;
			i += 2;
		}
		else if (anOp == 0x83 && ((aModRM & 0xf8) == 0xf0 || (aModRM & 0xf8) == 0xe0 || (aModRM & 0xf8) == 0xe8 ||
			(aModRM & 0xf8) == 0xf8 || (aModRM & 0xf8) == 0xd0))	// xor, and, sub, cmp, adc reg, imm8
		{
			aTaint[aRM] = true;
			i += 3;
		}
		else if (anOp == 0x8b || anOp == 0x89 || anOp == 0x8d || anOp == 0x3b || anOp == 0x39 || anOp == 0x85 ||
			anOp == 0x03 || anOp == 0x2b || anOp == 0x0b || anOp == 0x8a || anOp == 0x88)
		{
			// a register that is loaded from anywhere else no longer holds its constant or cell
			if (anOp == 0x8b || anOp == 0x03 || anOp == 0x2b || ((anOp == 0x8d || anOp == 0x0b) && aMod != 3))
			{
				aHasVal[aReg] = false;
				aHasMem[aReg] = false;
			}
			i += 1 + ModRMLength(d, i + 1);
		}
		else if (anOp == 0xc6)
			i += 1 + ModRMLength(d, i + 1) + 1;
		else if (anOp == 0xc7)
			i += 1 + ModRMLength(d, i + 1) + 4;
		else if (anOp == 0x83)
			i += 1 + ModRMLength(d, i + 1) + 1;
		else if (anOp == 0x81)
			i += 1 + ModRMLength(d, i + 1) + 4;
		else if ((anOp >= 0x40 && anOp <= 0x43) || (anOp >= 0x45 && anOp <= 0x4b) || anOp == 0x4d || anOp == 0x4e || anOp == 0x4f)
		{
			aHasVal[anOp & 7] = false;		// inc and dec
			i++;
		}
		else
			return false;
	}
	return i == anEnd;
}

}

const std::vector<TutorialWall>& Sexy::TutorialWalls(int thePart)
{
	static const std::vector<TutorialWall> sEmpty;
	return (thePart >= 0 && thePart < 5) ? gTutorialWalls[thePart] : sEmpty;
}

bool Sexy::LoadGameLevels()
{
	for (const LevelTable& aTable : gLevelTables)
		memset(aTable.mValues, 0, aTable.mCount * sizeof(int));
	for (int i = 0; i < 5; i++)
		gTutorialWalls[i].clear();

	std::vector<uint8_t> aData;
	if (!Platform::VFS::Get().ReadAll("WinMM.exe", aData) || aData.empty())
	{
		SDL_Log("GameLevels: WinMM.exe not found in the game folder; the level data is read from it");
		return false;
	}
	PEImage anImage;
	if (!anImage.Parse(aData))
	{
		SDL_Log("GameLevels: WinMM.exe is not a PE executable");
		return false;
	}

	int aMismatched = 0;
	for (const LevelTable& aTable : gLevelTables)
	{
		size_t anOffset = anImage.Offset(aTable.mVA);
		size_t aSize = aTable.mCount * 4;
		if (anOffset == 0 || anOffset + aSize > aData.size() || Hash(&aData[anOffset], aSize) != aTable.mHash)
		{
			aMismatched++;
			SDL_Log("GameLevels: %s does not match this executable", aTable.mName);
			continue;
		}
		for (uint32_t i = 0; i < aTable.mCount; i++)
			aTable.mValues[i] = (int) ReadU32(aData, anOffset + 4 * i);
	}
	for (int aPart = 0; aPart < NUM_TUTORIAL_PARTS && aPart < 5; aPart++)
	{
		const TutorialCode& aCode = gTutorialCode[aPart];
		size_t anOffset = anImage.Offset(aCode.mVA);
		if (anOffset == 0 || anOffset + aCode.mLength > aData.size() || Hash(&aData[anOffset], aCode.mLength) != aCode.mHash ||
			!DecodeWalls(aData, anOffset, aCode.mLength, gTutorialWalls[aPart]))
		{
			gTutorialWalls[aPart].clear();
			aMismatched++;
			SDL_Log("GameLevels: tutorial part %d does not match this executable", aPart);
		}
	}
	return aMismatched == 0;
}
