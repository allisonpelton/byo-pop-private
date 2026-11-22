// port: test hooks, the standard script commands (docs/TEST_COMMANDS.md), reachable only from a test (-script,
// -commands). A command that is never given changes nothing: no draw, no state, no random number. Every command answers
// with one line on stdout: OK <command>, ERROR <command>: <reason>, STATE {...} or MOVE {...}. The set:
//
//   state [bugs]          STATE {json}: screen (popcap, loading, transition, menu, intro, level, stats, victory, map,
//                         slots), sub (the screen's own state: the menu's window, the level's state, the map screen's),
//                         over (true when the lives are gone or the victory scene shows), update (the frame count),
//                         pending (a goto/level still on its way), transition (the doors: 0 open, 1 closing, 2 closed,
//                         3 opening), player (1 when a profile is current) and with it level (levels finished), levelNo
//                         (the level being played: levels finished + 1), score, lives, bananas, inLevel, percent (the
//                         ground cut so far), target (Player::GetTargetPercent), mode (the rope mode: 0 usual, 1 flame,
//                         2 smoke), shots (fired in this level), monkey {cell [column,row] the monkey points at, state
//                         (MonkeyState), shotPhase, cooldown, aim (1 when a rope would go: the preview is on), vertical,
//                         from, to (the stretch it would cut), locked, looking (startled by a bug), holding (a bonus is
//                         held), held (its BonusKind)}, rope {state (RopeState: 0 idle, 1 flying, 2 cutting, 3 snapped,
//                         4 aiming), vertical, from, to, cell}, field {cells (ground cells left), cut (cells cut so
//                         far), idle (no cut still landing)}, bugs (in the list), alive (alive), kinds (each bug's
//                         BugKind, in the list's order), menuButton (its centre in the 1920x1200 design space, and
//                         whether it is active). `state bugs` adds bugList: [kind, x, y, box width, box height, alive]
//                         for every bug, in design pixels
//   set <name> <value>    on the current player: progress (levels finished, so Play starts level value + 1), tutorial and
//                         walkthru (1 marks the tutorial or the walk-through done), kinds (the bug kinds in play, 2 to
//                         8), score, lives, bananas
//   win                   the percent filled reaches the level's target, so the level ends through its own check
//   lose                  the monkey is left with one life and loses it through Player::LoseLife (the rope's own loss),
//                         so the game ends as it does when the lives run out; in a level that is on
//   seed <n>              seeds the C library's generator (CrtSRand) and the game's own (GameRandomSeed, 0x4733b0's
//                         state). The game seeds the first itself at start (SexyApp's tick count; -seed=N replaces it)
//                         and the second at every level's start (LevelObj::InitCells: the level's number plus
//                         0x58d2764, so a level's ground is the same whatever came before), and the casino
//                         (Slots) seeds both from the tick count when it opens; so a run repeats when seed is given
//                         first and the casino is not entered (the same input, on the same updates)
//   player [name]         makes the named profile (default BOT) the current one, through Player::AddProfile as the
//                         new-player window does; an error while that window is open (answer it first: `move` types a
//                         name there). Given while the game loads, the menu then shows the sign at once
//   goto menu|map|level   on its way (the answer is OK at once; `state` has pending 1 until it is there). menu: from a
//                         level the in-level menu's quit answered yes (the game's own result), from the map screen its
//                         quit, from the stats and victory screens the click that goes on first. map: from a level the
//                         level is won (as `win`) and its tally clicked on, from the menu the Play button's own
//                         choice (an error before the first level is won: the map follows the first win). level: the
//                         level after the last won (`level` with that number)
//   level <n>             starts level n: the profile's progress is set to n - 1 by the steps FinishLevel takes for
//                         each level won (the map position, the bug kinds a world adds, the blocked bonuses), without the
//                         bananas and the score; the tutorial and the walk-through are marked done unless n is 1; then
//                         the menu's Play choice (or the map screen's go-on) starts it as the game does. From a level it
//                         goes through `goto menu` first. An error without a profile
//   move                  MOVE {what, input, updates} or MOVE null (the game is over): the bot's next move (Bot.cpp),
//                         input in the steps of docs/TEST_COMMANDS.md, in the 800x600 window's coordinates

#include "GameApp.h"
#include "Board.h"
#include "Bonus.h"
#include "Bot.h"
#include "BugObj.h"
#include "FillObj.h"
#include "GameWidget.h"
#include "InGameMenu.h"
#include "LevelObj.h"
#include "MainMenu.h"
#include "MenuButton.h"
#include "MetaGame.h"
#include "NewUser.h"
#include "Player.h"
#include "RopeObj.h"
#include "Stats.h"
#include "Transition.h"

#include "Compat2004.h"

#include <cstdio>
#include <cstdlib>
#include <sstream>
#include <string>

using namespace Sexy;

namespace
{

// A goto or level on its way: the command answers at once, and the update hook (TestHooksUpdate) takes the steps as the
// game gets to each place. A goal that does not come in a long while of updates is given up with an ERROR line
enum Goal
{
	GOAL_NONE = 0,
	GOAL_MENU = 1,
	GOAL_MAP = 2,
	GOAL_LEVEL = 3,
};

const int GOAL_GIVE_UP_UPDATES = 8000;
const char* const GOAL_NAMES[] = { "none", "menu", "map", "level" };

struct Pending
{
	int mGoal = GOAL_NONE;
	int mLevel = 0;           // the level to start (GOAL_LEVEL): 1 and up
	int mAge = 0;
	bool mRestarting = false; // a level was on when `level` came: the way back to the menu comes first
	bool mProgressApplied = false;
	bool mWinSent = false;
	bool mQuitSent = false;
};
Pending gPending;

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

LevelObj* LevelOnScreen()
{
	GameWidget* aWidget = gApp != nullptr ? gApp->mGameWidget : nullptr;
	if (aWidget == nullptr || aWidget->mScreen != SCREEN_LEVEL || aWidget->mBoard == nullptr)
		return nullptr;
	return aWidget->mBoard->mLevel;
}

bool HasProfile()
{
	return Player::sCurrent != nullptr && GetProfileName() != nullptr;
}

void ReportState(bool theBugs)
{
	Player* aPlayer = Player::sCurrent;
	std::ostringstream aJson;
	aJson << "{\"screen\":\"" << BotScreen() << "\",\"sub\":\"" << BotSubState() << "\",\"over\":"
		<< (BotGameOver() ? "true" : "false") << ",\"update\":" << (gApp != nullptr ? gApp->mUpdateCount : 0)
		<< ",\"pending\":" << (gPending.mGoal != GOAL_NONE ? 1 : 0) << ",\"transition\":" << Transition::GetPhase()
		<< ",\"player\":" << (aPlayer != nullptr ? 1 : 0);
	if (aPlayer != nullptr) {
		aJson << ",\"level\":" << aPlayer->mSavedProgress
			<< ",\"levelNo\":" << aPlayer->mSavedProgress + 1
			<< ",\"score\":" << aPlayer->mScore
			<< ",\"lives\":" << aPlayer->mLives
			<< ",\"bananas\":" << aPlayer->mBananas
			<< ",\"inLevel\":" << (aPlayer->mInLevel ? 1 : 0)
			<< ",\"percent\":" << aPlayer->mPercent
			<< ",\"target\":" << Player::GetTargetPercent()
			<< ",\"mode\":" << aPlayer->mRopeMode
			<< ",\"shots\":" << aPlayer->mShotsFired
			<< ",\"monkey\":{\"pos\":[" << aPlayer->mState[STATE_X] << "," << aPlayer->mState[STATE_Y] << "],\"cell\":[" << aPlayer->mCellX << "," << aPlayer->mCellY << "],\"state\":"
			<< aPlayer->mMonkeyState << ",\"shotPhase\":" << aPlayer->mShotPhase << ",\"cooldown\":"
			<< aPlayer->mShotCooldown << ",\"aim\":" << (aPlayer->mTargetTile != nullptr ? 1 : 0) << ",\"vertical\":"
			<< aPlayer->mSegVertical << ",\"from\":" << aPlayer->mSegFrom << ",\"to\":" << aPlayer->mSegTo
			<< ",\"locked\":" << aPlayer->mSegLocked << ",\"looking\":" << (aPlayer->mLooking ? 1 : 0)
			<< ",\"holding\":" << aPlayer->mHolding << ",\"held\":"
			<< (aPlayer->mHeld != nullptr ? aPlayer->mHeld->mKind : -1) << "}";
	}
	LevelObj* aLevel = static_cast<LevelObj*>(ObjBase::sLevel);
	if (aLevel != nullptr) {
		RopeObj* aRope = aLevel->mRope;
		if (aRope != nullptr)
			aJson << ",\"rope\":{\"state\":" << aRope->mState << ",\"vertical\":" << aRope->mVertical << ",\"from\":"
				<< aRope->mFrom << ",\"to\":" << aRope->mTo << ",\"cell\":[" << aRope->mCellX << "," << aRope->mCellY
				<< "]}";
		aJson << ",\"origin\":[" << ObjBase::sLevel->mState[STATE_X] << "," << ObjBase::sLevel->mState[STATE_Y] << "],\"field\":{\"cells\":" << aLevel->mCellCount - aLevel->mCellsCut << ",\"cut\":" << aLevel->mCellsCut
			<< ",\"idle\":" << (FillObj::IsIdle() ? 1 : 0) << "}";
	}
	aJson << ",\"bugs\":" << BugObj::sCount << ",\"alive\":" << BugObj::sAlive << ",\"kinds\":[";
	for (int i = 0; i < BugObj::sCount; i++) {
		BugObj* aBug = BugObj::Get(i);
		aJson << (i > 0 ? "," : "") << (aBug != nullptr ? aBug->mKind : -1);
	}
	aJson << "]";
	if (theBugs)
		aJson << ",\"bugList\":" << BotBugListJson() << ",\"menuButtons\":" << BotMenuJson();
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
	else if (theName == "bananas")
		aPlayer->mBananas = theValue;
	else {
		Error(theFull, "unknown name");
		return true;
	}
	Ok(theFull);
	return true;
}

// The profile as it is after thePlayed levels have been won: the steps Player::FinishLevel takes for the map, the bug
// kinds and the blocked bonuses (not the bananas, the score or the save)
void ApplyProgress(int thePlayed)
{
	Player* aPlayer = Player::sCurrent;
	aPlayer->mSavedProgress = 0;
	aPlayer->mMapLevel = 0;
	aPlayer->mPrevMapLevel = 0;
	aPlayer->mBugKinds = 2;
	aPlayer->mWorldProgress = 0.0f;
	aPlayer->mMapEvent = MAP_EVENT_NONE;
	for (int i = 0; i < thePlayed; i++) {
		int aStart;
		int aSize;
		int anEnd;
		int aMid;
		aPlayer->mSavedProgress = aPlayer->mSavedProgress + 1;
		aPlayer->mPrevMapLevel = aPlayer->mMapLevel;
		aPlayer->mMapLevel = aPlayer->MapPosition(aPlayer->mSavedProgress, &aStart, &aSize, &anEnd, &aMid,
			&aPlayer->mWorldProgress);
		aPlayer->Unblock(0);
		if (aPlayer->mMapEvent == MAP_EVENT_NEW_WORLD && aPlayer->mBugKinds < 8)
			aPlayer->mBugKinds = aPlayer->mBugKinds + 1;
		if (aPlayer->mMapEvent != MAP_EVENT_NONE) {
			aPlayer->mWorldProgress = 1.0f;
			aPlayer->Block(2);
			aPlayer->Block(3);
			aPlayer->Block(4);
			aPlayer->Block(14);
			aPlayer->Block(18);
		}
		aPlayer->mMapEvent = MAP_EVENT_NONE;
	}
	if (aPlayer->mLives < 1)
		aPlayer->mLives = PLAYER_LOST_LIVES_START;
	aPlayer->mDeaths = 0;
}

void GiveUp(const char* theWhy)
{
	std::string aGoal = GOAL_NAMES[gPending.mGoal];
	gPending = Pending();
	Reply("ERROR goto " + aGoal + ": " + theWhy);
}

// One step towards the menu: from a level the in-level menu is made and answered quit (what the menu button and its yes
// answer do); from the stats and the victory a click; from the map screen the quit flag. True once the menu is up
bool StepToMenu()
{
	GameWidget* aWidget = gApp->mGameWidget;
	if (aWidget->mScreen == SCREEN_MENU)
		return aWidget->mMainMenu != nullptr && Transition::GetPhase() == TRANSITION_OPEN;
	if (aWidget->mScreen == SCREEN_MAP && aWidget->mMetaGame != nullptr) {
		if (Transition::GetPhase() == TRANSITION_OPEN)
			aWidget->mMetaGame->mWantsMenu = true;
		return false;
	}
	LevelObj* aLevel = LevelOnScreen();
	if (aLevel == nullptr || Transition::GetPhase() != TRANSITION_OPEN)
		return false;
	switch (aLevel->mState) {
	case LEVEL_STATE_INTRO:
	case LEVEL_STATE_SETTLING:
	case LEVEL_STATE_PLAYING:
		if (aLevel->mMenu == nullptr && aLevel->mMenuButton != nullptr && !gPending.mQuitSent) {
			aLevel->mMenu = new InGameMenu();
			aLevel->Deactivate(aLevel->mMenuButton);
			aLevel->mMenu->mChoice = IN_GAME_MENU_QUIT;
			gPending.mQuitSent = true;
		}
		break;
	case LEVEL_STATE_STATS:
		if (aLevel->mStats != nullptr && aLevel->mStats->IsReady())
			ObjBase::sMouseHeld = true;
		break;
	case LEVEL_STATE_VICTORY:
		ObjBase::sMouseHeld = true;
		break;
	default:
		break;
	}
	return false;
}

void StepToMap()
{
	GameWidget* aWidget = gApp->mGameWidget;
	if (aWidget->mScreen == SCREEN_MENU) {
		MainMenu* aMenu = aWidget->mMainMenu;
		if (aMenu == nullptr || Transition::GetPhase() != TRANSITION_OPEN || aMenu->mMenuState != MENU_STATE_IDLE)
			return;
		if (!HasProfile() || Player::sCurrent->mSavedProgress < 1) {
			GiveUp("no level won yet, so there is no map (the map screen follows the first win)");
			return;
		}
		Player::StartMetaGame(-1, -1);
		aMenu->mChoice = MENU_CHOICE_PLAY;
		return;
	}
	LevelObj* aLevel = LevelOnScreen();
	if (aLevel == nullptr || Transition::GetPhase() != TRANSITION_OPEN)
		return;
	Player* aPlayer = Player::sCurrent;
	switch (aLevel->mState) {
	case LEVEL_STATE_PLAYING:
		if (!gPending.mWinSent && aPlayer != nullptr) {
			aPlayer->mPercent = Player::GetTargetPercent();
			gPending.mWinSent = true;
		}
		break;
	case LEVEL_STATE_STATS:
		if (aLevel->mStats != nullptr && aLevel->mStats->IsReady())
			ObjBase::sMouseHeld = true;
		break;
	case LEVEL_STATE_VICTORY:
		GiveUp("the game was won: the victory scene leads to the menu");
		break;
	default:
		break;
	}
}

void ApplyLevelProgress()
{
	if (gPending.mProgressApplied)
		return;
	ApplyProgress(gPending.mLevel - 1);
	if (gPending.mLevel > 1) {
		Player::sCurrent->mTutorialDone = 1;
		Player::sCurrent->mWalkThruDone = 1;
	}
	gPending.mProgressApplied = true;
}

void StepToLevel()
{
	GameWidget* aWidget = gApp->mGameWidget;
	if (!HasProfile()) {
		GiveUp("no player");
		return;
	}
	if (gPending.mRestarting) {
		if (StepToMenu())
			gPending.mRestarting = false;
		return;
	}
	if (aWidget->mScreen == SCREEN_MENU) {
		MainMenu* aMenu = aWidget->mMainMenu;
		if (aMenu == nullptr || Transition::GetPhase() != TRANSITION_OPEN || aMenu->mMenuState != MENU_STATE_IDLE ||
			!aMenu->mReady || aMenu->mButtonsArrived < MENU_BUTTONS)
			return;
		ApplyLevelProgress();
		// the level comes through the board (the map screen only follows a won level)
		Player::EndMetaGame();
		aMenu->mChoice = MENU_CHOICE_PLAY;
		return;
	}
	if (aWidget->mScreen == SCREEN_MAP && aWidget->mMetaGame != nullptr) {
		if (Transition::GetPhase() != TRANSITION_OPEN)
			return;
		ApplyLevelProgress();
		aWidget->mMetaGame->mLeaving = true;
	}
}

bool GoalReached()
{
	GameWidget* aWidget = gApp->mGameWidget;
	switch (gPending.mGoal) {
	case GOAL_MENU:
		return StepToMenu();
	case GOAL_MAP:
		return aWidget->mScreen == SCREEN_MAP && aWidget->mMetaGame != nullptr &&
			Transition::GetPhase() == TRANSITION_OPEN;
	case GOAL_LEVEL: {
		LevelObj* aLevel = LevelOnScreen();
		return !gPending.mRestarting && gPending.mProgressApplied && aLevel != nullptr &&
			aLevel->mState == LEVEL_STATE_PLAYING && Player::sCurrent != nullptr &&
			Player::sCurrent->mSavedProgress == gPending.mLevel - 1 && Transition::GetPhase() == TRANSITION_OPEN;
	}
	}
	return true;
}

} // namespace

// The pending goto or level takes a step; called once an update by GameWidget::Update (a test's flag, nothing else)
void Sexy::TestHooksUpdate()
{
	if (gPending.mGoal == GOAL_NONE || gApp == nullptr || gApp->mGameWidget == nullptr)
		return;
	GameWidget* aWidget = gApp->mGameWidget;
	// the screens before the menu only wait
	if (aWidget->mScreen == SCREEN_POPCAP || aWidget->mScreen == SCREEN_LOADING)
		return;
	gPending.mAge++;
	if (gPending.mAge > GOAL_GIVE_UP_UPDATES) {
		GiveUp("not reached in time");
		return;
	}
	if (GoalReached()) {
		gPending = Pending();
		return;
	}
	switch (gPending.mGoal) {
	case GOAL_MENU:
		break;
	case GOAL_MAP:
		StepToMap();
		break;
	case GOAL_LEVEL:
		StepToLevel();
		break;
	}
}

bool GameApp::HandleScriptCommand(const ScriptCommand& theCommand)
{
	const std::string& aName = theCommand.mCommand;
	std::string aFull = aName + (theCommand.mArgs.empty() ? "" : " " + theCommand.mArgs);
	if (aName == "state") {
		ReportState(theCommand.mArgs == "bugs");
		return true;
	}
	if (aName == "move") {
		if (!theCommand.mArgs.empty())
			return false; // `move <point>` is the framework's pointer command
		std::string aMove = BotMoveJson();
		Reply(aMove == "null" ? "MOVE null" : "MOVE " + aMove);
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
	if (aName == "lose") {
		Player* aPlayer = Player::sCurrent;
		LevelObj* aLevel = LevelOnScreen();
		if (aPlayer == nullptr || aLevel == nullptr || aLevel->mState != LEVEL_STATE_PLAYING ||
			aPlayer->mMonkeyState != MONKEY_PLAYING) {
			Error(aFull, "the monkey is not playing in a level");
			return true;
		}
		// the last life, then the rope's own loss of it: the monkey is tangled, falls and dies, and the game over
		// follows as it does when the last life goes
		aPlayer->mLives = 1;
		Player::LoseLife();
		Ok(aFull);
		return true;
	}
	if (aName == "seed") {
		if (theCommand.mArgs.empty()) {
			Error(aFull, "seed <n>");
			return true;
		}
		unsigned int aSeed = (unsigned int)strtoul(theCommand.mArgs.c_str(), nullptr, 0);
		CrtSRand(aSeed);
		GameRandomSeed(aSeed);
		Ok(aFull);
		return true;
	}
	if (aName == "player") {
		if (mGameWidget != nullptr && mGameWidget->mMainMenu != nullptr && mGameWidget->mMainMenu->mNewUser != nullptr) {
			Error(aFull, "the new-player window is open: `move` types a name there");
			return true;
		}
		std::string aPlayerName = theCommand.mArgs.empty() ? "BOT" : theCommand.mArgs;
		if (Player::sProfiles[0] == nullptr) {
			Error(aFull, "the profiles are not made yet");
			return true;
		}
		for (int i = 0; i < PLAYER_PROFILES; i++) {
			if (Player::sProfiles[i]->mInUse != 0 && !Player::NameIsFree(aPlayerName.c_str(), i)) {
				Player::SetCurrent(i);
				Ok(aFull);
				return true;
			}
		}
		if (Player::AddProfile(-1, aPlayerName.c_str()) < 0) {
			Error(aFull, "no free profile, or the name is not usable");
			return true;
		}
		Ok(aFull);
		return true;
	}
	if (aName == "goto" || aName == "level") {
		std::string aWhere = aName == "level" ? "level" : theCommand.mArgs;
		int aLevelNumber = 0;
		if (aWhere != "menu" && aWhere != "map" && aWhere != "level") {
			Error(aFull, "goto menu, map or level");
			return true;
		}
		if (mGameWidget == nullptr) {
			Error(aFull, "the game is not up");
			return true;
		}
		if (aWhere == "level") {
			if (!HasProfile()) {
				Error(aFull, "no player (`player <name>` first)");
				return true;
			}
			if (aName == "level") {
				aLevelNumber = atoi(theCommand.mArgs.c_str());
				if (aLevelNumber < 1) {
					Error(aFull, "level <n> with n from 1");
					return true;
				}
			} else {
				aLevelNumber = Player::sCurrent->mSavedProgress + 1;
			}
		}
		gPending = Pending();
		gPending.mGoal = aWhere == "menu" ? GOAL_MENU : aWhere == "map" ? GOAL_MAP : GOAL_LEVEL;
		gPending.mLevel = aLevelNumber;
		// a level on screen goes back to the menu first
		gPending.mRestarting = gPending.mGoal == GOAL_LEVEL && mGameWidget->mScreen == SCREEN_LEVEL;
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
