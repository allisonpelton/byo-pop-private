#include "R64Font.h"
#include "R64Bank.h"
#include "R64Blit.h"

#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace r64
{

#if UINTPTR_MAX == 0xffffffff
// (the original's offsets less what R64Font.h says the port leaves out: 4 bytes after +0x408, 20 after +0xd5f)
static_assert(offsetof(Font, colours) == 0x420 - 4, "Font layout");
static_assert(offsetof(Font, buf) == 0x824 - 4, "Font layout");
static_assert(offsetof(Font, owned) == 0xd60 - 20, "Font layout");
static_assert(sizeof(Font) == 0xd6c - 20, "Font layout");
#endif

namespace
{

// The original indexes the glyphs and colours with a signed char, so a code over 0x7f reads before the table; here
// such a code has no glyph and no colour
Image* GlyphOf(const Font* theFont, char c)
{
	return c < 0 ? nullptr : theFont->glyphs[(int)c];
}

Palette* ColourOf(const Font* theFont, char c)
{
	return c < 0 ? nullptr : theFont->colours[(int)c];
}

inline uint32_t PackColour(const PixelFormat* f, uint8_t r, uint8_t g, uint8_t b)
{
	return (uint32_t)(b >> (f->bLoss & 0x1f)) << (f->bShift & 0x1f) |
		(uint32_t)(g >> (f->gLoss & 0x1f)) << (f->gShift & 0x1f) |
		(uint32_t)(r >> (f->rLoss & 0x1f)) << (f->rShift & 0x1f);
}

// Font_DrawBounded's rectangle of the glyphs drawn so far
void GrowRect(int* theRect, bool& theFirst, const Image* theGlyph, int x, int y)
{
	if (theFirst)
	{
		theRect[0] = Image_DrawX(theGlyph) + x;
		theRect[2] = theGlyph->w + -1 + theRect[0];
		theRect[1] = Image_DrawY(theGlyph) + y;
		theRect[3] = theGlyph->h + -1 + Image_DrawY(theGlyph) + y;
		theFirst = false;
	}
	else
	{
		int gx = Image_DrawX(theGlyph) + x;
		int aMin = theRect[0];
		if (gx <= theRect[0])
			aMin = gx;
		theRect[0] = aMin;
		aMin = gx + -1 + theGlyph->w;
		if (aMin < theRect[2])
			aMin = theRect[2];
		theRect[2] = aMin;
		int gy = Image_DrawY(theGlyph) + y;
		aMin = theRect[1];
		if (gy <= theRect[1])
			aMin = gy;
		theRect[1] = aMin;
		aMin = gy + -1 + theGlyph->h;
		if (aMin < theRect[3])
			aMin = theRect[3];
		theRect[3] = aMin;
	}
}

} // namespace

// 0x423140
void ImageList_Append(ImageList* theList)
{
	Image* anImage = new Image();
	if (theList->current == nullptr)
	{
		anImage->next = nullptr;
		anImage->prev = nullptr;
		theList->head = anImage;
	}
	else
	{
		anImage->prev = theList->tail;
		anImage->next = nullptr;
		theList->tail->next = anImage;
	}
	theList->current = anImage;
	theList->tail = anImage;
}

// 0x40d0e0
bool ImageList_RemoveCurrent(ImageList* theList)
{
	Image* anImage = theList->current;
	if (anImage == nullptr)
		return false;
	Image* aPrev = anImage->prev;
	if (aPrev == nullptr || anImage->next == nullptr)
	{
		Image* aNext = anImage->next;
		if (aNext == nullptr)
		{
			if (aPrev == nullptr)
			{
				theList->current = nullptr;
				theList->head = nullptr;
				theList->tail = nullptr;
			}
			else
			{
				aPrev->next = nullptr;
				theList->current = aPrev;
				theList->tail = aPrev;
			}
		}
		else
		{
			aNext->prev = nullptr;
			theList->current = aNext;
			theList->head = aNext;
		}
	}
	else
	{
		aPrev->next = anImage->next;
		anImage->next->prev = anImage->prev;
		theList->current = anImage->next;
	}
	Image_Destroy(anImage);
	delete anImage;
	return true;
}

// 0x423820
Font::Font()
{
	owned.current = nullptr;
	owned.head = nullptr;
	owned.tail = nullptr;
	std::memset(glyphs, 0, sizeof(glyphs));
	spaceWidth = 4;
	spacing = 1;
	clip[0] = clip[1] = clip[2] = clip[3] = 0;
	height = 0;
	std::memset(colours, 0, sizeof(colours));
	align = 0;
	escapes = 0;
	std::memset(buf, 0, sizeof(buf));
	colourCode = 0;
	outline = nullptr;
	outlining = 0;
	dst = nullptr;
	dstPitch = 0;
	fmt = nullptr;
	palette = nullptr;
	dstW = 0;
	dstH = 0;
	std::memset(ownedPadding, 0, sizeof(ownedPadding));
	alpha = 0xff;
}

void Font_SetSpacing(Font* theFont, int theSpacing)
{
	theFont->spacing = theSpacing;
}

void Font_SetClip(Font* theFont, int l, int t, int r, int b)
{
	theFont->clip[0] = l;
	theFont->clip[1] = t;
	theFont->clip[2] = r;
	theFont->clip[3] = b;
}

void Font_SetTarget(Font* theFont, uint8_t* theDst, int theDPitch, const PixelFormat* theFmt, int w, int h)
{
	theFont->dst = theDst;
	theFont->dstPitch = theDPitch;
	theFont->fmt = theFmt;
	theFont->dstW = w;
	theFont->dstH = h;
	theFont->clip[0] = 0;
	theFont->clip[1] = 0;
	theFont->clip[2] = w;
	theFont->clip[3] = h;
}

void Font_SetPalette(Font* theFont, Palette* thePalette)
{
	theFont->palette = thePalette;
}

void Font_SetOutline(Font* theFont, Palette* thePalette)
{
	theFont->outline = thePalette;
}

void Font_SetColour(Font* theFont, char theCode, Palette* thePalette)
{
	if (theCode >= 0)
		theFont->colours[(int)theCode] = thePalette;
}

void Font_UseColour(Font* theFont, char theCode)
{
	theFont->palette = ColourOf(theFont, theCode);
}

void Font_SetAlign(Font* theFont, int theAlign)
{
	theFont->align = theAlign;
}

void Font_SetEscapes(Font* theFont, int theEscapes)
{
	theFont->escapes = theEscapes;
}

void Font_ResetStyle(Font* theFont)
{
	theFont->palette = nullptr;
	theFont->align = 0;
	theFont->outline = nullptr;
}

int Font_CharWidth(const Font* theFont, char c)
{
	Image* aGlyph = GlyphOf(theFont, c);
	int aWidth = 0;
	if (aGlyph != nullptr)
		return aGlyph->origW;
	if (c == ' ')
		aWidth = theFont->spaceWidth;
	return aWidth;
}

int Font_CharAdvance(const Font* theFont, char c)
{
	Image* aGlyph = GlyphOf(theFont, c);
	int aWidth = 0;
	if (aGlyph != nullptr)
		return aGlyph->origW + theFont->spacing;
	if (c == ' ')
		aWidth = theFont->spacing + theFont->spaceWidth;
	return aWidth;
}

int Font_StringWidth(const Font* theFont, const char* theText)
{
	const uint8_t* aText = (const uint8_t*)theText;
	int aWidth = 0;
	if (*aText != 0)
	{
		do
		{
			if (theFont->escapes == 1 && *aText == '^')
			{
				aText = aText + 1;
				if (*aText == 0)
					return aWidth;
			}
			else
			{
				uint8_t c = *aText;
				int anAdvance;
				if (theFont->glyphs[c] == nullptr)
				{
					if (c != ' ')
						goto next;
					anAdvance = theFont->spacing + theFont->spaceWidth;
				}
				else
					anAdvance = theFont->glyphs[c]->origW + theFont->spacing;
				aWidth = aWidth + anAdvance;
			}
		next:
			aText = aText + 1;
		} while (*aText != 0);
	}
	return aWidth;
}

int Font_StringHeight(const Font* theFont, const char* theText)
{
	const uint8_t* aText = (const uint8_t*)theText;
	int aHeight = 0;
	if (*aText != 0)
	{
		do
		{
			if (theFont->escapes == 1 && *aText == '^')
			{
				aText = aText + 1;
				if (*aText == 0)
					return aHeight;
			}
			else if (theFont->glyphs[*aText] != nullptr && aHeight < theFont->glyphs[*aText]->origH)
				aHeight = theFont->glyphs[*aText]->origH;
			aText = aText + 1;
		} while (*aText != 0);
	}
	return aHeight;
}

int Font_Height(const Font* theFont)
{
	return theFont->height;
}

int Font_Draw(Font* theFont, const char* theText, int x, int y, int theAlign)
{
	const uint8_t* aText = (const uint8_t*)theText;
	int aWidth = Font_StringWidth(theFont, theText);
	if (theAlign == 1)
		x = x - aWidth;
	else if (theAlign == 2)
		x = x - aWidth / 2;
	uint8_t aChar = *aText;
	if (aChar != 0)
	{
		do
		{
			if (theFont->glyphs[aChar] == nullptr)
			{
				if (aChar == ' ')
					x = x + theFont->spacing + theFont->spaceWidth;
			}
			else
			{
				Image_SetPalette(theFont->glyphs[aChar], theFont->palette);
				Image_BlitMasked(theFont->glyphs[*aText], theFont->dst, theFont->dstPitch, theFont->fmt, theFont->clip, x, y);
				x = x + theFont->glyphs[*aText]->origW + theFont->spacing;
			}
			aChar = aText[1];
			aText = aText + 1;
		} while (aChar != 0);
		return aWidth;
	}
	return aWidth;
}

int Font_DrawFill(Font* theFont, const char* theText, int x, int y, int theAlign, uint8_t theRed, uint8_t theGreen, uint8_t theBlue,
	uint32_t theAlpha)
{
	const uint8_t* aText = (const uint8_t*)theText;
	int aWidth = Font_StringWidth(theFont, theText);
	int aShift = aWidth;
	if (theAlign != 1)
	{
		if (theAlign != 2)
			goto aligned;
		aShift = aWidth / 2;
	}
	x = x - aShift;
aligned:
	uint8_t aChar = *aText;
	if (aChar != 0)
	{
		do
		{
			if (theFont->glyphs[aChar] == nullptr)
			{
				if (aChar == ' ')
					x = x + theFont->spacing + theFont->spaceWidth;
			}
			else
			{
				Image_SetPalette(theFont->glyphs[aChar], theFont->palette);
				const PixelFormat* f = theFont->fmt;
				Image_BlitFillAlpha(theFont->glyphs[*aText], theFont->dst, theFont->dstPitch, f, theFont->clip, x, y,
					PackColour(f, theRed, theGreen, theBlue), theAlpha);
				x = x + theFont->glyphs[*aText]->origW + theFont->spacing;
			}
			aChar = aText[1];
			aText = aText + 1;
		} while (aChar != 0);
		return aWidth;
	}
	return aWidth;
}

// The rectangle is clipped to the font's clip at the end. A glyph with a mask on a screen over 8 bits is drawn
// faded at the font's alpha: the glyph itself, or on an outline pass its mask.
int* Font_DrawBounded(Font* theFont, int* theRect, int x, int y, const char* theText, int theOutlinePass)
{
	const uint8_t* aText = (const uint8_t*)theText;
	theRect[0] = -1;
	theRect[1] = -1;
	theRect[2] = -1;
	theRect[3] = -1;
	bool aFirst = true;
	int aWidth = Font_StringWidth(theFont, theText);
	if (theFont->align == 1)
		x = x - aWidth;
	else if (theFont->align == 2)
		x = x - aWidth / 2;
	uint8_t aChar = *aText;
	while (aChar != 0)
	{
		if (theFont->escapes == 1 && *aText == '^')
		{
			aChar = aText[1];
			aText = aText + 1;
			if (aChar == 0)
				break;
			if (theFont->outlining == 0)
			{
				theFont->colourCode = (int)aChar;
				theFont->palette = ColourOf(theFont, (char)aChar);
			}
		}
		else
		{
			aChar = *aText;
			Image* aGlyph = theFont->glyphs[aChar];
			if (aGlyph == nullptr)
			{
				if (aChar == ' ')
					x = x + theFont->spacing + theFont->spaceWidth;
			}
			else
			{
				GrowRect(theRect, aFirst, aGlyph, x, y);
				Image_SetPalette(aGlyph, theFont->palette);
				if (aGlyph->hasMask == 1 && 8 < theFont->fmt->bpp)
				{
					if (theOutlinePass == 0)
						Image_BlitAlphaFade(aGlyph, theFont->dst, theFont->dstPitch, theFont->fmt, theFont->clip, x, y,
							(uint32_t)theFont->alpha);
					else
					{
						Image_SetPalette(aGlyph->mask, theFont->palette);
						Image_BlitAlphaFade(aGlyph->mask, theFont->dst, theFont->dstPitch, theFont->fmt, theFont->clip, x,
							y, (uint32_t)theFont->alpha);
					}
				}
				else
					Image_BlitMasked(aGlyph, theFont->dst, theFont->dstPitch, theFont->fmt, theFont->clip, x, y);
				x = x + theFont->glyphs[*aText]->origW + theFont->spacing;
			}
		}
		aText = aText + 1;
		aChar = *aText;
	}
	int dstX, dstY, srcX, srcY, blitWidth, blitHeight;
	if (ClipBlit(theFont->clip[0], theFont->clip[1], theFont->clip[2], theFont->clip[3], theRect[0], theRect[1],
			(theRect[2] - theRect[0]) + 1, (theRect[3] - theRect[1]) + 1, &dstX, &dstY, &srcX, &srcY, &blitWidth, &blitHeight) != 1)
	{
		theRect[0] = -1;
		theRect[1] = -1;
		theRect[2] = -1;
		theRect[3] = -1;
		return theRect;
	}
	theRect[0] = dstX;
	theRect[2] = dstX + blitWidth + -1;
	theRect[3] = dstY + -1 + blitHeight;
	theRect[1] = dstY;
	return theRect;
}

// Each glyph is the run of columns with some mask after the empty ones; the original does not stop at the strip's
// right edge while it skips empty columns, which here ends the strip instead (a column past the edge counts as
// empty).
int Font_StripCut(Font* theFont, const Bank* theBank, const char* theName, const char* theChars)
{
	Image* aStrip;
	if (theBank == nullptr || (aStrip = Bank_FindImage2(theBank, theName)) == nullptr)
		return 0;
	theFont->height = aStrip->h;
	int x = 0;
	int aCount = (int)std::strlen(theChars);
	int k = 0;
	if (0 < aCount)
	{
		do
		{
			int aSet = x < aStrip->w ? Image_MaskColumnSet(aStrip, x) : 0;
			int aStart = x;
			while (aSet == 0 && aStart < aStrip->w)
			{
				aStart = aStart + 1;
				aSet = Image_MaskColumnSet(aStrip, aStart);
			}
			int aWidth = 0;
			int aSet2 = aStart < aStrip->w ? Image_MaskColumnSet(aStrip, aStart) : 0;
			x = aStart;
			while (aSet2 != 0 && x < aStrip->w)
			{
				x = x + 1;
				aWidth = aWidth + 1;
				aSet2 = x < aStrip->w ? Image_MaskColumnSet(aStrip, x) : 0;
			}
			ImageList_Append(&theFont->owned);
			Image* aGlyph = theFont->owned.current;
			Image_Create(aGlyph, aWidth, theFont->height, &aStrip->fmt);
			Copy8(aGlyph->bits, 0, 0, aGlyph->pitch, 0, aStrip->bits, (int)(aStrip->fmt.bpp >> 3) * aStart, 0,
				aStrip->pitch, 0, (int)(aStrip->fmt.bpp >> 3) * aWidth, theFont->height);
			Image* aMask = new Image();
			aGlyph->mask = aMask;
			aGlyph->hasMask = 1;
			Image_Create(aMask, aWidth, theFont->height, &aStrip->mask->fmt);
			aGlyph->mask->palette = aStrip->mask->palette;
			Image* aStripMask = aStrip->mask;
			Copy8(aGlyph->mask->bits, 0, 0, aGlyph->mask->pitch, 0, aStripMask->bits, aStart, 0, aStripMask->pitch, 0,
				(int)(aStripMask->fmt.bpp >> 3) * aWidth, theFont->height);
			Image* aMade = theFont->owned.current;
			theFont->glyphs[(uint8_t)theChars[k]] = aMade;
			int aBottom = aMade->yOff + aMade->h;
			if (theFont->height < aBottom)
				theFont->height = aBottom;
			k = k + 1;
		} while (k < aCount);
	}
	ImageList_Append(&theFont->owned);
	Image* aSpace = theFont->owned.current;
	Image_Create(aSpace, theFont->owned.head->w / 2, theFont->height, &aStrip->fmt);
	Image* aMask = new Image();
	aSpace->mask = aMask;
	aSpace->hasMask = 1;
	Image_Create(aMask, theFont->owned.head->w / 2, theFont->height, &aStrip->mask->fmt);
	aSpace->mask->palette = aStrip->mask->palette;
	Image* aMade = theFont->owned.current;
	theFont->glyphs[' '] = aMade;
	int aBottom = aMade->yOff + aMade->h;
	if (theFont->height < aBottom)
		theFont->height = aBottom;
	return 1;
}

void Font_DrawOutlined(Font* theFont, int* theRect, int x, int y, const char* theText)
{
	int aRect[4];
	if (theFont->outline != nullptr)
	{
		Palette* aSaved = theFont->palette;
		theFont->palette = theFont->outline;
		theFont->outlining = 1;
		Font_DrawBounded(theFont, aRect, x, y + -1, theText, 1);
		Font_DrawBounded(theFont, aRect, x, y + 1, theText, 1);
		Font_DrawBounded(theFont, aRect, x + -1, y, theText, 1);
		Font_DrawBounded(theFont, aRect, x + 1, y, theText, 1);
		theFont->outlining = 0;
		theFont->palette = aSaved;
	}
	int* aDrawn = Font_DrawBounded(theFont, aRect, x, y, theText, 0);
	theRect[0] = aDrawn[0];
	theRect[1] = aDrawn[1];
	theRect[2] = aDrawn[2];
	theRect[3] = aDrawn[3];
	if (theFont->outline != nullptr)
	{
		theRect[0] = theRect[0] + -1;
		theRect[1] = theRect[1] + -1;
		theRect[2] = theRect[2] + 1;
		theRect[3] = theRect[3] + 1;
	}
}

int* Font_Printf(Font* theFont, int* theRect, int x, int y, const char* theFormat, ...)
{
	va_list anArgs;
	va_start(anArgs, theFormat);
	std::vsnprintf(theFont->buf, sizeof(theFont->buf), theFormat, anArgs);
	va_end(anArgs);
	Font_DrawOutlined(theFont, theRect, x, y, theFont->buf);
	return theRect;
}

// The images from the first whose name starts with the prefix's first letter to the bank's end, by position
void Font_LoadFromBank(Font* theFont, const Bank* theBank, const char* thePrefix)
{
	if (theBank == nullptr)
		return;
	size_t aSize = std::strlen(thePrefix);
	int aCount = (int)theBank->images.size();
	int i = 0;
	while (i != aCount)
	{
		if ((uint8_t)theBank->images[i]->name[0] == (uint8_t)*thePrefix)
			break;
		i = i + 1;
	}
	if (i != aCount)
	{
		theFont->height = 0;
		while (i != aCount)
		{
			const char* aName = theBank->images[i]->name;
			if (std::strncmp(aName, thePrefix, aSize) == 0)
			{
				long aCode = std::atol(aName + aSize);
				Image* aGlyph = Bank_ImageAt(theBank, (uint32_t)i);
				if (aCode >= 0 && aCode < 256)
					theFont->glyphs[aCode] = aGlyph;
				int aBottom = aGlyph->yOff + aGlyph->h;
				if (theFont->height < aBottom)
					theFont->height = aBottom;
			}
			i = i + 1;
		}
	}
}

void Font_Clear(Font* theFont)
{
	std::memset(theFont->glyphs, 0, sizeof(theFont->glyphs));
	theFont->spaceWidth = 0;
	theFont->spacing = 0;
	theFont->clip[0] = 0;
	theFont->clip[1] = 0;
	theFont->clip[2] = 0;
	theFont->clip[3] = 0;
	ImageList* aList = &theFont->owned;
	if (aList->current != nullptr)
	{
		aList->current = aList->head;
		bool aMore;
		do
		{
			aMore = ImageList_RemoveCurrent(aList);
		} while (aMore);
	}
}

} // namespace r64
