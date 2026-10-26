#ifndef __NINGPO_NINGPO_H__
#define __NINGPO_NINGPO_H__

// NingPo (0xb9c4 bytes, vtable 0x4622a8; the one object is the global at 0x475550): Ning Po Mahjong Deluxe, a
// subclass of the framework's PGApp. The game is a single screen: the title (a layout, a tile set and a background
// chosen with arrows), the board, and the two end screens (won, no more moves), with the controls down the left
// side and a panel of numbers at the bottom; everything else is a dialog (the options, the best times, the name
// entry, the rules).
//
// The functions are in src/NingPo.cpp (start-up, events, the flow of a game), src/NingPoDraw.cpp (the screen),
// src/NingPoControls.cpp (the controls) and src/NingPoDialogs.cpp (the dialogs); the board is src/Board.cpp.
//
// The port plays as the registered game (checkRegistration is always true): the trial's parts (the registration
// dialog, the "full version only" layouts and tile sets, the nag at exit, the count of games played) are left out.

#include "Board.h"
#include "PGApp.h"
#include "PGSprite.h"

namespace pg
{
class PGControl;
}

namespace ningpo
{

using pg::PGControl;
using pg::PGEvent;
using pg::PGPoint;
using pg::PGRect;
using pg::PGSprite;
using pg::PGSpriteEntry;
using pg::PyTime;

// The game's states (+0x81c0)
enum
{
	STATE_TITLE = 0,
	STATE_PLAYING = 1,
	STATE_WON = 2,
	STATE_GAME_OVER = 3,     // no more moves, and the player ended the game
	STATE_EXIT = 4,
};

// The sprites by index (the table at +0xa914, SpriteTable.inc); the runs are indexed by the setting
enum
{
	SPRITE_FONT = 0,
	SPRITE_PAUSED = 1,
	SPRITE_LEFT = 2,               // the controls' column
	SPRITE_START = 3,              // the title screen
	SPRITE_ARROW_LEFT = 4,
	SPRITE_ARROW_RIGHT = 5,
	SPRITE_ARROW_LEFT_LIT = 6,
	SPRITE_ARROW_RIGHT_LIT = 7,
	SPRITE_BACKGROUND = 8,         // the arena's background, loaded for the chosen one (0x404ad0)
	SPRITE_SHADOW_TOP = 9,
	SPRITE_SHADOW_RIGHT = 10,
	SPRITE_NOMATCH = 11,
	SPRITE_BOARD_COMPLETE = 12,
	SPRITE_GAME_OVER = 13,
	SPRITE_FULL_VERSION = 14,
	SPRITE_FULL_VERSION_SMALL = 15,
	SPRITE_PREVIEW_LAYOUT = 16,    // + layout
	SPRITE_TITLE_LAYOUT = 34,      // + layout
	SPRITE_PREVIEW_TILE = 52,      // + tile set
	SPRITE_PREVIEW_BACKGROUND = 61,  // + background
	SPRITE_LABEL_TIME = 100,
	SPRITE_LABEL_TILES_LEFT = 101,
	SPRITE_LABEL_MOVES = 102,
	SPRITE_DIGIT_0 = 103,          // .. 112, then the colon and the comma
	SPRITE_COLON = 113,
	SPRITE_COMMA = 114,
	SPRITE_OUTLINE_FREE = 115,
	SPRITE_OUTLINE_BLOCKED = 116,
	SPRITE_TILE_SHEET = 117,       // the tile set's sheet, loaded for the chosen one (0x404a60)
	NUM_SPRITES = 118,
};

enum
{
	NUM_LAYOUTS = 18,
	NUM_TILESETS = 9,
	NUM_BACKGROUNDS = 10,
	NUM_BEST_TIMES = 8,            // a layout's
};

// A best time (0x30 bytes, stored as the registry value "HS:<layout>x<rank>")
struct BestTime
{
	int mVersion;          // +0x00 0x300
	char mName[32];        // +0x04
	int mTime;             // +0x24 seconds with the penalties
	int mRawTime;          // +0x28 seconds played
	int mTilesLeft;        // +0x2c
};

class NingPo : public pg::PGApp
{
public:
	pg::HFont* mFont;                 // +0x0008
	PGControl* mShowMovesButton;      // +0x6aa0 lights the free tiles (0x405d20)
	PGControl* mUndoButton;           // +0x6aa4
	PGControl* mPauseButton;          // +0x6aa8
	PGControl* mShuffleButton;        // +0x6aac
	PGControl* mHintButton;           // +0x6ab0 "find match" while a tile is selected
	PGControl* mNewGameButton;        // +0x6ab4 back to the title
	PGControl* mBuyButton;            // +0x6ab8 the trial's
	Tile* mHintTiles[4];              // +0x6abc the hint's: the selected tile and its free matches, or a pair
	PyTime mHintStart;                // +0x6ad0 the hint shows for 0xd7b ms
	PyTime mLastAction;               // +0x6ad8 the autohint's clock
	bool mSpaceHeld;                  // +0x6ae0
	PyTime mFadeStart;                // +0x6ae8 when show moves lit the free tiles (0x404250)
	float mFade;                      // +0x6af0
	bool mRegistered;                 // +0x6af4
	int mTimesPlayed;                 // +0x6af8 the trial's
	PyTime mGameStart;                // +0x6b00
	PyTime mNoMatchTime;              // +0x6b08 the "no match" sign shows for 3000 ms
	int mLoadedTileset;               // +0x6b10 -1 before the first
	int mLoadedBackground;            // +0x6b14
	int mPenalty;                     // +0x6b1c seconds: a hint 30, an undo 60, a shuffle 120
	bool mCheat;                      // +0x6b21 Backspace, q, Backspace
	Board mBoard;                     // +0x6b28
	int mState;                       // +0x81c0
	int mHints;                       // +0x81c4
	int mUndos;                       // +0x81c8
	int mShuffles;                    // +0x81cc
	int mFinalTime;                   // +0x81d0 the game's time with the penalties, at its end
	int mFinalRawTime;                // +0x81d4
	PGRect mBoardRect;                // +0x81e0
	PGRect mNoMatchRect;              // +0x81f0
	PGRect mMovesRect;                // +0x8200
	PGRect mTilesLeftRect;            // +0x8210
	PGRect mClockRect;                // +0x8220
	PGRect mPreviewRect;              // +0x8230 the selected tile's preview, under the hint button
	float mSoundSetting;              // +0x8240 the options' settings, saved in the registry (0x407dd0)
	float mMusicSetting;              // +0x8244 (PGApp's mMusicVolume is the playing music's)
	int mAutohint;                    // +0x8248
	int mTileset;                     // +0x824c
	int mBackground;                  // +0x8250
	int mLayout;                      // +0x8254
	int mFullscreen;                  // +0x8258
	PGSprite mSprites[NUM_SPRITES];   // +0x825c
	PGSpriteEntry mSpriteTable[NUM_SPRITES];  // +0xa914
	BestTime mBestTimes[NUM_LAYOUTS][NUM_BEST_TIMES];  // +0xacc4

	NingPo();                         // 0x4010a0
	virtual ~NingPo();                // 0x4012c0

	// PGApp
	bool isRealtime() override { return false; }                                       // 0x405c60 (slot 0x24)
	bool init(const char* theCmdLine, const char* theResourceDir) override;           // 0x401a40 (slot 0x30)
	void shutdown() override;                                                          // 0x401c40 (slot 0x34)
	bool handleEvent(const PGEvent* theEvent) override;                                // 0x403cd0 (slot 0x5c)
	void render(const PGRect* theRect, bool theUnderDialog) override;                  // 0x401c80 (slot 0x60)
	void setActive(bool theActive) override;                                           // 0x405940 (slot 0x74)

	// 0x401330: the sprite table
	void initSpriteTable();
	// 0x405af0: nullptr (and an alert) for a sprite that is not loaded
	PGSprite* getSprite(int theIndex);
	// 0x404010: every sprite released
	void releaseSprites();

	// src/NingPo.cpp: the flow
	void onExit();                                      // 0x401950: the window closed
	void toggleFullscreen();                            // 0x4059e0
	bool checkRegistration();                           // 0x405cd0
	bool loadAssets();                                  // 0x405c70
	bool startup();                                     // 0x405960
	void playSound(const char* theName, int theVolume, int thePan, bool theOverlap);    // 0x405e10
	void openPartnerSite();                             // 0x405b60
	void openPurchasePage();                            // 0x401a00
	void handleKey(const PGEvent* theEvent);            // 0x403e20
	void cycleBackground(int theStep);                  // 0x403f10
	void cycleLayout(int theStep);                      // 0x403f50
	void cycleTileset(int theStep);                     // 0x403f90
	void cheatMatch();                                  // 0x403fd0
	void expireHint();                                  // 0x404030
	void updateNoMatch();                               // 0x4040e0
	void updateRemovedTiles();                          // 0x4041b0
	bool tick();                                        // 0x404250: false quits
	void onRightClick(const PGEvent* theEvent);         // 0x4044f0
	void onLeftClick(const PGEvent* theEvent);          // 0x4045c0
	void onMouseMove(const PGEvent* theEvent);          // 0x4047b0
	void enablePause(bool theEnabled);                  // 0x404820
	void enableNewGame(bool theEnabled);                // 0x404880
	void enableShuffle(bool theEnabled);                // 0x4048e0
	void enableShowMoves(bool theEnabled);              // 0x404940
	void enableHint(bool theEnabled);                   // 0x4049a0
	void enableUndo(bool theEnabled);                   // 0x404a00
	void loadTileset();                                 // 0x404a60
	void loadBackground();                              // 0x404ad0
	void startGame();                                   // 0x404b40
	void toTitle(bool theAsk);                          // 0x404bd0
	bool newGame();                                     // 0x404c90
	void addPenalty(int theSeconds);                    // 0x404db0
	void undo();                                        // 0x404ee0
	void shuffle();                                     // 0x405010
	void deselect();                                    // 0x4050c0
	void select(Tile* theTile);                         // 0x405100
	void showRules();                                   // 0x405170
	void hint(bool theManual);                          // 0x4051f0
	void endGame(bool theWon, int theTime, int theRawTime);  // 0x405410
	void match(Tile* theTile, Tile* theOther);          // 0x4054f0
	void showMoves();                                   // 0x405d20
	void createControls();                              // 0x406cd0 (src/NingPoControls.cpp)
	void loadSettings();                                // 0x407c40
	void saveSettings();                                // 0x407dd0
	void setPaused(bool thePaused);                     // 0x407ea0
	void resetBestTimes(int theLayout);                 // 0x407f10
	void saveBestTimes();                               // 0x407f70
	void loadBestTimes(bool theReset);                  // 0x408010
	void recordTime();                                  // 0x4081d0

	// src/NingPoDraw.cpp: the screen
	void drawNumber(const char* theText, int theX, int theY, bool theCommas, const PGRect* theClip);  // 0x402d30
	void drawPanel(const PGRect* theClip);              // 0x402fd0
	void drawTile(Tile* theTile, const PGRect* theClip, float theShadow, float theAlpha, float theHighlight);  // 0x403410
	void drawTopShadow(Tile* theTile, const PGRect* theClip, float theShadow);    // 0x403690
	void drawRightShadow(Tile* theTile, const PGRect* theClip, float theShadow);  // 0x4039a0
	void drawOutline(Tile* theTile, const PGRect* theClip, bool theFree);  // 0x404de0
	void drawFaded(const PGRect* theClip, const PGRect* theDest, PGSprite* theSprite, float theAlpha);  // 0x405690
	void drawNoMatch(const PGRect* theClip);            // 0x405890

	// src/NingPoDialogs.cpp
	void enterName(char* theName, int theSize);         // 0x408320
	bool askYesNo(const char* theBackground, const char* theOk, const char* theOkLit, const char* theCancel,
		const char* theCancelLit);                      // 0x408990
	void showBestTimes(int theLayout);                  // 0x408da0
	bool startupDialog(int theTimesPlayed);             // 0x409e90: false when the window was closed
	bool noMoreMoves();                                 // 0x40aa60: true ends the game
	void showOptions();                                 // 0x40aff0
};

// 0x475550: the game
NingPo* GetNingPo();

// 0x402ad0: theRect clipped to theClip into theDest, theSrc cut by as much; false when they do not meet. The game
// repeats this inline in most of its drawing (as the framework does, pg::ClipToRect)
bool ClipBlitRect(const PGRect* theRect, const PGRect* theClip, PGRect* theDest, PGRect* theSrc);
// 0x402bf0: *theValue clamped to theLow..theHigh; the change
int ClampDelta(int* theValue, int theLow, int theHigh);

} // namespace ningpo

#endif
