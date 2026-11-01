// The game's exe as data (see Exe.h): its sections mapped at their addresses.

#include "Exe.h"

#include <cstring>
#include <map>

namespace bonnies
{

namespace
{
const uint32_t kImageBase = 0x400000;
std::vector<uint8_t> gImage;  // the image as loaded, from the image base
const uint8_t kZeros[64] = {};

uint32_t rd32(const std::vector<uint8_t>& theData, size_t theOffset)
{
	if (theOffset + 4 > theData.size())
		return 0;
	return theData[theOffset] | theData[theOffset + 1] << 8 | theData[theOffset + 2] << 16 |
		(uint32_t)theData[theOffset + 3] << 24;
}

uint16_t rd16(const std::vector<uint8_t>& theData, size_t theOffset)
{
	if (theOffset + 2 > theData.size())
		return 0;
	return (uint16_t)(theData[theOffset] | theData[theOffset + 1] << 8);
}
}

bool loadExe(const std::vector<uint8_t>& theData)
{
	if (theData.size() < 0x40 || theData[0] != 'M' || theData[1] != 'Z')
		return false;
	uint32_t aPe = rd32(theData, 0x3c);
	if (rd32(theData, aPe) != 0x4550)
		return false;
	int aSections = rd16(theData, aPe + 6);
	int anOptSize = rd16(theData, aPe + 20);
	uint32_t anImageSize = rd32(theData, aPe + 24 + 56);
	if (rd32(theData, aPe + 24 + 28) != kImageBase || anImageSize > 0x2000000)
		return false;
	gImage.assign(anImageSize, 0);
	for (int i = 0; i < aSections; i++)
	{
		size_t aHeader = aPe + 24 + anOptSize + 40 * i;
		uint32_t aVirtualSize = rd32(theData, aHeader + 8);
		uint32_t anAddress = rd32(theData, aHeader + 12);
		uint32_t aRawSize = rd32(theData, aHeader + 16);
		uint32_t aRawPos = rd32(theData, aHeader + 20);
		uint32_t aSize = aRawSize < aVirtualSize ? aRawSize : aVirtualSize;
		if (anAddress + aSize > anImageSize || aRawPos + aSize > theData.size())
			return false;
		std::memcpy(gImage.data() + anAddress, theData.data() + aRawPos, aSize);
	}
	return true;
}

const uint8_t* exePtr(uint32_t theAddress)
{
	if (theAddress < kImageBase || theAddress - kImageBase + 64 > gImage.size())
		return kZeros;
	return gImage.data() + (theAddress - kImageBase);
}

uint32_t exeU32(uint32_t theAddress)
{
	uint32_t aValue;
	std::memcpy(&aValue, exePtr(theAddress), 4);
	return aValue;
}

int32_t exeI32(uint32_t theAddress)
{
	return (int32_t)exeU32(theAddress);
}

float exeFloat(uint32_t theAddress)
{
	float aValue;
	std::memcpy(&aValue, exePtr(theAddress), 4);
	return aValue;
}

const char* exeString(uint32_t theAddress)
{
	return (const char*)exePtr(theAddress);
}

std::vector<const char*> exeStringArray(uint32_t theAddress, int theCount)
{
	std::vector<const char*> aResult;
	for (int i = 0; i < theCount; i++)
		aResult.push_back(exeString(exeU32(theAddress + 4 * i)));
	return aResult;
}

namespace
{
std::map<uint32_t, std::string> gGlobalStrings;

// Emulates one initializer: the few instruction forms Visual C++ 7.1 emits for them (immediate and register stores to
// globals, pushes, the string constructor 0x41d720 and the calls registering destructors). Returns false at an
// instruction it does not know.
bool runInitializer(uint32_t theAddress)
{
	uint32_t aRegs[8] = {};  // eax, ecx, edx, ebx, esp, ebp, esi, edi
	std::vector<uint32_t> aStack;
	uint32_t pc = theAddress;
	for (int aCount = 0; aCount < 100000; aCount++)
	{
		const uint8_t* p = exePtr(pc);
		uint8_t op = p[0];
		if (op == 0xc3 || op == 0xc2 || op == 0xe9 || op == 0xeb)
			return true;
		if (op == 0x90)
		{
			pc += 1;
		}
		else if (op == 0xc7 && p[1] == 0x05)
		{
			uint32_t anAddress;
			std::memcpy(&anAddress, p + 2, 4);
			std::memcpy((void*)exePtr(anAddress), p + 6, 4);
			pc += 10;
		}
		else if (op == 0xa3)
		{
			uint32_t anAddress;
			std::memcpy(&anAddress, p + 1, 4);
			std::memcpy((void*)exePtr(anAddress), &aRegs[0], 4);
			pc += 5;
		}
		else if (op == 0x89 && (p[1] & 0xc7) == 0x05)
		{
			uint32_t anAddress;
			std::memcpy(&anAddress, p + 2, 4);
			std::memcpy((void*)exePtr(anAddress), &aRegs[p[1] >> 3 & 7], 4);
			pc += 6;
		}
		else if ((op == 0x89 || op == 0x88 || op == 0x8b) && (p[1] & 0xc7) == 0x44 && p[2] == 0x24)
		{
			pc += 4;  // a store to or load from the stack frame (the unwind state)
		}
		else if ((op == 0x89 || op == 0x88 || op == 0x8b) && (p[1] & 0xc7) == 0x45)
		{
			pc += 3;
		}
		else if (op == 0xc7 && p[1] == 0x44 && p[2] == 0x24)
		{
			pc += 8;
		}
		else if (op == 0xc6 && p[1] == 0x44 && p[2] == 0x24)
		{
			pc += 5;
		}
		else if (op >= 0xb8 && op <= 0xbf)
		{
			std::memcpy(&aRegs[op - 0xb8], p + 1, 4);
			pc += 5;
		}
		else if ((op == 0x33 || op == 0x31) && (p[1] & 0xc0) == 0xc0)
		{
			aRegs[p[1] & 7] = 0;
			pc += 2;
		}
		else if (op == 0x68)
		{
			uint32_t aValue;
			std::memcpy(&aValue, p + 1, 4);
			aStack.push_back(aValue);
			pc += 5;
		}
		else if (op == 0x6a)
		{
			aStack.push_back((uint32_t)(int32_t)(int8_t)p[1]);
			pc += 2;
		}
		else if (op >= 0x50 && op <= 0x57)
		{
			aStack.push_back(aRegs[op - 0x50]);
			pc += 1;
		}
		else if (op >= 0x58 && op <= 0x5f)
		{
			if (!aStack.empty())
				aStack.pop_back();
			pc += 1;
		}
		else if (op == 0x64 && p[1] == 0xa1)
		{
			pc += 6;
		}
		else if (op == 0x64 && p[1] == 0x89)
		{
			pc += 7;
		}
		else if (op == 0x83 && (p[1] == 0xc4 || p[1] == 0xec))
		{
			pc += 3;
		}
		else if (op == 0xe8)
		{
			int32_t aDelta;
			std::memcpy(&aDelta, p + 1, 4);
			uint32_t aTarget = pc + 5 + (uint32_t)aDelta;
			if (aTarget == 0x41d720)
			{
				// The string constructor from a literal (this in ecx, the literal pushed; it pops it)
				uint32_t aLiteral = aStack.empty() ? 0 : aStack.back();
				if (!aStack.empty())
					aStack.pop_back();
				gGlobalStrings[aRegs[1]] = aLiteral != 0 ? exeString(aLiteral) : "";
			}
			pc += 5;
		}
		else
			return false;
	}
	return false;
}

const std::string kEmpty;
}

int runStaticInitializers(std::vector<uint32_t>* theUnknown)
{
	int aDone = 0;
	for (uint32_t anEntry = 0x466000; anEntry < 0x466284; anEntry += 4)
	{
		uint32_t anInit = exeU32(anEntry);
		if (anInit < 0x443430 || anInit >= 0x4561fa)
			continue;
		if (runInitializer(anInit))
			aDone++;
		else if (theUnknown != nullptr)
			theUnknown->push_back(anInit);
	}
	return aDone;
}

const std::string& exeGlobalString(uint32_t theAddress)
{
	auto anIt = gGlobalStrings.find(theAddress);
	if (anIt == gGlobalStrings.end())
		return kEmpty;
	return anIt->second;
}

} // namespace bonnies
