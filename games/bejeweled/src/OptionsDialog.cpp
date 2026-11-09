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
#include "platform/ModManager.h"

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
		mApp->DoColorDepthExplanation();
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
	return PREFERRED_HEIGHT;
}

// 0x4d143a
void OptionsDialog::AddedToManager(WidgetManager* theWidgetManager)
{
	Dialog::AddedToManager(theWidgetManager);
	theWidgetManager->AddWidget(mHelpButton);
	theWidgetManager->AddWidget(mHighScoresButton);
	theWidgetManager->AddWidget(mMusicVolumeSlider);
	theWidgetManager->AddWidget(mSfxVolumeSlider);
	theWidgetManager->AddWidget(mCustomCursorsCheckbox);
	theWidgetManager->AddWidget(mFullscreenCheckbox);
	if (mModsButton != NULL) // port(mods)
		theWidgetManager->AddWidget(mModsButton);
	if (mExtrasButton != NULL) // port(variant)
		theWidgetManager->AddWidget(mExtrasButton);
}

// 0x4d14f3
void OptionsDialog::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Dialog::RemovedFromManager(theWidgetManager);
	theWidgetManager->RemoveWidget(mHelpButton);
	theWidgetManager->RemoveWidget(mHighScoresButton);
	theWidgetManager->RemoveWidget(mMusicVolumeSlider);
	theWidgetManager->RemoveWidget(mSfxVolumeSlider);
	theWidgetManager->RemoveWidget(mFullscreenCheckbox);
	theWidgetManager->RemoveWidget(mCustomCursorsCheckbox);
	if (mModsButton != NULL) // port(mods)
		theWidgetManager->RemoveWidget(mModsButton);
	if (mExtrasButton != NULL) // port(variant)
		theWidgetManager->RemoveWidget(mExtrasButton);
}

// 0x4d15ac
void OptionsDialog::Resize(int theX, int theY, int theWidth, int theHeight)
{
	Dialog::Resize(theX, theY, theWidth, theHeight);

	// The row of Help and High Scores sits in the middle of the space that the original's "more games" link left
	// between it and the Done button (the link is taken out): 12 px lower than the original's row, which stood above
	// the link
	int aButtonWidth = (mWidth - 54) / 2;
	mHelpButton->Resize(mX + 24, mY + ROW_Y, aButtonWidth, 32);
	mHighScoresButton->Resize(mX + 30 + aButtonWidth, mY + ROW_Y, aButtonWidth, 32);
	DialogButton* aSlotButton = (mExtrasButton != NULL) ? mExtrasButton : mModsButton;
	if (aSlotButton != NULL)
	{
		// port(mods): Mods (port(variant): or Extras) as a third button in the row
		aButtonWidth = (mWidth - 60) / 3;
		mHelpButton->Resize(mX + 24, mY + ROW_Y, aButtonWidth, 32);
		aSlotButton->Resize(mX + 30 + aButtonWidth, mY + ROW_Y, aButtonWidth, 32);
		mHighScoresButton->Resize(mX + 36 + 2 * aButtonWidth, mY + ROW_Y, aButtonWidth, 32);
	}

	mMusicVolumeSlider->Resize(mX + 136, mY + 60, mWidth - 160, 32);
	mSfxVolumeSlider->Resize(mX + 136, mY + 92, mWidth - 160, 32);
	mFullscreenCheckbox->Resize(mX + 32, mY + 124, 32, 32);
	mCustomCursorsCheckbox->Resize(mX + 146, mY + 124, 32, 32);
}

// 0x4d18f6
OptionsDialog::~OptionsDialog()
{
	delete mHelpButton;
	delete mHighScoresButton;
	delete mMusicVolumeSlider;
	delete mSfxVolumeSlider;
	delete mFullscreenCheckbox;
	delete mCustomCursorsCheckbox;
	delete mCheckboxCheckedImage;
	delete mCheckboxUncheckedImage;
	delete mModsButton; // port(mods)
	delete mExtrasButton; // port(variant)
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

	if (theId == HELP_BUTTON_ID)
		mApp->DoFullInstructions();
	else if (theId == HIGH_SCORES_BUTTON_ID)
		mApp->DoHighScoreDialog();
	else if (theId == MODS_BUTTON_ID)
		mApp->OpenModMenu(); // port(mods): over this dialog, which stays open
	else if (theId == EXTRAS_BUTTON_ID)
		mApp->ShowExtrasDialog(); // port(variant): over this dialog, which stays open
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

	mHighScoresButton = new DialogButton(mApp->mImages[IMAGE_BOXBUTTON], HIGH_SCORES_BUTTON_ID, this);
	mHighScoresButton->SetFont(mApp->mMainFont);
	mHighScoresButton->mLabel = mApp->GetString("OPTIONSDLG_HIGH_SCORES");

	// port(mods): Mods while an enabled mod has settings. Not an installed one that is off, as in Big Money: the
	// oracle's examples case has example-settings installed but off, and its Options screens stay the original.
	mModsButton = NULL;
	mExtrasButton = NULL;
	if (mApp->HasExtras())
	{
		// port(variant): Extras, with Mods inside it
		mExtrasButton = new DialogButton(mApp->mImages[IMAGE_BOXBUTTON], EXTRAS_BUTTON_ID, this);
		mExtrasButton->SetFont(mApp->mMainFont);
		mExtrasButton->mLabel = mApp->GetString("OPTIONSDLG_EXTRAS", _S("Extras"));
	}
	for (const Platform::ModInfo* aMod : Platform::ModManager::Get().GetEnabledMods())
	{
		if ((mExtrasButton == NULL) && aMod->HasSettings())
		{
			mModsButton = new DialogButton(mApp->mImages[IMAGE_BOXBUTTON], MODS_BUTTON_ID, this);
			mModsButton->SetFont(mApp->mMainFont);
			mModsButton->mLabel = mApp->GetString("OPTIONSDLG_MODS", _S("Mods"));
			break;
		}
	}

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
