// ThunderButton.h: the game's own buttons, check boxes, slider and the blue frame (the original's ThunderButton.cpp).
// ThunderButton is the button of every screen and dialog: a DialogButton that blinks, plays a sound when the mouse comes
// over it and when it is pressed, and draws its label in one of the colours of SetButtonFontColor.

#ifndef __THUNDERBUTTON_H__
#define __THUNDERBUTTON_H__

#include "ButtonWidget.h"
#include "Checkbox.h"
#include "DialogButton.h"
#include "Slider.h"
#include "Widget.h"

#include <string>

namespace Sexy
{

class Image;

// The colour sets of the button font (the table at 0x6505b0): each is three colours, the first two multiplied into the
// font's first two layers and the third added to its third layer. A button's label uses its set, one step on when the
// mouse is over it or it is pressed and two when both. The check boxes use the last four.
enum ButtonFontColor
{
	BUTTON_FONT_COLOR_NONE = 0x11, // leave the font's colours as they are
	BUTTON_FONT_COLOR_CHECKBOX = 13, // 13 is a check box, 14 with the mouse over it, 15 checked and with the mouse over it, 16 both
	BUTTON_FONT_COLOR_CHECKBOX_OVER = 14,
	BUTTON_FONT_COLOR_CHECKBOX_DOWN = 15,
	BUTTON_FONT_COLOR_CHECKBOX_DOWN_OVER = 16,
	NUM_BUTTON_FONT_COLORS = 17
};

void SetButtonFontColor(int theColor); // 0x40b570: colours FONT_BUTTON's layers with set theColor
void DrawScreenFrame(Graphics* g);	   // 0x405020: the four pieces of the screen's frame

class ThunderButton : public DialogButton
{
public:
	ThunderButton(Image* theComponentImage, int theId, ButtonListener* theListener); // 0x49ba30

	void Blink(int theTimes, bool theFlashFirst); // 0x496c00: makes the button blink theTimes times
	void StopBlink();							  // 0x496c40

	void Draw(Graphics* g) override;				  // 0x49d440
	void Update() override;						  // 0x496da0
	void MouseEnter() override;					  // 0x496c50
	void MouseDown(int x, int y, int theClickCount) override; // 0x496d30
	void MouseUp(int x, int y, int theClickCount) override;	  // 0x496d70

	using DialogButton::MouseDown;
	using DialogButton::MouseUp;

	int mDownSound;				// +0x138: the sample played when the button is pressed (-1 for none)
	int mHoverSound;			// +0x13c: the sound played when the mouse comes over it (-1 for none)
	float mHoverSoundPitch;		// +0x140: how many semitones the hover sound is raised (the main menu's buttons go up a major scale)
	int mLastHoverTime;			// +0x144: the update count of the last hover sound, so a quick pass makes only one
	bool mDrawEnabled;			// +0x148: false hides the whole button
	int mBlinkCount;			// +0x14c: counts down while it blinks, 0 when it does not
	bool mRightClicked;			// +0x150: the last release was the right button's
	bool mBlinkFades;			// +0x151: the blink fades in and out instead of flashing
	int mBlinkPeriod;			// +0x154: the updates one blink takes
	bool mStopBlinkOnHover;		// +0x158
	bool mDarkenWhenPressed;	// +0x159: draws it again darker while it is pressed
	int mFontColor;				// +0x15c: a ButtonFontColor
};

// A button that is only a picture (a symbol) tinted yellow, green when the mouse is over it (0x496de0)
class SimpleButton : public ButtonWidget
{
public:
	SimpleButton(Image* theImage, int theId, ButtonListener* theListener); // 0x496de0

	void Draw(Graphics* g) override;			  // 0x496e90
	void MouseDown(int x, int y, int theClickCount) override; // 0x496f60
	void MouseUp(int x, int y) override;					  // 0x496fa0
	void MouseEnter() override;					  // 0x496fd0
	void MouseLeave() override;					  // 0x496ff0

	using ButtonWidget::MouseDown;
	using ButtonWidget::MouseUp;

	Image* mImage;		// +0x120
	int mColor;			// +0x124: the picture's colour
	int mOverColor;		// +0x128: with the mouse over it
	int mDownColor;		// +0x12c: pressed
};

class ThunderCheckbox : public Checkbox
{
public:
	ThunderCheckbox(Image* theUncheckedImage, Image* theCheckedImage, int theId, CheckboxListener* theListener); // 0x49e7c0

	void Draw(Graphics* g) override;			  // 0x49bb10
	void Update() override;						  // 0x497130
	void MouseEnter() override;					  // 0x497010
	void MouseLeave() override;					  // 0x497050
	void MouseDown(int x, int y, int theClickCount) override; // 0x4970b0
	void MouseUp(int x, int y, int theClickCount) override;	  // 0x497100

	using Checkbox::MouseDown;
	using Checkbox::MouseUp;

	Font* mFont;				// +0xec
	std::string mLabel;			// +0xf0
	int mClickSound;			// +0x10c: the sample played when it is pressed (-1 for none)
	bool mStaysChecked;			// +0x110: once checked, a click does not uncheck it (a choice among several)
	int mTextOffsetX;			// +0x114
	int mTextOffsetY;			// +0x118
	int mLabelJustify;			// +0x11c: 1 centres the label and gives the pressed look of a button
	Rect mOverUncheckedRect;	// +0x120: the part of the picture drawn with the mouse over it, unchecked
	Rect mOverCheckedRect;		// +0x130: checked
	double mOverAlpha;			// +0x140: how far the over picture has faded in
	double mOverAlphaSpeed;		// +0x148: how fast it fades out
	double mOverAlphaFadeInSpeed; // +0x150
};

class ThunderSlider : public Slider
{
public:
	ThunderSlider(const std::string& theLabel, int theId, SliderListener* theListener); // 0x4ab510

	void Draw(Graphics* g) override; // 0x49bd70

	std::string mLabel; // +0xb8
};

// The blue frame of a dialog's picture: draws IMAGE_DLG_BLUEFRAME over its whole area and lets the mouse through
class ThunderFrame : public Widget
{
public:
	ThunderFrame(); // 0x497200

	void Draw(Graphics* g) override; // 0x49beb0
};

} // namespace Sexy

#endif
