#ifndef __COSMICBUGS_CBGLOBALS_H__
#define __COSMICBUGS_CBGLOBALS_H__

// The game's globals and the system layer's calls (the input, sound and music of 0x428000..0x42cd00, which the
// host provides)

#include "CBGame.h"
#include "R64Font.h"
#include "R64Sys.h"

namespace cb
{

// A high score (0x24 bytes; nine at 0x46e250: eight shown, the ninth for the new score)
struct HighScore
{
	char name[0x20];           // +0x00
	int score;                 // +0x20
};
static_assert(sizeof(HighScore) == 0x24, "HighScore");

// A font of the game file (the list at 0x470e4c; Font_Find gives its font by name)
struct FontEntry
{
	FontEntry* next;           // +0x000
	FontEntry* prev;           // +0x004
	char name[0x20];           // +0x008
	char strip[0x100];         // +0x028  Font_StripCut's image
	char chars[0x100];         // +0x128  and characters
	Font font;                 // +0x228
};

// A player (0x44 bytes; 11 of them at 0x46df60)
struct Profile
{
	char name[0x20];           // +0x00
	int levelReached;          // +0x20
	int autoFire;              // +0x24  the shooter mode (gAutoFire) the player last chose
	int savedLevel;            // +0x28  the game in progress (0 when there is none)
	int savedLives;            // +0x2c
	int savedScore;            // +0x30
	int beamsFired;            // +0x34
	int riderHits;             // +0x38  beamriders that reached the ship
	int kills;                 // +0x3c  enemies killed
	int powerupsCaught;        // +0x40
};
static_assert(sizeof(Profile) == 0x44, "Profile");

// A level's records (0x30 bytes, 1000 of them at 0x4623e0 by level number)
struct LevelRecord
{
	char shotsName[0x14];      // +0x00
	int fewestShots;           // +0x14
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
extern int gScore;                         // 0x46e5e8
extern int gFirstWeapon;                   // 0x46003c  no orbiting weapon has been updated yet this update
extern int gLevelInSet;                    // 0x46e5ec
extern int gMouseX;                        // 0x46e5bc
extern int gMouseY;                        // 0x46e5c0
extern int gTrial;                         // 0x46e5c4
extern int gTrialLeft;                     // 0x460044  ms
extern int gLives;                         // 0x460040
extern float gBeamSpeed;                   // 0x460038  the ship's beams' speed at the start
extern int gDifficulty;                    // 0x46e5e0  -1, 0 or 1 (the options' cycle)
extern int gAutoFire;                      // 0x460028  the shooter mode: 0 manual, 1 auto
extern int gQuit;                          // 0x46e5d0
extern int gFirstRun;                      // 0x46e5d4  cb.dat was missing
extern char gUrl[0x200];                   // 0x46e3a8  opened when the game ends
extern int gFrame;                         // 0x460a1c  updates
extern HighScore gHighScores[9];           // 0x46e250
extern List<FontEntry> gFonts;             // 0x470e4c
extern Font gFontSmall;                    // 0x471d10
extern Font gFontTreb11;                   // 0x46f368
extern Font gFontTreb20;                   // 0x46e5f0
extern Font gFontIcon;                     // 0x470f80
extern Image* gCursor;                     // 0x4735b4  the mouse pointer drawn
extern Image* gCursorArrow;                // 0x4735ac
extern Image* gCursorClick;                // 0x4735a0
extern Image* gCursorClock;                // 0x4735b0
extern int gCursorShown;                   // 0x460874
extern Palette gColourCodes[10];           // 0x4735b8  the fonts' colour codes '0' to '9'
// 0x472ab0: 800x600 in the screen's format, the level's background with the cells drawn on (each frame the dirty
// parts go to the screen under the objects)
extern Image gBackBuffer;
bool Sys_FileExists(const char* thePath);

// The game's texts, read from the player's exe by address (CBGameText.cpp)
const char* GT(uint32_t theVA);
const char* Localize(const char* theText);                       // 0x420e10
Font* Font_Find(const char* theName);                            // 0x420850

// The system layer
int Sound_Load(const char* thePath);                             // Sound_LoadList on the list at 0x473150
int Sound_Find(const char* thePath);                             // 0x428650 on it
void Sound_Play(int theSound);
void Music_Free();
void Music_Load(const char* thePath);
void Music_SetVolume(int theVolume);
void Music_Play();
int Screen_Fullscreen();                                         // 0x428f00
void Screen_SetFullscreen(int theOn);                            // 0x428ee0
int Screen_Centered();                                           // 0x428f40: 0 the screen is stretched to the display
void Screen_SetCentered(int theOn);                              // 0x428f10
int Sound_On();                                                  // 0x4290f0
void Sound_SetOn(int theOn);                                     // 0x42a010
int Music_On();                                                  // 0x429100
void Music_SetOn(int theOn);                                     // 0x42a020
void DrawLoadingScreen();
int CrtRand();                                                   // the CRT's rand
void SaveProfiles();                                             // 0x40c900
void Screen_Redraw();                                            // 0x40d040
void Cursor_Show();                                              // 0x404d60
void Cursor_Set(int theCursor);                                  // 0x404d20: 1 the click, 2 the clock, else the arrow
void Cursors_Init();                                             // 0x404d70: and the fonts' colour codes
long FileSize(const char* thePath);                              // 0x40c8c0: 0 when missing
void ResetProfiles();                                            // 0x40ca30: and saves them
void LoadProfiles();                                             // 0x40ccd0
int SortHighScores();                                            // 0x40ce10: and saves them
void GameInit();                                                 // 0x40ce40
void FreeAll();                                                  // 0x40d3a0
void GoToScreen();                                               // 0x40d3f0: the screen of kind gGoTo
void GameStep();                                                 // 0x40d6a0
void LoadGameFile();                                             // 0x41e9b0
// The port's: where cb.dat goes; a save is done (the web build keeps it); the account's name (0x429d30: the
// original's GetUserNameA), "" when there is none
const char* Sys_SavePath(const char* theName);
void Sys_SaveDone();
void Sys_UserName(char* theName, int theSize);

} // namespace cb

#endif
