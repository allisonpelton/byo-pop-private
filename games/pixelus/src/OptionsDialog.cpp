#include "OptionsDialog.h"
#include "PixelusApp.h"
#include "UserDirectory.h"
#include "GameText.h"
#include "Graphics.h"
#include "Image.h"
#include "ImageFont.h"
#include "DialogButton.h"
#include "Slider.h"
#include "Checkbox.h"
#include "WidgetManager.h"
#include "Insets.h"
#include <string.h>

using namespace Sexy;

// 0x419330
OptionsDialog::OptionsDialog(PixelusApp* theApp, bool theInGame) :
	GameDialog(theApp, theApp->mImages[IMAGE_DIALOG], theApp->mImages[IMAGE_DIALOGBUTTONCOMPONENTS], DIALOG_ID, true,
		GT(0x528d84), "", GT(0x5284c8), BUTTONS_FOOTER, NULL)
{
	mInGame = theInGame;
	mContentInsets = Insets(0x30, 0x3c, 0x30, 0x20);

	mMusicSlider = new Slider(theApp->mImages[IMAGE_SLIDERTRACK], theApp->mImages[IMAGE_SLIDERTHUMB], ID_MUSIC, this);
	mMusicSlider->SetValue(theApp->GetMusicVolume());
	mSoundSlider = new Slider(theApp->mImages[IMAGE_SLIDERTRACK], theApp->mImages[IMAGE_SLIDERTHUMB], ID_SOUND, this);
	mSoundSlider->SetValue(theApp->GetSfxVolume());

	mFullscreenBox = new Checkbox(theApp->mImages[IMAGE_CHECKBOX_UNCHECKED], theApp->mImages[IMAGE_CHECKBOX_CHECKED], ID_FULLSCREEN, this);
	mFullscreenBox->SetChecked(!theApp->mIsWindowed, false);
	mCursorsBox = new Checkbox(theApp->mImages[IMAGE_CHECKBOX_UNCHECKED], theApp->mImages[IMAGE_CHECKBOX_CHECKED], ID_CURSORS, this);
	mCursorsBox->SetChecked(theApp->mCustomCursorsEnabled, false);
	mHintsBox = new Checkbox(theApp->mImages[IMAGE_CHECKBOX_UNCHECKED], theApp->mImages[IMAGE_CHECKBOX_CHECKED], ID_HINTS, this);
	mHintsBox->SetChecked(theApp->GetHintsDisabled(), false);

	struct Spec { DialogButton** mButton; int mId; unsigned int mText; };
	Spec aSpecs[4] =
	{
		{&mResetButton, ID_RESET_HINTS, 0x528d58}, {&mCreditsButton, ID_CREDITS, 0x528d50},
		{&mHelpButton, ID_HOW_TO_PLAY, 0x50e4b0}, {&mMenuButton, ID_MAIN_MENU, 0x50e4a4}
	};
	for (int i = 0; i < 4; i++)
	{
		DialogButton* aButton = new DialogButton(theApp->mImages[IMAGE_DIALOGBUTTONCOMPONENTS], aSpecs[i].mId, this);
		aButton->mLabel = GT(aSpecs[i].mText);
		StyleButton(theApp, aButton, 3);
		*aSpecs[i].mButton = aButton;
	}
	if (!mInGame)
	{
		delete mMenuButton;
		mMenuButton = NULL;
	}
}

OptionsDialog::~OptionsDialog()
{
	delete mMusicSlider;
	delete mSoundSlider;
	delete mFullscreenBox;
	delete mCursorsBox;
	delete mHintsBox;
	delete mResetButton;
	delete mCreditsButton;
	delete mHelpButton;
	delete mMenuButton;
}

// 0x419020 / 0x4e7d90
void OptionsDialog::AddedToManager(WidgetManager* theWidgetManager)
{
	GameDialog::AddedToManager(theWidgetManager);
	theWidgetManager->AddWidget(mMusicSlider);
	theWidgetManager->AddWidget(mSoundSlider);
	theWidgetManager->AddWidget(mFullscreenBox);
	theWidgetManager->AddWidget(mCursorsBox);
	theWidgetManager->AddWidget(mHintsBox);
	theWidgetManager->AddWidget(mResetButton);
	theWidgetManager->AddWidget(mCreditsButton);
	theWidgetManager->AddWidget(mHelpButton);
	if (mMenuButton != NULL)
		theWidgetManager->AddWidget(mMenuButton);
}

void OptionsDialog::RemovedFromManager(WidgetManager* theWidgetManager)
{
	GameDialog::RemovedFromManager(theWidgetManager);
	theWidgetManager->RemoveWidget(mMusicSlider);
	theWidgetManager->RemoveWidget(mSoundSlider);
	theWidgetManager->RemoveWidget(mFullscreenBox);
	theWidgetManager->RemoveWidget(mCursorsBox);
	theWidgetManager->RemoveWidget(mHintsBox);
	theWidgetManager->RemoveWidget(mResetButton);
	theWidgetManager->RemoveWidget(mCreditsButton);
	theWidgetManager->RemoveWidget(mHelpButton);
	if (mMenuButton != NULL)
		theWidgetManager->RemoveWidget(mMenuButton);
}

// 0x4e7f50 and 0x4190b0: the sliders, then two columns of boxes, then the rows of buttons above the footer's
void OptionsDialog::Resize(int theX, int theY, int theWidth, int theHeight)
{
	GameDialog::Resize(theX, theY, theWidth, theHeight);
	Image* aBox = mApp->mImages[IMAGE_CHECKBOX_CHECKED];
	int aBoxWidth = aBox->GetWidth(), aBoxHeight = aBox->GetHeight();
	int aButtonHeight = mApp->mImages[IMAGE_DIALOGBUTTONCOMPONENTS]->GetHeight();

	mMusicSlider->Resize(mX + 0x98, mY + 0x5d, mWidth - 0xe0, 0x24);
	mSoundSlider->Resize(mX + 0x98, mY + 0x7d, mWidth - 0xe0, 0x24);
	mFullscreenBox->Resize(mX + 0x40, mY + 0x9f, aBoxWidth, aBoxHeight);
	mCursorsBox->Resize(mX + 0xc0, mY + 0x9f, aBoxWidth, aBoxHeight);
	mHintsBox->Resize(mX + 0x40, mY + 0x9f + aBoxHeight, aBoxWidth, aBoxHeight);

	int aLeft = mContentInsets.mLeft, aRight = mContentInsets.mRight;
	int aHalf = (mWidth - aLeft - aRight) / 2;
	mResetButton->Resize(mX + aLeft, mY + 0xf8, aHalf, aButtonHeight);
	mCreditsButton->Resize(mX + aLeft + aHalf, mY + 0xf8, aHalf, aButtonHeight);

	int aRowY = (mYesButton != NULL ? mYesButton->mY : mY + mHeight - 0x44) - 0x24;
	if (mMenuButton == NULL)
		mHelpButton->Resize(mX + aLeft, aRowY, aHalf * 2, aButtonHeight);
	else
	{
		mHelpButton->Resize(mX + aLeft, aRowY, aHalf, aButtonHeight);
		mMenuButton->Resize(mX + aLeft + aHalf, aRowY, aHalf, aButtonHeight);
	}
}

// 0x4e7c80
int OptionsDialog::GetPreferredHeight(int theWidth)
{
	return 0x188;
}

// 0x4e83c0
void OptionsDialog::Draw(Graphics* g)
{
	GameDialog::Draw(g);
	g->SetFont(mApp->mFonts[FONT_GARAMOND14]);
	g->SetColor(Color(255, 255, 255));
	g->DrawString(GT(0x528b28), 0x48, 0x75);
	g->DrawString(GT(0x528b1c), 0x48, 0x95);
	g->DrawString(GT(0x528b10), mFullscreenBox->mX - mX + 0x20, mFullscreenBox->mY - mY + 0x14);
	g->DrawString(GT(0x528b00), mCursorsBox->mX - mX + 0x20, mCursorsBox->mY - mY + 0x14);
	g->DrawString(GT(0x528ae4), mHintsBox->mX - mX + 0x20, mHintsBox->mY - mY + 0x14);
}

// 0x4e8950: the buttons of the dialog (the footer's is the app's: ButtonDepress(2002))
void OptionsDialog::ButtonDepress(int theId)
{
	switch (theId)
	{
	case ID_RESET_HINTS:
	{
		// every hint comes again
		UserProfile* aUser = mApp->mUserDirectory->GetCurrent();
		if (aUser != NULL)
			memset(aUser->mFlags, 0, sizeof(aUser->mFlags));
		mResetButton->SetDisabled(true);
		mResetButton->SetColor(0, Color(0x80, 0x80, 0x80));
		return;
	}
	case ID_CREDITS:
		mApp->ShowCredits();
		return;
	case ID_HOW_TO_PLAY:
		mApp->ShowHelp();
		return;
	case ID_MAIN_MENU:
		mApp->OptionsToMainMenu();
		return;
	}
	GameDialog::ButtonDepress(theId);
}

void OptionsDialog::SliderVal(int theId, double theVal)
{
	if (theId == ID_MUSIC)
		mApp->SetMusicVolume(theVal);
	else if (theId == ID_SOUND)
		mApp->SetSfxVolume(theVal);
}

void OptionsDialog::CheckboxChecked(int theId, bool theChecked)
{
	switch (theId)
	{
	case ID_FULLSCREEN:
		mApp->SwitchScreenMode(!theChecked);
		break;
	case ID_CURSORS:
		mApp->EnableCustomCursors(theChecked);
		break;
	case ID_HINTS:
		mApp->SetHintsDisabled(theChecked);
		break;
	}
}
