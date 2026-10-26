#ifndef __TIPTOP_GAME_H__
#define __TIPTOP_GAME_H__

#include "Lowos.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>

// The game's main module (TipTop.exe 0x4113db-0x418f5b: sounds, music, preferences, main, clocks, the save file's
// fields) and the globals that several modules share. The original's WinMain (0x42d020) and game main (0x411fff)
// are cut at the main loop so that the host can run one pass of the loop per update.

namespace tiptop
{

struct Handler;

// --- shared state ---
extern lowos::Sound* gSounds[0x2e];     // 0x488364.. the sounds of group 2, by id (scattered in the original)
extern uint16_t gSoundVolume;           // 0x488708
extern int gPreloaded;                  // 0x48870c the entries of the resource table loaded so far (the load bar)
extern char gRegistered;                // 0x48870a (the port plays as registered)
extern char gQuit;                      // 0x488705
extern char gSkipMouseDown;             // 0x4886fd the click that activated the window is not a click in the game
extern char gFullScreen;                // 0x4886ec
extern volatile char gPaused[3];        // 0x4886f0 the game clock is stopped while one is set
extern char gClockResync;               // 0x4886f3
extern char gPrefByte;                  // 0x477058 a byte of Preferences.dat (default 1)
extern volatile char gClockLock;        // 0x488499
extern uint32_t gRunCount;              // 0x488380 the trial's count of plays (the registry, not kept by the port)
extern uint8_t**& gParticleState;       // 0x4887a0 = gApp.mParticleImages: one image per configured particle (0x419991)
extern uint32_t gGameMode;              // 0x488700 0 normal, 1 time trial, 2 relax?, 3 (the level list of each)
extern char gBonusRound;                // 0x488704
extern char gBusy;                      // 0x4884a0 animations that hold the game (a CounterProc counter)
extern char gModal;                     // 0x4886f4 a confirmation dialog is open (a CounterProc counter)
extern char gModal2;                    // 0x4886f8
extern char gHighScoreEntered;          // 0x488498
extern uint8_t gPauseFlags;             // 0x4886fc 1 paused by the player, 2 the window is inactive
extern uint8_t gParticleCount;          // 0x489b54 particles on the screen (at most 0xc8 are started)
extern uint8_t gEffectCount;            // 0x489b55 particles and combo stars on the screen
extern void* gLevelList;                // 0x488494 the current mode's levels
extern uint32_t gLevelListCount;        // 0x4884a8

// --- the C library as the original used it ---
// MSVC's rand (0x44ccaf) and srand: the same sequence for a seed as the original
int Rand();
void SRand(uint32_t theSeed);

// --- 0x4113db-0x411c85: sounds, music, preferences ---
lowos::Sound* LoadSound(int theId);     // 0x4113db
void FreeSound(lowos::Sound* theSound); // 0x411456
void LoadMenuSounds();                  // 0x411473 sounds 0, 2 and 3 (what the splash screen needs)
void LoadGameSounds();                  // 0x4114b3 the others (after the load bar)
void Nothing();                         // 0x411720 an empty function the board calls every frame
void FreeSounds();                      // 0x411725
void StartMusic();                      // 0x41192f
void StopMusic();                       // 0x411975
void LoadPrefs();                       // 0x411999
void SavePrefs();                       // 0x411b38
void ResetHighScores();                 // 0x411c85

// The original repeats these three calls wherever an allocation fails (ScreenScope(2), the message, exit)
void NoMemory();

// --- clocks (ms): the game's stops while it is paused or inactive, the other always runs ---
uint32_t GameTime();                    // 0x4182d7
uint32_t RealTime();                    // 0x418510
void SetPause(char isPaused, int theIndex);  // 0x418716 gPaused[theIndex]

// --- the save file (savegame.dat) ---
// One field: 'D' theSize bytes as they are, 'T' a GameTime stored as the time from it to theNow (a 4-byte field),
// 'B' a 0x5c-byte Piece (23 fields, two of them times). Return 1 on failure: the file is closed and the writer
// removes savegame.dat.
char WriteField(char theType, size_t theSize, void* theData, FILE* theFile, uint32_t theNow);  // 0x41872f
char ReadField(char theType, size_t theSize, void* theData, FILE* theFile, uint32_t theNow);   // 0x418b51

// port (UserFile.cpp): the preferences and savegame.dat are in the user dir, not the working directory. A file
// written (and closed) is reported with UserFileWritten, so the web build saves it; UserRemove reports itself.
FILE* UserFopen(const char* theName, const char* theMode);
int UserRemove(const char* theName);
void UserFileWritten();

// The game's command handler: what a button, check box, slider, text field or animation does (0x41f922)
uint32_t Command(Handler* theSource, uint32_t theCommand);  // 0x41f922

// The beginning of 0x411fff, up to its main loop: the configuration, the registry, the screen and the first screen of
// the game. False if the game could not start (it has shown why).
bool GameInit();
// One pass of the main loop: input events, the frame (update and draw) and LOWOS_EndFrame. False once the game
// has quit.
bool GameFrame();
// The end of 0x411fff after its loop
void GameShutdown();

} // namespace tiptop

#endif
