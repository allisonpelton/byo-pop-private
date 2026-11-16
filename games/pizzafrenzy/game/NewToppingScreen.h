#ifndef __PIZZAFRENZY_NEWTOPPINGSCREEN_H__
#define __PIZZAFRENZY_NEWTOPPINGSCREEN_H__

// The screen that shows the toppings the player has not been shown yet (0x24c bytes, constructor 0x43b5b0, vtable
// 0x4fddf8; the application keeps it at +0xec, from res/screenLayouts/newToppingScreen.xml): a button for each new
// topping on the left (fifteen at most, three to a row) and, on the right, what the topping adds at its first upgrade.
// Choosing one makes it the player's, and the layout's other buttons go on to the application.

#include "GenericScreen.h"
#include "ImageNode.h"
#include "TextItem.h"
#include "ToppingButton.h"

namespace pizzafrenzy
{

class NewToppingScreen : public engine::GenericScreen
{
public:
	NewToppingScreen() {}  // 0x43b5b0

	// 0x17c: 0x43be30: the layout's nodes, and the screen hears its own actions
	void setup();
	// 0x180: 0x43b970 (the action's slot): a topping's button chooses the topping; any other action goes on
	void onAction(std::string theAction);
	// 0x184: 0x43b500: a button for each of the player's new toppings, and no topping shown
	void fillButtons();
	// 0x188: 0x43bad0: the topping's upgrade is shown, or nothing for no topping
	void showTopping(Topping* theTopping, int theUpgrade);
	// 0x18c: 0x43b3e0: the button of a topping, a little smaller than its size
	ToppingButton* createButton(Topping* theTopping);

private:
	engine::Ref<engine::Container> mButtons;       // +0x1d4: "leftButtons"
	engine::Ref<engine::ImageNode> mPictureBg;     // +0x1d8: "rightToppingBg"
	engine::Ref<engine::ImageNode> mPicture;       // +0x1dc: "rightTopping"
	engine::Ref<engine::TextItem> mCaption;        // +0x1e0: "rightToppingCaptionText"
	engine::Ref<engine::TextItem> mValue;          // +0x1e4: "rightToppingValue"
	engine::Ref<engine::TextItem> mLevel;          // +0x1e8: "rightToppingLevel"
	engine::Ref<engine::TextItem> mDescription;    // +0x1ec: "rightToppingDesc"
	std::string mValueTemplate;                    // +0x1f0: the text of the value as the layout has it
	std::string mLevelTemplate;                    // +0x20c: the text of the level as the layout has it
};

} // namespace pizzafrenzy

#endif
