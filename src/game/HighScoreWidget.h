#ifndef __HIGHSCOREWIDGET_H__
#define __HIGHSCOREWIDGET_H__

// HighScoreWidget.cpp: the game-over screen (slides down over a snapshot of the board) and the
// owner of the high-score tables / hiscores.dat. HighScoreDialog also creates one (never added to
// the WidgetManager) just to load, show and re-save the tables.
// Original: class Sexy::HighScoreWidget : public Widget, public ButtonListener, sizeof 0x23c.
// Vtables: Widget 0x506cdc (72 slots: 70 CheckForHighScore, 71 AddHighScore), ButtonListener 0x506cb8.
// Offsets are absolute; Ghidra's HighScoreWidget_data.offset_N is absolute N+0x8c.
// Behaviour spec: docs/GAME_NOTES.md (HighScoreWidget section).
// Port: hiscores.dat lives in the VFS user dir under GameConfig::ScopedFileName (gameplay mods get
// their own table); the game is always registered, so only the record-score dialog path is ported.

#include "Widget.h"
#include "ButtonListener.h"

namespace Sexy
{

class WinDMApp;
class Graphics;
class WidgetManager;
class HyperlinkWidget;
class DDImage;

class HighScoreWidget : public Widget, public ButtonListener
{
public:
	enum
	{
		NUM_HIGH_SCORES = 5,            // all-time entries per mode
		NUM_HIGH_SCORE_MODES = 2,       // 0 = normal, 1 = timed
		HIGHSCORE_CHECKSUM_KEY = 0x42BEEF,
		PLAY_AGAIN_LINK_ID = 0,
		SLIDE_SPEED = 8,                // px per tick
		PANEL_CLIP_BOTTOM = 436
	};

	// +0x088 ButtonListener vptr
	WinDMApp*		mApp;                                   // +0x08c
	int				mSlideY;                                // +0x090 panel y offset: -height after Resize, +8/tick up to 0
	HyperlinkWidget* mPlayAgainLink;                        // +0x094 "CLICK HERE TO PLAY AGAIN" (top-level widget)
	DDImage*		mBoardImage;                            // +0x098 snapshot of the board taken by the first Resize
	SexyString		mTip;                                   // +0x09c HSWIDGET_TIP0..7 / HSWIDGET_TIP_DEFAULT (Rand()%9)
	SexyString		mTodayNames[NUM_HIGH_SCORE_MODES];      // +0x0b8 today's best name per mode
	int				mTodayScores[NUM_HIGH_SCORE_MODES];     // +0x0f0 today's best score per mode (<= 0 shows "Empty")
	SexyString		mNames[NUM_HIGH_SCORE_MODES][NUM_HIGH_SCORES];  // +0x0f8 all-time names (mode stride 0x8c)
	int				mScores[NUM_HIGH_SCORE_MODES][NUM_HIGH_SCORES]; // +0x210 all-time scores, descending
	int				mColorCycle;                            // +0x238 hue counter, ++ every 10 ticks

public:
	static int		GetDayNumber();                                 // 0x4ced59 tm_year*366 + tm_yday (thiscall, this unused)
	int				GetRank(int theMode, int theScore);             // 0x4ced82 1-based place, 0 = none
	virtual void	AddedToManager(WidgetManager* theWidgetManager);  // 0x4cedaf slot 20
	virtual void	RemovedFromManager(WidgetManager* theWidgetManager); // 0x4cedd2 slot 21
	virtual void	Resize(int theX, int theY, int theWidth, int theHeight); // 0x4cedf5 slot 40
	virtual void	ButtonDepress(int theId);                       // 0x4ceeb7 ButtonListener slot 2 (play again)
	void			LoadHighScores();                               // 0x4ceec2
	void			SaveHighScores();                               // 0x4cf1b4
	static SexyString GetOrdinalSuffix(int theNum);                 // 0x4cf4a7 "st"/"nd"/"rd"/"th" (thiscall, this unused)
	virtual void	Draw(Graphics* g);                              // 0x4cf50a slot 26
	HighScoreWidget(WinDMApp* theApp);                              // 0x4d07cd
	virtual ~HighScoreWidget();                                     // 0x4d0dad (scalar deleting dtor 0x4d1202)
	void			DoCheckForHighScore();                          // 0x4d0e4b body of CheckForHighScore (Update calls it directly)
	virtual void	Update();                                       // 0x4d1187 slot 22

	virtual void	CheckForHighScore();                            // 0x4d11fd slot 70 (jmp 0x4d0e4b)
	virtual void	AddHighScore(const SexyString& theName);        // 0x4d06d8 slot 71

	// port: text helpers shared with HighScoreDialog::Draw (inlined at every site in the original)
	static void		DrawCenteredString(Graphics* g, const SexyString& theString, int theCenterX, int theY);
	static void		DrawHeading(Graphics* g, const SexyString& theString, int theCenterX, int theY); // centered, 1 px underline at y+3
	static void		DrawScoreRow(Graphics* g, const SexyString& theName, int theScore, int theCenterX, int theY); // name at x-74, score right-aligned at x+74

	using Widget::Resize;
};

}

#endif
