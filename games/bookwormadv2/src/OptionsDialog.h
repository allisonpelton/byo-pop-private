// Sexy::OptionsDialog (0x198 bytes, constructor 0x4842e0, vtables 0x7afcac, 0x7afe00 (its ButtonListener part at
// +0x88), 0x7afe20 (SliderListener, +0x168) and 0x7afe28 (CheckboxListener, +0x16c); code 0x4842e0-0x4853d0): the
// options. A GameDialog (dialog id 1, GameApp::DIALOG_OPTIONS) under the options' banner with a footer button
// ("Continue Game" over a battle, else "Close"), two sliders (the music's and the sound effects' volume) and five
// checkboxes: full screen, custom cursors, 3-D acceleration, the tiles' letter values and no tooltips; and a
// "Credits" button. The sliders act at once; the checkboxes are read when the dialog is done
// (GameApp::CloseOptionsDialog, 0x4457b0).
#pragma once

#include "CheckboxListener.h"
#include "GameDialog.h"
#include "SliderListener.h"

namespace Sexy {

class Checkbox;
class ButtonListener;
class DialogButton;
class Slider;

// 0x49f2a0: a DialogButton of id 1 on IMAGE_DIALOG_BUTTON, one cel wide, with its label, in the game's look (0x49f610,
// the default font)
DialogButton* MakeDialogButton(ButtonListener* theListener, const std::string& theLabel);

// 0x49f7b0: a checkbox of the given id on IMAGE_DIALOG_CHECKBOX (the left half the box unticked, the right half ticked)
Checkbox* MakeCheckbox(int theId, CheckboxListener* theListener);

class OptionsDialog : public GameDialog, public SliderListener, public CheckboxListener {
public:
	// the widgets' ids (the sliders' 0 and 1, the checkboxes' and the Credits button's)
	enum { ID_MUSIC = 0, ID_SOUND = 1, ID_FULLSCREEN = 2, ID_CURSORS = 3, ID_3D = 4, ID_LETTER_VALUES = 5, ID_CREDITS = 6,
	       ID_NO_TOOLTIPS = 7 };

	Slider* mMusicSlider;         // +0x170
	Slider* mSoundSlider;         // +0x174
	Checkbox* mFullscreenBox;     // +0x178: ticked while the game is not windowed
	Checkbox* mCursorsBox;        // +0x17c
	Checkbox* m3DBox;             // +0x180
	Checkbox* mLetterValuesBox;   // +0x184: the profile's EnablePips
	Checkbox* mNoTooltipsBox;     // +0x188: the profile's DisableTips
	DialogButton* mCreditsButton; // +0x18c
	bool mHasBoard;               // +0x190: over a battle
	int mExtraHeight;             // +0x194: what the controls add to the dialog's height (200 until GetPreferredHeight)

	explicit OptionsDialog(bool theHasBoard);  // 0x4842e0
	~OptionsDialog() override;                 // 0x484780

	// Dialog
	int GetPreferredHeight(int theWidth) override;                      // 0x484a80
	void AddedToManager(WidgetManager* theWidgetManager) override;      // 0x484b30
	void RemovedFromManager(WidgetManager* theWidgetManager) override;  // 0x484bf0
	void Resize(int theX, int theY, int theWidth, int theHeight) override;  // 0x484890
	void Draw(Graphics* g) override;                                    // 0x484cb0
	// ButtonListener: the click's sound; the Credits button (the footer button is the Dialog's)
	void ButtonPress(int theId) override;                               // 0x4853b0
	void ButtonDepress(int theId) override;                             // 0x4853d0

	// SliderListener (0x485050): the volumes follow the sliders
	void SliderVal(int theId, double theVal) override;
	// CheckboxListener (0x4850c0): the ones that are not allowed (no windowed mode, no 3-D) say so
	void CheckboxChecked(int theId, bool checked) override;

private:
	// the Dialog's text area (0x4bf230, 0x4bf270, 0x4bf240): where the controls start
	int ContentLeft() const;
	int ContentWidth() const;
	int ContentTop() const;
	void DrawBoxLabel(Graphics* g, const SexyString& theLabel, Checkbox* theBox);
};

}  // namespace Sexy
