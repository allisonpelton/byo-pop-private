#include "TileManifest.h"
#include "App.h"
#include "DocHandler.h"
#include "Exception.h"
#include "Host.h"
#include "Layout.h"
#include "PizzaFrenzy.h"
#include "Random.h"
#include "Stream.h"

#include <cstdio>

using namespace pizzafrenzy;

namespace
{
const int DEFAULT_TILE_LAYER = 3;
const int DEFAULT_EFFECT_LAYER = 4;
const int ROAD_TILE_COUNT = 16;
const int ROAD_TILE_ROWS = 4;
const int ROAD_TILE_SIZE = 25;
const float DEFAULT_MUSIC_VOLUME = 1.0f;
const float DEFAULT_MUSIC_FADE_IN_TIME = 0.1f;
const float DEFAULT_MUSIC_FADE_OUT_TIME = 0.2f;

// The handler of a topping's element (0x41d680, vtable 0x4ffc54): the upgrades of the topping, until its element ends
class ToppingDocHandler : public engine::BaseDocHandler
{
public:
	ToppingDocHandler(engine::DocHandlerStack* theParent, Topping* theTopping) :
		engine::BaseDocHandler("topping", theParent), mTopping(theTopping)
	{
	}

	// 0x421a30
	void startElement(const std::string& theName, const engine::XmlAttributes& theAttributes) override
	{
		if (theName != "upgrade")
			return;
		std::string aName = engine::getStringAttribute(theAttributes, "name", "");
		std::string aDescription = engine::getStringAttribute(theAttributes, "description", "");
		int aBonusMultiplier = (int)engine::getIntAttribute(theAttributes, "bonusMultiplier", 0);
		int aCombo = (int)engine::getIntAttribute(theAttributes, "combo", 0);
		mTopping->mUpgrades.push_back(new Upgrade(aName, aDescription, aBonusMultiplier, aCombo));
	}

private:
	engine::Ref<Topping> mTopping;  // +0x34
};

// The handler of the manifest's elements (0x40 bytes, vtable of 0x423bd0's class): each element makes an entry and
// registers it with the manifest
class TileManifestDocHandler : public engine::BaseDocHandler
{
public:
	TileManifestDocHandler(engine::DocHandlerStack* theParent, TileManifest* theManifest) :
		engine::BaseDocHandler("TileManifest", theParent), mManifest(theManifest)
	{
	}

	// 0x423bd0
	void startElement(const std::string& theName, const engine::XmlAttributes& theAttributes) override;

private:
	void addTile(const engine::XmlAttributes& theAttributes);
	void addVehicle(const engine::XmlAttributes& theAttributes);
	void addCharacter(const engine::XmlAttributes& theAttributes);
	void addTopping(const engine::XmlAttributes& theAttributes);
	void addMusic(const engine::XmlAttributes& theAttributes);

	TileManifest* mManifest;  // +0x34
};

engine::Image* loadImage(const std::string& thePath)
{
	return engine::App::instance()->getImage(thePath.c_str());
}
}

// 0x423bd0
void TileManifestDocHandler::startElement(const std::string& theName, const engine::XmlAttributes& theAttributes)
{
	if (theName == "tile")
	{
		addTile(theAttributes);
	}
	else if (theName == "decoration")
	{
		std::string aName = engine::getStringAttribute(theAttributes, "name", "");
		std::string aPath = engine::getStringAttribute(theAttributes, "path", "");
		std::string aHsvShift = engine::getStringAttribute(theAttributes, "hsvShift", "0,0,0");
		int aLayer = (int)engine::getIntAttribute(theAttributes, "layer", DEFAULT_TILE_LAYER);
		mManifest->addDecoration(aName, "decoration", aPath, aLayer, aHsvShift);
	}
	else if (theName == "roadTiles")
	{
		std::string aName = engine::getStringAttribute(theAttributes, "name", "");
		std::string aPath = engine::getStringAttribute(theAttributes, "path", "");
		mManifest->addRoadTiles(aName, aPath);
	}
	else if (theName == "vehicle")
	{
		addVehicle(theAttributes);
	}
	else if (theName == "character")
	{
		addCharacter(theAttributes);
	}
	else if (theName == "topping")
	{
		addTopping(theAttributes);
	}
	else if (theName == "particleFx")
	{
		// The effect's particle system, read from the file of "fxPath" by the signal build's particle document handler
		// (0x47bc40), is left out: the effect has its layer only
		std::string aName = engine::getStringAttribute(theAttributes, "name", "");
		int aLayer = (int)engine::getIntAttribute(theAttributes, "layer", DEFAULT_EFFECT_LAYER);
		mManifest->registerParticleEffect(aName, new ParticleEffect(aLayer));
	}
	else if (theName == "event")
	{
		std::string aName = engine::getStringAttribute(theAttributes, "name", "");
		std::string aCharacter = engine::getStringAttribute(theAttributes, "character", "");
		std::string aPortrait = engine::getStringAttribute(theAttributes, "portrait", "");
		std::string aType = engine::getStringAttribute(theAttributes, "type", "");
		std::string aSound = engine::getStringAttribute(theAttributes, "sound", "");
		std::string aDescription = engine::getStringAttribute(theAttributes, "description", "");
		std::string aDeliver = engine::getStringAttribute(theAttributes, "deliver", "yes");
		mManifest->registerEvent(aName, new SpecialEvent(aCharacter, aName, aType, loadImage(aPortrait), aSound,
		                                                 aDescription, aDeliver != "no"));
	}
	else if (theName == "funfact")
	{
		mManifest->addFunFact(engine::getStringAttribute(theAttributes, "text", ""));
	}
	else if (theName == "title")
	{
		std::string aText = engine::getStringAttribute(theAttributes, "text", "");
		int aPoints = (int)engine::getIntAttribute(theAttributes, "points", 0);
		mManifest->addTitle(new ScoreTitle(aText, aPoints));
	}
	else if (theName == "music")
	{
		addMusic(theAttributes);
	}
}

// 0x423bd0: the "tile" element of startElement. A tile of the map: a kitchen, a customer's house or a bridge
void TileManifestDocHandler::addTile(const engine::XmlAttributes& theAttributes)
{
	std::string aName = engine::getStringAttribute(theAttributes, "name", "");
	std::string aPath = engine::getStringAttribute(theAttributes, "path", "");
	std::string aClass = engine::getStringAttribute(theAttributes, "class", "");
	engine::getStringAttribute(theAttributes, "hsvShift", "0,0,0");  // read, and not used by a tile
	engine::Image* anImage = loadImage(aPath);
	engine::Point aBottomRightOffset = engine::getPointAttribute(theAttributes, "BROffset", engine::Point(0, 0));
	engine::Point aBase = engine::getPointAttribute(theAttributes, "base", engine::Point(0, 0));
	engine::Point aParkingSpot = engine::getPointAttribute(theAttributes, "parkingspot", engine::Point(0, 0));
	engine::Point aDimensions = engine::getPointAttribute(theAttributes, "dimensions", engine::Point(1, 1));
	engine::Ref<TileType> aTile;
	if (aClass == "kitchen")
	{
		aTile = new KitchenTileType(anImage, aParkingSpot);
	}
	else if (aClass == "customer")
	{
		std::string aCharacterName = engine::getStringAttribute(theAttributes, "character", "");
		aTile = new CustomerTileType(anImage, aParkingSpot, aCharacterName);
	}
	else if (aClass == "bridge")
	{
		std::string aRoadPath = engine::getStringAttribute(theAttributes, "roadPath", "");
		std::string anUnblockedPath = engine::getStringAttribute(theAttributes, "unblockedPath", "");
		std::string anUnblockedRoadPath = engine::getStringAttribute(theAttributes, "unblockedRoadPath", "");
		aTile = new BridgeTileType(anImage, loadImage(aRoadPath), loadImage(anUnblockedPath),
		                           loadImage(anUnblockedRoadPath));
	}
	else
	{
		throw engine::Exception("Error: invalid tile type");
	}
	aTile->mBottomRightOffset = aBottomRightOffset;
	aTile->mBase = aBase;
	aTile->mDimensions = aDimensions;
	mManifest->registerTile(aName, aClass, aTile);
}

// 0x423bd0: the "vehicle" element of startElement
void TileManifestDocHandler::addVehicle(const engine::XmlAttributes& theAttributes)
{
	std::string aName = engine::getStringAttribute(theAttributes, "name", "");
	std::string aNorthPath = engine::getStringAttribute(theAttributes, "Npath", "");
	std::string aSouthPath = engine::getStringAttribute(theAttributes, "Spath", "");
	std::string anEastWestPath = engine::getStringAttribute(theAttributes, "EWpath", "");
	std::string aClass = engine::getStringAttribute(theAttributes, "class", "");
	std::string anEffect = engine::getStringAttribute(theAttributes, "fx", "");
	int aSpeed = (int)engine::getIntAttribute(theAttributes, "speed", 0);
	std::string aDescription = engine::getStringAttribute(theAttributes, "description", "");
	std::string aParkedPath = engine::getStringAttribute(theAttributes, "ParkedPath", "");
	std::string aHoverPath = engine::getStringAttribute(theAttributes, "HoverPath", "");
	engine::Ref<Vehicle> aVehicle = new Vehicle(loadImage(aNorthPath), loadImage(aSouthPath), loadImage(anEastWestPath),
	                                            aDescription, aClass, anEffect);
	aVehicle->mSpeed = aSpeed;
	if (!aParkedPath.empty())
		aVehicle->setParkedImages(loadImage(aParkedPath), loadImage(aHoverPath));
	mManifest->registerTile(aName, "vehicle", aVehicle);
}

// 0x423bd0: the "character" element of startElement
void TileManifestDocHandler::addCharacter(const engine::XmlAttributes& theAttributes)
{
	std::string aName = engine::getStringAttribute(theAttributes, "name", "");
	std::string aTheme = engine::getStringAttribute(theAttributes, "theme", "");
	std::string aPath = engine::getStringAttribute(theAttributes, "path", "");
	int aNumPizzas = (int)engine::getIntAttribute(theAttributes, "numPizzas", 0);
	float aPatience = engine::getFloatAttribute(theAttributes, "patience", 0.0f);
	float aGenerosity = engine::getFloatAttribute(theAttributes, "generosity", 0.0f);
	std::string aGreeting = engine::getStringAttribute(theAttributes, "greeting", "");
	std::string aSpecial = engine::getStringAttribute(theAttributes, "special", "");
	engine::Ref<Character> aCharacter = new Character(aName, loadImage(aPath), aNumPizzas, aPatience, aGenerosity,
	                                                  aGreeting, aSpecial);
	std::string aKey = aName;
	aKey.push_back('_');
	aKey.append(aTheme);
	mManifest->registerCharacter(aKey, aCharacter);
}

// 0x423bd0: the "topping" element of startElement: a topping, and the handler of its upgrades put on the stack until
// its element ends
void TileManifestDocHandler::addTopping(const engine::XmlAttributes& theAttributes)
{
	std::string aName = engine::getStringAttribute(theAttributes, "name", "");
	std::string aCouponPath = engine::getStringAttribute(theAttributes, "couponPath", "");
	std::string aPath = engine::getStringAttribute(theAttributes, "path", "");
	std::string aDisplayName = engine::getStringAttribute(theAttributes, "display", "");
	engine::Image* anImage = loadImage(aPath);
	engine::Image* aCouponImage = loadImage(aCouponPath);
	engine::Ref<Topping> aTopping = new Topping(aName, aDisplayName, anImage, aCouponImage);
	mManifest->registerTopping(aName, aTopping);
	engine::Ref<ToppingDocHandler> aHandler = new ToppingDocHandler(getParent(), aTopping);
	getParent()->addHandler(aHandler);
}

// 0x423bd0: the "music" element of startElement
void TileManifestDocHandler::addMusic(const engine::XmlAttributes& theAttributes)
{
	std::string aName = engine::getStringAttribute(theAttributes, "name", "");
	std::string aFile = engine::getStringAttribute(theAttributes, "file", "");
	float aVolume = engine::getFloatAttribute(theAttributes, "vol", DEFAULT_MUSIC_VOLUME);
	float aFadeInTime = engine::getFloatAttribute(theAttributes, "fadeInTime", DEFAULT_MUSIC_FADE_IN_TIME);
	float aFadeOutTime = engine::getFloatAttribute(theAttributes, "fadeOutTime", DEFAULT_MUSIC_FADE_OUT_TIME);
	engine::getStringAttribute(theAttributes, "loop", "true");  // read as a text first, which nothing uses
	bool aLoop = engine::getBoolAttribute(theAttributes, "loop", true);
	int aCategory = (int)engine::getIntAttribute(theAttributes, "category", 0);
	mManifest->registerMusic(aName, new GameMusicClip(aFile, aFadeInTime, aFadeOutTime, aVolume, aLoop, aCategory));
}

// 0x41f8c0: the handler of the manifest goes on a stack of handlers, which reads the file; a failure is an error of
// the manifest
void TileManifest::load(const std::string& theFile)
{
	engine::Ref<engine::DocHandlerStack> aStack = new engine::DocHandlerStack();
	engine::Ref<TileManifestDocHandler> aHandler = new TileManifestDocHandler(aStack, this);
	aStack->addHandler(aHandler);
	try
	{
		engine::Ref<engine::InputStream> aStream = engine::App::instance()->openStream(theFile.c_str());
		engine::parseXml(*aStack, aStream);
	}
	catch (...)
	{
		throw engine::Exception("Error parsing tiles manifest\n");
	}
}

// 0x422d80
Topping* TileManifest::getTopping(const std::string& theName)
{
	if (theName.empty())
		return nullptr;
	std::map<std::string, engine::Ref<Topping>>::iterator anIt = mToppings.find(theName);
	if (anIt == mToppings.end())
		return nullptr;
	return anIt->second;
}

// 0x422c00
TileType* TileManifest::getTile(const std::string& theName, const std::string& theClass)
{
	if (theClass == "roads")
		return nullptr;
	std::map<std::string, std::map<std::string, engine::Ref<TileType>>>::iterator aClass = mTiles.find(theClass);
	if (aClass == mTiles.end())
		return nullptr;
	std::map<std::string, engine::Ref<TileType>>::iterator anIt = aClass->second.find(theName);
	if (anIt == aClass->second.end())
		return nullptr;
	return anIt->second;
}

// 0x422d30
TileType* TileManifest::getRoadTile(const std::string& theName, int theIndex)
{
	std::map<std::string, std::vector<engine::Ref<TileType>>>::iterator anIt = mRoadTiles.find(theName);
	if (anIt == mRoadTiles.end() || theIndex < 0 || ROAD_TILE_COUNT <= theIndex
	    || (int)anIt->second.size() <= theIndex)
		return nullptr;
	return anIt->second[theIndex];
}

// 0x422dd0
ParticleEffect* TileManifest::getParticleEffect(const std::string& theName)
{
	std::map<std::string, engine::Ref<ParticleEffect>>::iterator anIt = mParticleEffects.find(theName);
	if (anIt == mParticleEffects.end())
		return nullptr;
	return anIt->second;
}

// 0x422e10: the original's lookup adds a null entry for a name that is not there; the port leaves the map alone
GameMusicClip* TileManifest::getMusic(const std::string& theName)
{
	std::map<std::string, engine::Ref<GameMusicClip>>::iterator anIt = mMusic.find(theName);
	if (anIt == mMusic.end())
		return nullptr;
	return anIt->second;
}

// 0x422df0: the original's lookup adds a null entry for a name that is not there; the port leaves the map alone
SpecialEvent* TileManifest::getEvent(const std::string& theName)
{
	std::map<std::string, engine::Ref<SpecialEvent>>::iterator anIt = mEvents.find(theName);
	if (anIt == mEvents.end())
		return nullptr;
	return anIt->second;
}

// 0x422f90: the same lookup as the others', over the key the characters are registered under
Character* TileManifest::getCharacterByNameTheme(const std::string& theName)
{
	std::string aKey = theName;
	aKey.push_back('_');
	aKey.append(PizzaFrenzy::instance()->getCurrentCity()->mTheme);
	std::map<std::string, engine::Ref<Character>>::iterator anIt = mCharacters.find(aKey);
	if (anIt == mCharacters.end())
		return nullptr;
	return anIt->second;
}

// 0x422e60
void TileManifest::registerTile(const std::string& theName, const std::string& theClass, TileType* theTile)
{
	std::map<std::string, engine::Ref<TileType>>& aTiles = mTiles[theClass];
	if (aTiles[theName] != nullptr)
		throw engine::Exception("registering tile which already exists");
	aTiles[theName] = theTile;
}

// 0x4229f0: the sheet of road tiles is 4 rows of 4 cells of 25 by 25 pixels, each cell a tile; a name that is there
// already is let be
void TileManifest::addRoadTiles(const std::string& theName, const std::string& thePath)
{
	std::vector<engine::Ref<TileType>>& aRoadTiles = mRoadTiles[theName];
	if (!aRoadTiles.empty())
		return;
	aRoadTiles.resize(ROAD_TILE_COUNT);
	engine::Image* aSheet = loadImage(thePath);
	for (int i = 0; i < ROAD_TILE_COUNT; i++)
	{
		int aLeft = (i / ROAD_TILE_ROWS) * ROAD_TILE_SIZE;
		int aTop = (i % ROAD_TILE_ROWS) * ROAD_TILE_SIZE;
		engine::Image* aCell = engine::getHost()->cropImage(aSheet, aLeft, aTop, ROAD_TILE_SIZE, ROAD_TILE_SIZE);
		if (aCell != nullptr)
			aCell->setAlphaMode(engine::ALPHA_BLEND, 0xff);
		aRoadTiles[i] = new TileType(aCell, DEFAULT_TILE_LAYER);
	}
}

// 0x423980: a shift of the colours (hue in degrees, saturation and value) makes the tile's image a copy with them
// shifted
void TileManifest::addDecoration(const std::string& theName, const std::string& theClass, const std::string& thePath,
                                 int theLayer, const std::string& theHsvShift)
{
	float aHue = 0;
	float aSaturation = 0;
	float aValue = 0;
	sscanf(theHsvShift.c_str(), "%f,%f,%f", &aHue, &aSaturation, &aValue);
	engine::Image* anImage = loadImage(thePath);
	engine::Ref<TileType> aTile;
	if (aHue == 0.0f && aSaturation == 0.0f && aValue == 0.0f)
	{
		aTile = new TileType(anImage, theLayer);
	}
	else
	{
		anImage->setPivot(0);
		engine::Ref<engine::Image> aShifted = engine::getHost()->shiftImageColors(anImage, aHue, aSaturation, aValue);
		anImage->setPivot(1);
		if (aShifted == nullptr)
			aShifted = anImage;
		aTile = new TileType(aShifted, theLayer);
	}
	registerTile(theName, theClass, aTile);
}

// 0x422590
void TileManifest::registerCharacter(const std::string& theKey, Character* theCharacter)
{
	if (mCharacters[theKey] != nullptr)
		throw engine::Exception("registering character which already exists!");
	mCharacters[theKey] = theCharacter;
}

// 0x422660: the topping is kept by name and in the list
void TileManifest::registerTopping(const std::string& theName, Topping* theTopping)
{
	if (mToppings[theName] != nullptr)
		throw engine::Exception("registering topping which already exists!");
	mToppings[theName] = theTopping;
	mToppingList.push_back(theTopping);
}

// 0x422780
void TileManifest::registerParticleEffect(const std::string& theName, ParticleEffect* theEffect)
{
	if (mParticleEffects[theName] != nullptr)
		throw engine::Exception("registering fx which already exists!");
	mParticleEffects[theName] = theEffect;
}

// 0x422850
void TileManifest::registerEvent(const std::string& theName, SpecialEvent* theEvent)
{
	if (mEvents[theName] != nullptr)
		throw engine::Exception("registering event which already exists!");
	mEvents[theName] = theEvent;
}

// 0x422920
void TileManifest::registerMusic(const std::string& theName, GameMusicClip* theClip)
{
	if (mMusic[theName] != nullptr)
		throw engine::Exception("registering music clip which already exists!");
	mMusic[theName] = theClip;
}

// 0x41e120
ScoreTitle* TileManifest::getTitle(int thePoints)
{
	ScoreTitle* aTitle = mTitles[0];
	for (size_t i = 1; i < mTitles.size() && mTitles[i]->mPoints <= thePoints; i++)
		aTitle = mTitles[i];
	return aTitle;
}

// 0x41e600: the original reads the entry the draw gives without a check
const std::string& TileManifest::getRandomFunFact()
{
	static const std::string aNoFact;
	int aPick = engine::randomRange(0, (int)mFunFacts.size() - 1);
	if (aPick < 0 || (size_t)aPick >= mFunFacts.size())
		return aNoFact;
	return mFunFacts[aPick];
}

// 0x4223e0
void TileManifest::addFunFact(const std::string& theText)
{
	mFunFacts.push_back(theText);
}

// 0x421970
void TileManifest::addTitle(ScoreTitle* theTitle)
{
	mTitles.push_back(theTitle);
}
