// The port's host for TipTop (see TipTopApp.h).

#include "TipTopApp.h"
#include "Game.h"
#include "Lowos.h"
#include "Graphics.h"
#include "MemoryImage.h"
#include "WidgetManager.h"
#include "compat2004/Compat2004.h"

#include <SDL.h>
#include <cstring>

using namespace Sexy;

TipTopApp* Sexy::gApp = nullptr;

static uint32_t AppClock()
{
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
	MarkDirty();
}

// The original flipped its back buffer at the end of each pass; here the last pass's screen is shown
void TipTopWidget::Draw(Graphics* g)
{
	const uint32_t* aScreen = lowos::HostScreen();
	ulong* aBits = mScreenImage->GetBits();
	for (int i = 0; i < 640 * 480; i++)
		aBits[i] = 0xff000000 | aScreen[i];
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
}

TipTopApp::~TipTopApp()
{
	if (mWidget != nullptr)
	{
		mWidgetManager->RemoveWidget(mWidget);
		delete mWidget;
	}
}

void TipTopApp::Init()
{
	SexyAppBase::Init();
	if (mShutdown)
		return;
	// 0x411fff seeds the C library's generator with GetTickCount(); -seed=N makes a run repeatable
	CrtSRand(mHasSeedParam ? (unsigned int)mRandSeed : (unsigned int)GetClockTime());
	lowos::HostSetClock(AppClock);
	lowos::HostSetMessageFunc(AppMessage);
	lowos::HostSetWindowed(mIsWindowed);

	mWidget = new TipTopWidget();
	mWidget->Resize(0, 0, mWidth, mHeight);
	mWidgetManager->AddWidget(mWidget);
	mWidgetManager->SetFocus(mWidget);

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
	SexyAppBase::Shutdown();
}

void TipTopApp::RunFrame()
{
	if (!mGameStarted || mGameEnded)
		return;
	if (!tiptop::GameFrame())
	{
		mGameEnded = true;
		tiptop::GameShutdown();
		Shutdown();
		return;
	}
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
