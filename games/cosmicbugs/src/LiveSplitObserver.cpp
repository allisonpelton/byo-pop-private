// port: the autosplit block and its observer. See LiveSplitObserver.h.
//
// The block is the shared AutosplitBlock (platform/AutosplitBlock.h, layout 2; docs/WEB_PLAYER.md, "One design for every port"):
// a constant-initialised static object, so its magic exists only in .data's initialised bytes, once in the exe, and the
// AutosplitWriter keeps its counters, serials and the seq protocol. It is in every Windows and web build but written only once
// something asks for it, the page by calling byopop_livesplit_block or natively the "livesplit" feature of an enabled mod; until
// then active and self stay 0. The observer runs after each app update (CosmicBugsApp::RunFrame: ten of the engine's steps, one
// for each millisecond), reads the engine's current screen and its globals only, and sees each update's end state, not what
// happened inside it.
//
// What a game is. The engine has a list of screens (cb::gLevels) and one of them is current; a game is the boards played from
// the one a screen other than a board goes to (cb::ChangeScreen counts them in cb::gGameSerial, the way a board's serial is
// kept in the other ports), through each board's win to the next, until the game over, the mission-complete screen or the
// player leaves for a menu. The block's board serial is that game's, and it is 0 on every other screen. The mission-complete
// screen after the last board (board 577: the shipped game file's one hand-made board and its 576 random ones) is the same
// game's end, so it keeps the serial.
//
// Events, for the current game:
//   GO          the update in which a board is entered from the instructions page (its "Click here to play" button: the click is
//               handled in that update's instructions screen, which is left and replaced by the board in the same update). The
//               instructions page is what a profile that has reached no board gets from Start Game (CBMenu.cpp, ITEM_SELECT_LEVEL),
//               and what the title's Instructions button shows. A game entered from the level selection or the resume question
//               (a saved game) raises no GO and has AUTOSPLIT_FLAG_RESUMED; so does a game first seen at activation
//   level clear the board's state becomes LEVEL_WON (Level_Won: the fill reached the board's quota with nothing filling, which is
//               also when the best fill and fewest shots are recorded and the board counts as reached; the bonus texts and the click
//               through them follow)
//   game over   the board's mGameOver is set (Level_GameOver: the ship was hit with no lives left; "GAME OVER" is shown until a
//               click), kind 0; or the game is left for a menu before its end (Esc pauses a board and Esc again goes to the
//               title, where the game is saved), kind 1. Kinds 2 and 3 are not used
//   win         the mission-complete screen becomes current after the game's last board
// A game's first update is a baseline for the level clear and the game over (a board first seen won or over at activation has
// had its level clear or game over).

#include "LiveSplitObserver.h"

#ifdef COSMICBUGS_HAS_LIVESPLIT

#include "CosmicBugsApp.h"
#include "CBGlobals.h"
#include "CBLevel.h"
#include "platform/AutosplitBlock.h"
#include "platform/ModManager.h"

#include <SDL.h>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif
#include <cstdio>
#include <cstring>

using namespace Sexy;

// The block: constant-initialised, aligned, kept by the linker. Only the writer's Publish() writes it. The game id "CBDX" is
// Cosmic Bugs Deluxe, as "BWDX" is BookWorm Deluxe.
extern "C" __attribute__((used, aligned(64))) volatile AutosplitBlock gLiveSplitBlock =
	AUTOSPLIT_BLOCK_INIT(AUTOSPLIT_GAME_ID('C', 'B', 'D', 'X'));

namespace
{

// What the block's `state` holds: the board's state (cb::LevelState: 1 playing, 2 paused, 3 won) and two more
enum
{
	STATE_PLAYING = cb::LEVEL_PLAYING,
	STATE_PAUSED = cb::LEVEL_PAUSED,
	STATE_WON = cb::LEVEL_WON,
	STATE_GAME_OVER = 4,        // the board's mGameOver is set
	STATE_MISSION_COMPLETE = 5  // the mission-complete screen is current
};

enum
{
	MODE_GAME = 0               // the one mode: a game of boards (the shooter mode, manual or auto, is gameOwn's third word)
};

enum
{
	KIND_OUT_OF_LIVES = 0,      // the game over of the last life
	KIND_LEFT = 1               // the game left for a menu before its end
};

AutosplitWriter		sWriter(&gLiveSplitBlock);   // the counters, serials, latched flags and the seq writes
int					sPrevState = -1;            // the current game's state at the previous update
bool				sPageAsked = false;         // web: the page's timer asked for the block (byopop_livesplit_block)
int					sNextFeatureCheck = 0;      // the update at which the feature is looked up again while the block is off
std::string			sLogFileName;               // -livesplitlog
FILE*				sLogFile = NULL;
// what the last logged line held, so that a line is written only when a splitter would see something new
unsigned int		sLoggedKey[16];
bool				sLogged = false;

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
// state, the level, the mode, the skill and the flags; not the tick or the score)
void LogWrite(int theUpdate)
{
	if (sLogFileName.empty())
		return;
	const AutosplitBlock& aBlock = sWriter.Next();
	unsigned int aKey[16] = { aBlock.active, aBlock.flags, aBlock.boardSerial, aBlock.goCount, aBlock.levelClears, aBlock.gameOvers, aBlock.wins,
		(unsigned int) aBlock.state, (unsigned int) aBlock.level, (unsigned int) aBlock.displayLevel, (unsigned int) aBlock.mode,
		(unsigned int) aBlock.skill, aBlock.inGame, aBlock.deaths[0] | (aBlock.deaths[1] << 8) | (aBlock.deaths[2] << 16) | (aBlock.deaths[3] << 24), 0, 0 };
	if (sLogged && (memcmp(aKey, sLoggedKey, sizeof(aKey)) == 0))
		return;
	memcpy(sLoggedKey, aKey, sizeof(aKey));
	sLogged = true;
	if (sLogFile == NULL)
	{
		sLogFile = fopen(sLogFileName.c_str(), "ab");
		if (sLogFile == NULL)
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

void Activate()
{
	sWriter.Activate(SDL_GetPerformanceFrequency());
	AutosplitBlock& aNext = sWriter.Next();
	sLogged = false;
	SDL_Log("livesplit: block at 0x%08x (layout %u, %u bytes)", (unsigned) aNext.self, (unsigned) AUTOSPLIT_LAYOUT_VERSION,
		(unsigned) AUTOSPLIT_SIZE_V2);
}

// The writer's fields the game's own words hold: the mode and the skill as in the layout-1 view, then the shooter mode
void SetOwnWords(int theMode, int theSkill, int theShooterMode)
{
	AutosplitBlock& aNext = sWriter.Next();
	memcpy(aNext.gameOwn + 0, &theMode, sizeof(theMode));
	memcpy(aNext.gameOwn + 4, &theSkill, sizeof(theSkill));
	memcpy(aNext.gameOwn + 8, &theShooterMode, sizeof(theShooterMode));
}

} // namespace

void LiveSplitObserver::SetLogFile(const std::string& theFileName)
{
	sLogFileName = theFileName;
}

void LiveSplitObserver::AfterUpdate(CosmicBugsApp* theApp)
{
	if (!sWriter.IsActive() && !sPageAsked)
	{
		// the mod list is looked at every 64 updates only: a copy of the enabled mods' list is made each time
		if (theApp->mUpdateCount < sNextFeatureCheck)
			return;
		sNextFeatureCheck = theApp->mUpdateCount + 64;
		if (!FeatureOn())
			return;
	}
	AutosplitBlock& aNext = sWriter.Next();
	// On activation the current game (or none) is new to the block: it is never given a GO
	bool isActivation = !sWriter.IsActive();
	if (isActivation)
		Activate();

	// The game on screen: the one a board belongs to, and on the mission-complete screen the one that was on screen at the
	// previous update (a game's end, not another game); none on any other screen
	cb::Level* aLevel = cb::gLevels.mCurrent;
	int aKind = (aLevel != NULL) ? aLevel->mKind : -1;
	bool isMissionComplete = (aKind == cb::LEVEL_MISSION_COMPLETE) && (aNext.boardSerial != 0);
	uint32_t aSerial = 0;
	if (aKind == cb::LEVEL_BOARD)
		aSerial = (uint32_t) cb::gGameSerial;
	else if (isMissionComplete)
		aSerial = aNext.boardSerial;
	int aUpdate = theApp->mUpdateCount;

	bool isNewBoard = isActivation || (aSerial != aNext.boardSerial);
	if (isNewBoard)
	{
		if ((!isActivation) && (aNext.boardSerial != 0) && (!sWriter.HadGameOver()) && (!sWriter.HadWin()))
		{
			// the game on screen was left for another screen before its end: a game over of its own kind, written with the
			// game's last fields (the flags among them) before the block shows no game
			aNext.tick = aUpdate;
			sWriter.GameOver(KIND_LEFT);
			SDL_Log("livesplit: board %u game over (kind %d, left) at tick %d", (unsigned) aNext.boardSerial, KIND_LEFT, aUpdate);
			sWriter.Publish(SDL_GetPerformanceCounter());
			LogWrite(aUpdate);
		}
		sWriter.NewBoard(aSerial);
		sPrevState = -1;
	}

	if (aSerial != 0)
	{
		bool isOver = (aKind == cb::LEVEL_BOARD) && (aLevel->mGameOver != 0);
		int aState = isMissionComplete ? STATE_MISSION_COMPLETE : (isOver ? STATE_GAME_OVER : aLevel->mState);
		bool isFromInstructions = cb::gGameFromInstructions;
		int aGameOverKind = -1;     // the kind of game over this update raised, -1 for none
		if (isNewBoard)
		{
			// The baseline: a game seen over has had its game over, and its state is the previous one
			if (isOver)
				sWriter.SeenOver();
			sPrevState = aState;
			if (!isActivation && isFromInstructions)
			{
				// the update of the instructions page's Play click
				sWriter.Go(aUpdate);
				SDL_Log("livesplit: board %u GO at tick %d (board %d, difficulty %d, shooter %d)", (unsigned) aSerial, aUpdate,
					aLevel->mNumber, cb::gDifficulty, cb::gAutoFire);
			}
			else if (!isActivation)
				SDL_Log("livesplit: board %u starts from another screen than the instructions page: GO not counted", (unsigned) aSerial);
		}
		else
		{
			if ((aState == STATE_WON) && (sPrevState != STATE_WON))
			{
				sWriter.LevelClear();
				SDL_Log("livesplit: board %u level clear %d (board %d) at tick %d", (unsigned) aSerial, (int) aNext.boardLevelClears,
					aLevel->mNumber, aUpdate);
			}
			if (isMissionComplete && !sWriter.HadWin() && !sWriter.HadGameOver())
			{
				sWriter.Win();
				SDL_Log("livesplit: board %u win at tick %d", (unsigned) aSerial, aUpdate);
			}
			if (isOver && !sWriter.HadGameOver())
			{
				aGameOverKind = KIND_OUT_OF_LIVES;
				SDL_Log("livesplit: board %u game over (kind %d, out of lives) at tick %d", (unsigned) aSerial, aGameOverKind, aUpdate);
			}
		}

		// The score on screen is the game's score; the high-water mark follows it, and the game-over update's own score still
		// counts, so the mark is raised before the game over latches
		sWriter.SetDisplayScore(cb::gScore);
		if (aGameOverKind >= 0)
			sWriter.GameOver(aGameOverKind);

		uint32_t aFlags = 0;
		if (!isFromInstructions)
			aFlags |= AUTOSPLIT_FLAG_RESUMED;   // a saved game, or a board chosen on the level selection: no run starts there
		if (Platform::ModManager::Get().GetScoreKey().empty())
			aFlags |= AUTOSPLIT_FLAG_VANILLA;
		if ((aKind == cb::LEVEL_BOARD) && (aLevel->mState == cb::LEVEL_PAUSED))
			aFlags |= AUTOSPLIT_FLAG_PAUSED;
		aNext.flags = aFlags;               // the writer adds the game over's and the win's flags from its latches
		aNext.tick = aUpdate;
		aNext.state = aState;
		if (aKind == cb::LEVEL_BOARD)
		{
			aNext.level = aLevel->mNumber - 1;
			aNext.displayLevel = aLevel->mNumber;
		}
		aNext.score = cb::gScore;
		aNext.mode = MODE_GAME;
		aNext.skill = cb::gDifficulty;      // -1 easy, 0 normal, 1 hard, as the game names them (the original has no way to change it)
		aNext.progress = -1;
		SetOwnWords(aNext.mode, aNext.skill, cb::gAutoFire);

		sPrevState = aState;
	}
	else
		SetOwnWords(-1, 0, cb::gAutoFire);

	sWriter.Publish(SDL_GetPerformanceCounter());
	LogWrite(aUpdate);
}

void LiveSplitObserver::AppDestroyed(CosmicBugsApp* theApp)
{
	if (sWriter.IsActive())
	{
		sWriter.Deactivate();
		sWriter.Publish(SDL_GetPerformanceCounter());
		LogWrite(theApp->mUpdateCount);
	}
	if (sLogFile != NULL)
	{
		fclose(sLogFile);
		sLogFile = NULL;
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
