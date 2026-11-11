#include "Deal.h"

#include "Globals.h"
#include "Host.h"
#include "JBBase.h"

#include <cstring>

namespace mahjongchina
{

int gKindCounts[42];
int gBonusCounts[8];
int gQuadKinds;
int gPairKinds;
int gDealCount;  // 0x533630

// The search (0x413d90): its state is in globals in the original
static bool gSolve2b;              // 0x5335f8 a tile of kind 0x2b is among those taken
static int gSolveKinded;           // 0x5335fc the tiles on the board with a kind
static uint32_t gSolveLimit;       // 0x533600 ms
static uint32_t gSolveStart;       // 0x533604
static int gSolveSteps;            // 0x533608 -1 when the time is up
static int gQuadNext;              // 0x53360c
static int gQuadTiles;             // 0x533610
static int gPairNext;              // 0x533614
static int gPairTiles;             // 0x533618
static int gHalvesLeft;            // 0x53361c
static int gHalvesCount;           // 0x533620
static int gSolveBlank;            // 0x533624 the tiles taken without a kind
static int gSolveQuads;            // 0x533628 the pairs taken as halves of fours
static int gSolveTaken;            // 0x53362c
static Tile* gPairOrder[400];      // 0x533704 the order's pairs, filled from the end
static Tile* gQuadOrder[400];      // 0x533d44 the order's fours
static Tile* gHalves[400];         // 0x534384 the tiles that match 0x2d
static Tile* gSolveFreed[400][20]; // 0x5349c4 the tiles a move frees, by depth
static Tile* gSolveFree[400][400]; // 0x53c6c4 the free tiles, by depth

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
	int aSeen = 0;
	int i = 0;
	do
	{
		if (theSize == 4)
		{
			if (theCounts[i] != 0)
				goto used;
		take:
			if (aSeen == aPick)
			{
				if (theCounts[i] == 0)
					*theEmpty = *theEmpty - 1;
				int aCount = theCounts[i] + theSize;
				theCounts[i] = aCount;
				if (aCount == 4)
					*theFull = *theFull + 1;
				return;
			}
			aSeen = aSeen + 1;
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

// 0x413500
void Deal::Start(Layout* theLayout, int theQuadPercent, int theMove, bool theRedeal)
{
	mLayout = theLayout;
	mQuadPercent = theQuadPercent;
	mMove = theMove;
	int aCount = theLayout->CountBlank();
	if (aCount == 0 && !theRedeal)
		return;
	if (aCount % 2 != 0)
		return;
	if (theRedeal)
	{
		mLayout->CollectKinds(gKindCounts);
		int aFlowers = 0;
		int aSeasons = 0;
		for (int i = 0; i < 8; i++)
		{
			int aCountOf = gKindCounts[34 + i];
			gBonusCounts[i] = aCountOf;
			if (i <= 3)
				aSeasons = aSeasons + aCountOf;
			else
				aFlowers = aFlowers + aCountOf;
		}
		gKindCounts[34] = aSeasons;
		gKindCounts[35] = aFlowers;
	}
	else
		Distribute(aCount);
	if (mLayout->mHasCounts)
	{
		gHalvesCount = mLayout->GetHalves(gHalves, 400);
		gHalvesLeft = gHalvesCount;
		gSolveKinded = mLayout->CountKinded();
	}
	else
	{
		gHalvesCount = 0;
		gHalvesLeft = 0;
		gSolveKinded = 0;
	}
	gSolve2b = false;
	mLayout->ClearGroups();
	gDealCount = mLayout->CountLeft();
	gSolveTaken = 0;
	gSolveBlank = 0;
	gSolveQuads = 0;
	gSolveSteps = 0;
	gSolveStart = jb::RealTickCount();
	gSolveLimit = theRedeal ? 1000 : 2000;
	int aFree = mLayout->GetFree(gSolveFree[0], 400);
	if (0 < aFree)
	{
		Tile** aSlot = gSolveFree[0];
		int aRemaining = aFree;
		do
		{
			int aRandom = aFree == 0 ? 0 : jb::CrtRand() % aFree;
			Tile* aTile = *aSlot;
			*aSlot = gSolveFree[0][aRandom];
			aSlot = aSlot + 1;
			aRemaining = aRemaining - 1;
			gSolveFree[0][aRandom] = aTile;
		} while (aRemaining != 0);
	}
	if (Solve(0, aFree, 0))
		DealOrder();
	else
		DealAny();
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
			int* aBonusSlot = aBonus;
			do
			{
				if (aBonus + 8 <= aBonusSlot || *aBonusSlot < 1)
				{
					aDealt = aDealt + 1;
					gBonusCounts[i % 4] = gBonusCounts[i % 4] + 1;
				}
				else
					*aBonusSlot = *aBonusSlot - 1;
				i = i + 1;
				aBonusSlot = aBonusSlot + 1;
			} while (aDealt < aSeasonTotal);
		}
		int aFlowerTotal = gKindCounts[35];
		i = 0;
		aDealt = 0;
		if (0 < aFlowerTotal)
		{
			int* aBonusSlot = aBonus + 4;
			do
			{
				if (aBonus + 8 <= aBonusSlot || *aBonusSlot < 1)
				{
					aDealt = aDealt + 1;
					gBonusCounts[4 + i % 4] = gBonusCounts[4 + i % 4] + 1;
				}
				else
					*aBonusSlot = *aBonusSlot - 1;
				i = i + 1;
				aBonusSlot = aBonusSlot + 1;
			} while (aDealt < aFlowerTotal);
		}
	}
}

// 0x413aa0: the order's fours first, each four one kind while fours are left (a four uses two pairs' worth), then
// its pairs
void Deal::DealOrder()
{
	int aPairs = 0;
	int aQuads = 0;
	int* aCountSlot = gKindCounts;
	do
	{
		int aCount = *aCountSlot;
		aCountSlot = aCountSlot + 1;
		aQuads = aQuads + aCount / 4;
		aPairs = aPairs + aCount / 2;
	} while (aCountSlot < gKindCounts + 0x24);
	int i = 0;
	if (0 < gQuadTiles)
	{
		Tile** aTiles = gQuadOrder;
		do
		{
			if (0 < aQuads)
			{
				DealKind(aTiles, 4, &aQuads);
				i = i + 4;
				aTiles = aTiles + 4;
				aPairs = aPairs - 2;
			}
			else
			{
				DealKind(aTiles, 2, &aPairs);
				i = i + 2;
				aTiles = aTiles + 2;
			}
		} while (i < gQuadTiles);
	}
	i = 0;
	if (0 < gPairTiles)
	{
		Tile** aTiles = gPairOrder;
		do
		{
			DealKind(aTiles, 2, &aPairs);
			i = i + 2;
			aTiles = aTiles + 2;
		} while (i < gPairTiles);
	}
}

// 0x413b70
void DealKind(Tile** theTiles, int theSize, int* theLeft)
{
	int aLeft = *theLeft;
	int aPick = 0;
	if (aLeft != 0)
		aPick = jb::CrtRand() % aLeft;
	int aSeen = 0;
	int k = 0;
	while (gKindCounts[k] < theSize || (aSeen = aSeen + gKindCounts[k] / theSize, aSeen <= aPick))
	{
		k = k + 1;
		if (k == 0x24)
			k = 0;
	}
	int i = 0;
	if (0 < theSize)
	{
		do
		{
			if (k == 0x22)
				DealBonus(*theTiles, gKindCounts[34] - i, 0x23);
			else if (k == 0x23)
				DealBonus(*theTiles, gKindCounts[35] - i, 0x27);
			else
				(*theTiles)->mKind = k + 1;
			i = i + 1;
			theTiles = theTiles + 1;
		} while (i < theSize);
	}
	gKindCounts[k] = gKindCounts[k] - theSize;
	*theLeft = *theLeft - 1;
}

// 0x413c30
void Deal::DealAny()
{
	int aLeft = mLayout->CountBlank();
	Tile* aTile = mLayout->First();
	while (aTile != nullptr && 0 < aLeft)
	{
		if (aTile->mTaken == 0 && aTile->mKind == 0)
			DealOne(aTile, &aLeft);
		aTile = mLayout->Next();
	}
}

// 0x413c80
void DealOne(Tile* theTile, int* theLeft)
{
	int aLeft = *theLeft;
	int aPick;
	if (aLeft == 0)
		aPick = 0;
	else
		aPick = jb::CrtRand() % aLeft;
	int aSeen = 0;
	do
	{
		int k = 0;
		do
		{
			if (0 < gKindCounts[k] && (aSeen = aSeen + gKindCounts[k], aPick < aSeen))
			{
				if (k == 0x22)
					DealBonus(theTile, gKindCounts[34], 0x23);
				else if (k == 0x23)
					DealBonus(theTile, gKindCounts[35], 0x27);
				else
					theTile->mKind = k + 1;
				gKindCounts[k] = gKindCounts[k] - 1;
				*theLeft = *theLeft - 1;
				return;
			}
			k = k + 1;
		} while (k != 0x24);
	} while (true);
}

// 0x413d20
void DealBonus(Tile* theTile, int theRange, int theBase)
{
	int aPick;
	if (theRange == 0)
		aPick = 0;
	else
		aPick = jb::CrtRand() % theRange;
	int i = 0;
	int aFirst = theBase != 0x23 ? 4 : 0;
	int aSeen = 0;
	int* aCountSlot = gBonusCounts + aFirst;
	do
	{
		aSeen = aSeen + *aCountSlot;
		if (aPick < aSeen)
		{
			gBonusCounts[aFirst + i] = gBonusCounts[aFirst + i] - 1;
			theTile->mKind = i + theBase;
			return;
		}
		i = i + 1;
		aCountSlot = aCountSlot + 1;
	} while (i < 4);
}

// 0x413d90: done when every tile has gone but the halves (0x2d) left, or, once the 0x2b tile has gone, when only
// tiles with kinds are left. In a layout with kinds of its own a 0x2c pairs with any 0x2d on the board, 0x2e and
// 0x2f go alone and 0x2d only with a 0x2c.
bool Deal::Solve(int theDepth, int theCount, int theGroup)
{
	if (gHalvesLeft + gSolveTaken == gDealCount || (gSolve2b && gSolveKinded + gSolveTaken == gDealCount))
	{
		gQuadTiles = gSolveQuads * 4;
		gQuadNext = gQuadTiles - 1;
		gPairTiles = gSolveBlank - gQuadTiles;
		gPairNext = gPairTiles - 1;
		return true;
	}
	int i = 0;
	if (0 < theCount)
	{
		Tile** aSlot = gSolveFree[theDepth];
		do
		{
			Tile* aTile = *aSlot;
			if (!SolveInTime())
				return false;
			if (mLayout->mHasCounts)
			{
				int aKind = aTile->mKind;
				if (aKind == 0x2c)
				{
					if (0 < gHalvesLeft)
					{
						int k = gHalvesCount - 1;
						if (-1 < k)
						{
							Tile** aHalf = gHalves + k;
							do
							{
								if ((*aHalf)->mTaken == 0)
								{
									gHalvesLeft = gHalvesLeft - 1;
									if (Take(aTile, *aHalf, theCount, theDepth, theGroup))
										return true;
									gHalvesLeft = gHalvesLeft + 1;
								}
								k = k - 1;
								aHalf = aHalf - 1;
							} while (-1 < k);
						}
					}
					goto next;
				}
				if (aKind == 0x2d)
					goto next;
				if (aKind == 0x2e || aKind == 0x2f)
				{
					if (Take(aTile, nullptr, theCount, theDepth, theGroup))
						return true;
					goto next;
				}
			}
			{
				int j = i + 1;
				if (j < theCount)
				{
					Tile** anOtherSlot = aSlot + 1;
					do
					{
						Tile* anOther = *anOtherSlot;
						if (!SolveInTime())
							return false;
						if (KindsMatch(aTile->mKind, anOther->mKind) &&
							Take(aTile, anOther, theCount, theDepth, theGroup))
							return true;
						j = j + 1;
						anOtherSlot = anOtherSlot + 1;
					} while (j < theCount);
				}
			}
		next:
			aSlot = aSlot + 1;
			i = i + 1;
		} while (i < theCount);
	}
	return false;
}

// 0x413fa0
bool SolveInTime()
{
	if (gSolveSteps == -1)
		return false;
	bool aCheck = 1000 < gSolveSteps;
	gSolveSteps = gSolveSteps + 1;
	if (aCheck)
	{
		gSolveSteps = 0;
		if (gSolveLimit + gSolveStart < jb::RealTickCount())
		{
			gSolveSteps = -1;
			return false;
		}
	}
	return true;
}

// 0x413ff0: the next depth's free tiles are those still free and those the move frees; its first half is swapped
// with places among the old ones and a share (mQuadPercent) of the new
bool Deal::Take(Tile* theTile, Tile* theOther, int theCount, int theDepth, int theGroup)
{
	bool aSolved = false;
	mMove = mMove + 1;
	theTile->mTaken = mMove;
	gSolveTaken = gSolveTaken + 1;
	if (theTile->mKind == 0)
		gSolveBlank = gSolveBlank + 1;
	else
		gSolveKinded = gSolveKinded - 1;
	if (theTile->mKind == 0x2b)
		gSolve2b = true;
	if (theOther != nullptr)
	{
		theOther->mTaken = mMove;
		gSolveTaken = gSolveTaken + 1;
		if (theOther->mKind == 0)
			gSolveBlank = gSolveBlank + 1;
		else
			gSolveKinded = gSolveKinded - 1;
	}
	int aNext = 0;
	if (0 < theCount)
	{
		Tile** aTo = gSolveFree[theDepth + 1];
		Tile** aFrom = gSolveFree[theDepth];
		int aRemaining = theCount;
		do
		{
			Tile* aTile = *aFrom;
			if (aTile != theTile && aTile != theOther)
			{
				*aTo = aTile;
				aNext = aNext + 1;
				aTo = aTo + 1;
			}
			aFrom = aFrom + 1;
			aRemaining = aRemaining - 1;
		} while (aRemaining != 0);
	}
	int anOld = aNext;
	Tile** aFreed = gSolveFreed[theDepth];
	int aFreedCount = mLayout->UpdateNeighbours(theTile, theOther, aFreed, 0x14, true);
	if (0 < aFreedCount)
	{
		for (int k = 0; k < aFreedCount; k++)
			gSolveFree[theDepth + 1][anOld + k] = aFreed[k];
		aNext = anOld + aFreedCount;
	}
	int aHalf = anOld / 2;
	int aRange = anOld + (mQuadPercent * aFreedCount) / 100;
	if (0 < aHalf)
	{
		Tile** aSlot = gSolveFree[theDepth + 1];
		int aRemaining = aHalf;
		do
		{
			int aRandom;
			if (aRange == 0)
				aRandom = 0;
			else
				aRandom = jb::CrtRand() % aRange;
			Tile* aTile = *aSlot;
			aSlot = aSlot + 1;
			*(aSlot - 1) = gSolveFree[theDepth + 1][aRandom];
			gSolveFree[theDepth + 1][aRandom] = aTile;
			aRemaining = aRemaining - 1;
		} while (aRemaining != 0);
	}
	int aFlags = 0;
	Tile* aThird = nullptr;
	if (theTile->mKind == 0)
	{
		if (0 < aFreedCount)
		{
			aThird = aFreed[jb::CrtRand() % aFreedCount];
			if (aThird->mKind != 0)
				aThird = nullptr;
		}
		aFlags = Group(theTile, theOther, aThird, &theGroup);
	}
	else
	{
		theTile->mGroup = -1;
		theTile->mQuad = false;
		if (theOther != nullptr)
		{
			theOther->mGroup = -1;
			theOther->mQuad = false;
		}
	}
	if (Solve(theDepth + 1, aNext, theGroup))
	{
		if (theTile->mKind == 0)
			Record(theTile, theOther);
		aSolved = true;
	}
	mMove = mMove - 1;
	theTile->mTaken = 0;
	gSolveTaken = gSolveTaken - 1;
	if (theTile->mKind == 0)
		gSolveBlank = gSolveBlank - 1;
	else
		gSolveKinded = gSolveKinded + 1;
	if (theTile->mKind == 0x2b)
		gSolve2b = false;
	if (theOther != nullptr)
	{
		theOther->mTaken = 0;
		gSolveTaken = gSolveTaken - 1;
		if (theOther->mKind == 0)
			gSolveBlank = gSolveBlank - 1;
		else
			gSolveKinded = gSolveKinded + 1;
	}
	int aRemaining = aFreedCount;
	if (0 < aRemaining)
	{
		Tile** aSlot = aFreed;
		do
		{
			Tile* aTile = *aSlot;
			aSlot = aSlot + 1;
			aRemaining = aRemaining - 1;
			aTile->mFree = false;
		} while (aRemaining != 0);
	}
	if (0 < aFlags)
	{
		if ((aFlags & 1) != 0)
			theTile->mGroup = 0;
		if ((aFlags & 2) != 0)
			theOther->mGroup = 0;
		if ((aFlags & 4) != 0)
			aThird->mGroup = 0;
		if (theTile->mQuad)
		{
			theTile->mQuad = false;
			gSolveQuads = gSolveQuads - 1;
		}
	}
	return aSolved;
}

// 0x414310: a pair joins one already grouped or the group waiting, or with a third tile the move freed starts a
// four (at half of mQuadPercent); else it waits to be joined, unless mQuadPercent leaves it a pair
int Deal::Group(Tile* theTile, Tile* theOther, Tile* theThird, int* theGroup)
{
	int anId = gSolveTaken;
	int aFlags = 0;
	if (theTile->mKind != 0 || theOther == nullptr || theOther->mKind != 0)
		goto done;
	if (theTile->mGroup != 0)
	{
		theOther->mGroup = theTile->mGroup;
		theTile->mQuad = true;
		*theGroup = 0;
		aFlags = 2;
		goto done;
	}
	if (theOther->mGroup != 0)
	{
		theTile->mGroup = theOther->mGroup;
		aFlags = 1;
		theTile->mQuad = true;
		*theGroup = 0;
		goto done;
	}
	if (*theGroup == 0)
	{
		if (theThird != nullptr && jb::CrtRand() % 100 < mQuadPercent / 2)
		{
			theTile->mGroup = anId;
			theOther->mGroup = anId;
			theThird->mGroup = anId;
			*theGroup = 0;
			aFlags = 7;
			goto done;
		}
		{
			int aRandom = jb::CrtRand();
			theTile->mGroup = anId;
			theOther->mGroup = anId;
			if (aRandom % 100 < mQuadPercent)
				goto clear;
			*theGroup = anId;
		}
	}
	else
	{
		theTile->mGroup = *theGroup;
		theOther->mGroup = *theGroup;
		theTile->mQuad = true;
	clear:
		*theGroup = 0;
	}
	aFlags = 3;
done:
	if (theTile->mQuad)
		gSolveQuads = gSolveQuads + 1;
	return aFlags;
}

// 0x414420
void Deal::Record(Tile* theTile, Tile* theOther)
{
	int aGroup;
	if (theTile->mKind == 0 && (aGroup = theTile->mGroup, 0 < aGroup))
	{
		if (!theTile->mQuad)
		{
			gPairOrder[gPairNext] = theTile;
			gPairOrder[gPairNext - 1] = theOther;
			gPairNext = gPairNext - 2;
			return;
		}
		gQuadOrder[gQuadNext] = theTile;
		gQuadOrder[gQuadNext - 1] = theOther;
		gQuadNext = gQuadNext - 2;
		theTile->mGroup = 0;
		theOther->mGroup = 0;
		Tile* aPair[2];
		if (mLayout->FindGroup(aGroup, aPair, 2) == 2)
		{
			gQuadOrder[gQuadNext] = aPair[0];
			gQuadOrder[gQuadNext - 1] = aPair[1];
			gQuadNext = gQuadNext - 2;
			aPair[0]->mGroup = 0;
			aPair[1]->mGroup = 0;
		}
	}
}

} // namespace mahjongchina
