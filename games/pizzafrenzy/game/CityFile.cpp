#include "App.h"
#include "CityMap.h"
#include "DocHandler.h"
#include "Layout.h"
#include "PizzaFrenzy.h"
#include "Stream.h"
#include "Tiles.h"

using namespace pizzafrenzy;

namespace
{
const char* const DEFAULT_TILE_NAME = "NONAME";

// The handler of a city file's elements (0x40 bytes, constructor 0x410080, whose XML handler part has the vtable
// 0x4fa5a8): the "city" element sets the map's grid and background, the others put what they name on the map.
class CityFileDocHandler : public engine::BaseDocHandler
{
public:
	CityFileDocHandler(const std::string& theName, engine::DocHandlerStack* theParent, CityMap* theMap) :
		engine::BaseDocHandler(theName, theParent), mMap(theMap)
	{
	}

	// 0x410250
	void startElement(const std::string& theName, const engine::XmlAttributes& theAttributes) override
	{
		if (theName == "city")
		{
			engine::IntPair aGridSize = engine::getIntPointAttribute(theAttributes, "gridSize", {0, 0});
			int aCellSize = (int)engine::getIntAttribute(theAttributes, "cellSize", 0);
			engine::IntPair aGridOffset = engine::getIntPointAttribute(theAttributes, "gridOffset", {0, 0});
			std::string aBackground = engine::getStringAttribute(theAttributes, "background", "");
			int aUniqueId = (int)engine::getIntAttribute(theAttributes, "uniqueID", 0);
			std::string aHsvShift = engine::getStringAttribute(theAttributes, "hsvShift", "0,0,0");
			mMap->setGrid(aGridSize.mFirst, aGridSize.mSecond, aCellSize, aGridOffset, aUniqueId);
			mMap->setBackground(aBackground, aHsvShift);
		}
		else if (theName == "road")
		{
			std::string aTile = engine::getStringAttribute(theAttributes, "tile", DEFAULT_TILE_NAME);
			engine::getIntAttribute(theAttributes, "weight", 1);  // read and not kept
			addTile(new RoadTile(aTile), theAttributes);
		}
		else if (theName == "customer")
		{
			std::string aTile = engine::getStringAttribute(theAttributes, "tile", DEFAULT_TILE_NAME);
			addTile(new CustomerTile(aTile), theAttributes);
		}
		else if (theName == "kitchen")
		{
			std::string aTile = engine::getStringAttribute(theAttributes, "tile", DEFAULT_TILE_NAME);
			addTile(new KitchenTile(aTile), theAttributes);
		}
		else if (theName == "bridge")
		{
			std::string aTile = engine::getStringAttribute(theAttributes, "tile", DEFAULT_TILE_NAME);
			addTile(new BridgeTile(aTile), theAttributes);
		}
		else if (theName == "decoration")
		{
			std::string aTile = engine::getStringAttribute(theAttributes, "tile", DEFAULT_TILE_NAME);
			engine::Ref<Decoration> aDecoration = new Decoration(aTile, LAYER_GROUND);
			engine::IntPair aPosition = engine::getIntPointAttribute(theAttributes, "pos", {-1, -1});
			if (aDecoration->load())
			{
				aDecoration->setPosition(aPosition);
				aDecoration->addTo(mMap);
			}
		}
	}

private:
	// the tile takes the picture of its type and the name the file gives it (slot 0x130), and goes on its cell (0x13c);
	// the tile's slot 0x138, which is given the attributes, only answers yes
	void addTile(Tile* theTile, const engine::XmlAttributes& theAttributes)
	{
		engine::Ref<Tile> aTile = theTile;
		engine::IntPair aCell = engine::getIntPointAttribute(theAttributes, "cell", {-1, -1});
		std::string aName = engine::getStringAttribute(theAttributes, "name", "");
		if (aTile->accept(mMap, aName))
			aTile->place(mMap, aCell);
	}

	engine::Ref<CityMap> mMap;  // +0x34
};
}

// 0x405270: the screen's groups are emptied, the file's elements fill the map and the screen draws the city
void PizzaFrenzy::loadCityFile(const std::string& theFile, GameScreen* theScreen)
{
	theScreen->clearGroups();
	engine::Ref<engine::DocHandlerStack> aStack = new engine::DocHandlerStack();
	engine::Ref<CityFileDocHandler> aHandler = new CityFileDocHandler("city", aStack, mCityMap);
	aStack->addHandler(aHandler);
	engine::Ref<engine::InputStream> aStream = engine::App::instance()->openStream(theFile.c_str());
	engine::parseXml(*aStack, aStream);
	theScreen->drawCity();
}
