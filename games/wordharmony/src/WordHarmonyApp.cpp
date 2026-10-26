// The port's host for Word Harmony (see WordHarmonyApp.h).

#include "WordHarmonyApp.h"
#include "Render.h"
#include "SoundHost.h"
#include "engine/App.h"
#include "engine/Exception.h"
#include "engine/Host.h"
#include "game/Game.h"
#include "game/GameText.h"
#include "MemoryImage.h"
#include "WidgetManager.h"
#include "platform/VFS.h"

#include <SDL.h>
#include <algorithm>
#include <cstring>

using namespace Sexy;

WordHarmonyApp* Sexy::gApp = nullptr;

namespace
{

const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;

// The framework's side of engine::Host
class WordHarmonyHost : public engine::Host
{
public:
	uint32_t timeMs() override { return gApp->GetClockTime(); }

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
		return gApp->RegistryReadInteger(theName, &theValue);
	}

	bool registryWriteInt(const std::string& theName, int theValue) override
	{
		return gApp->RegistryWriteInteger(theName, theValue);
	}

	void setFullScreen(bool theFullScreen) override
	{
		if (theFullScreen == gApp->mIsWindowed)
			gApp->SwitchScreenMode(!theFullScreen);
	}

	bool isFullScreen() override { return !gApp->mIsWindowed; }

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
	mLastFrame = gApp->GetClockTime();
	mLastSoundTick = mLastFrame;
}

WordHarmonyWidget::~WordHarmonyWidget()
{
	delete mGraphics;
	delete mScreenImage;
}

// The run loop (0x4552b0): a frame whenever the frame period has passed; the window's 100 ms timer (WM_TIMER 1) updates
// the sound system with 100
void WordHarmonyWidget::Update()
{
	Widget::Update();
	if (!gApp->mEngineRunning)
		return;
	DWORD aNow = gApp->GetClockTime();
	if ((int)(aNow - mLastSoundTick) >= 100)
	{
		mLastSoundTick = aNow;
		if (engine::App::getSoundSystem() != nullptr)
			engine::App::getSoundSystem()->update(100);
	}
	int aFrameRate = engine::App::instance()->getConfig().mFrameRate;
	if ((int)(aNow - mLastFrame) * aFrameRate < 1000)
		return;
	mLastFrame = aNow;
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
		engine::App::instance()->frame(mWidget->mGraphics);
		mWidget->mScreenImage->BitsChanged();
	}
	catch (const std::exception& e)
	{
		EngineFailed(e.what());
		return;
	}
	if (mQuitRequested)
		Shutdown();
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
