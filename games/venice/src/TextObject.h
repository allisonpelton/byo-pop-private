// Bitmap fonts and the text objects that draw with them: a text object makes one child object for each letter of its string,
// each showing the letter's rectangle of the font's picture.

#ifndef VENICE_TEXTOBJECT_H
#define VENICE_TEXTOBJECT_H

#include "CursorList.h"
#include "ImageRecord.h"
#include "SceneObject.h"

// one letter's rectangle in the font's picture
struct GlyphRect
{
	float mLeft;
	float mTop;
	float mRight;
	float mBottom;
};

// A font (0x4310 bytes in the original): the picture of the letters in rows, each row as tall as the font's letter height, and
// the order of the letters in the picture, from the character set the data files give
class FontRecord
{
public:
	FontRecord* mNext = nullptr;
	FontRecord* mPrev = nullptr;
	char mName[256] = {}; // the picture's name
	char mCharacterSet[512] = {}; // the letters in the order they come in the picture
	GlyphRect mGlyphs[256] = {}; // by character code
	GlyphRect mKerned[256] = {}; // the glyphs after the data files' kerning, which only the layout of a text reads
	ImageRecord* mImage = nullptr;

	bool Load(const char* theName, const char* theCharacterSet, int theLetterHeight); // 0x4242f0
	void FindGlyphs(int theLetterHeight); // 0x4243a0
	void FindGlyphsInRow(int* theCharacterIndex, const uint32_t* theRow, int theTop, int theWidth, int theHeight); // 0x4244a0
	static bool ColumnHasPixels(int theColumn, const uint32_t* theRow, int theWidth, int theHeight); // 0x424610
	float MeasureWidth(const char* theString, float theScale, float theSpacing) const; // 0x424660
};

extern CursorList<FontRecord> gFonts; // 0x5ba448

// 0x424710: the font of this name, loaded from its picture the first time (a lookup alone gives no character set or height)
FontRecord* AcquireFont(const char* theName, const char* theCharacterSet, int theLetterHeight);
// 0x4247a0: moves a letter's edges in the latest font's layout
void AddFontKerning(const char* theCharacter, float theLeft, float theRight);

const int JustifyLeft = 0;
const int JustifyCenter = 1;
const int JustifyRight = 2;

// 600 bytes in the original
class TextObject : public SceneObject
{
public:
	char mText[256] = {}; // 0x148
	FontRecord* mFont = nullptr; // 0x248
	int mJustification = JustifyLeft; // 0x24c
	int mStagger = 0; // 0x250: milliseconds between one letter's start and the next's
	float mSpacing = 0; // 0x254: the space between the letters, in pixels

	TextObject(); // 0x424810
	void Load(const char* theText, SceneTag* theTag) override; // 0x424860
	// 0x424960: the string goes through the text translator and is set out in letters
	void SetText(const char* theString, const char* theFontName, float theSpacing, int theStagger, int theJustification);
	SceneStage* AddStage(float theMilliseconds, int theLoopStart, int theLoopEnd, int theVisible,
		const char* theName) override; // 0x424b60
	SceneFilter* AddFilter(int theType, int theAttribute, float theAmplitude, float thePeriod,
		float theOffset) override; // 0x424c70
	void SetAttribute(int theAttribute, float theStart, float theEnd) override; // 0x424cd0
	bool IsHot() override; // 0x4254b0
	SceneStage* GoToStageByName(const char* theName) override; // 0x4256e0
	void MakeSparkles(int theCount, float theRed, float theGreen, float theBlue, float theAlpha) override; // 0x425730
	void GetLayoutOffsets(float* theStartX, float* theEndX); // 0x424d50: where the letter at the cursor goes
	void GetBounds(float* theWidth, float* theHeight, float* theCenterX, float* theCenterY); // 0x425220
};

// 0x437c70 and 0x418cc0 cut a text into lines. The line that starts at theStart in theText goes into theLine (which must hold the
// whole text), and the start of the next one is returned. A line is at most theWidth letters; it ends at a '^' (which becomes a
// space) or else at the last space in the width. A text with no space in the width gives an empty line and skips a letter.
int CutTextLine(char* theText, int theStart, int theWidth, char* theLine);

// 0x426fb0: draws a string at once, as a text object (one that is kept for the purpose) that is a child of theObject, put at
// (theX, theY) from it, at one scale and alpha
void DrawTextOnObject(SceneObject* theObject, const char* theString, const char* theFontName, float theX, float theY,
	float theSpacing, int theJustification, float theScale, float theAlpha);

#endif
