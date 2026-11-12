// The standard test commands (docs/TEST_COMMANDS.md): script commands that put the game in a state or read it back. They are
// reachable only from a test (-script, -commands and the web page's test call), so a normal run never meets them.
//
// state                prints STATE {"screen","scenes","mode","level","score","lives","timer","moves","profile","board"}
// goto <screen>        menu (or title), options, help, highscores, credits, journey, flood, survival, trickshot, or the name
//                      of any screen the game has; the scenes there are killed
// level <n>            starts level n (a trick shot's number in the Trick Shot mode) of the current mode, as a level is pushed
//                      by the game, over the scenes there (they are killed)
// unlock all           the flags that unlock Flood, Trick Shot and Survival, and every Journey and Flood level
// set <name> <value>   score, mode (a name or 0 to 3), level (the level the profile is on, one to count) or lives (the tries)
// win, lose            ends the level in play: the middle layer goes to its victory stage, or the level is lost as a
//                      shot that fell does (a try less in Journey)
// seed <n>             seeds the game's random numbers

#include "VeniceApp.h"

#include "GameText.h"
#include "Layer.h"
#include "Level.h"
#include "Plate.h"
#include "Profiles.h"
#include "SceneFiles.h"
#include "SceneLoader.h"
#include "SceneObject.h"
#include "Scoring.h"
#include "Shooter.h"
#include "VeniceGlobals.h"
#include "VeniceMath.h"

#include "SexyAppBase.h"

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <string>

namespace
{
const char* const ModeNames[GameModeCount] = {"flood", "journey", "survival", "trickshot"};

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

std::string Quoted(const char* theText)
{
	std::string aQuoted = "\"";
	for (const char* aLetter = theText; *aLetter != '\0'; aLetter++) {
		if (*aLetter == '"' || *aLetter == '\\')
			aQuoted += '\\';
		aQuoted += *aLetter;
	}
	return aQuoted + "\"";
}

int ModeFromName(const std::string& theName)
{
	for (int aMode = 0; aMode < GameModeCount; aMode++) {
		if (theName == ModeNames[aMode])
			return aMode;
	}
	char* anEnd = nullptr;
	long aNumber = strtol(theName.c_str(), &anEnd, 10);
	if (anEnd != theName.c_str() && *anEnd == '\0' && aNumber >= 0 && aNumber < GameModeCount)
		return (int)aNumber;
	return -1;
}

// the plates of the level in play, their slots and how many of them are filled
std::string BoardJson()
{
	if (gPlateLayer == nullptr || gShooter == nullptr)
		return "null";
	int aPlateCount = 0;
	int aSlotCount = 0;
	int aFilledCount = 0;
	SceneObject* aPlate = nullptr;
	while ((aPlate = gPlateLayer->FindChildByType(ObjectTypePlate, aPlate)) != nullptr) {
		aPlateCount++;
		aSlotCount += static_cast<Plate*>(aPlate)->mSlotCount;
		aFilledCount += static_cast<Plate*>(aPlate)->CountFilledSlots();
	}
	std::string aBoard = "{\"plates\":" + std::to_string(aPlateCount) + ",\"slots\":" + std::to_string(aSlotCount) +
		",\"filled\":" + std::to_string(aFilledCount) + ",\"middle\":" + Quoted(gMiddleLayer->mStages.mCursor == nullptr ? "" : gMiddleLayer->mStages.mCursor->mName) +
		",\"shooter\":" + Quoted(gShooter->mStages.mCursor == nullptr ? "" : gShooter->mStages.mCursor->mName) +
		",\"hits\":" + std::to_string(gActiveLevel->mHits) + "}";
	return aBoard;
}

std::string StateJson()
{
	Profile& aProfile = CurrentProfile();
	std::string aScenes = "[";
	for (SceneObject* aScene = gScenes.mHead; aScene != nullptr; aScene = aScene->mNext) {
		if (aScene != gScenes.mHead)
			aScenes += ",";
		aScenes += Quoted(aScene->mName);
	}
	aScenes += "]";
	std::string aState = "{\"screen\":" + Quoted(gScenes.mTail == nullptr ? "" : gScenes.mTail->mName) + ",\"scenes\":" + aScenes +
		",\"mode\":" + std::to_string(aProfile.mMode) + ",\"modename\":" + Quoted(ModeNames[aProfile.mMode]) +
		",\"level\":" + std::to_string(aProfile.mLevelIndex[aProfile.mMode] + 1) + ",\"score\":" + std::to_string(aProfile.mScore) +
		",\"lives\":" + std::to_string(aProfile.mTriesLeft[aProfile.mMode]) +
		",\"profile\":" + Quoted(aProfile.mName);
	if (gActiveLevel != nullptr) {
		aState += ",\"timer\":" + std::to_string((long long)gActiveLevel->mTickCount) + ",\"moves\":" + std::to_string(gActiveLevel->mShotsFired) +
			",\"board\":" + BoardJson();
	}
	return aState + "}";
}

// the scenes already there go, and a screen or level is pushed over them (they are removed at the next update)
void KillAllScenes()
{
	for (SceneObject* aScene = gScenes.mHead; aScene != nullptr; aScene = aScene->mNext)
		aScene->mIsDead = true;
}

// the screen a name stands for: the screens of the main menu's buttons, else a screen the game has under that name
const char* ScreenFileName(const std::string& theName)
{
	if (theName == "menu" || theName == "title")
		return "main_menu";
	if (theName == "options")
		return "options_menu";
	if (theName == "help")
		return "loading_instructions";
	if (theName == "highscores")
		return "high_scores_all";
	if (theName == "credits")
		return "loading_credits";
	return nullptr;
}

// the scene a mode's button of the mode select screen pushes: the event's text is translated when the screen is loaded
// and again when the button is clicked (the first gives PICK_LEVEL_SELECT_OR_LOADING, the second the scene's name)
void StartMode(int theMode)
{
	CurrentProfile().mMode = theMode;
	std::string aLoadedText = TranslateText("PICK_LEVEL_SELECT_OR_LOADING2");
	PushSceneByText(TranslateText(aLoadedText.c_str()));
}
}

// port: the script commands of a test (the framework passes the ones it does not know)
bool VeniceApp::HandleScriptCommand(const Sexy::ScriptCommand& theCommand)
{
	const std::string& aName = theCommand.mCommand;
	std::istringstream anArgs(theCommand.mArgs);
	std::string anArg;
	anArgs >> anArg;
	std::string aLine = aName + (theCommand.mArgs.empty() ? "" : " " + theCommand.mArgs);
	Profile& aProfile = CurrentProfile();

	if (aName == "state") {
		Reply("STATE " + StateJson());
		return true;
	}
	if (aName == "goto") {
		if (gScenes.mTail == nullptr || aProfile.mName[0] == '\0') {
			Error(aLine, "no player is chosen yet (the name screen is up)");
			return true;
		}
		int aMode = ModeFromName(anArg);
		const char* aScreenFile = ScreenFileName(anArg);
		if (aMode >= 0 && !isdigit((unsigned char)anArg[0])) {
			KillAllScenes();
			StartMode(aMode);
			Ok(aLine);
		} else if (aScreenFile != nullptr || SceneFileExists(anArg.c_str())) {
			KillAllScenes();
			PushSceneByName(aScreenFile != nullptr ? aScreenFile : anArg.c_str());
			Ok(aLine);
		} else
			Error(aLine, "unknown screen '" + anArg + "'");
		return true;
	}
	if (aName == "level") {
		int aLevel = atoi(anArg.c_str());
		if (aLevel < 1 || gScenes.mTail == nullptr || aProfile.mName[0] == '\0') {
			Error(aLine, aLevel < 1 ? "the level must be 1 or more" : "no player is chosen yet");
			return true;
		}
		int aMode = aProfile.mMode;
		aProfile.mLevelIndex[aMode] = aLevel - 1;
		gLevelNumber = aLevel;
		KillAllScenes();
		PushSceneByNumber(aMode == GameModeTrickShot ? 1000 + aLevel - 1 : aLevel);
		Ok(aLine);
		return true;
	}
	if (aName == "unlock") {
		if (anArg != "all") {
			Error(aLine, "unlock all");
			return true;
		}
		aProfile.mFlagReachedBuilding3 = 1;
		aProfile.mFlagFinishedRelaxed = 1;
		aProfile.mFlagFinishedGame = 1;
		aProfile.mLevelIndex[GameModeJourney] = (int)gRelaxedScenes.mTail->mNumber - 1;
		aProfile.mLevelIndex[GameModeFlood] = (int)gGameScenes.mTail->mNumber - 1;
		Ok(aLine);
		return true;
	}
	if (aName == "set") {
		std::string aValue;
		anArgs >> aValue;
		if (anArg == "score" && !aValue.empty())
			aProfile.mScore = atoi(aValue.c_str());
		else if (anArg == "lives" && !aValue.empty())
			aProfile.mTriesLeft[aProfile.mMode] = atoi(aValue.c_str());
		else if (anArg == "level" && !aValue.empty() && atoi(aValue.c_str()) >= 1)
			aProfile.mLevelIndex[aProfile.mMode] = atoi(aValue.c_str()) - 1;
		else if (anArg == "mode" && ModeFromName(aValue) >= 0)
			aProfile.mMode = ModeFromName(aValue);
		else {
			Error(aLine, "set score|lives|level <n>, or set mode <name or 0 to 3>");
			return true;
		}
		Ok(aLine);
		return true;
	}
	if (aName == "win" || aName == "lose") {
		if (gMiddleLayer == nullptr || gShooter == nullptr || gActiveLevel == nullptr || gActiveLevel != gScenes.mTail) {
			Error(aLine, "no level is in play");
			return true;
		}
		if (aName == "win")
			gMiddleLayer->GoToStageByName("victory");
		else
			gActiveLevel->LoseTry();
		Ok(aLine);
		return true;
	}
	if (aName == "seed") {
		if (anArg.empty()) {
			Error(aLine, "seed <n>");
			return true;
		}
		SeedRandom((uint32_t)strtoul(anArg.c_str(), nullptr, 10));
		Ok(aLine);
		return true;
	}
	return false;
}
