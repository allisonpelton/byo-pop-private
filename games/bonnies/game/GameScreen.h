#ifndef __BONNIES_GAMESCREEN_H__
#define __BONNIES_GAMESCREEN_H__

// The game's screen (state 3, the vtable 0x457508, 0x120 bytes, ctor 0x402e80, the object 0x478510): the intro of
// each chapter (Bonnie typing its text), the board, the chapter's summary and the tutorial's hints.

#include "Screen.h"
#include "Str.h"

namespace bonnies
{

struct Event;
struct Font;
struct Image;

// 0x40eec0: the top left of the cell at theColumn, theRow, in screen fractions
void cellPos(int theColumn, int theRow, float* theX, float* theY);
// 0x40ef30: the top left of the board's first cell
void firstCellPos(float* theX, float* theY);
// 0x40eab0: theText (a C string) drawn in theFont
double drawCText(Font* theFont, const char* theText, float theX, float theY, float theW, float theH,
	uint32_t theColor, uint32_t theColor2, uint32_t theFlags, float theScale, float theAngle);
// 0x4101d0: the tile under theX, theY (theFree: anywhere in its cell, else near its centre); its distance from it
int hitTile(float theX, float theY, int* theColumn, int* theRow, float* theDistance, int theFree);

class GameScreen : public Screen
{
public:
	// 0x402e80
	GameScreen();

	// 0x410c00
	void init() override;
	// 0x40ee60
	void shutdown() override;
	// 0x413020
	void draw() override;
	// 0x413ae0
	void update() override;
	// 0x414400
	int handleEvent(Event* theEvent) override;

	// 0x40ed00
	void refresh();
	// 0x40edb0
	void reset();
	// 0x40f200: the centre of the tile at theColumn, theRow, in screen fractions
	void tilePos(int theColumn, int theRow, float* theX, float* theY);
	// 0x40f000: the rectangle of what changed (theAll: ...)
	void getRect(float* theX0, float* theY0, float* theX1, float* theY1, int theAll);
	// 0x40eb50: the chapter shown (its name)
	void setChapter(Str theName);
	// 0x410740: the screen flashes theColor for theTime ms
	void flash(int theTime, uint32_t theColor);
	// 0x40ed40: theText on the screen for theTime ms
	void setMessage(Str theText, int theTime);
	// 0x4103a0: the hint of tutorial step theStep at theX, theY on side theSide (theFlag: with its button, the board
	// blocked)
	void showHint(Str theText, int theStep, float theX, float theY, int theSide, int theFlag);
	// 0x410650: whether mStrCC is set
	bool isBusy();
	// 0x410660: the shuffle button's text (the last move ends the game)
	void updateHint();
	// 0x4106e0: a word on the board shown as the hint
	void showHintWord();
	// 0x410770: in play (mField58 is 1) and the game's 0xc60 not set
	int isPlaying();
	// 0x410790: the next step of the sequence
	void advance();
	// 0x413a30: the tutorial's hint put away (theClear: the marks on the board too)
	void leave(int theClear);
	// 0x40f3e0: Bonnie typing the chapter's text
	void drawIntro();
	// 0x410f90: the board and its panels
	void drawBoard();

	Str mChapter;             // +0x48
	Image* mPicture;          // +0x50: the chapter's picture
	Image* mField54;          // +0x54: the last chapter's
	int mField58;             // +0x58: the state: 1 in play, 2 the level won, 3-16 the steps after it, 0x11-0x13
	int mField5C;             // +0x5c: the step's start
	int mField60;             // +0x60: the next step's time (0 none)
	int mHintTime;            // +0x64: the last move
	int mHintShown;           // +0x68
	int mHintAgain;           // +0x6c
	int mField70;             // +0x70
	int mField74;             // +0x74
	int mField78;             // +0x78: the marked cells blink
	int mField7C;             // +0x7c: a '*' is at the bottom
	int mField80;             // +0x80: the last click's cell
	int mField84;             // +0x84
	int mField88;             // +0x88: the cursor's cell (-1 none)
	int mField8C;             // +0x8c
	float mField90;           // +0x90: the mouse
	float mField94;           // +0x94
	Str mMessage;             // +0x98
	uint32_t mMessageEnd;     // +0xa0
	int mFieldA4;             // +0xa4: a button is down
	int mFieldA8;             // +0xa8
	int mFieldAC;             // +0xac: the shuffle button ends the game
	int mFieldB0;             // +0xb0
	float mFieldB4;           // +0xb4: the fades
	float mFieldB8;           // +0xb8: the board's
	float mFieldBC;           // +0xbc: the summary's
	float mFieldC0;           // +0xc0
	float mFieldC4;           // +0xc4
	uint32_t mFieldC8;        // +0xc8
	Str mStrCC;               // +0xcc: the tutorial's hint
	int mTutorialStep;        // +0xd4: the hint's step
	float mHintX;             // +0xd8
	float mHintY;             // +0xdc
	int mHintSide;            // +0xe0
	int mFieldE4;             // +0xe4: the bonus count's time
	int mFieldE8;             // +0xe8: the points counted
	int mFieldEC;             // +0xec: the count's step
	uint32_t mFieldF0;        // +0xf0
	uint32_t mFieldF4;        // +0xf4: the last update
	uint32_t mFlashTime;      // +0xf8
	int mFlashLength;         // +0xfc
	uint32_t mFlashColor;     // +0x100
	int mTyped;               // +0x104: the characters typed (9999 all)
	int mTypeTime;            // +0x108
	int mSpacesTyped;         // +0x10c
	int mSpaces;              // +0x110
	int mLinesTyped;          // +0x114
	int mLines;               // +0x118
	int mField11C;            // +0x11c
};

} // namespace bonnies

#endif
