#ifndef __FONT_H__
#define __FONT_H__

// port(variant): headless shim of framework/Font.h for the verifier (games/bejeweled/docs/CODE_MODS_DESIGN.md
// 6.9). Not part of the original game. A font is its metrics, set by the verifier: only the score popups read
// one while the board plays (Board::Update moves a popup up to the points font's ascent / 2 + 2), and the
// replay's ENV records that ascent. Text widths only size the banner image, which nothing reads.

#include "Common.h"

namespace Sexy
{

class Font
{
public:
	int					mAscent;
	int					mHeight;

	Font() : mAscent(0), mHeight(0) {}
	virtual ~Font() {}

	virtual int			GetAscent() { return mAscent; }
	virtual int			GetHeight() { return mHeight; }
	virtual int			StringWidth(const SexyString& theString) { return 0; }
	virtual int			CharWidth(SexyChar theChar) { return 0; }
};

}

#endif
