#include "Deal.h"

#include "Globals.h"
#include "JBBase.h"

#include <cstring>

namespace mahjongjapan
{

int gKindCounts[42];
int gBonusCounts[8];
int gQuadKinds;
int gPairKinds;
int gDealCount;  // 0x533630

// 0x413340: the two halves of a pair kind (0x2c and 0x2d), 0x2b with itself, the wild tiles 0x2e and 0x2f with any
// kind up to 0x2a, else as 0x413380
bool TilesMatch(int theKind, int theOther)
{
	if (theKind == 0x2c)
	{
		if (theOther == 0x2d)
			return true;
	}
	else if (theKind == 0x2b && theOther == 0x2b)
		return true;
	if (0x2a < theOther)
		return false;
	if (theKind != 0x2e && theKind != 0x2f)
		return KindsMatch(theKind, theOther);
	return true;
}

// 0x413380: the seasons match one another, so do the flowers, the rest only themselves
bool KindsMatch(int theKind, int theOther)
{
	if (0x22 < theKind && theKind < 0x27 && 0x22 < theOther && theOther < 0x27)
		return true;
	if (0x26 < theKind && theKind < 0x2b && 0x26 < theOther && theOther < 0x2b)
		return true;
	return theKind == theOther;
}

// 0x4133c0: suits 5-45 by their number, winds and dragons 50, seasons and flowers 100, 0x2b 1000
int KindPoints(int theKind)
{
	if (theKind < 0x1c)
		return ((theKind - 1) % 9 + 1) * 5;
	if (theKind < 0x1f)
		return 0x32;
	if (0x1e < theKind && theKind < 0x23)
		return 0x32;
	if (0x22 < theKind && theKind < 0x27)
		return 100;
	if (0x26 < theKind && theKind < 0x2b)
		return 100;
	return theKind == 0x2b ? 1000 : 0;
}

// 0x413a00: theSize more tiles to one of the 36 counts (the 34 kinds, all seasons, all flowers). While some kinds are
// still empty (theEmpty) a four goes to an empty one and so does a pair; with none left, any kind not yet full
// (theFull counts those at four) takes it. One rand() picks which.
void AddKind(int* theCounts, int theSize, int* theEmpty, int* theFull)
{
	int aRange;
	int aPick;
	int anEmpty;
	if (theSize == 2 && *theEmpty == 0)
	{
		aRange = 0x24 - *theFull;
		if (aRange == 0)
		{
			aPick = 0;
			goto pick;
		}
	}
	else
	{
		aRange = *theEmpty;
		if (aRange == 0)
		{
			aPick = 0;
			goto pick;
		}
	}
	aPick = jb::CrtRand() % aRange;
pick:
	int n = 0;
	int i = 0;
	do
	{
		if (theSize == 4)
		{
			if (theCounts[i] != 0)
				goto used;
		take:
			if (n == aPick)
			{
				if (theCounts[i] == 0)
					*theEmpty = *theEmpty - 1;
				int aCount = theCounts[i] + theSize;
				theCounts[i] = aCount;
				if (aCount == 4)
					*theFull = *theFull + 1;
				return;
			}
			n = n + 1;
		}
		else
		{
			if (theSize == 2)
			{
				anEmpty = *theEmpty;
				if (0 < anEmpty)
				{
					if (theCounts[i] == 0)
						goto take;
					goto used;
				}
			}
			else
			{
			used:
				anEmpty = *theEmpty;
			}
			if (anEmpty == 0 && theCounts[i] < 4)
				goto take;
		}
		i = i + 1;
		if (i == 0x24)
			i = 0;
	} while (true);
}

// 0x4136a0: theTiles tiles over the kinds, in rounds of at most 0x90; mQuadPercent of each round's fours are dealt as
// fours and the rest as pairs. A layout with its own counts (0x4150c0) has them dealt first, at most four of a kind a
// round. The seasons and flowers then go round the four of each, skipping those the layout fixed.
void Deal::Distribute(int theTiles)
{
	int aSeasons = 0;
	for (int i = 0; i < 0x24; i++)
		gKindCounts[i] = 0;
	gPairKinds = 0;
	gQuadKinds = 0;
	int aFixed = 0;
	int aFixedDealt = 0;
	int aCounts[42];
	int aBonus[8];
	if (mLayout->mHasCounts && (aFixed = mLayout->GetCounts(aCounts), 0 < aFixed))
	{
		int aFlowers = 0;
		for (int i = 0; i < 8; i++)
		{
			int aCount = aCounts[34 + i];
			aBonus[i] = aCount;
			if (i < 4)
				aSeasons = aSeasons + aCount;
			else
				aFlowers = aFlowers + aCount;
		}
		aCounts[34] = aSeasons;
		aCounts[35] = aFlowers;
	}
	gDealCount = 0;
	int aRound;
	for (; 0 < theTiles; theTiles = theTiles - aRound)
	{
		int aRoundCounts[36];
		int anEmpty = 0x24;
		for (int i = 0; i < 0x24; i++)
			aRoundCounts[i] = 0;
		int aFull = 0;
		int aTaken = 0;
		if (0 < aFixed && aFixedDealt < aFixed)
		{
			for (int i = 0; i < 0x24; i++)
			{
				int aCount = aCounts[i];
				if (0 < aCount)
				{
					if (aCount < 4)
					{
						aTaken = aTaken + aCount;
						aRoundCounts[i] = aCount;
						anEmpty = anEmpty - 1;
					}
					else
					{
						aTaken = aTaken + 4;
						anEmpty = anEmpty - 1;
						aRoundCounts[i] = 4;
						aFull = aFull + 1;
					}
				}
			}
			aFixedDealt = aFixedDealt + aTaken;
		}
		aRound = theTiles;
		if (0x8f < aTaken + theTiles)
			aRound = 0x90 - aTaken;
		int aQuads = ((aRound / 4) * mQuadPercent) / 100;
		int aPairs = (aRound + aQuads * -4) / 2;
		gQuadKinds = gQuadKinds + aQuads;
		gPairKinds = gPairKinds + aPairs;
		if (0 < aQuads)
		{
			do
			{
				AddKind(aRoundCounts, 4, &anEmpty, &aFull);
				aQuads = aQuads - 1;
			} while (aQuads != 0);
		}
		if (0 < aPairs)
		{
			do
			{
				AddKind(aRoundCounts, 2, &anEmpty, &aFull);
				aPairs = aPairs - 1;
			} while (aPairs != 0);
		}
		if (0 < aTaken)
		{
			for (int i = 0; i < 0x24; i++)
			{
				int aCount = aCounts[i];
				if (0 < aCount)
				{
					if (aCount < 5)
					{
						aCounts[i] = 0;
						aRoundCounts[i] = aRoundCounts[i] - aCount;
					}
					else
					{
						aRoundCounts[i] = aRoundCounts[i] - 4;
						aCounts[i] = aCount - 4;
					}
				}
			}
		}
		for (int i = 0; i < 0x24; i++)
			gKindCounts[i] = gKindCounts[i] + aRoundCounts[i];
	}
	for (int i = 0; i < 8; i++)
		gBonusCounts[i] = 0;
	int aSeasonTotal = gKindCounts[34];
	if (aFixed < 1)
	{
		int i = 0;
		if (0 < aSeasonTotal)
		{
			do
			{
				gBonusCounts[i % 4] = gBonusCounts[i % 4] + 1;
				i = i + 1;
			} while (i < aSeasonTotal);
		}
		int aFlowerTotal = gKindCounts[35];
		i = 0;
		if (0 < aFlowerTotal)
		{
			do
			{
				gBonusCounts[4 + i % 4] = gBonusCounts[4 + i % 4] + 1;
				i = i + 1;
			} while (i < aFlowerTotal);
		}
	}
	else
	{
		// The original reads on past the eight fixed counts into its stack when the totals are larger; the port takes
		// those as 0 (docs/DEVIATIONS.md)
		int i = 0;
		int aDealt = 0;
		if (0 < aSeasonTotal)
		{
			int* p = aBonus;
			do
			{
				if (aBonus + 8 <= p || *p < 1)
				{
					aDealt = aDealt + 1;
					gBonusCounts[i % 4] = gBonusCounts[i % 4] + 1;
				}
				else
					*p = *p - 1;
				i = i + 1;
				p = p + 1;
			} while (aDealt < aSeasonTotal);
		}
		int aFlowerTotal = gKindCounts[35];
		i = 0;
		aDealt = 0;
		if (0 < aFlowerTotal)
		{
			int* p = aBonus + 4;
			do
			{
				if (aBonus + 8 <= p || *p < 1)
				{
					aDealt = aDealt + 1;
					gBonusCounts[4 + i % 4] = gBonusCounts[4 + i % 4] + 1;
				}
				else
					*p = *p - 1;
				i = i + 1;
				p = p + 1;
			} while (aDealt < aFlowerTotal);
		}
	}
}

} // namespace mahjongjapan
