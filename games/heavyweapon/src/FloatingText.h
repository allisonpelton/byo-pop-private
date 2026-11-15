#ifndef __HEAVYWEAPON_FLOATINGTEXT_H__
#define __HEAVYWEAPON_FLOATINGTEXT_H__

// FloatingText.cpp: a line of text over the field (points scored, a power-up's name) that rises and fades.
// Original: 0x4c bytes, constructor 0x42c000, Draw 0x42bf90; the board keeps them in its list at +0x244 and moves them
// (Board::UpdateFloatingTexts).

#include "Color.h"

#include <string>

namespace Sexy
{

class Board;
class Graphics;
class HeavyWeaponApp;

class FloatingText
{
public:
	HeavyWeaponApp*	mApp;			// +0x00
	Board*			mBoard;			// +0x04
	std::string		mText;			// +0x08
	int				mAlpha;			// +0x24 255 down 1 an update; it goes at 0. It also lifts the text: y + mAlpha / 2
	int				mX;				// +0x28 its left side, from the screen's centre
	int				mY;				// +0x2c
	int				mWidth;			// +0x30
	int				mHue;			// +0x34 the rainbow's hue, up 16 an update
	bool			mIsRainbow;		// +0x38 its colour cycles through the hues
	Color			mColor;			// +0x3c

public:
	// Centred on x and kept on the screen; Color(0) for white. It adds itself to theBoard's list.
	FloatingText(Board* theBoard, const std::string& theText, int x, int y, bool isRainbow, const Color& theColor); // 0x42c000

	void			Draw(Graphics* g);	// 0x42bf90
};

}

#endif
