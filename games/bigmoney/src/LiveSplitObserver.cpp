// port(mods): the LiveSplit block and its observer. See LiveSplitObserver.h and LiveSplitBlock.h.
//
// The block is the shared autosplit block, layout 2 (platform/AutosplitBlock.h; docs/WEB_PLAYER.md, "One design
// for every port"), written through its shared writer; its first 0xB0 bytes are layout 1 as LiveSplitBlock.h
// describes them. It is a constant-initialised static object, so its magic exists only in .data's initialised
// bytes, once in the exe. It is in every Windows and web build but written only while the "livesplit" feature is
// on or the web page's timer asked for it (byopop_livesplit_block); until then active and self stay 0, which
// tells the script to ask for the mod. The observer runs after each app
// update. Board::Update runs once per update (a replay at 2x-8x runs more board ticks per update, and is
// flagged), so the observer sees each board tick once. It reads the board's fields and the score key only.
//
// Events, against the previous update of the same board:
//   GO          Board::mGoTimer reaches 0 at level 0 (the "Go!" sound), on a board that did not resume a save
//               and is not a replay
//   level clear the board enters STATE_LEVEL_COMPLETE (LevelComplete)
//   game over   the board enters STATE_GAME_OVER (GameOver): no moves in Puzzle (no group left), else
//               overflow (Action's countdown, Strategy's full board); or STATE_FROZEN before its game over
//               (New Game: the game selector slides over the board and then removes it)
// A board's first update is a baseline only.

#include "LiveSplitObserver.h"

#ifdef BIGMONEY_HAS_LIVESPLIT

#include "LiveSplitBlock.h"
#include "WinBMApp.h"
#include "Board.h"
#include "ModRules.h"
#include "platform/ModManager.h"
#include "platform/VFS.h"

#include <SDL.h>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <vector>

using namespace Sexy;

// The block (layout 2): constant-initialised, aligned, kept by the linker. Only the writer writes it.
extern "C" __attribute__((used, aligned(64))) volatile AutosplitBlock gLiveSplitBlock = AUTOSPLIT_BLOCK_INIT(LIVESPLIT_GAME_ID);

namespace
{

AutosplitWriter		sWriter(&gLiveSplitBlock);  // what the next write publishes, and the writes
bool				sPageAsked = false;         // web: the page's timer asked for the block (byopop_livesplit_block)
int					sPrevGoTimer = 0;           // the current board at the previous update
int					sPrevState = -1;
std::string			sLogFileName;               // -livesplitlog
FILE*				sLogFile = NULL;

// The four kinds of game over, in the block's deaths[] order (layout 1's deathsOverflow .. deathsOther)
enum { DEATH_OVERFLOW = 0, DEATH_NO_MOVES = 1, DEATH_NEW_GAME = 2, DEATH_OTHER = 3 };
const char* const	gCauseNames[4] = { "overflow", "no moves", "New Game", "other" };

// The mode and skill: layout 2's own fields, and Big Money's own bytes at 0x80 (layout 1's mode and skill)
void SetModeAndSkill(int32_t theMode, int32_t theSkill)
{
	AutosplitBlock& s = sWriter.Next();
	memcpy(s.gameOwn + 0, &theMode, 4);
	memcpy(s.gameOwn + 4, &theSkill, 4);
	s.mode = theMode;
	s.skill = theSkill;
}

// One write (the writer's seq protocol), then the log line in layout 1's format, as the tools compare it
void Publish(int theUpdate)
{
	sWriter.Publish(SDL_GetPerformanceCounter());
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
	// The fields (volatile ones masked), then the block's first 0xB0 bytes as the script reads them
	LiveSplitBlockV1 aFields;
	memcpy(&aFields, &sWriter.Next(), sizeof(aFields));
	char aLine[600];
	FormatLiveSplitBlock(aFields, true, aLine, sizeof(aLine));
	LiveSplitBlockV1 aCopy;
	memcpy(&aCopy, (const void*) &gLiveSplitBlock, sizeof(aCopy));
	char aHex[LIVESPLIT_SIZE_V1 * 2 + 1];
	const uint8_t* aBytes = (const uint8_t*) &aCopy;
	for (unsigned i = 0; i < LIVESPLIT_SIZE_V1; i++)
		snprintf(aHex + 2 * i, 3, "%02x", aBytes[i]);
	fprintf(sLogFile, "u=%d %s hex=%s\n", theUpdate, aLine, aHex);
	fflush(sLogFile);
}

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

// Written to "<name>.tmp" and renamed once complete, so a failed copy leaves no truncated template
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
	std::string aTemp = theTo + ".tmp";
	FILE* anOut = fopen(aTemp.c_str(), "wb");
	if (anOut == NULL)
		return false;
	bool isWritten = aData.empty() || (fwrite(&aData[0], 1, aData.size(), anOut) == aData.size());
	isWritten = (fclose(anOut) == 0) && isWritten;
	if (isWritten && (rename(aTemp.c_str(), theTo.c_str()) == 0))
		return true;
	remove(aTemp.c_str());
	return false;
}

// The split templates (*.lss in the livesplit/ folder of the mod that turned the feature on) to
// <user dir>/livesplit/, each only if it is missing there: LiveSplit saves personal bests into the file it
// opened, so a copy is never overwritten
void CopyTemplates()
{
	const Platform::ModInfo* aMod = NULL;
	for (const Platform::ModInfo* aCandidate : Platform::ModManager::Get().GetEnabledMods())
	{
		const Platform::JsonValue& aFeatures = aCandidate->mFeaturesJson;
		if ((aMod == NULL) && aFeatures.IsObject() && aFeatures.Has("livesplit"))
			aMod = aCandidate;
	}
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
		SDL_Log("livesplit: templates: none in the mod");
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

void Activate()
{
	sWriter.Activate(SDL_GetPerformanceFrequency());
	AutosplitBlock& s = sWriter.Next();
	memset(s.gameOwn + 8, 0, 8);
	const std::string& aKey = Platform::ModManager::Get().GetScoreKey();
	for (size_t i = 0; (i < aKey.size()) && (i < 8); i++)
		s.gameOwn[8 + i] = (uint8_t) aKey[i];
	SetModeAndSkill(-1, 0);
	SDL_Log("livesplit: block at 0x%08x (layout %u, %u bytes)", (unsigned) s.self, (unsigned) AUTOSPLIT_LAYOUT_VERSION,
		(unsigned) AUTOSPLIT_SIZE_V2);
#ifndef __EMSCRIPTEN__
	CopyTemplates(); // the web page has its own templates (web/autosplit.js)
#endif
}

} // namespace

void LiveSplitObserver::SetLogFile(const std::string& theFileName)
{
	sLogFileName = theFileName;
}

void LiveSplitObserver::AfterUpdate(WinBMApp* theApp)
{
	if ((!sWriter.IsActive()) && (!ModFeatureEnabled("livesplit")) && (!sPageAsked))
		return;
	AutosplitBlock& s = sWriter.Next();
	// On activation the current board (or none) is new to the block: its first update is a baseline
	bool isActivation = !sWriter.IsActive();
	if (isActivation)
		Activate();

	Board* aBoard = theApp->mBoard;
	uint32_t aSerial = (aBoard != NULL) ? aBoard->mBoardSerial : 0;
	bool isNewBoard = isActivation || (aSerial != s.boardSerial);
	if (isNewBoard)
		sWriter.NewBoard(aSerial);

	if (aBoard == NULL)
	{
		sWriter.ClearBoard();
		SetModeAndSkill(-1, 0);
	}
	else
	{
		int aState = aBoard->mState;
		bool isReplay = (aBoard->mPlayer != NULL);
		int aGameOver = -1; // this update's kind of game over, if it had one
		if (!isNewBoard)
		{
			if ((sPrevGoTimer > 0) && (aBoard->mGoTimer == 0) && (aBoard->mLevel == 0))
			{
				if (aBoard->mResumed)
					SDL_Log("livesplit: board %u resumed a saved game: GO not counted", (unsigned) aSerial);
				else if (!isReplay)
				{
					sWriter.Go(aBoard->mUpdateCnt);
					SDL_Log("livesplit: board %u GO at tick %d (mode %d, skill %d)", (unsigned) aSerial, aBoard->mUpdateCnt,
						theApp->mGameType, theApp->mSkillLevel);
				}
			}
			if ((aState == STATE_LEVEL_COMPLETE) && (sPrevState != STATE_LEVEL_COMPLETE))
			{
				sWriter.LevelClear();
				SDL_Log("livesplit: board %u level clear %d (level %d) at tick %d", (unsigned) aSerial, (int) s.boardLevelClears,
					aBoard->mLevel + 1, aBoard->mUpdateCnt);
			}
			bool isGameOver = (aState == STATE_GAME_OVER) && (sPrevState != STATE_GAME_OVER);
			bool isNewGame = (aState == STATE_FROZEN) && (sPrevState != STATE_FROZEN) && (!sWriter.HadGameOver());
			if (isNewGame)
				aGameOver = DEATH_NEW_GAME;
			else if (isGameOver && (theApp->mGameType == GAMETYPE_PUZZLE) && (!aBoard->mHasMoves))
				aGameOver = DEATH_NO_MOVES;
			else if (isGameOver && (theApp->mGameType != GAMETYPE_PUZZLE))
				aGameOver = DEATH_OVERFLOW;
			else if (isGameOver)
				aGameOver = DEATH_OTHER;
		}

		// The HUD score rolls up to mScore; the splitter reads its high-water mark, frozen after the game over (the
		// game over's own update still counts, so the game over is recorded after it)
		sWriter.SetDisplayScore(aBoard->mDisplayScore);
		if (aGameOver >= 0)
		{
			sWriter.GameOver(aGameOver);
			SDL_Log("livesplit: board %u game over (%s) at tick %d", (unsigned) aSerial, gCauseNames[aGameOver], aBoard->mUpdateCnt);
		}

		uint32_t aFlags = 0;
		if (aBoard->mResumed)
			aFlags |= AUTOSPLIT_FLAG_RESUMED;
		if (Platform::ModManager::Get().GetScoreKey().empty() && !aBoard->mUserSeeded)
			aFlags |= AUTOSPLIT_FLAG_VANILLA;
		if (aBoard->mUserSeeded)
			aFlags |= AUTOSPLIT_FLAG_CHOSEN_SEED;
		if (isReplay)
			aFlags |= AUTOSPLIT_FLAG_REPLAY;
		if (aBoard->mPauseCount > 0)
			aFlags |= AUTOSPLIT_FLAG_PAUSED;
		s.flags = aFlags; // the writer adds AUTOSPLIT_FLAG_GAME_OVER
		s.tick = aBoard->mUpdateCnt;
		s.state = aState;
		s.level = aBoard->mLevel;
		s.displayLevel = aBoard->mLevel + 1;
		s.score = aBoard->mScore;
		SetModeAndSkill(theApp->mGameType, theApp->mSkillLevel);

		sPrevGoTimer = aBoard->mGoTimer;
		sPrevState = aState;
	}

	Publish(theApp->mUpdateCount);
}

#ifdef __EMSCRIPTEN__
// The page's speedrun timer: the block's address in the game's memory (a HEAPU8 offset). Asking turns its writing
// on from the next update, as features.livesplit does (web/autosplit.js).
extern "C" EMSCRIPTEN_KEEPALIVE uint32_t byopop_livesplit_block()
{
	sPageAsked = true;
	return (uint32_t) (uintptr_t) &gLiveSplitBlock;
}
#endif

void LiveSplitObserver::AppDestroyed(WinBMApp* theApp)
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

#endif
