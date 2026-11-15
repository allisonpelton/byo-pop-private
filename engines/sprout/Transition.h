#ifndef __SPROUT_TRANSITION_H__
#define __SPROUT_TRANSITION_H__

// The signal build's screen transitions (Pizza Frenzy's; Iggle Pop's Mac build names Transition, a Modifier and an
// Object, and its FadeTransition and SlideTransition). A transition is one of its screen's node listeners while it
// runs: its progress goes from 0 to 1 when it runs in and from 1 to 0 when it runs out, over its seconds, and each
// step applies the progress to the screen.

#include "Interpolator.h"
#include "Node.h"

namespace engine
{

class Transition : public Object, public NodeListener
{
public:
	Transition(float theDuration);  // 0x469000 (pizzafrenzy), 0x34 bytes

	// 0x00: 0x468ee0 (pizzafrenzy), a step of a running transition (the node is its own, mNode)
	void update(UpdateInfo& theInfo, Node* theNode) override;
	// 0x04: 0x490fc0 (pizzafrenzy), the node it applies to (not counted)
	virtual void attach(Node* theNode) { mNode = theNode; }
	// 0x08: 0x468f70 (pizzafrenzy), the progress set at once (clamped to 0 to 1) and applied
	virtual void setProgress(float theProgress);
	// 0x0c: 0x468e90 (pizzafrenzy): a finished transition starts over (its progress from 0); one still running only
	// turns round, from where it is
	virtual void start(int theDirection);
	// 0x10: 0x468ed0 (pizzafrenzy)
	virtual bool isDone() { return mDone; }
	// 0x14: the progress applied to the node
	virtual void apply() = 0;

	enum
	{
		IN = 0,  // towards 1
		OUT = 1  // towards 0
	};

	Node* mNode;       // +0x14
	int mDirection;    // +0x18
	float mProgress;   // +0x1c
	float mDuration;   // +0x20
	float mTimeLeft;   // +0x24
	bool mDone;        // +0x28
};

// A fade: the progress is the screen's opacity, and a screen at 0 is hidden (vtable 0x4f9458 (pizzafrenzy), built
// inline in the game's code, 0x401620 (pizzafrenzy))
class FadeTransition : public Transition
{
public:
	FadeTransition(float theDuration) : Transition(theDuration) {}  // 0x401620 (pizzafrenzy)

	// 0x14: 0x468e50 (pizzafrenzy)
	void apply() override;
};

// A slide (0x94 bytes, constructor 0x475160 (pizzafrenzy), vtable 0x502a28 (pizzafrenzy)): the screen, opaque, moves
// from one point to another, slowing over the last stretch
class SlideTransition : public Transition
{
public:
	SlideTransition(float theDuration);  // 0x475160 (pizzafrenzy)

	// 0x14: 0x475040 (pizzafrenzy)
	void apply() override;
	// 0x18: 0x475070 (pizzafrenzy), the points the progress 0 and 1 put the screen at
	virtual void setPoints(float theFromX, float theFromY, float theToX, float theToY);

	Point mFrom;                         // +0x2c
	Point mTo;                           // +0x34
	PositionInterpolator mInterpolator;  // +0x3c
};

} // namespace engine

#endif
