// port(mod): the shared autosplit block for Bookworm Adventures (see LiveSplitObserver.h).
//
// The game's rules are in its scripts, so the observer reads what they leave on the C++ side: the player's profile
// (Profile, the values the scripts set), the game screen (GameApp::mBookManager, the scripts' gBookManager), the board
// and the battle's state (gBattleEngine->mState, which BattleEngine::SetState mirrors from the scripts).
//
// A "board" is a session of one mode, from the main menu and back to it:
//   mode        0 the adventure (the game screen up, mTimedMode off), 1 the arena (the game screen up, mTimedMode on),
//               2 Word Master, 3 Letter Rip, 4 Link n' Spell (a mini-game's board started from the main menu's list,
//               GameApp::mMiniGameFromMenu); -1 elsewhere. A mini-game played inside the adventure is part of it
//   boardSerial a count of the sessions since the app started; 0 off one
//   skill       0: the game has no difficulty
//   GO          the session's first update, when it starts a new game: the adventure with a player who has completed no
//               chapter and has got no farther than the first battle (FarthestBook, FarthestChapter and FarthestStage
//               absent or 1, 1, 1, and no battle saved, InGame), the arena with a new run (TM_CurrentStage -1, as
//               Profile::ResetTimedMode leaves it), and every mini-game. Any other session has the RESUMED flag and no GO
//   level clear the adventure: a chapter completed (the count of the profile's CompletedChapters rises); the arena: an
//               enemy beaten (TM_LastEnemyIndex rises)
//   game over   kind 0 Lex falls in the adventure (the battle enters BE_DYING); the adventure goes on with the battle
//               again, so the session keeps inGame 1 and counts each fall. Kind 1 Lex falls in the arena, which ends the
//               run. Kind 2 a mini-game's end: its board goes as the hall of fame comes up (DoHighScoreScreenFromMG); a
//               mini-game left from its menu ends no game
//   win         the adventure: the profile's BeatRegularGame becomes true (the third book's end, bookmanager.lua); the
//               arena: its win dialog comes up (DIALOG_ARENA_WIN)
//   progress    the adventure and the arena: the books completed, 0 to 3 (FarthestBook - 1, and 3 once BeatRegularGame);
//               a mini-game: -1
//   level       the chapters completed (the adventure) or the enemies beaten (the arena); displayLevel the chapter in
//               play (CurrentBook's CurrentChapter, or TM_CurrentChapter); 0 in a mini-game
//   state       the battle's state (BattleEngine::BE_*), -1 with no battle; tick the app's update count
//   gameOwn     four int32: the book, the chapter and the stage in play (the profile's Current*, or the arena's TM_
//               keys), and the falls of this session
// A session's first update is the baseline for its level clears, falls and win.

#include "LiveSplitObserver.h"

#ifdef BOOKWORMADV_HAS_LIVESPLIT

#include "BattleEngine.h"
#include "Board.h"
#include "GameApp.h"
#include "Profile.h"
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
	AUTOSPLIT_BLOCK_INIT(AUTOSPLIT_GAME_ID('B', 'W', 'A', 'D'));

namespace
{

enum
{
	ModeNone = -1,
	ModeAdventure = 0,
	ModeArena = 1,
	ModeWordMaster = 2,
	ModeLetterRip = 3,
	ModeLinkNSpell = 4,

	GameOverKindAdventureFall = 0,
	GameOverKindArenaFall = 1,
	GameOverKindMiniGameEnd = 2,

	BookCount = 3
};

AutosplitWriter	sWriter(&gLiveSplitBlock);
uint32_t		sSessionCount = 0;      // the sessions since the app started (the boards' serials)
int				sMode = ModeNone;       // the session's mode
bool			sResumed = false;       // the session continued a game (no GO)
int				sPrevCompleted = 0;     // the chapters completed, or the arena's enemies beaten, at the previous update
int				sPrevBattleState = -1;  // gBattleEngine's state at the previous update
bool			sPrevBeaten = false;    // the profile's BeatRegularGame at the previous update
bool			sPrevArenaWin = false;  // the arena's win dialog was up at the previous update
int				sFalls = 0;             // Lex's falls in this session
bool			sPageAsked = false;     // web: the page's timer asked for the block
std::string		sLogFileName;
FILE*			sLogFile = nullptr;

bool FeatureOn()
{
	for (const Platform::ModInfo* aMod : Platform::ModManager::Get().GetEnabledMods())
		if (aMod->mFeaturesJson.IsObject() && aMod->mFeaturesJson.Has("livesplit"))
			return true;
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
	if (!sLogFileName.empty()) {
		if (sLogFile == nullptr)
			sLogFile = fopen(sLogFileName.c_str(), "ab");
		if (sLogFile != nullptr) {
			fprintf(sLogFile, "%s\n", aLine);
			fflush(sLogFile);
		}
	}
}

int ProfileInt(const Profile* theProfile, const char* theKey, int theDefault)
{
	return theProfile->KeyExists(theKey) ? theProfile->GetInt(theKey) : theDefault;
}

// The chapters the profile has completed: CompletedChapters holds a list of chapter numbers for each book
int CompletedChapters(const Profile* theProfile)
{
	const Profile::Value* aBooks = theProfile->GetValue("CompletedChapters");
	if (aBooks == nullptr || aBooks->mType != Profile::TYPE_TABLE)
		return 0;
	int aCount = 0;
	for (const Profile::Entry& aBook : aBooks->mTable)
		if (aBook.mValue.mType == Profile::TYPE_TABLE)
			for (const Profile::Entry& aChapter : aBook.mValue.mTable)
				if (aChapter.mValue.mType != Profile::TYPE_NONE)
					aCount++;
	return aCount;
}

// The mode the app is in, as the block numbers it
int CurrentMode(const GameApp* theApp)
{
	if (theApp->mBookManager != nullptr)
		return theApp->mTimedMode ? ModeArena : ModeAdventure;
	const Board* aBoard = theApp->mBoard;
	if (aBoard != nullptr && aBoard->IsMiniGame() && theApp->mMiniGameFromMenu) {
		if (theApp->mMiniGameName == "WordMaster")
			return ModeWordMaster;
		if (theApp->mMiniGameName == "LetterRip")
			return ModeLetterRip;
		if (theApp->mMiniGameName == "LinkNSpell")
			return ModeLinkNSpell;
	}
	return ModeNone;
}

// The figure a session's level clears follow: the chapters completed, or the arena's enemies beaten
int Completed(const Profile* theProfile, int theMode)
{
	if (theProfile == nullptr)
		return 0;
	if (theMode == ModeAdventure)
		return CompletedChapters(theProfile);
	if (theMode == ModeArena)
		return ProfileInt(theProfile, "TM_LastEnemyIndex", 0);
	return 0;
}

bool IsNewGame(const Profile* theProfile, int theMode)
{
	if (theMode >= ModeWordMaster)
		return true;
	if (theProfile == nullptr)
		return false;
	if (theMode == ModeArena)
		return ProfileInt(theProfile, "TM_CurrentStage", -1) == -1;
	return CompletedChapters(theProfile) == 0 && ProfileInt(theProfile, "FarthestBook", 1) == 1 &&
	       ProfileInt(theProfile, "FarthestChapter", 1) == 1 && ProfileInt(theProfile, "FarthestStage", 1) <= 1 &&
	       !(theProfile->KeyExists("InGame") && theProfile->GetBool("InGame"));
}

void SetOwn(AutosplitBlock& theNext, int theIndex, int32_t theValue)
{
	memcpy(theNext.gameOwn + 4 * theIndex, &theValue, 4);
}

}  // namespace

void LiveSplitObserver::SetLogFile(const std::string& theFileName)
{
	sLogFileName = theFileName;
}

void LiveSplitObserver::AfterUpdate(GameApp* theApp)
{
	if (!sWriter.IsActive() && !sPageAsked && !FeatureOn())
		return;

	AutosplitBlock& aNext = sWriter.Next();
	bool isActivation = !sWriter.IsActive();
	if (isActivation) {
		sWriter.Activate(SDL_GetPerformanceFrequency());
		SDL_Log("livesplit: block at 0x%08x (layout %u)", (unsigned)aNext.self, (unsigned)AUTOSPLIT_LAYOUT_VERSION);
	}

	const Profile* aProfile = theApp->mProfile;
	int aMode = CurrentMode(theApp);
	bool isNewSession = isActivation || aMode != sMode;
	int aBattleState = gBattleEngine != nullptr ? gBattleEngine->mState : -1;
	bool aBeaten = aProfile != nullptr && aProfile->KeyExists("BeatRegularGame") && aProfile->GetBool("BeatRegularGame");
	bool anArenaWin = theApp->GetDialog(GameApp::DIALOG_ARENA_WIN) != nullptr;
	int aCompleted = Completed(aProfile, aMode);

	if (isNewSession) {
		// a mini-game's end: its board went as the hall of fame came up
		if (!isActivation && sMode >= ModeWordMaster && theApp->mHallOfFame != nullptr && !sWriter.HadGameOver()) {
			sWriter.GameOver(GameOverKindMiniGameEnd);
			Log("session %u: the mini-game's end (app update %d)", (unsigned)aNext.boardSerial, theApp->mUpdateCount);
			sWriter.Publish(SDL_GetPerformanceCounter());
		}
		sMode = aMode;
		sFalls = 0;
		if (aMode == ModeNone) {
			sWriter.NewBoard(0);
		} else {
			sWriter.NewBoard(++sSessionCount);
			sResumed = !IsNewGame(aProfile, aMode);
			if (!isActivation && !sResumed) {
				sWriter.Go(theApp->mUpdateCount);
				Log("session %u: GO in mode %d (app update %d)", (unsigned)sSessionCount, aMode, theApp->mUpdateCount);
			} else {
				// a session already going on when the block was asked for, or a continued game: no GO
				sResumed = true;
				aNext.inGame = 1;
			}
		}
		sPrevCompleted = aCompleted;
		sPrevBattleState = aBattleState;
		sPrevBeaten = aBeaten;
		sPrevArenaWin = anArenaWin;
	}

	if (aMode == ModeNone) {
		sWriter.ClearBoard();
	} else {
		if (aCompleted > sPrevCompleted && !sWriter.HadWin()) {
			for (int i = sPrevCompleted; i < aCompleted; i++)
				sWriter.LevelClear();
			Log("session %u: level clear, %d done (app update %d)", (unsigned)aNext.boardSerial, aCompleted,
			    theApp->mUpdateCount);
		}
		if (aBattleState == BattleEngine::BE_DYING && sPrevBattleState != BattleEngine::BE_DYING &&
		    (aMode == ModeAdventure || aMode == ModeArena) && !sWriter.HadWin() && !sWriter.HadGameOver()) {
			sFalls++;
			if (aMode == ModeArena) {
				sWriter.GameOver(GameOverKindArenaFall);
			} else {
				// the battle is played again and the session goes on: the game over's counters without the writer's
				// latch (GAME_OVER and inGame 0), so that every fall is counted
				aNext.deaths[GameOverKindAdventureFall]++;
				aNext.gameOvers++;
				aNext.gameOverSerial = aNext.boardSerial;
			}
			Log("session %u: Lex falls (kind %d, fall %d, app update %d)", (unsigned)aNext.boardSerial,
			    aMode == ModeArena ? 1 : 0, sFalls, theApp->mUpdateCount);
		}
		bool isWin = (aMode == ModeAdventure && aBeaten && !sPrevBeaten) || (aMode == ModeArena && anArenaWin && !sPrevArenaWin);
		if (isWin && !sWriter.HadWin()) {
			sWriter.Win();
			Log("session %u: won in mode %d (app update %d)", (unsigned)aNext.boardSerial, aMode, theApp->mUpdateCount);
		}

		uint32_t aFlags = 0;
		if (sResumed)
			aFlags |= AUTOSPLIT_FLAG_RESUMED;
		if (theApp->mBoard != nullptr && theApp->mBoard->IsPaused())
			aFlags |= AUTOSPLIT_FLAG_PAUSED;
		if (Platform::ModManager::Get().GetScoreKey().empty())
			aFlags |= AUTOSPLIT_FLAG_VANILLA;
		aNext.flags = aFlags;
		aNext.tick = theApp->mUpdateCount;
		aNext.state = aBattleState;
		aNext.mode = aMode;
		aNext.skill = 0;
		aNext.level = aCompleted;
		bool isArena = aMode == ModeArena;
		int aBook = aProfile == nullptr || aMode >= ModeWordMaster ? 0 :
		            ProfileInt(aProfile, isArena ? "TM_CurrentBook" : "CurrentBook", 0);
		int aChapter = aProfile == nullptr || aMode >= ModeWordMaster ? 0 :
		               ProfileInt(aProfile, isArena ? "TM_CurrentChapter" : "CurrentChapter", 0);
		int aStage = aProfile == nullptr || aMode >= ModeWordMaster ? 0 :
		             ProfileInt(aProfile, isArena ? "TM_CurrentStage" : "CurrentStage", 0);
		aNext.displayLevel = aChapter;
		int aBooksDone = 0;
		if (aProfile != nullptr && aMode <= ModeArena) {
			aBooksDone = aBeaten ? BookCount : ProfileInt(aProfile, "FarthestBook", 1) - 1;
			aBooksDone = aBooksDone < 0 ? 0 : (aBooksDone > BookCount ? BookCount : aBooksDone);
		}
		aNext.progress = aMode <= ModeArena ? aBooksDone : -1;
		SetOwn(aNext, 0, aBook);
		SetOwn(aNext, 1, aChapter);
		SetOwn(aNext, 2, aStage);
		SetOwn(aNext, 3, sFalls);
	}

	sPrevCompleted = aCompleted;
	sPrevBattleState = aBattleState;
	sPrevBeaten = aBeaten;
	sPrevArenaWin = anArenaWin;
	sWriter.Publish(SDL_GetPerformanceCounter());
}

void LiveSplitObserver::AppDestroyed(GameApp*)
{
	if (sWriter.IsActive()) {
		sWriter.Deactivate();
		sWriter.Publish(SDL_GetPerformanceCounter());
	}
	if (sLogFile != nullptr) {
		fclose(sLogFile);
		sLogFile = nullptr;
	}
}

#ifdef __EMSCRIPTEN__
extern "C" EMSCRIPTEN_KEEPALIVE uint32_t byopop_livesplit_block()
{
	sPageAsked = true;
	return (uint32_t)(uintptr_t)&gLiveSplitBlock;
}
#endif

#endif
