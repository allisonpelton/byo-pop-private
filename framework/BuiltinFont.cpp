// port: the framework's built-in Latin-1 bitmap font (see BuiltinFont.h). The glyphs were drawn for this port.

#include "BuiltinFont.h"
#include "Graphics.h"
#include "DDImage.h"
#include "DDInterface.h"
#include "SexyAppBase.h"

#include <cctype>
#include <cstdint>
#include <cstring>

using namespace Sexy;

namespace
{

const int CELL_WIDTH = 8;    // the widest glyph is 7 pixels
const int CELL_HEIGHT = 12;
const int ASCENT = 10;       // the baseline is under cell row 9
const int TABLE_TOP = 2;     // the tables below start at cell row 2: capitals fill rows 2..9, lower case rows
                             // 4..9, descenders rows 10..11; accents on capitals use rows 0..1
const int SPACE_WIDTH = 3;

// Eight glyphs side by side per block, separated by a space: '#' is a set pixel. Each glyph keeps its width
// on every row; the advance is the width plus one.
struct GlyphBlock
{
	const char* mChars;      // the block's characters (Latin-1)
	const char* mRows[10];   // cell rows 2..11
};

const GlyphBlock kBlocks[] =
{
	// ! " # $ % & ' (
	{ "!\"#$%&'(", {
		"# #.# ..... ..#.. ##..# .##.. # ..#",
		"# #.# .#.#. .#### ##..# #..#. # .#.",
		"# ... ##### #.#.. ...#. #..#. . #..",
		"# ... .#.#. .###. ..#.. .##.. . #..",
		"# ... .#.#. ..#.# ..#.. #.#.# . #..",
		"# ... ##### ..#.# .#... #..#. . #..",
		". ... .#.#. ####. #..## #..#. . .#.",
		"# ... ..... ..#.. #..## .##.# . ..#",
		". ... ..... ..... ..... ..... . ...",
		". ... ..... ..... ..... ..... . ..." } },
	// ) * + , - . / 0
	{ ")*+,-./0", {
		"#.. ..... ..... .. .... . ....# .###.",
		".#. ..#.. ..... .. .... . ....# #...#",
		"..# #.#.# ..#.. .. .... . ...#. #..##",
		"..# .###. ..#.. .. .... . ..#.. #.#.#",
		"..# #.#.# ##### .. #### . ..#.. #.#.#",
		"..# ..#.. ..#.. .. .... . .#... ##..#",
		".#. ..... ..#.. .# .... . #.... #...#",
		"#.. ..... ..... .# .... # #.... .###.",
		"... ..... ..... #. .... . ..... .....",
		"... ..... ..... .. .... . ..... ....." } },
	// 1 2 3 4 5 6 7 8
	{ "12345678", {
		".#. .###. .###. ...#. ##### .###. ##### .###.",
		"##. #...# #...# ..##. #.... #.... ....# #...#",
		".#. ....# ....# .#.#. #.... #.... ...#. #...#",
		".#. ...#. ..##. #..#. ####. ####. ...#. .###.",
		".#. ..#.. ....# ##### ....# #...# ..#.. #...#",
		".#. .#... ....# ...#. ....# #...# ..#.. #...#",
		".#. #.... #...# ...#. #...# #...# ..#.. #...#",
		"### ##### .###. ...#. .###. .###. ..#.. .###.",
		"... ..... ..... ..... ..... ..... ..... .....",
		"... ..... ..... ..... ..... ..... ..... ....." } },
	// 9 : ; < = > ? @
	{ "9:;<=>?@", {
		".###. . .. .... .... .... .###. .####.",
		"#...# . .. ...# .... #... #...# #....#",
		"#...# . .. ..#. .... .#.. ....# #.##.#",
		"#...# # .# .#.. #### ..#. ...#. #.#.##",
		".#### . .. #... .... ...# ..#.. #.#.##",
		"....# . .. .#.. #### ..#. ..#.. #..#.#",
		"....# . .. ..#. .... .#.. ..... #.....",
		".###. # .# ...# .... #... ..#.. .####.",
		"..... . #. .... .... .... ..... ......",
		"..... . .. .... .... .... ..... ......" } },
	// A B C D E F G H
	{ "ABCDEFGH", {
		".###. ####. .###. ####. ##### ##### .###. #...#",
		"#...# #...# #...# #...# #.... #.... #...# #...#",
		"#...# #...# #.... #...# #.... #.... #.... #...#",
		"#...# ####. #.... #...# ####. ####. #.... #####",
		"##### #...# #.... #...# #.... #.... #.### #...#",
		"#...# #...# #.... #...# #.... #.... #...# #...#",
		"#...# #...# #...# #...# #.... #.... #...# #...#",
		"#...# ####. .###. ####. ##### #.... .###. #...#",
		"..... ..... ..... ..... ..... ..... ..... .....",
		"..... ..... ..... ..... ..... ..... ..... ....." } },
	// I J K L M N O P
	{ "IJKLMNOP", {
		"### .### #...# #.... #.....# #...# .###. ####.",
		".#. ...# #..#. #.... ##...## ##..# #...# #...#",
		".#. ...# #.#.. #.... #.#.#.# ##..# #...# #...#",
		".#. ...# ##... #.... #..#..# #.#.# #...# #...#",
		".#. ...# #.#.. #.... #.....# #.#.# #...# ####.",
		".#. #..# #..#. #.... #.....# #..## #...# #....",
		".#. #..# #...# #.... #.....# #..## #...# #....",
		"### .##. #...# ##### #.....# #...# .###. #....",
		"... .... ..... ..... ....... ..... ..... .....",
		"... .... ..... ..... ....... ..... ..... ....." } },
	// Q R S T U V W X
	{ "QRSTUVWX", {
		".###. ####. .###. ##### #...# #...# #.....# #...#",
		"#...# #...# #...# ..#.. #...# #...# #.....# #...#",
		"#...# #...# #.... ..#.. #...# #...# #.....# .#.#.",
		"#...# #...# .###. ..#.. #...# #...# #..#..# ..#..",
		"#...# ####. ....# ..#.. #...# #...# #..#..# ..#..",
		"#.#.# #.#.. ....# ..#.. #...# .#.#. #.#.#.# .#.#.",
		"#..#. #..#. #...# ..#.. #...# .#.#. ##...## #...#",
		".##.# #...# .###. ..#.. .###. ..#.. #.....# #...#",
		"..... ..... ..... ..... ..... ..... ....... .....",
		"..... ..... ..... ..... ..... ..... ....... ....." } },
	// Y Z [ \ ] ^ _ `
	{ "YZ[\\]^_`", {
		"#...# ##### ## #.... ## ..#.. ..... #.",
		"#...# ....# #. #.... .# .#.#. ..... .#",
		".#.#. ...#. #. .#... .# #...# ..... ..",
		"..#.. ..#.. #. ..#.. .# ..... ..... ..",
		"..#.. .#... #. ..#.. .# ..... ..... ..",
		"..#.. #.... #. ...#. .# ..... ..... ..",
		"..#.. #.... #. ....# .# ..... ..... ..",
		"..#.. ##### ## ....# ## ..... ..... ..",
		"..... ..... .. ..... .. ..... ##### ..",
		"..... ..... .. ..... .. ..... ..... .." } },
	// a b c d e f g h
	{ "abcdefgh", {
		"..... #.... ..... ....# ..... ..## ..... #....",
		"..... #.... ..... ....# ..... .#.. ..... #....",
		".###. ####. .###. .#### .###. #### .#### ####.",
		"....# #...# #...# #...# #...# .#.. #...# #...#",
		".#### #...# #.... #...# ##### .#.. #...# #...#",
		"#...# #...# #.... #...# #.... .#.. #...# #...#",
		"#...# #...# #...# #...# #...# .#.. #...# #...#",
		".#### ####. .###. .#### .###. .#.. .#### #...#",
		"..... ..... ..... ..... ..... .... ....# .....",
		"..... ..... ..... ..... ..... .... .###. ....." } },
	// i j k l m n o p
	{ "ijklmnop", {
		"# ..# #... #. ....... ..... ..... .....",
		". ... #... #. ....... ..... ..... .....",
		"# .## #..# #. ###.##. ####. .###. ####.",
		"# ..# #.#. #. #..#..# #...# #...# #...#",
		"# ..# ##.. #. #..#..# #...# #...# #...#",
		"# ..# #.#. #. #..#..# #...# #...# #...#",
		"# ..# #..# #. #..#..# #...# #...# #...#",
		"# ..# #..# .# #..#..# #...# .###. ####.",
		". ..# .... .. ....... ..... ..... #....",
		". ##. .... .. ....... ..... ..... #...." } },
	// q r s t u v w x
	{ "qrstuvwx", {
		"..... .... ..... .#.. ..... ..... ....... .....",
		"..... .... ..... .#.. ..... ..... ....... .....",
		".#### #.## .#### #### #...# #...# #.....# #...#",
		"#...# ##.. #.... .#.. #...# #...# #.....# .#.#.",
		"#...# #... .###. .#.. #...# #...# #..#..# ..#..",
		"#...# #... ....# .#.. #...# .#.#. #..#..# ..#..",
		"#...# #... ....# .#.. #...# .#.#. #.#.#.# .#.#.",
		".#### #... ####. ..## .#### ..#.. .#...#. #...#",
		"....# .... ..... .... ..... ..... ....... .....",
		"....# .... ..... .... ..... ..... ....... ....." } },
	// y z { | } ~ xA1 xA2
	{ "yz{|}~\xA1\xA2", {
		"..... ..... ..# # #.. ..... . .....",
		"..... ..... .#. # .#. ..... . ..#..",
		"#...# ##### .#. # .#. ..... # .####",
		"#...# ...#. #.. # ..# .##.# . #.#..",
		"#...# ..#.. .#. # .#. #..#. # #.#..",
		"#...# .#... .#. # .#. ..... # #.#..",
		"#...# #.... .#. # .#. ..... # .####",
		".#### ##### ..# # #.. ..... # ..#..",
		"....# ..... ... # ... ..... # .....",
		".###. ..... ... . ... ..... # ....." } },
	// xA3 xA4 xA5 xA6 xA7 xA8 xA9 xAA
	{ "\xA3\xA4\xA5\xA6\xA7\xA8\xA9\xAA", {
		"..##. ..... #...# # .### #.# .#####. .##.",
		".#..# #...# #...# # #... ... #.....# ...#",
		".#... .###. .#.#. # .##. ... #..##.# .###",
		"###.. .#.#. ..#.. . #..# ... #.#...# #..#",
		".#... .#.#. ##### . #..# ... #.#...# .###",
		".#... .###. ..#.. # .##. ... #..##.# ....",
		".#... #...# ##### # ...# ... #.....# ####",
		"##### ..... ..#.. # ###. ... .#####. ....",
		"..... ..... ..... # .... ... ....... ....",
		"..... ..... ..... . .... ... ....... ...." } },
	// xAB xAC xAD xAE xAF xB0 xB1 xB2
	{ "\xAB\xAC\xAD\xAE\xAF\xB0\xB1\xB2", {
		"..... ..... .... .#####. ##### .##. ..... ##.",
		"..... ..... .... #.....# ..... #..# ..#.. ..#",
		"..#.# ..... .... #.###.# ..... #..# ..#.. .#.",
		".#.#. ..... .... #.#.#.# ..... .##. ##### #..",
		"#.#.. ##### #### #.##..# ..... .... ..#.. ###",
		".#.#. ....# .... #.#.#.# ..... .... ..#.. ...",
		"..#.# ....# .... #.....# ..... .... ..... ...",
		"..... ..... .... .#####. ..... .... ##### ...",
		"..... ..... .... ....... ..... .... ..... ...",
		"..... ..... .... ....... ..... .... ..... ..." } },
	// xB3 xB4 xB5 xB6 xB7 xB8 xB9 xBA
	{ "\xB3\xB4\xB5\xB6\xB7\xB8\xB9\xBA", {
		"##. .# ..... .#### . .. .#. .##.",
		"..# #. ..... ###.# . .. ##. #..#",
		".#. .. #...# ###.# . .. .#. #..#",
		"..# .. #...# .##.# . .. .#. .##.",
		"##. .. #...# ..#.# # .. ### ....",
		"... .. #...# ..#.# . .. ... ####",
		"... .. ##..# ..#.# . .. ... ....",
		"... .. #.##. ..#.# . .. ... ....",
		"... .. #.... ..... . .# ... ....",
		"... .. #.... ..... . #. ... ...." } },
	// xBB xBC xBD xBE xBF xC6 xD0 xD7
	{ "\xBB\xBC\xBD\xBE\xBF\xC6\xD0\xD7", {
		"..... #....#. #....#. ##...#. ..... ..##### .####. .....",
		"..... #...#.. #...#.. .#..#.. ..... .#.#... .#...# .....",
		"#.#.. #..#... #..#... ##.#... ..#.. #..#... .#...# #...#",
		".#.#. ..#..#. ..#.##. .##..#. ..... #..#### ####.# .#.#.",
		"..#.# .#..##. .#....# ##..##. ..#.. #####.. .#...# ..#..",
		".#.#. #..#.#. #....#. ...#.#. ..#.. #..#... .#...# .#.#.",
		"#.#.. ...#### ....#.. ..#.### .#... #..#... .#...# #...#",
		"..... .....#. ....### .....#. #.... #..#### .####. .....",
		"..... ....... ....... ....... #...# ....... ...... .....",
		"..... ....... ....... ....... .###. ....... ...... ....." } },
	// xD8 xDE xDF xE6 xF0 xF7 xF8 xFE
	{ "\xD8\xDE\xDF\xE6\xF0\xF7\xF8\xFE", {
		".###. #.... .##.. ....... .#.#. ..... ..... #....",
		"#..## ####. #..#. ....... ..#.. ..... ..... #....",
		"#..## #...# #..#. .##.##. .#.#. ..#.. .###. ####.",
		"#.#.# #...# #.#.. ...#..# ....# ..... #..## #...#",
		"#.#.# #...# #..#. .###### .#### ##### #.#.# #...#",
		"##..# ####. #...# #..#... #...# ..... #.#.# #...#",
		"##..# #.... #...# #..#..# #...# ..#.. ##..# #...#",
		".###. #.... #.##. .##.##. .###. ..... .###. ####.",
		"..... ..... ..... ....... ..... ..... ..... #....",
		"..... ..... ..... ....... ..... ..... ..... #...." } },
};

// Accents for the composed letters, top row first
enum
{
	ACCENT_GRAVE,
	ACCENT_ACUTE,
	ACCENT_CIRCUMFLEX,
	ACCENT_TILDE,
	ACCENT_DIAERESIS,
	ACCENT_RING,
	ACCENT_CEDILLA
};

const char* const kAccents[][3] =
{
	{ "#.", ".#", NULL },
	{ ".#", "#.", NULL },
	{ ".#.", "#.#", NULL },
	{ ".#.#", "#.#.", NULL },
	{ "#.#", NULL, NULL },
	{ ".#.", "#.#", NULL },   // on a capital the letter's top closes the ring
	{ ".#", "#.", NULL }      // under the letter
};
const char* const kLowerRing[3] = { ".#.", "#.#", ".#." };

// The accented letters: a base letter ('1' for a dotless i) and an accent
struct ComposedGlyph
{
	unsigned char mChar;
	char mBase;
	int mAccent;
};

const ComposedGlyph kComposed[] =
{
	{ 0xC0, 'A', ACCENT_GRAVE }, { 0xC1, 'A', ACCENT_ACUTE }, { 0xC2, 'A', ACCENT_CIRCUMFLEX },
	{ 0xC3, 'A', ACCENT_TILDE }, { 0xC4, 'A', ACCENT_DIAERESIS }, { 0xC5, 'A', ACCENT_RING },
	{ 0xC7, 'C', ACCENT_CEDILLA },
	{ 0xC8, 'E', ACCENT_GRAVE }, { 0xC9, 'E', ACCENT_ACUTE }, { 0xCA, 'E', ACCENT_CIRCUMFLEX }, { 0xCB, 'E', ACCENT_DIAERESIS },
	{ 0xCC, 'I', ACCENT_GRAVE }, { 0xCD, 'I', ACCENT_ACUTE }, { 0xCE, 'I', ACCENT_CIRCUMFLEX }, { 0xCF, 'I', ACCENT_DIAERESIS },
	{ 0xD1, 'N', ACCENT_TILDE },
	{ 0xD2, 'O', ACCENT_GRAVE }, { 0xD3, 'O', ACCENT_ACUTE }, { 0xD4, 'O', ACCENT_CIRCUMFLEX },
	{ 0xD5, 'O', ACCENT_TILDE }, { 0xD6, 'O', ACCENT_DIAERESIS },
	{ 0xD9, 'U', ACCENT_GRAVE }, { 0xDA, 'U', ACCENT_ACUTE }, { 0xDB, 'U', ACCENT_CIRCUMFLEX }, { 0xDC, 'U', ACCENT_DIAERESIS },
	{ 0xDD, 'Y', ACCENT_ACUTE },
	{ 0xE0, 'a', ACCENT_GRAVE }, { 0xE1, 'a', ACCENT_ACUTE }, { 0xE2, 'a', ACCENT_CIRCUMFLEX },
	{ 0xE3, 'a', ACCENT_TILDE }, { 0xE4, 'a', ACCENT_DIAERESIS }, { 0xE5, 'a', ACCENT_RING },
	{ 0xE7, 'c', ACCENT_CEDILLA },
	{ 0xE8, 'e', ACCENT_GRAVE }, { 0xE9, 'e', ACCENT_ACUTE }, { 0xEA, 'e', ACCENT_CIRCUMFLEX }, { 0xEB, 'e', ACCENT_DIAERESIS },
	{ 0xEC, '1', ACCENT_GRAVE }, { 0xED, '1', ACCENT_ACUTE }, { 0xEE, '1', ACCENT_CIRCUMFLEX }, { 0xEF, '1', ACCENT_DIAERESIS },
	{ 0xF1, 'n', ACCENT_TILDE },
	{ 0xF2, 'o', ACCENT_GRAVE }, { 0xF3, 'o', ACCENT_ACUTE }, { 0xF4, 'o', ACCENT_CIRCUMFLEX },
	{ 0xF5, 'o', ACCENT_TILDE }, { 0xF6, 'o', ACCENT_DIAERESIS },
	{ 0xF9, 'u', ACCENT_GRAVE }, { 0xFA, 'u', ACCENT_ACUTE }, { 0xFB, 'u', ACCENT_CIRCUMFLEX }, { 0xFC, 'u', ACCENT_DIAERESIS },
	{ 0xFD, 'y', ACCENT_ACUTE }, { 0xFF, 'y', ACCENT_DIAERESIS }
};

struct Glyph
{
	int mWidth;
	uint16_t mRows[CELL_HEIGHT];  // bit x is pixel x
	bool mDefined;
};

struct GlyphSet
{
	Glyph mGlyphs[256];

	GlyphSet()
	{
		memset(mGlyphs, 0, sizeof(mGlyphs));

		for (const GlyphBlock& aBlock : kBlocks)
		{
			for (int aRow = 0; aRow < 10; aRow++)
			{
				const char* p = aBlock.mRows[aRow];
				for (size_t i = 0; aBlock.mChars[i] != 0; i++)
				{
					Glyph& aGlyph = mGlyphs[(unsigned char) aBlock.mChars[i]];
					int x = 0;
					for (; (*p != 0) && (*p != ' '); p++, x++)
					{
						if (*p == '#')
							aGlyph.mRows[TABLE_TOP + aRow] |= (uint16_t) (1 << x);
					}
					aGlyph.mWidth = x;
					aGlyph.mDefined = true;
					if (*p == ' ')
						p++;
				}
			}
		}

		mGlyphs[(unsigned char) ' '].mWidth = SPACE_WIDTH;
		mGlyphs[(unsigned char) ' '].mDefined = true;
		mGlyphs[0xA0] = mGlyphs[(unsigned char) ' '];

		for (const ComposedGlyph& aComposed : kComposed)
		{
			Glyph aBase = mGlyphs[(unsigned char) ((aComposed.mBase == '1') ? 'i' : aComposed.mBase)];
			if (aComposed.mBase == '1')
			{
				// the dotless i: the i without its dot
				aBase.mRows[TABLE_TOP] = 0;
				aBase.mRows[TABLE_TOP + 1] = 0;
			}
			bool isCapital = isupper((unsigned char) aComposed.mBase) != 0;
			const char* const* anAccent = kAccents[aComposed.mAccent];
			if ((aComposed.mAccent == ACCENT_RING) && !isCapital)
				anAccent = kLowerRing;
			int aRowCount = 0;
			while ((aRowCount < 3) && (anAccent[aRowCount] != NULL))
				aRowCount++;
			int anAccentWidth = (int) strlen(anAccent[0]);
			int aTop = (aComposed.mAccent == ACCENT_CEDILLA) ? 10 : isCapital ? 0 : 4 - aRowCount;

			Glyph aGlyph;
			memset(&aGlyph, 0, sizeof(aGlyph));
			aGlyph.mWidth = (aBase.mWidth > anAccentWidth) ? aBase.mWidth : anAccentWidth;
			aGlyph.mDefined = true;
			int aBaseOffset = (aGlyph.mWidth - aBase.mWidth) / 2;
			int anAccentOffset = (aGlyph.mWidth - anAccentWidth) / 2;
			for (int y = 0; y < CELL_HEIGHT; y++)
				aGlyph.mRows[y] = (uint16_t) (aBase.mRows[y] << aBaseOffset);
			for (int y = 0; y < aRowCount; y++)
			{
				for (int x = 0; anAccent[y][x] != 0; x++)
				{
					if (anAccent[y][x] == '#')
						aGlyph.mRows[aTop + y] |= (uint16_t) (1 << (x + anAccentOffset));
				}
			}
			mGlyphs[aComposed.mChar] = aGlyph;
		}
	}
};

const GlyphSet& Glyphs()
{
	static GlyphSet sGlyphs;
	return sGlyphs;
}

// The cell a character is drawn from: '?' for one the font lacks (control characters, 0x7F..0x9F)
unsigned char CellOf(SexyChar theChar)
{
	return Glyphs().mGlyphs[(unsigned char) theChar].mDefined ? (unsigned char) theChar : (unsigned char) '?';
}

const Glyph& GlyphOf(SexyChar theChar)
{
	return Glyphs().mGlyphs[CellOf(theChar)];
}

DDImage* gGlyphImage = NULL;

Image* GlyphImage()
{
	if (gGlyphImage != NULL)
		return gGlyphImage;
	if ((gSexyAppBase == NULL) || (gSexyAppBase->mDDInterface == NULL))
		return NULL;

	const GlyphSet& aSet = Glyphs();
	int aPitch = 16 * CELL_WIDTH;
	gGlyphImage = new DDImage(gSexyAppBase->mDDInterface);
	gGlyphImage->Create(aPitch, 16 * CELL_HEIGHT);
	ulong* aBits = gGlyphImage->GetBits();
	for (int i = 0; i < aPitch * 16 * CELL_HEIGHT; i++)
		aBits[i] = 0x00FFFFFF;
	for (int c = 0; c < 256; c++)
	{
		const Glyph& aGlyph = aSet.mGlyphs[c];
		int aCellX = (c % 16) * CELL_WIDTH;
		int aCellY = (c / 16) * CELL_HEIGHT;
		for (int y = 0; y < CELL_HEIGHT; y++)
		{
			for (int x = 0; x < aGlyph.mWidth; x++)
			{
				if (aGlyph.mRows[y] & (1 << x))
					aBits[(aCellY + y) * aPitch + aCellX + x] = 0xFFFFFFFF;
			}
		}
	}
	gGlyphImage->mHasTrans = true;
	gGlyphImage->mHasAlpha = true;
	gGlyphImage->BitsChanged();
	return gGlyphImage;
}

}

BuiltinFont::BuiltinFont(bool isBold, int theScale)
{
	mBold = isBold;
	mScale = (theScale >= 1) ? theScale : 1;
	mAscent = ASCENT * mScale;
	mAscentPadding = 0;
	mHeight = CELL_HEIGHT * mScale;
	mLineSpacingOffset = 2 * mScale;
}

BuiltinFont::BuiltinFont(const BuiltinFont& theFont) :
	Font(theFont),
	mBold(theFont.mBold),
	mScale(theFont.mScale)
{
}

BuiltinFont::~BuiltinFont()
{
}

int BuiltinFont::CharWidth(SexyChar theChar)
{
	return (GlyphOf(theChar).mWidth + 1 + (mBold ? 1 : 0)) * mScale;
}

int BuiltinFont::CharWidthKern(SexyChar theChar, SexyChar /*thePrevChar*/)
{
	return CharWidth(theChar);
}

int BuiltinFont::StringWidth(const SexyString& theString)
{
	int aWidth = 0;
	for (size_t i = 0; i < theString.size(); i++)
		aWidth += CharWidth(theString[i]);
	return aWidth;
}

void BuiltinFont::DrawString(Graphics* g, int theX, int theY, const SexyString& theString, const Color& theColor, const Rect& /*theClipRect*/)
{
	Image* anImage = GlyphImage();
	if (anImage == NULL)
		return;

	bool wasColorized = g->GetColorizeImages();
	Color anOldColor = g->GetColor();
	g->SetColorizeImages(true);
	g->SetColor(theColor);

	int x = theX;
	int aTop = theY - ASCENT * mScale;
	for (size_t i = 0; i < theString.size(); i++)
	{
		unsigned char aCell = CellOf(theString[i]);
		const Glyph& aGlyph = Glyphs().mGlyphs[aCell];
		if (aGlyph.mWidth > 0)
		{
			Rect aSrc((aCell % 16) * CELL_WIDTH, (aCell / 16) * CELL_HEIGHT, aGlyph.mWidth, CELL_HEIGHT);
			for (int aPass = 0; aPass < (mBold ? 2 : 1); aPass++)
			{
				if (mScale == 1)
					g->DrawImage(anImage, x + aPass, aTop, aSrc);
				else
					g->DrawImage(anImage, Rect(x + aPass * mScale, aTop, aGlyph.mWidth * mScale, CELL_HEIGHT * mScale), aSrc);
			}
		}
		x += CharWidth(theString[i]);
	}

	g->SetColor(anOldColor);
	g->SetColorizeImages(wasColorized);
}

Font* BuiltinFont::Duplicate()
{
	return new BuiltinFont(*this);
}

void BuiltinFont::FreeImage()
{
	delete gGlyphImage;
	gGlyphImage = NULL;
}

std::string BuiltinFont::GlyphRows(unsigned char theChar)
{
	const Glyph& aGlyph = GlyphOf((SexyChar) theChar);
	std::string aText;
	for (int y = 0; y < CELL_HEIGHT; y++)
	{
		for (int x = 0; x < aGlyph.mWidth; x++)
			aText += (aGlyph.mRows[y] & (1 << x)) ? '#' : '.';
		aText += '\n';
	}
	return aText;
}
