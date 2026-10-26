#ifndef __COSMICBUGS_CBGLOBALS_H__
#define __COSMICBUGS_CBGLOBALS_H__

// The game's globals and the system layer's calls (the input, sound and music of 0x428000..0x42cd00, which the
// host provides)

#include "CBGame.h"
#include "R64Font.h"

namespace cb
{

// A player (0x44 bytes; 11 of them at 0x46df60)
struct Profile
{
	char name[0x20];           // +0x00
	int levelReached;          // +0x20
	int unk24;                 // +0x24
	int savedLevel;            // +0x28  the game in progress (0 when there is none)
	int savedLives;            // +0x2c
	int savedScore;            // +0x30
	int beamsFired;            // +0x34
	int riderHits;             // +0x38  beamriders that reached the ship
	int unk3c[2];              // +0x3c
};
static_assert(sizeof(Profile) == 0x44, "Profile");

// A level's records (0x30 bytes, 1000 of them at 0x4623e0 by level number)
struct LevelRecord
{
	char timeName[0x14];       // +0x00
	int bestTime;              // +0x14
	char fillName[0x14];       // +0x18
	int bestFill;              // +0x2c  percent
};
static_assert(sizeof(LevelRecord) == 0x30, "LevelRecord");

// A level set of the game file (the levels from start to end, with their song)
struct LevelSet
{
	LevelSet* next;            // +0x00
	LevelSet* prev;            // +0x04
	char song[0x40];           // +0x08
	int start;                 // +0x48
	int end;                   // +0x4c
};

extern char gSongName[0x40];               // 0x6e5aa8  a song to start, "" for none
extern int gLogoCount;                     // 0x6e5ae8
extern int gGoTo;                          // 0x460034  the kind of screen to go to next
extern int gEscape;                        // 0x46e5e4  Esc on a board: pauses it, or leaves it when paused
extern int gDemo;                          // 0x46e5c8  the game plays itself
extern List<LevelSet> gLevelSets;          // 0x4700d4
extern int gProfile;                       // 0x46e24c
extern Profile gProfiles[11];              // 0x46df60
extern LevelRecord gLevelRecords[1000];    // 0x4623e0
extern int gRegistered;                    // 0x460004
extern int gScore;                         // 0x46e5e8
extern int gLowestHighScore;               // 0x46e36c
extern int gFlag46003c;                    // 0x46003c
extern int gLevelInSet;                    // 0x46e5ec
extern int gMouseX;                        // 0x46e5bc
extern int gMouseY;                        // 0x46e5c0
extern int gTrial;                         // 0x46e5c4
extern int gTrialLeft;                     // 0x460044  ms
extern int gLives;                         // 0x460040
extern float gBeamSpeed;                   // 0x460038  the ship's beams' speed at the start
extern int gDifficulty;                    // 0x46e5e0  -1, 0 or 1 (the options' cycle)
extern int gUnk460028;                     // 0x460028
extern const int kClip[4];                 // 0x460010  the screen
// 0x472ab0: 800x600 in the screen's format, the level's background with the cells drawn on (each frame the dirty
// parts go to the screen under the objects)
extern Image gBackBuffer;
bool Sys_FileExists(const char* thePath);

// The game's texts, read from the player's exe by address (CBText.cpp)
const char* GT(uint32_t theVA);
const char* Localize(const char* theText);                       // 0x420e10
Font* Font_Find(const char* theName);                            // 0x420850

// The system layer
int Mouse_Pressed(int theButton);
int Key_Pressed(int theKey);
int Sound_Load(const char* thePath);                             // Sound_LoadList on the list at 0x473150
int Sound_Find(const char* thePath);                             // 0x428650 on it
void Sound_Play(int theSound);
void Music_Free();
void Music_Load(const char* thePath);
void Music_SetVolume(int theVolume);
void Music_Play();
void DrawLoadingScreen();
int CrtRand();                                                   // the CRT's rand
void SaveProfiles();                                             // 0x40c900
void Fn40d040();                                                 // 0x40d040

} // namespace cb

#endif
