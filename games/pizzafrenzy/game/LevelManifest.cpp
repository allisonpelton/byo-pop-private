#include "LevelManifest.h"
#include "App.h"
#include "DocHandler.h"
#include "Exception.h"
#include "Layout.h"
#include "Level.h"
#include "PizzaFrenzy.h"
#include "Stream.h"

using namespace pizzafrenzy;

namespace
{
const float DEFAULT_WAVE_GAP = 3.0f;

// The handler of a city's elements (0x44 bytes, vtable 0x4fa6c8, whose XML handler part is 0x4fa6b4): the levels, and
// inside a level what it has. It goes off the stack at the end of its "city" element.
class CityDocHandler : public engine::BaseDocHandler
{
public:
	CityDocHandler(const std::string& theName, engine::DocHandlerStack* theParent, City* theCity) :
		engine::BaseDocHandler(theName, theParent), mCity(theCity)
	{
	}

	// 0x414f50
	void startElement(const std::string& theName, const engine::XmlAttributes& theAttributes) override;
	// 0x414e80: the level is done at the end of its element
	void endElement(const std::string& theName) override
	{
		if (theName == "level")
			mLevel = nullptr;
	}

private:
	void addLevel(const engine::XmlAttributes& theAttributes);

	engine::Ref<Level> mLevel;  // +0x34: the level being read
	engine::Ref<City> mCity;    // +0x38
};

// The handler of the manifest's elements (0x40 bytes, vtable 0x4fa6b0, whose XML handler part is 0x4fa69c): each
// "city" element makes a city, and the handler of its elements goes on the stack
class LevelManifest : public engine::BaseDocHandler
{
public:
	LevelManifest(const std::string& theName, engine::DocHandlerStack* theParent) :
		engine::BaseDocHandler(theName, theParent)
	{
	}

	// 0x416290
	void startElement(const std::string& theName, const engine::XmlAttributes& theAttributes) override
	{
		if (theName != "city")
			return;
		std::string aName = engine::getStringAttribute(theAttributes, "name", "No Name");
		std::string aTheme = engine::getStringAttribute(theAttributes, "theme", "Urban");
		mCity = new City();
		mCity->mName = aName;
		mCity->mTheme = aTheme;
		PizzaFrenzy::instance()->addCity(mCity);
		engine::Ref<CityDocHandler> aHandler = new CityDocHandler(theName, getParent(), mCity);
		getParent()->addHandler(aHandler);
	}

	// 0x414e40: the city is done at the end of its element
	void endElement(const std::string& theName) override
	{
		if (theName == "city")
			mCity = nullptr;
	}

private:
	engine::Ref<City> mCity;  // +0x34: the city being read
};
}

// 0x414f50
void CityDocHandler::startElement(const std::string& theName, const engine::XmlAttributes& theAttributes)
{
	std::string aName = engine::getStringAttribute(theAttributes, "name", "");
	if (theName == "level")
	{
		addLevel(theAttributes);
	}
	else if (theName == "topping")
	{
		mLevel->addTopping(PizzaFrenzy::instance()->mTileManifest->getTopping(aName));
	}
	else if (theName == "pizza")
	{
		int aPizzaId = (int)engine::getIntAttribute(theAttributes, "pizzaID", 0);
		mLevel->addPizza(PizzaFrenzy::instance()->getPizza(aPizzaId));
	}
	else if (theName == "kitchen")
	{
		mLevel->addKitchen(engine::getStringAttribute(theAttributes, "vanType", ""));
	}
	else if (theName == "boat")
	{
		engine::IntPair aPosition = engine::getIntPointAttribute(theAttributes, "pos", engine::IntPair{0, 0});
		engine::IntPair aDirection = engine::getIntPointAttribute(theAttributes, "direction", engine::IntPair{0, 1});
		engine::FloatPair aDelay = engine::getFloatRangeAttribute(theAttributes, "delay",
		                                                          engine::FloatPair{10.0f, 20.0f});
		mLevel->addBoat(aName, aPosition, aDelay, aDirection);
	}
	else if (theName == "event")
	{
		engine::IntPair aDelay = engine::getIntRangeAttribute(theAttributes, "delay", engine::IntPair{2, 10});
		bool aFirstWave = engine::getBoolAttribute(theAttributes, "firstWave", false);
		mLevel->addEvent(aName, aDelay, aFirstWave);
	}
	else if (theName == "wave")
	{
		engine::IntPair aSize = engine::getIntRangeAttribute(theAttributes, "waveSize", engine::IntPair{1, 1});
		engine::FloatPair aSpawnDelay = engine::getFloatRangeAttribute(theAttributes, "spawnDelay",
		                                                              engine::FloatPair{1.0f, 1.0f});
		engine::FloatPair aWaitTime = engine::getFloatRangeAttribute(theAttributes, "waitTime",
		                                                            engine::FloatPair{10.0f, 10.0f});
		mLevel->addWave(aSize, aSpawnDelay, aWaitTime);
	}
}

// 0x414f50: the "level" element of startElement. A bonus round has its time and no city file; a level of a mode has a
// story file of its own, or one "story" for all three
void CityDocHandler::addLevel(const engine::XmlAttributes& theAttributes)
{
	std::string aCityFile = engine::getStringAttribute(theAttributes, "city", "");
	std::string aName = engine::getStringAttribute(theAttributes, "name", "");
	std::string aMusic = engine::getStringAttribute(theAttributes, "music", "");
	std::string aSpeedStory = engine::getStringAttribute(theAttributes, "storySpeed", "");
	std::string aMemoryStory = engine::getStringAttribute(theAttributes, "storyMemory", "");
	std::string aConcentrationStory = engine::getStringAttribute(theAttributes, "storyConcentration", "");
	std::string aStory = engine::getStringAttribute(theAttributes, "story", "");
	if (!aStory.empty())
	{
		aConcentrationStory = aStory;
		aMemoryStory = aConcentrationStory;
		aSpeedStory = aMemoryStory;
	}
	int aGoal = (int)engine::getIntAttribute(theAttributes, "goal", 0);
	int aNumOrders = (int)engine::getIntAttribute(theAttributes, "numOrders", 0);
	float aWaveGap = engine::getFloatAttribute(theAttributes, "waveGap", DEFAULT_WAVE_GAP);
	int aTime = (int)engine::getIntAttribute(theAttributes, "time", 0);
	std::string aDeliverOnly = engine::getStringAttribute(theAttributes, "deliverOnly", "");
	std::string aType = engine::getStringAttribute(theAttributes, "type", "");
	if (aType == "bonus")
	{
		mLevel = new Level("", "", aMusic, 0, aTime, 0);
		mLevel->mBonusRound = true;
	}
	else
	{
		mLevel = new Level(aCityFile, aName, aMusic, aGoal, aNumOrders, aWaveGap);
	}
	if (!aSpeedStory.empty())
		mLevel->setStory(aSpeedStory, 0);
	if (!aMemoryStory.empty())
		mLevel->setStory(aMemoryStory, 1);
	if (!aConcentrationStory.empty())
		mLevel->setStory(aConcentrationStory, 2);
	if (!aDeliverOnly.empty())
		mLevel->mDeliverOnly = aDeliverOnly;
	mCity->addLevel(mLevel);
}

// 0x405470: the handler of the manifest goes on a stack of handlers, which reads the file; a failure is logged and is
// an error of the manifest
void pizzafrenzy::loadLevelManifest(const std::string& theFile)
{
	engine::Ref<engine::DocHandlerStack> aStack = new engine::DocHandlerStack();
	engine::Ref<LevelManifest> aHandler = new LevelManifest("LevelManifest", aStack);
	aStack->addHandler(aHandler);
	try
	{
		engine::Ref<engine::InputStream> aStream = engine::App::instance()->openStream(theFile.c_str());
		engine::parseXml(*aStack, aStream);
	}
	catch (...)
	{
		engine::App::instance()->log("error parsing level manifest");
		throw engine::Exception("Error parsing level manifest\n");
	}
}
