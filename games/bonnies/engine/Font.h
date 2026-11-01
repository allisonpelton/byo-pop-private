#ifndef __BONNIES_FONT_H__
#define __BONNIES_FONT_H__

// The bitmap fonts (0x428190-0x428f20, 0x42a640-0x42af30): an image with a frame for each character code, and
// optionally a stroke (<name>_stroke_<n>.png) and a glow (<name>_Glow_<n>.png) image drawn under it, n pixels out.
// Text is laid out in lines that break at blanks and punctuation, '|' is a zero-width mark, and the flags of drawText
// choose the alignment and the effects.

#include "Str.h"

namespace bonnies
{

struct Image;

// A font's description (0x24 bytes), the game's tables of them; mName is the resource key and the stroke and glow
// images' base name, mImage the main image's name
struct FontDesc
{
	Str mName;           // +0x00
	Str mImage;          // +0x08
	float mSpacing;      // +0x10: added to each character's width
	float mLineGap;      // +0x14: between lines
	float mColonX;       // +0x18: a ':' moved by this
	float mColonY;       // +0x1c
	float mSpaceWidth;   // +0x20: a ' ''s spacing
};

// drawText's flags
enum
{
	TEXT_TOP = 0x02,       // at the top (else centred down, or at the bottom with TEXT_BOTTOM)
	TEXT_BOTTOM = 0x04,
	TEXT_LEFT = 0x10,      // at the left (else centred, or at the right with TEXT_RIGHT)
	TEXT_RIGHT = 0x20,
	TEXT_SHADOW = 0x40,    // a shadow below right, in the second colour (else half black)
	TEXT_STROKE = 0x80,    // the stroke image under the text, in the second colour
	TEXT_GLOW = 0x100,     // the glow image under the text, in the second colour
	TEXT_SHIMMER = 0xe00   // a light sweeping across (0x200 strong, 0x800 half, else weak)
};

// A loaded font (0x34 bytes)
struct Font
{
	// 0x428190: the line of theText from theStart that fits in theWidth (a fraction of the screen) at theScale, and in
	// gTextPos where the next one starts; theRect (x, y, w, h) gets its size
	Str getLine(const Str& theText, int theStart, float theWidth, float* theRect, float theScale);
	// 0x428580: the number of lines of theText from theStart; theRect gets the size of them all
	int countLines(const Str& theText, int theStart, float theWidth, float* theRect, float theScale);
	// 0x4286c0: the width of the widest line
	float textWidth(const Str& theText, float theWidth);
	// 0x4286f0: a line's height, with the gap between lines when theWithGap
	double lineHeight(int theWithGap);
	// 0x428730: draws theText in the box theX, theY, theW, theH; returns the y below the last line
	float drawText(const Str& theText, float theX, float theY, float theW, float theH, uint32_t theColor,
		uint32_t theColor2, uint32_t theFlags, float theScale, float theAngle);

	FontDesc* mDesc;      // +0x00: a copy of the description
	Image* mImage;        // +0x04
	Image* mStroke;       // +0x08
	Image* mGlow;         // +0x0c
	int mSelecting;       // +0x10: characters mSelStart..mSelEnd are drawn selected
	int mSelStart;        // +0x14
	int mSelEnd;          // +0x18
	int mCharIndex;       // +0x1c: the character being drawn
	int mStrokeSize;      // +0x20
	int mGlowSize;        // +0x24
	int mDrawingGlow;     // +0x28
	int mDrawingStroke;   // +0x2c
	int mDrawingShadow;   // +0x30
};

// 0x48199c: where getLine's next line starts
extern int gTextPos;

// 0x42a9e0: loads a font with its stroke and glow images; 0x42aed0: frees it
Font* loadFont(FontDesc* theDesc);
void freeFont(Font* theFont);

} // namespace bonnies

#endif
