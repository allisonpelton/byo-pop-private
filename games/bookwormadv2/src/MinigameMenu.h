// Sexy::MinigameMenu (0xfc bytes, constructor 0x470120, vtable 0x6fd91c; its ButtonListener part at +0x88, PopAnimListener
// part at +0x8c and CheckboxListener part at +0x90; code 0x470120-0x472530): the menu where a mini-game is picked. A
// box (laid out by data/layout_minihut.xml) holds a title and, under it, a button for each mini-game the player has
// unlocked (the scripts register them: main.GetNumMiniGames; main.GetMiniGameClass gives a mini-game's class and
// main.GetMiniGameImageStr its button's picture), with a main menu button below. Lex and Moxie are animations
// (images/pam/Lex_normal.txt, moxie.pam) beside the box that go back to idling when they have played a label.
//
// Which mini-games are in the box depends on the adventure: all of them once it is complete, else by the book the
// player is in (WordUp, and any other, from the 4th, GoldenCoins from the 5th, MutaWords from the 6th), and the one
// that has just been unlocked is marked "new" (DrawOverlay). Opened from the main menu (0x443c20) the menu also has a
// "Show helpful tooltips" checkbox (the profile's ShowMGTips); opened from the chapter screen (the scripts'
// DoCSToMiniMenuTransition, 0x443cc0) it has neither that nor the main menu button.
//
// Made by GameApp (ShowMiniGameMenu, 0x4458b0; the app keeps it at +0x870) once the curtain has closed. A button closes
// the whole curtain again, and then either the scripts' main.StartMinigameFromMainMenu or main.StartMinigameFromCS
// starts the mini-game picked (0x472330) or the main menu comes back (0x4724d0).
#pragma once

#include <string>
#include <vector>

#include "ButtonListener.h"
#include "CheckboxListener.h"
#include "PopAnim.h"
#include "Widget.h"

namespace Sexy {

class Checkbox;
class FadeButton;
class GameApp;
class Graphics;
class XLayout;

class MinigameMenu : public Widget, public ButtonListener, public PopAnimListener, public CheckboxListener {
public:
	// the main menu button's and the checkbox's ids (a mini-game's button has its place in the list)
	enum { BUTTON_MAIN_MENU = 123, CHECKBOX_TIPS = 124 };

	// A mini-game of the list (0x38 bytes): its class in the scripts and the id of its button's picture
	struct MiniGame {
		std::string mClass;     // +0x00
		std::string mImageStr;  // +0x1c
	};

	XLayout* mLayout;                      // +0x94
	GameApp* mApp;                         // +0x98
	std::vector<FadeButton*> mButtons;     // +0xa0: a button for each mini-game the player has (its id is its place in mMiniGames)
	std::vector<MiniGame> mMiniGames;      // +0xb0
	std::vector<std::string> mResGroups;   // +0xbc: the resource groups the scripts say the mini-games' buttons are in
	PopAnim* mLex;                         // +0xcc, id -2
	PopAnim* mMoxie;                       // +0xd0, id -3
	FadeButton* mMainMenuButton;           // +0xd4
	std::string mClass;                    // +0xd8: the mini-game that was picked
	bool mFromMainMenu;                    // +0xf4: opened from the main menu (else from the chapter screen)
	Checkbox* mTipsBox;                    // +0xf8: "Show helpful tooltips"

	explicit MinigameMenu(GameApp* theApp);
	~MinigameMenu() override;  // 0x4715f0 (0x4715d0)

	// The curtain's callbacks (+0x8d4 of the app, with the menu as +0x8d0): once the whole curtain has closed, the
	// main menu button's (0x4724d0) brings the main menu back, and a mini-game button's (0x472330) starts the mini-game
	static void ReturnToMainMenu(void* theMenu);
	static void StartPickedMiniGame(void* theMenu);

	// Widget
	void Update() override;                  // 0x471850
	void Draw(Graphics* g) override;         // 0x4718a0
	void DrawOverlay(Graphics* g) override;  // 0x471a70

	// ButtonListener
	void ButtonPress(int theId) override;    // 0x4721d0: the click
	void ButtonDepress(int theId) override;  // 0x471f70

	// CheckboxListener (0x4721f0): the tooltips box is kept in the profile
	void CheckboxChecked(int theId, bool checked) override;

	// PopAnimListener: Lex or Moxie has played their label, and go back to idling (0x472250)
	void PopAnimStopped(int theId) override;
};

}  // namespace Sexy
