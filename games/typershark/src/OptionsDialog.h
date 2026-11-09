#ifndef __TYPERSHARK_OPTIONSDIALOG_H__
#define __TYPERSHARK_OPTIONSDIALOG_H__

// OptionsDialog.cpp: "OPTIONS" (dialog 0x30a, made by WinWSApp::ShowOptions, which sizes it to 420 x 474): the music and
// sound volume sliders, five check boxes (full screen, effects, custom cursors, space bar pauses the game, hardware
// acceleration), the Credits button and the footer button (id 0x30a + 2000, which the app's handler reads the check boxes
// at: the dialog itself applies nothing). The Credits button opens a plain dialog 0x30c with the credits.
// Original: class Sexy::OptionsDialog : public Dialog, public SliderListener, public CheckboxListener, vftable 0x52dd7c
// (52 slots), ButtonListener vftable 0x52dd60 (+0x58), SliderListener vftable 0x52dd58 (+0x110), CheckboxListener
// vftable 0x52dd50 (+0x114), sizeof 0x154 (operator new in 0x414150), constructor 0x43bf10, destructor body 0x4384f0
// (deleting destructor 0x439150). The class has its own app pointer at +0x118 (it is not a MoneyDialog).
//
// The original also had a Register button (id 1, +0x128) and a partner link button (id 2, +0x12c) that shared the Credits
// button's row: removed (registration, links out of the game). The Credits button takes the whole row in Resize.

#include "compat2004/Compat2004.h"
#include "SliderListener.h"
#include "CheckboxListener.h"

namespace Sexy
{

class WinWSApp;
class Graphics;
class WidgetManager;
class Checkbox;
class DialogButton;
class MemoryImage;
class SoundSlider;

class OptionsDialog : public Dialog2004, public SliderListener, public CheckboxListener
{
public:
	enum
	{
		CREDITS_BUTTON_ID = 4,
		HARDWARE_CHECKBOX_ID = 0x51615,			// the other check boxes have id 0
		MUSIC_SLIDER_ID = 0,
		SFX_SLIDER_ID = 1
	};

	// +0x110 SliderListener vfptr, +0x114 CheckboxListener vfptr
	WinWSApp*				mApp;					// +0x118 constructor argument
	Slider2004*				mMusicSlider;			// +0x11c id 0, value app->GetMusicVolume()
	SoundSlider*			mSfxSlider;				// +0x120 id 1, value app->GetSfxVolume(); releasing it plays sample 8
	DialogButton*			mField124;				// +0x124 never assigned by the original (a button its code still adds, removes and recolours: probably a removed help button)
	// +0x128 the Register button and +0x12c the partner link button: removed (see above)
	DialogButton*			mCreditsButton;			// +0x130 id 4, label text 0x52e890
	MemoryImage*			mCheckedImage;			// +0x134 24 x 25: the right half of IMAGE_CHECKED
	MemoryImage*			mUncheckedImage;		// +0x138 24 x 25: the left half of IMAGE_CHECKED
	bool					mFlag13c;				// +0x13c set to true by the constructor; nothing reads it
	Checkbox*				mFullScreenCheckbox;	// +0x140 id 0, checked = !app->mIsWindowed ("Fullscreen")
	Checkbox*				mEffectsCheckbox;		// +0x144 id 0, checked = app->mFastCPU ("Enable Fx (may slow down game)")
	Checkbox*				mSpacePausesCheckbox;	// +0x148 id 0, checked = the user's mSpacePauses; disabled while the typing tutor exists ("Space bar pauses game")
	Checkbox*				mCursorsCheckbox;		// +0x14c id 0, checked = app->mCustomCursorsEnabled ("Custom Cursors")
	Checkbox*				m3DCheckbox;			// +0x150 id 0x51615, checked = app->Is3DAccelerated() ("Hardware Acceleration")

public:
	OptionsDialog(WinWSApp* theApp);														// 0x43bf10
	virtual ~OptionsDialog() override;														// 0x4384f0 (deleting 0x439150), slot 0

	virtual void			AddedToManager(WidgetManager* theWidgetManager) override;		// 0x4386d0 slot 1
	virtual void			RemovedFromManager(WidgetManager* theWidgetManager) override;	// 0x438790 slot 2
	virtual void			SetColor(int theIdx, const Color& theColor) override;			// 0x438660 slot 7 (index 3 goes to the extra buttons too)
	virtual void			Resize(int theX, int theY, int theWidth, int theHeight) override;	// 0x438860 slot 13
	virtual void			Draw(Graphics* g) override;										// 0x43c8f0 slot 16
	virtual int				GetPreferredHeight(int theWidth) override;						// 0x438650 slot 50 (a constant, 438)

	virtual void			ButtonPress(int theId) override;								// 0x438a80 ButtonListener (+0x58) slot 3
	virtual void			ButtonDepress(int theId) override;								// 0x43cd90 ButtonListener (+0x58) slot 4
	virtual void			SliderVal(int theId, double theVal) override;					// 0x438a40 SliderListener (+0x110) slot 0
	virtual void			CheckboxChecked(int theId, bool checked) override;				// 0x43cc90 CheckboxListener (+0x114) slot 0

	using Dialog2004::ButtonPress;
	using Widget::Resize;
};

}

#endif
