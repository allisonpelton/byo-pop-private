// The standard test commands (docs/TEST_COMMANDS.md): script commands that put the game in a state or read it back. They are
// reachable only from a test (-script, -commands and the web page's test call), so a normal run never meets them.
//
// state                prints STATE {"screen","scenes","mode","level","score","lives","timer","moves","profile","over","board"}
// goto <screen>        menu (or title), options, help, highscores, credits, journey, flood, survival, trickshot, or the name
//                      of any screen the game has; the scenes there are killed
// level <n>            starts level n (a trick shot's number in the Trick Shot mode) of the current mode, as a level is pushed
//                      by the game, over the scenes there (they are killed)
// unlock all           the flags that unlock Flood, Trick Shot and Survival, and every Journey and Flood level
// set <name> <value>   score, mode (a name or 0 to 3), level (the level the profile is on, one to count) or lives (the tries)
// win, lose            ends the level in play: the middle layer goes to its victory stage, or the level is lost as a
//                      shot that fell does (a try less in Journey)
// seed <n>             seeds the game's random numbers
// move                 prints MOVE {"kind","input"} for the bot's next shot (Bot.h), MOVE {"kind":"wait",...} while a level is not ready
//                      for one and MOVE null when no level is on screen
// after <n> <command>  runs the command once n frames have run since the first update after the loading (frame 0), so that a
//                      comparison with the original can give an input at the update the original took it
// trace <file>|off     writes a FRAME line at the end of each of those updates: the ticks it ran, the random seed and the
//                      values the original's memory holds at the same places (floats as their bits), for that comparison
// bot <frames>|off     the bot (Bot.h) plays for that many frames: its shots are given as input a frame at a time (the
//                      trace logs each as an INPUT line) instead of the move command's reply
// ticks <file>         the ticks each frame runs, one number a frame from frame 0 (the original's, taken from a run of it),
//                      whatever the clock says; after the list the clock decides again

#include "VeniceApp.h"

#include "Bot.h"
#include "GameText.h"
#include "Layer.h"
#include "Level.h"
#include "Plate.h"
#include "Profiles.h"
#include "SceneFiles.h"
#include "SceneLoader.h"
#include "SceneObject.h"
#include "Scoring.h"
#include "Slot.h"
#include "TextObject.h"
#include "Shooter.h"
#include "VeniceGlobals.h"
#include "VeniceMath.h"

#include "SexyAppBase.h"

#include <SDL.h>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <string>
#include <vector>

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

// adds an entry for each object of the tree that a click works on (an event of a stage of the object takes the click)
void AppendButtons(SceneObject* theObject, std::string* theButtons)
{
	const SceneEvent* aClickEvent = nullptr;
	for (const SceneStage* aStage = theObject->mStages.mHead; aStage != nullptr && aClickEvent == nullptr; aStage = aStage->mNext) {
		for (const SceneEvent* anEvent = aStage->mEvents.mHead; anEvent != nullptr; anEvent = anEvent->mNext) {
			if (anEvent->mTrigger == TriggerClick) {
				aClickEvent = anEvent;
				break;
			}
		}
	}
	if (aClickEvent != nullptr) {
		// the place the click works at: the object's own picture, or the rectangle that a text's letters cover
		float aCenterX = theObject->GetWorldX(theObject->mLocal[AttributeX], theObject->mLocal[AttributeY]);
		float aCenterY = theObject->GetWorldY(theObject->mLocal[AttributeX], theObject->mLocal[AttributeY]);
		float aWidth = (theObject->mSourceRight - theObject->mSourceLeft) * theObject->mLocal[AttributeXScale];
		float aHeight = (theObject->mSourceBottom - theObject->mSourceTop) * theObject->mLocal[AttributeYScale];
		if (theObject->mType == ObjectTypeText) {
			float aBoundsWidth;
			float aBoundsHeight;
			float aBoundsX;
			float aBoundsY;
			static_cast<TextObject*>(theObject)->GetBounds(&aBoundsWidth, &aBoundsHeight, &aBoundsX, &aBoundsY);
			aCenterX += aBoundsX * theObject->mLocal[AttributeXScale];
			aCenterY += aBoundsY * theObject->mLocal[AttributeYScale];
			aWidth = aBoundsWidth * theObject->mLocal[AttributeXScale];
			aHeight = aBoundsHeight * theObject->mLocal[AttributeYScale];
		}
		char aText[400];
		snprintf(aText, sizeof(aText), "%s{\"name\":%s,\"x\":%d,\"y\":%d,\"w\":%d,\"h\":%d,\"action\":%d,\"parm1\":%s}",
			theButtons->size() > 1 ? "," : "", Quoted(theObject->mName).c_str(), (int)aCenterX, (int)aCenterY, (int)aWidth,
			(int)aHeight, aClickEvent->mAction, Quoted(aClickEvent->mParm1).c_str());
		*theButtons += aText;
	}
	for (SceneObject* aChild = theObject->mChildren.mHead; aChild != nullptr; aChild = aChild->mNext)
		AppendButtons(aChild, theButtons);
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
		",\"profile\":" + Quoted(aProfile.mName) + ",\"over\":" + (PlayIsOver() ? "true" : "false");
	if (gActiveLevel != nullptr) {
		aState += ",\"timer\":" + std::to_string((long long)gActiveLevel->mTickCount) + ",\"moves\":" + std::to_string(gActiveLevel->mShotsFired) +
			",\"board\":" + BoardJson();
	}
	return aState + "}";
}

// port: a command that waits for a frame (after)
struct FrameCommand
{
	int mFrame;
	Sexy::ScriptCommand mCommand;
};

std::vector<FrameCommand> sFrameCommands; // by frame; those of one frame in the order given
std::vector<int> sTickSchedule; // the ticks each frame runs

// port: the bot's play inside the frames (bot <frames>): the move command's shots, as input given frame by frame
enum BotPhase
{
	BotPhaseChoosing,
	BotPhaseAiming,
	BotPhasePressed,
	BotPhaseRecovering,
};

struct BotPlay
{
	int mFramesLeft = 0;
	int mPhase = BotPhaseChoosing;
	int mWait = 0;
	int mX = 0;
	int mY = 0;
};

BotPlay sBotPlay;
FILE* sTraceFile = nullptr;
}
namespace
{

int FloatBits(float theValue)
{
	int aBits;
	memcpy(&aBits, &theValue, sizeof(aBits));
	return aBits;
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
	if (aName == "plates") {
		// every plate of the level in play: its shape, where it is and the kind and state of each slot
		if (gPlateLayer == nullptr || gActiveLevel == nullptr || gActiveLevel != gScenes.mTail) {
			Error(aLine, "no level is in play");
			return true;
		}
		std::string aPlates = "[";
		SceneObject* aPlate = nullptr;
		while ((aPlate = gPlateLayer->FindChildByType(ObjectTypePlate, aPlate)) != nullptr) {
			char aText[200];
			snprintf(aText, sizeof(aText), "%s{\"shape\":\"%s\",\"x\":%.2f,\"y\":%.2f,\"stage\":\"%s\",\"slots\":[", aPlates.size() > 1 ? "," : "",
				static_cast<Plate*>(aPlate)->mShapeName, aPlate->mWorld[AttributeX], aPlate->mWorld[AttributeY], aPlate->mStages.mCursor->mName);
			aPlates += aText;
			SceneObject* aSlot = nullptr;
			bool aFirst = true;
			while ((aSlot = aPlate->FindChildByType(ObjectTypeSlot, aSlot)) != nullptr) {
				snprintf(aText, sizeof(aText), "%s[%d,%d]", aFirst ? "" : ",", static_cast<Slot*>(aSlot)->mKind, static_cast<Slot*>(aSlot)->mFilled);
				aPlates += aText;
				aFirst = false;
			}
			aPlates += "]}";
		}
		Reply("PLATES " + aPlates + "]");
		return true;
	}
	if (aName == "buttons") {
		// the objects of the screen on top that a click works: where each is (its centre, in the 800 x 600 screen) and how big, for
		// a driver that clicks with real input
		if (gScenes.mTail == nullptr) {
			Error(aLine, "no screen is up");
			return true;
		}
		std::string aButtons = "[";
		AppendButtons(gScenes.mTail, &aButtons);
		Reply("BUTTONS " + aButtons + "]");
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
	if (aName == "save") {
		// the profiles file is written where the game writes it (a user made, a level ended), not at quit: a test that makes a
		// player for another run asks for it
		SaveProfiles();
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
	if (aName == "player") {
		// port: the name box's work for a test that does not type the name: the first free slot gets the name, and is the current
		// player; a player that is chosen already stays
		if (aProfile.mName[0] != ' ') {
			Ok(aLine);
			return true;
		}
		std::string aPlayerName = anArg.empty() ? "Test" : anArg;
		for (int aSlot = 0; aSlot < ProfileCount; aSlot++) {
			if (gProfiles[aSlot].mName[0] == ' ') {
				gCurrentProfile = aSlot;
				strncpy(gProfiles[aSlot].mName, aPlayerName.c_str(), ProfileNameSize - 1);
				Ok(aLine);
				return true;
			}
		}
		Error(aLine, "no free player slot");
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
	if (aName == "after") {
		std::string aCommandName;
		anArgs >> aCommandName;
		if (anArg.empty() || aCommandName.empty()) {
			Error(aLine, "after <frame> <command> [arguments]");
			return true;
		}
		FrameCommand aFrameCommand;
		aFrameCommand.mFrame = atoi(anArg.c_str());
		aFrameCommand.mCommand.mUpdateCount = 0;
		aFrameCommand.mCommand.mCommand = aCommandName;
		std::string aRest;
		std::getline(anArgs, aRest);
		size_t aStart = aRest.find_first_not_of(" \t");
		aFrameCommand.mCommand.mArgs = aStart == std::string::npos ? "" : aRest.substr(aStart);
		aFrameCommand.mCommand.mLine = theCommand.mLine;
		size_t aPosition = sFrameCommands.size();
		while (aPosition > 0 && sFrameCommands[aPosition - 1].mFrame > aFrameCommand.mFrame)
			aPosition--;
		sFrameCommands.insert(sFrameCommands.begin() + aPosition, aFrameCommand);
		Ok(aLine);
		return true;
	}
	if (aName == "bot") {
		sBotPlay = BotPlay();
		sBotPlay.mFramesLeft = anArg == "off" ? 0 : atoi(anArg.c_str());
		std::string aKeyword;
		anArgs >> aKeyword;
		gBotContinuesPastSummaries = aKeyword == "continue";
		Ok(aLine);
		return true;
	}
	if (aName == "ticks") {
		FILE* aFile = anArg.empty() ? nullptr : fopen(anArg.c_str(), "r");
		if (aFile == nullptr) {
			Error(aLine, "ticks <file>: cannot read it");
			return true;
		}
		sTickSchedule.clear();
		int aTicks;
		while (fscanf(aFile, "%d", &aTicks) == 1)
			sTickSchedule.push_back(aTicks);
		fclose(aFile);
		Ok(aLine);
		return true;
	}
	if (aName == "trace") {
		if (sTraceFile != nullptr) {
			fclose(sTraceFile);
			sTraceFile = nullptr;
		}
		if (anArg.empty()) {
			Error(aLine, "trace <file> or trace off");
			return true;
		}
		if (anArg != "off") {
			sTraceFile = fopen(anArg.c_str(), "w");
			if (sTraceFile == nullptr) {
				Error(aLine, "cannot open " + anArg);
				return true;
			}
		}
		Ok(aLine);
		return true;
	}
	if (aName == "move") {
		BotMove aMove = ChooseBotMove();
		if (aMove.mAction == BotAction::None)
			Reply("MOVE null");
		else if (aMove.mAction == BotAction::Wait)
			Reply("MOVE {\"kind\":\"wait\",\"input\":[{\"wait\":10}]}");
		else if (aMove.mAction == BotAction::Swap) {
			char aText[200];
			snprintf(aText, sizeof(aText), "MOVE {\"kind\":\"swap\",\"input\":[{\"rclick\":[%d,%d]},{\"wait\":20}]}", aMove.mX, aMove.mY);
			Reply(aText);
		} else if (aMove.mAction == BotAction::Click) {
			char aText[200];
			snprintf(aText, sizeof(aText),
				"MOVE {\"kind\":\"message\",\"input\":[{\"move\":[%d,%d]},{\"wait\":2},{\"press\":[%d,%d]},{\"wait\":2},{\"release\":[%d,%d]},{\"wait\":20}]}",
				aMove.mX, aMove.mY, aMove.mX, aMove.mY, aMove.mX, aMove.mY);
			Reply(aText);
		} else {
			char aText[240];
			snprintf(aText, sizeof(aText),
				"MOVE {\"kind\":\"shot\",\"input\":[{\"move\":[%d,%d]},{\"wait\":%d},{\"press\":[%d,%d]},{\"wait\":2},{\"release\":[%d,%d]},{\"wait\":20}]}",
				aMove.mX, aMove.mY, aMove.mUpdatesToAim, aMove.mX, aMove.mY, aMove.mX, aMove.mY);
			Reply(aText);
		}
		return true;
	}
	return false;
}

// port: the commands that were waiting for the frames done to reach their frame, before the next frame
void VeniceApp::RunFrameCommands(int theFramesDone)
{
	if (sBotPlay.mFramesLeft > 0) {
		sBotPlay.mFramesLeft--;
		switch (sBotPlay.mPhase) {
		case BotPhaseChoosing: {
			BotMove aMove = ChooseBotMove();
			if (aMove.mAction == BotAction::Swap) {
				sBotPlay.mX = aMove.mX;
				sBotPlay.mY = aMove.mY;
				GiveBotInput(theFramesDone, "rclick");
				sBotPlay.mPhase = BotPhaseRecovering;
				sBotPlay.mWait = 20;
			} else if (aMove.mAction == BotAction::Shoot || aMove.mAction == BotAction::Click) {
				sBotPlay.mX = aMove.mX;
				sBotPlay.mY = aMove.mY;
				GiveBotInput(theFramesDone, "move");
				sBotPlay.mPhase = BotPhaseAiming;
				sBotPlay.mWait = aMove.mAction == BotAction::Click ? 2 : aMove.mUpdatesToAim;
			}
			break;
		}
		case BotPhaseAiming:
			if (--sBotPlay.mWait <= 0) {
				GiveBotInput(theFramesDone, "down");
				sBotPlay.mPhase = BotPhasePressed;
				sBotPlay.mWait = 2;
			}
			break;
		case BotPhasePressed:
			if (--sBotPlay.mWait <= 0) {
				GiveBotInput(theFramesDone, "up");
				sBotPlay.mPhase = BotPhaseRecovering;
				sBotPlay.mWait = 20;
			}
			break;
		default:
			if (--sBotPlay.mWait <= 0)
				sBotPlay.mPhase = BotPhaseChoosing;
			break;
		}
	}
	while (!sFrameCommands.empty() && sFrameCommands.front().mFrame < theFramesDone) {
		const Sexy::ScriptCommand aCommand = sFrameCommands.front().mCommand;
		sFrameCommands.erase(sFrameCommands.begin());
		std::string anError;
		int aResult = RunScriptCommand(aCommand, &anError);
		if (aResult == SCRIPT_FAILED)
			SDL_Log("after: line %d: %s", aCommand.mLine, anError.c_str());
		else if (aResult == SCRIPT_UNKNOWN)
			SDL_Log("after: line %d: unknown command '%s %s'", aCommand.mLine, aCommand.mCommand.c_str(), aCommand.mArgs.c_str());
	}
}

// port: one input of the bot's play, run now and written to the trace as the frame it comes after (`after <frame>` gives it
// back), so that the original can be given the same
void VeniceApp::GiveBotInput(int theFramesDone, const char* theName)
{
	Sexy::ScriptCommand aCommand;
	aCommand.mUpdateCount = 0;
	aCommand.mCommand = theName;
	aCommand.mArgs = std::to_string(sBotPlay.mX) + " " + std::to_string(sBotPlay.mY);
	aCommand.mLine = 0;
	if (sTraceFile != nullptr)
		fprintf(sTraceFile, "INPUT f=%d %s %s\n", theFramesDone - 1, aCommand.mCommand.c_str(), aCommand.mArgs.c_str());
	std::string anError;
	RunScriptCommand(aCommand, &anError);
}

// port: the ticks a test gives a frame, or -1 when it gives none
int VeniceApp::ScheduledTicks(int theFrame)
{
	return theFrame >= 0 && theFrame < (int)sTickSchedule.size() ? sTickSchedule[theFrame] : -1;
}

// port: a line of the trace at the end of a frame (the comparison with the original reads the same values from its memory:
// 0x5b5174, the last scene's tick at 0x100, the shooter's x at 0xa8 (while the level is the last scene), the level's tick, hits (0x3b0) and shots fired (0x3ac),
// and the first profile's score)
void VeniceApp::TraceFrame(int theFrame, int theTicksRun)
{
	if (sTraceFile == nullptr)
		return;
	int aSceneTick = gScenes.mTail != nullptr ? FloatBits(gScenes.mTail->mTickCount) : 0;
	int aKinds[12];
	GetSlotKindCountsForTest(aKinds);
	bool aLevelIsThere = gActiveLevel != nullptr && gActiveLevel == gScenes.mTail; // the original's pointer outlives the level
	fprintf(sTraceFile, "FRAME f=%d ticks=%d seed=%d scene=%d shooter=%d level=%d hits=%d shots=%d score=%d middle=%d plates=%d kinds=%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n", theFrame,
		theTicksRun, (int)gRandomSeed, aSceneTick, aLevelIsThere && gShooter != nullptr ? FloatBits(gShooter->mWorld[AttributeX]) : 0,
		aLevelIsThere ? FloatBits(gActiveLevel->mTickCount) : 0, aLevelIsThere ? gActiveLevel->mHits : 0,
		aLevelIsThere ? gActiveLevel->mShotsFired : 0, gProfiles[0].mScore,
		aLevelIsThere && gMiddleLayer != nullptr ? FloatBits(gMiddleLayer->mWorld[AttributeY]) : 0,
		aLevelIsThere && gPlateLayer != nullptr ? FloatBits(gPlateLayer->mWorld[AttributeY]) : 0, aKinds[0], aKinds[1],
		aKinds[2], aKinds[3], aKinds[4], aKinds[5], aKinds[6], aKinds[7], aKinds[8], aKinds[9], aKinds[10], aKinds[11]);
}
