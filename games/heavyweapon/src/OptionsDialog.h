#ifndef __HEAVYWEAPON_OPTIONSDIALOG_H__
#define __HEAVYWEAPON_OPTIONSDIALOG_H__

// OptionsDialog: the options box: sliders for the sound effects, the music and the particles; checkboxes for full screen, hardware
// acceleration, custom cursors and hints; and DONE, HELP and CREDITS buttons. DONE closes the box and applies the full screen
// and acceleration choices. Original: a Dialog of 0x158 bytes with a ButtonListener at +0x58, a SliderListener at +0x114 and
// a CheckboxListener at +0x118, constructor 0x42a540, vtable 0x52e928 (button listener 0x52e90c, slider listener 0x52e908,
// checkbox listener 0x52e904), destructor body 0x429d30 (deleting 0x42a510).
// The original also had a REGISTER button (+0x148, id 9), an UPDATE button (+0x14c, id 10) and an "Auto Updates" checkbox
// (+0x140, id 0x14), each there only while the game was unregistered or updatable; they are gone, and the box is the
// original's own layout for a registered copy with no update check (see Resize).

#include "Compat2004.h"
#include "ButtonListener.h"
#include "CheckboxListener.h"
#include "SliderListener.h"

namespace Sexy
{

class BracketButton;
class Checkbox;
class HeavyWeaponApp;
class Slider2004;

class OptionsDialog : public Dialog2004, public SliderListener, public CheckboxListener
{
public:
	enum
	{
		DIALOG_ID = 7,				// HeavyWeaponApp::DIALOG_OPTIONS
		BUTTON_DONE = 8,
		BUTTON_HELP = 0xb,
		BUTTON_CREDITS = 0xc,
		SLIDER_MUSIC = 0xd,
		SLIDER_SFX = 0xe,
		SLIDER_PARTICLES = 0xf,
		CHECKBOX_FULL_SCREEN = 0x10,
		CHECKBOX_ACCELERATION = 0x11,
		CHECKBOX_CURSORS = 0x12,
		CHECKBOX_HINTS = 0x13,
		BUTTON_ACCELERATION_YES = 0x22,		// the answers of the box that asks about hardware acceleration (YES leaves the
		BUTTON_ACCELERATION_NO = 0x21		// choice made; NO puts the checkbox back); both close the box, which is dialog 0x20
	};

	HeavyWeaponApp*	mApp;						// +0x120
	Slider2004*		mSfxSlider;					// +0x124 id 0xe "Sound FX"
	Slider2004*		mMusicSlider;				// +0x128 id 0xd "Music"
	Slider2004*		mParticlesSlider;			// +0x12c id 0xf "Detail": 0 turns the particles and the background animation off
	Checkbox*		mWindowedCheckbox;			// +0x130 id 0x10 "Full Screen Mode" (checked while the game is not windowed)
	Checkbox*		mAccelerationCheckbox;		// +0x134 id 0x11 "Hardware Acceleration"
	Checkbox*		mCursorsCheckbox;			// +0x138 id 0x12 "Use Custom Cursors"
	Checkbox*		mHintsCheckbox;				// +0x13c id 0x13 "Enable Hints"
	BracketButton*	mDoneButton;				// +0x144 id 8
	BracketButton*	mHelpButton;				// +0x150 id 0xb
	BracketButton*	mCreditsButton;				// +0x154 id 0xc

public:
	OptionsDialog(HeavyWeaponApp* theApp);															// 0x42a540
	virtual ~OptionsDialog();																		// 0x42a510 (body 0x429d30)

	virtual void	AddedToManager(WidgetManager* theManager) override;							// 0x429f50 slot 1
	virtual void	RemovedFromManager(WidgetManager* theManager) override;						// 0x429e70 slot 2
	virtual void	Resize(int theX, int theY, int theWidth, int theHeight) override;				// 0x42a030 slot 13
	virtual void	Draw(Graphics* g) override;													// 0x42b2f0 slot 16
	virtual void	ButtonPress(int theId) override;												// 0x42a340 listener slot 4
	virtual void	ButtonDepress(int theId) override;												// 0x42a360 listener slot 5
	virtual void	SliderVal(int theId, double theVal) override;									// 0x42a2c0 slider listener slot 0
	virtual void	CheckboxChecked(int theId, bool checked) override;								// 0x42ade0 checkbox listener slot 0

	using Dialog::Resize;
	using Dialog2004::ButtonPress;
};

}

#endif
