#include "OptionsDialog.h"
#include "WinBMApp.h"
#include "Res.h"
#include "ModRules.h"
#include "Compat12.h"

#include "Graphics.h"
#include "ImageFont.h"
#include "DialogButton.h"
#include "Checkbox.h"
#include "WidgetManager.h"
#include "platform/ModManager.h"

using namespace Sexy;

// 0x415560
void OptionsDialog::SetColor(int theIdx, const Color& theColor)
{
	// The button text colour also goes to the buttons (the links keep their own)
	Dialog::SetColor(theIdx, theColor);
	if (theIdx == COLOR_BUTTON_TEXT)
	{
		if (mModsButton != NULL)
			mModsButton->SetColor(DialogButton::COLOR_LABEL, theColor);
		if (mScoresButton != NULL)
			mScoresButton->SetColor(DialogButton::COLOR_LABEL, theColor);
		if (mHelpButton != NULL)
			mHelpButton->SetColor(DialogButton::COLOR_LABEL, theColor);
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
	{
		// port(mods): the fixes mod's "doneClicksOnce": the app plays the click for the dialog's own two buttons
		// (the ids the check above was meant to skip)
		if (((theId == Dialog::ID_YES) || (theId == Dialog::ID_NO)) && (ModFeatureBool("fixes", "doneClicksOnce", false)))
			return;
		mApp->PlaySample(SOUND_BUTTONCLICK);
	}
}

// 0x4156b0
int OptionsDialog::GetPreferredHeight(int theWidth)
{
	// The original adds ROW_PITCH per partner link (342 with Steam's one); the links are not ported
	return BASE_PREFERRED_HEIGHT;
}

// 0x415770
void OptionsDialog::AddedToManager(WidgetManager* theWidgetManager)
{
	Dialog::AddedToManager(theWidgetManager);
	if (mModsButton != NULL)
		theWidgetManager->AddWidget(mModsButton);
	theWidgetManager->AddWidget(mScoresButton);
	theWidgetManager->AddWidget(mHelpButton);
	theWidgetManager->AddWidget(mMusicSlider);
	theWidgetManager->AddWidget(mSfxSlider);
	theWidgetManager->AddWidget(mCustomCursorsCheckbox);
	theWidgetManager->AddWidget(mFullscreenCheckbox);
	theWidgetManager->AddWidget(mCreditsButton);
}

// 0x415830
void OptionsDialog::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Dialog::RemovedFromManager(theWidgetManager);
	if (mModsButton != NULL)
		theWidgetManager->RemoveWidget(mModsButton);
	theWidgetManager->RemoveWidget(mScoresButton);
	theWidgetManager->RemoveWidget(mHelpButton);
	theWidgetManager->RemoveWidget(mMusicSlider);
	theWidgetManager->RemoveWidget(mSfxSlider);
	theWidgetManager->RemoveWidget(mFullscreenCheckbox);
	theWidgetManager->RemoveWidget(mCustomCursorsCheckbox);
	theWidgetManager->RemoveWidget(mCreditsButton);
}

// 0x415970
OptionsDialog::~OptionsDialog()
{
	delete mModsButton;
	delete mScoresButton;
	delete mCreditsButton;
	delete mMusicSlider;
	delete mSfxSlider;
	delete mFullscreenCheckbox;
	delete mCustomCursorsCheckbox;
	delete mHelpButton;
}

// 0x415ad0
void OptionsDialog::Resize(int theX, int theY, int theWidth, int theHeight)
{
	// Places Done (mYesButton) at the bottom
	Dialog::Resize(theX, theY, theWidth, theHeight);

	int aLeft = mContentInsets.mLeft;
	int anInnerWidth = mWidth - aLeft - mContentInsets.mRight;

	// Two rows of buttons above Done; a button that is not laid out stays 0x0 (invisible). The original
	// stacks the partner links (not ported) up from just above Done first, one row each, and puts these
	// rows above them; the dialog is that much shorter, so everything else keeps its place from the top.
	int aRowY = mYesButton->mY - ROW_PITCH;
	int aHalfWidth = (anInnerWidth - 3) / 2;
	// The registered Steam original (DontUpdate): Help + Credits, then Scores across the full width
	mHelpButton->Resize(mX + aLeft, aRowY, aHalfWidth, BUTTON_HEIGHT);
	mCreditsButton->Resize(mX + aLeft + 3 + aHalfWidth, aRowY, aHalfWidth, BUTTON_HEIGHT);
	aRowY -= ROW_PITCH;

	if (mModsButton != NULL) // port(mods): MODS and Scores share the row
	{
		mModsButton->Resize(mX + aLeft, aRowY, aHalfWidth, BUTTON_HEIGHT);
		mScoresButton->Resize(mX + aLeft + 3 + aHalfWidth, aRowY, aHalfWidth, BUTTON_HEIGHT);
	}
	else
		mScoresButton->Resize(mX + aLeft, aRowY, anInnerWidth, BUTTON_HEIGHT);

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

	// The original opens partner link i's OptionsLinkURL%d for ids 100..199 (Steam: a store page); the
	// links are not ported

	switch (theId)
	{
	case MODS_BUTTON_ID:
		// port(mods): opens the mod menu over this dialog, which stays open
		mApp->OpenModMenu();
		break;
	case SCORES_BUTTON_ID:
		// Opens on top of this dialog, which stays open
		mApp->ShowHighScoreDialog();
		break;
	case HELP_BUTTON_ID:
		// Closes this dialog (KillDialog(1)) and shows the help screen
		mApp->ShowHelp();
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

	// port(mods): a MODS button while an installed mod (on or off) has settings
	bool hasModSettings = false;
	for (const Platform::ModInfo& aMod : Platform::ModManager::Get().GetMods())
		hasModSettings = hasModSettings || aMod.HasSettings();
	mModsButton = NULL;
	if (hasModSettings)
	{
		mModsButton = new DialogButton(mApp->mImages[IMAGE_DIALOGBUTTON], MODS_BUTTON_ID, this);
		mModsButton->SetFont(mApp->mFont3);
		mModsButton->mLabel = mApp->GetString("OPTIONS_MODS", _S("MODS"));
	}

	mScoresButton = new DialogButton(mApp->mImages[IMAGE_DIALOGBUTTON], SCORES_BUTTON_ID, this);
	mScoresButton->SetFont(mApp->mFont3);
	mScoresButton->mLabel = mApp->GetString("SCORES");

	mHelpButton = new DialogButton(mApp->mImages[IMAGE_DIALOGBUTTON], HELP_BUTTON_ID, this);
	mHelpButton->SetFont(mApp->mFont3);
	mHelpButton->mLabel = mApp->GetString("HELP");

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

	// port: the original ends by making one button per partner link (properties NumOptionsLinks, or one
	// unless NoPopLink, with labels OptionsLinkText%d; Steam's partner.xml has one). The links are not ported
	// (the user's decision, 2026-10-24).
}
