// port(mod): Dynomite's autosplit observer. See LiveSplitObserver.h and platform/AutosplitBlock.h (layout 2, game id "DYDX").
//
// The block is a constant-initialised static object, so its magic exists only in .data's initialised bytes, once in the exe,
// and the AutosplitWriter keeps its counters, serials and the seq protocol. It is in every Windows and web build but written
// only while an enabled mod has the feature "livesplit" or, on the web, the page asked for it (byopop_livesplit_block); until
// then active and self stay 0. The observer runs after each app update (FossilizedApp::UpdateFrames), reads the board's, the
// tutorial's and the side bar's fields only, and sees each update's end state, not what happened inside it.
//
// A game is one board (Dynomite::mSerial: StartGame makes a new one for each game, Replay and a continued saved game too) or
// one tutorial (Tutorial::mSerial), whose own boards (Tutorial::SetupBoard, one for each part) are not games of their own.
// The numbering the speedrun.com rules table reads:
//   mode   1 Stomped Puzzle, 2 Endless Puzzle, 3 Fossil Challenge, 4 Time Trial (the app's mGameMode, the board's mGameType
//          + 1), 5 the tutorial; -1 off a game
//   skill  Stomped, Endless and Time Trial: 1 Normal, 2 Hard, 3 Expert, as the game's own skill page names them;
//          Fossil Challenge: 1 Strategic, 2 Panic; 0 in the tutorial and off a game
//   level, displayLevel
//          Fossil Challenge: the fossils found (0-based) and the fossil being dug, 1 to 15 (DynomiteFossil::mLevel);
//          Stomped: the rounds cleared and the round's number (mRoundsCleared, mRound); else 0 and 1
//   state  a board: 0 before the first shot, 1 in play, 2 losing (the eggs stayed past the line: they run down off the
//          board), 3 a fossil fell (the round is won, the museum to come), 4 game over, 5 the end's panel is up;
//          the tutorial: its step (Tutorial::mStep, 0 to 0x6f)
//   tick   a board: Dynomite::mPlayTicks, the hundredths of a second played (they count from the first shot while the game
//          is not over; the Time Trial's clock on the side bar is the score, one less each such update); the tutorial: its
//          updates
//   progress
//          Fossil Challenge: the fragments found, 3 to a fossil (0 to 45); the tutorial: its parts done, 1 Endless,
//          2 Stomped, 4 Fossil Challenge, 8 Time Trial (each the update its last step shows); else -1
//   score, displayScore
//          the side bar's score and the figure it shows (SideBar::mScore, mScoreShown; equal at each update's end); in the
//          Time Trial the clock, in negative hundredths of a second
//   deaths 0 Endless Puzzle: the eggs ran off the bottom; 1 Fossil Challenge: the same; 2 the game was left (another
//          started, the main menu, or the tutorial closed before all four parts were done); 3 Stomped Puzzle: the eggs ran
//          off the bottom. The Time Trial cannot be lost.
//
// Events, for the current game:
//   GO          Stomped, Endless and the Fossil Challenge: the first update of a fresh board (not a continued saved game, not
//               first seen at activation). The board is made in the click that starts the game (the skill page's Let's
//               Play, or a second click on the skill or the fossil kind already chosen; Replay), so this is the update
//               after that click.
//               Time Trial: the first update with a shot fired (Dynomite::MouseDown fires the egg and sets mPlaying).
//               The tutorial: its first update (the start page's Next with Tutorial chosen makes it). Once a game
//   level clear the Fossil Challenge: a fossil's third fragment fell off the bottom (DynomiteFossil::PieceFell, mRound 3);
//               Stomped: a round is cleared (DynomiteStomped::mCleared); the tutorial: a part's last step shows
//   game over   the board's mGameOver rises without a win (the eggs have run off the bottom, the "game over" picture; the
//               loss was decided 15 to 20 updates before, when mLosing rose and Inchy began to sink), or the game is left
//   win         the Fossil Challenge: the fifteenth fossil's third fragment fell (the final board cleared; the museum and the
//               game's own end follow); Time Trial: mGameOver rises (every egg gone, "cleared"); the tutorial: it closes,
//               back at the main menu, with all four parts done (its last step, 0x6f, calls the menu button)

#include "LiveSplitObserver.h"

#ifdef DYNOMITE_HAS_LIVESPLIT

#include "platform/AutosplitBlock.h"
#include "platform/ModManager.h"
#include "FossilizedApp.h"
#include "Dynomite.h"
#include "DynomiteFossil.h"
#include "DynomiteStomped.h"
#include "SideBar.h"
#include "Tutorial.h"

#include <SDL.h>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif
#include <cstdio>
#include <cstring>

using namespace Sexy;

// The block (the shared layout 2): constant-initialised, aligned, kept by the linker. Only the writer's Publish() writes it.
extern "C" __attribute__((used, aligned(64))) volatile AutosplitBlock gLiveSplitBlock =
	AUTOSPLIT_BLOCK_INIT(AUTOSPLIT_GAME_ID('D', 'Y', 'D', 'X'));

namespace
{

enum { MODE_STOMPED = 1, MODE_ENDLESS = 2, MODE_FOSSIL = 3, MODE_TIME_TRIAL = 4, MODE_TUTORIAL = 5 };
enum { DEATH_ENDLESS = 0, DEATH_FOSSIL = 1, DEATH_LEFT = 2, DEATH_STOMPED = 3 };
const char* const	gDeathNames[4] = { "endless", "fossil", "left", "stomped" };
enum { STATE_WAITING = 0, STATE_PLAYING = 1, STATE_LOSING = 2, STATE_FOSSIL_FELL = 3, STATE_GAME_OVER = 4, STATE_END_PANEL = 5 };
const int			LAST_FOSSIL = 15;           // DynomiteFossil::NextRound: the game ends after the fifteenth fossil
const int			FRAGMENTS = 3;              // its rounds, one fragment each
// The tutorial's parts (Tutorial::ButtonPress's links and Step): each one's last step, before the next part's board
const int			TUTORIAL_PART_ENDS[4] = { 0x41, 0x4e, 0x62, 0x6e };
const int			TUTORIAL_ALL_PARTS = 0xf;

AutosplitWriter		sWriter(&gLiveSplitBlock);  // the counters, serials, latched flags and the seq writes
bool				sGoPossible = false;        // the current game may still have its GO
bool				sPrevOver = false;          // the current board's mGameOver at the previous update
bool				sPrevFell = false;          // its fossil had fallen (Fossil Challenge)
bool				sPrevCleared = false;       // its round was cleared (Stomped)
int					sTutorialStart = 0;         // the app's update count when the current tutorial was first seen
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

// The tutorial's parts done so far: a part is done once its last step has shown
int TutorialParts(const Tutorial* theTutor, int thePrevParts)
{
	int aParts = (thePrevParts < 0) ? 0 : thePrevParts;
	for (int i = 0; i < 4; i++)
	{
		if (theTutor->mShownStep == TUTORIAL_PART_ENDS[i])
			aParts |= 1 << i;
	}
	return aParts;
}

int BoardState(Dynomite* theBoard, bool theFossilFell)
{
	if (theBoard->mGameOverShown)
		return STATE_END_PANEL;
	if (theBoard->mGameOver)
		return STATE_GAME_OVER;
	if (theBoard->mLosing)
		return STATE_LOSING;
	if (theFossilFell)
		return STATE_FOSSIL_FELL;
	return theBoard->mPlaying ? STATE_PLAYING : STATE_WAITING;
}

// The tutorial's update: its step, the parts done (a level clear each) and its tick
void UpdateTutorial(FossilizedApp* theApp, Tutorial* theTutor, bool isNewGame, bool isActivation)
{
	AutosplitBlock& aNext = sWriter.Next();
	if (isNewGame)
	{
		sTutorialStart = theApp->mUpdateCount;
		sGoPossible = !isActivation;
		aNext.progress = 0;
	}
	if (sGoPossible)
	{
		sGoPossible = false;
		sWriter.Go(0);
		SDL_Log("livesplit: tutorial %u GO (mode %d, skill 0)", (unsigned) theTutor->mSerial, MODE_TUTORIAL);
	}
	int aParts = TutorialParts(theTutor, aNext.progress);
	if ((aParts != aNext.progress) && (!isNewGame))
	{
		sWriter.LevelClear();
		SDL_Log("livesplit: tutorial %u part done (parts 0x%x) at step 0x%x", (unsigned) theTutor->mSerial, aParts,
			theTutor->mShownStep);
	}
	aNext.progress = aParts;
	aNext.flags = theApp->mHasSeedParam ? AUTOSPLIT_FLAG_CHOSEN_SEED : AUTOSPLIT_FLAG_VANILLA;
	aNext.tick = theApp->mUpdateCount - sTutorialStart;
	aNext.state = theTutor->mStep;
	aNext.level = 0;
	aNext.displayLevel = 0;
	aNext.score = (theApp->mSideBar != NULL) ? theApp->mSideBar->mScore : 0;
	sWriter.SetDisplayScore((theApp->mSideBar != NULL) ? theApp->mSideBar->mScoreShown : 0);
	SetModeAndSkill(MODE_TUTORIAL, 0);
}

// A board's update: GO, a level clear, the game over or the win, and the fields
void UpdateBoard(FossilizedApp* theApp, Dynomite* theBoard, bool isNewGame, bool isActivation)
{
	AutosplitBlock& aNext = sWriter.Next();
	int aMode = theBoard->mGameType + 1;
	DynomiteFossil* aFossil = (aMode == MODE_FOSSIL) ? (DynomiteFossil*) theBoard : NULL;
	DynomiteStomped* aStomped = (aMode == MODE_STOMPED) ? (DynomiteStomped*) theBoard : NULL;
	int aSkill = (aFossil != NULL) ? (aFossil->mPanicKind ? 2 : 1) : theBoard->mDifficulty;
	bool isOver = theBoard->mGameOver;
	bool hasFallen = (aFossil != NULL) && aFossil->mFossilFell;
	bool isCleared = (aStomped != NULL) && aStomped->mCleared;

	if (isNewGame)
	{
		// The baseline: a board seen over has had its game over, and its flags are the previous ones
		if (isOver)
			sWriter.SeenOver();
		sPrevOver = isOver;
		sPrevFell = hasFallen;
		sPrevCleared = isCleared;
		sGoPossible = (!isActivation) && (!theBoard->mResumed) && (!isOver);
		if ((!isActivation) && (theBoard->mResumed))
			SDL_Log("livesplit: board %u continues a saved game: GO not counted", (unsigned) theBoard->mSerial);
	}

	if (sGoPossible)
	{
		if (isOver)
			sGoPossible = false;    // the start went by unseen: no GO for this board
		else if ((aMode != MODE_TIME_TRIAL) || (theBoard->mShotCount > 0))
		{
			sGoPossible = false;
			sWriter.Go(theBoard->mPlayTicks);
			SDL_Log("livesplit: board %u GO at tick %d (mode %d, skill %d)", (unsigned) theBoard->mSerial, theBoard->mPlayTicks,
				aMode, aSkill);
		}
	}

	if (!isNewGame)
	{
		if ((aFossil != NULL) && hasFallen && (!sPrevFell))
		{
			if (aFossil->mRound == FRAGMENTS)
			{
				sWriter.LevelClear();
				SDL_Log("livesplit: board %u fossil %d found at tick %d", (unsigned) theBoard->mSerial, aFossil->mLevel,
					theBoard->mPlayTicks);
				if ((aFossil->mLevel == LAST_FOSSIL) && (!sWriter.HadGameOver()) && (!sWriter.HadWin()) && (aNext.inGame != 0))
				{
					sWriter.Win();
					SDL_Log("livesplit: board %u win (the last fossil) at tick %d", (unsigned) theBoard->mSerial,
						theBoard->mPlayTicks);
				}
			}
		}
		if ((aStomped != NULL) && isCleared && (!sPrevCleared))
		{
			sWriter.LevelClear();
			SDL_Log("livesplit: board %u round %d cleared at tick %d", (unsigned) theBoard->mSerial, aStomped->mRound,
				theBoard->mPlayTicks);
		}
		if (isOver && (!sPrevOver) && (!sWriter.HadGameOver()) && (!sWriter.HadWin()))
		{
			if (aMode == MODE_TIME_TRIAL)
			{
				if (aNext.inGame != 0)
				{
					sWriter.Win();
					SDL_Log("livesplit: board %u win (cleared) at tick %d", (unsigned) theBoard->mSerial, theBoard->mPlayTicks);
				}
			}
			else
			{
				int aKind = (aMode == MODE_FOSSIL) ? DEATH_FOSSIL : ((aMode == MODE_STOMPED) ? DEATH_STOMPED : DEATH_ENDLESS);
				sWriter.GameOver(aKind);
				SDL_Log("livesplit: board %u game over (%s) at tick %d", (unsigned) theBoard->mSerial, gDeathNames[aKind],
					theBoard->mPlayTicks);
			}
		}
	}

	uint32_t aFlags = 0;
	if (theBoard->mResumed)
		aFlags |= AUTOSPLIT_FLAG_RESUMED;
	if (theApp->mHasSeedParam)
		aFlags |= AUTOSPLIT_FLAG_CHOSEN_SEED;   // port: only a test's -seed (the player has no way to choose one)
	else
		aFlags |= AUTOSPLIT_FLAG_VANILLA;       // no gameplay mod exists for Dynomite
	if (theBoard->mPauseCount > 0)
		aFlags |= AUTOSPLIT_FLAG_PAUSED;
	aNext.flags = aFlags;                       // the writer adds the game over's and the win's flags from its latches
	aNext.tick = theBoard->mPlayTicks;
	aNext.state = BoardState(theBoard, hasFallen);
	if (aFossil != NULL)
	{
		aNext.level = aFossil->mLevel - 1;
		aNext.displayLevel = aFossil->mLevel;
		aNext.progress = (aFossil->mLevel - 1) * FRAGMENTS + (aFossil->mRound - 1) + (hasFallen ? 1 : 0);
	}
	else if (aStomped != NULL)
	{
		aNext.level = aStomped->mRoundsCleared;
		aNext.displayLevel = aStomped->mRound;
	}
	else
	{
		aNext.level = 0;
		aNext.displayLevel = 1;
	}
	aNext.score = (theApp->mSideBar != NULL) ? theApp->mSideBar->mScore : 0;
	sWriter.SetDisplayScore((theApp->mSideBar != NULL) ? theApp->mSideBar->mScoreShown : aNext.score);
	SetModeAndSkill(aMode, aSkill);

	sPrevOver = isOver;
	sPrevFell = hasFallen;
	sPrevCleared = isCleared;
}

} // namespace

void LiveSplitObserver::SetLogFile(const std::string& theFileName)
{
	sLogFileName = theFileName;
}

void LiveSplitObserver::AfterUpdate(FossilizedApp* theApp)
{
	if ((!sWriter.IsActive()) && (!sPageAsked) && (!FeatureAsked()))
		return;
	AutosplitBlock& aNext = sWriter.Next();
	// On activation the current game (or none) is new to the block: it is never given a GO
	bool isActivation = !sWriter.IsActive();
	if (isActivation)
		Activate();

	// The tutorial is one game over all the boards it makes for its parts
	Tutorial* aTutor = theApp->mTutor;
	Dynomite* aBoard = (aTutor != NULL) ? NULL : theApp->mBoard;
	uint32_t aSerial = (aTutor != NULL) ? aTutor->mSerial : ((aBoard != NULL) ? aBoard->mSerial : 0);
	bool isNewGame = isActivation || (aSerial != aNext.boardSerial);
	if (isNewGame)
	{
		if ((!isActivation) && (aNext.inGame != 0))
		{
			// The game was left: the app made another or took it away (the update's own end state no longer has it). A
			// tutorial closed with every part done is the tutorial's win; any other, a game over on its own serial
			if ((aNext.mode == MODE_TUTORIAL) && (aNext.progress == TUTORIAL_ALL_PARTS) && (aTutor == NULL) && (aBoard == NULL) &&
				(theApp->mIntroScreen != NULL))
			{
				sWriter.Win();
				SDL_Log("livesplit: tutorial %u win (every part, back at the main menu)", (unsigned) aNext.boardSerial);
			}
			else
			{
				sWriter.GameOver(DEATH_LEFT);
				SDL_Log("livesplit: board %u game over (%s)", (unsigned) aNext.boardSerial, gDeathNames[DEATH_LEFT]);
			}
			Publish(theApp->mUpdateCount);
		}
		sWriter.NewBoard(aSerial);
	}

	if (aTutor != NULL)
		UpdateTutorial(theApp, aTutor, isNewGame, isActivation);
	else if (aBoard != NULL)
		UpdateBoard(theApp, aBoard, isNewGame, isActivation);
	else
	{
		sWriter.ClearBoard();
		SetModeAndSkill(-1, 0);
	}

	Publish(theApp->mUpdateCount);
}

void LiveSplitObserver::AppDestroyed(FossilizedApp* theApp)
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
