// port(mod): the shared autosplit block for Bejeweled Twist (see LiveSplitObserver.h).
//
// What the Splits window reads, per board (Board::mBoardSerial, a counter of boards made):
//   mode        the board's own numbers (Board::MODE_): 1 Classic, 2 Challenge, 3 Blitz, 4 Zen; -1 off a board. The
//               menus' replays (the main menu's background game, the stats' replay) are boards too, flagged REPLAY
//   skill       0: the game has no difficulty
//   GO          the board's first update when it is a new game: not a resumed save (Board::mGameLoaded) and not a
//               replay. A board is made by the click that starts it (Classic's New, Zen's or Blitz's button, a planet's
//               challenge), so this is that click's update or the one after it
//   level clear the level's "Level Complete" picture appears (Board::ShowLevelUp makes the LevelUpShow, which shows the
//               level just finished, Board::mLevel, the block's displayLevel); Classic and Zen only
//   game over   the board's game ends (Board::mGameOver rises) and it was not won: kind 0 the board blew up (a counter
//               ran out, or a coal or doom gem reached the bottom), 1 the second chance's wheel landed on a skull, 2
//               Blitz's clock ran out, 3 any other end (a challenge whose goal can no longer be met, the tutorial's
//               "start Classic" closing its board)
//   win         a challenge is met: the "Challenge Complete" award appears (Board::ShowChallengeComplete hands the
//               challenge menu its AwardTelly); the board's mGameOver that follows is no game over
//   progress    how many of the four modes are open in the player's profile (Profile::mModesUnlocked, bits 1 to 4);
//               4 is every mode, Unlock%'s end. Steam's copy (the port's) opens Blitz at once, so there the last one
//               is Challenge, at rank 2
//   gameOwn     four int32: the profile's mode bits as they are; the board's challenge category (0 Detonator to 12
//               Enigma, as properties/challenges.xml orders them; -1 off a challenge); its number in the category (from
//               1; 0 off a challenge); the challenges met in every category (each category's stage less one)

#include "LiveSplitObserver.h"

#ifdef BEJEWELEDTWIST_HAS_LIVESPLIT

#include "WinTwistApp.h"
#include "Board.h"
#include "Challenge.h"
#include "ChallengeMenu.h"
#include "Menu.h"
#include "Profile.h"
#include "WheelSpin.h"
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
	AUTOSPLIT_BLOCK_INIT(AUTOSPLIT_GAME_ID('B', 'J', 'T', 'W'));

namespace
{

AutosplitWriter		sWriter(&gLiveSplitBlock);
bool				sGoPossible = false;        // the current board may still have its GO
bool				sResumed = false;           // the current board resumed a saved game (seen at its first update)
bool				sPrevGameOver = false;      // the board's mGameOver at the previous update
TwistWidget*		sPrevLevelUp = nullptr;     // the board's LevelUpShow at the previous update
AwardTelly*			sPrevAward = nullptr;       // the challenge menu's award at the previous update
unsigned int		sPrevModeBits = 0;          // the profile's open modes at the previous update
bool				sPageAsked = false;         // web: the page's timer asked for the block
std::string			sLogFileName;
FILE*				sLogFile = nullptr;

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
		if (sLogFile == nullptr)
			sLogFile = fopen(sLogFileName.c_str(), "ab");
		if (sLogFile != nullptr)
		{
			fprintf(sLogFile, "%s\n", aLine);
			fflush(sLogFile);
		}
	}
}

int OpenModes(unsigned int theBits)
{
	int aCount = 0;
	for (int aMode = Board::MODE_CLASSIC; aMode <= Board::MODE_ZEN; aMode++)
		if ((theBits & (1u << aMode)) != 0)
			aCount++;
	return aCount;
}

int ChallengesMet(const Profile* theProfile)
{
	int aCount = 0;
	for (int aStage : theProfile->mChallengeStages)
		if (aStage > 1)
			aCount += aStage - 1;
	return aCount;
}

void SetOwn(AutosplitBlock& theNext, int theIndex, int32_t theValue)
{
	memcpy(theNext.gameOwn + 4 * theIndex, &theValue, 4);
}

}

void LiveSplitObserver::SetLogFile(const std::string& theFileName)
{
	sLogFileName = theFileName;
}

void LiveSplitObserver::AfterUpdate(WinTwistApp* theApp)
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
	uint32_t aSerial = (aBoard != nullptr) ? aBoard->mBoardSerial : 0;
	bool isNewBoard = isActivation || (aSerial != aNext.boardSerial);
	ChallengeMenu* aMenu = (theApp->mMainMenu != nullptr) ? theApp->mMainMenu->mChallengeMenu : nullptr;
	AwardTelly* anAward = (aMenu != nullptr) ? aMenu->mAwardTelly : nullptr;
	if (isNewBoard)
	{
		sWriter.NewBoard(aSerial);
		sGoPossible = false;
		sResumed = false;
		sPrevGameOver = false;
		sPrevLevelUp = nullptr;
		sPrevAward = anAward;
	}

	Profile* aProfile = theApp->mProfile;
	unsigned int aModeBits = (aProfile != nullptr) ? aProfile->mModesUnlocked : 0;
	if (aBoard == nullptr)
	{
		sWriter.ClearBoard();
	}
	else
	{
		bool isReplay = aBoard->mIsReplay;
		if (isNewBoard)
		{
			if (aBoard->mGameOver)
				sWriter.SeenOver();
			sResumed = aBoard->mGameLoaded;
			sPrevGameOver = aBoard->mGameOver;
			sPrevLevelUp = aBoard->mLevelUpShow;
			sGoPossible = (!isActivation) && (!sResumed) && (!isReplay) && (!aBoard->mGameOver);
		}

		if (sGoPossible)
		{
			sGoPossible = false;
			sWriter.Go(aBoard->mUpdateCnt);
			Log("board %u GO at tick %d (mode %d, app update %d, modes open %d)", (unsigned) aSerial, aBoard->mUpdateCnt,
				aBoard->mGameMode, theApp->mUpdateCount, OpenModes(aModeBits));
		}

		if ((!isNewBoard) && (!isReplay))
		{
			if ((aBoard->mLevelUpShow != nullptr) && (aBoard->mLevelUpShow != sPrevLevelUp))
			{
				sWriter.LevelClear();
				Log("board %u level clear %d at tick %d (app update %d)", (unsigned) aSerial, aBoard->mLevel, aBoard->mUpdateCnt,
					theApp->mUpdateCount);
			}
			if ((!sWriter.HadWin()) && (!sWriter.HadGameOver()) && (aBoard->mGameMode == Board::MODE_CHALLENGE) &&
				(anAward != nullptr) && (anAward != sPrevAward))
			{
				sWriter.Win();
				Log("board %u win (challenge %d of category %d) at tick %d (app update %d)", (unsigned) aSerial,
					(aBoard->mChallenge != nullptr) ? aBoard->mChallenge->mNumber : 0,
					(aBoard->mChallenge != nullptr) ? aBoard->mChallenge->mCategory : -1, aBoard->mUpdateCnt, theApp->mUpdateCount);
			}
			if ((!sWriter.HadWin()) && (!sWriter.HadGameOver()) && aBoard->mGameOver && (!sPrevGameOver))
			{
				int aKind = 3;
				if (aBoard->mEndedByNuke)
					aKind = (gWheelSpin != nullptr) ? 1 : 0;
				else if (aBoard->mGameMode == Board::MODE_BLITZ)
					aKind = 2;
				sWriter.GameOver(aKind);
				Log("board %u game over (kind %d) at tick %d on level %d (app update %d)", (unsigned) aSerial, aKind,
					aBoard->mUpdateCnt, aBoard->mLevel, theApp->mUpdateCount);
			}
		}

		uint32_t aFlags = 0;
		if (sResumed)
			aFlags |= AUTOSPLIT_FLAG_RESUMED;
		if (isReplay)
			aFlags |= AUTOSPLIT_FLAG_REPLAY;
		if (aBoard->mPauseCount != 0)
			aFlags |= AUTOSPLIT_FLAG_PAUSED;
		if (Platform::ModManager::Get().GetScoreKey().empty())
			aFlags |= AUTOSPLIT_FLAG_VANILLA;
		aNext.flags = aFlags;
		aNext.tick = aBoard->mUpdateCnt;
		aNext.state = sWriter.HadWin() ? 3 : (aBoard->mGameOver ? 2 : (aBoard->IsBusy() ? 1 : 0));
		aNext.level = aBoard->mLevel;
		aNext.displayLevel = aBoard->mLevel;
		aNext.score = aBoard->mScore;
		sWriter.SetDisplayScore(aBoard->mShownScore);
		aNext.mode = aBoard->mGameMode;
		aNext.skill = 0;
		sPrevGameOver = aBoard->mGameOver;
		sPrevLevelUp = aBoard->mLevelUpShow;
	}
	sPrevAward = anAward;

	// the profile's own figures, on and off a board
	if ((!isActivation) && (aModeBits != sPrevModeBits))
		Log("modes open %d (bits 0x%x) at app update %d", OpenModes(aModeBits), aModeBits, theApp->mUpdateCount);
	sPrevModeBits = aModeBits;
	aNext.progress = OpenModes(aModeBits);
	SetOwn(aNext, 0, (int32_t) aModeBits);
	Challenge* aChallenge = (aBoard != nullptr && aBoard->mGameMode == Board::MODE_CHALLENGE) ? aBoard->mChallenge : nullptr;
	SetOwn(aNext, 1, (aChallenge != nullptr) ? aChallenge->mCategory : -1);
	SetOwn(aNext, 2, (aChallenge != nullptr) ? aChallenge->mNumber : 0);
	SetOwn(aNext, 3, (aProfile != nullptr) ? ChallengesMet(aProfile) : 0);
	sWriter.Publish(SDL_GetPerformanceCounter());
}

void LiveSplitObserver::AppDestroyed(WinTwistApp*)
{
	if (sWriter.IsActive())
	{
		sWriter.Deactivate();
		sWriter.Publish(SDL_GetPerformanceCounter());
	}
	if (sLogFile != nullptr)
	{
		fclose(sLogFile);
		sLogFile = nullptr;
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
