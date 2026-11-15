#include "OptionsDialog.h"
#include "Board.h"
#include "BracketButton.h"
#include "HeavyWeaponApp.h"
#include "MessageDialog.h"
#include "YesNoDialog.h"
#include "Checkbox.h"
#include "DDImage.h"
#include "GameText.h"
#include "Graphics.h"
#include "ImageFont.h"
#include "Slider.h"
#include "Sprite.h"
#include "WidgetManager.h"
#include "Compat2004.h"
#include <cstdio>

using namespace Sexy;

namespace
{
	const int DIALOG_X = 0x8c;
	const int DIALOG_WIDTH = 0x168;
	const int DIALOG_HEIGHT = 0x19a;
	const int SCREEN_CENTER_Y = 0xf0;
	const int BUTTON_HEIGHT = 0x1e;
	const int BUTTON_INSET = 0x28;
	const int HELP_BOTTOM = 0x78;		// the HELP and CREDITS buttons' tops are this far above the box's bottom
	const int DONE_BOTTOM = 0x55;		// the DONE button's
	const int SLIDER_X = 0x78;
	const int SLIDER_SFX_Y = 0x37;
	const int SLIDER_MUSIC_Y = 0x55;
	const int SLIDER_PARTICLES_Y = 0x73;
	const int SLIDER_WIDTH_LESS = 0xa0;
	const int CHECKBOX_SIZE = 0x1e;
	const int CHECKBOX_FULL_SCREEN_Y = 0x9b;
	const int CHECKBOX_ACCELERATION_Y = 0xb9;
	const int CHECKBOX_CURSORS_Y = 0xd7;
	const int CHECKBOX_HINTS_Y = 0xf5;
	const int LABEL_SLIDER_X = 0x28;
	const int LABEL_CHECKBOX_X = 0x50;
	const int LABEL_SFX_Y = 0x4b;
	const int LABEL_MUSIC_Y = 0x69;
	const int LABEL_PARTICLES_Y = 0x87;
	const int LABEL_FULL_SCREEN_Y = 0xaf;
	const int LABEL_ACCELERATION_Y = 0xcd;
	const int LABEL_CURSORS_Y = 0xeb;
	const int LABEL_HINTS_Y = 0x109;
	const int READOUT_BOTTOM = 0x1e;	// the figure of a slider being dragged is this far above the box's bottom
	const int INVALID_MODE_WIDTH = 400;
	const int INVALID_MODE_HEIGHT = 0xf0;
	const int NOT_SUPPORTED_WIDTH = 0x15e;
	const int NOT_SUPPORTED_HEIGHT = 0x104;
	const int DISABLE_WIDTH = 0x1a4;
	const int DISABLE_HEIGHT = 0xe6;
	const int WARNING_WIDTH = 400;
	const int WARNING_HEIGHT = 300;
	const int SOUND_SLIDER_RELEASED = 6;
}

// 0x42a540: the sliders start at the current volumes and particle amount, the checkboxes at the current settings
OptionsDialog::OptionsDialog(HeavyWeaponApp* theApp) :
	Dialog2004(theApp->mSprites[IMAGE_DIALOG]->mImage, NULL, DIALOG_ID, true, GT(0x52d168), "", "", BUTTONS_NONE)
{
	mApp = theApp;
	SetButtonFont(theApp->mFonts[FONT_NORMAL]);
	SetHeaderFont(theApp->mFonts[FONT_RUBBERSTAMPLET20]);
	SetLinesFont(theApp->mFonts[FONT_NORMAL]);
	SetColor(COLOR_HEADER, Color(0xff, 0xff, 0));
	SetColor(COLOR_LINES, Color(0xc0, 0xc0, 0xc0));
	SetColor(COLOR_BUTTON_TEXT, Color(0, 0, 0));
	mDoneButton = new BracketButton(mApp, this, BUTTON_DONE);
	mDoneButton->SetTint(Color(0x40, 0xc0, 0x40));
	mDoneButton->mLabel = GT(0x52ea24);
	mHelpButton = new BracketButton(mApp, this, BUTTON_HELP);
	mHelpButton->SetTint(Color(0x40, 0xc0, 0x40));
	mHelpButton->mLabel = GT(0x52ea08);
	mCreditsButton = new BracketButton(mApp, this, BUTTON_CREDITS);
	mCreditsButton->SetTint(Color(0x40, 0xc0, 0x40));
	mCreditsButton->mLabel = GT(0x52ea00);
	mSfxSlider = new Slider2004(mApp->mSprites[IMAGE_SLIDERTRACK]->mImage, mApp->mSprites[IMAGE_SLIDERTHUMB]->mImage,
		SLIDER_SFX, this);
	mSfxSlider->SetValue(mApp->GetSfxVolume());
	mMusicSlider = new Slider2004(mApp->mSprites[IMAGE_SLIDERTRACK]->mImage, mApp->mSprites[IMAGE_SLIDERTHUMB]->mImage,
		SLIDER_MUSIC, this);
	mMusicSlider->SetValue(mApp->GetMusicVolume());
	mParticlesSlider = new Slider2004(mApp->mSprites[IMAGE_SLIDERTRACK]->mImage, mApp->mSprites[IMAGE_SLIDERTHUMB]->mImage,
		SLIDER_PARTICLES, this);
	mParticlesSlider->SetValue(mApp->mParticles);
	mWindowedCheckbox = new Checkbox(mApp->mSprites[IMAGE_UNCHECKED]->mImage, mApp->mSprites[IMAGE_CHECKED]->mImage,
		CHECKBOX_FULL_SCREEN, this);
	mWindowedCheckbox->mChecked = !mApp->mIsWindowed;
	mAccelerationCheckbox = new Checkbox(mApp->mSprites[IMAGE_UNCHECKED]->mImage, mApp->mSprites[IMAGE_CHECKED]->mImage,
		CHECKBOX_ACCELERATION, this);
	mAccelerationCheckbox->mChecked = mApp->Is3DAccelerated();
	mCursorsCheckbox = new Checkbox(mApp->mSprites[IMAGE_UNCHECKED]->mImage, mApp->mSprites[IMAGE_CHECKED]->mImage,
		CHECKBOX_CURSORS, this);
	mCursorsCheckbox->mChecked = mApp->mCustomCursorsEnabled;
	mHintsCheckbox = new Checkbox(mApp->mSprites[IMAGE_UNCHECKED]->mImage, mApp->mSprites[IMAGE_CHECKED]->mImage,
		CHECKBOX_HINTS, this);
	mHintsCheckbox->mChecked = (mApp->CurrentUser().mFlags & UserRecord::FLAG_NO_TIPS) == 0;
	mWidth = DIALOG_WIDTH;
	mHeight = DIALOG_HEIGHT;
	Resize(DIALOG_X, SCREEN_CENTER_Y - mHeight / 2, DIALOG_WIDTH, mHeight);
}

// 0x42a510 (body 0x429d30)
OptionsDialog::~OptionsDialog()
{
	delete mDoneButton;
	delete mHelpButton;
	delete mCreditsButton;
	delete mSfxSlider;
	delete mMusicSlider;
	delete mParticlesSlider;
	delete mWindowedCheckbox;
	delete mAccelerationCheckbox;
	delete mCursorsCheckbox;
	delete mHintsCheckbox;
}

// 0x429f50 slot 1
void OptionsDialog::AddedToManager(WidgetManager* theManager)
{
	Dialog2004::AddedToManager(theManager);
	theManager->AddWidget(mDoneButton);
	theManager->AddWidget(mHelpButton);
	theManager->AddWidget(mCreditsButton);
	theManager->AddWidget(mSfxSlider);
	theManager->AddWidget(mMusicSlider);
	theManager->AddWidget(mParticlesSlider);
	theManager->AddWidget(mWindowedCheckbox);
	theManager->AddWidget(mAccelerationCheckbox);
	theManager->AddWidget(mCursorsCheckbox);
	theManager->AddWidget(mHintsCheckbox);
}

// 0x429e70 slot 2
void OptionsDialog::RemovedFromManager(WidgetManager* theManager)
{
	Dialog2004::RemovedFromManager(theManager);
	theManager->RemoveWidget(mDoneButton);
	theManager->RemoveWidget(mHelpButton);
	theManager->RemoveWidget(mCreditsButton);
	theManager->RemoveWidget(mSfxSlider);
	theManager->RemoveWidget(mMusicSlider);
	theManager->RemoveWidget(mParticlesSlider);
	theManager->RemoveWidget(mWindowedCheckbox);
	theManager->RemoveWidget(mAccelerationCheckbox);
	theManager->RemoveWidget(mCursorsCheckbox);
	theManager->RemoveWidget(mHintsCheckbox);
}

// 0x42a030 slot 13: HELP and CREDITS side by side over a DONE button across the foot, the sliders and the checkboxes above them.
// The original's REGISTER and UPDATE buttons shared the row above the foot, and DONE then took only the left half of its row;
// with them gone DONE spans the row, which is the original's own layout for a copy with neither
void OptionsDialog::Resize(int theX, int theY, int theWidth, int theHeight)
{
	Dialog2004::Resize(theX, theY, theWidth, theHeight);
	mHelpButton->Resize(mX + BUTTON_INSET, mY + mHeight - HELP_BOTTOM, mWidth / 2 - 0x32, BUTTON_HEIGHT);
	mCreditsButton->Resize(mWidth / 2 + 10 + mX, mY + mHeight - HELP_BOTTOM, mWidth / 2 - 0x32, BUTTON_HEIGHT);
	mDoneButton->Resize(mX + BUTTON_INSET, mY + mHeight - DONE_BOTTOM, mWidth - 0x50, BUTTON_HEIGHT);
	mSfxSlider->Resize(mX + SLIDER_X, mY + SLIDER_SFX_Y, mWidth - SLIDER_WIDTH_LESS, BUTTON_HEIGHT);
	mMusicSlider->Resize(mX + SLIDER_X, mY + SLIDER_MUSIC_Y, mWidth - SLIDER_WIDTH_LESS, BUTTON_HEIGHT);
	mParticlesSlider->Resize(mX + SLIDER_X, mY + SLIDER_PARTICLES_Y, mWidth - SLIDER_WIDTH_LESS, BUTTON_HEIGHT);
	mWindowedCheckbox->Resize(mX + BUTTON_INSET, mY + CHECKBOX_FULL_SCREEN_Y, CHECKBOX_SIZE, CHECKBOX_SIZE);
	mAccelerationCheckbox->Resize(mX + BUTTON_INSET, mY + CHECKBOX_ACCELERATION_Y, CHECKBOX_SIZE, CHECKBOX_SIZE);
	mCursorsCheckbox->Resize(mX + BUTTON_INSET, mY + CHECKBOX_CURSORS_Y, CHECKBOX_SIZE, CHECKBOX_SIZE);
	mHintsCheckbox->Resize(mX + BUTTON_INSET, mY + CHECKBOX_HINTS_Y, CHECKBOX_SIZE, CHECKBOX_SIZE);
}

// 0x42b2f0 slot 16: the labels, and the figure of the slider being dragged at the foot
void OptionsDialog::Draw(Graphics* g)
{
	Dialog2004::Draw(g);
	WriteString2004(g, GT(0x52ed54), LABEL_SLIDER_X, LABEL_SFX_Y, -1, -1, true, 0, -1);
	WriteString2004(g, GT(0x52ed4c), LABEL_SLIDER_X, LABEL_MUSIC_Y, -1, -1, true, 0, -1);
	WriteString2004(g, GT(0x52ed44), LABEL_SLIDER_X, LABEL_PARTICLES_Y, -1, -1, true, 0, -1);
	WriteString2004(g, GT(0x52ed30), LABEL_CHECKBOX_X, LABEL_FULL_SCREEN_Y, -1, -1, true, 0, -1);
	WriteString2004(g, GT(0x52ed18), LABEL_CHECKBOX_X, LABEL_ACCELERATION_Y, -1, -1, true, 0, -1);
	WriteString2004(g, GT(0x52ed04), LABEL_CHECKBOX_X, LABEL_CURSORS_Y, -1, -1, true, 0, -1);
	WriteString2004(g, GT(0x52ecf4), LABEL_CHECKBOX_X, LABEL_HINTS_Y, -1, -1, true, 0, -1);
	char aBuffer[16];
	if (mSfxSlider->mDragging)
	{
		snprintf(aBuffer, sizeof(aBuffer), "%d", (int) (mSfxSlider->mVal * 100.0));
		WriteString2004(g, GT(0x52ecc8) + aBuffer + GT(0x52ece0), LABEL_SLIDER_X, mHeight - READOUT_BOTTOM, -1, -1, true, 0, -1);
	}
	if (mMusicSlider->mDragging)
	{
		snprintf(aBuffer, sizeof(aBuffer), "%d", (int) (mMusicSlider->mVal * 100.0));
		WriteString2004(g, GT(0x52ecb8) + aBuffer + GT(0x52ece0), LABEL_SLIDER_X, mHeight - READOUT_BOTTOM, -1, -1, true, 0, -1);
	}
	if (mParticlesSlider->mDragging)
	{
		const SexyString& aBackground = (mParticlesSlider->mVal != 0.0) ? GT(0x52ecb0) : GT(0x52ecb4);
		snprintf(aBuffer, sizeof(aBuffer), "%d", (int) (mParticlesSlider->mVal * 100.0));
		WriteString2004(g, GT(0x52ec94) + aBuffer + GT(0x52ece0) + GT(0x52eca0) + aBackground, LABEL_SLIDER_X,
			mHeight - READOUT_BOTTOM, -1, -1, true, 0, -1);
	}
}

// 0x42a340 listener slot 4
void OptionsDialog::ButtonPress(int theId)
{
	mApp->PlaySample(SOUND_BUTTONUP, 0);
}

// 0x42a360 listener slot 5: DONE closes the box and applies the full screen and acceleration choices (a switch of mode
// pauses the game); HELP and CREDITS open their screens; the box that asked about hardware acceleration reports YES (the
// checkbox stays as clicked) or NO (it goes back to as it was) and closes
void OptionsDialog::ButtonDepress(int theId)
{
	mApp->PlaySample(SOUND_BUTTONUP, 0);
	switch (theId)
	{
	case BUTTON_DONE:
		{
			bool is3d = mAccelerationCheckbox->mChecked;
			bool wantWindowed = !mWindowedCheckbox->mChecked;
			mApp->KillDialog(DIALOG_ID);
			mApp->SwitchScreenMode(wantWindowed, is3d);
		}
		return;
	case BUTTON_HELP:
		mApp->ShowHelp(static_cast<ButtonListener*>(NULL));
		return;
	case BUTTON_CREDITS:
		mApp->ShowCredits();
		return;
	case BUTTON_ACCELERATION_NO:
		mAccelerationCheckbox->mChecked = true;
		break;
	case BUTTON_ACCELERATION_YES:
		mAccelerationCheckbox->mChecked = false;
		break;
	default:
		return;
	}
	mApp->KillDialog(YesNoDialog::DIALOG_ID);
}

// 0x42a2c0 slider listener slot 0: the volumes and the particle amount follow the sliders; letting go of the sound effects
// slider plays a sound to hear the volume by (the dialog's mDragging is the byte at +0xf4)
void OptionsDialog::SliderVal(int theId, double theVal)
{
	if (theId == SLIDER_MUSIC)
		mApp->SetMusicVolume(theVal);
	else if (theId == SLIDER_SFX)
	{
		mApp->SetSfxVolume(theVal);
		if (!mSfxSlider->mDragging && !mDragging)
			mApp->PlaySample(SOUND_SLIDER_RELEASED, 0);
	}
	else if (theId == SLIDER_PARTICLES)
		mApp->mParticles = theVal;
}

// 0x42ade0 checkbox listener slot 0: a click sounds; then each box does its own work. Full screen cannot be turned off
// when the desktop's colour depth does not allow a window; hardware acceleration asks first, in the direction the card is
// doubtful about
void OptionsDialog::CheckboxChecked(int theId, bool checked)
{
	mApp->PlaySample(checked ? SOUND_BUTTONDOWN : SOUND_BUTTONUP, 0);
	switch (theId)
	{
	case CHECKBOX_FULL_SCREEN:
		if (!checked && mApp->mForceFullscreen)
		{
			mApp->AddDialog(MessageDialog::DIALOG_ID, new MessageDialog(mApp, INVALID_MODE_WIDTH, INVALID_MODE_HEIGHT,
				GT(0x52ec0c), GT(0x52ec20)));
			mWindowedCheckbox->mChecked = true;
		}
		break;
	case CHECKBOX_ACCELERATION:
		if (!checked)
		{
			if (!mApp->Is3DAccelerationRecommended())
			{
				mAccelerationCheckbox->mChecked = false;
				return;
			}
			mApp->AddDialog(YesNoDialog::DIALOG_ID, new YesNoDialog(mApp, this, GT(0x52ea2c), GT(0x52ea48), DISABLE_WIDTH,
				DISABLE_HEIGHT, BUTTON_ACCELERATION_YES, BUTTON_ACCELERATION_NO));
			return;
		}
		if (mApp->Is3DAccelerationSupported())
		{
			if (mApp->Is3DAccelerationRecommended())
			{
				mAccelerationCheckbox->mChecked = true;
				return;
			}
			mApp->AddDialog(YesNoDialog::DIALOG_ID, new YesNoDialog(mApp, this, GT(0x52eab0), GT(0x52eab8), WARNING_WIDTH,
				WARNING_HEIGHT, 0, 0));
			return;
		}
		mApp->AddDialog(MessageDialog::DIALOG_ID, new MessageDialog(mApp, NOT_SUPPORTED_WIDTH, NOT_SUPPORTED_HEIGHT,
			GT(0x52eb74), GT(0x52eb88)));
		mAccelerationCheckbox->SetChecked(false, false);
		return;
	case CHECKBOX_CURSORS:
		mApp->EnableCustomCursors(!mApp->mCustomCursorsEnabled);
		mApp->SetCursorImage(CURSOR_POINTER, mApp->mSprites[IMAGE_CURSOR_POINTER]->mImage);
		return;
	case CHECKBOX_HINTS:
		mApp->CurrentUser().mFlags ^= UserRecord::FLAG_NO_TIPS;
		mApp->mTipsShown = 0;
		if (mApp->mBoard != NULL)
		{
			mApp->mBoard->mClickCount = 0;
			mApp->mBoard->mShotsFired = 0;
		}
		return;
	}
}
