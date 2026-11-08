#ifndef __OPTIONSDIALOG_H__
#define __OPTIONSDIALOG_H__

// OptionsDialog.cpp: modal options dialog (id 2): music / sound sliders, fullscreen and custom
// cursor checkboxes and the Help / High Scores buttons (the original's Register button, update-check button and
// "more games" links are taken out: GAME_NOTES.md, "Registration code and the More Games button taken out").
// Original: class Sexy::OptionsDialog : public Dialog, public SliderListener, public CheckboxListener,
// sizeof 0x194. Vtables: primary 0x506fd4, ButtonListener 0x506fb0, SliderListener 0x506fa8
// (subobject +0x150), CheckboxListener 0x506fa0 (subobject +0x154). Offsets are absolute;
// Ghidra's OptionsDialog_data.offset_N is absolute N+0x158.
// Behaviour spec: games/bejeweled/docs/GAME_NOTES.md (OptionsDialog section).

#include "Dialog.h"
#include "SliderListener.h"
#include "CheckboxListener.h"

namespace Sexy
{

class WinDMApp;
class Graphics;
class WidgetManager;
class Slider;
class Checkbox;
class DialogButton;
class Image;

class OptionsDialog : public Dialog, public SliderListener, public CheckboxListener
{
public:
	enum
	{
		MUSIC_SLIDER_ID = 0,
		SFX_SLIDER_ID = 1,
		FULLSCREEN_CHECKBOX_ID = 0,
		CUSTOM_CURSORS_CHECKBOX_ID = 1,
		HELP_BUTTON_ID = 0,
		HIGH_SCORES_BUTTON_ID = 2,
		EXTRAS_BUTTON_ID = 5,           // port(variant): Extras (games/bejeweled/docs/CODE_MODS_DESIGN.md 2.6, 8)
		MODS_BUTTON_ID = 6,             // port(mods): Mods
		PREFERRED_HEIGHT = 290,         // the original's, with its one "more games" link row (256 + 34)
		ROW_Y = 178                     // the Help / High Scores row, from the dialog's top (the original's 166 + 12)
	};

	// +0x150 SliderListener vptr, +0x154 CheckboxListener vptr
	WinDMApp*		mApp;                       // +0x158
	Slider*			mMusicVolumeSlider;         // +0x15c slider 0 (slider-track / slider-thumb)
	Slider*			mSfxVolumeSlider;           // +0x160 slider 1
	Checkbox*		mFullscreenCheckbox;        // +0x164 checkbox 0 (initially !app->mIsWindowed); read by app on 2002
	Checkbox*		mCustomCursorsCheckbox;     // +0x168 checkbox 1 (initially app->mCustomCursorsEnabled); read by app on 2002
	DialogButton*	mHelpButton;                // +0x16c id 0 OPTIONSDLG_HELP
	DialogButton*	mHighScoresButton;          // +0x174 id 2 OPTIONSDLG_HIGH_SCORES
	Image*			mCheckboxCheckedImage;      // +0x18c CopyImage(checkbox1, Rect(0,0,31,32)), owned
	Image*			mCheckboxUncheckedImage;    // +0x190 CopyImage(checkbox1, Rect(31,0,31,32)), owned
	// port(mods): Mods, a third button in the row of Help and High Scores, while an enabled mod has settings: it opens
	// the mod menu over this dialog (docs/MOD_MENU_DESIGN.md 8.2). NULL otherwise: the row is the original's two.
	DialogButton*	mModsButton;
	// port(variant): Extras in that place instead while a feature mod adds to it (WinDMApp::HasExtras; ExtrasDialog.h,
	// which then has the Mods button). NULL otherwise.
	DialogButton*	mExtrasButton;

public:
	virtual void	SliderVal(int theId, double theVal);            // 0x4d1227 SliderListener slot 0
	virtual void	CheckboxChecked(int theId, bool checked);       // 0x4d1261 CheckboxListener slot 0
	virtual void	ButtonPress(int theId);                         // 0x4d1297 ButtonListener slot 1 (menuclick)
	virtual int		GetPreferredHeight(int theWidth);               // 0x4d1424 slot 73 (PREFERRED_HEIGHT)
	virtual void	AddedToManager(WidgetManager* theWidgetManager);  // 0x4d143a slot 20
	virtual void	RemovedFromManager(WidgetManager* theWidgetManager); // 0x4d14f3 slot 21
	virtual void	Resize(int theX, int theY, int theWidth, int theHeight); // 0x4d15ac slot 40
	virtual ~OptionsDialog();                                       // 0x4d18f6 (scalar deleting dtor 0x4d1a38)
	virtual void	Draw(Graphics* g);                              // 0x4d1a8f slot 26
	virtual void	ButtonDepress(int theId);                       // 0x4d1c58 ButtonListener slot 2
	OptionsDialog(WinDMApp* theApp);                                // 0x4d1fda

	using Dialog::ButtonPress;
	using Widget::Resize;
};

}

#endif
