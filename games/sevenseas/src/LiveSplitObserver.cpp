// port(mod): the shared autosplit block for Seven Seas (see LiveSplitObserver.h).
//
// What the Splits window reads, per board (Board::mBoardSerial, a counter of boards made):
//   mode        0 a game, 1 the tutorial; -1 off a board
//   skill       the menu's difficulty: 1 Easy (the original's "beginner"), 2 Average, 3 Hard; 0 off a board
//   GO          the board's first update when it is a new game (not resumed, not the tutorial). On Easy the board is made
//               by the demo's "Click Here To Start" button, so this is the click's update; on Average and Hard it is the
//               update after the menu's Play
//   level clear the level counter (Board::mLevel) goes up: the player closed the level's panel and NewLevel began the next
//               level. After a treasure level (10, 20, 30) the same call shows the map (SevenSeasApp::ShowMap), so there
//               the clear and the map are one update. Undo is cleared there (ClearUndo(0)), so no clear is taken back
//   game over   the board enters STATE_LEVEL_END (Board::GameOver, out of lives): kind 0. Kind 1 to 3 are unused. Leaving a
//               game through New Game saves it and removes the board; the block then shows no board and no game
//   progress    the levels completed: mLevel - 1 (30 once level 31 has begun, with the map up)
//   no win      the game goes on past level 30, so Win() is never called

#include "LiveSplitObserver.h"

#ifdef SEVENSEAS_HAS_LIVESPLIT

#include "SevenSeasApp.h"
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

using namespace Sexy;

extern "C" __attribute__((used, aligned(64))) volatile AutosplitBlock gLiveSplitBlock =
	AUTOSPLIT_BLOCK_INIT(AUTOSPLIT_GAME_ID('S', 'S', 'D', 'X'));

namespace
{

AutosplitWriter		sWriter(&gLiveSplitBlock);
bool				sGoPossible = false;        // the current board may still have its GO
int					sPrevLevel = 0;             // the board's level at the previous update
int					sPrevState = -1;
bool				sPageAsked = false;         // web: the page's timer asked for the block
std::string			sLogFileName;
FILE*				sLogFile = NULL;

bool FeatureOn()
{
	for (const Platform::ModInfo* aMod : Platform::ModManager::Get().GetEnabledMods())
	{
		if (aMod->mFeaturesJson.IsObject() && aMod->mFeaturesJson.Has("livesplit"))
			return true;
	}
	return false;
}

void Log(const char* theFormat, ...) __attribute__((format(printf, 1, 2)));

void Log(const char* theFormat, ...)
{
	char aLine[256];
	va_list aList;
	va_start(aList, theFormat);
	vsnprintf(aLine, sizeof(aLine), theFormat, aList);
	va_end(aList);
	SDL_Log("livesplit: %s", aLine);
	if (!sLogFileName.empty())
	{
		if (sLogFile == NULL)
			sLogFile = fopen(sLogFileName.c_str(), "ab");
		if (sLogFile != NULL)
		{
			fprintf(sLogFile, "%s\n", aLine);
			fflush(sLogFile);
		}
	}
}

void Publish()
{
	sWriter.Publish(SDL_GetPerformanceCounter());
}

}

void LiveSplitObserver::SetLogFile(const std::string& theFileName)
{
	sLogFileName = theFileName;
}

void LiveSplitObserver::AfterUpdate(SevenSeasApp* theApp)
{
	if ((!sWriter.IsActive()) && (!sPageAsked) && (!FeatureOn()))
		return;
	AutosplitBlock& aNext = sWriter.Next();
	bool isActivation = !sWriter.IsActive();
	if (isActivation)
	{
		sWriter.Activate(SDL_GetPerformanceFrequency());
		SDL_Log("livesplit: block at 0x%08x (layout %u)", (unsigned) aNext.self, (unsigned) AUTOSPLIT_LAYOUT_VERSION);
	}

	Board* aBoard = theApp->mBoard;
	uint32_t aSerial = (aBoard != NULL) ? aBoard->mBoardSerial : 0;
	bool isNewBoard = isActivation || (aSerial != aNext.boardSerial);
	if (isNewBoard)
	{
		sWriter.NewBoard(aSerial);
		sGoPossible = false;
		sPrevState = -1;
	}

	if (aBoard == NULL)
	{
		sWriter.ClearBoard();
	}
	else
	{
		int aState = aBoard->mState;
		bool isTutorial = theApp->mTutorial;
		if (isNewBoard)
		{
			if (aState == STATE_LEVEL_END)
				sWriter.SeenOver();
			sPrevLevel = aBoard->mLevel;
			sPrevState = aState;
			sGoPossible = (!isActivation) && (!aBoard->mResumed) && (!isTutorial);
		}

		if (sGoPossible)
		{
			sGoPossible = false;
			if (aState != STATE_LEVEL_END)
			{
				sWriter.Go(aBoard->mUpdateCnt);
				Log("board %u GO at tick %d (difficulty %d)", (unsigned) aSerial, aBoard->mUpdateCnt, theApp->mDifficulty);
			}
		}

		if (!isNewBoard)
		{
			if (aBoard->mLevel > sPrevLevel)
			{
				sWriter.LevelClear();
				Log("board %u level clear %d at tick %d (map %d)", (unsigned) aSerial, aBoard->mLevel - 1, aBoard->mUpdateCnt,
					(theApp->mMapWidget != NULL) ? 1 : 0);
			}
			if ((!sWriter.HadGameOver()) && (aState == STATE_LEVEL_END) && (sPrevState != STATE_LEVEL_END))
			{
				sWriter.GameOver(0);
				Log("board %u game over (out of lives) at tick %d after level %d", (unsigned) aSerial, aBoard->mUpdateCnt,
					aBoard->mLevel);
			}
		}

		uint32_t aFlags = 0;
		if (aBoard->mResumed)
			aFlags |= AUTOSPLIT_FLAG_RESUMED;
		if (Platform::ModManager::Get().GetScoreKey().empty())
			aFlags |= AUTOSPLIT_FLAG_VANILLA;
		aNext.flags = aFlags;
		aNext.tick = aBoard->mUpdateCnt;
		aNext.state = aState;
		aNext.level = aBoard->mLevel;
		aNext.displayLevel = aBoard->mLevel;
		aNext.score = aBoard->mScore;
		sWriter.SetDisplayScore(aBoard->mScore);
		aNext.mode = isTutorial ? 1 : 0;
		aNext.skill = theApp->mDifficulty;
		aNext.progress = (aBoard->mLevel > 0) ? aBoard->mLevel - 1 : 0;
		sPrevLevel = aBoard->mLevel;
		sPrevState = aState;
	}
	Publish();
}

void LiveSplitObserver::AppDestroyed(SevenSeasApp*)
{
	if (sWriter.IsActive())
	{
		sWriter.Deactivate();
		Publish();
	}
	if (sLogFile != NULL)
	{
		fclose(sLogFile);
		sLogFile = NULL;
	}
}

#ifdef __EMSCRIPTEN__
extern "C" EMSCRIPTEN_KEEPALIVE uint32_t byopop_livesplit_block()
{
	sPageAsked = true;
	return (uint32_t) (uintptr_t) &gLiveSplitBlock;
}
#endif

#endif
