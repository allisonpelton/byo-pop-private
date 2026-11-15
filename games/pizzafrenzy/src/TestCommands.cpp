// port: the standard test commands (docs/TEST_COMMANDS.md), reachable only from a test (-script, -commands, the web
// test page's command call). Each answers on stdout with one line: STATE {...}, MOVE {...} or MOVE null, OK
// <command>, or ERROR <command>: <reason>. A command never given changes nothing and draws no random number; "move"
// changes nothing either, and the bot draws from no generator. The game's notes list the commands and the state's
// keys (games/pizzafrenzy/docs/README.md, "Test commands").

#include "PizzaFrenzyApp.h"
#include "platform/MemReport.h"
#include "sprout/App.h"
#include "sprout/Random.h"
#include "sprout/ScreenMgr.h"
#include "sprout/host/SoundHost.h"
#include "game/CitySelectScreen.h"
#include "game/CustomerPopup.h"
#include "game/DialogState.h"
#include "game/GameState.h"
#include "game/LevelStartScreen.h"
#include "game/LevelStartState.h"
#include "game/MenuState.h"
#include "game/ModeStates.h"
#include "game/NewToppingScreen.h"
#include "game/NewToppingState.h"
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
		if (dynamic_cast<pf::GameState*>(aDialog->getNextState()) != nullptr)
			return "pause";
		return dialogName(theApp, aDialog->getShownScreen());
	}
	if (dynamic_cast<pf::StoryState*>(aState) != nullptr)
		return "story";
	if (dynamic_cast<pf::NewToppingState*>(aState) != nullptr)
		return "newTopping";
	if (dynamic_cast<pf::LevelStartState*>(aState) != nullptr)
		return "levelStart";
	if (dynamic_cast<pf::GameState*>(aState) != nullptr)
		return "game";
	if (dynamic_cast<pf::LevelSummaryState*>(aState) != nullptr)
		return "summary";
	return "other";
}

// where the mode's own states are: the introduction's titles, play, the prize blimp's flight and the level's end
const char* modePhase(pf::GameMode* theMode)
{
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
	bool anOver = aScreen == "summary" ||
		(aMode != nullptr && std::string(modePhase(aMode)) == "end");
	aState += std::string(",\"over\":") + (anOver ? "true" : "false");
	pf::GameStats* aStats = anApp->mGameStats;
	if (aStats != nullptr)
	{
		std::snprintf(aBuf, sizeof(aBuf), ",\"score\":%d,\"tips\":%d,\"tipGoal\":%d,\"money\":%d,\"happiness\":%d,"
			"\"ordersToGo\":%d,\"timer\":%.2f,\"delivered\":%d,\"ordered\":%d,\"combo\":%d", aStats->mScore,
			aStats->mTipMoney, aStats->mLevelGoal, aStats->mTipsEarned, aStats->mHappiness, aStats->mOrderCount,
			aStats->mElapsedTime, aStats->mPizzasDelivered, aStats->mPizzasOrdered, aStats->mComboLength);
		aState += aBuf;
	}
	if (aMode == nullptr || aMode->getMap() == nullptr)
		return aState + "}";
	std::snprintf(aBuf, sizeof(aBuf), ",\"phase\":%s,\"picks\":%d", jsonText(modePhase(aMode)).c_str(),
		aMode->getPickCount());
	aState += aBuf;
	aState += ",\"board\":{\"customers\":[";
	bool aFirst = true;
	for (const WaitingCustomer& aCustomer : waitingCustomers(aMode))
	{
		std::snprintf(aBuf, sizeof(aBuf), "%s{\"cell\":[%d,%d],\"x\":%d,\"y\":%d,\"topping\":%s,\"picked\":%s,"
			"\"wait\":%.2f,\"clickable\":%s,\"event\":%s,\"chameleon\":%s}", aFirst ? "" : ",",
			aCustomer.mTile->mCell.mFirst, aCustomer.mTile->mCell.mSecond, roundedPoint(aCustomer.mPoint.x),
			roundedPoint(aCustomer.mPoint.y), jsonText(toppingName(aCustomer.mOrder->mTopping)).c_str(),
			aCustomer.mPicked ? "true" : "false", aCustomer.mPopup->mWaitLeft,
			aCustomer.mClickable ? "true" : "false", aCustomer.mOrder->mEvent != nullptr ? "true" : "false",
			aCustomer.mPopup->mChameleon ? "true" : "false");
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

// The bot's move on the screens before play, each a real click or key on what a player presses, the place read from the
// screen's own node: the splash screen only runs on (it loads by itself), the menu's speed mode button, the postcard of
// the furthest open city, the story's Escape while its skip hint shows and else its Next button, the new topping's
// first button, and on the level start the first topping no van has into the first empty van and then the Next button.
// A screen still coming in is waited for; the summary has no move.
static std::string ScreenMove(pf::PizzaFrenzy* theApp)
{
	std::string aScreen = screenName(theApp);
	if (aScreen == "splash")
		return waitLine(WAIT_UPDATES);
	if (aScreen == "summary" || (aScreen != "menu" && aScreen != "citySelect" && aScreen != "story"
		&& aScreen != "newTopping" && aScreen != "levelStart"))
		return "null";
	if (theApp->mScreens->isBusy())
		return waitLine(WAIT_UPDATES);
	engine::GenericScreen* aShown = shownScreen(theApp->mScreens);
	if (aScreen == "menu")
	{
		if (aShown != theApp->mMainMenu)
			return "null";
		return nodeMove("speedMode", theApp->mMainMenu->getNode("SpeedGame"));
	}
	if (aScreen == "citySelect")
	{
		int aCity = theApp->mUserGameData->getMaxCity();
		if (aCity >= (int)theApp->mCities.size())
			aCity = (int)theApp->mCities.size() - 1;
		return nodeMove("city", theApp->mCitySelect->getPostcardButton(aCity));
	}
	if (aScreen == "story")
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

// The bot's next move (the line after "MOVE "), from the state alone: it reads the game and never changes it, and it
// draws from no generator. In the level's introduction it clicks to skip it. In play, with no order picked it takes a
// tip that waits for its click, else picks the customer with the least wait left; with an order picked it picks more
// customers of the same topping while the pick limit allows, then clicks a kitchen that makes the topping. Customers of
// the special events are left alone (their pizzas are not served yet). In a level with nothing to click (a lull, the
// blimp's flight, the level's end) it waits; the screens around play are ScreenMove's.
std::string PizzaFrenzyApp::TestMove()
{
	pf::PizzaFrenzy* anApp = pf::PizzaFrenzy::instance();
	pf::GameMode* aMode = anApp->mMode;
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
	for (const WaitingCustomer& aCustomer : aCustomers)
	{
		if (!aCanPickMore || aCustomer.mPicked || !aCustomer.mClickable || aCustomer.mOrder->mEvent != nullptr)
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
		|| aCmd == "set" || aCmd == "win" || aCmd == "lose" || aCmd == "seed";
	if (!aKnown)
		return SexyAppBase::HandleScriptCommand(theCommand);
	pf::PizzaFrenzy* anApp = pf::PizzaFrenzy::instance();
	if (anApp == nullptr || !mEngineRunning || anApp->mStates.getState() == nullptr)
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
		if (!aMenu)
			error("only from the main menu or the screens it opens");
		else if (aScreen == "menu")
			anApp->screenAction("highScoreDone"), ok();
		else if (aScreen == "highscores")
			anApp->screenAction("showHighScores"), ok();
		else if (aScreen == "options")
			anApp->screenAction("options"), ok();
		else if (aScreen == "toppings")
			anApp->screenAction("viewToppingBook"), ok();
		else if (aScreen == "cityselect")
		{
			anApp->mGameMode = pf::PizzaFrenzy::MODE_SPEED;
			anApp->showCitySelect();
			ok();
		}
		else if (aScreen == "speed")
		{
			// the first city's first level, or the game left in the mode when it has one (as its restore does)
			anApp->mGameMode = pf::PizzaFrenzy::MODE_SPEED;
			anApp->startGame(pf::PizzaFrenzy::MODE_SPEED, 0);
			ok();
		}
		else
			error("menu, citySelect, highscores, options, toppings or speed");
	}
	else if (aCmd == "level")
	{
		// "level <day>" or "level <city> <level>" (from 1): the mode's saved game is put at the level, with no score,
		// and the game starts from it through PizzaFrenzy::startGame, which then runs the story, the new toppings and
		// the level start as for any level
		bool aMenu = dynamic_cast<pf::MenuState*>(anApp->mStates.getState()) != nullptr;
		int aFirst = 0;
		int aSecond = 0;
		int aCount = std::sscanf(anArgs.c_str(), "%d %d", &aFirst, &aSecond);
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
			error("level <day> or level <city> <level>");
		else if (!aMenu)
			error("only from the main menu (goto menu first)");
		else if (aCity < 0 || aCity >= (int)anApp->mCities.size() || aLevel < 0
			|| aLevel >= (int)anApp->mCities[aCity]->mLevels.size())
			error("no such level");
		else if (anApp->mCities[aCity]->mLevels[aLevel]->mBonusRound)
			error("a bonus round is not played yet");
		else
		{
			int aDay = 1 + aLevel;
			for (int i = 0; i < aCity; i++)
				aDay += (int)anApp->mCities[i]->mLevels.size();
			pf::ModeRecord& aRecord = anApp->mUserGameData->mModes[pf::PizzaFrenzy::MODE_SPEED];
			aRecord.mLastCity = aCity;
			aRecord.mLastLevel = aLevel;
			aRecord.mLastDay = aDay;
			aRecord.mLastScore = 0;
			aRecord.mElapsedTime = 0;
			anApp->mGameMode = pf::PizzaFrenzy::MODE_SPEED;
			anApp->startGame(pf::PizzaFrenzy::MODE_SPEED, aCity);
			ok();
		}
	}
	else if (aCmd == "unlock")
	{
		// as the progress would leave the player's data: every city open (the last one the player got to is the
		// last), and every topping the player has not got given at level 0, as the new topping screen gives it
		if (anArgs != "all")
			error("unlock all");
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
