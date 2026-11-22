#include "LevelLoadState.h"

#include "Engine.h"
#include "FishFactory.h"
#include "Random.h"
#include "ScreenMgr.h"
#include "SimpleSound.h"

using namespace engine;

namespace
{

// The frame rates while a stage loads and while it plays
const int LOAD_FRAME_RATE = 30;
const int GAME_FRAME_RATE = 60;
// What the bar gains each frame once the stage is loaded (a double in the original's sum)
const double LOAD_BAR_STEP = 0.05;

// 0x48e0b0 (feedingfrenzy2): the sound system's slot 0x2c, when there is one
void stopAllSounds()
{
	simplesound::SimpleSound* aSound = simplesound::getSimpleSound();
	if (aSound != nullptr)
		aSound->stopAll();
}

// 0x40c0b0 (feedingfrenzy2): the stage's intro screen's data, the timed mode's own ("<id>_timed") first in that mode;
// null for none
ScreenDataEntry* findIntroData(FeedingFrenzyTwo* theApp, const StageManifest::Stage& theEntry)
{
	if (theApp->mGameData->getMode() == GAME_MODE_TIMED)
	{
		ScreenDataEntry* aData = theApp->mStageScreenData->getEntry(theEntry.mId + "_timed");
		if (aData != nullptr)
			return aData;
	}
	return theApp->mStageScreenData->getEntry(theEntry.mId);
}

} // namespace

LevelLoadState::LevelLoadState(FeedingFrenzyTwo* theApp, const StageManifest::Stage* theEntry, AppState* theNextState)
    : AppState(theApp), mLoading(false), mEntry(theEntry), mShownProgress(0.0f), mLoadStatus(STAGE_LOAD_NOT_STARTED),
      mScreen(nullptr), mLoadBar(nullptr), mNextStateLogged(false)
{
	mNextState = theNextState;
}

void LevelLoadState::enter()
{
	getEngine()->setFrameRate(LOAD_FRAME_RATE);
	// The application's sound at +0x38 stopped (0x490370 (feedingfrenzy2)): not yet translated
	stopAllSounds();
	// The stage's theme's loadTrack played by the application's music player (0x489670 and 0x48d420
	// (feedingfrenzy2)), then the sounds of the actors' library's +0x34 and of the engine's +0x134 that play stopped
	// (0x401eb0 and 0x4719f0 (feedingfrenzy2)) and the sound system's unused sources let go of (its slot 0x10,
	// 0x48de10 (feedingfrenzy2)): not yet translated, as the music and the sound effects are not
	if (mEntry->mResource.find("menu") == std::string::npos)
	{
		// 0x4966d0 (feedingfrenzy2): the game kept in the player's record, and the player saved
		if (mApp->mGameData)
			mApp->mUserData->recordGame(*mApp->mGameData);
		mApp->mUserData->save();
		// The map (+0x84) as the load screen, set up for the mode without its buttons (0x478d90 (feedingfrenzy2)):
		// not yet translated, so the stage loads with no screen
		getEngine()->log("LevelLoadState: the map not yet translated\n");
		mScreen = nullptr;
	}
	else
		mScreen = mApp->mGenericLoadScreen.get();
	mLoadBar = mScreen != nullptr ? dynamic_cast<Image*>(mScreen->findComponent("loadBar")) : nullptr;
	if (mLoadBar != nullptr)
	{
		mLoadBar->computeBounds();
		mLoadBar->setSource(RectI(0, 0, 0, mLoadBar->getHeight()));
	}
	mShownProgress = 0.0f;
	if (mScreen != nullptr)
		mApp->mScreenMgr->showScreen(mScreen, 0.0f);
	stopAllSounds();
}

void LevelLoadState::exit()
{
	getEngine()->setFrameRate(GAME_FRAME_RATE);
	// The sounds of the actors' library's +0x34 and of the engine's +0x134 that play stopped, and the sound system's
	// unused sources let go of (0x401eb0, 0x4719f0 and 0x48de10 (feedingfrenzy2)): not yet translated
}

void LevelLoadState::update(UpdateContext* theContext)
{
	if (!mLoading)
	{
		// port: with no screen (the map is not yet translated) the load starts at once
		if (mScreen == nullptr || mScreen->getPhase() == SCREEN_SHOWN)
			startLoading();
		return;
	}
	// While the loader runs, the bar follows its progress by a hundredth a frame: the port's load is over before
	// this update, so the bar starts from where it is
	if (mLoadStatus != STAGE_LOAD_FAILED && mLoadStatus != STAGE_LOAD_DONE)
		return;
	if (mLoadBar != nullptr && mShownProgress < 1.0f)
	{
		int aWidth = mLoadBar->getWidth();
		int aHeight = mLoadBar->getHeight();
		mShownProgress = (float)(mShownProgress + LOAD_BAR_STEP);
		mLoadBar->setSource(RectI(0, 0, (int)((double)aWidth * mShownProgress), aHeight));
		return;
	}
	// 0x494820 (feedingfrenzy2): the stage back under the screens
	if (mApp->mStageContainer != nullptr)
		mApp->mContainerSlot->addChild(mApp->mStageContainer.get());
	// The pause screen's stageTitle set to the stage's name (0x40c640 (feedingfrenzy2)): not yet translated
	if (mNextState)
	{
		mApp->mStateMgr.changeState(mNextState.get());
		return;
	}
	if (mNextStateLogged)
		return;
	// The stage's intro (StageIntroState, 0x18 bytes, 0x49d1b0 (feedingfrenzy2)) when it has intro data, else the
	// play (GamePlayState, 0x1c bytes): not yet translated
	if (findIntroData(mApp, *mEntry) != nullptr)
		getEngine()->log("LevelLoadState: %s loaded; the stage's intro not yet translated\n", mEntry->mId.c_str());
	else
		getEngine()->log("LevelLoadState: %s loaded; the play not yet translated\n", mEntry->mId.c_str());
	mNextStateLogged = true;
}

void LevelLoadState::startLoading()
{
	// 0x496dd0 (feedingfrenzy2): the stage taken out from under the screens while it loads
	if (mApp->mStageContainer != nullptr)
		mApp->mContainerSlot->removeChild(mApp->mStageContainer.get());
	// The loader's thread started (0x496d50 (feedingfrenzy2)): the port loads at once, drawing from the loader
	// thread's own random numbers, which start anew with each thread
	RandomThread aThread = getRandomThread();
	resetRandomThread(RANDOM_THREAD_STAGE_LOADER);
	setRandomThread(RANDOM_THREAD_STAGE_LOADER);
	load();
	setRandomThread(aThread);
	mLoading = true;
}

void LevelLoadState::load()
{
	Stage* aStage = mApp->mStage.get();
	mLoadStatus = STAGE_LOAD_RUNNING;
	aStage->getContainer()->setVisible(false);
	aStage->unload();
	FishFactory* aFactory = FishFactory::get();
	aFactory->clear();
	aFactory->load("resource/actors/fishes.xml");
	if (!aStage->load(*mEntry))
	{
		mLoadStatus = STAGE_LOAD_FAILED;
		return;
	}
	aStage->setStageTime((float)mEntry->mStageTime);
	aStage->setCanSuck(mEntry->mCanSuck);
	// The HUD's +0x180, +0x244 and +0x248 shown or hidden by canSuck (0x48a910 (feedingfrenzy2)), and the globals at 0x5ac61c and 0x5ac620 (feedingfrenzy2) cleared: not yet translated
	aStage->getContainer()->setVisible(true);
	mLoadStatus = STAGE_LOAD_DONE;
}
