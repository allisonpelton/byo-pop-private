// TestCommands.cpp: the test commands of Peggle (docs/TEST_COMMANDS.md), handled through ThunderballApp::HandleScriptCommand. A
// command that is never given changes nothing. Each answers on stdout with one line: STATE {...}, OK <command> or
// ERROR <command>: <reason>. They act through the game's own code.

#include "MainMenu.h"
#include "PlayerInfo.h"
#include "ThunderballApp.h"

#include <cstdio>
#include <string>

using namespace Sexy;

static void Reply(const std::string& theLine)
{
	printf("%s\n", theLine.c_str());
	fflush(stdout);
}

static std::string Words(const ScriptCommand& theCommand)
{
	return theCommand.mCommand + (theCommand.mArgs.empty() ? "" : " " + theCommand.mArgs);
}

bool ThunderballApp::HandleScriptCommand(const ScriptCommand& theCommand)
{
	if (theCommand.mCommand == "profile")
	{
		// makes the profile of that name if there is none and takes it as the one in use (the test's way round the dialog
		// that asks a new player for a name)
		std::string aName = theCommand.mArgs;
		if (aName.empty())
		{
			Reply("ERROR " + Words(theCommand) + ": needs a name");
			return true;
		}
		PlayerInfo* aProfile = mProfileMgr.GetProfile(aName);
		if (aProfile == nullptr)
		{
			aProfile = mProfileMgr.AddProfile(aName);
			mProfileMgr.Save();
		}
		mCurrentPlayer = aProfile;
		if (mMainMenu != nullptr)
			mMainMenu->SyncPlayerInfo();
		Reply("OK " + Words(theCommand));
		return true;
	}

	return false;
}
