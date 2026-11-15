#ifndef __SPROUT_STATE_H__
#define __SPROUT_STATE_H__

// The engine's state machine (the signal build's, Pizza Frenzy's): the game's application keeps one (at its +0x24) and
// hands it each frame's update and each screen's action; the current state gets them.

#include "Node.h"
#include "Object.h"

#include <string>

namespace engine
{

// A state (base vtable 0x4fc1e8 (pizzafrenzy)): entered when it becomes the current one, left when another does
class State : public Object
{
public:
	~State() override {}                              // 0x44a0a0 (pizzafrenzy)
	virtual void enter() {}                           // 0x04: 0x4d0470 (pizzafrenzy)
	virtual void exit() {}                            // 0x08: 0x4d0470 (pizzafrenzy)
	virtual void update(UpdateInfo& theInfo) {}       // 0x0c: 0x492310 (pizzafrenzy)
	virtual void action(const std::string& theAction) {}  // 0x10: 0x492310 (pizzafrenzy)
};

// The machine (constructor 0x470750 (pizzafrenzy), vtable 0x5022b4 (pizzafrenzy)): the current state at +0x0c, counted
class StateMgr : public Object
{
public:
	StateMgr() {}            // 0x470750 (pizzafrenzy)
	~StateMgr() override {}  // 0x4707d0 (pizzafrenzy)

	// 0x04: 0x4707f0 (pizzafrenzy): the current state is left, the new one counted and entered
	virtual bool changeState(State* theState);
	// 0x08: 0x4706c0 (pizzafrenzy), the same
	virtual bool pushState(State* theState) { return changeState(theState); }
	// 0x0c: 0x470860 (pizzafrenzy)
	virtual void update(UpdateInfo& theInfo);
	// 0x10: 0x470850 (pizzafrenzy)
	virtual void action(const std::string& theAction);

	State* getState() const { return mState; }

protected:
	Ref<State> mState;  // +0x0c
};

} // namespace engine

#endif
