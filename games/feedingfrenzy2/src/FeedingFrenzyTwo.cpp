#include "FeedingFrenzyTwo.h"

#include "Engine.h"
#include "ScreenMgr.h"
#include "SimpleSound.h"
#include "SplashState.h"
#include "PopupState.h"
#include "UserManager.h"

using namespace engine;

namespace
{

FeedingFrenzyTwo* gTheApp = nullptr;

} // namespace

ScreenDataEntry* FeedingFrenzyTwo::getScreenData(const std::string& theStageId)
{
	if (mGameData->getMode() == GAME_MODE_TIMED)
	{
		ScreenDataEntry* aData = mStageScreenData->getEntry(theStageId + "_timed");
		if (aData != nullptr)
			return aData;
	}
	return mStageScreenData->getEntry(theStageId);
}

FeedingFrenzyTwo* getApp()
{
	return gTheApp;
}

FeedingFrenzyTwo::FeedingFrenzyTwo()
	: mMenuRunning(false), mActive(true), mStageInPlay(false), mFreshGame(false), mMenuScreen(MENU_SCREEN_MAIN)
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

namespace
{

const float MASTER_VOLUME = 0.65f;  // 0x566d20 (feedingfrenzy2)

} // namespace

void FeedingFrenzyTwo::startSound()
{
	Engine* anEngine = getEngine();
	simplesound::SimpleSound* aSound = anEngine->makeSoundSystem();
	anEngine->setSoundSystem(aSound);
	// The application's +0x0c and +0x0d (whether sound and music can be played) set to whether it started: read by
	// nothing translated yet
	anEngine->log(aSound != nullptr ? "Sound Engine initialized.\n" : "Sound Engine failed to initialize.\n");
}

void FeedingFrenzyTwo::start(bool theSaverMode)
{
	Engine* anEngine = getEngine();
	// The options' window settings come first (not yet translated, as the options are not)
	startSound();
	if (simplesound::getSimpleSound() != nullptr)
		simplesound::getSimpleSound()->setMasterVolume(MASTER_VOLUME);
	// Then the sound's and the music's categories (0 and 1) on or off by the options' SoundEnabled and MusicEnabled
	// (the application's slots 0x18 and 0x1c, 0x494970 and 0x500fbe (feedingfrenzy2)): not yet translated, as the
	// options are not, so both stay at 1, as when enabled
	anEngine->mUpdateSignal.connect(this, &FeedingFrenzyTwo::updateSlot);
	anEngine->mKeyDownSignal.connect(this, &FeedingFrenzyTwo::keyDownSlot);
	// (The key-up, minimize and restore slots between them: not yet translated)
	anEngine->mActivateSignal.connect(this, &FeedingFrenzyTwo::activateSlot);

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

void FeedingFrenzyTwo::keyDownSlot(int theKey)
{
	if (mStateMgr.getState() != nullptr)
		mStateMgr.getState()->keyDown(theKey);
	// F11's screenshot: not yet translated
}

void FeedingFrenzyTwo::activateSlot(bool theActive)
{
	if (!theActive)
		mStateMgr.action("pause");
	mActive = theActive;
}

void FeedingFrenzyTwo::updateSlot(UpdateContext* theContext)
{
	mStateMgr.update(theContext);
}
