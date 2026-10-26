#ifndef __COSMICBUGS_CBLEVEL_H__
#define __COSMICBUGS_CBLEVEL_H__

// Levels (0x40f170..0x413d10). A level is a screen of the game: a board to fill (kind 0) or one of the menus and
// screens (kinds 1..0x10), each with its lists of objects. The kinds a level holds are only partly translated:
// their structs name the members the level code uses so far.

#include "CBEmitter.h"
#include "CBGlobals.h"

namespace cb
{

// A cell of the board, 16 by 16 pixels (0xe4 bytes; the loader makes each column with new[], 0x4034f0 constructs
// a cell, 0x419780 destroys one)
struct Cell : Obj
{
	// kind +0x08: 0 open, 3 edge, 5 solid; state +0x0c: 6 inside the board's frame
	int row;                   // +0xb0
	int col;                   // +0xb4
	int unkb8;                 // +0xb8
	int unkbc;                 // +0xbc
	int unkc0;                 // +0xc0
	int unkc4;                 // +0xc4
	int unkc8;                 // +0xc8
	int unkcc;                 // +0xcc
	int unkd0;                 // +0xd0
	int unkd4;                 // +0xd4  180 when made, 360 when the level is won
	int unkd8;                 // +0xd8
	int unkdc;                 // +0xdc
	int unke0;                 // +0xe0
};
static_assert(sizeof(Cell) == 0xe4, "Cell");

// The board's side panel (+0x40c; its members are not named yet)
struct Panel : Obj
{
};

// 0x110 bytes (0x404fc0 constructs one)
struct Enemy : Obj
{
	uint8_t unkb0[0x5c];       // +0xb0
	Obj* rider;                // +0x10c
};
static_assert(sizeof(Enemy) == 0x110, "Enemy");

// The player's ship (0x4c4 bytes, 0x41d540 constructs one)
struct Shooter : Obj
{
	uint8_t unkb0[0x40];       // +0xb0
	List<Obj> beams;           // +0xf0
	uint8_t unkfc[0x38c];      // +0xfc
	float unk488;              // +0x488
	int flags;                 // +0x48c  0x10: invincible
	uint8_t unk490[0x34];      // +0x490
};
static_assert(sizeof(Shooter) == 0x4c4, "Shooter");

// A menu (0x1e4 bytes, 0x415690 constructs one)
struct Menu : Obj
{
	uint8_t unkb0[0x10];       // +0xb0
	List<Obj> items;           // +0xc0
	uint8_t unkcc[0x10c];      // +0xcc
	int active;                // +0x1d8
	uint8_t unk1dc[8];         // +0x1dc
};
static_assert(sizeof(Menu) == 0x1e4, "Menu");

// A text on the screen (0x407dd0 makes one)
struct Text : Obj
{
	Font* font;                // +0xb0
	char text[0x104];          // +0xb4
	int unk1b8;                // +0x1b8  how long it stays once clicked away
	int unk1bc;                // +0x1bc
	int unk1c0;                // +0x1c0
	int unk1c4;                // +0x1c4  clicked away together with its neighbours
};

// A node of the level's key map (0x14 bytes)
struct KeyMapNode
{
	KeyMapNode* next;          // +0x00
	KeyMapNode* prev;          // +0x04
	int a;                     // +0x08
	int b;                     // +0x0c
	char key;                  // +0x10
};

// 0x478 bytes (0x41e880 allocates one, 0x413620 constructs it)
struct Level : Obj
{
	// kind +0x08: 0 a board, 1 main_menu, 2 high_score_entry, 3 high_scores, 4 credits, 5 select_level,
	// 6 select_level2, 7 options, 8 opening_nag, 9 closing_nag, 10 expiring_nag, 0xb mission_complete, 0xc help,
	// 0xd help2, 0xe logo, 0x10 change_user; state +0x0c: 1 playing, 2 leaving, 3 won
	Cell* columns[50];               // +0x0b0
	char title[0x40];                // +0x178
	int number;                      // +0x1b8
	char backImage[0x40];            // +0x1bc
	Image* background;               // +0x1fc
	int cols;                        // +0x200
	int rows;                        // +0x204
	int gameOver;                    // +0x208
	Animation* hTiles;               // +0x20c
	int hTileCount;                  // +0x210
	Animation* vTiles;               // +0x214
	int vTileCount;                  // +0x218
	Animation* cornerTiles;          // +0x21c
	int cornerTileCount;             // +0x220
	int hPattern[50];                // +0x224
	int hPatternCount;               // +0x2ec
	int vPattern[37];                // +0x2f0
	int vPatternCount;               // +0x384
	int numWalkers;                  // +0x388
	int numRollers;                  // +0x38c
	int seed;                        // +0x390  a random board's
	int unk394;                      // +0x394
	int clicked;                     // +0x398
	int rightClicked;                // +0x39c  or space
	Image* fillImage;                // +0x3a0
	Image* beamH;                    // +0x3a4
	Image* beamV;                    // +0x3a8
	Image* beamMiddle;               // +0x3ac
	Image* empty;                    // +0x3b0
	int totalCells;                  // +0x3b4  the open cells at the start
	int filledCells;                 // +0x3b8
	int fillQuota;                   // +0x3bc  percent
	int unk3c0;                      // +0x3c0
	int fillSound;                   // +0x3c4
	int fillEndSound;                // +0x3c8
	int symmetry;                    // +0x3cc  a random board's
	int time;                        // +0x3d0
	int unk3d4;                      // +0x3d4
	int bonus;                       // +0x3d8
	int unk3dc;                      // +0x3dc
	int unk3e0;                      // +0x3e0
	int musicPending;                // +0x3e4
	List<Shooter> shooters;          // +0x3e8
	List<Obj> fills;                 // +0x3f4
	List<ParticleSystem> systems;    // +0x400
	List<Panel> panels;              // +0x40c
	List<Enemy> enemies;             // +0x418  the current one is the one the game file's tags go to
	List<KeyMapNode> keyMap;         // +0x424
	List<Obj> unk430;                // +0x430
	List<Obj> stars;                 // +0x43c
	List<Text> texts;                // +0x448
	List<Menu> menus;                // +0x454
	List<Obj> pointers;              // +0x460
	List<Obj> unk46c;                // +0x46c
};
static_assert(sizeof(Level) == 0x478, "Level");

// 0x473138: the game file's levels; the current one is the level being played (the code's gLevel)
extern List<Level> gLevels;
// 0x473144: the game file's enemies outside any level
extern List<Enemy> gEnemyDefs;

Level* Level_Init(Level* theLevel);                              // 0x413620
void Level_Destroy(Level* theLevel);                             // 0x413730
// 0x412050: the level tag in theTag, the game file read on from p (null: theTag is all there is, the board is
// random and its cells are not made)
void Level_Load(Level* theLevel, const char* p, XmlTag* theTag);
int Level_Update(Level* theLevel);                               // 0x412960: 0 when it is left
void Level_SetBackground(Level* theLevel);                       // 0x40f170: and the song, by the level number
void Level_StartSong(Level* theLevel, int thePlay);              // 0x40f520
void Level_MakeRandom(Level* theLevel, int theSeed);             // 0x40f580
// 0x40fb00: a level number's enemies (walkers, rollers and two more kinds)
void Level_EnemyCounts(Level* theLevel, int* theA, int* theB, int* theC, int* theD);
void Level_MakeEdges(Level* theLevel);                           // 0x40fe20
void Level_SetPatterns(Level* theLevel, const char* theH, const char* theV); // 0x40fe90
void Level_LoadFillImages(Level* theLevel, const XmlTag* theTag); // 0x40ff30
void Level_DefaultFillImages(Level* theLevel);                   // 0x4100a0
int PortalCount();                                               // 0x4101f0
void Level_RedrawCells(Level* theLevel);                         // 0x410250
void Level_DrawTrialTime(Level* theLevel);                       // 0x410340
void DrawPaused();                                               // 0x410450
int Level_IsPlaying(Level* theLevel);                            // 0x410660
void Level_AddKey(Level* theLevel, char theKey, int theA, int theB); // 0x410c00
void Level_FindKey(Level* theLevel, char theKey, int* theA, int* theB); // 0x410c40
void Level_AddPointer(Level* theLevel);                          // 0x410cc0
void Level_UnlinkAll(Level* theLevel);                           // 0x410ad0
void Cells_Delete(Cell* theCells);
void Level_UpdateProgressStars();                                // 0x413990
void Level_SetUserName(const char* theName);                     // 0x413a90
int Pointer_Update(Obj* thePointer);                             // 0x413bf0
void Pointer_Draw(Obj* thePointer);                              // 0x413ce0

// The kinds' code the level calls (translated with each kind)
Cell* Cells_New();                                               // new[] of 37, each 0x4034f0
void Cell_Set(Cell* theCell, char theCode, int theCol, int theRow); // 0x404440: from a row's cells attribute
void Cell_StartFill(Cell* theCell);                              // 0x404370
int Cell_Touches(Cell* theCell, int theKind);                    // 0x403ee0
void Cell_MakeEdge(Cell* theCell);                               // 0x404080
void Cell_Redraw(Cell* theCell);                                 // 0x403540
int Fill_Update(Obj* theFill);                                   // 0x404cf0
void Panel_Append(List<Panel>* theList);                         // 0x411ea0
void Panel_Setup(Panel* thePanel);                               // 0x417ef0
void Panel_FromXml(Panel* thePanel, const char* p, const XmlTag* theTag); // 0x417d40
void Panel_Draw(Panel* thePanel, int theAll);                    // 0x418080
void Panel_Update(Panel* thePanel);                              // 0x418010
void Shooter_Insert(List<Shooter>* theList);                     // 0x4107e0
Shooter* Shooter_Init(Shooter* theShooter);                      // 0x41d540
void Shooter_Setup(Shooter* theShooter);                         // 0x41b0f0
void Shooter_FromXml(Shooter* theShooter, const char* p, const XmlTag* theTag); // 0x41aec0
void Shooter_Update(Shooter* theShooter);                        // 0x41e200
void Beams_Fn411900(List<Obj>* theList);                         // 0x411900
void Enemy_Insert(List<Enemy>* theList);                         // 0x4106b0
Enemy* Enemy_Init(Enemy* theEnemy);                              // 0x404fc0
void Enemy_FromXml(Enemy* theEnemy, const char* p, const XmlTag* theTag); // 0x405040
int Enemy_Update(Enemy* theEnemy);                               // 0x40ab70
void Menu_Append(List<Menu>* theList);                           // 0x410750
Menu* Menu_Init(Menu* theMenu);                                  // 0x415690
void Menu_FromXml(Menu* theMenu, const char* p, const XmlTag* theTag); // 0x415310
int Menu_Update(Menu* theMenu);                                  // 0x417c10
void Pointer_Append(List<Obj>* theList);                         // 0x4109d0
void Pointer_Insert(List<Obj>* theList);                         // 0x410920
void Pointer_FromXml(Obj* thePointer, const char* p, const XmlTag* theTag); // 0x40d860
void Text_Append(List<Text>* theList);                           // 0x407dd0
int Text_FromXml(Text* theText, const char* p, const XmlTag* theTag); // 0x41f950
void Text_Make(Text* theText, const char* theString, Font* theFont, int theX, int theY, int theA, int theB,
	int theC, int theD, int theE, int theF);                     // 0x41f0e0
int Text_Update(Text* theText);                                  // 0x41f1d0
void Star_Append(List<Obj>* theList);                            // 0x4111a0
int Star_Update(Obj* theStar);                                   // 0x41e2c0
void Level_AddStars(Level* theLevel);                            // 0x411440
int Fn419770(Obj* theObj);                                       // 0x419770
int Fn41ae10(Obj* theObj);                                       // 0x41ae10
void Level_UpdateRandom(Level* theLevel);                        // 0x4119c0
int Level_UpdateLeaving(Level* theLevel);                        // 0x411bc0
int Level_EnemiesLeft(Level* theLevel);                          // 0x4110e0

} // namespace cb

#endif
