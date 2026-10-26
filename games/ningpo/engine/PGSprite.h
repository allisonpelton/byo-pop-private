#ifndef __NINGPO_PGSPRITE_H__
#define __NINGPO_PGSPRITE_H__

// PGSprite (0x54 bytes, vtable 0x463200, 0x40fa50..0x40fdf0): a surface of the display, loaded from an image of
// the archive, with the reference count of the framework's objects (0x4144f0): load() of a loaded sprite adds a
// reference, release() drops one and frees the surface with the last.

#include "HDisplay.h"

namespace pg
{

class HFileSystem;

class PGSprite
{
public:
	int mRefCount;           // +0x08 (HObject)
	HDisplay* mDisplay;      // +0x0c
	HSurface* mSurface;      // +0x10
	char mName[64];          // +0x14

	PGSprite();              // 0x40fa50
	virtual ~PGSprite();     // 0x40fa80

	PGSprite* clone();                                     // 0x40fd70 (slot 0x20)
	bool isValid() const { return mSurface != nullptr; }   // 0x40fde0 (slot 0x24)
	// 0x40fb20 (slot 0x28): the whole sprite at a point, only its opaque pixels
	void drawAt(const PGPoint* theAt);
	// 0x40fab0 (slot 0x2c)
	void blitEx(const PGRect* theDest, const PGRect* theSrc, int s1, int s2, int s3, int s4, int d1, int d2, int d3,
		int d4, int theOp);
	// 0x40fb00 (slot 0x30)
	void blit(const PGRect* theDest, const PGRect* theSrc);
	// 0x40fc00 (slot 0x34)
	bool create(HDisplay* theDisplay, const HImage& theImage);
	// 0x40fc40 (slot 0x38)
	bool load(HDisplay* theDisplay, const HFileSystem* theFileSystem, const char* theName);
	// 0x40fd40 (slot 0x3c)
	void release();
	int width() const;                    // 0x40fbd0 (slot 0x40)
	int height() const;                   // 0x40fba0 (slot 0x44)
	void getRect(PGRect* theRect) const;  // 0x40fdf0 (slot 0x48)
};

} // namespace pg

#endif
