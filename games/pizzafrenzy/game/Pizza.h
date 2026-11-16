#ifndef __PIZZAFRENZY_PIZZA_H__
#define __PIZZAFRENZY_PIZZA_H__

// A pizza of the pizza manifest (res\manifests\pizzas.xml, read by PizzaManifest.h), which the application keeps by
// the number the manifest gives it (PizzaFrenzy::getPizza, 0x409990) and a bonus round lists by that number. The
// pizza is a name and the toppings on it, each with the place it lies at on the pizza.

#include "Geometry.h"
#include "Graphics.h"
#include "Object.h"
#include "TileEntries.h"

#include <string>
#include <vector>

namespace pizzafrenzy
{

// A topping of a pizza and its place on it (12 bytes)
struct PizzaEntry
{
	engine::Ref<Topping> mTopping;  // +0x00
	engine::Point mPlace;           // +0x04: whole numbers
};

class Pizza : public engine::Object
{
public:
	Pizza() : mId(0) {}  // 0x4194f0

	// 0x418db0: the picture of the pizza: the decorating pizza's image with each topping drawn at its place (a little
	// larger than its own), scaled to a width of 50 pixels
	engine::Image* makeImage();
	// 0x418f40: the pizza has no toppings
	void clearToppings() { mToppings.clear(); }
	// 0x419020: the pizza's element of the player's pizzas file, "pz", with an element "t" for each topping
	std::string toXml() const;

	// 0x419590: a topping at the place, after the ones there
	void addTopping(Topping* theTopping, const engine::Point& thePlace)
	{
		PizzaEntry anEntry;
		anEntry.mTopping = theTopping;
		anEntry.mPlace = thePlace;
		mToppings.push_back(anEntry);
	}

	std::string mName;                  // +0x0c
	int mId;                            // +0x28: the number the manifest gives it
	std::vector<PizzaEntry> mToppings;  // +0x30
};

} // namespace pizzafrenzy

#endif
