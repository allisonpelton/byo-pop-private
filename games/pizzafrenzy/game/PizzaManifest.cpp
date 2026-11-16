#include "PizzaManifest.h"
#include "App.h"
#include "DocHandler.h"
#include "Exception.h"
#include "Host.h"
#include "Layout.h"
#include "PizzaFrenzy.h"
#include "Stream.h"

using namespace pizzafrenzy;

namespace
{
const int DEFAULT_MAX_ID = 1;
const int NO_ID = -1;
const char* const DEFAULT_PIZZA_NAME = "Untitled";
const char* const DEFAULT_TOPPING_NAME = "Topping";

// The handler of the manifest's elements (vtable 0x4fa804, whose XML handler part is 0x4fa818, constructor 0x417e70):
// it stays on the stack for the whole document, named "pm"
class PizzaDocHandler : public engine::BaseDocHandler
{
public:
	PizzaDocHandler(const std::string& theName, engine::DocHandlerStack* theParent, bool theDesigned) :
		engine::BaseDocHandler(theName, theParent), mMaxId(DEFAULT_MAX_ID), mDesigned(theDesigned)
	{
	}

	// 0x418050
	void startElement(const std::string& theName, const engine::XmlAttributes& theAttributes) override
	{
		PizzaFrenzy* anApp = PizzaFrenzy::instance();
		if (theName == "pm")
		{
			mMaxId = (int)engine::getIntAttribute(theAttributes, "maxID", DEFAULT_MAX_ID);
		}
		else if (theName == "pz")
		{
			std::string aName = engine::getStringAttribute(theAttributes, "n", DEFAULT_PIZZA_NAME);
			int anId = (int)engine::getIntAttribute(theAttributes, "ID", NO_ID);
			mPizza = new Pizza();
			mPizza->mName = aName;
			// a pizza with no number takes the next; one with a number keeps it, and the next number is past it
			if (anId < 0)
				anId = mMaxId++;
			else if (mMaxId < anId)
				mMaxId = anId + 1;
			mPizza->mId = anId;
		}
		else if (theName == "t")
		{
			std::string aName = engine::getStringAttribute(theAttributes, "n", DEFAULT_TOPPING_NAME);
			engine::IntPair aDefault = {0, 0};
			engine::IntPair aPlace = engine::getIntPointAttribute(theAttributes, "p", aDefault);
			mPizza->addTopping(anApp->mTileManifest->getTopping(aName),
			                   engine::Point((float)aPlace.mFirst, (float)aPlace.mSecond));
		}
	}

	// 0x417fc0: a pizza is the application's at the end of its element, and the next number is its at the end of the
	// manifest
	void endElement(const std::string& theName) override
	{
		PizzaFrenzy* anApp = PizzaFrenzy::instance();
		if (theName == "pz")
		{
			anApp->addPizza(mPizza, mDesigned);
			mPizza = nullptr;
		}
		else if (theName == "pm")
			anApp->setPizzaMaxId(mMaxId, mDesigned);
	}

private:
	engine::Ref<Pizza> mPizza;  // +0x34: the pizza being read
	int mMaxId;                 // +0x38: the number the next pizza without one takes
	bool mDesigned;             // +0x3c: the player's own pizzas
};
}

// 0x4057b0: the handler goes on a stack of handlers, which reads the file; a failure is an error of the manifest. The
// player's own pizzas (theDesigned) are read from their file in the user folder, which the player has not made until
// the editor first saves: no file is no pizzas.
void pizzafrenzy::loadPizzaManifest(const std::string& theFile, bool theDesigned)
{
	engine::Ref<engine::InputStream> aStream;
	if (theDesigned)
	{
		std::string aData;
		if (!engine::getHost()->readUserFile(theFile, aData))
			return;
		aStream = new engine::MemoryStream(std::vector<uint8_t>(aData.begin(), aData.end()));
	}
	engine::Ref<engine::DocHandlerStack> aStack = new engine::DocHandlerStack();
	engine::Ref<PizzaDocHandler> aHandler = new PizzaDocHandler("pm", aStack, theDesigned);
	aStack->addHandler(aHandler);
	try
	{
		if (!theDesigned)
			aStream = engine::App::instance()->openStream(theFile.c_str());
		engine::parseXml(*aStack, aStream);
	}
	catch (...)
	{
		engine::App::instance()->log("error parsing pizza manifest");
		throw engine::Exception("Error parsing pizza manifest\n");
	}
}
