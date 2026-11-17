#include "FeedingFrenzyTwo.h"

#include "Engine.h"
#include "ScreenMgr.h"
#include "SplashState.h"

using namespace engine;

namespace
{

FeedingFrenzyTwo* gTheApp = nullptr;

} // namespace

FeedingFrenzyTwo* getApp()
{
	return gTheApp;
}

FeedingFrenzyTwo::FeedingFrenzyTwo()
{
	// The state manager is a member, not counted by anyone: the port holds a reference so its count never reaches 0
	mStateMgr.addRef();
	gTheApp = this;
}

FeedingFrenzyTwo::~FeedingFrenzyTwo()
{
	mStateMgr.changeState(nullptr);
	gTheApp = nullptr;
}

void FeedingFrenzyTwo::configure()
{
	Engine* anEngine = getEngine();
	anEngine->setUseLooseFiles(false);
	anEngine->addArchive("FF2.saf");
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

	// The popups (0x40a630 and 0x4921e0 (feedingfrenzy2)) and the debug display (DebugHud, 0x1f0 bytes, with
	// resource/fonts/debugFont.xml, added to the scene): not yet translated

	mStateMgr.changeState(new SplashState(this, theSaverMode));
}

void FeedingFrenzyTwo::updateSlot(UpdateContext* theContext)
{
	mStateMgr.update(theContext);
}
