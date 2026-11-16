#include "PizzaEditorState.h"
#include "DialogState.h"
#include "MenuState.h"

using namespace pizzafrenzy;

namespace
{
const char* const ACTION_EDITOR_EXIT = "editorExit";
const char* const ACTION_NAME_PIZZA = "namePizza";
}

// 0x4027d0
void PizzaEditorState::enter()
{
	mApp->mGameScreenActive = true;
	mApp->mScreens->showScreen(mApp->mPizzaEditorScreen, 0);
	mApp->mMusicPlayer->stop();
}

// 0x401150
void PizzaEditorState::exit()
{
	mApp->mGameScreenActive = false;
}

// 0x408250
void PizzaEditorState::action(const std::string& theAction)
{
	if (theAction.compare(ACTION_EDITOR_EXIT) == 0)
	{
		if (mFromNewTopping)
		{
			mApp->mGameStats->addDesignedPizzas();
			mApp->showLevelStart();
			return;
		}
		mApp->mStates.changeState(new MenuState(mApp));
	}
	else if (theAction.compare(ACTION_NAME_PIZZA) == 0)
		mApp->mStates.changeState(new DialogState(mApp, mApp->mPizzaNameScreen, this));
}
