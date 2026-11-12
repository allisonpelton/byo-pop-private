// The board and its tiles (see Board.h).

#include "Board.h"
#include "HFileSystem.h"
#include "NingPo.h"

#include <cstdio>
#include <cstring>
#include <vector>

namespace ningpo
{

// 0x462398: the tile set, {type, face, count}: the 34 suits and honours four times each (the faces in the sheet's
// rows of 13 leave out the last columns), then the flowers and the seasons, one of each, matching any of their kind
struct TileKind
{
	int mType;
	int mFace;
	int mCount;
};

static const TileKind TILE_KINDS[] =
{
	{0, 0, 4}, {1, 1, 4}, {2, 2, 4}, {3, 3, 4}, {4, 4, 4}, {5, 5, 4}, {6, 6, 4}, {7, 7, 4}, {8, 8, 4},
	{9, 13, 4}, {10, 14, 4}, {11, 15, 4}, {12, 16, 4}, {13, 17, 4}, {14, 18, 4}, {15, 19, 4}, {16, 20, 4}, {17, 21, 4},
	{18, 26, 4}, {19, 27, 4}, {20, 28, 4}, {21, 29, 4}, {22, 30, 4}, {23, 31, 4}, {24, 32, 4}, {25, 33, 4}, {26, 34, 4},
	{27, 39, 4}, {28, 40, 4}, {29, 41, 4}, {30, 42, 4}, {31, 43, 4}, {32, 44, 4}, {33, 45, 4},
	{34, 35, 1}, {34, 36, 1}, {34, 37, 1}, {34, 38, 1},
	{35, 48, 1}, {35, 49, 1}, {35, 50, 1}, {35, 51, 1},
};

static const int NUM_TILE_KINDS = (int)(sizeof(TILE_KINDS) / sizeof(TILE_KINDS[0]));

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Tile

// 0x40c740
Tile::Tile()
{
	mActive = false;
	mSelected = false;
	mType = -1;
	mX = 0;
	mY = 0;
	mZ = 0;
	mFace = 0;
	mRemovedTime = 0;
}

// 0x40c760
void Tile::getFaceRect(PGRect* theRect) const
{
	theRect->left = (mX * 0x21) / 2 + 0x83;
	theRect->top = (mY * 0x30) / 2 + 0x1e;
	theRect->left = theRect->left + mZ * 6;
	int aLeft = theRect->left;
	theRect->top = theRect->top + mZ * -7;
	theRect->left = aLeft + 6;
	theRect->right = aLeft + 0x27;
	theRect->bottom = theRect->top + 0x30;
}

// 0x40c7d0
void Tile::getRect(PGRect* theRect) const
{
	theRect->left = (mX * 0x21) / 2 + 0x83;
	theRect->top = (mY * 0x30) / 2 + 0x1e;
	theRect->left = theRect->left + mZ * 6;
	theRect->top = theRect->top + mZ * -7;
	theRect->right = theRect->left + 0x27;
	theRect->bottom = theRect->top + 0x37;
}

// 0x40c840
void Tile::getDirtyRect(PGRect* theRect) const
{
	getRect(theRect);
	theRect->top = theRect->top - GetNingPo()->getSprite(SPRITE_SHADOW_TOP)->height();
	theRect->right = theRect->right + GetNingPo()->getSprite(SPRITE_SHADOW_RIGHT)->width();
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Board

// 0x405e50
Board::Board()
{
	// port: the original is a global, zeroed before its constructor runs
	memset(mDrawOrder, 0, sizeof(mDrawOrder));
	mHover = nullptr;
	mSelected = nullptr;
	mTilesLeft = 0;
	mMoves = 0;
	for (int i = 0; i < NUM_UNDOS; i++)
	{
		mUndo[i][1] = nullptr;
		mUndo[i][0] = nullptr;
	}
	deselect();
	clearUndo();
}

// 0x405eb0
int Board::countFree()
{
	int aCount = 0;
	Tile* aTile = mTiles;
	int i = NUM_TILES;
	do
	{
		if (aTile->mActive)
		{
			if (isFree(aTile))
				aCount++;
		}
		aTile++;
		i--;
	} while (i != 0);
	return aCount;
}

// 0x405ef0
int Board::countMoves()
{
	char aSeen[36];
	memset(aSeen, 0, sizeof(aSeen));
	int aMoves = 0;
	Tile* aTile = mTiles;
	int aLeft = NUM_TILES;
	do
	{
		int i = 0;
		if (aTile->mActive && aSeen[aTile->mType] == 0)
		{
			if (isFree(aTile))
			{
				Tile* aMatches[3] = {nullptr, nullptr, nullptr};
				int aCount = getMatches(aTile, aMatches);
				int aFree = 0;
				if (0 < aCount)
				{
					do
					{
						if (isFree(aMatches[i]))
							aFree++;
						i++;
					} while (i < aCount);
				}
				aMoves = aMoves + (aFree + 1) / 2;
				aSeen[aTile->mType] = 1;
			}
		}
		aTile++;
		aLeft--;
	} while (aLeft != 0);
	return aMoves;
}

// 0x405fc0
void Board::draw(const PGRect* theClip, Tile* theHint0, Tile* theHint1, Tile* theHint2, Tile* theHint3,
	float thePulse, float theFade)
{
	NingPo* anApp = GetNingPo();
	Tile** anEntry = mDrawOrder;
	int i = NUM_TILES;
	do
	{
		Tile* aTile = *anEntry;
		if (aTile != nullptr)
		{
			if (!aTile->mActive)
			{
				// a removed tile fades out over 500 ms
				int64_t anElapsed = pg::PyTimeSub(anApp->mEventTime, aTile->mRemovedTime);
				if (anElapsed < 500)
				{
					int64_t anElapsed2 = pg::PyTimeSub(anApp->mEventTime, (*anEntry)->mRemovedTime);
					float aFade = 1.0f - (float)anElapsed2 * 0.002f;
					anApp->drawTile(*anEntry, theClip, aFade, aFade, 1.0f - aFade);
				}
			}
			else
			{
				if (!aTile->mSelected)
				{
					if (aTile == theHint0 || aTile == theHint1 || aTile == theHint2 || aTile == theHint3)
					{
						anApp->drawTile(aTile, theClip, 0.0f, 1.0f, 0.0f);
						anApp->drawTile(*anEntry, theClip, 1.0f, thePulse, 1.0f);
					}
					else
					{
						anApp->drawTile(aTile, theClip, 1.0f, 1.0f, 0.0f);
						if (isFree(*anEntry) && 0.0f < theFade)
							anApp->drawTile(*anEntry, theClip, 0.0f, 1.0f, theFade);
					}
				}
				else
				{
					anApp->drawTile(aTile, theClip, 0.0f, 1.0f, 0.0f);
					anApp->drawTile(*anEntry, theClip, 1.0f, 1.0f, 1.0f);
				}
				if (mHover == *anEntry)
				{
					if (!isFree(mHover))
						anApp->drawOutline(mHover, theClip, false);
					else
						anApp->drawOutline(mHover, theClip, true);
				}
			}
		}
		anEntry++;
		i--;
	} while (i != 0);
}

// 0x4061f0: where tiles overlap, the higher one wins, but a lower one's face shows through the higher one's edges
Tile* Board::tileAt(const PGPoint* thePoint, bool theActiveOnly)
{
	Tile* aBest = nullptr;
	Tile* aTile = mTiles;
	int i = NUM_TILES;
	do
	{
		if (aTile->mActive || !theActiveOnly)
		{
			PGRect aRect;
			PGRect aFace;
			aTile->getRect(&aRect);
			aTile->getFaceRect(&aFace);
			aFace.right = aFace.right - 1;
			aFace.bottom = aFace.bottom - 1;
			if (aRect.left <= thePoint->x && thePoint->x <= aRect.right && aRect.top <= thePoint->y &&
				thePoint->y <= aRect.bottom)
			{
				if (aBest != nullptr && aTile->mZ <= aBest->mZ)
				{
					PGRect aBestFace;
					aBest->getFaceRect(&aBestFace);
					aBestFace.bottom = aBestFace.bottom - 1;
					int x = thePoint->x;
					aBestFace.right = aBestFace.right - 1;
					if ((aBestFace.left <= x && x <= aBestFace.right && aBestFace.top <= thePoint->y &&
							thePoint->y <= aBestFace.bottom) ||
						(x < aFace.left || aFace.right < x || thePoint->y < aFace.top || aFace.bottom < thePoint->y))
						goto next;
				}
				aBest = aTile;
			}
		}
	next:
		aTile++;
		i--;
		if (i == 0)
			return aBest;
	} while (true);
}

// 0x4062f0
Tile* Board::tileAtCell(int theX, int theY, int theZ, bool theActiveOnly)
{
	int i = 0;
	Tile* aTile = mTiles;
	while (aTile->mX != theX || aTile->mY != theY || aTile->mZ != theZ || (theActiveOnly && !aTile->mActive))
	{
		i++;
		aTile++;
		if (0x8f < i)
			return nullptr;
	}
	return &mTiles[i];
}

// 0x406350: the tiles one level up, in the nine cells around, cover the tile's quarters (1 top left, 2 top right,
// 4 bottom right, 8 bottom left)
bool Board::isVisible(Tile* theTile)
{
	int y = theTile->mY;
	int x = theTile->mX;
	int z = theTile->mZ + 1;
	if (tileAtCell(x, y, z, true) != nullptr)
		return false;
	int aCovered = 0;
	if (tileAtCell(x - 1, y - 1, z, true) != nullptr)
		aCovered = 1;
	if (tileAtCell(x, y - 1, z, true) != nullptr)
		aCovered |= 3;
	if (tileAtCell(x + 1, y - 1, z, true) != nullptr)
		aCovered |= 2;
	if (tileAtCell(x - 1, y, z, true) != nullptr)
		aCovered |= 9;
	if (tileAtCell(x, y, z, true) != nullptr)
		aCovered |= 0xf;
	if (tileAtCell(x + 1, y, z, true) != nullptr)
		aCovered |= 6;
	y++;
	if (tileAtCell(x - 1, y, z, true) != nullptr)
		aCovered |= 8;
	if (tileAtCell(x, y, z, true) != nullptr)
		aCovered |= 0xc;
	if (tileAtCell(x + 1, y, z, true) != nullptr)
		aCovered |= 4;
	return aCovered != 0xf;
}

// 0x4064a0
bool Board::isFree(Tile* theTile)
{
	int x = theTile->mX;
	int z = theTile->mZ;
	int y = theTile->mY;
	int i = -1;
	int aY = y - 1;
	do
	{
		if (tileAtCell(x - 1, aY, z + 1, true) != nullptr || tileAtCell(x, aY, z + 1, true) != nullptr ||
			tileAtCell(x + 1, aY, z + 1, true) != nullptr)
			return false;
		i++;
		aY++;
	} while (i <= 1);
	int aLeft = 0;
	int aRight = 0;
	i = -1;
	do
	{
		if (tileAtCell(x - 2, y + i, z, true) != nullptr)
			aLeft++;
		i++;
	} while (i <= 1);
	if (aLeft == 0)
		return true;
	i = -1;
	do
	{
		if (tileAtCell(x + 2, y + i, z, true) != nullptr)
			aRight++;
		i++;
	} while (i <= 1);
	return aRight == 0;
}

// 0x4065a0
const char* ParseLayoutLine(const char* theLine, int* theCell)
{
	char aChar = *theLine;
	while (aChar != '\0')
	{
		if (aChar == '\n')
			return theLine;
		if (aChar != ' ')
			break;
		aChar = *++theLine;
	}
	if (*theLine != '\n' && *theLine != '\0')
	{
		sscanf(theLine, "%d,%d,%d\n", &theCell[0], &theCell[1], &theCell[2]);
		aChar = *theLine;
		while (aChar != '\0' && aChar != '\n')
			aChar = *++theLine;
		if (*theLine == '\n')
			theLine++;
	}
	return theLine;
}

// 0x406600
bool Board::loadLayout(const char* theName, int* theCells, unsigned int theSize)
{
	std::vector<uint8_t> aFile;
	if (!GetNingPo()->mFileSystem->readFile(theName, aFile))
		return false;
	if (0x2000 < (int)aFile.size())
		return false;
	// port: the original reads the file into 0x2000 bytes of its stack, unterminated; here they are zeroed
	char aBuf[0x2001];
	memset(aBuf, 0, sizeof(aBuf));
	if (!aFile.empty())
		memcpy(aBuf, aFile.data(), aFile.size());
	int aCell[3] = {0, 0, 0};
	const char* p = aBuf;
	unsigned int aCount = theSize / 0xc;
	unsigned int i = 0;
	do
	{
		p = ParseLayoutLine(p, aCell);
		i++;
		theCells[0] = aCell[0];
		theCells[1] = aCell[1];
		theCells[2] = aCell[2];
		theCells += 3;
	} while (i < aCount);
	return true;
}

// 0x4066d0
void Board::newGame(const char* theLayout)
{
	int aCells[432];
	// port: zeroed, for a layout that is missing (the original kept what was on its stack)
	memset(aCells, 0, sizeof(aCells));
	loadLayout(theLayout, aCells, sizeof(aCells));
	deselect();
	memset(mDrawOrder, 0, sizeof(mDrawOrder));
	// the original zeroes the tiles' 0x1200 bytes
	for (int i = 0; i < NUM_TILES; i++)
	{
		mTiles[i].mActive = false;
		mTiles[i].mSelected = false;
		mTiles[i].mType = 0;
		mTiles[i].mX = 0;
		mTiles[i].mY = 0;
		mTiles[i].mZ = 0;
		mTiles[i].mFace = 0;
		mTiles[i].mRemovedTime = 0;
	}
	const TileKind* aKind = TILE_KINDS;
	do
	{
		int aCount = aKind->mCount;
		if (0 < aCount)
		{
			do
			{
				int aSlot;
				do
				{
					aSlot = GetNingPo()->mRandom.next(0x8f);
				} while (mTiles[aSlot].mActive);
				Tile* aTile = &mTiles[aSlot];
				aTile->mActive = true;
				aTile->mType = aKind->mType;
				aTile->mFace = aKind->mFace;
				aTile->mX = aCells[aSlot * 3];
				aTile->mY = aCells[aSlot * 3 + 1];
				mDrawOrder[aSlot] = aTile;
				aCount--;
				aTile->mZ = aCells[aSlot * 3 + 2];
			} while (aCount != 0);
		}
		aKind++;
	} while (aKind < TILE_KINDS + NUM_TILE_KINDS);
	sortDrawOrder();
	mTilesLeft = NUM_TILES;
	mMoves = countMoves();
	clearUndo();
}

// 0x4067f0: a bubble sort
void Board::sortDrawOrder()
{
	int aSwaps;
	do
	{
		aSwaps = 0;
		int i = 0x8f;
		Tile** anEntry = &mDrawOrder[1];
		do
		{
			Tile* aPrev = anEntry[-1];
			if (0 < CompareDrawOrder(anEntry - 1, anEntry))
			{
				anEntry[-1] = *anEntry;
				*anEntry = aPrev;
				aSwaps++;
			}
			anEntry++;
			i--;
		} while (i != 0);
	} while (aSwaps != 0);
}

// 0x406850: the lower level first; on a level, from the right to the left, and from the top down between tiles in
// the same column
int CompareDrawOrder(Tile** a, Tile** b)
{
	Tile* aTile = *a;
	Tile* anOther = *b;
	if (aTile->mZ < anOther->mZ)
		return -1;
	if (anOther->mZ < aTile->mZ)
		return 1;
	int dx = aTile->mX - anOther->mX;
	if (1 < (dx < 0 ? -dx : dx))
		return anOther->mX - aTile->mX;
	if (aTile->mY < anOther->mY)
		return -1;
	return anOther->mY < aTile->mY ? 1 : 0;
}

// 0x4068c0
int Board::getMatches(Tile* theTile, Tile** theMatches)
{
	int aCount = 0;
	theMatches[0] = nullptr;
	Tile* aTile = mTiles;
	int i = NUM_TILES;
	do
	{
		if (aTile != theTile && aTile->mActive && aTile->mType == theTile->mType)
		{
			theMatches[aCount] = aTile;
			aCount++;
		}
		aTile++;
		i--;
	} while (i != 0);
	return aCount;
}

// 0x406910
Tile* Board::nextFreeMatch(Tile* theTile, int* theIndex, int theStart)
{
	int k = 0;
	int i;
	while (true)
	{
		i = (theStart + k) % NUM_TILES;
		Tile* aTile = &mTiles[i];
		if (aTile != theTile && aTile->mActive && isFree(aTile) && mTiles[i].mType == theTile->mType)
			break;
		k++;
		if (0x8f < k)
		{
			*theIndex = -1;
			return nullptr;
		}
	}
	*theIndex = i;
	return &mTiles[i];
}

// 0x4069a0
bool Board::findMove(Tile** theTile, Tile** theOther, int theNth)
{
	int aMoves = countMoves();
	if (aMoves != 0)
	{
		int i = 0;
		char aSeen[NUM_TILES];
		memset(aSeen, 0, sizeof(aSeen));
		Tile* aTile = mTiles;
		int aNth = theNth;
		do
		{
			if (aTile->mActive)
			{
				if (isFree(aTile))
				{
					int anOtherIndex;
					Tile* anOther = nextFreeMatch(aTile, &anOtherIndex, i);
					if (anOther != nullptr && (aSeen[i] = 1, aSeen[anOtherIndex] == 0))
					{
						if (aNth % aMoves == 0)
						{
							if (theTile != nullptr)
								*theTile = &mTiles[i];
							if (theOther != nullptr)
								*theOther = anOther;
							return true;
						}
						aNth++;
					}
				}
			}
			i++;
			aTile++;
		} while (i < NUM_TILES);
		if (theTile != nullptr)
			*theTile = nullptr;
		if (theOther != nullptr)
			*theOther = nullptr;
	}
	return false;
}

// 0x406aa0
void Board::deselect()
{
	if (mSelected != nullptr)
		mSelected->mSelected = false;
	mSelected = nullptr;
}

// 0x406ac0
void Board::removePair(Tile* theTile, Tile* theOther)
{
	theOther->mActive = false;
	theTile->mActive = false;
	theTile->mRemovedTime = GetNingPo()->mEventTime;
	theOther->mRemovedTime = GetNingPo()->mEventTime;
	theOther->mSelected = false;
	theTile->mSelected = false;
	mUndoIndex++;
	mUndo[mUndoIndex][0] = theTile;
	mUndo[mUndoIndex][1] = theOther;
	mTilesLeft = mTilesLeft + -2;
	deselect();
	mMoves = countMoves();
}

// 0x406b50
void Board::undo(Tile** theTile, Tile** theOther)
{
	if (mUndoIndex < 0)
	{
		*theTile = nullptr;
		*theOther = nullptr;
	}
	else
	{
		*theTile = mUndo[mUndoIndex][0];
		*theOther = mUndo[mUndoIndex][1];
		mUndoIndex = mUndoIndex + -1;
	}
	if (*theTile != nullptr)
	{
		(*theTile)->mActive = true;
		mTilesLeft = mTilesLeft + 1;
	}
	if (*theOther != nullptr)
	{
		(*theOther)->mActive = true;
		mTilesLeft = mTilesLeft + 1;
	}
	if (*theTile != nullptr || *theOther != nullptr)
		mMoves = countMoves();
}

// 0x406be0
void Board::clearUndo()
{
	unsigned int i = 0;
	do
	{
		mUndo[i][0] = nullptr;
		mUndo[i][1] = nullptr;
		i++;
	} while (i < NUM_UNDOS);
	mUndoIndex = -1;
}

// 0x406c00
void Board::shuffle()
{
	clearUndo();
	do
	{
		int i = 0;
		Tile* aTile = mTiles;
		do
		{
			if (aTile->mActive)
			{
				int j;
				do
				{
					do
					{
						j = GetNingPo()->mRandom.next(0x8f);
					} while (i == j);
				} while (!mTiles[j].mActive);
				Tile aCopy = *aTile;
				aTile->mType = mTiles[j].mType;
				aTile->mFace = mTiles[j].mFace;
				mTiles[j].mType = aCopy.mType;
				mTiles[j].mFace = aCopy.mFace;
			}
			i++;
			aTile++;
		} while (i < NUM_TILES);
		if (findMove(nullptr, nullptr, 0))
			break;
	} while (1 < countFree());
	mMoves = countMoves();
}

} // namespace ningpo
