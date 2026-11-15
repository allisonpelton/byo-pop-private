#ifndef __SPROUT_GENERICSCREEN_H__
#define __SPROUT_GENERICSCREEN_H__

// The signal build's screen (Pizza Frenzy's; 0x1dc bytes, constructor 0x464540 (pizzafrenzy), vtable
// 0x5013b8 (pizzafrenzy); Iggle Pop's Mac build names it GenericScreen, a ScreenLayout and an ILayoutActionListener).
// It is its own layout's listener and sends each action on through its action signal, which the game's application
// connects to. The screen manager (ScreenMgr.h) shows and hides it: shown, it waits its delay, runs its in transition
// and then connects (takes the keys and the mouse); hidden, it disconnects, waits its delay and runs its out
// transition.

#include "ScreenLayout.h"
#include "Signal.h"
#include "Transition.h"

namespace engine
{

class GenericScreen : public ScreenLayout, public ScreenListener
{
public:
	GenericScreen();            // 0x464540 (pizzafrenzy)
	~GenericScreen() override;  // 0x464410 (pizzafrenzy)

	// 0x94: 0x464200 (pizzafrenzy), the showing and hiding, then the fade container's update
	void update(UpdateInfo& theInfo) override;
	// 0x164: 0x464080 (pizzafrenzy), the transitions in and out (counted; the same one may be both), each attached
	// to the screen, and the screen made ready to show
	virtual void setTransitions(Transition* theIn, Transition* theOut);
	// 0x168: 0x464010 (pizzafrenzy), the seconds a show or hide waits before its transition
	virtual void setShowDelay(float theDelay) { mShowDelay = theDelay; }
	// 0x16c: 0x464150 (pizzafrenzy): the in transition back at its start, and the screen disconnected
	virtual void resetTransitions();
	// 0x170: 0x464020 (pizzafrenzy)
	virtual void show();
	// 0x174: 0x464190 (pizzafrenzy): a hidden screen stays as it is
	virtual void hide();
	// 0x178: 0x464050 (pizzafrenzy), neither showing nor hiding
	virtual bool isIdle();

	// The layout's listener (+0x1ac, vtable 0x5013b0 (pizzafrenzy)), slot 0: 0x464330 (pizzafrenzy)
	void screenAction(std::string theAction) override { mActionSignal.emit(theAction); }

	enum
	{
		STATE_NEW = 0,
		STATE_HIDDEN = 1,
		STATE_SHOW_WAIT = 2,
		STATE_SHOWING = 3,
		STATE_SHOWN = 4,
		STATE_HIDE_WAIT = 5,
		STATE_HIDING = 6
	};

	Signal<std::string> mActionSignal;  // +0x1b0
	int mState;                         // +0x1c0
	Ref<Transition> mOutTransition;     // +0x1c4
	Ref<Transition> mInTransition;      // +0x1c8
	float mShowDelay;                   // +0x1cc
	float mDelayLeft;                   // +0x1d0
};

} // namespace engine

#endif
