// The main menu's panels: the screens the menu pans to (MenuPanel, vtable 0x7d9d1c, a TwistWidget with only its own
// destructor 0x69a667), the challenge menu and the end-of-game stats.
//
// port: the high scores, profile and new player panels are ported; the others are not yet. Each of those has the fields
// and methods the menu uses, with the original's offsets and addresses; a method not yet ported does nothing (and says
// so once in the log). They come in their own stages.
#pragma once

#include "ButtonListener.h"
#include "CurvedVal.h"
#include "ListListener.h"
#include "TwistDialogListener.h"
#include "TwistWidget.h"

#include <vector>

namespace Sexy {

class EditListener;
class EffectsManager;
class Image;
class ListWidget;
class Menu;
class TwistButton;
class TwistEditWidget;
class WinTwistApp;

class MenuPanel : public TwistWidget {
public:
	~MenuPanel() override;  // 0x69a667
};

// Sexy::HighScoresPanel (0x350 bytes; 0x694076-0x697d5b, menu +0x2dc): below the gates. It shows one of the two top
// tens (classic, or the other timeless mode: WinTwistApp::ScoreList) or, for any other category, the player's stats
// with a button to view each of the best moves' replays. A new category fades the old one out, then shows its rows
// one by one. After a game the bar with the replay and quit buttons rises under it (ShowBar), and a new high score
// goes into its list (mState 1 to 3).
class HighScoresPanel : public MenuPanel, public ButtonListener {
public:
	// The buttons' ids: the bar's two, then the three view-replay buttons of the stats page
	enum { BUTTON_QUIT = 0, BUTTON_REPLAY = 1, BUTTON_VIEW_REPLAY = 2 };

	// A row of a list (0x30 bytes), copied from the app's high scores
	struct Row {
		bool mUnknown0 = false;   // +0x00
		float mUnknown4 = 1.0f;   // +0x04
		float mUnknown8 = 0;      // +0x08
		SexyString mName;         // +0x0c
		int mLevel = 0;           // +0x28
		int mScore = 0;           // +0x2c
	};

	Menu* mMenu;                       // +0x14c
	Image* mDialogImage;               // +0x150: IMAGE_MAINMENU_SCORESDIALOG
	float mRevealTimer = 0;            // +0x154: the next row shows when it runs out
	bool mMouseDown = false;           // +0x158: between a press and its release (not read)
	int mState = 0;                    // +0x15c: 0 shown; 1 to 3 a new score goes into its list
	float mStateTimer = 0;             // +0x160
	float mUnknown164 = 5.0f;          // +0x164
	WinTwistApp* mApp;                 // +0x170
	int mShown = -99;                  // +0x174: the category shown (mCategory, once the old one has faded out)
	bool mUnknown178 = false;          // +0x178
	CurvedVal mRowSlide;               // +0x180: the rows make room for the new score
	int mNewRow = -1;                  // +0x1f0: the new score's row, -1 none
	int mUnknown1F4 = 0;               // +0x1f4
	int mUnknown1F8 = -1;              // +0x1f8
	Color mLastRowColor;               // +0x1fc: the tenth row's name colour, for the name that falls off the list
	int mRevealed = -1;                // +0x20c: the rows up to this one show
	float mFlash = 0;                  // +0x210: the glow of the row that just showed
	float mUnknown214 = 0;             // +0x214
	float mUnknown218 = 0;             // +0x218
	std::vector<Row> mRows[2];         // +0x21c: the two lists
	std::vector<TwistButton*> mReplayButtons;  // +0x23c: the stats page's
	EffectsManager* mFXManager;        // +0x24c
	TwistWidget* mBar;                 // +0x250: IMAGE_MAINMENU_SCORESDIALOGBAR, with the replay and quit buttons
	CurvedVal mBarSlide;               // +0x258: the bar rises from below the screen
	bool mHide = false;                // +0x2c8: mAlpha falls to 0, else rises to 1
	CurvedVal mFade;                   // +0x2d0: the rows' alpha while a category fades out
	bool mFading = false;              // +0x340: mFade runs
	int mCategory = -99;               // +0x344: the category to show (the gates' buttons pick it)
	float mVoiceDelay = 0;             // +0x348: the new high score's voice when it runs out
	bool mBarPressed = false;          // +0x34c: a bar button was pressed

	explicit HighScoresPanel(Menu* theMenu);  // 0x697d5b
	~HighScoresPanel() override;              // 0x695ae6

	void AddedToManager(WidgetManager* theWidgetManager) override;      // 0x695937
	void RemovedFromManager(WidgetManager* theWidgetManager) override;  // 0x694076
	void Update() override;                                             // 0x695b8d
	void Draw(Graphics* g) override;                                    // 0x696a02
	void MouseDown(int x, int y, int theBtnNum, int theClickCount) override;  // 0x694196
	void MouseUp(int x, int y, int theClickCount) override;                   // 0x6941a0
	using Widget::MouseDown;
	using Widget::MouseUp;

	void ButtonPress(int theId, int theClickCount) override;  // 0x69415c
	using ButtonListener::ButtonPress;
	void ButtonDepress(int theId) override;                   // 0x695124

	void SetCategory(int theCategory, bool theNow);  // 0x69599c
	void Refresh();          // 0x695a77: the top ten of classic again, for a new player
	void ShowBar();          // 0x694bd3: the bar rises under the panel (after a game)
	void HideBar();          // 0x694da4
	void ShowAll();          // 0x695966: every row at once, without the fade
	void StartFade();        // 0x694b5e
	void ShowCategory();     // 0x695890: mShown becomes mCategory
	void MakeReplayButtons();  // 0x69570d
	void ResetNewRow();      // 0x6941aa

	float GetRowHeight();    // 0x6940a3
	int GetRowY(int theRow); // 0x6940c6
	int GetNameX();          // 0x6940fd
	int GetScoreRight();     // 0x694113
	int GetLevelRight();     // 0x694135
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

// Sexy::ProfilePanel (0x1f0 bytes; 0x6a037b-0x6a0f55, menu +0x2e8): the player list, above the gates (which move
// it), with the current player's rank and name, and the buttons to make a new player and to delete one. It slides
// down onto the screen and back up (mSlide, a share of the screen's height).
class ProfilePanel : public MenuPanel, public ListListener, public TwistDialogListener, public ButtonListener {
public:
	// The buttons' ids
	enum { BUTTON_CREATE = 0, BUTTON_DELETE = 1 };
	// The delete dialog's id
	enum { DIALOG_DELETE = 0x20 };

	WinTwistApp* mApp;               // +0x154
	Menu* mMenu;                     // +0x158
	Image* mDialogImage;             // +0x15c: IMAGE_MAINMENU_PROFILEDIALOG
	bool mMouseDown = false;         // +0x160: between a press and its release (not read)
	ListWidget* mList;               // +0x164: the players
	TwistButton* mCreateButton;      // +0x168: off with 8 players
	TwistButton* mDeleteButton;      // +0x16c
	bool mRefresh = false;           // +0x170: fill the list again at the next update
	CurvedVal mSlide;                // +0x180: 0 down on the screen; Show and Hide run it

	explicit ProfilePanel(Menu* theMenu);  // 0x6a077b
	~ProfilePanel() override;              // 0x6a037b

	void AddedToManager(WidgetManager* theWidgetManager) override;      // 0x6a041f
	void RemovedFromManager(WidgetManager* theWidgetManager) override;  // 0x6a05f4
	void Update() override;                                             // 0x6a0ae9
	void Draw(Graphics* g) override;                                    // 0x6a0c83
	void DrawAll(ModalFlags* theFlags, Graphics* g) override;           // 0x6a0637
	void KeyChar(SexyChar theChar) override;                            // 0x6a06e0: Enter and Escape go back
	void MouseDown(int x, int y, int theBtnNum, int theClickCount) override;  // 0x6a06a8
	void MouseUp(int x, int y, int theClickCount) override;                   // 0x6a06b2
	using TwistWidget::MouseDown;
	using TwistWidget::MouseUp;

	void ListClicked(int theId, int theIdx, int theClickCount) override;  // 0x6a0b54
	void DialogButtonPress(int theDialogId, int theButtonId) override;    // 0x6a0748
	void DialogButtonDepress(int theDialogId, int theButtonId) override;  // 0x6a0c10
	void ButtonPress(int theId, int theClickCount) override;              // 0x6a0691
	void ButtonDepress(int theId) override;                               // 0x6a0e33

	void Show();                // 0x6a0f55
	void Hide();                // 0x6a0713
	void Refresh();             // 0x6a09db: the list from the players, the current one selected
	void ShowNewUser();         // 0x6a06bc
};

// Sexy::NewUserPanel (0x1c8 bytes, vtable 0x7d9bfc; its code is in the menu's file, 0x699695-0x69ab42; menu +0x3d4):
// the box for a new player's name, with OK and (when there is a player to go back to) cancel. Its buttons and its
// edit box report to the menu.
class NewUserPanel : public TwistWidget {
public:
	// The buttons' ids
	enum { BUTTON_OK = 100, BUTTON_CANCEL = 101 };

	TwistEditWidget* mEditWidget = nullptr;  // +0x148: while it is in a manager
	EditListener* mEditListener;     // +0x14c
	bool mCapitalize = false;        // +0x150: the name's first letter is made a capital at the next update
	CurvedVal mSlide;                // +0x158: from above the screen, in halves of its height

	NewUserPanel(ButtonListener* theButtonListener, EditListener* theEditListener, bool canCancel);  // 0x699695
	~NewUserPanel() override;        // 0x6998c9

	void AddedToManager(WidgetManager* theWidgetManager) override;      // 0x69ab42
	void RemovedFromManager(WidgetManager* theWidgetManager) override;  // 0x6999a4
	void Update() override;                                             // 0x69a7f7
	void Draw(Graphics* g) override;                                    // 0x699984
	void DrawAll(ModalFlags* theFlags, Graphics* g) override;           // 0x699924

	void Hide();                     // 0x6998fe: slides back up, then goes
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
	int Unknown6E0DDB(bool, bool);        // 0x6e0ddb: the challenges' locks and progress for the player
};

// Sexy::EndStats (0x4c8 bytes; 0x6fa5be-0x6fc0eb, menu +0x3dc): the stats after a game
class EndStats : public TwistWidget {
public:
	explicit EndStats(WinTwistApp* theApp);  // 0x6fa6e5
	virtual int GetPreferredWidth();   // slot 70 (+0x118)
	virtual int GetPreferredHeight();  // slot 71 (+0x11c)
};

}  // namespace Sexy
