// The standard test commands (docs/TEST_COMMANDS.md): script commands that put the game in a state or read it back. They are
// reachable only from a test (-script, -commands and the web page's test call), so a normal run never meets them.
//
// state                prints STATE {"screen","zoom","mode","level","score","health","coins","timer","phase","paused","dialogs",
//                      "enemy","gnomes"}: the screen on top, the mode (0 classic, 1 tough, 2 marathon), the level, the points, the
//                      hearts, the coins, how much of the level's time has gone (0 to 1), the level's phase, the pause count,
//                      the ids of the dialogs open, the number of creatures up and one entry for each cubby that holds one
// goto <screen>        menu, highscores, trophies, shop, challenge, or a mode's name (classic, tough, marathon) to start it
// level <n>            starts level n of the mode on screen (classic if there is none), as a new game of the mode starts
// unlock all           Tough Cookie, in the current player
// set <name> <value>   score, level, health, coins
// win, lose            ends the level in play now (its summary comes up), or the game (the health goes)
// seed <n>             seeds the game's random numbers as the start of the game does
// move                 prints MOVE {"kind":"smash","input":[...]} for the creature the bot would hit, or MOVE null

#include "Application.h"

#include "Compat2004.h"
#include "Creature.h"
#include "Cubby.h"
#include "Gameplay.h"
#include "GameScreen.h"
#include "LevelRules.h"
#include "Object.h"
#include "Screens.h"
#include "Stats.h"
#include "Timer.h"
#include "UserManager.h"
#include "WidgetManager.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

using namespace Sexy;

namespace
{
const char* const kModeNames[NUM_GAME_MODES] = {"classic", "tough", "marathon"};

// the repeated "take a widget off the manager, delete it, forget it"
template <class T>
void DropWidget(T*& theWidget)
{
	gApp->mWidgetManager->RemoveWidget(theWidget);
	gApp->SafeDeleteWidget(theWidget);
	theWidget = nullptr;
}

void Reply(const std::string& theLine)
{
	printf("%s\n", theLine.c_str());
	fflush(stdout);
}

void Ok(const std::string& theCommand)
{
	Reply("OK " + theCommand);
}

void Error(const std::string& theCommand, const std::string& theReason)
{
	Reply("ERROR " + theCommand + ": " + theReason);
}

const char* GetScreenName()
{
	if (gApp->mLoadingScreen != nullptr)
		return "loading";
	if (gApp->mChallengeScreen != nullptr)
		return "challenge";
	if (gApp->mShopScreen != nullptr)
		return "shop";
	if (gApp->mResurrectScreen != nullptr)
		return "resurrect";
	if (gApp->mGameWon != nullptr)
		return "gamewon";
	if (gApp->mGameScreen != nullptr)
		return "game";
	if (gApp->mTrophyRoom != nullptr)
		return "trophies";
	if (gApp->GetDialog(DIALOG_HIGH_SCORE_TABLE) != nullptr)
		return "highscores";
	if (gApp->mMainMenu != nullptr)
		return "menu";
	return "none";
}

// "mode" of a goto or a level: a mode's name, or 0 to 2
int ModeFromName(const std::string& theName)
{
	for (int aMode = 0; aMode < NUM_GAME_MODES; aMode++)
	{
		if (theName == kModeNames[aMode])
			return aMode;
	}
	return -1;
}

std::string StateJson()
{
	char aText[300];
	Gameplay* aGameplay = gApp->mGameScreen != nullptr ? GetGameplay() : nullptr;
	std::string aDialogs;
	for (int anId = 0; anId < 0x20; anId++)
	{
		if (gApp->GetDialog(anId) != nullptr)
			aDialogs += (aDialogs.empty() ? "" : ",") + std::to_string(anId);
	}

	int aCreaturesUp = 0;
	std::string aGnomes;
	if (aGameplay != nullptr)
	{
		for (size_t i = 0; i < aGameplay->mCubbies.size(); i++)
		{
			Cubby* aCubby = aGameplay->GetCubby((int)i);
			if (aCubby == nullptr || aCubby->mCreature == nullptr)
				continue;
			Creature* aCreature = aCubby->mCreature;
			aCreaturesUp++;
			snprintf(aText, sizeof(aText), "%s{\"cubby\":%d,\"kind\":%d,\"state\":%d,\"cubbyState\":%d,\"rise\":%.4f,\"health\":%d,\"x\":%d,\"y\":%d}",
				aGnomes.empty() ? "" : ",", (int)i, aCreature->mKind, aCreature->mState, aCubby->mState, aCubby->mRise,
				aCreature->mHealth, (int)aCreature->mX, (int)aCreature->mY);
			aGnomes += aText;
		}
	}

	snprintf(aText, sizeof(aText),
		"STATE {\"screen\":\"%s\",\"zoom\":%d,\"mode\":%d,\"level\":%d,\"score\":%d,\"health\":%.2f,\"coins\":%d,\"timer\":%.4f,"
		"\"phase\":%d,\"paused\":%d,\"dialogs\":[",
		GetScreenName(), gApp->mZooming ? 1 : 0, gApp->mGameMode, gApp->mGameScreen != nullptr ? gApp->mGameScreen->mLevel : 0,
		GetStats()->GetInt(STAT_SCORE), GetStats()->Get(STAT_HEALTH), GetStats()->GetInt(STAT_COIN_BALANCE),
		aGameplay != nullptr ? GetTimer()->GetProgress() : 0.0, aGameplay != nullptr ? aGameplay->mPhase : -1, gApp->mPauseCount);
	std::string aState = aText;
	aState += aDialogs + "],\"enemy\":" + std::to_string(aCreaturesUp) + ",\"gnomes\":[" + aGnomes + "]}";
	return aState;
}

// the creature the bot hits: the first cubby, in the order of the board, whose creature stands fully up and is not a bomb,
// and the point where the hammer lands on it
bool ChooseBotTarget(int* theCubbyIndex, int* theX, int* theY)
{
	if (gApp->mGameScreen == nullptr || !gApp->mGameScreen->IsInPlay())
		return false;

	Gameplay* aGameplay = GetGameplay();
	for (size_t i = 0; i < aGameplay->mCubbies.size(); i++)
	{
		Cubby* aCubby = aGameplay->GetCubby((int)i);
		if (aCubby == nullptr || aCubby->mCreature == nullptr || aCubby->mState != Cubby::STATE_UP)
			continue;
		Creature* aCreature = aCubby->mCreature;
		if (aCreature->mFinished || aCreature->mUntouchable || aCreature->mSpent || aCreature->mKind == KIND_BOMB)
			continue;

		double aPoint[2];
		aCubby->GetStrikePoint(aPoint);
		if (!aCubby->MouseOver((int)aPoint[0], (int)aPoint[1]))
			continue;
		*theCubbyIndex = (int)i;
		*theX = (int)aPoint[0];
		*theY = (int)aPoint[1];
		return true;
	}
	return false;
}
}

// port: the seeding that Init gives the generators (the 2004 framework's: see AstroPopApp::Init)
void Application::SeedGame(unsigned int theSeed)
{
	mRandSeed = theSeed;
	CrtSRand(theSeed);
	MTRand aRand;
	SeedMTRand2004(aRand, (unsigned long)theSeed);
	uint32_t aState[MTRAND_N];
	int anIndex;
	aRand.GetState(aState, &anIndex);
	SetRandMTState(aState, anIndex);
}

// port: a new game of a mode, at a level, through the calls that WipeNewGame and CreateGame make; the screens and the dialogs
// there are taken down first
void Application::StartTestGame(int theMode, int theLevel)
{
	for (int anId = 0; anId < 0x20; anId++)
		KillDialog(anId);
	if (mMainMenu != nullptr)
		DropWidget(mMainMenu);
	if (mTrophyRoom != nullptr)
		DropWidget(mTrophyRoom);
	if (mZooming)
		EndZoomWipe();
	if (mGameWon != nullptr)
		DropWidget(mGameWon);
	if (mLevelIntro != nullptr)
		DropWidget(mLevelIntro);
	RemoveAllScreens();
	mPauseCount = 0;
	mLevelUpPending = false;
	mGameWonPending = false;
	mShopOpening = false;
	mChallengeOpening = false;
	mChallengeLeaving = false;

	mGameMode = theMode;
	ObjectManager::Get()->Clear();
	GetRulesSheet(true);
	GetStats()->Reset();

	mGameScreen = new GameScreen();
	if (theLevel > 1)
		mGameScreen->SetLevel(theLevel, 0);
	mGameScreen->Resize(0, 0, mWidth, mHeight);
	mWidgetManager->AddWidget(mGameScreen);
	mWidgetManager->SetFocus(mGameScreen);
	mGameScreen->Start(mGameMode);
	StartMusic(MUSIC_GAME, false);
	CreateLevelIntro();
}

bool Application::HandleScriptCommand(const ScriptCommand& theCommand)
{
	const std::string& aName = theCommand.mCommand;
	std::string anArg = theCommand.mArgs;
	while (!anArg.empty() && anArg.back() == ' ')
		anArg.pop_back();
	std::string aLine = aName + (anArg.empty() ? "" : " " + anArg);

	if (aName == "state")
	{
		Reply(StateJson());
		return true;
	}

	if (aName == "goto")
	{
		int aMode = ModeFromName(anArg);
		if (aMode >= 0)
		{
			StartTestGame(aMode, 1);
			Ok(aLine);
		}
		else if (anArg == "menu" || anArg == "title")
		{
			for (int anId = 0; anId < 0x20; anId++)
				KillDialog(anId);
			if (mZooming)
				EndZoomWipe();
			if (mGameWon != nullptr)
				DropWidget(mGameWon);
			if (mLevelIntro != nullptr)
				DropWidget(mLevelIntro);
			if (mTrophyRoom != nullptr)
				DropWidget(mTrophyRoom);
			RemoveAllScreens();
			mPauseCount = 0;
			ObjectManager::Get()->Clear();
			if (mMainMenu == nullptr)
				CreateMainMenu();
			Ok(aLine);
		}
		else if (anArg == "highscores")
		{
			if (mMainMenu == nullptr)
				return Error(aLine, "the main menu is not on screen"), true;
			ShowHighScoreTableFromMenu();
			Ok(aLine);
		}
		else if (anArg == "trophies")
		{
			if (mMainMenu == nullptr)
				return Error(aLine, "the main menu is not on screen"), true;
			ShowTrophyRoomFromMenu();
			Ok(aLine);
		}
		else if (anArg == "shop" || anArg == "challenge")
		{
			if (mGameScreen == nullptr)
				return Error(aLine, "no game is on screen"), true;
			if (anArg == "shop")
				DoShopScreen();
			else
				DoChallengeScreen();
			Ok(aLine);
		}
		else
			Error(aLine, "menu, highscores, trophies, shop, challenge, classic, tough or marathon");
		return true;
	}

	if (aName == "level")
	{
		int aLevel = atoi(anArg.c_str());
		if (aLevel < 1)
			return Error(aLine, "level <n>, from 1"), true;
		StartTestGame(mGameScreen != nullptr ? mGameMode : GAME_MODE_CLASSIC, aLevel);
		Ok(aLine);
		return true;
	}

	if (aName == "unlock")
	{
		User* aUser = mUserManager != nullptr ? mUserManager->GetCurrentUser() : nullptr;
		if (anArg != "all" || aUser == nullptr)
			return Error(aLine, "unlock all, with a player"), true;
		aUser->mToughUnlocked = true;
		Ok(aLine);
		return true;
	}

	if (aName == "set")
	{
		size_t aSpace = anArg.find(' ');
		std::string aKey = anArg.substr(0, aSpace);
		double aValue = aSpace == std::string::npos ? 0.0 : atof(anArg.c_str() + aSpace + 1);
		if (aKey == "score")
			GetStats()->Set(STAT_SCORE, aValue);
		else if (aKey == "health")
			GetStats()->Set(STAT_HEALTH, aValue);
		else if (aKey == "coins")
			GetStats()->Set(STAT_COIN_BALANCE, aValue);
		else if (aKey == "level" && mGameScreen != nullptr)
			mGameScreen->SetLevel((int)aValue, mGameScreen->mCycle);
		else
			return Error(aLine, "score, health, coins, or level with a game on screen"), true;
		Ok(aLine);
		return true;
	}

	if (aName == "win" || aName == "lose")
	{
		if (mGameScreen == nullptr || GetGameplay() == nullptr)
			return Error(aLine, "no game is on screen"), true;
		Gameplay* aGameplay = GetGameplay();
		if (aName == "win")
		{
			if (aGameplay->mPhase != Gameplay::PHASE_PLAYING && aGameplay->mPhase != Gameplay::PHASE_DRAINING)
				return Error(aLine, "the level is not in play"), true;
			aGameplay->mPhase = Gameplay::PHASE_PAUSING;
			aGameplay->mPhaseTimer = 0.0;
		}
		else
			GetStats()->Set(STAT_HEALTH, 0.0);
		Ok(aLine);
		return true;
	}

	if (aName == "seed")
	{
		if (anArg.empty())
			return Error(aLine, "seed <n>"), true;
		SeedGame((unsigned int)strtoul(anArg.c_str(), nullptr, 10));
		Ok(aLine);
		return true;
	}

	if (aName == "move")
	{
		int aCubbyIndex, aX, aY;
		if (!ChooseBotTarget(&aCubbyIndex, &aX, &aY))
		{
			Reply("MOVE null");
			return true;
		}
		char aText[200];
		snprintf(aText, sizeof(aText), "MOVE {\"kind\":\"smash\",\"cubby\":%d,\"input\":[{\"move\":[%d,%d]},{\"wait\":2},{\"click\":[%d,%d]},{\"wait\":10}]}",
			aCubbyIndex, aX, aY, aX, aY);
		Reply(aText);
		return true;
	}

	return false;
}
