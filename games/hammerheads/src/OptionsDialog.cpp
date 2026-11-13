#include "OptionsDialog.h"

#include "Application.h"
#include "Checkbox.h"
#include "Color.h"
#include "CreditsDialog.h"
#include "DialogButton.h"
#include "Font.h"
#include "GameScreen.h"
#include "Graphics.h"
#include "Image.h"
#include "Insets.h"
#include "MainMenu.h"
#include "NcButton.h"
#include "Res.h"
#include "Slider.h"
#include "UserManager.h"
#include "WidgetManager.h"

using namespace Sexy;

namespace
{

// The box is this wide and tall
const int DIALOG_WIDTH = 0x1c2;
const int DIALOG_HEIGHT = 0x244;

// The room around the page: left, top, right and bottom
const Insets CONTENT_INSETS(0x3c, 0x32, 0x3c, 0x32);
const int SPACE_AFTER_HEADER = 0x1e;

// The ids of the sliders, the buttons it makes and the check boxes ("Done" is the dialog's own, the id of its footer button)
const int SLIDER_MUSIC = 0;
const int SLIDER_SFX = 1;
const int BUTTON_RESET_HELP = 2;
const int BUTTON_HOW_TO_PLAY = 3;
const int BUTTON_CREDITS = 4;
const int CHECKBOX_FULL_SCREEN = 5;
const int CHECKBOX_HARDWARE = 6;
const int CHECKBOX_CURSORS = 7;
const int BUTTON_DONE = Dialog::ID_FOOTER;

// What the escape key answers: like "Done", it applies what was chosen
const int ESCAPE_RESULT = 999;

// The cels of the dialog buttons
const int BUTTON_CELS = 3;

// The music slider is this wide, as far down in the box as this, and as tall as its thumb; the sound effects slider is under it
// with this gap
const int SLIDER_WIDTH = 0x118;
const int SLIDER_TOP = 0x82;
const int SLIDER_GAP = 0x23;

// The check boxes are this far under the one above them; the full screen box is this far to the right of the cursors box
const int CHECKBOX_GAP = 8;
const int FULL_SCREEN_LEFT_PAD = 0x8c;

// The slider's name is this far above it
const int SLIDER_LABEL_RISE = 6;

// A check box's name is this far to its right and down
const int CHECKBOX_LABEL_X = 0x23;
const int CHECKBOX_LABEL_Y = 0x14;

// The label of the button that has been pressed: a grey
const Color USED_BUTTON_COLOR(0xdc, 0xdc, 0xdc);

// What a box with one "OK" button tells (dialog 0x15)
void ShowNotice(const char* theHeader, const char* theLines)
{
	gApp->DoDialog(DIALOG_NOTICE, true, theHeader, theLines, "OK", Dialog::BUTTONS_FOOTER);
}

}

// 0x45b760
OptionsDialog::OptionsDialog() :
	NcMovingDialog(DIALOG_HEIGHT, IMAGE_DIALOG_BOX, IMAGE_DIALOG_BUTTON, DIALOG_OPTIONS, true, "Options", "", "Done",
		Dialog::BUTTONS_FOOTER)
{
	mMusicSlider = nullptr;
	mSfxSlider = nullptr;
	mResetHelpButton = nullptr;
	mHowToPlayButton = nullptr;
	mCreditsButton = nullptr;
	mHardwareBox = nullptr;
	mFullScreenBox = nullptr;
	mCursorsBox = nullptr;

	mContentInsets = CONTENT_INSETS;
	mSpaceAfterHeader = SPACE_AFTER_HEADER;
	SetHeaderFont(FONT_PTBANANASPLIT27);
	SetLinesFont(FONT_EUROFURENCE14);
	SetColor(Dialog::COLOR_HEADER, Color(255, 255, 255, 255));
	SetColor(Dialog::COLOR_LINES, Color(0, 0, 0, 255));

	mMusicSlider = new Slider(IMAGE_SLIDER_TRACK, IMAGE_SLIDER_THUMB, SLIDER_MUSIC, this);
	mMusicSlider->SetValue(gApp->GetMusicVolume());
	mSfxSlider = new Slider(IMAGE_SLIDER_TRACK, IMAGE_SLIDER_THUMB, SLIDER_SFX, this);
	mSfxSlider->SetValue(gApp->GetSfxVolume());

	mResetHelpButton = new DialogButton(IMAGE_DIALOG_BUTTON, BUTTON_RESET_HELP, this);
	mResetHelpButton->mLabel = "Reset Help";
	mHowToPlayButton = new DialogButton(IMAGE_DIALOG_BUTTON, BUTTON_HOW_TO_PLAY, this);
	mHowToPlayButton->mLabel = "How To Play";
	mCreditsButton = new DialogButton(IMAGE_DIALOG_BUTTON, BUTTON_CREDITS, this);
	mCreditsButton->mLabel = "Credits";

	mHardwareBox = new Checkbox(IMAGE_CHECKBOX_UNCHECKED, IMAGE_CHECKBOX_CHECKED, CHECKBOX_HARDWARE, this);
	mFullScreenBox = new Checkbox(IMAGE_CHECKBOX_UNCHECKED, IMAGE_CHECKBOX_CHECKED, CHECKBOX_FULL_SCREEN, this);
	mCursorsBox = new Checkbox(IMAGE_CHECKBOX_UNCHECKED, IMAGE_CHECKBOX_CHECKED, CHECKBOX_CURSORS, this);

	SetupButtonCels(mYesButton, BUTTON_CELS);
	SetupButtonCels(mResetHelpButton, BUTTON_CELS);
	SetupButtonCels(mHowToPlayButton, BUTTON_CELS);
	SetupButtonCels(mCreditsButton, BUTTON_CELS);
	SetEscapeResult(ESCAPE_RESULT);
}

// 0x45b1d0
OptionsDialog::~OptionsDialog()
{
	if (mMusicSlider != nullptr)
		delete mMusicSlider;
	if (mSfxSlider != nullptr)
		delete mSfxSlider;
	if (mResetHelpButton != nullptr)
		delete mResetHelpButton;
	if (mHowToPlayButton != nullptr)
		delete mHowToPlayButton;
	if (mCreditsButton != nullptr)
		delete mCreditsButton;
	if (mCursorsBox != nullptr)
		delete mCursorsBox;
	if (mHardwareBox != nullptr)
		delete mHardwareBox;
	if (mFullScreenBox != nullptr)
		delete mFullScreenBox;
}

// 0x45b2c0: the check boxes start from what the game is doing now
void OptionsDialog::AddedToManager(WidgetManager* theWidgetManager)
{
	Dialog::AddedToManager(theWidgetManager);
	theWidgetManager->AddWidget(mMusicSlider);
	theWidgetManager->AddWidget(mSfxSlider);
	theWidgetManager->AddWidget(mResetHelpButton);
	theWidgetManager->AddWidget(mHowToPlayButton);
	theWidgetManager->AddWidget(mCreditsButton);
	mHardwareBox->mChecked = gApp->Is3DAccelerated();
	mFullScreenBox->mChecked = !gApp->mIsWindowed;
	mCursorsBox->mChecked = gApp->mCustomCursorsEnabled;
	theWidgetManager->AddWidget(mHardwareBox);
	theWidgetManager->AddWidget(mFullScreenBox);
	theWidgetManager->AddWidget(mCursorsBox);
}

// 0x45b390
void OptionsDialog::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Dialog::RemovedFromManager(theWidgetManager);
	theWidgetManager->RemoveWidget(mMusicSlider);
	theWidgetManager->RemoveWidget(mSfxSlider);
	theWidgetManager->RemoveWidget(mResetHelpButton);
	theWidgetManager->RemoveWidget(mHowToPlayButton);
	theWidgetManager->RemoveWidget(mCreditsButton);
	theWidgetManager->RemoveWidget(mCursorsBox);
	theWidgetManager->RemoveWidget(mHardwareBox);
	theWidgetManager->RemoveWidget(mFullScreenBox);
}

// 0x45b420: the music slider is in the middle, the other slider and the first check box under it, and the buttons stack up from
// "Done" at the bottom
void OptionsDialog::Resize(int theX, int theY, int theWidth, int theHeight)
{
	Dialog::Resize(theX, theY, theWidth, theHeight);

	mMusicSlider->Resize((theWidth - SLIDER_WIDTH) / 2 + theX, theY + SLIDER_TOP, SLIDER_WIDTH, IMAGE_SLIDER_THUMB->GetHeight());
	mSfxSlider->Layout(LAY_Below | LAY_SameLeft | LAY_SameSize, mMusicSlider, 0, SLIDER_GAP, 0, 0);
	mCreditsButton->Layout(LAY_Above | LAY_SameLeft | LAY_SameSize, mYesButton, 0, 0, 0, 0);
	mResetHelpButton->Layout(LAY_Above | LAY_SameLeft | LAY_SameSize, mCreditsButton, 0, 0, 0, 0);
	mHowToPlayButton->Layout(LAY_Above | LAY_SameLeft | LAY_SameSize, mResetHelpButton, 0, 0, 0, 0);
	mHardwareBox->Layout(LAY_Below | LAY_SameLeft, mSfxSlider, 0, CHECKBOX_GAP, 0, 0);
	mHardwareBox->Resize(mHardwareBox->mX, mHardwareBox->mY, IMAGE_CHECKBOX_UNCHECKED->GetWidth(),
		IMAGE_CHECKBOX_UNCHECKED->GetHeight());
	mCursorsBox->Layout(LAY_Below | LAY_SameLeft | LAY_SameSize, mHardwareBox, 0, CHECKBOX_GAP, 0, 0);
	mFullScreenBox->Layout(LAY_SameTop | LAY_Right | LAY_SameSize, mCursorsBox, FULL_SCREEN_LEFT_PAD, 0, 0, 0);
}

// 0x45bcb0: the names of the sliders and the check boxes
void OptionsDialog::Draw(Graphics* g)
{
	Dialog::Draw(g);

	g->SetFont(FONT_EUROFURENCE14BOLD);
	g->SetColor(Color(0, 0, 0, 255));
	g->DrawString("Music Volume", mMusicSlider->mX - mX, (mMusicSlider->mY - mY) - SLIDER_LABEL_RISE);
	g->DrawString("Sound Effects Volume", mSfxSlider->mX - mX, (mSfxSlider->mY - mY) - SLIDER_LABEL_RISE);
	g->DrawString("Hardware Acceleration", (mHardwareBox->mX - mX) + CHECKBOX_LABEL_X, (mHardwareBox->mY - mY) + CHECKBOX_LABEL_Y);
	g->DrawString("Full Screen", (mFullScreenBox->mX - mX) + CHECKBOX_LABEL_X, (mFullScreenBox->mY - mY) + CHECKBOX_LABEL_Y);
	g->DrawString("Custom Cursors", (mCursorsBox->mX - mX) + CHECKBOX_LABEL_X, (mCursorsBox->mY - mY) + CHECKBOX_LABEL_Y);
}

// 0x45b580: a slider moved: the volume follows at once
void OptionsDialog::SliderVal(int theId, double theVal)
{
	if (theId == SLIDER_MUSIC)
	{
		gApp->SetMusicVolume(theVal);
		return;
	}
	if (theId == SLIDER_SFX)
		gApp->SetSfxVolume(theVal);
}

// 0x44fe90: every button of the box clicks when it is pressed (the box's own buttons make no sound of themselves)
void OptionsDialog::ButtonPress(int theId, int theClickCount)
{
	gApp->PlaySample(SOUND_UI_BUTTON_CLICK);
}

// 0x45b5d0: "Done" takes the box away; the other buttons leave it up. It does not go through the moving dialog's own button
// answer, which would close the box on any of them.
void OptionsDialog::ButtonDepress(int theId)
{
	Dialog::ButtonDepress(theId);

	if (theId == BUTTON_DONE)
	{
		CloseWith(BUTTON_DONE);
		return;
	}
	if (theId == BUTTON_RESET_HELP)
	{
		gApp->mUserManager->GetCurrentUser()->mShowHowToPlay = true;
		mResetHelpButton->SetDisabled(true);
		mResetHelpButton->SetColor(ButtonWidget::COLOR_LABEL, USED_BUTTON_COLOR);
		return;
	}
	if (theId == BUTTON_HOW_TO_PLAY)
	{
		gApp->ShowHowToPlay(false);
		return;
	}
	if (theId == BUTTON_CREDITS)
		MakeCreditsDialog();
}

// 0x45b6b0: once the box is out of sight after "Done" or the escape key, what was chosen is applied (the screen mode, the 3D mode
// and the cursors; the volumes are set as the sliders move), the box goes, the game goes on, and the keyboard goes back to the
// game menu, the game or the main menu
void OptionsDialog::Closed()
{
	if (mCloseResult != Dialog::ID_YES && mCloseResult != ESCAPE_RESULT)
		return;

	gApp->SwitchScreenMode(!mFullScreenBox->mChecked, mHardwareBox->mChecked, false);
	gApp->EnableCustomCursors(mCursorsBox->mChecked);
	gApp->KillDialog(this);
	gApp->PauseGame(false);

	Widget* aFocus;
	if (gApp->mGameScreen == nullptr)
	{
		aFocus = gApp->mMainMenu;
		if (aFocus == nullptr)
			return;
	}
	else
	{
		aFocus = gApp->GetDialog(DIALOG_GAME_MENU);
		if (aFocus == nullptr)
		{
			gApp->mWidgetManager->SetFocus(gApp->mGameScreen);
			return;
		}
	}
	gApp->mWidgetManager->SetFocus(aFocus);
}

// 0x45bf20: a check box was ticked or cleared. A box that cannot be ticked says so (a notice, and the box is cleared again, which
// asks this once more); a warning comes when 3D works but is not recommended.
void OptionsDialog::CheckboxChecked(int theId, bool checked)
{
	gApp->PlaySample(SOUND_UI_CHECK);

	if (theId == CHECKBOX_HARDWARE)
	{
		if (checked)
		{
			if (!gApp->Is3DAccelerationSupported())
			{
				mHardwareBox->SetChecked(false, true);
				ShowNotice("Not Supported",
					"Hardware acceleration can not be enabled on this computer. \nYour \t\t\t\t\tvideo card does not meet the minimum "
					"requirements for this game.");
			}
			else if (!gApp->Is3DAccelerationRecommended())
			{
				ShowNotice("Warning",
					"Your video card may not fully support this feature.\n\t\t\t\t\tIf you experience slower performance, please "
					"disable Hardware Acceleration.");
			}
		}
	}
	else if (theId == CHECKBOX_FULL_SCREEN)
	{
		if (gApp->mForceFullscreen)
		{
			if (checked)
				return;
			ShowNotice("No Windowed Mode",
				"Windowed mode is only available if your desktop is running in\n\t\t\t\teither 16 bit or 32 bit color mode, which it is "
				"not.");
		}
		else
		{
			if (checked)
				return;
			if (gApp->mWidth < gApp->mScreenWidth && gApp->mHeight < gApp->mScreenHeight)
				return;
			ShowNotice("No Windowed Mode",
				"Windowed mode is only available if your desktop resolution\n\t\t\t\tis larger than 800 by 600 pixels, which it is "
				"not.");
		}
		mFullScreenBox->SetChecked(true, true);
	}
}

// 0x45c2e0
OptionsDialog* Sexy::MakeOptionsDialog()
{
	gApp->PauseGame(true);
	OptionsDialog* aDialog = new OptionsDialog();
	aDialog->Resize(0, 0, DIALOG_WIDTH, DIALOG_HEIGHT);
	gApp->CenterDialog(aDialog);
	gApp->AddDialog(DIALOG_OPTIONS, aDialog);
	return aDialog;
}
