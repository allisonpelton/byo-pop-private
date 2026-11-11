#ifndef __WATERBUGS_TESTCOMMANDS_H__
#define __WATERBUGS_TESTCOMMANDS_H__

// Test hooks: the standard test commands of docs/TEST_COMMANDS.md for Water Bugs. They are reached only from a test
// (-script, -commands, the web test page's command call) through WaterBugsApp::HandleScriptCommand, and a command that
// is never given changes nothing and draws no random number.

#include "SexyAppBase.h"

namespace Platform
{
struct MemReport;
}

namespace wb
{

// The engine's holders for the "memreport" command (the framework answers the command, and counts its own pictures,
// the sounds and the mixer's songs): the art banks' pixels, the animations' frames, masks and tints, the run-length
// masks, the screen buffers. Reads the game's lists and changes nothing.
void AddMemReportHolders(Platform::MemReport* theReport);

// true when theCommand is one of the game's test commands (it has been answered on stdout)
bool HandleTestCommand(Sexy::SexyAppBase* theApp, const Sexy::ScriptCommand& theCommand);

} // namespace wb

#endif
