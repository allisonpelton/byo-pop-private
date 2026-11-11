#ifndef __COSMICBUGS_R64TEXTINPUT_H__
#define __COSMICBUGS_R64TEXTINPUT_H__

// Retro64's one-line text entry (0x42c260..0x42d1f0): a box with a frame, a fill and a blinking caret, typed into
// from the key queue (R64Sys.h) in a bitmap font. It draws straight onto the screen and adds what it drew to the
// dirty rectangles. (The original can also draw onto a surface of its own, with a background image or black under
// it, as the game never asks, the port leaves it out.)

#include "R64Font.h"
#include "R64Draw.h"

namespace r64
{

// 0x4e8 bytes in the original, whose offsets these are. Left out: its surface (+0x08), background (+0x0c), wasActive
// (+0x38) and redraw (+0x80), all for the own surface, so a member after one of them sits 4, 8, 12 or 16 bytes lower
// here.
struct TextInput
{
	int mFontPadding[2];        // +0x00, +0x04  nothing reads them
	Font* mFont;                // +0x10
	int mX;                     // +0x14
	int mY;                     // +0x18
	int mW;                     // +0x1c
	int mH;                     // +0x20
	int mBox[4];                // +0x24  l, t, r, b (inclusive): clicks inside focus it
	int mActive;                // +0x34  0: not drawn and not updated
	int mFrame[4];              // +0x3c
	int mInner[4];              // +0x4c  the frame less a pixel: the text and the caret are clipped to it
	int mPadX;                  // +0x5c
	int mPadY;                  // +0x60
	int mTextRect[4];           // +0x64  the frame less 4 pixels: the text sits on its bottom
	uint8_t mFrameColour[4];    // +0x74  r, g, b, a (a 0: no frame)
	uint8_t mFillColour[4];     // +0x78  (a 0: no fill)
	char mColourCode;           // +0x7c  the font's "^c" the text is drawn in
	int mFocused;               // +0x84  1: takes keys and shows the caret
	uint8_t mCaretColour[4];    // +0x88
	int mCaretOn;               // +0x8c  the caret's blink
	double mBlinkTime;          // +0x90  seconds
	double mBlinkClock;         // +0x98
	int mCaretWidth;            // +0xa0  at the end of the text
	int mCaretHeight;           // +0xa4
	int mCaretDrop;             // +0xa8  above the text's bottom
	int mCaret;                 // +0xac  the character it is before
	void (*mOnEnter)(const char* theText); // +0xb0
	void (*mOnKey)(int theKey); // +0xb4
	void (*mOnFocus)(int theFocused); // +0xb8
	int mEntered;               // +0xbc  this update: return was pressed
	int mTyped;                 // +0xc0  this update: a key was taken
	int mClickedIn;             // +0xc4  this update: a click focused it
	int mMaxLength;             // +0xc8
	int mLength;                // +0xcc
	char* mText;                // +0xd0  maxLength + 1 bytes
	int mScroll;                // +0xd4  pixels of the text left of the box
	int mPressed;               // +0xd8  the left button went down inside
	Palette* mOutline;          // +0xdc
	int mBlocked[255];          // +0xe0  by character: 1 not typable here (nothing here blocks any)
	int mNoFocus;               // +0x4dc  1: clicks are ignored and it drops the focus at each update (nothing sets it)
	int mCaseMode;              // +0x4e0  0 as typed, 1 capitals, 2 small letters (shift for the rest)
	int mEndPadding;            // +0x4e4  nothing reads it

	TextInput();  // 0x42ca90
	~TextInput(); // 0x42caa0
	TextInput(const TextInput&) = delete;
	TextInput& operator=(const TextInput&) = delete;
};

void TextInput_Clear(TextInput* theInput);                                             // 0x42c280
void TextInput_Init(TextInput* theInput, Font* theFont, int theMaxLength, char theColourCode, int l, int t, int r,
	int b);                                                                               // 0x42c3b0
void TextInput_SetFillColour(TextInput* theInput, uint8_t r, uint8_t g, uint8_t b, uint8_t a); // 0x42c5e0
void TextInput_SetOutline(TextInput* theInput, char theCode);                         // 0x42c610
void TextInput_SetOnEnter(TextInput* theInput, void (*theFunc)(const char*));         // 0x42c630
void TextInput_SetFocus(TextInput* theInput, int theFocused);                         // 0x42c640
char* TextInput_GetText(TextInput* theInput);                                         // 0x42c660
void TextInput_SetCaseMode(TextInput* theInput, int theMode);                         // 0x42c670
void TextInput_Draw(TextInput* theInput, uint8_t* theDst, int theDPitch, const PixelFormat* theFmt, Dirty* theDirty); // 0x42cb50
void TextInput_SetText(TextInput* theInput, const char* theText);                     // 0x42ccf0
void TextInput_Update(TextInput* theInput, double theSeconds);                        // 0x42cde0

} // namespace r64

#endif
