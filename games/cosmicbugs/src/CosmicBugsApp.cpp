// port: the host for Cosmic Bugs (see CosmicBugsApp.h).

#include "CosmicBugsApp.h"
#include "CBGlobals.h"
#include "CBLevel.h"
#include "CBFixes.h"
#include "CBTestCommands.h"
#include "LiveSplitObserver.h"
#include "R64.h"
#include "R64Sys.h"
#include "DDInterface.h"
#ifndef __EMSCRIPTEN__
#include "SurfaceDisplay.h"
#endif
#include "Graphics.h"
#include "MemoryImage.h"
#include "MusicInterface.h"
#include "SoundInstance.h"
#include "SoundManager.h"
#include "WidgetManager.h"
#include "platform/GameOptions.h"
#include "platform/VFS.h"
#include "WebSettings.h"

#include <SDL.h>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>

using namespace Sexy;

CosmicBugsApp* Sexy::gApp = nullptr;

namespace
{

// BASS_SetVolume(60) at the start (0x42ad70): the original plays everything at 60%
const double OUTPUT_VOLUME = 0.6;

r64::PixelFormat gScreenFormat565;

// The framework's side of the engine's system layer
class CosmicBugsHost : public r64::SysHost
{
public:
	bool ReadFile(const char* thePath, std::vector<uint8_t>& theData) override
	{
		return Platform::VFS::Get().ReadAll(thePath, theData);
	}

	void SetMousePos(int x, int y) override
	{
		gApp->WarpMouse(x, y);
	}

	void GetMousePos(int* x, int* y) override
	{
		*x = gApp->mMouseX;
		*y = gApp->mMouseY;
	}

	bool LockScreen(uint8_t** theBits, int* thePitch) override
	{
		*theBits = (uint8_t*)gApp->mScreen.data();
		*thePitch = r64::SCREEN_WIDTH * 2;
		return true;
	}

	void Present(int) override
	{
		gApp->Present();
	}

	void SetScreenMode(int theFullscreen, int) override
	{
		gApp->SetGameScreenMode(theFullscreen != 0);
	}

	uint32_t TimeMs() override
	{
		return gApp->GetClockTime();
	}

	// The original's activation (its WndProc's WM_ACTIVATEAPP) is the framework's mActive, set from SDL's focus
	// events, on the web too (the page's window losing the focus freezes the game as the original's does; the test
	// kit's blur and focus move it there). A test run's window may never be active, except in a -realinput run, where
	// only the file's focus events reach mActive and so say what the window has
	bool IsActive() override
	{
		return gApp->mActive || (gApp->IsTestRun() && gApp->mRealInputFileName.empty());
	}

	// The site's "Always skip start-up logos" (framework/WebSettings.h: the page sets it before the game starts, the
	// exe takes -skiplogos), which the logo screens read (GameStep)
	bool SkipLogos() override
	{
		return Byopop::SiteSkipLogos();
	}

	int LoadSound(const char* thePath) override
	{
		int anId = gApp->mSoundManager != nullptr ? gApp->mSoundManager->LoadSound(thePath) : -1;
		if (anId < 0)
			SDL_Log("cannot load the sound %s", thePath);
		return anId;
	}

	void PlaySound(int theSound) override
	{
		if (gApp->mSoundManager == nullptr)
			return;
		SoundInstance* anInstance = gApp->mSoundManager->GetSoundInstance((unsigned)theSound);
		if (anInstance != nullptr)
			anInstance->Play(false, true);
	}

	void StopSounds() override
	{
		if (gApp->mSoundManager != nullptr)
			gApp->mSoundManager->StopAllSounds();
	}

	bool LoadMusic(const char* thePath) override
	{
		bool anOk = gApp->mMusicInterface != nullptr && gApp->mMusicInterface->LoadMusic(0, thePath);
		if (!anOk)
			SDL_Log("cannot load the music %s", thePath);
		return anOk;
	}

	void FreeMusic() override
	{
		if (gApp->mMusicInterface != nullptr)
			gApp->mMusicInterface->UnloadMusic(0);
	}

	void PlayMusic(int theRestart) override
	{
		if (gApp->mMusicInterface == nullptr)
			return;
		if (theRestart != 0)
			gApp->mMusicInterface->PlayMusic(0, 0, false);
		else
			gApp->mMusicInterface->ResumeMusic(0);
	}

	void PauseMusic() override
	{
		if (gApp->mMusicInterface != nullptr)
			gApp->mMusicInterface->PauseMusic(0);
	}

	void SetMusicVolume(int theVolume) override
	{
		if (gApp->mMusicInterface != nullptr)
			gApp->mMusicInterface->SetVolume(OUTPUT_VOLUME * theVolume / 100.0);
	}

	const char* SavePath(const char* theName) override
	{
		static std::string sPaths[4];
		static int sNext = 0;
		std::string& aPath = sPaths[sNext];
		sNext = (sNext + 1) % 4;
		aPath = Platform::VFS::Get().GetUserDir() + "/" + theName;
		return aPath.c_str();
	}

	void SaveDone() override
	{
		gApp->SyncUserData();
	}

	// a test run (and the web) types the same name on every machine
	void UserName(char* theName, int theSize) override
	{
		const char* aName = nullptr;
#if !defined(__EMSCRIPTEN__)
		if (!gApp->IsTestRun())
		{
#ifdef _WIN32
			aName = SDL_getenv("USERNAME");
#else
			aName = SDL_getenv("USER");
#endif
		}
#endif
		if (aName == nullptr)
			aName = "";
		std::strncpy(theName, aName, theSize - 1);
		theName[theSize - 1] = 0;
	}
};

CosmicBugsHost gHost;

} // namespace

CosmicBugsWidget::CosmicBugsWidget()
{
	mScreenImage = nullptr;
	mWantsFocus = true;
#ifndef __EMSCRIPTEN__
	// the window's display keeps the screen itself (CreateDDInterface), so the widget has no picture of it
	if (dynamic_cast<SurfaceDisplay*>(gApp->mDDInterface) != nullptr)
	{
		mPixels.assign(r64::SCREEN_WIDTH * r64::SCREEN_HEIGHT, 0xff000000);
		return;
	}
#endif
	mScreenImage = new MemoryImage(gApp);
	mScreenImage->Create(r64::SCREEN_WIDTH, r64::SCREEN_HEIGHT);
	// opaque, so that the framework need not look through the 480000 pixels for alpha after each change, and with every
	// alpha byte 0xff (ScreenDiff), so that it need not copy and premultiply them pixel by pixel in strips to upload them
	// (where premultiplying changes nothing): the same pixels reach the screen, in one upload (Water Bugs' and the
	// Mahjong ports' screen)
	mScreenImage->SetImageMode(false, false);
	mScreenImage->mUploadAsIs = true;
}

CosmicBugsWidget::~CosmicBugsWidget()
{
#ifndef __EMSCRIPTEN__
	// the screen's pixels go with the widget
	if (SurfaceDisplay* aDisplay = dynamic_cast<SurfaceDisplay*>(gApp->mDDInterface))
		aDisplay->SetFrame(nullptr, 0, nullptr, -1);
#endif
	delete mScreenImage;
}

void CosmicBugsWidget::Update()
{
	Widget::Update();
	gApp->RunFrame();
	MarkDirty();
}

// What the original presented since the last frame goes to the screen's pixels (ScreenDiff widens just what changed). A
// SurfaceDisplay is handed the pixels and the rectangles that changed (the first frame, the whole screen) and shows them
// itself; otherwise the widget's picture is drawn, and the framework uploads it if there were any changes
void CosmicBugsWidget::Draw(Graphics* g)
{
	CosmicBugsApp* anApp = gApp;
#ifndef __EMSCRIPTEN__
	if (SurfaceDisplay* aDisplay = dynamic_cast<SurfaceDisplay*>(anApp->mDDInterface))
	{
		if (anApp->mPresented)
		{
			mScreenDiff.Update(anApp->mScreen.data(), mPixels.data());
			if (mScreenDiff.Whole())
				aDisplay->SetFrame(mPixels.data(), r64::SCREEN_WIDTH * 4, nullptr, -1);
			else
			{
				mFrameRects.clear();
				for (const r64::ChangedRect& aRect : mScreenDiff.Rects())
					mFrameRects.push_back(Rect(aRect.mX, aRect.mY, aRect.mWidth, aRect.mHeight));
				aDisplay->SetFrame(mPixels.data(), r64::SCREEN_WIDTH * 4, mFrameRects.data(), (int)mFrameRects.size());
			}
			anApp->mPresented = false;
		}
		return;
	}
#endif
	if (anApp->mPresented)
	{
		static_assert(sizeof(ulong) == sizeof(uint32_t), "the image's pixels are 32 bits");
		mScreenDiff.Update(anApp->mScreen.data(), reinterpret_cast<uint32_t*>(mScreenImage->GetBits()));
		if (mScreenDiff.Whole() || !mScreenDiff.Rects().empty())
			mScreenImage->BitsChanged();
		anApp->mPresented = false;
	}
	g->DrawImage(mScreenImage, 0, 0);
}

// The framework's key codes are Windows' virtual keys, which the original's window procedure got
void CosmicBugsWidget::KeyDown(KeyCode theKey)
{
	r64::Sys_KeyEvent((int)theKey, 1);
}

void CosmicBugsWidget::KeyUp(KeyCode theKey)
{
	r64::Sys_KeyEvent((int)theKey, 0);
}

void CosmicBugsWidget::MouseMove(int x, int y)
{
	gApp->mMouseX = x;
	gApp->mMouseY = y;
}

void CosmicBugsWidget::MouseDrag(int x, int y)
{
	gApp->mMouseX = x;
	gApp->mMouseY = y;
}

// The framework's buttons: 0 left, 1 right, 2 middle; the engine's 1, 2 and 3
void CosmicBugsWidget::MouseDown(int x, int y, int theBtnNum, int)
{
	gApp->mMouseX = x;
	gApp->mMouseY = y;
	r64::Sys_MouseButton(theBtnNum + 1, 1);
}

void CosmicBugsWidget::MouseUp(int x, int y, int theBtnNum, int)
{
	gApp->mMouseX = x;
	gApp->mMouseY = y;
	r64::Sys_MouseButton(theBtnNum + 1, 0);
}

CosmicBugsApp::CosmicBugsApp()
{
	gApp = this;
	mProdName = "CosmicBugs";
	mTitle = "Cosmic Bugs";
	mRegKey = "PopCap\\CosmicBugs";
	mWidth = r64::SCREEN_WIDTH;
	mHeight = r64::SCREEN_HEIGHT;
	mWidget = nullptr;
	mGameStarted = false;
	mGameEnded = false;
	mApplyingGameMode = false;
	mScreen.assign(r64::SCREEN_WIDTH * r64::SCREEN_HEIGHT, 0);
	mPresented = false;
	mMouseX = r64::SCREEN_CENTRE_X;
	mMouseY = r64::SCREEN_CENTRE_Y;
}

CosmicBugsApp::~CosmicBugsApp()
{
	LiveSplitObserver::AppDestroyed(this);   // port: the autosplit block's last write (LiveSplitObserver.h)
	if (mWidget != nullptr)
	{
		mWidgetManager->RemoveWidget(mWidget);
		delete mWidget;
	}
}

#ifndef __EMSCRIPTEN__
DDInterface* CosmicBugsApp::CreateDDInterface()
{
	// test only: SURFACE_DISPLAY=off keeps the framework's own display and the widget's picture (the path before the
	// SurfaceDisplay), to measure and compare the two
	const char* aMode = SDL_getenv("SURFACE_DISPLAY");
	if ((aMode != nullptr) && (std::strcmp(aMode, "off") == 0))
		return SexyAppBase::CreateDDInterface();
	return new SurfaceDisplay(this);
}
#endif

// The fixes mod's toggles (games/cosmicbugs/mods/fixes), read once through Platform::FixOn; without them every fix stays off
static void LoadFixes()
{
	cb::gFixes = cb::Fixes();
	cb::gFixes.mWalkerTurnMemory = Platform::FixOn("walkerTurnMemory");
	cb::gFixes.mShieldFlash = Platform::FixOn("shieldFlash");
	SDL_Log("fixes: walkerTurnMemory %s, shieldFlash %s", cb::gFixes.mWalkerTurnMemory ? "on" : "off",
		cb::gFixes.mShieldFlash ? "on" : "off");
}

// 0x42ae60 (WinMain: cp.cfg, the window and the screen, the sound, the game; what follows GameMain is Shutdown)
void CosmicBugsApp::Init()
{
	SexyAppBase::Init();
	if (mShutdown)
		return;
	LoadFixes();
	r64::PixelFormat_Init(&gScreenFormat565, 16, 0xf800, 0x7e0, 0x1f);
	r64::Sys_SetScreenFormat(&gScreenFormat565);
	r64::Sys_SetHost(&gHost);
	if (mSoundManager != nullptr)
		mSoundManager->SetVolume(OUTPUT_VOLUME);
	// the game draws its own pointer
	SetCursor(CURSOR_NONE);

	mWidget = new CosmicBugsWidget();
	mWidget->Resize(0, 0, mWidth, mHeight);
	mWidgetManager->AddWidget(mWidget);
	mWidgetManager->SetFocus(mWidget);

	if (!cb::LoadGameTexts())
	{
		mGameEnded = true;
		Shutdown();
		return;
	}
	// WinMain: cp.cfg, the screen in its mode, the sound, then the game
	r64::LoadConfig();
#ifdef __EMSCRIPTEN__
	// the site's "Full screen at start: Windowed" (framework/WebSettings.h): the game's own option, set as its menu
	// sets it
	if (Byopop::SiteWindowed())
		r64::Screen_SetFullscreen(0);
#endif
	SetGameScreenMode(r64::Screen_Fullscreen() != 0);
	r64::Sound_Start();
	cb::GameStart();
	mGameStarted = true;
}

void CosmicBugsApp::Shutdown()
{
	if (mGameStarted && !mGameEnded)
	{
		mGameEnded = true;
		cb::GameEnd();
		r64::Sound_Shutdown();
	}
	SexyAppBase::Shutdown();
}

void CosmicBugsApp::HandleCmdLineParam(const std::string& theParamName, const std::string& theParamValue)
{
	if (theParamName == "-livesplitlog")
		LiveSplitObserver::SetLogFile(theParamValue);   // port: one line for each change the autosplit block shows (tests)
	else
		SexyAppBase::HandleCmdLineParam(theParamName, theParamValue);
}

void CosmicBugsApp::RunFrame()
{
	if (!mGameStarted || mGameEnded)
		return;
	if (cb::GameFrame() == 0)
	{
		mGameEnded = true;
		cb::GameEnd();
		r64::Sound_Shutdown();
		Shutdown();
	}
	else
		LiveSplitObserver::AfterUpdate(this);   // port: the autosplit block, after the update (LiveSplitObserver.h)
}

void CosmicBugsApp::Present()
{
	mPresented = true;
}

void CosmicBugsApp::SetGameScreenMode(bool theFullscreen)
{
#ifdef __EMSCRIPTEN__
	// The page's Fullscreen button owns the browser's full screen: SDL's request from the game (cp.cfg's default is
	// the full screen) comes outside a click, and the canvas stays black after it
	(void)theFullscreen;
	return;
#endif
	if (theFullscreen == !mIsWindowed)
		return;
	mApplyingGameMode = true;
	SwitchScreenMode(!theFullscreen);
	mApplyingGameMode = false;
}

// The original moves the system's pointer (0x428890); on the web, and in a test run, only the game's moves, until
// the real one does
void CosmicBugsApp::WarpMouse(int x, int y)
{
	mMouseX = x;
	mMouseY = y;
#ifndef __EMSCRIPTEN__
	if (IsTestRun() || !mActive || mDDInterface == nullptr || mHWnd == nullptr)
		return;
	DDInterface* aDisplay = mDDInterface;
	double aX = x;
	double aY = y;
	if (aDisplay->mPresentationRect.mWidth > 0 && aDisplay->mPresentationRect.mHeight > 0)
	{
		aX = aX * aDisplay->mPresentationRect.mWidth / aDisplay->mWidth + aDisplay->mPresentationRect.mX;
		aY = aY * aDisplay->mPresentationRect.mHeight / aDisplay->mHeight + aDisplay->mPresentationRect.mY;
	}
	int aWindowWidth = 0;
	int aWindowHeight = 0;
	SDL_GetWindowSize(mHWnd, &aWindowWidth, &aWindowHeight);
	if (aWindowWidth > 0 && aWindowHeight > 0 && aDisplay->mDisplayWidth > 0 && aDisplay->mDisplayHeight > 0)
	{
		aX = aX * aWindowWidth / aDisplay->mDisplayWidth;
		aY = aY * aWindowHeight / aDisplay->mDisplayHeight;
	}
	SDL_WarpMouseInWindow(mHWnd, (int)aX, (int)aY);
#endif
}

// The window's close button: WM_DESTROY's (0x42a7a0) end of the game
void CosmicBugsApp::CloseRequestAsync()
{
	if (mGameStarted && !mGameEnded)
		cb::gQuit = 1;
	else
		SexyAppBase::CloseRequestAsync();
}

// Alt+Enter goes to the game like its F7 (0x42a7a0); it then asks for the mode through SetScreenMode. (The web
// page's Fullscreen button works on the page and never reaches the game.)
void CosmicBugsApp::SwitchScreenMode(bool theWantWindowed, bool theIs3d, bool theForce)
{
	if (mApplyingGameMode || !mGameStarted || mGameEnded || theForce)
	{
		SexyAppBase::SwitchScreenMode(theWantWindowed, theIs3d, theForce);
		return;
	}
	if (theWantWindowed != mIsWindowed)
		r64::Screen_SetFullscreen(theWantWindowed ? 0 : 1);
}

// The script's "rdown"/"rup" [<x> <y>]: the right button pressed or let go (there, else where the pointer is, which the
// game's own warps may have moved). The game polls its buttons once an update, so the framework's rclick (pressed and
// let go in one update) never reaches it. The standard test commands (docs/TEST_COMMANDS.md: state, goto, level, unlock
// all, set, win, lose, seed, move) are CBTestCommands.cpp's; each answers on the log with its own line(s). "state" also
// prints the older one-line summary the suite reads.
bool CosmicBugsApp::HandleScriptCommand(const ScriptCommand& theCommand)
{
	if (theCommand.mCommand == "rdown" || theCommand.mCommand == "rup")
	{
		int x = 0, y = 0;
		if (std::sscanf(theCommand.mArgs.c_str(), "%d %d", &x, &y) == 2)
		{
			mMouseX = x;
			mMouseY = y;
		}
		r64::Sys_MouseButton(2, theCommand.mCommand == "rdown" ? 1 : 0);
		return true;
	}
	if (theCommand.mCommand == "state")
	{
		cb::Level* aLevel = cb::gLevels.mCurrent;
		SDL_Log("state: score %d, lives %d, board %d, filled %d of %d%%", cb::gScore, cb::gLives, cb::gLevelInSet,
			aLevel != nullptr ? aLevel->mFilledCells : -1, aLevel != nullptr ? aLevel->mFillQuota : -1);
	}
	std::vector<std::string> aReplies;
	if (!cb::TestCommand(theCommand.mCommand, theCommand.mArgs, aReplies))
		return SexyAppBase::HandleScriptCommand(theCommand);
	// (on the log, like the framework's own answers: a printf reached the web page's test kit only on some page loads,
	// the log's lines always)
	for (const std::string& aLine : aReplies)
		SDL_Log("%s", aLine.c_str());
	return true;
}
