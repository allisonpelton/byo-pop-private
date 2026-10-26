#ifndef __PIXELUS_OPTIONSDIALOG_H__
#define __PIXELUS_OPTIONSDIALOG_H__

// OptionsDialog.cpp: the options dialog (Pixelus.exe 0x419330 constructor over the application layer's 0x4e8b80, Draw
// 0x4e83c0 and 0x419260, Resize 0x4e7f50 and 0x4190b0, buttons 0x4e8950; vftable 0x50e3d0, dialog id 2). Two sliders
// (music, sound effects), the check boxes for the screen and the cursors, the hints, and the buttons: reset the hints,
// the credits, how to play, and, while a puzzle is open, back to the main menu.
//
// port: the "Hardware Acceleration" box and the "Check for updates" button of the original have nothing to switch (SDL
// draws the same way everywhere) and are left out, as are the registration button and the "visit" links of the
// application layer.

#include "GameDialog.h"
#include "SliderListener.h"
#include "CheckboxListener.h"

namespace Sexy
{

class Slider;
class Checkbox;
class DialogButton;

class OptionsDialog : public GameDialog, public SliderListener, public CheckboxListener
{
public:
	enum
	{
		DIALOG_ID = 2,
		ID_RESET_HINTS = 2,
		ID_CREDITS = 3,
		ID_MUSIC = 4,
		ID_SOUND = 5,
		ID_FULLSCREEN = 6,
		ID_CURSORS = 7,
		ID_HINTS = 8,
		ID_HOW_TO_PLAY = 500,		// +0x190
		ID_MAIN_MENU = 501			// +0x194, only in a game
	};

	Slider*			mMusicSlider;		// +0x158
	Slider*			mSoundSlider;		// +0x15c
	Checkbox*		mFullscreenBox;		// +0x160
	Checkbox*		mCursorsBox;		// +0x168
	Checkbox*		mHintsBox;			// +0x16c
	DialogButton*	mResetButton;		// +0x174
	DialogButton*	mCreditsButton;		// +0x178
	DialogButton*	mHelpButton;		// +0x190
	DialogButton*	mMenuButton;		// +0x194
	bool			mInGame;			// +0x198

public:
	OptionsDialog(PixelusApp* theApp, bool theInGame);	// 0x419330
	virtual ~OptionsDialog();

	virtual void	AddedToManager(WidgetManager* theWidgetManager) override;		// 0x419020
	virtual void	RemovedFromManager(WidgetManager* theWidgetManager) override;
	virtual void	Resize(int theX, int theY, int theWidth, int theHeight) override;	// 0x4190b0
	virtual int		GetPreferredHeight(int theWidth) override;						// 0x4e7c80
	virtual void	Draw(Graphics* g) override;										// 0x419260
	virtual void	ButtonDepress(int theId) override;								// 0x4e8950
	virtual void	SliderVal(int theId, double theVal) override;
	virtual void	CheckboxChecked(int theId, bool theChecked) override;

	using Widget::Resize;
};

}

#endif
