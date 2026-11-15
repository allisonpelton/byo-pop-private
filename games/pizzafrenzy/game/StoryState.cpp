#include "StoryState.h"
#include "DialogState.h"
#include "MenuState.h"

using namespace pizzafrenzy;

namespace
{
const char* const STORY_MUSIC = "music_story";
const char* const ACTION_STORY_DONE = "storyDone";
const char* const ACTION_END_GAME = "endGame";
const char* const ACTION_CONFIRM_QUIT = "confirmQuit";
}

// 0x406560
void StoryState::enter()
{
	mApp->mMusicPlayer->play(mApp->mTileManifest->getMusic(STORY_MUSIC));
	mApp->mScreens->showScreen(mApp->mStoryScreen, 0);
	mApp->mStoryScreen->connect();
	mStarted = false;
	mGameEnded = false;
	mStoryDone = false;
}

// 0x408360: the story starts when the screen is up; at its end the level goes on, or the game is over. The original
// then posts the score on the global board (a dialog, when the game scored) before the menu; the port has no online
// scores, so the menu comes straight.
void StoryState::update(engine::UpdateInfo& theInfo)
{
	mApp->mStoryScreen->updateScreen(theInfo);
	if (!mStarted && !mApp->mScreens->isBusy())
	{
		mApp->mStoryScreen->startStory();
		mStarted = true;
	}
	if (mStoryDone)
	{
		mApp->mStoryScreen->endStory();
		mApp->giveToppings();
		return;
	}
	if (!mGameEnded)
		return;
	if (mApp->mHighScores->addCurrentScore())
		mApp->mShowHighScores = true;
	mApp->mStates.changeState(new MenuState(mApp));
}

// 0x4038a0
void StoryState::action(const std::string& theAction)
{
	if (theAction.compare(ACTION_STORY_DONE) == 0)
	{
		mStoryDone = true;
		return;
	}
	if (theAction.compare(ACTION_END_GAME) == 0)
	{
		mApp->mStoryScreen->endStory();
		mGameEnded = true;
	}
}

// 0x4064a0
void LevelSummaryState::enter()
{
	mApp->mScreens->showScreen(mApp->mStoryScreen, 0);
	mApp->mStoryScreen->connect();
	mApp->mMusicPlayer->play(mApp->mTileManifest->getMusic(STORY_MUSIC));
	mUpgradeShown = false;
	mStoryDone = false;
}

// 0x40cff0: when the summary's story is done the upgrades come one at a time, and then the next level starts
void LevelSummaryState::update(engine::UpdateInfo& theInfo)
{
	mApp->mStoryScreen->updateScreen(theInfo);
	if (!mStarted && !mApp->mScreens->isBusy())
	{
		mApp->mStoryScreen->startStory();
		mStarted = true;
		return;
	}
	if (!mStoryDone)
		return;
	mStoryDone = false;
	if (!showNextUpgrade())
	{
		if (!mUpgradeShown)
		{
			mApp->mStoryScreen->endStory();
			mApp->startLevel();
			return;
		}
		mUpgradeShown = false;
		mApp->mStoryScreen->startUpgradeStory();
	}
	else
		mUpgradeShown = true;
	mApp->mStoryScreen->startStory();
}

// 0x4037e0: the quit button asks first; the confirmation takes this state back when it closes
void LevelSummaryState::action(const std::string& theAction)
{
	if (theAction.compare(ACTION_STORY_DONE) == 0)
	{
		mStoryDone = true;
		return;
	}
	if (theAction.compare(ACTION_CONFIRM_QUIT) == 0)
		mApp->mStates.changeState(new DialogState(mApp, mApp->mGameExitConfirm, this));
}

// 0x40bf80
bool LevelSummaryState::showNextUpgrade()
{
	GameStats* aStats = mApp->mGameStats;
	for (auto& aCombo : aStats->mBestCombos)
	{
		Topping* aTopping = aCombo.first;
		if (aTopping == nullptr)
			continue;
		int aNewLevel = 0;
		for (size_t i = 0; i < aTopping->mUpgrades.size(); i++)
		{
			if (aTopping->mUpgrades[i]->mCombo <= aCombo.second)
				aNewLevel = (int)i;
		}
		if (aStats->mToppingLevels[aTopping] < aNewLevel)
		{
			mApp->mStoryScreen->showUpgrade(aTopping, aTopping->mUpgrades[aNewLevel]);
			aStats->mToppingLevels[aTopping] = aNewLevel;
			mApp->saveGame();
			return true;
		}
	}
	return false;
}
