#ifndef __ALCHEMY_HIGHSCORES_H__
#define __ALCHEMY_HIGHSCORES_H__

// The score table (widget class 0x4e4198 + listener 0x4e417c, ctor 0x40bad1, size 0x21c): five scores for each of the two
// kinds of game (strategic and time trial), and the best score of the day. It is also the dialog of the all-time scores'
// data (HighScoreDialog makes one and only reads it). The widget shows the table when a game is over, after the statistics
// of that game have been drawn into a picture of their own; the original's internet scores (a second button, a name
// dialog for them, the nag of the unregistered game) are not in the port.

#include "Widget.h"
#include "ButtonListener.h"
#include <string>

namespace Sexy
{

class WinAlchApp;
class DialogButton;
class MemoryImage;

class HighScoreWidget : public Widget, public ButtonListener
{
public:
	enum
	{
		NUM_MODES = 2,              // 0: strategic, 1: time trial
		NUM_ENTRIES = 5
	};

	WinAlchApp*			mApp;                               // +0x5c
	int					mReveal;                            // +0x60 pixels of the table that are uncovered (6 more each update)
	DialogButton*		mStartButton;                       // +0x64 id 1 (the original's id 0 is the internet score button)
	MemoryImage*		mBoardImage;                        // +0x6c the board as it was at the end of the game
	MemoryImage*		mTableImage;                        // +0x70 the table on top of it
	int					mAnim;                              // +0x90 the rows' colours move on every 10 updates
	std::string			mTodayName[NUM_MODES];              // +0x98 the best score of the day
	int					mTodayScore[NUM_MODES];             // +0xd0
	std::string			mName[NUM_MODES][NUM_ENTRIES];      // +0xd8
	int					mScore[NUM_MODES][NUM_ENTRIES];     // +0x1f0

public:
	HighScoreWidget(WinAlchApp* theApp);                    // 0x40bad1
	virtual ~HighScoreWidget();                             // 0x40be02: writes the file

	static int			GetDayNum();                                // 0x409e60: the day, for the best score of the day
	void				DrawColumn(Graphics* g, const Rect& theRect, int theDX, int theDY, const SexyString& theLabel, const SexyString& theValue); // 0x409f01
	void				ReadFromFile();                             // 0x40a15a
	void				WriteToFile();                             // 0x40a341
	int					GetScorePlace(int theMode, int theScore);  // 0x409e88: 1 to 5, or 0 for a score that is not in the table
	void				ProcessScoreStuff();                       // 0x40a57b: says what the score of the game that ended got
	void				NameEntered(const SexyString& theName); // 0x40b9e8: puts the score of the game that ended in the table

	virtual void		AddedToManager(WidgetManager* theWidgetManager) override;       // 0x409eb3
	virtual void		RemovedFromManager(WidgetManager* theWidgetManager) override;   // 0x409eda
	virtual void		Resize(int theX, int theY, int theWidth, int theHeight) override; // 0x40b0b3
	virtual void		Draw(Graphics* g) override;                                     // 0x40a957
	virtual void		Update() override;                                              // 0x40afa9
	virtual void		ButtonPress(int theId) override;                                // 0x409f5b
	virtual void		ButtonDepress(int theId) override;                              // 0x409f6b

	using Widget::Resize;
	using ButtonListener::ButtonPress;
};

}

#endif
