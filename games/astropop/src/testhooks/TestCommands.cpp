// port: the standard test commands (docs/TEST_COMMANDS.md), reached only from a test's -script or live command
// channel through HandleScriptCommand. A command that is never given changes nothing. Each answers on stdout with one
// line: STATE {...}, OK <command> or ERROR <command>: <reason>. They act through the game's own code.
// Implemented: state, goto menu|classic|survival, level, seed, unlock all, set, win, lose. (move, the bot, is in BoardBot.cpp.)

#include "AstroPopApp.h"
#include "Board.h"
#include "Compat2004.h"
#include "LoaderScreen.h"
#include "MainMenu.h"
#include "UserProfile.h"
#include "WidgetManager.h"

#include <cstdio>
#include <string>

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
		if (mBoard != NULL)
		{
			aLine += StrFormat(",\"mode\":%d,\"level\":%d,\"score\":%d,\"stage\":%d,\"updates\":%d",
				mBoard->mMode, mBoard->m1940, mBoard->mScore, mBoard->mField193C, mBoard->mUpdateCnt);
			aLine += StrFormat(",\"type\":%d,\"paused\":%d,\"over\":%d,\"hand\":%d", mBoard->m1a60, mBoard->m1924,
				mBoard->mFlag8E8 ? 1 : 0, (int) mBoard->m70.size());
			aLine += ",\"field\":" + mBoard->DescribeField();
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
		// the two pilots that have to be earned (the unlock flags of the profile: bits 2 and 6, with their check value)
		int aFlags = 0x13 | 4 | 0x40;
		mUser->SetInt(PROFILE_UNLOCK_A, aFlags);
		mUser->SetInt(PROFILE_UNLOCK_B, aFlags + 0x193);
		mUser->Save();
		ReplyOK(theCommand);
		return true;
	}

	if (theCommand.mCommand == "set")
	{
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
		else
		{
			ReplyError(theCommand, "set takes score, level or stage");
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

	return false;
}
