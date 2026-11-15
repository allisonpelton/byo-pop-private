// WinDMAppOptions.cpp: the Options dialog and the How To Play screen as WinDMApp opens and closes them (DoOptions,
// FinishOptionsDialog, DoHelpScreen, FinishHelpScreen). Translated from the Ghidra export and the assembly.
// Not ported: the Automatic Update checkbox's effect on mAlwaysUpdate (FinishOptionsDialog).
#include "WinDMApp.h"

#include "Board.h"
#include "Checkbox.h"
#include "HelpScreen.h"
#include "MainMenu.h"
#include "OptionsDialog.h"
#include "Profile.h"
#include "Res.h"
#include "ResourceManager.h"

using namespace Sexy;

// 0x5dc173
void WinDMApp::DoOptions()
{
	OptionsDialog* aDialog = new OptionsDialog(this);
	SetupDialog(aDialog, 0x2aa);
	AddDialog(DIALOG_OPTIONS, aDialog);
}

// 0x5e1287: the dialog is done (Done or Back to Game, How To Play or Main Menu): the settings of its controls take effect
void WinDMApp::FinishOptionsDialog(bool theOk)
{
	OptionsDialog* aDialog = static_cast<OptionsDialog*>(GetDialog(DIALOG_OPTIONS));
	bool aWindowed = !aDialog->mFullscreenCheckbox->IsChecked();
	bool a3D = aDialog->m3DCheckbox->IsChecked();
	bool aHighRes = aDialog->mHighResCheckbox->IsChecked();
	if (!theOk || aHighRes == mHighRes)
	{
		SwitchScreenMode(aWindowed, a3D);
		if (a3D && !m3dImagesLoaded)
		{
			// the 3D pictures were not loaded at the start: load them now
			mResourceManager->StartLoadResources("3dImages");
			while (mResourceManager->LoadNextResource())
				;
			if (mResourceManager->HadError())
			{
				ShowResourceError(false);
				mLoadingFailed = true;
				return;
			}
			Extract3dImagesResources(mResourceManager);
			m3dImagesLoaded = true;
		}
	}
	else
	{
		// another resolution: the pictures are loaded again at its size
		mHighRes = aHighRes;
		DoReloading(aWindowed, a3D);
	}
	mProfile->mRandomBackdrops = aDialog->mBackdropsCheckbox->IsChecked();
	// (the Automatic Update checkbox set mAlwaysUpdate here; it is not ported)
	EnableCustomCursors(aDialog->mCursorsCheckbox->IsChecked());
	ClearUpdateBacklog(false);
	KillDialog(DIALOG_OPTIONS);
}

// 0x5dc44d: the board slides away while the screen shows, and the menu hides; a screen opened over a board that is not
// shown (the hint of the first game) asks "Don't Show Again" or "Continue" instead of only offering the way back
void WinDMApp::DoHelpScreen()
{
	bool aBoardShown = (mBoard != nullptr && mBoard->mScreenState == Board::SCREEN_SHOWN);
	HelpScreen* aScreen = new HelpScreen(this, mBoard == nullptr || aBoardShown);
	SetupDialog(aScreen, 0);
	AddDialog(DIALOG_HELP, aScreen);
	aScreen->Show();
	if (aBoardShown)
		mBoard->Hide();
	if (mMainMenu != nullptr)
		mMainMenu->Hide();
}

// 0x5dc501
void WinDMApp::FinishHelpScreen(bool theNeverShowAgain)
{
	HelpScreen* aScreen = static_cast<HelpScreen*>(GetDialog(DIALOG_HELP));
	if (theNeverShowAgain)
		mProfile->DisableHint(3);
	aScreen->Hide();
	if (mBoard == nullptr)
		mMainMenu->Show();
	else
		mBoard->Show();
	if (mBoard != nullptr)
	{
		// the wait before the gems fall in ends with the screen
		if (mBoard->mDropDelay > 0)
			mBoard->mDropDelay = 0;
	}
}
