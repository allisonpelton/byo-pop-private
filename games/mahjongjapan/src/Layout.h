#ifndef __MAHJONGJAPAN_LAYOUT_H__
#define __MAHJONGJAPAN_LAYOUT_H__

// A layout (0x4144e0, vtable 0x51d808) and its tiles (0x413260, vtable 0x51d7f8, 0x48 bytes). Only what the parts
// translated so far use is declared here yet; 0x4144e0..0x415800 is translated next.

#include "JBBase.h"

#include <string>

namespace mahjongjapan
{

// 0x413260, 0x4132d0
class Tile
{
public:
	int mX = 0;            // +0x04
	int mY = 0;            // +0x08
	int mZ = 0;            // +0x0c
	int mKind = 0;         // +0x10 1-based
	bool mFlag14 = false;  // +0x14
	int m18 = 0;           // +0x18
	int m1c = 0;           // +0x1c
	jb::Rect mRect = {-1, -1, -1, -1};  // +0x20
	bool mFlag38 = false;  // +0x38
	bool mFlag39 = false;  // +0x39
	int m3c = 0;           // +0x3c
	int m40 = 0;           // +0x40
	bool mFlag44 = false;  // +0x44

	// 0x413260
	Tile() {}
	// 0x4132d0
	Tile(int theX, int theY, int theZ, int theKind) : mX(theX), mY(theY), mZ(theZ), mKind(theKind) {}
	// 0x4132b0, 0x413330
	virtual ~Tile() {}
};

// 0x4144e0
class Layout
{
public:
	jb::ListNode* mTiles = nullptr;  // +0x04
	jb::ListNode* mCursor = nullptr;  // +0x0c
	bool mHasCounts = false;          // +0x14 the layout gives the kinds' counts
	std::string mName;                // +0x20

	// 0x4144e0
	Layout();
	// 0x414510, 0x414530
	virtual ~Layout();

	// 0x414590, 0x4145b0
	Tile* First();
	Tile* Next();
	// 0x4150c0: the 42 counts; how many tiles they fix
	int GetCounts(int* theCounts);
	// 0x415110
	bool Load(int theId);
};

} // namespace mahjongjapan

#endif
