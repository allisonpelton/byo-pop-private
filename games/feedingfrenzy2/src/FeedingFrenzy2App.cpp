// port: the host for Feeding Frenzy 2 (see FeedingFrenzy2App.h).

#include "FeedingFrenzy2App.h"
#include "DXDrawContext.h"
#include "EngineImpl.h"
#include "FeedingFrenzyTwo.h"
#include "Graphics.h"
#include "MemoryImage.h"
#include "Surface.h"
#include "WidgetManager.h"

using namespace Sexy;

FeedingFrenzy2App* Sexy::gApp = nullptr;

namespace
{

// The game's screen (FeedingFrenzyTwo's configuration, 0x48c080 (feedingfrenzy2))
const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;
// And its frame rate
const int FRAME_RATE = 60;

} // namespace

FeedingFrenzy2Widget::FeedingFrenzy2Widget() : mNextFrame(0)
{
	mScreenImage = new MemoryImage(gApp);
	mScreenImage->Create(SCREEN_WIDTH, SCREEN_HEIGHT);
	mScreenImage->SetImageMode(false, false);
	mScreenImage->mUploadAsIs = true;
}

FeedingFrenzy2Widget::~FeedingFrenzy2Widget()
{
	delete mScreenImage;
}

// The run loop (0x4383b0 (feedingfrenzy2)): a frame runs once the clock passes the time the last one set, else the
// loop sleeps
void FeedingFrenzy2Widget::Update()
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

void FeedingFrenzy2Widget::Draw(Graphics* g)
{
	g->DrawImage(mScreenImage, 0, 0);
}

FeedingFrenzy2App::FeedingFrenzy2App()
{
	gApp = this;
	mProdName = "FeedingFrenzy2";
	mTitle = "Feeding Frenzy 2 - Shipwreck Showdown";
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

FeedingFrenzy2App::~FeedingFrenzy2App()
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

uint64_t FeedingFrenzy2App::FramePeriod() const
{
	return mEngine->getTicksPerSecond() / FRAME_RATE;
}

void FeedingFrenzy2App::Init()
{
	SexyAppBase::Init();
	if (mShutdown)
		return;
	mWidget = new FeedingFrenzy2Widget();
	mWidget->Resize(0, 0, mWidth, mHeight);
	mWidgetManager->AddWidget(mWidget);
	mWidgetManager->SetFocus(mWidget);
	mEngine = new engine::EngineImpl(SCREEN_WIDTH, SCREEN_HEIGHT);
	mEngine->setVirtualClock(mFastMode);
	mDisplay = new engine::Surface();
	mDisplay->addRef();
	mDisplay->allocate(SCREEN_WIDTH, SCREEN_HEIGHT);
	// The run (0x4383b0 (feedingfrenzy2)) has 0x49bc60 (feedingfrenzy2) make the application and ask for its
	// configuration, then starts it (the screensaver's letters are not yet translated)
	mGame = new FeedingFrenzyTwo();
	mGame->addRef();
	mGame->configure();
	mGame->start(false);
	mLastFrameTicks = mEngine->getTicks();
	mEngineRunning = true;
}

void FeedingFrenzy2App::RunFrame()
{
	uint64_t aFrameStart = mEngine->getTicks();
	engine::UpdateContext aContext;
	float aTicks = (float)(int64_t)(aFrameStart - mLastFrameTicks);
	aContext.mElapsed = engine::minOf(0.33f, aTicks / (float)mEngine->getTicksPerSecond());
	aContext.mFrame = (uint64_t)mFrames;
	aContext.mDetailLevel = mEngine->getDetailLevel();
	mFrames++;
	// (the sound's update, 0x42b8c0 (feedingfrenzy2), comes here with the sound)
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

void FeedingFrenzy2App::Shutdown()
{
	mEngineRunning = false;
	SexyAppBase::Shutdown();
}
