#ifndef __NOAH_DIALOGS_H__
#define __NOAH_DIALOGS_H__

// The dialogs of Noah's Ark Deluxe: DifficultyDialog (WinNoah.exe 0x410c00), HighScoreDialog (0x4113e0),
// RecordScoreDialog (0x41d750), OptionsDialog (0x41c230) and TutorialDialog (0x4205b0). RegisterDialog
// (0x41de70) and the trial's other dialogs are not ported. All derive from the 2002 dialog (an app
// ButtonListener, buttons mId + 2000 / + 3000: Dialog2004).

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

class NoahApp;
class HighScoreWidget;

// 0x410c00: Easy, Medium and Hard start the game (the difficulty is 0, 4 or 8)
class DifficultyDialog : public Dialog2004
{
public:
	NoahApp*			mApp;                   // +0xc8
	DialogButton*		mEasyButton;            // +0xcc id 0x3c
	DialogButton*		mMediumButton;          // +0xd0 id 0x3d
	DialogButton*		mHardButton;            // +0xd4 id 0x3e

public:
	DifficultyDialog(NoahApp* theApp);
	virtual ~DifficultyDialog();

	virtual void		AddedToManager(WidgetManager* theWidgetManager) override;
	virtual void		RemovedFromManager(WidgetManager* theWidgetManager) override;
	virtual void		Resize(int theX, int theY, int theWidth, int theHeight) override;
	virtual int			GetPreferredHeight(int theWidth) override;
	virtual void		ButtonPress(int theId) override;
	virtual void		ButtonDepress(int theId) override;
	using Widget::Resize;
};

// 0x4113e0: the five best scores
class HighScoreDialog : public Dialog2004
{
public:
	NoahApp*			mApp;                   // +0xc8
	HighScoreWidget*	mScores;                // +0xc4 the list (loaded from hiscores.dat, never shown as a widget)

public:
	HighScoreDialog(NoahApp* theApp);
	virtual ~HighScoreDialog();

	virtual int			GetPreferredHeight(int theWidth) override;
	virtual void		Draw(Graphics* g) override;
};

// 0x41d750: asks for the name to put beside a high score
class RecordScoreDialog : public Dialog2004, public EditListener
{
public:
	NoahApp*			mApp;                   // +0xc8
	EditWidget*			mNameEdit;              // +0xcc id 0

public:
	RecordScoreDialog(NoahApp* theApp, const std::string& thePlace);
	virtual ~RecordScoreDialog();

	virtual void		AddedToManager(WidgetManager* theWidgetManager) override;
	virtual void		RemovedFromManager(WidgetManager* theWidgetManager) override;
	virtual void		Resize(int theX, int theY, int theWidth, int theHeight) override;
	virtual int			GetPreferredHeight(int theWidth) override;
	virtual void		Draw(Graphics* g) override;
	virtual void		EditWidgetText(int theId, const SexyString& theString) override;
	virtual bool		AllowChar(int theId, SexyChar theChar) override;
	using Widget::Resize;

	void				DrawEditBox(Graphics* g, int theX, int theY, int theWidth);     // 0x41dca0
};

// 0x41c230: volume, full screen, hints, and the way to the help, scores and credits
class OptionsDialog : public Dialog2004, public SliderListener, public CheckboxListener
{
public:
	NoahApp*			mApp;                   // +0xcc
	Slider*				mMusicSlider;           // +0xd0 id 0
	Slider*				mSfxSlider;             // +0xd4 id 1
	Checkbox*			mFullscreenCheckbox;    // +0xd8 id 0
	Checkbox*			mHintsCheckbox;         // +0xdc id 1
	DialogButton*		mHelpButton;            // +0xe0 id 0
	DialogButton*		mScoresButton;          // +0xe8 id 2
	DialogButton*		mCreditsButton;         // +0xf0 id 4

public:
	OptionsDialog(NoahApp* theApp);
	virtual ~OptionsDialog();

	virtual void		AddedToManager(WidgetManager* theWidgetManager) override;
	virtual void		RemovedFromManager(WidgetManager* theWidgetManager) override;
	virtual void		Resize(int theX, int theY, int theWidth, int theHeight) override;
	virtual int			GetPreferredHeight(int theWidth) override;
	virtual void		Draw(Graphics* g) override;
	virtual void		SetColor(int theIdx, const Color& theColor) override;
	virtual void		ButtonPress(int theId) override;
	virtual void		ButtonDepress(int theId) override;
	virtual void		SliderVal(int theId, double theVal) override;
	using Widget::Resize;
};

// 0x4205b0: how to play, with a small animated picture and a check box to not see it again
class TutorialDialog : public Dialog2004, public CheckboxListener
{
public:
	NoahApp*			mApp;                   // +0xc8
	Checkbox*			mAgainCheckbox;         // +0xcc id 0
	int					mWait;                  // +0xd0 updates the last part of the picture has shown
	int					mStep;                  // +0xd4 which part of the picture is showing (0 to 4)
	int					mCursorX;               // +0xd8 the cursor's position in the picture
	int					mCursorY;               // +0xdc
	bool				mClicked;               // +0xe0 the cursor is pressed down

public:
	TutorialDialog(NoahApp* theApp);
	virtual ~TutorialDialog();

	virtual void		AddedToManager(WidgetManager* theWidgetManager) override;
	virtual void		RemovedFromManager(WidgetManager* theWidgetManager) override;
	virtual void		Resize(int theX, int theY, int theWidth, int theHeight) override;
	virtual int			GetPreferredHeight(int theWidth) override;
	virtual void		ButtonPress(int theId) override;
	virtual void		Update() override;
	virtual void		Draw(Graphics* g) override;
	using Widget::Resize;
};

}

#endif
