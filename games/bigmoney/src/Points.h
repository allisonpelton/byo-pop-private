#ifndef __BIGMONEY_POINTS_H__
#define __BIGMONEY_POINTS_H__

// Points.cpp: floating text widget - the "+N" score popups (Board::AddPoints, WinBMApp::mFloatFont,
// centred) and the bonus messages (Board::ShowBonusText, mFont1, left-aligned). It has no Update of its
// own: Board::Update keeps them in Board::mPointsWidgets, cycles mHue, moves them up and deletes them
// when mLife runs out.
// Original: class Sexy::Points : public Widget, sizeof 0xac (new at 0x407f23 / 0x408160),
// vftable 0x4fba54 (54 slots; overrides 0 ~Widget, 13 Draw, 44 IsPointVisible = `return true` stub).
// Offsets are absolute (Widget is 0x80 bytes in 1.2).
// Behaviour spec: re/bigmoney/analysis/board_draw.md section 4.

#include "Widget.h"
#include <string>

namespace Sexy
{

class WinBMApp;
class Graphics;
class Font;

class Points : public Widget
{
public:
	enum
	{
		JUSTIFY_CENTER = 0,             // theX is the text centre
		JUSTIFY_RIGHT = 1,              // theX is the right edge; any other value: left edge (Board passes -1)
		FADE_TICKS = 18                 // alpha = min(mLife / 18, 1)
	};

	WinBMApp*		mApp;                   // +0x80
	int				mLife;                  // +0x84 updates left (Board decrements, removes at <= 0)
	std::string		mText;                  // +0x88
	Font*			mFont;                  // +0xa4
	int				mHue;                   // +0xa8 HSL hue of the text, Board: (mHue + 7) % 255 per update

public:
	Points(WinBMApp* theApp, Font* theFont, const std::string& theText, int theX, int theY, int theLife, int theJustify); // 0x417630
	virtual ~Points();                                              // 0x4175c0 (scalar deleting dtor 0x417610) slot 0

	virtual void	Draw(Graphics* g);                              // 0x4174f0 slot 13
};

}

#endif
