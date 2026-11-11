#ifndef __COSMICBUGS_CBGAME_H__
#define __COSMICBUGS_CBGAME_H__

// Cosmic Bugs' game code (0x401000..0x420b10) over Retro64's engine. The game keeps its state in globals and in
// structs linked into lists; each struct here keeps the original's layout (pointers are 4 bytes on both targets) so
// offsets in comments can be checked against the export.

#include "R64.h"
#include "R64Bank.h"
#include "R64Xml.h"
#include "R64Draw.h"

#include <type_traits>

#include <cstddef>
#include <cstdint>

namespace cb
{

using namespace r64;

// The game's lists: {current, head, tail} of nodes that start with {next, prev}
template <class T>
struct List
{
	T* current;
	T* head;
	T* tail;
};

// A node's links, as the list's own type (kinds that extend Obj keep Obj links)
template <class T>
T* Node_Next(T* theNode)
{
	return static_cast<T*>(theNode->next);
}

template <class T>
T* Node_Prev(T* theNode)
{
	return static_cast<T*>(theNode->prev);
}

// 0x4108b0 and the like
template <class T>
void List_Init(List<T>* theList)
{
	theList->current = nullptr;
	theList->head = nullptr;
	theList->tail = nullptr;
}

// 0x420b10 and the like: a new node at the tail, made current. 0x4108c0 is one (the level's key map is another)
template <class T>
T* List_Append(List<T>* theList, T* theNode)
{
	if (theList->current == nullptr)
	{
		theNode->next = nullptr;
		theNode->prev = nullptr;
		theList->head = theNode;
	}
	else
	{
		theNode->prev = theList->tail;
		theNode->next = nullptr;
		theList->tail->next = theNode;
	}
	theList->current = theNode;
	theList->tail = theNode;
	return theNode;
}

struct Obj;
// 0x407430's delete: the base's destructor, then the memory (CBObj.cpp)
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

// 0x401000 and the like: deletes the current node; the one after it (else before it) becomes current. False when
// the list is empty. 0x401000, 0x401100, 0x401180, 0x411120, 0x411240, 0x411f50 and 0x441770 are instances of it: for
// gAnims, gLevels (whose nodes end in the level's destructor 0x413730: List_DeleteNode(Level*)), gFonts, the level's
// power-ups, the nodes that end in the Obj destructor 0x419780, its shooters, and nodes freed without a destructor
// (gLevelSets and others)
template <class T>
bool List_RemoveCurrent(List<T>* theList)
{
	T* aNode = theList->current;
	if (aNode == nullptr)
		return false;
	T* aPrev = Node_Prev(aNode);
	if (aPrev == nullptr)
	{
		if (aNode->next != nullptr)
		{
			T* aNext = Node_Next(aNode);
			aNext->prev = nullptr;
			theList->current = aNext;
			theList->head = aNext;
			List_DeleteNode(aNode);
			return true;
		}
	}
	else if (aNode->next != nullptr)
	{
		aPrev->next = aNode->next;
		aNode->next->prev = aNode->prev;
		theList->current = Node_Next(aNode);
		List_DeleteNode(aNode);
		return true;
	}
	if (aPrev == nullptr)
	{
		theList->current = nullptr;
		theList->head = nullptr;
		theList->tail = nullptr;
	}
	else
	{
		aPrev->next = nullptr;
		theList->current = aPrev;
		theList->tail = aPrev;
	}
	List_DeleteNode(aNode);
	return true;
}

// 0x40b0f0: List_RemoveCurrent for nodes the list does not own (the particle pools): unlinks without deleting
template <class T>
bool List_Unlink(List<T>* theList)
{
	T* aNode = theList->current;
	if (aNode == nullptr)
		return false;
	T* aPrev = Node_Prev(aNode);
	if (aPrev == nullptr)
	{
		if (aNode->next != nullptr)
		{
			T* aNext = Node_Next(aNode);
			aNext->prev = nullptr;
			theList->current = aNext;
			theList->head = aNext;
			return true;
		}
	}
	else if (aNode->next != nullptr)
	{
		aPrev->next = aNode->next;
		aNode->next->prev = aNode->prev;
		theList->current = Node_Next(aNode);
		return true;
	}
	if (aPrev == nullptr)
	{
		theList->current = nullptr;
		theList->head = nullptr;
		theList->tail = nullptr;
	}
	else
	{
		aPrev->next = nullptr;
		theList->current = aPrev;
		theList->tail = aPrev;
	}
	return true;
}

// 0x4106b0 and the like: a new node before the current one, made current
template <class T>
T* List_InsertBefore(List<T>* theList, T* theNode)
{
	if (theList->current == nullptr)
	{
		theNode->next = nullptr;
		theNode->prev = nullptr;
	}
	else
	{
		theNode->next = theList->current;
		theNode->prev = theList->current->prev;
		if (theList->current->prev != nullptr)
			theList->current->prev->next = theNode;
		theList->current->prev = theNode;
	}
	theList->current = theNode;
	if (theNode->next == nullptr)
		theList->tail = theNode;
	if (theNode->prev == nullptr)
		theList->head = theNode;
	return theNode;
}

// 0x410a60: the next node made current; at the end the head is, and the result is false
template <class T>
bool List_Next(List<T>* theList)
{
	T* aNode = theList->current;
	if (aNode != nullptr)
	{
		if (aNode->next != nullptr)
		{
			theList->current = Node_Next(aNode);
			return true;
		}
		theList->current = theList->head;
	}
	return false;
}

// 0x410880: the number of nodes (the walk leaves the head current)
template <class T>
int List_Count(List<T>* theList)
{
	T* aHead = theList->head;
	int aCount = 0;
	theList->current = aHead;
	if (aHead != nullptr)
	{
		while (true)
		{
			T* aNode = theList->current;
			aCount = aCount + 1;
			if (aNode == nullptr)
				break;
			if (aNode->next == nullptr)
			{
				theList->current = aHead;
				return aCount;
			}
			theList->current = Node_Next(aNode);
		}
	}
	return aCount;
}

// 0x40f040 and the like
template <class T>
void List_Clear(List<T>* theList)
{
	if (theList->current != nullptr)
	{
		theList->current = theList->head;
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
	if (theList->current != nullptr)
	{
		theList->current = theList->head;
		while (true)
		{
			theFunc(theList->current);
			T* aNode = theList->current;
			if (aNode == nullptr)
				return;
			if (aNode->next == nullptr)
			{
				theList->current = theList->head;
				return;
			}
			theList->current = Node_Next(aNode);
		}
	}
}

// 0x5dcc bytes: an animation's frames, from art.bbk, a FLIC (with its mask FLIC) or a TGA
struct Animation
{
	Animation* next;          // +0x000
	Animation* prev;          // +0x004
	char name[0x80];          // +0x008
	Image* frames[256];       // +0x088
	int count;                // +0x488
	int frameTime;            // +0x48c
	char swap1[0x20];         // +0x490
	char swap2[0x20];         // +0x4b0
	char swap3[0x20];         // +0x4d0
	char file[0x80];          // +0x4f0  a bank image's name (the first of count), or X.eco for X.flc, or X.tga
	int rotation;             // +0x570  degrees the frames are turned when loaded
	Palette palette;          // +0x574  a FLIC's colours
	Palette tints[10];        // +0xd7c  the colours with their green parts changed (Animation_BuildTints)

	Animation();  // 0x401480
	~Animation(); // 0x401b00
	Animation(const Animation&) = delete;
	Animation& operator=(const Animation&) = delete;
};

void Animation_FreeMasks(Animation* theAnim);                    // 0x401510
void Animation_BuildTints(Animation* theAnim);                   // 0x401580
Image* Animation_Frame(const Animation* theAnim, int theIndex);  // 0x4019c0
// 0x401be0: the animation named theName, made current, with its frame count and time; null if none
Animation* Animation_Find(const char* theName, int* theCount, int* theFrameTime);
// 0x401c70: (re)loads the frames; theReload keeps the frame images, which Animation_FreeMasks stripped of masks
int Animation_Load(Animation* theAnim, int theReload);
// 0x4025c0: every animation again, after the screen's format changed
void Animation_ReloadAll();
// 0x402610: an <animation> tag; Animation_Load's result
int Animation_FromXml(Animation* theAnim, const XmlTag* theTag);

// Utilities (0x420090..0x420850)
// 0x420090: the whole file, malloc'd, with a 0 after it (the port's; not counted in theSize); null if missing
uint8_t* ReadWholeFile(const char* thePath, size_t* theSize);
void ParseHex4(const char* theText, int* a, int* b, int* c, int* d);       // 0x420110: "rrggbbaa"
void ParseHex3(const char* theText, int* a, int* b, int* c);               // 0x4201f0: "rrggbb"
void Rand();                                                               // 0x420290
void SeedRand(int theSeed);                                                // 0x4202c0
uint32_t RandMod(uint32_t theRange);                                       // 0x4202d0
int RandRange(int theLow, int theHigh);                                    // 0x4202e0
int RandValue();                                                           // the generator's state
float Dist3(const float* a, const float* b);                               // 0x420300
float SegmentDist3(const float* p, const float* a, const float* b);        // 0x420360
float SegmentDist(float px, float py, float ax, float ay, float bx, float by); // 0x420460
// 0x4204c0: recolours the pixel *thePixel (in theFmt) if the component the flags pick is over each other one's
// share theShare (percent); the others' mean replaces it, then setR/G/B take that dominant value and mulR/G/B
// scale (x/128, at most the dominant value)
void ColourShift(uint32_t* thePixel, const PixelFormat* theFmt, char theRed, char theGreen, char theBlue,
	uint8_t theMulR, uint8_t theMulG, uint8_t theMulB, int theSetR, int theSetG, int theSetB, int theShare);
void SwapColours(Image* theImage, const char* theSwap);                    // 0x420700
double CircleOverlap(float x1, float y1, float r1, float x2, float y2, float r2); // 0x420780
int BoxOverlap(float x1, float y1, float w1, float h1, float x2, float y2, float w2, float h2); // 0x4207d0
void RotateImage(Image* theImage, int theAngle);                           // 0x4208c0
// 0x428d60: the original only formats its text (vsprintf) and hands it to OutputDebugStringA; the append to log.txt is
// 0x428db0, which nothing translated calls (its callers are the untranslated network packet manager's shutdown)
void Log(const char* theFormat, ...);

// Globals
extern Bank gArtBank;            // 0x472bd8
extern Bank gArt8Bank;           // 0x473168
extern List<Animation> gAnims;   // 0x46f35c
extern Palette gGreyPalette;     // 0x460058: r = g = b = index (its packed colours stay 0)
extern int gGreyPaletteUnset;    // 0x460860
extern Dirty gDirty;             // 0x472a7c
extern const int kScreenClip[4]; // 0x460010: {0, 0, 799, 599}

} // namespace cb

#endif
