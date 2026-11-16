#include "PizzaTopping.h"
#include "App.h"
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

// 0x4197c0
void PizzaTopping::mouseDownEvent(engine::MouseEvent& e)
{
	engine::ImageButton::mouseDownEvent(e);
	engine::Point aMouse = engine::App::instance()->getMousePosition();
	mDragOffset = *getPosition();
	mDragOffset.subtract(aMouse);
}

// 0x419760
void PizzaTopping::mouseMoveEvent(engine::MouseEvent& e)
{
	if (mState == STATE_DOWN || mState == STATE_OUT_DOWN)
	{
		engine::Point aPlace = engine::App::instance()->getMousePosition();
		aPlace.add(mDragOffset);
		setPosition(aPlace);
	}
}
