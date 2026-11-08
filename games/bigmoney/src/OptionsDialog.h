#ifndef __OPTIONSDIALOG_H__
#define __OPTIONSDIALOG_H__

// OptionsDialog.cpp: modal options dialog (dialog 1): music / sound sliders, fullscreen and custom
// cursor checkboxes, Scores / Help / Credits buttons (and Mods, below). The original also has the
// partner "more games" links (properties NumOptionsLinks, OptionsLinkText%d, OptionsLinkURL%d); the port
// leaves them out (the user's decision, 2026-10-24) and lays the dialog out as with none.
// Original: class Sexy::OptionsDialog : public MoneyDialog, public SliderListener, public CheckboxListener,
// sizeof 0x198. Vtables: primary 0x4fb774 (60), ButtonListener 0x4fb754, SliderListener 0x4fb74c
// (subobject +0x158), CheckboxListener 0x4fb744 (subobject +0x15c). Offsets are absolute.
// Behaviour spec: re/bigmoney/analysis/dialogs.md (section 3).
// The registered Steam original (NoReg, DontUpdate) never lays out its Register and Check Updates buttons, and the
// port does not make them (GAME_NOTES.md, "Registration code"). port(mods): when an installed mod has settings, the
// slot where Register would be holds a MODS button (mModsButton).

#include "MoneyDialog.h"
#include "SliderListener.h"
#include "CheckboxListener.h"

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
		MODS_BUTTON_ID = 1,             // the original's Register button had this id
		SCORES_BUTTON_ID = 2,
		VISIT_POPCAP_BUTTON_ID = 3,     // never created; the original's ButtonDepress opens a PopCap page (not ported)
		HELP_BUTTON_ID = 4,
		CREDITS_BUTTON_ID = 6,
		LINK_BUTTON_ID_BASE = 100,      // the original's partner link i has id 100+i (the links are not ported)
		ROW_PITCH = 34,
		BUTTON_HEIGHT = 36,
		BASE_PREFERRED_HEIGHT = 308     // the original adds 34 per partner link (342 shipped); the port has none
	};

	// +0x158 SliderListener vptr, +0x15c CheckboxListener vptr
	WinBMApp*		mApp;                       // +0x160
	Slider12*		mMusicSlider;               // +0x164 id 0 (slidertrack / sliderthumb), value app->GetMusicVolume()
	Slider12*		mSfxSlider;                 // +0x168 id 1, value app->GetSfxVolume()
	Checkbox*		mFullscreenCheckbox;        // +0x16c id 0, checked = !app->mIsWindowed; WinBMApp reads it on 2001
	Checkbox*		mCustomCursorsCheckbox;     // +0x170 id 0, checked = app->mCustomCursorsEnabled; WinBMApp reads it on 2001
	DialogButton*	mScoresButton;              // +0x178 id 2 SCORES
	DialogButton*	mHelpButton;                // +0x17c id 4 HELP
	DialogButton*	mCreditsButton;             // +0x184 id 6 CREDITS
	// +0x188 std::vector<DialogButton*> mLinkButtons (ids 100+i): the partner links, not ported
	// port(mods): an installed mod has settings: a "MODS" button opens the mod menu (docs/MOD_MENU_DESIGN.md 8.3);
	// NULL without one
	DialogButton*	mModsButton;

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
