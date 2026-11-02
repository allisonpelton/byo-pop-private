// Sexy::ChallengeMenu (0x9e8 bytes; 0x6dd2cd-0x6e665e, menu +0x4a8) and Sexy::ChallengeButton (0x658 bytes; vtable
// 0x7e011c): the planets of the challenge categories, picked from the main menu's play menu.
//
// port: the original draws its planets in a small 3D world (a camera the mouse drags, planets as spheres with two
// cloud layers, the sun, a moon meter by each planet). The port keeps the menu's logic (which categories are open, how
// far the player has got in each, the picks, the saved-game question, the start of the challenge through
// Board::SetChallenge and the hyperspace) and lays the planets out flat on the screen; the 3D camera, the
// projection that places each planet (0x6dde98, 0x6e3f41), the planet pictures (0x6e4fad's meshes, the software
// planet), the sun (0x6e132d), the moon meter and the eclipse are left out and are listed in NOTES.md.
#pragma once

#include "ButtonListener.h"
#include "ButtonWidget.h"
#include "TwistDialogListener.h"
#include "TwistWidget.h"

#include <vector>

namespace Sexy {

class Menu;
class TwistButton;
class WinTwistApp;

class ChallengeMenu;

// One category's planet; its id is the category's number
class ChallengeButton : public ButtonWidget {
public:
	ChallengeMenu* mMenu = nullptr;
	int mCategory = 0;        // +0x518
	int mStage = 0;           // +0x164: the challenge the planet starts (from 1)
	bool mOpen = false;       // +0x51c: the category is open to the player
	bool mEclipse = false;    // +0x5c6: every challenge of the category is done; its night challenge is next
	int mProgress = 0;        // +0x5c8: the category's best (Profile +0x204)

	ChallengeButton(int theCategory, ChallengeMenu* theMenu);  // 0x6e5337
	void Draw(Graphics* g) override;                           // 0x6dde98 (the 2D planet only as a disc)
};

class ChallengeMenu : public TwistWidget, public ButtonListener, public TwistDialogListener {
public:
	// The ids of the two buttons below the planets (a planet's is its category's number)
	enum { BUTTON_PLAY_MENU = 100, BUTTON_MAIN_MENU = 101 };

	std::vector<ChallengeButton*> mButtons;   // +0x164: one for each category
	int mHover = -1;                          // +0x160: the planet the mouse is over
	TwistButton* mPlayMenuButton = nullptr;   // +0x174
	TwistButton* mMainMenuButton = nullptr;   // +0x178
	int mCategory = 0;                        // +0x17c: the planet picked
	int mStageSel = 0;                        // +0x180: and its challenge
	int mStarted = 0;                         // +0x184: set when a challenge starts
	float mHoverFade = 0;                     // +0x198: the hovered planet's text
	int mState = 0;                           // +0x1c4: 0 away, 1 shown by the play menu, 2 leaving for a menu, 3 leaving
	                                          // for a challenge, 4 shown at once (after a game)
	float mAmount = 0;                        // port: how far the menu is in (the original's curves at +0x1c8)
	bool mUnknown9BC = false;                 // +0x9bc

	ChallengeMenu();                          // 0x6e5c3e
	~ChallengeMenu() override;                // 0x6e3f22

	void AddedToManager(WidgetManager* theWidgetManager) override;      // 0x6dfc49
	void RemovedFromManager(WidgetManager* theWidgetManager) override;  // 0x6dd2cd
	void Update() override;                                             // 0x6e3f41
	void Draw(Graphics* g) override;                                    // 0x6dd322
	void MouseWheel(int theDelta) override;                             // 0x6dfe2a (the camera's zoom: left out)

	void DialogButtonDepress(int theDialogId, int theButtonId) override;  // 0x6e58f7
	void ButtonDepress(int theId) override;                               // 0x6e54c3
	void ButtonMouseEnter(int theId) override;                            // 0x6e665e
	void ButtonMouseLeave(int theId) override;                            // 0x6dd418

	void Show(int theMode, bool theNow);       // 0x6e30d5
	void Close(int theState);                  // 0x6e3142
	void SetState(int theState, bool theNow);  // 0x6e2318
	// 0x6e0ddb: each planet's open/closed state and stage from the player's progress
	int UpdateProgress(bool theAnimate, bool theClearTips);
	void Start(bool theNewGame);               // 0x6e4ab6: the picked challenge
	void Layout();                             // port: the flat layout of the planets
};

}  // namespace Sexy
