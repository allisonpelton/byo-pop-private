// The port's host for Ning Po (see NingPoApp.h).

#include "NingPoApp.h"
#include "GameText.h"
#include "HDisplay.h"
#include "HInput.h"
#include "NingPo.h"
#include "Graphics.h"
#include "MemoryImage.h"
#include "WidgetManager.h"
#include "platform/VFS.h"

#include <SDL.h>
#include <cstring>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>

// The game's sleep yields to the browser (ASYNCIFY). A page that runs hidden can set Module.ningpoSetTimeout to a
// timer of its own: a hidden tab slows setTimeout down to once a second.
EM_ASYNC_JS(void, ningpo_sleep, (int theMs), {
	await new Promise(function(theResolve) { (Module['ningpoSetTimeout'] || setTimeout)(theResolve, theMs); });
});
#endif

using namespace Sexy;

NingPoApp* Sexy::gApp = nullptr;

namespace
{

// The framework's side of pg::Host
class NingPoHost : public pg::Host
{
public:
	uint32_t TimeMs() override
	{
		return gApp->GetClockTime();
	}

	void Sleep(int theMs) override
	{
		gApp->RunFramework(theMs);
	}

	void ScreenChanged() override
	{
		if (gApp->mWidget != nullptr)
			gApp->mWidget->mScreenChanged = true;
	}

	bool ReadGameFile(const std::string& theName, std::vector<uint8_t>& theData) override
	{
		return Platform::VFS::Get().ReadAll(theName, theData);
	}

	bool RegistryReadString(const std::string& theName, std::string& theValue) override
	{
		return gApp->RegistryReadString(theName, &theValue);
	}

	bool RegistryWriteString(const std::string& theName, const std::string& theValue) override
	{
		return gApp->RegistryWriteString(theName, theValue);
	}

	bool RegistryReadData(const std::string& theName, std::vector<uint8_t>& theData) override
	{
		std::vector<uchar> aBuf(0x10000);
		ulong aLength = (ulong)aBuf.size();
		if (!gApp->RegistryReadData(theName, aBuf.data(), &aLength))
			return false;
		theData.assign(aBuf.begin(), aBuf.begin() + aLength);
		return true;
	}

	bool RegistryWriteData(const std::string& theName, const void* theData, size_t theSize) override
	{
		return gApp->RegistryWriteData(theName, (const uchar*)theData, (ulong)theSize);
	}

	// the framework opens nothing in a test run (it logs "OpenURL in a test run")
	void OpenURL(const std::string& theURL) override
	{
		gApp->SexyAppBase::OpenURL(theURL);
	}

	std::string ClipboardText() override
	{
		return gApp->GetClipboard();
	}

	// a test run types the same name on every machine
	std::string UserName() override
	{
		if (gApp->IsTestRun())
			return "";
#if defined(__EMSCRIPTEN__)
		return "";
#elif defined(_WIN32)
		const char* aName = SDL_getenv("USERNAME");
		return aName != nullptr ? aName : "";
#else
		const char* aName = SDL_getenv("USER");
		return aName != nullptr ? aName : "";
#endif
	}

	void SetFullscreen(bool theFullscreen) override
	{
		gApp->SetGameFullscreen(theFullscreen);
	}

	void Log(const char* theText) override
	{
		SDL_Log("%s", theText);
	}

	// test runs (-script, -fast, rundesk) never block on a message box
	void ShowMessage(const char* theCaption, const char* theText) override
	{
		SDL_Log("%s: %s", theCaption, theText);
		if (!gApp->mFastMode && !gApp->IsTestRun())
			SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_WARNING, theCaption, theText, nullptr);
	}
};

NingPoHost gHost;

// MapVirtualKey(theKey, MAPVK_VK_TO_CHAR) on a US layout: the key's character unshifted, a letter in capitals; 0
// for a key without one
int VirtualKeyChar(int theKey)
{
	if ((theKey >= '0' && theKey <= '9') || (theKey >= 'A' && theKey <= 'Z'))
		return theKey;
	if (theKey >= 0x60 && theKey <= 0x69)
		return '0' + theKey - 0x60;
	switch (theKey)
	{
	case 0x08:
	case 0x09:
	case 0x0d:
	case 0x1b:
	case 0x20:
		return theKey;
	case 0x6a: return '*';
	case 0x6b: return '+';
	case 0x6d: return '-';
	case 0x6e: return '.';
	case 0x6f: return '/';
	case 0xba: return ';';
	case 0xbb: return '=';
	case 0xbc: return ',';
	case 0xbd: return '-';
	case 0xbe: return '.';
	case 0xbf: return '/';
	case 0xc0: return '`';
	case 0xdb: return '[';
	case 0xdc: return '\\';
	case 0xdd: return ']';
	case 0xde: return '\'';
	default: return 0;
	}
}

// 0x433290: a letter is lower case unless Shift is held
int KeyEventChar(int theKey)
{
	int aChar = VirtualKeyChar(theKey);
	if (aChar >= 'A' && aChar <= 'Z' && !gApp->mWidgetManager->mKeyDown[KEYCODE_SHIFT])
		aChar = aChar - 'A' + 'a';
	return aChar;
}

} // namespace

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// The widget: the window procedure (0x433290) and the screen

NingPoWidget::NingPoWidget()
{
	mScreenImage = new MemoryImage(gApp);
	mScreenImage->Create(640, 480);
	mScreenChanged = true;
	mWantsFocus = true;
}

NingPoWidget::~NingPoWidget()
{
	delete mScreenImage;
}

// the original's 10 ms timer (WM_TIMER) posts the game's tick
void NingPoWidget::Update()
{
	Widget::Update();
	if (gApp->mGameRunning)
		gApp->PostEvent(pg::EV_TICK, 0, 0, 0, 0, 0);
	if (mScreenChanged)
		MarkDirty();
}

// The game's back buffer is 15-bit (5-5-5); the original blitted the changed rectangles to the window
void NingPoWidget::Draw(Graphics* g)
{
	mScreenChanged = false;
	ulong* aBits = mScreenImage->GetBits();
	pg::HDisplay* aDisplay = gApp->mGame != nullptr ? gApp->mGame->mDisplay : nullptr;
	if (aDisplay != nullptr && aDisplay->mScreen.size() >= 640 * 480 && aDisplay->mWidth == 640)
	{
		const uint16_t* aScreen = aDisplay->mScreen.data();
		for (int i = 0; i < 640 * 480; i++)
		{
			uint32_t p = aScreen[i];
			uint32_t r = (p >> 10) & 0x1f;
			uint32_t gr = (p >> 5) & 0x1f;
			uint32_t b = p & 0x1f;
			aBits[i] = 0xff000000 | (((r << 3) | (r >> 2)) << 16) | (((gr << 3) | (gr >> 2)) << 8) | ((b << 3) | (b >> 2));
		}
	}
	else
	{
		for (int i = 0; i < 640 * 480; i++)
			aBits[i] = 0xff000000;
	}
	mScreenImage->BitsChanged();
	g->DrawImage(mScreenImage, 0, 0);
}

// WM_CHAR
void NingPoWidget::KeyChar(SexyChar theChar)
{
	uint8_t aChar = (uint8_t)theChar;
	if (aChar != 0)
		gApp->PostEvent(pg::EV_DOWN, 0, aChar, 0, 0, 0);
}

// WM_KEYDOWN: Ctrl+V pastes; a key with a character comes as WM_CHAR, the others go down with their virtual key
void NingPoWidget::KeyDown(KeyCode theKey)
{
	if (theKey == KEYCODE_SHIFT || theKey == KEYCODE_CONTROL || theKey == KEYCODE_MENU)
		return;
	int aChar = KeyEventChar(theKey);
	if (aChar == 'v')
	{
		if (gApp->mWidgetManager->mKeyDown[KEYCODE_CONTROL])
			gApp->PostEvent(pg::EV_PASTE, 0, 0, 0, 0, 0);
	}
	else if (aChar == 0)
		gApp->PostEvent(pg::EV_DOWN, theKey, 0, 0, 0, 0);
}

// WM_KEYUP: the key's character, or its virtual key
void NingPoWidget::KeyUp(KeyCode theKey)
{
	if (theKey == KEYCODE_SHIFT || theKey == KEYCODE_CONTROL)
		return;
	int aChar = KeyEventChar(theKey);
	gApp->PostEvent(pg::EV_UP, aChar == 0 ? theKey : 0, aChar, 0, 0, 0);
}

void NingPoWidget::MouseMove(int x, int y)
{
	gApp->PostEvent(pg::EV_MOUSE_MOVE, 0, 0, x, y, 0);
}

void NingPoWidget::MouseDrag(int x, int y)
{
	gApp->PostEvent(pg::EV_MOUSE_MOVE, 0, 0, x, y, 0);
}

// The framework's buttons: 0 left, 1 right, 2 middle (the original took no middle button)
void NingPoWidget::MouseDown(int x, int y, int theBtnNum, int theClickCount)
{
	if (theBtnNum == 0 || theBtnNum == 1)
		gApp->PostEvent(pg::EV_DOWN, theBtnNum == 0 ? pg::BUTTON_LEFT : pg::BUTTON_RIGHT, 0, x, y, 0);
}

void NingPoWidget::MouseUp(int x, int y, int theBtnNum, int theClickCount)
{
	if (theBtnNum == 0 || theBtnNum == 1)
		gApp->PostEvent(pg::EV_UP, theBtnNum == 0 ? pg::BUTTON_LEFT : pg::BUTTON_RIGHT, 0, x, y, 0);
}

void NingPoWidget::MouseWheel(int theDelta)
{
	if (theDelta != 0)
		gApp->PostEvent(pg::EV_WHEEL, 0, 0, 0, 0, theDelta);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// The app

NingPoApp::NingPoApp()
{
	gApp = this;
	mProdName = "NingPo";
	mTitle = "Ning Po Mahjong Deluxe";
	// the original's key (HRegistry::open with the cvars companyName and productName)
	mRegKey = "Pyrogon\\NingPo";
	mWidth = 640;
	mHeight = 480;
	mWidget = nullptr;
	mGame = nullptr;
	mGameRunning = false;
	mApplyingGameMode = false;
}

NingPoApp::~NingPoApp()
{
	if (mWidget != nullptr)
	{
		mWidgetManager->RemoveWidget(mWidget);
		delete mWidget;
	}
}

void NingPoApp::Init()
{
	SexyAppBase::Init();
	if (mShutdown)
		return;
	pg::SetHost(&gHost);
	mWidget = new NingPoWidget();
	mWidget->Resize(0, 0, mWidth, mHeight);
	mWidgetManager->AddWidget(mWidget);
	mWidgetManager->SetFocus(mWidget);
}

// The game's own loop (0x401000 WinMain: PGApp::run); the framework runs in its sleeps. When it ends the framework
// ends too: on the web its loop then finishes the page's run (the saves' backup, Module.onGameExit).
void NingPoApp::DoMainLoop()
{
	if (!mShutdown)
	{
		// the default best times' names are read before the game is made
		ningpo::LoadGameTexts();
		mGame = new ningpo::NingPo();
		// test hook: -seed=N seeds the game's generator instead of the time of its first event (PGApp::mainLoop)
		if (mHasSeedParam)
		{
			mGame->mRandom.seed((uint32_t)mRandSeed);
			mGame->mRandomSeeded = true;
		}
		mGameRunning = true;
		mGame->run("", "");
		mGameRunning = false;
		delete mGame;
		mGame = nullptr;
		Shutdown();
	}
	SexyAppBase::DoMainLoop();
}

void NingPoApp::RunFramework(int theMs)
{
	DWORD anEnd = GetClockTime() + (DWORD)theMs;
	do
	{
		// the framework was shut down (a script's quit): the game is asked to quit, as the close button does
		if (mShutdown)
		{
			PostEvent(pg::EV_QUIT, 0, 0, 0, 0, 0);
			return;
		}
		if (mExitToTop)
			mExitToTop = false;
#ifdef __EMSCRIPTEN__
		// as SexyAppBase::EmscriptenMainLoop: step until a frame was drawn or nothing is due, then yield
		int aDrawCount = mDrawCount;
		for (int aStep = 0; aStep < 1000 && !mShutdown; aStep++)
		{
			UpdateAppStep(nullptr);
			if (mDrawCount != aDrawCount || mNothingDue)
				break;
		}
		UpdateUserDataBackup(false);
		ningpo_sleep(1);
#else
		UpdateAppStep(nullptr);
#endif
	} while ((int)(GetClockTime() - anEnd) < 0);
}

void NingPoApp::PostEvent(int theType, int theCode, int theChar, int theX, int theY, int theValue)
{
	pg::HInput* anInput = pg::GetInput();
	if (anInput == nullptr || !mGameRunning)
		return;
	pg::PGEvent anEvent;
	memset(&anEvent, 0, sizeof(anEvent));
	anEvent.mType = theType;
	anEvent.mCode = theCode;
	anEvent.mChar = (uint8_t)theChar;
	anEvent.mX = theX;
	anEvent.mY = theY;
	anEvent.mValue = theValue;
	anEvent.mModifiers = (mWidgetManager->mKeyDown[KEYCODE_CONTROL] ? 4 : 0) |
		(mWidgetManager->mKeyDown[KEYCODE_MENU] ? 2 : 0) | (mWidgetManager->mKeyDown[KEYCODE_SHIFT] ? 1 : 0);
	anInput->postEvent(anEvent);
}

// WM_CLOSE goes to the game, which quits (or ends the dialog that is up)
void NingPoApp::CloseRequestAsync()
{
	if (mGameRunning)
		PostEvent(pg::EV_QUIT, 0, 0, 0, 0, 0);
	else
		SexyAppBase::CloseRequestAsync();
}

// WM_ACTIVATE
void NingPoApp::GotFocus()
{
	SexyAppBase::GotFocus();
	PostEvent(pg::EV_ACTIVATE, 0, 0, 0, 0, 1);
}

void NingPoApp::LostFocus()
{
	SexyAppBase::LostFocus();
	PostEvent(pg::EV_ACTIVATE, 0, 0, 0, 0, 0);
}

// Alt+Enter (and the web page's full screen button) go to the game like the original's Alt+Enter, which asks
// for the mode itself (SetGameFullscreen)
void NingPoApp::SwitchScreenMode(bool wantWindowed, bool is3d, bool force)
{
	if (mApplyingGameMode || !mGameRunning || force)
	{
		SexyAppBase::SwitchScreenMode(wantWindowed, is3d, force);
		return;
	}
	if (wantWindowed != mIsWindowed)
		PostEvent(pg::EV_TOGGLE_SCREEN, 0, 0, 0, 0, 0);
}

void NingPoApp::SetGameFullscreen(bool theFullscreen)
{
	if (theFullscreen != mIsWindowed)
		return;
	mApplyingGameMode = true;
	SwitchScreenMode(!theFullscreen);
	mApplyingGameMode = false;
}
