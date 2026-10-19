#ifndef __HIGHSCOREWIDGET_H__
#define __HIGHSCOREWIDGET_H__

// HighScoreWidget.cpp: the game-over high-score panel (Board creates it at game over, slides it up from
// the bottom and keeps it in board->mHighScoreWidget) and the owner of the three high-score tables and
// hiscores.dat. HighScoreDialog creates a private one (never added to a WidgetManager) just to load,
// show and re-save the tables.
// Original: class Sexy::HighScoreWidget : public Widget, public ButtonListener, sizeof 0x304,
// vftable 0x4f651c (57 slots: 54 CheckForHighScore, 55 AddHighScore, 56 the internet "submitted"
// state, not ported), ButtonListener 0x4f64f8. Offsets are absolute.
// Behaviour spec: re/bigmoney/analysis/dialogs.md (section 1, hiscores.dat in section 5).

#include "Widget.h"
#include "ButtonListener.h"
#include <string>

namespace Sexy
{

class WinBMApp;
class Graphics;
class WidgetManager;
class DialogButton;

class HighScoreWidget : public Widget, public ButtonListener
{
public:
	enum
	{
		NUM_HIGH_SCORE_MODES = 3,           // tables per game type: Action, Strategy, Puzzle (app->mGameType)
		NUM_HIGH_SCORES = 5,                // all-time entries per table, descending
		HIGHSCORE_CHECKSUM_KEY = 0x42BEEF,
		NEW_GAME_BUTTON_ID = 1,
		CHECK_HIGH_SCORE_TICK = 280,        // Update: the 280th update calls DoCheckForHighScore
		CLIP_SCREEN_Y = 252                 // Draw hides everything above this screen y
	};

	// +0x080 ButtonListener vptr
	WinBMApp*		mApp;                                   // +0x084
	// +0x088 int, ctor 0, never read
	DialogButton*	mNewGameButton;                         // +0x08c id 1 "CLICK_HERE_TO_START_A_NEW_GAME"; added to the manager but never sized (0x0, invisible), so it never fires
	// +0x090 DialogButton* "submit to internet high score list" (id 0, never sized): internet high scores, not ported
	// +0x094 Widget*, +0x098 Widget* (always NULL, deleted by the dtor), +0x09c std::string (unused): internet leftovers, not ported
	int				mColorCycle;                            // +0x0b8 ++ every 10 updates; row hues
	std::string		mFileName;                              // port(mods): hiscores.dat, or its copy for the gameplay mods / a set seed (ModRules.h)
	bool			mViewOnly;                              // port(mods): a replay's game-over screen: no record check, never saved (Replay.h)
	// +0x0bc int, never initialised or used
	std::string		mTodayNames[NUM_HIGH_SCORE_MODES];      // +0x0c0 today's best name per mode ("" if none)
	int				mTodayScores[NUM_HIGH_SCORE_MODES];     // +0x114 today's best score per mode (0 = none: "N/A")
	std::string		mNames[NUM_HIGH_SCORE_MODES][NUM_HIGH_SCORES];  // +0x120 all-time names (mode stride 0x8c)
	int				mScores[NUM_HIGH_SCORE_MODES][NUM_HIGH_SCORES]; // +0x2c4 all-time scores (mode stride 0x14)
	// +0x300 int, ctor 0, set to 1000 by the internet "submitted" slot 56 (0x412c60), never read: not ported

public:
	static int		GetDayNumber();                                 // 0x412b80 tm_year*366 + tm_yday (local time)
	int				GetRank(int theMode, int theScore);             // 0x412bb0 1-based place, 0 = none; ties do not place
	virtual void	AddedToManager(WidgetManager* theWidgetManager);  // 0x412bf0 slot 7
	virtual void	RemovedFromManager(WidgetManager* theWidgetManager); // 0x412c20 slot 8
	// slot 25 Resize (0x412c50) is a bare jmp to Widget::Resize: not overridden here
	void			LoadHighScores();                               // 0x412eb0
	void			SaveHighScores();                               // 0x413100
	void			DoCheckForHighScore();                          // 0x4133b0 body of CheckForHighScore (Update calls it directly)
	virtual void	Draw(Graphics* g);                              // 0x413b10 slot 13
	virtual void	Update();                                       // 0x4144e0 slot 9
	virtual void	CheckForHighScore();                            // 0x414520 slot 54 (jmp 0x4133b0); WinBMApp calls it after a registration
	virtual void	AddHighScore(const std::string& theName);       // 0x414530 slot 55; WinBMApp on RecordScoreDialog OK / Enter
	HighScoreWidget(WinBMApp* theApp, bool theSeededTables = false, bool theViewOnly = false); // 0x414640 (port(mods): theSeededTables, ModRules.h; theViewOnly, Replay.h)
	virtual ~HighScoreWidget();                                     // 0x414c50 (scalar deleting dtor 0x414d60 slot 0); always re-saves

	// ButtonListener slot 1: 0x401c70, one body shared with Board (mApp->PlaySample(SOUND_BUTTONCLICK)).
	// Slot 2 ButtonDepress is the empty default: the own button does nothing.
	virtual void	ButtonPress(int theId);                         // 0x401c70 ButtonListener slot 1

	using ButtonListener::ButtonPress;
};

}

#endif
