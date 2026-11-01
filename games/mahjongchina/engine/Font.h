#ifndef __MAHJONGCHINA_FONT_H__
#define __MAHJONGCHINA_FONT_H__

// The engine's bitmap font (vtable 0x51df90): an image with the glyphs in rows, and a TXT resource that describes
// them. Its "H" line gives the line height, the ascent, the spacing between glyphs, the width of a space and the
// number of rows; each "S" line is a row (its first character, then the width of each glyph); each "K" line gives
// the kerning of one character before others. CharMap (vtable 0x51e444) holds the rows.

#include "JBBase.h"

namespace jb
{

class Object;

class CharMap
{
public:
	// 0xc bytes (0x42ef10)
	struct Kern
	{
		uint16_t mChar;
		int mKern;
		Kern* mNext;
	};
	// 0x18 bytes (vtable 0x51e440)
	struct Row
	{
		uint16_t mFirst = 0;      // +0x04
		int mCount = 0;           // +0x08
		int* mX = nullptr;        // +0x0c
		int* mWidths = nullptr;   // +0x10
		Kern** mKerns = nullptr;  // +0x14 a list per glyph

		// 0x42ef50
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

	// 0x42efc0
	CharMap(int theCapacity);
	// 0x42f0d0
	virtual ~CharMap();

	// 0x42f0f0: an "S" line from thePos
	bool ParseRow(const std::string& theLine, int thePos);
	// 0x42f2e0: a "K" line from thePos; false when no row has its character
	bool ParseKerns(const std::string& theLine, int thePos);
	// 0x42f4b0: theChar's glyph, with its kerning before theNext
	bool Lookup(uint16_t theChar, uint16_t theNext, Glyph& theGlyph);
	// 0x42f580: a space, or a character of a row
	bool Has(uint16_t theChar);
};

class Font
{
public:
	int mImage = 0;           // +0x04 an image id of the manager's table
	bool mKeyed = false;      // +0x08
	uint32_t mKey = 0;        // +0x0c
	int* mWidths = nullptr;   // +0x10 GetWidths' buffer
	int mWidthsSize = 0;      // +0x14
	CharMap* mMap = nullptr;  // +0x18
	int mLineHeight = 0;      // +0x1c
	int mAscent = 0;          // +0x20
	int mSpacing = 0;         // +0x24
	int mSpaceWidth = 0;      // +0x28
	int mTabCount = 0;        // +0x2c
	int mTabs[10] = {};       // +0x30

	// 0x4238f0
	Font() {}
	// 0x423930
	virtual ~Font();

	// 0x423960
	int GetLineHeight() const { return mLineHeight; }
	// 0x423970
	bool Load(int theImage, int theTextId, Module theModule);
	// 0x423bd0
	bool LoadKeyed(int theImage, int theTextId, uint32_t theKey, Module theModule);
	// 0x423c00: the width of a text at theScale percent (0 past 2000 characters)
	int GetWidth(const char* theText, int theScale);
	// 0x423cc0
	bool HasChar(uint8_t theChar);
	// 0x423ce0: the text without the characters the font lacks
	void Filter(std::string& theText);
	// 0x423d80: the index of the character that goes past theMaxWidth, or the length
	int FitCount(const char* theText, int theMaxWidth);
	// 0x423e20: the advance of each character, in a buffer the font owns
	int* GetWidths(const char* theText, int* theLength);
	// 0x423ef0: the number of characters drawn, stopping before a glyph past theMaxWidth (-1: no limit)
	int DrawLeft(Object* theObject, int theX, int theY, const char* theText, int theMaxWidth, int theScaleX,
		int theScaleY, int theAlpha);
	// 0x4240a0
	void DrawCentred(Object* theObject, int theX, int theY, const char* theText, int theWidth, int theScaleX,
		int theScaleY, int theAlpha);
	// 0x4240f0: ending at theX
	void DrawRight(Object* theObject, int theX, int theY, const char* theText, int theScaleX, int theScaleY,
		int theAlpha);
	// 0x424130, 0x4242a0: lines split at '\n', theLineStep apart
	void DrawLines(Object* theObject, int theX, int theY, const char* theText, int theLineStep, int theScaleX,
		int theScaleY, int theAlpha);
	void DrawLinesCentred(Object* theObject, int theX, int theY, const char* theText, int theWidth,
		int theLineStep, int theScaleX, int theScaleY, int theAlpha);
	// 0x424420, 0x424460: wrapped at word ends to theWidth, no lower than theMaxHeight; the number of lines, which
	// theMeasureOnly counts without drawing
	int DrawWrapped(Object* theObject, int theX, int theY, int theWidth, int theMaxHeight, const char* theText,
		int theLineStep, bool theMeasureOnly, int theAlpha);
	int DrawWrappedCentred(Object* theObject, int theX, int theY, int theWidth, int theMaxHeight,
		const char* theText, int theLineStep, bool theMeasureOnly, int theAlpha);
	// 0x4244a0
	int DrawWrappedWorker(Object* theObject, int theX, int theY, int theWidth, int theMaxHeight,
		const char* theText, int theLineStep, bool theMeasureOnly, int theAlpha, bool theCentred);
	// 0x4248b0: at most 10
	void SetTabStops(const int* theStops, int theCount);
	// 0x4248f0: the distance to the next tab stop, or 0
	int GetTabWidth(int thePos);

	// 0x424760: the word from theStart (after its leading spaces) to a ".,:; " it ends with; the position after it,
	// or -1 at the end
	static int NextWord(int theStart, const std::string& theText, std::string& theWord);
	// 0x424880: 1 when the text ends with a space, else 0 (the original's loop steps forward from the last
	// character, so it never counts more than one)
	static int TrailingSpaces(const char* theText);
};

} // namespace jb

#endif
