#ifndef __COSMICBUGS_R64TEXTINPUT_H__
#define __COSMICBUGS_R64TEXTINPUT_H__

// Retro64's one-line text entry (0x42c260..0x42d1f0): a box with a frame, a fill and a blinking caret, typed into
// from the key queue (R64Sys.h) in a bitmap font. It draws either straight onto the screen or onto a surface of its
// own (with a background image or black under it), and adds what it drew to the dirty rectangles.

#include "R64Font.h"
#include "R64Draw.h"

namespace r64
{

// 0x4e8 bytes
struct TextInput
{
	int unk00;                 // +0x00
	int unk04;                 // +0x04
	Image* surface;            // +0x08  drawn on instead of the screen (none here: the game passes null)
	Image* background;         // +0x0c  under the text on the surface (null: black)
	Font* font;                // +0x10
	int x;                     // +0x14
	int y;                     // +0x18
	int w;                     // +0x1c
	int h;                     // +0x20
	int box[4];                // +0x24  l, t, r, b (inclusive): clicks inside focus it
	int active;                // +0x34  0: not drawn and not updated
	int wasActive;             // +0x38
	int frame[4];              // +0x3c
	int inner[4];              // +0x4c  the frame less a pixel: the text and the caret are clipped to it
	int padX;                  // +0x5c
	int padY;                  // +0x60
	int textRect[4];           // +0x64  the frame less 4 pixels: the text sits on its bottom
	uint8_t frameColour[4];    // +0x74  r, g, b, a (a 0: no frame)
	uint8_t fillColour[4];     // +0x78  (a 0: no fill)
	char colourCode;           // +0x7c  the font's "^c" the text is drawn in
	int redraw;                // +0x80
	int focused;               // +0x84  1: takes keys and shows the caret
	uint8_t caretColour[4];    // +0x88
	int caretOn;               // +0x8c  the caret's blink
	double blinkTime;          // +0x90  seconds
	double blinkClock;         // +0x98
	int caretWidth;            // +0xa0  at the end of the text
	int caretHeight;           // +0xa4
	int caretDrop;             // +0xa8  above the text's bottom
	int caret;                 // +0xac  the character it is before
	void (*onEnter)(const char* theText); // +0xb0
	void (*onKey)(int theKey); // +0xb4
	void (*onFocus)(int theFocused); // +0xb8
	int entered;               // +0xbc  this update: return was pressed
	int typed;                 // +0xc0  this update: a key was taken
	int clickedIn;             // +0xc4  this update: a click focused it
	int maxLength;             // +0xc8
	int length;                // +0xcc
	char* text;                // +0xd0  maxLength + 1 bytes
	int scroll;                // +0xd4  pixels of the text left of the box
	int pressed;               // +0xd8  the left button went down inside
	Palette* outline;          // +0xdc
	int blocked[255];          // +0xe0  by character: 1 not typable here (nothing here blocks any)
	int noFocus;               // +0x4dc  1: clicks are ignored and it drops the focus at each update (nothing sets it)
	int caseMode;              // +0x4e0  0 as typed, 1 capitals, 2 small letters (shift for the rest)
	int unk4e4;                // +0x4e4

	TextInput();  // 0x42ca90
	~TextInput(); // 0x42caa0
	TextInput(const TextInput&) = delete;
	TextInput& operator=(const TextInput&) = delete;
};

void TextInput_Clear(TextInput* theInput);                                             // 0x42c280
void TextInput_Init(TextInput* theInput, Image* theSurface, Image* theBackground, Font* theFont, int theMaxLength,
	char theColourCode, int l, int t, int r, int b);                                   // 0x42c3b0
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
