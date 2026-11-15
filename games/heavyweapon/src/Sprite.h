#ifndef __HEAVYWEAPON_SPRITE_H__
#define __HEAVYWEAPON_SPRITE_H__

// Sprite.cpp: an image cut into a grid of equal cels, with the byte mask of its alpha for the pixel collision test.
// Original: vtable 0x52f97c (one slot, the destructor), 0x20 bytes: +4 the app, +8 the image, +0xc the alpha mask, +0x10
// the cel width, +0x14 the cel height, +0x18 the columns, +0x1c the rows. Every image of the game is one (the app's
// table, HeavyWeaponApp::mSprites, and the background animations' strips).

#include "Rect.h"

#include <string>

namespace Sexy
{

class DDImage;
class Graphics;
class HeavyWeaponApp;

class Sprite
{
public:
	HeavyWeaponApp*	mApp;			// +0x04
	DDImage*		mImage;			// +0x08
	unsigned char*	mAlphaMask;		// +0x0c one byte a pixel, the image's alpha (GetAlphaMask)
	int				mCelWidth;		// +0x10
	int				mCelHeight;		// +0x14
	int				mColumns;		// +0x18
	int				mRows;			// +0x1c

public:
	Sprite(HeavyWeaponApp* theApp);												// 0x42d4e0
	virtual ~Sprite();															// 0x42d670

	bool			Load(const std::string& theFileName, int theColumns, int theRows);	// 0x42d500
	unsigned char*	GetAlphaMask();												// 0x42d580
	void			Draw(Graphics* g, int x, int y, bool isCentered, int theColumn, int theRow, bool isMirrored); // 0x42d6b0
	bool			Collides(int x, int y, int theColumn, int theRow, bool isMirrored, Sprite* theOther, int theOtherX,
						int theOtherY, int theOtherColumn, int theOtherRow, bool isOtherMirrored, bool isCentered); // 0x42d750
};

// The intersection of two rectangles, an empty one at 0, 0 when they do not meet (0x42d5e0, Rect::Intersection inline)
Rect SpriteIntersection(const Rect& theRect, const Rect& theOther);

}

#endif
