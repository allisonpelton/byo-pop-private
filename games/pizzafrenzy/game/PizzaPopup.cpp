#include "PizzaPopup.h"
#include "Tiles.h"

using namespace pizzafrenzy;

// 0x451ab0: the base's 0x455000, then the button's picture, the price's holder and its number (translated next)
void PizzaKitchenPopup::setOwner(BuildingTile* theTile)
{
	PizzaPopup::setOwner(theTile);
}

// 0x4519b0
void PizzaKitchenPopup::setTopping(Topping* theTopping)
{
	mTopping = theTopping;
	// the topping's picture and the price of its upgrade (0x451920) are shown here: translated next
}
