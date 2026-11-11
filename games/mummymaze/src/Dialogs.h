#ifndef __MUMMYMAZE_DIALOGS_H__
#define __MUMMYMAZE_DIALOGS_H__

// The dialogs of Mummy Maze Deluxe that are more than a header, some lines and buttons: the name for a high score
// (Dialogs.cpp), the options and the high scores (Dialogs2.cpp).

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

class MMApp;
class MemoryImage;
class HighScoreWidget;

// the colours of a name box's text, and the box itself (Dialogs.cpp)
extern int kNameEditColors[5][4];
void DrawEditBox(Graphics* g, Image* theBox, int theX, int theY, int theWidth);

// 0x427dc0: the name for a high score
class RecordScoreDialog : public Dialog2004, public EditListener
{
public:
	MMApp*			mApp;                   // +0xc4
	EditWidget*			mNameEdit;              // +0xc8

public:
	RecordScoreDialog(MMApp* theApp, const SexyString& theLines);
	virtual ~RecordScoreDialog();

	virtual void		AddedToManager(WidgetManager* theWidgetManager) override;           // 0x428090
	virtual void		RemovedFromManager(WidgetManager* theWidgetManager) override;       // 0x4280c0
	virtual void		Resize(int theX, int theY, int theWidth, int theHeight) override;   // 0x4280f0
	virtual int			GetPreferredHeight(int theWidth) override;                          // 0x428080
	virtual void		Draw(Graphics* g) override;                                         // 0x428140
	virtual void		EditWidgetText(int theId, const SexyString& theString) override;    // 0x428450
	using Widget::Resize;
};

// 0x401000: the saved adventures to choose from (five slots, each a name or NEW GAME), and the way to delete one
class AdventureDialog : public Dialog2004
{
public:
	MMApp*			mApp;                   // +0xc4
	DialogButton*		mSlots[5];              // +0xc8 ids 0 to 4
	DialogButton*		mDeleteButton;          // +0xdc id 5
	bool				mHasDelete;             // +0xe0

public:
	AdventureDialog(MMApp* theApp);
	virtual ~AdventureDialog();

	virtual void		AddedToManager(WidgetManager* theWidgetManager) override;           // 0x4016d0
	virtual void		RemovedFromManager(WidgetManager* theWidgetManager) override;       // 0x401710
	virtual void		Resize(int theX, int theY, int theWidth, int theHeight) override;   // 0x401750
	virtual int			GetPreferredHeight(int theWidth) override;                          // 0x4016b0
	virtual void		ButtonPress(int theId) override;                                    // 0x412d80
	virtual void		ButtonDepress(int theId) override;                                  // 0x4017f0
	using Widget::Resize;
	using Dialog2004::ButtonPress;
};

// 0x412750: the saved adventures again, to pick the one to delete
class DeleteDialog : public Dialog2004
{
public:
	MMApp*			mApp;
	DialogButton*		mSlots[5];              // +0xc8 ids 0 to 4

public:
	DeleteDialog(MMApp* theApp);
	virtual ~DeleteDialog();

	virtual void		AddedToManager(WidgetManager* theWidgetManager) override;           // 0x412ca0
	virtual void		RemovedFromManager(WidgetManager* theWidgetManager) override;       // 0x412cd0
	virtual void		Resize(int theX, int theY, int theWidth, int theHeight) override;   // 0x412d00
	virtual int			GetPreferredHeight(int theWidth) override;                          // 0x412c90
	virtual void		ButtonPress(int theId) override;                                    // 0x412d80
	virtual void		ButtonDepress(int theId) override;                                  // 0x412da0
	using Widget::Resize;
	using Dialog2004::ButtonPress;
};

// 0x421db0: the name of a new adventure
class NameDialog : public Dialog2004, public EditListener
{
public:
	MMApp*			mApp;                   // +0xc4
	EditWidget*			mNameEdit;              // +0xc8

public:
	NameDialog(MMApp* theApp);
	virtual ~NameDialog();

	virtual void		AddedToManager(WidgetManager* theWidgetManager) override;
	virtual void		RemovedFromManager(WidgetManager* theWidgetManager) override;
	virtual void		Resize(int theX, int theY, int theWidth, int theHeight) override;   // 0x422100
	virtual int			GetPreferredHeight(int theWidth) override;                          // 0x428080
	virtual void		Draw(Graphics* g) override;                                         // 0x4222d0
	virtual void		EditWidgetText(int theId, const SexyString& theString) override;
	using Widget::Resize;
};

// 0x4250f0: the options: music and sound volume, the speed of the game, four switches, and the way to the tutorial, the
// registration, the high scores, the web site and the credits
class OptionsDialog : public Dialog2004, public SliderListener, public CheckboxListener
{
public:
	MMApp*			mApp;                   // +0xc8
	Slider*				mMusicSlider;           // +0xcc id 0
	Slider*				mSfxSlider;             // +0xd0 id 1
	Slider*				mSpeedSlider;           // +0xd4 id 2: 0 normal, a half fast, 1 fastest
	Checkbox*			mFullscreenCheckbox;    // +0xd8
	Checkbox*			mCursorsCheckbox;       // +0xdc
	Checkbox*			mClockCheckbox;         // +0xe0
	Checkbox*			mAnkhCheckbox;          // +0xe4
	DialogButton*		mTutorialButton;        // +0xe8 id 0
	DialogButton*		mScoresButton;          // +0xf0 id 2
	DialogButton*		mCreditsButton;         // +0xf8 id 4
	MemoryImage*		mCheckedImage;          // the check box pictures (the checkboxes do not own them)
	MemoryImage*		mUncheckedImage;

public:
	OptionsDialog(MMApp* theApp);
	virtual ~OptionsDialog();

	virtual void		AddedToManager(WidgetManager* theWidgetManager) override;           // 0x425ff0
	virtual void		RemovedFromManager(WidgetManager* theWidgetManager) override;       // 0x4260c0
	virtual void		Resize(int theX, int theY, int theWidth, int theHeight) override;   // 0x426190
	virtual int			GetPreferredHeight(int theWidth) override;                          // 0x425f50
	virtual void		Draw(Graphics* g) override;                                         // 0x4263c0
	virtual void		SetColor(int theIdx, const Color& theColor) override;               // 0x425f60
	virtual void		ButtonPress(int theId) override;                                    // 0x426b50
	virtual void		ButtonDepress(int theId) override;                                  // 0x426b70
	virtual void		SliderVal(int theId, double theVal) override;                       // 0x426a30
	using Widget::Resize;
	using Dialog2004::ButtonPress;
};

// 0x42b720: a tip: the first time a new kind of danger is in a maze, with a box to not see it again
class WarningDialog : public Dialog2004, public CheckboxListener
{
public:
	MMApp*			mApp;                   // +0xc4
	Checkbox*			mCheckbox;              // +0xc8 "do not show this screen again"
	int					mWhich;                 // +0xcc 0 two mummies, 1 gate, 2 traps, 3 red mummy, 4 scorpion, 5 dark maze, 6 no ankh
	MemoryImage*		mCheckedImage;
	MemoryImage*		mUncheckedImage;

public:
	WarningDialog(MMApp* theApp, int theWhich);
	virtual ~WarningDialog();

	virtual void		AddedToManager(WidgetManager* theWidgetManager) override;           // 0x42bba0
	virtual void		RemovedFromManager(WidgetManager* theWidgetManager) override;       // 0x42bbd0
	virtual void		Resize(int theX, int theY, int theWidth, int theHeight) override;   // 0x42bc00
	virtual int			GetPreferredHeight(int theWidth) override;                          // 0x42bb80
	virtual void		ButtonPress(int theId) override;                                    // 0x412d80
	virtual void		Draw(Graphics* g) override;                                         // 0x42bc50
	using Widget::Resize;
	using Dialog2004::ButtonPress;
};

// 0x413c30: the high scores, as a dialog of the options
class HighScoreDialog : public Dialog2004
{
public:
	MMApp*			mApp;                   // +0xc4
	HighScoreWidget*	mScores;                // +0xc0 only the list is used

public:
	HighScoreDialog(MMApp* theApp);
	virtual ~HighScoreDialog();

	virtual int			GetPreferredHeight(int theWidth) override;                          // 0x413f80
	virtual void		Draw(Graphics* g) override;                                         // 0x413f90
};

}

#endif
