// port(variant): the LiveSplit block and its observer (games/bejeweled/docs/CODE_MODS_DESIGN.md 4.15),
// desktop builds only. See LiveSplitObserver.h and LiveSplitBlock.h.
//
// The block is a constant-initialised static object, so its magic exists only in .data's initialised
// bytes, once in the exe. In every desktop build, but written only while features.livesplit is on: until
// then active and self stay 0, which tells the script to ask for the mod. The observer runs after each app
// update (Board::Update runs at most once per update, so it sees every board tick once, before its frame
// is drawn) and only reads: mBoard's fields, ModManager's score key, the board's RulesId and
// gGameConfig.mFeatures. It never calls Rand() (games/bejeweled/tools/check_rng_use.py), writes game
// state, draws, or touches saves, high scores, the registry or replays.
//
// Events, against the previous update (the same board):
//   GO          the board tick passes 150 (Board::Update plays SOUND_GO and shows GO or the instructions),
//               on a board that did not resume a save (LoadGame does not restore mUpdateCnt, so a resumed
//               board shows GO again; it is not counted) and is not a replay (Board::mPlaybackOnly)
//   level clear the board enters STATE_BLACK_HOLE (DoWhirlpool) on an update its tick advanced
//   game over   the board enters STATE_GAME_OVER (GameOver is the only setter); the cause is New Game
//               (mNewGamePending), else time up (from STATE_PLAYING, the only state that checks the time),
//               else no moves (from STATE_FALLING), else other
// A board's first update and the update a LoadGame restores a state in are baselines only.

#include "LiveSplitObserver.h"

#ifdef BEJ_HAS_LIVESPLIT

#include "LiveSplitBlock.h"
#include "WinDMApp.h"
#include "Board.h"
#include "GameConfig.h"
#include "platform/ModManager.h"
#include "platform/VFS.h"

#include <SDL.h>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif
#include <algorithm>
#include <atomic>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <vector>

using namespace Sexy;

// The block (layout 1). Constant-initialised (no constructor runs), aligned, kept by the linker even if
// gc-sections or LTO is ever added. Only Publish() and the header below write it.
extern "C" __attribute__((used, aligned(64))) volatile LiveSplitBlockV1 gLiveSplitBlock = {
	LIVESPLIT_MAGIC_BYTES, LIVESPLIT_GAME_ID, LIVESPLIT_LAYOUT_VERSION, LIVESPLIT_SIZE_V1
};

namespace
{

LiveSplitBlockV1	sShadow;                    // what the next write publishes (the header fields unused)
bool				sActive = false;            // the block says active
bool				sGameOver = false;          // the current board had its game over (flag 0x40)
int					sPrevTick = 0;              // the current board at the previous update
int					sPrevState = -1;
bool				sPrevResumed = false;
std::string			sLogFileName;               // -livesplitlog
FILE*				sLogFile = NULL;

const char* const	gCauseNames[4] = {"no moves", "time up", "New Game", "other"};
bool				sPageAsked = false;         // web: the page's timer asked for the block (byopop_livesplit_block)

// The block is written: the feature is on, or (web) the page's timer asked for it
bool FeatureOn()
{
	return gGameConfig.mFeatures.mLiveSplit || sPageAsked;
}

// One write: seq odd, the fields, seq even (LiveSplitBlock.h)
void Publish(int theUpdate)
{
	volatile LiveSplitBlockV1& aLiveBlock = gLiveSplitBlock;
	const LiveSplitBlockV1& aShadow = sShadow;
	uint32_t aSeq = aLiveBlock.seq + 1;
	aLiveBlock.seq = aSeq;
	std::atomic_thread_fence(std::memory_order_release);
	aLiveBlock.self = aShadow.self;
	aLiveBlock.active = aShadow.active;
	aLiveBlock.updates = aShadow.updates;
	aLiveBlock.flags = aShadow.flags;
	aLiveBlock.boardSerial = aShadow.boardSerial;
	aLiveBlock.goCount = aShadow.goCount;
	aLiveBlock.goSerial = aShadow.goSerial;
	aLiveBlock.levelClears = aShadow.levelClears;
	aLiveBlock.levelClearSerial = aShadow.levelClearSerial;
	aLiveBlock.gameOvers = aShadow.gameOvers;
	aLiveBlock.gameOverSerial = aShadow.gameOverSerial;
	aLiveBlock.deathsNoMoves = aShadow.deathsNoMoves;
	aLiveBlock.deathsTimeUp = aShadow.deathsTimeUp;
	aLiveBlock.deathsNewGame = aShadow.deathsNewGame;
	aLiveBlock.deathsOther = aShadow.deathsOther;
	aLiveBlock.tick = aShadow.tick;
	aLiveBlock.state = aShadow.state;
	aLiveBlock.level = aShadow.level;
	aLiveBlock.displayLevel = aShadow.displayLevel;
	aLiveBlock.displayScore = aShadow.displayScore;
	aLiveBlock.score = aShadow.score;
	aLiveBlock.boardLevelClears = aShadow.boardLevelClears;
	aLiveBlock.goTick = aShadow.goTick;
	aLiveBlock.displayScoreMax = aShadow.displayScoreMax;
	for (int i = 0; i < 32; i++)
		aLiveBlock.rulesId[i] = aShadow.rulesId[i];
	aLiveBlock.qpc = aShadow.qpc;
	aLiveBlock.qpcFrequency = aShadow.qpcFrequency;
	std::atomic_thread_fence(std::memory_order_release);
	aLiveBlock.seq = aSeq + 1;
	sShadow.seq = aSeq + 1;

	if (!sLogFileName.empty())
	{
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
		char aLine[512];
		FormatLiveSplitBlock(sShadow, true, aLine, sizeof(aLine));
		fprintf(sLogFile, "u=%d %s\n", theUpdate, aLine);
	}
}

#ifndef __EMSCRIPTEN__ // the templates: desktop only
bool EndsWithNoCase(const std::string& theText, const char* theEnd)
{
	size_t n = strlen(theEnd);
	if (theText.size() < n)
		return false;
	for (size_t i = 0; i < n; i++)
	{
		if (tolower((unsigned char) theText[theText.size() - n + i]) != tolower((unsigned char) theEnd[i]))
			return false;
	}
	return true;
}

// Written to "<name>.tmp" and renamed only once complete: a copy that fails part way (a full disk, a
// locked file) leaves no truncated template, which CopyTemplates would then keep for good
bool CopyHostFile(const std::string& theFrom, const std::string& theTo)
{
	FILE* anIn = fopen(theFrom.c_str(), "rb");
	if (anIn == NULL)
		return false;
	std::vector<char> aData;
	char aBuffer[4096];
	size_t aCount;
	while ((aCount = fread(aBuffer, 1, sizeof(aBuffer), anIn)) > 0)
		aData.insert(aData.end(), aBuffer, aBuffer + aCount);
	bool isRead = (ferror(anIn) == 0);
	fclose(anIn);
	if (!isRead)
		return false;
	std::string aTempPath = theTo + ".tmp";
	FILE* anOut = fopen(aTempPath.c_str(), "wb");
	if (anOut == NULL)
		return false;
	bool isWritten = aData.empty() || (fwrite(&aData[0], 1, aData.size(), anOut) == aData.size());
	isWritten = (fclose(anOut) == 0) && isWritten;
	if (isWritten && (rename(aTempPath.c_str(), theTo.c_str()) == 0))
		return true;
	remove(aTempPath.c_str());
	return false;
}

// The split templates (*.lss in the livesplit/ folder of the mod that turned the feature on) to
// <user dir>/livesplit/, each only if it is missing there: LiveSplit saves personal bests into the file it
// opened, so a copy is never overwritten
void CopyTemplates()
{
	const std::string& aModId = gGameConfig.mFeatures.mLiveSplitMod;
	const Platform::ModInfo* aMod = NULL;
	for (const Platform::ModInfo* aCandidate : Platform::ModManager::Get().GetEnabledMods())
	{
		if (aCandidate->mId == aModId)
			aMod = aCandidate;
	}
	// The folder, in any case (VFS::FindCaseInsensitive finds files only)
	std::string aFrom;
	if (aMod != NULL)
	{
		for (const std::string& anEntry : Platform::ListDirectory(aMod->mPath))
		{
			if ((SDL_strcasecmp(anEntry.c_str(), "livesplit") == 0) && Platform::IsDirectory(aMod->mPath + "/" + anEntry))
				aFrom = aMod->mPath + "/" + anEntry;
		}
	}
	if (aFrom.empty())
	{
		SDL_Log("livesplit: templates: none in mod %s", aModId.c_str());
		return;
	}
	std::vector<std::string> aNames = Platform::ListDirectory(aFrom);
	std::sort(aNames.begin(), aNames.end());
	std::string aTo = Platform::VFS::Get().GetUserDir() + "/livesplit";
	int aCopied = 0;
	int aKept = 0;
	int aFailed = 0;
	for (const std::string& aName : aNames)
	{
		if (!EndsWithNoCase(aName, ".lss") || !Platform::IsFile(aFrom + "/" + aName))
			continue;
		if (Platform::IsFile(aTo + "/" + aName))
		{
			aKept++;
			continue;
		}
		Platform::MakeDirs(aTo);
		if (CopyHostFile(aFrom + "/" + aName, aTo + "/" + aName))
			aCopied++;
		else
			aFailed++;
	}
	SDL_Log("livesplit: templates: %d copied, %d already there, %d failed: %s", aCopied, aKept, aFailed, aTo.c_str());
}

#endif

// The fields of "no board" (boardSerial 0): before the first board, and after the last one
void ClearBoardFields()
{
	LiveSplitBlockV1& aShadow = sShadow;
	aShadow.boardSerial = 0;
	aShadow.boardLevelClears = 0;
	aShadow.goTick = -1;
	aShadow.displayScoreMax = 0;
	memset(aShadow.rulesId, 0, sizeof(aShadow.rulesId));
	aShadow.flags = 0;
	aShadow.tick = 0;
	aShadow.state = -1;
	aShadow.level = 0;
	aShadow.displayLevel = 0;
	aShadow.displayScore = 0;
	aShadow.score = 0;
}

void Activate()
{
	sActive = true;
	sShadow.self = (uint32_t) (uintptr_t) &gLiveSplitBlock;
	sShadow.active = 1;
	sShadow.qpcFrequency = SDL_GetPerformanceFrequency();
	SDL_Log("livesplit: block at 0x%08x (layout %u, %u bytes)", (unsigned) sShadow.self, (unsigned) LIVESPLIT_LAYOUT_VERSION,
		(unsigned) LIVESPLIT_SIZE_V1);
#ifndef __EMSCRIPTEN__
	CopyTemplates(); // the web page ships its own templates (web/autosplit.js)
#endif
}

} // namespace

bool LiveSplitObserver::Wanted()
{
	return FeatureOn() || sActive;
}

void LiveSplitObserver::SetLogFile(const std::string& theFileName)
{
	sLogFileName = theFileName;
}

void LiveSplitObserver::AfterUpdate(WinDMApp* theApp)
{
	LiveSplitBlockV1& aShadow = sShadow;
	if (!FeatureOn())
	{
		// NewGame's gGameConfig.Load() found the feature off: the script then asks for the mod
		if (sActive)
		{
			sActive = false;
			aShadow.active = 0;
			aShadow.updates++;
			aShadow.qpc = SDL_GetPerformanceCounter();
			Publish(theApp->mUpdateCount);
			SDL_Log("livesplit: off");
		}
		return;
	}
	// On activation the current board (or none) is new to the block: its first update is a baseline
	bool isActivation = !sActive;
	if (isActivation)
		Activate();

	Board* aBoard = theApp->mBoard;
	uint32_t aSerial = (aBoard != NULL) ? aBoard->mBoardSerial : 0;
	bool isNewBoard = isActivation || (aSerial != aShadow.boardSerial);
	if (isNewBoard)
	{
		aShadow.boardSerial = aSerial;
		aShadow.boardLevelClears = 0;
		aShadow.goTick = -1;
		aShadow.displayScoreMax = 0;
		memset(aShadow.rulesId, 0, sizeof(aShadow.rulesId));
		if (aBoard != NULL)
			aBoard->mRules.RulesId(aShadow.rulesId);
		sGameOver = false;
	}

	if (aBoard == NULL)
		ClearBoardFields();
	else
	{
		int aTick = aBoard->mUpdateCnt;
		int aState = aBoard->mState;
		// A board a replay drives (WatchController's playback board): never a run start, flag 0x20
		bool isReplay = aBoard->mPlaybackOnly;
		// A LoadGame in this update (Board::GameLoadedVariant): it restored a state (a vortex too), not
		// played into it
		bool isLoad = aBoard->mResumed && !sPrevResumed;
		bool isOverNow = false;
		if (!isNewBoard && !isLoad)
		{
			if ((sPrevTick < 150) && (aTick >= 150))
			{
				if (aBoard->mResumed)
					SDL_Log("livesplit: board %u resumed a saved game: GO at tick %d not counted", (unsigned) aSerial, aTick);
				else if (!isReplay)
				{
					aShadow.goCount++;
					aShadow.goSerial = aSerial;
					aShadow.goTick = aTick;
					SDL_Log("livesplit: board %u GO at tick %d (%s)", (unsigned) aSerial, aTick, aBoard->mTimedMode ? "Timetrial" : "Normal");
				}
			}
			if ((aState == STATE_BLACK_HOLE) && (sPrevState != STATE_BLACK_HOLE) && (aTick != sPrevTick))
			{
				aShadow.levelClears++;
				aShadow.levelClearSerial = aSerial;
				aShadow.boardLevelClears++;
				SDL_Log("livesplit: board %u level clear %d (level %d) at tick %d", (unsigned) aSerial, (int) aShadow.boardLevelClears,
					aBoard->mDisplayLevel, aTick);
			}
			if ((aState == STATE_GAME_OVER) && (sPrevState != STATE_GAME_OVER))
			{
				int aCause;
				if (aBoard->mNewGamePending)
				{
					aCause = 2;
					aShadow.deathsNewGame++;
				}
				else if (sPrevState == STATE_PLAYING)
				{
					aCause = 1;
					aShadow.deathsTimeUp++;
				}
				else if (sPrevState == STATE_FALLING)
				{
					aCause = 0;
					aShadow.deathsNoMoves++;
				}
				else
				{
					aCause = 3;
					aShadow.deathsOther++;
				}
				aShadow.gameOvers++;
				aShadow.gameOverSerial = aSerial;
				isOverNow = true;
				SDL_Log("livesplit: board %u game over (%s) at tick %d", (unsigned) aSerial, gCauseNames[aCause], aTick);
			}
		}

		// The HUD score can roll back down (a hint), so the splitter reads the high-water mark; frozen after
		// the game-over update, although the HUD may still roll during the shake
		if (!sGameOver)
			aShadow.displayScoreMax = std::max(aShadow.displayScoreMax, (int32_t) aBoard->mDisplayScore);
		if (isOverNow)
			sGameOver = true;

		uint32_t aFlags = 0;
		if (aBoard->mTimedMode)
			aFlags |= LIVESPLIT_FLAG_TIMED;
		if (aBoard->mResumed)
			aFlags |= LIVESPLIT_FLAG_RESUMED;
		if (Platform::ModManager::Get().GetScoreKey().empty() && !aBoard->mUserSeeded)
			aFlags |= LIVESPLIT_FLAG_VANILLA;
		if (aBoard->mUserSeeded)
			aFlags |= LIVESPLIT_FLAG_CHOSEN_SEED;
		if (aBoard->mOwnRng != NULL)
			aFlags |= LIVESPLIT_FLAG_OWN_RNG;
		if (isReplay)
			aFlags |= LIVESPLIT_FLAG_REPLAY;
		if (sGameOver)
			aFlags |= LIVESPLIT_FLAG_GAME_OVER;
		if ((aBoard->mPauseCount > 0) || (aBoard->mUserPauseCount > 0))
			aFlags |= LIVESPLIT_FLAG_PAUSED;
		aShadow.flags = aFlags;
		aShadow.tick = aTick;
		aShadow.state = aState;
		aShadow.level = aBoard->mLevel;
		aShadow.displayLevel = aBoard->mDisplayLevel;
		aShadow.displayScore = aBoard->mDisplayScore;
		aShadow.score = aBoard->mScore;

		sPrevTick = aTick;
		sPrevState = aState;
		sPrevResumed = aBoard->mResumed;
	}

	aShadow.updates++;
	aShadow.qpc = SDL_GetPerformanceCounter();
	Publish(theApp->mUpdateCount);
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

void LiveSplitObserver::AppDestroyed(WinDMApp* theApp)
{
	if (sActive)
	{
		sActive = false;
		sShadow.active = 0;
		ClearBoardFields();
		sShadow.updates++;
		sShadow.qpc = SDL_GetPerformanceCounter();
		Publish(theApp->mUpdateCount);
	}
	if (sLogFile != NULL)
	{
		fclose(sLogFile);
		sLogFile = NULL;
	}
}

#endif
