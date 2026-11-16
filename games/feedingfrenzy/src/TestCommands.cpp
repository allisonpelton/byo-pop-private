// port: the standard test commands (docs/TEST_COMMANDS.md), reachable only from a test (-script, -commands, the web
// test page's command call). Each answers on stdout with one line: STATE {...}, MOVE {...} or MOVE null, OK
// <command>, or ERROR <command>: <reason>. A command never given changes nothing and draws no random number; "move"
// changes nothing either. The game's notes list the commands and the state's keys (games/feedingfrenzy/docs/README.md,
// "Test commands").

#include "FeedingFrenzyApp.h"
#include "platform/MemReport.h"
#include "sprout/App.h"
#include "sprout/host/SoundHost.h"

#include <cstdio>
#include <string>

using namespace Sexy;

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

std::string FeedingFrenzyApp::TestState()
{
	return "{}";
}

std::string FeedingFrenzyApp::TestMove()
{
	return "null";
}

bool FeedingFrenzyApp::HandleScriptCommand(const ScriptCommand& theCommand)
{
	const std::string& aCmd = theCommand.mCommand;
	const std::string& anArgs = theCommand.mArgs;
	auto ok = [&]() { std::printf("OK %s%s%s\n", aCmd.c_str(), anArgs.empty() ? "" : " ", anArgs.c_str()); };
	auto error = [&](const char* theWhy) { std::printf("ERROR %s: %s\n", aCmd.c_str(), theWhy); };
	bool aKnown = aCmd == "state" || aCmd == "move";
	if (!aKnown)
		return SexyAppBase::HandleScriptCommand(theCommand);
	if (aCmd == "state")
		std::printf("STATE %s\n", TestState().c_str());
	else if (aCmd == "move")
		std::printf("MOVE %s\n", TestMove().c_str());
	else
		error("not translated yet");
	(void)ok;
	return true;
}

// port: -statelog: the values the identical-play runs compare with the original's
void FeedingFrenzyApp::LogState() {}
