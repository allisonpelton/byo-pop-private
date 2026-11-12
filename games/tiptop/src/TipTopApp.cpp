// port: the port's host for TipTop (see TipTopApp.h).

#include "TipTopApp.h"
#include "ModFeatures.h"
#include "Game.h"
#include "Lowos.h"
#include "MusicSynth.h"
#include "platform/Audio.h"
#include "Graphics.h"
#include "MemoryImage.h"
#include "WidgetManager.h"
#include "compat2004/Compat2004.h"

#include <SDL.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>

using namespace Sexy;

TipTopApp* Sexy::gApp = nullptr;

namespace Sexy
{
AutoplayState* NewAutoplayState(unsigned int theSeed);  // testhooks/Autoplay.cpp
void DeleteAutoplayState(AutoplayState* theState);
}

// fixes mod, "steadyClock": the game's timers run on its updates (mFrameTime ms each) instead of the machine's clock
static bool gSteadyClock = false;

static uint32_t AppClock()
{
	if (gSteadyClock)
		return 1000 + (uint32_t)gApp->mUpdateCount * (uint32_t)gApp->mFrameTime;
	return gApp->GetClockTime();
}

static void AppMessage(const char* theText, const char* theCaption)
{
	SDL_Log("%s: %s", theCaption, theText);
	// test runs (-script, -autoplay, -fast) never block on a message box
	if (!gApp->mFastMode && gApp->mScriptFileName.empty() && !gApp->mScriptedInput)
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, theCaption, theText, nullptr);
}

TipTopWidget::TipTopWidget()
{
	mScreenImage = new MemoryImage(gApp);
	mScreenImage->Create(640, 480);
	// opaque, so that the framework need not look through the 307200 pixels for alpha after each change, and with every
	// alpha byte 0xff (Draw), so that it need not copy and premultiply them pixel by pixel in strips to upload them
	// (where premultiplying changes nothing): the same pixels reach the screen, in one upload
	mScreenImage->SetImageMode(false, false);
	mScreenImage->mUploadAsIs = true;
	mShownWrites = 0;
	mHasShown = false;
	mWantsFocus = true;
}

TipTopWidget::~TipTopWidget()
{
	delete mScreenImage;
}

void TipTopWidget::Update()
{
	Widget::Update();
	gApp->RunFrame();
	// The framework gets a frame only when the screen shows something new. A pass that wrote nothing to it, or wrote the
	// pixels that were there (a menu draws all of its layers every pass), leaves the picture as shown, and the framework
	// then draws no frame (it draws one a second at least, and at once when the window needs it), as the original's
	// dirty layers drew nothing on a screen that stood still.
	uint32_t aWrites = lowos::HostScreenWrites();
	if (!mHasShown)
		MarkDirty();
	else if ((aWrites != mShownWrites) && (!mDirty))
	{
		if (ScreenDiffersFromShown())
			MarkDirty();
		else
			mShownWrites = aWrites;
	}
}

// Whether the screen holds other pixels than mScreenImage, which Draw filled from it (the alpha byte aside)
bool TipTopWidget::ScreenDiffersFromShown()
{
	const uint32_t* aScreen = lowos::HostScreen();
	int aWidth, aHeight;
	lowos::HostScreenSize(&aWidth, &aHeight);
	const ulong* aShown = mScreenImage->GetBits();
	for (int y = 0; y < 480; y++)
	{
		const uint32_t* aRow = aScreen + y * aWidth;
		const ulong* aShownRow = aShown + y * 640;
		for (int x = 0; x < 640; x += 16)
		{
			uint32_t aDifference = 0;
			for (int i = 0; i < 16; i++)
				aDifference |= (aRow[x + i] ^ (uint32_t)aShownRow[x + i]) & 0x00ffffff;
			if (aDifference != 0)
				return true;
		}
	}
	return false;
}

// The original flipped its back buffer at the end of each pass; here the last pass's screen is shown
void TipTopWidget::Draw(Graphics* g)
{
	const uint32_t* aScreen = lowos::HostScreen();
	int aWidth, aHeight;
	lowos::HostScreenSize(&aWidth, &aHeight);
	mShownWrites = lowos::HostScreenWrites();
	mHasShown = true;
	ulong* aBits = mScreenImage->GetBits();
	// the top left 640x480 of a larger -canvas screen
	for (int y = 0; y < 480; y++)
		for (int x = 0; x < 640; x++)
			aBits[y * 640 + x] = 0xff000000 | aScreen[y * aWidth + x];
	mScreenImage->BitsChanged();
	g->DrawImage(mScreenImage, 0, 0);
}

void TipTopWidget::KeyChar(SexyChar theChar)
{
	uint8_t aChar = (uint8_t)theChar;
	if (aChar != 0)
		lowos::HostChar(aChar);
}

void TipTopWidget::KeyDown(KeyCode theKey)
{
	int aCode = lowos::VirtualKeyToScanCode(theKey);
	if (aCode != 0)
		lowos::HostKey(aCode, true);
}

void TipTopWidget::KeyUp(KeyCode theKey)
{
	int aCode = lowos::VirtualKeyToScanCode(theKey);
	if (aCode != 0)
		lowos::HostKey(aCode, false);
}

void TipTopWidget::MouseMove(int x, int y)
{
	lowos::HostMouseMove(x, y);
}

void TipTopWidget::MouseDrag(int x, int y)
{
	lowos::HostMouseMove(x, y);
}

// The framework's buttons: 0 left, 1 right, 2 middle; the original's window messages gave the same numbers
void TipTopWidget::MouseDown(int x, int y, int theBtnNum, int theClickCount)
{
	lowos::HostMouseMove(x, y);
	lowos::HostMouseButton(theBtnNum, true);
}

void TipTopWidget::MouseUp(int x, int y, int theBtnNum, int theClickCount)
{
	lowos::HostMouseMove(x, y);
	lowos::HostMouseButton(theBtnNum, false);
}

TipTopApp::TipTopApp()
{
	gApp = this;
	mProdName = "TipTop";
	mTitle = "TipTop Deluxe";
	mRegKey = "PopCap\\TipTop";
	mWidth = 640;
	mHeight = 480;
	mWidget = nullptr;
	mGameStarted = false;
	mGameEnded = false;
	mApplyingGameMode = false;
	mAutoplay = false;
	mAutoplayMode = 0;
	mAutoplayGames = 1;
	mAutoplayCap = 100000;
	mAutoplayState = nullptr;
	mAutoplayUi = false;
	mAutoplaySave = 0;
	mStateLog = nullptr;
	mButtonLog = nullptr;
	mSoundLog = nullptr;
}

TipTopApp::~TipTopApp()
{
	if (mWidget != nullptr)
	{
		mWidgetManager->RemoveWidget(mWidget);
		delete mWidget;
	}
	if (mAutoplayState != nullptr)
		DeleteAutoplayState(mAutoplayState);
	if (mStateLog != nullptr)
		fclose(mStateLog);
	if (mButtonLog != nullptr)
		fclose(mButtonLog);
	if (mScreenHashLog != nullptr)
		fclose(mScreenHashLog);
	if (mSoundLog != nullptr)
	{
		lowos::SetSoundLog(nullptr, nullptr);
		fclose(mSoundLog);
	}
}

void TipTopApp::Init()
{
	SexyAppBase::Init();
	if (mShutdown)
		return;
	// 0x411fff seeds the C library's generator with GetTickCount(); -seed=N makes a run repeatable
	tiptop::SRand(mHasSeedParam ? (unsigned int)mRandSeed : (unsigned int)GetClockTime());
	gSteadyClock = !mFastMode && Sexy::FixOn("steadyClock");  // a -fast run is steady already
	lowos::HostSetClock(AppClock);
	lowos::HostSetMessageFunc(AppMessage);
	lowos::HostSetWindowed(mIsWindowed);

	mWidget = new TipTopWidget();
	mWidget->Resize(0, 0, mWidth, mHeight);
	mWidgetManager->AddWidget(mWidget);
	mWidgetManager->SetFocus(mWidget);

	if (mSoundFont.empty())
	{
		char* aBase = SDL_GetBasePath();
		mSoundFont = std::string(aBase != nullptr ? aBase : "") + "tiptop.sf2";
		SDL_free(aBase);
	}
	tiptop::StartMusicSynth(mSoundFont);
	if (!tiptop::GameInit())
	{
		mGameEnded = true;
		Shutdown();
		return;
	}
	mGameStarted = true;
}

void TipTopApp::Shutdown()
{
	if (mGameStarted && !mGameEnded)
	{
		mGameEnded = true;
		tiptop::GameShutdown();
	}
	tiptop::StopMusicSynth();
	SexyAppBase::Shutdown();
}

void TipTopApp::RunFrame()
{
	if (!mGameStarted || mGameEnded)
		return;
	if (mAutoplay)
	{
		Autoplay();
		if (mShutdown)
			return;
	}
	if (mScrambleScreen)
		ScrambleScreen();
	if (!tiptop::GameFrame())
	{
		mGameEnded = true;
		tiptop::GameShutdown();
		Shutdown();
		return;
	}
	if (mScreenHashLog != nullptr)
		LogScreenHash();
	if (!mStateLogName.empty())
		UpdateStateLog();
	if (!mButtonLogName.empty())
		UpdateButtonLog();
	if (mSoundLog != nullptr)
		UpdateMusicLog();
	if (!mCanvasShot.empty())
		SaveCanvasShot();
	int aMode = lowos::HostTakeScreenModeRequest();
	if (aMode != 0)
	{
		mApplyingGameMode = true;
		SwitchScreenMode(aMode == 2);
		mApplyingGameMode = false;
		lowos::HostSetWindowed(mIsWindowed);
	}
}

// The window's close button goes to the game (0x41b2b8: it quits, or asks first while a game is on)
void TipTopApp::CloseRequestAsync()
{
	if (mGameStarted && !mGameEnded)
		lowos::HostClose();
	else
		SexyAppBase::CloseRequestAsync();
}

void TipTopApp::GotFocus()
{
	SexyAppBase::GotFocus();
	lowos::HostSetActive(true);
}

void TipTopApp::LostFocus()
{
	SexyAppBase::LostFocus();
	lowos::HostSetActive(false);
}

// Alt+Enter (and the web page's full screen button) go to the game like the original's Alt+Enter; the game then
// asks for the mode through LOWOS_ScreenScope, which RunFrame applies.
void TipTopApp::SwitchScreenMode(bool wantWindowed, bool is3d, bool force)
{
	if (mApplyingGameMode || !mGameStarted || mGameEnded || force)
	{
		SexyAppBase::SwitchScreenMode(wantWindowed, is3d, force);
		return;
	}
	if (wantWindowed != mIsWindowed)
		lowos::HostRequestScreenSwitch();
}

// TipTop's music has its own volume (the options' slider, BuildMusicVolumes, in the channels' CC 7). The framework's music
// volume, which it scales for BASS (0.85 of 40 out of 100), would turn the synth down by another 9 dB in the mixer, so only its
// muting reaches the mixer.
void TipTopApp::SetMusicVolume(double theVolume)
{
	mMusicVolume = theVolume;
	Platform::Mixer::Get().SetMusicMasterGain((mMuteCount > 0) ? 0.0f : 1.0f);
}

// the command line: -autoplay=<seed> -autoplaymode=regular|strategy|speed -autoplaygames=N -autoplaycap=N
// -autoplaylog=<file> -autoplayshots=<prefix> -autoplayui -autoplaysave=N -autoplayscript=<file> -statelog=<file> -buttonlog=<file> -soundlog=<file> (testhooks/Autoplay.cpp); -canvas=<width>x<height> (testhooks/Canvas.cpp);
// -scramble, -screenhash=<file> (testhooks/Canvas.cpp); -soundfont=<file> (MusicSynth.h)
void TipTopApp::HandleCmdLineParam(const std::string& theParamName, const std::string& theParamValue)
{
	if (theParamName == "-autoplay")
	{
		mAutoplay = true;
		mAutoplayState = NewAutoplayState((unsigned int)atoi(theParamValue.c_str()));
		mScriptedInput = true;  // the bot's input only, as in a -script run
	}
	else if (theParamName == "-autoplaymode")
		mAutoplayMode = (theParamValue == "strategy") ? 1 : (theParamValue == "speed") ? 2 : 0;
	else if (theParamName == "-autoplaygames")
		mAutoplayGames = atoi(theParamValue.c_str());
	else if (theParamName == "-autoplaycap")
		mAutoplayCap = atoi(theParamValue.c_str());
	else if (theParamName == "-autoplaylog")
		mAutoplayLog = theParamValue;
	else if (theParamName == "-autoplayshots")
		mAutoplayShots = theParamValue;
	else if (theParamName == "-autoplaysave")
		mAutoplaySave = atoi(theParamValue.c_str());
	else if (theParamName == "-autoplayui")
		mAutoplayUi = true;
	else if (theParamName == "-autoplayscript")
		mAutoplayScript = theParamValue;
	else if (theParamName == "-statelog")
		mStateLogName = theParamValue;
	else if (theParamName == "-buttonlog")
		mButtonLogName = theParamValue;
	else if (theParamName == "-soundlog")
	{
		if (mSoundLog == nullptr)
			mSoundLog = fopen(theParamValue.c_str(), "w");
		if (mSoundLog != nullptr)
			lowos::SetSoundLog(mSoundLog, &mUpdateCount);
	}
	else if (theParamName == "-scramble")
		mScrambleScreen = true;
	else if (theParamName == "-screenhash")
		mScreenHashLog = fopen(theParamValue.c_str(), "w");
	else if (theParamName == "-canvas")
	{
		int aWidth = 0, aHeight = 0;
		if (sscanf(theParamValue.c_str(), "%dx%d", &aWidth, &aHeight) == 2 && aWidth >= 640 && aHeight >= 480 &&
			aWidth <= 4096 && aHeight <= 4096)
			lowos::HostSetCanvas(aWidth, aHeight);
		else
			SDL_Log("-canvas=%s: expected <width>x<height>, from 640x480 to 4096x4096", theParamValue.c_str());
	}
	else if (theParamName == "-soundfont")
		mSoundFont = theParamValue;
	else
		SexyAppBase::HandleCmdLineParam(theParamName, theParamValue);
}
