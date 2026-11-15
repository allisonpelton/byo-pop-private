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
	// 0x410650: whether mHintText is set
	bool isBusy();
	// 0x410660: the shuffle button's text (the last move ends the game)
	void updateHint();
	// 0x4106e0: a word on the board shown as the hint
	void showHintWord();
	// 0x410770: in play (mSequenceState is 1) and the game's mGameOverTime not set
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
	Image* mLastPicture;      // +0x54: the last chapter's
	int mSequenceState;       // +0x58: the state: 1 in play, 2 the level won, 3-16 the steps after it, 0x11-0x13
	int mStepStart;           // +0x5c: the step's start
	int mStepEnd;             // +0x60: the next step's time (0 none)
	int mHintTime;            // +0x64: the last move
	int mHintShown;           // +0x68
	int mHintAgain;           // +0x6c
	int mHintClock;           // +0x70: the idle hint's time
	int mBlinkStart;          // +0x74: when the marked cells began to blink
	int mBlinking;            // +0x78: the marked cells blink
	int mStarAtBottom;        // +0x7c: a '*' is at the bottom
	int mLastColumn;          // +0x80: the last click's cell
	int mLastRow;             // +0x84
	int mCursorColumn;        // +0x88: the cursor's cell (-1 none)
	int mCursorRow;           // +0x8c
	float mMouseX;            // +0x90: the mouse
	float mMouseY;            // +0x94
	Str mMessage;             // +0x98
	uint32_t mMessageEnd;     // +0xa0
	int mMouseDown;           // +0xa4: a button is down
	int mClicks;              // +0xa8: the presses counted towards a double click
	int mShuffleEndsGame;     // +0xac: the shuffle button ends the game
	int mHintDue;             // +0xb0: when the hint shows again (0 none)
	float mLeaveFade;         // +0xb4: the summary's way out into the next chapter, 0 to 1
	float mBoardFade;         // +0xb8: 1 the board shows; the chapter's picture comes in as it falls to 0
	float mSummaryFade;       // +0xbc: the summary's
	float mBookOpen;          // +0xc0: the summary's book opening, 0 to 1
	float mSummaryDark;       // +0xc4: the summary's dark backdrop, 0 to 1
	uint32_t mSummaryStart;   // +0xc8: when the summary began (nothing reads it)
	Str mHintText;            // +0xcc: the tutorial's hint
	int mTutorialStep;        // +0xd4: the hint's step
	float mHintX;             // +0xd8
	float mHintY;             // +0xdc
	int mHintSide;            // +0xe0
	int mCountTime;           // +0xe4: the bonus count's time
	int mCountedPoints;       // +0xe8: the points counted
	int mCountStep;           // +0xec: the count's step
	uint32_t mCountStart;     // +0xf0
	uint32_t mLastUpdate;     // +0xf4: the last update
	uint32_t mFlashTime;      // +0xf8
	int mFlashLength;         // +0xfc
	uint32_t mFlashColor;     // +0x100
	int mTyped;               // +0x104: the characters typed (9999 all)
	int mTypeTime;            // +0x108
	int mSpacesTyped;         // +0x10c
	int mSpaces;              // +0x110
	int mLinesTyped;          // +0x114
	int mLines;               // +0x118
	int mHintsChecked;        // +0x11c: the tutorial's hints have been checked once (the shuffle button waits for it)
};

} // namespace bonnies

#endif
