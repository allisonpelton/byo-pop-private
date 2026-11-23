#include "StageInfoScreen.h"
#include "FishFood.h"
#include "StageMgr.h"
#include "TextItem.h"

using namespace engine;
using namespace feedingfrenzy;

namespace
{
// The actions of the screens' buttons (the layouts name them)
const char* const START_STAGE_ACTION = "stageStart";
const char* const QUIT_GAME_ACTION = "quitGame";
const char* const CONTINUE_GAME_ACTION = "continueGame";
const char* const NEXT_STAGE_ACTION = "nextStage";
const char* const OPTIONS_ACTION = "options";
}

// 0x429db0
StageInfoScreen::StageInfoScreen(StageMgr* theStage) : mStage(theStage), mAcceptable(true)
{
}

// 0x429c10
Node* StageInfoScreen::hitTest(unsigned int theFlags, Point& thePoint)
{
	if (!mAcceptable)
		return nullptr;
	return ScreenLayout::hitTest(theFlags, thePoint);
}

void StageInfoScreen::setText(const std::string& theNodeName, const std::string& theText)
{
	TextItem* aText = static_cast<TextItem*>(getNode(theNodeName));
	if (aText != nullptr)
		aText->setText(theText);
}

// 0x429c50: the buttons that go on in the stage, or to the options
void StageInfoScreen::buttonPress(std::string theName)
{
	if (theName == START_STAGE_ACTION)
	{
		FishFood::instance()->slideLogoOut();
		mStage->startPlaying();
	}
	else if (theName == QUIT_GAME_ACTION)
		mStage->closeScreen();
	else if (theName == CONTINUE_GAME_ACTION)
		mStage->useContinue();
	else if (theName == NEXT_STAGE_ACTION)
		mStage->goToNextStage();
	else if (theName == OPTIONS_ACTION)
	{
		mAcceptable = false;
		FishFood::instance()->screenAction(OPTIONS_ACTION);
	}
}
