// The main menu's panels: the screens the menu pans to (MenuPanel, vtable 0x7d9d1c, a TwistWidget with only its own
// destructor 0x69a667), the challenge menu and the end-of-game stats.
//
// port: these are not ported yet. Each class has the fields and methods the menu uses, with the original's offsets
// and addresses; a method not yet ported does nothing (and says so once in the log). They come in their own stages.
#pragma once

#include "TwistWidget.h"

namespace Sexy {

class Menu;
class WinTwistApp;

class MenuPanel : public TwistWidget {
public:
	~MenuPanel() override;  // 0x69a667
};

// Sexy::HighScoresPanel (0x350 bytes; 0x694076-0x696a02, menu +0x2dc): below the gates
class HighScoresPanel : public MenuPanel {
public:
	int mUnknown140 = 0;     // +0x140
	bool mUnknown2C8 = false;  // +0x2c8
	int mCategory = -1;      // +0x344: the list shown (the gates' buttons pick it)

	explicit HighScoresPanel(Menu* theMenu);  // 0x697d5b
	void SetCategory(int theCategory, bool theNow);  // 0x69599c
	void Unknown694BD3();    // 0x694bd3
	void Unknown695966();    // 0x695966
};

// Sexy::HelpPanel (400 bytes; 0x6981b0-0x6984b0, menu +0x2e0): below and to the right
class HelpPanel : public MenuPanel {
public:
	bool mUnknown17C = false;  // +0x17c
	int mPage = 0;             // +0x180: the help page

	explicit HelpPanel(WinTwistApp* theApp);  // 0x699077
	void SetPage(int thePage);  // 0x698319
	void NextPage();            // 0x6983aa
	void PrevPage();            // 0x6983bc
};

// Sexy::ProfilePanel (0x1f0 bytes; 0x6a041f-0x6a0e33, menu +0x2e8): the player list, above the gates
class ProfilePanel : public MenuPanel {
public:
	Image* mDialogImage;       // +0x15c: IMAGE_MAINMENU_PROFILEDIALOG

	explicit ProfilePanel(Menu* theMenu);  // 0x6a077b
	void Show();                // 0x6a0f55
	void Hide();                // 0x6a0713
};

// Sexy::OptionsPanel (0x1a0 bytes; 0x69e0f1-0x69fc10, menu +0x2ec)
class OptionsPanel : public MenuPanel {
public:
	explicit OptionsPanel(WinTwistApp* theApp);  // 0x69e4c5
	void Unknown6A006A(bool);  // 0x6a006a
};

// Sexy::ChallengeMenu (0x9e8 bytes; 0x6dd2cd-0x6e665e, menu +0x4a8)
class ChallengeMenu : public TwistWidget {
public:
	ChallengeMenu();          // 0x6e5c3e
	void Show(int theMode, bool theNow);  // 0x6e30d5
};

// Sexy::EndStats (0x4c8 bytes; 0x6fa5be-0x6fc0eb, menu +0x3dc): the stats after a game
class EndStats : public TwistWidget {
public:
	explicit EndStats(WinTwistApp* theApp);  // 0x6fa6e5
	virtual int GetPreferredWidth();   // slot 70 (+0x118)
	virtual int GetPreferredHeight();  // slot 71 (+0x11c)
};

}  // namespace Sexy
