#include "FeedingFrenzyTwo.h"

#include "Engine.h"
#include "ScreenMgr.h"
#include "SplashState.h"
#include "PopupState.h"
#include "UserManager.h"

using namespace engine;

namespace
{

FeedingFrenzyTwo* gTheApp = nullptr;

} // namespace

FeedingFrenzyTwo* getApp()
{
	return gTheApp;
}

FeedingFrenzyTwo::FeedingFrenzyTwo() : mMenuRunning(false), mMenuScreen(MENU_SCREEN_MAIN)
{
	// The state manager is a member, not counted by anyone: the port holds a reference so its count never reaches 0
	mStateMgr.addRef();
	gTheApp = this;
}

FeedingFrenzyTwo::~FeedingFrenzyTwo()
{
	mStateMgr.changeState(nullptr);
	// The shutdown (0x495650 (feedingfrenzy2), not yet translated) closes the players' key (0x4973d0 (feedingfrenzy2))
	// and deletes the players (0x4a4700 (feedingfrenzy2))
	UserManager::destroy();
	PopupState::releaseScreenMgr();
	gTheApp = nullptr;
}

void FeedingFrenzyTwo::configure()
{
	Engine* anEngine = getEngine();
	anEngine->setUseLooseFiles(false);
	anEngine->addArchive("FF2.saf");
	mStrings = new StringTable();
	mStrings->load("resource/baseStrings.xml");
	mVendor = mStrings->getString("vendor");
	mAppName = mStrings->getString("appName");
	// The options (+0xb8, FeedingFrenzyTwoOptions, 0xd8 bytes, its constructor 0x496c80 (feedingfrenzy2), over the
	// vendor and the name) read, and the window's title, size, rate and full screen from them: not yet translated (the
	// port's host sets the window)
}

void FeedingFrenzyTwo::start(bool theSaverMode)
{
	Engine* anEngine = getEngine();
	anEngine->mUpdateSignal.connect(this, &FeedingFrenzyTwo::updateSlot);

	mScene = new Scene();
	mScene->enable();

	mContainerSlot = new Container();
	mContainerSlot->setName("ContainerSlot");
	mScene->addChild(mContainerSlot.get());

	mStageContainer = new Container();
	mStageContainer->setName("StageContainer");

	mScreenMgr = new ScreenMgr();
	mScene->addChild(mScreenMgr.get());

	UserManager::instance()->open(mAppName, mVendor);

	// The debug display (DebugHud, 0x1f0 bytes, with resource/fonts/debugFont.xml, added to the scene): not yet
	// translated

	mStateMgr.changeState(new SplashState(this, theSaverMode));
}

void FeedingFrenzyTwo::updateSlot(UpdateContext* theContext)
{
	mStateMgr.update(theContext);
}
