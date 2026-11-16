#include "StageDocHandler.h"
#include "App.h"
#include "Font.h"
#include "FishSpawner.h"
#include "Layout.h"
#include "PlayerFish.h"
#include "PlayerFishDocHandler.h"
#include "StageMgr.h"
#include "Stream.h"

#include <cstdio>

using namespace engine;

namespace feedingfrenzy
{

// What the stage's file does not say
const char* const DEFAULT_STAGE_NAME = "No Name Stage";
const char* const NO_NEXT_STAGE = "end";
const int DEFAULT_NEXT_LEVEL_AT = 1000;
const int DEFAULT_STAGE_WIDTH = 640;
const int DEFAULT_STAGE_HEIGHT = 480;
const char* const DEFAULT_STAGE_KIND = "normal";
const char* const NO_PLAYER_FILE = "NONE";
const float DEFAULT_GROWTH_LEVEL = 0.1f;
const float DEFAULT_GROWTH_SIZE = 10.0f;
const int DEFAULT_GROWTH_SCORE = 9999;
const char* const NO_FISH_FILE = "NO TEMPLATE";
const int DEFAULT_POOL_SIZE = 10;
const char* const DEFAULT_SPAWN_MODE = "normal";
const char* const NO_BACKGROUND_LAYOUT = "NO BACKGROUND";
const char* const DEFAULT_AMBIENT_FILE = "config\\sound\\ambience1.xml";
// The point a fish stands at when its element gives none
const float DEFAULT_PLACE_X = 320.0f;
const float DEFAULT_PLACE_Y = 240.0f;
// The wait between fish when a rule gives none, in seconds from low to high
const float DEFAULT_RESPAWN_LOW = 0.0f;
const float DEFAULT_RESPAWN_HIGH = 5.0f;
// The factor the exe applies to the sizes and places a file gives (1 at the size the game runs at)
const float PLACE_SCALE = 1.0f;
// The seconds a minute has, for the stage's time ("1:30")
const int SECONDS_PER_MINUTE = 60;

// The names of the stage's elements
const char* const STAGE_ELEMENT = "stage";
const char* const PLAYER_FILE_ELEMENT = "playerFish";
const char* const GROWTH_LEVEL_ELEMENT = "size";
const char* const ENEMY_FISH_ELEMENT = "enemyFish";
const char* const SPAWN_RULE_ELEMENT = "spawnInfo";
const char* const STATIC_SPAWN_ELEMENT = "staticSpawn";
const char* const ENVIRONMENT_ELEMENT = "environment";
const char* const MUSIC_ELEMENT = "music";

// 0x448450: a time as the file writes it, minutes and seconds ("1:30"), in seconds; the default if the attribute is
// not there
static float getTimeAttribute(const XmlAttributes& theAttributes, const std::string& theName, float theDefault)
{
	XmlAttributes::const_iterator anIt = theAttributes.find(theName);
	if (anIt == theAttributes.end())
		return theDefault;
	int aMinutes = 0;
	int aSeconds = 0;
	sscanf(anIt->second.c_str(), "%d:%d", &aMinutes, &aSeconds);
	return (float)(aMinutes * SECONDS_PER_MINUTE) + (float)aSeconds;
}

// 0x425ba0
void StageDocHandler::startElement(const std::string& theName, const XmlAttributes& theAttributes)
{
	if (theName == STAGE_ELEMENT)
		readStage(theAttributes);
	else if (theName == PLAYER_FILE_ELEMENT)
		readPlayerFile(theAttributes);
	else if (theName == GROWTH_LEVEL_ELEMENT)
		readGrowthLevel(theAttributes);
	else if (theName == ENEMY_FISH_ELEMENT)
		readEnemyFish(theAttributes);
	else if (theName == SPAWN_RULE_ELEMENT)
	{
		if (mSpawner != nullptr)
			readSpawnRule(theAttributes);
	}
	else if (theName == STATIC_SPAWN_ELEMENT)
	{
		if (mSpawner != nullptr)
			readStaticSpawn(theAttributes);
	}
	else if (theName == ENVIRONMENT_ELEMENT)
		readEnvironment(theAttributes);
	else if (theName == MUSIC_ELEMENT)
		readMusic(theAttributes);
}

// 0x4241c0: the kind of fish whose element ends goes to the stage
void StageDocHandler::endElement(const std::string& theName)
{
	if (theName == ENEMY_FISH_ELEMENT && mSpawner != nullptr)
	{
		mStage->addSpawner(mSpawner);
		mSpawner = nullptr;
	}
}

// 0x424330
void StageDocHandler::readStage(const XmlAttributes& theAttributes)
{
	mStage->setStageName(getStringAttribute(theAttributes, "friendlyName", DEFAULT_STAGE_NAME));

	std::string aNextStage = getStringAttribute(theAttributes, "nextStage", NO_NEXT_STAGE);
	int aNextLevelAt = getIntAttribute(theAttributes, "nextLevelAt", DEFAULT_NEXT_LEVEL_AT);
	mStage->setNext(aNextStage, aNextLevelAt);

	int aWidth = (int)((float)getIntAttribute(theAttributes, "stageWidth", DEFAULT_STAGE_WIDTH) * PLACE_SCALE);
	int aHeight = (int)((float)getIntAttribute(theAttributes, "stageHeight", DEFAULT_STAGE_HEIGHT) * PLACE_SCALE);

	std::string aKind = getStringAttribute(theAttributes, "type", DEFAULT_STAGE_KIND);
	if (aKind == "normal")
		mStage->setType(STAGE_NORMAL);
	else if (aKind == "timedBonus")
		mStage->setType(STAGE_TIMED_BONUS);
	else if (aKind == "survival")
		mStage->setType(STAGE_SURVIVAL);
	else if (aKind == "bonus")
		mStage->setType(STAGE_BONUS);
	else if (aKind == "menu")
		mStage->setType(STAGE_MENU);

	mStage->setTimeLimit(getTimeAttribute(theAttributes, "time", 0.0f));
	mStage->setWorldSize(aWidth, aHeight);
}

// 0x424970: the file of the player's fish is read, and gives the stage its player
void StageDocHandler::readPlayerFile(const XmlAttributes& theAttributes)
{
	std::string aFile = getStringAttribute(theAttributes, "config", NO_PLAYER_FILE);
	Ref<DocHandlerStack> aStack = new DocHandlerStack();
	Ref<PlayerFishDocHandler> aHandler = new PlayerFishDocHandler(PLAYER_FILE_ELEMENT, aStack, mStage);
	aStack->addHandler(aHandler);
	Ref<InputStream> aStream = App::instance()->openStream(aFile.c_str());
	parseXml(*aStack, aStream);
}

// 0x424bd0: the player grows to a size at a score
void StageDocHandler::readGrowthLevel(const XmlAttributes& theAttributes)
{
	float aLevel = getFloatAttribute(theAttributes, "size", DEFAULT_GROWTH_LEVEL);
	float aSize = getFloatAttribute(theAttributes, "scale", DEFAULT_GROWTH_SIZE);
	int aScore = getIntAttribute(theAttributes, "targetScore", DEFAULT_GROWTH_SCORE);
	PlayerFish* aPlayer = mStage->getPlayer();
	if (aPlayer != nullptr)
		aPlayer->addGrowthLevel(aLevel, aSize, aScore);
}

// 0x424d50: a kind of fish: its file, how many of it the pool makes and how it counts
void StageDocHandler::readEnemyFish(const XmlAttributes& theAttributes)
{
	mSpawner = new FishSpawner();
	std::string aFile = getStringAttribute(theAttributes, "template", NO_FISH_FILE);
	int aPoolSize = getIntAttribute(theAttributes, "poolSize", DEFAULT_POOL_SIZE);
	bool aDisplayMenuImage = getBoolAttribute(theAttributes, "displayMenuImage", true);

	std::string aMode = getStringAttribute(theAttributes, "mode", DEFAULT_SPAWN_MODE);
	int aSpawnMode;
	if (aMode == "onlyUneaten")
		aSpawnMode = SPAWN_ONLY_UNEATEN;
	else if (aMode == "uncounted")
		aSpawnMode = SPAWN_UNCOUNTED;
	else
		aSpawnMode = SPAWN_NORMAL;
	mSpawner->setMode(aSpawnMode);
	mSpawner->load(aFile, aPoolSize);
	mSpawner->setDisplayMenuImage(aDisplayMenuImage);
}

// 0x425130: from a size of the player, how many fish of the kind there may be at once and how long one takes to come
void StageDocHandler::readSpawnRule(const XmlAttributes& theAttributes)
{
	float aPlayerLevel = getFloatAttribute(theAttributes, "playerSize", 0.0f);
	int aMaxCount = getIntAttribute(theAttributes, "maxCount", 0);
	FloatPair aRespawn = getFloatRangeAttribute(theAttributes, "respawnFreq",
	                                            FloatPair{DEFAULT_RESPAWN_LOW, DEFAULT_RESPAWN_HIGH});
	mSpawner->addRule(aPlayerLevel, aMaxCount, aRespawn.mFirst, aRespawn.mSecond);
}

// 0x425ba0: a fish of the kind that stands in the stage when it starts
void StageDocHandler::readStaticSpawn(const XmlAttributes& theAttributes)
{
	Point aPlace = getPointAttribute(theAttributes, "pos", Point(DEFAULT_PLACE_X, DEFAULT_PLACE_Y));
	aPlace.x *= PLACE_SCALE;
	aPlace.y *= PLACE_SCALE;
	bool aRespawn = getBoolAttribute(theAttributes, "doRespawn", false);
	mSpawner->addPlacement(aPlace.x, aPlace.y, aRespawn);
}

// 0x4252d0: the layouts of the pictures behind the fish, in front of them and between
void StageDocHandler::readEnvironment(const XmlAttributes& theAttributes)
{
	std::string aBackground = getStringAttribute(theAttributes, "bgLayout", NO_BACKGROUND_LAYOUT);
	std::string aForeground = getStringAttribute(theAttributes, "fgLayout", "");
	std::string aMiddle = getStringAttribute(theAttributes, "mgLayout", "");
	mStage->setLayoutNames(aBackground, aForeground, aMiddle);
}

// 0x4256b0: the sounds the stage opens and ends with, its music and its ambient sounds
void StageDocHandler::readMusic(const XmlAttributes& theAttributes)
{
	std::string anIntro = getStringAttribute(theAttributes, "introSound", "");
	std::string anOutro = getStringAttribute(theAttributes, "outroSound", "");
	std::string aMusic = getStringAttribute(theAttributes, "gameMusic", "");
	std::string anAmbient = getStringAttribute(theAttributes, "ambientSound", DEFAULT_AMBIENT_FILE);
	mStage->setSounds(anIntro, anOutro, anAmbient, aMusic);
}

// 0x4240f0: a handler reads the stage's file
void loadStageFile(const std::string& theFile, StageMgr* theStage)
{
	Ref<DocHandlerStack> aStack = new DocHandlerStack();
	Ref<StageDocHandler> aHandler = new StageDocHandler(STAGE_ELEMENT, aStack, theStage);
	aStack->addHandler(aHandler);
	Ref<InputStream> aStream = App::instance()->openStream(theFile.c_str());
	parseXml(*aStack, aStream);
}

} // namespace feedingfrenzy
