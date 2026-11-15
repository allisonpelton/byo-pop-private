#ifndef __SPROUT_SCREENMGR_H__
#define __SPROUT_SCREENMGR_H__

// The signal build's screen manager (Pizza Frenzy's; 0x150 bytes, constructor 0x46b710 (pizzafrenzy), vtable
// 0x501d18 (pizzafrenzy); Iggle Pop's Mac build names it ScreenMgr): the container the generic screens are shown in,
// one at a time. A screen asked for waits in a queue; when the one before has finished switching, the current screen
// hides and the new one is added and shown, and once both are idle the old one leaves.

#include "GenericScreen.h"
#include <deque>

namespace engine
{

class ScreenMgr : public Container
{
public:
	ScreenMgr();            // 0x46b710 (pizzafrenzy)
	~ScreenMgr() override;  // 0x46b390 (pizzafrenzy)

	// 0x88: 0x47a4c0 (pizzafrenzy), the mouse (not the children's connections)
	void connect() override { setMouseMasked(false); }
	// 0x8c: 0x46af40 (pizzafrenzy)
	void disconnect() override { setMouseMasked(true); }
	// 0x94: 0x46b280 (pizzafrenzy)
	void update(UpdateInfo& theInfo) override;

	// 0x46b7f0 (pizzafrenzy), a screen to show after the ones asked for before, with the delay its show and hide
	// wait (null hides the current one)
	void showScreen(GenericScreen* theScreen, float theDelay);
	// 0x46b090 (pizzafrenzy), a switch is running or asked for
	bool isBusy() { return mSwitching || mQueue.size() != 0; }

	Ref<GenericScreen> mCurrent;              // +0x128
	Ref<GenericScreen> mNext;                 // +0x12c
	std::deque<Ref<GenericScreen> > mQueue;   // +0x130
	bool mSwitching;                          // +0x144

private:
	// 0x46afd0 (pizzafrenzy): once the new screen and the old one are idle, the old one leaves
	void finishSwitch();
};

} // namespace engine

#endif
