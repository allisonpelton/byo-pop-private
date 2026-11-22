#ifndef __SPROUT2_SELECTOR_H__
#define __SPROUT2_SELECTOR_H__

// A container that shows one of its children (Selector, vtable 0x55cc84 (feedingfrenzy2) with 75 slots, 0xd4 bytes;
// its constructor 0x475240 (feedingfrenzy2)): an image button's looks, one for each state. A change of child may
// cross-fade over a time, either the old child fading out over the new or the new fading in over the old; the bounds
// are the shown child's.

#include "Container.h"

namespace engine
{

class Selector : public Container
{
public:
	// 0x475240 (feedingfrenzy2): visible and animated, nothing shown
	Selector();

	// 0x68 (0x520730 (feedingfrenzy2))
	std::string getClassName() override { return "Selector"; }
	// 0xa0 (0x44cb50 (feedingfrenzy2)): the shown child, then the one fading, each whatever its own states
	void paint(DrawContext* theContext) override;
	// 0xa4 (0x439280 (feedingfrenzy2)): the children, then the fade
	void update(UpdateContext& theContext) override;
	// 0xa8 (0x439130 (feedingfrenzy2)): the shown child's bounds and the origin, scaled by the scale's size and moved by
	// the position (without a child shown, Container's bounds, scaled and moved again)
	void computeBounds() override;

	// 0x118 (0x42c0d0 (feedingfrenzy2)): a child shown, at once or over the seconds; theFadeIn fades the new child in
	// over the old, else the old fades out over it
	virtual void select(int theIndex, float theFadeTime, bool theFadeIn);
	// (0x11c and 0x124 (0x5010ef), the index shown, and 0x128 (0x5091f1), the count of children (feedingfrenzy2), are
	// not called in feedingfrenzy2)
	// 0x120 (0x4374b0 (feedingfrenzy2)): null when none is shown
	virtual Component* getSelected();

private:
	int mCurrent;       // +0xb8: the child shown, -1 for none
	int mNext;          // +0xbc: the child being faded to, -1 for none
	float mFadeTimer;   // +0xc0: the seconds left of the fade
	float mFadeTime;    // +0xc4: 0 when no fade is under way
	bool mFadeIn;       // +0xc8
};

} // namespace engine

#endif
