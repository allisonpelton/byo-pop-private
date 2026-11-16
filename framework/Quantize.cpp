#include "Quantize.h"
#include <assert.h>
#include <math.h>
#include <string.h>

using namespace Sexy;

bool Sexy::Quantize8Bit(const ulong* theSrcBits, int theWidth, int theHeight, uchar* theDestColorIndices, ulong* theDestColorTable)
{
	int aSize = theWidth*theHeight;

	int aColorTableSize = 0;
		
	ulong aSearchTable[256];
	uchar aTranslationTable[256]; // From search table to color table

	if (aSize > 0)
	{
		aSearchTable[0] = theSrcBits[0];
		theDestColorTable[0] = theSrcBits[0];
		aTranslationTable[0] = 0;
		theDestColorIndices[0] = 0;
		aColorTableSize++;
	}

	for (int anIdx = 1; anIdx < aSize; anIdx++)
	{
		ulong aColor = theSrcBits[anIdx];		

		int aLeftPos = 0;
		int aRightPos = aColorTableSize-1;
		int aMiddlePos = (aLeftPos+aRightPos)/2;

		for (;;)
		{	
			ulong aCheckColor = aSearchTable[aMiddlePos];
			
			if (aColor < aCheckColor)
				aRightPos = aMiddlePos - 1;
			else if (aColor > aCheckColor)
				aLeftPos = aMiddlePos + 1;			
			else
			{
				theDestColorIndices[anIdx] = aTranslationTable[aMiddlePos];
				break;
			}

			if (aLeftPos > aRightPos)
			{
				if (aColorTableSize >= 256)
					return false;

				int anInsertPos = aLeftPos;
				if ((anInsertPos < aColorTableSize) && (aColor > aSearchTable[anInsertPos]))
					anInsertPos++;

				// Insert color into the table
				memmove(aSearchTable+anInsertPos+1, aSearchTable+anInsertPos, (aColorTableSize-anInsertPos) * sizeof(ulong));
				aSearchTable[anInsertPos] = aColor;

				memmove(aTranslationTable+anInsertPos+1, aTranslationTable+anInsertPos, (aColorTableSize-anInsertPos) * sizeof(uchar));
				aTranslationTable[anInsertPos] = aColorTableSize;

				theDestColorTable[aColorTableSize] = aColor;

				theDestColorIndices[anIdx] = aColorTableSize;

				aColorTableSize++;

				break;
			}

			aMiddlePos = (aLeftPos+aRightPos)/2;
		}
	}

	return true;
}

int Sexy::QuantizeIndices(const ulong* theSrcBits, int theWidth, int theHeight, const uint8_t* theIndices, const uint32_t* theColors,
	const uint8_t* theMaskIndices, const uint32_t* theMaskColors, uchar* theDestColorIndices, ulong* theDestColorTable)
{
	int aSize = theWidth*theHeight;
	int aColorTableSize = 0;

	// a colour's entry, looked up only at the first pixel of each index (or pair): the same colour from two indices is
	// one entry, as in Quantize8Bit; -1 for a 257th colour
	auto anEntryOf = [&](ulong theColor) -> int
	{
		for (int i = 0; i < aColorTableSize; i++)
		{
			if (theDestColorTable[i] == theColor)
				return i;
		}
		if (aColorTableSize >= 256)
			return -1;
		theDestColorTable[aColorTableSize] = theColor;
		return aColorTableSize++;
	};

	if (theMaskIndices == NULL)
	{
		// 256 indices give 256 colours at most: never a refusal
		short anEntries[256];
		for (int i = 0; i < 256; i++)
			anEntries[i] = -1;
		for (int anIdx = 0; anIdx < aSize; anIdx++)
		{
			int anIndex = theIndices[anIdx];
			int anEntry = anEntries[anIndex];
			if (anEntry < 0)
			{
				ulong aColor = theColors[anIndex];
				if (aColor != theSrcBits[anIdx])
					return -1;
				anEntry = anEntryOf(aColor);
				anEntries[anIndex] = (short) anEntry;
			}
			else if (theDestColorTable[anEntry] != theSrcBits[anIdx])
				return -1;
			theDestColorIndices[anIdx] = (uchar) anEntry;
		}
		return 1;
	}

	// a colour index and a mask index: an entry for each of the 65,536 pairs, made once and set back after each use (only
	// the pairs seen); the loading runs on one thread
	static std::vector<short> aPairEntries(65536, -1);
	std::vector<uint16_t> aSeen;
	int aResult = 1;
	for (int anIdx = 0; anIdx < aSize; anIdx++)
	{
		int aPair = (theIndices[anIdx] << 8) | theMaskIndices[anIdx];
		int anEntry = aPairEntries[aPair];
		if (anEntry < 0)
		{
			ulong aColor = (theColors[theIndices[anIdx]] & 0x00FFFFFF) | ((theMaskColors[theMaskIndices[anIdx]] & 0xFF) << 24);
			if (aColor != theSrcBits[anIdx])
			{
				aResult = -1;
				break;
			}
			anEntry = anEntryOf(aColor);
			if (anEntry < 0)
			{
				aResult = 0;
				break;
			}
			aPairEntries[aPair] = (short) anEntry;
			aSeen.push_back((uint16_t) aPair);
		}
		else if (theDestColorTable[anEntry] != theSrcBits[anIdx])
		{
			aResult = -1;
			break;
		}
		theDestColorIndices[anIdx] = (uchar) anEntry;
	}
	for (uint16_t aPair : aSeen)
		aPairEntries[aPair] = -1;
	return aResult;
}
