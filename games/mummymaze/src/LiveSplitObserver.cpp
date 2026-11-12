// port(mod): the autosplit observer. See LiveSplitObserver.h.
//
// The block is the shared AutosplitBlock (platform/AutosplitBlock.h, layout 2): a constant-initialised static object, so its magic
// is only in .data's initialised bytes, once in the exe, and the AutosplitWriter keeps its counters, serials and the seq protocol.
// It is in every Windows and web build but written only while the "livesplit" feature mod is on or, on the web, the page asked
// for it (byopop_livesplit_block); until then active and self stay 0. The observer runs after each update (MMApp::UpdateFrames),
// reads the app's and the board's fields only, and sees each update's end state, not what happened inside it.
//
// A "board" of the block is a game: from the menu's Classic or Adventure link until the main menu is back. The game's own Board
// is one chamber (or the ending scene of a pyramid), so a game spans many of them, and the map and the level-complete and
// high-score screens in between. A game starts when the main menu is gone and a board, the world map or the level-complete screen
// is up in a classic or adventure mode (the tutorial is no game), and ends when the menu is back.
//
// Events, for the current game:
//   GO          the first update of a fresh game (not a resumed save, not first seen at activation) on which its first chamber's
//               board takes the explorer's moves: STATE_READY (the explorer has walked in), the maze has risen (mRiseFade),
//               no dialog is up. That is what Board::UpdateExplorer and Board::MouseDown wait for. Once per game
//   level clear a chamber solved: the level-complete screen comes up (Board::Update, L_finish: the explorer left the maze, MMApp::
//               DoNextLevel, whose NextLevelWidget takes the board away, so the screen is seen with no board: the observer
//               reports it as STATE_DONE). Fifteen to a pyramid, in the classic game and the adventure
//   treasure    the ending scene's chest opens (Board::Update: STATE_READY to STATE_ENDING_FADE in MODE_CLASSIC_DONE or
//               MODE_ADVENTURE_DONE, SOUND_OPENTREASURE). The ending scene follows the pyramid's fifteenth chamber
//   win         the treasure of the last pyramid (14) opens: the classic game's or the adventure's final one
//   game over   Abandon Hope in the classic game, once its solution has played (Board::DoGameOver with mShowingSolution, which
//               brings up the high scores). Being caught is not one: the panel offers Undo, Try again and the others and the
//               game goes on; in the adventure it only counts a catch. Leaving by Save and Quit ends the game as the menu does
// A game's first update, and the first update after the block was asked for, is a baseline: it raises no event.

#include "LiveSplitObserver.h"

#ifdef MUMMYMAZE_HAS_LIVESPLIT

#include "MummyApp.h"
#include "Board.h"
#include "platform/AutosplitBlock.h"
#include "platform/ModManager.h"

#include <SDL.h>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <string>

using namespace Sexy;

// The block (the shared layout 2): constant-initialised, aligned, kept by the linker. Only the writer's Publish() writes it.
extern "C" __attribute__((used, aligned(64))) volatile AutosplitBlock gLiveSplitBlock = AUTOSPLIT_BLOCK_INIT(AUTOSPLIT_GAME_ID('M', 'M', 'D', 'X'));

namespace
{

// The block's mode numbers, as the site's rules table names them
enum
{
	LIVESPLIT_MODE_CLASSIC = 0,
	LIVESPLIT_MODE_ADVENTURE = 1
};

enum
{
	CHAMBERS_PER_PYRAMID = 15,
	LAST_PYRAMID = 14,
	FEATURE_CHECK_UPDATES = 100,   // how often the feature mod is looked for while the block is not asked for
	DEATH_ABANDON_HOPE = 0         // the game's one kind of game over
};

AutosplitWriter		sWriter(&gLiveSplitBlock);   // the counters, serials, latched flags and the seq writes
bool				sPageAsked = false;          // web: the page's timer asked for the block (byopop_livesplit_block)
int					sFeatureCountdown = 0;       // updates until the feature mod is looked for again
bool				sResumeNext = false;         // a saved game was loaded: the game that starts next resumes it
uint32_t			sGameSerial = 0;             // the last serial given to a game
bool				sInGame = false;             // a game is in play
bool				sGoPossible = false;         // the current game may still have its GO
bool				sResumed = false;            // the current game resumed a saved game
int					sTreasures = 0;              // chests opened in the current game
const Board*		sPrevBoard = NULL;           // the board seen at the previous update
int					sPrevBoardTick = 0;          // its update count then
int					sPrevState = -1;             // its state then
bool				sPrevLevelComplete = false;  // the level-complete screen was up then
std::string			sLogFileName;                // -livesplitlog
FILE*				sLogFile = NULL;

// Whether an enabled mod names the feature "livesplit" (the mod that carries the block's native script)
bool FeatureAsked()
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

// One write (the writer's seq protocol); with the events of this update, one line of them and the block, in the log
void Publish(MMApp* theApp, const char* theEvents)
{
	sWriter.Publish(SDL_GetPerformanceCounter());
	if (theEvents[0] == 0)
		return;
	SDL_Log("livesplit: %s", theEvents);
	if (sLogFileName.empty())
		return;
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
	fprintf(sLogFile, "u=%d %s | %s\n", theApp->mUpdateCount, theEvents, aLine);
	fflush(sLogFile);
}

// Appends one event to the update's list
void AddEvent(char* theEvents, size_t theSize, const char* theFormat, ...) __attribute__((format(printf, 3, 4)));
void AddEvent(char* theEvents, size_t theSize, const char* theFormat, ...)
{
	size_t aLength = strlen(theEvents);
	if ((aLength > 0) && (aLength + 3 < theSize))
	{
		theEvents[aLength++] = ';';
		theEvents[aLength++] = ' ';
		theEvents[aLength] = 0;
	}
	if (aLength + 1 >= theSize)
		return;
	va_list aList;
	va_start(aList, theFormat);
	vsnprintf(theEvents + aLength, theSize - aLength, theFormat, aList);
	va_end(aList);
}

} // namespace

void LiveSplitObserver::SetLogFile(const std::string& theFileName)
{
	sLogFileName = theFileName;
}

void LiveSplitObserver::SavedGameLoaded()
{
	sResumeNext = true;
}

void LiveSplitObserver::AfterUpdate(MMApp* theApp)
{
	bool isActivation = false;
	if (!sWriter.IsActive())
	{
		if (!sPageAsked)
		{
			if (sFeatureCountdown > 0)
			{
				sFeatureCountdown--;
				return;
			}
			sFeatureCountdown = FEATURE_CHECK_UPDATES;
			if (!FeatureAsked())
				return;
		}
		isActivation = true;
		sWriter.Activate(SDL_GetPerformanceFrequency());
		sInGame = false;
		sResumeNext = false;
		SDL_Log("livesplit: block at 0x%08x (layout %u, %u bytes)", (unsigned) sWriter.Next().self, (unsigned) AUTOSPLIT_LAYOUT_VERSION,
			(unsigned) AUTOSPLIT_SIZE_V2);
	}

	AutosplitBlock& aNext = sWriter.Next();
	char anEvents[320];
	anEvents[0] = 0;

	int aMode = theApp->mGameMode;
	Board* aBoard = theApp->mBoard;
	bool isEnding = (aMode == MODE_ADVENTURE_DONE) || (aMode == MODE_CLASSIC_DONE);
	bool isClassic = (aMode == MODE_CLASSIC) || (aMode == MODE_CLASSIC_DONE);
	bool hasScreen = (aBoard != NULL) || (theApp->mWorldMap != NULL) || (theApp->mLevelComplete != NULL);
	// the victory and high-score screens belong to a game already in play (the menu's High Scores link shows the latter with no game)
	bool isInPlay = (aMode < MODE_TUTORIAL) && (theApp->mMainMenu == NULL) &&
		(hasScreen || (sInGame && ((theApp->mVictory != NULL) || (theApp->mHighScores != NULL))));

	if (isInPlay && !sInGame)
	{
		// A game starts, or was already running when the block was asked for (then it is never given a GO)
		sInGame = true;
		sWriter.NewBoard(++sGameSerial);
		sResumed = sResumeNext && !isActivation;
		sResumeNext = false;
		sGoPossible = !isActivation && !sResumed;
		sTreasures = 0;
		sPrevBoard = NULL;
		sPrevBoardTick = 0;
		sPrevState = -1;
		sPrevLevelComplete = false;
		AddEvent(anEvents, sizeof(anEvents), isClassic ? "game %d: classic" : "game %d: adventure", (int) sGameSerial);
		if (sResumed)
			AddEvent(anEvents, sizeof(anEvents), "game %d resumed a saved game: no GO", (int) sGameSerial);
	}
	else if (!isInPlay && sInGame)
	{
		sInGame = false;
		sWriter.NewBoard(0);
	}

	if (sInGame)
	{
		bool isLevelComplete = (theApp->mLevelComplete != NULL);
		int aState = (aBoard != NULL) ? aBoard->mState : (isLevelComplete ? (int) STATE_DONE : -1);
		bool isNewBoard = (aBoard != sPrevBoard) || ((aBoard != NULL) && (aBoard->mUpdateCnt < sPrevBoardTick));
		if (isNewBoard)
			sPrevState = -1;
		if (isActivation)
		{
			sPrevState = aState;    // the baseline: what the board was already doing raises no event
			sPrevLevelComplete = isLevelComplete;
		}
		int aPyramid = isClassic ? theApp->mClassicPyramid : theApp->mAdvPyramid;
		int aChamber = isClassic ? theApp->mClassicChamber : theApp->mAdvChamber;
		int aTick = theApp->mUpdateCount;
		int aGame = (int) sGameSerial;

		if (isLevelComplete && !sPrevLevelComplete)
		{
			sWriter.LevelClear();
			AddEvent(anEvents, sizeof(anEvents), "level clear %d at tick %d", aPyramid * CHAMBERS_PER_PYRAMID + aChamber + 1, aTick);
		}
		if (aBoard != NULL)
		{
			if (sGoPossible)
			{
				if (isEnding || (aState == STATE_DONE))
					sGoPossible = false;    // the start went by unseen: no GO for this game
				else if ((aState == STATE_READY) && aBoard->mStarted && (aBoard->mRiseFade >= 1.0) && (theApp->GetDialogCount() == 0))
				{
					sGoPossible = false;
					sWriter.Go(aTick);
					AddEvent(anEvents, sizeof(anEvents), "game %d GO at tick %d", aGame, aTick);
				}
			}
			if (isEnding && (aState == STATE_ENDING_FADE) && (sPrevState != STATE_ENDING_FADE) && (sPrevState != STATE_ENDING))
			{
				sTreasures++;
				AddEvent(anEvents, sizeof(anEvents), "treasure %d at tick %d", sTreasures, aTick);
				if ((aPyramid >= LAST_PYRAMID) && !sWriter.HadWin())
				{
					sWriter.Win();
					AddEvent(anEvents, sizeof(anEvents), "win at tick %d", aTick);
				}
			}
			if ((aMode == MODE_CLASSIC) && aBoard->mShowingSolution && (aState == STATE_LOST) && !sWriter.HadGameOver() && !sWriter.HadWin())
			{
				sWriter.GameOver(DEATH_ABANDON_HOPE);
				AddEvent(anEvents, sizeof(anEvents), "game over (Abandon Hope) at tick %d", aTick);
			}
		}

		uint32_t aFlags = 0;
		if (sResumed)
			aFlags |= AUTOSPLIT_FLAG_RESUMED;
		if (!(theApp->mFixes.mGameClock || theApp->mFixes.mSolverFight || theApp->mFixes.mKeyTaps))
			aFlags |= AUTOSPLIT_FLAG_VANILLA;
		if (theApp->GetDialogCount() > 0)
			aFlags |= AUTOSPLIT_FLAG_PAUSED;
		aNext.flags = aFlags;               // the writer adds the game over's and the win's flags from its latches
		aNext.tick = aTick;
		aNext.state = aState;
		aNext.level = aPyramid * CHAMBERS_PER_PYRAMID + aChamber;
		aNext.displayLevel = aNext.level + 1;
		aNext.mode = isClassic ? LIVESPLIT_MODE_CLASSIC : LIVESPLIT_MODE_ADVENTURE;
		aNext.skill = 0;
		// The treasures collected. The classic game counts a pyramid's treasure when the next pyramid has begun (the pyramid
		// number rises with the new pyramid's board), and the last pyramid's when its chest opens, since no pyramid follows; the
		// adventure counts each chest as it opens, since the world map follows
		if (isClassic)
			aNext.progress = aPyramid + (((aPyramid >= LAST_PYRAMID) && isEnding && ((aState == STATE_ENDING_FADE) || (aState == STATE_ENDING))) ? 1 : 0);
		else
			aNext.progress = sTreasures;
		aNext.score = isClassic ? theApp->mScore : 0;
		sWriter.SetDisplayScore(aNext.score);

		sPrevLevelComplete = isLevelComplete;
		sPrevBoard = aBoard;
		sPrevBoardTick = (aBoard != NULL) ? aBoard->mUpdateCnt : 0;
		sPrevState = aState;
	}

	Publish(theApp, anEvents);
}

void LiveSplitObserver::AppDestroyed(MMApp* theApp)
{
	if (sWriter.IsActive())
	{
		sWriter.Deactivate();
		Publish(theApp, "");
	}
	if (sLogFile != NULL)
	{
		fclose(sLogFile);
		sLogFile = NULL;
	}
}

#ifdef __EMSCRIPTEN__
// The page's speedrun timer: the block's address in the game's memory (a HEAPU8 offset); asking turns its writing on from the next
// update, as the feature mod does (web/autosplit.js)
extern "C" EMSCRIPTEN_KEEPALIVE uint32_t byopop_livesplit_block()
{
	sPageAsked = true;
	return (uint32_t) (uintptr_t) &gLiveSplitBlock;
}
#endif

#endif
