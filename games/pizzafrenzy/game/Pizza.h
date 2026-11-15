#ifndef __PIZZAFRENZY_PIZZA_H__
#define __PIZZAFRENZY_PIZZA_H__

// A pizza of the pizza manifest (res\manifests\pizzas.xml), which the application keeps by the number the manifest
// gives it (PizzaFrenzy::getPizza, 0x409990) and a bonus round lists by that number. The pizzas and their manifest are
// translated next; the class is only what the levels' lists hold.

#include "Object.h"

namespace pizzafrenzy
{

class Pizza : public engine::Object
{
public:
	Pizza() : mId(0) {}

	int mId;  // +0x28: the number the manifest gives it
};

} // namespace pizzafrenzy

#endif
