#ifndef __PIZZAFRENZY_PIZZA_H__
#define __PIZZAFRENZY_PIZZA_H__

// A pizza of the pizza manifest (res\manifests\pizzas.xml, read by PizzaManifest.h), which the application keeps by
// the number the manifest gives it (PizzaFrenzy::getPizza, 0x409990) and a bonus round lists by that number. The
// pizza is a name and the toppings on it, each with the place it lies at on the pizza.

#include "Geometry.h"
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
