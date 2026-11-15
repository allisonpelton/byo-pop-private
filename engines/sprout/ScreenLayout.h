#ifndef __SPROUT_SCREENLAYOUT_H__
#define __SPROUT_SCREENLAYOUT_H__

// A screen built from a layout file (0xd4 bytes, constructor 0x428640 (wordharmony), vtable 0x483500 (wordharmony);
// 0x1ac bytes, constructor 0x466380 (pizzafrenzy), vtable 0x501568 (pizzafrenzy); the game's screens derive from it).
// It keeps its named nodes, listens to its buttons under their actions and passes a pressed button's action on to its
// own listener: the game's controller in the listener build, the generic screen itself in the signal build
// (GenericScreen.h), which sends it on through a signal. The signal build's screen also turns keys into actions
// (Enter its default action, Escape its cancel action, others by the layout's key actions) while it is connected,
// that is while it is shown, and takes the mouse only then.

#include "Button.h"
#include "FadeContainer.h"
#include "Signal.h"
#include <map>
#include <string>

namespace engine
{

// What a screen tells when one of its buttons is pressed (the controller's listener at +0x0c, vtable
// 0x480a10 (wordharmony); the generic screen's +0x1ac, vtable 0x5013b0 (pizzafrenzy))
class ScreenListener
{
public:
	virtual void screenAction(std::string theAction) = 0;  // slot 0x00
};

class ScreenLayout : public FadeContainer, public ButtonListener, public HasSlots
{
public:
	ScreenLayout();  // 0x428640 (wordharmony), 0x466380 (pizzafrenzy)

	// 0xe4: 0x427cd0 (wordharmony); 0x13c: 0x4648f0 (pizzafrenzy), adds the elements of a layout file of the archive
	virtual bool loadLayout(std::string theFile);
	// 0xe8: 0x4285e0 (wordharmony); 0x140: 0x466320 (pizzafrenzy), a node by its name (the last one of a name wins;
	// not counted)
	virtual void registerNode(Node* theNode, std::string theName);
	// 0xec: 0x427df0 (wordharmony); 0x144: 0x4649f0 (pizzafrenzy), listens to a button under an action (theName is
	// not used)
	virtual void listenTo(Button* theButton, std::string theName, std::string theAction);

	// The signal build's (Pizza Frenzy's)
	// (0x148, 0x465d60 (pizzafrenzy), connects a button's signal to slot 0x14c; nothing reaches it)
	// 0x14c: 0x464b10 (pizzafrenzy), an action to the listener
	virtual void sendAction(std::string theAction);
	// 0x150: 0x464c30 (pizzafrenzy), the Enter key's action ("" for none)
	virtual void setDefaultAction(std::string theAction) { mDefaultAction = theAction; }
	// 0x154: 0x464c50 (pizzafrenzy), the Escape key's
	virtual void setCancelAction(std::string theAction) { mCancelAction = theAction; }
	// 0x158: 0x464b60 (pizzafrenzy), a key that went down (a virtual key code) while the screen is connected
	virtual void keyAction(int theKey);
	// 0x15c: 0x465d90 (pizzafrenzy): a key that has an action keeps it
	virtual bool addKeyAction(int theKey, std::string theAction);
	// 0x160: 0x465890 (pizzafrenzy)
	virtual void clearKeyActions() { mKeyActions.clear(); }
	// 0x88: 0x465d10 (pizzafrenzy): the keys, the mouse and the children's connections
	void connect() override;
	// 0x8c: 0x465430 (pizzafrenzy)
	void disconnect() override;

	// 0x427f10 (wordharmony), 0x464c00 (pizzafrenzy)
	Node* getNode(std::string theName);
	// 0x427cc0 (wordharmony), 0x47a140 (pizzafrenzy)
	void setListener(ScreenListener* theListener) { mListener = theListener; }

	// The button listener (+0xc0, vtable 0x4834f4 (wordharmony); +0x144, vtable 0x5026ec (pizzafrenzy)): only a
	// press goes on
	void buttonEnter(std::string theName) override {}  // 0x401c90 (wordharmony), 0x496410 (pizzafrenzy)
	void buttonPress(std::string theName) override;    // 0x427e90 (wordharmony), 0x464a90 (pizzafrenzy)
	void buttonExit(std::string theName) override {}   // 0x401c90 (wordharmony), 0x496410 (pizzafrenzy)

	std::map<std::string, Node*> mNodes;    // +0xc4; +0x158 (pizzafrenzy)
	ScreenListener* mListener;              // +0xd0; +0x164 (pizzafrenzy)
	std::map<int, std::string> mKeyActions; // +0x168 (pizzafrenzy)
	std::string mDefaultAction;             // +0x174 (pizzafrenzy)
	std::string mCancelAction;              // +0x190 (pizzafrenzy)

private:
	// 0x4646f0 (pizzafrenzy), from the application's key signal
	void onKeyDown(int theKey) { keyAction(theKey); }
};

} // namespace engine

#endif
