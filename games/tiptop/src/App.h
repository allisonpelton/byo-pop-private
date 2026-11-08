#ifndef __TIPTOP_APP_H__
#define __TIPTOP_APP_H__

// The application object (TipTop.exe 0x4197c6-0x426fc7; one instance at 0x488710, 0x198 bytes): the screens, their
// buttons and animations, the score messages and particles, the level summary, the save file and the commands. The
// board (Board.h) is the game itself.

#include "Config.h"
#include "Gui.h"
#include "Lowos.h"

#include <cstdint>

namespace tiptop
{

struct Board;

// A text drawn once into an image of the display's format (0x419991): the score, the time, the bonus line
struct TextImage
{
	uint8_t mDrawn;                // +0x00 the text is in the image
	uint8_t* mBits;                // +0x04
	uint32_t mPitch;               // +0x08
	lowos::Surface* mSurface;      // +0x0c
};

enum
{
	SCREEN_NONE = 0,
	SCREEN_SPLASH = 1,
	SCREEN_MAINMENU = 2,
	SCREEN_OPTIONS = 3,
	SCREEN_INSTRUCTIONS = 4,
	SCREEN_CREDITS = 5,
	SCREEN_GAME = 6,
	SCREEN_LEVELSUMMARY = 7,
	SCREEN_NEWGAME = 8,
	SCREEN_HIGHSCORES = 9,
	SCREEN_GAMEOVER = 10,
};

struct App
{
	lowos::Surface* mBackground;          // +0x00 image 0x26
	lowos::Surface* mLevelComplete[2];    // +0x04 images 0x28, 0x27
	lowos::Surface* mComboAnims[8];       // +0x0c images 0xbc..
	lowos::Surface* mComboStars[4];       // +0x2c images 0xc4..
	lowos::Surface* mBarFlash;            // +0x3c image 0x32: the progress bar when it flashes
	lowos::Surface* mBarLow;              // +0x40 image 0x35: the lower half of the bar's range
	lowos::Surface* mBarHigh;             // +0x44 image 0x34
	lowos::Surface* mBarFrame;            // +0x48 image 0x33: its size is the bar's
	lowos::Surface* mSkin;                // +0x4c image 0x44: the scroll bars (GuiInit)
	uint8_t* mBarBits;                    // +0x50 the bar drawn for the screen
	Font* mScoreFonts[8];                 // +0x54 images 0xcc..: the fonts of the score messages (1-based in 0x41eb9c)
	int32_t mPosX;                        // +0x74 a point the board reports (0x41ee5a)
	int32_t mPosY;                        // +0x78
	uint32_t mBarLevel;                   // +0x7c the bar's height shown
	uint32_t mBarTime;                    // +0x80
	uint32_t mBarDrawn;                   // +0x84 mBarLevel when mBarBits was drawn
	char mBarFlashDrawn;                  // +0x88
	Handler* mInstrPrev;                  // +0x8c
	uint8_t** mParticleImages;            // +0x90 (gParticleState)
	lowos::Surface* mImage68;             // +0x94 image 0x68
	TextImage mTexts[4];                  // +0x98 score, time, bonus line, chain
	Layer* mLayer;                        // +0xd8 the screen's layer
	int32_t mScreen;                      // +0xdc
	Handler* mFullScreenBox;              // +0xe0
	Handler* mNameInput;                  // +0xe4 the text field of the high score name (0x41f922)
	Board* mBoard;                        // +0xe8
	Handler* mEC;                         // +0xec
	int32_t mPrevScreen;                  // +0xf0
	Font* mLevelFont;                     // +0xf4 image 0x49
	Font* mDigitFont;                     // +0xf8 image 0x2a
	Font* mTextFont;                      // +0xfc image 0x67
	Handler* mCounted[4];                 // +0x100 handlers that clear themselves here when freed (CounterProc)
	Handler* mGameButtons[4];             // +0x110 instructions, options, end game, pause
	uint32_t m134;                        // +0x134
	uint32_t m138;                        // +0x138
	char m13c;                            // +0x13c
	char m13d;                            // +0x13d
	char mCrossed;                        // +0x13e set by the board (0x41ee83)
	char m13f;                            // +0x13f
	uint32_t mSummaryEvents;              // +0x140
	uint32_t m144;                        // +0x144

	int LoadImages();                     // 0x4197c6
	void BuildImages();                   // 0x419991
	void MakeTextImage(TextImage* theImage, uint32_t theWidth, uint32_t theHeight, uint32_t theBpp);
	static void ToggleFullScreen(char theMode);  // 0x41b36c
	int Init();                           // 0x41b42e
	void SetConfig(Config* theConfig);    // 0x41b755
	void FreeImages();                    // 0x41b771
	void Shutdown();                      // 0x41b98a
	int Frame();                          // 0x41b9fe
	void NextSummaryEvent();              // 0x41bbce
	void NewGame();                       // 0x41bc36
	void NextLevel();                     // 0x41bca1
	void DrawScoreLevel();                // 0x41bce0
	void DrawGame();                      // 0x41bdcc
	static void HandleKeys();             // 0x41c5bf
	void AddSummaryItems();               // 0x41c6dd
	void AddGameButtons(Layer* theLayer); // 0x41c80e
	void SetScreen(int theScreen);        // 0x41c8f5
	char CanSave();                       // 0x41ef2b
	char SaveGame();                      // 0x41efbb
	uint32_t LoadGame();                  // 0x41f499
	Board* GetBoard();                    // 0x420e97
	static void UpdatePause();            // 0x426dd5
	void DrawBar(uint32_t theValue, uint32_t theMax);  // 0x4265a6

	// What the board calls
	// A score message at (x, y): theScore with word theWord in score font theFont (0 none)
	void ScoreMessage(int16_t x, int16_t y, uint32_t theScore, uint8_t theWord, uint8_t theFont);  // 0x41eb9c
	// Combo stars of kind theKind (1-based) bursting at (x, y)
	void Starburst(int16_t x, int16_t y, char theKind);  // 0x41ec11
	// The score message, combo animation and stars of a combo of theCount blocks in chain theChain
	void Combo(int16_t x, int16_t y, uint8_t theCount, uint8_t theChain);  // 0x41ecf4
	void SetPosition(int x, int y);       // 0x41ee5a (relative to the block grid)
	void SetCrossed();                    // 0x41ee83
	void Nothing();                       // 0x41ee96
	// The particles of theList's effects at theWhere, after theDelay ms
	void Effects(const GuiRect* theWhere, int theDelay, const EffectList* theList, const Effect* theEffects);  // 0x41eea1
};

extern App gApp;  // 0x488710

// The motion of a falling or floating thing of kind theKind (0x426d39): 10 rocket, 11 explosion (theFlag: of a
// faller), 12 hammer, 13 magnet, 14 rainbow, 15 color bomb, 16 column nuke, else a floater or (theFlag) the primary
// faller.
Motion* MotionOf(int theKind, char theFlag);  // 0x426d39

} // namespace tiptop

#endif
