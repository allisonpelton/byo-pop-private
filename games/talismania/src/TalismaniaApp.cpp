// port: the host for Talismania (see TalismaniaApp.h).

#include "TalismaniaApp.h"
#include "DXDrawContext.h"
#include "EngineImpl.h"
#include "AtlantisApp.h"
#include "Graphics.h"
#include "MemoryImage.h"
#include "Surface.h"
#include "WidgetManager.h"

#include <cstring>

using namespace Sexy;

TalismaniaApp* Sexy::gApp = nullptr;

namespace
{

// The game's screen and frame rate (the application options' screendimensions and targetfps, config/gameoptions.xml)
const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;
const int FRAME_RATE = 90;

} // namespace

TalismaniaWidget::TalismaniaWidget() : mNextFrame(0), mMouseX(0), mMouseY(0)
{
	ClearKeys();
	mScreenImage = new MemoryImage(gApp);
	mScreenImage->Create(SCREEN_WIDTH, SCREEN_HEIGHT);
	mScreenImage->SetImageMode(false, false);
	mScreenImage->mUploadAsIs = true;
}

TalismaniaWidget::~TalismaniaWidget()
{
	delete mScreenImage;
}

// The run loop (0x677bd0): a frame runs once the clock passes the time the last one set, else the
// loop sleeps
void TalismaniaWidget::Update()
{
	Widget::Update();
	if (!gApp->mEngineRunning)
		return;
	if (gApp->mFastMode)
	{
		// port: -fast runs a frame each update, as the original does on origrun's virtual clock, which skips the wait
		// and moves the clock on by it
		gApp->mEngine->advanceClock(gApp->FramePeriod());
	}
	else
	{
		uint64_t aTicks = gApp->mEngine->getTicks();
		if (aTicks < mNextFrame)
			return;
		mNextFrame = aTicks + gApp->FramePeriod();
	}
	gApp->RunFrame();
}

void TalismaniaWidget::Draw(Graphics* g)
{
	g->DrawImage(mScreenImage, 0, 0);
}

namespace
{

// 0x67ac90: a position kept inside the screen
engine::Vector2<int> clampToScreen(int x, int y)
{
	if (x < 0)
		x = 0;
	else if (x > SCREEN_WIDTH - 1)
		x = SCREEN_WIDTH - 1;
	if (y < 0)
		y = 0;
	else if (y > SCREEN_HEIGHT - 1)
		y = SCREEN_HEIGHT - 1;
	return engine::Vector2<int>(x, y);
}

// The framework's mouse wheel turns in notches; WM_MOUSEWHEEL's delta is in WHEEL_DELTA steps
const int WHEEL_DELTA_PER_NOTCH = 120;

} // namespace

void TalismaniaWidget::ClearKeys()
{
	std::memset(mKeyIsDown, 0, sizeof(mKeyIsDown));
}

// WM_KEYDOWN: the key-down signal for the first, the repeat signal (with a count of 1) for each after it while the key
// is held, never both
void TalismaniaWidget::KeyDown(KeyCode theKey)
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
	if (!aRepeat)
		gApp->mEngine->mKeyDownSignal.emit(aKey);
	else
		gApp->mEngine->mKeyRepeatSignal.emit(aKey, 1);
}

// WM_KEYUP
void TalismaniaWidget::KeyUp(KeyCode theKey)
{
	int aKey = (int)theKey;
	if (aKey >= 0 && aKey < 256)
		mKeyIsDown[aKey] = false;
	if (gApp->mEngineRunning)
		gApp->mEngine->mKeyUpSignal.emit(aKey);
}

// WM_CHAR
void TalismaniaWidget::KeyChar(SexyChar theChar)
{
	if (gApp->mEngineRunning)
		gApp->mEngine->mCharSignal.emit((char)theChar);
}

// WM_MOUSEMOVE
void TalismaniaWidget::MouseMove(int x, int y)
{
	if (!gApp->mEngineRunning)
		return;
	engine::Vector2<int> aPoint = clampToScreen(x, y);
	mMouseX = aPoint.x;
	mMouseY = aPoint.y;
	gApp->mEngine->mMouseMoveSignal.emit(aPoint);
}

void TalismaniaWidget::MouseDrag(int x, int y)
{
	MouseMove(x, y);
}

// WM_LBUTTONDOWN and WM_RBUTTONDOWN (the framework's buttons: 0 left, 1 right; the original took no middle button,
// and its window class has no double clicks, so a second click is another press). The original captures the mouse
// for the window while a button is down; the framework does the same for its widgets
void TalismaniaWidget::MouseDown(int x, int y, int theBtnNum, int theClickCount)
{
	if (!gApp->mEngineRunning)
		return;
	engine::Vector2<int> aPoint = clampToScreen(x, y);
	mMouseX = aPoint.x;
	mMouseY = aPoint.y;
	if (theBtnNum == 0)
		gApp->mEngine->mMouseDownSignal.emit(aPoint);
	else if (theBtnNum == 1)
		gApp->mEngine->mRightDownSignal.emit(aPoint);
}

// WM_LBUTTONUP and WM_RBUTTONUP
void TalismaniaWidget::MouseUp(int x, int y, int theBtnNum, int theClickCount)
{
	if (!gApp->mEngineRunning)
		return;
	engine::Vector2<int> aPoint = clampToScreen(x, y);
	mMouseX = aPoint.x;
	mMouseY = aPoint.y;
	if (theBtnNum == 0)
		gApp->mEngine->mMouseUpSignal.emit(aPoint);
	else if (theBtnNum == 1)
		gApp->mEngine->mRightUpSignal.emit(aPoint);
}

// WM_MOUSEWHEEL: at the mouse's place (the original turns the message's screen position into the window's)
void TalismaniaWidget::MouseWheel(int theDelta)
{
	if (!gApp->mEngineRunning)
		return;
	gApp->mEngine->mMouseWheelSignal.emit(clampToScreen(mMouseX, mMouseY), theDelta * WHEEL_DELTA_PER_NOTCH);
}

TalismaniaApp::TalismaniaApp()
{
	gApp = this;
	mProdName = "Talismania";
	mTitle = "Talismania Deluxe";
	// The key the engine's registry keys are below (engines/sprout2/host/RegistryKey.h): Software\<vendor>\<appName> of
	// HKEY_CURRENT_USER, the vendor and the name being the application options' vendorname and gamekey
	// (config/gameoptions.xml)
	mRegKey = "Gamehouse\\Talismania";
	mWidth = SCREEN_WIDTH;
	mHeight = SCREEN_HEIGHT;
	mWidget = nullptr;
	mEngine = nullptr;
	mDisplay = nullptr;
	mGame = nullptr;
	mEngineRunning = false;
	mFrames = 0;
	mLastFrameTicks = 0;
}

TalismaniaApp::~TalismaniaApp()
{
	if (mWidget != nullptr)
	{
		mWidgetManager->RemoveWidget(mWidget);
		delete mWidget;
	}
	if (mGame != nullptr)
		mGame->release();
	if (mDisplay != nullptr)
		mDisplay->release();
	delete mEngine;
}

void TalismaniaApp::ReadFromRegistry()
{
	mReadFromRegistry = true;
}

void TalismaniaApp::WriteToRegistry()
{
}

uint64_t TalismaniaApp::FramePeriod() const
{
	return mEngine->getFramePeriod();
}

void TalismaniaApp::Init()
{
	SexyAppBase::Init();
	if (mShutdown)
		return;
	mWidget = new TalismaniaWidget();
	mWidget->Resize(0, 0, mWidth, mHeight);
	mWidgetManager->AddWidget(mWidget);
	mWidgetManager->SetFocus(mWidget);
	mEngine = new engine::EngineImpl(SCREEN_WIDTH, SCREEN_HEIGHT);
	mEngine->setVirtualClock(mFastMode);
	mEngine->setFrameRate(FRAME_RATE);
	mDisplay = new engine::Surface();
	mDisplay->addRef();
	mDisplay->allocate(SCREEN_WIDTH, SCREEN_HEIGHT);
	// The run (0x677bd0) has 0x678230 make the application (the game's boot, 0x401100) and ask for its configuration,
	// then starts it
	mGame = AtlantisApp::boot();
	mGame->configure();
	mGame->start();
	mLastFrameTicks = mEngine->getTicks();
	mEngineRunning = true;
	// port: the original's window gets WM_ACTIVATE as it opens; the framework has the focus from the start and sends
	// no first activation, so the state it has is passed on
	mEngine->mActivateSignal.emit(mHasFocus);
}

void TalismaniaApp::GotFocus()
{
	SexyAppBase::GotFocus();
	if (mEngineRunning)
		mEngine->mActivateSignal.emit(true);
}

void TalismaniaApp::LostFocus()
{
	SexyAppBase::LostFocus();
	if (mWidget != nullptr)
		mWidget->ClearKeys();
	if (mEngineRunning)
		mEngine->mActivateSignal.emit(false);
}

void TalismaniaApp::RunFrame()
{
	uint64_t aFrameStart = mEngine->getTicks();
	engine::UpdateContext aContext;
	float aTicks = (float)(int64_t)(aFrameStart - mLastFrameTicks);
	aContext.mElapsed = engine::minOf(0.33f, aTicks / (float)mEngine->getTicksPerSecond());
	aContext.mFrame = (uint64_t)mFrames;
	aContext.mDetailLevel = mEngine->getDetailLevel();
	mFrames++;
	// (the sound's update, 0x42b8c0 (talismania), comes here with the sound)
	mEngine->mUpdateSignal.emit(&aContext);

	// The frame drawn into the display through a context whose first state is at the origin, unscaled and opaque;
	// the original draws and presents only while the display is ready (DirectDraw's surface not lost)
	{
		engine::DXDrawContext aDrawContext(mDisplay);
		aDrawContext.pushState(engine::DrawState());
		aDrawContext.setDetailLevel(mEngine->getDetailLevel());
		mEngine->mDrawSignal.emit(&aDrawContext);
	}
	// The display's slot 0x14, its flip: port: the pixels copied to the framework's screen image, opaque as the
	// display's back buffer is
	const uint32_t* aSource = mDisplay->getPixels();
	uint32_t* aTarget = (uint32_t*)mWidget->mScreenImage->GetBits();
	for (int i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; i++)
		aTarget[i] = aSource[i] | 0xff000000;
	mWidget->mScreenImage->BitsChanged();
	mWidget->MarkDirty();
	mLastFrameTicks = aFrameStart;

	if (mEngine->mQuitRequested)
		Shutdown();
}

void TalismaniaApp::Shutdown()
{
	mEngineRunning = false;
	SexyAppBase::Shutdown();
}
