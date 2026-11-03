// The application class of Atomica Deluxe 2.52 (Atomica.exe 0x406260-0x40b1e0). See AtomicaApp.h and
// docs/GAME_NOTES.md section 3.

#include "AtomicaApp.h"
#include <cstdlib>
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
	mAutoplay = false;
	mAutoplayMode = 0;
	mAutoplayGames = 1;
	mAutoplayCap = 100000;
	mAutoplayFinish = 0;
	mAutoplayBonus = false;
	mAutoplayState = NULL;
	mTraceState = NULL;
	mMode = 0;
	mB3ea0 = false;
	mExpertMode = 0;
	mB3eac = false;
	mPauseCount = 0;
	mI440 = 0;
	mShowSymbols = false;
	mShowTips = true;
	for (int i = 0; i < 0xff; i++)
		mHintsShown[i] = false;
	mLevelUpDialog = NULL;
	mScoreDialog = NULL;
	mHighScoresBox = NULL;
	mQuitDialog = NULL;
	mConfirmDialog = NULL;
	mOptionsDialog = NULL;
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

	ReadHighScores();
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

// the title screen's play link was clicked: the menu's doors close over it (and the slam deletes it, 0x405e20)
void AtomicaApp::TitleScreenDone()
{
	if (mMenu != NULL)	// port: the title screen is still there until the doors have shut; a second click does not start over
		return;
	SDL_Log("title screen done");
	ShowMainMenu(false);
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

// 0x406ec0 / 0x406dc0: the registry values of the game: the hints shown (255 words), the colour-blind symbols, and the
// hints being switched off (named ShowTips, it holds one when the hints are off)
void AtomicaApp::WriteToRegistry()
{
	SexyApp::WriteToRegistry();
	int aHints[0xff];
	for (int i = 0; i < 0xff; i++)
		aHints[i] = mHintsShown[i] ? 1 : 0;
	RegistryWriteData("GameHints", (const uchar*) aHints, sizeof(aHints));
	RegistryWriteInteger("ShowSymbols", mShowSymbols ? 1 : 0);
	RegistryWriteInteger("ShowTips", mShowTips ? 0 : 1);
}

void AtomicaApp::ReadFromRegistry()
{
	SexyApp::ReadFromRegistry();
	int aHints[0xff];
	ulong aLength = sizeof(aHints);
	if (RegistryReadData("GameHints", (uchar*) aHints, &aLength))
		for (int i = 0; (i < 0xff) && ((ulong) (i * 4 + 4) <= aLength); i++)
			mHintsShown[i] = aHints[i] == 1;
	int aValue = 0;
	if (RegistryReadInteger("ShowSymbols", &aValue))
		mShowSymbols = aValue == 1;
	if (RegistryReadInteger("ShowTips", &aValue))
		mShowTips = aValue != 1;
}

// the command line: -autoplay=<seed> -autoplaymode=strategy|action -autoplaygames=N -autoplaycap=N -autoplaylog=<file>
// -autoplayshots=<prefix>
void AtomicaApp::HandleCmdLineParam(const std::string& theParamName, const std::string& theParamValue)
{
	if (theParamName == "-autoplay")
	{
		mAutoplay = true;
		mAutoplayState = NewAutoplayState((unsigned int) atoi(theParamValue.c_str()));
		mScriptedInput = true;   // the bot's input only, as in a -script run
	}
	else if (theParamName == "-autoplaymode")
		mAutoplayMode = (theParamValue == "action") ? 1 : 0;
	else if (theParamName == "-autoplaygames")
		mAutoplayGames = atoi(theParamValue.c_str());
	else if (theParamName == "-autoplaycap")
		mAutoplayCap = atoi(theParamValue.c_str());
	else if (theParamName == "-autoplaybonus")
		mAutoplayBonus = theParamValue != "0";
	else if (theParamName == "-autoplayfinish")
		mAutoplayFinish = atoi(theParamValue.c_str());
	else if (theParamName == "-autoplaylog")
		mAutoplayLog = theParamValue;
	else if (theParamName == "-statetrace")
		mTraceFile = theParamValue;
	else if (theParamName == "-autoplayscript")
		mAutoplayScript = theParamValue;
	else if (theParamName == "-autoplayshots")
		mAutoplayShots = theParamValue;
	else
		SexyApp::HandleCmdLineParam(theParamName, theParamValue);
}

void AtomicaApp::UpdateFrames()
{
	// the trace is taken before the next update, so that it holds what the last update's draw changed (the random numbers)
	if (!mTraceFile.empty())
		TraceUpdate();
	// the bot acts before the update, where a script's commands (and a player's input) come: after the last update's draw
	if (mAutoplay)
		AutoplayUpdate();
	SexyApp::UpdateFrames();
}
