#ifndef __SPROUT_SELECTOR_H__
#define __SPROUT_SELECTOR_H__

// A container that shows one of its children (0xbc bytes, constructor 0x426f10 (wordharmony), vtable
// 0x4832d8 (wordharmony)): an image button's up, over and down images. A change can cross-fade: the next child fades in
// over the current one, or the current one fades out before the next one shows.

#include "Node.h"

namespace engine
{

class Selector : public Container
{
public:
	Selector();  // 0x426f10 (wordharmony)

	void draw(Graphics* g) override;            // 0x6c: 0x426c90 (wordharmony)
	void update(UpdateInfo& theInfo) override;  // 0x70: 0x426de0 (wordharmony)
	void layout() override;                     // 0x74: 0x426b70 (wordharmony)
	// 0xd8: 0x426b00 (wordharmony)
	virtual void select(int theIndex, float theDuration, bool theFadeIn);
	// 0xe0: 0x426d90 (wordharmony)
	virtual Node* getSelected();
	// 0xe4: 0x426c00 (wordharmony)
	virtual int getCount() { return (int)mChildren.size(); }

	int mCurrent;    // +0xa8
	int mNext;       // +0xac
	float mTime;     // +0xb0
	float mDuration; // +0xb4
	bool mFadeIn;    // +0xb8
};

} // namespace engine

#endif
