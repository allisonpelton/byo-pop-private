// The bonus basket: a basket that moves across the plate layer in the Trick Shot mode's levels. A jewel that falls into its
// mouth stays in it, scoring, until the basket's stage ends, when the basket shoots every jewel it holds back out.

#ifndef VENICE_BONUSBASKET_H
#define VENICE_BONUSBASKET_H

#include "SceneObject.h"

// 0x148 bytes in the original
class BonusBasket : public SceneObject
{
public:
	bool Update() override; // 0x401000: a falling jewel in the basket's mouth is caught
	void OnCustomEvent() override; // 0x401430: the jewels in the basket are shot out of it (the stage that ends sends this)
};

#endif
