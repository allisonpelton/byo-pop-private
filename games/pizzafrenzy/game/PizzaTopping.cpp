#include "PizzaTopping.h"
#include "Selector.h"

using namespace pizzafrenzy;

namespace
{
const int PIVOT_CENTRE = 1;
const char* const CLICK_SOUND = "res\\sounds\\menu_clicked.ogg";
const int IMAGE_UP = 0;
const int IMAGE_OVER = 1;
}

// 0x419a30
void PizzaTopping::setTopping(Topping* theTopping)
{
	setPivotMode(PIVOT_CENTRE);
	setImages(theTopping->mImage, nullptr, nullptr);
	setSounds("", CLICK_SOUND);
	mTopping = theTopping;
	setSmoothType(0);
}

// 0x419830
void PizzaTopping::setMarked(bool theMarked)
{
	mSelector->select(theMarked ? IMAGE_OVER : IMAGE_UP, 0, true);
	mMarked = theMarked;
}
