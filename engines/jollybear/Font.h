#ifndef __JB_FONT_H__
#define __JB_FONT_H__

// The engine's bitmap font (vtable 0x51ce7c (mahjongchina), 0x51df90 (mahjongjapan)): an image with the glyphs in rows,
// and a TXT resource that describes them. Its "H" line gives the line height, the ascent, the spacing between glyphs,
// the width of a space and the number of rows; each "S" line is a row (its first character, then the width of each
// glyph); each "K" line gives the kerning of one character before others. CharMap (vtable 0x51d390 (mahjongchina),
// 0x51e444 (mahjongjapan)) holds the rows.

#include "JBBase.h"

namespace jb
{

class Object;

class CharMap
{
public:
	// 0xc bytes (0x435490 (mahjongchina), 0x42ef10 (mahjongjapan))
	struct Kern
	{
		uint16_t mChar;
		int mKern;
		Kern* mNext;
	};
	// 0x18 bytes (vtable 0x51d38c (mahjongchina), 0x51e440 (mahjongjapan))
	struct Row
	{
		uint16_t mFirst = 0;      // +0x04
		int mCount = 0;           // +0x08
		int* mX = nullptr;        // +0x0c
		int* mWidths = nullptr;   // +0x10
		Kern** mKerns = nullptr;  // +0x14 a list per glyph

		// 0x4354d0 (mahjongchina), 0x42ef50 (mahjongjapan)
		~Row();
	};
	// What a lookup gives
	struct Glyph
	{
		int mRow, mX, mWidth, mKern;
	};

	Row* mRows = nullptr;  // +0x04
	int mUsed = 0;         // +0x08
	int mCapacity;         // +0x0c

	// 0x435540 (mahjongchina), 0x42efc0 (mahjongjapan)
	CharMap(int theCapacity);
	// 0x435650 (mahjongchina), 0x42f0d0 (mahjongjapan)
	virtual ~CharMap();

	// 0x435670 (mahjongchina), 0x42f0f0 (mahjongjapan): an "S" line from thePos
	bool ParseRow(const std::string& theLine, int thePos);
	// 0x435860 (mahjongchina), 0x42f2e0 (mahjongjapan): a "K" line from thePos; false when no row has its character
	bool ParseKerns(const std::string& theLine, int thePos);
	// 0x435a30 (mahjongchina), 0x42f4b0 (mahjongjapan): theChar's glyph, with its kerning before theNext
	bool Lookup(uint16_t theChar, uint16_t theNext, Glyph& theGlyph);
	// 0x435b00 (mahjongchina), 0x42f580 (mahjongjapan): a space, or a character of a row
	bool Has(uint16_t theChar);
};

class Font
{
public:
	int mImage = 0;           // +0x04 an image id of the manager's table
	int* mWidths = nullptr;   // +0x10 GetWidths' buffer
	int mWidthsSize = 0;      // +0x14
	CharMap* mMap = nullptr;  // +0x18
	int mLineHeight = 0;      // +0x1c
	int mAscent = 0;          // +0x20
	int mSpacing = 0;         // +0x24
	int mSpaceWidth = 0;      // +0x28

	// 0x425b50 (mahjongchina), 0x4238f0 (mahjongjapan)
	Font() {}
	// 0x425b90 (mahjongchina), 0x423930 (mahjongjapan)
	virtual ~Font();

	// 0x423960 (mahjongjapan)
	int GetLineHeight() const { return mLineHeight; }
	// 0x425bc0 (mahjongchina), 0x423970 (mahjongjapan)
	bool Load(int theImage, int theTextId, Module theModule);
	// 0x425e50 (mahjongchina), 0x423c00 (mahjongjapan): the width of a text at theScale percent (0 past 2000
	// characters)
	int GetWidth(const char* theText, int theScale);
	// 0x425f10 (mahjongchina), 0x423cc0 (mahjongjapan)
	bool HasChar(uint8_t theChar);
	// 0x425f30 (mahjongchina), 0x423ce0 (mahjongjapan): the text without the characters the font lacks
	void Filter(std::string& theText);
	// 0x425fd0 (mahjongchina), 0x423d80 (mahjongjapan): the index of the character that goes past theMaxWidth, or the
	// length
	int FitCount(const char* theText, int theMaxWidth);
	// 0x426070 (mahjongchina), 0x423e20 (mahjongjapan): the advance of each character, in a buffer the font owns
	int* GetWidths(const char* theText, int* theLength);
	// 0x426140 (mahjongchina), 0x423ef0 (mahjongjapan): the number of characters drawn, stopping before a glyph past
	// theMaxWidth (-1: no limit)
	int DrawLeft(Object* theObject, int theX, int theY, const char* theText, int theMaxWidth, int theScaleX,
		int theScaleY, int theAlpha);
	// 0x4262f0 (mahjongchina), 0x4240a0 (mahjongjapan)
	void DrawCentred(Object* theObject, int theX, int theY, const char* theText, int theWidth, int theScaleX,
		int theScaleY, int theAlpha);
	// 0x426340 (mahjongchina), 0x4240f0 (mahjongjapan): ending at theX
	void DrawRight(Object* theObject, int theX, int theY, const char* theText, int theScaleX, int theScaleY,
		int theAlpha);
	// 0x426380 (mahjongchina), 0x424130 (mahjongjapan), 0x4264e0 (mahjongchina), 0x4242a0 (mahjongjapan): lines split
	// at '\n', theLineStep apart
	void DrawLines(Object* theObject, int theX, int theY, const char* theText, int theLineStep, int theScaleX,
		int theScaleY, int theAlpha);
	void DrawLinesCentred(Object* theObject, int theX, int theY, const char* theText, int theWidth,
		int theLineStep, int theScaleX, int theScaleY, int theAlpha);
	// 0x426690 (mahjongchina), 0x424460 (mahjongjapan): wrapped at word ends to theWidth, no lower than theMaxHeight;
	// the number of lines, which theMeasureOnly counts without drawing
	int DrawWrappedCentred(Object* theObject, int theX, int theY, int theWidth, int theMaxHeight,
		const char* theText, int theLineStep, bool theMeasureOnly, int theAlpha);
	// 0x4266d0 (mahjongchina), 0x4244a0 (mahjongjapan)
	int DrawWrappedWorker(Object* theObject, int theX, int theY, int theWidth, int theMaxHeight,
		const char* theText, int theLineStep, bool theMeasureOnly, int theAlpha, bool theCentred);

	// 0x426960 (mahjongchina), 0x424760 (mahjongjapan): the word from theStart (after its leading spaces) to a ".,:; "
	// it ends with; the position after it, or -1 at the end
	static int NextWord(int theStart, const std::string& theText, std::string& theWord);
	// 0x426a70 (mahjongchina), 0x424880 (mahjongjapan): 1 when the text ends with a space, else 0 (the original's loop
	// steps forward from the last character, so it never counts more than one)
	static int TrailingSpaces(const char* theText);
};

} // namespace jb

#endif
