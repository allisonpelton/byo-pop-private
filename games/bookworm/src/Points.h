#ifndef __BOOKWORM_POINTS_H__
#define __BOOKWORM_POINTS_H__

// Points.cpp: floating text widget - the "+N" score popups (Board::AddPoints, GoldScore, centred)
// and the bonus messages (Board::WriteBonusText, LockergnomeSmallColor, left-aligned). It has no Update of
// its own: Board::Update keeps them in Board::mPopups, cycles mHue, moves them up 1 px every 2 ticks,
// decrements mLife and deletes them at 0. The text fades out over the last 10 updates.
// Original: class Sexy::Points : public Widget, sizeof 0x84, vftable 0x500e0c (47 slots; overrides
// 0 dtor, 16 Draw). Offsets are absolute (2004 Widget is 0x58 bytes).
// Behaviour spec: re/bookworm/analysis/board_draw.md section 4.

#include "Widget.h"
#include <string>

namespace Sexy
{

class BookWormApp;
class Graphics;
class Font;

class Points : public Widget
{
public:
	enum
	{
		JUSTIFY_CENTER = 0,             // theX is the text centre
		JUSTIFY_RIGHT = 1,              // theX is the right edge; any other value: left edge (Board passes -1)
		FADE_TICKS = 10,                // alpha = min(255, mLife * 255 / 10)
		MIN_X = 0xe2,                   // 226: left clamp
		MAX_RIGHT = 0x27e               // 638: right clamp of x + width
	};

	BookWormApp*	mApp;                   // +0x58
	int				mLife;                  // +0x5c updates left (Board decrements it, deletes the widget at <= 0)
	std::string		mText;                  // +0x60
	Font*			mFont;                  // +0x7c
	int				mHue;                   // +0x80 Board: (mHue + 7) % 255 per update; never drawn

public:
	Points(BookWormApp* theApp, Font* theFont, const std::string& theText, int theX, int theY, int theLife,
		int theJustify);                                            // 0x431a50
	virtual ~Points();                                              // 0x4319e0 body, 0x431a20 slot 0

	virtual void	Draw(Graphics* g) override;                     // 0x431950 slot 16
};

}

#endif
