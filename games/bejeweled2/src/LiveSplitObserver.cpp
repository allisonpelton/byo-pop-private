// port(mod): the autosplit block and its observer. See LiveSplitObserver.h.
//
// The block is the shared AutosplitBlock (platform/AutosplitBlock.h, layout 2), a constant-initialised static object, so its
// magic exists only in .data's initialised bytes, once in the exe, and the AutosplitWriter keeps its counters, serials and the
// seq protocol. It is in every Windows and web build but written only while the "livesplit" feature is on or, on the web, the
// page asked for it (byopop_livesplit_block); until then active and self stay 0, which tells the script to ask for the mod.
// The observer runs after each update (WinDMApp::UpdateFrames) and sees each update's end state, not what happened inside it.
//
// A board here is one game of one mode (WinDMApp::DoNewGame makes it, with a serial of the Board's), from the portal to the
// main menu's next game or a quit; a puzzle board spans all the planets of the galaxy. Events, for the current board:
//   GO          the first update on which a fresh board (not a saved game that LoadGame filled, not first seen when the block was
//               asked for) is on the screen: Board::Show takes it out of SCREEN_HIDDEN once the portal is through and the
//               how-to-play screen (when the profile still wants it) is closed, which is the moment the speedrun.com rules start
//               their timers ("pressing Continue on the How to Play Screen", "selecting No"). The click on the mode's button is
//               the portal's length earlier (see the log line, which says the board's update count)
//   level clear Board::mLevelClears went up: the bar of a level filled (STATE_LEVEL_COMPLETE_WAIT, in Classic, Action, Endless
//               and the secret modes but Cognito), a puzzle was solved for the first time (a galaxy planet's pod) or solved at
//               all (Cognito's)
//   win         Board::mWins went up: the fifth puzzle of the galaxy's last planet, or the last puzzle of the secret galaxy
//   game over   the board enters STATE_GAME_OVER_FALL (the gems fall away: NO MOVES in Classic, Endless, Twilight and Hidden, TIME UP
//               in Action, Hyper and Finity), or the board goes away in the middle of the game before either (the main menu, a new
//               game, a change of the display)
// A board's first update is a baseline for the clears and wins (a board already over when first seen has had its game over).

#include "LiveSplitObserver.h"

#ifdef BEJ2_HAS_LIVESPLIT

#include "platform/AutosplitBlock.h"
#include "platform/ModManager.h"
#include "WinDMApp.h"
#include "Board.h"
#include "Profile.h"
#include "GalaxyParser.h"

#include <SDL.h>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif
#include <cstdio>
#include <cstring>

using namespace Sexy;

// The block (the shared layout 2): constant-initialised, aligned, kept by the linker. Only the writer's Publish() writes it.
extern "C" __attribute__((used, aligned(64))) volatile AutosplitBlock gLiveSplitBlock =
	AUTOSPLIT_BLOCK_INIT(AUTOSPLIT_GAME_ID('B', '2', 'D', 'X'));

namespace
{

// WinDMApp::mGameMode, which is the block's mode too
enum { MODE_CLASSIC = 0, MODE_ACTION = 1, MODE_PUZZLE = 2, MODE_ENDLESS = 3, MODE_TWILIGHT = 4, MODE_HYPER = 5, MODE_COGNITO = 6,
	MODE_FINITY = 7, MODE_HIDDEN = 8, MODE_COUNT = 9 };
enum { DEATH_NO_MOVES = 0, DEATH_TIME_UP = 1, DEATH_LEFT = 2 };
const char* const	gDeathNames[4] = { "no moves", "time up", "left", "other" };
// gameOwn: bytes 0 to 8 the levels cleared in each mode (by WinDMApp::mGameMode) since the block was asked for (a byte each, which stops
// at 255), byte 16 whether the board's first move has been made (the 100% category's timer starts on it), bytes 24 to 31 the score key
// of the mods that change the scores (empty for none)
enum { OWN_CLEARS = 0, OWN_MOVED = 16, OWN_SCORE_KEY = 24 };

AutosplitWriter		sWriter(&gLiveSplitBlock);  // the counters, serials, latched flags and the seq writes
bool				sGoPossible = false;        // the current board may still have its GO
int					sMode = -1;                 // the current board's mode
int					sPrevClears = 0;            // the board's Board::mLevelClears at the previous update
int					sPrevWins = 0;              // and its mWins
int					sCreatedAt = 0;             // the board's update count at its first sighting
int					sModeClears[MODE_COUNT];    // the levels cleared in each mode (gameOwn)
bool				sPageAsked = false;         // web: the page's timer asked for the block (byopop_livesplit_block)
std::string			sLogFileName;               // -livesplitlog
FILE*				sLogFile = NULL;
AutosplitBlock		sLogged;                    // the fields of the last line the log wrote
bool				sHasLogged = false;

// An enabled mod has the feature "livesplit" in its manifest (true, or an object)
bool FeatureAsked()
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

// The mode and skill (Bejeweled 2 has none), layout 2's own fields
void SetModeAndSkill(int32_t theMode, int32_t theSkill)
{
	AutosplitBlock& aNext = sWriter.Next();
	aNext.mode = theMode;
	aNext.skill = theSkill;
}

// One write (the writer's seq protocol), then -livesplitlog's line: only when a field the rules read has changed since the
// last line (the updates, tick, score, score on screen and clock change with every write and are left out of the test)
void Publish(int theUpdate)
{
	sWriter.Publish(SDL_GetPerformanceCounter());
	if (sLogFileName.empty())
		return;

	AutosplitBlock aFields = sWriter.Next();
	aFields.seq = 0;
	aFields.updates = 0;
	aFields.tick = 0;
	aFields.displayScore = 0;
	aFields.score = 0;
	aFields.displayScoreMax = 0;
	aFields.qpc = 0;
	if (sHasLogged && memcmp(&aFields, &sLogged, sizeof(aFields)) == 0)
		return;
	sLogged = aFields;
	sHasLogged = true;

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
	FormatAutosplitBlock(sWriter.Next(), true, aLine, sizeof(aLine));
	fprintf(sLogFile, "u=%d %s\n", theUpdate, aLine);
	fflush(sLogFile);
}

void Activate()
{
	sWriter.Activate(SDL_GetPerformanceFrequency());
	AutosplitBlock& aNext = sWriter.Next();
	memset(sModeClears, 0, sizeof(sModeClears));
	memset(aNext.gameOwn, 0, sizeof(aNext.gameOwn));
	const std::string& aKey = Platform::ModManager::Get().GetScoreKey();
	for (size_t i = 0; (i < aKey.size()) && (i < 8); i++)
		aNext.gameOwn[OWN_SCORE_KEY + i] = (uint8_t) aKey[i];
	SetModeAndSkill(-1, 0);
	SDL_Log("livesplit: block at 0x%08x (layout %u, %u bytes)", (unsigned) aNext.self, (unsigned) AUTOSPLIT_LAYOUT_VERSION,
		(unsigned) AUTOSPLIT_SIZE_V2);
}

// The first galaxy's puzzles solved (the profile's count, which a fresh player's categories end on: 20 for the 25% category's four planets)
int GalaxyProgress(WinDMApp* theApp)
{
	return (theApp->mProfile != NULL) ? theApp->mProfile->mGalaxyOneSolved : -1;
}

// What the HUD says is the level: the level after the ones cleared; in the galaxy's puzzle mode the planet (mPlanetNum counts from 0)
int DisplayLevel(const Board* theBoard)
{
	if (theBoard->mIsPuzzle && !theBoard->mIsSecret)
		return theBoard->mPlanetNum + 1;
	return theBoard->mLevel + 1;
}

} // namespace

void LiveSplitObserver::SetLogFile(const std::string& theFileName)
{
	sLogFileName = theFileName;
}

void LiveSplitObserver::AfterUpdate(WinDMApp* theApp)
{
	if (!sWriter.IsActive())
	{
		// the mod's feature is looked for once a second: a mod is enabled before a game starts, and this costs nothing between
		if ((!sPageAsked) && ((theApp->mUpdateCount % 100 != 0) || !FeatureAsked()))
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
		// the board that went away in the middle of the game was left: its own write, then the new board's
		if ((!isActivation) && (aNext.boardSerial != 0) && (aNext.inGame == 1) && (!sWriter.HadGameOver()) && (!sWriter.HadWin()))
		{
			sWriter.GameOver(DEATH_LEFT);
			SDL_Log("livesplit: board %u game over (%s) at update %d", (unsigned) aNext.boardSerial, gDeathNames[DEATH_LEFT],
				(int) theApp->mUpdateCount);
			Publish(theApp->mUpdateCount);
		}
		sWriter.NewBoard(aSerial);
		sGoPossible = false;
		sPrevClears = 0;
		sPrevWins = 0;
	}

	if (aBoard == NULL)
	{
		sWriter.ClearBoard();
		SetModeAndSkill(-1, 0);
		aNext.gameOwn[OWN_MOVED] = 0;
	}
	else
	{
		int aState = aBoard->mState;
		bool isOver = (aState == Board::STATE_GAME_OVER_FALL) || (aState == Board::STATE_GAME_OVER);
		bool isShown = (aBoard->mScreenState != Board::SCREEN_HIDDEN);
		int aGameOverKind = -1;     // the kind of game over this update raised (gDeathNames), -1 for none
		if (isNewBoard)
		{
			// The baseline: a board seen over has had its game over, and its counters are the previous ones
			if (isOver)
				sWriter.SeenOver();
			sPrevClears = aBoard->mLevelClears;
			sPrevWins = aBoard->mWins;
			sMode = theApp->mGameMode;
			sCreatedAt = aBoard->mUpdateCnt;
			sGoPossible = (!isActivation) && (!aBoard->mResumed);
			if ((!isActivation) && (aBoard->mResumed))
				SDL_Log("livesplit: board %u continues a saved game: GO not counted", (unsigned) aSerial);
			else if (!isActivation)
				SDL_Log("livesplit: board %u made at update %d (mode %d)", (unsigned) aSerial, (int) theApp->mUpdateCount, sMode);
		}

		if (sGoPossible)
		{
			if ((aBoard->mLevelClears != 0) || isOver)
				sGoPossible = false;    // the start went by unseen: no GO for this board
			else if (isShown)
			{
				sGoPossible = false;
				sWriter.Go(aBoard->mUpdateCnt);
				SDL_Log("livesplit: board %u GO at tick %d, update %d (mode %d)", (unsigned) aSerial, aBoard->mUpdateCnt,
					(int) theApp->mUpdateCount, sMode);
			}
		}

		if (!isNewBoard)
		{
			for (; sPrevClears < aBoard->mLevelClears; sPrevClears++)
			{
				sWriter.LevelClear();
				if ((sMode >= 0) && (sMode < MODE_COUNT) && (sModeClears[sMode] < 255))
					sModeClears[sMode]++;
				SDL_Log("livesplit: board %u level clear %d (mode %d, level %d) at tick %d", (unsigned) aSerial,
					(int) aNext.boardLevelClears, sMode, DisplayLevel(aBoard), aBoard->mUpdateCnt);
			}
			for (; sPrevWins < aBoard->mWins; sPrevWins++)
			{
				sWriter.Win();
				SDL_Log("livesplit: board %u win at tick %d", (unsigned) aSerial, aBoard->mUpdateCnt);
			}
			if ((!sWriter.HadGameOver()) && isOver)
			{
				aGameOverKind = aBoard->mIsAction ? DEATH_TIME_UP : DEATH_NO_MOVES;
				SDL_Log("livesplit: board %u game over (%s) at tick %d", (unsigned) aSerial, gDeathNames[aGameOverKind],
					aBoard->mUpdateCnt);
			}
		}

		// The score the splitter reads is the highest the board's score reached, frozen at the game over or win; the update's
		// own score still counts, so the mark is raised before the game over latches
		sWriter.RaiseScoreMax(aBoard->mScore);
		if (aGameOverKind >= 0)
			sWriter.GameOver(aGameOverKind);

		uint32_t aFlags = 0;
		if (aBoard->mResumed)
			aFlags |= AUTOSPLIT_FLAG_RESUMED;
		if (Platform::ModManager::Get().GetScoreKey().empty())
			aFlags |= AUTOSPLIT_FLAG_VANILLA;
		if ((aBoard->mPauseCount > 0) || (theApp->GetDialog(DIALOG_HELP) != NULL))
			aFlags |= AUTOSPLIT_FLAG_PAUSED;
		aNext.flags = aFlags;               // the writer adds the game over's and the win's flags from its latches
		aNext.tick = aBoard->mUpdateCnt;
		aNext.state = aState;
		aNext.level = aBoard->mLevel;
		aNext.displayLevel = DisplayLevel(aBoard);
		aNext.displayScore = aBoard->mShownScore;
		aNext.score = aBoard->mScore;
		SetModeAndSkill(sMode, 0);
		aNext.progress = ((sMode == MODE_PUZZLE) ? GalaxyProgress(theApp) : -1);
		for (int i = 0; i < MODE_COUNT; i++)
			aNext.gameOwn[OWN_CLEARS + i] = (uint8_t) sModeClears[i];
		aNext.gameOwn[OWN_MOVED] = aBoard->mNoMoveYet ? 0 : 1;
	}

	Publish(theApp->mUpdateCount);
}

void LiveSplitObserver::AppDestroyed(WinDMApp* theApp)
{
	if (sWriter.IsActive())
	{
		sWriter.Deactivate();
		SetModeAndSkill(-1, 0);
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
// next update, as features.livesplit does (web/autosplit.js)
extern "C" EMSCRIPTEN_KEEPALIVE uint32_t byopop_livesplit_block()
{
	sPageAsked = true;
	return (uint32_t) (uintptr_t) &gLiveSplitBlock;
}
#endif

#endif
