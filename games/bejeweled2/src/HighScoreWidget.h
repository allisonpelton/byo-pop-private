#ifndef __HIGHSCOREWIDGET_H__
#define __HIGHSCOREWIDGET_H__

// HighScoreWidget.h: the panel of high scores that slides in over the menu or the board (Sexy::HighScoreWidget,
// 0x5c352e-0x5c530a and its helpers; the object is 0xcf0 bytes). It shows the tables of the two classic modes, or of the
// one secret mode the game was in, the player's rank and either the statistics of the profile or those of the game that
// just ended, and a button back and one to play again (or to reset the tables when no game is open). The tables live in
// users/hiscores.dat; the score of a finished game is put into them the moment the panel opens.
// The offsets in the comments are the original's, from the start of the object.
// (The original's helpers for the day of the year, the "st", "nd", "rd" and "th" of a place and the time have no callers
// and are not ported.)

#include "Widget.h"
#include "ButtonListener.h"
#include "CurvedVal.h"
#include <string>

namespace Sexy
{

class WinDMApp;
class PodButton;

class HighScoreWidget : public Widget, public ButtonListener
{
public:
	enum
	{
		NUM_TABLES = 9,         // one table for each game mode
		NUM_PLACES = 10
	};

	enum
	{
		BUTTON_BACK = 0,        // "back to the menu" or "back to the game"
		BUTTON_PLAY_AGAIN = 1,
		BUTTON_RESET = 2        // "reset scores" (the panel was opened from the menu)
	};

public:
	WinDMApp*		mApp;               // +0x8c (the ButtonListener subobject is at +0x88)
	PodButton*		mBackButton;        // +0x94 id 0
	PodButton*		mActionButton;      // +0x98 id 1 (play again), or id 2 (reset the scores) when no game is open
	int				mSlideDelay;        // +0x9c updates before the panel slides in
	CurvedVal		mPanelSlide;        // +0xa0 how far the tables are out of the panel to the left and right (900 to 0)
	CurvedVal		mButtonSlide;       // +0xd0 the same for the buttons and their frames (1500 to 0)
	CurvedVal		mTitleSlide;        // +0x100 the title is this far above its place (400 to 0)
	CurvedVal		mBottomSlide;       // +0x130 the bottom plate is this far below its place (600 to 0)
	std::string		mRank;              // +0x160 the player's rank, like "Senior Gemologist"
	std::string		mNames[NUM_TABLES][NUM_PLACES];     // +0x17c
	int				mScores[NUM_TABLES][NUM_PLACES];    // +0xb54
	int				mNewPlace[NUM_TABLES];              // +0xcbc the place the last score took in each table (0 to 9), -1 for none
	int				mHueOffset;         // +0xce0 the colors of the places cycle, one step every 10 updates
	int				mClosing;           // +0xce4 1 once a button was pressed: the panel slides out
	bool			mGameEnded;         // +0xce8 a finished game opened the panel (never read)
	bool			mShowGame;          // +0xce9 the statistics are those of the game that just ended
	int				mPulse;             // +0xcec counts the updates; the new place's color pulses with it

public:
	HighScoreWidget(WinDMApp* theApp);                              // 0x5c530a
	virtual ~HighScoreWidget();                                     // 0x5c5798

	virtual void	AddedToManager(WidgetManager* theManager) override;     // 0x5c352e
	virtual void	RemovedFromManager(WidgetManager* theManager) override; // 0x5c355e
	virtual void	Update() override;                              // 0x5c35fa
	virtual void	Resize(int theX, int theY, int theWidth, int theHeight) override;   // 0x5c37d7
	virtual void	Draw(Graphics* g) override;                     // 0x5c3efe
	virtual void	ButtonPress(int theId) override;                // 0x5c37fe
	virtual void	ButtonDepress(int theId) override;              // 0x5c50f7

	virtual void	ProcessScoreStuff();                            // 0x5c385d (slot 70, through the thunk at 0x5c38f2)
	virtual void	NameEntered(const std::string& theName);        // 0x5c5029 (slot 71)

	void			ResetHighScores();                              // 0x5c38f7
	void			ReadFromFile();                                 // 0x5c3add
	void			WriteToFile();                                  // 0x5c3c76
	int				GetScorePlace(int theMode, int theScore);       // 0x5c3501 1 to 10, 0 when the score is not in the table
	void			DrawShadowString(Graphics* g, const std::string& theString, int theX, int theY);   // 0x5c358e
};

}

#endif
