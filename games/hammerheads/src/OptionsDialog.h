#ifndef __HAMMERHEADS_OPTIONSDIALOG_H__
#define __HAMMERHEADS_OPTIONSDIALOG_H__

// The options box: the volumes of the music and the sounds, the check boxes for hardware acceleration, full screen and the
// custom cursors, the buttons that show the how-to-play movie again and open the credits, and "Done". The game stands still while
// it is up (the counted pause of PauseGame).
// Original: class Sexy::OptionsDialog : public NcMovingDialog, public SliderListener, public CheckboxListener (0x190 bytes; the
// constructor is 0x45b760, the destructor 0x45b1d0).

#include "CheckboxListener.h"
#include "NcMovingDialog.h"
#include "SliderListener.h"

namespace Sexy
{

class Checkbox;
class DialogButton;
class Slider;

class OptionsDialog : public NcMovingDialog, public SliderListener, public CheckboxListener
{
public:
	Slider*			mMusicSlider;		// +0x170: id 0
	Slider*			mSfxSlider;			// +0x174: id 1
	DialogButton*	mResetHelpButton;	// +0x178: id 2
	DialogButton*	mHowToPlayButton;	// +0x17c: id 3
	DialogButton*	mCreditsButton;		// +0x180: id 4
	Checkbox*		mHardwareBox;		// +0x184: id 6, the 3D mode
	Checkbox*		mFullScreenBox;		// +0x188: id 5
	Checkbox*		mCursorsBox;		// +0x18c: id 7, the cursors of the game's own pictures

public:
	OptionsDialog();														// 0x45b760
	virtual ~OptionsDialog();												// 0x45b1d0

	virtual void	AddedToManager(WidgetManager* theWidgetManager) override;			// 0x45b2c0
	virtual void	RemovedFromManager(WidgetManager* theWidgetManager) override;		// 0x45b390
	virtual void	Resize(int theX, int theY, int theWidth, int theHeight) override;	// 0x45b420
	virtual void	Draw(Graphics* g) override;								// 0x45bcb0
	using NcMovingDialog::ButtonPress;
	virtual void	ButtonPress(int theId, int theClickCount) override;		// 0x44fe90
	virtual void	ButtonDepress(int theId) override;						// 0x45b5d0
	virtual void	Closed() override;										// 0x45b6b0

	virtual void	SliderVal(int theId, double theVal) override;			// 0x45b580
	virtual void	CheckboxChecked(int theId, bool checked) override;		// 0x45bf20
};

// 0x45c2e0: the game is paused, and the box is made, put in the middle of the screen and added as dialog 2
OptionsDialog* MakeOptionsDialog();

}

#endif
