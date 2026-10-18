#include "OptionsDialog.h"
#include "WinBMApp.h"
#include "Res.h"
#include "Compat12.h"

#include "Graphics.h"
#include "ImageFont.h"
#include "DialogButton.h"
#include "Checkbox.h"
#include "WidgetManager.h"

using namespace Sexy;

// 0x415560
void OptionsDialog::SetColor(int theIdx, const Color& theColor)
{
	// The button text colour also goes to the five buttons (the links keep their own)
	Dialog::SetColor(theIdx, theColor);
	if (theIdx == COLOR_BUTTON_TEXT)
	{
		if (mRegisterButton != NULL)
			mRegisterButton->SetColor(DialogButton::COLOR_LABEL, theColor);
		if (mScoresButton != NULL)
			mScoresButton->SetColor(DialogButton::COLOR_LABEL, theColor);
		if (mHelpButton != NULL)
			mHelpButton->SetColor(DialogButton::COLOR_LABEL, theColor);
		if (mCheckUpdatesButton != NULL)
			mCheckUpdatesButton->SetColor(DialogButton::COLOR_LABEL, theColor);
		if (mCreditsButton != NULL)
			mCreditsButton->SetColor(DialogButton::COLOR_LABEL, theColor);
	}
}

// 0x4155e0
void OptionsDialog::SliderVal(int theId, double theVal)
{
	// Applied live; closing the dialog does not revert anything
	if (theId == MUSIC_SLIDER_ID)
		mApp->SetMusicVolume(theVal);
	else if (theId == SFX_SLIDER_ID)
		mApp->SetSfxVolume(theVal);
}

// 0x415620
void OptionsDialog::CheckboxChecked(int theId, bool checked)
{
	// Both checkboxes have id 0, so unticking either one while fullscreen is forced re-ticks fullscreen
	if ((!checked) && (mApp->mForceFullscreen))
		mFullscreenCheckbox->SetChecked(true, true);
}

// 0x415660
void OptionsDialog::ButtonPress(int theId)
{
	Dialog::ButtonPress(theId);

	// Meant to skip the dialog's own YES/NO ids, but those are 1000/1001 here, so this plays for every
	// button: Done clicks twice (also WinBMApp::ButtonPress(2001))
	if ((theId != mId + 2000) && (theId != mId + 3000))
		mApp->PlaySample(SOUND_BUTTONCLICK);
}

// 0x4156b0
int OptionsDialog::GetPreferredHeight(int theWidth)
{
	return (int) mLinkButtons.size() * ROW_PITCH + BASE_PREFERRED_HEIGHT;
}

// 0x415770
void OptionsDialog::AddedToManager(WidgetManager* theWidgetManager)
{
	Dialog::AddedToManager(theWidgetManager);
	theWidgetManager->AddWidget(mRegisterButton);
	theWidgetManager->AddWidget(mScoresButton);
	theWidgetManager->AddWidget(mHelpButton);
	theWidgetManager->AddWidget(mMusicSlider);
	theWidgetManager->AddWidget(mSfxSlider);
	theWidgetManager->AddWidget(mCustomCursorsCheckbox);
	theWidgetManager->AddWidget(mFullscreenCheckbox);
	theWidgetManager->AddWidget(mCheckUpdatesButton);
	theWidgetManager->AddWidget(mCreditsButton);
	for (int i = 0; i < (int) mLinkButtons.size(); i++)
		theWidgetManager->AddWidget(mLinkButtons[i]);
}

// 0x415830
void OptionsDialog::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Dialog::RemovedFromManager(theWidgetManager);
	theWidgetManager->RemoveWidget(mRegisterButton);
	theWidgetManager->RemoveWidget(mScoresButton);
	theWidgetManager->RemoveWidget(mHelpButton);
	theWidgetManager->RemoveWidget(mMusicSlider);
	theWidgetManager->RemoveWidget(mSfxSlider);
	theWidgetManager->RemoveWidget(mFullscreenCheckbox);
	theWidgetManager->RemoveWidget(mCustomCursorsCheckbox);
	theWidgetManager->RemoveWidget(mCheckUpdatesButton);
	theWidgetManager->RemoveWidget(mCreditsButton);
	for (int i = 0; i < (int) mLinkButtons.size(); i++)
		theWidgetManager->RemoveWidget(mLinkButtons[i]);
}

// 0x415970
OptionsDialog::~OptionsDialog()
{
	delete mRegisterButton;
	delete mScoresButton;
	delete mCheckUpdatesButton;
	delete mCreditsButton;
	delete mMusicSlider;
	delete mSfxSlider;
	delete mFullscreenCheckbox;
	delete mCustomCursorsCheckbox;
	delete mHelpButton;
	for (int i = 0; i < (int) mLinkButtons.size(); i++)
		delete mLinkButtons[i];
}

// 0x415ad0
void OptionsDialog::Resize(int theX, int theY, int theWidth, int theHeight)
{
	// Places Done (mYesButton) at the bottom
	Dialog::Resize(theX, theY, theWidth, theHeight);

	int aLeft = mContentInsets.mLeft;
	int anInnerWidth = mWidth - aLeft - mContentInsets.mRight;
	int aLinkCount = (int) mLinkButtons.size();

	// The links stack up from just above Done
	for (int i = 0; i < aLinkCount; i++)
		mLinkButtons[i]->Resize(mX + aLeft, (i - aLinkCount) * ROW_PITCH + mYesButton->mY, anInnerWidth, BUTTON_HEIGHT);

	// Then two rows of buttons above the links; a button that is not laid out stays 0x0 (invisible)
	int aRowY = mYesButton->mY - (aLinkCount + 1) * ROW_PITCH;
	int aHalfWidth = (anInnerWidth - 3) / 2;
	bool dontUpdate = mApp->GetBoolean("DontUpdate", false);
	if (!dontUpdate)
	{
		mCheckUpdatesButton->Resize(mX + aLeft, aRowY, aHalfWidth, BUTTON_HEIGHT);
		mCreditsButton->Resize(mX + aLeft + 3 + aHalfWidth, aRowY, aHalfWidth, BUTTON_HEIGHT);
		aRowY -= ROW_PITCH;

		if (!mApp->mBuildUnlocked)
		{
			int aThirdWidth = (anInnerWidth - 6) / 3;
			mHelpButton->Resize(mX + aLeft, aRowY, aThirdWidth, BUTTON_HEIGHT);
			mRegisterButton->Resize(mX + aLeft + 3 + aThirdWidth, aRowY, aThirdWidth, BUTTON_HEIGHT);
			mScoresButton->Resize(mX + 6 + aThirdWidth * 2 + aLeft, aRowY, aThirdWidth, BUTTON_HEIGHT);
		}
		else
		{
			mHelpButton->Resize(mX + aLeft, aRowY, aHalfWidth, BUTTON_HEIGHT);
			mScoresButton->Resize(mX + aLeft + 3 + aHalfWidth, aRowY, aHalfWidth, BUTTON_HEIGHT);
		}
	}
	else
	{
		// The Steam build (DontUpdate, NoReg): Help + Credits, then Scores across the full width
		mHelpButton->Resize(mX + aLeft, aRowY, aHalfWidth, BUTTON_HEIGHT);
		mCreditsButton->Resize(mX + aLeft + 3 + aHalfWidth, aRowY, aHalfWidth, BUTTON_HEIGHT);
		aRowY -= ROW_PITCH;

		if (!mApp->mBuildUnlocked)
		{
			mRegisterButton->Resize(mX + aLeft, aRowY, aHalfWidth, BUTTON_HEIGHT);
			mScoresButton->Resize(mX + aLeft + 3 + aHalfWidth, aRowY, aHalfWidth, BUTTON_HEIGHT);
		}
		else
			mScoresButton->Resize(mX + aLeft, aRowY, anInnerWidth, BUTTON_HEIGHT);
	}

	mMusicSlider->Resize(mX + 152, mY + 68, mWidth - 192, 36);
	mSfxSlider->Resize(mX + 152, mY + 100, mWidth - 192, 36);
	mFullscreenCheckbox->Resize(mX + 48, mY + 138, 20, 20);
	mCustomCursorsCheckbox->Resize(mX + 176, mY + 138, 20, 20);
}

// 0x415eb0
void OptionsDialog::Draw(Graphics* g)
{
	MoneyDialog::Draw(g);

	g->SetFont(mApp->mFont3);
	g->SetColorizeImages(true);
	g->SetColor(mColors[COLOR_LINES]);
	g->DrawString(mApp->GetString("OPTDLG_MUSIC"), 48, 88);
	g->DrawString(mApp->GetString("SOUND_FX"), 48, 120);
	g->DrawString(mApp->GetString("FULLSCREEN"), mFullscreenCheckbox->mX - mX + 28, mFullscreenCheckbox->mY - mY + 14);
	g->DrawString(mApp->GetString("CUSTOM_CURSORS"), mCustomCursorsCheckbox->mX - mX + 28, mCustomCursorsCheckbox->mY - mY + 14);
	g->SetColorizeImages(false);
}

// 0x416210
void OptionsDialog::ButtonDepress(int theId)
{
	// Done (footer) reaches WinBMApp::ButtonDepress(2001), which applies the checkboxes and closes
	Dialog::ButtonDepress(theId);

	if ((theId >= LINK_BUTTON_ID_BASE) && (theId < LINK_BUTTON_ID_BASE + 100))
	{
		// Shipped: OptionsLinkURL0 = steam://publisher/popcap
		mApp->OpenURL(mApp->GetString(StrFormat("OptionsLinkURL%d", theId - LINK_BUTTON_ID_BASE), "http://www.popcap.com"), false);
		return;
	}

	switch (theId)
	{
	case REGISTER_BUTTON_ID:
		// Only laid out in a locked build; registered, the app shows "already registered"
		mApp->ShowRegisterDialog();
		break;
	case SCORES_BUTTON_ID:
		// Opens on top of this dialog, which stays open
		mApp->ShowHighScoreDialog();
		break;
	case VISIT_POPCAP_BUTTON_ID:
		mApp->OpenURL("http://www.popcap.com", false);
		break;
	case HELP_BUTTON_ID:
		// Closes this dialog (KillDialog(1)) and shows the help screen
		mApp->ShowHelp();
		break;
	case CHECK_UPDATES_BUTTON_ID:
		// port: called DoUpdateCheck (0x420c50); update checks are not ported (the button is only laid out without DontUpdate)
		break;
	case CREDITS_BUTTON_ID:
		mApp->ShowCreditsDialog();
		break;
	}
}

// 0x416720
OptionsDialog::OptionsDialog(WinBMApp* theApp) :
	MoneyDialog(theApp, theApp->mImages[IMAGE_DIALOG], theApp->mImages[IMAGE_DIALOGBUTTON], DIALOG_OPTIONS, true,
		theApp->GetString("OPTIONS"), "", theApp->GetString("DONE"), BUTTONS_FOOTER)
{
	mApp = theApp;
	SetButtonFont(mApp->mFont3);
	SetHeaderFont(mApp->mFont1);
	SetLinesFont(mApp->mFont3);

	mRegisterButton = new DialogButton(mApp->mImages[IMAGE_DIALOGBUTTON], REGISTER_BUTTON_ID, this);
	mRegisterButton->SetFont(mApp->mFont3);
	mRegisterButton->mLabel = mApp->GetString("REGISTER");

	mScoresButton = new DialogButton(mApp->mImages[IMAGE_DIALOGBUTTON], SCORES_BUTTON_ID, this);
	mScoresButton->SetFont(mApp->mFont3);
	mScoresButton->mLabel = mApp->GetString("SCORES");

	mHelpButton = new DialogButton(mApp->mImages[IMAGE_DIALOGBUTTON], HELP_BUTTON_ID, this);
	mHelpButton->SetFont(mApp->mFont3);
	mHelpButton->mLabel = mApp->GetString("HELP");

	mCheckUpdatesButton = new DialogButton(mApp->mImages[IMAGE_DIALOGBUTTON], CHECK_UPDATES_BUTTON_ID, this);
	mCheckUpdatesButton->SetFont(mApp->mFont3);
	mCheckUpdatesButton->mLabel = mApp->GetString("CHECK_UPDATES");

	mCreditsButton = new DialogButton(mApp->mImages[IMAGE_DIALOGBUTTON], CREDITS_BUTTON_ID, this);
	mCreditsButton->SetFont(mApp->mFont3);
	mCreditsButton->mLabel = mApp->GetString("CREDITS");

	mMusicSlider = new Slider12(mApp->mImages[IMAGE_SLIDERTRACK], mApp->mImages[IMAGE_SLIDERTHUMB], MUSIC_SLIDER_ID, this);
	mMusicSlider->SetValue(mApp->GetMusicVolume());

	mSfxSlider = new Slider12(mApp->mImages[IMAGE_SLIDERTRACK], mApp->mImages[IMAGE_SLIDERTHUMB], SFX_SLIDER_ID, this);
	mSfxSlider->SetValue(mApp->GetSfxVolume());

	// SetChecked tells the listener, so CheckboxChecked already runs here
	mFullscreenCheckbox = new Checkbox(mApp->mCheckboxUncheckedImage, mApp->mCheckboxCheckedImage, FULLSCREEN_CHECKBOX_ID, this);
	mFullscreenCheckbox->SetChecked(!mApp->mIsWindowed, true);

	mCustomCursorsCheckbox = new Checkbox(mApp->mCheckboxUncheckedImage, mApp->mCheckboxCheckedImage, CUSTOM_CURSORS_CHECKBOX_ID, this);
	mCustomCursorsCheckbox->SetChecked(mApp->mCustomCursorsEnabled, true);

	// Both properties are read again for every test. partner.xml (Steam): one link, "More Games".
	for (int i = 0; i < mApp->GetInteger("NumOptionsLinks", mApp->GetBoolean("NoPopLink", false) ? 0 : 1); i++)
	{
		DialogButton* aLinkButton = new DialogButton(mApp->mImages[IMAGE_DIALOGBUTTON], LINK_BUTTON_ID_BASE + i, this);
		aLinkButton->SetFont(mApp->mFont3);
		aLinkButton->SetColor(DialogButton::COLOR_LABEL, Color(187, 255, 1));
		aLinkButton->mLabel = mApp->GetString(StrFormat("OptionsLinkText%d", i), "Visit PopCap.com For More Games!");
		mLinkButtons.push_back(aLinkButton);
	}
}
