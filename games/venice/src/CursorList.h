// port: the engine's intrusive doubly linked list with a built-in cursor (the original's {cursor, head, tail} triple).
//
// Every list of the engine (an object's children and stages, a stage's filters and events, the scene stack, the image
// cache, the sound and music lists) is such a triple, and the original's loops walk it with the cursor: Begin puts the
// cursor on the head, Next moves it on, and the cursor wraps to the head when Next runs off the end. A node removed
// while the list is walked leaves the cursor on the node after it (on the one before it for the tail), so the loops
// that remove as they go keep their order. A node class names its links mNext and mPrev.

#ifndef VENICE_CURSORLIST_H
#define VENICE_CURSORLIST_H

template <class Node> class CursorList
{
public:
	Node* mCursor = nullptr;
	Node* mHead = nullptr;
	Node* mTail = nullptr;

	// 0x444400 (and the copies of it for each list type)
	bool Begin()
	{
		if (mCursor == nullptr)
			return false;
		mCursor = mHead;
		return true;
	}

	// 0x434f60 and its copies: true while the cursor moved onto another node; off the end it goes back to the head
	bool Next()
	{
		if (mCursor == nullptr)
			return false;
		if (mCursor->mNext == nullptr) {
			mCursor = mHead;
			return false;
		}
		mCursor = mCursor->mNext;
		return true;
	}

	// 0x40e750: the node goes on the end and the cursor on it
	void Append(Node* theNode)
	{
		if (mCursor == nullptr) {
			theNode->mNext = nullptr;
			theNode->mPrev = nullptr;
			mHead = theNode;
		} else {
			theNode->mPrev = mTail;
			theNode->mNext = nullptr;
			mTail->mNext = theNode;
		}
		mCursor = theNode;
		mTail = theNode;
	}

	// 0x42b150: the node goes in front of the cursor's node and the cursor on it
	void InsertBeforeCursor(Node* theNode)
	{
		if (mCursor == nullptr) {
			theNode->mNext = nullptr;
			theNode->mPrev = nullptr;
		} else {
			theNode->mNext = mCursor;
			theNode->mPrev = mCursor->mPrev;
			if (mCursor->mPrev != nullptr)
				mCursor->mPrev->mNext = theNode;
			mCursor->mPrev = theNode;
		}
		mCursor = theNode;
		if (theNode->mNext == nullptr)
			mTail = theNode;
		if (theNode->mPrev == nullptr)
			mHead = theNode;
	}

	// Takes the cursor's node out of the list without freeing it; the cursor goes to the node after it, or to the one
	// before it when it was the tail (0x401590, 0x435330 and the copies of them). Returns the node, null for an empty list.
	Node* UnlinkCursor()
	{
		Node* aNode = mCursor;
		if (aNode == nullptr)
			return nullptr;
		Node* aNextCursor;
		if (aNode->mPrev == nullptr || aNode->mNext == nullptr) {
			if (aNode->mNext == nullptr) {
				if (aNode->mPrev == nullptr) {
					aNextCursor = nullptr;
					mHead = nullptr;
					mTail = nullptr;
				} else {
					aNextCursor = aNode->mPrev;
					aNextCursor->mNext = nullptr;
					mTail = aNextCursor;
				}
			} else {
				aNextCursor = aNode->mNext;
				aNextCursor->mPrev = nullptr;
				mHead = aNextCursor;
			}
		} else {
			aNode->mPrev->mNext = aNode->mNext;
			aNode->mNext->mPrev = aNode->mPrev;
			aNextCursor = aNode->mNext;
		}
		mCursor = aNextCursor;
		return aNode;
	}

	// UnlinkCursor and delete: the removal of the original's lists whose nodes are freed
	bool RemoveCursor()
	{
		Node* aNode = UnlinkCursor();
		if (aNode == nullptr)
			return false;
		delete aNode;
		return true;
	}

	// 0x401560, 0x411020, 0x444430 and the copies of them: removes every node
	void Clear()
	{
		mCursor = mHead;
		while (RemoveCursor()) {
		}
	}

	bool IsEmpty() const { return mHead == nullptr; }
};

#endif
