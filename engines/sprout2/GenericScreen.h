#ifndef __SPROUT2_GENERICSCREEN_H__
#define __SPROUT2_GENERICSCREEN_H__

// A screen that comes and goes through transitions (GenericScreen, 0x1a8 bytes, vtable 0x55c4bc (feedingfrenzy2) with
// 96 slots; its constructor 0x4848d0 (feedingfrenzy2)): a layout read from a screen's XML, the listener of its own
// actions, which it passes on through a signal. Shown, it waits its delay, then runs its in transition and is enabled;
// hidden, it is disabled, waits its delay, runs its out transition and ends hidden. A screen with no transition is made
// visible or invisible at once.

#include "ScreenLayout.h"
#include "Transition.h"

namespace engine
{

// Where a screen is in its coming and going (+0x188)
enum ScreenPhase
{
	SCREEN_NEW = 0,              // as made
	SCREEN_HIDDEN = 1,           // its out transition done
	SCREEN_WAITING_TO_SHOW = 2,  // the delay before the in transition
	SCREEN_SHOWING = 3,          // the in transition under way
	SCREEN_SHOWN = 4,            // shown and enabled
	SCREEN_WAITING_TO_HIDE = 5,  // the delay before the out transition
	SCREEN_HIDING = 6,           // the out transition under way
};

class GenericScreen : public ScreenLayout, public ILayoutActionListener
{
public:
	GenericScreen();

	// 0xa4 (0x435de0 (feedingfrenzy2)): the phase moved on, then the layout's update
	void update(UpdateContext& theContext) override;

	// 0x158 (0x4849f0 (feedingfrenzy2)): the transitions in and out, each pointed at the screen (any before taken
	// out of the modifiers), then the screen reset
	virtual void setTransitions(Transition* theIn, Transition* theOut);
	// 0x15c (0x4098a0 (feedingfrenzy2)): the seconds a show or a hide waits before its transition
	virtual void setDelay(float theDelay) { mDelay = theDelay; }
	// 0x160 (0x4844d0 (feedingfrenzy2)): as before a show: the out transition taken out of the modifiers, the in
	// transition put in at its start; disabled
	virtual void reset();
	// 0x164 (0x40ec60 (feedingfrenzy2)): the coming begun
	virtual void show();
	// 0x168 (0x4845e0 (feedingfrenzy2)): the going begun, unless the screen is hidden; disabled
	virtual void hide();
	// 0x16c (0x417d40 (feedingfrenzy2)): neither waiting nor in a transition
	virtual bool isSettled();
	// 0x170 (0x47db20 (feedingfrenzy2)): a ScreenPhase
	virtual int getPhase() { return mPhase; }
	// 0x174 (0x490170) and 0x178 (0x509329) (feedingfrenzy2): the seconds of the transition in and out, 0 for none
	virtual float getInDuration() { return mInTransition == nullptr ? 0.0f : mInTransition->getDuration(); }
	virtual float getOutDuration() { return mOutTransition == nullptr ? 0.0f : mOutTransition->getDuration(); }
	// 0x17c (0x501a78 (feedingfrenzy2)): the screen marked for deletion when it ends hidden
	virtual void setDeleteWhenHidden(bool theDeleteWhenHidden) { mDeleteWhenHidden = theDeleteWhenHidden; }

	// ILayoutActionListener's 0x00 (0x40b4a0 (feedingfrenzy2)): the action emitted
	void layoutAction(std::string theAction) override { mActionSignal.emit(theAction); }

	sigslot::signal1<const std::string&> mActionSignal;  // +0x178: the layout's actions

private:
	int mPhase;                       // +0x188: ScreenPhase
	Ref<Transition> mOutTransition;   // +0x18c
	Ref<Transition> mInTransition;    // +0x190
	bool mDeleteWhenHidden;           // +0x194
	float mDelay;                     // +0x198
	float mCountdown;                 // +0x19c: the delay's seconds left
};

} // namespace engine

#endif
