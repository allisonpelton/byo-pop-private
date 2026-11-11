// port: the standard test commands (docs/TEST_COMMANDS.md), reached only from a test's -script or live command
// channel through HandleScriptCommand. A command that is never given changes nothing. Each answers on stdout with one
// line: STATE {...}, OK <command> or ERROR <command>: <reason>. They act through the game's own code.
// Implemented: state, goto menu|classic|survival, level, seed, unlock all, set, win, lose, move (the bot is in BoardBot.cpp).

#include "AstroPopApp.h"
#include "Board.h"
#include "Compat2004.h"
#include "Dialog.h"
#include "LoaderScreen.h"
#include "MainMenu.h"
#include "Particle.h"
#include "Res.h"
#include "UserProfile.h"
#include "WidgetManager.h"

#include <climits>
#include <cstdio>
#include <string>
#include <SDL.h>

using namespace Sexy;

namespace
{

void Reply(const std::string& theLine)
{
	printf("%s\n", theLine.c_str());
	fflush(stdout);
}

void ReplyOK(const ScriptCommand& theCommand)
{
	Reply("OK " + theCommand.mCommand + (theCommand.mArgs.empty() ? "" : " " + theCommand.mArgs));
}

void ReplyError(const ScriptCommand& theCommand, const std::string& theReason)
{
	Reply("ERROR " + theCommand.mCommand + (theCommand.mArgs.empty() ? "" : " " + theCommand.mArgs) + ": " + theReason);
}

// The name of the screen the application shows now
const char* ScreenName(AstroPopApp* theApp)
{
	if (theApp->mBoard != NULL)
		return "game";
	if (theApp->mScreen650 != NULL)
		return "screen";
	if (theApp->mMainMenu != NULL && theApp->mMainMenu->mWidgetManager != NULL)
		return "menu";
	if (theApp->mLoaderScreen != NULL)
		return "title";
	return "none";
}

// A test starts with no player when the folder it runs in is new: the original asks for a name on its first run, and a
// test makes the player the dialog would make (the dialog is not translated yet)
void EnsureUser(AstroPopApp* theApp)
{
	if (theApp->mUser != NULL)
		return;
	theApp->mUser = new UserProfile();
	theApp->mUser->InitNew("Test", -1);
	theApp->ValidateUser();
}

// The title screen and the selector both go before a game starts: the title's button is pressed (the descriptor and the
// profile load), and a folder with no player gets the one the new-player dialog would make
void LeaveMenus(AstroPopApp* theApp)
{
	if (theApp->mLoaderScreen != NULL)
		theApp->OnClickToPlay();
	EnsureUser(theApp);
	theApp->CloseMainMenu();
}

// The words of a command's arguments as numbers (missing ones stay as they are)
void ReadInts(const std::string& theArgs, int* theValues, int theMax)
{
	const char* aPos = theArgs.c_str();
	for (int i = 0; i < theMax; i++)
	{
		char* anEnd = NULL;
		long aValue = strtol(aPos, &anEnd, 10);
		if (anEnd == aPos)
			break;
		theValues[i] = (int) aValue;
		aPos = anEnd;
	}
}

}

// -trace (a test flag): the values that origrun's --watch reads in the original (docs/ORIGRUN.md), in the same lines
// ("watch 1: 0 -> 10 at update 312"), so that tools/origcmp pairs them value by value: 1 the board's score (+0x1974), 2 its level
// (+0x1940), 3 the position in the game's generator (the MTRand that 0x5a4e28 points to; the index at +0x9c0) and 4 the position
// in Rand()'s generator (the MTRand at 0x5a5e98), each "-" while there is no board (watch 4 always has a value). Logged in the
// update in which a value differs from the last one logged, with the update count the framework keeps (+0x4a0 in the original).
void AstroPopApp::TraceUpdate()
{
	static const int WATCHES = 4;
	static int sLast[WATCHES] = {INT_MIN, INT_MIN, INT_MIN, INT_MIN};
	static bool sFirst = true;
	int aValue[WATCHES] = {INT_MIN, INT_MIN, INT_MIN, INT_MIN};
	uint32_t aState[MTRAND_N];
	int anIndex = 0;
	if (mBoard != NULL)
	{
		aValue[0] = mBoard->mScore;
		aValue[1] = mBoard->m1940;
	}
	gGameRand.GetState(aState, &anIndex);
	aValue[2] = anIndex;
	GetRandMTState(aState, &anIndex);
	aValue[3] = anIndex;
	for (int i = 0; i < WATCHES; i++)
	{
		if (!sFirst && aValue[i] == sLast[i])
			continue;
		std::string anOld = sFirst ? "?" : (sLast[i] == INT_MIN ? "-" : StrFormat("%d", sLast[i]));
		std::string aNew = aValue[i] == INT_MIN ? "-" : StrFormat("%d", aValue[i]);
		SDL_Log("watch %d: %s -> %s at update %d", i + 1, anOld.c_str(), aNew.c_str(), mUpdateCount);
		sLast[i] = aValue[i];
	}
	sFirst = false;
}

bool AstroPopApp::HandleScriptCommand(const ScriptCommand& theCommand)
{
	if (theCommand.mCommand == "state")
	{
		std::string aUser = "null";
		if (mUser != NULL)
		{
			std::string aName;
			mUser->GetString(PROFILE_NAME, &aName);
			aUser = "\"" + aName + "\"";
		}
		std::string aLine = std::string("STATE {\"screen\":\"") + ScreenName(this) + "\",\"user\":" + aUser;
		aLine += StrFormat(",\"appupdates\":%d", mUpdateCount);
		if (mBoard != NULL)
		{
			aLine += StrFormat(",\"mode\":%d,\"level\":%d,\"score\":%d,\"stage\":%d,\"updates\":%d",
				mBoard->mMode, mBoard->m1940, mBoard->mScore, mBoard->mField193C, mBoard->mUpdateCnt);
			aLine += StrFormat(",\"type\":%d,\"paused\":%d,\"over\":%d,\"hand\":%d", mBoard->m1a60, mBoard->m1924,
				mBoard->mFlag8E8 ? 1 : 0, (int) mBoard->mHeldBricks.size());
			aLine += StrFormat(",\"busy\":%d", mBoard->BotBusyReason());
			aLine += ",\"field\":" + mBoard->DescribeField();
		}
		// the box that asks where a classic game starts (a pilot who has passed a checkpoint): the middle of its two gates,
		// the first level's and the checkpoint's, and of their row
		Dialog* aGates = GetDialog(0x78);
		if (aGates != NULL)
		{
			int aGateWidth = IMAGE_SLOW_WARP_GATE->GetCelWidth();
			aLine += StrFormat(",\"checkpoint\":[%d,%d,%d]", aGates->mX + aGateWidth / 2,
				aGates->mX + aGateWidth + 0x19 + (aGateWidth - 0x19) / 2,
				aGates->mY + aGates->mHeight - 0x28 - IMAGE_SLOW_WARP_GATE->mHeight / 2);
		}
		Reply(aLine + "}");
		return true;
	}

	if (theCommand.mCommand == "goto")
	{
		if (!mLoadingThreadCompleted)
		{
			ReplyError(theCommand, "the resources are still loading");
			return true;
		}
		std::string aScreen = theCommand.mArgs.substr(0, theCommand.mArgs.find(' '));
		std::string aRest = aScreen.size() < theCommand.mArgs.size() ? theCommand.mArgs.substr(aScreen.size()) : "";
		if (aScreen == "menu")
		{
			if (mLoaderScreen != NULL)
				OnClickToPlay();
			else
				ShowMainMenu();
			ReplyOK(theCommand);
		}
		else if (aScreen == "classic")
		{
			// goto classic [level [pilot]]: a new game of the first type
			int aValues[2] = { 1, 0 };
			ReadInts(aRest, aValues, 2);
			EnsureUser(this);
			LeaveMenus(this);
			mClassicPilot = aValues[1];
			StartClassicGame(aValues[0], false, true);
			ReplyOK(theCommand);
		}
		else if (aScreen == "survival")
		{
			// goto survival [pilot]: a new game of the second type
			int aValues[1] = { 0 };
			ReadInts(aRest, aValues, 1);
			EnsureUser(this);
			LeaveMenus(this);
			StartSurvivalGame(aValues[0]);
			ReplyOK(theCommand);
		}
		else
			ReplyError(theCommand, "unknown screen (menu, classic, survival so far)");
		return true;
	}

	if (theCommand.mCommand == "level")
	{
		int aLevel = 0;
		ReadInts(theCommand.mArgs, &aLevel, 1);
		if (mBoard == NULL || mBoard->m1a60 != 1)
			ReplyError(theCommand, "needs a classic game");
		else if (aLevel < 1)
			ReplyError(theCommand, "level needs a number from 1");
		else
		{
			EnsureUser(this);
			StartClassicGame(aLevel, false, true);
			ReplyOK(theCommand);
		}
		return true;
	}

	if (theCommand.mCommand == "seed")
	{
		int aSeed = 0;
		ReadInts(theCommand.mArgs, &aSeed, 1);
		// the same seeding as Init
		CrtSRand((unsigned int) aSeed);
		MTRand aRand;
		SeedMTRand2004(aRand, (unsigned long) aSeed);
		uint32_t aState[MTRAND_N];
		int anIndex;
		aRand.GetState(aState, &anIndex);
		SetRandMTState(aState, anIndex);
		ReplyOK(theCommand);
		return true;
	}

	if (theCommand.mCommand == "unlock")
	{
		if (theCommand.mArgs != "all")
		{
			ReplyError(theCommand, "unlock needs: all");
			return true;
		}
		EnsureUser(this);
		// the two pilots that have to be earned (the unlock flags of the profile: bits 2 and 6 and their check value; of the
		// four values a profile may hold on loading, 0x44 is the one with both)
		int aFlags = 0x44;
		mUser->SetInt(PROFILE_UNLOCK_A, aFlags);
		mUser->SetInt(PROFILE_UNLOCK_B, aFlags + 0x193);
		mUser->Save();
		ReplyOK(theCommand);
		return true;
	}

	if (theCommand.mCommand == "set")
	{
		// the weapon's charge for every new board and every new game or level (the original's patch list for the weapon tours
		// stores 18000 in the board's constructor and in ResetState: `set chargestart 18000`), kept until `set chargestart -1`
		if (theCommand.mArgs.compare(0, 11, "chargestart") == 0)
		{
			int aStart = -1;
			ReadInts(theCommand.mArgs.size() > 11 ? theCommand.mArgs.substr(11) : std::string(), &aStart, 1);
			gTestChargeAtReset = (float) aStart;
			ReplyOK(theCommand);
			return true;
		}
		if (mBoard == NULL)
		{
			ReplyError(theCommand, "needs a game");
			return true;
		}
		std::string aName = theCommand.mArgs.substr(0, theCommand.mArgs.find(' '));
		std::string aRest = aName.size() < theCommand.mArgs.size() ? theCommand.mArgs.substr(aName.size()) : "";
		int aValue = 0;
		ReadInts(aRest, &aValue, 1);
		if (aName == "score")
			mBoard->mScore = aValue;
		else if (aName == "level")
			mBoard->m1940 = aValue;
		else if (aName == "stage")
			mBoard->mField193C = aValue;
		else if (aName == "charge")
			mBoard->m1bd8 = aRest.empty() ? mBoard->m1bdc : (float) aValue;   // the weapon's charge: full without a number
		else if (aName == "stories")
		{
			// the player has seen all the stories of the four pilots (the profile's counts, 0x4b .. 0x4e: the last story of
			// a pilot is then followed by the finale), so a test can reach it
			EnsureUser(this);
			mUser->SetInt(0x4b, 9);
			mUser->SetInt(0x4c, 8);
			mUser->SetInt(0x4d, 8);
			mUser->SetInt(0x4e, 8);
			mUser->Save();
		}
		else
		{
			ReplyError(theCommand, "set takes score, level, stage, charge [N] or stories");
			return true;
		}
		ReplyOK(theCommand);
		return true;
	}

	if (theCommand.mCommand == "win")
	{
		// the level is complete (a classic game); the second type of game has no win: it ends
		if (mBoard == NULL)
			ReplyError(theCommand, "needs a game");
		else if (mBoard->m1a60 == 1)
		{
			mBoard->LevelComplete();
			ReplyOK(theCommand);
		}
		else
			ReplyError(theCommand, "win needs a classic game (lose ends the other)");
		return true;
	}

	if (theCommand.mCommand == "lose")
	{
		if (mBoard == NULL)
			ReplyError(theCommand, "needs a game");
		else
		{
			mBoard->GameOver();
			ReplyOK(theCommand);
		}
		return true;
	}

	if (theCommand.mCommand == "move")
	{
		// the bot's next move (BoardBot.cpp); changes nothing
		std::string aMove = (mBoard != NULL) ? mBoard->BotMove() : "null";
		Reply(aMove == "null" ? "MOVE null" : "MOVE " + aMove);
		return true;
	}

	return false;
}
