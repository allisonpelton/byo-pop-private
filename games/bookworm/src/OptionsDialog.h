#ifndef __BOOKWORM_OPTIONSDIALOG_H__
#define __BOOKWORM_OPTIONSDIALOG_H__

// OptionsDialog.cpp: dialog 1 (DIALOG_OPTIONS). Slides in from the right edge (x 640 -> 240), music and
// sound sliders, five checkboxes, help / (register) / credits buttons, the partner links (properties
// NumOptionsLinks, OptionsLinkText%d, OptionsLinkURL%d) and the footer button (2001 -> the app ->
// ApplyAndClose, which slides it out again).
// Original: class Sexy::OptionsDialog : public MoneyDialog, public SliderListener, public CheckboxListener,
// sizeof 0x188 (new in BookWormApp::ShowOptions 0x4107c0). Vtables: primary 0x500b4c (53 slots),
// ButtonListener 0x500b2c (+0x58), SliderListener 0x500b24 (+0x120), CheckboxListener 0x500b1c (+0x124).
// Offsets are absolute. Behaviour spec: re/bookworm/analysis/dialogs.md section 3.
// The registered build (mBuildUnlocked) creates the register button but never adds or lays it out.

#include "MoneyDialog.h"
#include "SliderListener.h"
#include "CheckboxListener.h"
#include <list>
#include <vector>
#include <string>

namespace Sexy
{

class BookWormApp;
class Graphics;
class WidgetManager;
class Slider2004;
class Checkbox;
class DialogButton;

class OptionsDialog : public MoneyDialog, public SliderListener, public CheckboxListener
{
public:
	enum
	{
		HELP_BUTTON_ID = 33,
		REGISTER_BUTTON_ID = 34,        // only added when !mBuildUnlocked (never in the registered build)
		CREDITS_BUTTON_ID = 35,
		LINK_BUTTON_ID = 1036,          // every link button; ButtonDepress opens mLinkURLs[id % 1000 - 36]
		FULLSCREEN_CHECKBOX_ID = 37,
		CUSTOM_CURSORS_CHECKBOX_ID = 38,
		SHOW_HINTS_CHECKBOX_ID = 39,
		SHOW_DEFS_CHECKBOX_ID = 40,
		HARDWARE_ACCEL_CHECKBOX_ID = 41,
		MUSIC_SLIDER_ID = 42,
		SFX_SLIDER_ID = 43,

		SLIDE_BASE_X = 240,             // resting x; Update resizes to SLIDE_BASE_X + offset
		SLIDE_START_OFFSET = 240,       // written by the creator (app ShowOptions)
		SLIDE_CLOSE_OFFSET = -240,      // written by ApplyAndClose (only its sign matters)
		ROW_PITCH = 36,
		DEFAULT_DIALOG_HEIGHT = 330,    // OptionsDialogHeight default (partner.xml: 315)
		MAX_LINKS = 3
	};

	// +0x120 SliderListener vfptr, +0x124 CheckboxListener vfptr
	BookWormApp*	mApp2;                  // +0x128 app (all OptionsDialog code uses this copy)
	Slider2004*		mMusicSlider;           // +0x12c id 42, value app->GetMusicVolume()
	Slider2004*		mSfxSlider;             // +0x130 id 43, value app->GetSfxVolume()
	Checkbox*		mFullscreenCheckbox;    // +0x134 id 37, checked = !app->mIsWindowed
	Checkbox*		mCustomCursorsCheckbox; // +0x138 id 38, checked = app->mCustomCursorsEnabled
	Checkbox*		mShowHintsCheckbox;     // +0x13c id 39, checked = app->mShowHints
	Checkbox*		mShowDefsCheckbox;      // +0x140 id 40, checked = app->mShowDefs
	Checkbox*		m3DCheckbox;            // +0x144 id 41, checked = app->Is3DAccelerated() (always created)
	DialogButton*	mRegisterButton;        // +0x148 id 34, label GetString("RegisterLabel", GT_OPTIONS_REGISTER)
	DialogButton*	mHelpButton;            // +0x14c id 33 GT_OPTIONS_HELP
	DialogButton*	mCreditsButton;         // +0x150 id 35 GT_OPTIONS_CREDITS
	std::list<DialogButton*> mLinkButtons;  // +0x154 (head +0x158, size +0x15c) labels OptionsLinkText%d
	std::vector<std::string> mLinkURLs;     // +0x160 (first +0x164, last +0x168, end +0x16c) OptionsLinkURL%d
	int				mSlideOffset;           // +0x170 0; > 0 sliding in, < 0 sliding out, 0 at rest
	// +0x174 pad
	double			mSlidePhase;            // +0x178 += 0.055 per update while sliding
	bool			mClosing;               // +0x180 set by ApplyAndClose; Update kills the dialog at offset 0
	// +0x181 pad to 0x188

public:
	OptionsDialog(BookWormApp* theApp);                                 // 0x42fe90
	virtual ~OptionsDialog();                                           // 0x42fc90 body, 0x42fe60 slot 0

	virtual void	AddedToManager(WidgetManager* theWidgetManager);    // 0x42eeb0 slot 1
	virtual void	RemovedFromManager(WidgetManager* theWidgetManager);// 0x42efa0 slot 2
	virtual void	SetColor(int theIdx, const Color& theColor);        // 0x42ee20 slot 7 (idx 3 recolours the extra buttons)
	virtual void	Resize(int theX, int theY, int theWidth, int theHeight); // 0x42f090 slot 13
	virtual void	Draw(Graphics* g);                                  // 0x42f600 slot 16
	virtual void	Update();                                           // 0x42ebe0 slot 17 (slide in/out)
	virtual int		GetPreferredHeight(int theWidth);                   // 0x42f450 slot 50 (property + 36 * links)
	virtual void	ApplyAndClose();                                    // 0x42ed00 slot 52 (new virtual)

	virtual void	ButtonPress(int theId);                             // 0x42eba0 +0x58 slot 3
	virtual void	ButtonDepress(int theId);                           // 0x42fbe0 +0x58 slot 4
	virtual void	SliderVal(int theId, double theVal);                // 0x42eb40 +0x120 slot 0
	virtual void	CheckboxChecked(int theId, bool checked);           // 0x42fa50 +0x124 slot 0

	using Dialog2004::ButtonPress;
	using Widget::Resize;
};

}

#endif
