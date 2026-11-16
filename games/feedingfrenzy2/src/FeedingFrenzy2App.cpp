// port: the host for Feeding Frenzy 2 (see FeedingFrenzy2App.h).

#include "FeedingFrenzy2App.h"
#include "EngineImpl.h"
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

FeedingFrenzy2App::FeedingFrenzy2App()
{
	gApp = this;
	mProdName = "FeedingFrenzy2";
	mTitle = "Feeding Frenzy 2 - Shipwreck Showdown";
	mWidth = SCREEN_WIDTH;
	mHeight = SCREEN_HEIGHT;
	mWidget = nullptr;
	mEngine = nullptr;
	mEngineRunning = false;
	mFrames = 0;
}

FeedingFrenzy2App::~FeedingFrenzy2App()
{
	if (mWidget != nullptr)
	{
		mWidgetManager->RemoveWidget(mWidget);
		delete mWidget;
	}
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
	mEngineRunning = true;
}

void FeedingFrenzy2App::RunFrame()
{
	engine::UpdateContext aContext;
	aContext.mFrame = (uint64_t)mFrames;
	aContext.mDetailLevel = mEngine->getDetailLevel();
	mFrames++;
	mEngine->mUpdateSignal.emit(&aContext);
	if (mEngine->mQuitRequested)
		Shutdown();
}

void FeedingFrenzy2App::Shutdown()
{
	mEngineRunning = false;
	SexyAppBase::Shutdown();
}
