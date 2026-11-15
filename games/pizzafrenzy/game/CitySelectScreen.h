#ifndef __PIZZAFRENZY_CITYSELECTSCREEN_H__
#define __PIZZAFRENZY_CITYSELECTSCREEN_H__

// The city select (0x218 bytes, constructor 0x434210, vtable 0x4fdd88), which a game in a mode the player has played
// before starts from: a postcard of each city, four to a row, with Lisa in her speech bubble beside them. The cities
// the player has not reached have a lock over their postcards and cannot be chosen; the one the mouse is over is
// taken out of the row and drawn over the locks, so it grows over its neighbours. A press sends the action
// "citySelect<number>", which the menu state turns into a game (MenuState.cpp).

#include "Level.h"
#include "GenericScreen.h"
#include "Button.h"
#include "ImageNode.h"
#include "TextTyper.h"

#include <vector>

namespace pizzafrenzy
{

class CitySelectScreen : public engine::GenericScreen
{
public:
	CitySelectScreen() {}  // 0x434210

	// 0x17c: 0x434300: Lisa and her text, and the postcards of the cities
	void setup(std::vector<engine::Ref<City>>& theCities);
	// 0x180: 0x433480: the locks over the cities the player has not reached
	void refresh();
	// 0x184: 0x434790: a postcard and a button for each city, and the one with a question mark for a city that is not
	// there
	void setCities(std::vector<engine::Ref<City>>& theCities);
	// 0x88: 0x433c30: Lisa says what the screen is for
	void connect() override;

	// The button listener (+0x144, vtable 0x4fdd70): the postcard the mouse is over comes to the front
	void buttonEnter(std::string theName) override;  // 0x4332d0
	void buttonExit(std::string theName) override;   // 0x4333a0

private:
	// 0x433690: the postcard of a city (a card with the picture of its first level's city, drawn as the game screen
	// draws it, and the city's name; with no city, a blank picture and three question marks)
	engine::Image* makePostcard(City* theCity);

	engine::Ref<engine::Container> mPostcards;           // +0x1d4: the buttons, in the node "postcardGroup"
	std::vector<engine::Ref<engine::Image>> mPostcardImages;  // +0x1d8
	std::vector<engine::Ref<engine::ImageButton>> mButtons;   // +0x1e8: one for each postcard image
	engine::Ref<engine::Node> mLisa;                     // +0x1f8: the node "Lisa"
	engine::Ref<engine::Node> mLisaBubble;               // +0x1fc: the node "LisaBubble"
	engine::Ref<engine::TextTyper> mLisaTyper;           // +0x200
	engine::Ref<engine::Image> mUnknownPostcard;         // +0x204: the postcard with the question marks
	engine::Ref<engine::ImageButton> mHoveredButton;     // +0x208: the button the mouse is over, which is taken out of
	                                                     // the row
	engine::Ref<engine::Container> mLocks;               // +0x20c: the locks, and the button the mouse is over
};

} // namespace pizzafrenzy

#endif
