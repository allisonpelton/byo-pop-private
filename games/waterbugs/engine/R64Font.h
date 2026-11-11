#ifndef __COSMICBUGS_R64FONT_H__
#define __COSMICBUGS_R64FONT_H__

// Retro64's bitmap fonts (0x422320..0x423920): a glyph image per character, taken from a bank by name
// (Font_LoadFromBank) or cut from one strip image at its empty columns (Font_StripCut), drawn onto a target buffer
// with a palette, an optional outline and '^' colour codes.

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

// 0x423140: a new image at the tail, made current
void ImageList_Append(ImageList* theList);
// 0x40d0e0: destroys the current image; the one after it (else before it) becomes current. False when empty.
bool ImageList_RemoveCurrent(ImageList* theList);

// 0xd6c bytes in the original; the offsets here are its offsets. Left out: its flag for unclipped drawing (+0x408;
// no font is drawn unclipped) and its texture, textureX, textureY and maskTarget (+0xd50 to +0xd5f; nothing sets
// them), so a member after +0x408 sits 4 bytes lower here and one after +0xd5f 20 bytes lower.
struct Font
{
	Image* mGlyphs[256];        // +0x000  by character code
	int mSpaceWidth;            // +0x400  what a space without a glyph advances, with the spacing
	int mSpacing;               // +0x404  between characters
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
	uint8_t mOwnedPadding[0x104]; // +0xc4c  the bytes before owned: nothing reads them
	ImageList mOwned;           // +0xd60  the glyphs Font_StripCut made

	Font();  // 0x423820
	Font(const Font&) = delete;
	Font& operator=(const Font&) = delete;
};

void Font_SetSpacing(Font* theFont, int theSpacing);                          // 0x422320
void Font_SetClip(Font* theFont, int l, int t, int theRight, int theBottom);                 // 0x422340
// 0x422370: the clip becomes {0, 0, w, h}
void Font_SetTarget(Font* theFont, uint8_t* theDst, int theDPitch, const PixelFormat* theFmt, int theWidth,
	int theHeight);
void Font_SetPalette(Font* theFont, Palette* thePalette);                     // 0x4223d0
void Font_SetOutline(Font* theFont, Palette* thePalette);                     // 0x4223e0
void Font_SetColour(Font* theFont, char theCode, Palette* thePalette);        // 0x4223f0
void Font_UseColour(Font* theFont, char theCode);                             // 0x422410
void Font_SetAlign(Font* theFont, int theAlign);                              // 0x422430
void Font_SetEscapes(Font* theFont, int theEscapes);                          // 0x422440
// 0x422450: the glyphs' own palettes, left aligned, no outline
void Font_ResetStyle(Font* theFont);
int Font_CharWidth(const Font* theFont, char c);                              // 0x422490
int Font_CharAdvance(const Font* theFont, char c);                            // 0x4224c0
int Font_StringWidth(const Font* theFont, const char* theText);               // 0x422500
int Font_StringHeight(const Font* theFont, const char* theText);              // 0x422570
int Font_Height(const Font* theFont);                                         // 0x4225c0
// 0x4225d0: theAlign 1 right, 2 centred; no colour codes; returns the width
int Font_Draw(Font* theFont, const char* theText, int x, int y, int theAlign);
// 0x422730: every glyph as a colour at an alpha
int Font_DrawFill(Font* theFont, const char* theText, int x, int y, int theAlign, uint8_t r, uint8_t g, uint8_t b,
	uint32_t theAlpha);
// 0x422930: draws with the font's alignment and colour codes and returns the drawn rectangle {l, t, r, b} in
// theRect (clipped to the font's clip; all -1 when nothing of it is left). theOutlinePass draws the glyphs' masks.
int* Font_DrawBounded(Font* theFont, int* theRect, int x, int y, const char* theText, int theOutlinePass);
// 0x4231d0: cuts the bank image theName at its empty columns into a glyph per character of theChars, plus a
// space half as wide as the first
int Font_StripCut(Font* theFont, const Bank* theBank, const char* theName, const char* theChars);
// 0x4234c0
void Font_DrawOutlined(Font* theFont, int* theRect, int x, int y, const char* theText);
// 0x4235b0
int* Font_Printf(Font* theFont, int* theRect, int x, int y, const char* theFormat, ...);
// 0x4235f0: the bank's images named thePrefix followed by a character code
void Font_LoadFromBank(Font* theFont, const Bank* theBank, const char* thePrefix);
// 0x423910: forgets the glyphs and destroys the ones Font_StripCut made
void Font_Clear(Font* theFont);

} // namespace r64

#endif
