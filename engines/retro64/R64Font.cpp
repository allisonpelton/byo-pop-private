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
static_assert(offsetof(Font, mColours) == 0x420, "Font layout");
static_assert(offsetof(Font, mBuf) == 0x824, "Font layout");
static_assert(offsetof(Font, mTextureUnused) == 0xd50, "Font layout");
static_assert(sizeof(Font) == 0xd6c, "Font layout");
#endif

namespace
{

// The original indexes the glyphs and colours with a signed char, so a code over 0x7f reads before the table; here
// such a code has no glyph and no colour
Image* GlyphOf(const Font* theFont, char c)
{
	return c < 0 ? nullptr : theFont->mGlyphs[(int)c];
}

Palette* ColourOf(const Font* theFont, char c)
{
	return c < 0 ? nullptr : theFont->mColours[(int)c];
}

inline uint32_t PackColour(const PixelFormat* f, uint8_t r, uint8_t g, uint8_t b)
{
	return (uint32_t)(b >> (f->mBLoss & 0x1f)) << (f->mBShift & 0x1f) |
		(uint32_t)(g >> (f->mGLoss & 0x1f)) << (f->mGShift & 0x1f) |
		(uint32_t)(r >> (f->mRLoss & 0x1f)) << (f->mRShift & 0x1f);
}

// Font_DrawBounded's rectangle of the glyphs drawn so far
void GrowRect(int* theRect, bool& theFirst, const Image* theGlyph, int x, int y)
{
	if (theFirst)
	{
		theRect[0] = Image_DrawX(theGlyph) + x;
		theRect[2] = theGlyph->mW + -1 + theRect[0];
		theRect[1] = Image_DrawY(theGlyph) + y;
		theRect[3] = theGlyph->mH + -1 + Image_DrawY(theGlyph) + y;
		theFirst = false;
	}
	else
	{
		int aGx = Image_DrawX(theGlyph) + x;
		int aMin = theRect[0];
		if (aGx <= theRect[0])
			aMin = aGx;
		theRect[0] = aMin;
		aMin = aGx + -1 + theGlyph->mW;
		if (aMin < theRect[2])
			aMin = theRect[2];
		theRect[2] = aMin;
		int aGy = Image_DrawY(theGlyph) + y;
		aMin = theRect[1];
		if (aGy <= theRect[1])
			aMin = aGy;
		theRect[1] = aMin;
		aMin = aGy + -1 + theGlyph->mH;
		if (aMin < theRect[3])
			aMin = theRect[3];
		theRect[3] = aMin;
	}
}

} // namespace

// 0x422c60 (cosmicbugs), 0x423140 (waterbugs)
void ImageList_Append(ImageList* theList)
{
	Image* anImage = new Image();
	if (theList->mCurrent == nullptr)
	{
		anImage->mNext = nullptr;
		anImage->mPrev = nullptr;
		theList->mHead = anImage;
	}
	else
	{
		anImage->mPrev = theList->mTail;
		anImage->mNext = nullptr;
		theList->mTail->mNext = anImage;
	}
	theList->mCurrent = anImage;
	theList->mTail = anImage;
}

// 0x40cef0 (cosmicbugs), 0x40d0e0 (waterbugs)
bool ImageList_RemoveCurrent(ImageList* theList)
{
	Image* anImage = theList->mCurrent;
	if (anImage == nullptr)
		return false;
	Image* aPrev = anImage->mPrev;
	if (aPrev == nullptr || anImage->mNext == nullptr)
	{
		Image* aNext = anImage->mNext;
		if (aNext == nullptr)
		{
			if (aPrev == nullptr)
			{
				theList->mCurrent = nullptr;
				theList->mHead = nullptr;
				theList->mTail = nullptr;
			}
			else
			{
				aPrev->mNext = nullptr;
				theList->mCurrent = aPrev;
				theList->mTail = aPrev;
			}
		}
		else
		{
			aNext->mPrev = nullptr;
			theList->mCurrent = aNext;
			theList->mHead = aNext;
		}
	}
	else
	{
		aPrev->mNext = anImage->mNext;
		anImage->mNext->mPrev = anImage->mPrev;
		theList->mCurrent = anImage->mNext;
	}
	Image_Destroy(anImage);
	delete anImage;
	return true;
}

// 0x423340 (cosmicbugs), 0x423820 (waterbugs)
Font::Font()
{
	mOwned.mCurrent = nullptr;
	mOwned.mHead = nullptr;
	mOwned.mTail = nullptr;
	std::memset(mGlyphs, 0, sizeof(mGlyphs));
	mSpaceWidth = 4;
	mSpacing = 1;
	mClip[0] = mClip[1] = mClip[2] = mClip[3] = 0;
	mHeight = 0;
	std::memset(mColours, 0, sizeof(mColours));
	mAlign = 0;
	mEscapes = 0;
	std::memset(mBuf, 0, sizeof(mBuf));
	mColourCode = 0;
	mOutline = nullptr;
	mOutlining = 0;
	mDst = nullptr;
	mDstPitch = 0;
	mFmt = nullptr;
	mPalette = nullptr;
	mDstW = 0;
	mDstH = 0;
	std::memset(mOwnedPadding, 0, sizeof(mOwnedPadding));
	mAlpha = 0xff;
}

void Font_SetSpacing(Font* theFont, int theSpacing)
{
	theFont->mSpacing = theSpacing;
}

void Font_SetClip(Font* theFont, int l, int t, int r, int b)
{
	theFont->mClip[0] = l;
	theFont->mClip[1] = t;
	theFont->mClip[2] = r;
	theFont->mClip[3] = b;
}

void Font_SetTarget(Font* theFont, uint8_t* theDst, int theDPitch, const PixelFormat* theFmt, int theWidth,
	int theHeight)
{
	theFont->mDst = theDst;
	theFont->mDstPitch = theDPitch;
	theFont->mFmt = theFmt;
	theFont->mDstW = theWidth;
	theFont->mDstH = theHeight;
	theFont->mClip[0] = 0;
	theFont->mClip[1] = 0;
	theFont->mClip[2] = theWidth;
	theFont->mClip[3] = theHeight;
}

void Font_SetPalette(Font* theFont, Palette* thePalette)
{
	theFont->mPalette = thePalette;
}

void Font_SetOutline(Font* theFont, Palette* thePalette)
{
	theFont->mOutline = thePalette;
}

void Font_SetColour(Font* theFont, char theCode, Palette* thePalette)
{
	if (theCode >= 0)
		theFont->mColours[(int)theCode] = thePalette;
}

void Font_UseColour(Font* theFont, char theCode)
{
	theFont->mPalette = ColourOf(theFont, theCode);
}

void Font_SetAlign(Font* theFont, int theAlign)
{
	theFont->mAlign = theAlign;
}

void Font_SetEscapes(Font* theFont, int theEscapes)
{
	theFont->mEscapes = theEscapes;
}

void Font_ResetStyle(Font* theFont)
{
	theFont->mPalette = nullptr;
	theFont->mAlign = 0;
	theFont->mOutline = nullptr;
}

int Font_CharWidth(const Font* theFont, char c)
{
	Image* aGlyph = GlyphOf(theFont, c);
	int aWidth = 0;
	if (aGlyph != nullptr)
		return aGlyph->mOrigW;
	if (c == ' ')
		aWidth = theFont->mSpaceWidth;
	return aWidth;
}

int Font_CharAdvance(const Font* theFont, char c)
{
	Image* aGlyph = GlyphOf(theFont, c);
	int aWidth = 0;
	if (aGlyph != nullptr)
		return aGlyph->mOrigW + theFont->mSpacing;
	if (c == ' ')
		aWidth = theFont->mSpacing + theFont->mSpaceWidth;
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
			if (theFont->mEscapes == 1 && *aText == '^')
			{
				aText = aText + 1;
				if (*aText == 0)
					return aWidth;
			}
			else
			{
				uint8_t c = *aText;
				int anAdvance;
				if (theFont->mGlyphs[c] == nullptr)
				{
					if (c != ' ')
						goto next;
					anAdvance = theFont->mSpacing + theFont->mSpaceWidth;
				}
				else
					anAdvance = theFont->mGlyphs[c]->mOrigW + theFont->mSpacing;
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
			if (theFont->mEscapes == 1 && *aText == '^')
			{
				aText = aText + 1;
				if (*aText == 0)
					return aHeight;
			}
			else if (theFont->mGlyphs[*aText] != nullptr && aHeight < theFont->mGlyphs[*aText]->mOrigH)
				aHeight = theFont->mGlyphs[*aText]->mOrigH;
			aText = aText + 1;
		} while (*aText != 0);
	}
	return aHeight;
}

int Font_Height(const Font* theFont)
{
	return theFont->mHeight;
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
			if (theFont->mGlyphs[aChar] == nullptr)
			{
				if (aChar == ' ')
					x = x + theFont->mSpacing + theFont->mSpaceWidth;
			}
			else
			{
				Image_SetPalette(theFont->mGlyphs[aChar], theFont->mPalette);
				Image_BlitMasked(theFont->mGlyphs[*aText], theFont->mDst, theFont->mDstPitch, theFont->mFmt,
					theFont->mClip, x, y);
				x = x + theFont->mGlyphs[*aText]->mOrigW + theFont->mSpacing;
			}
			aChar = aText[1];
			aText = aText + 1;
		} while (aChar != 0);
		return aWidth;
	}
	return aWidth;
}

int Font_DrawFill(Font* theFont, const char* theText, int x, int y, int theAlign, uint8_t theRed, uint8_t theGreen,
	uint8_t theBlue, uint32_t theAlpha)
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
			if (theFont->mGlyphs[aChar] == nullptr)
			{
				if (aChar == ' ')
					x = x + theFont->mSpacing + theFont->mSpaceWidth;
			}
			else
			{
				Image_SetPalette(theFont->mGlyphs[aChar], theFont->mPalette);
				const PixelFormat* f = theFont->mFmt;
				Image_BlitFillAlpha(theFont->mGlyphs[*aText], theFont->mDst, theFont->mDstPitch, f, theFont->mClip, x,
					y, PackColour(f, theRed, theGreen, theBlue), theAlpha);
				x = x + theFont->mGlyphs[*aText]->mOrigW + theFont->mSpacing;
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
	if (theFont->mAlign == 1)
		x = x - aWidth;
	else if (theFont->mAlign == 2)
		x = x - aWidth / 2;
	uint8_t aChar = *aText;
	while (aChar != 0)
	{
		if (theFont->mEscapes == 1 && *aText == '^')
		{
			aChar = aText[1];
			aText = aText + 1;
			if (aChar == 0)
				break;
			if (theFont->mOutlining == 0)
			{
				theFont->mColourCode = (int)aChar;
				theFont->mPalette = ColourOf(theFont, (char)aChar);
			}
		}
		else
		{
			aChar = *aText;
			Image* aGlyph = theFont->mGlyphs[aChar];
			if (aGlyph == nullptr)
			{
				if (aChar == ' ')
					x = x + theFont->mSpacing + theFont->mSpaceWidth;
			}
			else
			{
				GrowRect(theRect, aFirst, aGlyph, x, y);
				Image_SetPalette(aGlyph, theFont->mPalette);
				if (aGlyph->mHasMask == 1 && 8 < theFont->mFmt->mBpp)
				{
					if (theOutlinePass == 0)
						Image_BlitAlphaFade(aGlyph, theFont->mDst, theFont->mDstPitch, theFont->mFmt,
							theFont->mClip, x, y, (uint32_t)theFont->mAlpha);
					else
					{
						Image_SetPalette(aGlyph->mMask, theFont->mPalette);
						Image_BlitAlphaFade(aGlyph->mMask, theFont->mDst, theFont->mDstPitch, theFont->mFmt,
							theFont->mClip, x, y, (uint32_t)theFont->mAlpha);
					}
				}
				else
					Image_BlitMasked(aGlyph, theFont->mDst, theFont->mDstPitch, theFont->mFmt, theFont->mClip, x, y);
				x = x + theFont->mGlyphs[*aText]->mOrigW + theFont->mSpacing;
			}
		}
		aText = aText + 1;
		aChar = *aText;
	}
	int aDstX, aDstY, aSrcX, aSrcY, aBlitWidth, aBlitHeight;
	if (ClipBlit(theFont->mClip[0], theFont->mClip[1], theFont->mClip[2], theFont->mClip[3], theRect[0], theRect[1],
			(theRect[2] - theRect[0]) + 1, (theRect[3] - theRect[1]) + 1, &aDstX, &aDstY, &aSrcX, &aSrcY, &aBlitWidth,
				&aBlitHeight) != 1)
	{
		theRect[0] = -1;
		theRect[1] = -1;
		theRect[2] = -1;
		theRect[3] = -1;
		return theRect;
	}
	theRect[0] = aDstX;
	theRect[2] = aDstX + aBlitWidth + -1;
	theRect[3] = aDstY + -1 + aBlitHeight;
	theRect[1] = aDstY;
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
	theFont->mHeight = aStrip->mH;
	int x = 0;
	int aCount = (int)std::strlen(theChars);
	int k = 0;
	if (0 < aCount)
	{
		do
		{
			int aSet = x < aStrip->mW ? Image_MaskColumnSet(aStrip, x) : 0;
			int aStart = x;
			while (aSet == 0 && aStart < aStrip->mW)
			{
				aStart = aStart + 1;
				aSet = Image_MaskColumnSet(aStrip, aStart);
			}
			int aWidth = 0;
			int aSet2 = aStart < aStrip->mW ? Image_MaskColumnSet(aStrip, aStart) : 0;
			x = aStart;
			while (aSet2 != 0 && x < aStrip->mW)
			{
				x = x + 1;
				aWidth = aWidth + 1;
				aSet2 = x < aStrip->mW ? Image_MaskColumnSet(aStrip, x) : 0;
			}
			ImageList_Append(&theFont->mOwned);
			Image* aGlyph = theFont->mOwned.mCurrent;
			Image_Create(aGlyph, aWidth, theFont->mHeight, &aStrip->mFmt);
			Copy8(aGlyph->mBits, 0, 0, aGlyph->mPitch, 0, aStrip->mBits, (int)(aStrip->mFmt.mBpp >> 3) * aStart, 0,
				aStrip->mPitch, 0, (int)(aStrip->mFmt.mBpp >> 3) * aWidth, theFont->mHeight);
			Image* aMask = new Image();
			aGlyph->mMask = aMask;
			aGlyph->mHasMask = 1;
			Image_Create(aMask, aWidth, theFont->mHeight, &aStrip->mMask->mFmt);
			aGlyph->mMask->mPalette = aStrip->mMask->mPalette;
			Image* aStripMask = aStrip->mMask;
			Copy8(aGlyph->mMask->mBits, 0, 0, aGlyph->mMask->mPitch, 0, aStripMask->mBits, aStart, 0,
				aStripMask->mPitch, 0, (int)(aStripMask->mFmt.mBpp >> 3) * aWidth, theFont->mHeight);
			Image* aMade = theFont->mOwned.mCurrent;
			theFont->mGlyphs[(uint8_t)theChars[k]] = aMade;
			int aBottom = aMade->mYOff + aMade->mH;
			if (theFont->mHeight < aBottom)
				theFont->mHeight = aBottom;
			k = k + 1;
		} while (k < aCount);
	}
	ImageList_Append(&theFont->mOwned);
	Image* aSpace = theFont->mOwned.mCurrent;
	Image_Create(aSpace, theFont->mOwned.mHead->mW / 2, theFont->mHeight, &aStrip->mFmt);
	Image* aMask = new Image();
	aSpace->mMask = aMask;
	aSpace->mHasMask = 1;
	Image_Create(aMask, theFont->mOwned.mHead->mW / 2, theFont->mHeight, &aStrip->mMask->mFmt);
	aSpace->mMask->mPalette = aStrip->mMask->mPalette;
	Image* aMade = theFont->mOwned.mCurrent;
	theFont->mGlyphs[' '] = aMade;
	int aBottom = aMade->mYOff + aMade->mH;
	if (theFont->mHeight < aBottom)
		theFont->mHeight = aBottom;
	return 1;
}

void Font_DrawOutlined(Font* theFont, int* theRect, int x, int y, const char* theText)
{
	int aRect[4];
	if (theFont->mOutline != nullptr)
	{
		Palette* aSaved = theFont->mPalette;
		theFont->mPalette = theFont->mOutline;
		theFont->mOutlining = 1;
		Font_DrawBounded(theFont, aRect, x, y + -1, theText, 1);
		Font_DrawBounded(theFont, aRect, x, y + 1, theText, 1);
		Font_DrawBounded(theFont, aRect, x + -1, y, theText, 1);
		Font_DrawBounded(theFont, aRect, x + 1, y, theText, 1);
		theFont->mOutlining = 0;
		theFont->mPalette = aSaved;
	}
	int* aDrawn = Font_DrawBounded(theFont, aRect, x, y, theText, 0);
	theRect[0] = aDrawn[0];
	theRect[1] = aDrawn[1];
	theRect[2] = aDrawn[2];
	theRect[3] = aDrawn[3];
	if (theFont->mOutline != nullptr)
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
	std::vsnprintf(theFont->mBuf, sizeof(theFont->mBuf), theFormat, anArgs);
	va_end(anArgs);
	Font_DrawOutlined(theFont, theRect, x, y, theFont->mBuf);
	return theRect;
}

// The images from the first whose name starts with the prefix's first letter to the bank's end, by position
void Font_LoadFromBank(Font* theFont, const Bank* theBank, const char* thePrefix)
{
	if (theBank == nullptr)
		return;
	size_t aSize = std::strlen(thePrefix);
	int aCount = (int)theBank->mImages.size();
	int i = 0;
	while (i != aCount)
	{
		if ((uint8_t)theBank->mImages[i]->mName[0] == (uint8_t)*thePrefix)
			break;
		i = i + 1;
	}
	if (i != aCount)
	{
		theFont->mHeight = 0;
		while (i != aCount)
		{
			const char* aName = theBank->mImages[i]->mName;
			if (std::strncmp(aName, thePrefix, aSize) == 0)
			{
				long aCode = std::atol(aName + aSize);
				Image* aGlyph = Bank_ImageAt(theBank, (uint32_t)i);
				if (aCode >= 0 && aCode < 256)
					theFont->mGlyphs[aCode] = aGlyph;
				int aBottom = aGlyph->mYOff + aGlyph->mH;
				if (theFont->mHeight < aBottom)
					theFont->mHeight = aBottom;
			}
			i = i + 1;
		}
	}
}

void Font_Clear(Font* theFont)
{
	std::memset(theFont->mGlyphs, 0, sizeof(theFont->mGlyphs));
	theFont->mSpaceWidth = 0;
	theFont->mSpacing = 0;
	theFont->mClip[0] = 0;
	theFont->mClip[1] = 0;
	theFont->mClip[2] = 0;
	theFont->mClip[3] = 0;
	ImageList* aList = &theFont->mOwned;
	if (aList->mCurrent != nullptr)
	{
		aList->mCurrent = aList->mHead;
		bool aMore;
		do
		{
			aMore = ImageList_RemoveCurrent(aList);
		} while (aMore);
	}
}

} // namespace r64
