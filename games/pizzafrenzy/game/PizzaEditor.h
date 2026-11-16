#ifndef __PIZZAFRENZY_PIZZAEDITOR_H__
#define __PIZZAFRENZY_PIZZAEDITOR_H__

// The pizza editor's work (0x60 bytes, constructor 0x40f300, vtable 0x4fa524; the application makes it once and keeps
// it at +0xfc): the player's pizzas are shown one at a time on the editor's screen (userPizzaEditor.xml, the
// application's +0x100) with a button for each topping the player has. A click on a button picks its topping, which
// follows the mouse as a cursor; a click on the pizza puts it there and a click of the right button takes the topping
// under it back. The screen's buttons go to the next, the previous and a new pizza, delete the pizza, name it (its
// name screen is pizzaNameScreen.xml, the application's +0x104) and leave, which writes the player's pizzas file.
// The place of a topping is kept as the whole pixels it has from the middle of the pizza.
//
// The original has one more field (+0x54, a flag that nothing reads), and the designer mode flag (+0x55) is always
// set: the editor works on the player's pizzas only.

#include "GenericScreen.h"
#include "ImageNode.h"
#include "Node.h"
#include "Object.h"
#include "Pizza.h"
#include "PizzaTopping.h"
#include "Signal.h"
#include "TextItem.h"
#include "TileEntries.h"

#include <string>
#include <vector>

namespace pizzafrenzy
{

class PizzaEditor : public engine::Object, public engine::HasSlots
{
public:
	// 0x40f300: the editor's screen, the name screen and the application's list of the player's pizzas, whose
	// last pizza is the first one shown
	PizzaEditor(engine::GenericScreen* theScreen, engine::GenericScreen* theNameScreen,
	            std::vector<engine::Ref<Pizza>>* thePizzas, bool theDesigned);
	~PizzaEditor() override;  // 0x40f4b0, 0x40ef80

	// 0x04: 0x40e560: the screen's groups and caption, and the cursor, which is on the screen with no picture
	void setUp();
	// 0x0c: 0x40f9d0: a button for each topping the player has, and the pizza that is shown (a new one when there is
	// none)
	void show();
	// 0x08: 0x40fc20: the actions of the editor's screen and of the name screen
	void action(std::string theAction);

	// port (the test commands' state): what the editor shows
	Pizza* getPizza() { return mPizza; }
	Topping* getSelectedTopping() { return mSelectedTopping; }
	int getPizzaIndex() const { return mPizzaIndex; }
	size_t getPizzaCount() const { return mPizzas->size(); }
	engine::Container* getToppingButtons() { return mToppingButtons; }
	engine::Container* getPizzaToppingGroup() { return mPizzaToppingGroup; }
	const std::vector<engine::Ref<PizzaTopping>>& getPlacedToppings() const { return mPlaced; }
	engine::GenericScreen* getScreen() { return mScreen; }

private:
	// 0x40f5b0: the pizza of the list's number is shown with its caption and its toppings, each at its place
	void showPizza();
	// 0x40f4d0: a topping of the kind is put on the pizza at the place
	void placeTopping(const engine::Point& thePlace, Topping* theTopping);
	// 0x40e310: the pizza has the toppings that are on it, and the application keeps it (its picture and topping are
	// made again)
	void savePizza();
	// 0x40e870: the caption shows the pizza's number in the list and its name
	void showCaption();
	// 0x40f6b0: a new pizza is added to the list and shown
	void addPizza();
	// 0x40f670, 0x40f800: the next and the previous pizza, which go round
	void showNextPizza();
	void showPreviousPizza();
	// 0x40fb20: the pizza is let go of the list, and the one at its place is shown
	void deletePizza();
	// 0x40ebe0, 0x40e990: the list is written to the player's pizzas file
	void saveFile();
	// 0x40f8b0: the topping is picked (none lets go of the pick); the mouse's signals are the editor's while one is
	void selectTopping(Topping* theTopping);
	// 0x40e170: the cursor follows the mouse, and is faint while the mouse is far from the middle of the pizza
	void onMouseMove(engine::Point& thePoint);
	// 0x40f840: a click near the middle of the pizza puts the picked topping there
	void onMouseDown(engine::Point& thePoint);
	// 0x40e940, 0x40e7b0: a click of the right button takes back the topping that is within 15 pixels
	void onRightDown(engine::Point& thePoint);

	std::vector<engine::Ref<Pizza>>* mPizzas;                  // +0x1c: the application's player's pizzas
	engine::Ref<Pizza> mPizza;                                 // +0x20: the pizza that is shown
	engine::Ref<Topping> mSelectedTopping;                     // +0x24
	engine::Ref<engine::GenericScreen> mScreen;                // +0x28
	engine::Ref<engine::GenericScreen> mNameScreen;            // +0x2c
	engine::Ref<engine::Container> mToppingButtons;            // +0x30: "toppingButtons"
	engine::Ref<engine::Container> mPizzaToppingGroup;         // +0x34: "targetToppings", on the pizza's picture
	engine::Ref<engine::TextItem> mCaption;                    // +0x38: "caption"
	std::vector<engine::Ref<PizzaTopping>> mPlaced;            // +0x3c: the toppings that are on the pizza
	engine::Ref<engine::ImageNode> mCursor;                    // +0x4c
	int mPizzaIndex;                                           // +0x50: the shown pizza's place, -1 for none
	bool mDesigned;                                            // +0x55: the player's pizzas
};

} // namespace pizzafrenzy

#endif
