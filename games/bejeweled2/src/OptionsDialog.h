#ifndef __OPTIONSDIALOG_H__
#define __OPTIONSDIALOG_H__

// OptionsDialog.h: the options (Sexy::OptionsDialog, 0x5cae38-0x5cd63c; 0x1d8 bytes). A PodDialog with two sliders (music
// and sound effects), five checkboxes (full screen, hardware acceleration, custom cursors, high resolution, random
// backdrops) and the buttons Done (or Back to Game), How To Play and Main Menu. The checkboxes that would leave the
// display in a state the game does not like ask for a confirmation first (dialog 3, FinishOptionVerify answers it).
// The offsets in the comments are the original's, from the start of the object.
//
// Not ported (registration, online and outbound code): the Register button (+0x1b4, id 11), the row of buttons that open
// PopCap's pages (a vector at +0x1bc, ids 100 and up, and id 13), the Update Check button (+0x1cc, id 14) and the
// Automatic Update checkbox (+0x1a4, id 3, with its confirmation, type 6). The layout is the one the original uses for
// a registered copy whose update checks are off, so nothing is left over where those controls were.

#include "PodDialog.h"
#include "SliderListener.h"
#include "CheckboxListener.h"

namespace Sexy
{

class Slider;
class Checkbox;
class Graphics;
class WinDMApp;

class OptionsDialog : public PodDialog, public SliderListener, public CheckboxListener
{
public:
	// the ids of the checkboxes
	enum
	{
		CHECKBOX_FULLSCREEN = 0,
		CHECKBOX_CURSORS = 1,
		CHECKBOX_ACCELERATION = 2,
		CHECKBOX_UPDATE = 3,        // not ported
		CHECKBOX_HIGH_RES = 4,
		CHECKBOX_BACKDROPS = 5
	};

	// the ids of the buttons (the Done button is the dialog's yes button, 1000)
	enum
	{
		BUTTON_HELP = 10,
		BUTTON_REGISTER = 11,       // not ported
		BUTTON_MAIN_MENU = 12,
		BUTTON_LINK = 13,           // not ported
		BUTTON_UPDATE_CHECK = 14    // not ported
	};

	// what the confirmation dialog (id 3) that is open asks; FinishOptionVerify does it when the answer is yes
	enum
	{
		VERIFY_ACCELERATION_ON = 0,
		VERIFY_ACCELERATION_OFF = 1,
		VERIFY_HIGH_RES_ON = 2,
		VERIFY_HIGH_RES_OFF = 3,
		VERIFY_WINDOWED = 4,        // windowed mode needs the low resolution
		VERIFY_FULLSCREEN = 5,      // high resolution needs full screen
		VERIFY_UPDATE_ON = 6,       // not ported
		VERIFY_BACKDROPS_RANDOM = 7
	};

	// +0x184 the SliderListener, +0x188 the CheckboxListener, +0x18c the app (the dialog has its own copy: PodDialog::mApp)
	Slider*			mMusicSlider;           // +0x190 id 0
	Slider*			mSfxSlider;             // +0x194 id 1
	Checkbox*		mFullscreenCheckbox;    // +0x198
	Checkbox*		m3DCheckbox;            // +0x19c the hardware acceleration
	Checkbox*		mCursorsCheckbox;       // +0x1a0
	// +0x1a4 the Automatic Update checkbox (not ported)
	Checkbox*		mHighResCheckbox;       // +0x1a8
	Checkbox*		mBackdropsCheckbox;     // +0x1ac
	PodDialogButton* mHelpButton;           // +0x1b0
	// +0x1b4 the Register button (not ported)
	PodDialogButton* mMainMenuButton;       // +0x1b8
	// +0x1bc the buttons that open PopCap's pages, +0x1cc the Update Check button (not ported)
	bool			mInVerify;              // +0x1d0 FinishOptionVerify is setting the checkboxes (CheckboxChecked leaves them alone)
	int				mVerifyType;            // +0x1d4 VERIFY_...

public:
	OptionsDialog(WinDMApp* theApp);                // 0x5ccaf0
	virtual ~OptionsDialog();                       // 0x5cb5e7

	virtual int		GetPreferredHeight(int theWidth) override;                          // 0x5cae38
	virtual void	SliderVal(int theId, double theVal) override;                       // 0x5cae4d
	virtual void	CheckboxChecked(int theId, bool checked) override;                  // 0x5cbcb6
	virtual void	ButtonDepress(int theId) override;                                  // 0x5cc563
	virtual void	AddedToManager(WidgetManager* theManager) override;                 // 0x5cb09c
	virtual void	RemovedFromManager(WidgetManager* theManager) override;             // 0x5cb189
	virtual void	Draw(Graphics* g) override;                                         // 0x5cb7ae
	virtual void	Resize(int theX, int theY, int theWidth, int theHeight) override;   // 0x5cd63c

	void			DrawCheckbox(Graphics* g, Checkbox* theCheckbox, const SexyString& theLine1, const SexyString& theLine2); // 0x5cb276
	void			CheckFullscreenHighres();                   // 0x5cbbd0
	void			FinishOptionVerify(bool theYes);            // 0x5cc95a
};

}

#endif
