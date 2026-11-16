#ifndef __PIZZAFRENZY_DIALOGSTATE_H__
#define __PIZZAFRENZY_DIALOGSTATE_H__

// The state that shows a dialog (0x2c bytes, constructor 0x402420, vtable 0x4f94c8): a screen that comes over the
// state before it, in a screen manager of its own (the global at 0x532904, shared by every dialog) with the screens
// beneath dimmed. Its actions are the dialogs' buttons: each shows another dialog or closes this one, which goes on
// to the state it was made with.

#include "PizzaFrenzy.h"
#include "ScreenMgr.h"
#include "State.h"

namespace pizzafrenzy
{

class DialogState : public engine::State
{
public:
	// 0x402420: the screen to show first, and the state that follows
	DialogState(PizzaFrenzy* theApp, engine::GenericScreen* theScreen, engine::State* theNextState);

	// 0x04: 0x406170: the screens beneath dim, the dialogs' screen manager comes over them and shows the screen
	void enter() override;
	// 0x08: 0x402670: the screens beneath are as they were
	void exit() override;
	// 0x0c: 0x40cf30: once the dialog has closed, the screens beneath wake and the next state starts
	void update(engine::UpdateInfo& theInfo) override;
	// 0x10: 0x40d3e0
	void action(const std::string& theAction) override;

	// 0x14: 0x406310: the dialog goes
	void close();
	// (slot 0x18, 0x4026a0, which empties the dialogs' screen manager, takes the manager out of the scene and wakes the
	// screens beneath, is not called by anything translated yet)
	// 0x1c: 0x406270: another screen takes the dialog's place
	void show(engine::GenericScreen* theScreen);
	// port (the test commands' state): the screen the dialogs' screen manager shows or is about to show, none when it
	// is closing, and the state that follows the dialog
	engine::GenericScreen* getShownScreen() const;
	engine::State* getNextState() const { return mNextState; }

private:
	// The dialogs' screen manager (0x532904)
	static engine::Ref<engine::ScreenMgr> sDialogScreens;

	PizzaFrenzy* mApp;                     // +0x0c
	engine::Ref<engine::GenericScreen> mScreen;  // +0x10: the screen to show first
	engine::Ref<engine::State> mNextState;       // +0x14
	bool mClosing;                         // +0x18: the screen is leaving
	bool mResume;                          // +0x19: the game that was left goes on when the dialog is gone
	int mCity;                             // +0x1c: the city the game starts in
	bool mFromPause;                       // +0x20: the dialog was opened from the pause menu, which it goes back to
};

} // namespace pizzafrenzy

#endif
