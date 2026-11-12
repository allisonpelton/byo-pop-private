// port: the host for Water Bugs (see WaterBugsApp.h).

#include "WaterBugsApp.h"
#include "platform/GameOptions.h"
#include "WBGlobals.h"
#include "WBLevel.h"
#include "testhooks/TestCommands.h"
#include "R64.h"
#include "R64Sys.h"
#include "DDInterface.h"
#include "Graphics.h"
#include "MemoryImage.h"
#include "MusicInterface.h"
#include "SoundInstance.h"
#include "SoundManager.h"
#include "WidgetManager.h"
#include "platform/VFS.h"
#include "WebSettings.h"

#include <SDL.h>
#include <cstdint>
#include <cstring>
#include <string>

using namespace Sexy;

WaterBugsApp* Sexy::gApp = nullptr;

namespace
{

// BASS_SetVolume(60) at the start (0x42b250): the original plays everything at 60%
const double OUTPUT_VOLUME = 0.6;

r64::PixelFormat gScreenFormat565;

// fixes mod, "steadyClock": the game's clock runs on the framework's updates (mFrameTime ms each) instead of the
// machine's
bool gSteadyClock = false;

// The framework's side of the engine's system layer
class WaterBugsHost : public r64::SysHost
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
		if (gSteadyClock)
			return 1000u + (uint32_t)gApp->mUpdateCount * (uint32_t)gApp->mFrameTime;
		return gApp->GetClockTime();
	}

	bool FixOn(const char* theFix) override
	{
		return Platform::FixOn(theFix);
	}

	// the site's "Always skip start-up logos", or -skiplogos (framework/WebSettings.h)
	bool SkipLogos() override
	{
		return Byopop::SiteSkipLogos();
	}

	// The original's activation (its WndProc's WM_ACTIVATEAPP) is the framework's mActive, set from SDL's focus events:
	// the game stops while the window is not the active one, natively and on the web (where the page's canvas losing
	// the focus is the same). A test run's window may never be active, except in a -realinput run, where the file's
	// focus lines are the only focus events there are (docs/REAL_INPUT.md)
	bool IsActive() override
	{
#ifdef __EMSCRIPTEN__
		return gApp->mActive;
#else
		return gApp->mActive || (gApp->IsTestRun() && gApp->mRealInputFileName.empty());
#endif
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

WaterBugsHost gHost;

} // namespace

namespace
{

// A 5-6-5 pixel as the framework's 32-bit picture holds it: opaque, each component widened by repeating its top bits
uint32_t gPixelsFromScreen16[65536];

void BuildPixelsFromScreen16()
{
	for (uint32_t p = 0; p < 65536; p++)
	{
		uint32_t aRed = (p >> 11) & 0x1f;
		uint32_t aGreen = (p >> 5) & 0x3f;
		uint32_t aBlue = p & 0x1f;
		gPixelsFromScreen16[p] = 0xff000000 | (((aRed << 3) | (aRed >> 2)) << 16) |
			(((aGreen << 2) | (aGreen >> 4)) << 8) |
			((aBlue << 3) | (aBlue >> 2));
	}
}

} // namespace

WaterBugsWidget::WaterBugsWidget()
{
	mScreenImage = new MemoryImage(gApp);
	mScreenImage->Create(r64::SCREEN_WIDTH, r64::SCREEN_HEIGHT);
	// opaque, so that the framework need not look through the 480000 pixels for alpha after each change, and with every
	// alpha byte 0xff (Draw), so that it need not premultiply them pixel by pixel to upload them (the Mahjong ports'
	// screen)
	mScreenImage->SetImageMode(false, false);
	mScreenImage->mUploadAsIs = true;
	BuildPixelsFromScreen16();
	mWantsFocus = true;
}

WaterBugsWidget::~WaterBugsWidget()
{
	delete mScreenImage;
}

void WaterBugsWidget::Update()
{
	Widget::Update();
	gApp->RunFrame();
	MarkDirty();
}

static_assert(r64::SCREEN_WIDTH % WaterBugsWidget::CHANGE_BLOCK == 0, "a row has whole blocks");
static_assert(WaterBugsWidget::CHANGE_BLOCK % 8 == 0, "a block is whole 16-byte vectors");

// Sixteen bytes (eight pixels) at a time, in the compiler's vector type (one SSE2 or WebAssembly SIMD register, else
// plain words), loaded from any address
typedef uint32_t PixelVector __attribute__((vector_size(16), aligned(4), may_alias));

// Whether two runs of the screen's pixels (a multiple of eight of them) are the same
static bool SamePixels(const uint16_t* theFirst, const uint16_t* theSecond, int theCount)
{
	const PixelVector* aFirst = reinterpret_cast<const PixelVector*>(theFirst);
	const PixelVector* aSecond = reinterpret_cast<const PixelVector*>(theSecond);
	PixelVector aDifference = {0, 0, 0, 0};
	for (int i = 0; i < theCount / 8; i++)
		aDifference |= aFirst[i] ^ aSecond[i];
	return (aDifference[0] | aDifference[1] | aDifference[2] | aDifference[3]) == 0;
}

// The blocks of the screen that differ from the copy the image was last widened from. The engine's dirty rectangles
// say what it copies from its back buffer, not everything it drew (it draws sprites, beams, text and fades straight
// into the screen, some outside them), so the screen itself is compared: a row at a time, and the rows that differ a
// block at a time
void WaterBugsWidget::FindChangedBlocks()
{
	std::memset(mChangedBlocks, 0, sizeof(mChangedBlocks));
	const uint16_t* aScreen = gApp->mScreen.data();
	for (int y = 0; y < r64::SCREEN_HEIGHT; y++)
	{
		const uint16_t* aNow = aScreen + y * r64::SCREEN_WIDTH;
		const uint16_t* aBefore = mCopiedScreen.data() + y * r64::SCREEN_WIDTH;
		if (SamePixels(aNow, aBefore, r64::SCREEN_WIDTH))
			continue;
		for (int aColumn = 0; aColumn < CHANGE_COLUMNS; aColumn++)
		{
			int aLeft = aColumn * CHANGE_BLOCK;
			if (!SamePixels(aNow + aLeft, aBefore + aLeft, CHANGE_BLOCK))
				mChangedBlocks[y / CHANGE_BLOCK][aColumn] = 1;
		}
	}
}

// The rectangles to copy to the image: each row of changed blocks gives its runs (runs a gap of at most CHANGE_GAP
// blocks apart are one), and a run of the same extent under another joins it, so that a frame is a few dozen uploads
void WaterBugsWidget::CollectChangedRects()
{
	static std::vector<int> anOpen;      // the rectangles that end at the row above
	static std::vector<int> aNowOpen;    // the rectangles that end at this row
	mChangedRects.clear();
	anOpen.clear();
	for (int aRow = 0; aRow < CHANGE_ROWS; aRow++)
	{
		aNowOpen.clear();
		int aTop = aRow * CHANGE_BLOCK;
		int aBottom = aTop + CHANGE_BLOCK < r64::SCREEN_HEIGHT ? aTop + CHANGE_BLOCK : r64::SCREEN_HEIGHT;
		int aColumn = 0;
		while (aColumn < CHANGE_COLUMNS)
		{
			if (mChangedBlocks[aRow][aColumn] == 0)
			{
				aColumn++;
				continue;
			}
			int aRunFirst = aColumn;
			int aRunLast = aColumn;
			for (int aNext = aColumn + 1; aNext < CHANGE_COLUMNS && aNext <= aRunLast + 1 + CHANGE_GAP; aNext++)
			{
				if (mChangedBlocks[aRow][aNext] != 0)
					aRunLast = aNext;
			}
			aColumn = aRunLast + 1;
			int aLeft = aRunFirst * CHANGE_BLOCK;
			int aRight = (aRunLast + 1) * CHANGE_BLOCK;
			int aRect = -1;
			for (int i : anOpen)
			{
				const Rect& aAbove = mChangedRects[i];
				if (aAbove.mX == aLeft && aAbove.mX + aAbove.mWidth == aRight)
				{
					aRect = i;
					mChangedRects[i].mHeight = aBottom - aAbove.mY;
					break;
				}
			}
			if (aRect < 0)
			{
				aRect = (int)mChangedRects.size();
				mChangedRects.push_back(Rect(aLeft, aTop, aRight - aLeft, aBottom - aTop));
			}
			aNowOpen.push_back(aRect);
		}
		anOpen.swap(aNowOpen);
	}
}

// What the original presented since the last copy goes to the image: just the rectangles that changed are widened (and
// stay in mChangedRects until the next Draw), and the framework uploads the image if there were any
void WaterBugsWidget::Draw(Graphics* g)
{
	WaterBugsApp* anApp = gApp;
	if (anApp->mPresented)
	{
		ulong* aBits = mScreenImage->GetBits();
		if (mCopiedScreen.empty())
		{
			// the first picture is the whole screen
			for (int i = 0; i < r64::SCREEN_WIDTH * r64::SCREEN_HEIGHT; i++)
				aBits[i] = gPixelsFromScreen16[anApp->mScreen[i]];
			mCopiedScreen = anApp->mScreen;
			mScreenImage->BitsChanged();
		}
		else
		{
			FindChangedBlocks();
			CollectChangedRects();
			for (const Rect& aRect : mChangedRects)
			{
				for (int y = aRect.mY; y < aRect.mY + aRect.mHeight; y++)
				{
					const uint16_t* aSource = &anApp->mScreen[y * r64::SCREEN_WIDTH];
					ulong* aDestination = &aBits[y * r64::SCREEN_WIDTH];
					for (int x = aRect.mX; x < aRect.mX + aRect.mWidth; x++)
						aDestination[x] = gPixelsFromScreen16[aSource[x]];
					std::memcpy(&mCopiedScreen[y * r64::SCREEN_WIDTH + aRect.mX], aSource + aRect.mX, aRect.mWidth * 2);
				}
			}
			if (!mChangedRects.empty())
				mScreenImage->BitsChanged();
		}
		anApp->mPresented = false;
	}
	g->DrawImage(mScreenImage, 0, 0);
}

// The framework's key codes are Windows' virtual keys, which the original's window procedure got
void WaterBugsWidget::KeyDown(KeyCode theKey)
{
	r64::Sys_KeyEvent((int)theKey, 1);
}

void WaterBugsWidget::KeyUp(KeyCode theKey)
{
	r64::Sys_KeyEvent((int)theKey, 0);
}

void WaterBugsWidget::MouseMove(int x, int y)
{
	gApp->mMouseX = x;
	gApp->mMouseY = y;
}

void WaterBugsWidget::MouseDrag(int x, int y)
{
	gApp->mMouseX = x;
	gApp->mMouseY = y;
}

// The framework's buttons: 0 left, 1 right, 2 middle; the engine's 1, 2 and 3
void WaterBugsWidget::MouseDown(int x, int y, int theBtnNum, int)
{
	gApp->mMouseX = x;
	gApp->mMouseY = y;
	r64::Sys_MouseButton(theBtnNum + 1, 1);
}

void WaterBugsWidget::MouseUp(int x, int y, int theBtnNum, int)
{
	gApp->mMouseX = x;
	gApp->mMouseY = y;
	r64::Sys_MouseButton(theBtnNum + 1, 0);
}

WaterBugsApp::WaterBugsApp()
{
	gApp = this;
	mProdName = "WaterBugs";
	mTitle = "Water Bugs";
	mRegKey = "PopCap\\WaterBugs";
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

WaterBugsApp::~WaterBugsApp()
{
	if (mWidget != nullptr)
	{
		mWidgetManager->RemoveWidget(mWidget);
		delete mWidget;
	}
}

// 0x42b340 (WinMain: cp.cfg, the window and the screen, the sound, the game; what follows GameMain is Shutdown)
void WaterBugsApp::Init()
{
	SexyAppBase::Init();
	if (mShutdown)
		return;
	gSteadyClock = !mFastMode && Platform::FixOn("steadyClock");	// a -fast run is steady already
	r64::PixelFormat_Init(&gScreenFormat565, 16, 0xf800, 0x7e0, 0x1f);
	r64::Sys_SetScreenFormat(&gScreenFormat565);
	r64::Sys_SetHost(&gHost);
	if (mSoundManager != nullptr)
		mSoundManager->SetVolume(OUTPUT_VOLUME);
	// the game draws its own pointer
	SetCursor(CURSOR_NONE);

	mWidget = new WaterBugsWidget();
	mWidget->Resize(0, 0, mWidth, mHeight);
	mWidgetManager->AddWidget(mWidget);
	mWidgetManager->SetFocus(mWidget);

	if (!wb::LoadGameTexts())
	{
		mGameEnded = true;
		Shutdown();
		return;
	}
	// WinMain (0x42b340): cp.cfg, the screen in its mode, the sound, then the game
	r64::LoadConfig();
#ifdef __EMSCRIPTEN__
	// the site's "Full screen at start: Windowed" (framework/WebSettings.h): the game's own option, set as its menu
	// sets it
	if (Byopop::SiteWindowed())
		r64::Screen_SetFullscreen(0);
#endif
	SetGameScreenMode(r64::Screen_Fullscreen() != 0);
	r64::Sound_Start();
	wb::GameStart();
	mGameStarted = true;
}

void WaterBugsApp::Shutdown()
{
	if (mGameStarted && !mGameEnded)
	{
		mGameEnded = true;
		wb::GameEnd();
		r64::Sound_Shutdown();
	}
	SexyAppBase::Shutdown();
}

void WaterBugsApp::RunFrame()
{
	if (!mGameStarted || mGameEnded)
		return;
	if (wb::GameFrame() == 0)
	{
		mGameEnded = true;
		wb::GameEnd();
		r64::Sound_Shutdown();
		Shutdown();
	}
}

void WaterBugsApp::Present()
{
	mPresented = true;
}

void WaterBugsApp::SetGameScreenMode(bool theFullscreen)
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

// The original moves the system's pointer (0x428d70), and the pointer's move reaches the window as a mouse move. Where
// the port does not move it (in a test run, on the web, with the window inactive) the warp goes to the framework as
// that move: its widget manager passes on only moves from the position it knew, and a driver's pointer (-realinput)
// moves with it, so that a move back to the point it moved to before the warp is a move again, as it is for a hand
void WaterBugsApp::WarpMouse(int x, int y)
{
	mMouseX = x;
	mMouseY = y;
#ifndef __EMSCRIPTEN__
	if (mRealMouseX >= 0 && mDDInterface != nullptr)
		mDDInterface->LogicalToWindow(x, y, &mRealMouseX, &mRealMouseY);
	if (IsTestRun() || !mActive || mDDInterface == nullptr || mHWnd == nullptr)
	{
		FollowWarp(x, y);
		return;
	}
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
#else
	FollowWarp(x, y);
#endif
}

void WaterBugsApp::FollowWarp(int x, int y)
{
	mLastMouseX = x;
	mLastMouseY = y;
	if (mWidgetManager != nullptr)
		mWidgetManager->MouseMove(x, y);
}

// The window's close button: WM_DESTROY's (0x42ac80) end of the game
void WaterBugsApp::CloseRequestAsync()
{
	if (mGameStarted && !mGameEnded)
		wb::gQuit = 1;
	else
		SexyAppBase::CloseRequestAsync();
}

// Alt+Enter goes to the game like its F7 (0x42ac80); it then asks for the mode through SetScreenMode. (The web
// page's Fullscreen button works on the page and never reaches the game.)
void WaterBugsApp::SwitchScreenMode(bool theWantWindowed, bool theIs3d, bool theForce)
{
	if (mApplyingGameMode || !mGameStarted || mGameEnded || theForce)
	{
		SexyAppBase::SwitchScreenMode(theWantWindowed, theIs3d, theForce);
		return;
	}
	if (theWantWindowed != mIsWindowed)
		r64::Screen_SetFullscreen(theWantWindowed ? 0 : 1);
}

bool WaterBugsApp::HandleScriptCommand(const ScriptCommand& theCommand)
{
	if (theCommand.mCommand == "state")
	{
		// (the check suite reads this line)
		wb::Level* aLevel = wb::gLevels.mCurrent;
		SDL_Log("state: score %d, lives %d, board %d, filled %d of %d%%", wb::gScore, wb::gLives, wb::gLevelInSet,
			aLevel != nullptr ? aLevel->mFilledCells : -1, aLevel != nullptr ? aLevel->mFillQuota : -1);
	}
	if (wb::HandleTestCommand(this, theCommand))
		return true;
	return SexyAppBase::HandleScriptCommand(theCommand);
}

void WaterBugsApp::AddMemReportHolders(Platform::MemReport* theReport)
{
	wb::AddMemReportHolders(theReport);
}
