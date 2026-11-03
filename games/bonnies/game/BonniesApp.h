#ifndef __BONNIES_BONNIESAPP_H__
#define __BONNIES_BONNIESAPP_H__

// The game's application (0x401000-0x404140, the vtable 0x4572f0, 0x2fc bytes, the object 0x4752a0): the states the
// game goes through (each one screen), the fonts and images every screen shares, the partial redraw of the board and
// the program's main (0x403b80).

#include "App.h"

namespace bonnies
{

struct Image;

class BonniesApp : public App
{
public:
	// 0x401160
	BonniesApp();
	// 0x4015c0
	~BonniesApp() override;

	// slot 0, 0x403440: the player's name, the engine, the overlay screen and the first state (the logo)
	void init(int theWidth, int theHeight, int theFullScreen) override;
	// slot 1, 0x403980
	void shutdown() override;
	// slot 2, 0x4016d0: the whole screen, or only the board's rectangle while nothing else changes
	void draw() override;
	// slot 3, 0x403da0: pauses while the window is inactive, and changes the state when one was asked for
	void update() override;
	// slot 4, 0x403e90: the app's keys (Alt+Enter full screen, F1 help, Alt+F4 quit), then the screens
	int handleEvent(Event* theEvent) override;
	// slot 9, 0x402250: the game's own dialog (0x41bf10)
	int messageBox(int theFlags, const Str& theText1, const Str& theText2 = Str(), const Str& theText3 = Str(),
		const Str& theText4 = Str(), const Str& theText5 = Str()) override;
	// slot 10, 0x402230
	int isPaused() override;
	// slot 11, 0x402c00: the game is being played
	bool isRunning() override;
	// slots 12, 13, 0x402360, 0x402380: the options' volumes
	float getSoundVolume() override;
	float getMusicVolume() override;
	// slot 14, 0x402b30: the window moved, kept in the options
	void windowMoved(int theX, int theY, int theClientX, int theClientY) override;
	// slots 15, 16, 0x402b80, 0x402bc0
	void getWindowPos(int* theX, int* theY) override;
	void getWindowPos2(int* theX, int* theY) override;
	// slot 17, 0x401d00: paused and inactive for more than a frame
	int isIdle() override;
	// slot 18, 0x401e00: the debug menu's commands
	void menuCommand(int theCommand) override;

	// 0x401630: the shared images (the table 0x4662c0), once
	void loadImages();
	// 0x401d40: the debug menu's check marks
	void updateMenu();
	// 0x402510: Bonnie's speech balloon with theText, pointing from (theX, theY) to one side (theSide 0-6); its box
	// in theRect when given, drawn when theDraw
	void balloon(const Str& theText, float theX, float theY, int theSide, int theTall, float* theRect, int theDraw);
	// 0x402c20: the sounds, images and fonts, the game, the dictionary and the books
	void loadResources();
	// 0x402fc0: shows theState's screen and removes the last one (or only asks for it, when theLater)
	void setState(int theState, int theLater, int theForce);

	int mInited;              // +0x1d8
	Screen* mScreen;          // +0x1dc: the state's screen
	Str mDebugText;           // +0x1e0
	int mDebug1E8;            // +0x1e8 (the menu's 0x2af9)
	int mDebug1EC;            // +0x1ec (0x2afa)
	int mPaused;              // +0x1f0
	int mAutoPaused;          // +0x1f4: paused because the window was inactive
	int mPauseInactive;       // +0x1f8 (1): pause while the window is inactive
	int mDebug1FC;            // +0x1fc (0x2afd)
	int mState;               // +0x200
	int mNextState;           // +0x204 (-1)
	int mPrevState;           // +0x208 (-1)
	int mLastGameState;       // +0x20c (2): the last of the menu and the game
	int mSpecialMode;         // +0x210: the debug mode that turns tiles into a kind (its character)
	int mRedrawUntil;         // +0x214: the whole screen is redrawn until then
	int mField218;            // +0x218
	Font* mFonts[12];         // +0x21c
	Image* mImages[36];       // +0x24c
	int mLoadImages[8];       // +0x2dc: which kinds of images loadImages loads (all set by update)
};

// 0x403b80: the program
int bonniesMain();
// 0x402ee0: at exit: saves the game in progress and frees the app
void bonniesExit();

} // namespace bonnies

#endif
