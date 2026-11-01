#ifndef __MAHJONGJAPAN_JBBASE_H__
#define __MAHJONGJAPAN_JBBASE_H__

// What the engine took from Win32 and MSVC 6 and the port provides itself: RECT and its functions (with Win32's
// semantics: an empty rect has right <= left or bottom <= top, UnionRect ignores empty rects, IntersectRect leaves
// an empty result zeroed), MulDiv (rounded, as Win32's), the engine's linked lists of {object, next, prev} nodes
// (0x420710 adds at the head), and the C runtime's rand/srand (0x417d9f/0x417d95, MSVC's LCG).

#include <cstdint>
#include <string>

namespace jb
{

struct Rect
{
	int left = 0, top = 0, right = 0, bottom = 0;
};

struct Point
{
	int x = 0, y = 0;
};

inline void SetRect(Rect& r, int l, int t, int rr, int b) { r.left = l; r.top = t; r.right = rr; r.bottom = b; }
inline void SetRectEmpty(Rect& r) { r = Rect(); }
inline bool IsRectEmpty(const Rect& r) { return r.right <= r.left || r.bottom <= r.top; }
inline void OffsetRect(Rect& r, int dx, int dy) { r.left += dx; r.right += dx; r.top += dy; r.bottom += dy; }
inline bool PtInRect(const Rect& r, int x, int y) { return x >= r.left && x < r.right && y >= r.top && y < r.bottom; }
bool IntersectRect(Rect& out, const Rect& a, const Rect& b);
bool UnionRect(Rect& out, const Rect& a, const Rect& b);
int MulDiv(int a, int b, int c);

// The C runtime's generator (srand 0x417d95, rand 0x417d9f)
void CrtSrand(unsigned int theSeed);
int CrtRand();
// GetFileAttributesA(...) != -1 and DeleteFileA, on the host's files
bool FileExists(const char* thePath);
void RemoveFile(const char* thePath);
// GetTickCount, timeGetTime (the host's millisecond clock)
uint32_t TickCount();

// The module a resource comes from: the exe, or Resource.dll (the original's HMODULE; null meant the app's
// Resource.dll, app +0x30)
enum class Module { Exe, Dll };
// FindResourceA, SizeofResource and LockResource: the bytes of a resource, or nullptr (the app provides it)
const uint8_t* FindResourceData(Module theModule, const char* theType, int theId, uint32_t* theSize);
// isspace for the C locale (CString's TrimLeft and TrimRight)
inline bool IsSpace(char c) { return c == ' ' || (c >= '\t' && c <= '\r'); }

// The engine's CString helpers, on std::string
// 0x422a00: the text from thePos to the next theSep (or to the end); thePos moves past the separator, or to the
// end; at or past the end the token is empty and thePos stays
std::string Tokenize(const std::string& theText, char theSep, int& thePos);
// 0x422ae0: a decimal number, or a hex one after an x or X anywhere in the text (strtol)
int ParseNumber(const std::string& theText);
// 0x422b70: the first of theSet at or after theStart, or -1
int FindOneOf(const std::string& theText, int theStart, const char* theSet);
// 0x4234c0, 0x423510
void TrimRight(std::string& theText);
void TrimLeft(std::string& theText);
// 0x422cb0, 0x422ce0: CString::Mid (the start and count clamped to the text)
std::string Mid(const std::string& theText, int theFirst);
std::string Mid(const std::string& theText, int theFirst, int theCount);
// 0x423700: CString::Delete (the original copied past the end for a count beyond it; the callers never pass one)
void Delete(std::string& theText, int theIndex, int theCount);
// 0x423660, 0x423760: CString::Insert
void Insert(std::string& theText, int theIndex, char theChar);
void Insert(std::string& theText, int theIndex, const char* theInsert);
// 0x4231d0: CString::Format
std::string Format(const char* theFormat, ...);
// 0x423260: the format is the Resource.dll string theId
std::string FormatDll(int theId, ...);
// 0x423450, 0x4233d0: GetTimeFormatA "H:mm:ss" of theSeconds (the port always uses ':')
std::string FormatTime(int theSeconds);
// 0x4232d0: GetNumberFormatA with the locale's grouping and no decimals (the port groups by threes with ',')
std::string FormatNumber(int theNumber);

// ShellExecuteA "open" (the app provides it; it never opens a page in a test run)
void OpenUrl(const char* theUrl);

// 0x423590, 0x423570 (CString::LoadString on Resource.dll and on the exe, 0x4235b0): empty when there is none
std::string LoadDllString(int theId);
std::string LoadExeString(int theId);

// lstrcmpiA / _stricmp for ASCII
int StrICmp(const char* a, const char* b);

// A node of the engine's linked lists (0xc bytes)
struct ListNode
{
	void* mObj;
	ListNode* mNext;
	ListNode* mPrev;
};

// 0x420710: a node at the head
void ListAdd(ListNode*& theHead, void* theObj);
// The removal every list has its own copy of (0x42a360, 0x42a3b0...): the first node of theObj, if any
void ListRemove(ListNode*& theHead, void* theObj);
bool ListContains(ListNode* theHead, void* theObj);
// Frees every node (the destructors' loops)
void ListClear(ListNode*& theHead);

} // namespace jb

#endif
