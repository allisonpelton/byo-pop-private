#include "OptionsDialog.h"
#include "WinDMApp.h"
#include "Res.h"

#include "Graphics.h"
#include "ImageFont.h"
#include "DDImage.h"
#include "DialogButton.h"
#include "Slider.h"
#include "Checkbox.h"
#include "WidgetManager.h"

using namespace Sexy;

// 0x4d1227
void OptionsDialog::SliderVal(int theId, double theVal)
{
	if (theId == MUSIC_SLIDER_ID)
		mApp->SetMusicVolume(theVal);
	else if (theId == SFX_SLIDER_ID)
		mApp->SetSfxVolume(theVal);
}

// 0x4d1261
void OptionsDialog::CheckboxChecked(int theId, bool checked)
{
	if ((theId == FULLSCREEN_CHECKBOX_ID) && (!checked) && (mApp->mForceFullscreen))
	{
		mApp->ShowColorDepthDialog();
		mFullscreenCheckbox->SetChecked(true, true);
	}
}

// 0x4d1297
void OptionsDialog::ButtonPress(int theId)
{
	// The footer Done button also reaches WinDMApp::ButtonPress(2002), so it clicks twice
	Dialog::ButtonPress(theId);
	mApp->PlaySample(SOUND_MENUCLICK);
}

// 0x4d1424
int OptionsDialog::GetPreferredHeight(int theWidth)
{
	return (int) mLinkButtons.size() * LINK_BUTTON_PITCH + BASE_PREFERRED_HEIGHT;
}

// 0x4d143a
void OptionsDialog::AddedToManager(WidgetManager* theWidgetManager)
{
	Dialog::AddedToManager(theWidgetManager);
	theWidgetManager->AddWidget(mHelpButton);
	theWidgetManager->AddWidget(mRegisterButton);
	theWidgetManager->AddWidget(mHighScoresButton);
	theWidgetManager->AddWidget(mMusicVolumeSlider);
	theWidgetManager->AddWidget(mSfxVolumeSlider);
	theWidgetManager->AddWidget(mCustomCursorsCheckbox);
	theWidgetManager->AddWidget(mFullscreenCheckbox);
	theWidgetManager->AddWidget(mCheckUpdatesButton);
	for (int i = 0; i < (int) mLinkButtons.size(); i++)
		theWidgetManager->AddWidget(mLinkButtons[i]);
}

// 0x4d14f3
void OptionsDialog::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Dialog::RemovedFromManager(theWidgetManager);
	theWidgetManager->RemoveWidget(mHelpButton);
	theWidgetManager->RemoveWidget(mRegisterButton);
	theWidgetManager->RemoveWidget(mHighScoresButton);
	theWidgetManager->RemoveWidget(mMusicVolumeSlider);
	theWidgetManager->RemoveWidget(mSfxVolumeSlider);
	theWidgetManager->RemoveWidget(mFullscreenCheckbox);
	theWidgetManager->RemoveWidget(mCustomCursorsCheckbox);
	theWidgetManager->RemoveWidget(mCheckUpdatesButton);
	for (int i = 0; i < (int) mLinkButtons.size(); i++)
		theWidgetManager->RemoveWidget(mLinkButtons[i]);
}

// 0x4d15ac
void OptionsDialog::Resize(int theX, int theY, int theWidth, int theHeight)
{
	Dialog::Resize(theX, theY, theWidth, theHeight);

	// port: the build is always unlocked, so the register button is never laid out (0x0, invisible);
	// a locked build gave help, register and high scores (mWidth-60)/3 px each
	int aButtonWidth = (mWidth - 54) / 2;
	mHelpButton->Resize(mX + 24, mY + 166, aButtonWidth, 32);
	mHighScoresButton->Resize(mX + 30 + aButtonWidth, mY + 166, aButtonWidth, 32);

	// The links stack up from just above the Done button
	for (int i = 0; i < (int) mLinkButtons.size(); i++)
		mLinkButtons[i]->Resize(mX + 24, (i - (int) mLinkButtons.size()) * LINK_BUTTON_PITCH + mYesButton->mY, mWidth - 48, 32);

	mMusicVolumeSlider->Resize(mX + 136, mY + 60, mWidth - 160, 32);
	mSfxVolumeSlider->Resize(mX + 136, mY + 92, mWidth - 160, 32);
	mFullscreenCheckbox->Resize(mX + 32, mY + 124, 32, 32);
	mCustomCursorsCheckbox->Resize(mX + 146, mY + 124, 32, 32);
	// mCheckUpdatesButton is never laid out either
}

// 0x4d18f6
OptionsDialog::~OptionsDialog()
{
	delete mHelpButton;
	delete mRegisterButton;
	delete mHighScoresButton;
	delete mMusicVolumeSlider;
	delete mSfxVolumeSlider;
	for (int i = 0; i < (int) mLinkButtons.size(); i++)
		delete mLinkButtons[i];
	delete mCheckUpdatesButton;
	delete mFullscreenCheckbox;
	delete mCustomCursorsCheckbox;
	delete mCheckboxCheckedImage;
	delete mCheckboxUncheckedImage;
}

// 0x4d1a8f
void OptionsDialog::Draw(Graphics* g)
{
	Dialog::Draw(g);

	g->SetFont(mApp->mMainFont);
	g->SetColorizeImages(true);
	g->SetColor(Color(255, 255, 0));
	g->DrawString(mApp->GetString("OPTIONSDLG_MUSIC"), 32, 80);
	g->DrawString(mApp->GetString("OPTIONSDLG_SOUND_FX"), 32, 112);
	g->DrawString(mApp->GetString("OPTIONSDLG_FULLSCREEN"), mFullscreenCheckbox->mX - mX + 34, mFullscreenCheckbox->mY - mY + 21);
	g->DrawString(mApp->GetString("OPTIONSDLG_CUSTOM_CURSORS"), mCustomCursorsCheckbox->mX - mX + 34, mCustomCursorsCheckbox->mY - mY + 21);
	g->SetColorizeImages(false);
}

// 0x4d1c58
void OptionsDialog::ButtonDepress(int theId)
{
	// Done (footer) reaches WinDMApp::ButtonDepress(2002), which applies the checkboxes
	Dialog::ButtonDepress(theId);

	if ((theId >= LINK_BUTTON_ID_BASE) && (theId < LINK_BUTTON_ID_BASE + 100))
		mApp->OpenURL(mApp->GetString(StrFormat("OptionsLinkURL%d", theId - LINK_BUTTON_ID_BASE), _S("http://www.popcap.com")), false);
	else if (theId == HELP_BUTTON_ID)
		mApp->ShowFullInstructionsDialog();
	else if (theId == REGISTER_BUTTON_ID)
		mApp->ShowRegisterDialog();
	else if (theId == HIGH_SCORES_BUTTON_ID)
		mApp->ShowHighScoreDialog();
	else if (theId == VISIT_POPCAP_BUTTON_ID)
		mApp->OpenURL("http://www.popcap.com", false);
	// port: CHECK_UPDATES_BUTTON_ID called DoUpdateCheck(); update checks are not ported (the button is never laid out)
}

// 0x4d1fda
OptionsDialog::OptionsDialog(WinDMApp* theApp) :
	Dialog(theApp->mImages[IMAGE_DIALOGBOX], theApp->mImages[IMAGE_BOXBUTTON], DIALOG_OPTIONS, true,
		theApp->GetString("OPTIONSDLG_OPTIONS"), _S(""), theApp->GetString("OPTIONSDLG_DONE"), BUTTONS_FOOTER)
{
	mApp = theApp;
	mCheckboxCheckedImage = mApp->CopyImage(mApp->mImages[IMAGE_CHECKBOX], Rect(0, 0, 31, 32));
	mCheckboxUncheckedImage = mApp->CopyImage(mApp->mImages[IMAGE_CHECKBOX], Rect(31, 0, 31, 32));

	SetButtonFont(mApp->mMainFont);
	SetHeaderFont(mApp->mHeaderFont);
	SetLinesFont(mApp->mMainFont);

	mHelpButton = new DialogButton(mApp->mImages[IMAGE_BOXBUTTON], HELP_BUTTON_ID, this);
	mHelpButton->SetFont(mApp->mMainFont);
	mHelpButton->mLabel = mApp->GetString("OPTIONSDLG_HELP");

	mRegisterButton = new DialogButton(mApp->mImages[IMAGE_BOXBUTTON], REGISTER_BUTTON_ID, this);
	mRegisterButton->SetFont(mApp->mMainFont);
	mRegisterButton->mLabel = mApp->GetString("OPTIONSDLG_REGISTER");

	mHighScoresButton = new DialogButton(mApp->mImages[IMAGE_BOXBUTTON], HIGH_SCORES_BUTTON_ID, this);
	mHighScoresButton->SetFont(mApp->mMainFont);
	mHighScoresButton->mLabel = mApp->GetString("OPTIONSDLG_HIGH_SCORES");

	// partner.xml: one link, "Play More Great Games!"
	for (int i = 0; i < mApp->GetInteger("NumOptionsLinks", 1); i++)
	{
		DialogButton* aLinkButton = new DialogButton(mApp->mImages[IMAGE_BOXBUTTON], LINK_BUTTON_ID_BASE + i, this);
		aLinkButton->SetFont(mApp->mMainFont);
		aLinkButton->mLabel = mApp->GetString(StrFormat("OptionsLinkText%d", i), _S("Visit PopCap.com For More Games"));
		mLinkButtons.push_back(aLinkButton);
	}

	mCheckUpdatesButton = new DialogButton(mApp->mImages[IMAGE_BOXBUTTON], CHECK_UPDATES_BUTTON_ID, this);
	mCheckUpdatesButton->SetFont(mApp->mMainFont);
	mCheckUpdatesButton->mLabel = mApp->GetString("OPTIONSDLG_CHECK_FOR_UPDATES");

	mMusicVolumeSlider = new Slider(mApp->mImages[IMAGE_SLIDER_TRACK], mApp->mImages[IMAGE_SLIDER_THUMB], MUSIC_SLIDER_ID, this);
	mMusicVolumeSlider->SetValue(mApp->GetMusicVolume());

	mSfxVolumeSlider = new Slider(mApp->mImages[IMAGE_SLIDER_TRACK], mApp->mImages[IMAGE_SLIDER_THUMB], SFX_SLIDER_ID, this);
	mSfxVolumeSlider->SetValue(mApp->GetSfxVolume());

	// SetChecked tells the listener, so a forced-fullscreen app can already pop the color depth dialog here
	mFullscreenCheckbox = new Checkbox(mCheckboxUncheckedImage, mCheckboxCheckedImage, FULLSCREEN_CHECKBOX_ID, this);
	mFullscreenCheckbox->SetChecked(!mApp->mIsWindowed, true);

	mCustomCursorsCheckbox = new Checkbox(mCheckboxUncheckedImage, mCheckboxCheckedImage, CUSTOM_CURSORS_CHECKBOX_ID, this);
	mCustomCursorsCheckbox->SetChecked(mApp->mCustomCursorsEnabled, true);
}
