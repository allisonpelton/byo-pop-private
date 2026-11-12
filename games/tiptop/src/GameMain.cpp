// The game's main module (TipTop.exe 0x4113db-0x418f5b): sounds, music, preferences, main, the clocks and the save
// file's fields. See Game.h.

#include "Game.h"
#include "App.h"
#include "Config.h"
#include "GameText.h"
#include "Gui.h"
#include "Music.h"
#include "Res.h"
#ifdef __EMSCRIPTEN__
#include "SiteProfiles.h"
#endif

#include "compat2004/Compat2004.h"
#include "framework/SDLSoundManager.h"

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <string>

namespace tiptop
{

lowos::Sound* gSounds[0x2e];
uint16_t gSoundVolume = 0;
int gPreloaded = 0;
char gQuit = 0;
char gSkipMouseDown = 0;
char gFullScreen = 0;
volatile char gPaused[3];
char gClockResync = 0;
char gPrefByte = 1;
volatile char gClockLock = 0;
uint32_t gGameMode = 0;
char gBonusRound = 0;
char gBusy = 0;
char gModal = 0;
char gModal2 = 0;
char gHighScoreEntered = 0;
uint8_t gPauseFlags = 0;
uint8_t gParticleCount = 0;
uint8_t gEffectCount = 0;
void* gLevelList = nullptr;
uint32_t gLevelListCount = 0;

namespace
{

// The clocks' state (0x488858-0x4888a4): the last counter read, the counter at the start of the current second, the
// milliseconds of the seconds before and of the current one
struct Clock
{
	uint64_t mFraction;     // ms since mBase
	uint64_t mWhole;        // ms of the whole seconds counted
	uint64_t mBase;
	uint64_t mLast;
	uint64_t mFrequency;
};
Clock gGameClock;          // 0x488858
Clock gRealClock;          // 0x488880

// The C library's text of the open modes (not game text)
const char* const kRead = "rb";
const char* const kWrite = "wb";

// The resource table's entries the load bar loads (0x477060, 0x74 bytes each, the end marked by flags 0): those of
// types 2, 4 and 8
bool IsPreloadEntry(int theIndex)
{
	const res::Entry* anEntry = &res::GameTable()[theIndex];
	return anEntry->mType == 4 || anEntry->mType == 2 || anEntry->mType == 8;
}

} // namespace

uint32_t gRandState = 1;
uint32_t gRandDraws = 0;

int Rand()
{
	gRandState = gRandState * 214013u + 2531011u;
	gRandDraws++;
	return Sexy::CrtRand();
}

void SRand(uint32_t theSeed)
{
	gRandState = theSeed;
	Sexy::CrtSRand(theSeed);
}

// The original repeats these three calls wherever an allocation fails
void NoMemory()
{
	lowos::ScreenScope(2);
	lowos::MessageBox_(GTC(0x0046c2d0), GTC(0x0046c318));
	exit(0);
}

// 0x4113db: sound theId of group 2, made a LOWOS sound in the format its WAV header gives (null if it is missing)
lowos::Sound* LoadSound(int theId)
{
	if (res::Load(res::GROUP_SOUNDS, 1, theId) == 0)
	{
		uint8_t* aWav = (uint8_t*)res::Get(res::GROUP_SOUNDS, theId);
		uint16_t aChannels, aBits;
		uint32_t aSize, aRate;
		memcpy(&aChannels, aWav + 0x16, 2);
		memcpy(&aBits, aWav + 0x22, 2);
		memcpy(&aSize, aWav + 0x28, 4);
		memcpy(&aRate, aWav + 0x18, 4);
		lowos::Sound* aSound = lowos::CreateSound(aWav + 0x2c, aSize, aRate,
			(uint8_t)((aChannels != 2 ? 0x100 : 2) | (aBits != 0x10 ? 0x100 : 0xd)));
		// port: BYOPOP_SOUNDLOG's line for the sound, by the name TipTop.dat stores it under (Sounds\X.ogg)
		if (aSound != nullptr && Sexy::SoundLogOn())
		{
			const char* aStored = res::StoredName(res::GROUP_SOUNDS, theId);
			std::string aName = aStored != nullptr ? aStored : "?";
			std::replace(aName.begin(), aName.end(), '\\', '/');
			Sexy::SoundLog("load %d %s frames=%d rate=%d ch=%d", aSound->mHandle, aName.c_str(),
				(int)(aSize / (aChannels * (aBits / 8))), (int)aRate, (int)aChannels);
		}
		return aSound;
	}
	return nullptr;
}

// 0x411456
void FreeSound(lowos::Sound* theSound)
{
	if (theSound != nullptr)
		lowos::FreeSound(theSound);
}

static void LoadSoundTo(lowos::Sound** theSound, int theId)
{
	*theSound = LoadSound(theId);
}

static void FreeSoundOf(lowos::Sound** theSound)
{
	tiptop::FreeSound(*theSound);
	*theSound = nullptr;
}

// 0x411473: the sounds of the splash screen (it also read a LOWOS global it did not use, 0x42d210)
void LoadMenuSounds()
{
	FreeSounds();
	LoadSoundTo(&gSounds[0x0], 0);
	LoadSoundTo(&gSounds[0x2], 2);
	LoadSoundTo(&gSounds[0x3], 3);
}

// 0x4114b3: the other sounds, behind the busy cursor
void LoadGameSounds()
{
	lowos::ShowCursorLock(1);
	LoadSoundTo(&gSounds[0x1], 1);
	for (int i = 4; i <= 0x2d; i++)
		LoadSoundTo(&gSounds[i], i);
	lowos::ShowCursorLock(0);
}

// 0x411720
void Nothing()
{
}

// 0x411725
void FreeSounds()
{
	for (int i = 0; i <= 0x2d; i++)
		FreeSoundOf(&gSounds[i]);
	res::KeepOnly(res::GROUP_SOUNDS, 0);
}

// 0x41192f
void StartMusic()
{
	if (MusicPlaying() == 0)
	{
		if (res::Load(res::GROUP_MUSIC, 1, 0) == 0)
		{
			if (PlayMusic(res::GROUP_MUSIC, 0) != 0)
				res::Free(res::GROUP_MUSIC, 1, 0);
		}
	}
}

// 0x411975
void StopMusic()
{
	if (MusicPlaying() != 0)
	{
		StopMusic_();
		res::Free(res::GROUP_MUSIC, 1, 0);
	}
}

// 0x411999: Preferences.dat: the high scores, full screen, the sound and music volumes and a byte
void LoadPrefs()
{
	if ((uint32_t)(gConfig.mInitialSfxvolume << 8) < 0xffff)
		gSoundVolume = (uint16_t)(gConfig.mInitialSfxvolume << 8);
	else
		gSoundVolume = 0xffff;
	BuildMusicVolumes(gConfig.mInitialMusicvolume);
	FILE* aFile = UserFopen(GTC(0x0046c368), kRead);
	if (aFile == nullptr)
	{
		gFullScreen = 0;
		return;
	}
	if (fread(gHighScores, 1, 0x230, aFile) == 0x230)
	{
		fread(&gFullScreen, 1, 1, aFile);
		if ((gFullScreen == 1) || (gFullScreen == 0))
		{
			uint16_t aVolume;
			if (fread(&aVolume, 1, 2, aFile) == 2)
			{
				gSoundVolume = aVolume;
				uint32_t aMusicVolume;
				if (fread(&aMusicVolume, 1, 4, aFile) == 4)
				{
					BuildMusicVolumes(aMusicVolume);
					if (fread(&gPrefByte, 1, 1, aFile) == 1)
					{
						fclose(aFile);
					}
					else
					{
						gPrefByte = 1;
						fclose(aFile);
					}
				}
				else
				{
					fclose(aFile);
				}
			}
			else
			{
				fclose(aFile);
			}
		}
		else
		{
			gFullScreen = 0;
			fclose(aFile);
		}
	}
	else
	{
		ResetHighScores();
		gFullScreen = 0;
		fclose(aFile);
	}
}

// 0x411b38: removes the file if a write fails
void SavePrefs()
{
	FILE* aFile = UserFopen(GTC(0x0046c368), kWrite);
	if (aFile == nullptr)
	{
		UserRemove(GTC(0x0046c368));
		return;
	}
	uint32_t aMusicVolume = MusicVolume();
	if (fwrite(gHighScores, 1, 0x230, aFile) == 0x230 && fwrite(&gFullScreen, 1, 1, aFile) == 1 &&
		fwrite(&gSoundVolume, 1, 2, aFile) == 2 && fwrite(&aMusicVolume, 1, 4, aFile) == 4 &&
		fwrite(&gPrefByte, 1, 1, aFile) == 1)
	{
		fclose(aFile);
		UserFileWritten();
	}
	else
	{
		fclose(aFile);
		UserRemove(GTC(0x0046c368));
	}
}

// 0x411c85: the empty tables: dashes, 0 points, level 1
void ResetHighScores()
{
	for (int i = 0; i < 5; i = i + 1)
	{
		for (int aMode = 0; aMode < 4; aMode = aMode + 1)
		{
			strcpy(gHighScores[aMode][i].mName, GTC(0x0046c380));
			gHighScores[aMode][i].mScore = 0;
			gHighScores[aMode][i].mLevel = 1;
		}
	}
}

// The original kept the trial's count of plays, and 0x412b2e made the exe's folder the current one: the port has
// neither (it plays as registered, and the host chooses the folders).

// 0x412b90: the preferences, the app and the board's copy of the configuration
static bool InitApp()
{
#ifdef __EMSCRIPTEN__
	SiteKeepStartingScores();	// the web page's players (SiteProfiles.h): the configuration's high scores, not a player's
#endif
	LoadPrefs();
	int aResult = gApp.Init();
	if (aResult != 0)
		gApp.SetConfig(&gConfig);
	return aResult != 0;
}

// 0x411fff, up to the main loop
bool GameInit()
{
	// TipTopApp::Init has seeded the C library's generator with the tick count, as this did
	LoadGameTexts();
	res::SetTable(res::GameTable());
	res::SetDataFile(0, GTC(0x0046c444));
	if (LoadConfig(&gConfig) < 0)
	{
		lowos::ScreenScope(2);
		lowos::MessageBox_(GTC(0x0046c424), GTC(0x0046c318));
		exit(0);
	}
	lowos::InitLock(&gClockLock);
	if (!InitApp())
	{
		lowos::ScreenScope(2);
		lowos::MessageBox_(GTC(0x0046c3c0), GTC(0x0046c318));
		exit(0);
	}
#ifdef __EMSCRIPTEN__
	SiteStart();	// the web page's players (framework/WebProfiles.h): how the site's names fit the score's text field
#endif
	return true;
}

static uint16_t gLastMouseX = 0xffff;  // the loop's local_32[3], local_32[4]
static uint16_t gLastMouseY = 0xffff;

// 0x411fff: one pass of the main loop
bool GameFrame()
{
	if (gQuit != 0)
		return false;
	uint8_t aChar = lowos::NextChar();
	if (aChar != 0)
		Dispatch(MSG_CHAR, aChar, 0);
	uint16_t x, y;
	lowos::GetMousePos(&x, &y);
	if ((x < 0x280) && (y < 0x1e0))
	{
		if ((x != gLastMouseX) || (y != gLastMouseY))
		{
			Dispatch(MSG_MOUSEMOVE, x, y);
			gLastMouseX = x;
			gLastMouseY = y;
		}
		uint32_t anEvent = lowos::NextMouseEvent();
		if ((anEvent & 1) != 0)
		{
			if ((anEvent & 0x80000000) == 0)
				Dispatch(MSG_MOUSEUP, x, y);
			else if (gSkipMouseDown == 0)
				Dispatch(MSG_MOUSEDOWN, x, y);
		}
	}
	gSkipMouseDown = 0;
	if ((gQuit != 0) || (gApp.Frame(), gQuit != 0))
		return false;
	char aResult = 0;
	lowos::EndFrame(&aResult);
	if (aResult == 0)
	{
		lowos::ScreenScope(2);
		lowos::MessageBox_(GTC(0x0046c39c), GTC(0x0046c318));
		exit(0);
	}
	// the original slept 50 ms per pass while the game was paused; the host paces the frames
	return true;
}

// 0x411fff after the loop
void GameShutdown()
{
	for (int aList = 0; aList < 4; aList++)
		free(gLevels[aList]);
	if (gConfig.mMainmenuanimationsCount != 0)
		free(gConfig.mMainmenuanimations);
	if (gConfig.mNewgamemenuanimationsCount != 0)
		free(gConfig.mNewgamemenuanimations);
	if (gConfig.mHighscoresmenuanimationsCount != 0)
		free(gConfig.mHighscoresmenuanimations);
	if (gConfig.mOptionsmenuanimationsCount != 0)
		free(gConfig.mOptionsmenuanimations);
	if (gConfig.mInstructionsmenuanimationsCount != 0)
		free(gConfig.mInstructionsmenuanimations);
	if (gConfig.mCreditsmenuanimationsCount != 0)
		free(gConfig.mCreditsmenuanimations);
	if (gConfig.mComboanimParamsCount != 0)
		free(gConfig.mComboanimParams);
	if (gConfig.mCombostarParamsCount != 0)
		free(gConfig.mCombostarParams);
	if (gConfig.mStarburstParamsCount != 0)
		free(gConfig.mStarburstParams);
	if (gConfig.mParticles != nullptr)
		free(gConfig.mParticles);
	if (gConfig.mColumnnukeEffects.mEffects != nullptr)
		free(gConfig.mColumnnukeEffects.mEffects);
	if (gConfig.mColorbombEffects.mEffects != nullptr)
		free(gConfig.mColorbombEffects.mEffects);
	SavePrefs();
	for (int i = 0; (i < gPreloaded && res::GameTable()[i].mFlags != 0); i = i + 1)
	{
		if (IsPreloadEntry(i))
			res::Free(res::GameTable()[i].mGroup, 1, res::GameTable()[i].mId);
	}
}

// 0x4182d7, 0x418510: milliseconds from the performance counter, whole seconds and the current one counted apart;
// the game clock stands still while paused and skips the pause when it resumes (gClockResync)
static uint32_t ReadClock(Clock& theClock, bool isGameClock)
{
	lowos::Lock(&gClockLock);
	if (theClock.mFrequency == 0)
		theClock.mFrequency = (uint64_t)lowos::PerfFrequency();
	if (theClock.mLast == 0)
	{
		theClock.mLast = (uint64_t)lowos::PerfCounter();
		theClock.mBase = theClock.mLast;
	}
	if (!isGameClock || ((gPaused[0] == 0 && gPaused[1] == 0) && gPaused[2] == 0))
	{
		uint64_t aNow = (uint64_t)lowos::PerfCounter();
		if ((isGameClock && gClockResync != 0) || aNow < theClock.mLast)
		{
			if (isGameClock)
				gClockResync = 0;
			theClock.mBase = aNow;
			theClock.mWhole = theClock.mWhole + theClock.mFraction;
			theClock.mFraction = 0;
		}
		theClock.mLast = aNow;
		uint64_t aSeconds = (aNow - theClock.mBase) / theClock.mFrequency;
		// the original kept only the low half of the sum
		theClock.mWhole = (uint32_t)(theClock.mWhole + (uint32_t)(aSeconds * 1000));
		theClock.mBase = aSeconds * theClock.mFrequency + theClock.mBase;
		theClock.mFraction = (aNow - theClock.mBase) * 1000 / theClock.mFrequency;
	}
	lowos::Unlock(&gClockLock);
	return (uint32_t)theClock.mFraction + (uint32_t)theClock.mWhole;
}

// 0x4182d7
uint32_t GameTime()
{
	return ReadClock(gGameClock, true);
}

// 0x418510
uint32_t RealTime()
{
	return ReadClock(gRealClock, false);
}

// 0x418716
void SetPause(char isPaused, int theIndex)
{
	gPaused[(uint8_t)theIndex] = isPaused;
	gClockResync = 1;
}

// 0x41872f: the save file's field writer
char WriteField(char theType, size_t theSize, void* theData, FILE* theFile, uint32_t theNow)
{
	uint32_t* aData = (uint32_t*)theData;
	if (theType == 'B')
	{
		// a Piece: its 23 fields; 'T' marks the times
		static const char kTypes[23] = {'D', 'D', 'D', 'T', 'D', 'D', 'D', 'D', 'T', 'D', 'D', 'D', 'D', 'T', 'D',
			'D', 'D', 'D', 'D', 'T', 'D', 'D', 'D'};
		static const uint8_t kSizes[23] = {4, 1, 4, 4, 4, 4, 4, 4, 4, 4, 2, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 1};
		for (int i = 0; i < 23; i++)
		{
			if (WriteField(kTypes[i], kSizes[i], aData + i, theFile, theNow) != 0)
				return 1;
		}
	}
	else if (theType == 'D')
	{
		if (fwrite(theData, 1, theSize, theFile) != theSize)
		{
			fclose(theFile);
			UserRemove(GTC(0x0046ec28));
			return 1;
		}
	}
	else if (theType == 'T')
	{
		uint32_t aValue = (theNow < *aData) ? 0 : theNow - *aData;
		if (fwrite(&aValue, 1, 4, theFile) != 4)
		{
			fclose(theFile);
			UserRemove(GTC(0x0046ec28));
			return 1;
		}
	}
	return 0;
}

// 0x418b51: the save file's field reader
char ReadField(char theType, size_t theSize, void* theData, FILE* theFile, uint32_t theNow)
{
	uint32_t* aData = (uint32_t*)theData;
	if (theType == 'B')
	{
		static const char kTypes[23] = {'D', 'D', 'D', 'T', 'D', 'D', 'D', 'D', 'T', 'D', 'D', 'D', 'D', 'T', 'D',
			'D', 'D', 'D', 'D', 'T', 'D', 'D', 'D'};
		static const uint8_t kSizes[23] = {4, 1, 4, 4, 4, 4, 4, 4, 4, 4, 2, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 1};
		for (int i = 0; i < 23; i++)
		{
			if (ReadField(kTypes[i], kSizes[i], aData + i, theFile, theNow) != 0)
				return 1;
		}
	}
	else if (theType == 'D')
	{
		if (fread(theData, 1, theSize, theFile) != theSize)
		{
			fclose(theFile);
			return 1;
		}
	}
	else if (theType == 'T')
	{
		if (fread(theData, 1, 4, theFile) != 4)
		{
			fclose(theFile);
			return 1;
		}
		if (theNow < *aData)
			*aData = 0;
		else
			*aData = theNow - *aData;
	}
	return 0;
}

} // namespace tiptop
