#include "Layout.h"

#include "Deal.h"
#include "Host.h"

#include <vector>

namespace mahjong
{

// 0x412eb0 (mahjongchina), 0x413430 (mahjongjapan)
void Tile::Serialize(jb::Stream& theStream)
{
	theStream.Char(mX);
	theStream.Char(mY);
	theStream.Char(mZ);
	theStream.Char(mKind);
	theStream.Short(mTaken);
}

// 0x412f00 (mahjongchina), 0x413480 (mahjongjapan)
void Tile::SerializeLayout(jb::Stream& theStream)
{
	theStream.Char(mX);
	theStream.Char(mY);
	theStream.Char(mZ);
	theStream.Char(mKind);
}

// 0x413f60 (mahjongchina), 0x4144e0 (mahjongjapan)
Layout::Layout()
{
}

// 0x413fb0 (mahjongchina), 0x414530 (mahjongjapan)
Layout::~Layout()
{
	Clear(true);
}

// 0x414010 (mahjongchina), 0x414590 (mahjongjapan)
Tile* Layout::First()
{
	jb::ListNode* aNode = mHead;
	if (aNode == nullptr)
	{
		mCursor = nullptr;
		return nullptr;
	}
	mCursor = aNode->mNext;
	return (Tile*)aNode->mObj;
}

// 0x414030 (mahjongchina), 0x4145b0 (mahjongjapan)
Tile* Layout::Next()
{
	jb::ListNode* aNode = mCursor;
	if (aNode == nullptr)
		return nullptr;
	Tile* aTile = (Tile*)aNode->mObj;
	mCursor = aNode->mNext;
	return aTile;
}

// 0x414050 (mahjongchina), 0x4145d0 (mahjongjapan)
void Layout::Reset(bool theKinds)
{
	for (jb::ListNode* aNode = mHead; aNode != nullptr; aNode = aNode->mNext)
	{
		Tile* aTile = (Tile*)aNode->mObj;
		aTile->mTaken = 0;
		aTile->mLiftStart = 0;
		if (theKinds && !aTile->mPreset)
			aTile->mKind = 0;
	}
}

// 0x414090 (mahjongchina), 0x414610 (mahjongjapan)
void Layout::AddTile(int theX, int theY, int theZ, int theKind)
{
	Tile* aTile = new Tile(theX, theY, theZ, theKind);
	if (theKind != 0)
	{
		aTile->mPreset = true;
		mHasCounts = true;
	}
	Append(aTile);
}

// 0x414120 (mahjongchina), 0x4146a0 (mahjongjapan)
void Layout::Append(Tile* theTile)
{
	jb::ListNode* aNode = new jb::ListNode;
	aNode->mNext = nullptr;
	aNode->mObj = theTile;
	aNode->mPrev = nullptr;
	mCount = mCount + 1;
	if (mHead == nullptr)
	{
		mTail = aNode;
		mHead = aNode;
		return;
	}
	mTail->mNext = aNode;
	aNode->mPrev = mTail;
	aNode->mNext = nullptr;
	mTail = aNode;
}

// 0x414180 (mahjongchina), 0x414700 (mahjongjapan)
void Layout::Clear(bool theDelete)
{
	jb::ListNode* aNode = mHead;
	while (aNode != nullptr)
	{
		if (theDelete && aNode->mObj != nullptr)
			delete (Tile*)aNode->mObj;
		jb::ListNode* aNext = aNode->mNext;
		delete aNode;
		aNode = aNext;
	}
	mCursor = nullptr;
	mTail = nullptr;
	mHead = nullptr;
	mCount = 0;
	mHasCounts = false;
	mName.clear();
}

// 0x4141d0 (mahjongchina), 0x414750 (mahjongjapan)
Tile* Layout::HitTest(int theX, int theY, int* theZ, int* theTileX, int* theTileY)
{
	*theTileX = 0;
	*theTileY = 0;
	jb::ListNode* aNode = mTail;
	while (true)
	{
		if (aNode == nullptr)
			return nullptr;
		Tile* aTile = (Tile*)aNode->mObj;
		if (aTile->mTaken == 0 && (*theZ == -1 || aTile->mZ == *theZ) && jb::PtInRect(aTile->mRect, theX, theY))
			break;
		aNode = aNode->mPrev;
	}
	Tile* aTile = (Tile*)aNode->mObj;
	*theTileX = aTile->mX;
	*theTileY = aTile->mY;
	*theZ = aTile->mZ;
	return aTile;
}

// 0x414260 (mahjongchina), 0x4147e0 (mahjongjapan)
void Layout::UpdateFree()
{
	for (jb::ListNode* aNode = mHead; aNode != nullptr; aNode = aNode->mNext)
	{
		Tile* aTile = (Tile*)aNode->mObj;
		if (aTile->mTaken == 0)
			aTile->mFree = IsFree(aTile->mX, aTile->mY, aTile->mZ);
	}
}

// 0x4142a0 (mahjongchina), 0x414820 (mahjongjapan)
void Layout::UpdateFree(Tile* theTile)
{
	theTile->mFree = IsFree(theTile->mX, theTile->mY, theTile->mZ);
}

// 0x4142c0 (mahjongchina), 0x414840 (mahjongjapan)
int Layout::GetFree(Tile** theTiles, int theMax)
{
	int aCount = 0;
	for (jb::ListNode* aNode = mHead; aNode != nullptr; aNode = aNode->mNext)
	{
		Tile* aTile = (Tile*)aNode->mObj;
		if (aTile->mTaken == 0 && aTile->mFree)
		{
			if (aCount < theMax)
				*theTiles = aTile;
			aCount = aCount + 1;
			theTiles = theTiles + 1;
		}
	}
	return aCount;
}

// 0x414300 (mahjongchina), 0x414880 (mahjongjapan)
int Layout::GetLocks(Tile** theTiles, int theMax)
{
	return GetMatching(KIND_LOCK, theTiles, theMax);
}

// 0x414320 (mahjongchina), 0x4148a0 (mahjongjapan)
int Layout::GetMatching(int theKind, Tile** theTiles, int theMax)
{
	jb::ListNode* aNode = mHead;
	int aCount = 0;
	if (aNode == nullptr)
		return 0;
	do
	{
		Tile* aTile = (Tile*)aNode->mObj;
		if (aTile->mTaken == 0 && KindsMatch(aTile->mKind, theKind))
		{
			if (aCount < theMax)
				*theTiles = aTile;
			aCount = aCount + 1;
			theTiles = theTiles + 1;
		}
		aNode = aNode->mNext;
	} while (aNode != nullptr);
	return aCount;
}

// 0x414380 (mahjongchina), 0x414900 (mahjongjapan)
int Layout::CountKinded()
{
	int aCount = 0;
	for (jb::ListNode* aNode = mHead; aNode != nullptr; aNode = aNode->mNext)
	{
		Tile* aTile = (Tile*)aNode->mObj;
		if (aTile->mTaken == 0 && aTile->mKind != 0)
			aCount = aCount + 1;
	}
	return aCount;
}

// 0x4143b0 (mahjongchina), 0x414930 (mahjongjapan)
int Layout::GetFreeSpecials(Tile** theTiles, int theMax)
{
	if (!mHasCounts)
		return 0;
	int aCount = 0;
	for (jb::ListNode* aNode = mHead; aNode != nullptr; aNode = aNode->mNext)
	{
		Tile* aTile = (Tile*)aNode->mObj;
		if (aTile->mTaken == 0 && aTile->mFree)
		{
			int aKind = aTile->mKind;
			if (aKind == KIND_KEY || aKind == KIND_JOKER || aKind == KIND_MAGNET)
			{
				if (aCount < theMax)
					*theTiles = aTile;
				aCount = aCount + 1;
				theTiles = theTiles + 1;
			}
		}
	}
	return aCount;
}

// 0x414410 (mahjongchina), 0x414990 (mahjongjapan)
int Layout::CountBlank()
{
	int aCount = 0;
	for (jb::ListNode* aNode = mHead; aNode != nullptr; aNode = aNode->mNext)
	{
		if (((Tile*)aNode->mObj)->mKind == 0)
			aCount = aCount + 1;
	}
	return aCount;
}

// 0x414430 (mahjongchina), 0x4149b0 (mahjongjapan)
int Layout::UpdateNeighbours(Tile** theTiles, int theNum, Tile** theOut, int theMax, bool theFree)
{
	jb::ListNode* aNode = mHead;
	int anOut = 0;
	int aTop = 0;
	std::vector<int> aX(theNum);
	std::vector<int> aY(theNum);
	std::vector<int> aZ(theNum);
	if (0 < theNum)
	{
		int i = 0;
		int aRemaining = theNum;
		do
		{
			aX[i] = theTiles[i]->mX;
			aY[i] = theTiles[i]->mY;
			int z = theTiles[i]->mZ;
			aZ[i] = z;
			if (aTop < z)
				aTop = z;
			i = i + 1;
			aRemaining = aRemaining - 1;
		} while (aRemaining != 0);
	}
	if (aNode != nullptr)
	{
		Tile** anOutAt = theOut;
		do
		{
			Tile* aTile = (Tile*)aNode->mObj;
			int x = aTile->mX;
			int z = aTile->mZ;
			int y = aTile->mY;
			if (aTop < z)
				break;
			if (aTile->mTaken == 0 && aTile->mFree != theFree && 0 < theNum)
			{
				int k = 0;
				do
				{
					int aGivenZ = aZ[k];
					if ((z == aGivenZ - 1 && aX[k] - 1 <= x && x <= aX[k] + 1 && aY[k] - 1 <= y && y <= aY[k] + 1) ||
						(z == aGivenZ && ((x == aX[k] - 2 && aY[k] - 1 <= y && y <= aY[k] + 1) ||
							(z == aGivenZ && x == aX[k] + 2 && aY[k] - 1 <= y && y <= aY[k] + 1))))
					{
						if (IsFree(x, y, z) == theFree)
						{
							((Tile*)aNode->mObj)->mFree = theFree;
							if (theOut != nullptr && anOut < theMax)
							{
								*anOutAt = (Tile*)aNode->mObj;
								anOut = anOut + 1;
								anOutAt = anOutAt + 1;
							}
						}
						break;
					}
					k = k + 1;
				} while (k < theNum);
			}
			aNode = aNode->mNext;
		} while (aNode != nullptr);
	}
	return anOut;
}

// 0x414660 (mahjongchina), 0x414be0 (mahjongjapan)
int Layout::UpdateNeighbours(Tile* theTile, Tile* theOther, Tile** theOut, int theMax, bool theFree)
{
	Tile* aTiles[2] = {theTile, theOther};
	return UpdateNeighbours(aTiles, (theOther != nullptr) + 1, theOut, theMax, theFree);
}

// 0x4146a0 (mahjongchina), 0x414c20 (mahjongjapan)
bool Layout::IsFree(int theX, int theY, int theZ)
{
	bool aRight = false;
	bool aLeft = false;
	for (jb::ListNode* aNode = mHead; aNode != nullptr; aNode = aNode->mNext)
	{
		Tile* aTile = (Tile*)aNode->mObj;
		int z = aTile->mZ;
		int x = aTile->mX;
		int y = aTile->mY;
		if (theZ + 1 < z)
			break;
		if (theZ <= z && aTile->mTaken == 0)
		{
			if (z == theZ + 1 && theX - 1 <= x && x <= theX + 1 && theY - 1 <= y && y <= theY + 1)
				return false;
			if (z == theZ)
			{
				if (!aLeft && x == theX - 2 && theY - 1 <= y && y <= theY + 1)
					aLeft = true;
				if (!aRight && x == theX + 2 && theY - 1 <= y && y <= theY + 1)
					aRight = true;
				if (aLeft && aRight)
					return false;
			}
		}
	}
	return true;
}

// 0x414790 (mahjongchina), 0x414d10 (mahjongjapan): the seasons count together (at 0x23) and so do the flowers (at
// 0x27)
void Layout::CountMoves(int* thePairs, int* theLeft, int* theKinds)
{
	int aCounts[NUM_COUNTED_KINDS];
	jb::ListNode* aNode = mHead;
	*theKinds = 0;
	*theLeft = 0;
	*thePairs = 0;
	for (int i = 0; i < NUM_COUNTED_KINDS; i++)
		aCounts[i] = 0;
	for (; aNode != nullptr; aNode = aNode->mNext)
	{
		Tile* aTile = (Tile*)aNode->mObj;
		if (aTile->mTaken == 0)
		{
			*theLeft = *theLeft + 1;
			int aKind;
			if (aTile->mFree && (aKind = aTile->mKind, aKind < NUM_COUNTED_KINDS))
			{
				if (aKind < KIND_FIRST_SEASON || KIND_LAST_SEASON < aKind)
				{
					if (aKind < KIND_FIRST_FLOWER || KIND_LAST_FLOWER < aKind)
						aCounts[aKind] = aCounts[aKind] + 1;
					else
						aCounts[KIND_FIRST_FLOWER] = aCounts[KIND_FIRST_FLOWER] + 1;
				}
				else
					aCounts[KIND_FIRST_SEASON] = aCounts[KIND_FIRST_SEASON] + 1;
			}
		}
	}
	int* aCountSlot = aCounts;
	int aRemaining = NUM_COUNTED_KINDS;
	do
	{
		if (1 < *aCountSlot)
		{
			*thePairs = *thePairs + *aCountSlot / 2;
			*theKinds = *theKinds + 1;
		}
		aCountSlot = aCountSlot + 1;
		aRemaining = aRemaining - 1;
	} while (aRemaining != 0);
}

// 0x414870 (mahjongchina), 0x414df0 (mahjongjapan)
int Layout::CountLeft()
{
	int aCount = 0;
	for (jb::ListNode* aNode = mHead; aNode != nullptr; aNode = aNode->mNext)
	{
		if (((Tile*)aNode->mObj)->mTaken == 0)
			aCount = aCount + 1;
	}
	return aCount;
}

// 0x414890 (mahjongchina), 0x414e10 (mahjongjapan)
int Layout::CountFree2b()
{
	int aCount = 0;
	for (jb::ListNode* aNode = mHead; aNode != nullptr; aNode = aNode->mNext)
	{
		Tile* aTile = (Tile*)aNode->mObj;
		if (aTile->mTaken == 0 && aTile->mKind == KIND_GOLD && aTile->mFree)
			aCount = aCount + 1;
	}
	return aCount;
}

// 0x4148c0 (mahjongchina), 0x414e40 (mahjongjapan)
bool Layout::HasFreeFlowers()
{
	jb::ListNode* aNode = mHead;
	int aCount = 0;
	while (true)
	{
		if (aNode == nullptr)
			return false;
		Tile* aTile = (Tile*)aNode->mObj;
		if (aTile->mTaken == 0 && KIND_LAST_SEASON < aTile->mKind && aTile->mKind < KIND_GOLD && aTile->mFree &&
			(aCount = aCount + 1, 1 < aCount))
			break;
		aNode = aNode->mNext;
	}
	return true;
}

// 0x414900 (mahjongchina), 0x414e80 (mahjongjapan)
int Layout::GetTaken(int theMove, Tile** theTiles, int theMax)
{
	int aCount = 0;
	for (jb::ListNode* aNode = mHead; aNode != nullptr; aNode = aNode->mNext)
	{
		if (((Tile*)aNode->mObj)->mTaken == theMove)
		{
			if (aCount < theMax)
				*theTiles = (Tile*)aNode->mObj;
			aCount = aCount + 1;
			theTiles = theTiles + 1;
		}
	}
	return aCount;
}

// 0x414940 (mahjongchina), 0x414ec0 (mahjongjapan)
int Layout::GetPairsOf(int theIndex, Tile** theTiles, int theMax)
{
	int aCounts[NUM_COUNTED_KINDS];
	jb::ListNode* aHead = mHead;
	for (int i = 0; i < NUM_COUNTED_KINDS; i++)
		aCounts[i] = 0;
	for (jb::ListNode* aNode = aHead; aNode != nullptr; aNode = aNode->mNext)
	{
		Tile* aTile = (Tile*)aNode->mObj;
		int aKind;
		if (aTile->mTaken == 0 && aTile->mFree && (aKind = aTile->mKind, aKind < NUM_COUNTED_KINDS))
		{
			if (aKind < KIND_FIRST_SEASON || KIND_LAST_SEASON < aKind)
			{
				if (KIND_LAST_SEASON < aKind && aKind < KIND_GOLD)
					aKind = KIND_FIRST_FLOWER;
			}
			else
				aKind = KIND_FIRST_SEASON;
			aCounts[aKind] = aCounts[aKind] + 1;
		}
	}
	int aSeen = 0;
	int aKind = 0;
	int* aCountSlot = aCounts;
	do
	{
		if (1 < *aCountSlot)
		{
			if (theIndex == aSeen)
			{
				if (aKind < NUM_COUNTED_KINDS)
				{
					int aCount = 0;
					for (jb::ListNode* aNode = aHead; aNode != nullptr; aNode = aNode->mNext)
					{
						Tile* aTile = (Tile*)aNode->mObj;
						if (aTile->mTaken == 0 && aTile->mFree && KindsMatch(aTile->mKind, aKind))
						{
							if (aCount < theMax)
								*theTiles = aTile;
							aCount = aCount + 1;
							theTiles = theTiles + 1;
						}
					}
					return aCount;
				}
				return 0;
			}
			aSeen = aSeen + 1;
		}
		aKind = aKind + 1;
		aCountSlot = aCountSlot + 1;
	} while (aKind < NUM_COUNTED_KINDS);
	return 0;
}

// 0x414a50 (mahjongchina), 0x414fd0 (mahjongjapan)
void Layout::ApplyMagnetLinks()
{
	for (jb::ListNode* aNode = mHead; aNode != nullptr; aNode = aNode->mNext)
	{
		Tile* aTile = (Tile*)aNode->mObj;
		Tile* aLink = aTile->mMagnetLink;
		if (aLink != nullptr && aTile->mTaken != 0)
		{
			aLink->mKind = KIND_MAGNET;
			aTile->mMagnetLink->mPreset = true;
			aTile->mMagnetLink = nullptr;
		}
	}
}

// 0x414a90 (mahjongchina), 0x415010 (mahjongjapan)
void Layout::ClearGroups()
{
	for (jb::ListNode* aNode = mHead; aNode != nullptr; aNode = aNode->mNext)
	{
		((Tile*)aNode->mObj)->mGroup = 0;
		((Tile*)aNode->mObj)->mQuad = false;
	}
}

// 0x414ab0 (mahjongchina), 0x415030 (mahjongjapan)
int Layout::FindGroup(int theGroup, Tile** theTiles, int theMax)
{
	int aCount = 0;
	for (jb::ListNode* aNode = mHead; aNode != nullptr; aNode = aNode->mNext)
	{
		if (((Tile*)aNode->mObj)->mGroup == theGroup)
		{
			if (aCount < theMax)
				*theTiles = (Tile*)aNode->mObj;
			aCount = aCount + 1;
			theTiles = theTiles + 1;
		}
	}
	return aCount;
}

// 0x414af0 (mahjongchina), 0x415070 (mahjongjapan)
int Layout::CollectKinds(int* theCounts)
{
	int aCount = 0;
	for (int i = 0; i < 0x2a; i++)
		theCounts[i] = 0;
	for (jb::ListNode* aNode = mHead; aNode != nullptr; aNode = aNode->mNext)
	{
		Tile* aTile = (Tile*)aNode->mObj;
		if (aTile->mTaken == 0 && !aTile->mPreset)
		{
			aCount = aCount + 1;
			theCounts[aTile->mKind - 1] = theCounts[aTile->mKind - 1] + 1;
			((Tile*)aNode->mObj)->mKind = 0;
		}
	}
	return aCount;
}

// 0x414b40 (mahjongchina), 0x4150c0 (mahjongjapan)
int Layout::GetCounts(int* theCounts)
{
	int aCount = 0;
	for (int i = 0; i < 0x2a; i++)
		theCounts[i] = 0;
	for (jb::ListNode* aNode = mHead; aNode != nullptr; aNode = aNode->mNext)
	{
		Tile* aTile = (Tile*)aNode->mObj;
		int aKind;
		if (aTile->mPreset && (aKind = aTile->mKind, aKind < KIND_GOLD))
		{
			aCount = aCount + 1;
			theCounts[aKind - 1] = theCounts[aKind - 1] + 1;
		}
	}
	return aCount;
}

// 0x414b90 (mahjongchina), 0x415110 (mahjongjapan): the LAYOUTS resource of the module the app keeps at +0x30
// (Resource.dll)
// 0x424960 (mahjongjapan) inlined here: the getter of that module (jb::Module::DLL)
bool Layout::Load(int theId)
{
	jb::Stream aStream;
	uint32_t aSize = 0;
	const uint8_t* aData = jb::FindResourceData(jb::Module::DLL, "LAYOUTS", theId & 0xffff, &aSize);
	if (aData == nullptr)
		return false;
	aStream.OpenReadMemory(aData, (int)aSize);
	return Load(aStream);
}

// 0x414c90 (mahjongchina), 0x415210 (mahjongjapan)
bool Layout::Load(jb::Stream& theStream)
{
	Clear(true);
	SerializeLayout(theStream);
	if (mMagic == 0x4d4c && mVersion == 1 && theStream.ChecksumOk())
		return true;
	Clear(true);
	return false;
}

// 0x414ce0 (mahjongchina), 0x415260 (mahjongjapan)
void Layout::GetLowest(Tile** theTile, Tile** theOther)
{
	int aLowest = 0xff;
	*theTile = nullptr;
	*theOther = nullptr;
	for (jb::ListNode* aNode = mHead; aNode != nullptr; aNode = aNode->mNext)
	{
		Tile* aTile = (Tile*)aNode->mObj;
		if (aTile->mTaken == 0)
		{
			int aKind = aTile->mKind;
			if (aKind < KIND_FIRST_SEASON || KIND_LAST_SEASON < aKind)
			{
				if (KIND_LAST_SEASON < aKind && aKind < KIND_GOLD)
					aKind = KIND_FIRST_FLOWER;
			}
			else
				aKind = KIND_FIRST_SEASON;
			if (aKind < aLowest)
			{
				*theTile = aTile;
				aLowest = aKind;
			}
			else if (aKind == aLowest && aKind < KIND_GOLD)
				*theOther = aTile;
		}
	}
}

// 0x414d50 (mahjongchina), 0x4152d0 (mahjongjapan)
int Layout::TakeKinds()
{
	int aCount = 0;
	for (jb::ListNode* aNode = mHead; aNode != nullptr; aNode = aNode->mNext)
	{
		Tile* aTile = (Tile*)aNode->mObj;
		if (aTile->mTaken == 0 && aTile->mKind < KIND_GOLD)
		{
			aCount = aCount + 1;
			aTile->mTaken = 1;
		}
	}
	return aCount;
}

// 0x414d80 (mahjongchina), 0x415300 (mahjongjapan)
int Layout::TakeSpecials()
{
	int aCount = 0;
	for (jb::ListNode* aNode = mHead; aNode != nullptr; aNode = aNode->mNext)
	{
		Tile* aTile = (Tile*)aNode->mObj;
		if (aTile->mTaken == 0 && KIND_LAST_FLOWER < aTile->mKind)
		{
			aCount = aCount + 1;
			aTile->mTaken = 1;
		}
	}
	return aCount;
}

// 0x414db0 (mahjongchina), 0x415330 (mahjongjapan): a 40x50 mask of the tile's face, cleared where the tiles on the
// board after it cover it
bool Layout::IsVisible(Tile* theTile)
{
	if (theTile->mFree)
		return true;
	bool aCovered = false;
	char aMask[2000];
	int i = 0;
	do
	{
		char* aCell = aMask + i;
		int aRemaining = 0x28;
		do
		{
			*aCell = 1;
			aCell = aCell + 0x32;
			aRemaining = aRemaining - 1;
		} while (aRemaining != 0);
		i = i + 1;
	} while (i < 0x32);
	jb::ListNode* aNode = mHead;
	jb::Rect aFace;
	aFace.left = theTile->mRect.left + 7;
	aFace.top = theTile->mRect.top + 1;
	aFace.right = theTile->mRect.left + 0x2f;
	aFace.bottom = theTile->mRect.top + 0x33;
	bool anAfter = false;
	if (aNode == nullptr)
		return true;
	do
	{
		Tile* aTile = (Tile*)aNode->mObj;
		jb::Rect anOver;
		if (aTile == theTile)
			anAfter = true;
		else if (anAfter && aTile->mTaken == 0 && jb::IntersectRect(anOver, aFace, aTile->mRect))
		{
			jb::OffsetRect(anOver, -aFace.left, -aFace.top);
			for (int y = anOver.top; y < anOver.bottom; y = y + 1)
			{
				if (anOver.left < anOver.right)
				{
					int aRemaining = anOver.right - anOver.left;
					char* aCell = aMask + y + anOver.left * 0x32;
					do
					{
						*aCell = 0;
						aCell = aCell + 0x32;
						aRemaining = aRemaining - 1;
					} while (aRemaining != 0);
				}
			}
			aCovered = true;
		}
		aNode = aNode->mNext;
	} while (aNode != nullptr);
	if (!aCovered)
		return true;
	i = 0;
	do
	{
		int x = 0;
		char* aCell = aMask + i;
		do
		{
			if (*aCell != 0)
				return true;
			x = x + 1;
			aCell = aCell + 0x32;
		} while (x < 0x28);
		i = i + 1;
	} while (i <= 0x31);
	return false;
}

// 0x414f10 (mahjongchina), 0x415490 (mahjongjapan)
void Layout::Serialize(jb::Stream& theStream)
{
	int aLinks = 0;
	if (!theStream.IsReading())
	{
		theStream.Short(mCount);
		for (jb::ListNode* aNode = mHead; aNode != nullptr; aNode = aNode->mNext)
		{
			theStream.Serialize((Tile*)aNode->mObj);
			if (((Tile*)aNode->mObj)->mMagnetLink != nullptr)
				aLinks = aLinks + 1;
		}
		theStream.Short(aLinks);
		if (0 < aLinks)
		{
			int anIndex = 0;
			for (jb::ListNode* aNode = mHead; aNode != nullptr; aNode = aNode->mNext)
			{
				if (((Tile*)aNode->mObj)->mMagnetLink != nullptr)
				{
					int aLink = IndexOf(((Tile*)aNode->mObj)->mMagnetLink);
					theStream.Short(anIndex);
					theStream.Short(aLink);
				}
				anIndex = anIndex + 1;
			}
		}
		return;
	}
	Tile aRead;
	int aCount = 0;
	theStream.Short(aCount);
	if (aCount != mCount)
		return;
	for (jb::ListNode* aNode = mHead; aNode != nullptr; aNode = aNode->mNext)
	{
		theStream.Serialize(&aRead);
		Tile* aTile = (Tile*)aNode->mObj;
		if (aRead.mX != aTile->mX || aRead.mY != aTile->mY || aRead.mZ != aTile->mZ)
			return;
		if (!aTile->mPreset || aTile->mKind == KIND_MAGNET)
			aTile->mKind = aRead.mKind;
		((Tile*)aNode->mObj)->mTaken = aRead.mTaken;
	}
	int i = 0;
	int aTileIndex = 0;
	int aLinkIndex = 0;
	theStream.Short(aLinks);
	if (0 < aLinks)
	{
		do
		{
			theStream.Short(aTileIndex);
			theStream.Short(aLinkIndex);
			Tile* aTile = At(aTileIndex);
			Tile* aLink;
			if (aTile == nullptr || (aLink = At(aLinkIndex)) == nullptr)
				break;
			aTile->mMagnetLink = aLink;
			i = i + 1;
		} while (i < aLinks);
	}
}

// 0x4150f0 (mahjongchina), 0x415670 (mahjongjapan)
void Layout::SerializeLayout(jb::Stream& theStream)
{
	mMagic = 0x4d4c;
	mVersion = 1;
	theStream.Short(mMagic);
	theStream.Char(mVersion);
	theStream.String(mName);
	if (!theStream.IsReading())
	{
		theStream.Short(mCount);
		for (jb::ListNode* aNode = mHead; aNode != nullptr; aNode = aNode->mNext)
			((Tile*)aNode->mObj)->SerializeLayout(theStream);
	}
	else
	{
		Tile aRead;
		int aCount = 0;
		theStream.Short(aCount);
		int i = 0;
		if (0 < aCount)
		{
			do
			{
				aRead.SerializeLayout(theStream);
				AddTile(aRead.mX, aRead.mY, aRead.mZ, aRead.mKind);
				i = i + 1;
			} while (i < aCount);
		}
	}
	theStream.Checksum();
}

// 0x4151f0 (mahjongchina), 0x415770 (mahjongjapan)
int Layout::IndexOf(Tile* theTile)
{
	jb::ListNode* aNode = mHead;
	int anIndex = 0;
	while (true)
	{
		if (aNode == nullptr)
			return 0;
		if (aNode->mObj == theTile)
			break;
		aNode = aNode->mNext;
		anIndex = anIndex + 1;
	}
	return anIndex;
}

// 0x415210 (mahjongchina), 0x415790 (mahjongjapan)
Tile* Layout::At(int theIndex)
{
	jb::ListNode* aNode = mHead;
	int anIndex = 0;
	if (aNode != nullptr)
	{
		do
		{
			if (anIndex == theIndex)
				return (Tile*)aNode->mObj;
			aNode = aNode->mNext;
			anIndex = anIndex + 1;
		} while (aNode != nullptr);
	}
	return nullptr;
}

} // namespace mahjong
