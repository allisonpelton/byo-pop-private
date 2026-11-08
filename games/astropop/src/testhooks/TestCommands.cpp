// port: the standard test commands (docs/TEST_COMMANDS.md), reached only from a test's -script or live command
// channel through HandleScriptCommand. A command that is never given changes nothing. Each answers on stdout with one
// line: STATE {...}, OK <command> or ERROR <command>: <reason>. They act through the game's own code.
// Implemented so far: state, goto menu. The rest come with the screens and the Board.

#include "AstroPopApp.h"
#include "LoaderScreen.h"
#include "MainMenu.h"
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
	if (theApp->mScreen64c != NULL)
		return "screen";
	if (theApp->mMainMenu != NULL && theApp->mMainMenu->mWidgetManager != NULL)
		return "menu";
	if (theApp->mLoaderScreen != NULL)
		return "title";
	return "none";
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
		Reply(std::string("STATE {\"screen\":\"") + ScreenName(this) + "\",\"user\":" + aUser + "}");
		return true;
	}

	if (theCommand.mCommand == "goto")
	{
		if (theCommand.mArgs == "menu")
		{
			if (mLoaderScreen != NULL)
			{
				mWidgetManager->RemoveWidget(mLoaderScreen);
				SafeDeleteWidget(mLoaderScreen);
				mLoaderScreen = NULL;
			}
			ShowMainMenu();
			ReplyOK(theCommand);
		}
		else
			ReplyError(theCommand, "unknown screen (menu only so far)");
		return true;
	}

	return false;
}
