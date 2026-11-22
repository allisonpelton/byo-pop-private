// port: test hooks, the standard script commands (docs/TEST_COMMANDS.md), reachable only from a test (-script). A
// command that is never given changes nothing: no draw, no state. Every command answers with one line on stdout:
// OK <command>, ERROR <command>: <reason> or STATE {...}. The set so far:
//
//   state                 STATE {json}: player, level (levels finished), score, lives, bananas, bugs (in the list),
//                         alive (the bugs alive), kinds (each bug's BugKind, in the list's order),
//                         menuButton (its centre in the 1920x1200 design space, and whether it is active)
//   set <name> <value>    on the current player: progress (levels finished, so Play starts level value + 1), tutorial and
//                         walkthru (1 marks the tutorial or the walk-through done), kinds (the
//                         bug kinds in play, 2 to 8), score, lives
//   win                   the percent filled reaches the level's target, so the level ends through its own check

#include "GameApp.h"
#include "BugObj.h"
#include "LevelObj.h"
#include "MenuButton.h"
#include "Player.h"

#include <cstdio>
#include <cstdlib>
#include <sstream>
#include <string>

using namespace Sexy;

namespace
{

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

void ReportState()
{
	Player* aPlayer = Player::sCurrent;
	std::ostringstream aJson;
	aJson << "{\"player\":" << (aPlayer != nullptr ? 1 : 0);
	if (aPlayer != nullptr) {
		aJson << ",\"level\":" << aPlayer->mSavedProgress
			<< ",\"score\":" << aPlayer->mScore
			<< ",\"lives\":" << aPlayer->mLives
			<< ",\"bananas\":" << aPlayer->mBananas
			<< ",\"inLevel\":" << (aPlayer->mInLevel ? 1 : 0);
	}
	aJson << ",\"bugs\":" << BugObj::sCount << ",\"alive\":" << BugObj::sAlive << ",\"kinds\":[";
	for (int i = 0; i < BugObj::sCount; i++) {
		BugObj* aBug = BugObj::Get(i);
		aJson << (i > 0 ? "," : "") << (aBug != nullptr ? aBug->mKind : -1);
	}
	aJson << "]";
	LevelObj* aLevel = static_cast<LevelObj*>(ObjBase::sLevel);
	if (aLevel != nullptr && aLevel->mMenuButton != nullptr) {
		MenuButton* aButton = aLevel->mMenuButton;
		aJson << ",\"menuButton\":{\"x\":" << aButton->mScreenX << ",\"y\":" << aButton->mScreenY
			<< ",\"active\":" << (aButton->IsActive() ? 1 : 0) << "}";
	}
	aJson << "}";
	Reply("STATE " + aJson.str());
}

bool SetValue(const std::string& theFull, const std::string& theName, int theValue)
{
	Player* aPlayer = Player::sCurrent;
	if (aPlayer == nullptr) {
		Error(theFull, "no current player");
		return true;
	}
	if (theName == "progress")
		aPlayer->mSavedProgress = theValue;
	else if (theName == "tutorial")
		aPlayer->mTutorialDone = theValue;
	else if (theName == "walkthru")
		aPlayer->mWalkThruDone = theValue;
	else if (theName == "kinds")
		aPlayer->mBugKinds = theValue;
	else if (theName == "score")
		aPlayer->mScore = theValue;
	else if (theName == "lives")
		aPlayer->mLives = theValue;
	else {
		Error(theFull, "unknown name");
		return true;
	}
	Ok(theFull);
	return true;
}

} // namespace

bool GameApp::HandleScriptCommand(const ScriptCommand& theCommand)
{
	const std::string& aName = theCommand.mCommand;
	std::string aFull = aName + (theCommand.mArgs.empty() ? "" : " " + theCommand.mArgs);
	if (aName == "state") {
		ReportState();
		return true;
	}
	if (aName == "win") {
		Player* aPlayer = Player::sCurrent;
		if (aPlayer == nullptr || !aPlayer->mInLevel) {
			Error(aFull, "not in a level");
			return true;
		}
		// The level's own check (LevelObj's playing state) ends it once the fill reaches the target
		aPlayer->mPercent = Player::GetTargetPercent();
		Ok(aFull);
		return true;
	}
	if (aName == "set") {
		std::istringstream anArgs(theCommand.mArgs);
		std::string aKey;
		int aValue = 0;
		if (!(anArgs >> aKey >> aValue)) {
			Error(aFull, "expected set <name> <value>");
			return true;
		}
		return SetValue(aFull, aKey, aValue);
	}
	return false;
}
