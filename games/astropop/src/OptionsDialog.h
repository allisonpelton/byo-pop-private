#ifndef __ASTROPOP_OPTIONSDIALOG_H__
#define __ASTROPOP_OPTIONSDIALOG_H__

// OptionsDialog: the options box (id 0x66; original class of 0x154 bytes, constructor 0x4850a0, destructor body 0x4822f0,
// vftable 0x56aaf8, with the slider listener at +0x110 and the checkbox listener at +0x114). Two sliders (music and sounds),
// five boxes to tick, and the buttons: the CLOSE of the box (its footer button; "Resume Game" in a game), the way out of
// the game, the hints, the help and the credits. Offsets in the comments are the original's.
// port: the original also has a REGISTER button (+0x13c, id 7) and an Update button (+0x140, id 8) in it; they are left out
// with the rest of the registration and update code, and the layout is the one the original takes without them.

#include "Text2004.h"
#include "Dialog.h"
#include "SliderListener.h"
#include "CheckboxListener.h"

namespace Sexy
{

class Slider;
class Checkbox;
class DialogButton;

class OptionsDialog : public Text2004<Dialog>, public SliderListener, public CheckboxListener
{
public:
	bool			mFromGame;          // +0x119 the box was opened in a game (the button that leaves it is there)
	int				mColourSet;         // +0x11c the colour set of the box
	Slider*			mMusicSlider;       // +0x120
	Slider*			mSoundSlider;       // +0x124
	Checkbox*		mFullscreenCheck;   // +0x128
	Checkbox*		mCursorCheck;       // +0x12c the game's own cursors
	Checkbox*		mAccelCheck;        // +0x130 hardware acceleration (3D)
	Checkbox*		mHintsCheck;        // +0x134
	Checkbox*		mColorblindCheck;   // +0x138 the overlays on the bricks
	DialogButton*	mLeaveButton;       // +0x144 "Main Menu" ("Quit Mission" in a demonstration), id 0xfa8
	DialogButton*	mHintsResetButton;  // +0x148 id 9
	DialogButton*	mHelpButton;        // +0x14c id 10
	DialogButton*	mCreditsButton;     // +0x150 id 11

public:
	OptionsDialog(bool theHints, int theColourSet, bool theFromGame);       // 0x4850a0
	virtual ~OptionsDialog();                                               // 0x483e20 (body 0x4822f0)

	virtual void	AddedToManager(WidgetManager* theManager) override;     // 0x482ac0
	virtual void	RemovedFromManager(WidgetManager* theManager) override; // 0x482bd0
	virtual void	Resize(int theX, int theY, int theWidth, int theHeight) override;   // 0x4826d0
	virtual void	Draw(Graphics* g) override;                             // 0x485d40
	virtual int		GetPreferredHeight(int theWidth) override;              // 0x482a80
	virtual void	ButtonDepress(int theId) override;                      // 0x486350

	virtual void	SliderVal(int theId, double theVal) override;           // 0x482d10
	virtual void	CheckboxChecked(int theId, bool theChecked) override;   // 0x4860e0

private:
	DialogButton*	MakeButton(int theId, const char* theLabel);            // a button of the box's picture
	void			DrawCheckLabel(Graphics* g, const std::string& theText, Checkbox* theCheck);   // 0x482cc0
};

}

#endif
