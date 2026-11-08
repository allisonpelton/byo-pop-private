// The port's host for Mahjong Escape: Ancient Japan (see MahjongApp.h).

#include "MahjongApp.h"

#include "GameText.h"
#include "Host.h"
#include "MainMenu.h"
#include "Manager.h"
#include "ModFeatures.h"
#include "PeResources.h"
#include "Players.h"
#include "Splash.h"
#include "User.h"
#include "Graphics.h"
#include "KeyCodes.h"
#include "MemoryImage.h"
#include "WidgetManager.h"
#include "platform/VFS.h"

#include <SDL.h>
#include <cstring>

using namespace Sexy;

MahjongApp* Sexy::gApp = nullptr;

namespace mahjongjapan
{

// 0x40f540
jb::Layer* MahjongGame::CreateFirstLayer()
{
	return new jb::Splash(true);
}

// 0x40f5a0
void MahjongGame::StartGame()
{
	SwitchTo(new MainMenu());
}

// 0x40f620
void MahjongGame::OnInit()
{
	gPlayers.Load();
	int aCurrent = gPlayers.mCurrent;
	if (gPlayers.mCurrent != -1)
		gUser.Load(aCurrent, gPlayers.GetName(aCurrent));
}

// 0x40f650
void MahjongGame::OnExit()
{
	if (gPlayers.mCurrent != -1)
		gUser.Save();
}

} // namespace mahjongjapan

namespace
{

std::string GameFolder()
{
	std::string aDir = Platform::VFS::Get().GetBaseDir();
	if (!aDir.empty() && aDir.back() != '/' && aDir.back() != '\\')
		aDir += '/';
	return aDir;
}

std::string RegName(const char* theSection, const char* theName)
{
	if (theSection == nullptr || *theSection == 0)
		return theName;
	return std::string(theSection) + "\\" + theName;
}

// The fixes mod's "steadyClock": the game's clock, its frame pacing and its timers run on the updates (mFrameTime ms each)
// instead of the machine's clock, so the same input plays the same game on every run. "seededDeals": a seed given to
// the run (-seed=N, the site's ?seed=N) seeds every deal and shuffle, where the original seeds them from the clock.
static bool gSteadyClock = false;
static bool gSeededDeals = false;
static uint32_t gSeedCount = 0;

static uint32_t GameClock()
{
	if (gSteadyClock)
		return 1000 + (uint32_t)gApp->mUpdateCount * (uint32_t)gApp->mFrameTime;
	return gApp->GetClockTime();
}

// The framework's side of jb::Host
class MahjongHost : public jb::Host
{
public:
	mahjongjapan::PeResources mExe;
	mahjongjapan::PeResources mDll;

	uint32_t TickCount() override
	{
		return GameClock();
	}

	// the deal's time limit (1 or 2 s) runs on the machine's clock even in a -fast run, whose virtual clock does not move
	// inside a frame: a deal that finds no order would never end
	uint32_t RealTickCount() override
	{
		return SDL_GetTicks();
	}

	uint32_t SeedTick() override
	{
		if (gSeededDeals && gApp->mHasSeedParam)
			return (uint32_t)gApp->mRandSeed + gSeedCount++;
		return GameClock();
	}

	const uint8_t* FindResource(jb::Module theModule, const char* theTypeName, int theTypeId, int theId,
		uint32_t* theSize) override
	{
		const mahjongjapan::PeResources& aFile = theModule == jb::Module::Dll ? mDll : mExe;
		if (theTypeName != nullptr)
			return aFile.Find(mahjongjapan::PeResources::Key(theTypeName),
				mahjongjapan::PeResources::Key((uint32_t)theId), theSize);
		return aFile.Find(mahjongjapan::PeResources::Key((uint32_t)theTypeId),
			mahjongjapan::PeResources::Key((uint32_t)theId), theSize);
	}

	bool LoadString(jb::Module theModule, int theId, std::string& theText) override
	{
		return (theModule == jb::Module::Dll ? mDll : mExe).LoadString((uint32_t)theId, theText);
	}

	// the registry keys are read from the same emulated registry
	bool RegReadInt(const char* theSection, const char* theName, bool theMachine, int& theValue) override
	{
		return gApp->RegistryReadInteger(RegName(theSection, theName), &theValue);
	}

	bool RegReadString(const char* theSection, const char* theName, bool theMachine, std::string& theValue) override
	{
		return gApp->RegistryReadString(RegName(theSection, theName), &theValue);
	}

	bool RegWriteInt(const char* theSection, const char* theName, int theValue) override
	{
		return gApp->RegistryWriteInteger(RegName(theSection, theName), theValue);
	}

	bool RegWriteString(const char* theSection, const char* theName, const char* theValue) override
	{
		return gApp->RegistryWriteString(RegName(theSection, theName), theValue);
	}

	bool RegDeleteValue(const char* theSection, const char* theName) override
	{
		gApp->RegistryEraseValue(RegName(theSection, theName));
		return true;
	}

	bool RegDeleteKey(const char* theSection) override
	{
		return gApp->RegistryEraseKey(theSection != nullptr ? theSection : "");
	}

	std::string CommandLine() override
	{
		return "";
	}

	// the players, their settings and saved games, and the save slots' pictures: all in the user folder
	std::string FolderPath(int theCsidl) override
	{
		std::string aDir = Platform::VFS::Get().GetUserDir();
		if (!aDir.empty() && aDir.back() != '/' && aDir.back() != '\\')
			aDir += '/';
		return aDir;
	}

	std::string ExeFolder() override
	{
		return GameFolder();
	}

	void ScreenSize(int& theWidth, int& theHeight) override
	{
		SDL_DisplayMode aMode;
		if (SDL_GetDesktopDisplayMode(0, &aMode) == 0 && aMode.w > 0 && aMode.h > 0)
		{
			theWidth = aMode.w;
			theHeight = aMode.h;
			return;
		}
		theWidth = 800;
		theHeight = 600;
	}

	// test runs (-script, -fast, rundesk) never block on a message box; MB_YESNO's answer is IDYES
	int MessageBox(const char* theText, const char* theCaption, unsigned int theFlags) override
	{
		SDL_Log("%s: %s", theCaption, theText);
		if (!gApp->mFastMode && !gApp->IsTestRun())
			SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_WARNING, theCaption, theText, nullptr);
		return (theFlags & 0xf) == 4 ? 6 : 1;
	}

	void Log(const char* theText) override
	{
		SDL_Log("%s", theText);
	}

	int InputFlags() override
	{
		return gApp->InputFlags();
	}

	void GetMousePos(int& theX, int& theY, bool& theOutside) override
	{
		theX = gApp->mWidgetManager->mLastMouseX;
		theY = gApp->mWidgetManager->mLastMouseY;
		theOutside = !gApp->mWidgetManager->mMouseIn;
	}

	void SetCursor(int theKind) override
	{
		gApp->SetCursor(theKind == 1 ? CURSOR_HAND : theKind == 4 ? CURSOR_WAIT : CURSOR_POINTER);
	}

	// the framework keeps the mouse's events going to the widget while a button is held
	void CaptureMouse(bool theOn) override
	{
	}

	bool GetClipboard(std::string& theText) override
	{
		theText = gApp->GetClipboard();
		return true;
	}

	bool SetClipboard(const char* theText) override
	{
		gApp->CopyToClipboard(theText);
		return true;
	}

	// the frame's pacing: the next frame waits until then (RunFrame)
	void Sleep(uint32_t theMs) override
	{
		gApp->mNextFrame = GameClock() + theMs;
	}

	void SetFullScreen(bool theOn) override
	{
		if (theOn == !gApp->mIsWindowed)
			return;
		gApp->mApplyingGameMode = true;
		gApp->SwitchScreenMode(!theOn);
		gApp->mApplyingGameMode = false;
	}

	void Present(const uint8_t* thePixels, int thePitch) override
	{
		MahjongWidget* aWidget = gApp->mWidget;
		if (aWidget == nullptr)
			return;
		for (int y = 0; y < 600; y++)
			memcpy(&aWidget->mScreen[y * 800], thePixels + y * thePitch, 800 * 4);
		aWidget->mScreenChanged = true;
	}
};

MahjongHost gHost;

} // namespace

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// The widget: the window procedure and the screen

MahjongWidget::MahjongWidget()
{
	mScreenImage = new MemoryImage(gApp);
	mScreenImage->Create(800, 600);
	mScreen.assign(800 * 600, 0);
	mScreenChanged = true;
	mButtons = 0;
	mWantsFocus = true;
}

MahjongWidget::~MahjongWidget()
{
	delete mScreenImage;
}

void MahjongWidget::Update()
{
	Widget::Update();
	gApp->RunFrame();
	if (mScreenChanged)
		MarkDirty();
}

void MahjongWidget::Draw(Graphics* g)
{
	if (mScreenChanged)
	{
		ulong* aBits = mScreenImage->GetBits();
		for (int i = 0; i < 800 * 600; i++)
			aBits[i] = 0xff000000 | (mScreen[i] & 0xffffff);
		mScreenImage->BitsChanged();
		mScreenChanged = false;
	}
	g->DrawImage(mScreenImage, 0, 0);
}

// WM_CHAR
void MahjongWidget::KeyChar(SexyChar theChar)
{
	uint8_t aChar = (uint8_t)theChar;
	if (aChar != 0 && gApp->mGameStarted && !gApp->mGameEnded)
		jb::gApp->mManager.Char(aChar, 1, 0);
}

// WM_KEYDOWN (the framework's key codes are the virtual keys)
void MahjongWidget::KeyDown(KeyCode theKey)
{
	if (gApp->mGameStarted && !gApp->mGameEnded)
		jb::gApp->mManager.Key((int)theKey, 1, 0);
}

void MahjongWidget::MouseMove(int x, int y)
{
	if (gApp->mGameStarted && !gApp->mGameEnded)
		jb::gApp->mManager.MouseMove(gApp->InputFlags(), x, y);
}

void MahjongWidget::MouseDrag(int x, int y)
{
	MouseMove(x, y);
}

// The framework's buttons: 0 left, 1 right, 2 middle. A second click comes as the double click alone, as Windows
// sends WM_LBUTTONDBLCLK in place of the second WM_LBUTTONDOWN.
void MahjongWidget::MouseDown(int x, int y, int theBtnNum, int theClickCount)
{
	if (theBtnNum == 0)
		mButtons |= 1;
	else if (theBtnNum == 1)
		mButtons |= 2;
	else
		mButtons |= 0x10;
	if (!gApp->mGameStarted || gApp->mGameEnded)
		return;
	jb::Manager& aManager = jb::gApp->mManager;
	int aFlags = gApp->InputFlags();
	if (theBtnNum == 0)
	{
		if (theClickCount == 2)
			aManager.LButtonDbl(aFlags, x, y);
		else
			aManager.LButtonDown(aFlags, x, y);
	}
	else if (theBtnNum == 1)
	{
		if (theClickCount == 2)
			aManager.RButtonDbl(aFlags, x, y);
		else
			aManager.RButtonDown(aFlags, x, y);
	}
}

void MahjongWidget::MouseUp(int x, int y, int theBtnNum, int theClickCount)
{
	if (theBtnNum == 0)
		mButtons &= ~1;
	else if (theBtnNum == 1)
		mButtons &= ~2;
	else
		mButtons &= ~0x10;
	if (!gApp->mGameStarted || gApp->mGameEnded)
		return;
	jb::Manager& aManager = jb::gApp->mManager;
	int aFlags = gApp->InputFlags();
	if (theBtnNum == 0)
		aManager.LButtonUp(aFlags, x, y);
	else if (theBtnNum == 1)
		aManager.RButtonUp(aFlags, x, y);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// The app

MahjongApp::MahjongApp()
{
	gApp = this;
	mProdName = "Mahjong Escape Ancient Japan";
	mTitle = "Mahjong Escape: Ancient Japan";
	// the original's key: Software\JollyBear\ and the title with only its letters, digits and spaces (0x429ac0)
	mRegKey = "JollyBear\\Mahjong Escape Ancient Japan";
	mWidth = 800;
	mHeight = 600;
	// the frame's pacing (25 ms, jb::Host::Sleep) is checked every 5 ms
	mFrameTime = 5;
	mWidget = nullptr;
	mGame = nullptr;
	mGameStarted = false;
	mGameEnded = false;
	mApplyingGameMode = false;
	mNextFrame = 0;
}

MahjongApp::~MahjongApp()
{
	if (mWidget != nullptr)
	{
		mWidgetManager->RemoveWidget(mWidget);
		delete mWidget;
	}
	delete mGame;
}

void MahjongApp::Init()
{
	SexyAppBase::Init();
	if (mShutdown)
		return;
	std::string aFolder = GameFolder();
	if (!gHost.mExe.Open(aFolder + "MahjongEscapeAJ.exe") || !gHost.mDll.Open(aFolder + "Resource.dll"))
	{
		SDL_Log("MahjongEscapeAJ.exe or Resource.dll is missing from %s", aFolder.c_str());
		Shutdown();
		return;
	}
	mahjongjapan::LoadGameTexts(aFolder);
	jb::SetHost(&gHost);
	// the fixes mod (games/mahjongjapan/mods/fixes; docs/MOD_ROADMAP.md): none without a mod folder, and the game is the original
	jb::gSkipLogos = FixOn("skipLogos");
	gSteadyClock = !mFastMode && FixOn("steadyClock");  // a -fast run is steady already
	gSeededDeals = FixOn("seededDeals");

	mWidget = new MahjongWidget();
	mWidget->Resize(0, 0, mWidth, mHeight);
	mWidgetManager->AddWidget(mWidget);
	mWidgetManager->SetFocus(mWidget);

#ifdef __EMSCRIPTEN__
	// A browser enters full screen only from a click, so the game starts in the page, as every game's web build does
	// (SexyAppBase::MakeWindow); the page's full screen button and the options switch it later
	gHost.RegWriteInt(nullptr, "FullScreen", 0);
#endif
	mGame = new mahjongjapan::MahjongGame();
	if (!mGame->Init())
	{
		mGameEnded = true;
		Shutdown();
		return;
	}
	mGameStarted = true;
}

// -gameseed=N (the site's ?seed=N): the seed of the fixes mod's "seededDeals", as -seed=N is for a native run
void MahjongApp::HandleCmdLineParam(const std::string& theParamName, const std::string& theParamValue)
{
	if (theParamName == "-gameseed")
	{
		mRandSeed = strtoul(theParamValue.c_str(), nullptr, 10);
		mHasSeedParam = true;
		return;
	}
	SexyAppBase::HandleCmdLineParam(theParamName, theParamValue);
}

void MahjongApp::Shutdown()
{
	if (mGameStarted && !mGameEnded)
	{
		mGameEnded = true;
		if (mGame->mManager.mRunning)
			mGame->mManager.Shutdown();
		mGame->Exit();
	}
	SexyAppBase::Shutdown();
}

// The engine keeps its own settings in the key (0x426060 reads them, 0x4266c0 writes them at the exit). The framework's
// options would share it, and its MusicVolume, written after the engine's, would undo a change made in the options.
void MahjongApp::ReadFromRegistry()
{
	mReadFromRegistry = true;
}

void MahjongApp::WriteToRegistry()
{
}

void MahjongApp::RunFrame()
{
	if (!mGameStarted || mGameEnded)
		return;
	if (!mGame->mManager.mRunning)
	{
		// the game quit (App::Quit): the settings are saved as the original's WinMain did after its loop
		mGameEnded = true;
		mGame->Exit();
		Shutdown();
		return;
	}
	if ((int)(GameClock() - mNextFrame) < 0)
		return;
	mGame->mManager.Frame();
	TestFrame();
}

int MahjongApp::InputFlags()
{
	int aFlags = mWidget != nullptr ? mWidget->mButtons : 0;
	if (mWidgetManager->mKeyDown[KEYCODE_SHIFT])
		aFlags |= 4;
	if (mWidgetManager->mKeyDown[KEYCODE_CONTROL])
		aFlags |= 8;
	return aFlags;
}

// The window's close button: the game quits (App::Quit, CanQuit is always true)
void MahjongApp::CloseRequestAsync()
{
	if (mGameStarted && !mGameEnded)
		mGame->Quit();
	else
		SexyAppBase::CloseRequestAsync();
}

void MahjongApp::GotFocus()
{
	SexyAppBase::GotFocus();
	if (mGameStarted && !mGameEnded)
		mGame->mManager.Activate(true);
}

void MahjongApp::LostFocus()
{
	SexyAppBase::LostFocus();
	if (mGameStarted && !mGameEnded)
		mGame->mManager.Activate(false);
}

// Alt+Enter and the web page's full screen button change the mode as the options' check box does
void MahjongApp::SwitchScreenMode(bool wantWindowed, bool is3d, bool force)
{
	if (mApplyingGameMode || !mGameStarted || mGameEnded || force)
	{
		SexyAppBase::SwitchScreenMode(wantWindowed, is3d, force);
		return;
	}
	if (wantWindowed != mIsWindowed)
		mGame->mManager.SetFullScreen(!wantWindowed, true);
}
