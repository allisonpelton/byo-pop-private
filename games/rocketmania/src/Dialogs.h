#ifndef __ROCKETMANIA_DIALOGS_H__
#define __ROCKETMANIA_DIALOGS_H__

// The dialogs of Rocket Mania Deluxe. All of them are 2003 dialogs (id + 2000 / + 3000 for the dialog's own buttons,
// docs/FRAMEWORK_2003.md) drawn by the game itself: the dialog picture with a title in the Showcard font, the
// game's own buttons (DialogButton, the dark button picture) and an app-wide click sound.

#include "Compat2004.h"
#include "DialogButton.h"
#include "Checkbox.h"
#include "CheckboxListener.h"
#include "SliderListener.h"

namespace Sexy
{

class RocketApp;

// 0x4118d0: a dialog with a black field behind its picture and a phase that goes round once in 2000 updates (the base of
// the options dialog)
class MoneyDialog : public Dialog2004
{
public:
	RocketApp*			mApp;                   // +0xec
	double				mPhase;                 // +0xf0

public:
	MoneyDialog(RocketApp* theApp, int theId, bool isModal, const SexyString& theHeader, const SexyString& theLines,
		const SexyString& theFooter, int theButtonMode);

	virtual void		Update() override;      // 0x411b20
	virtual void		Draw(Graphics* g) override;     // 0x411980
};

// 0x403990: id 7 "Credits", opened over the options dialog; the picture, the names of the makers and a DONE button
class CreditsDialog : public Dialog2004
{
public:
	RocketApp*			mApp;                   // +0xec
	double				mPhase;                 // +0xf0 these three move on in every update; nothing reads them
	double				mScroll1;               // +0xf8
	double				mScroll2;               // +0x100

public:
	CreditsDialog(RocketApp* theApp);

	virtual int			GetPreferredHeight(int theWidth) override;     // 0x403c10
	virtual void		Draw(Graphics* g) override;                    // 0x403c20
	virtual void		Update() override;                             // 0x4042c0
};

// 0x411ba0: id 1 "Options": the volumes, the screen mode, the cursors, the hints; the DONE button applies them
// (RocketApp::ButtonDepress). The original also lists a register button (the game is registered), an update check
// (online) and the partner links (the user's decision for the ports, 2026-10-24): none of them is ported.
class OptionsDialog : public MoneyDialog, public SliderListener, public CheckboxListener
{
public:
	enum
	{
		CHECK_FULLSCREEN = 0,
		CHECK_CURSORS = 1,
		CHECK_HINTS = 2,
		CHECK_HARDWARE = 3,
		BUTTON_CREDITS = 6,
		BUTTON_RESET_HINTS = 0xe
	};

	bool				mFromMenu;              // +0x100 opened from the menu, not from a game
	int					mLastSoundUpdate;       // +0x104 the update at which the sound slider last clicked
	Slider*				mMusicSlider;           // +0x11c
	Slider*				mSfxSlider;             // +0x120
	Checkbox*			mFullscreenCheckbox;    // +0x124
	Checkbox*			m3DCheckbox;            // +0x128 hardware acceleration
	Checkbox*			mCursorsCheckbox;       // +0x12c
	Checkbox*			mHintsCheckbox;         // +0x130 "disable hints"
	DialogButton*		mResetHintsButton;      // +0x138
	DialogButton*		mCreditsButton;         // +0x13c

public:
	OptionsDialog(RocketApp* theApp, bool theFromMenu);
	virtual ~OptionsDialog();

	virtual void		SetColor(int theIdx, const Color& theColor) override;               // 0x4127c0
	virtual void		AddedToManager(WidgetManager* theWidgetManager) override;          // 0x412860
	virtual void		RemovedFromManager(WidgetManager* theWidgetManager) override;      // 0x412940
	virtual void		Resize(int theX, int theY, int theWidth, int theHeight) override;  // 0x412a20
	virtual int			GetPreferredHeight(int theWidth) override;                         // 0x412770
	virtual void		Draw(Graphics* g) override;                                        // 0x412c60
	virtual void		ButtonPress(int theId) override;                                   // 0x413470
	virtual void		ButtonDepress(int theId) override;                                 // 0x4134b0
	virtual void		SliderVal(int theId, double theVal) override;                      // 0x413040
	virtual void		CheckboxChecked(int theId, bool checked) override;                 // 0x4130b0
	using Widget::Resize;
};

// 0x404380: id 0xc "Difficulty": EASY, MEDIUM, HARD and CANCEL (the ids 0 to 3)
class DifficultyDialog : public Dialog2004
{
public:
	RocketApp*			mApp;                   // +0xec
	DialogButton*		mButtons[4];            // +0xf0 easy, +0xf4 medium, +0xf8 hard, +0xfc cancel

public:
	DifficultyDialog(RocketApp* theApp);
	virtual ~DifficultyDialog();

	virtual void		AddedToManager(WidgetManager* theWidgetManager) override;
	virtual void		RemovedFromManager(WidgetManager* theWidgetManager) override;
	virtual void		SetColor(int theIdx, const Color& theColor) override;
	virtual void		Resize(int theX, int theY, int theWidth, int theHeight) override;
	virtual void		Draw(Graphics* g) override;
	virtual void		ButtonPress(int theId) override;
	virtual void		ButtonDepress(int theId) override;
	using Widget::Resize;
};

// 0x402630: id 0xf "Continue": a saved game of the mode exists; NEW GAME or CONTINUE SAVED GAME
class ContinueDialog : public Dialog2004
{
public:
	RocketApp*			mApp;                   // +0x100
	int					mMode;                  // +0xec the mode whose saved game it is
	DialogButton*		mNewButton;             // +0x104 id 0
	DialogButton*		mContinueButton;        // +0x108 id 1
	int					mSavedDifficulty;       // +0xf4 read from the save file (1 to 3)
	int					mSavedScore;            // +0xf8
	int					mSavedLevel;            // +0xfc the level index (shown plus one)

public:
	ContinueDialog(RocketApp* theApp, int theMode);
	virtual ~ContinueDialog();

	virtual void		AddedToManager(WidgetManager* theWidgetManager) override;
	virtual void		RemovedFromManager(WidgetManager* theWidgetManager) override;
	virtual void		SetColor(int theIdx, const Color& theColor) override;
	virtual void		Resize(int theX, int theY, int theWidth, int theHeight) override;
	virtual void		Draw(Graphics* g) override;
	virtual void		ButtonPress(int theId) override;
	virtual void		ButtonDepress(int theId) override;
	using Widget::Resize;
};

}

#endif
