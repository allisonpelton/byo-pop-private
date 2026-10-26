#ifndef __WORDHARMONY_ENGINE_SCREENLAYOUT_H__
#define __WORDHARMONY_ENGINE_SCREENLAYOUT_H__

// A screen built from a layout file (0xd4 bytes, constructor 0x428640, vtable 0x483500; the game's screens derive from
// it). It keeps its named nodes, listens to its buttons under their actions and passes a pressed button's action on
// to its own listener (the game's controller).

#include "Button.h"
#include "FadeContainer.h"
#include <map>
#include <string>

namespace engine
{

// What a screen tells when one of its buttons is pressed (the controller's listener at +0x0c, vtable 0x480a10)
class ScreenListener
{
public:
	virtual void screenAction(std::string theAction) = 0;  // slot 0x00
};

class ScreenLayout : public FadeContainer, public ButtonListener
{
public:
	ScreenLayout();  // 0x428640

	// 0xe4: 0x427cd0, adds the elements of a layout file of the archive
	virtual bool loadLayout(std::string theFile);
	// 0xe8: 0x4285e0, a node by its name (the last one of a name wins; not counted)
	virtual void registerNode(Node* theNode, std::string theName);
	// 0xec: 0x427df0, listens to a button under an action (theName is not used)
	virtual void listenTo(Button* theButton, std::string theName, std::string theAction);

	// 0x427f10
	Node* getNode(std::string theName);
	// 0x427cc0
	void setListener(ScreenListener* theListener) { mListener = theListener; }

	// The button listener (+0xc0, vtable 0x4834f4): only a press goes on
	void buttonEnter(std::string theName) override {}  // 0x401c90
	void buttonPress(std::string theName) override;    // 0x427e90
	void buttonExit(std::string theName) override {}   // 0x401c90

	std::map<std::string, Node*> mNodes;  // +0xc4
	ScreenListener* mListener;            // +0xd0
};

} // namespace engine

#endif
