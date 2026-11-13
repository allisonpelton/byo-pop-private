// The standard test commands (docs/TEST_COMMANDS.md): script commands that put the game in a state or read it back. They are
// reachable only from a test (-script, -commands and the web page's test call), so a normal run never meets them.
//
// state                prints STATE {"screen","zoom","mode","level","score","health","coins","timer","phase","paused","dialogs",
//                      "enemy","gnomes"}: the screen on top, the mode (0 classic, 1 tough, 2 marathon), the level, the points, the
//                      hearts, the coins, how much of the level's time has gone (0 to 1), the level's phase, the pause count,
//                      the ids of the dialogs open, the number of creatures up and one entry for each cubby that holds one
// goto <screen>        menu, highscores, trophies, shop, challenge, or a mode's name (classic, tough, marathon) to start it
// level <n>            starts level n of the mode on screen (classic if there is none), as a new game of the mode starts
// unlock all|marathon  Tough Cookie, or Marathon (the restart points it asks for), in the current player
// set <name> <value>   score, level, health, coins
// win, lose            ends the level in play now (its summary comes up), or the game (the health goes)
// seed <n>             seeds the game's random numbers as the start of the game does
// move                 prints MOVE {"kind":"smash","input":[...]} for the creature the bot would hit, or MOVE null

#include "Application.h"
#include "Avatar.h"

#include "ChallengeScreen.h"
#include "Compat2004.h"
#include "Creature.h"
#include "Cubby.h"
#include "CubbyContainer.h"
#include "DialogButton.h"
#include "Gameplay.h"
#include "GameScreen.h"
#include "LevelIntroWidget.h"
#include "LevelRules.h"
#include "MainMenu.h"
#include "NcButton.h"
#include "NcMovingDialog.h"
#include "Object.h"
#include "ResurrectButton.h"
#include "ResurrectScreenWidget.h"
#include "Screens.h"
#include "ShopScreen.h"
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

// the bot's steps (ten updates each) a box has to rest before the bot presses its button
const int BOX_RESTING_STEPS = 6;

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
	char aText[400];
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
		"\"phase\":%d,\"paused\":%d,\"inplay\":%d,\"dialogs\":[",
		GetScreenName(), gApp->mZooming ? 1 : 0, gApp->mGameMode, gApp->mGameScreen != nullptr ? gApp->mGameScreen->mLevel : 0,
		GetStats()->GetInt(STAT_SCORE), GetStats()->Get(STAT_HEALTH), GetStats()->GetInt(STAT_COIN_BALANCE),
		aGameplay != nullptr ? GetTimer()->GetProgress() : 0.0, aGameplay != nullptr ? aGameplay->mPhase : -1, gApp->mPauseCount,
		gApp->mGameScreen != nullptr && gApp->mGameScreen->IsInPlay() ? 1 : 0);
	std::string aState = aText;
	aState += aDialogs + "],\"enemy\":" + std::to_string(aCreaturesUp) + ",\"gnomes\":[" + aGnomes + "]}";
	return aState;
}

// the hammer has no swing under way and no click waiting. A swing that is not over would be spent on a creature that has gone by
// then, and a swing whose target goes (the shop closing within its first frames) makes the original strike a null pointer
// (Avatar::FixedUpdate, 0x42eb80, crashes where the port tests it), so the bot waits for the hammer before it leaves the shop too
bool BotHammerIdle()
{
	if (gApp->mGameScreen == nullptr || !gApp->mGameScreen->mAvatar.IsValid())
		return false;
	Avatar* anAvatar = static_cast<Avatar*>(gApp->mGameScreen->mAvatar.Get());
	return anAvatar->mSwing == Avatar::SWING_NONE && anAvatar->mClicks.empty();
}

// the creature the bot hits: the first cubby, in the order of the board, whose creature stands fully up and is not a bomb (or is
// one, when theWantBomb is set: phase one's scenarios hit one now and then), and the point where the hammer lands on it
bool ChooseBotTarget(int* theCubbyIndex, int* theX, int* theY, bool theWantBomb = false)
{
	if (gApp->mGameScreen == nullptr || (!gApp->mGameScreen->IsInPlay() && !gApp->IsShopOpen() && !gApp->IsChallengeOpen()))
		return false;
	if (!BotHammerIdle())
		return false;

	// the cubbies of the screen in use: the shop's, the bonus round's or the level's
	Gameplay* aGameplay = GetGameplay();
	CubbyContainer* aContainer = nullptr;
	if (gApp->IsShopOpen())
		aContainer = gApp->mShopScreen;
	else if (gApp->IsChallengeOpen())
		aContainer = gApp->mChallengeScreen;
	size_t aCubbyCount = aContainer != nullptr ? aContainer->mCubbies.size() : aGameplay->mCubbies.size();
	for (size_t i = 0; i < aCubbyCount; i++)
	{
		Cubby* aCubby = aContainer != nullptr ? aContainer->GetCubby((int)i) : aGameplay->GetCubby((int)i);
		if (aCubby == nullptr || aCubby->mCreature == nullptr || aCubby->mState != Cubby::STATE_UP)
			continue;
		Creature* aCreature = aCubby->mCreature;
		if (aCreature->mFinished || aCreature->mUntouchable || aCreature->mSpent || (aCreature->mKind == KIND_BOMB) != theWantBomb)
			continue;

		// the strike point is above the head and may be a clear pixel: the point of the cubby's bounds nearest to it that has a
		// solid pixel of the creature under it (the game's own test of a point on a creature), and solid pixels a few away on
		// every side, so that a creature that moves before the click is taken (a coin falls a pixel or two a step) is still hit
		const int aMargin = 5;
		double aPoint[2];
		aCubby->GetStrikePoint(aPoint);
		int aBestX = 0, aBestY = 0;
		double aBestDistance = -1.0;
		for (int aY = aCubby->mBounds.mY; aY < aCubby->mBounds.mY + aCubby->mBounds.mHeight; aY++)
		{
			for (int aX = aCubby->mBounds.mX; aX < aCubby->mBounds.mX + aCubby->mBounds.mWidth; aX++)
			{
				double aDistance = (aX - aPoint[0]) * (aX - aPoint[0]) + (aY - aPoint[1]) * (aY - aPoint[1]);
				if ((aBestDistance < 0.0 || aDistance < aBestDistance) && aCubby->MouseOver(aX, aY) &&
					aCubby->MouseOver(aX - aMargin, aY) && aCubby->MouseOver(aX + aMargin, aY) &&
					aCubby->MouseOver(aX, aY - aMargin) && aCubby->MouseOver(aX, aY + aMargin))
				{
					aBestDistance = aDistance;
					aBestX = aX;
					aBestY = aY;
				}
			}
		}
		if (aBestDistance < 0.0)
			continue;
		*theCubbyIndex = (int)i;
		*theX = aBestX;
		*theY = aBestY;
		return true;
	}
	return false;
}

// the page or the box that waits for its one button, and where that button is (phase one's scenarios let the bot answer the level's
// introduction and the "Ready?" box; the box slides in first, so a click that comes too early is made again at the bot's next step)
bool FindWaitingButton(int* theX, int* theY)
{
	if (gApp->mLevelIntro != nullptr)
	{
		*theX = gApp->mLevelIntro->mButton->mX + gApp->mLevelIntro->mButton->mWidth / 2;
		*theY = gApp->mLevelIntro->mButton->mY + gApp->mLevelIntro->mButton->mHeight / 2;
		return true;
	}

	// the page of the restart points (after a lost game): the highest point
	if (gApp->mResurrectScreen != nullptr && !gApp->mResurrectScreen->mPointButtons.empty())
	{
		ResurrectButton* aButton = gApp->mResurrectScreen->mPointButtons.back();
		*theX = aButton->mX + aButton->mWidth / 2;
		*theY = aButton->mY + aButton->mHeight / 2;
		return true;
	}

	// the boxes with one button to press; a box that slides in is not pressed before it has stopped
	static const int aBoxes[] = {DIALOG_READY_GO, DIALOG_BONUS_ROUND, DIALOG_CHALLENGE_FINISHED, DIALOG_GAME_OVER, DIALOG_GAME_WON,
		DIALOG_TOUGH_UNLOCKED};
	static Dialog* sRestingBox = nullptr;
	static int sRestingSteps = 0;
	for (int anId : aBoxes)
	{
		Dialog* aDialog = gApp->GetDialog(anId);
		if (aDialog == nullptr)
			continue;
		if (NcMovingDialog* aMoving = dynamic_cast<NcMovingDialog*>(aDialog))
		{
			if (aMoving->mY != aMoving->mRestY)
				return false;
		}
		// the original's box has settled some updates after this one's (its slide runs in frames, and a click that comes while it
		// slides is lost), so a box is pressed only after it has rested for a few steps of the bot
		if (aDialog != sRestingBox)
		{
			sRestingBox = aDialog;
			sRestingSteps = 0;
		}
		if (++sRestingSteps < BOX_RESTING_STEPS)
			return false;
		if (aDialog->mYesButton == nullptr)
			return false;
		*theX = aDialog->mYesButton->mX + aDialog->mYesButton->mWidth / 2;
		*theY = aDialog->mYesButton->mY + aDialog->mYesButton->mHeight / 2;
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
	// a test has no login: the game always has a player (the roster is empty on the first run)
	if (mUserManager->GetCurrentUser() == nullptr)
		mUserManager->CreateUser("Player");

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
			// the two screens' cubbies have the same names, so a game never has both (the original's own flow leaves one first)
			if (mShopScreen != nullptr || mChallengeScreen != nullptr)
				return Error(aLine, "a shop or challenge screen is on: `level <n>` starts the game over first"), true;
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
		if ((anArg != "all" && anArg != "marathon") || aUser == nullptr)
			return Error(aLine, "unlock all|marathon, with a player"), true;
		if (anArg == "all")
			aUser->mToughUnlocked = true;
		else
		{
			// Marathon opens once the classic list holds three restart points (HasFewRestartPoints, 0x41f290)
			while (aUser->GetRestartPointCount(GAME_MODE_CLASSIC) < 3)
				aUser->mClassicRestartPoints.push_back(RestartPoint());
		}
		// the main menu reads the unlocks when it is made and when the player changes: tell one already up
		if (mMainMenu != nullptr)
			mMainMenu->UserChanged();
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

	if (aName == "trace")
	{
		if (anArg.empty())
			return Error(aLine, "trace <file>|off"), true;
		if (!StartTrace(anArg))
			return Error(aLine, "the file could not be made"), true;
		Ok(aLine);
		return true;
	}

	// a cheat word typed (the letters go to the cheat table as the main menu passes them on; a word that is whole switches its cheat):
	// phase one starts Classic Bash at a later level with `cheat jumpto10`, which the original gets as a patched start (idplay)
	if (aName == "cheat")
	{
		if (anArg.empty())
			return Error(aLine, "cheat <word>"), true;
		for (char aChar : anArg)
			KeyChar(aChar);
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

	// the bot's move played at once as the framework's `click` plays one, for phase one's scenarios (tools/idplay): the mode is
	// empty for the bot's own hit, "bomb" for a bomb that stands up, "miss" for a click on bare ground. A click made is
	// written to the log as `BOTCLICK <update count> <x> <y>`; the scenario then replays it as a plain click.
	if (aName == "botclick")
	{
		int aCubbyIndex = 0, aX = 400, aY = 590;
		bool aMiss = anArg == "miss";
		if (gApp->mGameScreen == nullptr)
			return true;
		bool aShop = gApp->IsShopOpen();
		if (FindWaitingButton(&aX, &aY))
			aMiss = true;		// the point is chosen: the click is made as the miss's is
		else if (gApp->GetDialog(DIALOG_READY_GO) != nullptr || gApp->mLevelIntro != nullptr || gApp->mZooming)
			return true;		// a box that has not stopped, or a wipe: nothing to press yet
		else if (aShop)
		{
			// the shop: the first three clicks buy (the first gnome with an item standing up), then the arrow goes back to the game
			static int aShopClicks = 0, aLastShopUpdate = -100000;
			if (mUpdateCount - aLastShopUpdate > 2000)
				aShopClicks = 0;
			aLastShopUpdate = mUpdateCount;
			if (aShopClicks < 3 && ChooseBotTarget(&aCubbyIndex, &aX, &aY))
				aShopClicks++;
			else if (aShopClicks >= 3 && gApp->mShopScreen->mBackButton != nullptr && BotHammerIdle())
			{
				aX = gApp->mShopScreen->mBackButton->mX + gApp->mShopScreen->mBackButton->mWidth / 2;
				aY = gApp->mShopScreen->mBackButton->mY + gApp->mShopScreen->mBackButton->mHeight / 2;
			}
			else
				return true;
			aMiss = true;
		}
		else if (!gApp->mGameScreen->IsInPlay() && !gApp->IsChallengeOpen())
			return true;
		if (!aMiss && !ChooseBotTarget(&aCubbyIndex, &aX, &aY, anArg == "bomb"))
			return true;
		ScriptCommand aClick = theCommand;
		aClick.mCommand = "click";
		aClick.mArgs = std::to_string(aX) + " " + std::to_string(aY);
		std::string anError;
		RunScriptCommand(aClick, &anError);
		Reply("BOTCLICK " + std::to_string(mUpdateCount) + " " + std::to_string(aX) + " " + std::to_string(aY));
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
