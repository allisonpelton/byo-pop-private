#ifndef __SEVENSEAS_DIALOGS_H__
#define __SEVENSEAS_DIALOGS_H__

// The dialogs of Seven Seas Deluxe: OptionsDialog (WinSS.exe 0x418760), NextLevelDialog (0x417a30),
// RecordScoreDialog (0x419f40), HighScoreDialog (0x40f7b0) and NagDialog (the bonus screen's explanation 0x40c7d0
// and the warnings of the new kinds of ship 0x4262c0). RegisterDialog (0x41a650) and the trial's dialogs are not
// ported. All derive from the 2003 dialog (an app ButtonListener, buttons mId + 2000 / + 3000: Dialog2004).

#include "Compat2004.h"
#include "DialogButton.h"
#include "Checkbox.h"
#include "CheckboxListener.h"
#include "EditWidget.h"
#include "EditListener.h"
#include "Slider.h"
#include "SliderListener.h"
#include <string>

namespace Sexy
{

class SevenSeasApp;
class HighScoreWidget;

// 0x418760: volume, full screen, custom cursors, and the way to the help, the scores and the site
class OptionsDialog : public Dialog2004, public SliderListener, public CheckboxListener
{
public:
	SevenSeasApp*		mApp;                   // +0xdc
	DialogButton*		mHelpButton;            // +0xf0 id 0
	DialogButton*		mRegisterButton;        // +0xf4 id 1 (hidden)
	DialogButton*		mScoresButton;          // +0xf8 id 2
	DialogButton*		mWebButton;             // +0xfc id 3
	Slider*				mMusicSlider;           // +0xe0 id 0
	Slider*				mSfxSlider;             // +0xe4 id 1
	Checkbox*			mFullscreenCheckbox;    // +0xe8 id 0
	Checkbox*			mCursorsCheckbox;       // +0xec id 1

public:
	OptionsDialog(SevenSeasApp* theApp);
	virtual ~OptionsDialog();

	virtual void		AddedToManager(WidgetManager* theWidgetManager) override;
	virtual void		RemovedFromManager(WidgetManager* theWidgetManager) override;
	virtual void		Resize(int theX, int theY, int theWidth, int theHeight) override;
	virtual int			GetPreferredHeight(int theWidth) override;
	virtual void		Draw(Graphics* g) override;
	virtual void		ButtonDepress(int theId) override;
	virtual void		SliderVal(int theId, double theVal) override;
	using Widget::Resize;
};

// 0x417a30: after a level: the pirates sunk, the bonus and the rank
class NextLevelDialog : public Dialog2004
{
public:
	SevenSeasApp*		mApp;                   // +0xd4

public:
	NextLevelDialog(SevenSeasApp* theApp);

	virtual int			GetPreferredHeight(int theWidth) override;
	virtual void		Draw(Graphics* g) override;
};

// 0x419f40: asks for the name to put beside a high score
class RecordScoreDialog : public Dialog2004, public EditListener
{
public:
	SevenSeasApp*		mApp;
	EditWidget*			mNameEdit;              // id 0
	SexyString			mPlace;

public:
	RecordScoreDialog(SevenSeasApp* theApp, const SexyString& thePlace);
	virtual ~RecordScoreDialog();

	virtual void		AddedToManager(WidgetManager* theWidgetManager) override;
	virtual void		RemovedFromManager(WidgetManager* theWidgetManager) override;
	virtual void		Resize(int theX, int theY, int theWidth, int theHeight) override;
	virtual int			GetPreferredHeight(int theWidth) override;
	virtual void		Draw(Graphics* g) override;
	virtual void		EditWidgetText(int theId, const SexyString& theString) override;
	virtual bool		AllowChar(int theId, SexyChar theChar) override;
	using Widget::Resize;
};

// 0x40f7b0: the five best scores
class HighScoreDialog : public Dialog2004
{
public:
	SevenSeasApp*		mApp;
	HighScoreWidget*	mScores;                // the list (loaded from hiscores.dat, never shown as a widget)

public:
	HighScoreDialog(SevenSeasApp* theApp);
	virtual ~HighScoreDialog();

	virtual int			GetPreferredHeight(int theWidth) override;
	virtual void		Draw(Graphics* g) override;
};

// 0x40c7d0 / 0x4262c0: a page of text with a check box that turns it off
class NagDialog : public Dialog2004, public CheckboxListener
{
public:
	SevenSeasApp*		mApp;
	Checkbox*			mCheckbox;              // id 0 ("do not show this screen again")
	int					mWhich;                 // 0 blue pirate, 1 ghost ship, 2 sea monster, 3 whirlpools, -1 the bonus screen

public:
	NagDialog(SevenSeasApp* theApp, int theDialogId, int theWhich);
	virtual ~NagDialog();

	virtual void		AddedToManager(WidgetManager* theWidgetManager) override;
	virtual void		RemovedFromManager(WidgetManager* theWidgetManager) override;
	virtual void		Resize(int theX, int theY, int theWidth, int theHeight) override;
	virtual int			GetPreferredHeight(int theWidth) override;
	virtual void		Draw(Graphics* g) override;
	using Widget::Resize;
};

class HighScoreWidget;

}

#endif
