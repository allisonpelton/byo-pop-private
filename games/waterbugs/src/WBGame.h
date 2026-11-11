#ifndef __WATERBUGS_WBGAME_H__
#define __WATERBUGS_WBGAME_H__

// Water Bugs' game code (0x4010a0..0x420ff0) over Retro64's engine. The game keeps its state in globals and in
// structs linked into lists; each struct here keeps the original's layout (pointers are 4 bytes on both targets) so
// offsets in comments can be checked against the export.

#include "R64.h"
#include "R64Bank.h"
#include "R64Xml.h"
#include "R64Draw.h"

#include <type_traits>

#include <cstddef>
#include <cstdint>

namespace wb
{

using namespace r64;

// The board: 50 columns by 37 rows of 16-pixel cells, 800 by 592 pixels of the screen's 800 by 600
constexpr int CELL_SIZE = 16;
constexpr int CELL_HALF = 8;                                  // from a cell's corner to its middle
constexpr int GRID_COLUMNS = 50;
constexpr int GRID_ROWS = 37;
constexpr int BOARD_WIDTH = GRID_COLUMNS * CELL_SIZE;         // 800
constexpr int BOARD_HEIGHT = GRID_ROWS * CELL_SIZE;           // 592

// The game's lists: {current, head, tail} of nodes that start with {next, prev}
template <class T>
struct List
{
	T* mCurrent;
	T* mHead;
	T* mTail;
};

// A node's links, as the list's own type (kinds that extend Obj keep Obj links)
template <class T>
T* Node_Next(T* theNode)
{
	return static_cast<T*>(theNode->mNext);
}

template <class T>
T* Node_Prev(T* theNode)
{
	return static_cast<T*>(theNode->mPrev);
}

// 0x41c130 and the like
template <class T>
// 0x41c130
void List_Init(List<T>* theList)
{
	theList->mCurrent = nullptr;
	theList->mHead = nullptr;
	theList->mTail = nullptr;
}

// 0x420ff0 and the like: a new node at the tail, made current
template <class T>
// 0x410810 (the level's key map is one of its instances)
T* List_Append(List<T>* theList, T* theNode)
{
	if (theList->mCurrent == nullptr)
	{
		theNode->mNext = nullptr;
		theNode->mPrev = nullptr;
		theList->mHead = theNode;
	}
	else
	{
		theNode->mPrev = theList->mTail;
		theNode->mNext = nullptr;
		theList->mTail->mNext = theNode;
	}
	theList->mCurrent = theNode;
	theList->mTail = theNode;
	return theNode;
}

struct Obj;
// 0x40c200's delete: the base's destructor, then the memory (WBObj.cpp)
void Obj_Delete(Obj* theObj);

// How a list deletes a node: objects through their base, others plainly (an overload for a kind that needs more)
template <class T>
void List_DeleteNode(T* theNode)
{
	if constexpr (std::is_base_of<Obj, T>::value)
		Obj_Delete(theNode);
	else
		delete theNode;
}

// 0x4010a0 and the like: deletes the current node; the one after it (else before it) becomes current. False when
// the list is empty.
template <class T>
// 0x401000, 0x4010a0, 0x4011a0, 0x401220, 0x411030, 0x411150 and 0x412540 are instances of it: for nodes freed without a
// destructor (gLevelSets and others), gAnims, gFonts, the level's power-ups, the nodes that end in the Obj destructor
// 0x419c00, its shooters and gLevels (whose nodes end in the level's destructor: List_DeleteNode(Level*))
bool List_RemoveCurrent(List<T>* theList)
{
	T* aNode = theList->mCurrent;
	if (aNode == nullptr)
		return false;
	T* aPrev = Node_Prev(aNode);
	if (aPrev == nullptr)
	{
		if (aNode->mNext != nullptr)
		{
			T* aNext = Node_Next(aNode);
			aNext->mPrev = nullptr;
			theList->mCurrent = aNext;
			theList->mHead = aNext;
			List_DeleteNode(aNode);
			return true;
		}
	}
	else if (aNode->mNext != nullptr)
	{
		aPrev->mNext = aNode->mNext;
		aNode->mNext->mPrev = aNode->mPrev;
		theList->mCurrent = Node_Next(aNode);
		List_DeleteNode(aNode);
		return true;
	}
	if (aPrev == nullptr)
	{
		theList->mCurrent = nullptr;
		theList->mHead = nullptr;
		theList->mTail = nullptr;
	}
	else
	{
		aPrev->mNext = nullptr;
		theList->mCurrent = aPrev;
		theList->mTail = aPrev;
	}
	List_DeleteNode(aNode);
	return true;
}

// 0x40b260: List_RemoveCurrent for nodes the list does not own (the particle pools): unlinks without deleting
template <class T>
// 0x40b260
bool List_Unlink(List<T>* theList)
{
	T* aNode = theList->mCurrent;
	if (aNode == nullptr)
		return false;
	T* aPrev = Node_Prev(aNode);
	if (aPrev == nullptr)
	{
		if (aNode->mNext != nullptr)
		{
			T* aNext = Node_Next(aNode);
			aNext->mPrev = nullptr;
			theList->mCurrent = aNext;
			theList->mHead = aNext;
			return true;
		}
	}
	else if (aNode->mNext != nullptr)
	{
		aPrev->mNext = aNode->mNext;
		aNode->mNext->mPrev = aNode->mPrev;
		theList->mCurrent = Node_Next(aNode);
		return true;
	}
	if (aPrev == nullptr)
	{
		theList->mCurrent = nullptr;
		theList->mHead = nullptr;
		theList->mTail = nullptr;
	}
	else
	{
		aPrev->mNext = nullptr;
		theList->mCurrent = aPrev;
		theList->mTail = aPrev;
	}
	return true;
}

// 0x410610 and the like: a new node before the current one, made current
template <class T>
T* List_InsertBefore(List<T>* theList, T* theNode)
{
	if (theList->mCurrent == nullptr)
	{
		theNode->mNext = nullptr;
		theNode->mPrev = nullptr;
	}
	else
	{
		theNode->mNext = theList->mCurrent;
		theNode->mPrev = theList->mCurrent->mPrev;
		if (theList->mCurrent->mPrev != nullptr)
			theList->mCurrent->mPrev->mNext = theNode;
		theList->mCurrent->mPrev = theNode;
	}
	theList->mCurrent = theNode;
	if (theNode->mNext == nullptr)
		theList->mTail = theNode;
	if (theNode->mPrev == nullptr)
		theList->mHead = theNode;
	return theNode;
}

// 0x4109b0: the next node made current; at the end the head is, and the result is false
template <class T>
// 0x4109b0
bool List_Next(List<T>* theList)
{
	T* aNode = theList->mCurrent;
	if (aNode != nullptr)
	{
		if (aNode->mNext != nullptr)
		{
			theList->mCurrent = Node_Next(aNode);
			return true;
		}
		theList->mCurrent = theList->mHead;
	}
	return false;
}

// 0x4107e0: the number of nodes (the walk leaves the head current)
template <class T>
// 0x4107e0
int List_Count(List<T>* theList)
{
	T* aHead = theList->mHead;
	int aCount = 0;
	theList->mCurrent = aHead;
	if (aHead != nullptr)
	{
		while (true)
		{
			T* aNode = theList->mCurrent;
			aCount = aCount + 1;
			if (aNode == nullptr)
				break;
			if (aNode->mNext == nullptr)
			{
				theList->mCurrent = aHead;
				return aCount;
			}
			theList->mCurrent = Node_Next(aNode);
		}
	}
	return aCount;
}

// 0x403480 and the like
template <class T>
void List_Clear(List<T>* theList)
{
	if (theList->mCurrent != nullptr)
	{
		theList->mCurrent = theList->mHead;
		bool aMore;
		do
		{
			aMore = List_RemoveCurrent(theList);
		} while (aMore);
	}
}

// The original's walk over a list: theFunc may remove the current node, and the walk then goes on from the node
// after the one that took its place (so that one is skipped this time). It ends with the head current again.
template <class T, class F>
void List_Each(List<T>* theList, F theFunc)
{
	if (theList->mCurrent != nullptr)
	{
		theList->mCurrent = theList->mHead;
		while (true)
		{
			theFunc(theList->mCurrent);
			T* aNode = theList->mCurrent;
			if (aNode == nullptr)
				return;
			if (aNode->mNext == nullptr)
			{
				theList->mCurrent = theList->mHead;
				return;
			}
			theList->mCurrent = Node_Next(aNode);
		}
	}
}

// 0x5dcc bytes: an animation's frames, from art.bbk, a FLIC (with its mask FLIC) or a TGA
struct Animation
{
	Animation* mNext;          // +0x000
	Animation* mPrev;          // +0x004
	char mName[0x80];          // +0x008
	Image* mFrames[256];       // +0x088
	int mCount;                // +0x488
	int mFrameTime;            // +0x48c
	char mSwap1[0x20];         // +0x490
	char mSwap2[0x20];         // +0x4b0
	char mSwap3[0x20];         // +0x4d0
	char mFile[0x80];          // +0x4f0  a bank image's name (the first of count), or X.eco for X.flc, or X.tga
	int mRotation;             // +0x570  degrees the frames are turned when loaded
	Palette mPalette;          // +0x574  a FLIC's colours
	Palette mTints[10];        // +0xd7c  the colours with their green parts changed (Animation_BuildTints)

	Animation();  // 0x401530
	~Animation(); // 0x401bb0
	Animation(const Animation&) = delete;
	Animation& operator=(const Animation&) = delete;
};

void Animation_BuildTints(Animation* theAnim);                   // 0x401630
Image* Animation_Frame(const Animation* theAnim, int theIndex);  // 0x401a70
// 0x401c90: the animation named theName, made current, with its frame count and time; null if none
Animation* Animation_Find(const char* theName, int* theCount, int* theFrameTime);
// 0x401d20: loads the frames
int Animation_Load(Animation* theAnim);
// 0x4026c0: an <animation> tag; Animation_Load's result
int Animation_FromXml(Animation* theAnim, const XmlTag* theTag);

// Utilities (0x420570..0x420d30)
// 0x420570: the whole file, malloc'd, with a 0 after it (the port's; not counted in theSize); null if missing
uint8_t* ReadWholeFile(const char* thePath, size_t* theSize);
void ParseHex4(const char* theText, int* a, int* b, int* c, int* d);       // 0x4205f0: "rrggbbaa"
void Rand();                                                               // 0x420770
void SeedRand(int theSeed);                                                // 0x4207a0
uint32_t RandMod(uint32_t theRange);                                       // 0x4207b0
int RandRange(int theLow, int theHigh);                                    // 0x4207c0
extern int gRandCalls;                                                     // the port's count of draws, for tests
int RandValue();                                                           // the generator's state
float Dist3(const float* a, const float* b);                               // 0x4207e0
float SegmentDist3(const float* p, const float* a, const float* b);        // 0x420840
float SegmentDist(float thePx, float thePy, float theAx, float theAy, float theBx, float theBy); // 0x420940
// 0x4209a0: recolours the pixel *thePixel (in theFmt) if the component the flags pick is over each other one's
// share theShare (percent); the others' mean replaces it, then setR/G/B take that dominant value and mulR/G/B
// scale (x/128, at most the dominant value)
void ColourShift(uint32_t* thePixel, const PixelFormat* theFmt, char theRed, char theGreen, char theBlue,
	uint8_t theMulR, uint8_t theMulG, uint8_t theMulB, int theSetR, int theSetG, int theSetB, int theShare);
void SwapColours(Image* theImage, const char* theSwap);                    // 0x420be0
double CircleOverlap(float theX1, float theY1, float theR1, float theX2, float theY2, float theR2); // 0x420c60
// 0x420cb0
int BoxOverlap(float theX1, float theY1, float theW1, float theH1, float theX2, float theY2, float theW2, float theH2);
void RotateImage(Image* theImage, int theAngle);                           // 0x420da0

// Globals
extern Bank gArtBank;            // 0x472bd8
extern Bank gArt8Bank;           // 0x473168
extern List<Animation> gAnims;   // 0x46f35c
extern Palette gGreyPalette;     // 0x460058: r = g = b = index (its packed colours stay 0)
extern int gGreyPaletteUnset;    // 0x460860
extern Dirty gDirty;             // 0x472a7c
extern const int SCREEN_CLIP[4]; // 0x460010: {0, 0, 799, 599}

} // namespace wb

#endif
