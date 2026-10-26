#include "FossilizedApp.h"
#include "Dynomite.h"
#include "SideBar.h"
#include "AngleTable.h"
#include "InchySmall.h"
#include "IntroScreen.h"
#include "TitleScreen.h"
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
	mSkill = 1;
	mFossilKind = 0;
	mGameMode = 2;
	mInTutorial = false;
	mField10bc = NULL;
	mSideBar = NULL;
	mIntroScreen = NULL;
	// port: the original leaves these to its allocation
	mField1090 = 0;
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
	LoadProperties();
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
	if (mField10bc != NULL)
	{
		mWidgetManager->RemoveWidget(mField10bc);
		SafeDeleteWidget(mField10bc);
		mField10bc = NULL;
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
	// TODO: OptionsDialog (0x42ee20, 0x154 bytes)
}

// 0x419130: a picture of the screen for the save dialog, then the save dialog
void FossilizedApp::SaveGame()
{
	// TODO: SaveGameDialog (0x433910, 0x2d0 bytes)
}

// 0x418810
void FossilizedApp::StartGame()
{
	// TODO: the board of the chosen mode
}

// 0x419040
void FossilizedApp::StartTutorial()
{
	// TODO: the tutorial
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
