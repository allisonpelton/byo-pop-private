// port(mod): Alchemy's autosplit observer. See LiveSplitObserver.h and platform/AutosplitBlock.h (layout 2, game id "ALDX").
//
// The block is a constant-initialised static object, so its magic exists only in .data's initialised bytes, once in the exe,
// and the AutosplitWriter keeps its counters, serials and the seq protocol. It is in every Windows and web build but written
// only while an enabled mod has the feature "livesplit" or, on the web, the page asked for it (byopop_livesplit_block); until
// then active and self stay 0. The observer runs after each app update (WinAlchApp::UpdateFrames), reads the board's fields
// and the side panel's score only, and sees each update's end state, not what happened inside it.
//
// A board is one game of Alchemy (Board::mBoardSerial; the app makes a new one for each game, the tutorial and a continued
// saved game too, and keeps it through the boards of the game, which it clears one after the other). The numbering the
// speedrun.com rules table reads:
//   mode   0 Strategic, 1 Time Trial, 2 Tutorial (-1 off a board)
//   skill  1 Easy, 2 Average, 3 Hard, as the game's own skill level; 0 in the tutorial and off a board
//   level  Board::mBoardsCleared (0-based), displayLevel the number the player sees (mBoardsCleared + 1)
//   state  Board::mState (Board::State)
//   deaths 0 the forge overflowed by a discard in Strategic (the only way a Strategic game is lost);
//          1 Time Trial: the forge stayed full until the game's grace ran out (Board::mFullCount); 2 the game was left (the
//          player started another or went back to the selector before it ended); 3 Time Trial: a discard overflowed the forge
//
// Events, for the current board:
//   GO          the first update of a fresh board (not a continued saved game, not first seen at activation) that is
//               STATE_PLAYING with nothing done yet: the board is made in the click that starts the game (IntroScreen's start
//               or tutorial button), so this is the update after that click. Once per board
//   level clear the board enters STATE_BOARD_CLEARED: ShowBoardComplete puts the statistics panel up ("1 Board Cleared",
//               "N Boards Cleared"), in the update the last piece has vanished
//   game over   the board enters STATE_ENDING (Board::GameOver: the forge overflowed or, in Time Trial, stayed full), or the
//               board is replaced or taken away while the game is on (left)
//   win         the tutorial's last step (Board::NextTutorial case 0xe) is shown: the wizard's closing words and the "Click here
//               to start playing!" button. That button only returns to the selector (Board::ButtonDepress case 1)
// A board's first update is a baseline for the level clear and the game over (a board that is already over when first seen has
// had its game over).

#include "LiveSplitObserver.h"

#ifdef ALCHEMY_HAS_LIVESPLIT

#include "platform/AutosplitBlock.h"
#include "platform/ModManager.h"
#include "WinAlchApp.h"
#include "Board.h"
#include "Sidebar.h"

#include <SDL.h>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif
#include <cstdio>
#include <cstring>

using namespace Sexy;

// The block (the shared layout 2): constant-initialised, aligned, kept by the linker. Only the writer's Publish() writes it.
extern "C" __attribute__((used, aligned(64))) volatile AutosplitBlock gLiveSplitBlock =
	AUTOSPLIT_BLOCK_INIT(AUTOSPLIT_GAME_ID('A', 'L', 'D', 'X'));

namespace
{

enum { MODE_STRATEGIC = 0, MODE_TIME_TRIAL = 1, MODE_TUTORIAL = 2 };
enum { DEATH_STRATEGIC_FORGE = 0, DEATH_TIME_TRIAL_FULL = 1, DEATH_LEFT = 2, DEATH_TIME_TRIAL_DISCARD = 3 };
const char* const	gDeathNames[4] = { "strategic forge", "time trial full", "left", "time trial discard" };
const int			TUTORIAL_LAST_STEP = 0xe;   // Board::NextTutorial's closing step
const int			FULL_FORGE_UPDATES = 0x95;  // BoardUpdate.cpp: the time trial ends when the forge has been full for more

AutosplitWriter		sWriter(&gLiveSplitBlock);  // the counters, serials, latched flags and the seq writes
bool				sGoPossible = false;        // the current board may still have its GO
int					sPrevState = -1;            // the current board's state at the previous update
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

// The mode and skill: layout 2's own fields, and the same two numbers in the game's own bytes at 0x80 and 0x84, where Big
// Money's and BookWorm's layout-1 blocks keep theirs
void SetModeAndSkill(int32_t theMode, int32_t theSkill)
{
	AutosplitBlock& aNext = sWriter.Next();
	memcpy(aNext.gameOwn + 0, &theMode, sizeof(theMode));
	memcpy(aNext.gameOwn + 4, &theSkill, sizeof(theSkill));
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
	SetModeAndSkill(-1, 0);
	SDL_Log("livesplit: block at 0x%08x (layout %u, %u bytes)", (unsigned) sWriter.Next().self, (unsigned) AUTOSPLIT_LAYOUT_VERSION,
		(unsigned) AUTOSPLIT_SIZE_V2);
}

} // namespace

void LiveSplitObserver::SetLogFile(const std::string& theFileName)
{
	sLogFileName = theFileName;
}

void LiveSplitObserver::AfterUpdate(WinAlchApp* theApp)
{
	if ((!sWriter.IsActive()) && (!sPageAsked) && (!FeatureAsked()))
		return;
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
		if ((!isActivation) && (aNext.inGame != 0))
		{
			// The game on the old board was left: the app made another board or took it away (the update's own end state
			// no longer has it), so its game over is written first, on its own serial
			sWriter.GameOver(DEATH_LEFT);
			SDL_Log("livesplit: board %u game over (%s)", (unsigned) aNext.boardSerial, gDeathNames[DEATH_LEFT]);
			Publish(theApp->mUpdateCount);
		}
		sWriter.NewBoard(aSerial);
		sGoPossible = false;
		sPrevState = -1;
	}

	if (aBoard == NULL)
	{
		sWriter.ClearBoard();
		SetModeAndSkill(-1, 0);
	}
	else
	{
		int aState = aBoard->mState;
		bool isOverState = (aState == Board::STATE_ENDING) || (aState == Board::STATE_GAME_OVER);
		int aMode = aBoard->mTutorial ? MODE_TUTORIAL : (aBoard->mTimeTrial ? MODE_TIME_TRIAL : MODE_STRATEGIC);
		int aSkill = (aMode == MODE_TUTORIAL) ? 0 : (aBoard->mSkillBase / 5 + 1);   // the board's own: mSkillBase is skill * 5 - 5
		if (isNewBoard)
		{
			// The baseline: a board seen over has had its game over, and its state is the previous one
			if (isOverState)
				sWriter.SeenOver();
			sPrevState = aState;
			sGoPossible = (!isActivation) && (!aBoard->mResumed);
			if ((!isActivation) && (aBoard->mResumed))
				SDL_Log("livesplit: board %u continues a saved game: GO not counted", (unsigned) aSerial);
		}

		if (sGoPossible)
		{
			if ((!aBoard->mPristine) || (aBoard->mBoardsCleared != 0) || isOverState)
				sGoPossible = false;    // the start went by unseen: no GO for this board
			else if (aState == Board::STATE_PLAYING)
			{
				sGoPossible = false;
				sWriter.Go(aBoard->mUpdateCnt);
				SDL_Log("livesplit: board %u GO at tick %d (mode %d, skill %d)", (unsigned) aSerial, aBoard->mUpdateCnt, aMode, aSkill);
			}
		}

		// The score the side panel shows (its rolling figure), which the board's high-water mark follows
		sWriter.SetDisplayScore((theApp->mSidebar != NULL) ? (int32_t) theApp->mSidebar->mScoreAnim : aBoard->mScore);

		if (!isNewBoard)
		{
			if ((aState == Board::STATE_BOARD_CLEARED) && (sPrevState != Board::STATE_BOARD_CLEARED))
			{
				sWriter.LevelClear();
				SDL_Log("livesplit: board %u board cleared %d at tick %d", (unsigned) aSerial, (int) aNext.boardLevelClears, aBoard->mUpdateCnt);
			}
			if ((!sWriter.HadGameOver()) && (!sWriter.HadWin()) && isOverState)
			{
				int aKind;
				if (aMode == MODE_STRATEGIC)
					aKind = DEATH_STRATEGIC_FORGE;
				else
					aKind = (aBoard->mFullCount > FULL_FORGE_UPDATES) ? DEATH_TIME_TRIAL_FULL : DEATH_TIME_TRIAL_DISCARD;
				sWriter.GameOver(aKind);
				SDL_Log("livesplit: board %u game over (%s) at tick %d", (unsigned) aSerial, gDeathNames[aKind], aBoard->mUpdateCnt);
			}
			if ((!sWriter.HadGameOver()) && (!sWriter.HadWin()) && (aNext.inGame != 0) && aBoard->mTutorial &&
				(aBoard->mTutorialStep == TUTORIAL_LAST_STEP))
			{
				sWriter.Win();
				SDL_Log("livesplit: board %u win (tutorial done) at tick %d", (unsigned) aSerial, aBoard->mUpdateCnt);
			}
		}

		uint32_t aFlags = 0;
		if (aBoard->mResumed)
			aFlags |= AUTOSPLIT_FLAG_RESUMED;
		if (theApp->mHasSeedParam)
			aFlags |= AUTOSPLIT_FLAG_CHOSEN_SEED;   // port: only a test's -seed (the player has no way to choose one)
		else
			aFlags |= AUTOSPLIT_FLAG_VANILLA;       // no gameplay mod exists for Alchemy: its fixes mod leaves play as it was
		if (aBoard->mPauseCount > 0)
			aFlags |= AUTOSPLIT_FLAG_PAUSED;
		aNext.flags = aFlags;                       // the writer adds the game over's and the win's flags from its latches
		aNext.tick = aBoard->mUpdateCnt;
		aNext.state = aState;
		aNext.level = aBoard->mBoardsCleared;
		aNext.displayLevel = aBoard->mBoardsCleared + 1;
		aNext.score = aBoard->mScore;
		SetModeAndSkill(aMode, aSkill);

		sPrevState = aState;
	}

	Publish(theApp->mUpdateCount);
}

void LiveSplitObserver::AppDestroyed(WinAlchApp* theApp)
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
// next update, as the feature does natively (web/autosplit.js)
extern "C" EMSCRIPTEN_KEEPALIVE uint32_t byopop_livesplit_block()
{
	sPageAsked = true;
	return (uint32_t) (uintptr_t) &gLiveSplitBlock;
}
#endif

#endif
