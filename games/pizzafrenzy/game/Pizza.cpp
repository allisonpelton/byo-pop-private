#include "Pizza.h"
#include "App.h"
#include "Host.h"

#include <cstdio>

using namespace pizzafrenzy;

namespace
{
const char* const DECORATE_IMAGE = "res\\pizza\\decoratePizza.jpg";

// The toppings are drawn this much larger than their pictures, and the whole is as wide as the pizza's picture is
// asked to be
const float TOPPING_DRAW_SCALE = 1.2f;
const float PICTURE_WIDTH = 50.0f;
const int SMOOTH_TYPE_UNSNAPPED = 1;

// The player's pizzas file indents each depth by a tab
const char* const PIZZA_INDENT = "\t";
}

// 0x418db0
engine::Image* Pizza::makeImage()
{
	engine::Image* aBase = engine::App::instance()->getImage(DECORATE_IMAGE);
	engine::Ref<engine::Image> aCanvas = aBase->clone();
	engine::Graphics* aGraphics = engine::getHost()->createImageGraphics(aCanvas);
	for (size_t i = 0; i < mToppings.size(); i++)
	{
		PizzaEntry& anEntry = mToppings[i];
		if (aGraphics == nullptr)
			continue;
		aGraphics->state().mScale = engine::Point(TOPPING_DRAW_SCALE, TOPPING_DRAW_SCALE);
		aGraphics->setSmoothType(SMOOTH_TYPE_UNSNAPPED);
		engine::Image* aPicture = anEntry.mTopping->mImage;
		float aX = (float)(aBase->getWidth() / 2 + (int)anEntry.mPlace.x);
		float aY = (float)(aBase->getHeight() / 2 + (int)anEntry.mPlace.y);
		aGraphics->drawImage(aPicture, engine::IntRect(0, 0, aPicture->getWidth(), aPicture->getHeight()), aX, aY);
	}
	delete aGraphics;
	float aScale = PICTURE_WIDTH / (float)aCanvas->getWidth();
	return aCanvas->createScaled(aScale, aScale);
}

// 0x419020, with the writer's layout (0x477800, 0x477520): the attributes follow the name after a space each, an
// element with children ends with its own closing tag, and the rest end with "/>"
std::string Pizza::toXml() const
{
	std::string aText = std::string(PIZZA_INDENT) + "<pz n=\"" + mName + "\" ID=\"" + std::to_string(mId) + "\" ";
	if (mToppings.empty())
		return aText + "/>\n";
	aText += ">\n";
	for (size_t i = 0; i < mToppings.size(); i++)
	{
		char aPlace[64];
		std::snprintf(aPlace, sizeof(aPlace), "%d,%d", (int)mToppings[i].mPlace.x, (int)mToppings[i].mPlace.y);
		aText += std::string(PIZZA_INDENT) + PIZZA_INDENT + "<t n=\"" + mToppings[i].mTopping->mName + "\" p=\"" +
			aPlace + "\" />\n";
	}
	aText += std::string(PIZZA_INDENT) + "</pz>\n";
	return aText;
}
