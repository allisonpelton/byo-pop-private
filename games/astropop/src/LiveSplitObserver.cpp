// port(autosplit): the autosplit block and its observer. See LiveSplitObserver.h.
//
// The block is the shared AutosplitBlock (platform/AutosplitBlock.h, layout 2), a constant-initialised static object, so its
// magic exists only in .data's initialised bytes, once in the exe, and the AutosplitWriter keeps its counters, serials and the
// seq protocol. It is in every Windows and web build but written only once asked for (the web page's call of
// byopop_livesplit_block, an enabled mod with the feature "livesplit", or -livesplitlog); until then active and self stay 0.
// The observer runs after each app update (AstroPopApp::UpdateFrames), reads the Board's fields only, and sees each update's
// end state, not what happened inside it.
//
// What the block holds for AstroPop:
//   board       a game: a classic game (Board::mGameType 1) or a survival game (2); each Board has a serial from the order they are
//               made (Board::mBoardSerial). The demonstration under the title and the menus (type 0) is no game: serial 0, mode -1.
//   mode        Board::mGameType: 1 the classic game, 2 survival. skill: the pilot (Board::mMode): 0 Vector, 1 Sprocket, 2 Turbot, 3 Vixx.
//   GO          the first update of a fresh game: a classic game from level 1 or a survival game, not a saved game that goes on and
//               not a classic game that goes on from a checkpoint (which are marked AUTOSPLIT_FLAG_RESUMED), and not a game that was
//               already on when the block was first asked for. The game's first update is the nearest moment to "the click that
//               starts the game" the Board has: the name, the mode and the pilot are chosen before the Board exists.
//   level clear Board::LevelComplete, seen as Board::mLevelsCompleted going up by one: the update in which the level's goal is
//               reached and the "Level Up" text swooshes in (or, for the levels before a multiple of four, the bonus round starts
//               at once). progress is the level just cleared (Board::mLevelNumber, which changes only when the next level's gate
//               is picked), 0 before the first; -1 in survival, which has no levels.
//   game over   Board::GameOver, seen as Board::mGameOver: kind 0 a new row pushed a column of bricks to the ship (Board::AddRow), 1 the stack
//               filled the field (Board::CheckStackHeight), 2 the player left the game (the menu came up over a game still on: END
//               GAME), 3 any other game over (a test command's `lose`). The game has no win: the classic game goes on after level 32.
//   own         gameOwn[0..3]: how many times the pilot's SUPA weapon was started in this game (Board::mPowerupActive going to 1:
//               BeginPlay, by the weapon button, Return or the middle button), as a little-endian u32; gameOwn[4..7]: the level the
//               game began at (i32, 1 for a new game), the rest zero.
//   state       0 playing, 1 a level is complete (the level-up screen, the warp and the gate), 2 a bonus round, 3 game over.
//   displayLevel the level on screen (classic: Board::mLevelNumber), the stage in survival (Board::mStageNumber, a minute of play each);
//               level is Board::mLevelNumber. displayScore is the score on screen (Board::mScoreShown), score the score (Board::mScore).

#include "LiveSplitObserver.h"

#ifdef ASTROPOP_HAS_LIVESPLIT

#include "platform/AutosplitBlock.h"
#include "platform/ModManager.h"
#include "AstroPopApp.h"
#include "Board.h"

#include <SDL.h>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif
#include <cstdio>
#include <cstring>

using namespace Sexy;

// The block (the shared layout 2, platform/AutosplitBlock.h): constant-initialised, aligned, kept by the linker. Only the
// writer's Publish() writes it.
extern "C" __attribute__((used, aligned(64))) volatile AutosplitBlock gLiveSplitBlock = AUTOSPLIT_BLOCK_INIT(AUTOSPLIT_GAME_ID('A', 'P', 'D', 'X'));

namespace
{

AutosplitWriter		sWriter(&gLiveSplitBlock);   // the counters, serials, latched flags and the seq writes
bool				sPageAsked = false;         // web: the page's timer asked for the block (byopop_livesplit_block)
int					sAskCountdown = 0;          // updates until the mods are looked at again for the feature
bool				sResumed = false;           // the current game continues a saved game or starts from a checkpoint
int					sPrevPowerup = 0;           // the current game's Board::mPowerupActive at the previous update
int					sPrevLevelsCompleted = 0;   // its Board::mLevelsCompleted at the previous update
unsigned int		sSupaUses = 0;              // how many times the SUPA weapon was started in the current game
int					sStartLevel = 0;            // the level the current game began at
int					sProgress = -1;             // the level just cleared in the current game, -1 in survival
std::string			sLogFileName;               // -livesplitlog
FILE*				sLogFile = NULL;
AutosplitBlock		sLogged;                    // the block as the last log line had it
bool				sHaveLogged = false;

const char* const	gCauseNames[4] = { "a row pushed a column to the ship", "the stack filled the field", "left", "other" };
const char* const	gPilotNames[4] = { "Vector", "Sprocket", "Turbot", "Vixx" };

// The mod feature "livesplit" of an enabled mod, as the other ports ask (ModFeatureEnabled): true or an object
bool IsFeatureOn()
{
	for (const Platform::ModInfo* aMod : Platform::ModManager::Get().GetEnabledMods())
	{
		const Platform::JsonValue& aFeatures = aMod->mFeaturesJson;
		if (aFeatures.IsObject() && aFeatures.Has("livesplit"))
		{
			const Platform::JsonValue& aValue = aFeatures["livesplit"];
			return aValue.IsObject() || aValue.AsBool(false);
		}
	}
	return false;
}

void SetModeAndSkill(int theMode, int theSkill)
{
	AutosplitBlock& aNext = sWriter.Next();
	aNext.mode = theMode;
	aNext.skill = theSkill;
}

void SetOwn()
{
	AutosplitBlock& aNext = sWriter.Next();
	memset(aNext.gameOwn, 0, sizeof(aNext.gameOwn));
	uint32_t aUses = sSupaUses;
	int32_t aStartLevel = sStartLevel;
	memcpy(aNext.gameOwn + 0, &aUses, sizeof(aUses));
	memcpy(aNext.gameOwn + 4, &aStartLevel, sizeof(aStartLevel));
}

// The coarse state of a game, the block's `state`
int StateOf(const Board* theBoard)
{
	if (theBoard->mGameOver)
		return 3;
	if (theBoard->mBonusRoundOn || theBoard->mBonusWarpIn)
		return 2;
	if (theBoard->mLevelUpPhase || theBoard->mWarpCharging || theBoard->mWarpActive || theBoard->mWarpFlight || theBoard->mAtWarpGate ||
		theBoard->mWarpGatePicked)
		return 1;
	return 0;
}

// Whether the log's line has to be written: anything of the block but the clocks, the scores and the tick has changed
bool Changed(const AutosplitBlock& theNow, const AutosplitBlock& theThen)
{
	return theNow.active != theThen.active || theNow.flags != theThen.flags || theNow.boardSerial != theThen.boardSerial ||
		theNow.goCount != theThen.goCount || theNow.goSerial != theThen.goSerial || theNow.levelClears != theThen.levelClears ||
		theNow.levelClearSerial != theThen.levelClearSerial || theNow.gameOvers != theThen.gameOvers ||
		theNow.gameOverSerial != theThen.gameOverSerial || memcmp(theNow.deaths, theThen.deaths, sizeof(theNow.deaths)) != 0 ||
		theNow.state != theThen.state || theNow.level != theThen.level || theNow.displayLevel != theThen.displayLevel ||
		theNow.boardLevelClears != theThen.boardLevelClears || theNow.goTick != theThen.goTick || memcmp(theNow.gameOwn, theThen.gameOwn, sizeof(theNow.gameOwn)) != 0 ||
		theNow.wins != theThen.wins || theNow.mode != theThen.mode || theNow.skill != theThen.skill || theNow.inGame != theThen.inGame ||
		theNow.progress != theThen.progress;
}

// One write (the writer's seq protocol), then -livesplitlog's line when something other than the clocks and the scores changed
void Publish(int theUpdate)
{
	sWriter.Publish(SDL_GetPerformanceCounter());

	if (!sLogFileName.empty())
	{
		AutosplitBlock aCopy;
		memcpy(&aCopy, &sWriter.Next(), sizeof(aCopy));
		if (sHaveLogged && !Changed(aCopy, sLogged))
			return;
		if (sLogFile == NULL)
		{
			sLogFile = fopen(sLogFileName.c_str(), "wb");
			if (sLogFile == NULL)
			{
				SDL_Log("livesplit: cannot write %s", sLogFileName.c_str());
				sLogFileName.clear();
				return;
			}
		}
		char aLine[700];
		FormatAutosplitBlock(aCopy, true, aLine, sizeof(aLine));
		fprintf(sLogFile, "u=%d %s\n", theUpdate, aLine);
		fflush(sLogFile);
		sLogged = aCopy;
		sHaveLogged = true;
	}
}

void Activate()
{
	sWriter.Activate(SDL_GetPerformanceFrequency());
	SetModeAndSkill(-1, 0);
	SetOwn();
	AutosplitBlock& aNext = sWriter.Next();
	SDL_Log("livesplit: block at 0x%08x (layout %u, %u bytes)", (unsigned) aNext.self, (unsigned) AUTOSPLIT_LAYOUT_VERSION,
		(unsigned) AUTOSPLIT_SIZE_V2);
}

} // namespace

void LiveSplitObserver::SetLogFile(const std::string& theFileName)
{
	sLogFileName = theFileName;
}

void LiveSplitObserver::AfterUpdate(AstroPopApp* theApp)
{
	if (!sWriter.IsActive())
	{
		bool isAsked = sPageAsked || !sLogFileName.empty();
		if (!isAsked && --sAskCountdown < 0)
		{
			sAskCountdown = 99;     // the mods are looked at once in a hundred updates, not each one
			isAsked = IsFeatureOn();
		}
		if (!isAsked)
			return;
	}

	AutosplitBlock& aNext = sWriter.Next();
	// On activation the current game (or none) is new to the block: it is never given a GO
	bool isActivation = !sWriter.IsActive();
	if (isActivation)
		Activate();

	Board* aBoard = theApp->mBoard;
	bool isGame = (aBoard != NULL) && (aBoard->mGameType != 0);     // the demonstration (type 0) is no game
	uint32_t aSerial = isGame ? aBoard->mBoardSerial : 0;
	bool isNewBoard = isActivation || (aSerial != aNext.boardSerial);
	if (isNewBoard)
	{
		sWriter.NewBoard(aSerial);
		sResumed = false;
		sPrevPowerup = 0;
		sPrevLevelsCompleted = 0;
		sSupaUses = 0;
		sStartLevel = 0;
		sProgress = -1;
	}

	if (!isGame)
	{
		sWriter.ClearBoard();
		SetModeAndSkill(-1, 0);
		SetOwn();
	}
	else
	{
		bool isOver = aBoard->mGameOver;
		// The menu or the pilot screen over a game that is not over: the player left it (END GAME)
		bool isLeft = !isOver && ((theApp->mMainMenu != NULL) || (theApp->mCharSelector != NULL));
		int aGameOverKind = -1;     // the kind of game over this update raised, -1 for none
		if (isNewBoard)
		{
			// The baseline: a game seen over has had its game over
			if (isOver)
				sWriter.SeenOver();
			sResumed = aBoard->mRestoredGame || aBoard->mStartedFromCheckpoint || (aBoard->mLevelNumber != 1);
			sStartLevel = aBoard->mLevelNumber;
			sPrevPowerup = aBoard->mPowerupActive;
			sPrevLevelsCompleted = aBoard->mLevelsCompleted;
			sProgress = (aBoard->mGameType == 1) ? ((aBoard->mLevelNumber > 1) ? aBoard->mLevelNumber - 1 : 0) : -1;
			if (!isActivation)
			{
				if (sResumed)
					SDL_Log("livesplit: board %u continues a saved game or a checkpoint (level %d): GO not counted", (unsigned) aSerial, sStartLevel);
				else if (!isOver)
				{
					sWriter.Go(aBoard->mUpdateCnt);
					SDL_Log("livesplit: board %u GO at tick %d (%s, pilot %s)", (unsigned) aSerial, aBoard->mUpdateCnt,
						(aBoard->mGameType == 1) ? "classic" : "survival", gPilotNames[aBoard->mMode & 3]);
				}
			}
		}
		else
		{
			// A level is cleared when Board::LevelComplete adds one to mLevelsCompleted
			if (aBoard->mLevelsCompleted > sPrevLevelsCompleted)
			{
				sWriter.LevelClear();
				sProgress = aBoard->mLevelNumber;
				SDL_Log("livesplit: board %u level clear %d (level %d) at tick %d", (unsigned) aSerial, (int) aNext.boardLevelClears,
					aBoard->mLevelNumber, aBoard->mUpdateCnt);
			}
			sPrevLevelsCompleted = aBoard->mLevelsCompleted;

			// The SUPA weapon is started when Board::mPowerupActive goes to 1 (BeginPlay)
			if ((aBoard->mPowerupActive == 1) && (sPrevPowerup != 1))
			{
				sSupaUses++;
				SDL_Log("livesplit: board %u SUPA weapon use %u at tick %d", (unsigned) aSerial, sSupaUses, aBoard->mUpdateCnt);
			}
			sPrevPowerup = aBoard->mPowerupActive;

			if ((!sWriter.HadGameOver()) && (isOver || isLeft))
			{
				aGameOverKind = isLeft ? 2 : ((aBoard->mGameOverCause >= 0 && aBoard->mGameOverCause <= 1) ? aBoard->mGameOverCause : 3);
				SDL_Log("livesplit: board %u game over (%s) at tick %d", (unsigned) aSerial, gCauseNames[aGameOverKind], aBoard->mUpdateCnt);
			}
		}

		// The score the splitter reads is the highest the game's score reached, frozen at the game over: the game-over
		// update's own score still counts, so the high-water mark is raised before the game over latches
		sWriter.SetDisplayScore(aBoard->mScoreShown);
		sWriter.RaiseScoreMax(aBoard->mScore);
		if (aGameOverKind >= 0)
			sWriter.GameOver(aGameOverKind);

		uint32_t aFlags = AUTOSPLIT_FLAG_VANILLA;       // no gameplay mod: the fixes mod leaves the rules as they were
		if (sResumed)
			aFlags |= AUTOSPLIT_FLAG_RESUMED;
		if (aBoard->mPauseCount > 0)
			aFlags |= AUTOSPLIT_FLAG_PAUSED;
		aNext.flags = aFlags;               // the writer adds the game over's flag from its latch
		aNext.tick = aBoard->mUpdateCnt;
		aNext.state = StateOf(aBoard);
		aNext.level = aBoard->mLevelNumber;
		aNext.displayLevel = (aBoard->mGameType == 2) ? aBoard->mStageNumber : aBoard->mLevelNumber;
		aNext.score = aBoard->mScore;
		aNext.progress = sProgress;
		SetModeAndSkill(aBoard->mGameType, aBoard->mMode);
		SetOwn();
	}

	Publish(theApp->mUpdateCount);
}

void LiveSplitObserver::AppDestroyed(AstroPopApp* theApp)
{
	if (sWriter.IsActive())
	{
		sWriter.Deactivate();
		SetModeAndSkill(-1, 0);
		sSupaUses = 0;
		sStartLevel = 0;
		SetOwn();
		Publish(theApp->mUpdateCount);
	}
	if (sLogFile != NULL)
	{
		fclose(sLogFile);
		sLogFile = NULL;
	}
}

#ifdef __EMSCRIPTEN__
// The page's speedrun timer: the block's address in the game's memory (a HEAPU8 offset); asking turns its writing on from the
// next update, as the feature does (web/autosplit.js)
extern "C" EMSCRIPTEN_KEEPALIVE uint32_t byopop_livesplit_block()
{
	sPageAsked = true;
	return (uint32_t) (uintptr_t) &gLiveSplitBlock;
}
#endif

#endif
