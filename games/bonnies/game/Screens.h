#ifndef __BONNIES_SCREENS_H__
#define __BONNIES_SCREENS_H__

// The screens of the app's states other than the game's (0x402fc0 makes them).

#include "Screen.h"
#include "Str.h"

namespace bonnies
{

struct Event;
struct Font;
struct Image;

// State 1 (the vtable 0x457540, 0x130 bytes)
// State 1 (the vtable 0x457540, the object 0x4791b0): the logos
class LogoScreen : public Screen
{
public:
	void init() override;
	void shutdown() override;
	void draw() override;
	void update() override;
	int handleEvent(Event* theEvent) override;

	Image* mPopCap = nullptr;       // +0x48
	Image* mDistributor = nullptr;  // +0x4c
	Image* mGames = nullptr;        // +0x50
	Image* mCrayon = nullptr;       // +0x54
	Image* mTM = nullptr;           // +0x58
	Image* mLetters[9] = {};        // +0x5c
	uint32_t mFallStart = 0;        // +0x80: the letters began to fall
	uint32_t mLastUpdate = 0;       // +0x84
	int mShown = 0;                 // +0x88: the game's logo is whole
	int mDone = 0;                  // +0x8c
	uint32_t mHoldUntil = 0;        // +0x90: the logo shown stays until then
	uint32_t mFadeEnd = 0;          // +0x94: the fade into the next logo ends then
	int mShowPopCap = 0;            // +0x98: the next is PopCap's logo
	int mShowDistributor = 0;       // +0x9c
	int mShowWhite = 0;             // +0xa0: the next is the end
	int mShowLetters = 0;           // +0xa4: the game's logo is shown
	int mPopCapShown = 0;           // +0xa8
	int mDistributorShown = 0;      // +0xac
	int mEarly = 0;                 // +0xb0: the distributor's logo first
	int mCursorHidden = 0;          // +0xb4
	float mOffsets[9] = {};         // +0xb8: the letters' heights above their places
	float mSpeeds[9] = {};          // +0xdc
	int mBounces[9] = {};           // +0x100
	int mLanded = 0;                // +0x124
	uint32_t mLandTime = 0;         // +0x128
	int mField12C = 0;              // +0x12c
};

// State 2 (the vtable 0x457524, 0x8c bytes)
class MenuScreen : public Screen
{
public:
	// 0x41af20
	void init() override;
	// 0x419f70
	void shutdown() override;
	// 0x41a040
	void draw() override;
	// 0x41b580
	void update() override;
	// 0x41a810
	int handleEvent(Event* theEvent) override;

	// 0x419d40: the player's name and the buttons that need a player
	void refresh();
	// 0x41a690: the difficulty, in a dialog that does not block (update reads its answer)
	void chooseDifficulty();

	Image* mSetup;            // +0x48: the bookstore
	Image* mBlink;            // +0x4c: Bonnie blinking
	Image* mWave;             // +0x50
	Image* mSign;             // +0x54
	Image* mLeaf;             // +0x58
	Image* mBooks[5];         // +0x5c: each button's book lit
	Font* mFont70;            // +0x70: the version
	Font* mFont74;            // +0x74: the greeting
	Font* mFont78;            // +0x78: the player's name
	int mField7C;             // +0x7c: asks for the difficulty at the next update (nothing sets it)
	int mWaiting;             // +0x80: the difficulty's dialog is up
	int mStory;               // +0x84: the first button (1) or the second
	int mStartLevel;          // +0x88
};

// State 4 (the vtable 0x457578, the object 0x478980): the help's three pages
class HelpScreen : public Screen
{
public:
	void init() override;
	void shutdown() override;
	void draw() override;
	void update() override;
	int handleEvent(Event* theEvent) override;

	// 0x415090: the page and its arrows, once the help's images are loaded
	void load();
	// 0x4156b0
	void turn(int theStep);

	Image* mImage = nullptr;  // +0x48
	int mPage = 0;            // +0x4c
	int mLoaded = 0;          // +0x50
};

// State 5 (the vtable 0x45755c, 0x60 bytes, the object 0x47ae70): Bonnie's story, before the first level and after
// the last
class StoryScreen : public Screen
{
public:
	void init() override;
	void shutdown() override;
	void draw() override;
	int handleEvent(Event* theEvent) override;

	// 0x41ca40
	void turn();

	Image* mImage = nullptr;  // +0x48: the page
	Image* mImage2 = nullptr; // +0x4c: never loaded
	int mStory = 0;           // +0x50: 1 the beginning, 2 the end
	int mPage = 0;            // +0x54
	int mPages = 0;           // +0x58
	Font* mFont = nullptr;    // +0x5c
};

// State 6 (the vtable 0x4575b0, the object 0x4780e8): the credits' pages
class CreditsScreen : public Screen
{
public:
	void init() override;
	void shutdown() override;
	void draw() override;
	void update() override;
	int handleEvent(Event* theEvent) override;

	// 0x40e5d0: the fonts and the picture, once the menu's images are loaded
	void load();

	Image* mImage = nullptr;  // +0x48
	Font* mFont1 = nullptr;   // +0x50
	Font* mFont2 = nullptr;   // +0x54
	Font* mFont3 = nullptr;   // +0x58
	int mLoaded = 0;          // +0x5c
};

extern int gCreditsReturn;    // 0x473e28

// State 7 (the vtable 0x457594, 0x60 bytes)
// State 7 (the vtable 0x457594, the object 0x478d98): the high scores
class HighScoresScreen : public Screen
{
public:
	void init() override;
	void shutdown() override;
	void draw() override;
	void update() override;
	int handleEvent(Event* theEvent) override;

	// 0x416310: the other table
	void turn(int theStep);

	Image* mImage = nullptr;  // +0x48
	uint32_t mStart = 0;      // +0x4c
	Font* mFont1 = nullptr;   // +0x50
	Font* mFont2 = nullptr;   // +0x54
	Font* mFont3 = nullptr;   // +0x58
	int mTable = 0;           // +0x5c
};

extern int gHighScoresToGame; // 0x478d9c

// Always there (the vtable 0x4574bc, 0x48 bytes, the object 0x47b698), above every screen
class OverlayScreen : public Screen
{
public:
	void init() override;
	void shutdown() override;
	void draw() override;
};

// A book's end (the vtable 0x458a4c, 0x64 bytes)
class BookEndScreen : public Screen
{
public:
	void init() override;
	void shutdown() override;
	void draw() override;
	void update() override;
	int handleEvent(Event* theEvent) override;

	Image* mBackground = nullptr;  // +0x48
	Image* mSummary = nullptr;     // +0x4c: the book's
	uint32_t mStart = 0;           // +0x50
	uint32_t mEnd = 0;             // +0x54: the click
	int mShown = 0;                // +0x58: faded in
	int mAdvanced = 0;             // +0x5c: the game went on to the next level
	Font* mFont = nullptr;         // +0x60
};

// 0x41d110: a book's end
void showBookEnd();

// The modal panels on the "Options" image (the vtable 0x4586ec and those like it)
class OptionsPanel : public Screen
{
public:
	void init() override;
	void shutdown() override;
	void draw() override;

	Image* mImage = nullptr;  // +0x48
};

// 0x4175f0's (the vtable 0x4586ec): a new player's name
class NamePanel : public OptionsPanel
{
public:
	int handleEvent(Event* theEvent) override;
};

// 0x4175f0: asks for a new player's name (theCancel: with a cancel button); whether one was given
int askName(int theCancel);

// 0x418e10's (the vtable 0x4587bc, 0x54 bytes): the players
class PlayersPanel : public OptionsPanel
{
public:
	int handleEvent(Event* theEvent) override;

	Str selected();
	void layout();
	void refill();

	Str mOriginal;            // +0x4c: the player when it opened
};

// 0x418e10: the players (the menu's button)
void choosePlayer();

// The interstitial (the vtable 0x458ad4, 0x50 bytes)
class InterstitialScreen : public OptionsPanel
{
public:
	void init() override;
	void draw() override;
	int handleEvent(Event* theEvent) override;

	int mField4C = 0;         // +0x4c
};

// 0x41bf10's (the vtable 0x458994, 0x58 bytes): the game's dialog (Dialog.h)
class GameDialog : public Screen
{
public:
	// 0x41bc30
	void init() override;
	// 0x41b840
	void shutdown() override;
	// 0x41b8a0
	void draw() override;
	// 0x41ba20
	int handleEvent(Event* theEvent) override;

	// 0x41b6f0: the chosen level's book and chapter
	void showLevel();

	Image* mThumbs = nullptr;       // +0x48: the levels' pictures (the chooser's)
	Image* mThumbsFrame = nullptr;  // +0x4c
	Image* mNotepad = nullptr;      // +0x50
	Image* mBonnie = nullptr;       // +0x54
};

// The options (the vtable 0x458788, 0x58 bytes)
class OptionsMenu : public Screen
{
public:
	void init() override;
	void shutdown() override;
	void draw() override;
	void update() override;
	int handleEvent(Event* theEvent) override;

	// 0x417b40
	void load();

	int mLoaded = 0;          // +0x48
	int mField4C = 0;         // +0x4c
	int mField50 = 0;         // +0x50
	Image* mImage = nullptr;  // +0x54
};

// 0x418710: the game's menu (Esc or its button)
void showGameMenu();
// 0x41d600: the interstitial, when the portal has one (theWin: after a won game; theLoss: after a lost one)
void showInterstitial(int theWin, int theLoss);

} // namespace bonnies

#endif
