// Sexy::MinigameMenu (0xf0 bytes, constructor 0x4805e0, vtable 0x7af4b4; its ButtonListener part at +0x88 and
// PopAnimListener part at +0x8c; code 0x4805d0-0x481f60): the menu where a mini-game is picked. A box (laid out by
// data/layout_minihut.xml) holds a title, a welcome and a line of instructions, and under them a button for each
// mini-game the scripts have registered (main.GetNumMiniGames; main.GetMiniGameClass gives its class and
// main.GetMiniGameImageStr its button's picture), with a main menu button below. Lex and Moxie are animations
// (images/pam/lex.pam, moxie.pam) beside the box that go back to idling when they have played a label.
//
// Made by GameApp (ShowMiniGameMenu, 0x44f9d0; the app keeps it at +0x834) once the curtain has closed over the
// main menu's mini-games item, or over the hall of fame after a mini-game. A button closes the whole curtain, and
// then either the scripts' main.StartMinigameFromMainMenu starts the mini-game picked (0x481cd0) or the main menu
// comes back (0x481e00).
#pragma once

#include <string>
#include <vector>

#include "ButtonListener.h"
#include "PopAnim.h"
#include "Widget.h"

namespace Sexy {

class FadeButton;
class GameApp;
class Graphics;
class XLayout;

class MinigameMenu : public Widget, public ButtonListener, public PopAnimListener {
public:
	// the main menu button's id (a mini-game's button has its number in the list)
	enum { BUTTON_MAIN_MENU = 123 };

	// A mini-game of the list (0x38 bytes): its class in the scripts and the id of its button's picture
	struct MiniGame {
		std::string mClass;     // +0x00
		std::string mImageStr;  // +0x1c
	};

	XLayout* mLayout;                      // +0x90
	GameApp* mApp;                         // +0x94
	std::vector<FadeButton*> mButtons;     // +0x98: a button for each mini-game (its id is its place in mMiniGames)
	std::vector<MiniGame> mMiniGames;      // +0xa8
	std::vector<std::string> mResGroups;   // +0xb8: the resource groups the scripts say the mini-games' buttons are in
	PopAnim* mLex;                         // +0xc8, id -2
	PopAnim* mMoxie;                       // +0xcc, id -3
	FadeButton* mMainMenuButton;           // +0xd0
	std::string mClass;                    // +0xd4: the mini-game that was picked

	explicit MinigameMenu(GameApp* theApp);
	~MinigameMenu() override;  // 0x481670 (0x481650)

	// The curtain's callbacks (+0x898 of the app, with the menu as +0x894): once the whole curtain has closed, the
	// main menu button's (0x481e00) brings the main menu back, and a mini-game button's (0x481cd0) starts the
	// mini-game
	static void ReturnToMainMenu(void* theMenu);
	static void StartPickedMiniGame(void* theMenu);

	// Widget
	void Update() override;           // 0x4818c0
	void Draw(Graphics* g) override;  // 0x481900

	// ButtonListener
	void ButtonPress(int theId) override;    // 0x481be0: the click
	void ButtonDepress(int theId) override;  // 0x4819a0

	// PopAnimListener: Lex or Moxie has played their label, and go back to idling (0x481c00)
	void PopAnimStopped(int theId) override;
	void PopAnimCommand(int theId, const std::string& theCommand, const std::string& theParam) override;  // 0x4805d0: nothing
};

}  // namespace Sexy
