// ObjString: a line of text in the scene graph (0x46e910..0x46f5a0 in the original). Each letter is a child object
// showing the font's cel for its character, placed beside the last by the cels' widths; the string object itself stays
// hidden, holds the font and the text, and may show a blinking text cursor (the name entry).
#pragma once

#include "Anim.h"
#include "ObjBase.h"

namespace Sexy
{

// The keys a name being typed takes besides the typed characters (ObjBase::sKeyPressed and sKeyHeld)
enum EditKey
{
	KEY_BACKSPACE = 0x08,
	KEY_ENTER = 0x0d,
	KEY_LEFT = 0x25,
	KEY_RIGHT = 0x27,
	KEY_DELETE = 0x2e,
};

class ObjString : public ObjBase
{
public:
	char* mText;             // +0x27c: as many characters as the text Create was given, and no more
	ObjBase** mGlyphs;       // +0x284: a child object a character
	int mLength;             // +0x290: the text's length at Create, the most the string holds
	int* mGlyphX;            // +0x294: each glyph's drawn x, in window pixels, and one past the last
	bool mLeftAligned;       // +0x298: the text starts at the position; otherwise it is centred on it
	int mCursor;             // +0x29c: the text cursor's index, or -1 for none
	float mDrawnY;           // +0x2a0: the drawn y, kept for the cursor
	int mBlink;              // +0x2a4: counts 0 to 31; the cursor shows for the first 16

	ObjString();
	~ObjString();
	bool Update() override;
	void Draw(Graphics* g) override;

	void SetFont(Anim* theFont) { theFont->AddRef(); mAnim = theFont; } // 0x46ec40
	int GetTextLength();
	void Create(const char* theText, bool leftAligned, int theMaxWidth);
	void SetText(const char* theText);
	void SetCursor(int theIndex);
	void MoveCursor(int theDelta);
	void InsertTypedKey();
	void DeleteAtCursor();
	void DeleteBeforeCursor();
	void Trim();

	static float TextWidth(const char* theText, ObjBase* theObj);
};

} // namespace Sexy
