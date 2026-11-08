// The port's host for Word Harmony (see WordHarmonyApp.h).

#include "WordHarmonyApp.h"
#include "Render.h"
#include "ModFeatures.h"
#include "SoundHost.h"
#include "engine/App.h"
#include "engine/Exception.h"
#include "engine/Host.h"
#include "game/Game.h"
#include "game/GameText.h"
#include "game/WordBallSystem.h"
#include "engine/Random.h"
#include "MemoryImage.h"
#include "WidgetManager.h"
#include "platform/VFS.h"

#include <SDL.h>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include "WebSettings.h"
#endif
#include <algorithm>
#include <cstring>

using namespace Sexy;

WordHarmonyApp* Sexy::gApp = nullptr;

namespace
{

bool gTestSeedGiven = false;
unsigned int gTestSeed = 0;

const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;

// The framework's side of engine::Host
class WordHarmonyHost : public engine::Host
{
public:
	// port: with -fast, or the fixes mod's steadyClock, the engine has a clock of its own, which moves one frame period
	// a frame (WordHarmonyWidget::Update)
	uint32_t timeMs() override { return gApp->mFastMode || gApp->mSteadyClock ? gApp->mEngineClock : gApp->GetClockTime(); }

	bool fixOn(const char* theFix) override { return Sexy::FixOn(theFix); }

	// port: -seed=N, for the game's srand of the tick count
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

	engine::Image* createImage(const std::vector<uint8_t>& theData, const std::vector<uint8_t>* theMask) override
	{
		return wordharmony::decodeImage(theData, theMask);
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
#ifdef __EMSCRIPTEN__
		// the site's "Full screen at start: Windowed" (framework/WebSettings.h): the game's own option reads as off, as if the
		// player had left it unticked in the game's options, so the game never asks the page for full screen
		if ((theName == "FullScreen") && Byopop::SiteWindowed())
		{
			theValue = 0;
			return true;
		}
#endif
		return gApp->RegistryReadInteger(theName, &theValue);
	}

	bool registryWriteInt(const std::string& theName, int theValue) override
	{
		return gApp->RegistryWriteInteger(theName, theValue);
	}

	// On the web the page's stage goes full screen, as the page's Fullscreen button does: SDL's window cannot (the
	// canvas was left with no size). A browser grants it only soon after a click, so the settings' full screen at the
	// start leaves the game in the page.
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

	void openURL(const std::string& theURL) override { gApp->OpenURL(theURL); }

	void log(const char* theText) override
	{
		std::string aText = theText;
		while (!aText.empty() && (aText.back() == '\n' || aText.back() == '\r'))
			aText.pop_back();
		SDL_Log("%s", aText.c_str());
	}

	void showMessage(const char* theCaption, const char* theText) override { gApp->MsgBox(theText, theCaption); }

	engine::SoundSystem* createSoundSystem() override { return new wordharmony::HostSoundSystem(); }

	std::string commandLine() override { return gApp->mCommandLine; }
};

WordHarmonyHost gHost;

}

// The widget: the window procedure (0x455ac0) and the screen

WordHarmonyWidget::WordHarmonyWidget()
{
	mScreenImage = new MemoryImage(gApp);
	mScreenImage->Create(SCREEN_WIDTH, SCREEN_HEIGHT);
	mGraphics = new wordharmony::ScreenGraphics((uint32_t*)mScreenImage->GetBits(), SCREEN_WIDTH, SCREEN_HEIGHT);
	mLastLoop = gApp->GetClockTime();
	mAccumulated = 0;
	mLastSoundTick = gApp->mSteadyClock ? (DWORD)(gApp->mUpdateCount * gApp->mFrameTime) : mLastLoop;
}

WordHarmonyWidget::~WordHarmonyWidget()
{
	delete mGraphics;
	delete mScreenImage;
}

// The run loop (0x4552b0): the milliseconds since the last pass are added up, and once they reach the frame period
// (1000 / the frame rate: 33) a frame runs and the period is taken off; the loop then waits for the rest of the period.
// The window's 100 ms timer (WM_TIMER 1) updates the sound system with 100.
void WordHarmonyWidget::Update()
{
	Widget::Update();
	if (!gApp->mEngineRunning)
		return;
	DWORD aNow = gApp->GetClockTime();
	// fixes mod, "steadyClock": the sound system's 100 ms tick (its fades) on the updates as well
	DWORD aSoundNow = gApp->mSteadyClock ? (DWORD)(gApp->mUpdateCount * gApp->mFrameTime) : aNow;
	if ((int)(aSoundNow - mLastSoundTick) >= 100)
	{
		mLastSoundTick = aSoundNow;
		if (engine::App::getSoundSystem() != nullptr)
			engine::App::getSoundSystem()->update(100);
	}
	int aPeriod = 1000 / engine::App::instance()->getConfig().mFrameRate;
	if (gApp->mFastMode)
	{
		// port: -fast runs a frame each update, as the original does on origrun's virtual clock, which skips the wait for the
		// rest of the period and moves the clock on by it: each frame is one period after the last
		gApp->mEngineClock += aPeriod;
	}
	else
	{
		// fixes mod, "steadyClock": the time added up is the framework's updates (mFrameTime each, which it runs at that
		// rate and catches up), not the machine's milliseconds, so the frames fall on the same updates as the input
		mAccumulated += gApp->mSteadyClock ? gApp->mFrameTime : (int)(aNow - mLastLoop);
		mLastLoop = aNow;
		if (mAccumulated < aPeriod)
			return;
		mAccumulated -= aPeriod;
		// fixes mod, "steadyClock": the original times each frame by the machine's clock (the milliseconds since the
		// last, at most 330), so a frame that comes late moves the balls, the water and the timers further and play
		// differs from run to run; with the fix each frame is one period on the engine's clock, as with -fast
		if (gApp->mSteadyClock)
			gApp->mEngineClock += aPeriod;
	}
	gApp->RunFrame();
	MarkDirty();
}

void WordHarmonyWidget::Draw(Graphics* g)
{
	g->DrawImage(mScreenImage, 0, 0);
}

// WM_KEYDOWN and WM_KEYUP: the virtual key
void WordHarmonyWidget::KeyDown(KeyCode theKey)
{
	if (gApp->mEngineRunning)
		engine::App::instance()->getRootWindow()->keyDown((int)theKey);
}

void WordHarmonyWidget::KeyUp(KeyCode theKey)
{
	if (gApp->mEngineRunning)
		engine::App::instance()->getRootWindow()->keyUp((int)theKey);
}

void WordHarmonyWidget::MouseMove(int x, int y)
{
	if (gApp->mEngineRunning)
		engine::App::instance()->getRootWindow()->handleMouseMove(engine::Point((float)x, (float)y));
}

void WordHarmonyWidget::MouseDrag(int x, int y)
{
	MouseMove(x, y);
}

// The framework's buttons: 0 left, 1 right (the original took no middle button)
void WordHarmonyWidget::MouseDown(int x, int y, int theBtnNum, int theClickCount)
{
	if (!gApp->mEngineRunning)
		return;
	engine::Window* aRoot = engine::App::instance()->getRootWindow();
	if (theBtnNum == 0)
		aRoot->handleMouseDown(engine::Point((float)x, (float)y));
	else if (theBtnNum == 1)
		aRoot->handleRightDown(engine::Point((float)x, (float)y));
}

void WordHarmonyWidget::MouseUp(int x, int y, int theBtnNum, int theClickCount)
{
	if (!gApp->mEngineRunning)
		return;
	engine::Window* aRoot = engine::App::instance()->getRootWindow();
	if (theBtnNum == 0)
		aRoot->handleMouseUp(engine::Point((float)x, (float)y));
	else if (theBtnNum == 1)
		aRoot->handleRightUp(engine::Point((float)x, (float)y));
}

// The app

WordHarmonyApp::WordHarmonyApp()
{
	gApp = this;
	mProdName = "WordHarmony";
	mTitle = "Word Harmony";
	// the original's key (0x421750: HKEY_CURRENT_USER\Software\GameHouse\WordHarmony)
	mRegKey = "GameHouse\\WordHarmony";
	mWidth = SCREEN_WIDTH;
	mHeight = SCREEN_HEIGHT;
	mWidget = nullptr;
	mEngineRunning = false;
	mQuitRequested = false;
	mFrames = 0;
	mEngineClock = 0;
	mSteadyClock = false;
	mStateLog = nullptr;
}

WordHarmonyApp::~WordHarmonyApp()
{
	if (mWidget != nullptr)
	{
		mWidgetManager->RemoveWidget(mWidget);
		delete mWidget;
	}
}

void WordHarmonyApp::SetGameCommandLine(int argc, char** argv)
{
	mCommandLine = "WordHarmony.exe";
	for (int i = 1; i < argc; i++)
	{
		const char* a = argv[i];
		if ((a[0] == '/' || a[0] == '-') && a[1] != '\0' && std::strchr("sScCpPlL", a[1]) != nullptr &&
			(a[2] == '\0' || a[2] == ':'))
		{
			mCommandLine += " ";
			mCommandLine += a;
		}
	}
}

// WinMain (0x4552b0) up to its loop: the controller made and started
void WordHarmonyApp::Init()
{
	SexyAppBase::Init();
	if (mShutdown)
		return;
	engine::setHost(&gHost);
	// fixes mod, "steadyClock" (read at the start: its toggle applies on a restart): the sounds end by the engine's clock
	// too, so the music's draws from the game's random numbers when a clip ends fall on the same frames on every run
	mSteadyClock = !mFastMode && FixOn("steadyClock");
	wordharmony::HostSound::sGameClock = mFastMode || mSteadyClock;
	mWidget = new WordHarmonyWidget();
	mWidget->Resize(0, 0, mWidth, mHeight);
	mWidgetManager->AddWidget(mWidget);
	mWidgetManager->SetFocus(mWidget);
	loadGameTexts();
	try
	{
		new engine::App();
		engine::App::instance()->start(new Game());
		mEngineRunning = true;
	}
	catch (const std::exception& e)
	{
		EngineFailed(e.what());
	}
}

void WordHarmonyApp::RunFrame()
{
	try
	{
		uint32_t* aBits = (uint32_t*)mWidget->mScreenImage->GetBits();
		std::fill(aBits, aBits + SCREEN_WIDTH * SCREEN_HEIGHT, 0xff000000);
		// what the input since the last frame changed is logged under that frame, as origrun logs the original's
		if (mStateLog != nullptr)
			LogState();
		if (mFrames < mBotUntil)
			BotTick();
		mFrames++;
		wordharmony::HostSound::sLogFrame = mFrames;
		engine::App::instance()->frame(mWidget->mGraphics);
		mWidget->mScreenImage->BitsChanged();
		if (mStateLog != nullptr)
			LogState();
	}
	catch (const std::exception& e)
	{
		EngineFailed(e.what());
		return;
	}
	if (mQuitRequested)
		Shutdown();
}

// port: the test hooks of the identical-play runs (games/wordharmony/docs/IDENTICAL_PLAY.md)
void WordHarmonyApp::HandleCmdLineParam(const std::string& theParamName, const std::string& theParamValue)
{
	if (theParamName == "-soundlog")
	{
		wordharmony::HostSound::sLog = fopen(theParamValue.c_str(), "w");
		return;
	}
	if (theParamName == "-statelog")
	{
		mStateLog = fopen(theParamValue.c_str(), "w");
		return;
	}
	if (theParamName == "-seed")
	{
		gTestSeedGiven = true;
		gTestSeed = strtoul(theParamValue.c_str(), nullptr, 10);
	}
	SexyAppBase::HandleCmdLineParam(theParamName, theParamValue);
}

namespace
{

std::string intText(int theValue)
{
	return std::to_string(theValue);
}

// a float as its bits, as origrun's watch reads it
std::string floatText(float theValue)
{
	int aBits;
	memcpy(&aBits, &theValue, 4);
	return std::to_string(aBits);
}

}

// The values compared with the original's (the names of games/wordharmony/tools/identical/case.sh), "-" while they do not
// exist. A line for each value that changed in the frame.
void WordHarmonyApp::LogState()
{
	std::vector<std::pair<std::string, std::string>> aState;
	Game* aGame = Game::sInstance;
	aState.emplace_back("rand", intText((int)engine::crtRandState()));
	aState.emplace_back("state", intText(aGame->mState));
	aState.emplace_back("nextstate", intText(aGame->mNextState));
	aState.emplace_back("timer", floatText(aGame->mTimer));
	aState.emplace_back("score", intText(aGame->mScore));
	aState.emplace_back("level", intText(aGame->mLevel));
	aState.emplace_back("mode", intText(aGame->mMode));
	aState.emplace_back("template", intText(aGame->mTemplate));
	aState.emplace_back("time", intText(aGame->mTime));
	aState.emplace_back("levelpoints", intText(aGame->mLevelPoints));
	aState.emplace_back("lettersleft", intText(aGame->mLettersLeft));
	aState.emplace_back("lettersused", intText(aGame->mLettersUsed));
	aState.emplace_back("bonus", intText(aGame->mBonus));
	aState.emplace_back("wildcards", intText((int)aGame->mWildcards));
	aState.emplace_back("hints", intText(aGame->mHints));
	aState.emplace_back("hintsused", intText(aGame->mHintsUsed));
	aState.emplace_back("levelhints", intText(aGame->mLevelHintsUsed));
	aState.emplace_back("leveltime", floatText(aGame->mLevelTime));
	WordBallSystem* aBalls = aGame->mBallSystem;
	auto aBallValue = [aBalls](const std::string& theText) { return aBalls != nullptr ? theText : std::string("-"); };
	aState.emplace_back("ballcount", aBallValue(aBalls ? intText((int)aBalls->mCount) : ""));
	aState.emplace_back("ballsleft", aBallValue(aBalls ? intText(aBalls->mBallsLeft) : ""));
	aState.emplace_back("steptime", aBallValue(aBalls ? floatText(aBalls->mStepTime) : ""));
	aState.emplace_back("spawntime", aBallValue(aBalls ? floatText(aBalls->mSpawnTime) : ""));
	aState.emplace_back("nextslot", aBallValue(aBalls ? intText(aBalls->mNextSlot) : ""));
	aState.emplace_back("tocome", aBallValue(aBalls ? intText(aBalls->mBallsToCome) : ""));
	aState.emplace_back("water", aBallValue(aBalls ? floatText(aBalls->mWaterLevel) : ""));
	aState.emplace_back("fill", aBallValue(aBalls ? floatText(aBalls->mFill) : ""));
	aState.emplace_back("hintindex", aBallValue(aBalls ? intText((int)aBalls->mHintIndex) : ""));
	aState.emplace_back("ball0x", aBallValue(aBalls ? floatText(aBalls->getBall(0)->getPosition()->x) : ""));
	aState.emplace_back("ball0y", aBallValue(aBalls ? floatText(aBalls->getBall(0)->getPosition()->y) : ""));
	aState.emplace_back("ball0state", aBallValue(aBalls ? intText(aBalls->getBall(0)->mState) : ""));
	aState.emplace_back("ball0letter", aBallValue(aBalls ? intText(aBalls->getBall(0)->mLetter) : ""));
	aState.emplace_back("ball3x", aBallValue(aBalls ? floatText(aBalls->getBall(3)->getPosition()->x) : ""));
	aState.emplace_back("ball3y", aBallValue(aBalls ? floatText(aBalls->getBall(3)->getPosition()->y) : ""));
	if (mLoggedState.size() != aState.size())
		mLoggedState.assign(aState.size(), std::make_pair(std::string(), std::string()));
	for (size_t i = 0; i < aState.size(); i++)
	{
		if (mLoggedState[i].second != aState[i].second)
			fprintf(mStateLog, "%d %s %s\n", mFrames, aState[i].first.c_str(), aState[i].second.c_str());
	}
	mLoggedState = aState;
	fflush(mStateLog);
}

void WordHarmonyApp::EngineFailed(const char* theWhat)
{
	SDL_Log("Word Harmony: %s", theWhat);
	mEngineRunning = false;
	MsgBox(theWhat, "Word Harmony");
	Shutdown();
}

// The run loop's end: the controller's shutdown (the settings saved)
void WordHarmonyApp::Shutdown()
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
			SDL_Log("Word Harmony: %s", e.what());
		}
	}
	SexyAppBase::Shutdown();
}

// WM_ACTIVATEAPP: the controller's slot 0x14 (a lost focus pauses a game)
void WordHarmonyApp::GotFocus()
{
	SexyAppBase::GotFocus();
	if (mEngineRunning)
		engine::App::instance()->getController()->activate(true);
}

void WordHarmonyApp::LostFocus()
{
	SexyAppBase::LostFocus();
	if (mEngineRunning)
		engine::App::instance()->getController()->activate(false);
}
