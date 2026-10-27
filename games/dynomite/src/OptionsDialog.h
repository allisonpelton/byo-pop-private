#ifndef __OPTIONSDIALOG_H__
#define __OPTIONSDIALOG_H__

// The options dialog (dialog 2; vtable 0x4fc4d8, ButtonListener 0x4fc4c8 at +0x58, SliderListener 0x4fc4c4 at +0x110,
// CheckboxListener 0x4fc4c0 at +0x114; 0x154 bytes; constructor 0x42ee20, destructor 0x42eca0): the music and sound
// sliders, the full screen and custom cursor checkboxes, the register button (only in an unregistered copy) beside the
// high scores button, and the links under them, each a button: the credits, and in the original "check for updates"
// (unless the property DontUpdate) and the partner links (NumOptionsLinks, OptionsLinkText%d, OptionsLinkURL%d). The
// footer is DONE (2002), which the app's ButtonDepress applies.
// port: neither the update link nor the partner links are ported (no online features; the user's decision of
// 2026-10-24 for BookWorm's, whose retail partner.xml has the same single "more games" link). The retail properties
// set DontUpdate and one partner link, so the port's dialog has one link where the original has two, and is 33
// pixels shorter.

#include "CheckboxListener.h"
#include "Compat2004.h"
#include "PtrList.h"
#include "SliderListener.h"
#include <string>

namespace Sexy
{

class Checkbox;
class DialogButton;
class FossilizedApp;
class Graphics;
class WidgetManager;

// one link, 0x3c bytes
struct OptionsLink
{
	std::string			mText;				// +0 the button's label
	std::string			mURL;				// +0x1c "GOTO:CREDITS", "GOTO:UPDATES" or an address
	DialogButton*		mButton;			// +0x38 id 100 on
};

class OptionsDialog : public Dialog2004, public SliderListener, public CheckboxListener
{
public:
	FossilizedApp*		mApp;				// +0x118
	Slider2004*			mMusicSlider;		// +0x11c id 0
	Slider2004*			mSfxSlider;			// +0x120 id 1
	Checkbox*			mFullscreenCheckbox;	// +0x124 id 0, checked when full screen
	Checkbox*			mCustomCursorsCheckbox;	// +0x128 id 0
	DialogButton*		mRegisterButton;	// +0x12c id 1
	DialogButton*		mHighScoresButton;	// +0x130 id 2
	PtrList<OptionsLink>	mLinks;			// +0x134
	int					mNextLinkId;		// +0x14c (100)
	bool				mShowRegister;		// +0x150 not the property NOREG (upper case: the retail file's NoReg is
											// another key, so it is always set)

	OptionsDialog(FossilizedApp* theApp);	// 0x42ee20
	virtual ~OptionsDialog();				// 0x42eca0

	virtual int			GetPreferredHeight(int theWidth) override;	// 0x42e5a0: 0x111 and 0x21 a link
	// 0x42e5c0: colour 3 is the buttons' too, with white for their colour 1
	virtual void		SetColor(int theIdx, const Color& theColor) override;
	virtual void		AddedToManager(WidgetManager* theWidgetManager) override;		// 0x42e670
	virtual void		RemovedFromManager(WidgetManager* theWidgetManager) override;	// 0x42e710
	// 0x42e7b0: the register and high scores buttons side by side (or the high scores one alone), the links under
	// them, the sliders and the checkboxes
	virtual void		Resize(int theX, int theY, int theWidth, int theHeight) override;
	virtual void		Draw(Graphics* g) override;	// 0x42fb70: the sliders' and the checkboxes' labels
	virtual void		SliderVal(int theId, double theVal) override;			// 0x42e970: the volumes
	// 0x42e9b0: where full screen is forced, clearing a checkbox (either: the id is not tested) says so and checks
	// the full screen one again
	virtual void		CheckboxChecked(int theId, bool checked) override;
	virtual void		ButtonPress(int theId) override;		// 0x42e9e0: the click
	// 0x42eb70: register, the high scores, and the links (GOTO:CREDITS, GOTO:UPDATES, else the address)
	virtual void		ButtonDepress(int theId) override;
	using Dialog2004::ButtonPress;
};

}

#endif
