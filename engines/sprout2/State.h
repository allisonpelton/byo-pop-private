#ifndef __SPROUT2_STATE_H__
#define __SPROUT2_STATE_H__

// States and the machines that run them (State, vtable 0x5502d8 (feedingfrenzy2); StateMgr, vtable 0x55bb40
// (feedingfrenzy2), and its typed form AbstractStateMgr<T>): a machine holds one state, which it enters, updates each
// frame and leaves when another takes its place. The game's application, its fish and its screens each run one.
// Only the slots the translated states use are declared; the rest (State's 0x10 to 0x20, StateMgr's 0x10 to 0x18)
// are added with their first caller.

#include "Engine.h"
#include "Object.h"

namespace engine
{

class State : public Object
{
public:
	// 0x04: the state has become the current one
	virtual void enter() {}
	// 0x08: another takes its place
	virtual void exit() {}
	// 0x0c: each frame while it is the current one
	virtual void update(UpdateContext* theContext) {}
};

template <class T>
class StateMgr : public Object
{
public:
	StateMgr() : mChanging(false) {}

	// AbstractStateMgr's 0x1c (0x430760, a jump to StateMgr's 0x04, 0x434c40 (feedingfrenzy2)): the current state
	// left, the new one put in its place and entered (StateMgr's untyped 0x04, which AbstractStateMgr makes return
	// false, takes none)
	virtual bool changeState(T* theState)
	{
		mChanging = true;
		if (mState != nullptr)
			mState->exit();
		mState = theState;
		mChanging = false;
		if (mState != nullptr)
			mState->enter();
		return true;
	}
	// 0x0c (0x4448d0 (feedingfrenzy2))
	virtual void update(UpdateContext* theContext)
	{
		if (mState != nullptr)
			mState->update(theContext);
	}
	// AbstractStateMgr's 0x20 (0x50b668 (feedingfrenzy2))
	virtual T* getState() { return mState.get(); }

private:
	Ref<T> mState;    // +0x0c
	bool mChanging;   // +0x10: while the old state leaves (not yet seen read)
};

} // namespace engine

#endif
