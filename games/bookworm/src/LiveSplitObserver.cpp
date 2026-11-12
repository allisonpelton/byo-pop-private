// port(mod): the LiveSplit block and its observer. See LiveSplitObserver.h and LiveSplitBlock.h.
//
// The block is the shared AutosplitBlock (platform/AutosplitBlock.h, layout 2), a constant-initialised static object, so its
// magic exists only in .data's initialised bytes, once in the exe, and the AutosplitWriter keeps its counters, serials and the
// seq protocol. It is in every Windows and web build but written only while the "livesplit" feature is on or, on the web, the
// page asked for it (byopop_livesplit_block); until then active and self stay 0, which tells the script to ask for the mod. The observer runs after each app
// update (BookWormApp::UpdateFrames), reads the board's fields and the score key only, and sees each update's
// end state, not what happened inside it.
//
// BookWorm keeps a board after the player leaves it: BookWormApp::DoGameSelector abandons it (BOARD_INACTIVE)
// and takes it off the widget manager, but BookWormApp::mBoard stays until the selector's roll-down is done
// (KillBoard). A saved game that is continued is a new Board that LoadGame fills in the same call that
// makes it (BookWormApp::ButtonDepress, DIALOG_CONTINUE), so the observer never sees it before mResumed is set.
//
// Events, for the current board (Board::mBoardSerial):
//   GO          the first update of a fresh board (not resumed, not first seen at activation) that is
//               BOARD_PLAYING and not paused (the help screen and dialogs pause it), at level 1 with no word
//               or scramble yet. Once per board; a board that is already further when it first plays is
//               given no GO
//   level clear the board enters BOARD_LEVEL_COMPLETE (Board::Submit: the score reaches the level's threshold
//               while the board is playing)
//   game over   the board enters BOARD_GAMEOVER_PENDING (BurnTiles: a fire tile burned through the bottom;
//               Classic: after a word, Action: when the countdown ran out), or BOARD_INACTIVE (LEAVE GAME)
//               before its game over. The game-over states that follow (BOARD_GAMEOVER, BOARD_GAMEOVER_MUSIC)
//               are the same game over
// A board's first update is a baseline for the level clear and the game over (a board that is already over
// or left when first seen has had its game over).

#include "LiveSplitObserver.h"

#ifdef BOOKWORM_HAS_LIVESPLIT

#include "LiveSplitBlock.h"
#include "BookWormApp.h"
#include "Board.h"
#include "StatKeeper.h"
#include "ModFeatures.h"
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

// The block (the shared layout 2, platform/AutosplitBlock.h): constant-initialised, aligned, kept by the linker. Only the
// writer's Publish() writes it. LiveSplitBlock.h is the layout-1 view of its first 0xB0 bytes.
extern "C" __attribute__((used, aligned(64))) volatile AutosplitBlock gLiveSplitBlock = AUTOSPLIT_BLOCK_INIT(LIVESPLIT_GAME_ID);

namespace
{

AutosplitWriter		sWriter(&gLiveSplitBlock);   // the counters, serials, latched flags and the seq writes
bool				sGoPossible = false;        // the current board may still have its GO
int					sPrevState = -1;            // the current board's state at the previous update
bool				sPageAsked = false;         // web: the page's timer asked for the block (byopop_livesplit_block)
std::string			sLogFileName;               // -livesplitlog
FILE*				sLogFile = NULL;

const char* const	gCauseNames[4] = { "fire", "countdown", "left", "other" };

// BookWorm's mode and skill, which the layout-1 view has at 0x80 and 0x84 (the shared block's gameOwn[0..3] and [4..7]) and
// the layout-2 fields repeat at 0xB8 and 0xBC
void SetModeAndSkill(int theMode, int theSkill)
{
	AutosplitBlock& aNext = sWriter.Next();
	memcpy(aNext.gameOwn + 0, &theMode, sizeof(theMode));
	memcpy(aNext.gameOwn + 4, &theSkill, sizeof(theSkill));
	aNext.mode = theMode;
	aNext.skill = theSkill;
}

// One write (the writer's seq protocol, LiveSplitBlock.h), then -livesplitlog's line
void Publish(int theUpdate)
{
	sWriter.Publish(SDL_GetPerformanceCounter());

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
		// The fields (volatile ones masked), then the block's bytes as the script reads them
		char aLine[600];
		LiveSplitBlockV1 aFields;
		memcpy(&aFields, &sWriter.Next(), sizeof(aFields));
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
}

#ifndef __EMSCRIPTEN__    // the split templates: desktop only (the web page ships its own, web/autosplit.js)

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

#endif

void Activate()
{
	sWriter.Activate(SDL_GetPerformanceFrequency());
	AutosplitBlock& aNext = sWriter.Next();
	memset(aNext.gameOwn + 8, 0, 8);
	const std::string& aKey = Platform::ModManager::Get().GetScoreKey();
	for (size_t i = 0; (i < aKey.size()) && (i < 8); i++)
		aNext.gameOwn[8 + i] = (uint8_t) aKey[i];
	SetModeAndSkill(-1, 0);
	SDL_Log("livesplit: block at 0x%08x (layout %u, %u bytes)", (unsigned) aNext.self, (unsigned) AUTOSPLIT_LAYOUT_VERSION,
		(unsigned) AUTOSPLIT_SIZE_V2);
#ifndef __EMSCRIPTEN__
	CopyTemplates();
#endif
}

} // namespace

void LiveSplitObserver::SetLogFile(const std::string& theFileName)
{
	sLogFileName = theFileName;
}

void LiveSplitObserver::AfterUpdate(BookWormApp* theApp)
{
	if ((!sWriter.IsActive()) && (!ModFeatureEnabled("livesplit")) && (!sPageAsked))
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
		const StatKeeper* aStats = aBoard->mGameStats;
		bool isOverState = (aState == BOARD_GAMEOVER_PENDING) || (aState == BOARD_GAMEOVER_MUSIC) || (aState == BOARD_GAMEOVER);
		bool isLeft = (aState == BOARD_INACTIVE);
		int aGameOverKind = -1;     // the kind of game over this update raised (gCauseNames), -1 for none
		if (isNewBoard)
		{
			// The baseline: a board seen over or left has had its game over, and its state is the previous one
			if (isOverState || isLeft)
				sWriter.SeenOver();
			sPrevState = aState;
			sGoPossible = (!isActivation) && (!aBoard->mResumed) && (aBoard->mPlayer == NULL);
			if ((!isActivation) && (aBoard->mResumed))
				SDL_Log("livesplit: board %u resumed a saved game: GO not counted", (unsigned) aSerial);
			if ((!isActivation) && (aBoard->mPlayer != NULL))
				SDL_Log("livesplit: board %u plays a replay: GO not counted", (unsigned) aSerial);
		}

		if (sGoPossible)
		{
			if ((aBoard->mLevel != 0) || (aStats->mWords != 0) || (aStats->mScrambles != 0) || isOverState || isLeft)
				sGoPossible = false;    // the start went by unseen: no GO for this board
			else if ((aState == BOARD_PLAYING) && (aBoard->mPauseCount == 0))
			{
				sGoPossible = false;
				sWriter.Go(aBoard->mUpdateCnt);
				SDL_Log("livesplit: board %u GO at tick %d (mode %d)", (unsigned) aSerial, aBoard->mUpdateCnt, theApp->mGameMode);
			}
		}

		if (!isNewBoard)
		{
			if ((aState == BOARD_LEVEL_COMPLETE) && (sPrevState != BOARD_LEVEL_COMPLETE))
			{
				sWriter.LevelClear();
				SDL_Log("livesplit: board %u level clear %d (level %d) at tick %d", (unsigned) aSerial, (int) aNext.boardLevelClears,
					aBoard->mLevel + 1, aBoard->mUpdateCnt);
			}
			if ((!sWriter.HadGameOver()) && (isOverState || isLeft))
			{
				if (isLeft)
					aGameOverKind = 2;
				else if (theApp->mGameMode == GAMEMODE_CLASSIC)
					aGameOverKind = 0;
				else if (theApp->mGameMode == GAMEMODE_ACTION)
					aGameOverKind = 1;
				else
					aGameOverKind = 3;
				SDL_Log("livesplit: board %u game over (%s) at tick %d", (unsigned) aSerial, gCauseNames[aGameOverKind], aBoard->mUpdateCnt);
			}
		}

		// The score the splitter reads is the highest the game's score reached, frozen at the game over
		// (LiveSplitBlock.h says why it is not the HUD's). The game-over update's own score still counts, so
		// the high-water mark is raised before the game over latches.
		sWriter.RaiseScoreMax(aStats->mScore);
		if (aGameOverKind >= 0)
			sWriter.GameOver(aGameOverKind);

		uint32_t aFlags = 0;
		if (aBoard->mResumed)
			aFlags |= LIVESPLIT_FLAG_RESUMED;
		if (aBoard->mPlayer != NULL)
			aFlags |= LIVESPLIT_FLAG_REPLAY;    // port(mod): a watched replay (Replay.h)
		if (Platform::ModManager::Get().GetScoreKey().empty())
			aFlags |= LIVESPLIT_FLAG_VANILLA;
		if (aBoard->mPauseCount > 0)
			aFlags |= LIVESPLIT_FLAG_PAUSED;
		aNext.flags = aFlags;               // the writer adds the game over's flag from its latch
		aNext.tick = aBoard->mUpdateCnt;
		aNext.state = aState;
		aNext.level = aBoard->mLevel;
		aNext.displayLevel = aBoard->mLevel + 1;
		aNext.displayScore = aBoard->mShownScore;
		aNext.score = aStats->mScore;
		SetModeAndSkill(theApp->mGameMode, 0);
		// The rank the summary screen shows (LevelUpWidget: the level after a level's summary, the level reached at a game
		// over), which a category like Bookworm Supreme ends on: the levels cleared. Board::mLevel counts up only when the
		// next level starts, so a level just cleared (the summary, the continue button's NEXT_LEVEL) is one more.
		int aRank = aBoard->mLevel + (((aState == BOARD_LEVEL_COMPLETE) || (aState == BOARD_NEXT_LEVEL)) ? 1 : 0);
		aNext.progress = std::min(aRank, 31);

		sPrevState = aState;
	}

	Publish(theApp->mUpdateCount);
}

void LiveSplitObserver::AppDestroyed(BookWormApp* theApp)
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
