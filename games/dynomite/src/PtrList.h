#ifndef __PTRLIST_H__
#define __PTRLIST_H__

// The game's list of pointers (vtable 0x4fc748, 0x18 bytes; 0x430ea0-0x431180): a malloc'd array with a cursor
// (ResetIter/Next) that Remove keeps on the next item, so a loop may remove the item it has just been given. An
// optional sort key (an int at a byte offset in each item) makes Add insert in order.

#include <cstdlib>

namespace Sexy
{

template <class T> class PtrList
{
public:
	int		mCount;			// +4
	int		mIter;			// +8
	T**		mItems;			// +0xc
	int		mSortOffset;	// +0x10
	short	mSortMode;		// +0x14 0 none, 1 ascending, else descending

	// 0x430ea0
	PtrList() : mCount(0), mIter(0), mItems(NULL), mSortOffset(0), mSortMode(0) {}
	// 0x430ec0 (the items are not freed)
	virtual ~PtrList()
	{
		if (mItems != NULL)
			free(mItems);
		mItems = NULL;
	}

	// 0x430ef0: empties the list without freeing the items
	void Clear()
	{
		if (mItems != NULL)
			free(mItems);
		mItems = NULL;
		mCount = 0;
	}

	// 0x431180
	void Add(T* theItem)
	{
		if (mSortMode != 0)
		{
			SortedAdd(theItem);
			return;
		}
		if (mCount == 0)
		{
			mCount = 1;
			mItems = (T**) malloc(sizeof(T*));
			mIter = 0;
			mItems[0] = theItem;
			return;
		}
		mCount++;
		mItems = (T**) realloc(mItems, mCount * sizeof(T*));
		mItems[mCount - 1] = theItem;
	}

	// 0x430f10
	void SortedAdd(T* theItem)
	{
		if (mCount == 0)
		{
			mCount = 1;
			mItems = (T**) malloc(sizeof(T*));
			mIter = 0;
			mItems[0] = theItem;
			return;
		}
		int aKey = Key(theItem);
		mCount++;
		T** anOld = mItems;
		bool inserted = false;
		T** aNew = (T**) malloc(mCount * sizeof(T*));
		int i = 0;
		T** aDest = aNew;
		if ((mCount != 1) && (-1 < mCount - 1))
		{
			do
			{
				int anOther = Key(*anOld);
				if ((mSortMode == 1) ? (aKey < anOther) : (anOther < aKey))
				{
					if (!inserted)
					{
						inserted = true;
						if (i < mIter)
							mIter++;
						*aDest++ = theItem;
					}
				}
				*aDest++ = *anOld++;
				i++;
			} while (i < mCount - 1);
		}
		if (!inserted)
			*aDest = theItem;
		free(mItems);
		mItems = aNew;
	}

	// 0x430fe0: removes every occurrence; true if there was one
	bool Remove(T* theItem)
	{
		T** aNew = (T**) malloc(mCount * sizeof(T*));
		T** aSrc = mItems;
		bool found = false;
		int i = 0;
		T** aDest = aNew;
		if (0 < mCount)
		{
			do
			{
				if (*aSrc == theItem)
				{
					if (i <= mIter)
						mIter--;
					found = true;
				}
				else
					*aDest++ = *aSrc;
				aSrc++;
				i++;
			} while (i < mCount);
			if (found == true)
			{
				free(mItems);
				mItems = (T**) realloc(aNew, mCount * sizeof(T*) - sizeof(T*));
				mCount--;
				if (mCount < 1)
				{
					free(mItems);
					mItems = NULL;
				}
				if (mIter < 0)
					mIter = 0;
				return found;
			}
		}
		free(aNew);
		if (mIter < 0)
			mIter = 0;
		return found;
	}

	// 0x431090
	void ResetIter() { mIter = 0; }

	// 0x4310a0
	T* Next()
	{
		if (mCount <= mIter)
			return NULL;
		return mItems[mIter++];
	}

	// 0x4310c0
	T* Get(int theIndex)
	{
		if ((theIndex < mCount) && (-1 < theIndex))
			return mItems[theIndex];
		return NULL;
	}

	// 0x4310e0: theItem before the item at thePos (or at the end)
	void InsertAt(T* theItem, int thePos)
	{
		if (mCount < thePos)
			thePos = mCount;
		T** aSrc = mItems;
		mCount++;
		T** aNew = (T**) malloc(mCount * sizeof(T*));
		T** aDest = aNew;
		int i = 0;
		int n = thePos;
		if (0 < thePos)
		{
			do
			{
				*aDest++ = *aSrc++;
				n--;
				i = thePos;
			} while (n != 0);
		}
		*aDest = theItem;
		i++;
		while (i < mCount)
		{
			aDest++;
			*aDest = *aSrc++;
			i++;
		}
		free(mItems);
		mItems = aNew;
	}

private:
	int Key(T* theItem) { return *(int*) ((char*) theItem + mSortOffset); }
};

}

#endif
