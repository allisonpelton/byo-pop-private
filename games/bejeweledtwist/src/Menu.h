// Sexy::Menu (0x4b8 bytes, a TwistWidget, TwistDialogListener, ButtonListener and EditListener; 0x699a11-0x69c529,
// vtable 0x7d9efc; app +0x864): the main menu. It is three screens wide and three high, centred on the screen, and
// pans between its panels: the spiral button in the middle, the options above, the profiles above the gates (moved
// by the gates themselves), the high scores to the left and the help to the right, on the middle row. The gates'
// buttons (Gates.h) send their presses here.
#pragma once

#include "ButtonListener.h"
#include "ColorCycle.h"
#include "CurvedVal.h"
#include "EditListener.h"
#include "TwistDialogListener.h"
#include "TwistWidget.h"

namespace Sexy {

class ChallengeMenu;
class EffectsManager;
class EndStats;
class HelpPanel;
class HighScoresPanel;
class NewUserPanel;
class OptionsPanel;
class PlayButton;
class ProfilePanel;
class WinTwistApp;

class Menu : public TwistWidget, public TwistDialogListener, public ButtonListener, public EditListener {
public:
	// What the menu shows (mPanel)
	enum { PANEL_MAIN = 0, PANEL_PROFILE = 1, PANEL_SCORES = 2, PANEL_OPTIONS = 4, PANEL_HELP = 5 };

	int mPanX = 0;                     // +0x154: where the pan goes
	int mPanY = 0;                     // +0x158
	int mPanFromX = 0;                 // +0x15c
	int mPanFromY = 0;                 // +0x160
	CurvedVal mPan;                    // +0x168: 0 at mPanFrom, 1 at mPan
	WinTwistApp* mApp;                 // +0x1d8
	float mUnknown1DC = 0;             // +0x1dc: rises by 0.01 per update to 1 (not yet read)
	int mState = 0;                    // +0x1e0: 2 the menu, 3 a game is starting
	int mGameMode = -1;                // +0x1e4: the game to start (-1: the last one)
	int mUnknown1E8 = 0;               // +0x1e8
	CurvedVal mStart;                  // +0x1f0: runs while the menu comes up and while a game starts
	float mStartDelay = 0;             // +0x260: mStart waits until this falls to 0 (by 0.01 per update)
	CurvedVal mUnknown268;             // +0x268
	float mUnknown2D8 = 0.01f;         // +0x2d8
	HighScoresPanel* mHighScoresPanel = nullptr;  // +0x2dc
	HelpPanel* mHelpPanel = nullptr;   // +0x2e0
	PlayButton* mPlayButton = nullptr; // +0x2e4
	ProfilePanel* mProfilePanel = nullptr;  // +0x2e8
	OptionsPanel* mOptionsPanel = nullptr;  // +0x2ec
	int mProfileY = 0;                 // +0x2f0: the menu's y with the profiles on the screen
	int mUnknown2F4 = 0;               // +0x2f4: the menu's height when it was last sized
	ColorCycle mColorCycles[4];        // +0x2f8: red, yellow, green and cyan, a quarter apart
	float mGatesShut = 1.0f;           // +0x3c8: rises to 1 while the gates are shut, falls while they are open
	float mNoBoard = 1.0f;             // +0x3cc: rises to 1 while there is no board, falls while there is
	bool mNoProfile = true;            // +0x3d0: no player yet, or the tutorial's
	NewUserPanel* mNewUserPanel = nullptr;  // +0x3d4: while it is up
	int mPanel = PANEL_MAIN;           // +0x3d8
	EndStats* mEndStats = nullptr;     // +0x3dc: the stats after a game
	EffectsManager* mFXManager;        // +0x3e0: just behind the menu
	SexyString mUnknown3E4;            // +0x3e4: the new high score's name
	int mUnknown400 = 0;               // +0x400: its level
	int mUnknown404 = 0;               // +0x404: its score
	int mUnknown408 = -1;              // +0x408: the high score just made (-1: none)
	bool mPanning = false;             // +0x40c: the menu is not where its pan ends
	CurvedVal mZoom;                   // +0x410: Direct3D only (DrawAll's zoom)
	ChallengeMenu* mChallengeMenu;     // +0x4a8
	int mMusic = 0;                    // +0x4ac: the menu's tune (PlayMusic)
	bool mFirstShow = true;            // +0x4b0: the title drops in only the first time
	int mAutoPlayGames = 0;            // +0x4b4: -autoplay's games started

	explicit Menu(WinTwistApp* theApp);  // 0x69d001
	~Menu() override;                    // 0x69adbc

	void AddedToManager(WidgetManager* theWidgetManager) override;      // 0x69c05f
	void RemovedFromManager(WidgetManager* theWidgetManager) override;  // 0x699ae1
	void Update() override;                                             // 0x69b642
	void Draw(Graphics* g) override;                                    // 0x69a695
	void DrawAll(ModalFlags* theFlags, Graphics* g) override;           // 0x699bbd
	void Resize(int theX, int theY, int theWidth, int theHeight) override;  // 0x699a40
	using Widget::Resize;
	void GotFocus() override;                                           // 0x69c504
	void KeyChar(SexyChar theChar) override;                            // 0x699d2a
	void MouseWheel(int theDelta) override;                             // 0x699a11

	void DialogButtonDepress(int theDialogId, int theButtonId) override;  // 0x699d33
	void ButtonPress(int theId, int theClickCount) override;  // 0x699b73
	void ButtonDepress(int theId) override;                   // 0x69c529
	using ButtonListener::ButtonPress;
	bool AllowChar(int theId, SexyChar theChar) override;     // 0x699dc6
	bool AllowText(int theId, const SexyString& theText) override;  // 0x69a881
	void EditWidgetText(int theId, const SexyString& theString) override;  // 0x69baaf: the new player's name

	void SetPan(int theX, int theY, bool theNow);  // 0x69af49
	void SetState(int theState);                   // 0x69b50a
	void ShowMain(bool theNow);                    // 0x69b36c
	void ShowProfiles();                           // 0x69afc5
	void ShowScores(bool theNow);                  // 0x69b093
	// 0x69aef5: the high score just made, for the scores shown next
	void SetNewHighScore(int thePlace, SexyString theName, int theLevel, int theScore);
	// 0x69aabc: the high score just made goes into the scores panel's list, and the menu forgets it
	void ShowNewHighScore();
	void ShowHelp(bool theNow);                    // 0x69b20d
	void ShowEndStats();                           // 0x699e17
	void ShowChallenges(bool theNow);              // 0x699eea
	void HideChallenges();                         // 0x699f3a
	void FlipProfileButton(bool theFlipped);       // 0x699b93
	void PlayMusic(int theMusic);                  // 0x699b45: -1 the one playing last
	void StartGame(int theMode);                   // 0x69ba8a
	void ShowNewUser();                            // 0x69bc9b: the new player panel, unless it is up
	void RefreshPlayer();                          // 0x699f60: the scores and challenges for a new player
};

}  // namespace Sexy
