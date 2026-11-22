#include "StageIntroState.h"

#include "Engine.h"
#include "PopupState.h"
#include "ScreenMgr.h"

using namespace engine;

void StageIntroState::enter()
{
	// (The id is the game's stage entry's, 0x403460 (feedingfrenzy2))
	mApp->mStageIntro->showFor(mApp->mGameData->getStageEntry()->mId, mApp->mStage.get());
	mApp->mScreenMgr->showScreen(mApp->mStageIntro.get(), 0.0f);
}

void StageIntroState::action(const std::string& theAction)
{
	if (theAction == "stageStartContinue")
		// The play (GamePlayState, 0x1c bytes, its constructor 0x49b390 (feedingfrenzy2)) in its place: not yet
		// translated
		getEngine()->log("StageIntroState: the play not yet translated\n");
	else if (theAction == "stageStartOptions")
		// The options (+0xa4, their 0x50ab13 (feedingfrenzy2) given false first) as a popup over the intro: not yet
		// translated
		getEngine()->log("StageIntroState: the options not yet translated\n");
	else if (theAction == "stageStartQuit")
		mApp->mStateMgr.pushState(new PopupState(mApp, mApp->mGameExitConfirm.get(), this));
}
