#ifndef __FEEDINGFRENZY2_SCREENMGR_H__
#define __FEEDINGFRENZY2_SCREENMGR_H__

// The game's screens, one at a time (ScreenMgr, 0xe0 bytes, vtable 0x55d6f4 (feedingfrenzy2); its constructor 0x48de20
// (feedingfrenzy2)): a container holding the current screen, with a queue of the screens to show after it. The next
// screen in the queue is added and shown while the current one hides; when both have settled, the current one is
// taken out and the next takes its place.

#include "Container.h"
#include "GenericScreen.h"

#include <deque>

class ScreenMgr : public engine::Container
{
public:
	ScreenMgr();

	// 0x68 (0x52043d (feedingfrenzy2))
	std::string getClassName() override { return "ScreenMgr"; }
	// 0x90 (0x500fab) and 0x94 (0x497c20) (feedingfrenzy2): the mouse taken and given up; the screens are not
	// enabled or disabled
	void enable() override { removeOffState(engine::STATE_INPUT); }
	void disable() override { addOffState(engine::STATE_INPUT); }
	// 0xa4 (0x4375b0 (feedingfrenzy2)): the next screen begun, or the change under way finished, then the children
	// updated
	void update(engine::UpdateContext& theContext) override;

	// 0x4098b0 (feedingfrenzy2): the screen given its delay (when there is one) and queued
	void showScreen(engine::GenericScreen* theScreen, float theDelay);
	// 0x407da0 (feedingfrenzy2): a change under way or a screen queued
	bool isBusy() const { return mChanging || !mQueue.empty(); }
	engine::GenericScreen* getCurrent() { return mCurrent.get(); }

private:
	// 0x418c10 (feedingfrenzy2): once both screens have settled, the old one taken out and the new one current
	void finishChange();

	engine::Ref<engine::GenericScreen> mCurrent;               // +0xb8
	engine::Ref<engine::GenericScreen> mNext;                  // +0xbc
	std::deque<engine::Ref<engine::GenericScreen>> mQueue;     // +0xc0
	bool mChanging;                                            // +0xd4
};

#endif
