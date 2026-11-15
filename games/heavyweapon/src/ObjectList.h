#ifndef __HEAVYWEAPON_OBJECTLIST_H__
#define __HEAVYWEAPON_OBJECTLIST_H__

// ObjectList: the game's own doubly linked list of owned pointers, which carries its walk's position inside itself.
// Original: 0x10 bytes, +0 the first node (an empty node that holds nothing and always stays), +4 the walk's node,
// +8 the last node, +0xc the number of items. A node is 0xc bytes: +0 the item, +4 the next node, +8 the one before.
// The board keeps every kind of thing on the field in one (enemies, shots, bombs, debris, power-ups, ...).
//
// A walk is written out in the original at every use, as the board's code here does:
//
//		for (aList.mCurrent = aList.mHead; aList.mCurrent->mNext != NULL; )
//		{
//			aList.mCurrent = aList.mCurrent->mNext;
//			T* anItem = aList.Current();
//			...
//		}
//
// and RemoveCurrent (or Remove of the walk's own node) steps the walk back to the node before, so the walk goes on
// with the node after the one removed. A walk inside a walk of the same list saves mCurrent and puts it back.

#include <cstddef>

namespace Sexy
{

template <class T> class ObjectList
{
public:
	struct Node
	{
		T*			mItem;		// +0
		Node*		mNext;		// +4
		Node*		mPrev;		// +8
	};

	Node*			mHead;		// +0x0 the empty node that starts the chain
	Node*			mCurrent;	// +0x4 the walk's node
	Node*			mTail;		// +0x8 the last node (the head when the list is empty)
	int				mCount;		// +0xc

public:
	ObjectList()
	{
		mHead = new Node;
		mHead->mItem = NULL;
		mHead->mNext = NULL;
		mHead->mPrev = NULL;
		mCurrent = mHead;
		mTail = mHead;
		mCount = 0;
	}

	// The board's destructor walks each list from its head and frees every node with its item (0x410910, 0x410980
	// and the like, one for each kind of item)
	~ObjectList()
	{
		Node* aNode = mHead;
		while (aNode != NULL)
		{
			Node* aNext = aNode->mNext;
			delete aNode->mItem;
			delete aNode;
			aNode = aNext;
		}
	}

	// The walk's item. The original reads it through a check of the count, which is never 0 inside a walk.
	T* Current()
	{
		if (mCount == 0)
			return NULL;
		return mCurrent->mItem;
	}

	// 0x410940 (and inline at every use): a node at the end
	Node* Append(T* theItem)
	{
		Node* aNode = new Node;
		aNode->mNext = NULL;
		aNode->mPrev = mTail;
		mTail->mNext = aNode;
		aNode->mItem = theItem;
		mTail = aNode;
		mCount++;
		return aNode;
	}

	// 0x4314f0: the node and its item go; a walk standing on the node steps back to the one before. The item is
	// deleted before its node comes out of the chain, as the original does.
	void Remove(Node* theNode)
	{
		if (theNode == mHead)
			return;
		if (mCurrent == theNode)
			mCurrent = theNode->mPrev;
		delete theNode->mItem;
		if (theNode == mTail)
			mTail = theNode->mPrev;
		theNode->mPrev->mNext = theNode->mNext;
		if (theNode->mNext != NULL)
			theNode->mNext->mPrev = theNode->mPrev;
		delete theNode;
		mCount--;
	}

	// 0x4109b0, 0x410ab0 (and inline at every use): the walk's node and its item go, the walk stepping back to the
	// node before. Here the node comes out of the chain before its item is deleted.
	void RemoveCurrent()
	{
		Node* aNode = mCurrent;
		if (aNode == mHead)
			return;
		if (aNode == mTail)
			mTail = aNode->mPrev;
		aNode->mPrev->mNext = aNode->mNext;
		if (aNode->mNext != NULL)
			aNode->mNext->mPrev = aNode->mPrev;
		delete aNode->mItem;
		Node* aPrev = aNode->mPrev;
		delete aNode;
		mCurrent = aPrev;
		mCount--;
	}

	// 0x4122a0, 0x4125d0 and the like: every item goes, the head stays
	void Clear()
	{
		if (mHead->mNext == NULL)
			return;
		mCurrent = mHead;
		while (mCurrent->mNext != NULL)
		{
			mCurrent = mCurrent->mNext;
			if (mCurrent != mHead)
				RemoveCurrent();
		}
		mCount = 0;
		mCurrent = mHead;
		mTail = mHead;
	}
};

}

#endif
