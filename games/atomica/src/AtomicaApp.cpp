// The application class of Atomica Deluxe 2.52 (Atomica.exe 0x406260-0x40b1e0). See AtomicaApp.h and
// docs/GAME_NOTES.md section 3.

#include "AtomicaApp.h"
#include "Screens.h"
#include "GameText.h"
#include "Compat2004.h"
#include "ImageFont.h"
#include "MusicInterface.h"
#include "SoundManager.h"
#include "WidgetManager.h"

#include <SDL.h>
#include <cstdio>
#include <cstring>

using namespace Sexy;

AtomicaApp* Sexy::gApp = NULL;

// 0x406570
AtomicaApp::AtomicaApp()
{
	gApp = this;
	mProdName = "Atomica";
	mProductVersion = "2.52";
	mRegKey = "PopCap\\" + mProdName;
	mTitle = "Atomica Deluxe " + mProductVersion;
	mWidth = GAME_WIDTH;
	mHeight = GAME_HEIGHT;

	mAtomicFont = NULL;
	mLevelFont = NULL;
	mScoreFont = NULL;
	mStencilFont = NULL;
	mMonitorFont = NULL;
	mMonitorFontMedium = NULL;
	mMonitorFontSmall = NULL;
	mFloatyFont = NULL;
	mAnnounceFont = NULL;
	mCommentFont = NULL;
	mMouseoverFont = NULL;
	mBoard = NULL;
	mMenu = NULL;
	mTitleScreen = NULL;
	mMode = 0;
	mB3ea0 = false;
	mExpertMode = 0;
	mB3eac = false;
	mPauseCount = 0;
	mI440 = 0;
	mShowSymbols = false;
	mShowTips = true;
	mLevelUpDialog = NULL;
	mOptionsDialog = NULL;
	mHelpDialog = NULL;
	mMenuDialog = NULL;
	mSprites = new SpriteMgr(this);
}

AtomicaApp::~AtomicaApp()
{
	delete mSprites;
}

// 0x4067b0
void AtomicaApp::Init()
{
	SexyApp::Init();
	LoadGameTexts();

	// the 2003 SexyApp::Init seeds the game's own generator with the tick count; -seed=N makes a run repeatable
	mRand.Seed(mHasSeedParam ? (unsigned int) mRandSeed : (unsigned int) GetTickCount());
	CrtSRand(mHasSeedParam ? (unsigned int) mRandSeed : (unsigned int) GetTickCount());

	mMusicInterface->LoadMusic(0, "music\\atomica.it");
	mMusicInterface->LoadMusic(1, "music\\atomica.it");

	mAtomicFont = new ImageFont(this, "data\\atomicfont.txt");
	mMonitorFont = new ImageFont(this, "data\\monitorfont.txt");
	mMonitorFontMedium = new ImageFont(this, "data\\monitorfont(medium).txt");
	mMonitorFontSmall = new ImageFont(this, "data\\monitorfont(small).txt");

	Sprite* aCursor = mSprites->Load("cursor1");
	SetCursorImage(CURSOR_POINTER, aCursor->mImage);
	aCursor = mSprites->Load("pointer2");
	SetCursorImage(CURSOR_HAND, aCursor->mImage);
	aCursor = mSprites->Load("overhand");
	SetCursorImage(CURSOR_DRAGGING, aCursor->mImage);
	aCursor = mSprites->Load("typer");
	SetCursorImage(CURSOR_TEXT, aCursor->mImage);
	mDialogMonitor = mSprites->Load("dialogmonitor");
	mSndCapClick = new Sound(this, "capclick");

	mTitleScreen = new TitleScreen(this);
	mTitleScreen->Resize(0, 0, mWidth, mHeight);
	mWidgetManager->AddWidget(mTitleScreen);
}

// the title screen's play link was clicked (the menu is not ported yet: the game starts)
void AtomicaApp::TitleScreenDone()
{
	SDL_Log("title screen done");
	if (mTitleScreen != NULL)
	{
		mWidgetManager->RemoveWidget(mTitleScreen);
		SafeDeleteWidget(mTitleScreen);
		mTitleScreen = NULL;
	}
	StartGame();
}

// port: the loading runs in the main thread (the framework runs LoadingThreadProc once the first frame is up), so the
// proc draws the title screen's bar itself now and then
void AtomicaApp::PumpLoading()
{
	if (mTitleScreen != NULL)
	{
		mTitleScreen->mProgress = GetLoadingThreadProgress();
		mTitleScreen->MarkDirty();
		DrawDirtyStuff();
	}
}

void AtomicaApp::LoadingThreadCompleted()
{
	SexyApp::LoadingThreadCompleted();
}

// 0x406ec0 / 0x406dc0: the registry values of the game
void AtomicaApp::WriteToRegistry()
{
	SexyApp::WriteToRegistry();
}

void AtomicaApp::ReadFromRegistry()
{
	SexyApp::ReadFromRegistry();
}

void AtomicaApp::HandleCmdLineParam(const std::string& theParamName, const std::string& theParamValue)
{
	SexyApp::HandleCmdLineParam(theParamName, theParamValue);
}
