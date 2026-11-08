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
extern int gTrialSlot;                     // 0x460044  cb.dat's third field (the trial's ms left in the original), kept so the file stays the original's
extern int gLives;                         // 0x460040
extern float gBeamSpeed;                   // 0x460038  the ship's beams' speed at the start
extern int gDifficulty;                    // 0x46e5e0  -1, 0 or 1 (the options' cycle)
extern int gAutoFire;                      // 0x460028  the shooter mode: 0 manual, 1 auto
extern int gQuit;                          // 0x46e5d0
extern int gFirstRun;                      // 0x46e5d4  cb.dat was missing
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

// The game's texts, read from the player's exe by address (CBGameText.cpp): false when none could be
const char* GT(uint32_t theVA);
#define GTA(theVA) (theVA)
bool LoadGameTexts();
const char* Localize(const char* theText);                       // 0x420e10
Font* Font_Find(const char* theName);                            // 0x420850

// The game's sounds (0x473150): SoundList_Load and SoundList_Find on them
extern SoundList gSounds;
int Sound_Load(const char* thePath);
int Sound_Find(const char* thePath);
int CrtRand();                                                   // 0x444f09: the CRT's rand
void CrtSrand(unsigned theSeed);                                 // 0x444efc
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
bool ChangeScreen();                                             // GameStep's leaving of a screen for gGoTo's; false when none is left
void GameStep();                                                 // 0x40d6a0
void GameDraw();                                                 // 0x40ce80
// The start (CBStart.cpp)
extern char gGameFile[0x100];              // 0x4622e0  the game file read next
extern Font gFontBase;                     // 0x4700e0  (loaded, never drawn)
// 0x46e394: the screen locked for drawing (its pitch and pixels are gScreenPitch and gScreenBits, which
// Screen_Lock sets, as LockScreen on it does)
extern ScreenLock gScreenLock;
int Screen_Lock();
void LoadArt8();                                                 // 0x41e4b0
void LoadArtAndFonts();                                          // 0x41e4d0
void DrawLoadingScreen();                                        // 0x41e560
void LoadGameFile();                                             // 0x41e9b0
// 0x41ee60, the game's main: GameStart is what comes before its loop, GameFrame one pass of the loop (0 when the
// game quits), GameEnd what follows it
void GameStart();
int GameFrame();
void GameEnd();
int Font_LoadXml(const char* p, const XmlTag* theTag);           // 0x420ba0: a font tag of the game file

} // namespace cb

#endif
