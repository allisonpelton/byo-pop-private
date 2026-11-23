// Sexy::OptionsDialog (bejeweled3, made by 0x681370, vtable 0x86ebcc for its button listener): the menu the game screen's Menu button
// opens (dialog id 0x15). See OptionsDialog.cpp for what is and is not made.
#pragma once

#include "Bej3Dialog.h"

namespace Sexy {

class DialogButton;

class OptionsDialog : public Bej3Dialog {
public:
	enum { BUTTON_BACK = 0, BUTTON_MAIN_MENU = 1, BUTTON_HOW_TO_PLAY = 2 };   // the ids 0x681370 gives its buttons
	enum { DIALOG_ID = 0x15 };

	DialogButton* mBackButton = nullptr;       // id 0
	DialogButton* mMainMenuButton = nullptr;   // id 1
	DialogButton* mHowToPlayButton = nullptr;  // id 2: only while the board has asked for a help (its +0x2b44 is not -1)

	// 0x681370 (bejeweled3): theInGame is the game screen's menu (it has the two buttons); the first screen's has none made yet
	explicit OptionsDialog(bool theInGame);
	~OptionsDialog() override;

	void ButtonDepress(int theId) override;   // 0x697460 (bejeweled3)
};

}  // namespace Sexy
