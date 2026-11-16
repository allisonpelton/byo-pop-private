#ifndef __PIZZAFRENZY_PIZZAMANIFEST_H__
#define __PIZZAFRENZY_PIZZAMANIFEST_H__

// The pizza manifest (res\manifests\pizzas.xml), which the loading state reads before the level manifest, whose
// levels list their pizzas by number: each "pz" element is a Pizza (Pizza.h) with a "t" element for each of its
// toppings, and the application keeps the pizzas by their numbers (PizzaFrenzy::addPizza).
//
// The player's own pizzas file (userPizzas.xml) is read the same way with the designer's flag, from the user folder
// (the original's is beside the exe): the pizzas go to the player's list, and the pizza editor writes it.

#include <string>

namespace pizzafrenzy
{

// 0x4057b0, its first read: reads the file; throws an Exception when it cannot be read
void loadPizzaManifest(const std::string& theFile, bool theDesigned = false);

} // namespace pizzafrenzy

#endif
