// port(autosplit): the autosplit block and its observer. See LiveSplitObserver.h.
//
// The block is the shared AutosplitBlock (platform/AutosplitBlock.h, layout 2), a constant-initialised static object, so its
// magic exists only in .data's initialised bytes, once in the exe, and the AutosplitWriter keeps its counters, serials and the
// seq protocol. It is in every Windows and web build but written only once asked for (the web page's call of
// byopop_livesplit_block, an enabled mod with the feature "livesplit", or -livesplitlog); until then active and self stay 0.
//
// What the block holds for Hammer Heads (games/hammerheads/docs/GAME_NOTES.md, "Autosplit"):
//   board       a game on the game screen: from the screen being made for a new game, a saved game or a restart point until the
//               screen is gone (the menus, the high scores, the trophy room from the menu are no board: serial 0, mode -1). The
//               shop, a bonus round, the trophy room of a game and the in-game menu keep it; the game over box's "try again"
//               makes a new one. A saved game continued or a restart point starts a board with AUTOSPLIT_FLAG_RESUMED and no Go.
//   mode        0 Classic, 1 Tough Cookie, 2 Marathon (GAME_MODE_*). skill 0.
//   Go          the first press of Go! of a board that is not resumed: on the level introduction (LevelIntroWidget::ButtonDepress, a
//               level with one, as Classic's first) or on a "Ready?" box (ReadyGoDialog::ButtonDepress).
//   level clear Application::LevelFinished, every mode; displayLevel is the level just finished (1-based). The win is the last
//               row of the rules sheet's end in Classic and Tough Cookie, in the same update as its level clear.
//   game over   Gameplay::GameOver: kind 0 in Classic and Tough Cookie, kind 1 in Marathon.
//   inGame      1 from Go (a resumed board's: from its start) until the game over or the win.
//   progress    Classic and Tough Cookie: 0, then 1 from the press of Go! on the "Ready?" box that follows the trophy room in
//               which the trophy for destroying a king (Regicist) was awarded: the end of the speedrun categories. -1 in Marathon.
//   state       0 the game is paused (a box, the shop, a bonus round's wipe, the trophy room, the in-game menu), 1 a level in play,
//               3 the game over box. PAUSED is set while the game is paused. displayScore and score are the game's score.

#include "LiveSplitObserver.h"

#ifdef HAMMERHEADS_HAS_LIVESPLIT

#include "platform/AutosplitBlock.h"
#include "platform/ModManager.h"
#include "Application.h"
#include "GameScreen.h"
#include "Stats.h"
#include "UserManager.h"

#include <SDL.h>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif
#include <cstdio>
#include <cstring>

using namespace Sexy;

// The block (the shared layout 2, platform/AutosplitBlock.h): constant-initialised, aligned, kept by the linker. Only the
// writer's Publish() writes it. The game id "HHDX" is Hammer Heads Deluxe.
extern "C" __attribute__((used, aligned(64))) volatile AutosplitBlock gLiveSplitBlock = AUTOSPLIT_BLOCK_INIT(AUTOSPLIT_GAME_ID('H', 'H', 'D', 'X'));

namespace
{

enum
{
	STATE_PAUSED = 0,
	STATE_PLAYING = 1,
	STATE_GAME_OVER = 3,

	GAME_OVER_CLASSIC = 0,
	GAME_OVER_MARATHON = 1
};

AutosplitWriter		sWriter(&gLiveSplitBlock);	// the counters, serials, latched flags and the seq writes
bool				sPageAsked = false;			// web: the page's timer asked for the block (byopop_livesplit_block)
int					sAskCountdown = 0;			// updates until the mods are looked at again for the feature
std::string			sLogFileName;				// -livesplitlog
FILE*				sLogFile = NULL;
AutosplitBlock		sLogged;					// the block as the last log line had it
bool				sHaveLogged = false;

// What the game's hooks noted since the last update
uint32_t			sSerial = 0;				// the serial of the last board made (counted whether or not the block is asked for)
bool				sBoardAlive = false;		// the game screen was made and has not gone
bool				sBoardResumed = false;
bool				sGoDone = false;			// the board's Go was written, or it needs none
bool				sGoPending = false;
bool				sKingTrophyPending = false;	// the trophy room that was shown awarded Regicist
int					sProgress = -1;
bool				sClearPending = false;
int					sClearLevel = 0;
bool				sWinPending = false;
bool				sOverPending = false;
bool				sGameFinished = false;		// the board had its game over or its win

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

bool Changed(const AutosplitBlock& theNow, const AutosplitBlock& theThen)
{
	return theNow.active != theThen.active || theNow.flags != theThen.flags || theNow.boardSerial != theThen.boardSerial ||
		theNow.goCount != theThen.goCount || theNow.goSerial != theThen.goSerial || theNow.levelClears != theThen.levelClears ||
		theNow.levelClearSerial != theThen.levelClearSerial || theNow.gameOvers != theThen.gameOvers ||
		theNow.gameOverSerial != theThen.gameOverSerial || memcmp(theNow.deaths, theThen.deaths, sizeof(theNow.deaths)) != 0 ||
		theNow.state != theThen.state || theNow.level != theThen.level || theNow.displayLevel != theThen.displayLevel ||
		theNow.boardLevelClears != theThen.boardLevelClears || theNow.goTick != theThen.goTick || theNow.wins != theThen.wins ||
		theNow.winSerial != theThen.winSerial || theNow.mode != theThen.mode || theNow.skill != theThen.skill ||
		theNow.inGame != theThen.inGame || theNow.progress != theThen.progress;
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
	AutosplitBlock& aNext = sWriter.Next();
	SDL_Log("livesplit: block at 0x%08x (layout %u, %u bytes)", (unsigned) aNext.self, (unsigned) AUTOSPLIT_LAYOUT_VERSION,
		(unsigned) AUTOSPLIT_SIZE_V2);
}

} // namespace

void LiveSplitObserver::SetLogFile(const std::string& theFileName)
{
	sLogFileName = theFileName;
}

void LiveSplitObserver::BoardCreated(bool theResumed)
{
	sSerial++;
	sBoardAlive = true;
	sBoardResumed = theResumed;
	sGoDone = theResumed;
	sGoPending = false;
	sKingTrophyPending = false;
	sProgress = -1;
	sClearPending = false;
	sWinPending = false;
	sOverPending = false;
	sGameFinished = false;
}

void LiveSplitObserver::GoPressed()
{
	if (!sBoardAlive)
		return;
	if (!sGoDone)
	{
		sGoDone = true;
		sGoPending = true;
	}
	if (sKingTrophyPending)
	{
		sKingTrophyPending = false;
		sProgress = 1;
	}
}

void LiveSplitObserver::LevelEnded(int theLevel, bool theIsWin)
{
	if (!sBoardAlive)
		return;
	sClearPending = true;
	sClearLevel = theLevel;
	if (theIsWin)
		sWinPending = true;
}

void LiveSplitObserver::GameEnded()
{
	if (sBoardAlive)
		sOverPending = true;
}

void LiveSplitObserver::TrophiesAwarded(const std::vector<int>& theTrophies)
{
	for (int aTrophy : theTrophies)
	{
		if (aTrophy == TROPHY_REGICIST)
			sKingTrophyPending = true;
	}
}

void LiveSplitObserver::AfterUpdate(Application* theApp)
{
	if (!sWriter.IsActive())
	{
		bool isAsked = sPageAsked || !sLogFileName.empty();
		if (!isAsked && --sAskCountdown < 0)
		{
			sAskCountdown = 99;		// the mods are looked at once in a hundred updates, not each one
			isAsked = IsFeatureOn();
		}
		if (!isAsked)
			return;
	}

	AutosplitBlock& aNext = sWriter.Next();
	// On activation the current game (or none) is new to the block: it is never given a Go
	bool isActivation = !sWriter.IsActive();
	if (isActivation)
		Activate();

	if (theApp->mGameScreen == nullptr)
		sBoardAlive = false;
	uint32_t aSerial = sBoardAlive ? sSerial : 0;
	if (isActivation || aSerial != aNext.boardSerial)
		sWriter.NewBoard(aSerial);

	if (aSerial == 0)
	{
		sWriter.ClearBoard();
		sGoPending = sClearPending = sWinPending = sOverPending = false;
	}
	else
	{
		int aTick = theApp->mUpdateCount;
		int aMode = theApp->mGameMode;
		bool isClassic = aMode == GAME_MODE_CLASSIC || aMode == GAME_MODE_TOUGH_COOKIE;
		if (isActivation)
		{
			sGoDone = true;				// a game already on when the block is first asked for has no Go
			sGoPending = false;
		}
		if (sGoPending)
		{
			sWriter.Go(aTick);
			sGoPending = false;
			SDL_Log("livesplit: board %u Go at update %d", (unsigned) aSerial, aTick);
		}
		int aDisplayLevel = theApp->mGameScreen->mLevel;
		if (sClearPending)
		{
			aDisplayLevel = sClearLevel;
			sWriter.LevelClear();
			sClearPending = false;
			SDL_Log("livesplit: board %u level clear %d at update %d%s", (unsigned) aSerial, sClearLevel, aTick, sWinPending ? ", the win" : "");
		}
		if (sWinPending)
		{
			sWriter.Win();
			sWinPending = false;
			sGameFinished = true;
		}
		if (sOverPending)
		{
			sWriter.GameOver(isClassic ? GAME_OVER_CLASSIC : GAME_OVER_MARATHON);
			sOverPending = false;
			sGameFinished = true;
			SDL_Log("livesplit: board %u game over at update %d", (unsigned) aSerial, aTick);
		}

		double aScore = GetStats()->Get(STAT_SCORE);
		sWriter.SetDisplayScore((int32_t) aScore);

		uint32_t aFlags = 0;
		if (sBoardResumed)
			aFlags |= AUTOSPLIT_FLAG_RESUMED;
		bool isPaused = theApp->mPauseCount > 0;
		if (isPaused)
			aFlags |= AUTOSPLIT_FLAG_PAUSED;
		if (Platform::ModManager::Get().GetScoreKey().empty())
			aFlags |= AUTOSPLIT_FLAG_VANILLA;
		aNext.flags = aFlags;				// the writer adds the game over's and the win's flags from its latches
		aNext.tick = aTick;
		aNext.state = sGameFinished ? STATE_GAME_OVER : (isPaused ? STATE_PAUSED : STATE_PLAYING);
		aNext.level = theApp->mGameScreen->mLevel - 1;
		aNext.displayLevel = aDisplayLevel;
		aNext.score = (int32_t) aScore;
		aNext.mode = aMode;
		aNext.skill = 0;
		aNext.progress = isClassic ? (sProgress < 0 ? 0 : sProgress) : -1;
		aNext.inGame = (sGoDone && !sGameFinished) ? 1 : 0;
	}

	Publish(theApp->mUpdateCount);
}

void LiveSplitObserver::AppDestroyed(Application* theApp)
{
	if (sWriter.IsActive())
	{
		sWriter.Deactivate();
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
