// port: the standard test commands (docs/TEST_COMMANDS.md), reachable only from a test (-script, -commands, the web
// test page's command call). Each answers on stdout with one line: STATE {...}, MOVE {...} or MOVE null, OK
// <command>, or ERROR <command>: <reason>. A command never given changes nothing and draws no random number; "move"
// changes nothing either, and the bot draws from no generator. The game's notes list the commands and the state's
// keys (games/pizzafrenzy/docs/README.md, "Test commands").

#include "PizzaFrenzyApp.h"
#include "platform/MemReport.h"
#include "sprout/App.h"
#include "sprout/Button.h"
#include "sprout/EditBox.h"
#include "sprout/Random.h"
#include "sprout/ScreenMgr.h"
#include "sprout/host/SoundHost.h"
#include "game/BonusStates.h"
#include "game/CitySelectScreen.h"
#include "game/ConcentrationGame.h"
#include "game/CustomerPopup.h"
#include "game/DialogState.h"
#include "game/GameState.h"
#include "game/LevelStartScreen.h"
#include "game/LevelStartState.h"
#include "game/MemoryGame.h"
#include "game/MenuState.h"
#include "game/ModeStates.h"
#include "game/NewToppingScreen.h"
#include "game/NewToppingState.h"
#include "game/PizzaEditor.h"
#include "game/PizzaEditorState.h"
#include "game/PizzaFrenzy.h"
#include "game/SplashState.h"
#include "game/StoryScreen.h"
#include "game/ToppingButton.h"
#include "game/StoryState.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <typeinfo>
#include <map>
#include <string>

using namespace Sexy;

namespace pf = pizzafrenzy;

namespace
{

// the pointer's rest point for a click that only has to skip the level's introduction: the middle of the screen
const int SCREEN_CENTER_X = 400;
const int SCREEN_CENTER_Y = 300;

// the updates a move waits after its last step: the click is handled in the next update and the screen follows it
const int MOVE_UPDATES = 2;

// the updates a move waits while a screen comes in, or the splash screen loads
const int WAIT_UPDATES = 30;

const char* const MODE_NAMES[] = { "speed", "memory", "concentration" };

// the mode whose button the bot clicks on the main menu (the botmode command); -1: the mode the game was last in
int gBotMode = -1;
// botmode editor: the menu's design button instead of a mode's
const int BOT_EDITOR = 3;

std::string jsonText(const std::string& theText)
{
	std::string aText = "\"";
	for (char c : theText)
	{
		if (c == '"' || c == '\\')
			aText += '\\';
		if ((unsigned char)c >= 0x20)
			aText += c;
	}
	return aText + "\"";
}

// the number of the mode a name stands for (a lower case one of MODE_NAMES)
int modeNumber(const std::string& theName)
{
	for (int i = 0; i < 3; i++)
	{
		if (theName == MODE_NAMES[i])
			return i;
	}
	return 0;
}

std::string lowerCase(std::string theText)
{
	for (char& c : theText)
		c = (char)std::tolower((unsigned char)c);
	return theText;
}

// the screen the manager shows, or is about to show (the last one asked for), none while it is closing
engine::GenericScreen* shownScreen(engine::ScreenMgr* theScreens)
{
	if (!theScreens->mQueue.empty())
		return theScreens->mQueue.back();
	if (theScreens->mSwitching && theScreens->mNext != nullptr)
		return theScreens->mNext;
	return theScreens->mCurrent;
}

// the name of the dialog that is open
const char* dialogName(pf::PizzaFrenzy* theApp, engine::GenericScreen* theScreen)
{
	if (theScreen == nullptr)
		return "dialog";
	if (theScreen == theApp->mPauseMenu) return "pause";
	if (theScreen == theApp->mOptions) return "options";
	if (theScreen == theApp->mToppingBook) return "toppings";
	if (theScreen == theApp->mUserSelect) return "players";
	if (theScreen == theApp->mNewUserEntry) return "newPlayer";
	if (theScreen == theApp->mDeleteUserConfirm) return "deletePlayer";
	if (theScreen == theApp->mHighScoreResetConfirm) return "resetScores";
	if (theScreen == theApp->mRestoreGameConfirm) return "restoreGame";
	if (theScreen == theApp->mAppQuitConfirm) return "quit";
	if (theScreen == theApp->mGameExitConfirm) return "exitGame";
	if (theScreen == theApp->mUnlockNotice) return "unlockNotice";
	if (theScreen == theApp->mPizzaNameScreen) return "pizzaName";
	return "dialog";
}

// The screen the game is on, named for the state the application is in and, where a state shows more than one screen,
// the screen it shows. A paused game is "pause" whatever the dialog on top of it.
std::string screenName(pf::PizzaFrenzy* theApp)
{
	engine::State* aState = theApp->mStates.getState();
	if (aState == nullptr || dynamic_cast<pf::SplashState*>(aState) != nullptr)
		return "splash";
	if (dynamic_cast<pf::MenuState*>(aState) != nullptr)
	{
		engine::GenericScreen* aScreen = shownScreen(theApp->mScreens);
		if (aScreen == theApp->mCitySelect)
			return "citySelect";
		if (aScreen == theApp->mHighScores)
			return "highScores";
		return "menu";
	}
	if (pf::DialogState* aDialog = dynamic_cast<pf::DialogState*>(aState))
	{
		if (dynamic_cast<pf::GameState*>(aDialog->getNextState()) != nullptr ||
			dynamic_cast<pf::BonusGameState*>(aDialog->getNextState()) != nullptr)
			return "pause";
		return dialogName(theApp, aDialog->getShownScreen());
	}
	if (dynamic_cast<pf::StoryState*>(aState) != nullptr)
		return "story";
	if (dynamic_cast<pf::NewToppingState*>(aState) != nullptr)
		return shownScreen(theApp->mScreens) == theApp->mDesignerScreen ? "designer" : "newTopping";
	if (dynamic_cast<pf::PizzaEditorState*>(aState) != nullptr)
		return "editor";
	if (dynamic_cast<pf::LevelStartState*>(aState) != nullptr)
		return "levelStart";
	if (dynamic_cast<pf::GameState*>(aState) != nullptr || dynamic_cast<pf::BonusGameState*>(aState) != nullptr)
		return "game";
	if (dynamic_cast<pf::LevelSummaryState*>(aState) != nullptr)
		return "summary";
	return "other";
}

// where the mode's own states are: the introduction's titles, play, the prize blimp's flight and the level's end; the
// bonus round's are its introduction, a pizza coming in ("arrive"), play, the scoring of a pizza, and its end
const char* modePhase(pf::GameMode* theMode)
{
	if (pf::BonusGame* aBonus = dynamic_cast<pf::BonusGame*>(theMode))
	{
		switch (aBonus->getPhase())
		{
		case pf::BonusGame::PHASE_INTRO: return "intro";
		case pf::BonusGame::PHASE_ARRIVE: return "arrive";
		case pf::BonusGame::PHASE_PLAY: return "play";
		case pf::BonusGame::PHASE_SCORE: return "score";
		case pf::BonusGame::PHASE_END: return "end";
		}
	}
	engine::State* aState = theMode->mStates.getState();
	if (dynamic_cast<pf::ModeIntroState*>(aState) != nullptr)
		return "intro";
	if (dynamic_cast<pf::ModePlayState*>(aState) != nullptr)
		return "play";
	if (dynamic_cast<pf::PrizeBlimpState*>(aState) != nullptr)
		return "blimp";
	if (dynamic_cast<pf::ModeEndState*>(aState) != nullptr)
		return "end";
	return "";
}

// the centre of a popup on its tile, in the screen's points: the rectangle the game's own hover test uses
// (GameScreen::updateHover: the popup's bounds moved by the tile's place)
engine::Point popupCenter(pf::Tile* theTile, pf::PizzaPopup* thePopup)
{
	engine::Rect aRect = *thePopup->getBounds();
	engine::Point* aPlace = theTile->getPosition();
	return engine::Point((aRect.left + aRect.right) / 2.0f + aPlace->x, (aRect.top + aRect.bottom) / 2.0f + aPlace->y);
}

int roundedPoint(float theValue)
{
	return (int)std::lround(theValue);
}

// a customer who waits, as the state and the bot see it
struct WaitingCustomer
{
	pf::CustomerTile* mTile;
	pf::CustomerPopup* mPopup;
	pf::Order* mOrder;
	engine::Point mPoint;
	bool mPicked;
	bool mClickable;
	bool mHidden;  // the order is behind the question mark (the concentration mode)
	int mAge;      // the place in the orders by age, the oldest 0; -1 for none
};

bool isPickedTile(pf::GameMode* theMode, pf::CustomerTile* theTile)
{
	for (const engine::Ref<pf::CustomerTile>& aPick : theMode->getPicks())
	{
		if (aPick.get() == theTile)
			return true;
	}
	return false;
}

std::vector<WaitingCustomer> waitingCustomers(pf::GameMode* theMode)
{
	std::vector<WaitingCustomer> aCustomers;
	for (const engine::Ref<pf::Tile>& aTile : theMode->getMap()->mCustomers)
	{
		pf::CustomerTile* aHouse = static_cast<pf::CustomerTile*>(aTile.get());
		pf::PizzaPopup* aPopup = aHouse->getOccupant();
		if (aPopup == nullptr || aPopup->getKind() != pf::POPUP_CUSTOMER)
			continue;
		pf::CustomerPopup* aCustomerPopup = static_cast<pf::CustomerPopup*>(aPopup);
		if (aCustomerPopup->getOrder() == nullptr)
			continue;
		WaitingCustomer aCustomer;
		aCustomer.mTile = aHouse;
		aCustomer.mPopup = aCustomerPopup;
		aCustomer.mOrder = aCustomerPopup->getOrder();
		aCustomer.mPoint = popupCenter(aHouse, aCustomerPopup);
		aCustomer.mPicked = isPickedTile(theMode, aHouse);
		aCustomer.mClickable = aHouse->isClickable();
		aCustomer.mHidden = aCustomerPopup->mQuestionMark->isVisible();
		aCustomer.mAge = -1;
		int anIndex = 0;
		for (const engine::Ref<pf::Order>& anOrder : theMode->getActiveOrders())
		{
			if (anOrder.get() == aCustomer.mOrder)
				aCustomer.mAge = anIndex;
			anIndex++;
		}
		aCustomers.push_back(aCustomer);
	}
	return aCustomers;
}

// How many orders can be picked together for the topping: GameStats::getPickLimit (0x41a670) without its insertion of
// a topping the player has no level of, so that asking changes nothing
int pickLimit(pf::GameStats* theStats, pf::Topping* theTopping)
{
	if (pf::PizzaFrenzy::instance()->mGameMode == pf::PizzaFrenzy::MODE_CONCENTRATION)
		return 1;
	pf::ToppingMap<int>::iterator aLevel = theStats->mToppingLevels.find(theTopping);
	if (aLevel == theStats->mToppingLevels.end())
		return 1;
	return aLevel->second + 1;
}

std::string toppingName(pf::Topping* theTopping)
{
	return theTopping == nullptr ? "" : theTopping->mName;
}

// one step of a move's input: the pointer's move and then its click
std::string clickInput(const engine::Point& thePoint)
{
	char aBuf[128];
	int x = roundedPoint(thePoint.x);
	int y = roundedPoint(thePoint.y);
	std::snprintf(aBuf, sizeof(aBuf), "[{\"move\":[%d,%d]},{\"click\":[%d,%d]}]", x, y, x, y);
	return aBuf;
}

// the input of a move that presses a key
std::string keyInput(const char* theKey)
{
	return std::string("[{\"key\":\"") + theKey + "\"}]";
}

// a move of one real input (a click on a place, or a key) that waits the usual updates
std::string inputLine(const char* theWhat, const std::string& theDetail, const std::string& theInput)
{
	char aBuf[64];
	std::snprintf(aBuf, sizeof(aBuf), ",\"updates\":%d}", MOVE_UPDATES);
	return std::string("{\"what\":\"") + theWhat + "\"" + theDetail + ",\"input\":" + theInput + aBuf;
}

// a move that only lets the game run on: the screen is still coming in, or the splash screen is loading
std::string waitLine(int theUpdates)
{
	char aBuf[128];
	std::snprintf(aBuf, sizeof(aBuf), "{\"what\":\"wait\",\"input\":[{\"wait\":%d}],\"updates\":%d}", theUpdates,
		theUpdates);
	return aBuf;
}

// the centre of a node on the 800x600 screen: its bounds (in its parent's points, as the mouse's hit test takes them)
// moved by the places of the parents
engine::Point nodeCenter(engine::Node* theNode)
{
	engine::Rect aRect = *theNode->getBounds();
	float aX = (aRect.left + aRect.right) / 2.0f;
	float aY = (aRect.top + aRect.bottom) / 2.0f;
	for (engine::Node* aParent = theNode->mParent; aParent != nullptr; aParent = aParent->mParent)
	{
		aX += aParent->getPosition()->x;
		aY += aParent->getPosition()->y;
	}
	return engine::Point(aX, aY);
}

std::string moveLine(const char* theWhat, const std::string& theDetail, const engine::Point& thePoint)
{
	char aBuf[64];
	std::snprintf(aBuf, sizeof(aBuf), ",\"updates\":%d}", MOVE_UPDATES);
	return std::string("{\"what\":\"") + theWhat + "\"" + theDetail + ",\"input\":" + clickInput(thePoint) + aBuf;
}

// a move that clicks a node of the shown screen, which has to be there, shown and enabled (a topping button is a group
// whose card inside it takes the click, so the group's own flag is not asked)
std::string nodeMove(const char* theWhat, engine::Node* theNode)
{
	bool aGroup = dynamic_cast<pf::ToppingButton*>(theNode) != nullptr;
	if (theNode == nullptr || !theNode->isVisible() || (!aGroup && !theNode->isEnabled()))
		return waitLine(WAIT_UPDATES);
	return moveLine(theWhat, "", nodeCenter(theNode));
}

// the point of the player's pizza (in the screen's points) that a topping of the bonus round's pizza lies at
engine::Point pizzaPoint(pf::BonusGame* theGame, pf::PizzaTopping* theTopping)
{
	engine::Point aGroup = pf::BonusGame::getAbsPosition(theGame->getUserGroup());
	return engine::Point(aGroup.x + theTopping->getPosition()->x, aGroup.y + theTopping->getPosition()->y);
}

// the bonus round's part of the state: the time and pizzas left, the topping picked, the toppings of the target pizza
// (with the point of the player's pizza each is put at), the ones put, and the buttons with the number each has left
std::string bonusState(pf::BonusGame* theGame)
{
	char aBuf[256];
	std::snprintf(aBuf, sizeof(aBuf), ",\"bonus\":{\"timeLeft\":%.2f,\"pizzasLeft\":%d,\"playing\":%s,\"selected\":%s",
		theGame->getTimeLeft(), theGame->getPizzasLeft(), theGame->isPlaying() ? "true" : "false",
		jsonText(toppingName(theGame->getSelectedTopping())).c_str());
	std::string aState = aBuf;
	aState += ",\"targets\":[";
	bool aFirst = true;
	for (const engine::Ref<pf::PizzaTopping>& aTarget : theGame->getTargetToppings())
	{
		engine::Point aPoint = pizzaPoint(theGame, aTarget);
		std::snprintf(aBuf, sizeof(aBuf), "%s{\"topping\":%s,\"x\":%d,\"y\":%d}", aFirst ? "" : ",",
			jsonText(toppingName(aTarget->getTopping())).c_str(), roundedPoint(aPoint.x), roundedPoint(aPoint.y));
		aState += aBuf;
		aFirst = false;
	}
	aState += "],\"placed\":[";
	aFirst = true;
	for (const engine::Ref<pf::PizzaTopping>& aPlaced : theGame->getUserToppings())
	{
		engine::Point aPoint = pizzaPoint(theGame, aPlaced);
		std::snprintf(aBuf, sizeof(aBuf), "%s{\"topping\":%s,\"x\":%d,\"y\":%d}", aFirst ? "" : ",",
			jsonText(toppingName(aPlaced->getTopping())).c_str(), roundedPoint(aPoint.x), roundedPoint(aPoint.y));
		aState += aBuf;
		aFirst = false;
	}
	aState += "],\"buttons\":[";
	aFirst = true;
	for (const auto& aButton : theGame->getButtons())
	{
		engine::Point aPoint = nodeCenter(aButton.second);
		std::snprintf(aBuf, sizeof(aBuf), "%s{\"topping\":%s,\"number\":%d,\"x\":%d,\"y\":%d}",
			aFirst ? "" : ",", jsonText(toppingName(aButton.first)).c_str(), aButton.second->getNumber(),
			roundedPoint(aPoint.x), roundedPoint(aPoint.y));
		aState += aBuf;
		aFirst = false;
	}
	return aState + "]}";
}

}

// the pizza editor's part of the state: the pizza it shows (its number in the list of the player's pizzas, its name and
// its toppings with their places on the pizza), the topping picked, the buttons of the toppings the player has, the
// name the name screen holds, and the middle of the pizza on the screen, where a click puts the picked topping
std::string editorState(pf::PizzaFrenzy* theApp)
{
	pf::PizzaEditor* anEditor = theApp->mPizzaEditor;
	pf::Pizza* aPizza = anEditor->getPizza();
	char aBuf[256];
	engine::Point aMiddle = pf::BonusGame::getAbsPosition(anEditor->getPizzaToppingGroup());
	engine::EditBox* aNameEntry = static_cast<engine::EditBox*>(theApp->mPizzaNameScreen->getNode("nameEntry"));
	std::snprintf(aBuf, sizeof(aBuf), ",\"editor\":{\"pizza\":%d,\"pizzas\":%d,\"id\":%d,\"nextId\":%d,\"name\":",
		anEditor->getPizzaIndex() + 1, (int)anEditor->getPizzaCount(), aPizza == nullptr ? -1 : aPizza->mId,
		theApp->getPizzaMaxId(true));
	std::string aText = aBuf;
	aText += jsonText(aPizza == nullptr ? "" : aPizza->mName);
	aText += ",\"picked\":" + jsonText(toppingName(anEditor->getSelectedTopping()));
	std::snprintf(aBuf, sizeof(aBuf), ",\"buttons\":%d,\"middle\":[%d,%d],\"nameEntry\":",
		(int)anEditor->getToppingButtons()->mChildren.size(), roundedPoint(aMiddle.x), roundedPoint(aMiddle.y));
	aText += aBuf + jsonText(aNameEntry->getText()) + ",\"toppings\":[";
	bool aFirst = true;
	for (const engine::Ref<pf::PizzaTopping>& aPlaced : anEditor->getPlacedToppings())
	{
		std::snprintf(aBuf, sizeof(aBuf), "%s{\"topping\":", aFirst ? "" : ",");
		aText += aBuf + jsonText(toppingName(aPlaced->getTopping()));
		std::snprintf(aBuf, sizeof(aBuf), ",\"x\":%d,\"y\":%d}", (int)aPlaced->getPosition()->x,
			(int)aPlaced->getPosition()->y);
		aText += aBuf;
		aFirst = false;
	}
	return aText + "]}";
}

// The game's state as one JSON line: the screen the game is on, the player's progress, and, in a level, the stats'
// numbers and the board (every customer with an order, every kitchen, every tip waiting for its click), all points in
// the game's 800x600 screen. money is the tips the level has brought, tips the tip jar's money, which the coupons are
// bought with, tipGoal the jar's size; timer is the stats' time, in seconds.
std::string PizzaFrenzyApp::TestState()
{
	pf::PizzaFrenzy* anApp = pf::PizzaFrenzy::instance();
	char aBuf[512];
	std::string aScreen = screenName(anApp);
	int aModeNumber = anApp->mGameMode;
	const char* aModeName = aModeNumber >= 0 && aModeNumber < 3 ? MODE_NAMES[aModeNumber] : "";
	pf::GameMode* aMode = anApp->mMode;
	std::string aState = "{\"screen\":" + jsonText(aScreen);
	std::snprintf(aBuf, sizeof(aBuf), ",\"mode\":%s,\"city\":%d,\"level\":%d,\"day\":%d,\"frame\":%d",
		jsonText(aModeName).c_str(), anApp->mCurrentCity + 1, anApp->mCurrentLevel + 1, anApp->mDay, mFrames);
	aState += aBuf;
	if (anApp->mCurrentCity >= 0 && anApp->mCurrentCity < (int)anApp->mCities.size())
		aState += ",\"cityName\":" + jsonText(anApp->mCities[anApp->mCurrentCity]->getName());
	bool aLevelOver = aScreen == "summary" ||
		(aMode != nullptr && std::string(modePhase(aMode)) == "end");
	if (aScreen == "story" || aScreen == "newTopping" || aScreen == "levelStart" || aScreen == "game" ||
		aScreen == "pause" || aScreen == "summary")
		mGameSeen = true;
	// the game is over when it has been played and the menu is back (after a game over, or the game won story)
	bool anOver = mGameSeen && (aScreen == "menu" || aScreen == "highScores" || aScreen == "citySelect");
	aState += std::string(",\"levelOver\":") + (aLevelOver ? "true" : "false");
	aState += std::string(",\"over\":") + (anOver ? "true" : "false");
	if (aScreen == "editor" || aScreen == "pizzaName")
		return aState + editorState(anApp) + "}";
	pf::GameStats* aStats = anApp->mGameStats;
	if (aStats != nullptr)
	{
		std::snprintf(aBuf, sizeof(aBuf), ",\"score\":%d,\"tips\":%d,\"tipGoal\":%d,\"money\":%d,\"happiness\":%d,"
			"\"ordersToGo\":%d,\"timer\":%.2f,\"delivered\":%d,\"ordered\":%d,\"combo\":%d", aStats->mScore,
			aStats->mTipMoney, aStats->mLevelGoal, aStats->mTipsEarned, aStats->mHappiness, aStats->mOrderCount,
			aStats->mElapsedTime, aStats->mPizzasDelivered, aStats->mPizzasOrdered, aStats->mComboLength);
		aState += aBuf;
	}
	if (pf::BonusGame* aBonus = dynamic_cast<pf::BonusGame*>(aMode))
	{
		std::snprintf(aBuf, sizeof(aBuf), ",\"phase\":%s", jsonText(modePhase(aMode)).c_str());
		return aState + aBuf + bonusState(aBonus) + "}";
	}
	if (aMode == nullptr || aMode->getMap() == nullptr)
		return aState + "}";
	std::snprintf(aBuf, sizeof(aBuf), ",\"phase\":%s,\"picks\":%d", jsonText(modePhase(aMode)).c_str(),
		aMode->getPickCount());
	aState += aBuf;
	if (pf::ConcentrationGame* aConcentration = dynamic_cast<pf::ConcentrationGame*>(aMode))
	{
		std::snprintf(aBuf, sizeof(aBuf), ",\"showLeft\":%.2f", aConcentration->getShowSecondsLeft());
		aState += aBuf;
	}
	aState += ",\"board\":{\"customers\":[";
	bool aFirst = true;
	for (const WaitingCustomer& aCustomer : waitingCustomers(aMode))
	{
		std::snprintf(aBuf, sizeof(aBuf), "%s{\"cell\":[%d,%d],\"x\":%d,\"y\":%d,\"topping\":%s,\"picked\":%s,"
			"\"wait\":%.2f,\"clickable\":%s,\"event\":%s,\"chameleon\":%s,\"hidden\":%s,\"age\":%d}",
			aFirst ? "" : ",",
			aCustomer.mTile->mCell.mFirst, aCustomer.mTile->mCell.mSecond, roundedPoint(aCustomer.mPoint.x),
			roundedPoint(aCustomer.mPoint.y), jsonText(toppingName(aCustomer.mOrder->mTopping)).c_str(),
			aCustomer.mPicked ? "true" : "false", aCustomer.mPopup->mWaitLeft,
			aCustomer.mClickable ? "true" : "false", aCustomer.mOrder->mEvent != nullptr ? "true" : "false",
			aCustomer.mPopup->mChameleon ? "true" : "false", aCustomer.mHidden ? "true" : "false", aCustomer.mAge);
		aState += aBuf;
		aFirst = false;
	}
	aState += "],\"kitchens\":[";
	aFirst = true;
	for (const engine::Ref<pf::Tile>& aTile : aMode->getMap()->mKitchens)
	{
		pf::KitchenTile* aKitchen = static_cast<pf::KitchenTile*>(aTile.get());
		pf::PizzaPopup* aPopup = aKitchen->getOccupant();
		if (aPopup == nullptr)
			continue;
		engine::Point aPoint = popupCenter(aKitchen, aPopup);
		std::snprintf(aBuf, sizeof(aBuf), "%s{\"cell\":[%d,%d],\"x\":%d,\"y\":%d,\"topping\":%s,\"clickable\":%s,"
			"\"vanOut\":%s}", aFirst ? "" : ",", aKitchen->mCell.mFirst, aKitchen->mCell.mSecond,
			roundedPoint(aPoint.x), roundedPoint(aPoint.y), jsonText(toppingName(aKitchen->getTopping())).c_str(),
			aKitchen->isClickable() ? "true" : "false", aKitchen->isVanOut() ? "true" : "false");
		aState += aBuf;
		aFirst = false;
	}
	aState += "],\"tips\":[";
	aFirst = true;
	for (const engine::Ref<pf::TipPopup>& aTip : aMode->getMap()->mTips)
	{
		const engine::Rect* aBounds = aTip->getBounds();
		engine::Point aCenter((aBounds->left + aBounds->right) / 2.0f, (aBounds->top + aBounds->bottom) / 2.0f);
		std::snprintf(aBuf, sizeof(aBuf), "%s{\"x\":%d,\"y\":%d,\"amount\":%d,\"waiting\":%s}", aFirst ? "" : ",",
			roundedPoint(aCenter.x), roundedPoint(aCenter.y), aTip->getAmount(),
			aTip->isWaiting() ? "true" : "false");
		aState += aBuf;
		aFirst = false;
	}
	return aState + "]}}";
}

// The name the bot gives its pizza, typed key by key into the name screen
const char* const BOT_PIZZA_NAME = "BOT PIZZA";
// The name of the player the bot makes on a first launch
const char* const BOT_PLAYER_NAME = "BOT";

// The toppings the bot puts on a pizza, each this far from the middle of the pizza (the first is on the middle)
const int BOT_PIZZA_TOPPINGS = 3;
const int BOT_TOPPING_SPACING = 40;

// a move of the keys that type a text, the editing box emptied first by as many Backspaces as it holds letters
std::string typeInput(const std::string& theOldText, const char* theText)
{
	std::string anInput = "[";
	for (size_t i = 0; i < theOldText.size(); i++)
		anInput += std::string(i == 0 ? "" : ",") + "{\"key\":\"Backspace\"}";
	for (const char* aLetter = theText; *aLetter != '\0'; aLetter++)
	{
		std::string aKey = *aLetter == ' ' ? " " : std::string(1, *aLetter);
		anInput += std::string(anInput.size() > 1 ? "," : "") + "{\"key\":\"" + aKey + "\"}";
	}
	return anInput + "]";
}

// The bot's move in the pizza designer's introduction (its first button) and in the pizza editor, as a player designs a
// pizza: with no topping picked, the first topping's button; with one picked, a click on the pizza, until the pizza has
// the toppings of BOT_PIZZA_TOPPINGS; then its name is edited (the edit button opens the name screen, where the name is
// typed and the OK button pressed), and then the pizza is left by the editor's exit button, which saves the file.
static std::string DesignerMove(pf::PizzaFrenzy* theApp)
{
	engine::Container* aGroup = static_cast<engine::Container*>(theApp->mDesignerScreen->getNode("pizzaDesignerGroup"));
	for (const engine::Ref<engine::Node>& aChild : aGroup->mChildren)
	{
		engine::Container* aButtonGroup = dynamic_cast<engine::Container*>(aChild.get());
		if (aButtonGroup != nullptr && !aButtonGroup->mChildren.empty())
			return nodeMove("tryDesigner", aButtonGroup->mChildren.front());
	}
	return waitLine(WAIT_UPDATES);
}

static std::string EditorMove(pf::PizzaFrenzy* theApp)
{
	pf::PizzaEditor* anEditor = theApp->mPizzaEditor;
	pf::Pizza* aPizza = anEditor->getPizza();
	if (aPizza == nullptr)
		return waitLine(WAIT_UPDATES);
	engine::Container* aButtons = anEditor->getToppingButtons();
	int aPlaced = (int)anEditor->getPlacedToppings().size();
	if (aPlaced < BOT_PIZZA_TOPPINGS)
	{
		if (anEditor->getSelectedTopping() == nullptr)
		{
			if (aButtons->mChildren.empty())
				return waitLine(WAIT_UPDATES);
			return nodeMove("topping", aButtons->mChildren.front());
		}
		engine::Point aPlace = pf::BonusGame::getAbsPosition(anEditor->getPizzaToppingGroup());
		aPlace.x += (float)(aPlaced * BOT_TOPPING_SPACING);
		return moveLine("place", "", aPlace);
	}
	if (aPizza->mName.compare(BOT_PIZZA_NAME) != 0)
		return nodeMove("editName", anEditor->getScreen()->getNode("editCaption"));
	return nodeMove("exit", anEditor->getScreen()->getNode("menu"));
}

// the button of a dialog's layout that sends the action, or none
static engine::Node* actionButton(engine::GenericScreen* theScreen, const char* theAction)
{
	for (const auto& aNamed : theScreen->mNodes)
	{
		engine::Button* aButton = dynamic_cast<engine::Button*>(aNamed.second);
		if (aButton == nullptr)
			continue;
		for (const auto* anEntry : aButton->mListeners)
			if (anEntry->mName.compare(theAction) == 0)
				return aButton;
	}
	return nullptr;
}

// A name screen (the pizza's, or the new player's on a first launch): the name typed into its "nameEntry", then its
// "done" button
static std::string NameMove(engine::GenericScreen* theScreen, const char* theName)
{
	engine::EditBox* anEntry = static_cast<engine::EditBox*>(theScreen->getNode("nameEntry"));
	if (anEntry->getText().compare(theName) != 0)
		return inputLine("typeName", "", typeInput(anEntry->getText(), theName));
	return nodeMove("nameDone", theScreen->getNode("done"));
}

// The bot's move on the screens before play, each a real click or key on what a player presses, the place read from the
// screen's own node: the splash screen only runs on (it loads by itself), the menu's speed mode button, the postcard of
// the furthest open city, the story's Escape while its skip hint shows and else its Next button, the new topping's
// first button, on the level start the first topping no van has into the first empty van and then the Next button, and
// on a name screen the name typed and its done button, and on the box that offers the mode's saved game its button that
// starts the mode again (saveGameNoContinue), so a game is played from its start.
// A screen still coming in is waited for; the summary has no move.
static std::string ScreenMove(pf::PizzaFrenzy* theApp)
{
	std::string aScreen = screenName(theApp);
	if (aScreen == "splash")
		return waitLine(WAIT_UPDATES);
	if (aScreen != "menu" && aScreen != "citySelect" && aScreen != "story" && aScreen != "summary"
		&& aScreen != "newTopping" && aScreen != "levelStart" && aScreen != "designer" && aScreen != "editor"
		&& aScreen != "pizzaName" && aScreen != "newPlayer" && aScreen != "restoreGame")
		return "null";
	if (theApp->mScreens->isBusy())
		return waitLine(WAIT_UPDATES);
	if (aScreen == "designer")
		return DesignerMove(theApp);
	if (aScreen == "editor")
		return EditorMove(theApp);
	if (aScreen == "pizzaName")
		return NameMove(theApp->mPizzaNameScreen, BOT_PIZZA_NAME);
	if (aScreen == "newPlayer")
		return NameMove(theApp->mNewUserEntry, BOT_PLAYER_NAME);
	if (aScreen == "restoreGame")
		return nodeMove("startAgain", actionButton(theApp->mRestoreGameConfirm, "saveGameNoContinue"));
	engine::GenericScreen* aShown = shownScreen(theApp->mScreens);
	if (aScreen == "menu")
	{
		if (aShown != theApp->mMainMenu)
			return "null";
		// the button of the mode botmode named, else the mode the game was last in (the speed mode's from launch)
		if (gBotMode == BOT_EDITOR)
			return nodeMove("editPizza", theApp->mMainMenu->getNode("editPizzaButton"));
		int aMode = gBotMode >= 0 ? gBotMode : theApp->mGameMode;
		if (aMode == pf::PizzaFrenzy::MODE_MEMORY)
			return nodeMove("memoryMode", theApp->mMainMenu->getNode("NewGame"));
		if (aMode == pf::PizzaFrenzy::MODE_CONCENTRATION)
			return nodeMove("concentrationMode", theApp->mMainMenu->getNode("ConcentrationGame"));
		return nodeMove("speedMode", theApp->mMainMenu->getNode("SpeedGame"));
	}
	if (aScreen == "citySelect")
	{
		int aCity = theApp->mUserGameData->getMaxCity();
		if (aCity >= (int)theApp->mCities.size())
			aCity = (int)theApp->mCities.size() - 1;
		return nodeMove("city", theApp->mCitySelect->getPostcardButton(aCity));
	}
	if (aScreen == "story" || aScreen == "summary")
	{
		engine::Node* aSkip = theApp->mStoryScreen->getNode("skipString");
		if (aShown != theApp->mStoryScreen)
			return "null";
		if (aSkip != nullptr && aSkip->isVisible())
			return inputLine("skipStory", "", keyInput("Escape"));
		return nodeMove("next", theApp->mStoryScreen->getNode("continueButton"));
	}
	if (aScreen == "newTopping")
	{
		engine::Container* aButtons =
			static_cast<engine::Container*>(theApp->mNewToppingScreen->getNode("leftButtons"));
		if (aButtons->mChildren.empty())
			return waitLine(WAIT_UPDATES);
		return nodeMove("newTopping", aButtons->mChildren.front());
	}
	pf::LevelStartScreen* aStart = theApp->mLevelStartScreen;
	engine::Node* aDone = aStart->getNode("doneButton");
	if (aDone != nullptr && aDone->isEnabled())
		return nodeMove("levelStart", aDone);
	if (aStart->isHolding())
	{
		int aVan = aStart->getFirstEmptyVan();
		if (aVan < 0)
			return "null";
		char aName[32];
		std::snprintf(aName, sizeof(aName), "vanButton%d", aVan);
		return nodeMove("van", aStart->getNode(aName));
	}
	return nodeMove("topping", aStart->findFreeToppingButton());
}

// The bot's move in the bonus round, from the state alone as a player plays it: the introduction is skipped with a
// click; with a pizza in, the first topping of the target that the player has not put yet (the target's toppings in
// order, the ones of a kind matched to the ones put of that kind in order) is wanted: its button is clicked when
// another topping or none is picked, and then the point of the player's pizza where the target has it. Every other
// phase waits.
static std::string BonusMove(pf::BonusGame* theGame)
{
	std::string aPhase = modePhase(theGame);
	if (aPhase == "intro")
		return moveLine("skipIntro", "", engine::Point((float)SCREEN_CENTER_X, (float)SCREEN_CENTER_Y));
	if (aPhase != "play" || !theGame->isPlaying())
		return waitLine(WAIT_UPDATES);
	std::map<pf::Topping*, int> aPlacedCount;
	for (const engine::Ref<pf::PizzaTopping>& aPlaced : theGame->getUserToppings())
		aPlacedCount[aPlaced->getTopping()]++;
	std::map<pf::Topping*, int> aSeen;
	pf::PizzaTopping* aWanted = nullptr;
	for (const engine::Ref<pf::PizzaTopping>& aTarget : theGame->getTargetToppings())
	{
		if (aSeen[aTarget->getTopping()]++ >= aPlacedCount[aTarget->getTopping()])
		{
			aWanted = aTarget;
			break;
		}
	}
	if (aWanted == nullptr)
		return waitLine(WAIT_UPDATES);
	std::string aDetail = ",\"topping\":" + jsonText(toppingName(aWanted->getTopping()));
	if (theGame->getSelectedTopping() != aWanted->getTopping())
	{
		pf::ToppingButton* aButton = theGame->getButton(aWanted->getTopping());
		if (aButton == nullptr || aButton->getNumber() <= 0)
			return waitLine(WAIT_UPDATES);
		return moveLine("topping", aDetail, nodeCenter(aButton));
	}
	return moveLine("place", aDetail, pizzaPoint(theGame, aWanted));
}

// The bot's next move (the line after "MOVE "), from the state alone: it reads the game and never changes it, and it
// draws from no generator. In the level's introduction it clicks to skip it. In play, with no order picked it takes a
// tip that waits for its click, else picks the customer with the least wait left; with an order picked it picks more
// customers of the same topping while the pick limit allows, then clicks a kitchen that makes the topping. Customers of
// the special events are picked like the others, except that an event the police take (no pizza wanted) is picked
// alone and sent to the police's house instead of a kitchen, and is never picked with other orders. In a level with
// nothing to click (a lull, the blimp's flight, the level's end) it waits; the screens around play are ScreenMove's.
std::string PizzaFrenzyApp::TestMove()
{
	pf::PizzaFrenzy* anApp = pf::PizzaFrenzy::instance();
	pf::GameMode* aMode = anApp->mMode;
	if (pf::BonusGame* aBonus = dynamic_cast<pf::BonusGame*>(aMode))
	{
		if (dynamic_cast<pf::BonusGameState*>(anApp->mStates.getState()) != nullptr)
			return BonusMove(aBonus);
	}
	if (aMode == nullptr || dynamic_cast<pf::GameState*>(anApp->mStates.getState()) == nullptr)
		return ScreenMove(anApp);
	std::string aPhase = modePhase(aMode);
	if (aPhase == "intro")
		return moveLine("skipIntro", "", engine::Point((float)SCREEN_CENTER_X, (float)SCREEN_CENTER_Y));
	if (aPhase != "play")
		return waitLine(WAIT_UPDATES);
	std::vector<WaitingCustomer> aCustomers = waitingCustomers(aMode);
	const std::vector<engine::Ref<pf::CustomerTile>>& aPicks = aMode->getPicks();
	char aBuf[128];
	if (aPicks.empty())
	{
		for (const engine::Ref<pf::TipPopup>& aTip : aMode->getMap()->mTips)
		{
			if (!aTip->isWaiting())
				continue;
			const engine::Rect* aBounds = aTip->getBounds();
			engine::Point aPoint((aBounds->left + aBounds->right) / 2.0f, (aBounds->top + aBounds->bottom) / 2.0f);
			std::snprintf(aBuf, sizeof(aBuf), ",\"amount\":%d", aTip->getAmount());
			return moveLine("tip", aBuf, aPoint);
		}
	}
	// the customer to pick: the one with the least wait left (the first of them when two wait as long) among those that
	// take a click, for the topping of the orders picked when some are
	pf::Topping* aTopping = nullptr;
	if (!aPicks.empty())
	{
		pf::Order* aPickedOrder = aPicks.front()->getOrder();
		if (aPickedOrder == nullptr)
			return waitLine(WAIT_UPDATES);
		aTopping = aPickedOrder->mTopping;
	}
	const WaitingCustomer* aBest = nullptr;
	bool aCanPickMore = aPicks.empty() || (int)aPicks.size() < pickLimit(anApp->mGameStats, aTopping);
	// the memory mode serves the customers in the order they came: the next one to pick is the oldest not picked yet
	// (the police's own customer, whose order is none of the list's, first), and when it cannot be picked now none is
	const WaitingCustomer* anOldest = nullptr;
	bool anInOrder = anApp->mGameMode == pf::PizzaFrenzy::MODE_MEMORY;
	if (anInOrder)
	{
		for (const WaitingCustomer& aCustomer : aCustomers)
		{
			if (aCustomer.mPicked)
				continue;
			int anAge = aCustomer.mAge;
			if (anAge < 0 && aCustomer.mOrder->mCharacter->mSpecial == pf::Character::SPECIAL_POLICE)
				anAge = -1;
			else if (anAge < 0)
				continue;
			if (anOldest == nullptr || anAge < anOldest->mAge)
				anOldest = &aCustomer;
		}
	}
	for (const WaitingCustomer& aCustomer : aCustomers)
	{
		if (!aCanPickMore || aCustomer.mPicked || !aCustomer.mClickable)
			continue;
		if (anInOrder && &aCustomer != anOldest)
			continue;
		if (!aCustomer.mOrder->asksForPizza() && !aPicks.empty())
			continue;
		if (aTopping != nullptr && aCustomer.mOrder->mTopping != aTopping)
			continue;
		if (aBest == nullptr || aCustomer.mPopup->mWaitLeft < aBest->mPopup->mWaitLeft)
			aBest = &aCustomer;
	}
	if (aBest != nullptr)
	{
		std::snprintf(aBuf, sizeof(aBuf), ",\"cell\":[%d,%d],\"topping\":", aBest->mTile->mCell.mFirst,
			aBest->mTile->mCell.mSecond);
		return moveLine("pick", aBuf + jsonText(toppingName(aBest->mOrder->mTopping)), aBest->mPoint);
	}
	if (aPicks.empty())
		return waitLine(WAIT_UPDATES);
	if (!aPicks.front()->getOrder()->asksForPizza())
	{
		pf::CustomerTile* aPoliceTile = aMode->getMap()->findCustomerTileBySpecial(pf::Character::SPECIAL_POLICE);
		pf::PizzaPopup* aPolice = aPoliceTile == nullptr ? nullptr : aPoliceTile->getOccupant();
		if (aPolice == nullptr || aPolice->getKind() != pf::POPUP_POLICE || !aPoliceTile->isClickable())
			return waitLine(WAIT_UPDATES);
		std::snprintf(aBuf, sizeof(aBuf), ",\"cell\":[%d,%d]", aPoliceTile->mCell.mFirst, aPoliceTile->mCell.mSecond);
		return moveLine("police", aBuf, popupCenter(aPoliceTile, aPolice));
	}
	for (const engine::Ref<pf::Tile>& aTile : aMode->getMap()->mKitchens)
	{
		pf::KitchenTile* aKitchen = static_cast<pf::KitchenTile*>(aTile.get());
		pf::PizzaPopup* aPopup = aKitchen->getOccupant();
		if (aPopup == nullptr || !aKitchen->isClickable() || aKitchen->getTopping() != aTopping)
			continue;
		std::snprintf(aBuf, sizeof(aBuf), ",\"cell\":[%d,%d],\"topping\":", aKitchen->mCell.mFirst,
			aKitchen->mCell.mSecond);
		return moveLine("send", aBuf + jsonText(toppingName(aTopping)), popupCenter(aKitchen, aPopup));
	}
	return waitLine(WAIT_UPDATES);
}

// docs/TEST_COMMANDS.md, "memreport": the framework answers the command and counts its own pictures and the mixer's
// songs; what the engine holds beside them is the image table's 32-bit pixels and the sound system's decoded samples
void PizzaFrenzyApp::AddMemReportHolders(Platform::MemReport* theReport)
{
	size_t aBytes = 0;
	int aCount = engine::App::instance()->getImageMemory(&aBytes);
	theReport->mImageCount += aCount;
	theReport->mImagePixelBytes += aBytes;
	engine::HostSoundSystem* aSounds = dynamic_cast<engine::HostSoundSystem*>(engine::App::getSoundSystem());
	if (aSounds != nullptr)
	{
		theReport->mSoundCount += aSounds->getSoundMemory(&aBytes);
		theReport->mSoundBytes += aBytes;
	}
}

bool PizzaFrenzyApp::HandleScriptCommand(const ScriptCommand& theCommand)
{
	const std::string& aCmd = theCommand.mCommand;
	const std::string& anArgs = theCommand.mArgs;
	auto ok = [&]() { std::printf("OK %s%s%s\n", aCmd.c_str(), anArgs.empty() ? "" : " ", anArgs.c_str()); };
	auto error = [&](const char* theWhy) { std::printf("ERROR %s: %s\n", aCmd.c_str(), theWhy); };
	bool aKnown = aCmd == "state" || aCmd == "move" || aCmd == "goto" || aCmd == "level" || aCmd == "unlock"
		|| aCmd == "set" || aCmd == "win" || aCmd == "lose" || aCmd == "seed" || aCmd == "botmode";
	if (!aKnown)
		return SexyAppBase::HandleScriptCommand(theCommand);
	pf::PizzaFrenzy* anApp = pf::PizzaFrenzy::instance();
	if (aCmd == "botmode")
	{
		// the bot's choice alone, so it can be sent before the game has started
		int aMode = -1;
		for (int i = 0; i < 3; i++)
			if (anArgs == MODE_NAMES[i])
				aMode = i;
		if (anArgs == "editor")
			aMode = BOT_EDITOR;
		if (aMode < 0)
			error("botmode speed|memory|concentration|editor");
		else
			gBotMode = aMode, ok();
	}
	else if (anApp == nullptr || !mEngineRunning || anApp->mStates.getState() == nullptr)
	{
		error("the game has not started");
	}
	else if (aCmd == "state")
	{
		std::printf("STATE %s\n", TestState().c_str());
	}
	else if (aCmd == "move")
	{
		std::printf("MOVE %s\n", TestMove().c_str());
	}
	else if (aCmd == "seed")
	{
		char* anEnd = nullptr;
		unsigned long aSeed = std::strtoul(anArgs.c_str(), &anEnd, 10);
		if (anArgs.empty() || *anEnd != '\0')
			error("seed <n>");
		else
			engine::crtSrand((unsigned int)aSeed), ok();
	}
	else if (aCmd == "goto")
	{
		// the menu state's own actions and the game's own functions: the screens open as the menu's buttons open them
		bool aMenu = dynamic_cast<pf::MenuState*>(anApp->mStates.getState()) != nullptr;
		std::string aScreen = lowerCase(anArgs);
		if (aScreen == "designer")
		{
			// the pizza designer's introduction, which the new topping screen shows when the player has five toppings:
			// from the menu, the new topping state comes with the introduction on its screen
			bool aNewTopping = dynamic_cast<pf::NewToppingState*>(anApp->mStates.getState()) != nullptr;
			if (!aMenu && !aNewTopping)
				error("only from the main menu or while the new topping screen is shown");
			else
			{
				if (aMenu)
					anApp->mStates.changeState(new pf::NewToppingState(anApp));
				anApp->mScreens->showScreen(anApp->mDesignerScreen, 0);
				ok();
			}
		}
		else if (!aMenu)
			error("only from the main menu or the screens it opens");
		else if (aScreen == "menu")
			anApp->screenAction("highScoreDone"), ok();
		else if (aScreen == "highscores")
			anApp->screenAction("showHighScores"), ok();
		else if (aScreen == "options")
			anApp->screenAction("options"), ok();
		else if (aScreen == "toppings")
			anApp->screenAction("viewToppingBook"), ok();
		else if (aScreen == "editor")
			anApp->screenAction("editPizza"), ok();
		else if (aScreen == "cityselect")
		{
			anApp->mGameMode = pf::PizzaFrenzy::MODE_SPEED;
			anApp->showCitySelect();
			ok();
		}
		else if (aScreen == "speed" || aScreen == "memory" || aScreen == "concentration")
		{
			// the first city's first level, or the game left in the mode when it has one (as its restore does)
			int aModeNumber = modeNumber(aScreen);
			anApp->mGameMode = aModeNumber;
			anApp->startGame(aModeNumber, 0);
			ok();
		}
		else
			error("menu, citySelect, highscores, options, toppings, editor, designer, speed, memory or concentration");
	}
	else if (aCmd == "level")
	{
		// "level <day>" or "level <city> <level>" (from 1): the mode's saved game is put at the level, with no score,
		// and the game starts from it through PizzaFrenzy::startGame, which then runs the story, the new toppings and
		// the level start as for any level
		bool aMenu = dynamic_cast<pf::MenuState*>(anApp->mStates.getState()) != nullptr;
		int aFirst = 0;
		int aSecond = 0;
		int aModeNumber = pf::PizzaFrenzy::MODE_SPEED;
		std::string aNumbers = anArgs;
		for (int i = 0; i < 3; i++)
		{
			std::string aName = MODE_NAMES[i];
			if (lowerCase(anArgs).compare(0, aName.size(), aName) == 0)
			{
				aModeNumber = i;
				aNumbers = anArgs.substr(aName.size());
			}
		}
		int aCount = std::sscanf(aNumbers.c_str(), "%d %d", &aFirst, &aSecond);
		int aCity = -1;
		int aLevel = -1;
		if (aCount == 2)
		{
			aCity = aFirst - 1;
			aLevel = aSecond - 1;
		}
		else if (aCount == 1)
		{
			int aDay = aFirst - 1;
			for (size_t i = 0; i < anApp->mCities.size() && aDay >= 0; i++)
			{
				if (aDay < (int)anApp->mCities[i]->mLevels.size())
				{
					aCity = (int)i;
					aLevel = aDay;
					break;
				}
				aDay -= (int)anApp->mCities[i]->mLevels.size();
			}
		}
		if (aCount < 1)
			error("level [speed|memory|concentration] <day> or <city> <level>");
		else if (!aMenu)
			error("only from the main menu (goto menu first)");
		else if (aCity < 0 || aCity >= (int)anApp->mCities.size() || aLevel < 0
			|| aLevel >= (int)anApp->mCities[aCity]->mLevels.size())
			error("no such level");
		else
		{
			int aDay = 1 + aLevel;
			for (int i = 0; i < aCity; i++)
				aDay += (int)anApp->mCities[i]->mLevels.size();
			pf::ModeRecord& aRecord = anApp->mUserGameData->mModes[aModeNumber];
			aRecord.mLastCity = aCity;
			aRecord.mLastLevel = aLevel;
			aRecord.mLastDay = aDay;
			aRecord.mLastScore = 0;
			aRecord.mElapsedTime = 0;
			anApp->mGameMode = aModeNumber;
			anApp->startGame(aModeNumber, aCity);
			ok();
		}
	}
	else if (aCmd == "unlock")
	{
		// as the progress would leave the player's data: every city open (the last one the player got to is the
		// last), and every topping the player has not got given at level 0, as the new topping screen gives it
		if (anArgs != "all")
			error("unlock all");
		else if (anApp->mUserGameData == nullptr || anApp->mTileManifest == nullptr)
			error("no player's data yet (the game is still loading)");
		else
		{
			pf::UserGameData* aData = anApp->mUserGameData;
			if (!anApp->mCities.empty())
				aData->setMaxCity((int)anApp->mCities.size() - 1);
			std::map<std::string, engine::Ref<pf::Topping>>& aToppings = anApp->mTileManifest->getToppings();
			for (auto& aTopping : aToppings)
			{
				if (aData->getToppingLevel(aTopping.second) < 0)
					aData->mToppingLevels[aTopping.second] = 0;
				aData->removeNewTopping(aTopping.second);
			}
			aData->save();
			ok();
		}
	}
	else if (aCmd == "set")
	{
		char aName[32] = "";
		int aValue = 0;
		pf::GameMode* aMode = anApp->mMode;
		if (std::sscanf(anArgs.c_str(), "%31s %d", aName, &aValue) != 2)
			error("set <name> <value>");
		else if (aMode == nullptr)
			error("no level is being played");
		else
		{
			std::string aKey = lowerCase(aName);
			pf::GameStats* aStats = aMode->getStats();
			if (aKey == "score")
				aStats->mScore = aValue, ok();
			else if (aKey == "tips")
				aStats->mTipMoney = std::max(aValue, 0), ok();
			else if (aKey == "money")
				aStats->mTipsEarned = aValue, ok();
			else if (aKey == "happiness")
				aMode->addHappiness(aValue - aStats->mHappiness), ok();
			else if (aKey == "orderstogo")
				aStats->mOrderCount = aValue, ok();
			else
				error("score, tips, money, happiness or ordersToGo");
		}
	}
	else if (aCmd == "win" || aCmd == "lose")
	{
		pf::GameMode* aMode = anApp->mMode;
		if (aMode == nullptr || std::string(modePhase(aMode)) != "play"
			|| dynamic_cast<pf::GameState*>(anApp->mStates.getState()) == nullptr)
			error("no level is being played");
		else
			aMode->endLevelForTest(aCmd == "win"), ok();
	}
	std::fflush(stdout);
	return true;
}

namespace
{
std::string bitsText(float theValue)
{
	int aBits = 0;
	std::memcpy(&aBits, &theValue, sizeof(aBits));
	return std::to_string(aBits);
}
}

void PizzaFrenzyApp::LogState()
{
	pf::PizzaFrenzy* anApp = pf::PizzaFrenzy::instance();
	std::vector<std::string> aState;
	aState.push_back(std::to_string((int)engine::crtRandState()));
	engine::State* aCurrent = anApp != nullptr ? anApp->mStates.getState() : nullptr;
	aState.push_back(aCurrent != nullptr ? typeid(*aCurrent).name() : "-");
	aState.push_back(anApp != nullptr ? std::to_string(anApp->mCurrentCity) : "-");
	aState.push_back(anApp != nullptr ? std::to_string(anApp->mCurrentLevel) : "-");
	pf::GameStats* aStats = anApp != nullptr ? anApp->mGameStats.get() : nullptr;
	auto aStat = [aStats](int theValue) { return aStats != nullptr ? std::to_string(theValue) : std::string("-"); };
	aState.push_back(aStat(aStats ? aStats->mScore : 0));
	aState.push_back(aStats ? bitsText(aStats->mElapsedTime) : "-");
	aState.push_back(aStat(aStats ? aStats->mPizzasDelivered : 0));
	aState.push_back(aStat(aStats ? aStats->mPizzasOrdered : 0));
	aState.push_back(aStat(aStats ? aStats->mHappiness : 0));
	aState.push_back(aStat(aStats ? aStats->mCustomersServed : 0));
	aState.push_back(aStat(aStats ? aStats->mTipMoney : 0));
	aState.push_back(aStat(aStats ? aStats->mWavesStarted : 0));
	aState.push_back(aStat(aStats ? aStats->mComboLength : 0));
	pf::GameMode* aMode = anApp != nullptr ? dynamic_cast<pf::GameMode*>(anApp->mMode.get()) : nullptr;
	pf::GameMode::LoggedValues aValues = aMode != nullptr ? aMode->loggedValues() : pf::GameMode::LoggedValues();
	aState.push_back(aMode ? bitsText(aValues.mWaveTimer) : "-");
	aState.push_back(aMode ? bitsText(aValues.mSpawnTimer) : "-");
	aState.push_back(aMode ? bitsText(aValues.mDeliveryTimer) : "-");
	aState.push_back(aMode ? std::to_string(aValues.mWaveCustomersLeft) : "-");
	aState.push_back(aMode ? std::to_string(aValues.mOrdersBegun) : "-");
	aState.push_back(aMode ? std::to_string(aValues.mDeliveryStreak) : "-");
	static const char* const NAMES[] = {"rand", "state", "city", "level", "score", "elapsed", "delivered", "ordered",
		"happiness", "served", "tipmoney", "waves", "combo", "wavetimer", "spawntimer", "deliverytimer", "customersleft",
		"ordersbegun", "streak"};
	if (mLoggedState.size() != aState.size())
		mLoggedState.assign(aState.size(), std::string());
	for (size_t i = 0; i < aState.size(); i++)
	{
		if (mLoggedState[i] != aState[i])
			std::fprintf(mStateLog, "%d %s %s\n", mFrames, NAMES[i], aState[i].c_str());
	}
	mLoggedState = aState;
	std::fflush(mStateLog);
}
