#include "StoryState.h"
#include "MenuState.h"

using namespace pizzafrenzy;

namespace
{
const char* const STORY_MUSIC = "music_story";
const char* const ACTION_STORY_DONE = "storyDone";
const char* const ACTION_END_GAME = "endGame";
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
