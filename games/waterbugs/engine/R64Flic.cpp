#include "R64Flic.h"

#include <cstdlib>
#include <cstring>

namespace r64
{

uint8_t gFlicPalette[0x300];
int gFlicSkipPalette = 0;

namespace
{

inline uint16_t U16(const uint8_t* p)
{
	return (uint16_t)(p[0] | p[1] << 8);
}

inline uint32_t U32(const uint8_t* p)
{
	return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}

// 0x40c4d0: COLOR_256 and COLOR_64 alike, without scaling. The index moves only by each packet's skip, as in the
// original; colours past the palette's end are dropped (the original writes on into the globals after it).
void DecodeColour(const uint8_t* theData)
{
	uint8_t aPackets = *theData;
	const uint8_t* aCursor = theData + 2;
	uint8_t anIndex = 0;
	if (gFlicSkipPalette == 0)
	{
		uint32_t aPacketsLeft = aPackets;
		if (aPackets != 0)
		{
			do
			{
				anIndex = (uint8_t)(anIndex + *aCursor);
				const uint8_t* aCountAt = aCursor + 1;
				uint16_t aCount = *aCountAt;
				aCursor = aCursor + 2;
				if (*aCountAt == 0)
					aCount = 0x100;
				int16_t k = 0;
				if ((int16_t)aCount * 3 != 0)
				{
					int j = 0;
					do
					{
						k = (int16_t)(k + 1);
						int at = (int)anIndex * 3 + j;
						if (at < 0x300)
							gFlicPalette[at] = *aCursor;
						j = k;
						aCursor = aCursor + 1;
					} while (j < (int16_t)aCount * 3);
				}
				aPacketsLeft = aPacketsLeft - 1;
			} while (aPacketsLeft != 0);
		}
	}
}

// 0x40c5a0: BYTE_RUN, each row as many packets as its count byte says (positive: a byte repeated, negative:
// bytes copied)
void DecodeByteRun(const uint8_t* theCursor, uint8_t* theDst, int16_t thePitch, uint16_t theHeight)
{
	if (0 < (int16_t)theHeight)
	{
		uint32_t aRows = theHeight;
		do
		{
			uint8_t aPackets = *theCursor;
			theCursor = theCursor + 1;
			int16_t k = 0;
			uint8_t* aDstPixel = theDst;
			if (aPackets != 0)
			{
				do
				{
					int8_t aSize = (int8_t)*theCursor;
					theCursor = theCursor + 1;
					if (aSize < 0)
					{
						int16_t i = 0;
						if (0 < -(int)aSize)
						{
							do
							{
								i = (int16_t)(i + 1);
								*aDstPixel = *theCursor;
								aDstPixel = aDstPixel + 1;
								theCursor = theCursor + 1;
							} while ((int)i < -(int)aSize);
						}
					}
					else
					{
						uint8_t v = *theCursor;
						theCursor = theCursor + 1;
						uint16_t n = (uint16_t)(int16_t)aSize;
						if (0 < (int16_t)n)
						{
							std::memset(aDstPixel, v, n);
							aDstPixel = aDstPixel + n;
						}
					}
					k = (int16_t)(k + 1);
				} while (k < (int16_t)(uint16_t)aPackets);
			}
			theDst = theDst + thePitch;
			aRows = aRows - 1;
		} while (aRows != 0);
	}
}

// 0x40c670: DELTA_FLI. The original reads another skip and row count after the rows until it reaches theEnd.
void DecodeDeltaFli(const uint8_t* theData, uint8_t* theDst, int16_t thePitch, const uint8_t* theEnd)
{
	const uint8_t* aCursor = theData;
	uint8_t* aRow = theDst;
	do
	{
		int16_t aRows = (int16_t)U16(aCursor + 2);
		aRow = aRow + (int)(int16_t)U16(aCursor) * (int)thePitch;
		aCursor = aCursor + 4;
		int16_t aRowsDone = 0;
		if (0 < aRows)
		{
			do
			{
				uint8_t aPackets = *aCursor;
				aCursor = aCursor + 1;
				int16_t k = 0;
				uint8_t* aDstPixel = aRow;
				if (aPackets != 0)
				{
					do
					{
						int8_t aSize = (int8_t)aCursor[1];
						aDstPixel = aDstPixel + *aCursor;
						aCursor = aCursor + 2;
						if (0 < aSize)
						{
							uint32_t n = (uint32_t)(uint16_t)(int16_t)aSize;
							do
							{
								*aDstPixel = *aCursor;
								aDstPixel = aDstPixel + 1;
								aCursor = aCursor + 1;
								n = n - 1;
							} while (n != 0);
						}
						if (aSize < 0)
						{
							uint8_t v = *aCursor;
							aCursor = aCursor + 1;
							int16_t i = 0;
							if (0 < -(int)aSize)
							{
								do
								{
									i = (int16_t)(i + 1);
									*aDstPixel = v;
									aDstPixel = aDstPixel + 1;
								} while ((int)i < -(int)aSize);
							}
						}
						if (theEnd <= aCursor)
							return;
						k = (int16_t)(k + 1);
					} while (k < (int16_t)(uint16_t)aPackets);
				}
				aRowsDone = (int16_t)(aRowsDone + 1);
				aRow = aRow + thePitch;
			} while (aRowsDone < aRows);
		}
		if (theEnd <= aCursor)
			return;
	} while (true);
}

// 0x40c770: DELTA_FLC, in words: 0xc000 words skip rows, a 0x8000 word puts its low byte where the row's packets
// end, and the packets skip bytes then copy or repeat words
void DecodeDeltaFlc(const uint8_t* theData, uint8_t* theDst, int16_t thePitch)
{
	const uint8_t* aCursor = theData + 2;
	if (0 < (int16_t)U16(theData))
	{
		uint32_t aLines = U16(theData);
		do
		{
			uint16_t aWord = U16(aCursor);
			const uint8_t* aLookahead = aCursor;
			while (aCursor = aLookahead + 2, (aWord & 0xc000) == 0xc000)
			{
				theDst = theDst - (int)(int16_t)aWord * (int)thePitch;
				aLookahead = aCursor;
				aWord = U16(aCursor);
			}
			uint16_t aCount = aWord;
			if ((int16_t)aWord < 0)
			{
				aCount = U16(aCursor);
				aCursor = aLookahead + 4;
			}
			uint32_t aPacketsLeft = aCount;
			uint8_t* aDstPixel = theDst;
			if (0 < (int16_t)aCount)
			{
				do
				{
					aDstPixel = aDstPixel + *aCursor;
					int8_t aSize = (int8_t)aCursor[1];
					aCursor = aCursor + 2;
					if (0 < aSize)
					{
						uint32_t i = (uint32_t)(uint16_t)(int16_t)aSize;
						do
						{
							std::memcpy(aDstPixel, aCursor, 2);
							aDstPixel = aDstPixel + 2;
							aCursor = aCursor + 2;
							i = i - 1;
						} while (i != 0);
					}
					if (aSize < 0)
					{
						uint8_t v[2] = {aCursor[0], aCursor[1]};
						aCursor = aCursor + 2;
						int16_t i = 0;
						if (0 < -(int)aSize)
						{
							do
							{
								i = (int16_t)(i + 1);
								std::memcpy(aDstPixel, v, 2);
								aDstPixel = aDstPixel + 2;
							} while ((int)i < -(int)aSize);
						}
					}
					aPacketsLeft = aPacketsLeft - 1;
				} while (aPacketsLeft != 0);
			}
			if ((int16_t)aWord < 0)
				*aDstPixel = (uint8_t)aWord;
			theDst = theDst + thePitch;
			aLines = aLines - 1;
		} while (aLines != 0);
	}
}

} // namespace

// The header's 0x80 bytes, then for an FLC the first frame at the header's oframe1 (an FLI's follows the header).
// The block's size is the header's file size, and what the file has past it is not read.
Flic* Flic_Open(const uint8_t* theFile, size_t theFileSize)
{
	if (theFileSize < 0x80)
		return nullptr;
	uint32_t aSize = U32(theFile);
	uint16_t aMagic = U16(theFile + 4);
	size_t aPos = 0x80;
	if (aMagic != 0xaf11)
	{
		if (aMagic != 0xaf12)
			return nullptr;
		aPos = U32(theFile + 0x50);
	}
	if (aSize < 0x14)
		return nullptr;
	Flic* aFlic = new Flic();
	aFlic->mem = (uint8_t*)std::calloc(aSize, 1);
	aFlic->memSize = aSize;
	aFlic->frames = U16(theFile + 6);
	aFlic->width = U16(theFile + 8);
	aFlic->height = U16(theFile + 0xa);
	if (aMagic == 0xaf11)
		aFlic->speed = U16(theFile + 0x10);
	else
	{
		int16_t aSpeed = (int16_t)((U32(theFile + 0x10) * 7) / 100);
		aFlic->speed = (uint16_t)aSpeed;
		if (aSpeed == 0)
			aFlic->speed = 1;
	}
	aFlic->frame = 0;
	aFlic->offsetPadding = 0;
	aFlic->offset = 0x14;
	aFlic->fd = 0;
	aFlic->endPadding = 0;
	if (aPos < theFileSize)
	{
		size_t n = theFileSize - aPos;
		if (n > aSize - 0x14)
			n = aSize - 0x14;
		std::memcpy(aFlic->mem + 0x14, theFile + aPos, n);
	}
	return aFlic;
}

// A frame or chunk that would run past the block ends the frame here (the original reads on)
void Flic_NextFrame(Flic* theFlic, uint8_t* theDst, int theDPitch)
{
	uint8_t* aMem = theFlic->mem;
	uint32_t anEnd = theFlic->memSize;
	if (theFlic->offset + 16 > anEnd)
		return;
	uint8_t* aFrame = aMem + theFlic->offset;
	theFlic->frame = (uint16_t)(theFlic->frame + 1);
	uint8_t* aChunk = aFrame + 16;
	int16_t aPitch = (int16_t)theDPitch;
	if (U16(aFrame + 4) == 0xf1fa && U16(aFrame + 6) != 0)
	{
		int16_t i = 0;
		do
		{
			if ((uint32_t)(aChunk - aMem) + 6 > anEnd || (uint32_t)(aChunk - aMem) + U32(aChunk) > anEnd ||
				U32(aChunk) < 6)
				break;
			switch (U16(aChunk + 4))
			{
			case 4:
				DecodeColour(aChunk + 6);
				aChunk[4] = 0xb;
				aChunk[5] = 0;
				DecodeColour(aChunk + 6);
				break;
			case 7:
				DecodeDeltaFlc(aChunk + 6, theDst, aPitch);
				break;
			case 0xb:
				DecodeColour(aChunk + 6);
				break;
			case 0xc:
				DecodeDeltaFli(aChunk + 6, theDst, aPitch, aChunk + U32(aChunk) + -1);
				break;
			case 0xf:
				DecodeByteRun(aChunk + 6, theDst, aPitch, theFlic->height);
				break;
			}
			aChunk = aChunk + U32(aChunk);
			i = (int16_t)(i + 1);
		} while ((int)i < (int)(uint32_t)U16(aFrame + 6));
	}
	if (theFlic->frames < theFlic->frame)
	{
		theFlic->frame = 1;
		theFlic->offset = 0x14;
		aFrame = aMem + 0x14;
	}
	theFlic->offset = theFlic->offset + U32(aFrame);
}

void Flic_Close(Flic* theFlic)
{
	std::free(theFlic->mem);
	delete theFlic;
}

} // namespace r64
