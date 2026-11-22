// BBFont: the game's two fonts (0x42b8f0..0x42c290 in the original). Each loads a PopCap image font, finds every
// glyph's inked columns and the rows any glyph inks, and redraws the 256 glyphs, scaled to the window, into an Anim
// of 16 by 16 square cels; the text objects (ObjString) draw a string cel by cel, each cel as wide as its glyph.
#pragma once

#include "Anim.h"

namespace Sexy
{

class DDImage;
class ImageFont;

// A glyph's inked box in the font's picture, columns and rows both inclusive
struct GlyphBox
{
	int mLeft;
	int mTop;
	int mRight;
	int mBottom;
};

class BBFont : public Anim
{
public:
	float mScale;            // the glyphs' scale into the cels
	int mCelSize;            // a cel's width and height
	ImageFont* mImageFont;
	bool mFitCapHeight;      // scaled so a glyph is 43 design pixels high (the red font), not by the window alone
	GlyphBox mGlyphs[256];

	BBFont();
	~BBFont();
	void Reload() override;

	bool Load(bool fitCapHeight, const char* theFileName);
	DDImage* BuildCels(DDImage* theFontImage);

	static BBFont* Create(const char* theFileName, bool fitCapHeight);
};

} // namespace Sexy
