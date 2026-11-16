#ifndef __SPROUT2_TRANSITION_H__
#define __SPROUT2_TRANSITION_H__

// A screen's way in or out (Transition, vtable 0x55c480 (feedingfrenzy2), its constructor 0x489b90 (feedingfrenzy2)):
// a modifier that moves a progress from 0 to 1 (in) or from 1 to 0 (out) over a duration and applies it to its
// component; FadeTransition (vtable 0x5561a8 (feedingfrenzy2), 0x34 bytes, its constructor 0x489bf0 (feedingfrenzy2))
// applies it as the opacity. A transition is done when it is made and when it reaches its end; starting a done one
// begins it from its start, and turning one under way around continues from where it is.

#include "Component.h"

namespace engine
{

enum TransitionDirection
{
	TRANSITION_IN = 0,   // the progress rises to 1
	TRANSITION_OUT = 1,  // the progress falls to 0
};

class Transition : public Object, public Modifier
{
public:
	explicit Transition(float theDuration) : mDuration(theDuration) {}

	// 0x00 (0x482d30 (feedingfrenzy2)): the time passed taken from what is left, the progress applied
	void modify(UpdateContext& theContext, Component* theComponent) override;
	// 0x04 (0x485300 (feedingfrenzy2)): the component the progress applies to (not counted)
	virtual void setTarget(Component* theTarget) { mTarget = theTarget; }
	// 0x08 (0x482de0 (feedingfrenzy2)): the progress set, 0 to 1, and applied; a transition under way that it puts at
	// its end is done
	virtual void setProgress(float theProgress);
	// 0x0c (0x40a6a0 (feedingfrenzy2)): begun in the direction
	virtual void start(int theDirection);
	// 0x10 (0x4143e0 (feedingfrenzy2))
	virtual bool isDone() { return mDone; }
	// 0x14: the progress shown on the component
	virtual void apply() = 0;

protected:
	// Whether the progress is at the end of the direction
	bool isAtEnd() const;

	Component* mTarget = nullptr;  // +0x14
	int mDirection = TRANSITION_IN;  // +0x18: TransitionDirection
	// +0x1c and +0x24: not set by the constructor; every path that reads them sets them first. The port starts them
	// at 0
	float mProgress = 0.0f;        // +0x1c
	float mDuration;               // +0x20: in seconds
	float mRemaining = 0.0f;       // +0x24: the seconds left
	bool mDone = true;             // +0x28
};

class FadeTransition : public Transition
{
public:
	explicit FadeTransition(float theDuration) : Transition(theDuration) {}

	// 0x14 (0x482db0 (feedingfrenzy2)): the progress as the component's opacity; at 0 the component is switched off
	// (STATE_VISIBLE among its off states), above it switched on
	void apply() override;
};

} // namespace engine

#endif
