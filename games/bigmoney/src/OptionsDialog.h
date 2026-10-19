#ifndef __OPTIONSDIALOG_H__
#define __OPTIONSDIALOG_H__

// OptionsDialog.cpp: modal options dialog (dialog 1): music / sound sliders, fullscreen and custom
// cursor checkboxes, Register / Scores / Help / Check Updates / Credits buttons and the partner
// "more games" links (properties NumOptionsLinks, OptionsLinkText%d, OptionsLinkURL%d).
// Original: class Sexy::OptionsDialog : public MoneyDialog, public SliderListener, public CheckboxListener,
// sizeof 0x198. Vtables: primary 0x4fb774 (60), ButtonListener 0x4fb754, SliderListener 0x4fb74c
// (subobject +0x158), CheckboxListener 0x4fb744 (subobject +0x15c). Offsets are absolute.
// Behaviour spec: re/bigmoney/analysis/dialogs.md (section 3).
// The Steam build is registered and build-unlocked (NoReg) and has DontUpdate, so Register and Check
// Updates are created and added but never laid out (0x0, invisible), as in the original. port(mods): when an
// installed mod has settings, the Register slot holds a MODS button (mModsButton).

#include "MoneyDialog.h"
#include "SliderListener.h"
#include "CheckboxListener.h"
#include <vector>

namespace Sexy
{

class WinBMApp;
class Graphics;
class WidgetManager;
class Slider12;
class Checkbox;
class DialogButton;

class OptionsDialog : public MoneyDialog, public SliderListener, public CheckboxListener
{
public:
	enum
	{
		MUSIC_SLIDER_ID = 0,
		SFX_SLIDER_ID = 1,
		FULLSCREEN_CHECKBOX_ID = 0,
		CUSTOM_CURSORS_CHECKBOX_ID = 0, // the original gives both checkboxes id 0
		REGISTER_BUTTON_ID = 1,
		SCORES_BUTTON_ID = 2,
		VISIT_POPCAP_BUTTON_ID = 3,     // handled in ButtonDepress, never created
		HELP_BUTTON_ID = 4,
		CHECK_UPDATES_BUTTON_ID = 5,
		CREDITS_BUTTON_ID = 6,
		LINK_BUTTON_ID_BASE = 100,      // link i has id 100+i (ButtonDepress accepts 100..199)
		ROW_PITCH = 34,
		BUTTON_HEIGHT = 36,
		BASE_PREFERRED_HEIGHT = 308     // + 34 per link (342 shipped)
	};

	// +0x158 SliderListener vptr, +0x15c CheckboxListener vptr
	WinBMApp*		mApp;                       // +0x160
	Slider12*		mMusicSlider;               // +0x164 id 0 (slidertrack / sliderthumb), value app->GetMusicVolume()
	Slider12*		mSfxSlider;                 // +0x168 id 1, value app->GetSfxVolume()
	Checkbox*		mFullscreenCheckbox;        // +0x16c id 0, checked = !app->mIsWindowed; WinBMApp reads it on 2001
	Checkbox*		mCustomCursorsCheckbox;     // +0x170 id 0, checked = app->mCustomCursorsEnabled; WinBMApp reads it on 2001
	DialogButton*	mRegisterButton;            // +0x174 id 1 REGISTER (laid out only when !app->mBuildUnlocked)
	DialogButton*	mScoresButton;              // +0x178 id 2 SCORES
	DialogButton*	mHelpButton;                // +0x17c id 4 HELP
	DialogButton*	mCheckUpdatesButton;        // +0x180 id 5 CHECK_UPDATES (laid out only without DontUpdate)
	DialogButton*	mCreditsButton;             // +0x184 id 6 CREDITS
	std::vector<DialogButton*> mLinkButtons;    // +0x188 ids 100+i, labels OptionsLinkText%d (VC7 vector: first +0x18c, last +0x190, end +0x194)
	// port(mods): an installed mod has settings: the Register button (never laid out in this build) becomes
	// "MODS" in its slot and opens the mod menu (docs/MOD_MENU_DESIGN.md 8.3)
	bool			mModsButton;

public:
	virtual void	SetColor(int theIdx, const Color& theColor) override;    // 0x415560 slot 19 (idx 3 also recolours the five buttons)
	virtual void	SliderVal(int theId, double theVal) override;            // 0x4155e0 SliderListener slot 0
	virtual void	CheckboxChecked(int theId, bool checked) override;       // 0x415620 CheckboxListener slot 0
	virtual void	ButtonPress(int theId) override;                         // 0x415660 ButtonListener slot 1 (buttonclick)
	virtual int		GetPreferredHeight(int theWidth) override;               // 0x4156b0 slot 57 (308 + 34*links)
	virtual void	AddedToManager(WidgetManager* theWidgetManager) override;  // 0x415770 slot 7
	virtual void	RemovedFromManager(WidgetManager* theWidgetManager) override; // 0x415830 slot 8
	virtual ~OptionsDialog();                                       // 0x415970 (scalar deleting dtor 0x416700 slot 0)
	virtual void	Resize(int theX, int theY, int theWidth, int theHeight) override; // 0x415ad0 slot 25
	virtual void	Draw(Graphics* g) override;                              // 0x415eb0 slot 13
	virtual void	ButtonDepress(int theId) override;                       // 0x416210 ButtonListener slot 2
	OptionsDialog(WinBMApp* theApp);                                // 0x416720

	using Dialog::ButtonPress;
	using Widget::Resize;
};

}

#endif
