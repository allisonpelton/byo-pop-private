#include "UserGameData.h"
#include "Layout.h"
#include "PizzaFrenzy.h"
#include "Players.h"
#include "TileManifest.h"

#include <cstdio>
#include <cstdlib>

using namespace pizzafrenzy;

namespace
{
const int NOT_SET = -1;
const char* const DEFAULT_TOPPING_LEVELS = "Cheese=0,Pepperoni=0,";

// The name of a mode's parameter, such as "mode0LastLevel" (0x46db00 formats it)
std::string modeParameter(int theMode, const char* theName)
{
	char aNumber[16];
	snprintf(aNumber, sizeof(aNumber), "%d", theMode);
	return std::string("mode") + aNumber + theName;
}
}

// 0x428e00 (with the records' 0x427aa0 and 0x427ac0)
UserGameData::UserGameData() : mMaxCity(0), mUnlockTopping(false)
{
	for (int i = 0; i < MODE_COUNT; i++)
	{
		mModes[i].mMode = i;
		mModes[i].mLastCity = 0;
		mModes[i].mLastLevel = 0;
		mModes[i].mLastPizza = 0;
		mModes[i].mLastDay = 0;
		mModes[i].mLastScore = 0;
		mModes[i].mElapsedTime = 0;
		mModes[i].mHighestDay = 0;
	}
	reset();
}

// 0x427e80: the last pizza, the elapsed time and the last topping of a mode are not reset
void UserGameData::reset()
{
	for (int i = 0; i < MODE_COUNT; i++)
	{
		mModes[i].mMode = i;
		mModes[i].mLastLevel = NOT_SET;
		mModes[i].mLastCity = NOT_SET;
		mModes[i].mLastDay = NOT_SET;
		mModes[i].mLastScore = NOT_SET;
		mModes[i].mHighestDay = NOT_SET;
	}
	mToppingLevels.clear();
	mNewToppings.clear();
	mPickToppings.clear();
	mMaxCity = 0;
	mPlayerName.clear();
	mUnlockTopping = false;
}

// 0x429720
bool UserGameData::init()
{
	reset();
	engine::Players* aPlayers = engine::Players::instance();
	if (aPlayers == nullptr)
		return false;
	aPlayers->connect(this, &UserGameData::onPlayerChanged);
	return load();
}

// 0x4290c0
bool UserGameData::load()
{
	engine::UserInfo* aUser = engine::Players::instance()->current();
	if (aUser == nullptr)
		return false;
	reset();
	mPlayerName = aUser->getName();
	engine::Params& aParams = aUser->getParams();
	mMaxCity = (int)engine::getIntAttribute(aParams, "maxCity", 0);
	mUnlockTopping = engine::getBoolAttribute(aParams, "unlockTopping", false);
	setToppingLevels(engine::getStringAttribute(aParams, "ToppingLevels", DEFAULT_TOPPING_LEVELS));
	setToppings(mNewToppings, engine::getStringAttribute(aParams, "NewToppings", ""));
	setToppings(mPickToppings, engine::getStringAttribute(aParams, "PickToppings", ""));
	for (int i = 0; i < MODE_COUNT; i++)
	{
		ModeRecord& aMode = mModes[i];
		aMode.mLastLevel = (int)engine::getIntAttribute(aParams, modeParameter(i, "LastLevel"), NOT_SET);
		aMode.mLastCity = (int)engine::getIntAttribute(aParams, modeParameter(i, "LastCity"), NOT_SET);
		aMode.mHighestDay = (int)engine::getIntAttribute(aParams, modeParameter(i, "HighestDay"), NOT_SET);
		aMode.mLastDay = (int)engine::getIntAttribute(aParams, modeParameter(i, "LastDay"), NOT_SET);
		aMode.mLastPizza = (int)engine::getIntAttribute(aParams, modeParameter(i, "LastPizza"), NOT_SET);
		aMode.mLastScore = (int)engine::getIntAttribute(aParams, modeParameter(i, "LastScore"), NOT_SET);
		aMode.mLastTopping = engine::getStringAttribute(aParams, modeParameter(i, "LastTopping"), "");
		aMode.mElapsedTime = engine::getFloatAttribute(aParams, modeParameter(i, "ElapsedTime"), 0.0f);
	}
	return true;
}

// 0x427910
bool UserGameData::hasSavedGame(int theMode)
{
	return -1 < mModes[theMode].mLastDay && -1 < mModes[theMode].mLastScore;
}

// 0x428670
void UserGameData::clearSavedGame(int theMode)
{
	ModeRecord& aMode = mModes[theMode];
	aMode.mLastLevel = -1;
	aMode.mLastCity = -1;
	aMode.mLastDay = -1;
	aMode.mLastScore = -1;
	save();
}

// 0x4278a0
void UserGameData::setHighestDay(int theMode, int theDay)
{
	if (mModes[theMode].mHighestDay < theDay)
		mModes[theMode].mHighestDay = theDay;
}

// 0x427980
void UserGameData::setMaxCity(int theCity)
{
	if (mMaxCity <= theCity)
		mMaxCity = theCity;
}

// 0x4287c0
int UserGameData::getToppingLevel(Topping* theTopping)
{
	std::map<Topping*, int>::iterator anIt = mToppingLevels.find(theTopping);
	if (anIt == mToppingLevels.end())
		return NOT_SET;
	return anIt->second;
}

// 0x428d30
void UserGameData::addNewTopping(Topping* theTopping)
{
	std::map<Topping*, int>::iterator anIt = mToppingLevels.find(theTopping);
	if (anIt == mToppingLevels.end() || anIt->second < 0)
	{
		for (size_t i = 0; i < mNewToppings.size(); i++)
		{
			if (mNewToppings[i] == theTopping)
				return;
		}
		mNewToppings.push_back(theTopping);
	}
}

// 0x4286a0
void UserGameData::removeNewTopping(Topping* theTopping)
{
	for (size_t i = 0; i < mNewToppings.size(); i++)
	{
		if (mNewToppings[i] == theTopping)
		{
			mNewToppings.erase(mNewToppings.begin() + i);
			return;
		}
	}
}

// 0x427ff0
bool UserGameData::save()
{
	engine::Players* aPlayers = engine::Players::instance();
	engine::UserInfo* aUser = aPlayers->current();
	if (aUser == nullptr)
		return false;
	engine::Params& aParams = aUser->getParams();
	aParams.setInt("maxCity", mMaxCity);
	aParams.setBool("unlockTopping", mUnlockTopping);
	aParams.setString("ToppingLevels", getToppingLevels());
	aParams.setString("NewToppings", getToppingNames(mNewToppings));
	aParams.setString("PickToppings", getToppingNames(mPickToppings));
	for (int i = 0; i < MODE_COUNT; i++)
	{
		ModeRecord& aMode = mModes[i];
		aParams.setInt(modeParameter(i, "LastLevel"), aMode.mLastLevel);
		aParams.setInt(modeParameter(i, "LastCity"), aMode.mLastCity);
		aParams.setInt(modeParameter(i, "HighestDay"), aMode.mHighestDay);
		aParams.setInt(modeParameter(i, "LastDay"), aMode.mLastDay);
		aParams.setInt(modeParameter(i, "LastPizza"), aMode.mLastPizza);
		aParams.setInt(modeParameter(i, "LastScore"), aMode.mLastScore);
		aParams.setString(modeParameter(i, "LastTopping"), aMode.mLastTopping);
		aParams.setFloat(modeParameter(i, "ElapsedTime"), aMode.mElapsedTime);
	}
	aPlayers->saveUser(aUser);
	return true;
}

// 0x428820: "name=level," for each topping; a name the manifest has no topping for is left out. The text after the last
// comma is not read.
void UserGameData::setToppingLevels(const std::string& theLevels)
{
	mToppingLevels.clear();
	size_t aStart = 0;
	size_t aComma = theLevels.find(',', 0);
	while (aComma != std::string::npos)
	{
		std::string anEntry = theLevels.substr(aStart, aComma - aStart);
		aStart = aComma + 1;
		size_t anEquals = anEntry.find('=', 0);
		if (anEquals != std::string::npos)
		{
			std::string aName = anEntry.substr(0, anEquals);
			long aLevel = atol(anEntry.substr(anEquals + 1).c_str());
			Topping* aTopping = PizzaFrenzy::instance()->mTileManifest->getTopping(aName);
			if (aTopping != nullptr)
				mToppingLevels[aTopping] = (int)aLevel;
		}
		aComma = theLevels.find(',', aStart);
	}
}

// 0x4289f0, 0x428b90: "name," for each topping; a name the manifest has no topping for is left out
void UserGameData::setToppings(std::vector<engine::Ref<Topping>>& theToppings, const std::string& theNames)
{
	theToppings.clear();
	size_t aStart = 0;
	size_t aComma = theNames.find(',', 0);
	while (aComma != std::string::npos)
	{
		std::string aName = theNames.substr(aStart, aComma - aStart);
		aStart = aComma + 1;
		Topping* aTopping = PizzaFrenzy::instance()->mTileManifest->getTopping(aName);
		if (aTopping != nullptr)
			theToppings.push_back(aTopping);
		aComma = theNames.find(',', aStart);
	}
}

// 0x427b20: the original's map is ordered by the toppings' addresses, as this one is
std::string UserGameData::getToppingLevels()
{
	std::string aLevels;
	for (std::map<Topping*, int>::iterator anIt = mToppingLevels.begin(); anIt != mToppingLevels.end(); ++anIt)
	{
		char aNumber[16];
		snprintf(aNumber, sizeof(aNumber), "%d", anIt->second);
		aLevels += anIt->first->mName + "=" + aNumber + ",";
	}
	return aLevels;
}

// 0x427c80, 0x427d80
std::string UserGameData::getToppingNames(std::vector<engine::Ref<Topping>>& theToppings)
{
	std::string aNames;
	for (size_t i = 0; i < theToppings.size(); i++)
		aNames += theToppings[i]->mName + ",";
	return aNames;
}
