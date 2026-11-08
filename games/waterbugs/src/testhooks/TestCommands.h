#ifndef __WATERBUGS_TESTCOMMANDS_H__
#define __WATERBUGS_TESTCOMMANDS_H__

// Test hooks: the standard test commands of docs/TEST_COMMANDS.md for Water Bugs. They are reached only from a test
// (-script, -commands, the web test page's command call) through WaterBugsApp::HandleScriptCommand, and a command that
// is never given changes nothing and draws no random number.

#include "SexyAppBase.h"

namespace wb
{

// true when theCommand is one of the game's test commands (it has been answered on stdout)
bool HandleTestCommand(Sexy::SexyAppBase* theApp, const Sexy::ScriptCommand& theCommand);

} // namespace wb

#endif
