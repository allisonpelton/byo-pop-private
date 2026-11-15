#ifndef __PIZZAFRENZY_LEVELSTARTSCREEN_H__
#define __PIZZAFRENZY_LEVELSTARTSCREEN_H__

// The level's start (0x274 bytes, constructor 0x448860, vtable 0x4fe9a0; the application keeps it at +0xe4, from
// res/screenLayouts/toppingSelectionScreen.xml): the player picks the topping each of the level's vans carries. Its
// two pages hold a button for each topping the player has, twelve to a page, and under them is a button for each van.
// A click on a topping picks it up (it follows the mouse until the right button puts it back), and a click on a van
// puts it there. It has the book's two arrows but is a screen of its own: the book (ToppingBookScreen.h) shares no
// class or code with it. A topping button shows its name in a caption while the mouse is over it.

#include "GenericScreen.h"
#include "ImageNode.h"
#include "TextItem.h"
#include "ToppingButton.h"
#include "Level.h"

#include <map>
#include <vector>

namespace pizzafrenzy
{

class LevelStartScreen : public engine::GenericScreen
{
public:
	LevelStartScreen() : mLevel(nullptr), mPage(0) {}  // 0x448860

	// 0x17c: 0x446f30: the layout's nodes, the picture that follows the mouse, and the connections to the mouse and to
	// the screen's own actions
	void setup();
	// 0x180: 0x447c60: the screen shows the level: a van button for each of its vans, with the toppings the player
	// picked for the last level, and the first pages
	void setLevel(Level* theLevel);
	// 0x184: 0x445ed0, from the application's mouse move: the held topping's picture follows the mouse
	void onMouseMove(engine::Point& thePoint);
	// 0x188: 0x445db0, from the application's right button: the held topping is put back
	void onRightDown(engine::Point& thePoint) override;
	// 0x18c: 0x4464d0: the topping buttons are made again for the pages at the book's place
	void showPages();
	// 0x190: 0x447990 (the action's slot): the arrows turn two pages; a topping's button picks it up; a van's button
	// puts the held topping in it; "selectionDone" and the menu action go on to the application
	void onAction(std::string theAction);
	// 0x194: 0x446210: the side (0 left, 1 right) shows the page: its number and its twelve toppings
	void showPage(int theSide, int thePage);
	// 0x198: 0x446010: the topping is held, and the van buttons take clicks
	void pickUp(Topping* theTopping);
	// 0x19c: 0x4460b0: the held topping is put back, and the van buttons take no clicks
	void putBack();
	// 0x1a0: 0x446130: the done button can be clicked when every van has a topping
	void updateDoneButton();
	// 0x1a4: 0x448300: the levels' vans carry the toppings picked, and they are the player's picks for the next level
	void selectionDone();
	// 0x1a8: 0x4484f0: the topping's button takes clicks, or is darkened and takes none
	void setToppingAvailable(Topping* theTopping, bool theAvailable);
	// 0x1ac: 0x448530: the button of a topping, with the topping's level out of 4 as its caption
	ToppingButton* createToppingButton(Topping* theTopping, int theLevel);

	// The button listener's slots (0x4fe988; the screen's second base): the caption shows the topping under the mouse
	// 0x448660, and goes when the mouse leaves 0x4461b0
	void buttonEnter(std::string theName) override;
	void buttonExit(std::string theName) override;

	// The test bot's reads (it changes nothing): whether a topping is picked up, the first van with no topping (-1
	// when every van has one), and the first topping button on the pages that no van has and that is not picked up
	bool isHolding() { return mHeld != nullptr; }
	int getFirstEmptyVan();
	engine::Node* findFreeToppingButton();

private:
	engine::Ref<engine::Node> mPages[2];                    // +0x1d4: "leftPage", "rightPage"
	engine::Ref<engine::Container> mButtonGroups[2];        // +0x1dc: where the topping buttons of a page go
	engine::Ref<engine::TextItem> mPageNumbers[2];          // +0x1e4: "page %d/%d"
	engine::Ref<engine::Node> mTurnButtons[2];              // +0x1ec: the arrows
	engine::Ref<engine::Node> mDoneButton;                  // +0x1f4: "doneButton"
	engine::Ref<engine::Container> mVanButtons;             // +0x1f8: "vanButtons", where the van buttons go
	std::vector<engine::Ref<engine::ImageButton>> mVanButtonList;  // +0x1fc
	std::vector<engine::Ref<engine::ImageNode>> mVanPictures;      // +0x20c: the picture of each van's topping
	std::vector<engine::Ref<Topping>> mChosen;              // +0x21c: the topping of each van, null for none
	ToppingMap<engine::Ref<ToppingButton>> mToppingButtons;  // +0x22c: the buttons of the pages shown
	engine::Ref<Topping> mHeld;                             // +0x238: the topping picked up
	engine::Ref<engine::ImageNode> mHeldPicture;            // +0x23c: its picture, which follows the mouse
	Level* mLevel;                                          // +0x240
	engine::Ref<engine::TextItem> mCaptionText;             // +0x244: "captionText"
	engine::Ref<engine::FadeContainer> mCaption;            // +0x248: "caption"
	std::string mPageTemplate;                              // +0x24c: a page number's text as the layout has it
	int mPage;                                              // +0x268: the page on the left
};

} // namespace pizzafrenzy

#endif
