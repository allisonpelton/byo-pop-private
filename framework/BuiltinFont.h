#ifndef __BUILTINFONT_H__
#define __BUILTINFONT_H__

// port: the framework's own font, so framework UI (the mod menu, framework/ModMenu.h) has text before a game
// has loaded any art, and in every game. A Latin-1 bitmap font written for this port (no game or third-party
// glyphs): a 12 px cell, caps 8 px tall, lower case 6 px, proportional advances. ASCII 33..126 and the Latin-1
// symbols are drawn by hand; the accented letters are composed from a base letter and an accent. Every glyph is
// drawn white into one image (made when text is first drawn) and tinted with the colour; at render scale N the
// screen scales it with nearest filtering, so it stays sharp. Bold is drawn twice, one pixel apart; theScale
// draws every pixel theScale times as large.

#include "Font.h"

namespace Sexy
{

class Image;

class BuiltinFont : public Font
{
public:
	bool					mBold;
	int						mScale;

public:
	BuiltinFont(bool isBold = false, int theScale = 1);
	BuiltinFont(const BuiltinFont& theFont);
	virtual ~BuiltinFont();

	virtual int				StringWidth(const SexyString& theString);
	virtual int				CharWidth(SexyChar theChar);
	virtual int				CharWidthKern(SexyChar theChar, SexyChar thePrevChar);
	virtual void			DrawString(Graphics* g, int theX, int theY, const SexyString& theString, const Color& theColor, const Rect& theClipRect);
	virtual Font*			Duplicate();

	// Frees the glyph image (made again when needed); the app calls it before its display goes away
	static void				FreeImage();
	// The glyph cell of a Latin-1 character as text rows ('#' set, '.' clear), for tests
	static std::string		GlyphRows(unsigned char theChar);
};

}

#endif
