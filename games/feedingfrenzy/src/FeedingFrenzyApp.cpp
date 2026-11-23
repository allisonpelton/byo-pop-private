// port: the host for Feeding Frenzy (see FeedingFrenzyApp.h).

#include "FeedingFrenzyApp.h"
#include "platform/GameOptions.h"
#include "sprout/App.h"
#include "sprout/Exception.h"
#include "sprout/Host.h"
#include "sprout/ImageFilter.h"
#include "sprout/host/Render.h"
#include "sprout/host/SoundHost.h"
#include "game/FishFood.h"
#include "MemoryImage.h"
#include "WidgetManager.h"
#include "platform/UserFiles.h"
#include "platform/VFS.h"
#ifndef __EMSCRIPTEN__
#include "SurfaceDisplay.h"
#endif

#include <SDL.h>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif
#include <algorithm>
#include <cstring>

using namespace Sexy;

FeedingFrenzyApp* Sexy::gApp = nullptr;

namespace
{

// port: the text shown for an exception that is not the engine's own
const char* const OTHER_EXCEPTION_TEXT = "Exception";

// The host's ticks: microseconds (the original's QueryPerformanceCounter, whose rate the machine sets)
const uint64_t TICKS_PER_SECOND = 1000000;

// port: -seed=N (FeedingFrenzyApp::HandleCmdLineParam), which takes the place of the time the game seeds rand() with
bool gTestSeedGiven = false;
unsigned int gTestSeed = 0;

// The framework's side of engine::Host
class FeedingFrenzyHost : public engine::Host
{
public:
	// port: with -fast the engine has a clock of its own, which moves one frame period a frame
	uint32_t timeMs() override
	{
		return gApp->mFastMode ? (uint32_t)(gApp->mEngineClock / 1000) : gApp->GetClockTime();
	}
	uint64_t timeTicks() override
	{
		if (gApp->mFastMode)
			return gApp->mEngineClock;
		return (uint64_t)SDL_GetPerformanceCounter() * TICKS_PER_SECOND / SDL_GetPerformanceFrequency();
	}
	uint64_t ticksPerSecond() override { return TICKS_PER_SECOND; }

	bool fixOn(const char* theFix) override { return Platform::FixOn(theFix); }

	// port: -seed=N, for the game's srand of the time
	bool testSeed(unsigned int& theSeed) override
	{
		if (!gTestSeedGiven)
			return false;
		theSeed = gTestSeed;
		return true;
	}

	bool readGameFile(const std::string& theName, std::vector<uint8_t>& theData) override
	{
		return Platform::VFS::Get().ReadAll(theName, theData);
	}

	bool gameFileExists(const std::string& theName) override { return Platform::VFS::Get().Exists(theName); }

	// port: the settings and the scores are in the user folder (the original's are beside the exe)
	bool readUserFile(const std::string& theName, std::string& theData) override
	{
		return Platform::ReadHostFile(Platform::VFS::Get().GetUserDir() + "/" + theName, &theData);
	}

	bool writeUserFile(const std::string& theName, const std::string& theData) override
	{
		return Platform::WriteUserFileAtomic(theName, theData);
	}

	engine::Image* createImage(const std::vector<uint8_t>& theData, const std::vector<uint8_t>* theMask) override
	{
		return engine::decodeImage(theData, theMask);
	}

	bool registryReadString(const std::string& theName, std::string& theValue) override
	{
		return gApp->RegistryReadString(theName, &theValue);
	}

	bool registryWriteString(const std::string& theName, const std::string& theValue) override
	{
		return gApp->RegistryWriteString(theName, theValue);
	}

	bool registryReadInt(const std::string& theName, int& theValue) override
	{
		return gApp->RegistryReadInteger(theName, &theValue);
	}

	bool registryWriteInt(const std::string& theName, int theValue) override
	{
		return gApp->RegistryWriteInteger(theName, theValue);
	}

	// The keys below a key of the registry (the framework's registry keeps the values under their keys' names)
	bool registryGetSubKeys(const std::string& theKey, std::vector<std::string>& theNames) override
	{
		return gApp->RegistryGetSubKeys(theKey, &theNames);
	}

	// A key of the registry with its values deleted
	bool registryDeleteKey(const std::string& theKey) override
	{
		return gApp->RegistryEraseKey(theKey);
	}

	// The clipboard's text
	std::string clipboardText() override
	{
		return gApp->GetClipboard();
	}

	// The Ctrl key
	bool controlDown() override
	{
		return gApp->mCtrlDown;
	}

	// The values in a key: the registry's names that start with the key's and have no key of their own after it
	bool registryGetValueNames(const std::string& theKey, std::vector<std::string>& theNames) override
	{
		theNames.clear();
		std::vector<std::string> aSubKeys;
		bool aFound = gApp->RegistryGetSubKeys(theKey, &aSubKeys);  // reads the registry in
		std::string aPrefix = RemoveTrailingSlash("SOFTWARE\\" + gApp->mRegKey) + "\\";
		if (!theKey.empty())
			aPrefix += theKey + "\\";
		for (RegistryValueMap::const_iterator anItr = gApp->mRegistry.begin(); anItr != gApp->mRegistry.end(); ++anItr)
		{
			if (sexy_strnicmp(anItr->first.c_str(), aPrefix.c_str(), aPrefix.length()) != 0)
				continue;
			std::string aName = anItr->first.substr(aPrefix.length());
			if (aName.find('\\') == std::string::npos)
				theNames.push_back(aName);
		}
		return aFound;
	}

	// 0x4bcb40 (feedingfrenzy): a blank image; 0x4bcbd0 (feedingfrenzy): graphics that draw into an image
	engine::Image* createBlankImage(int theWidth, int theHeight) override
	{
		return new engine::ScreenImage(theWidth, theHeight);
	}

	engine::Graphics* createImageGraphics(engine::Image* theImage) override
	{
		engine::ScreenImage* anImage = dynamic_cast<engine::ScreenImage*>(theImage);
		return anImage == nullptr ? nullptr : new engine::ImageGraphics(anImage);
	}

	// 0x4229f0 (feedingfrenzy): a new image, cleared to transparent and blended, with the part of the image drawn on it
	engine::Image* cropImage(engine::Image* theImage, int theLeft, int theTop, int theWidth, int theHeight) override
	{
		engine::ScreenImage* aCopy = new engine::ScreenImage(theWidth, theHeight);
		aCopy->fill(engine::Color4(0.0f, 0.0f, 0.0f, 0.0f));
		aCopy->setAlphaMode(engine::ALPHA_BLEND, 0xff);
		engine::ImageGraphics aGraphics(aCopy);
		aGraphics.drawImage(theImage, engine::IntRect(theLeft, theTop, theLeft + theWidth, theTop + theHeight), 0, 0);
		return aCopy;
	}

	// 0x423980 (feedingfrenzy): the image drawn whole on a new transparent, blended one, whose colours are then shifted
	// (engine::HsvShiftFilter); the image's pivot is put at the corner for the draw and back at the centre
	engine::Image* shiftImageColors(engine::Image* theImage, float theHue, float theSaturation, float theValue) override
	{
		theImage->setPivot(0);
		engine::ScreenImage* aCopy = new engine::ScreenImage(theImage->getWidth(), theImage->getHeight());
		aCopy->fill(engine::Color4(0.0f, 0.0f, 0.0f, 0.0f));
		aCopy->setAlphaMode(engine::ALPHA_BLEND, 0xff);
		engine::ImageGraphics aGraphics(aCopy);
		aGraphics.drawImage(theImage);
		engine::HsvShiftFilter aFilter;
		aFilter.setShift(theHue, theSaturation, theValue);
		aCopy->applyFilter(aFilter);
		theImage->setPivot(1);
		return aCopy;
	}

	// On the web the page's stage goes full screen, as the page's Fullscreen button does
	void setFullScreen(bool theFullScreen) override
	{
#ifdef __EMSCRIPTEN__
		EM_ASM({
			var aStage = document.getElementById("stage");
			if ($0 && !document.fullscreenElement && aStage && aStage.requestFullscreen)
				aStage.requestFullscreen().catch(function () {});
			else if (!$0 && document.fullscreenElement && document.exitFullscreen)
				document.exitFullscreen().catch(function () {});
		}, theFullScreen);
#else
		if (theFullScreen == gApp->mIsWindowed)
			gApp->SwitchScreenMode(!theFullScreen);
#endif
	}

	bool isFullScreen() override
	{
#ifdef __EMSCRIPTEN__
		return EM_ASM_INT({ return document.fullscreenElement ? 1 : 0; }) != 0;
#else
		return !gApp->mIsWindowed;
#endif
	}

	// PostQuitMessage(): the program ends after the frame
	void quit() override { gApp->mQuitRequested = true; }

	// port: never called; the links are removed (CLAUDE.md, "Registration and outbound buttons are removed")
	void openURL(const std::string& theURL) override {}

	void log(const char* theText) override
	{
		std::string aText = theText;
		while (!aText.empty() && (aText.back() == '\n' || aText.back() == '\r'))
			aText.pop_back();
		SDL_Log("%s", aText.c_str());
	}

	void showMessage(const char* theCaption, const char* theText) override { gApp->MsgBox(theText, theCaption); }

	// port: the original keeps no string table (its texts are in the archive and the code)
	std::string loadString(int theId) override { return std::string(); }

	// 0x48ef60 (feedingfrenzy): the sound system of the window (DirectSound)
	engine::SoundSystem* createSoundSystem() override { return new engine::HostSoundSystem(); }

	// 0x42bb50 (feedingfrenzy) and 0x42c060 (feedingfrenzy): the pointer is hidden while a stage is played; the window
	// keeps it in itself with the player fish's putting it back after every reading
	void setCursorLocked(bool theLocked) override
	{
		gApp->SetCursor(theLocked ? CURSOR_NONE : CURSOR_POINTER);
	}

	// 0x4108a0 (feedingfrenzy): the pointer moves to the point of the screen (not in a test run, where the framework
	// does not move the real pointer)
	void warpCursor(int theX, int theY) override { gApp->WarpMouse(theX, theY); }
};

FeedingFrenzyHost gHost;

// 0x4bd050 (feedingfrenzy): every mouse position the window procedure passes on is held inside the 640x480 screen
engine::Point clampToScreen(int x, int y)
{
	if (x < 0)
		x = 0;
	else if (x > feedingfrenzy::SCREEN_WIDTH - 1)
		x = feedingfrenzy::SCREEN_WIDTH - 1;
	if (y < 0)
		y = 0;
	else if (y > feedingfrenzy::SCREEN_HEIGHT - 1)
		y = feedingfrenzy::SCREEN_HEIGHT - 1;
	return engine::Point((float)x, (float)y);
}

}

// The widget: the window procedure (0x4bd3b0) and the screen

FeedingFrenzyWidget::FeedingFrenzyWidget()
{
	mScreenImage = nullptr;
	mSurfaceDisplay = nullptr;
#ifndef __EMSCRIPTEN__
	// port (for speed): the window's display keeps the screen itself (CreateDDInterface)
	mSurfaceDisplay = dynamic_cast<SurfaceDisplay*>(gApp->mDDInterface);
#endif
	if (mSurfaceDisplay != nullptr)
	{
		mSurfaceBits.assign(feedingfrenzy::SCREEN_WIDTH * feedingfrenzy::SCREEN_HEIGHT, 0);
		mPixels = mSurfaceBits.data();
#ifndef __EMSCRIPTEN__
		mSurfaceDisplay->SetFrame(mPixels, feedingfrenzy::SCREEN_WIDTH * 4, nullptr, -1);
#endif
	}
	else
	{
		mScreenImage = new MemoryImage(gApp);
		mScreenImage->Create(feedingfrenzy::SCREEN_WIDTH, feedingfrenzy::SCREEN_HEIGHT);
		mScreenImage->SetImageMode(false, false);
		mScreenImage->mUploadAsIs = true;
		mPixels = (const uint32_t*)mScreenImage->GetBits();
	}
	mGraphics = new engine::ScreenGraphics(const_cast<uint32_t*>(mPixels), feedingfrenzy::SCREEN_WIDTH, feedingfrenzy::SCREEN_HEIGHT);
	mNextFrame = 0;
	mLastTimer = gApp->GetClockTime();
	ClearKeys();
}

FeedingFrenzyWidget::~FeedingFrenzyWidget()
{
	delete mGraphics;
	delete mScreenImage;
}

// The run loop (0x4bdf20): a frame runs once the clock reaches the time the last one set (its start and the frame
// period, 0x4bd210), else the loop sleeps; the window's 100 ms timer (WM_TIMER 1) goes to the timer signal.
void FeedingFrenzyWidget::Update()
{
	Widget::Update();
	if (!gApp->mEngineRunning)
		return;
	uint32_t aNow = gApp->GetClockTime();
	if ((int)(aNow - mLastTimer) >= 100)
	{
		mLastTimer = aNow;
		engine::App::instance()->mTimerSignal.emit(100);
	}
	if (gApp->mFastMode)
	{
		// port: -fast runs a frame each update, as the original does on origrun's virtual clock, which skips the wait
		// and moves the clock on by it: each frame is one period after the last
		gApp->mEngineClock += gApp->FramePeriod();
	}
	else
	{
		uint64_t aTicks = gHost.timeTicks();
		if (aTicks < mNextFrame)
			return;
		mNextFrame = aTicks + gApp->FramePeriod();
	}
	gApp->RunFrame();
}

void FeedingFrenzyWidget::Draw(Graphics* g)
{
	if (mScreenImage != nullptr)
		g->DrawImage(mScreenImage, 0, 0);
}

void FeedingFrenzyWidget::ClearKeys()
{
	std::memset(mKeyIsDown, 0, sizeof(mKeyIsDown));
}

// WM_KEYDOWN (the window procedure 0x4bd3b0): the key-down signal for the first, the repeat signal (with a count of 1)
// for each after it while the key is held, never both
void FeedingFrenzyWidget::KeyDown(KeyCode theKey)
{
	int aKey = (int)theKey;
	bool aRepeat = false;
	if (aKey >= 0 && aKey < 256)
	{
		aRepeat = mKeyIsDown[aKey];
		mKeyIsDown[aKey] = true;
	}
	if (!gApp->mEngineRunning)
		return;
	engine::App* anApp = engine::App::instance();
	if (!aRepeat)
		anApp->mKeyDownSignal.emit(aKey);
	else
		anApp->mKeyRepeatSignal.emit(aKey, 1);
}

void FeedingFrenzyWidget::KeyUp(KeyCode theKey)
{
	int aKey = (int)theKey;
	if (aKey >= 0 && aKey < 256)
		mKeyIsDown[aKey] = false;
	if (gApp->mEngineRunning)
		engine::App::instance()->mKeyUpSignal.emit(aKey);
}

// WM_CHAR
void FeedingFrenzyWidget::KeyChar(SexyChar theChar)
{
	if (gApp->mEngineRunning)
		engine::App::instance()->mCharSignal.emit((int)(unsigned char)theChar);
}

void FeedingFrenzyWidget::MouseMove(int x, int y)
{
	if (!gApp->mEngineRunning)
		return;
	engine::App* anApp = engine::App::instance();
	engine::Point aPoint = clampToScreen(x, y);
	anApp->mMousePosition = aPoint;
	anApp->mMouseMoveSignal.emit(aPoint);
}

void FeedingFrenzyWidget::MouseDrag(int x, int y)
{
	MouseMove(x, y);
}

// The framework's buttons: 0 left, 1 right (the original took no middle button)
void FeedingFrenzyWidget::MouseDown(int x, int y, int theBtnNum, int theClickCount)
{
	if (!gApp->mEngineRunning)
		return;
	engine::App* anApp = engine::App::instance();
	engine::Point aPoint = clampToScreen(x, y);
	anApp->mMousePosition = aPoint;
	if (theBtnNum == 0)
		anApp->mMouseDownSignal.emit(aPoint);
	else if (theBtnNum == 1)
		anApp->mRightDownSignal.emit(aPoint);
}

void FeedingFrenzyWidget::MouseUp(int x, int y, int theBtnNum, int theClickCount)
{
	if (!gApp->mEngineRunning)
		return;
	engine::App* anApp = engine::App::instance();
	engine::Point aPoint = clampToScreen(x, y);
	anApp->mMousePosition = aPoint;
	if (theBtnNum == 0)
		anApp->mMouseUpSignal.emit(aPoint);
	else if (theBtnNum == 1)
		anApp->mRightUpSignal.emit(aPoint);
}

// The app

FeedingFrenzyApp::FeedingFrenzyApp()
{
	gApp = this;
	mProdName = "FeedingFrenzy";
	mTitle = "Feeding Frenzy";
	// the original's settings (the game's settings, under HKEY_CURRENT_USER\Software\GameHouse\Feeding Frenzy)
	mRegKey = "GameHouse\\FeedingFrenzy";
	mWidth = feedingfrenzy::SCREEN_WIDTH;
	mHeight = feedingfrenzy::SCREEN_HEIGHT;
	mWidget = nullptr;
	mEngineRunning = false;
	mQuitRequested = false;
	mFrames = 0;
	mEngineClock = 0;
}

FeedingFrenzyApp::~FeedingFrenzyApp()
{
#ifndef __EMSCRIPTEN__
	if (SurfaceDisplay* aDisplay = dynamic_cast<SurfaceDisplay*>(mDDInterface))
		aDisplay->SetFrame(nullptr, 0, nullptr, -1);
#endif
	if (mWidget != nullptr)
	{
		mWidgetManager->RemoveWidget(mWidget);
		delete mWidget;
	}
}

#ifndef __EMSCRIPTEN__
DDInterface* FeedingFrenzyApp::CreateDDInterface()
{
	const char* aMode = SDL_getenv("SURFACE_DISPLAY");
	if ((aMode != nullptr) && (strcmp(aMode, "off") == 0))
		return SexyAppBase::CreateDDInterface();
	return new SurfaceDisplay(this);
}
#endif

uint64_t FeedingFrenzyApp::FramePeriod() const
{
	int aRate = engine::App::instance()->getConfig().mFrameRate;
	return aRate > 0 ? TICKS_PER_SECOND / (uint64_t)aRate : TICKS_PER_SECOND / 30;
}

// WinMain (0x4bdf20) up to its loop: the controller made (0x40d0d0) and started
void FeedingFrenzyApp::Init()
{
	SexyAppBase::Init();
	if (mShutdown)
		return;
	engine::setHost(&gHost);
	mWidget = new FeedingFrenzyWidget();
	mWidget->Resize(0, 0, mWidth, mHeight);
	mWidgetManager->AddWidget(mWidget);
	mWidgetManager->SetFocus(mWidget);
	try
	{
		new engine::App();
		engine::App::instance()->startSignalled(new feedingfrenzy::FishFood());
		mEngineRunning = true;
		// port: the original's window gets WM_ACTIVATE as it opens, which sets the game's active flag (+0x188); the
		// framework has the focus from the start and sends no first activation, so the state it has is passed on
		engine::App::instance()->mActivateSignal.emit(mHasFocus);
	}
	catch (const engine::Exception& e)
	{
		EngineFailed(e.what());
	}
	catch (...)
	{
		EngineFailed(OTHER_EXCEPTION_TEXT);
	}
}

void FeedingFrenzyApp::RunFrame()
{
	try
	{
		mWidget->mGraphics->clear();
		// port: what the input since the last frame changed is logged under that frame, as origrun logs the original's
		if (mStateLog != nullptr)
			LogState();
		mFrames++;
		engine::App::instance()->frameSignalled(mWidget->mGraphics);
		if (mStateLog != nullptr)
			LogState();
		if (mWidget->mGraphics->finish())
		{
#ifndef __EMSCRIPTEN__
			if (mWidget->mSurfaceDisplay != nullptr)
			{
				static std::vector<Rect> sRects;
				sRects.clear();
				for (const engine::ScreenGraphics::Rectangle& aRectangle : mWidget->mGraphics->redrawn())
					sRects.push_back(Rect(aRectangle.mX, aRectangle.mY, aRectangle.mWidth, aRectangle.mHeight));
				mWidget->mSurfaceDisplay->SetFrame(mWidget->mPixels, feedingfrenzy::SCREEN_WIDTH * 4, sRects.data(),
					(int)sRects.size());
			}
			else
#endif
				mWidget->mScreenImage->BitsChanged();
			mWidget->MarkDirty();
		}
	}
	catch (const engine::Exception& e)
	{
		EngineFailed(e.what());
		return;
	}
	catch (...)
	{
		EngineFailed(OTHER_EXCEPTION_TEXT);
		return;
	}
	if (mQuitRequested)
		Shutdown();
}

void FeedingFrenzyApp::EngineFailed(const char* theWhat)
{
	SDL_Log("Feeding Frenzy: %s", theWhat);
	mEngineRunning = false;
	MsgBox(theWhat, "Exception", MB_ICONERROR);
	Shutdown();
}

// The run loop's end: the controller's shutdown (the settings saved)
// port: -seed=N, for the identical-play runs
void FeedingFrenzyApp::HandleCmdLineParam(const std::string& theParamName, const std::string& theParamValue)
{
	if (theParamName == "-seed")
	{
		gTestSeedGiven = true;
		gTestSeed = strtoul(theParamValue.c_str(), nullptr, 10);
	}
	else if (theParamName == "-statelog")
		mStateLog = fopen(theParamValue.c_str(), "w");
	SexyAppBase::HandleCmdLineParam(theParamName, theParamValue);
}

void FeedingFrenzyApp::Shutdown()
{
	if (mEngineRunning)
	{
		mEngineRunning = false;
		try
		{
			engine::App::instance()->shutdown();
		}
		catch (const std::exception& e)
		{
			SDL_Log("Feeding Frenzy: %s", e.what());
		}
	}
	SexyAppBase::Shutdown();
}

// WM_ACTIVATE: the activate signal
void FeedingFrenzyApp::GotFocus()
{
	SexyAppBase::GotFocus();
	if (mEngineRunning)
		engine::App::instance()->mActivateSignal.emit(true);
}

void FeedingFrenzyApp::LostFocus()
{
	SexyAppBase::LostFocus();
	if (mWidget != nullptr)
		mWidget->ClearKeys();
	if (mEngineRunning)
		engine::App::instance()->mActivateSignal.emit(false);
}
