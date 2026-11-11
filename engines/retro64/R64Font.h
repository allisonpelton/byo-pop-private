#ifndef __RETRO64_R64FONT_H__
#define __RETRO64_R64FONT_H__

// Retro64's bitmap fonts (0x421e40..0x423440 (cosmicbugs), 0x422320..0x423920 (waterbugs)): a glyph image per
// character, taken from a bank by name (Font_LoadFromBank) or cut from one strip image at its empty columns
// (Font_StripCut), drawn onto a target buffer with a palette, an optional outline, a texture through the glyphs' masks
// and '^' colour codes.

#include "R64.h"

namespace r64
{

struct Bank;

// {current, head, tail} of images linked through Image::prev and next
struct ImageList
{
	Image* mCurrent;
	Image* mHead;
	Image* mTail;
};

// 0x422c60 (cosmicbugs), 0x423140 (waterbugs): a new image at the tail, made current
void ImageList_Append(ImageList* theList);
// 0x40cef0 (cosmicbugs), 0x40d0e0 (waterbugs): destroys the current image; the one after it (else before it) becomes
// current. False when empty.
bool ImageList_RemoveCurrent(ImageList* theList);

// 0xd6c bytes in both exes, whose offsets these are
struct Font
{
	Image* mGlyphs[256];        // +0x000  by character code
	int mSpaceWidth;            // +0x400  what a space without a glyph advances, with the spacing
	int mSpacing;               // +0x404  between characters
	int mClipped;               // +0x408  0: draws unclipped (a game sets it for the fonts it draws clipped)
	int mClip[4];               // +0x40c  l, t, r, b
	int mHeight;                // +0x41c  the tallest glyph
	Palette* mColours[255];     // +0x420  the palettes "^c" picks, by c
	int mAlign;                 // +0x81c  Font_DrawBounded: 0 left, 1 right, 2 centred
	int mEscapes;               // +0x820  1: '^' and the character after it pick a colour
	char mBuf[0x400];           // +0x824  Font_Printf's text
	int mColourCode;            // +0xc24  the last "^c"
	Palette* mOutline;          // +0xc28  Font_DrawOutlined draws this first, a pixel up, down, left and right
	int mOutlining;             // +0xc2c  while it does, colour codes are ignored
	int mAlpha;                 // +0xc30  of glyphs with a mask on a screen over 8 bits
	uint8_t* mDst;              // +0xc34
	int mDstPitch;                // +0xc38
	int mDstW;                  // +0xc3c
	int mDstH;                  // +0xc40
	Palette* mPalette;          // +0xc44  the glyphs' palette (null: each glyph's own)
	const PixelFormat* mFmt;    // +0xc48  the target's
	uint8_t mOwnedPadding[0x104]; // +0xc4c  the bytes before the texture: nothing reads them
	Image* mTexture;            // +0xd50  drawn through each glyph's mask instead of the glyph
	int mTextureX;              // +0xd54
	int mTextureY;              // +0xd58
	Image* mMaskTarget;         // +0xd5c  each glyph's mask is also added to this image
	ImageList mOwned;           // +0xd60  the glyphs Font_StripCut made

	Font();  // 0x423340 (cosmicbugs), 0x423820 (waterbugs)
	Font(const Font&) = delete;
	Font& operator=(const Font&) = delete;
};

// 0x421e40 (cosmicbugs), 0x422320 (waterbugs)
void Font_SetSpacing(Font* theFont, int theSpacing);
// 0x421e50 (cosmicbugs), 0x422330 (waterbugs)
void Font_SetClipped(Font* theFont, int theClipped);
// 0x421e60 (cosmicbugs), 0x422340 (waterbugs)
void Font_SetClip(Font* theFont, int l, int t, int theRight, int theBottom);
// 0x421e90 (cosmicbugs), 0x422370 (waterbugs): the clip becomes {0, 0, w, h}
void Font_SetTarget(Font* theFont, uint8_t* theDst, int theDPitch, const PixelFormat* theFmt, int theWidth,
	int theHeight);
// 0x421ef0 (cosmicbugs), 0x4223d0 (waterbugs)
void Font_SetPalette(Font* theFont, Palette* thePalette);
// 0x421f00 (cosmicbugs), 0x4223e0 (waterbugs)
void Font_SetOutline(Font* theFont, Palette* thePalette);
// 0x421f10 (cosmicbugs), 0x4223f0 (waterbugs)
void Font_SetColour(Font* theFont, char theCode, Palette* thePalette);
// 0x421f30 (cosmicbugs), 0x422410 (waterbugs)
void Font_UseColour(Font* theFont, char theCode);
// 0x421f50 (cosmicbugs), 0x422430 (waterbugs)
void Font_SetAlign(Font* theFont, int theAlign);
// 0x421f60 (cosmicbugs), 0x422440 (waterbugs)
void Font_SetEscapes(Font* theFont, int theEscapes);
// 0x421f70 (cosmicbugs), 0x422450 (waterbugs): unclipped, the glyphs' own palettes, left aligned, no outline, texture
// or mask target
void Font_ResetStyle(Font* theFont);
// 0x421fb0 (cosmicbugs), 0x422490 (waterbugs)
int Font_CharWidth(const Font* theFont, char c);
// 0x421fe0 (cosmicbugs), 0x4224c0 (waterbugs)
int Font_CharAdvance(const Font* theFont, char c);
// 0x422020 (cosmicbugs), 0x422500 (waterbugs)
int Font_StringWidth(const Font* theFont, const char* theText);
// 0x422090 (cosmicbugs), 0x422570 (waterbugs)
int Font_StringHeight(const Font* theFont, const char* theText);
// 0x4220e0 (cosmicbugs), 0x4225c0 (waterbugs)
int Font_Height(const Font* theFont);
// 0x4220f0 (cosmicbugs), 0x4225d0 (waterbugs): theAlign 1 right, 2 centred; no colour codes; returns the width
int Font_Draw(Font* theFont, const char* theText, int x, int y, int theAlign);
// 0x422250 (cosmicbugs), 0x422730 (waterbugs): every glyph as a colour at an alpha
int Font_DrawFill(Font* theFont, const char* theText, int x, int y, int theAlign, uint8_t r, uint8_t g, uint8_t b,
	uint32_t theAlpha);
// 0x422450 (cosmicbugs), 0x422930 (waterbugs): draws with the font's alignment and colour codes and returns the drawn
// rectangle {l, t, r, b} in theRect (clipped to the font's clip; all -1 when nothing of it is left). theOutlinePass
// draws the glyphs' masks.
int* Font_DrawBounded(Font* theFont, int* theRect, int x, int y, const char* theText, int theOutlinePass);
// 0x422cf0 (cosmicbugs), 0x4231d0 (waterbugs): cuts the bank image theName at its empty columns into a glyph per
// character of theChars, plus a space half as wide as the first
int Font_StripCut(Font* theFont, const Bank* theBank, const char* theName, const char* theChars);
// 0x422fe0 (cosmicbugs), 0x4234c0 (waterbugs)
void Font_DrawOutlined(Font* theFont, int* theRect, int x, int y, const char* theText);
// 0x4230d0 (cosmicbugs), 0x4235b0 (waterbugs)
int* Font_Printf(Font* theFont, int* theRect, int x, int y, const char* theFormat, ...);
// 0x423110 (cosmicbugs), 0x4235f0 (waterbugs): the bank's images named thePrefix followed by a character code
void Font_LoadFromBank(Font* theFont, const Bank* theBank, const char* thePrefix);
// 0x423430 (cosmicbugs), 0x423910 (waterbugs): forgets the glyphs and destroys the ones Font_StripCut made
void Font_Clear(Font* theFont);

} // namespace r64

#endif
