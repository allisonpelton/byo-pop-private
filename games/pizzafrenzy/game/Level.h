#ifndef __PIZZAFRENZY_LEVEL_H__
#define __PIZZAFRENZY_LEVEL_H__

// The cities and their levels, read by the level manifest (LevelManifest.h) from
// res\manifests\levelManifestSpeed.xml. A city is a name and a theme with its levels in order; a level is a day of
// the game: the city file it is played in, its goal, what the game gives it (the toppings it gives the player, the
// kitchens, the boats, the special events and the waves of customers), or, for a bonus round, its time and its pizzas.

#include "DocHandler.h"
#include "Object.h"
#include "Pizza.h"
#include "TileEntries.h"

#include <string>
#include <vector>

namespace pizzafrenzy
{

class CityMap;

// A kitchen a level has (0x24 bytes, made by 0x417930): the van it drives
struct LevelKitchen
{
	Topping* mTopping;    // +0x00: the topping the van carries: null when the manifest makes it, the level start's pick
	std::string mVanType; // +0x04
};

// A boat that sails down a river of a level (0x34 bytes, made by 0x4179d0, constructor 0x416900)
struct LevelBoat
{
	std::string mName;           // +0x00
	engine::IntPair mPosition;   // +0x1c
	engine::FloatPair mDelay;    // +0x24: the seconds it waits, from the first to the second
	engine::IntPair mDirection;  // +0x2c
};

// A special event of a level (0x28 bytes, made by 0x417ad0, constructor 0x416960)
struct LevelEvent
{
	std::string mName;          // +0x00: the event of the tile manifest
	engine::IntPair mDelay;     // +0x1c
	bool mFirstWave;            // +0x24
};

// A wave of customers (0x18 bytes, made by 0x417bb0)
struct LevelWave
{
	engine::IntPair mSize;           // +0x00: how many customers, from the first to the second
	engine::FloatPair mSpawnDelay;   // +0x08
	engine::FloatPair mWaitTime;     // +0x10
};

// A level (0x148 bytes, constructor 0x417620)
class Level : public engine::Object
{
public:
	// The modes a story is kept for, by the game's mode number
	static const int MODE_COUNT = 3;

	Level(const std::string& theCityFile, const std::string& theName, const std::string& theMusic, int theGoal,
	      int theNumOrders, float theWaveGap);  // 0x417620

	const std::string& getCityFile() { return mCityFile; }  // 0x4041e0
	// 0x404210: the story file of the mode, "" for none
	const std::string& getStory(int theMode) { return mStories[theMode]; }
	// 0x416a10
	void setStory(const std::string& theStory, int theMode) { mStories[theMode] = theStory; }
	// 0x4165b0
	std::vector<engine::Ref<Topping>>& getToppings() { return mToppings; }
	void addTopping(Topping* theTopping);          // 0x417830
	void addPizza(Pizza* thePizza);                // 0x4178b0
	void addKitchen(const std::string& theVanType);  // 0x417930
	// 0x4179d0
	void addBoat(const std::string& theName, engine::IntPair thePosition, const engine::FloatPair& theDelay,
	             engine::IntPair theDirection);
	void addEvent(const std::string& theName, engine::IntPair theDelay, bool theFirstWave);  // 0x417ad0
	// 0x417bb0
	void addWave(const engine::IntPair& theSize, const engine::FloatPair& theSpawnDelay,
	             const engine::FloatPair& theWaitTime);

	// 0x416730: the kitchens of the map are given the level's vans and toppings
	void setUpKitchens(CityMap* theMap);
	// 0x4167d0: the level's boats are made into the map's boat spawners
	void setUpBoats(CityMap* theMap);

	std::string mCityFile;                           // +0x0c
	std::string mMusic;                              // +0x28: a clip of the tile manifest
	std::string mName;                               // +0x44
	bool mBonusRound;                                // +0x60: a level of the bonus round, which has no city file
	std::string mStories[MODE_COUNT];                // +0x64: by mode (speed, memory, concentration)
	int mGoal;                                       // +0xb8
	float mWaveGap;                                  // +0xbc
	int mNumOrders;                                  // +0xc0 (a bonus round's time in seconds)
	std::vector<engine::Ref<Topping>> mToppings;     // +0xc4: the toppings the level gives the player
	std::vector<engine::Ref<Pizza>> mPizzas;         // +0xd4: a bonus round's pizzas
	std::vector<LevelKitchen> mKitchens;             // +0xe4
	std::vector<LevelBoat> mBoats;                   // +0xf4
	std::vector<LevelEvent> mEvents;                 // +0x104
	std::vector<LevelWave> mWaves;                   // +0x114
	std::string mDeliverOnly;                        // +0x124: nothing translated reads it
};

// A city (0x5c bytes, constructor 0x4160f0)
class City : public engine::Object
{
public:
	City() {}  // 0x4160f0

	void addLevel(Level* theLevel) { mLevels.push_back(theLevel); }  // 0x417df0
	const std::string& getName() { return mName; }                   // 0x404250

	std::vector<engine::Ref<Level>> mLevels;  // +0x0c
	std::string mName;                        // +0x1c
	std::string mTheme;                       // +0x38: the key the customers of the city are found by (0x422f90)
};

} // namespace pizzafrenzy

#endif
