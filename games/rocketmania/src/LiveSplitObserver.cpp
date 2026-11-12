// port: the autosplit block and its observer. See LiveSplitObserver.h.
//
// The block is the shared AutosplitBlock (platform/AutosplitBlock.h, layout 2; docs/WEB_PLAYER.md, "One design for every port"):
// a constant-initialised static object, so its magic exists only in .data's initialised bytes, once in the exe, and the
// AutosplitWriter keeps its counters, serials and the seq protocol. It is in every Windows and web build but written only once
// something asks for it, the page by calling byopop_livesplit_block or natively the "livesplit" feature of an enabled mod; until
// then active and self stay 0. The observer runs after each app update (RocketApp::UpdateFrames), reads the board's fields only
// and sees each update's end state, not what happened inside it.
//
// Rocket Mania has one board at a time: RocketApp::mBoard is made when a game starts (RocketApp::StartBoard, a new game or a
// continued one) and deleted when the player goes back to the menu (RocketApp::ShowGameSelector) or starts another game. Board::
// mBoardSerial counts the games, so a game replaced within one update still is a new board here. A level is the board's mLevel
// (0 for the first), and the text on screen is "Level" plus mLevel + 1.
//
// Events, for the current board:
//   GO          the first update of a fresh board (not a continued save, not first seen at activation) that is not paused, at
//               its first level and still playing (state 0). That update is the one on which the "Get Ready!" banner
//               (Board::mBannerTime, set when the level starts) begins to count down; the banner is drawn at the first level
//               only when the tutorial messages (RocketApp::mNoHints) are off, and is drawn at every later level
//   level clear the board enters state 4 (Board::EndLevelA: the level's rockets are launched, which is when "Level Up!" is
//               drawn above the board; state 5 and 6 are the level-up screens that follow, and the next level starts after
//               them). Arcade and classic modes only: the strategy mode has no end of a level
//   game over   the board enters state 2 (Board::Timers, 300 updates after Board::GameOver set state 1: "Game Over" or "Out of
//               Time" is drawn meanwhile and the game-over screen opens). Not at state 1, because in classic mode a launch that
//               is still burning when the night runs out can finish the level (state 1 to state 4, "JUST IN TIME!"), and then
//               the game goes on. The kind is Board::mGameOverKind: 0 the night ran out (classic), 1 the top line is full
//               (arcade), 2 no matches are left (strategy); 3 stays unused
//   win         none: the game has no last level
// A board's first update is a baseline for the level clear and the game over (a board first seen in state 2 has had its
// game over).

#include "LiveSplitObserver.h"

#ifdef ROCKETMANIA_HAS_LIVESPLIT

#include "RocketApp.h"
#include "Board.h"
#include "platform/AutosplitBlock.h"
#include "platform/ModManager.h"

#include <SDL.h>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif
#include <cstdio>
#include <cstring>

using namespace Sexy;

// The block: constant-initialised, aligned, kept by the linker. Only the writer's Publish() writes it. The game id "RMDX" is
// Rocket Mania Deluxe, as "BWDX" is BookWorm Deluxe.
extern "C" __attribute__((used, aligned(64))) volatile AutosplitBlock gLiveSplitBlock =
	AUTOSPLIT_BLOCK_INIT(AUTOSPLIT_GAME_ID('R', 'M', 'D', 'X'));

namespace
{

enum
{
	STATE_PLAYING = 0,        // Board::mState
	STATE_GAME_OVER = 1,
	STATE_FINISHED = 2,
	STATE_LEVEL_UP = 4        // "Level Up!" is on screen and the level's countdown runs
};

AutosplitWriter		sWriter(&gLiveSplitBlock);   // the counters, serials, latched flags and the seq writes
bool				sGoPossible = false;        // the current board may still have its GO
int					sPrevState = -1;            // the current board's state at the previous update
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

} // namespace

void LiveSplitObserver::SetLogFile(const std::string& theFileName)
{
	sLogFileName = theFileName;
}

void LiveSplitObserver::AfterUpdate(RocketApp* theApp)
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
	// On activation the current board (or none) is new to the block: it is never given a GO
	bool isActivation = !sWriter.IsActive();
	if (isActivation)
		Activate();

	Board* aBoard = theApp->mBoard;
	uint32_t aSerial = (aBoard != NULL) ? aBoard->mBoardSerial : 0;
	bool isNewBoard = isActivation || (aSerial != aNext.boardSerial);
	if (isNewBoard)
	{
		sWriter.NewBoard(aSerial);
		sGoPossible = false;
		sPrevState = -1;
	}

	if (aBoard != NULL)
	{
		int aState = aBoard->mState;
		int aGameOverKind = -1;     // the kind of game over this update raised (Board::mGameOverKind), -1 for none
		if (isNewBoard)
		{
			// The baseline: a board seen over has had its game over, and its state is the previous one
			if (aState == STATE_FINISHED)
				sWriter.SeenOver();
			sPrevState = aState;
			sGoPossible = (!isActivation) && (!aBoard->mResumed);
			if ((!isActivation) && aBoard->mResumed)
				SDL_Log("livesplit: board %u continues a saved game: GO not counted", (unsigned) aSerial);
		}

		if (sGoPossible)
		{
			if ((aBoard->mLevel != 0) || (aState != STATE_PLAYING))
				sGoPossible = false;    // the start went by unseen: no GO for this board
			else if (aBoard->mStarted && (aBoard->mPause < 1))
			{
				sGoPossible = false;
				sWriter.Go(aBoard->mUpdateCnt);
				SDL_Log("livesplit: board %u GO at tick %d (mode %d, difficulty %d)", (unsigned) aSerial, aBoard->mUpdateCnt,
					theApp->mMode, theApp->mDifficulty);
			}
		}

		if (!isNewBoard)
		{
			if ((aState == STATE_LEVEL_UP) && (sPrevState != STATE_LEVEL_UP))
			{
				sWriter.LevelClear();
				SDL_Log("livesplit: board %u level clear %d (level %d) at tick %d", (unsigned) aSerial, (int) aNext.boardLevelClears,
					aBoard->mLevel + 1, aBoard->mUpdateCnt);
			}
			if ((!sWriter.HadGameOver()) && (aState == STATE_FINISHED))
			{
				aGameOverKind = ((aBoard->mGameOverKind >= 0) && (aBoard->mGameOverKind < 3)) ? aBoard->mGameOverKind : 3;
				SDL_Log("livesplit: board %u game over (kind %d) at tick %d", (unsigned) aSerial, aGameOverKind, aBoard->mUpdateCnt);
			}
		}

		// The score on screen counts up to the score; the high-water mark follows it, and the game-over update's own score
		// still counts, so the mark is raised before the game over latches
		sWriter.SetDisplayScore(aBoard->mScoreShown);
		if (aGameOverKind >= 0)
			sWriter.GameOver(aGameOverKind);

		uint32_t aFlags = 0;
		if (aBoard->mResumed)
			aFlags |= AUTOSPLIT_FLAG_RESUMED;
		if (Platform::ModManager::Get().GetScoreKey().empty())
			aFlags |= AUTOSPLIT_FLAG_VANILLA;
		if (aBoard->mPause > 0)
			aFlags |= AUTOSPLIT_FLAG_PAUSED;
		aNext.flags = aFlags;               // the writer adds the game over's flag from its latch
		aNext.tick = aBoard->mUpdateCnt;
		aNext.state = aState;
		aNext.level = aBoard->mLevel;
		aNext.displayLevel = aBoard->mLevel + 1;
		aNext.score = aBoard->mScore;
		aNext.mode = theApp->mMode;
		aNext.skill = theApp->mDifficulty - 1;      // 0 easy, 1 medium (the speedrun board's Normal), 2 hard
		aNext.progress = -1;

		sPrevState = aState;
	}

	sWriter.Publish(SDL_GetPerformanceCounter());
	LogWrite(theApp->mUpdateCount);
}

void LiveSplitObserver::AppDestroyed(RocketApp* theApp)
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
