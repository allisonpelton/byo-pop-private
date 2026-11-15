#ifndef __PIZZAFRENZY_USERGAMEDATA_H__
#define __PIZZAFRENZY_USERGAMEDATA_H__

// What the game keeps of the current player (Iggle Pop's UserGameData, 0x128 bytes, constructor 0x428e00; the
// application keeps it at +0x128): how far the player got, the toppings and what was left in each mode. It is read
// from the parameters of the player (engines/sprout/Players.h) when the player changes, and written back to them by
// save.
//
// Not translated, as nothing the loading state, the menu state, the city select or the start of a game reaches calls
// them: the destructor, and the readers and writers of the fields the game's other states use.

#include "Object.h"
#include "Signal.h"
#include "TileEntries.h"

#include <map>
#include <string>
#include <vector>

namespace pizzafrenzy
{

// What the player left in one mode (0x3c bytes, constructors 0x427aa0 and 0x427ac0)
struct ModeRecord
{
	int mMode;                // +0x00
	int mLastCity;            // +0x04
	int mLastLevel;           // +0x08
	int mLastPizza;           // +0x0c
	int mLastDay;             // +0x10
	int mLastScore;           // +0x14
	float mElapsedTime;       // +0x18
	std::string mLastTopping; // +0x1c
	int mHighestDay;          // +0x38
};

class UserGameData : public engine::Object, public engine::HasSlots
{
public:
	static const int MODE_COUNT = 3;

	UserGameData();  // 0x428e00

	// 0x429720: ties the data to the players' signal, for the current player to be read when it changes, and reads it
	bool init();
	// 0x4290c0: reads the current player's parameters; false when there is no current player
	bool load();
	// 0x427ff0: writes the data to the current player's parameters, and the player to the registry; false when there is
	// no current player
	bool save();
	// 0x427e80
	void reset();
	// 0x427910: the mode has a game left in it (a day and a score)
	bool hasSavedGame(int theMode);
	// 0x428670: the mode has no game left in it, which is saved
	void clearSavedGame(int theMode);

	// The saved game of a mode
	int getLastCity(int theMode) { return mModes[theMode].mLastCity; }          // 0x4277e0
	int getLastLevel(int theMode) { return mModes[theMode].mLastLevel; }        // 0x4277b0
	int getLastDay(int theMode) { return mModes[theMode].mLastDay; }            // 0x427810
	int getLastScore(int theMode) { return mModes[theMode].mLastScore; }        // 0x427840
	float getElapsedTime(int theMode) { return mModes[theMode].mElapsedTime; }  // 0x4278d0
	// The furthest day the player got to in the mode
	int getHighestDay(int theMode) { return mModes[theMode].mHighestDay; }      // 0x427880
	// 0x4278a0: a day further than the furthest is the new furthest
	void setHighestDay(int theMode, int theDay);
	// 0x4504b0: the last city the player has got to
	int getMaxCity() { return mMaxCity; }
	// 0x427980: a city further than the last is the new last
	void setMaxCity(int theCity);
	// 0x427940: the toppings the player has not been shown yet
	std::vector<engine::Ref<Topping>>& getNewToppings() { return mNewToppings; }
	// 0x427960: the player is shown the toppings a level gives
	bool getUnlockTopping() { return mUnlockTopping; }
	// 0x4287c0: the level of the topping, -1 when the player does not have it
	int getToppingLevel(Topping* theTopping);
	// 0x428d30: a topping the player does not have is new, once
	void addNewTopping(Topping* theTopping);
	// 0x4286a0: the topping is not new any more
	void removeNewTopping(Topping* theTopping);

	std::string mPlayerName;  // +0x1c
	int mMaxCity;             // +0x38
	std::map<Topping*, int> mToppingLevels;                 // +0x3c: the level of each topping
	std::vector<engine::Ref<Topping>> mNewToppings;         // +0x4c
	std::vector<engine::Ref<Topping>> mPickToppings;        // +0x5c
	bool mUnlockTopping;      // +0x68
	ModeRecord mModes[MODE_COUNT];  // +0x6c

private:
	void onPlayerChanged() { load(); }  // the players' signal's slot, 0x4290c0

	void setToppingLevels(const std::string& theLevels);   // 0x428820
	// 0x4289f0, 0x428b90
	void setToppings(std::vector<engine::Ref<Topping>>& theToppings, const std::string& theNames);
	std::string getToppingLevels();                                      // 0x427b20
	std::string getToppingNames(std::vector<engine::Ref<Topping>>& theToppings);  // 0x427c80, 0x427d80
};

} // namespace pizzafrenzy

#endif
