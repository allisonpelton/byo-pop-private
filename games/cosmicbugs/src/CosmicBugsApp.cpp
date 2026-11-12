// The port's host for Cosmic Bugs (see CosmicBugsApp.h).

#include "CosmicBugsApp.h"
#include "CBGlobals.h"
#include "CBLevel.h"
#include "CBFixes.h"
#include "CBTestCommands.h"
#include "R64.h"
#include "R64Sys.h"
#include "DDInterface.h"
#include "Graphics.h"
#include "MemoryImage.h"
#include "MusicInterface.h"
#include "SoundInstance.h"
#include "SoundManager.h"
#include "WidgetManager.h"
#include "platform/ModManager.h"
#include "platform/VFS.h"
#include "WebSettings.h"

#include <SDL.h>
#include <cstdio>
#include <cstring>
#include <string>

using namespace Sexy;

CosmicBugsApp* Sexy::gApp = nullptr;

namespace
{

// BASS_SetVolume(60) at the start (0x42ad70): the original plays everything at 60%
const double kOutputVolume = 0.6;

r64::PixelFormat gScreenFormat565;

// The framework's side of the engine's system layer
class CosmicBugsHost : public r64::SysHost
{
public:
	bool ReadFile(const char* thePath, std::vector<uint8_t>& theData) override
	{
		return Platform::VFS::Get().ReadAll(thePath, theData);
	}

	void AddUpdateRect(int x, int y, int w, int h) override
	{
		gApp->AddUpdateRect(x, y, w, h);
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
		*thePitch = 800 * 2;
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

	// The site's "Always skip start-up logos" (framework/WebSettings.h: the page sets it before the game starts, the exe takes
	// -skiplogos), which the logo screens read (GameStep)
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
		bool ok = gApp->mMusicInterface != nullptr && gApp->mMusicInterface->LoadMusic(0, thePath);
		if (!ok)
			SDL_Log("cannot load the music %s", thePath);
		return ok;
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
			gApp->mMusicInterface->SetVolume(kOutputVolume * theVolume / 100.0);
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

// The 16-bit screen's pixels as the framework's 32 bits (565 widened as Draw did with shifts: each channel's high bits
// repeated into the low ones), made once; an alpha of 0xff on every one
static uint32_t gPixelsFromScreen16[65536];

static void BuildPixelsFromScreen16()
{
	for (uint32_t p = 0; p < 65536; p++)
	{
		uint32_t aRed = (p >> 11) & 0x1f;
		uint32_t aGreen = (p >> 5) & 0x3f;
		uint32_t aBlue = p & 0x1f;
		gPixelsFromScreen16[p] = 0xff000000 | (((aRed << 3) | (aRed >> 2)) << 16) | (((aGreen << 2) | (aGreen >> 4)) << 8) |
			((aBlue << 3) | (aBlue >> 2));
	}
}

CosmicBugsWidget::CosmicBugsWidget()
{
	mScreenImage = new MemoryImage(gApp);
	mScreenImage->Create(800, 600);
	// opaque, so that the framework need not look through the 480000 pixels for alpha after each change, and with every
	// alpha byte 0xff (Draw), so that it need not copy and premultiply them pixel by pixel in strips to upload them (where
	// premultiplying changes nothing): the same pixels reach the screen, in one upload (Water Bugs' and the Mahjong ports' screen)
	mScreenImage->SetImageMode(false, false);
	mScreenImage->mUploadAsIs = true;
	BuildPixelsFromScreen16();
	mWantsFocus = true;
}

CosmicBugsWidget::~CosmicBugsWidget()
{
	delete mScreenImage;
}

void CosmicBugsWidget::Update()
{
	Widget::Update();
	gApp->RunFrame();
	MarkDirty();
}

// What the original presented since the last copy (all of it when nothing said which part) goes to the image
void CosmicBugsWidget::Draw(Graphics* g)
{
	CosmicBugsApp* anApp = gApp;
	if (anApp->mPresented)
	{
		int l = 0, t = 0, r = 800, b = 600;
		if (!anApp->mWholeScreen && anApp->mDirtyRight > anApp->mDirtyLeft)
		{
			l = anApp->mDirtyLeft;
			t = anApp->mDirtyTop;
			r = anApp->mDirtyRight;
			b = anApp->mDirtyBottom;
		}
		ulong* aBits = mScreenImage->GetBits();
		for (int y = t; y < b; y++)
		{
			const uint16_t* aSrc = &anApp->mScreen[y * 800];
			ulong* aDst = &aBits[y * 800];
			for (int x = l; x < r; x++)
			{
				aDst[x] = gPixelsFromScreen16[aSrc[x]];
			}
		}
		mScreenImage->BitsChanged();
		anApp->mPresented = false;
		anApp->mWholeScreen = false;
		anApp->mDirtyLeft = anApp->mDirtyTop = 800;
		anApp->mDirtyRight = anApp->mDirtyBottom = 0;
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
	mWidth = 800;
	mHeight = 600;
	mWidget = nullptr;
	mGameStarted = false;
	mGameEnded = false;
	mApplyingGameMode = false;
	mScreen.assign(800 * 600, 0);
	mPresented = false;
	mWholeScreen = false;
	mDirtyLeft = mDirtyTop = 800;
	mDirtyRight = mDirtyBottom = 0;
	mMouseX = 400;
	mMouseY = 300;
}

CosmicBugsApp::~CosmicBugsApp()
{
	if (mWidget != nullptr)
	{
		mWidgetManager->RemoveWidget(mWidget);
		delete mWidget;
	}
}

// The fixes mod's toggles (games/cosmicbugs/mods/fixes), from the highest-priority enabled mod that has the feature
// "fixes"; without one every fix stays off and the game plays as the original does
static void LoadFixes()
{
	cb::gFixes = cb::Fixes();
	for (const Platform::ModInfo* aMod : Platform::ModManager::Get().GetEnabledMods())
	{
		if (!aMod->mFeaturesJson.IsObject() || !aMod->mFeaturesJson.Has("fixes"))
			continue;
		Platform::JsonValue aTurns = Platform::ModManager::Get().GetLiveValue(aMod->mId, "walkerTurnMemory");
		Platform::JsonValue aShield = Platform::ModManager::Get().GetLiveValue(aMod->mId, "shieldFlash");
		cb::gFixes.walkerTurnMemory = aTurns.IsBool() && aTurns.AsBool(false);
		cb::gFixes.shieldFlash = aShield.IsBool() && aShield.AsBool(false);
		break;
	}
	SDL_Log("fixes: walkerTurnMemory %s, shieldFlash %s", cb::gFixes.walkerTurnMemory ? "on" : "off",
		cb::gFixes.shieldFlash ? "on" : "off");
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
		mSoundManager->SetVolume(kOutputVolume);
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
	// the site's "Full screen at start: Windowed" (framework/WebSettings.h): the game's own option, set as its menu sets it
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
}

void CosmicBugsApp::AddUpdateRect(int x, int y, int w, int h)
{
	if (x < mDirtyLeft)
		mDirtyLeft = x < 0 ? 0 : x;
	if (y < mDirtyTop)
		mDirtyTop = y < 0 ? 0 : y;
	if (x + w > mDirtyRight)
		mDirtyRight = x + w > 800 ? 800 : x + w;
	if (y + h > mDirtyBottom)
		mDirtyBottom = y + h > 600 ? 600 : y + h;
}

void CosmicBugsApp::Present()
{
	if (mDirtyRight <= mDirtyLeft)
		mWholeScreen = true;
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
void CosmicBugsApp::SwitchScreenMode(bool wantWindowed, bool is3d, bool force)
{
	if (mApplyingGameMode || !mGameStarted || mGameEnded || force)
	{
		SexyAppBase::SwitchScreenMode(wantWindowed, is3d, force);
		return;
	}
	if (wantWindowed != mIsWindowed)
		r64::Screen_SetFullscreen(wantWindowed ? 0 : 1);
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
		cb::Level* aLevel = cb::gLevels.current;
		SDL_Log("state: score %d, lives %d, board %d, filled %d of %d%%", cb::gScore, cb::gLives, cb::gLevelInSet,
			aLevel != nullptr ? aLevel->filledCells : -1, aLevel != nullptr ? aLevel->fillQuota : -1);
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
