// The main menu's panels: the screens the menu pans to (MenuPanel, vtable 0x7d9d1c, a TwistWidget with only its own
// destructor 0x69a667), the challenge menu and the end-of-game stats.
//
// port: the high scores, help, options, profile and new player panels are ported; the others are not yet. Each of those has the fields
// and methods the menu uses, with the original's offsets and addresses; a method not yet ported does nothing (and says
// so once in the log). They come in their own stages.
#pragma once

#include "ButtonListener.h"
#include "CheckboxListener.h"
#include "CurvedVal.h"
#include "ListListener.h"
#include "PopAnim.h"
#include "SliderListener.h"
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
class TwistCheckbox;
class TwistSlider;
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

// Sexy::HelpPanel (400 bytes; 0x698131-0x699077, menu +0x2e0, and the app's +0xf44 in a game): the sixteen pages
// of how to play, each an animation of twistpam.pam (its label in the table at 0x84e5b0, played again whenever it
// stops) under the page's title and text. In the menu the gates turn the pages; over a game it has its own back,
// previous and next buttons and zooms in and out.
class HelpPanel : public MenuPanel, public ButtonListener, public PopAnimListener {
public:
	enum { PAGE_COUNT = 16 };
	// The buttons' ids (only over a game)
	enum { BUTTON_BACK = 0, BUTTON_PREV = 1, BUTTON_NEXT = 2 };

	Image* mDialogImage;             // +0x150: IMAGE_HOWTOPLAY_HELPDIALOG
	bool mMouseDown = false;         // +0x154: between a press and its release (not read)
	int mUnknown158 = 0;             // +0x158
	int mUnknown15C = 0;             // +0x15c
	float mUnknown160 = 5.0f;        // +0x160
	WinTwistApp* mApp;               // +0x16c
	bool mUnknown170 = false;        // +0x170
	EffectsManager* mFXManager;      // +0x174
	PopAnim* mAnim;                  // +0x178: the page's animation
	bool mUnknown17C = false;        // +0x17c
	int mPage = 0;                   // +0x180: the help page
	TwistButton* mPrevButton;        // +0x184: over a game only
	TwistButton* mNextButton;        // +0x188

	explicit HelpPanel(WinTwistApp* theApp);  // 0x699077
	~HelpPanel() override;                    // 0x698131

	void AddedToManager(WidgetManager* theWidgetManager) override;      // 0x6981b0
	void RemovedFromManager(WidgetManager* theWidgetManager) override;  // 0x6981d0
	void Update() override;                                             // 0x6981f0
	void Draw(Graphics* g) override;                                    // 0x6984b0
	void MouseDown(int x, int y, int theBtnNum, int theClickCount) override;  // 0x6982e6
	void MouseUp(int x, int y, int theClickCount) override;                   // 0x6982f0
	using Widget::MouseDown;
	using Widget::MouseUp;

	void ButtonPress(int theId, int theClickCount) override;  // 0x6982d2
	using ButtonListener::ButtonPress;
	void ButtonDepress(int theId) override;                   // 0x6983cf
	void PopAnimStopped(int theId) override;                  // 0x698366: the page's animation again

	void MakeAnim();            // 0x698f78
	void SetPage(int thePage);  // 0x698319
	void PrevPage();            // 0x6983aa
	void NextPage();            // 0x6983bc
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

// Sexy::OptionsPanel (0x1a0 bytes; 0x69e0f1-0x6a028b, menu +0x2ec, and the game's options dialog, mOptionsDialog):
// the volume sliders, the check boxes (full screen, 3D acceleration, hints, custom cursors, visualizers) and the
// three art resolutions as radio buttons. Over a game it also has the return, quit (or challenge) and restart (or
// help) buttons. A box that needs a restart, or that cannot be set, asks or says so in dialog 4 or 0x28 to 0x30;
// the settings are applied when the panel closes (ApplySettings, from WinTwistApp::FinishOptionsDialog).
class OptionsPanel : public MenuPanel, public SliderListener, public CheckboxListener, public ButtonListener,
	public TwistDialogListener {
public:
	// The sliders' ids
	enum { SLIDER_MUSIC = 0, SLIDER_SOUNDS = 1 };
	// The check boxes' ids; the resolutions' are RES_FIRST + their index in the table at 0x84e754
	enum { CHECKBOX_FULLSCREEN = 0, CHECKBOX_3D = 1, CHECKBOX_HINTS = 2, CHECKBOX_CUSTOM_CURSORS = 4,
		CHECKBOX_VISUALIZERS = 5, CHECKBOX_RES_FIRST = 6 };
	enum { RES_COUNT = 3 };
	// The buttons' ids (over a game only)
	enum { BUTTON_RETURN = 0, BUTTON_QUIT = 1, BUTTON_RESTART = 2, BUTTON_HELP = 3 };
	// What dialog 4 asked (mQuestion): a yes (Answer) sets the boxes to it and applies them. The art's question is
	// the size it would switch to (600 also when the larger art needs 3D)
	enum { QUESTION_3D_ON = 0, QUESTION_3D_OFF = 1, QUESTION_RES_600 = 2, QUESTION_RES_768 = 3,
		QUESTION_RES_1200 = 4 };

	WinTwistApp* mApp;                         // +0x158
	TwistSlider* mMusicSlider;                 // +0x15c
	TwistSlider* mSoundsSlider;                // +0x160
	TwistCheckbox* mFullscreenCheckbox;        // +0x164
	TwistCheckbox* m3DCheckbox;                // +0x168
	TwistCheckbox* mCustomCursorsCheckbox;     // +0x16c
	TwistCheckbox* mHintsCheckbox;             // +0x170
	TwistCheckbox* mVisualizersCheckbox;       // +0x178
	TwistCheckbox* mResCheckboxes[RES_COUNT] = {};  // +0x17c: 600, 768 and 1200
	TwistButton* mQuitButton = nullptr;        // +0x188: over a game; the challenge button in a challenge
	TwistButton* mReturnButton = nullptr;      // +0x18c: over a game
	TwistButton* mRestartButton = nullptr;     // +0x190: over a game; the help button in classic
	bool mSettingBoxes = false;                // +0x194: Answer sets boxes, which must not ask again
	int mQuestion = 0;                         // +0x198

	explicit OptionsPanel(WinTwistApp* theApp);  // 0x69e4c5
	~OptionsPanel() override;                    // 0x69e013

	void AddedToManager(WidgetManager* theWidgetManager) override;      // 0x69e10a: pauses the game
	void RemovedFromManager(WidgetManager* theWidgetManager) override;  // 0x69e201: and resumes it
	void Update() override;                                             // 0x69e252
	void Draw(Graphics* g) override;                                    // 0x69e3c3
	void KeyDown(KeyCode theKey) override;                              // 0x69e36f: Escape and Enter return
	virtual int GetPreferredWidth();                                    // 0x69e0f1 (slot 70, as EndStats's)

	void SliderVal(int theId, double theVal) override;          // 0x69e301
	void CheckboxChecked(int theId, bool checked) override;     // 0x69ed33
	void ButtonDepress(int theId) override;                     // 0x69fc10
	using ButtonListener::ButtonPress;

	// 0x69df27: a radio button of the shared image at the place of thePlaceId in resources.xml
	TwistCheckbox* MakeCheckbox(int thePlaceId, int theId);
	// 0x6a006a: applies the boxes (full screen, 3D, custom cursors) and, with theResolution, the resolution, which
	// restarts the game
	void ApplySettings(bool theResolution);
	void Answer(bool theYes);  // 0x6a028b: dialog 4's answer (anything but Cancel)
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
