#ifndef __SPROUT2_FONT_H__
#define __SPROUT2_FONT_H__

// A bitmap font (engine::Font, 0x1c40 bytes; its constructor 0x4076d0 and destructor 0x48c400 (feedingfrenzy2)): one
// image holding the glyphs in rows, and an XML file giving each glyph's three widths, read by its document handler
// (Font::FontDocHandler, vtable 0x56058c (feedingfrenzy2)). The handler places the glyphs in the image itself, left to
// right and row by row, as the file lists them: a glyph's width (b) with the gap on each side.

#include "DocHandler.h"
#include "Geometry.h"
#include "Object.h"

#include <string>

namespace engine
{

class Engine;
class Surface;

class Font : public Object
{
public:
	// 0x4076d0 (feedingfrenzy2)
	explicit Font(Engine* theEngine);
	~Font() override;

	// 0x407800 (feedingfrenzy2): the font file read (from the engine's createFont, 0x407760 (feedingfrenzy2))
	void load(const std::string& thePath);

	// 0x4312b0 (feedingfrenzy2): how far a character moves the pen, unscaled
	int getCharWidth(char theChar) const;
	// 0x47a2e0 (feedingfrenzy2): the sum of the characters' widths
	int getStringWidth(const char* theText) const;
	// The glyph's place in the image; an empty rectangle for a character the font lacks
	const RectI& getGlyphRect(char theChar) const { return mGlyphRects[glyphIndex(theChar)]; }
	bool hasGlyph(char theChar) const
	{
		const RectI& aRect = getGlyphRect(theChar);
		return aRect.left < aRect.right && aRect.top < aRect.bottom;
	}
	int getLeadingSpace(char theChar) const { return mLeadingSpaces[glyphIndex(theChar)]; }
	int getGlyphWidth(char theChar) const { return mGlyphWidths[glyphIndex(theChar)]; }
	int getTrailingSpace(char theChar) const { return mTrailingSpaces[glyphIndex(theChar)]; }

	Surface* getImage() const { return mImage; }
	int getHeight() const { return mHeight; }
	int getSpaceWidth() const { return mSpaceWidth; }
	int getLeftOverlap() const { return mLeftOverlap; }
	int getRightOverlap() const { return mRightOverlap; }
	int getTopOverlap() const { return mTopOverlap; }
	int getBottomOverlap() const { return mBottomOverlap; }

private:
	class FontDocHandler;

	// The original indexes its tables with the character as a signed char, so a character above 127 (only a typed
	// name could hold one) reads memory before its entry; the port reads the character's own entry
	static int glyphIndex(char theChar) { return (unsigned char)theChar; }

	// 0x434620 (feedingfrenzy2): a glyph of the file placed at the layout's pen
	void addGlyph(int theChar, int theLeadingSpace, int theGlyphWidth, int theTrailingSpace);

	Engine* mEngine;                 // +0x0c
	Ref<Surface> mImage;             // +0x10
	int mLeadingSpaces[256];         // +0x14: a, the space before the glyph
	int mGlyphWidths[256];           // +0x414: b, the glyph's width in the image
	int mTrailingSpaces[256];        // +0x814: c, the space after it
	RectI mGlyphRects[256];          // +0xc14
	int mHeight;                     // +0x1c14
	int mSpaceWidth;                 // +0x1c18: what a character without a glyph moves the pen
	int mGap;                        // +0x1c1c: the empty pixels around each glyph in the image
	int mLayoutX;                    // +0x1c20: the pen placing the glyphs in the image
	int mLayoutY;                    // +0x1c24
	int mLeftOverlap;                // +0x1c28
	int mRightOverlap;               // +0x1c2c
	int mTopOverlap;                 // +0x1c30
	int mBottomOverlap;              // +0x1c34
};

} // namespace engine

#endif
