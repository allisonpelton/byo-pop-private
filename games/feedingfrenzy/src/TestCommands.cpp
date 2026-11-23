// port: the standard test commands (docs/TEST_COMMANDS.md), reachable only from a test (-script, -commands, the web
// test page's command call). Each answers on stdout with one line: STATE {...}, MOVE {...} or MOVE null, OK
// <command>, or ERROR <command>: <reason>. A command never given changes nothing and draws no random number; "move"
// changes nothing either, and the bot draws from no generator. The game's notes list the commands and the state's
// keys (games/feedingfrenzy/docs/README.md, "Test commands").

#include "FeedingFrenzyApp.h"
#include "game/FishBase.h"
#include "game/FishFood.h"
#include "game/PlayerFish.h"
#include "game/StageMgr.h"
#include "platform/MemReport.h"
#include "sprout/App.h"
#include "sprout/Random.h"
#include "sprout/host/SoundHost.h"

#include <cmath>
#include <cstdio>
#include <string>

using namespace Sexy;
namespace ff = feedingfrenzy;

namespace
{
// the updates a move waits after its last step: the click is handled in the next update and the screen follows it
const int MOVE_UPDATES = 4;
// the updates a move waits while a screen comes in, or the game loads
const int WAIT_UPDATES = 10;
// the point the player fish puts the pointer back to after every reading (the pointer's rest point in a stage)
const int CURSOR_REST_X = 325;
const int CURSOR_REST_Y = 265;
// how far from the rest point a steering move puts the pointer at most, in pixels
const float MOUSE_STEP_LIMIT = 60.0f;
// how much of the speed that is missing the pointer's step asks for, per pixel of speed
const float MOUSE_STEP_PER_SPEED = 0.35f;
// the speed the bot asks the fish to swim at towards a fish it is going to eat, in pixels a second
const float CHASE_SPEED = 260.0f;
// how near a fish that can eat the player pushes it away, in pixels, and how fast at the player's feet
const float DANGER_DISTANCE = 170.0f;
const float DANGER_SPEED = 520.0f;

// The name the bot types for a score that made the list
const char* const BOT_PLAYER_NAME = "BOT";

// Whether a game has been started since the launch, which the state's "over" needs: the game is over once the menu is
// back after one
bool gGameStarted = false;

std::string numberText(float theValue)
{
	char aBuf[32];
	std::snprintf(aBuf, sizeof(aBuf), "%.1f", theValue);
	return aBuf;
}

std::string jsonText(const std::string& theText)
{
	std::string aResult = "\"";
	for (size_t i = 0; i < theText.size(); i++)
	{
		char c = theText[i];
		if (c == '"' || c == '\\')
		{
			aResult += '\\';
			aResult += c;
		}
		else if ((unsigned char)c >= 0x20)
			aResult += c;
	}
	return aResult + "\"";
}

const char* stageStateName(int theState)
{
	switch (theState)
	{
	case ff::STATE_START_SCREEN:
		return "start";
	case ff::STATE_LOADING:
		return "loading";
	case ff::STATE_PAUSED:
		return "paused";
	case ff::STATE_PLAYING:
		return "playing";
	case ff::STATE_MERMAID:
		return "mermaid";
	case ff::STATE_ENDING:
		return "ending";
	case ff::STATE_END_SCREEN:
		return "end";
	}
	return "unknown";
}

const char* stageTypeName(int theType)
{
	switch (theType)
	{
	case ff::STAGE_NORMAL:
		return "normal";
	case ff::STAGE_TIMED_NORMAL:
		return "timednormal";
	case ff::STAGE_TIMED_BONUS:
		return "timedbonus";
	case ff::STAGE_BONUS:
		return "bonus";
	case ff::STAGE_SURVIVAL:
		return "survival";
	case ff::STAGE_MENU:
		return "menu";
	}
	return "unknown";
}

// The name of the screen the game is on: the controller's state, and in a stage what the stage is doing; a popup on
// top of them (options, the high scores, the pause screen) is the screen
std::string screenName()
{
	ff::FishFood* aFood = ff::FishFood::instance();
	if (aFood == nullptr)
		return "none";
	std::string aPopup = aFood->getPopupName();
	std::string aState = aFood->getStateName();
	if (aState == "stage")
	{
		ff::StageMgr* aStage = aFood->getStageMgr();
		if (aStage != nullptr)
		{
			std::string aShown = aStage->getScreenName();
			if (!aShown.empty())
				return aShown;
			if (aPopup == "pause")
				return aPopup;
			return std::string("stage-") + stageStateName(aStage->getState());
		}
	}
	if (!aPopup.empty())
		return aPopup;
	return aState;
}

// the centre of a node on the 640x480 screen: its bounds (in its parent's points, as the mouse's hit test takes them)
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

int roundedPoint(float theValue)
{
	return (int)std::floor(theValue + 0.5f);
}

// a move of the input steps, which waits the updates after the last
std::string inputLine(const char* theWhat, const std::string& theDetail, const std::string& theInput, int theUpdates)
{
	char aBuf[64];
	std::snprintf(aBuf, sizeof(aBuf), ",\"updates\":%d}", theUpdates);
	return std::string("{\"what\":\"") + theWhat + "\"" + theDetail + ",\"input\":" + theInput + aBuf;
}

// a move that only lets the game run on: a screen is coming in, or the game is loading
std::string waitLine(int theUpdates)
{
	char aBuf[128];
	std::snprintf(aBuf, sizeof(aBuf), "{\"what\":\"wait\",\"input\":[{\"wait\":%d}],\"updates\":%d}", theUpdates,
		theUpdates);
	return aBuf;
}

// the input of a click on a point: the pointer's move and then its click
std::string clickInput(const engine::Point& thePoint)
{
	char aBuf[128];
	int x = roundedPoint(thePoint.x);
	int y = roundedPoint(thePoint.y);
	std::snprintf(aBuf, sizeof(aBuf), "[{\"move\":[%d,%d]},{\"click\":[%d,%d]}]", x, y, x, y);
	return aBuf;
}

// a move that clicks a node of the shown screen, which has to be there, shown and enabled
std::string nodeMove(const char* theWhat, engine::Node* theNode)
{
	if (theNode == nullptr || !theNode->isVisible() || !theNode->isEnabled())
		return waitLine(WAIT_UPDATES);
	return inputLine(theWhat, "", clickInput(nodeCenter(theNode)), MOVE_UPDATES);
}

// The first of the names that is a node of the screen, shown and enabled
engine::Node* firstButton(engine::ScreenLayout* theScreen, const char* const* theNames, int theCount)
{
	for (int i = 0; i < theCount; i++)
	{
		engine::Node* aNode = theScreen->getNode(theNames[i]);
		if (aNode != nullptr && aNode->isVisible() && aNode->isEnabled())
			return aNode;
	}
	return nullptr;
}

// The buttons that go on from a screen, by name, the first that shows and is enabled being the one the bot presses
const char* const CONTINUE_BUTTONS[] = {"continueBtn", "doneBtn", "OK", "resumeBtn"};
const int CONTINUE_BUTTON_COUNT = 4;

// The fish the bot goes for: the nearest one the player can eat that is in play, on the screen or off it (the stage is
// larger than the screen, which follows the player)
ff::FishBase* chaseTarget(ff::StageMgr* theStage, ff::PlayerFish* thePlayer)
{
	ff::FishBase* aBest = nullptr;
	float aBestDistance = 0.0f;
	engine::Point aPlayerAt = thePlayer->getCentre();
	const std::vector<ff::FishBase*>& aFish = theStage->getFishList();
	for (size_t i = 0; i < aFish.size(); i++)
	{
		ff::FishBase* aVictim = aFish[i];
		if (aVictim == thePlayer || !aVictim->isInPlay() || aVictim->isEaten() || aVictim->isHidden() ||
		    !thePlayer->canEat(aVictim) || !aVictim->canBeEatenBy(thePlayer))
			continue;
		engine::Point aAt = aVictim->getCentre();
		aAt.subtract(aPlayerAt);
		float aDistance = aAt.length();
		if (aBest == nullptr || aDistance < aBestDistance)
		{
			aBest = aVictim;
			aBestDistance = aDistance;
		}
	}
	return aBest;
}

// The bot's move in a stage that is played: the pointer is moved from the rest point by how much the fish's speed
// is to change to go for the nearest fish it can eat
std::string playMove(ff::StageMgr* theStage)
{
	ff::PlayerFish* aPlayer = theStage->getPlayer();
	if (aPlayer == nullptr || aPlayer->isDying())
		return waitLine(1);
	ff::FishBase* aTarget = chaseTarget(theStage, aPlayer);
	engine::Point aWant(0.0f, 0.0f);
	std::string aDetail = ",\"target\":null";
	if (aTarget != nullptr)
	{
		aWant = aTarget->getCentre();
		aWant.subtract(aPlayer->getCentre());
		aWant.normalize();
		aWant.scale(CHASE_SPEED);
		char aBuf[96];
		std::snprintf(aBuf, sizeof(aBuf), ",\"target\":\"%s\"", aTarget->getTypeName().c_str());
		aDetail = aBuf;
	}
	// a fish that can eat the player pushes it away when it is near
	engine::Point aPlayerAt = aPlayer->getCentre();
	const std::vector<ff::FishBase*>& aFish = theStage->getFishList();
	for (size_t i = 0; i < aFish.size(); i++)
	{
		ff::FishBase* aThreat = aFish[i];
		if (aThreat == aPlayer || !aThreat->isInPlay() || aThreat->isEaten() || aThreat->isHidden() ||
		    !aThreat->canEat(aPlayer))
			continue;
		engine::Point aAway = aPlayerAt;
		aAway.subtract(aThreat->getCentre());
		float aDistance = aAway.length();
		if (aDistance < DANGER_DISTANCE && 0.0f < aDistance)
		{
			aAway.scale(DANGER_SPEED * (1.0f - aDistance / DANGER_DISTANCE) / aDistance);
			aWant.add(aAway);
		}
	}
	engine::Point aSpeed = aPlayer->getVelocity();
	aWant.subtract(aSpeed);
	aWant.scale(MOUSE_STEP_PER_SPEED);
	float aLength = aWant.length();
	if (MOUSE_STEP_LIMIT < aLength)
		aWant.scale(MOUSE_STEP_LIMIT / aLength);
	char aBuf[128];
	std::snprintf(aBuf, sizeof(aBuf), "[{\"move\":[%d,%d]}]", CURSOR_REST_X + roundedPoint(aWant.x),
		CURSOR_REST_Y + roundedPoint(aWant.y));
	return inputLine("swim", aDetail, aBuf, 1);
}

// The mode and the stage a command names, for "goto" and "level"
int modeNumber(const std::string& theName)
{
	if (theName == "normal" || theName == "newgame")
		return ff::GAME_MODE_NORMAL;
	if (theName == "timed" || theName == "timeattack")
		return ff::GAME_MODE_TIMED;
	return -1;
}
} // namespace

// port: the memreport test command: what the engine holds beside the framework's own pictures and sounds
void FeedingFrenzyApp::AddMemReportHolders(Platform::MemReport* theReport)
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

// The game's state as one JSON line: the screen the game is on, the game being played (its mode, stage, score and the
// fish left) and, in a stage, the stage's state and the fish in it, with their places on the screen (the screen's
// 640x480 pixels). timer is the seconds left of a stage that has a time.
std::string FeedingFrenzyApp::TestState()
{
	ff::FishFood* aFood = ff::FishFood::instance();
	std::string aState = "{\"screen\":" + jsonText(screenName());
	if (aFood == nullptr)
		return aState + "}";
	ff::GameSession* aSession = aFood->getSession();
	bool aPlaying = std::string(aFood->getStateName()) != "menu" && std::string(aFood->getStateName()) != "loading";
	if (aPlaying)
		gGameStarted = true;
	aState += std::string(",\"over\":") + (gGameStarted && !aPlaying ? "true" : "false");
	aState += std::string(",\"mode\":") + (aSession->getMode() == ff::GAME_MODE_TIMED ? "\"timed\"" : "\"normal\"");
	aState += ",\"furthest\":" + std::to_string(aFood->getScoreList(aSession->getMode()).getFurthestStage());
	if (aPlaying)
	{
		aState += ",\"level\":" + std::to_string(aFood->getStageIndex() + 1);
		aState += ",\"score\":" + std::to_string(aSession->getScore());
		aState += ",\"lives\":" + std::to_string(aSession->getLives());
		aState += ",\"continues\":" + std::to_string(aSession->getContinues());
	}
	ff::StageMgr* aStage = aFood->getStageMgr();
	if (aPlaying && aStage != nullptr && aStage->getType() != ff::STAGE_MENU)
	{
		aState += ",\"stage\":" + jsonText(aStage->getStageName());
		aState += std::string(",\"type\":\"") + stageTypeName(aStage->getType()) + "\"";
		aState += std::string(",\"phase\":\"") + stageStateName(aStage->getState()) + "\"";
		aState += ",\"timer\":" + numberText(aStage->getSecondsLeft());
		aState += ",\"eaten\":" + std::to_string(aStage->countEaten());
		aState += ",\"toEat\":" + std::to_string(aStage->countFishToEat());
		ff::PlayerFish* aPlayer = aStage->getPlayer();
		if (aPlayer != nullptr)
		{
			engine::Point aAt = aStage->worldToScreen(aPlayer->getCentre());
			aState += ",\"player\":{\"x\":" + numberText(aAt.x) + ",\"y\":" + numberText(aAt.y) + ",\"size\":" +
				numberText(aPlayer->getLevel()) + ",\"score\":" + numberText(aPlayer->getScore()) + ",\"state\":" +
				std::to_string(aPlayer->getState()) + ",\"chain\":" + std::to_string(aPlayer->getChainCount()) + "}";
		}
		aState += ",\"fish\":[";
		const std::vector<ff::FishBase*>& aFish = aStage->getFishList();
		bool aFirst = true;
		for (size_t i = 0; i < aFish.size(); i++)
		{
			ff::FishBase* aOne = aFish[i];
			if (aOne == aPlayer || !aOne->isInPlay())
				continue;
			engine::Point aAt = aStage->worldToScreen(aOne->getCentre());
			aState += aFirst ? "" : ",";
			aFirst = false;
			aState += "{\"type\":" + jsonText(aOne->getTypeName()) + ",\"x\":" + numberText(aAt.x) + ",\"y\":" +
				numberText(aAt.y) + ",\"level\":" + numberText(aOne->getLevel());
			aState += std::string(",\"seen\":") + (aStage->isFishOnScreen(aOne) ? "true" : "false");
			if (aPlayer != nullptr)
				aState += std::string(",\"edible\":") + (aPlayer->canEat(aOne) ? "true" : "false");
			aState += "}";
		}
		aState += "]";
	}
	return aState + "}";
}

// The bot's next move, as the input that makes it: on a menu or a screen the button a player presses to go on, in a
// stage the pointer's step towards the fish to eat. MOVE null when the game is not in a state with a move
std::string FeedingFrenzyApp::TestMove()
{
	ff::FishFood* aFood = ff::FishFood::instance();
	if (aFood == nullptr)
		return "null";
	std::string aScreen = screenName();
	engine::ScreenLayout* aClick = aFood->getClickScreen();
	std::string aState = aFood->getStateName();
	if (aState == "loading" || aState == "loadmap" || aState == "stageended" || aState == "nextstage")
		return waitLine(WAIT_UPDATES);
	if (aScreen == "entername" && aClick != nullptr && aClick->getNode("continueBtn") != nullptr)
	{
		// a score that made the list: the bot types its name and presses Done
		std::string aName = aFood->getEnteredName();
		std::string aWanted = BOT_PLAYER_NAME;
		if (aName == aWanted)
			return nodeMove("done", aClick->getNode("continueBtn"));
		if (aName.size() < aWanted.size() && aWanted.compare(0, aName.size(), aName) == 0)
			return inputLine("type", "", std::string("[{\"key\":\"") + aWanted[aName.size()] + "\"}]", 2);
		return inputLine("erase", "", "[{\"key\":\"Backspace\"}]", 2);
	}
	if (aState == "menu" && aClick != nullptr && aScreen == "menu")
		return nodeMove("new game", aClick->getNode("newGameBtnImg"));
	if (aState == "stage")
	{
		ff::StageMgr* aStage = aFood->getStageMgr();
		if (aStage == nullptr)
			return waitLine(WAIT_UPDATES);
		if (aStage->getState() == ff::STATE_PLAYING && aScreen == "stage-playing")
			return playMove(aStage);
		if (aClick != nullptr)
		{
			engine::Node* aButton = firstButton(aClick, CONTINUE_BUTTONS, CONTINUE_BUTTON_COUNT);
			if (aButton != nullptr)
				return nodeMove("continue", aButton);
		}
		return waitLine(WAIT_UPDATES);
	}
	if (aClick != nullptr)
	{
		engine::Node* aButton = firstButton(aClick, CONTINUE_BUTTONS, CONTINUE_BUTTON_COUNT);
		if (aButton != nullptr)
			return nodeMove("continue", aButton);
	}
	return waitLine(WAIT_UPDATES);
}

bool FeedingFrenzyApp::HandleScriptCommand(const ScriptCommand& theCommand)
{
	const std::string& aCmd = theCommand.mCommand;
	const std::string& anArgs = theCommand.mArgs;
	auto ok = [&]() { std::printf("OK %s%s%s\n", aCmd.c_str(), anArgs.empty() ? "" : " ", anArgs.c_str()); };
	auto error = [&](const std::string& theWhy) { std::printf("ERROR %s: %s\n", aCmd.c_str(), theWhy.c_str()); };
	bool aKnown = aCmd == "state" || aCmd == "move" || aCmd == "goto" || aCmd == "level" || aCmd == "unlock" ||
		aCmd == "set" || aCmd == "win" || aCmd == "lose" || aCmd == "seed";
	// "move" with arguments is the framework's pointer move, not the bot's question
	if (!aKnown || (aCmd == "move" && anArgs.find_first_not_of(" \t") != std::string::npos))
		return SexyAppBase::HandleScriptCommand(theCommand);
	ff::FishFood* aFood = ff::FishFood::instance();
	if (aFood == nullptr)
	{
		error("the game is not up");
		return true;
	}
	if (aCmd == "state")
		std::printf("STATE %s\n", TestState().c_str());
	else if (aCmd == "move")
		std::printf("MOVE %s\n", TestMove().c_str());
	else if (aCmd == "seed")
	{
		unsigned int aSeed = 0;
		if (std::sscanf(anArgs.c_str(), "%u", &aSeed) != 1)
			error("seed needs a number");
		else
		{
			engine::crtSrand(aSeed);
			ok();
		}
	}
	else if (aCmd == "goto")
	{
		std::string aState = aFood->getStateName();
		int aMode = modeNumber(anArgs);
		if (aMode >= 0)
		{
			aFood->startGame(aMode, 0);
			ok();
		}
		else if (aState != "menu")
			error("the game is not at the main menu");
		else if (anArgs == "title" || anArgs == "menu")
			ok();
		else if (anArgs == "options")
		{
			aFood->screenAction("options");
			ok();
		}
		else if (anArgs == "highscores")
		{
			aFood->screenAction("highScores");
			ok();
		}
		else if (anArgs == "credits")
		{
			aFood->screenAction("showCredits");
			ok();
		}
		else
			error("not a screen of this game: title, menu, options, highscores, credits, normal, timed");
	}
	else if (aCmd == "level")
	{
		int aLevel = 0;
		if (std::sscanf(anArgs.c_str(), "%d", &aLevel) != 1 || aLevel < 1 || aFood->getStageCount() < aLevel)
			error("level needs a stage number from 1 to " + std::to_string(aFood->getStageCount()));
		else
		{
			aFood->startGame(aFood->getSession()->getMode(), aLevel - 1);
			ok();
		}
	}
	else if (aCmd == "unlock")
	{
		if (anArgs != "all")
			error("unlock needs: all");
		else
		{
			aFood->unlockAllStages();
			ok();
		}
	}
	else if (aCmd == "set")
	{
		char aName[32] = "";
		int aValue = 0;
		ff::StageMgr* aStage = aFood->getStageMgr();
		if (std::sscanf(anArgs.c_str(), "%31s %d", aName, &aValue) != 2)
			error("set needs a name and a number");
		else if (std::string(aName) == "score")
		{
			aFood->getSession()->addScore(aValue - aFood->getSession()->getScore());
			ok();
		}
		else if (std::string(aName) == "lives")
		{
			aFood->getSession()->addLives(aValue - aFood->getSession()->getLives());
			ok();
		}
		else if (std::string(aName) == "timer" && aStage != nullptr)
		{
			aStage->setSecondsLeft((float)aValue);
			ok();
		}
		else
			error("not a key of this game's state that can be set: score, lives, timer");
	}
	else if (aCmd == "win" || aCmd == "lose")
	{
		ff::StageMgr* aStage = aFood->getStageMgr();
		if (std::string(aFood->getStateName()) != "stage" || aStage == nullptr ||
		    aStage->getState() != ff::STATE_PLAYING || aStage->getPlayer() == nullptr ||
		    aStage->getPlayer()->isDying())
			error("no stage is being played");
		else
		{
			// win: the stage is cleared (the mermaid swims in); lose: the player is eaten, as a fish's bite does
			if (aCmd == "win")
				aStage->beginMermaidEnd();
			else
				aStage->getPlayer()->die();
			ok();
		}
	}
	return true;
}

// port: -statelog: the values the identical-play runs compare with the original's
void FeedingFrenzyApp::LogState() {}
