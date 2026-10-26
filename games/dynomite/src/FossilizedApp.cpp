#include "FossilizedApp.h"
#include "TitleScreen.h"
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
	// TODO: the side bar (0x435420) at +0x19e4, then the main menu (0x4186d0)
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
