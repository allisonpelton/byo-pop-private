#ifndef __HEAVYWEAPON_BANNER_H__
#define __HEAVYWEAPON_BANNER_H__

// Banner.cpp: a line of big letters over the field ("get ready", a mission's name, "boss"). Each letter flies in from
// the front, the first one first, wobbles as it settles, holds, and fades.
// Original: 0x78 bytes, constructor 0x41c810, Update 0x41c5f0, Draw 0x41cae0; the board keeps them in its list at
// +0x2c4 (the list's Remove of one node: 0x41c3c0). Its empty vector at +0x34 is never filled and is left out.

#include "ObjectList.h"

#include <string>
#include <vector>

namespace Sexy
{

class Board;
class Graphics;
class ImageFont;

class Banner
{
public:
	Board*						mBoard;			// +0x00
	ImageFont*					mFont;			// +0x04
	std::string					mText;			// +0x08
	int							mPenX;			// +0x24 the next letter's left side while drawing, from the centre
	int							mY;				// +0x28 its baseline's top: the y asked for less the font's descent
	int							mWidth;			// +0x2c the text's width in the font
	int							mDelay;			// +0x30 updates before the letters start
	std::vector<double>			mScales;		// +0x44 each letter's size, 9 + 2 * its index down 0.3 an update. It is
												// drawn from 8 down and at its own size from 1; past 0 it times the
												// hold, the fade (-70 to -75.1) and the end (the last letter at -80)
	std::vector<double>			mWobbles;		// +0x54 each letter's wobble: 1 down 0.02 an update from size 8, to 0
	std::vector<double>			mWobbleAngles;	// +0x64 each letter's wobble angle, up 0.3 an update below size 0
	ObjectList<Banner>::Node*	mNode;			// +0x74 its node in the board's list

public:
	// It adds itself to theBoard's list
	Banner(Board* theBoard, ImageFont* theFont, const std::string& theText, int y, int theDelay);	// 0x41c810

	void			Update();			// 0x41c5f0; at the end it deletes itself
	void			Draw(Graphics* g);	// 0x41cae0
};

}

#endif
