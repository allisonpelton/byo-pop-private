// TipTop's user interface toolkit (TipTop.exe 0x427375-0x42cfae, and the glyph blits 0x40a792-0x40aaf9). See Gui.h.

#include "Gui.h"
#include "Game.h"
#include "GameText.h"
#include "Res.h"

#include <cstdlib>
#include <cstring>
#include <vector>

#include <SDL.h>

namespace tiptop
{

namespace
{

// The registered images (0x489f00): LoadSurface keeps where it stored each surface, to restore them (0x4274d9)
struct SurfaceNode
{
	SurfaceNode* mNext;        // +0x00
	int32_t mId;               // +0x04
	uint8_t mFlags;            // +0x08
	lowos::Surface** mOut;     // +0x0c
};

SurfaceNode* gSurfaces = nullptr;     // 0x489f00
char gToken[0x104];                   // 0x48a0a0

lowos::Surface* gSkin = nullptr;      // 0x48a1a8 the scroll bars' images
uint16_t gHandlerCount = 0;           // 0x48a1ac
uint16_t gLayerCount = 0;             // 0x48a1b0
Handler* gHandlers = nullptr;         // 0x48a1b4
Layer* gLayers = nullptr;             // 0x48a1b8
char gGuiInited = 0;                  // 0x48a1bc
Layer* gModalTop = nullptr;           // 0x48a1c0
int gFreeDepth = 0;                   // 0x48a1c4
uint8_t gDrawBit = 0;                 // 0x48a1c8 one bit per draw dispatch in progress
uint32_t gScrollTick = 0;             // 0x48a1cc
lowos::Surface* gSliderKnob = nullptr;  // 0x48a1d0
int gTextInputs = 0;                  // 0x48a1d4

// The scroll bars' parts in the skin image (0x47e2a0) and the screen (0x47e310)
enum
{
	SKIN_UP, SKIN_DOWN, SKIN_LEFT, SKIN_RIGHT, SKIN_UP_PRESSED, SKIN_DOWN_PRESSED, SKIN_LEFT_PRESSED,
	SKIN_RIGHT_PRESSED, SKIN_VTRACK, SKIN_HTRACK, SKIN_VTHUMB, SKIN_HTHUMB, SKIN_VLINE, SKIN_HLINE, SKIN_SCREEN
};
const GuiRect kSkin[] =
{
	{0, 1, 10, 9}, {0, 11, 10, 9}, {10, 10, 9, 10}, {20, 10, 9, 10}, {20, 1, 10, 9}, {30, 1, 10, 9},
	{37, 10, 9, 10}, {40, 0, 9, 10}, {10, 1, 10, 4}, {33, 10, 4, 10}, {10, 5, 10, 4}, {29, 10, 4, 10},
	{0, 10, 10, 1}, {19, 10, 1, 10}, {0, 0, 640, 480},
};
const uint16_t SCREEN_WIDTH = (uint16_t)kSkin[SKIN_SCREEN].w;   // 0x47e314
const uint16_t SCREEN_HEIGHT = (uint16_t)kSkin[SKIN_SCREEN].h;  // 0x47e316

void PlayRollover()
{
	if (gSounds[2] != nullptr)
		lowos::PlaySound(gSounds[2], (uint32_t)gSoundVolume << 16, 1);
}

void PlayClick()
{
	if (gSounds[3] != nullptr)
		lowos::PlaySound(gSounds[3], (uint32_t)gSoundVolume << 16, 1);
}

} // namespace

// --- images ---

// 0x427375
char LoadSurface(int theId, uint8_t theFlags, lowos::Surface** theSurface)
{
	*theSurface = nullptr;
	if (res::Load(0, 1, theId) != 0)
	{
		lowos::MessageBox_(GTC(0x46f138), GTC(0x46c318));
		return 1;
	}
	lowos::Image* anImage = (lowos::Image*)res::Get(0, theId);
	lowos::Surface* aSurface = lowos::RegisterSurface(anImage->mBits, anImage->mPitch, anImage->mHeight, theFlags);
	if (aSurface == nullptr)
		return 1;
	SurfaceNode** aNode;
	for (aNode = &gSurfaces; *aNode != nullptr; aNode = &(*aNode)->mNext)
	{
	}
	*aNode = (SurfaceNode*)malloc(sizeof(SurfaceNode));
	if (*aNode == nullptr)
	{
		FreeSurface(aSurface);
		return 1;
	}
	(*aNode)->mNext = nullptr;
	(*aNode)->mId = theId;
	(*aNode)->mFlags = theFlags;
	(*aNode)->mOut = theSurface;
	*theSurface = aSurface;
	return 0;
}

// 0x427451
void FreeSurface(lowos::Surface* theSurface)
{
	if (theSurface == nullptr)
		return;
	for (SurfaceNode** aNode = &gSurfaces; *aNode != nullptr; aNode = &(*aNode)->mNext)
	{
		if (*(*aNode)->mOut == theSurface)
		{
			SurfaceNode* aFree = *aNode;
			*(*aNode)->mOut = nullptr;
			lowos::UnregisterSurface(theSurface);
			res::Free(0, 1, aFree->mId);
			*aNode = aFree->mNext;
			free(aFree);
			return;
		}
	}
	lowos::MessageBox_(GTC(0x46f150), GTC(0x46c498));
}

// 0x4274d9: after the display lost its surfaces, reloads every image the game uses and registers them again.
// (The port's surfaces are never lost.) Returns 1 if one could not be restored, all are then freed.
char ReloadSurfaces()
{
	res::Entry* aTable = res::GameTable();
	SurfaceNode** aNode = &gSurfaces;
	for (int i = 0; i < gPreloaded && aTable[i].mFlags != 0; i++)
	{
		if (aTable[i].mType == res::TYPE_JPEG || aTable[i].mType == res::TYPE_PCX)
			res::Free(0, 1, aTable[i].mId);
	}
	while (*aNode != nullptr)
	{
		int anIndex = res::IndexOf(0, (*aNode)->mId);
		uint32_t aRefs = (uint32_t)aTable[anIndex].mRefCount;
		for (uint32_t i = 0; i < aRefs; i++)
			res::Free(0, 1, (*aNode)->mId);
		for (uint32_t i = 0; i < aRefs; i++)
		{
			if (res::Load(0, 1, (*aNode)->mId) != 0)
				goto fail;
		}
		{
			lowos::Image* anImage = (lowos::Image*)res::Get(0, (*aNode)->mId);
			lowos::Surface* aSurface = lowos::RegisterSurface(anImage->mBits, anImage->mPitch, anImage->mHeight,
				(*aNode)->mFlags);
			if (aSurface == nullptr)
			{
				res::Free(0, 1, (*aNode)->mId);
				goto fail;
			}
			*(*aNode)->mOut = aSurface;
		}
		aNode = &(*aNode)->mNext;
		lowos::Pump();
	}
	for (int i = 0; i < gPreloaded; i++)
	{
		if (aTable[i].mFlags == 0)
			return 0;
		if (aTable[i].mType == res::TYPE_JPEG || aTable[i].mType == res::TYPE_PCX)
			res::Load(0, 1, aTable[i].mId);
	}
	return 0;

fail:
	while (*aNode != nullptr)
	{
		SurfaceNode* aFree = *aNode;
		*aNode = aFree->mNext;
		free(aFree);
	}
	while (gSurfaces != nullptr)
		FreeSurface(*gSurfaces->mOut);
	return 1;
}

// --- the configuration tokenizer ---

// 0x4280b1: 1 for a character that ends a token (letters, digits and \/:_.^-@#! make up tokens)
int IsSeparator(char c)
{
	signed char ch = (signed char)c;
	if (ch >= 'a' && ch <= 'z')
		return 0;
	if (ch >= 'A' && ch <= 'Z')
		return 0;
	if (ch >= '0' && ch <= '9')
		return 0;
	if (ch == '\\' || ch == '/' || ch == ':' || ch == '_' || ch == '.' || ch == '^' || ch == '-' || ch == '@' ||
		ch == '#' || ch == '!')
		return 0;
	return 1;
}

// 0x428186: skips separators and ; comments. 1 at the end of the text (or of the line, if stopAtLineEnd), 0 at a
// token.
int SkipToToken(const char** theText, char stopAtLineEnd)
{
	for (;;)
	{
		if (**theText == 0)
			return 1;
		if (**theText == ';')
		{
			do
			{
				(*theText)++;
				if (**theText == 0)
					return 1;
			} while (**theText != '\n' && **theText != '\r');
		}
		if (stopAtLineEnd != 0)
		{
			if (**theText == '\n')
				return 1;
			if (**theText == '\r')
				return 1;
		}
		if (IsSeparator(**theText) == 0)
			return 0;
		if (**theText == '\n')
		{
			if ((*theText)[1] == '\r')
				(*theText)++;
		}
		else if (**theText == '\r' && (*theText)[1] == '\n')
		{
			(*theText)++;
		}
		(*theText)++;
	}
}

// 0x42828e: skips past the next theChar outside comments. 2 at the end of the text, else 1 if a token was
// skipped on the way.
int SkipPast(const char** theText, char theChar)
{
	int aSkipped = 0;
	for (;;)
	{
		if (**theText == 0)
			return 2;
		if (**theText == ';')
		{
			do
			{
				(*theText)++;
				if (**theText == 0)
					return 2;
			} while (**theText != '\n' && **theText != '\r');
		}
		if (IsSeparator(**theText) == 0)
			aSkipped = 1;
		if (**theText == theChar)
			break;
		if (**theText == '\n')
		{
			if ((*theText)[1] == '\r')
				(*theText)++;
		}
		else if (**theText == '\r' && (*theText)[1] == '\n')
		{
			(*theText)++;
		}
		(*theText)++;
	}
	(*theText)++;
	return aSkipped;
}

// 0x42838f
char TokenChar(int theIndex)
{
	return gToken[theIndex];
}

// 0x42839d
const char* Token()
{
	return gToken;
}

// 0x4283a7: reads the token at the text (one too long reads as empty)
void ReadToken(const char** theText)
{
	int i = 0;
	while (IsSeparator(**theText) == 0 && **theText != 0)
	{
		gToken[i] = **theText;
		(*theText)++;
		i++;
		if (i == 0x103)
		{
			gToken[0] = 0;
			return;
		}
	}
	gToken[i] = 0;
}

// 0x428413
int32_t TokenInt()
{
	return atol(gToken);
}

// 0x428423
double TokenFloat()
{
	return atof(gToken);
}

// --- fonts and text ---

// 0x40a792: the rect (x, y, theWidth, theHeight) clipped to theBounds into theOut; 0 if nothing is left
char ClipRect(int16_t x, int16_t y, int16_t theWidth, int16_t theHeight, const GuiRect* theBounds, GuiRect* theOut)
{
	if (x < theBounds->x)
	{
		theWidth = (int16_t)(theWidth + (x - theBounds->x));
		if (theWidth < 0)
			return 0;
		x = theBounds->x;
	}
	if (y < theBounds->y)
	{
		theHeight = (int16_t)(theHeight + (y - theBounds->y));
		if (theHeight < 0)
			return 0;
		y = theBounds->y;
	}
	int16_t anOver = (int16_t)((x + theWidth) - (theBounds->w + theBounds->x));
	if (anOver > 0)
	{
		if (theWidth <= anOver)
			return 0;
		theWidth = (int16_t)(theWidth - anOver);
	}
	anOver = (int16_t)((y + theHeight) - (theBounds->h + theBounds->y));
	if (anOver > 0)
	{
		if (theHeight <= anOver)
			return 0;
		theHeight = (int16_t)(theHeight - anOver);
	}
	theOut->x = x;
	theOut->y = y;
	theOut->w = theWidth;
	theOut->h = theHeight;
	return 1;
}

// 0x40a8e0: a glyph (theSrc in the font's image) to the screen at (x, y), clipped (0: the screen); theLevel 0xff
// opaque, 0 invisible
void BlitGlyph(lowos::Surface* theSurface, int16_t x, int16_t y, const GuiRect* theSrc, const GuiRect* theClip,
	uint8_t theLevel)
{
	if (theLevel == 0)
		return;
	GuiRect aGlyph = *theSrc;
	GuiRect aScreen;
	if (theClip == nullptr)
	{
		theClip = &aScreen;
		aScreen.x = 0;
		aScreen.y = 0;
		lowos::GetDisplayFormat(nullptr, (uint16_t*)&aScreen.w, (uint16_t*)&aScreen.h);
	}
	if (x < theClip->x)
	{
		aGlyph.w = (int16_t)(aGlyph.w + (x - theClip->x));
		if (aGlyph.w < 0)
			return;
		aGlyph.x = (int16_t)(aGlyph.x - (x - theClip->x));
		x = theClip->x;
	}
	if (y < theClip->y)
	{
		aGlyph.h = (int16_t)(aGlyph.h + (y - theClip->y));
		if (aGlyph.h < 0)
			return;
		aGlyph.y = (int16_t)(aGlyph.y - (y - theClip->y));
		y = theClip->y;
	}
	int16_t anOver = (int16_t)((x + aGlyph.w) - (theClip->w + theClip->x));
	if (anOver > 0)
	{
		if ((int)(uint16_t)aGlyph.w <= (int)anOver)
			return;
		aGlyph.w = (int16_t)(aGlyph.w - anOver);
	}
	anOver = (int16_t)((y + aGlyph.h) - (theClip->h + theClip->y));
	if (anOver > 0)
	{
		if ((int)(uint16_t)aGlyph.h <= (int)anOver)
			return;
		aGlyph.h = (int16_t)(aGlyph.h - anOver);
	}
	if (theLevel == 0xff)
		lowos::BlitRegistered((uint16_t)aGlyph.w, (uint16_t)aGlyph.h, x, y, aGlyph.x, aGlyph.y, theSurface);
	else
		lowos::BlitLevel((uint16_t)aGlyph.w, (uint16_t)aGlyph.h, x, y, aGlyph.x, aGlyph.y, theSurface, theLevel);
}

// 0x40aaf9: a glyph into a pixel buffer of the display's format, clipped to theClip
void GlyphToBuffer(lowos::Surface* theSurface, uint8_t* theDest, uint32_t theDestPitch, int16_t x, int16_t y,
	const GuiRect* theSrc, const GuiRect* theClip, uint8_t theLevel)
{
	if (theLevel == 0)
		return;
	lowos::PixelFormat aDisplay;
	lowos::PixelFormat aLogical;
	uint8_t aBytes = lowos::GetDisplayFormat(&aDisplay, nullptr, nullptr);
	lowos::GetLogicalFormat(&aLogical, nullptr, nullptr);
	GuiRect aGlyph = *theSrc;
	if (x < theClip->x)
	{
		aGlyph.w = (int16_t)(aGlyph.w + (x - theClip->x));
		if (aGlyph.w < 0)
			return;
		aGlyph.x = (int16_t)(aGlyph.x - (x - theClip->x));
		x = theClip->x;
	}
	if (y < theClip->y)
	{
		aGlyph.h = (int16_t)(aGlyph.h + (y - theClip->y));
		if (aGlyph.h < 0)
			return;
		aGlyph.y = (int16_t)(aGlyph.y - (y - theClip->y));
		y = theClip->y;
	}
	int16_t anOver = (int16_t)((x + aGlyph.w) - (theClip->w + theClip->x));
	if (anOver > 0)
	{
		if ((int)(uint16_t)aGlyph.w <= (int)anOver)
			return;
		aGlyph.w = (int16_t)(aGlyph.w - anOver);
	}
	anOver = (int16_t)((y + aGlyph.h) - (theClip->h + theClip->y));
	if (anOver > 0)
	{
		if ((int)(uint16_t)aGlyph.h <= (int)anOver)
			return;
		aGlyph.h = (int16_t)(aGlyph.h - anOver);
	}
	uint8_t aFlags = (uint8_t)((lowos::SurfaceFlags(theSurface) & 10) | lowos::SURF_TRANSLUCENT);
	uint32_t aWidth = lowos::SurfaceWidth(theSurface) & 0xffff;
	// the original passes the color key in the logical format with that format, and the blitter converts it
	uint32_t aKey = lowos::ConvertColor(&aLogical, &aDisplay, 0xfefe01);
	lowos::BlitPixels(&aDisplay, &aDisplay, theDest + x * aBytes + y * theDestPitch,
		lowos::SurfaceBits(theSurface) + aWidth * aBytes * aGlyph.y + aBytes * aGlyph.x, (uint16_t)aGlyph.h, (uint16_t)aGlyph.w,
		theDestPitch, aWidth * aBytes, aFlags, aBytes, aBytes, aKey, theLevel);
}

// 0x4285ce: a font from an image of one row of glyphs separated by columns of the bottom-left pixel's color, in
// the order of theChars
Font* LoadFont(int theImageId, const char* theChars, uint8_t theSpaceWidth, uint8_t theSpacing, uint8_t theFlags)
{
	Font* aFont = (Font*)malloc(sizeof(Font));
	if (aFont == nullptr)
		NoMemory();
	memset(aFont, 0, sizeof(Font));
	aFont->mImageId = theImageId;
	if (LoadSurface(theImageId, (uint8_t)(((theFlags & 0x40) != 0 ? lowos::SURF_COLORKEY : 0) | 1),
			&aFont->mSurface) != 0)
		NoMemory();
	aFont->mSpaceWidth = theSpaceWidth;
	aFont->mSpacing = theSpacing;
	uint16_t aHeight = lowos::SurfaceHeight(aFont->mSurface);
	uint32_t aSeparator = lowos::GetPixel(aFont->mSurface, 0, aHeight - 1);
	uint16_t x = 0;
	uint16_t aWidth = (uint16_t)lowos::SurfaceWidth(aFont->mSurface);
	uint16_t aRows = lowos::SurfaceHeight(aFont->mSurface);
	for (uint32_t i = 0;; i++)
	{
		uint8_t aChar = (uint8_t)theChars[i];
		if (aChar == 0)
			return aFont;
		aFont->mGlyphs[aChar].mX = x;
		if (x == aWidth)
			lowos::MessageBox_(GTC(0x46f1e0), GTC(0x46c498));
		// the glyph ends at the next column of the separator's color
		while (++x != aWidth)
		{
			uint16_t y = 0;
			for (;;)
			{
				if (y == (uint16_t)(aRows - 1))
					goto found;
				if (lowos::GetPixel(aFont->mSurface, x, y) != aSeparator)
					break;
				y++;
			}
		}
	found:
		aFont->mGlyphs[aChar].mWidth = (uint8_t)(x - aFont->mGlyphs[aChar].mX);
		// and the next one starts at the next column that is not
		while (x != aWidth)
		{
			x++;
			for (uint16_t y = 0; y < (uint16_t)(aRows - 1); y++)
			{
				if (lowos::GetPixel(aFont->mSurface, x, y) != aSeparator)
					goto next;
			}
		}
	next:;
	}
}

// 0x4287fb
void FreeFont(Font* theFont)
{
	if (theFont != nullptr)
	{
		FreeSurface(theFont->mSurface);
		free(theFont);
	}
}

// 0x42881c
const char* DrawText(Font* theFont, uint8_t* theDest, uint32_t theDestPitch, const char* theText,
	const GuiRect* theClip, GuiRect* theExtent, int16_t theX, int16_t theY, uint8_t theLevel, uint8_t theFlags)
{
	const uint8_t* aPos = (const uint8_t*)theText;
	GuiRect aClip;
	aClip.x = 0;
	aClip.y = 0;
	lowos::GetDisplayFormat(nullptr, (uint16_t*)&aClip.w, (uint16_t*)&aClip.h);
	if (theClip == nullptr)
		theClip = &aClip;
	GuiRect aGlyph;  // the line's extent first, then each glyph's place in the font image
	if ((theFlags & 0x2c) != 0)
	{
		DrawText(theFont, nullptr, 0, theText, nullptr, &aGlyph, 0, 0, 0, (uint8_t)((theFlags & 0xd3) | 2));
		if ((theFlags & 4) == 0)
		{
			if ((theFlags & 0x20) != 0)
				theX = (int16_t)(theX - aGlyph.w);
		}
		else
		{
			theX = (int16_t)(theClip->x + (int16_t)((int)((uint32_t)(uint16_t)theClip->w - (uint32_t)(uint16_t)aGlyph.w) >> 1));
		}
		if ((theFlags & 8) != 0)
			theY = (int16_t)(theClip->y + (int16_t)((int)((uint32_t)(uint16_t)theClip->h - (uint32_t)(uint16_t)aGlyph.h) >> 1));
	}
	if (theClip != &aClip)
	{
		if (ClipRect(theClip->x, theClip->y, theClip->w, theClip->h, &aClip, &aClip) == 0)
			aClip.w = 0;
		theClip = &aClip;
	}
	if (theExtent != nullptr)
	{
		theExtent->x = theX;
		theExtent->y = theY;
		theExtent->h = (int16_t)(lowos::SurfaceHeight(theFont->mSurface) - 1);
		theExtent->w = 0;
	}
	int16_t aRight = (int16_t)(theClip->x + theClip->w);
	int aWord = 0;  // the characters left of the word that wrapping measured
	aGlyph.y = 0;
	aGlyph.h = (int16_t)(lowos::SurfaceHeight(theFont->mSurface) - 1);
	for (;;)
	{
		if (*aPos == 0)
			return nullptr;
		if (*aPos == '\n')
		{
			if (aPos[1] != 0)
				return (const char*)aPos + 1;
			return nullptr;
		}
		if ((theFlags & 1) != 0)
		{
			if (aWord == 0)
			{
				// at a word: does it fit on the line? (one of 15 characters or more is cut where the line ends)
				int16_t anEnd = theX;
				if (theFont->mGlyphs[*aPos].mWidth == 0)
				{
					do
					{
						if (theFont->mGlyphs[aPos[aWord]].mWidth == 0)
							anEnd = (int16_t)(anEnd + theFont->mSpaceWidth + theFont->mSpacing);
						else
							anEnd = (int16_t)(anEnd + theFont->mGlyphs[aPos[aWord]].mWidth + theFont->mSpacing);
						aWord++;
						if (aPos[aWord] == 0)
							return nullptr;
						if (aPos[aWord] == '\n')
						{
							if (aPos[aWord + 1] != 0)
								return (const char*)aPos + aWord + 1;
							return nullptr;
						}
					} while (theFont->mGlyphs[aPos[aWord]].mWidth == 0);
					if (aRight <= anEnd)
						return (const char*)aPos + aWord;
				}
				else
				{
					do
					{
						if (theFont->mGlyphs[aPos[aWord]].mWidth == 0)
							anEnd = (int16_t)(anEnd + theFont->mSpaceWidth + theFont->mSpacing);
						else
							anEnd = (int16_t)(anEnd + theFont->mGlyphs[aPos[aWord]].mWidth + theFont->mSpacing);
						aWord++;
						if (aRight <= anEnd)
						{
							if (aWord < 0xf)
								aWord = 0;
							break;
						}
					} while (theFont->mGlyphs[aPos[aWord]].mWidth != 0);
				}
			}
			if (aWord == 0)
				return (const char*)aPos;
		}
		if (aRight <= theX)
			return (const char*)aPos;
		aGlyph.w = theFont->mGlyphs[*aPos].mWidth;
		if (aGlyph.w != 0)
		{
			aGlyph.x = (int16_t)theFont->mGlyphs[*aPos].mX;
			if ((theFlags & 2) == 0)
			{
				uint8_t aLevel = (theFlags & 0x10) != 0 ? theLevel : 0xff;
				if (theDest == nullptr)
				{
					BlitGlyph(theFont->mSurface, theX, theY, &aGlyph, theClip, aLevel);
				}
				else
				{
					// the clip is always the local one here, so a glyph drawn into a buffer is clipped to itself
					if (theClip == &aClip)
					{
						aClip.x = theX;
						aClip.y = theY;
						aClip.h = aGlyph.h;
						aClip.w = aGlyph.w;
					}
					GlyphToBuffer(theFont->mSurface, theDest, theDestPitch, theX, theY, &aGlyph, theClip, aLevel);
				}
			}
			theX = (int16_t)(theX + aGlyph.w);
			if (theExtent != nullptr)
				theExtent->w = (int16_t)(theExtent->w + aGlyph.w);
		}
		aPos++;
		aWord--;
		uint16_t anAdvance = theFont->mSpacing;
		if (aGlyph.w == 0)
			anAdvance = (uint16_t)(anAdvance + theFont->mSpaceWidth);
		theX = (int16_t)(theX + anAdvance);
		if (theExtent != nullptr)
			theExtent->w = (int16_t)(theExtent->w + anAdvance);
	}
}

// --- rectangles ---

// 0x428d6d: grows a rectangle by 8 on each side, within the screen
void InflateDirty(GuiRect* theRect)
{
	if (theRect->x < 9)
		theRect->x = 0;
	else
		theRect->x = (int16_t)(theRect->x - 8);
	if (theRect->y < 9)
		theRect->y = 0;
	else
		theRect->y = (int16_t)(theRect->y - 8);
	theRect->w = (int16_t)(theRect->w + 0x10);
	theRect->h = (int16_t)(theRect->h + 0x10);
	if ((int)SCREEN_WIDTH < (int)((uint32_t)(uint16_t)theRect->w + (int)theRect->x))
		theRect->w = (int16_t)(SCREEN_WIDTH - theRect->x);
	if ((int)SCREEN_HEIGHT < (int)((uint32_t)(uint16_t)theRect->h + (int)theRect->y))
		theRect->h = (int16_t)(SCREEN_HEIGHT - theRect->y);
}

// 0x428e40
int PointInRect(uint16_t x, uint16_t y, const GuiRect* theRect)
{
	if ((int)x < (int)theRect->x || (int)((int)theRect->x + (uint32_t)(uint16_t)theRect->w) <= (int)x ||
		(int)y < (int)theRect->y || (int)((int)theRect->y + (uint32_t)(uint16_t)theRect->h) <= (int)y)
		return 0;
	return 1;
}

// 0x428e98: how far a layer's content is scrolled down
uint16_t LayerScrollY(Layer* theLayer)
{
	if (theLayer->mVScroll == nullptr)
		return 0;
	return theLayer->mVScroll->u.scroll.mPos;
}

// --- layers and handlers ---

// 0x428eb5
void GuiInit(uint16_t theLayers, uint16_t theHandlers, lowos::Surface* theSkin)
{
	gGuiInited = 1;
	uint16_t aHandlers = (uint16_t)(theHandlers + theLayers * 2);
	// one block in the original, the handlers after the layers
	gLayers = (Layer*)calloc(theLayers, sizeof(Layer));
	gHandlers = (Handler*)calloc(aHandlers, sizeof(Handler));
	if (gLayers == nullptr || gHandlers == nullptr)
		NoMemory();
	gLayerCount = theLayers;
	gSkin = theSkin;
	gHandlerCount = aHandlers;
	// (and a heap for the draw dispatch's lists, 0x43b270 and 0x43a3e0: the port uses a vector)
}

// 0x428fd3
Layer* NewLayer(uint16_t theContentWidth, uint16_t theContentHeight, const GuiRect* theRect)
{
	Layer* aLayer = gLayers;
	uint16_t aLeft = gLayerCount;
	do
	{
		if (aLayer->mRect.w == 0)
		{
			memmove(&aLayer->mRect, theRect, sizeof(GuiRect));
			aLayer->mVScroll = nullptr;
			aLayer->mHScroll = nullptr;
			aLayer->mDirty = 1;
			aLayer->mChild = nullptr;
			aLayer->mParent = nullptr;
			aLayer->mNext = nullptr;
			aLayer->mPrev = nullptr;
			aLayer->mModalBelow = nullptr;
			aLayer->mModalAbove = nullptr;
			SetLayerContent(aLayer, theContentWidth, theContentHeight);
			return aLayer;
		}
		aLayer++;
		aLeft--;
	} while (aLeft != 0);
	return nullptr;
}

// 0x42907f: makes a layer the top modal one: only handlers inside it get input
void PushModal(Layer* theLayer)
{
	theLayer->mModalBelow = gModalTop;
	if (theLayer->mModalBelow != nullptr)
		theLayer->mModalBelow->mModalAbove = theLayer;
	gModalTop = theLayer;
	for (uint32_t i = 0; i < gHandlerCount; i++)
	{
		Handler* h = &gHandlers[i];
		if (h->mProc != nullptr)
		{
			char isIn = IsInside(h->mLayer, gModalTop);
			char wasIn = IsInside(h->mLayer, theLayer->mModalBelow);
			if (isIn != 0 && wasIn == 0)
				Send(h, MSG_UNCOVERED, 0, 0);
			if (isIn == 0 && wasIn != 0)
				Send(h, MSG_COVERED, 0, 0);
		}
	}
}

// 0x42917f: whether a layer is inside the top modal layer (any layer is when there is none)
char InTopModal(Layer* theLayer)
{
	if (theLayer == nullptr)
		return 0;
	if (gModalTop == nullptr)
		return 1;
	while (theLayer != nullptr)
	{
		if (theLayer == gModalTop)
			return 1;
		theLayer = theLayer->mParent;
	}
	return 0;
}

// 0x4291bf: whether a layer is theAncestor or inside it (any layer is inside none)
char IsInside(Layer* theLayer, Layer* theAncestor)
{
	if (theLayer == nullptr)
		return 0;
	if (theAncestor == nullptr)
		return 1;
	while (theLayer != nullptr)
	{
		if (theLayer == theAncestor)
			return 1;
		theLayer = theLayer->mParent;
	}
	return 0;
}

// 0x4291f9: frees a layer with its handlers and child layers
void FreeLayer(Layer* theLayer)
{
	if (theLayer == nullptr)
		return;
	static Layer* sOldTop;  // the original's local of the outermost call
	if (gFreeDepth == 0)
		sOldTop = gModalTop;
	gFreeDepth++;
	Handler* h = gHandlers;
	for (uint16_t n = gHandlerCount; n != 0; n--)
	{
		if (h->mLayer == theLayer)
			FreeHandler(h);
		h++;
	}
	theLayer->mRect.w = 0;
	if (theLayer == gModalTop)
		gModalTop = theLayer->mModalBelow;
	if (theLayer->mModalBelow != nullptr)
		theLayer->mModalBelow->mModalAbove = theLayer->mModalAbove;
	if (theLayer->mModalAbove != nullptr)
		theLayer->mModalAbove->mModalBelow = theLayer->mModalBelow;
	if (theLayer->mParent != nullptr)
	{
		if (theLayer->mParent->mChild == theLayer)
		{
			theLayer->mParent->mChild = nullptr;
			if (theLayer->mNext != nullptr)
			{
				theLayer->mParent->mChild = theLayer->mNext;
				theLayer->mNext->mPrev = theLayer->mPrev;
			}
			if (theLayer->mPrev != nullptr)
			{
				theLayer->mParent->mChild = theLayer->mPrev;
				theLayer->mPrev->mNext = theLayer->mNext;
			}
		}
		else
		{
			if (theLayer->mNext != nullptr)
				theLayer->mNext->mPrev = theLayer->mPrev;
			if (theLayer->mPrev != nullptr)
				theLayer->mPrev->mNext = theLayer->mNext;
		}
	}
	while (theLayer->mChild != nullptr)
		FreeLayer(theLayer->mChild);
	gFreeDepth--;
	if (gFreeDepth == 0 && sOldTop != gModalTop)
	{
		for (uint32_t i = 0; i < gHandlerCount; i++)
		{
			Handler* g = &gHandlers[i];
			if (g->mProc != nullptr)
			{
				char isIn = IsInside(g->mLayer, gModalTop);
				char wasIn = IsInside(g->mLayer, sOldTop);
				if (isIn != 0 && wasIn == 0)
					Send(g, MSG_UNCOVERED, 0, 0);
				if (isIn == 0 && wasIn != 0)
					Send(g, MSG_COVERED, 0, 0);
			}
		}
	}
}

// 0x42946d
void AddChildLayer(Layer* theParent, Layer* theChild)
{
	theChild->mNext = theParent->mChild;
	if (theParent->mChild != nullptr)
		theParent->mChild->mPrev = theChild;
	theChild->mPrev = nullptr;
	theChild->mParent = theParent;
	theParent->mChild = theChild;
}

// 0x4294ac
void FreeAllLayers()
{
	for (uint32_t i = 0; i < gLayerCount; i++)
	{
		if (gLayers[i].mRect.w != 0)
			FreeLayer(&gLayers[i]);
	}
}

// 0x4294f7: a handler of theProc on a layer (0: none), in theRect (0: the screen). Its procedure gets MSG_CLASSINIT
// first if no handler of it exists, then MSG_CREATE with theA and theB.
Handler* NewHandler(HandlerProc theProc, Layer* theLayer, const GuiRect* theRect, uintptr_t theA, uintptr_t theB)
{
	Handler* aHandler = gHandlers;
	uint16_t aLeft = gHandlerCount;
	while (aHandler->mProc != nullptr)
	{
		aHandler++;
		aLeft--;
		if (aLeft == 0)
			return nullptr;
	}
	uint16_t i = 0;
	do
	{
		if (gHandlers[i].mProc == theProc)
			goto known;
		i++;
	} while (i != gHandlerCount);
	theProc(MSG_CLASSINIT, 0, 0, nullptr);
known:
	aHandler->mProc = theProc;
	aHandler->mLayer = theLayer;
	if (theLayer != nullptr)
		theLayer->mDirty = 1;
	if (theRect == nullptr)
	{
		aHandler->mRect.x = 0;
		aHandler->mRect.y = 0;
		aHandler->mRect.w = (int16_t)SCREEN_WIDTH;
		aHandler->mRect.h = (int16_t)SCREEN_HEIGHT;
	}
	else
	{
		memmove(&aHandler->mRect, theRect, sizeof(GuiRect));
	}
	aHandler->mPass = 0;
	aHandler->mPending = 0;
	theProc(MSG_CREATE, theA, theB, aHandler);
	return aHandler;
}

// 0x429611: the procedure gets MSG_DESTROY, and MSG_CLASSFREE if it was its last handler
void FreeHandler(Handler* theHandler)
{
	if (theHandler == nullptr || theHandler->mProc == nullptr)
		return;
	theHandler->mProc(MSG_DESTROY, 0, 0, theHandler);
	HandlerProc aProc = theHandler->mProc;
	theHandler->mProc = nullptr;
	uint16_t i = 0;
	do
	{
		if (gHandlers[i].mProc == aProc)
			return;
		i++;
	} while (i != gHandlerCount);
	aProc(MSG_CLASSFREE, 0, 0, nullptr);
}

// Test hooks (testhooks/Autoplay.cpp): the handler table, free slots included
Handler* HandlerTable(uint16_t* theCount)
{
	*theCount = gHandlerCount;
	return gHandlers;
}

// 0x429692
Handler* NextHandler(Handler* theHandler)
{
	do
	{
		if (theHandler == &gHandlers[gHandlerCount - 1])
			theHandler = gHandlers;
		else
			theHandler++;
	} while (theHandler->mProc == nullptr);
	return theHandler;
}

// 0x4296d1
Handler* PrevHandler(Handler* theHandler)
{
	do
	{
		if (theHandler == gHandlers)
			theHandler = &gHandlers[gHandlerCount - 1];
		else
			theHandler--;
	} while (theHandler->mProc == nullptr);
	return theHandler;
}

// 0x429711: sends a message to every handler. Handlers created meanwhile do not get it. Input goes only to the
// handlers inside the top modal layer; drawing goes in six passes (Handler::mPass), and with modal layers from the
// handlers outside them up to those of the top one.
void Dispatch(uint32_t theMsg, uintptr_t theA, uintptr_t theB)
{
	if (gGuiInited == 0)
		return;
	uint8_t aMsg = (uint8_t)theMsg;
	bool isInput = aMsg == MSG_MOUSEDOWN || aMsg == MSG_MOUSEMOVE || aMsg == MSG_MOUSEUP || aMsg == MSG_KEY ||
		aMsg == MSG_PASTE || aMsg == MSG_TAB || aMsg == MSG_BACKTAB || aMsg == MSG_CHAR;
	if (aMsg == MSG_DRAW)
	{
		if (gDrawBit == 0)
			gDrawBit = 1;
		else
			gDrawBit = (uint8_t)(gDrawBit << 1);
	}
	Handler* aHandler = gHandlers;
	for (uint16_t n = gHandlerCount; n != 0; n--)
	{
		if (aHandler->mProc != nullptr)
		{
			if (isInput && gModalTop != nullptr)
			{
				if (IsInside(aHandler->mLayer, gModalTop) != 0)
					aHandler->mPending++;
			}
			else
			{
				aHandler->mPending++;
			}
		}
		aHandler++;
	}
	if (aMsg == MSG_DRAW && gModalTop != nullptr)
	{
		// a list of groups, each ended by 0: the handlers of the top modal layer, of the one below it, ..., and of
		// every other layer (handlers without a layer are in none)
		std::vector<Handler*> aList;
		aList.push_back(nullptr);
		Layer* aLayer = gModalTop;
		for (;;)
		{
			aHandler = gHandlers;
			for (uint16_t n = gHandlerCount; n != 0; n--)
			{
				if (aHandler->mProc != nullptr && aHandler->mPending != 0 && (aHandler->mVisited & gDrawBit) == 0 &&
					IsInside(aHandler->mLayer, aLayer) != 0)
				{
					aHandler->mVisited |= gDrawBit;
					aList.push_back(aHandler);
				}
				aHandler++;
			}
			aList.push_back(nullptr);
			if (aLayer == nullptr)
				break;
			aLayer = aLayer->mModalBelow;
		}
		Handler** aBase = aList.data();
		Handler** anEnd = aBase + aList.size() - 1;
		Handler** aGroup;
		do
		{
			do
			{
				aGroup = anEnd;
				anEnd = aGroup - 1;
			} while (*anEnd != nullptr);
			for (uint8_t aPass = 0; aPass < 6; aPass++)
			{
				for (Handler** p = aGroup; *p != nullptr; p++)
				{
					aHandler = *p;
					if (aHandler->mProc != nullptr && aHandler->mPending != 0 && aPass == aHandler->mPass)
					{
						uintptr_t anA = theA;
						if (aHandler->mLayer != nullptr && aHandler->mLayer->mDirty != 0)
							anA = 1;
						aHandler->mPending--;
						aHandler->mVisited &= (uint8_t)~gDrawBit;
						aHandler->mProc(theMsg, anA, theB, aHandler);
					}
				}
			}
		} while (anEnd != aBase);
	}
	else
	{
		for (uint8_t aPass = 0; aPass < 6; aPass++)
		{
			aHandler = gHandlers;
			for (uint16_t n = gHandlerCount; n != 0; n--)
			{
				if (aHandler->mProc != nullptr && aHandler->mPending != 0)
				{
					if (!isInput || InTopModal(aHandler->mLayer) != 0)
					{
						uintptr_t anA = theA;
						if (aMsg == MSG_DRAW)
						{
							if (aPass != aHandler->mPass)
								goto next;
							if (aHandler->mLayer != nullptr && aHandler->mLayer->mDirty != 0)
								anA = 1;
						}
						aHandler->mProc(theMsg, anA, theB, aHandler);
					}
					aHandler->mPending--;
				}
			next:
				aHandler++;
			}
			if (aMsg != MSG_DRAW)
				break;
		}
	}
	if (aMsg == MSG_DRAW)
	{
		Layer* aLayer = gLayers;
		for (uint16_t n = gLayerCount; n != 0; n--)
		{
			aLayer->mDirty = 0;
			aLayer++;
		}
		gDrawBit = (uint8_t)(gDrawBit >> 1);
	}
}

// 0x429c75
uint32_t Send(Handler* theHandler, uint32_t theMsg, uintptr_t theA, uintptr_t theB)
{
	if (theHandler == nullptr)
		return 0;
	if (theHandler->mProc == nullptr)
		return 0;
	return theHandler->mProc(theMsg, theA, theB, theHandler);
}

// 0x429ca4: the size of a layer's content; scroll bars come and go as it needs them, inside its rectangle
void SetLayerContent(Layer* theLayer, uint16_t theWidth, uint16_t theHeight)
{
	theLayer->mContentWidth = theWidth;
	theLayer->mContentHeight = theHeight;
	bool aVertical = false;
	if (theLayer->mHScroll != nullptr)
		theLayer->mRect.h = (int16_t)(theLayer->mRect.h + kSkin[SKIN_UP].w);
	if (theLayer->mVScroll != nullptr)
		theLayer->mRect.w = (int16_t)(theLayer->mRect.w + kSkin[SKIN_UP].w);
	bool aHorizontal = (uint16_t)theLayer->mRect.w < theWidth;
	if (aHorizontal)
		theLayer->mRect.h = (int16_t)(theLayer->mRect.h - kSkin[SKIN_UP].w);
	if ((uint16_t)theLayer->mRect.h < theHeight)
	{
		aVertical = true;
		theLayer->mRect.w = (int16_t)((uint16_t)theLayer->mRect.w - (uint16_t)kSkin[SKIN_UP].w);
		if (!aHorizontal && (uint16_t)theLayer->mRect.w < theWidth)
		{
			aHorizontal = true;
			theLayer->mRect.h = (int16_t)((uint16_t)theLayer->mRect.h - (uint16_t)kSkin[SKIN_UP].w);
		}
	}
	GuiRect aRect;
	if (aHorizontal)
	{
		aRect.x = theLayer->mRect.x;
		aRect.y = (int16_t)(theLayer->mRect.y + theLayer->mRect.h);
		aRect.w = theLayer->mRect.w;
		aRect.h = kSkin[SKIN_UP].w;
		theLayer->mHScroll = NewHandler(ScrollBarProc, theLayer, &aRect, theWidth, 0);
	}
	else
	{
		FreeHandler(theLayer->mHScroll);
		theLayer->mHScroll = nullptr;
	}
	if (!aVertical)
	{
		FreeHandler(theLayer->mVScroll);
		theLayer->mVScroll = nullptr;
	}
	else
	{
		aRect.x = (int16_t)(theLayer->mRect.x + theLayer->mRect.w);
		aRect.y = theLayer->mRect.y;
		aRect.w = kSkin[SKIN_UP].w;
		aRect.h = theLayer->mRect.h;
		theLayer->mVScroll = NewHandler(ScrollBarProc, theLayer, &aRect, theHeight, 0);
	}
	theLayer->mDirty = 1;
}

// --- scroll bars ---

namespace
{

// 0x429e79: one step of the part pressed: a line (8) by the arrows, a page by the track
void ScrollStep(Handler* theHandler)
{
	auto& aScroll = theHandler->u.scroll;
	uint16_t aLength;
	if (theHandler->mRect.w == kSkin[SKIN_UP].w)
		aLength = (uint16_t)theHandler->mRect.h;
	else
		aLength = (uint16_t)theHandler->mRect.w;
	aScroll.mRepeat = 5000;
	uint8_t aPart = aScroll.mState & 7;
	if (aPart == 1)
	{
		if (aScroll.mPos < 9)
			aScroll.mPos = 0;
		else
			aScroll.mPos = (uint16_t)(aScroll.mPos - 8);
	}
	else if (aPart == 2)
	{
		aLength = (uint16_t)(aScroll.mContent - aLength);
		aScroll.mPos = (uint16_t)(aScroll.mPos + 8);
		if (aLength < aScroll.mPos)
			aScroll.mPos = aLength;
	}
	else if (aPart == 4)
	{
		if (aLength < aScroll.mPos)
			aScroll.mPos = (uint16_t)(aScroll.mPos - aLength);
		else
			aScroll.mPos = 0;
	}
	else if (aPart == 5)
	{
		uint16_t aMax = (uint16_t)(aScroll.mContent - aLength);
		aScroll.mPos = (uint16_t)(aScroll.mPos + aLength);
		if (aMax < aScroll.mPos)
			aScroll.mPos = aMax;
	}
}

void SwapAxes(Handler* theHandler)
{
	int16_t t = theHandler->mRect.x;
	theHandler->mRect.x = theHandler->mRect.y;
	theHandler->mRect.y = t;
	t = theHandler->mRect.w;
	theHandler->mRect.w = theHandler->mRect.h;
	theHandler->mRect.h = t;
}

// The thumb's top while it is dragged, from the mouse
uint32_t DraggedThumb(Handler* theHandler, uint32_t theThumbPos, uint16_t theMouse)
{
	int v = (int)(theThumbPos + theMouse);
	if (theHandler->u.scroll.mGrabY < (int16_t)v)
		return (uint32_t)(v - theHandler->u.scroll.mGrabY);
	return 0;
}

// A pressed part follows the mouse: it stays pressed while the mouse is near it
void TrackPart(Handler* theHandler, GuiRect* theRect, uint16_t x, uint16_t y)
{
	InflateDirty(theRect);
	if (PointInRect(x, y, theRect) == 0)
	{
		theHandler->u.scroll.mState &= 0xf7;
		theHandler->u.scroll.mState |= 0x10;
	}
	else
	{
		theHandler->u.scroll.mState |= 0x18;
	}
}

// Draws theLength pixels of a part along the bar, 4, 2 or 1 at a time
void DrawRun(uint16_t& theLength, int16_t& thePos, bool isVertical, int16_t theAcross, int16_t theOther,
	const GuiRect& theSrc)
{
	for (; theLength != 0; )
	{
		uint16_t aStep;
		if (theLength < 4)
			aStep = theLength < 2 ? 1 : 2;
		else
			aStep = 4;
		if (isVertical)
			lowos::BlitRegistered((uint16_t)theAcross, aStep, theOther, thePos, theSrc.x, theSrc.y, gSkin);
		else
			lowos::BlitRegistered(aStep, (uint16_t)theAcross, thePos, theOther, theSrc.x, theSrc.y, gSkin);
		thePos = (int16_t)(thePos + aStep);
		theLength = (uint16_t)(theLength - aStep);
	}
}

} // namespace

// 0x429fd5: a scroll bar of a layer (vertical when it is as wide as the arrows). Create: a = the content's length.
uint32_t ScrollBarProc(uint32_t theMsg, uintptr_t theA, uintptr_t theB, Handler* theHandler)
{
	uint8_t aMsg = (uint8_t)theMsg;
	auto& aScroll = theHandler->u.scroll;
	int16_t anOldPos = 0;
	uint32_t aTrack = 0;     // the track's length between the arrows
	uint32_t aThumb = 0;     // the thumb's length
	uint32_t aThumbPos = 0;  // the thumb's place in the track
	if (aMsg != MSG_CREATE && aMsg != MSG_CLASSINIT && aMsg != MSG_CLASSFREE)
	{
		anOldPos = (int16_t)aScroll.mPos;
		uint16_t aLength;
		uint32_t anArrow;
		if ((uint16_t)theHandler->mRect.w == (uint16_t)kSkin[SKIN_UP].w)
		{
			aLength = (uint16_t)theHandler->mRect.h;
			anArrow = (uint16_t)kSkin[SKIN_UP].h;
		}
		else
		{
			aLength = (uint16_t)theHandler->mRect.w;
			anArrow = (uint16_t)kSkin[SKIN_LEFT].w;
		}
		aTrack = (uint32_t)aLength - anArrow * 2;
		aThumb = (aTrack * aLength) / aScroll.mContent;
		if (aThumb < 3)
			aThumb = 3;
		uint16_t aPos = (aScroll.mState & 7) == 3 ? aScroll.mGrabPos : aScroll.mPos;
		aThumbPos = (aTrack - aThumb) * aPos;
		aThumbPos = aThumbPos / ((uint32_t)aScroll.mContent - aLength);
		if (aTrack - aThumb < aThumbPos)
			aThumbPos = aTrack - aThumb;
	}
	if (aMsg == MSG_CREATE)
	{
		aScroll.mState = 0;
		aScroll.mContent = (uint16_t)theA;
		aScroll.mPos = 0;
		gScrollTick = lowos::TickCount();
	}
	else if (aMsg == MSG_MOUSEDOWN)
	{
		// a horizontal bar is handled as a vertical one, with its axes swapped
		bool isHorizontal = (uint16_t)theHandler->mRect.h == (uint16_t)kSkin[SKIN_UP].w;
		if (isHorizontal)
		{
			SwapAxes(theHandler);
			uintptr_t t = theA;
			theA = theB;
			theB = (uintptr_t)(intptr_t)(int16_t)t;
		}
		uint16_t x = (uint16_t)theA;
		uint16_t y = (uint16_t)theB;
		GuiRect aRect = {theHandler->mRect.x, theHandler->mRect.y, kSkin[SKIN_UP].w, kSkin[SKIN_UP].h};
		if (PointInRect(x, y, &aRect) != 0)
			aScroll.mState = (uint8_t)((aScroll.mState & 0xf0) | 0x19);
		aRect.x = theHandler->mRect.x;
		aRect.y = (int16_t)((theHandler->mRect.y + theHandler->mRect.h) - kSkin[SKIN_DOWN].h);
		aRect.w = kSkin[SKIN_DOWN].w;
		aRect.h = kSkin[SKIN_DOWN].h;
		if (PointInRect(x, y, &aRect) != 0)
			aScroll.mState = (uint8_t)((aScroll.mState & 0xf0) | 0x1a);
		aRect.x = theHandler->mRect.x;
		aRect.y = (int16_t)(theHandler->mRect.y + kSkin[SKIN_UP].h);
		aRect.w = kSkin[SKIN_VTRACK].w;
		aRect.h = (int16_t)aThumbPos;
		if (PointInRect(x, y, &aRect) != 0)
			aScroll.mState = (uint8_t)((aScroll.mState & 0xf0) | 0x1c);
		aRect.x = theHandler->mRect.x;
		aRect.y = (int16_t)(theHandler->mRect.y + (int16_t)aThumbPos + kSkin[SKIN_UP].h);
		aRect.w = kSkin[SKIN_VTHUMB].w;
		aRect.h = (int16_t)(uint16_t)aThumb;
		if (PointInRect(x, y, &aRect) != 0)
		{
			aScroll.mState = (uint8_t)((aScroll.mState & 0xf0) | 0x1b);
			aScroll.mGrabY = (int16_t)y;
			aScroll.mMouse = y;
			aScroll.mGrabPos = aScroll.mPos;
		}
		aRect.x = theHandler->mRect.x;
		aRect.y = (int16_t)(theHandler->mRect.y + (int16_t)aThumbPos + (uint16_t)aThumb + kSkin[SKIN_UP].h);
		aRect.w = kSkin[SKIN_VTRACK].w;
		aRect.h = (int16_t)((int16_t)aTrack - ((int16_t)aThumbPos + (uint16_t)aThumb));
		if (PointInRect(x, y, &aRect) != 0)
			aScroll.mState = (uint8_t)((aScroll.mState & 0xf0) | 0x1d);
		if (isHorizontal)
			SwapAxes(theHandler);
		if ((aScroll.mState & 0xf) != 0)
			ScrollStep(theHandler);
	}
	else if (aMsg > MSG_MOUSEDOWN)
	{
		if (aMsg < MSG_KEY)
		{
			bool isHorizontal = (uint16_t)theHandler->mRect.h == (uint16_t)kSkin[SKIN_UP].w;
			if (isHorizontal)
			{
				SwapAxes(theHandler);
				uintptr_t t = theA;
				theA = theB;
				theB = (uintptr_t)(intptr_t)(int16_t)t;
			}
			uint16_t y = (uint16_t)theB;
			uint16_t x = (uint16_t)theA;
			GuiRect aRect;
			if ((aScroll.mState & 7) == 1)
			{
				aRect = {theHandler->mRect.x, theHandler->mRect.y, kSkin[SKIN_UP].w, kSkin[SKIN_UP].h};
				TrackPart(theHandler, &aRect, x, y);
			}
			if ((aScroll.mState & 7) == 2)
			{
				aRect = {theHandler->mRect.x, (int16_t)((theHandler->mRect.y + theHandler->mRect.h) - kSkin[SKIN_DOWN].h),
					kSkin[SKIN_DOWN].w, kSkin[SKIN_DOWN].h};
				TrackPart(theHandler, &aRect, x, y);
			}
			if ((aScroll.mState & 7) == 4)
			{
				aRect = {theHandler->mRect.x, (int16_t)(theHandler->mRect.y + kSkin[SKIN_UP].h), kSkin[SKIN_VTRACK].w,
					(int16_t)aThumbPos};
				TrackPart(theHandler, &aRect, x, y);
			}
			if ((aScroll.mState & 7) == 3)
			{
				// the thumb is dragged while the mouse stays near the track
				aRect = {theHandler->mRect.x, (int16_t)(theHandler->mRect.y + kSkin[SKIN_UP].h), kSkin[SKIN_VTHUMB].w,
					(int16_t)aTrack};
				InflateDirty(&aRect);
				aScroll.mMouse = y;
				if (PointInRect(x, y, &aRect) == 0)
				{
					aScroll.mState &= 0xf7;
					aScroll.mState |= 0x10;
					aScroll.mPos = aScroll.mGrabPos;
				}
				else
				{
					aScroll.mState |= 0x18;
					uint32_t aNew = DraggedThumb(theHandler, aThumbPos, (uint16_t)theB);
					if (aNew < aTrack - aThumb)
						aScroll.mPos = (uint16_t)((aNew * ((uint32_t)aScroll.mContent - (uint16_t)theHandler->mRect.h)) /
							(aTrack - aThumb));
					else
						aScroll.mPos = (uint16_t)(aScroll.mContent - theHandler->mRect.h);
				}
			}
			if ((aScroll.mState & 7) == 5)
			{
				aRect = {theHandler->mRect.x,
					(int16_t)(theHandler->mRect.y + (int16_t)aThumbPos + (int16_t)aThumb + kSkin[SKIN_UP].h),
					kSkin[SKIN_VTRACK].w, (int16_t)((int16_t)aTrack - ((int16_t)aThumbPos + (int16_t)aThumb))};
				TrackPart(theHandler, &aRect, x, y);
			}
			if (isHorizontal)
				SwapAxes(theHandler);
			if (aMsg == MSG_MOUSEUP)
			{
				aScroll.mState &= 0xf0;
				aScroll.mState |= 0x10;
			}
		}
		else if (aMsg == MSG_UPDATE)
		{
			// the original measures from the last scroll bar's creation, not from the last update
			uint16_t anElapsed = (uint16_t)(((uint16_t)lowos::TickCount() - (uint16_t)gScrollTick) & 0x7ff);
			if ((aScroll.mState & 8) != 0 && (aScroll.mState & 7) != 3)
			{
				while (aScroll.mRepeat <= anElapsed)
				{
					anElapsed = (uint16_t)(anElapsed - aScroll.mRepeat);
					ScrollStep(theHandler);
				}
				aScroll.mRepeat = (uint16_t)(aScroll.mRepeat - anElapsed);
			}
		}
		else if (aMsg == MSG_DRAW && (theA != 0 || (aScroll.mState & 0x10) != 0))
		{
			aScroll.mState &= 0xef;
			uint8_t aPressed = aScroll.mState & 0xf;
			if ((uint16_t)theHandler->mRect.w == (uint16_t)kSkin[SKIN_UP].w)
			{
				GuiRect aSrc = aPressed == 9 ? kSkin[SKIN_UP_PRESSED] : kSkin[SKIN_UP];
				int16_t x = theHandler->mRect.x;
				int16_t y = theHandler->mRect.y;
				int16_t aWidth = kSkin[SKIN_UP].w;
				lowos::BlitRegistered((uint16_t)kSkin[SKIN_UP].w, (uint16_t)kSkin[SKIN_UP].h, x, y, aSrc.x, aSrc.y,
					gSkin);
				aSrc = aPressed == 10 ? kSkin[SKIN_DOWN_PRESSED] : kSkin[SKIN_DOWN];
				y = (int16_t)((theHandler->mRect.y + theHandler->mRect.h) - kSkin[SKIN_DOWN].h);
				lowos::BlitRegistered((uint16_t)aWidth, (uint16_t)kSkin[SKIN_UP].h, x, y, aSrc.x, aSrc.y, gSkin);
				if (aPressed == 0xb)
				{
					aThumbPos = DraggedThumb(theHandler, aThumbPos, aScroll.mMouse);
					if (aTrack - aThumb < aThumbPos)
						aThumbPos = aTrack - aThumb;
				}
				uint16_t aLeft = aThumbPos != 0 ? (uint16_t)((int16_t)aThumbPos - 1) : 0;
				y = (int16_t)(theHandler->mRect.y + kSkin[SKIN_UP].h);
				aWidth = kSkin[SKIN_VTRACK].w;
				DrawRun(aLeft, y, true, aWidth, x, kSkin[SKIN_VTRACK]);
				if (aThumbPos != 0)
				{
					lowos::BlitRegistered((uint16_t)aWidth, 1, x, y, kSkin[SKIN_VLINE].x, kSkin[SKIN_VLINE].y, gSkin);
					y++;
				}
				aLeft = (uint16_t)aThumb;
				DrawRun(aLeft, y, true, aWidth, x, kSkin[SKIN_VTHUMB]);
				aLeft = (uint16_t)((int16_t)aTrack - ((int16_t)aThumb + (int16_t)aThumbPos));
				if (aLeft != 0)
				{
					lowos::BlitRegistered((uint16_t)aWidth, 1, x, y, kSkin[SKIN_VLINE].x, kSkin[SKIN_VLINE].y, gSkin);
					y++;
					aLeft--;
					if (aLeft != 0 && ((aThumb + aThumbPos) & 1) != 0)
					{
						// keeps the track's pattern in step
						lowos::BlitRegistered((uint16_t)aWidth, 1, x, y, kSkin[SKIN_VTRACK].x,
							(int16_t)(kSkin[SKIN_VTRACK].y + 1), gSkin);
						y++;
						aLeft--;
					}
				}
				DrawRun(aLeft, y, true, aWidth, x, kSkin[SKIN_VTRACK]);
			}
			else
			{
				GuiRect aSrc = aPressed == 9 ? kSkin[SKIN_LEFT_PRESSED] : kSkin[SKIN_LEFT];
				int16_t x = theHandler->mRect.x;
				int16_t y = theHandler->mRect.y;
				lowos::BlitRegistered((uint16_t)kSkin[SKIN_LEFT].w, (uint16_t)kSkin[SKIN_LEFT].h, x, y, aSrc.x, aSrc.y,
					gSkin);
				aSrc = aPressed == 10 ? kSkin[SKIN_RIGHT_PRESSED] : kSkin[SKIN_RIGHT];
				x = (int16_t)((theHandler->mRect.x + theHandler->mRect.w) - kSkin[SKIN_RIGHT].w);
				lowos::BlitRegistered((uint16_t)kSkin[SKIN_LEFT].w, (uint16_t)kSkin[SKIN_LEFT].h, x, y, aSrc.x, aSrc.y,
					gSkin);
				if (aPressed == 0xb)
				{
					aThumbPos = DraggedThumb(theHandler, aThumbPos, aScroll.mMouse);
					if (aTrack - aThumb < aThumbPos)
						aThumbPos = aTrack - aThumb;
				}
				uint16_t aLeft = aThumbPos != 0 ? (uint16_t)((int16_t)aThumbPos - 1) : 0;
				x = (int16_t)(theHandler->mRect.x + kSkin[SKIN_LEFT].w);
				y = theHandler->mRect.y;
				int16_t aHeight = kSkin[SKIN_HTRACK].h;
				DrawRun(aLeft, x, false, aHeight, y, kSkin[SKIN_HTRACK]);
				if (aThumbPos != 0)
				{
					lowos::BlitRegistered(1, (uint16_t)aHeight, x, y, kSkin[SKIN_HLINE].x, kSkin[SKIN_HLINE].y, gSkin);
					x++;
				}
				aLeft = (uint16_t)aThumb;
				DrawRun(aLeft, x, false, aHeight, y, kSkin[SKIN_HTHUMB]);
				aLeft = (uint16_t)((int16_t)aTrack - ((int16_t)aThumb + (int16_t)aThumbPos));
				if (aLeft != 0)
				{
					lowos::BlitRegistered(1, (uint16_t)aHeight, x, y, kSkin[SKIN_HLINE].x, kSkin[SKIN_HLINE].y, gSkin);
					x++;
					aLeft--;
					if (aLeft != 0 && ((aThumb + aThumbPos) & 1) != 0)
					{
						lowos::BlitRegistered(1, (uint16_t)aHeight, x, y, (int16_t)(kSkin[SKIN_HTRACK].x + 1),
							kSkin[SKIN_HTRACK].y, gSkin);
						x++;
						aLeft--;
					}
				}
				DrawRun(aLeft, x, false, aHeight, y, kSkin[SKIN_HTRACK]);
			}
		}
	}
	if (aMsg != MSG_CREATE && aMsg != MSG_CLASSINIT && aMsg != MSG_CLASSFREE && (int16_t)aScroll.mPos != anOldPos)
		theHandler->mLayer->mDirty = 1;
	return 0;
}

// --- the toolkit's widgets ---

// 0x42b3eb: a button. Create: a = the image (normal over hovered), b = the command (| BUTTON_ENTER: Enter presses it,
// | BUTTON_ESCAPE: Escape does)
uint32_t ButtonProc(uint32_t theMsg, uintptr_t theA, uintptr_t theB, Handler* theHandler)
{
	auto& aButton = theHandler->u.button;
	uint8_t aMsg = (uint8_t)theMsg;
	if (aMsg == MSG_CREATE)
	{
		if (LoadSurface((int)theA, 1, &aButton.mImage) != 0)
			NoMemory();
		aButton.mCommand = (uint32_t)theB;
		aButton.mState = 0;
		aButton.mImageId = (int32_t)theA;
		theHandler->mRect.w = (int16_t)lowos::SurfaceWidth(aButton.mImage);
		theHandler->mRect.h = (int16_t)(lowos::SurfaceHeight(aButton.mImage) / 2);
	}
	else if (aMsg == MSG_DESTROY)
	{
		FreeSurface(aButton.mImage);
	}
	else if (aMsg == MSG_MOUSEDOWN)
	{
		if (PointInRect((uint16_t)theA, (uint16_t)theB, &theHandler->mRect) != 0)
		{
			PlayClick();
			Command(theHandler, aButton.mCommand & COMMAND_MASK);
		}
	}
	else if (aMsg == MSG_MOUSEMOVE)
	{
		if (PointInRect((uint16_t)theA, (uint16_t)theB, &theHandler->mRect) == 0)
			aButton.mState &= 0xfe;
		else if ((~aButton.mState & 1) != 0)
			aButton.mState |= 1;
	}
	else if (aMsg == MSG_DRAW)
	{
		uint32_t aSrcY = (aButton.mState & 1) == 0 ? 0 : (uint16_t)theHandler->mRect.h;
		lowos::BlitRegistered((uint16_t)theHandler->mRect.w, (uint16_t)theHandler->mRect.h, theHandler->mRect.x,
			theHandler->mRect.y, 0, (int)aSrcY, aButton.mImage);
		if ((aButton.mState & 3) == 1)
			PlayRollover();
		aButton.mState &= 0xfd;
		aButton.mState = (uint8_t)(aButton.mState + (aButton.mState & 1) * 2);
	}
	else if (aMsg == MSG_CHAR)
	{
		if (theA == 0xd && (aButton.mCommand & BUTTON_ENTER) != 0)
		{
			PlayClick();
			Command(theHandler, aButton.mCommand & COMMAND_MASK);
		}
		if (theA == 0x1b && (aButton.mCommand & BUTTON_ESCAPE) != 0)
		{
			PlayClick();
			Command(theHandler, aButton.mCommand & COMMAND_MASK);
		}
	}
	return 0;
}

namespace
{

// The color key as the shaped buttons compare it: to the display's format and back
uint32_t ShapeKey()
{
	lowos::PixelFormat aDisplay;
	lowos::PixelFormat aLogical;
	lowos::GetDisplayFormat(&aDisplay, nullptr, nullptr);
	lowos::GetLogicalFormat(&aLogical, nullptr, nullptr);
	uint32_t aKey = lowos::ConvertColor(&aLogical, &aDisplay, 0xfefe01);
	return lowos::ConvertColor(&aDisplay, &aLogical, aKey);
}

} // namespace

// 0x42b6b4: a button whose image's color-keyed pixels are not part of it
uint32_t ShapedButtonProc(uint32_t theMsg, uintptr_t theA, uintptr_t theB, Handler* theHandler)
{
	auto& aButton = theHandler->u.button;
	uint8_t aMsg = (uint8_t)theMsg;
	if (aMsg == MSG_CREATE)
	{
		if (LoadSurface((int)theA, 9, &aButton.mImage) != 0)
			NoMemory();
		aButton.mCommand = (uint32_t)theB;
		aButton.mState = 0;
		aButton.mImageId = (int32_t)theA;
		theHandler->mRect.w = (int16_t)lowos::SurfaceWidth(aButton.mImage);
		theHandler->mRect.h = (int16_t)(lowos::SurfaceHeight(aButton.mImage) / 2);
	}
	else if (aMsg == MSG_DESTROY)
	{
		FreeSurface(aButton.mImage);
	}
	else if (aMsg == MSG_MOUSEDOWN)
	{
		uint32_t aKey = ShapeKey();
		if (PointInRect((uint16_t)theA, (uint16_t)theB, &theHandler->mRect) != 0 &&
			lowos::GetPixel(aButton.mImage, (int)theA - theHandler->mRect.x, (int)theB - theHandler->mRect.y) != aKey)
		{
			PlayClick();
			Command(theHandler, aButton.mCommand & COMMAND_MASK);
		}
	}
	else if (aMsg == MSG_MOUSEMOVE)
	{
		uint32_t aKey = ShapeKey();
		if (PointInRect((uint16_t)theA, (uint16_t)theB, &theHandler->mRect) == 0 ||
			lowos::GetPixel(aButton.mImage, (int)theA - theHandler->mRect.x, (int)theB - theHandler->mRect.y) == aKey)
			aButton.mState &= 0xfe;
		else if ((~aButton.mState & 1) != 0)
			aButton.mState |= 1;
	}
	else if (aMsg == MSG_DRAW)
	{
		uint32_t aSrcY = (aButton.mState & 1) == 0 ? 0 : (uint16_t)theHandler->mRect.h;
		lowos::BlitRegistered((uint16_t)theHandler->mRect.w, (uint16_t)theHandler->mRect.h, theHandler->mRect.x,
			theHandler->mRect.y, 0, (int)aSrcY, aButton.mImage);
		if ((aButton.mState & 3) == 1)
			PlayRollover();
		aButton.mState &= 1;
		aButton.mState = (uint8_t)(aButton.mState + aButton.mState * 2);
	}
	else if (aMsg == MSG_CHAR)
	{
		if (theA == 0xd && (aButton.mCommand & BUTTON_ENTER) != 0)
		{
			PlayClick();
			Command(theHandler, aButton.mCommand & COMMAND_MASK);
		}
		if (theA == 0x1b && (aButton.mCommand & BUTTON_ESCAPE) != 0)
		{
			PlayClick();
			Command(theHandler, aButton.mCommand & COMMAND_MASK);
		}
	}
	return 0;
}

// 0x42ba49: word-wrapped text filling its layer, which scrolls it. Create: a = the text, b = the font.
uint32_t TextAreaProc(uint32_t theMsg, uintptr_t theA, uintptr_t theB, Handler* theHandler)
{
	auto& aText = theHandler->u.text;
	uint8_t aMsg = (uint8_t)theMsg;
	GuiRect anExtent;
	if (aMsg == MSG_CREATE)
	{
		memcpy(&theHandler->mRect, &theHandler->mLayer->mRect, sizeof(GuiRect));
		aText.mText = (const char*)theA;
		aText.mFont = (Font*)theB;
		const char* aLine = aText.mText;
		theHandler->mRect.w = (int16_t)(theHandler->mRect.w - kSkin[SKIN_UP].w);
		uint16_t aHeight = 0;
		while (aLine != nullptr)
		{
			aLine = DrawText(aText.mFont, nullptr, 0, aLine, &theHandler->mRect, &anExtent, theHandler->mRect.x,
				(int16_t)aHeight, 0, 3);
			aHeight = (uint16_t)(aHeight + anExtent.h);
		}
		if ((int)(int16_t)aHeight < (int)(uint16_t)theHandler->mRect.h)
			aHeight = (uint16_t)theHandler->mRect.h;
		SetLayerContent(theHandler->mLayer, (uint16_t)theHandler->mRect.w, aHeight);
	}
	else if (aMsg == MSG_DRAW && theA != 0)
	{
		const char* aLine = aText.mText;
		int16_t y = (int16_t)(theHandler->mRect.y - (int16_t)LayerScrollY(theHandler->mLayer));
		while (aLine != nullptr)
		{
			aLine = DrawText(aText.mFont, nullptr, 0, aLine, &theHandler->mRect, &anExtent, theHandler->mRect.x, y, 0, 1);
			y = (int16_t)(y + anExtent.h);
		}
	}
	return 0;
}

// 0x42bc76: a line of text at the handler's place. Create: a = the text, b = the font's image | 0x40000000 centered
// on x | 0x80000000 centered on y. Each label loads its own font.
uint32_t LabelProc(uint32_t theMsg, uintptr_t theA, uintptr_t theB, Handler* theHandler)
{
	auto& aText = theHandler->u.text;
	uint8_t aMsg = (uint8_t)theMsg;
	if (aMsg == MSG_CREATE)
	{
		aText.mText = (const char*)theA;
		aText.mFont = LoadFont((int)(theB & 0x3fffffff), GTC(0x46eca8), 4, 0, 0x40);
		aText.mAlign = (uint8_t)((uint32_t)theB >> 0x1e);
	}
	else if (aMsg == MSG_DESTROY)
	{
		FreeFont(aText.mFont);
	}
	else if (aMsg == MSG_DRAW)
	{
		GuiRect aClip;
		aClip.x = (int16_t)(theHandler->mRect.x - ((aText.mAlign & 1) != 0 ? 400 : 0));
		aClip.y = (int16_t)(theHandler->mRect.y - ((aText.mAlign & 2) != 0 ? 400 : 0));
		aClip.w = 800;
		aClip.h = 800;
		DrawText(aText.mFont, nullptr, 0, aText.mText, &aClip, nullptr, theHandler->mRect.x, theHandler->mRect.y, 0,
			(uint8_t)(((aText.mAlign & 2) != 0 ? 8 : 0) | ((aText.mAlign & 1) != 0 ? 4 : 0)));
	}
	return 0;
}

// 0x42bd93: a check box. Create: a = the image (four frames: off, on, off hovered, on hovered), b = the command.
uint32_t CheckBoxProc(uint32_t theMsg, uintptr_t theA, uintptr_t theB, Handler* theHandler)
{
	auto& aCheck = theHandler->u.check;
	uint8_t aMsg = (uint8_t)theMsg;
	if (aMsg == MSG_CREATE)
	{
		if (LoadSurface((int)theA, 1, &aCheck.mImage) != 0)
			NoMemory();
		aCheck.mImageId = (int32_t)theA;
		theHandler->mRect.w = (int16_t)lowos::SurfaceWidth(aCheck.mImage);
		theHandler->mRect.h = (int16_t)(lowos::SurfaceHeight(aCheck.mImage) / 4);
		aCheck.mCommand = (uint32_t)theB;
		aCheck.mState = 0;
	}
	else if (aMsg == MSG_DESTROY)
	{
		FreeSurface(aCheck.mImage);
	}
	else if (aMsg == MSG_MOUSEDOWN)
	{
		if (PointInRect((uint16_t)theA, (uint16_t)theB, &theHandler->mRect) != 0)
		{
			aCheck.mState ^= 1;
			Command(theHandler, aCheck.mCommand);
		}
	}
	else if (aMsg == MSG_MOUSEMOVE)
	{
		if (PointInRect((uint16_t)theA, (uint16_t)theB, &theHandler->mRect) == 0)
		{
			aCheck.mState &= 0xfd;
		}
		else if ((~aCheck.mState & 2) != 0)
		{
			PlayRollover();
			aCheck.mState |= 2;
		}
	}
	else if (aMsg == MSG_DRAW)
	{
		uint32_t aChecked = (aCheck.mState & 1) == 0 ? 0 : (uint16_t)theHandler->mRect.h;
		uint32_t aHovered = (aCheck.mState & 2) == 0 ? 0 : (uint32_t)(uint16_t)theHandler->mRect.h << 1;
		lowos::BlitRegistered((uint16_t)theHandler->mRect.w, (uint16_t)theHandler->mRect.h, theHandler->mRect.x,
			theHandler->mRect.y, 0, (int)(aChecked + aHovered), aCheck.mImage);
	}
	else if (aMsg == MSG_SETVALUE)
	{
		if (theA == 0)
			aCheck.mState &= 0xfe;
		else
			aCheck.mState |= 1;
	}
	else if (aMsg == MSG_GETVALUE)
	{
		return (aCheck.mState & 1) != 0;
	}
	return 0;
}

// 0x42bfdd: a slider: its knob (image 0) moves along the handler's width. Create: b = the command, sent as it
// moves. Its value is 16.16, 0 to 1.
uint32_t SliderProc(uint32_t theMsg, uintptr_t theA, uintptr_t theB, Handler* theHandler)
{
	auto& aSlider = theHandler->u.slider;
	uint8_t aMsg = (uint8_t)theMsg;
	if (aMsg < MSG_CLASSINIT)
	{
		if (aMsg == MSG_DRAW)
		{
			uint32_t aSrcY = (aSlider.mState & 1) == 0 ? 0 : (uint16_t)theHandler->mRect.h;
			lowos::BlitRegistered(lowos::SurfaceWidth(gSliderKnob) & 0xffff, (uint16_t)theHandler->mRect.h,
				theHandler->mRect.x + aSlider.mPos, theHandler->mRect.y, 0, (int)aSrcY, gSliderKnob);
		}
		else if (aMsg == MSG_CREATE)
		{
			uint16_t aHeight = lowos::SurfaceHeight(gSliderKnob);
			theHandler->mRect.h = (int16_t)((int)(aHeight - 1) / 2);
			aSlider.mCommand = (uint32_t)theB;
			aSlider.mState = 0;
			aSlider.mPos = 0;
		}
		else if (aMsg == MSG_MOUSEDOWN)
		{
			uint16_t aKnob = (uint16_t)lowos::SurfaceWidth(gSliderKnob);
			GuiRect r = {(int16_t)(theHandler->mRect.x + (int16_t)aSlider.mPos), theHandler->mRect.y, (int16_t)aKnob,
				theHandler->mRect.h};
			if (PointInRect((uint16_t)theA, (uint16_t)theB, &r) == 0)
			{
				// a click on the track: the knob jumps there
				if (PointInRect((uint16_t)theA, (uint16_t)theB, &theHandler->mRect) != 0)
				{
					uint32_t aCenter = (uint32_t)(aKnob / 2 + (int)theHandler->mRect.x);
					uint32_t x = (uint32_t)theA;
					if (aCenter < x)
						x = x - aCenter;
					else
						x = 0;
					if ((uint32_t)(uint16_t)theHandler->mRect.w - (uint32_t)aKnob < x)
						x = (uint32_t)(uint16_t)theHandler->mRect.w - (uint32_t)aKnob;
					aSlider.mPos = (int32_t)x;
					aSlider.mState |= 1;
					aSlider.mGrab = aKnob / 2 + (int)theHandler->mRect.x;
					Command(theHandler, aSlider.mCommand);
				}
			}
			else
			{
				aSlider.mState |= 1;
				aSlider.mGrab = (int32_t)theA - aSlider.mPos;
			}
		}
		else if (aMsg == MSG_MOUSEMOVE)
		{
			if ((~aSlider.mState & 1) == 0)
			{
				uint32_t x = (uint32_t)theA;
				if (x < (uint32_t)aSlider.mGrab)
					x = (uint32_t)aSlider.mGrab;
				x = x - (uint32_t)aSlider.mGrab;
				uint16_t aMax = (uint16_t)(theHandler->mRect.w - (int16_t)lowos::SurfaceWidth(gSliderKnob));
				if ((uint32_t)aMax < x)
					x = aMax;
				if ((uint32_t)aSlider.mPos != x)
				{
					aSlider.mPos = (int32_t)x;
					Command(theHandler, aSlider.mCommand);
				}
			}
		}
		else if (aMsg == MSG_MOUSEUP)
		{
			aSlider.mState &= 0xfe;
		}
	}
	else if (aMsg == MSG_CLASSINIT)
	{
		if (LoadSurface(0, 9, &gSliderKnob) != 0)
			NoMemory();
	}
	else if (aMsg == MSG_CLASSFREE)
	{
		FreeSurface(gSliderKnob);
	}
	else if (aMsg == MSG_SETVALUE)
	{
		uint16_t aMax = (uint16_t)(theHandler->mRect.w - (int16_t)lowos::SurfaceWidth(gSliderKnob));
		aSlider.mPos = (int32_t)(((uint32_t)theA * (uint32_t)aMax) >> 0x10);
	}
	else if (aMsg == MSG_GETVALUE)
	{
		uint16_t aMax = (uint16_t)(theHandler->mRect.w - (int16_t)lowos::SurfaceWidth(gSliderKnob));
		return ((uint32_t)aSlider.mPos << 0x10) / (uint32_t)aMax;
	}
	return 0;
}

// 0x42c36b: an image. Create: b = a surface to show, or 0 to load image a (| 0x10000: color-keyed).
uint32_t ImageProc(uint32_t theMsg, uintptr_t theA, uintptr_t theB, Handler* theHandler)
{
	auto& i = theHandler->u.image;
	uint8_t aMsg = (uint8_t)theMsg;
	if (aMsg == MSG_CREATE)
	{
		if (theB == 0)
		{
			if (LoadSurface((int)(theA & 0xffff), (uint8_t)(((theA & 0x10000) != 0 ? 8 : 0) | 1), &i.mImage) != 0)
				NoMemory();
			i.mOwned = 1;
			i.mImageId = (int32_t)(theA & 0xffff);
		}
		else
		{
			i.mImage = (lowos::Surface*)theB;
			i.mOwned = 0;
		}
		theHandler->mRect.w = (int16_t)lowos::SurfaceWidth(i.mImage);
		theHandler->mRect.h = (int16_t)lowos::SurfaceHeight(i.mImage);
	}
	else if (aMsg == MSG_DESTROY)
	{
		if (i.mOwned != 0)
			FreeSurface(i.mImage);
	}
	else if (aMsg == MSG_DRAW)
	{
		lowos::BlitRegistered((uint16_t)theHandler->mRect.w, (uint16_t)theHandler->mRect.h, theHandler->mRect.x,
			theHandler->mRect.y, 0, 0, i.mImage);
	}
	return 0;
}

// 0x42c482: a text field. Create: a = the font, b = the command sent on Enter | the longest text << 16. Get value
// (17) gives it the focus (a 1) or takes it (a 0); set value (16) copies its text to a.
uint32_t TextInputProc(uint32_t theMsg, uintptr_t theA, uintptr_t theB, Handler* theHandler)
{
	auto& anInput = theHandler->u.input;
	uint8_t aMsg = (uint8_t)theMsg;
	GuiRect anExtent;
	if (aMsg < MSG_BACKTAB)
	{
		if (aMsg == MSG_TAB)
		{
			if ((~anInput.mState & 1) == 0)
				anInput.mState |= 4;
		}
		else
		{
			uint16_t y = (uint16_t)theB;
			if (aMsg == MSG_CREATE)
			{
				anInput.mFont = (Font*)theA;
				anInput.mCommand = (uint16_t)theB;
				anInput.mSize = (uint16_t)(((uint32_t)theB >> 0x10) + 1);
				anInput.mBuffer = (char*)malloc(((uint32_t)theB >> 0x10) + 1);
				if (anInput.mBuffer == nullptr)
					NoMemory();
				anInput.mBuffer[0] = 0;
				anInput.mBlinkTime = RealTime();
				anInput.mState = 1;
				// a space measures the line's height, centered in the field
				DrawText(anInput.mFont, nullptr, 0, GTC(0x46f21c), &theHandler->mRect, &anExtent, 0, theHandler->mRect.y, 0,
					10);
				theHandler->mRect.h = anExtent.h;
				theHandler->mRect.y = anExtent.y;
				lowos::FlushInput();
				gTextInputs++;
			}
			else if (aMsg == MSG_DESTROY)
			{
				free(anInput.mBuffer);
				gTextInputs--;
			}
			else if (aMsg == MSG_MOUSEDOWN)
			{
				if (gTextInputs != 1)
				{
					if (PointInRect((uint16_t)theA, y, &theHandler->mRect) == 0)
					{
						if ((anInput.mState & 1) != 0)
						{
							for (Handler* aNextField = NextHandler(theHandler); aNextField != theHandler; aNextField = NextHandler(aNextField))
							{
								if (aNextField->mProc == TextInputProc && PointInRect((uint16_t)theA, y, &aNextField->mRect) != 0 &&
									InTopModal(aNextField->mLayer) != 0)
								{
									anInput.mState &= 0xfe;
									return 0;
								}
							}
						}
					}
					else if ((~anInput.mState & 1) != 0)
					{
						anInput.mState |= 1;
						anInput.mBlinkTime = RealTime();
					}
				}
			}
			else if (aMsg == MSG_UPDATE)
			{
				if (InTopModal(theHandler->mLayer) == 0)
				{
					if ((anInput.mState & 1) != 0)
					{
						anInput.mState |= 0x10;
						anInput.mState &= 0xfe;
					}
				}
				else if ((anInput.mState & 0x10) != 0)
				{
					if ((~anInput.mState & 1) != 0)
					{
						anInput.mState |= 1;
						anInput.mBlinkTime = RealTime();
					}
					anInput.mState &= 0xef;
				}
				if ((anInput.mState & 0xc) != 0 && (anInput.mState & 1) != 0)
				{
					// Tab and Shift+Tab: the focus goes to the next or previous field of the layer
					Handler* aNextField = theHandler;
					do
					{
						if ((anInput.mState & 8) == 0)
							aNextField = NextHandler(aNextField);
						else
							aNextField = PrevHandler(aNextField);
						if (aNextField == theHandler)
							goto moved;
					} while (aNextField->mLayer != theHandler->mLayer || aNextField->mProc != TextInputProc);
					Send(theHandler, MSG_GETVALUE, 0, 0);
					Send(aNextField, MSG_GETVALUE, 1, 0);
				moved:
					anInput.mState &= 0xf3;
				}
				if ((anInput.mState & 2) != 0)
				{
					anInput.mState &= 0xfd;
					Command(theHandler, anInput.mCommand);
				}
			}
			else if (aMsg == MSG_DRAW)
			{
				DrawText(anInput.mFont, nullptr, 0, anInput.mBuffer, nullptr, &anExtent, 0, 0, 0, 2);
				uint32_t aScroll;
				if ((uint16_t)theHandler->mRect.w < (uint16_t)anExtent.w)
					aScroll = (uint32_t)(uint16_t)anExtent.w - (uint32_t)(uint16_t)theHandler->mRect.w;
				else
					aScroll = 0;
				DrawText(anInput.mFont, nullptr, 0, anInput.mBuffer, &theHandler->mRect, &anExtent,
					(int16_t)(theHandler->mRect.x - (int16_t)aScroll), theHandler->mRect.y, 0, 0x40);
				uint32_t aNow = RealTime();
				if (aNow < anInput.mBlinkTime)
					anInput.mBlinkTime = aNow;
				if ((anInput.mState & 1) != 0 && (aNow - anInput.mBlinkTime) % 0x29b < 0x14d)
					lowos::FillRectangle(2, (uint16_t)anExtent.h, anExtent.x + 1 + (uint16_t)anExtent.w, anExtent.y,
						0xf0f0c0);
			}
			else if (aMsg == MSG_PASTE && (anInput.mState & 1) != 0)
			{
				// the original reads the Windows clipboard's text (CF_TEXT) the same way
				if (SDL_HasClipboardText())
				{
					char* aClip = SDL_GetClipboardText();
					if (aClip != nullptr)
					{
						int i;
						for (i = 0; i < anInput.mSize - 1; i++)
						{
							if (anInput.mBuffer[i] == 0)
								break;
						}
						if (i < anInput.mSize - 1)
						{
							// appends its printable ASCII
							for (int j = 0;; j++)
							{
								signed char c = (signed char)aClip[j];
								if (c == 0 || c > 0x1f)
								{
									if (i == anInput.mSize - 1)
									{
										anInput.mBuffer[i] = 0;
										break;
									}
									anInput.mBuffer[i] = (char)c;
									if (c == 0)
										break;
									i++;
								}
							}
						}
						SDL_free(aClip);
					}
				}
			}
		}
	}
	else if (aMsg == MSG_BACKTAB)
	{
		if ((~anInput.mState & 1) == 0)
			anInput.mState |= 8;
	}
	else if (aMsg == MSG_CHAR)
	{
		if ((~anInput.mState & 1) != 0)
			return 0;
		if (theA < 0x20)
		{
			if (theA == 8 && anInput.mBuffer[0] != 0)
			{
				for (int i = 0; i < anInput.mSize; i++)
				{
					if (anInput.mBuffer[i] == 0)
					{
						anInput.mBuffer[i - 1] = 0;
						break;
					}
				}
			}
			if (theA == 0xd)
				anInput.mState |= 2;
		}
		else
		{
			for (int i = 0; i < anInput.mSize - 1; i++)
			{
				if (anInput.mBuffer[i] == 0)
				{
					anInput.mBuffer[i] = (char)theA;
					anInput.mBuffer[i + 1] = 0;
					break;
				}
			}
			anInput.mBlinkTime = RealTime();
		}
	}
	else if (aMsg == MSG_SETVALUE)
	{
		strcpy((char*)theA, anInput.mBuffer);
	}
	else if (aMsg == MSG_GETVALUE)
	{
		if ((~anInput.mState & 1) != 0 && theA != 0)
			anInput.mBlinkTime = RealTime();
		anInput.mState &= 0xfe;
		anInput.mState |= (uint8_t)(theA != 0);
		if (theA != 0)
			lowos::FlushInput();
	}
	else if (aMsg == MSG_GETLENGTH)
	{
		return (uint32_t)strlen(anInput.mBuffer);
	}
	return 0;
}

// 0x42cbe4: a strip of frames that waits, fades in while rising, holds, and fades out while rising further, then
// frees itself and sends its command. Create: a = AnimationParams, b = the command | the frame count << 16
// (| ANIMATION_ADDITIVE); the handler's rectangle gives its center.
uint32_t AnimationProc(uint32_t theMsg, uintptr_t theA, uintptr_t theB, Handler* theHandler)
{
	auto& anAnimation = theHandler->u.anim;
	uint8_t aMsg = (uint8_t)theMsg;
	if (aMsg == MSG_CREATE)
	{
		const AnimationParams* aParams = (const AnimationParams*)theA;
		anAnimation.mImage = aParams->mImage;
		anAnimation.mDelay = aParams->mDelay;
		anAnimation.mRise = aParams->mRise;
		anAnimation.mFade = aParams->mFade;
		anAnimation.mHold = aParams->mHold;
		anAnimation.mStart = GameTime();
		anAnimation.mFlags = (uint32_t)theB & 0x8000ffff;
		theHandler->mRect.w = (int16_t)lowos::SurfaceWidth(anAnimation.mImage);
		theHandler->mRect.w = (int16_t)((uint64_t)(uint16_t)theHandler->mRect.w /
			(uint64_t)(int64_t)(int)((uint32_t)theB >> 0x10 & 0x7fff));
		theHandler->mRect.h = (int16_t)lowos::SurfaceHeight(anAnimation.mImage);
		theHandler->mRect.x = (int16_t)((uint16_t)theHandler->mRect.x - (uint16_t)theHandler->mRect.w / 2);
		theHandler->mRect.y = (int16_t)(theHandler->mRect.y - (uint16_t)theHandler->mRect.h / 2);
	}
	else if (aMsg == MSG_DESTROY)
	{
		Command(theHandler, anAnimation.mFlags & 0x7fffffff);
	}
	else if (aMsg == MSG_DRAW)
	{
		uint32_t aTime = GameTime();
		if (aTime < anAnimation.mStart)
		{
			FreeHandler(theHandler);
		}
		else
		{
			aTime = aTime - anAnimation.mStart;
			GuiRect aRect = theHandler->mRect;
			if ((uint32_t)anAnimation.mDelay <= aTime)
			{
				uint8_t aLevel;
				aTime = aTime - anAnimation.mDelay;
				if (aTime < (uint32_t)(anAnimation.mFade / 2))
				{
					aRect.y = (int16_t)(aRect.y - (int16_t)((aTime * anAnimation.mRise) / anAnimation.mFade));
					aLevel = (uint8_t)((aTime * 0x100) / (uint32_t)(anAnimation.mFade / 2));
				}
				else
				{
					aTime = aTime - anAnimation.mFade / 2;
					if (aTime < anAnimation.mHold)
					{
						aLevel = 0xff;
						aRect.y = (int16_t)(aRect.y - anAnimation.mRise / 2);
					}
					else
					{
						aTime = aTime - anAnimation.mHold;
						if ((uint32_t)(anAnimation.mFade / 2) <= aTime)
						{
							FreeHandler(theHandler);
							return 0;
						}
						aRect.y = (int16_t)((aRect.y - anAnimation.mRise / 2) - (int16_t)((aTime * anAnimation.mRise) / anAnimation.mFade));
						aLevel = (uint8_t)(0xff - (uint8_t)((aTime * 0x100) / (uint32_t)(anAnimation.mFade / 2)));
					}
				}
				uint32_t aFrames = (lowos::SurfaceWidth(anAnimation.mImage) & 0xffff) / (uint32_t)(uint16_t)aRect.w;
				int aFrameX = (int)(((uint64_t)aTime / 100) % (uint64_t)aFrames) * (uint16_t)aRect.w;
				if ((anAnimation.mFlags & 0x80000000u) == 0)
				{
					lowos::BlitLevel((uint16_t)aRect.w, (uint16_t)aRect.h, aRect.x, aRect.y, aFrameX, 0, anAnimation.mImage, aLevel);
				}
				else
				{
					uint8_t* aBits = lowos::SurfaceBits(anAnimation.mImage);
					uint32_t aWidth = lowos::SurfaceWidth(anAnimation.mImage);
					uint8_t aBytes = lowos::GetDisplayFormat(nullptr, nullptr, nullptr);
					lowos::Blit((uint16_t)aRect.w, (uint16_t)aRect.h, aRect.x, aRect.y, aFrameX, 0, (aWidth & 0xffff) * aBytes, aBits,
						0x15, aLevel);
				}
			}
		}
	}
	return 0;
}

// 0x42cf34: counts itself in a = char*, and keeps a pointer to itself at b (Handler**, may be 0) while it lives
uint32_t CounterProc(uint32_t theMsg, uintptr_t theA, uintptr_t theB, Handler* theHandler)
{
	auto& aCounter = theHandler->u.counter;
	uint8_t aMsg = (uint8_t)theMsg;
	if (aMsg == MSG_CREATE)
	{
		aCounter.mCounter = (char*)theA;
		(*aCounter.mCounter)++;
		aCounter.mBackRef = (Handler**)theB;
		if (aCounter.mBackRef != nullptr)
			*aCounter.mBackRef = theHandler;
	}
	else if (aMsg == MSG_DESTROY)
	{
		(*aCounter.mCounter)--;
		if (aCounter.mBackRef != nullptr)
			*aCounter.mBackRef = nullptr;
	}
	return 0;
}

// 0x42cfae: keeps resource b of group a loaded while it lives
uint32_t ResourceProc(uint32_t theMsg, uintptr_t theA, uintptr_t theB, Handler* theHandler)
{
	auto& r = theHandler->u.res;
	uint8_t aMsg = (uint8_t)theMsg;
	if (aMsg == MSG_CREATE)
	{
		r.mGroup = (int32_t)theA;
		r.mId = (int32_t)theB;
		r.mLoaded = (uint8_t)(1 - (res::Load((int)theA, 1, (int)theB) != 0));
	}
	else if (aMsg == MSG_DESTROY && r.mLoaded != 0)
	{
		res::Free(r.mGroup, 1, r.mId);
	}
	return 0;
}

// port (test hooks, testhooks/Canvas.cpp): every layer in use draws all its handlers again in the next frame
void MarkAllLayersDirty()
{
	for (uint16_t i = 0; i < gLayerCount; i++)
		if (gLayers[i].mRect.w != 0)
			gLayers[i].mDirty = 1;
}

} // namespace tiptop
