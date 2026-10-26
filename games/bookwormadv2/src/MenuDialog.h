// Sexy::MenuDialog (0x17c bytes, constructor 0x47fd00, vtables 0x7af25c and 0x7af3b0 (its ButtonListener part at
// +0x88); code 0x47fd00-0x4805c8): the menu that the battle's, the chapter map's and the treasure screen's "Menu"
// buttons open. A GameDialog (dialog id 0x11, GameApp::DIALOG_MENU) under the menu's banner with four FadeButtons of
// its own stacked down it: Options, the way out ("Main Menu"; "Quit To Main Menu" over a battle, "Quit Mini-game"
// over a mini-game of the main menu's list), "Abandon Game" (only over an adventure's mini-game) and the way back
// ("Return To Game" over a battle, else "Close"). Escape closes it like the way back. Its opener is
// GameApp::ShowGameMenu (0x4456a0).
#pragma once

#include "GameDialog.h"

namespace Sexy {

class FadeButton;

class MenuDialog : public GameDialog {
public:
	// the buttons' ids, top to bottom (the index in mButtons)
	enum { BUTTON_OPTIONS, BUTTON_MAIN_MENU, BUTTON_ABANDON, BUTTON_RETURN, NUM_BUTTONS };
	// what the app tells the menu when the "really quit?" question is answered yes (GameApp::ButtonDepress)
	enum { ANSWER_QUIT_YES = 1018 };

	FadeButton* mButtons[NUM_BUTTONS];  // +0x168 (no "Abandon Game" over a battle that is not an adventure's mini-game)
	bool mHasBoard;                     // +0x178: over a battle (or a mini-game)
	bool mFromTreasure;                 // +0x179: the treasure screen's menu

	// theMiniGameFromMenu: the board is a mini-game that the main menu's list started
	MenuDialog(bool theHasBoard, bool theFromTreasure, bool theMiniGameFromMenu);  // 0x47fd00
	~MenuDialog() override;                                                        // 0x480180

	// ButtonListener (0x480210, 0x480230): the click's sound; what each button does
	void ButtonPress(int theId) override;
	void ButtonDepress(int theId) override;
};

}  // namespace Sexy
