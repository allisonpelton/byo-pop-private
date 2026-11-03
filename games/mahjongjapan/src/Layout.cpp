#include "Layout.h"

#include "Deal.h"
#include "Host.h"

#include <vector>

namespace mahjongjapan
{

// 0x413430
void Tile::Serialize(jb::Stream& theStream)
{
	theStream.Char(mX);
	theStream.Char(mY);
	theStream.Char(mZ);
	theStream.Char(mKind);
	theStream.Short(mTaken);
}

// 0x413480
void Tile::SerializeLayout(jb::Stream& theStream)
{
	theStream.Char(mX);
	theStream.Char(mY);
	theStream.Char(mZ);
	theStream.Char(mKind);
}

// 0x4144e0
Layout::Layout()
{
}

// 0x414530
Layout::~Layout()
{
	Clear(true);
}

// 0x414590
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

// 0x4145b0
Tile* Layout::Next()
{
	jb::ListNode* aNode = mCursor;
	if (aNode == nullptr)
		return nullptr;
	Tile* aTile = (Tile*)aNode->mObj;
	mCursor = aNode->mNext;
	return aTile;
}

// 0x4145d0
void Layout::Reset(bool theKinds)
{
	for (jb::ListNode* aNode = mHead; aNode != nullptr; aNode = aNode->mNext)
	{
		Tile* aTile = (Tile*)aNode->mObj;
		aTile->mTaken = 0;
		aTile->m3c = 0;
		if (theKinds && !aTile->mPreset)
			aTile->mKind = 0;
	}
}

// 0x414610
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

// 0x4146a0
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

// 0x414700
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

// 0x414750
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

// 0x4147e0
void Layout::UpdateFree()
{
	for (jb::ListNode* aNode = mHead; aNode != nullptr; aNode = aNode->mNext)
	{
		Tile* aTile = (Tile*)aNode->mObj;
		if (aTile->mTaken == 0)
			aTile->mFree = IsFree(aTile->mX, aTile->mY, aTile->mZ);
	}
}

// 0x414820
void Layout::UpdateFree(Tile* theTile)
{
	theTile->mFree = IsFree(theTile->mX, theTile->mY, theTile->mZ);
}

// 0x414840
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

// 0x414880
int Layout::GetHalves(Tile** theTiles, int theMax)
{
	return GetMatching(0x2d, theTiles, theMax);
}

// 0x4148a0
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

// 0x414900
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

// 0x414930
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
			if (aKind == 0x2c || aKind == 0x2e || aKind == 0x2f)
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

// 0x414990
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

// 0x4149b0
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
		int n = theNum;
		do
		{
			aX[i] = theTiles[i]->mX;
			aY[i] = theTiles[i]->mY;
			int z = theTiles[i]->mZ;
			aZ[i] = z;
			if (aTop < z)
				aTop = z;
			i = i + 1;
			n = n - 1;
		} while (n != 0);
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
					int kz = aZ[k];
					if ((z == kz - 1 && aX[k] - 1 <= x && x <= aX[k] + 1 && aY[k] - 1 <= y && y <= aY[k] + 1) ||
						(z == kz && ((x == aX[k] - 2 && aY[k] - 1 <= y && y <= aY[k] + 1) ||
							(z == kz && x == aX[k] + 2 && aY[k] - 1 <= y && y <= aY[k] + 1))))
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

// 0x414be0
int Layout::UpdateNeighbours(Tile* theTile, Tile* theOther, Tile** theOut, int theMax, bool theFree)
{
	Tile* aTiles[2] = {theTile, theOther};
	return UpdateNeighbours(aTiles, (theOther != nullptr) + 1, theOut, theMax, theFree);
}

// 0x414c20
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

// 0x414d10: the seasons count together (at 0x23) and so do the flowers (at 0x27)
void Layout::CountMoves(int* thePairs, int* theLeft, int* theKinds)
{
	int aCounts[0x2c];
	jb::ListNode* aNode = mHead;
	*theKinds = 0;
	*theLeft = 0;
	*thePairs = 0;
	for (int i = 0; i < 0x2c; i++)
		aCounts[i] = 0;
	for (; aNode != nullptr; aNode = aNode->mNext)
	{
		Tile* aTile = (Tile*)aNode->mObj;
		if (aTile->mTaken == 0)
		{
			*theLeft = *theLeft + 1;
			int aKind;
			if (aTile->mFree && (aKind = aTile->mKind, aKind < 0x2c))
			{
				if (aKind < 0x23 || 0x26 < aKind)
				{
					if (aKind < 0x27 || 0x2a < aKind)
						aCounts[aKind] = aCounts[aKind] + 1;
					else
						aCounts[0x27] = aCounts[0x27] + 1;
				}
				else
					aCounts[0x23] = aCounts[0x23] + 1;
			}
		}
	}
	int* p = aCounts;
	int n = 0x2c;
	do
	{
		if (1 < *p)
		{
			*thePairs = *thePairs + *p / 2;
			*theKinds = *theKinds + 1;
		}
		p = p + 1;
		n = n - 1;
	} while (n != 0);
}

// 0x414df0
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

// 0x414e10
int Layout::CountFree2b()
{
	int aCount = 0;
	for (jb::ListNode* aNode = mHead; aNode != nullptr; aNode = aNode->mNext)
	{
		Tile* aTile = (Tile*)aNode->mObj;
		if (aTile->mTaken == 0 && aTile->mKind == 0x2b && aTile->mFree)
			aCount = aCount + 1;
	}
	return aCount;
}

// 0x414e40
bool Layout::HasFreeFlowers()
{
	jb::ListNode* aNode = mHead;
	int aCount = 0;
	while (true)
	{
		if (aNode == nullptr)
			return false;
		Tile* aTile = (Tile*)aNode->mObj;
		if (aTile->mTaken == 0 && 0x26 < aTile->mKind && aTile->mKind < 0x2b && aTile->mFree &&
			(aCount = aCount + 1, 1 < aCount))
			break;
		aNode = aNode->mNext;
	}
	return true;
}

// 0x414e80
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

// 0x414ec0
int Layout::GetPairsOf(int theIndex, Tile** theTiles, int theMax)
{
	int aCounts[0x2c];
	jb::ListNode* aHead = mHead;
	for (int i = 0; i < 0x2c; i++)
		aCounts[i] = 0;
	for (jb::ListNode* aNode = aHead; aNode != nullptr; aNode = aNode->mNext)
	{
		Tile* aTile = (Tile*)aNode->mObj;
		int aKind;
		if (aTile->mTaken == 0 && aTile->mFree && (aKind = aTile->mKind, aKind < 0x2c))
		{
			if (aKind < 0x23 || 0x26 < aKind)
			{
				if (0x26 < aKind && aKind < 0x2b)
					aKind = 0x27;
			}
			else
				aKind = 0x23;
			aCounts[aKind] = aCounts[aKind] + 1;
		}
	}
	int n = 0;
	int aKind = 0;
	int* p = aCounts;
	do
	{
		if (1 < *p)
		{
			if (theIndex == n)
			{
				if (aKind < 0x2c)
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
			n = n + 1;
		}
		aKind = aKind + 1;
		p = p + 1;
	} while (aKind < 0x2c);
	return 0;
}

// 0x414fd0
void Layout::ApplyWildLinks()
{
	for (jb::ListNode* aNode = mHead; aNode != nullptr; aNode = aNode->mNext)
	{
		Tile* aTile = (Tile*)aNode->mObj;
		Tile* aLink = aTile->mWildLink;
		if (aLink != nullptr && aTile->mTaken != 0)
		{
			aLink->mKind = 0x2f;
			aTile->mWildLink->mPreset = true;
			aTile->mWildLink = nullptr;
		}
	}
}

// 0x415010
void Layout::ClearGroups()
{
	for (jb::ListNode* aNode = mHead; aNode != nullptr; aNode = aNode->mNext)
	{
		((Tile*)aNode->mObj)->mGroup = 0;
		((Tile*)aNode->mObj)->mQuad = false;
	}
}

// 0x415030
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

// 0x415070
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

// 0x4150c0
int Layout::GetCounts(int* theCounts)
{
	int aCount = 0;
	for (int i = 0; i < 0x2a; i++)
		theCounts[i] = 0;
	for (jb::ListNode* aNode = mHead; aNode != nullptr; aNode = aNode->mNext)
	{
		Tile* aTile = (Tile*)aNode->mObj;
		int aKind;
		if (aTile->mPreset && (aKind = aTile->mKind, aKind < 0x2b))
		{
			aCount = aCount + 1;
			theCounts[aKind - 1] = theCounts[aKind - 1] + 1;
		}
	}
	return aCount;
}

// 0x415110: the LAYOUTS resource of the module the app keeps at +0x30 (Resource.dll)
bool Layout::Load(int theId)
{
	jb::Stream aStream;
	uint32_t aSize = 0;
	const uint8_t* aData = jb::FindResourceData(jb::Module::Dll, "LAYOUTS", theId & 0xffff, &aSize);
	if (aData == nullptr)
		return false;
	aStream.OpenReadMemory(aData, (int)aSize);
	return Load(aStream);
}

// 0x415210
bool Layout::Load(jb::Stream& theStream)
{
	Clear(true);
	SerializeLayout(theStream);
	if (mMagic == 0x4d4c && mVersion == 1 && theStream.ChecksumOk())
		return true;
	Clear(true);
	return false;
}

// 0x415260
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
			if (aKind < 0x23 || 0x26 < aKind)
			{
				if (0x26 < aKind && aKind < 0x2b)
					aKind = 0x27;
			}
			else
				aKind = 0x23;
			if (aKind < aLowest)
			{
				*theTile = aTile;
				aLowest = aKind;
			}
			else if (aKind == aLowest && aKind < 0x2b)
				*theOther = aTile;
		}
	}
}

// 0x4152d0
int Layout::TakeKinds()
{
	int aCount = 0;
	for (jb::ListNode* aNode = mHead; aNode != nullptr; aNode = aNode->mNext)
	{
		Tile* aTile = (Tile*)aNode->mObj;
		if (aTile->mTaken == 0 && aTile->mKind < 0x2b)
		{
			aCount = aCount + 1;
			aTile->mTaken = 1;
		}
	}
	return aCount;
}

// 0x415300
int Layout::TakeSpecials()
{
	int aCount = 0;
	for (jb::ListNode* aNode = mHead; aNode != nullptr; aNode = aNode->mNext)
	{
		Tile* aTile = (Tile*)aNode->mObj;
		if (aTile->mTaken == 0 && 0x2a < aTile->mKind)
		{
			aCount = aCount + 1;
			aTile->mTaken = 1;
		}
	}
	return aCount;
}

// 0x415330: a 40x50 mask of the tile's face, cleared where the tiles on the board after it cover it
bool Layout::IsVisible(Tile* theTile)
{
	if (theTile->mFree)
		return true;
	bool aCovered = false;
	char aMask[2000];
	int i = 0;
	do
	{
		char* p = aMask + i;
		int n = 0x28;
		do
		{
			*p = 1;
			p = p + 0x32;
			n = n - 1;
		} while (n != 0);
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
					int n = anOver.right - anOver.left;
					char* p = aMask + y + anOver.left * 0x32;
					do
					{
						*p = 0;
						p = p + 0x32;
						n = n - 1;
					} while (n != 0);
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
		char* p = aMask + i;
		do
		{
			if (*p != 0)
				return true;
			x = x + 1;
			p = p + 0x32;
		} while (x < 0x28);
		i = i + 1;
	} while (i <= 0x31);
	return false;
}

// 0x415490
void Layout::Serialize(jb::Stream& theStream)
{
	int aLinks = 0;
	if (!theStream.IsReading())
	{
		theStream.Short(mCount);
		for (jb::ListNode* aNode = mHead; aNode != nullptr; aNode = aNode->mNext)
		{
			theStream.Serialize((Tile*)aNode->mObj);
			if (((Tile*)aNode->mObj)->mWildLink != nullptr)
				aLinks = aLinks + 1;
		}
		theStream.Short(aLinks);
		if (0 < aLinks)
		{
			int anIndex = 0;
			for (jb::ListNode* aNode = mHead; aNode != nullptr; aNode = aNode->mNext)
			{
				if (((Tile*)aNode->mObj)->mWildLink != nullptr)
				{
					int aLink = IndexOf(((Tile*)aNode->mObj)->mWildLink);
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
		if (!aTile->mPreset || aTile->mKind == 0x2f)
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
			aTile->mWildLink = aLink;
			i = i + 1;
		} while (i < aLinks);
	}
}

// 0x415670
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

// 0x415770
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

// 0x415790
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

} // namespace mahjongjapan
