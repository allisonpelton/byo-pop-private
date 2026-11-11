#ifndef __ALCHEMY_DIALOGS_H__
#define __ALCHEMY_DIALOGS_H__

// The dialogs of Alchemy Deluxe that are classes of their own (Alchemy.exe 0x4096c8-0x40f314): the all-time scores, the
// options, and the name for a high score. The ones that are lines of text and buttons are DoDialog calls (AppScreens.cpp).
// The original's dialogs for the registration, the internet scores and the update check are not in the port.

#include "Compat2004.h"
#include "EditWidget.h"
#include "EditListener.h"
#include "Slider.h"
#include "SliderListener.h"
#include "Checkbox.h"
#include "CheckboxListener.h"
#include "DialogButton.h"

namespace Sexy
{

class WinAlchApp;
class HighScoreWidget;

// 0x4096c8 (size 0x464): the best score of the day and the five best of all time, for each kind of game
class HighScoreDialog : public Dialog2004
{
public:
	HighScoreWidget*	mScores;                // +0x114 the table's data (the widget itself is never shown)
	WinAlchApp*			mApp;                   // +0x118

public:
	HighScoreDialog(WinAlchApp* theApp);
	virtual ~HighScoreDialog();                   // 0x40965a

	virtual int			GetPreferredHeight(int theWidth) override;                      // 0x4096a4: 0x156
	virtual void		Draw(Graphics* g) override;                                     // 0x409854
};

// 0x40d836 (size 0x144): the music and sound volumes, full screen and custom cursors, and the ways to the high scores
// and the help. The original's button to the web site (id 3, "more games"), the registration button (id 1) and the update
// check's button (id 5) are not in the port.
class OptionsDialog : public Dialog2004, public SliderListener, public CheckboxListener
{
public:
	WinAlchApp*			mApp;                   // +0x11c
	Slider*				mMusicSlider;           // +0x120 id 0
	Slider*				mSfxSlider;             // +0x124 id 1
	Checkbox*			mFullscreenBox;         // +0x128
	Checkbox*			mCursorsBox;            // +0x12c
	DialogButton*		mScoresButton;          // +0x134 id 2
	DialogButton*		mHelpButton;            // +0x13c id 4

public:
	OptionsDialog(WinAlchApp* theApp);
	virtual ~OptionsDialog();                   // 0x40d528

	virtual void		AddedToManager(WidgetManager* theWidgetManager) override;       // 0x40d679
	virtual void		RemovedFromManager(WidgetManager* theWidgetManager) override;   // 0x40d701
	virtual void		Resize(int theX, int theY, int theWidth, int theHeight) override; // 0x40dee4
	virtual int			GetPreferredHeight(int theWidth) override;                      // 0x40de70
	virtual void		Draw(Graphics* g) override;                                     // 0x40e18e
	virtual void		SetColor(int theIdx, const Color& theColor) override;           // 0x40d602
	virtual void		ButtonPress(int theId) override;                                // 0x40d7f0
	virtual void		ButtonDepress(int theId) override;                              // 0x40e34b
	virtual void		SliderVal(int theId, double theVal) override;                   // 0x40d789

	using Widget::Resize;
	using Dialog2004::ButtonPress;
};

// 0x40ed00 (size 0x12c): the name for a score of the table
class RecordScoreDialog : public Dialog2004, public EditListener
{
public:
	WinAlchApp*			mApp;                   // +0x118
	EditWidget*			mEdit;                  // +0x11c

public:
	RecordScoreDialog(WinAlchApp* theApp, const SexyString& theLines);
	virtual ~RecordScoreDialog();                 // 0x40ea5d

	SexyString			GetName();

	virtual void		AddedToManager(WidgetManager* theWidgetManager) override;       // 0x40eac0
	virtual void		RemovedFromManager(WidgetManager* theWidgetManager) override;   // 0x40eaeb
	virtual void		Resize(int theX, int theY, int theWidth, int theHeight) override; // 0x40eb0a
	virtual int			GetPreferredHeight(int theWidth) override;                      // 0x40eab1
	virtual void		Draw(Graphics* g) override;                                     // 0x40eed1
	virtual void		EditWidgetText(int theId, const SexyString& theString) override; // 0x40eb4e: Enter answers the OK button

	using Widget::Resize;
};

}

#endif
