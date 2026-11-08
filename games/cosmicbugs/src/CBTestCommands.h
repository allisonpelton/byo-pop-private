#ifndef __COSMICBUGS_CBTESTCOMMANDS_H__
#define __COSMICBUGS_CBTESTCOMMANDS_H__

// port: the standard test commands (docs/TEST_COMMANDS.md), reached only from a test (the exe's -script and live
// command channel through CosmicBugsApp::HandleScriptCommand, the headless runner's "cmd" event). A normal run never
// reaches them, so unmodded play is the original's. Each command answers with lines: STATE {...}, MOVE {...}|null,
// OK <command> or ERROR <command>: <reason>.

#include <string>
#include <vector>

namespace cb
{

// Runs one command; false when it is none of the standard ones (the caller passes it on). The answer lines are
// appended to theReplies.
bool TestCommand(const std::string& theCommand, const std::string& theArgs, std::vector<std::string>& theReplies);

} // namespace cb

#endif
