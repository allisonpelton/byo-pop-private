#ifndef __BOOKWORM_SHADOWFONT_H__
#define __BOOKWORM_SHADOWFONT_H__

// ShadowFont.cpp: an ImageFont that draws every string twice, a solid-colour copy at an offset (the
// shadow) and then the text. The title screen's play link, welcome text and version line use it.
// Original: class Sexy::ShadowFont : public ImageFont, sizeof 0x60 (new in TitleScreen::TitleScreen
// 0x434380 and in Duplicate 0x4332a0), vftable 0x50117c (23 slots: ImageFont's with 0, 7, 10, 11 replaced).
// Offsets are absolute (the 2004 ImageFont is 0x48 bytes). Behaviour spec: re/bookworm/analysis/screens.md.

#include "ImageFont.h"
#include "Color.h"
#include "Point.h"

namespace Sexy
{

class Graphics;

class ShadowFont : public ImageFont
{
public:
	Color			mShadowColor;           // +0x48 shadow colour (its own alpha; the text colour's alpha is not applied)
	Point			mShadowOffset;          // +0x58 shadow offset (x +0x58, y +0x5c); (0,0) draws no shadow

public:
	ShadowFont(const ImageFont& theFont, const Color& theShadowColor, const Point& theOffset); // 0x433250
	ShadowFont(const ShadowFont& theFont);                          // 0x433200
	virtual ~ShadowFont();                                          // 0x4330c0 (scalar deleting dtor 0x4331d0) slot 0
	virtual int		StringWidth(const SexyString& theString) override;       // 0x4330d0 slot 7
	virtual void	DrawString(Graphics* g, int theX, int theY, const SexyString& theString,
						const Color& theColor, const Rect& theClipRect) override; // 0x433160 slot 10
	virtual Font*	Duplicate() override;                           // 0x4332a0 slot 11
};

}

#endif
