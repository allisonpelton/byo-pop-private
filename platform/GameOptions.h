#ifndef __PLATFORM_GAMEOPTIONS_H__
#define __PLATFORM_GAMEOPTIONS_H__

#include <string>

// port: a game's improvements over its original, as options (docs/MOD_ROADMAP.md, "What a player gets"). Each fix (a bug
// of the original mended: "The fixes mod") and each lite change (a faster computation that may give another result:
// "The lite mod") is an option named by an id, on or off. A port asks FixOn("id") or LiteOn("id") at the one place where
// it would differ from its original, and keeps the original's path beside it. Nothing set means every option off: the
// original game, which is what every test and frozen baseline runs.
//
// Where a value comes from, the first that gives one:
//   1. the command line: -fix=<id> or -fix=<id>=off, -lite=<id> or -lite=<id>=off (one option each, and they may repeat),
//      then -original (every other option off);
//   2. the front end the app attached with SetOptionSource: in these ports the mod framework, whose enabled mod with the
//      feature "fixes" (or "lite") gives the value (ModOptionSource, platform/ModManager.cpp);
//   3. off.
// The game's code needs nothing else: no mod manager and no settings file, so a game builds and plays its improvements
// with or without the mod framework.
namespace Platform
{
enum OptionGroup
{
	OPTION_FIX,		// the fixes
	OPTION_LITE,	// the lite changes
	NUM_OPTION_GROUPS
};

// A front end's value of one option: true when it has one (*theValue set), false to leave it to the next source
typedef bool (*OptionSource)(OptionGroup theGroup, const char* theId, bool* theValue);
void SetOptionSource(OptionSource theSource);

bool OptionOn(OptionGroup theGroup, const char* theId);
inline bool FixOn(const char* theId) { return OptionOn(OPTION_FIX, theId); }
inline bool LiteOn(const char* theId) { return OptionOn(OPTION_LITE, theId); }

// A command line parameter of the options (-original, -fix, -lite), from SexyAppBase::HandleCmdLineParam: true when it
// was one of them
bool HandleOptionParam(const std::string& theName, const std::string& theValue);
// The command line's options forgotten (tests)
void ResetOptionParams();
}

#endif
