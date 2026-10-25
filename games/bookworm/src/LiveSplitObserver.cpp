// port(mod): the LiveSplit block and its observer. See LiveSplitObserver.h and LiveSplitBlock.h.
//
// The block is a constant-initialised static object, so its magic exists only in .data's initialised bytes,
// once in the exe. It is in every Windows build but written only while the "livesplit" feature is on; until
// then active and self stay 0, which tells the script to ask for the mod. The observer runs after each app
// update (BookWormApp::UpdateFrames), reads the board's fields and the score key only, and sees each update's
// end state, not what happened inside it.
//
// BookWorm keeps a board after the player leaves it: BookWormApp::ShowGameSelector abandons it (BOARD_INACTIVE)
// and takes it off the widget manager, but BookWormApp::mBoard stays until the selector's roll-down is done
// (RemoveBoard). A saved game that is continued is a new Board that LoadGame fills in the same call that
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
#include <algorithm>
#include <atomic>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <vector>

using namespace Sexy;

// The block (layout 1): constant-initialised, aligned, kept by the linker. Only Publish() writes it.
extern "C" __attribute__((used, aligned(64))) volatile LiveSplitBlockV1 gLiveSplitBlock = {
	LIVESPLIT_MAGIC_BYTES, LIVESPLIT_GAME_ID, LIVESPLIT_LAYOUT_VERSION, LIVESPLIT_SIZE_V1
};

namespace
{

LiveSplitBlockV1	sShadow;                    // what the next write publishes (the header fields unused)
bool				sActive = false;            // the block says active
bool				sGameOver = false;          // the current board had its game over (flag 0x40)
bool				sGoPossible = false;        // the current board may still have its GO
int					sPrevState = -1;            // the current board's state at the previous update
std::string			sLogFileName;               // -livesplitlog
FILE*				sLogFile = NULL;

const char* const	gCauseNames[4] = { "fire", "countdown", "left", "other" };

// One write: seq odd, the fields, seq even (LiveSplitBlock.h)
void Publish(int theUpdate)
{
	volatile LiveSplitBlockV1& b = gLiveSplitBlock;
	const LiveSplitBlockV1& s = sShadow;
	uint32_t aSeq = b.seq + 1;
	b.seq = aSeq;
	std::atomic_thread_fence(std::memory_order_release);
	b.self = s.self;
	b.active = s.active;
	b.updates = s.updates;
	b.flags = s.flags;
	b.boardSerial = s.boardSerial;
	b.goCount = s.goCount;
	b.goSerial = s.goSerial;
	b.levelClears = s.levelClears;
	b.levelClearSerial = s.levelClearSerial;
	b.gameOvers = s.gameOvers;
	b.gameOverSerial = s.gameOverSerial;
	b.deathsFire = s.deathsFire;
	b.deathsCountdown = s.deathsCountdown;
	b.deathsNewGame = s.deathsNewGame;
	b.deathsOther = s.deathsOther;
	b.tick = s.tick;
	b.state = s.state;
	b.level = s.level;
	b.displayLevel = s.displayLevel;
	b.displayScore = s.displayScore;
	b.score = s.score;
	b.boardLevelClears = s.boardLevelClears;
	b.goTick = s.goTick;
	b.scoreMax = s.scoreMax;
	b.mode = s.mode;
	b.skill = s.skill;
	for (int i = 0; i < 8; i++)
		b.scoreKey[i] = s.scoreKey[i];
	b.qpc = s.qpc;
	b.qpcFrequency = s.qpcFrequency;
	std::atomic_thread_fence(std::memory_order_release);
	b.seq = aSeq + 1;
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
		// The fields (volatile ones masked), then the block's bytes as the script reads them
		char aLine[600];
		FormatLiveSplitBlock(sShadow, true, aLine, sizeof(aLine));
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

// The fields of "no board" (boardSerial 0): before the first board, and after the last one
void ClearBoardFields()
{
	LiveSplitBlockV1& s = sShadow;
	s.boardSerial = 0;
	s.boardLevelClears = 0;
	s.goTick = -1;
	s.scoreMax = 0;
	s.flags = 0;
	s.tick = 0;
	s.state = -1;
	s.level = 0;
	s.displayLevel = 0;
	s.displayScore = 0;
	s.score = 0;
	s.mode = -1;
	s.skill = 0;
}

void Activate()
{
	sActive = true;
	sShadow.self = (uint32_t)(uintptr_t) &gLiveSplitBlock;
	sShadow.active = 1;
	sShadow.qpcFrequency = SDL_GetPerformanceFrequency();
	memset(sShadow.scoreKey, 0, sizeof(sShadow.scoreKey));
	const std::string& aKey = Platform::ModManager::Get().GetScoreKey();
	for (size_t i = 0; (i < aKey.size()) && (i < 8); i++)
		sShadow.scoreKey[i] = (uint8_t) aKey[i];
	SDL_Log("livesplit: block at 0x%08x (layout %u, %u bytes)", (unsigned) sShadow.self, (unsigned) LIVESPLIT_LAYOUT_VERSION,
		(unsigned) LIVESPLIT_SIZE_V1);
	CopyTemplates();
}

} // namespace

void LiveSplitObserver::SetLogFile(const std::string& theFileName)
{
	sLogFileName = theFileName;
}

void LiveSplitObserver::AfterUpdate(BookWormApp* theApp)
{
	if ((!sActive) && (!ModFeatureEnabled("livesplit")))
		return;
	LiveSplitBlockV1& s = sShadow;
	// On activation the current board (or none) is new to the block: it is never given a GO
	bool isActivation = !sActive;
	if (isActivation)
		Activate();

	Board* aBoard = theApp->mBoard;
	uint32_t aSerial = (aBoard != NULL) ? aBoard->mBoardSerial : 0;
	bool isNewBoard = isActivation || (aSerial != s.boardSerial);
	if (isNewBoard)
	{
		s.boardSerial = aSerial;
		s.boardLevelClears = 0;
		s.goTick = -1;
		s.scoreMax = 0;
		sGameOver = false;
		sGoPossible = false;
		sPrevState = -1;
	}

	if (aBoard == NULL)
		ClearBoardFields();
	else
	{
		int aState = aBoard->mState;
		const StatKeeper* aStats = aBoard->mGameStats;
		bool isOverState = (aState == BOARD_GAMEOVER_PENDING) || (aState == BOARD_GAMEOVER_MUSIC) || (aState == BOARD_GAMEOVER);
		bool isLeft = (aState == BOARD_INACTIVE);
		bool isOverNow = false;
		if (isNewBoard)
		{
			// The baseline: a board seen over or left has had its game over, and its state is the previous one
			sGameOver = isOverState || isLeft;
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
				s.goCount++;
				s.goSerial = aSerial;
				s.goTick = aBoard->mUpdateCnt;
				SDL_Log("livesplit: board %u GO at tick %d (mode %d)", (unsigned) aSerial, aBoard->mUpdateCnt, theApp->mGameMode);
			}
		}

		if (!isNewBoard)
		{
			if ((aState == BOARD_LEVEL_COMPLETE) && (sPrevState != BOARD_LEVEL_COMPLETE))
			{
				s.levelClears++;
				s.levelClearSerial = aSerial;
				s.boardLevelClears++;
				SDL_Log("livesplit: board %u level clear %d (level %d) at tick %d", (unsigned) aSerial, (int) s.boardLevelClears,
					aBoard->mLevel + 1, aBoard->mUpdateCnt);
			}
			if ((!sGameOver) && (isOverState || isLeft))
			{
				int aCause;
				if (isLeft)
				{
					aCause = 2;
					s.deathsNewGame++;
				}
				else if (theApp->mGameMode == GAMEMODE_CLASSIC)
				{
					aCause = 0;
					s.deathsFire++;
				}
				else if (theApp->mGameMode == GAMEMODE_ACTION)
				{
					aCause = 1;
					s.deathsCountdown++;
				}
				else
				{
					aCause = 3;
					s.deathsOther++;
				}
				s.gameOvers++;
				s.gameOverSerial = aSerial;
				isOverNow = true;
				SDL_Log("livesplit: board %u game over (%s) at tick %d", (unsigned) aSerial, gCauseNames[aCause], aBoard->mUpdateCnt);
			}
		}

		// The score the splitter reads is the highest the game's score reached, frozen at the game over
		// (LiveSplitBlock.h says why it is not the HUD's)
		if (!sGameOver)
			s.scoreMax = std::max(s.scoreMax, (int32_t) aStats->mScore);
		if (isOverNow)
			sGameOver = true;

		uint32_t aFlags = 0;
		if (aBoard->mResumed)
			aFlags |= LIVESPLIT_FLAG_RESUMED;
		if (aBoard->mPlayer != NULL)
			aFlags |= LIVESPLIT_FLAG_REPLAY;    // port(mod): a watched replay (Replay.h)
		if (Platform::ModManager::Get().GetScoreKey().empty())
			aFlags |= LIVESPLIT_FLAG_VANILLA;
		if (sGameOver)
			aFlags |= LIVESPLIT_FLAG_GAME_OVER;
		if (aBoard->mPauseCount > 0)
			aFlags |= LIVESPLIT_FLAG_PAUSED;
		s.flags = aFlags;
		s.tick = aBoard->mUpdateCnt;
		s.state = aState;
		s.level = aBoard->mLevel;
		s.displayLevel = aBoard->mLevel + 1;
		s.displayScore = aBoard->mShownScore;
		s.score = aStats->mScore;
		s.mode = theApp->mGameMode;
		s.skill = 0;

		sPrevState = aState;
	}

	s.updates++;
	s.qpc = SDL_GetPerformanceCounter();
	Publish(theApp->mUpdateCount);
}

void LiveSplitObserver::AppDestroyed(BookWormApp* theApp)
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
