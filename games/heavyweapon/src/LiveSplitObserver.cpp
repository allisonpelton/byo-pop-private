// port(autosplit): the autosplit block and its observer. See LiveSplitObserver.h.
//
// The block is the shared AutosplitBlock (platform/AutosplitBlock.h, layout 2), a constant-initialised static object, so its
// magic exists only in .data's initialised bytes, once in the exe, and the AutosplitWriter keeps its counters, serials and the
// seq protocol. It is in every Windows and web build but written only once asked for (the web page's call of
// byopop_livesplit_block, an enabled mod with the feature "livesplit", or -livesplitlog); until then active and self stay 0.
// The observer runs after each app update (HeavyWeaponApp::UpdateFrames) and sees each update's end state, not what happened
// inside it. It reads the app's game counter (mGameSerial, mGameIsSurvival, mGameResumed, set by BeginGame at the clicks that
// start a game and cleared by EndGame), mLevel, mScore, mLives, the game-over box's presence, and the board's results
// screen, suspension and update count.
//
// What the block holds for Heavy Weapon:
//   board       a game: a campaign or a survival game, from the click that starts it until the main menu or the high scores
//               come back (a quit, the game-over box's SCORES, the end after the final cutscene). Its serial is the app's
//               mGameSerial. The game-over box's CONTINUE in a campaign goes on with the same board (the level again, or the
//               armory); in survival it starts a new one, as survival starts again. The menus, the high scores, the help and
//               the credits outside a game are no board: serial 0, mode -1.
//   mode        0 the campaign, 1 survival. skill 0: the game has none.
//   GO          the update of the click that begins a game: the main menu's campaign for a new player (NewGame(false), which
//               shows the first mission's map), the continue dialog's NEW GAME, survival (or the help's PLAY before a first
//               survival game) and the game-over box's CONTINUE in survival. The click is handled inside that update, so GO is
//               not an update late. The continue dialog's CONTINUE begins a board with AUTOSPLIT_FLAG_RESUMED and no GO, as
//               does the test command `level` outside a campaign; a game already on when the block is first asked for has
//               neither.
//   level clear a campaign level's results screen coming up (Board::Update, at mLevelLength + RESULTS_TIME, after the boss).
//               The final level's (HeavyWeaponApp::FINAL_LEVEL) is also the win, in the same update.
//   game over   the game-over box coming up (Board::Update: the last tank's death with none left): kind 0 in the campaign,
//               kind 1 in survival. A campaign continued from the box can have another on the same board.
//   inGame      1 from GO until the game over or the win, and again from the next level started on the same board (the box's
//               CONTINUE); a resumed board's from its first level started.
//   progress    the campaign's levels beaten (0 to 19: the level index the game began at, then each cleared level's index
//               + 1); -1 in survival.
//   state       0 between levels (the map, the armory, the help, a cutscene), 1 a level in play, 2 the level's results, 3 the
//               game-over box. level is mLevel (0-based), displayLevel mLevel + 1 in the campaign and 0 in survival. tick is the
//               app's update count. displayScore and score are the app's score (mScore, the one the status bar shows).
//   flags       RESUMED as above; PAUSED while the board is suspended (the pause screen, the mouse off the field, a dialog);
//               VANILLA with no gameplay mod (no score key).
//   own         gameOwn[0..3]: the tanks left (mLives, the one in play included) as a little-endian i32; the rest zero.

#include "LiveSplitObserver.h"

#ifdef HEAVYWEAPON_HAS_LIVESPLIT

#include "platform/AutosplitBlock.h"
#include "platform/ModManager.h"
#include "HeavyWeaponApp.h"
#include "Board.h"

#include <SDL.h>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif
#include <cstdio>
#include <cstring>

using namespace Sexy;

// The block (the shared layout 2, platform/AutosplitBlock.h): constant-initialised, aligned, kept by the linker. Only the
// writer's Publish() writes it. The game id "HWDX" is Heavy Weapon Deluxe.
extern "C" __attribute__((used, aligned(64))) volatile AutosplitBlock gLiveSplitBlock = AUTOSPLIT_BLOCK_INIT(AUTOSPLIT_GAME_ID('H', 'W', 'D', 'X'));

namespace
{

enum
{
	MODE_CAMPAIGN = 0,
	MODE_SURVIVAL = 1,

	STATE_BETWEEN_LEVELS = 0,
	STATE_PLAYING = 1,
	STATE_RESULTS = 2,
	STATE_GAME_OVER = 3,

	GAME_OVER_CAMPAIGN = 0,
	GAME_OVER_SURVIVAL = 1
};

AutosplitWriter		sWriter(&gLiveSplitBlock);	// the counters, serials, latched flags and the seq writes
bool				sPageAsked = false;			// web: the page's timer asked for the block (byopop_livesplit_block)
int					sAskCountdown = 0;			// updates until the mods are looked at again for the feature
bool				sResumed = false;			// the current board continues the player's saved campaign
bool				sStarted = false;			// its GO was written, or (resumed) a level of it has started
bool				sOverPending = false;		// it had a game over and no level has started since
int					sProgress = -1;				// the campaign's levels beaten, -1 in survival
Board*				sPrevBoard = NULL;			// the app's board at the previous update
int					sPrevBoardUpdates = 0;		// its update count then
bool				sPrevResults = false;		// its results screen was up
bool				sPrevOver = false;			// the game-over box was up
std::string			sLogFileName;				// -livesplitlog
FILE*				sLogFile = NULL;
AutosplitBlock		sLogged;					// the block as the last log line had it
bool				sHaveLogged = false;

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

void SetOwn(int32_t theLives)
{
	AutosplitBlock& aNext = sWriter.Next();
	memset(aNext.gameOwn, 0, sizeof(aNext.gameOwn));
	memcpy(aNext.gameOwn + 0, &theLives, sizeof(theLives));
}

// Whether the log's line has to be written: anything of the block but the clocks, the scores and the tick has changed
bool Changed(const AutosplitBlock& theNow, const AutosplitBlock& theThen)
{
	return theNow.active != theThen.active || theNow.flags != theThen.flags || theNow.boardSerial != theThen.boardSerial ||
		theNow.goCount != theThen.goCount || theNow.goSerial != theThen.goSerial || theNow.levelClears != theThen.levelClears ||
		theNow.levelClearSerial != theThen.levelClearSerial || theNow.gameOvers != theThen.gameOvers ||
		theNow.gameOverSerial != theThen.gameOverSerial || memcmp(theNow.deaths, theThen.deaths, sizeof(theNow.deaths)) != 0 ||
		theNow.state != theThen.state || theNow.level != theThen.level || theNow.displayLevel != theThen.displayLevel ||
		theNow.boardLevelClears != theThen.boardLevelClears || theNow.goTick != theThen.goTick ||
		memcmp(theNow.gameOwn, theThen.gameOwn, sizeof(theNow.gameOwn)) != 0 || theNow.wins != theThen.wins ||
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

void LiveSplitObserver::AfterUpdate(HeavyWeaponApp* theApp)
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
	// On activation the current game (or none) is new to the block: it is never given a GO
	bool isActivation = !sWriter.IsActive();
	if (isActivation)
		Activate();

	uint32_t aSerial = theApp->mGameSerial;
	Board* aBoard = theApp->mBoard;
	bool isSurvival = theApp->mGameIsSurvival;
	bool isOver = theApp->GetDialog(Board::DIALOG_GAME_OVER) != NULL;
	bool hasResults = aBoard != NULL && !isSurvival && aBoard->mResultsScreen != NULL;
	// A level starts when a board appears that was not there at the previous update
	bool isLevelStart = aBoard != NULL && (aBoard != sPrevBoard || aBoard->mUpdateCnt < sPrevBoardUpdates);
	bool isNewBoard = isActivation || (aSerial != aNext.boardSerial);
	if (isNewBoard)
		sWriter.NewBoard(aSerial);

	if (aSerial == 0)
	{
		sWriter.ClearBoard();
		sResumed = false;
		sStarted = false;
		sOverPending = false;
		sProgress = -1;
		SetOwn(0);
	}
	else
	{
		int aTick = theApp->mUpdateCount;
		bool isLevelClear = false;
		bool isWin = false;
		bool isGameOver = false;
		if (isNewBoard)
		{
			// The baseline: what is up already raises no event
			sResumed = theApp->mGameResumed;
			sStarted = !sResumed || aBoard != NULL;		// a resumed board (or one the block first sees) counts from a level
			if (isActivation)
				sStarted = aBoard != NULL;
			sOverPending = isOver;
			if (isOver)
				sWriter.SeenOver();
			sProgress = isSurvival ? -1 : theApp->mLevel + (hasResults ? 1 : 0);
			if (!isActivation)
			{
				if (sResumed)
					SDL_Log("livesplit: board %u continues the saved campaign (level %d): GO not counted", (unsigned) aSerial,
						theApp->mLevel + 1);
				else
				{
					sWriter.Go(aTick);
					SDL_Log("livesplit: board %u GO at update %d (%s)", (unsigned) aSerial, aTick, isSurvival ? "survival" : "campaign");
				}
			}
		}
		else
		{
			if (isLevelStart)
			{
				sStarted = true;
				sOverPending = false;
			}
			if (hasResults && !sPrevResults)
			{
				isLevelClear = true;
				isWin = theApp->mLevel == HeavyWeaponApp::FINAL_LEVEL;
				sProgress = theApp->mLevel + 1;
				SDL_Log("livesplit: board %u level clear %d (level %d) at update %d%s", (unsigned) aSerial,
					(int) aNext.boardLevelClears + 1, theApp->mLevel + 1, aTick, isWin ? ", the win" : "");
			}
			if (isOver && !sPrevOver)
			{
				isGameOver = true;
				sOverPending = true;
				SDL_Log("livesplit: board %u game over (%s) at update %d", (unsigned) aSerial, isSurvival ? "survival" : "campaign", aTick);
			}
		}

		// The score's high-water mark takes this update's score before a game over or a win freezes it
		sWriter.SetDisplayScore(theApp->mScore);
		if (isLevelClear)
			sWriter.LevelClear();
		if (isWin)
			sWriter.Win();
		if (isGameOver)
			sWriter.GameOver(isSurvival ? GAME_OVER_SURVIVAL : GAME_OVER_CAMPAIGN);

		uint32_t aFlags = 0;
		if (sResumed)
			aFlags |= AUTOSPLIT_FLAG_RESUMED;
		if (aBoard != NULL && aBoard->mSuspended)
			aFlags |= AUTOSPLIT_FLAG_PAUSED;
		if (Platform::ModManager::Get().GetScoreKey().empty())
			aFlags |= AUTOSPLIT_FLAG_VANILLA;
		aNext.flags = aFlags;				// the writer adds the game over's and the win's flags from its latches
		aNext.tick = aTick;
		aNext.state = isOver ? STATE_GAME_OVER : (hasResults ? STATE_RESULTS : (aBoard != NULL ? STATE_PLAYING : STATE_BETWEEN_LEVELS));
		aNext.level = theApp->mLevel;
		aNext.displayLevel = isSurvival ? 0 : theApp->mLevel + 1;
		aNext.score = theApp->mScore;
		aNext.mode = isSurvival ? MODE_SURVIVAL : MODE_CAMPAIGN;
		aNext.skill = 0;
		aNext.progress = sProgress;
		aNext.inGame = (sStarted && !sOverPending && !sWriter.HadWin()) ? 1 : 0;
		SetOwn(theApp->mLives);
	}

	sPrevBoard = aBoard;
	sPrevBoardUpdates = aBoard != NULL ? aBoard->mUpdateCnt : 0;
	sPrevResults = hasResults;
	sPrevOver = isOver;
	Publish(theApp->mUpdateCount);
}

void LiveSplitObserver::AppDestroyed(HeavyWeaponApp* theApp)
{
	if (sWriter.IsActive())
	{
		sWriter.Deactivate();
		SetOwn(0);
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
