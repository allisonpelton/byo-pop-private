// port: the autosplit block and its observer. See LiveSplitObserver.h.
//
// The block is the shared AutosplitBlock (platform/AutosplitBlock.h, layout 2; docs/WEB_PLAYER.md, "One design for every port"):
// a constant-initialised static object, so its magic exists only in .data's initialised bytes, once in the exe, and the
// AutosplitWriter keeps its counters, serials and the seq protocol. It is in every Windows and web build but written only once
// something asks for it, the page by calling byopop_livesplit_block or natively the "livesplit" feature of an enabled mod; until
// then active and self stay 0. The observer runs after each app update (WinWSApp::UpdateFrames), reads the app, the board and the
// diver only, and sees each update's end state, not what happened inside it.
//
// What the block calls a board is a game of Typer Shark, from the difficulty (or the abyss) being chosen to the menu: one game
// is many of the game's own Boards (WinWSApp::mBoard), a new one for each level that the map's start button begins
// (WinWSApp::StartBoard), and the map screen between two of them. WinWSApp::mAutosplitGameSerial counts the games (CreateBoard
// makes the first Board of a game, RestartBoard the first of a "play again"), and Board::mBoardSerial counts the Boards.
//
// Numbering: mode 0 Adventure, 1 Abyss; skill the difficulty as the game numbers it (WinWSApp::mDifficulty: 0 Easy, 1 Normal,
// 2 Hard, 3 Expert, 4 X-Treme!), 0 in the abyss. state is the diver's state (Player.h: 0 swimming, 1 climbing down, 2, 3 and 10
// beaten, 4 on the floor, 5 the streak game or the clams, 6 waiting or the treasure, 7 and 12 level won, 8 game over, 9 afraid of a
// boss), 20 while the map screen is up between two levels. level is the diver's level number (the HUD shows it plus one, which
// displayLevel is). tick is the app's update count. displayScore is the points on the HUD; score is the same, but the depth in
// feet in the abyss. progress, in an adventure, is the levels beaten and left: the diver's level number, plus one while the map
// is up after its level (so it reaches N on the update that the map comes back after the Nth level); -1 in the abyss. gameOwn
// holds two int32: the game's time in milliseconds as the game-over screen shows it (the levels' times so far, WinWSApp::
// mGameTimeSoFar, and the board's timer; frozen at the game over, to the very number the dialog shows) and the depth an abyss
// game started at (0 from the surface and in an adventure).
//
// Events, for the current game:
//   GO          the first update of a new game that is not a continued save, and not a dive that starts below the surface, and not
//               first seen at activation. That is the update after the difficulty button (or the Abyss button, or the "CONTINUE?"
//               dialog's yes) made the first Board
//   level clear the diver enters the level-won state (12, Board::Update: back at the bottom after the treasure, the level bonus
//               added) or state 7 without it, or state 6 in a bonus level (the treasure shown, Board::ShowTreasure: the space bar
//               and the continue link leave from there). The map comes back when the player presses the space bar or the link
//               (WinWSApp::ReturnToMap): that is when progress rises. The abyss has no level end
//   game over   the diver enters state 8 (Board::PlayerEaten with no life left: the game-over dialog opens). Kind 0 in an
//               adventure, 1 in the abyss; 2 and 3 are unused. Leaving a game from the options dialog is no game over: the game
//               is simply gone from the app (no board)
//   win         the win dialog appears (MapScreen::Update, when the map's final scroll after the last site is done)
// A game's first update is a baseline for the level clear, the game over and the win (a game seen already over has had its game
// over), and a Board's first update is a baseline for its own level clear.

#include "LiveSplitObserver.h"

#ifdef TYPERSHARK_HAS_LIVESPLIT

#include "WinWSApp.h"
#include "Board.h"
#include "Player.h"
#include "MapScreen.h"
#include "LessonManager.h"
#include "GameOverDialog.h"
#include "WSTimer.h"
#include "platform/AutosplitBlock.h"
#include "platform/ModManager.h"

#include <SDL.h>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif
#include <cstdio>
#include <cstring>

using namespace Sexy;

// The block: constant-initialised, aligned, kept by the linker. Only the writer's Publish() writes it. The game id "TSDX" is
// Typer Shark Deluxe, as "BWDX" is BookWorm Deluxe.
extern "C" __attribute__((used, aligned(64))) volatile AutosplitBlock gLiveSplitBlock =
	AUTOSPLIT_BLOCK_INIT(AUTOSPLIT_GAME_ID('T', 'S', 'D', 'X'));

namespace
{

enum
{
	MODE_ADVENTURE = 0,
	MODE_ABYSS = 1,

	STATE_GAME_OVER = 8,      // Player::mState
	STATE_BONUS_DONE = 6,     // in a bonus level: the treasure is shown, and the space bar leaves
	STATE_WON_A = 7,
	STATE_WON_B = 12,
	STATE_MAP = 20            // the block's own: the map screen is up between two levels
};

AutosplitWriter		sWriter(&gLiveSplitBlock);   // the counters, serials, latched flags and the seq writes
bool				sGoPossible = false;        // the current game may still have its GO
unsigned int		sLevelBoardSerial = 0;      // the Board of the game seen at the previous update (Board::mBoardSerial)
bool				sPrevDone = false;          // that Board was in a level-won state at the previous update
bool				sPrevWinDialog = false;     // the win dialog was up at the previous update
bool				sResumed = false;           // the current game continues a saved game, or a dive that started below the surface
int					sStartDepth = 0;            // the depth an abyss game started at
int					sFrozenTime = -1;           // the game-over screen's time, once the game is over (else -1)
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

// -livesplitlog's line for this write, when it differs from the last one in what a splitter acts on (the game, the events, the
// state, the level, the mode, the skill, the progress and the flags; not the tick or the score)
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

void SetOwn(int theTime, int theStartDepth)
{
	AutosplitBlock& aNext = sWriter.Next();
	memset(aNext.gameOwn, 0, sizeof(aNext.gameOwn));
	memcpy(aNext.gameOwn + 0, &theTime, sizeof(theTime));
	memcpy(aNext.gameOwn + 4, &theStartDepth, sizeof(theStartDepth));
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

} // namespace

void LiveSplitObserver::SetLogFile(const std::string& theFileName)
{
	sLogFileName = theFileName;
}

void LiveSplitObserver::AfterUpdate(WinWSApp* theApp)
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

	Board* aBoard = theApp->mBoard;
	uint32_t aSerial = ((aBoard != NULL) && (aBoard->mPlayer != NULL)) ? theApp->mAutosplitGameSerial : 0;
	bool isNewGame = isActivation || (aSerial != aNext.boardSerial);
	if (isNewGame)
	{
		sWriter.NewBoard(aSerial);
		sGoPossible = false;
		sLevelBoardSerial = 0;
		sPrevDone = false;
		sPrevWinDialog = false;
		sFrozenTime = -1;
		sResumed = false;
		sStartDepth = 0;
		SetOwn(0, 0);
	}

	if (aSerial == 0)
	{
		sWriter.ClearBoard();
	}
	else
	{
		Player* aDiver = aBoard->mPlayer;
		int aState = aDiver->mState;
		bool isAbyss = (theApp->mLessonManager != NULL) ? theApp->mLessonManager->mAbyssMode : (theApp->mDifficulty == -1);
		// the game's one level-won moment for a level of each kind: the level-won states, and state 6 of a bonus level
		bool isDone = (!isAbyss) && ((aState == STATE_WON_A) || (aState == STATE_WON_B) ||
			((aState == STATE_BONUS_DONE) && (theApp->mLessonManager != NULL) && theApp->mLessonManager->IsBonusLevel()));
		bool isOnMap = (theApp->mMapScreen != NULL) && theApp->mMapScreen->mVisible && (!aBoard->mVisible);
		bool isWinDialog = theApp->GetDialog(DIALOG_WIN_GAME) != NULL;

		if (isNewGame)
		{
			// The baseline: a game seen over has had its game over. A dive that starts below the surface is the "CONTINUE?"
			// dialog's, a continued saved game is the profile's: neither starts a run
			sStartDepth = isAbyss ? aBoard->mScore : 0;
			sResumed = theApp->mAutosplitContinued || (isAbyss && (sStartDepth > 0));
			sGoPossible = (!isActivation) && (!sResumed);
			if (aState == STATE_GAME_OVER)
				sWriter.SeenOver();
			if ((!isActivation) && sResumed)
				SDL_Log("livesplit: game %u continues a saved game or a deeper dive: GO not counted", (unsigned) aSerial);
		}

		bool isNewLevelBoard = (aBoard->mBoardSerial != sLevelBoardSerial);
		if (isNewLevelBoard)
		{
			sLevelBoardSerial = aBoard->mBoardSerial;
			sPrevDone = isDone;           // the baseline: a Board seen already done raises no level clear
		}

		if (sGoPossible)
		{
			sGoPossible = false;
			sWriter.Go(theApp->mUpdateCount);
			SDL_Log("livesplit: game %u GO at tick %d (mode %d, skill %d)", (unsigned) aSerial, theApp->mUpdateCount,
				isAbyss ? MODE_ABYSS : MODE_ADVENTURE, isAbyss ? 0 : theApp->mDifficulty);
		}

		// The points on the HUD are the score the splitter follows; the update's own points count before a game over or a win latches
		sWriter.SetDisplayScore(aDiver->mScore);

		int aGameOverKind = -1;     // the kind of game over this update raised, -1 for none
		if (!isNewGame)
		{
			if (isDone && (!sPrevDone) && (!isNewLevelBoard))
			{
				sWriter.LevelClear();
				SDL_Log("livesplit: game %u level clear %d (level %d) at tick %d", (unsigned) aSerial, (int) aNext.boardLevelClears,
					aDiver->mLevelNumber + 1, theApp->mUpdateCount);
			}
			if ((!sWriter.HadGameOver()) && (aState == STATE_GAME_OVER))
			{
				aGameOverKind = isAbyss ? 1 : 0;
				SDL_Log("livesplit: game %u game over (kind %d) at tick %d", (unsigned) aSerial, aGameOverKind, theApp->mUpdateCount);
			}
			if (isWinDialog && (!sPrevWinDialog) && (!sWriter.HadWin()))
			{
				sWriter.Win();
				SDL_Log("livesplit: game %u win at tick %d", (unsigned) aSerial, theApp->mUpdateCount);
			}
		}
		sPrevDone = isDone;
		sPrevWinDialog = isWinDialog;

		// The time as the game-over screen shows it: the levels' times so far and this board's timer, which the dialog reads
		// (Board::PlayerEaten) once, so it is taken from the dialog when there is one
		int aTime = 0;
		if (aBoard->mGameTimer != NULL)
			aTime = theApp->mGameTimeSoFar + ((aBoard->mGameTimer->mUpdateCnt - aBoard->mGameTimer->mStart) - aBoard->mGameTimer->mPausedTotal);
		if (((aGameOverKind >= 0) || ((sFrozenTime < 0) && (aState == STATE_GAME_OVER))) && (aBoard->mGameOverDialog != NULL))
			sFrozenTime = aBoard->mGameOverDialog->mGameTime;
		if (sFrozenTime >= 0)
			aTime = sFrozenTime;
		if (aGameOverKind >= 0)
			sWriter.GameOver(aGameOverKind);

		uint32_t aFlags = 0;
		if (sResumed)
			aFlags |= AUTOSPLIT_FLAG_RESUMED;
		if (Platform::ModManager::Get().GetScoreKey().empty())
			aFlags |= AUTOSPLIT_FLAG_VANILLA;
		if (aBoard->mPaused)
			aFlags |= AUTOSPLIT_FLAG_PAUSED;
		aNext.flags = aFlags;               // the writer adds the game over's and the win's flags from its latches
		aNext.tick = theApp->mUpdateCount;
		aNext.state = isOnMap ? STATE_MAP : aState;
		aNext.level = aDiver->mLevelNumber;
		aNext.displayLevel = aDiver->mLevelNumber + 1;
		aNext.score = isAbyss ? aBoard->mScore : aDiver->mScore;
		aNext.mode = isAbyss ? MODE_ABYSS : MODE_ADVENTURE;
		aNext.skill = isAbyss ? 0 : theApp->mDifficulty;
		aNext.progress = isAbyss ? -1 : (aDiver->mLevelNumber + (isOnMap ? 1 : 0));
		SetOwn(aTime, sStartDepth);
	}

	sWriter.Publish(SDL_GetPerformanceCounter());
	LogWrite(theApp->mUpdateCount);
}

void LiveSplitObserver::AppDestroyed(WinWSApp* theApp)
{
	if (sWriter.IsActive())
	{
		sWriter.Deactivate();
		SetOwn(0, 0);
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
