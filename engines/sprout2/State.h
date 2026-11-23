#ifndef __SPROUT2_STATE_H__
#define __SPROUT2_STATE_H__

// States and the machines that run them (State, vtable 0x5502d8 (feedingfrenzy2); StateMgr, vtable 0x55bb40
// (feedingfrenzy2), and its typed form AbstractStateMgr<T>): a machine holds one state, which it enters, updates each
// frame and leaves when another takes its place. The game's application, its fish and its screens each run one.
// A state can be put aside for another (a popup) and put back: it is suspended rather than left, and resumed rather
// than entered.

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
	// 0x10: an action of a screen's (a button's name), passed on by the machine; State's own does nothing
	virtual void action(const std::string& theAction) {}
	// 0x14: another put in its place for a while (pushState)
	virtual void suspend() {}
	// 0x18: put back in its place (popState)
	virtual void resume() {}
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
	// 0x14 (0x489840 (feedingfrenzy2)): the current state suspended, the new one put in its place and entered (the
	// machine keeps no stack: the new state holds the old one to put back)
	virtual void pushState(T* theState)
	{
		if (mState != nullptr)
			mState->suspend();
		mState = theState;
		if (mState != nullptr)
			mState->enter();
	}
	// 0x18 (0x48ad30 (feedingfrenzy2)): the current state left, the given one put back and resumed
	virtual void popState(T* theState)
	{
		if (mState != nullptr)
			mState->exit();
		mState = theState;
		if (mState != nullptr)
			mState->resume();
	}
	// 0x0c (0x4448d0 (feedingfrenzy2)). port: the state held while it updates, as one may put another in its place
	// (the original's state lives on in freed memory until it returns)
	virtual void update(UpdateContext* theContext)
	{
		Ref<T> aState = mState;
		if (aState != nullptr)
			aState->update(theContext);
	}
	// 0x10 (0x40a610 (feedingfrenzy2)): passed to the current state
	virtual void action(const std::string& theAction)
	{
		if (mState != nullptr)
			mState->action(theAction);
	}
	// AbstractStateMgr's 0x20 (0x50b668 (feedingfrenzy2))
	virtual T* getState() { return mState.get(); }

private:
	Ref<T> mState;    // +0x0c
	bool mChanging;   // +0x10: while the old state leaves (not yet seen read)
};

} // namespace engine

#endif
