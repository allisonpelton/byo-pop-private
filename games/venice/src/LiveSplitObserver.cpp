// port: the autosplit block and its observer. See LiveSplitObserver.h.
//
// The block is the shared AutosplitBlock (platform/AutosplitBlock.h, layout 2; docs/WEB_PLAYER.md, "One design for every port"):
// a constant-initialised static object, so its magic exists only in .data's initialised bytes, once in the exe, and the
// AutosplitWriter keeps its counters, serials and the seq protocol. It is in every Windows and web build but written only once
// something asks for it, the page by calling byopop_livesplit_block or natively the "livesplit" feature of an enabled mod; until
// then active and self stay 0. The observer runs after each update of the scenes (RunDueUpdates), reads the scene stack, the
// level in play and the current profile only, and sees each update's end state, not what happened inside it.
//
// What the block calls a board is one pass through a mode: from the click on a mode of the mode menu until the game is back at
// the main menu (the mode menu closed, or a quit from the pause menu). It holds the tutorial, the maps, the levels, the
// summaries, the honor roll and the win scene of that mode, and it ends there, so a second click on a mode is a new board.
//
// Numbering: mode as the game's profile numbers it (Profiles.h, GameMode): 0 Flood, 1 Journey, 2 Survival, 3 Trick Shot; skill
// 0, the game has none. state is the screen on top (the stack's last scene): 0 a menu or a message (the main menu, the mode
// menu, the continue prompt, loading, the options), 1 the world map (level select), 2 a level in play (scrolling, or standing
// still for a trick shot; the tutorial counts), 3 the level won (its middle layer is in the victory stage), 4 the level lost
// (the defeat stage: sunk, or a Journey level out of tries), 5 a summary (the level's, the trick shot's, Survival's), 6 the
// honor roll (high scores), 7 the win scene, 8 the pause menu or the in-game menu over a level. level is the profile's
// level index of the mode (the building the player is on, from 0, or the trick shot's index), displayLevel the number the map
// shows (level + 1: the profile moves on at the victory, so it changes with the level clear). tick is the count of updates
// the scenes ran since the game started. displayScore and score are the profile's score (the one the HUD adds to). progress is
// -1. gameOwn holds two int32: the level in play's time in milliseconds (its updates times 4; 0 with none) and the tries left
// of the mode (Journey; 3 at the start of each level).
//
// Events, for the current board:
//   GO          Journey and Flood: the click away from the world map in a new game (the first update with the loading scene above
//               the map: its click's stage lasts 10 ms), not in a continued one. Trick Shot: the board's first update in a new
//               game, or the update the profile's progress is reset by "new game" of the continue prompt. Survival: the first
//               update with a level in play. A board that continues a saved game (the continue prompt's "continue", progress
//               above zero) has the RESUMED flag and no GO; a board that is already going on when the block is asked for has
//               none either
//   level clear Journey, Flood and Survival: the update the middle layer of the level in play enters its victory stage (Panel.cpp:
//               ProgressBar::CheckVictory, the last plate gone). Trick Shot: the update the count of trick shots made rises (the
//               missed shot's victory stage is not one)
//   game over   the update the middle layer enters its defeat stage (Level::LoseTry: a plate sank, or a Journey level with no try
//               left), once for a board: kind 0 Journey, 1 Flood, 2 Survival, 3 Trick Shot. A Journey try lost with tries left
//               is no game over (it has no defeat stage)
//   win         Journey and Flood: the world map comes up with the text CONGRATULATIONS! (the profile has passed the last building:
//               its level index + 1 is 72 or more, GameText.cpp: REGION_LEVEL); every mode: the win scene is on the stack
//               (GameText.cpp: NextTrickShot, and the map's click after CONGRATULATIONS!)
// A board's first update is a baseline for the level clear, the game over and the win, and a level scene's first update is a
// baseline for its own level clear and game over (a board seen already over has had its game over).

#include "LiveSplitObserver.h"

#ifdef VENICE_HAS_LIVESPLIT

#include "Level.h"
#include "Profiles.h"
#include "VeniceGlobals.h"
#include "platform/AutosplitBlock.h"
#include "platform/ModManager.h"

#include <SDL.h>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif
#include <cstdio>
#include <cstring>

// The block: constant-initialised, aligned, kept by the linker. Only the writer's Publish() writes it. The game id "VNDX" is
// Venice Deluxe, as "TSDX" is Typer Shark Deluxe.
extern "C" __attribute__((used, aligned(64))) volatile AutosplitBlock gLiveSplitBlock =
	AUTOSPLIT_BLOCK_INIT(AUTOSPLIT_GAME_ID('V', 'N', 'D', 'X'));

namespace
{

enum
{
	StateMenu = 0,
	StateMap = 1,
	StatePlay = 2,
	StateWon = 3,
	StateLost = 4,
	StateSummary = 5,
	StateHonorRoll = 6,
	StateWinScene = 7,
	StatePaused = 8,

	GameOverKindJourney = 0,
	GameOverKindFlood = 1,
	GameOverKindSurvival = 2,
	GameOverKindTrickShot = 3,

	// the level index + 1 at which the map says CONGRATULATIONS! (0x48 in GameText.cpp: REGION_LEVEL)
	CongratulationsLevelNumber = 72,
	FeatureCheckUpdates = 64,
	LevelTickMilliseconds = 4
};

// What one look at the scene stack shows
struct StackView
{
	SceneObject* mTop = nullptr;              // the scene on top
	SceneObject* mPlayLevel = nullptr;        // the last scene with the layers of a level in play (a level, the tutorial)
	int mCount = 0;                           // scenes on the stack
	bool mHasMainMenu = false;
	bool mHasContinuePrompt = false;          // the continue prompt (continue or new game) is up
	int mModeMenuIndex = -1;                  // where the mode menu is on the stack, -1 when it is not
	int mMapIndex = -1;                       // where the world map is
	int mLoadingIndex = -1;                   // where the last loading scene is
	bool mHasWinScene = false;
};

AutosplitWriter sWriter(&gLiveSplitBlock);    // the counters, serials, latched flags and the seq writes
int sUpdateCount = 0;                         // the updates of the scenes since the game started
int sNextFeatureCheck = 0;                    // the update at which the feature is looked up again while the block is off
bool sPageAsked = false;                      // web: the page's timer asked for the block (byopop_livesplit_block)
unsigned int sBoardSerial = 0;                // the boards started so far
bool sInBoard = false;                        // a mode has been chosen and the game has not gone back to the main menu since
bool sGoDone = false;                         // the board's GO has been written, or will not be
bool sResumed = false;                        // the board continues a saved game: it began with progress in its mode (the continue prompt)
bool sPreviousWinSceneUp = false;
bool sPreviousCongratulations = false;
SceneObject* sLevelScene = nullptr;           // the level in play at the previous update
float sLevelTicks = 0;                        // its update count then
bool sPreviousWon = false;                    // that level's middle layer was in the victory stage
bool sPreviousLost = false;                   // and in the defeat stage
int sPreviousTrickShotsMade = 0;
std::string sLogFileName;                     // -livesplitlog
FILE* sLogFile = nullptr;
// what the last logged line held, so that a line is written only when a splitter would see something new
unsigned int sLoggedKey[16];
bool sLogged = false;

// The "livesplit" feature of the highest-priority enabled mod that names it (as a game's ModFeatureEnabled)
bool FeatureOn()
{
	for (const Platform::ModInfo* aMod : Platform::ModManager::Get().GetEnabledMods())
	{
		if (aMod->mFeaturesJson.IsObject() && aMod->mFeaturesJson.Has("livesplit"))
		{
			const Platform::JsonValue& aValue = aMod->mFeaturesJson["livesplit"];
			return aValue.IsObject() || aValue.AsBool(false);
		}
	}
	return false;
}

// -livesplitlog's line for this write, when it differs from the last one in what a splitter acts on (the board, the events, the
// state, the level, the mode, the flags; not the tick, the score or the time)
void LogWrite(int theUpdate)
{
	if (sLogFileName.empty())
		return;
	const AutosplitBlock& aBlock = sWriter.Next();
	unsigned int aKey[16] = { aBlock.active, aBlock.flags, aBlock.boardSerial, aBlock.goCount, aBlock.levelClears, aBlock.gameOvers, aBlock.wins,
		(unsigned int) aBlock.state, (unsigned int) aBlock.level, (unsigned int) aBlock.displayLevel, (unsigned int) aBlock.mode,
		(unsigned int) aBlock.skill, aBlock.inGame, aBlock.deaths[0] | (aBlock.deaths[1] << 8) | (aBlock.deaths[2] << 16) | (aBlock.deaths[3] << 24),
		(unsigned int) aBlock.progress, 0 };
	if (sLogged && (memcmp(aKey, sLoggedKey, sizeof(aKey)) == 0))
		return;
	memcpy(sLoggedKey, aKey, sizeof(aKey));
	sLogged = true;
	if (sLogFile == nullptr)
	{
		sLogFile = fopen(sLogFileName.c_str(), "ab");
		if (sLogFile == nullptr)
		{
			SDL_Log("livesplit: cannot write %s", sLogFileName.c_str());
			sLogFileName.clear();
			return;
		}
	}
	char aLine[700];
	FormatAutosplitBlock(aBlock, true, aLine, sizeof(aLine));
	fprintf(sLogFile, "u=%d %s\n", theUpdate, aLine);
	fflush(sLogFile);
}

void SetOwn(int theLevelMilliseconds, int theTriesLeft)
{
	AutosplitBlock& aNext = sWriter.Next();
	memset(aNext.gameOwn, 0, sizeof(aNext.gameOwn));
	memcpy(aNext.gameOwn + 0, &theLevelMilliseconds, sizeof(theLevelMilliseconds));
	memcpy(aNext.gameOwn + 4, &theTriesLeft, sizeof(theTriesLeft));
}

void Activate()
{
	sWriter.Activate(SDL_GetPerformanceFrequency());
	AutosplitBlock& aNext = sWriter.Next();
	SetOwn(0, 0);
	sLogged = false;
	SDL_Log("livesplit: block at 0x%08x (layout %u, %u bytes)", (unsigned) aNext.self, (unsigned) AUTOSPLIT_LAYOUT_VERSION,
		(unsigned) AUTOSPLIT_SIZE_V2);
}

bool NameHas(const SceneObject* theScene, const char* thePart)
{
	return strstr(theScene->mName, thePart) != nullptr;
}

// The middle layer of a level as Level::Update finds it (0x40f090): the layer inside the first layer of the scene. A screen of
// menus has none; a level in play has it, with the plates' layer inside it.
SceneObject* MiddleLayerOf(SceneObject* theScene)
{
	SceneObject* anOuterLayer = theScene->FindChildByType(ObjectTypeLayer, nullptr);
	if (anOuterLayer == nullptr)
		return nullptr;
	return anOuterLayer->FindChildByType(ObjectTypeLayer, nullptr);
}

// A scene with a shooter and the layers of a level (Level::Update's gShooter and gMiddleLayer): a level in play, the tutorial,
// Survival's and the trick shots'. The maps, the summaries and the win scene have layers, but no shooter.
bool IsPlayLevel(SceneObject* theScene)
{
	return theScene->FindChildByType(ObjectTypeShooter, nullptr) != nullptr && MiddleLayerOf(theScene) != nullptr;
}

void LookAtStack(StackView& theStack)
{
	int anIndex = 0;
	for (SceneObject* aScene = gScenes.mHead; aScene != nullptr; aScene = aScene->mNext)
	{
		if (NameHas(aScene, "main_menu"))
			theStack.mHasMainMenu = true;
		if (NameHas(aScene, "continue_game"))
			theStack.mHasContinuePrompt = true;
		if (NameHas(aScene, "mode_select_menu"))
			theStack.mModeMenuIndex = anIndex;
		if (NameHas(aScene, "level_select"))
			theStack.mMapIndex = anIndex;
		if (NameHas(aScene, "loading"))
			theStack.mLoadingIndex = anIndex;
		if (NameHas(aScene, "win_sequence"))
			theStack.mHasWinScene = true;
		if (IsPlayLevel(aScene))
			theStack.mPlayLevel = aScene;
		theStack.mTop = aScene;
		anIndex++;
	}
	theStack.mCount = anIndex;
}

const char* StageNameOf(const SceneObject* theLayer)
{
	if (theLayer == nullptr || theLayer->mStages.mCursor == nullptr)
		return "";
	return theLayer->mStages.mCursor->mName;
}

int CountTrickShotsMade(const Profile& theProfile)
{
	int aCount = 0;
	for (int aRegion = 0; aRegion < TrickShotRegionCount; aRegion++)
		for (int aShot = 0; aShot < TrickShotsPerRegion; aShot++)
			if (theProfile.mTrickShotDone[aRegion][aShot] != 0)
				aCount++;
	return aCount;
}

// The screen on top, as the block's state numbers it
int StateOf(const StackView& theStack, bool isWon, bool isLost)
{
	const SceneObject* aTop = theStack.mTop;
	if (NameHas(aTop, "win_sequence"))
		return StateWinScene;
	if (NameHas(aTop, "high_scores"))
		return StateHonorRoll;
	if (NameHas(aTop, "summary"))
		return StateSummary;
	if (NameHas(aTop, "level_select"))
		return StateMap;
	if (NameHas(aTop, "pause") || NameHas(aTop, "in_game_menu"))
		return theStack.mPlayLevel != nullptr ? StatePaused : StateMenu;
	// a level in play is the top scene, or has a message of the cupid over it
	if (theStack.mPlayLevel == nullptr || (aTop != theStack.mPlayLevel && !NameHas(aTop, "cupid")))
		return StateMenu;
	if (isLost)
		return StateLost;
	return isWon ? StateWon : StatePlay;
}

} // namespace

void LiveSplitObserver::SetLogFile(const std::string& theFileName)
{
	sLogFileName = theFileName;
}

void LiveSplitObserver::AfterUpdate()
{
	sUpdateCount++;
	if (!sWriter.IsActive() && !sPageAsked)
	{
		// the mod list is looked at every 64 updates only: a copy of the enabled mods' list is made each time
		if (sUpdateCount < sNextFeatureCheck)
			return;
		sNextFeatureCheck = sUpdateCount + FeatureCheckUpdates;
		if (!FeatureOn())
			return;
	}
	AutosplitBlock& aNext = sWriter.Next();
	// On activation the board in play (or none) is new to the block: it is never given a GO
	bool isActivation = !sWriter.IsActive();
	if (isActivation)
		Activate();

	StackView aStack;
	LookAtStack(aStack);
	Profile& aProfile = CurrentProfile();
	int aMode = aProfile.mMode;
	if (aMode < 0 || aMode >= GameModeCount)
		aMode = GameModeJourney;
	int aLevelIndex = aProfile.mLevelIndex[aMode];
	int aTrickShotsMade = CountTrickShotsMade(aProfile);

	// The board: a mode was chosen (a scene is above the mode menu) and the game has not gone back to the main menu since
	bool isAboveModeMenu = aStack.mModeMenuIndex >= 0 && aStack.mCount > aStack.mModeMenuIndex + 1;
	bool wasInBoard = sInBoard;
	if (aStack.mCount == 0 || (sInBoard && aStack.mHasMainMenu && !isAboveModeMenu))
		sInBoard = false;
	else if (!sInBoard && isAboveModeMenu)
		sInBoard = true;
	else if (isActivation && aStack.mPlayLevel != nullptr && !aStack.mHasMainMenu)
		sInBoard = true;                    // asked for in the middle of a game
	bool isNewBoard = sInBoard && (!wasInBoard || isActivation);
	if (!sInBoard)
	{
		sWriter.ClearBoard();
		sPreviousWinSceneUp = false;
		sPreviousCongratulations = false;
		sLevelScene = nullptr;
		SetOwn(0, 0);
		aNext.tick = sUpdateCount;
		sWriter.Publish(SDL_GetPerformanceCounter());
		LogWrite(sUpdateCount);
		return;
	}

	SceneObject* aMiddleLayer = aStack.mPlayLevel != nullptr ? MiddleLayerOf(aStack.mPlayLevel) : nullptr;
	const char* aStage = StageNameOf(aMiddleLayer);
	bool isWon = strstr(aStage, "victory") != nullptr;
	bool isLost = strstr(aStage, "defea") != nullptr;
	bool isMapUp = aStack.mMapIndex >= 0;
	bool isCongratulations = isMapUp && (aMode == GameModeJourney || aMode == GameModeFlood) && aLevelIndex + 1 >= CongratulationsLevelNumber;
	bool isMapClicked = isMapUp && aStack.mLoadingIndex > aStack.mMapIndex;

	if (isNewBoard)
	{
		sBoardSerial++;
		sWriter.NewBoard(sBoardSerial);
		sGoDone = isActivation;             // a board that is going on when the block is asked for has no GO
		sResumed = (aMode == GameModeTrickShot ? (aLevelIndex + aTrickShotsMade) : aLevelIndex) > 0;
		sPreviousWinSceneUp = aStack.mHasWinScene;
		sPreviousCongratulations = isCongratulations;
		sLevelScene = nullptr;
		sPreviousTrickShotsMade = aTrickShotsMade;
		if (isActivation && isLost)
			sWriter.SeenOver();
	}

	// A level scene seen for the first time (or played again from its start) is a baseline for its own events
	bool isNewLevel = false;
	if (aStack.mPlayLevel != sLevelScene || (aStack.mPlayLevel != nullptr && aStack.mPlayLevel->mTickCount < sLevelTicks))
	{
		isNewLevel = true;
		sLevelScene = aStack.mPlayLevel;
		sPreviousWon = isWon;
		sPreviousLost = isLost;
	}
	sLevelTicks = aStack.mPlayLevel != nullptr ? aStack.mPlayLevel->mTickCount : 0;

	// A new game (no progress in the mode) may start the run; continuing a saved one does not
	bool isFresh = (aMode == GameModeTrickShot ? (aLevelIndex + aTrickShotsMade) : aLevelIndex) == 0;
	if (isFresh && !sGoDone && aStack.mHasContinuePrompt)
		sResumed = false;                   // "new game" of the continue prompt reset the progress
	if (!sGoDone)
	{
		bool isGo = false;
		if (aMode == GameModeJourney || aMode == GameModeFlood)
			isGo = !sResumed && isFresh && isMapClicked;
		else if (aMode == GameModeTrickShot)
			isGo = !sResumed && isFresh;
		else
			isGo = aStack.mPlayLevel != nullptr;
		if (isGo)
		{
			sGoDone = true;
			sWriter.Go(sUpdateCount);
			SDL_Log("livesplit: board %u GO at tick %d (mode %d)", sBoardSerial, sUpdateCount, aMode);
		}
	}

	sWriter.SetDisplayScore(aProfile.mScore);

	if (!isNewBoard)
	{
		bool isClear = false;
		if (aMode == GameModeTrickShot)
			isClear = aTrickShotsMade > sPreviousTrickShotsMade;
		else if (!isNewLevel)
			isClear = isWon && !sPreviousWon;
		if (isClear)
		{
			sWriter.LevelClear();
			SDL_Log("livesplit: board %u level clear %d (level %d) at tick %d", sBoardSerial, (int) aNext.boardLevelClears, aLevelIndex + 1,
				sUpdateCount);
		}
		if (!sWriter.HadGameOver() && !isNewLevel && isLost && !sPreviousLost)
		{
			int aKind = aMode == GameModeJourney ? GameOverKindJourney : (aMode == GameModeFlood ? GameOverKindFlood :
				(aMode == GameModeSurvival ? GameOverKindSurvival : GameOverKindTrickShot));
			sWriter.GameOver(aKind);
			SDL_Log("livesplit: board %u game over (kind %d) at tick %d", sBoardSerial, aKind, sUpdateCount);
		}
		bool isWinNow = (aStack.mHasWinScene && !sPreviousWinSceneUp) || (isCongratulations && !sPreviousCongratulations);
		if (isWinNow && !sWriter.HadWin())
		{
			sWriter.Win();
			SDL_Log("livesplit: board %u win at tick %d", sBoardSerial, sUpdateCount);
		}
	}
	sPreviousWon = isWon;
	sPreviousLost = isLost;
	sPreviousWinSceneUp = aStack.mHasWinScene;
	sPreviousCongratulations = isCongratulations;
	sPreviousTrickShotsMade = aTrickShotsMade;

	uint32_t aFlags = 0;
	if (sResumed)
		aFlags |= AUTOSPLIT_FLAG_RESUMED;
	if (Platform::ModManager::Get().GetScoreKey().empty())
		aFlags |= AUTOSPLIT_FLAG_VANILLA;
	int aState = StateOf(aStack, isWon, isLost);
	if (aState == StatePaused)
		aFlags |= AUTOSPLIT_FLAG_PAUSED;
	aNext.flags = aFlags;                   // the writer adds the game over's and the win's flags from its latches
	aNext.tick = sUpdateCount;
	aNext.state = aState;
	aNext.level = aLevelIndex;
	aNext.displayLevel = aLevelIndex + 1;
	aNext.score = aProfile.mScore;
	aNext.mode = aMode;
	aNext.skill = 0;
	aNext.progress = -1;
	SetOwn(aStack.mPlayLevel != nullptr ? (int) (aStack.mPlayLevel->mTickCount * LevelTickMilliseconds) : 0, aProfile.mTriesLeft[aMode]);

	sWriter.Publish(SDL_GetPerformanceCounter());
	LogWrite(sUpdateCount);
}

void LiveSplitObserver::AppDestroyed()
{
	if (sWriter.IsActive())
	{
		sWriter.Deactivate();
		SetOwn(0, 0);
		sWriter.Publish(SDL_GetPerformanceCounter());
		LogWrite(sUpdateCount);
	}
	if (sLogFile != nullptr)
	{
		fclose(sLogFile);
		sLogFile = nullptr;
	}
}

#ifdef __EMSCRIPTEN__
// The page's speedrun timer: the block's address in the game's memory (a HEAPU8 offset); asking turns its writing on from the
// next update, as the feature does natively (web/autosplit.js)
extern "C" EMSCRIPTEN_KEEPALIVE uint32_t byopop_livesplit_block()
{
	sPageAsked = true;
	return (uint32_t) (uintptr_t) &gLiveSplitBlock;
}
#endif

#endif
