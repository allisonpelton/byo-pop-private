// The C runtime's qsort, translated from the games' exes (platform/crt/CrtQsort.h). Every exe's copy is the same
// algorithm: an insertion-sort-like pass for 8 elements or fewer (CrtShortSort, which moves the largest to the end, again
// and again), above that the median of three of the first, middle and last elements, then a partition around the middle
// one, the smaller part next and the larger one on a stack of 30 entries.
// qsort 0x5e8950 (Bejeweled 2), 0x783340 (Bejeweled Twist), 0x444c50 with its short sort 0x444bc0 (Cosmic Bugs),
// 0x445090 with its short sort 0x445000 (Water Bugs)

#include "platform/crt/CrtQsort.h"

#include <cstddef>

namespace Platform
{

namespace
{

void CrtSwap(char* theA, char* theB, size_t theWidth)
{
	if (theA == theB)
		return;
	while (theWidth--)
	{
		char aByte = *theA;
		*theA++ = *theB;
		*theB++ = aByte;
	}
}

// moves the largest to the end, again and again
void CrtShortSort(char* theLo, char* theHi, size_t theWidth, int (*theCompare)(const void*, const void*))
{
	while (theHi > theLo)
	{
		char* aMax = theLo;
		for (char* p = theLo + theWidth; p <= theHi; p += theWidth)
		{
			if (theCompare(p, aMax) > 0)
				aMax = p;
		}
		CrtSwap(aMax, theHi, theWidth);
		theHi -= theWidth;
	}
}

} // namespace

void CrtQsort(void* theBase, size_t theNum, size_t theWidth, int (*theCompare)(const void*, const void*))
{
	if (theNum < 2)
		return;
	char* aLoStack[30];
	char* aHiStack[30];
	int aStackPtr = 0;
	char* aLo = (char*)theBase;
	char* aHi = (char*)theBase + theWidth * (theNum - 1);
	for (;;)
	{
		size_t aSize = (size_t)(aHi - aLo) / theWidth + 1;
		if (aSize <= 8)
		{
			CrtShortSort(aLo, aHi, theWidth, theCompare);
		}
		else
		{
			// the median of three, then the partition
			char* aMid = aLo + (aSize / 2) * theWidth;
			if (theCompare(aLo, aMid) > 0)
				CrtSwap(aLo, aMid, theWidth);
			if (theCompare(aLo, aHi) > 0)
				CrtSwap(aLo, aHi, theWidth);
			if (theCompare(aMid, aHi) > 0)
				CrtSwap(aMid, aHi, theWidth);
			char* aLoGuy = aLo;
			char* aHiGuy = aHi;
			for (;;)
			{
				if (aMid > aLoGuy)
				{
					do
					{
						aLoGuy += theWidth;
					} while (aLoGuy < aMid && theCompare(aLoGuy, aMid) <= 0);
				}
				if (aMid <= aLoGuy)
				{
					do
					{
						aLoGuy += theWidth;
					} while (aLoGuy <= aHi && theCompare(aLoGuy, aMid) <= 0);
				}
				do
				{
					aHiGuy -= theWidth;
				} while (aHiGuy > aMid && theCompare(aHiGuy, aMid) > 0);
				if (aHiGuy < aLoGuy)
					break;
				CrtSwap(aLoGuy, aHiGuy, theWidth);
				if (aMid == aHiGuy)
					aMid = aLoGuy;
			}
			aHiGuy += theWidth;
			if (aMid < aHiGuy)
			{
				do
				{
					aHiGuy -= theWidth;
				} while (aHiGuy > aMid && theCompare(aHiGuy, aMid) == 0);
			}
			if (aMid >= aHiGuy)
			{
				do
				{
					aHiGuy -= theWidth;
				} while (aHiGuy > aLo && theCompare(aHiGuy, aMid) == 0);
			}
			// the smaller part next, the larger one on the stack
			if (aHiGuy - aLo >= aHi - aLoGuy)
			{
				if (aLo < aHiGuy)
				{
					aLoStack[aStackPtr] = aLo;
					aHiStack[aStackPtr] = aHiGuy;
					aStackPtr++;
				}
				if (aLoGuy < aHi)
				{
					aLo = aLoGuy;
					continue;
				}
			}
			else
			{
				if (aLoGuy < aHi)
				{
					aLoStack[aStackPtr] = aLoGuy;
					aHiStack[aStackPtr] = aHi;
					aStackPtr++;
				}
				if (aLo < aHiGuy)
				{
					aHi = aHiGuy;
					continue;
				}
			}
		}
		aStackPtr--;
		if (aStackPtr < 0)
			return;
		aLo = aLoStack[aStackPtr];
		aHi = aHiStack[aStackPtr];
	}
}

} // namespace Platform
