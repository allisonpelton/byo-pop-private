#include "FossilizedApp.h"
#include "GameText.h"
#include "Dynomite.h"
#include "DynomitePanic.h"
#include "DynomiteFossil.h"
#include "DynomiteStomped.h"
#include "DynomiteTimeTrial.h"
#include "CreditsDialog.h"
#include "EditWidget.h"
#include "EnterNameDialog.h"
#include "HighScores.h"
#include "HighScoresDialog.h"
#include "OptionsDialog.h"
#include "Checkbox.h"
#include "SideBar.h"
#include "AngleTable.h"
#include "InchySmall.h"
#include "IntroScreen.h"
#include "TitleScreen.h"
#include "Tutorial.h"
#include "compat2004/Compat2004.h"
#include "DDImage.h"
#include "Dialog.h"
#include "ImageFont.h"
#include "MusicInterface.h"
#include "SoundManager.h"
#include "WidgetManager.h"
#include <SDL.h>

using namespace Sexy;

// 0x4198b0
FossilizedApp::FossilizedApp()
{
	mProdName = "Dynomite";
	mTitle = "Dynomite Deluxe";
	mProductVersion = "2.71";
	mRegKey = "PopCap\\Dynomite";
	mSoundInitialized = false;
	mWidth = 640;
	mHeight = 480;
	mMusicVolume = 0.5;
	mSfxVolume = 0.85;
	mAngles = new AngleTable();
	mRand.SRand(GetTickCount());
	mPicker.Seed(GetTickCount() >> 1);
	mSkill = 1;
	mFossilKind = 0;
	mGameMode = 2;
	mInTutorial = false;
	mTutor = NULL;
	mSideBar = NULL;
	mIntroScreen = NULL;
	// port: the original leaves these to its allocation
	mField1090 = 0;
	mLockCursor = false;
	mField4d8 = 0;
	mPackFile[0] = 0;
	mPuzzleSkills = 0;
	mBoard = NULL;
	mTitleScreen = NULL;
	mImgTitle = NULL;
	mTinyFont = NULL;
	mGrindInstance = NULL;
	mSound1a50 = NULL;
	mFootfallRumbleInstance = NULL;
	mTicktockInstance = NULL;
}

// 0x419320
FossilizedApp::~FossilizedApp()
{
	if (mAngles != NULL)
	{
		delete mAngles;
		mAngles = NULL;
	}
	if (mTitleScreen != NULL)
	{
		mWidgetManager->RemoveWidget(mTitleScreen);
		delete mTitleScreen;
	}
}

// 0x419ad0
DDImage* FossilizedApp::GetGameImage(const std::string& thePath)
{
	DDImage* anImage = GetImage(thePath);
	if (anImage == NULL)
	{
		Popup("Failed to load image:" + thePath);
		mLoadingFailed = true;
		return NULL;
	}
	anImage->Palletize();
	return anImage;
}

// 0x41f820
void FossilizedApp::Init()
{
	SexyApp::Init();
	// port: SexyApp::Init seeds the C library generator with the tick count, and the constructor seeds the app's
	// own two with it; -seed=N makes a test run repeatable
	CrtSRand(mHasSeedParam ? (unsigned int) mRandSeed : (unsigned int) GetTickCount());
	if (mHasSeedParam)
	{
		mRand.SRand((unsigned int) mRandSeed);
		mPicker.Seed((unsigned int) mRandSeed >> 1);
	}
	LoadProperties();
	// port: the few texts the original keeps in its exe come from the player's Dynomite.exe (GameText.h)
	LoadGameTexts();
	DIALOG_YES_STRING = GetString("DIALOG_BUTTON_YES");
	DIALOG_NO_STRING = GetString("DIALOG_BUTTON_NO");
	DIALOG_OK_STRING = GetString("DIALOG_BUTTON_OK");
	DIALOG_CANCEL_STRING = GetString("DIALOG_BUTTON_CANCEL");
	mImgTitle = GetGameImage("images\\Title");
	mImgGreenbar = GetGameImage("images\\greenbar");
	mWhiteFont = new ImageFont(this, "data\\whitefont.txt");
	mWhiteFontSmall = new ImageFont(this, "data\\whitefontsmall.txt");
	mWhiteFont2 = new ImageFont(this, "data\\WhiteFont2.txt");
	mPicUnregistered = new Pic(this, "images\\unregistered", true);
	mSoundInitialized = mSoundManager->Initialized();
	if (!mSoundInitialized)
		SDL_Log("Sound manager could not initialize");	// a message box in the original
	mSndClick = new GameSound(this, "sounds\\click");
	mSndEgglands = new GameSound(this, "sounds\\egglands");
	mPicDialog = new Pic(this, "images\\dialog", false);
	mPicDbutton = new Pic(this, "images\\dbutton", false);
	mPicWaitbar = new Pic(this, "images\\waitbar", false);
	mTinyFont = NULL;
	mTitleScreen = new TitleScreen(this);
	mTitleScreen->Resize(0, 0, mWidth, mHeight);
	mWidgetManager->AddWidget(mTitleScreen);
	mMusicInterface->LoadMusic(0, "music\\amazonas.it");
	mMusicInterface->LoadMusic(1, "music\\omniphilia.it");
	mMusicInterface->LoadMusic(2, "music\\amazonas.it");
	mMusicInterface->LoadMusic(4, "music\\amazonas.it");
	mMusicInterface->PlayMusic(2, 0x16, false);
	mPicCursor1 = new Pic(this, "images\\cursor1", true);
	mCompletedLoadingThreadTasks++;
	mPicPointer2 = new Pic(this, "images\\pointer2", true);
	mCompletedLoadingThreadTasks++;
	mPicOverhand = new Pic(this, "images\\overhand", true);
	mCompletedLoadingThreadTasks++;
	mPicTyper = new Pic(this, "images\\typer", true);
	mCompletedLoadingThreadTasks++;
	SetCursorImage(0, mPicCursor1->mImage);
	SetCursorImage(1, mPicPointer2->mImage);
	SetCursorImage(2, mPicOverhand->mImage);
	SetCursorImage(3, mPicTyper->mImage);
	if (mLoadingFailed)
		Shutdown();
}

// 0x419250: the title screen goes; the side bar (0x8c bytes, 150 x 480) and the main menu come
void FossilizedApp::TitleScreenDone()
{
	mWidgetManager->RemoveWidget(mTitleScreen);
	SafeDeleteWidget(mTitleScreen);
	mTitleScreen = NULL;
	mSideBar = new SideBar(this);
	mSideBar->Resize(0, 0, 0x96, 0x1e0);
	mWidgetManager->AddWidget(mSideBar);
	mSideBar->MarkDirty();
	ShowMainMenu();
}

// 0x4186d0
void FossilizedApp::ShowMainMenu()
{
	if (mBoard != NULL)
	{
		mWidgetManager->RemoveWidget(mBoard);
		SafeDeleteWidget(mBoard);
		mBoard = NULL;
	}
	if (mTutor != NULL)
	{
		mWidgetManager->RemoveWidget(mTutor);
		SafeDeleteWidget(mTutor);
		mTutor = NULL;
	}
	mSideBar->DisableGameButtons();
	mSideBar->SetPauseState(0);
	mIntroScreen = new IntroScreen(this);
	mIntroScreen->Resize(0x96, 0, 0x1ea, 0x1e0);
	mWidgetManager->AddWidget(mIntroScreen);
	mWidgetManager->BringToBack(mIntroScreen);
	mSideBar->mInchy->Sink(0);
}

// 0x418a00 (slot 18): the game's dialog, in the white small font on its dialog pictures
Dialog* FossilizedApp::NewDialog(int theDialogId, bool isModal, const SexyString& theDialogHeader,
	const SexyString& theDialogLines, const SexyString& theDialogFooter, int theButtonMode)
{
	Dialog* aDialog = new Dialog2004(mPicDialog->mImage, mPicDbutton->mImage, theDialogId, isModal, theDialogHeader,
		theDialogLines, theDialogFooter, theButtonMode);
	aDialog->SetButtonFont(mWhiteFontSmall);
	aDialog->SetHeaderFont(mWhiteFontSmall);
	aDialog->SetLinesFont(mWhiteFontSmall);
	aDialog->SetColor(1, Color(0xff, 0xff, 0xff));
	aDialog->SetColor(3, Color(0xff, 0xff, 0xff));
	aDialog->mContentInsets = Insets(0x26, 0x26, 0x26, 0x26);
	aDialog->Resize(0xdc, 0x96, 0x170, aDialog->GetPreferredHeight(0x15c));
	return aDialog;
}

// 0x41e770: the side bar's menu button; in a game it asks first (dialog 1)
void FossilizedApp::MenuButton()
{
	if (mInTutorial == true)
	{
		ShowMainMenu();
		mInTutorial = false;
		return;
	}
	if (mBoard != NULL)
	{
		if (mBoard->mField131 == true)
		{
			ShowMainMenu();
			return;
		}
		DoDialog(1, true, GetString("FOSSAPP_NEW_GAME"), GetString("ARE_YOU_SURE_YOU_WANT_TO_QUIT_THE_CURRENT_GAME"), "",
			Dialog::BUTTONS_YES_NO);
	}
}

// 0x41e590: dialog 0
void FossilizedApp::ConfirmExit()
{
	DoDialog(0, true, GetString("EXIT_GAME"), GetString("ARE_YOU_SURE_YOU_WANT_TO_EXIT_DYNOMITE"), "",
		Dialog::BUTTONS_YES_NO);
}

// 0x418ba0: the options dialog (2)
void FossilizedApp::ShowOptions()
{
	OptionsDialog* aDialog = new OptionsDialog(this);
	aDialog->SetColor(0, Color(0xff, 0xff, 0xff));
	aDialog->SetColor(1, Color(0xff, 0xff, 0xff));
	aDialog->SetColor(3, Color(0xff, 0xff, 0xff));
	aDialog->mContentInsets = Insets(0x26, 0x26, 0x26, 0x26);
	aDialog->Resize(0xcd, 0x32, 0x17c, aDialog->GetPreferredHeight(0x17c));
	AddDialog(2, aDialog);
}

// 0x418cd0: the options dialog's high scores (4)
void FossilizedApp::ShowHighScores()
{
	HighScoresDialog* aDialog = new HighScoresDialog(this);
	aDialog->Resize(0xb5, 0x32, 0x1ac, aDialog->GetPreferredHeight(0x1ac));
	AddDialog(4, aDialog);
}

// 0x418d60: the options dialog's credits (22)
void FossilizedApp::ShowCredits()
{
	CreditsDialog* aDialog = new CreditsDialog(this);
	aDialog->Resize(0xdc, 0x3c, 0x15e, aDialog->GetPreferredHeight(0x1ac));
	AddDialog(0x16, aDialog);
}

// 0x41e940: dialog 3, where the desktop allows only full screen
void FossilizedApp::ShowNoWindowed()
{
	DoDialog(3, true, GetString("NO_WINDOWED"), GetString("WINDOWED_MODE_IS_ONLY_AVAILABLE_IF_YOUR_DESKTOP__"),
		GetString("OK"), Dialog::BUTTONS_FOOTER);
}

// 0x419130: a picture of the screen for the save dialog, then the save dialog
void FossilizedApp::SaveGame()
{
	// TODO: SaveGameDialog (0x433910, 0x2d0 bytes)
}

// 0x418810: the chosen mode's board in place of the main menu (or the board before)
void FossilizedApp::StartGame()
{
	if (mIntroScreen != NULL)
	{
		mWidgetManager->RemoveWidget(mIntroScreen);
		SafeDeleteWidget(mIntroScreen);
		mIntroScreen = NULL;
	}
	if (mBoard != NULL)
	{
		mWidgetManager->RemoveWidget(mBoard);
		mBoard->SetActive(false);
		SafeDeleteWidget(mBoard);
		mBoard = NULL;	// port: the original keeps the deleted board's pointer until the new one
	}
	Dynomite* aBoard;
	int aSkill = mSkill;
	switch (mGameMode)
	{
	case 1:
		aBoard = new DynomiteStomped();
		break;
	case 2:
		aBoard = new DynomitePanic();
		break;
	case 3:
		// the fossil challenge starts with its kind for the skill
		aBoard = new DynomiteFossil();
		aSkill = mFossilKind;
		break;
	case 4:
		aBoard = new DynomiteTimeTrial();
		break;
	default:
		mSideBar->GameStarted();
		return;
	}
	aBoard->Resize(0x96, 0, 0x1ea, 0x1e0);
	aBoard->Init(this, aSkill);
	mWidgetManager->AddWidget(aBoard);
	mWidgetManager->BringToBack(aBoard);
	mWidgetManager->SetFocus(aBoard);
	mBoard = aBoard;
	mSideBar->GameStarted();
}

// 0x418470
void FossilizedApp::ShowBuyDialog()
{
	// TODO: the unregistered copy's dialog 21 (0x4411d0); the port runs as a registered copy
}

// 0x4185a0
void FossilizedApp::ShowRegisterDialog(const char* theLines)
{
	// TODO: the unregistered copy's dialog 14 (0x432740); the port runs as a registered copy
}

// 0x418df0
void FossilizedApp::ShowHighScoreDialog(HighScores* theHighScores)
{
	EnterNameDialog* aDialog = new EnterNameDialog(this, theHighScores);
	aDialog->Resize(0xd3, 0xe1, 0x177, aDialog->GetPreferredHeight(0x177));
	AddDialog(6, aDialog);
}

// 0x41e250
void FossilizedApp::ReadFromRegistry()
{
	SexyApp::ReadFromRegistry();
	// port: the original opens HKLM\SOFTWARE\<mRegKey> itself; the framework's registry is the same key
	int aValue;
	if (RegistryReadInteger("LockCursor", &aValue))
		mLockCursor = aValue != 0;
	if (RegistryReadInteger("FossilGameType", &aValue))
		mFossilKind = aValue;
	if (RegistryReadInteger("SkillLevel", &aValue))
		mSkill = aValue;
	if (RegistryReadInteger("GameType", &aValue))
		mGameMode = aValue;
	if (RegistryReadInteger("StompedLevel", &aValue))
		mField1090 = aValue;
	std::string aName;
	if (RegistryReadString("LastScoreName", &aName))
		mLastScoreName = aName;
}

// 0x41e420
void FossilizedApp::WriteToRegistry()
{
	SexyApp::WriteToRegistry();
	RegistryWriteInteger("LockCursor", mLockCursor);
	RegistryWriteInteger("FossilGameType", mFossilKind);
	RegistryWriteInteger("SkillLevel", mSkill);
	RegistryWriteInteger("GameType", mGameMode);
	RegistryWriteInteger("StompedLevel", mField1090);
	RegistryWriteInteger("StompedLevel", mField1090);
	if (mLastScoreName.length() != 0)
		RegistryWriteString("LastScoreName", mLastScoreName);
}

// 0x4201b0
void FossilizedApp::ButtonDepress(int theId)
{
	if (theId < 0x1f5)
	{
		if (theId == 500)
		{
			// TODO: the save dialog's OK (dialog 7: an empty slot saves, a used one asks first, dialog 10)
		}
		else if (theId == 5)
			StartGame();
		else if (theId == 100)
		{
			EnterNameDialog* aDialog = (EnterNameDialog*) GetDialog(6);
			mLastScoreName = aDialog->mNameEdit->mString;
			aDialog->mHighScores->Add(mSideBar->mScore, aDialog->mNameEdit->mString.c_str(), (int) mBoard->mStartTicks);
			aDialog->mHighScores->Save();
			KillDialog(6);
		}
		else if (theId == 0x65)
			KillDialog(6);
	}
	else if (theId == 0x1f5)
		KillDialog(7);
	else if (theId == 700)
	{
		// TODO: the save dialog's second OK (dialog 8, the saved game's name), which saves SLOTn.SAV
	}
	else if (theId == 0x2bd)
		KillDialog(8);

	int aMod = theId % 10000;
	if (aMod < 2000)
		return;
	if (aMod < 3000)
	{
		int aDialogId = theId - 2000;
		if (aDialogId > 0x2724)
		{
			if (aDialogId == 0x4e34)
			{
				KillDialog(0x4e34);
				KillDialog(0x14);
				return;
			}
			KillDialog(aDialogId);
			return;
		}
		if (aDialogId == 0x2724)
		{
			// TODO: the unregistered copy's buy dialog opens the buy page (the port runs as a registered copy)
			KillDialog(0x2724);
			KillDialog(0x14);
			return;
		}
		switch (aDialogId)
		{
		case 0:
			KillDialog(0);
			Shutdown();
			return;
		case 1:
			KillDialog(1);
			ShowMainMenu();
			return;
		case 2:
		{
			// the volumes are already set; then dialog 2 goes and the board gets the focus, and as in the original it
			// runs on into dialog 3's
			OptionsDialog* aDialog = (OptionsDialog*)GetDialog(2);
			bool wantWindowed = !aDialog->mFullscreenCheckbox->IsChecked();
			if (mIsWindowed != wantWindowed)
				SwitchScreenMode(wantWindowed);
			EnableCustomCursors(aDialog->mCustomCursorsCheckbox->IsChecked());
			KillDialog(2);
			if (mBoard != NULL)
				mWidgetManager->SetFocus(mBoard);
		}
		case 3:
			KillDialog(3);
			return;
		case 4:
			GetDialog(2);
			KillDialog(4);
			return;
		case 5:
			return;
		case 9:
			KillDialog(9);
			return;
		case 10:
			KillDialog(10);
			// TODO: the chosen slot's saved game (dialog 7's slot name, then 0x418f40) comes with the saved games
			return;
		case 0xb:
			KillDialog(0xb);
			ShowMainMenu();
			return;
		case 0xc:
			KillDialog(0xc);
			return;
		case 0xd:
			((DynomiteFossil*)mBoard)->NextRound();
			return;
		case 0xe:
			// TODO: the registration dialog's OK (the port runs as a registered copy)
			return;
		case 0x10:
			KillDialog(0x10);
			return;
		case 0x13:
			KillDialog(0x13);
			// port: the original then looks for an update online (0x41f4c0), which the port does not do
			return;
		case 0x16:
			KillDialog(0x16);
			return;
		}
		KillDialog(aDialogId);
		return;
	}
	if (aMod < 4000)
	{
		int aDialogId = theId - 3000;
		if (aDialogId == 0x2724)
		{
			KillDialog(0x2724);
			aDialogId = 0x14;
		}
		KillDialog(aDialogId);
		if (mBoard != NULL)
			mWidgetManager->SetFocus(mBoard);
	}
}

// 0x419040
void FossilizedApp::StartTutorial()
{
	if (mTutor != NULL)
	{
		mWidgetManager->RemoveWidget(mTutor);
		SafeDeleteWidget(mTutor);
		mTutor = NULL;
	}
	if (mBoard != NULL)
	{
		mWidgetManager->RemoveWidget(mBoard);
		SafeDeleteWidget(mBoard);
	}
	mTutor = new Tutorial(this);
	mTutor->Start();
	mWidgetManager->AddWidget(mTutor);
}

// 0x418fd0
void FossilizedApp::LoadSavedGame(const char* theFile)
{
	// TODO: the saved game's board
}

// port: the loading runs in the main thread, so the loader draws the title screen now and then
void FossilizedApp::PumpLoading()
{
	if (mTitleScreen != NULL)
	{
		mTitleScreen->mProgress = GetLoadingThreadProgress();
		mTitleScreen->MarkDirty();
		DrawDirtyStuff();
	}
}

// port: the loading runs in the main thread (the framework runs LoadingThreadProc once the first frame is up)
void FossilizedApp::LoadingThreadProc()
{
	LoadGame();
}
