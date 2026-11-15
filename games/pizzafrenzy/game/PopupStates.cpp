#include "PopupStates.h"
#include "PizzaPopup.h"

using namespace pizzafrenzy;

namespace
{
const float APPEAR_SECONDS = 0.2f;
const float APPEAR_START_SCALE = 0.5f;
const float CLOSE_SECONDS = 0.5f;
}

// 0x0c: the state is counted for the length of the update, so that a change of state it makes does not free it while
// it runs
void PopupStateMgr::update(engine::UpdateInfo& theInfo)
{
	engine::Ref<engine::State> aState = mState;
	StateMgr::update(theInfo);
}

// 0x454ee0
bool PopupStateMgr::setState(engine::State* theState)
{
	engine::Ref<engine::State> aNewState = theState;
	if (mState != nullptr)
		mState->exit();
	mState = nullptr;
	mState = theState;
	if (mState != nullptr)
		mState->enter();
	return true;
}

// 0x454d90
void AppearState::enter()
{
	mTimeLeft = APPEAR_SECONDS;
}

// 0x454da0
void AppearState::update(engine::UpdateInfo& theInfo)
{
	mTimeLeft = mTimeLeft - theInfo.mSeconds;
	if (mTimeLeft < 0.0f)
		mTimeLeft = 0.0f;
	float aShare = mTimeLeft / APPEAR_SECONDS;
	mOwner->setOpacity(1.0f - aShare);
	mOwner->setScale(1.0f - aShare * APPEAR_START_SCALE);
	if (mTimeLeft <= 0.0f)
		mOwner->rest();
}

// 0x451310
void RestState::enter()
{
	mOwner->setOpacity(1.0f);
	mOwner->setScale(1.0f);
}

// 0x454e30
void ClosingState::enter()
{
	mTimeLeft = CLOSE_SECONDS;
	mOwner->setLeaving(true);
	mOwner->setSelectionVisible(false);
}

// 0x454e50
void ClosingState::exit()
{
	mOwner->setLeaving(false);
}

// 0x454e70
void ClosingState::update(engine::UpdateInfo& theInfo)
{
	mTimeLeft = mTimeLeft - theInfo.mSeconds;
	if (mTimeLeft < 0.0f)
		mTimeLeft = 0.0f;
	mOwner->setOpacity(mTimeLeft / CLOSE_SECONDS);
	mOwner->setScale(mTimeLeft / CLOSE_SECONDS);
	if (mTimeLeft <= 0.0f)
		mOwner->leave();
}
