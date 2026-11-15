#include "State.h"

using namespace engine;

// 0x4707f0 (pizzafrenzy): the old state's exit runs while it is still the current one; it is released after the new
// one is counted
bool StateMgr::changeState(State* theState)
{
	if (mState != nullptr)
		mState->exit();
	mState = theState;
	if (theState != nullptr)
		theState->enter();
	return true;
}

// 0x470860 (pizzafrenzy)
void StateMgr::update(UpdateInfo& theInfo)
{
	if (mState != nullptr)
		mState->update(theInfo);
}

// 0x470850 (pizzafrenzy)
void StateMgr::action(const std::string& theAction)
{
	if (mState != nullptr)
		mState->action(theAction);
}
