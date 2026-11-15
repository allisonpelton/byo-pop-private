#ifndef __JB_JBBASE_H__
#define __JB_JBBASE_H__

// What the engine took from Win32 and MSVC 6 and the port provides itself: RECT and its functions (with Win32's
// semantics: an empty rect has right <= left or bottom <= top, UnionRect ignores empty rects, IntersectRect leaves an
// empty result zeroed), MulDiv (rounded, as Win32's), the engine's linked lists of {object, next, prev} nodes
// (0x420160 (mahjongchina), 0x420710 (mahjongjapan) adds at the head), and the C runtime's rand/srand (rand
// 0x4176d6 (mahjongchina), 0x417d9f (mahjongjapan); srand 0x4176cc (mahjongchina), 0x417d95 (mahjongjapan); MSVC's
// LCG).

#include <cstdint>
#include <string>

namespace jb
{

// The screen the engine draws: the original's 800x600 window or display mode
enum
{
	SCREEN_WIDTH = 800,
	SCREEN_HEIGHT = 600,
};

// The mouse cursor an object asks for (Object::SetCursorKind, Manager::SetCursor)
enum
{
	CURSOR_KIND_ARROW = 0,
	CURSOR_KIND_HAND = 1,
	CURSOR_KIND_TEXT = 2,
	CURSOR_KIND_WAIT = 4,
};

struct Rect
{
	int left = 0, top = 0, right = 0, bottom = 0;
};

struct Point
{
	int x = 0, y = 0;
};

inline void SetRect(Rect& theRect, int theLeft, int theTop, int theRight, int theBottom)
{
	theRect.left = theLeft;
	theRect.top = theTop;
	theRect.right = theRight;
	theRect.bottom = theBottom;
}
inline void SetRectEmpty(Rect& theRect) { theRect = Rect(); }
inline bool IsRectEmpty(const Rect& theRect) { return theRect.right <= theRect.left || theRect.bottom <= theRect.top; }
inline void OffsetRect(Rect& theRect, int theDX, int theDY)
{
	theRect.left += theDX;
	theRect.right += theDX;
	theRect.top += theDY;
	theRect.bottom += theDY;
}
inline bool PtInRect(const Rect& theRect, int x, int y)
{
	return x >= theRect.left && x < theRect.right && y >= theRect.top && y < theRect.bottom;
}
bool IntersectRect(Rect& theOut, const Rect& theA, const Rect& theB);
bool UnionRect(Rect& theOut, const Rect& theA, const Rect& theB);
int MulDiv(int theNumber, int theNumerator, int theDenominator);

// The C runtime's generator (srand 0x4176cc (mahjongchina), 0x417d95 (mahjongjapan), rand 0x4176d6 (mahjongchina),
// 0x417d9f (mahjongjapan))
void CrtSrand(unsigned int theSeed);
int CrtRand();
// GetFileAttributesA(...) != -1 and DeleteFileA, on the host's files
bool FileExists(const char* thePath);
void RemoveFile(const char* thePath);
// GetTickCount, timeGetTime (the host's millisecond clock)
uint32_t TickCount();
// the machine's own clock (the deal's time limit), and the number that seeds the generator (the clock unless a mod says
// otherwise)
uint32_t RealTickCount();
uint32_t SeedTick();
// port: the start does not wait: no logo screens (two seconds each, with nothing loaded meanwhile) and no fade out of
// the black before the first screen; the game opens at its main menu (-logos keeps the original's start)
extern bool gFastStart;

// The module a resource comes from: the exe, or Resource.dll (the original's HMODULE; null meant the app's
// Resource.dll, app +0x30)
enum class Module { EXE, DLL };
// FindResourceA, SizeofResource and LockResource: the bytes of a resource, or nullptr (the app provides it)
const uint8_t* FindResourceData(Module theModule, const char* theType, int theId, uint32_t* theSize);
// isspace for the C locale (CString's TrimLeft and TrimRight)
inline bool IsSpace(char theChar) { return theChar == ' ' || (theChar >= '\t' && theChar <= '\r'); }

// The engine's CString helpers, on std::string 0x424c90 (mahjongchina), 0x422a00 (mahjongjapan): the text from thePos
// to the next theSep (or to the end); thePos moves past the separator, or to the end; at or past the end the token is
// empty and thePos stays
std::string Tokenize(const std::string& theText, char theSep, int& thePos);
// 0x424d70 (mahjongchina), 0x422ae0 (mahjongjapan): a decimal number, or a hex one after an x or X anywhere in the text
// (strtol)
int ParseNumber(const std::string& theText);
// 0x424e00 (mahjongchina), 0x422b70 (mahjongjapan): the first of theSet at or after theStart, or -1
int FindOneOf(const std::string& theText, int theStart, const char* theSet);
// 0x425750 (mahjongchina), 0x4234c0 (mahjongjapan), 0x4257a0 (mahjongchina), 0x423510 (mahjongjapan)
void TrimRight(std::string& theText);
void TrimLeft(std::string& theText);
// 0x424f40 (mahjongchina), 0x422cb0 (mahjongjapan), 0x424f70 (mahjongchina), 0x422ce0 (mahjongjapan): CString::Mid (the
// start and count clamped to the text)
std::string Mid(const std::string& theText, int theFirst);
std::string Mid(const std::string& theText, int theFirst, int theCount);
// 0x425990 (mahjongchina), 0x423700 (mahjongjapan): CString::Delete (the original copied past the end for a count
// beyond it; the callers never pass one)
void Delete(std::string& theText, int theIndex, int theCount);
// 0x4258f0 (mahjongchina), 0x423660 (mahjongjapan), 0x4259f0 (mahjongchina), 0x423760 (mahjongjapan): CString::Insert
void Insert(std::string& theText, int theIndex, char theChar);
void Insert(std::string& theText, int theIndex, const char* theInsert);
// 0x425460 (mahjongchina), 0x4231d0 (mahjongjapan): CString::Format
std::string Format(const char* theFormat, ...);
// 0x4254f0 (mahjongchina), 0x423260 (mahjongjapan): the format is the Resource.dll string theId
std::string FormatDll(int theId, ...);
// 0x4256e0 (mahjongchina), 0x423450 (mahjongjapan), 0x425660 (mahjongchina), 0x4233d0 (mahjongjapan): GetTimeFormatA
// "H:mm:ss" of theSeconds (the port always uses ':')
std::string FormatTime(int theSeconds);
// 0x425560 (mahjongchina), 0x4232d0 (mahjongjapan): GetNumberFormatA with the locale's grouping and no decimals (the
// port groups by threes with ',')
std::string FormatNumber(int theNumber);

// 0x425820 (mahjongchina), 0x423590 (mahjongjapan), 0x425800 (mahjongchina), 0x423570 (mahjongjapan)
// (CString::LoadString on Resource.dll and on the exe, 0x425840 (mahjongchina), 0x4235b0 (mahjongjapan)): empty when
// there is none
std::string LoadDllString(int theId);
std::string LoadExeString(int theId);

// lstrcmpiA / _stricmp for ASCII
int StrICmp(const char* theA, const char* theB);

// A node of the engine's linked lists (0xc bytes)
struct ListNode
{
	void* mObj;
	ListNode* mNext;
	ListNode* mPrev;
};

// 0x420160 (mahjongchina), 0x420710 (mahjongjapan): a node at the head
void ListAdd(ListNode*& theHead, void* theObj);
// The removal every list has its own copy of (0x42a360 (mahjongjapan), 0x42a3b0 (mahjongjapan)...): the first node of
// theObj, if any
void ListRemove(ListNode*& theHead, void* theObj);
bool ListContains(ListNode* theHead, void* theObj);
// Frees every node (the destructors' loops)
void ListClear(ListNode*& theHead);

} // namespace jb

#endif
