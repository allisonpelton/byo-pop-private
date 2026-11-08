#ifndef __COSMICBUGS_CBLEVEL_H__
#define __COSMICBUGS_CBLEVEL_H__

// Levels (0x40f170..0x413d10). A level is a screen of the game: a board to fill (kind 0) or one of the menus and
// screens (kinds 1..0x10), each with its lists of objects. The kinds a level holds are only partly translated:
// their structs name the members the level code uses so far.

#include "CBEmitter.h"
#include "CBGlobals.h"
#include "R64TextInput.h"

namespace cb
{

// A cell of the board, 16 by 16 pixels (0xe4 bytes; the loader makes each column with new[], 0x4034f0 constructs
// a cell, 0x419780 destroys one)
struct Cell : Obj
{
	// kind +0x08: 0 open, 1 and 2 a beam's, 3 edge, 4 filled, 5 solid; state +0x0c: 3 still, 5 being captured,
	// 6 waiting for the board's wave, 7 in the wave (which builds the board at the start, and takes it down when won)
	int row;                   // +0xb0
	int col;                   // +0xb4
	int step;                  // +0xb8  of the capture or the wave
	int fromKind;              // +0xbc  a capture spreads to the neighbours of this kind
	int toKind;                // +0xc0  and makes them this kind
	int offX;                  // +0xc4  where it slides in from (capture) or out to (wave)
	int offY;                  // +0xc8
	int shadowRight;           // +0xcc  the left neighbour's shadow is drawn on it
	int shadowBelow;           // +0xd0  the upper neighbour's
	int steps;                 // +0xd4  180 when made, 360 when the level is won
	int firstParticle;         // +0xd8
	int lastParticle;          // +0xdc
	int unmarked;              // +0xe0  0 once the last flood reached it
};
static_assert(sizeof(Cell) == 0xe4, "Cell");

// The board's panel along the bottom (+0x40c; CBPowerup.cpp): the fill percentage, level, score and lives in
// their fonts at their places, and its items (the logo) drawn over it
struct Panel : Obj
{
	int scoreX;                // +0xb0
	int scoreY;                // +0xb4
	int percentX;              // +0xb8
	int percentY;              // +0xbc
	int livesX;                // +0xc0
	int livesY;                // +0xc4
	int levelX;                // +0xc8
	int levelY;                // +0xcc
	Font* percentFont;         // +0xd0
	Font* livesFont;           // +0xd4
	Font* scoreFont;           // +0xd8
	Font* levelFont;           // +0xdc
	List<Obj> items;           // +0xe0
};
static_assert(sizeof(Panel) == 0xec, "Panel");
void Panel_Destroy(Panel* thePanel);                             // 0x411e30
void List_DeleteNode(Panel* thePanel);                           // 0x411fd0's delete

struct Beam;

// An enemy (0x110 bytes, 0x404fc0 constructs one). The game file's enemy tags make a level's enemies, and outside
// a level the defaults (gEnemyDefs) that random boards copy. kind +0x08: 0 roller, 1 burst_roller, 2 spin_roller,
// 3 turn_roller, 4 bump_roller, 5 snake_shooter, 7 line_walker, 8 scurry_walker, 9 smart_walker, 10 cannon_walker,
// 0xb laser_walker, 0xf frog, 0x10 teleporter_roller, 0x11 jellyfish_roller, 0x13 crab_walker, 0x14 turtle_walker,
// 0x15 lobster_walker, and the shots: 6 a snake shooter's snake, 0xc a cannon ball, 0xd a laser shot; state +0x0c:
// 0 appearing, 1 and 2 dying, 3 about
struct Enemy : Obj
{
	Animation* leftAnim;       // +0xb0
	Animation* rightAnim;      // +0xb4
	Animation* upAnim;         // +0xb8  Enemy_Face shows it going down the screen (and downAnim going up)
	Animation* downAnim;       // +0xbc
	int phase;                 // +0xc0  a spin roller's, a frog's, a teleporter's
	int firstParticle;         // +0xc4
	int lastParticle;          // +0xc8
	int shotDelay;             // +0xcc  2000..3000 when made: a snake shooter's updates between shots
	int lastZap;               // +0xd0  the age it last made the electricity sound
	int lastTurn;              // +0xd4  the age a walker last turned at a junction
	int countdown;             // +0xd8  it dies when this comes down to 1
	float baseSpeed;           // +0xdc  the game file's speed
	float baseAcceleration;    // +0xe0  the game file's acceleration
	float baseFrameTime;       // +0xe4  the game file's frame time
	float acceleration;        // +0xe8
	float decay;               // +0xec
	float turnSpeed;           // +0xf0
	float lastShot;            // +0xf4  the age it last shot (a smart walker: last turned to the ship)
	float prevVx;              // +0xf8  the speed after the last update (bump rollers, turtles)
	float prevVy;              // +0xfc
	Beam* lastBeam;            // +0x100  the beam it touched last
	Cell* hitCell;             // +0x104  the cell it bounced off last
	Obj* shield;               // +0x108  the flash where a beam touches it
	Obj* ice;                  // +0x10c  an ice capsule; while there is one the enemy does not move
};
static_assert(sizeof(Enemy) == 0x110, "Enemy");

struct Shooter;
struct Rider;

// A beam the ship fires (0x17c bytes, 0x41dad0 constructs one): its head (x, y) runs from where it was fired until
// the cell ahead is not open
struct Beam : Obj
{
	float startX;              // +0xb0
	float startY;              // +0xb4
	Shooter* owner;            // +0xb8
	int hit;                   // +0xbc
	List<Rider> riders;        // +0xc0
	Obj unkcc;                 // +0xcc
};
static_assert(sizeof(Beam) == 0x17c, "Beam");

// A beamrider: an enemy's shot that runs back along a beam to the ship (0xc0 bytes, 0x407d40 appends one)
struct Rider : Obj
{
	Beam* beam;                // +0xb0
	Enemy* enemy;              // +0xb4  the one that sent it
	int firstImage;            // +0xb8  its trail's particles
	int lastImage;             // +0xbc
};
static_assert(sizeof(Rider) == 0xc0, "Rider");

// The player's ship (0x4c4 bytes, 0x41d540 constructs one; CBShooter.cpp). state +0x0c: 1 hit, 3 under the
// player's control, 4 coming in, 5 firing, 6 dying
struct Shooter : Obj
{
	char vertName[0x20];       // +0xb0  the vertical animation's name (each new life starts with it)
	char horizName[0x20];      // +0xd0
	List<Beam> beams;          // +0xf0  while firing, the pair: the head goes down or right
	Animation* horizAnim;      // +0xfc
	Animation* vertAnim;       // +0x100
	Animation* horizShootAnim; // +0x104
	Animation* vertShootAnim;  // +0x108
	int unk10c;                // +0x10c
	Obj wingsH;                // +0x110  drawn over the ship while no weapon orbits it
	Obj wingsV;                // +0x1c0
	Obj sweeper;               // +0x270  (flag 4) at both ends of the open span, pushing enemies off its line
	Obj unk320;                // +0x320
	Obj bubble;                // +0x3d0  (flag 0x10) the invincibubble
	int beamAcross;            // +0x480  1: the beams go left and right
	float appearTime;          // +0x484  updates the ship takes to come in
	float beamSpeed;           // +0x488
	int flags;                 // +0x48c  4: the sweeper, 8: it turns by itself, 0x10: invincible
	int unk490;                // +0x490
	int unk494;                // +0x494
	int drillSound;            // +0x498
	int fireAge;               // +0x49c  its age when it last fired
	int hitSound;              // +0x4a0  clank_sound: a beam meets a wall
	int shootSound;            // +0x4a4
	int particle;              // +0x4a8  particle_0's index in the art bank
	int noAutoTurn;            // +0x4ac  the player turned it, or the auto-turn option is off
	float extLeft;             // +0x4b0  the open span about the ship (0x41ba40)
	float extRight;            // +0x4b4
	float extTop;              // +0x4b8
	float extBottom;           // +0x4bc
	int cellPoints;            // +0x4c0  the score for each cell captured
};
static_assert(sizeof(Shooter) == 0x4c4, "Shooter");

struct Text;

// A menu's item (0x1dc bytes; 0x415270 appends one, 0x4151b0 inserts one; CBMenu.cpp). kind +0x08 what a click
// does (MenuItem_FromXml's types: 0 buy_now .. 0x23 a requester's no); it is drawn with its animation, else as its
// name in its font (with the setting it changes)
struct MenuItem : Obj
{
	char name[0x80];           // +0xb0  (localized; a profile slot's number, a key's character)
	char shown[0x80];          // +0x130  what was last drawn, without colour codes (the hover box's size)
	Font* font;                // +0x1b0
	Font* hotFont;             // +0x1b4  under the mouse (none: font)
	int hot;                   // +0x1b8  under the mouse
	int firstParticle;         // +0x1bc  the sparkles while it is hot
	int lastParticle;          // +0x1c0
	int autoClick;             // +0x1c4  clicked at its next update (HighScore_Entered)
	int box;                   // +0x1c8  a frame is drawn round it
	int boxSize;               // +0x1cc  (0: round the text)
	int showOnlyHot;           // +0x1d0  drawn only while hot
	int alwaysHot;             // +0x1d4
	int heightAdder;           // +0x1d8  makes the hover box taller
};
static_assert(sizeof(MenuItem) == 0x1dc, "MenuItem");

// A menu (0x1e4 bytes, 0x415690 constructs one, 0x4156e0 destroys it; CBMenu.cpp)
struct Menu : Obj
{
	Image* logo;               // +0xb0  logo_animation's first frame, drawn about (logoX, logoY)
	int logoX;                 // +0xb4
	int logoY;                 // +0xb8
	int unkbc;                 // +0xbc
	List<MenuItem> items;      // +0xc0
	int lockedLevel;           // +0xcc  shown with locked_animation until the player reaches this level
	char lockedAnim[0x80];     // +0xd0
	Text* entry;               // +0x150  a requester's name entry (one of the level's texts)
	Text* texts[0x20];         // +0x154  a requester's texts (the level's), removed with it
	int textCount;             // +0x1d4
	List<Menu> requesters;     // +0x1d8  over the menu (the newest first): while there is one, only it is used
};
static_assert(sizeof(Menu) == 0x1e4, "Menu");

// A text on the screen (0x407dd0 makes one, 0x410a80 destroys it; CBText.cpp). state +0x0c: 3 a high score's
// name being typed; else 4 dormant, 5 fading in, 6 shown, 7 fading out, 8 gone (it loops back to 5, or is removed).
// kind +0x08 what it shows and refreshes each update: 0 its text, 1 a high score's name, 2 its score, 3 the level,
// 5 shots fired, 6 lives lost, 7 bugs killed, 8 power-ups caught
struct Text : Obj
{
	Font* font;                // +0xb0
	char text[0x100];          // +0xb4  in capitals
	int fadeIn;                // +0x1b4  ms
	int fadeOut;               // +0x1b8  ms (also how long it takes once clicked away)
	int idle;                  // +0x1bc  ms shown
	int dormant;               // +0x1c0  ms before it fades in
	int loopTime;              // +0x1c4  ms gone before it fades in again (0: it is removed; CBLevel: clicked away with its neighbours)
	int align;                 // +0x1c8  0 left, 1 right, 2 centred on x
	int row;                   // +0x1cc  a high score's
	int blink;                 // +0x1d0  ms; not 0: blinks while shown
	int firstParticle;         // +0x1d4  the art bank images its bubbles are drawn with
	int lastParticle;          // +0x1d8
	int unk1dc;                // +0x1dc
	TextInput input;           // +0x1e0  a high score's name entry
};
static_assert(sizeof(Text) == 0x6c8, "Text");

// A power-up (0xd8 bytes; 0x418900 makes one of a random kind, 0x4186c0 destroys it; CBPowerup.cpp). It falls
// from where an enemy died until the ship (under the player's control) touches it. kind +0x08 the effect (1
// slow motion .. 0x11 a magnet, else a faster beam); state +0x0c 4 while it falls
struct Powerup : Obj
{
	float gravity;             // +0xb0  added to vy each update
	char text[0x20];           // +0xb4  shown when it is collected: a colour code and the effect's name
	Obj* attached;             // +0xd4  follows it (nothing here sets one)
};

// A weapon a power-up gives (0xd8 bytes, in the level's list at +0x46c; CBPowerup.cpp). It orbits the ship
// (state 3) until a click launches it (state 4) straight out the way it is from the ship. kind +0x08: 0 a mine,
// 1 a missile, 3 an ice shot, 4 a time bomb, 5 a magnet, 6 an anchor
struct Weapon : Obj
{
	int unkb0;                 // +0xb0
	float drag;                // +0xb4  the speed is multiplied by it each update once launched
	int unkb8;                 // +0xb8
	float launchSpeed;         // +0xbc
	int across;                // +0xc0  the ship's beamAcross at the last update
	int particle;              // +0xc4  particle_0's index in the art bank
	int tooClose;              // +0xc8  an enemy touches it: it cannot be launched
	int fuseSound;             // +0xcc
	int angle;                 // +0xd0  degrees about the ship
	float radius;              // +0xd4  its distance from the ship
};
static_assert(sizeof(Weapon) == 0xd8, "Weapon");
static_assert(sizeof(Powerup) == 0xd8, "Powerup");

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
	// 6 select_level2, 7 options, 0xb mission_complete, 0xc help,
	// 0xd help2, 0xe logo, 0x10 change_user; state +0x0c: 1 playing, 2 paused, 3 won
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
	int chain;                       // +0x394  enemies killed while the current capture fills, plus 1
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
	int shots;                       // +0x3d0  beams fired (the level's record keeps the fewest)
	int lines;                       // +0x3d4  beam pairs that have stopped
	int bonus;                       // +0x3d8
	int unk3dc;                      // +0x3dc
	int unk3e0;                      // +0x3e0
	int musicPending;                // +0x3e4
	List<Shooter> shooters;          // +0x3e8
	List<Cell> fills;                // +0x3f4  cells being filled (the columns own them)
	List<ParticleSystem> systems;    // +0x400
	List<Panel> panels;              // +0x40c
	List<Enemy> enemies;             // +0x418  the current one is the one the game file's tags go to
	List<KeyMapNode> keyMap;         // +0x424
	List<Powerup> powerups;          // +0x430
	List<Obj> stars;                 // +0x43c
	List<Text> texts;                // +0x448
	List<Menu> menus;                // +0x454
	List<Obj> pointers;              // +0x460
	List<Weapon> weapons;            // +0x46c
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
void DrawPaused();                                               // 0x410450
int Level_IsPlaying(Level* theLevel);                            // 0x410660
void Level_AddKey(Level* theLevel, char theKey, int theA, int theB); // 0x410c00
void Level_FindKey(Level* theLevel, char theKey, int* theA, int* theB); // 0x410c40
void Level_AddPointer(Level* theLevel);                          // 0x410cc0
void Level_UnlinkAll(Level* theLevel);                           // 0x410ad0
void Level_UpdateProgressStars();                                // 0x413990
void Level_SetUserName(const char* theName);                     // 0x413a90
int Pointer_Update(Obj* thePointer);                             // 0x413bf0
void Pointer_Draw(Obj* thePointer);                              // 0x413ce0

// Beams and beamriders (CBBeam.cpp)
void Beam_Setup(Beam* theBeam, Shooter* theOwner, float theX, float theY, float theVx, float theVy); // 0x402850
void Rider_Setup(Rider* theRider, Beam* theBeam, float theX, float theY, Enemy* theEnemy); // 0x402940
int Rider_Update(Rider* theRider);                               // 0x402a00: 0 when it is done
void Beam_DrawTo(Beam* theBeam, uint8_t* theDst, int theDPitch); // 0x402cd0
void Beam_Draw(Beam* theBeam);                                   // 0x403130
int Beam_Update(Beam* theBeam);                                  // 0x403150: 0 once it has hit
void Beam_Destroy(Beam* theBeam);                                // 0x403410
void List_DeleteNode(Beam* theBeam);                             // 0x4112c0's delete
Beam* Beam_Init(Beam* theBeam);                                  // 0x41dad0
void Rider_Append(List<Rider>* theList);                         // 0x407d40
Beam* Obj_TouchedBeam(Obj* theObj, int theWDiv, int theHDiv);   // 0x40ec90: the ship's beam it touches

// The kinds' code the level calls (translated with each kind)
// Cells (CBCell.cpp)
// A column: the original's new[] of 37, each constructed by 0x4034f0 (the port adds a 38th, see Cells_New)
Cell* Cells_New();
void Cells_Delete(Cell* theCells);
Cell* Cell_Init(Cell* theCell);                                  // 0x4034f0
void Cell_Redraw(Cell* theCell);                                 // 0x403540
uint32_t Cell_SameNeighbours(Cell* theCell);                     // 0x403df0: bit 4 + 3 dy + dx
uint32_t Cell_Touches(Cell* theCell, int theKind);               // 0x403ee0: the same for neighbours of a kind
void Cell_Flood(Cell* theCell);                                  // 0x403fb0: marks the open region, counting it
int Cell_PickImage(Cell* theCell);                               // 0x404080: by the neighbours of its kind
void Cell_StartCapture(Cell* theCell, Image* theImage, int theFrom, int theTo); // 0x404290
void Cell_StartFill(Cell* theCell);                              // 0x404370: the wave reaches it
void Cell_Set(Cell* theCell, char theCode, int theCol, int theRow); // 0x404440: from a row's cells attribute
void Cell_FillNext(Cell* theCell);                               // 0x4045e0: the wave goes on to a neighbour
void Cell_SpreadCapture(Cell* theCell);                          // 0x404680
int Cell_UpdateCapture(Cell* theCell);                           // 0x4047d0: 0 when done
int Cell_UpdateFill(Cell* theCell);                              // 0x404a10: 0 when done
int Fill_Update(Cell* theCell);                                  // 0x404cf0: 0 when done
void Panel_Append(List<Panel>* theList);                         // 0x411ea0
void Panel_Setup(Panel* thePanel);                               // 0x417ef0
void Panel_FromXml(Panel* thePanel, const char* p, const XmlTag* theTag); // 0x417d40
void Panel_Draw(Panel* thePanel, int theAll);                    // 0x418080
void Panel_Update(Panel* thePanel);                              // 0x418010
void Shooter_Insert(List<Shooter>* theList);                     // 0x4107e0
Shooter* Shooter_Init(Shooter* theShooter);                      // 0x41d540
void Shooter_Setup(Shooter* theShooter);                         // 0x41b0f0
void Shooter_FromXml(Shooter* theShooter, const char* p, const XmlTag* theTag); // 0x41aec0
int Shooter_Update(Shooter* theShooter);                         // 0x41e200
void Shooter_Hit(Shooter* theShooter, float theVx, float theVy);  // 0x41bee0: a beamrider reached it
void Beams_RemoveAll(List<Beam>* theList);                       // 0x411900
void Enemy_Insert(List<Enemy>* theList);                         // 0x4106b0
// Enemies (CBEnemy.cpp)
Enemy* Enemy_Init(Enemy* theEnemy);                              // 0x404fc0
void Enemy_FromXml(Enemy* theEnemy, const char* p, const XmlTag* theTag); // 0x405040
void Enemy_Draw(Enemy* theEnemy);                                // 0x4055b0
int Enemy_Bounce(Enemy* theEnemy);                               // 0x4055e0: off cells that are not open
int BurstRoller_Move(Enemy* theEnemy);                           // 0x405750
int Jellyfish_Move(Enemy* theEnemy);                             // 0x4057c0
int BumpRoller_Move(Enemy* theEnemy);                            // 0x405970
void SpinRoller_Move(Enemy* theEnemy);                           // 0x406560
int Frog_Move(Enemy* theEnemy);                                  // 0x406790
int LaserShot_Move(Enemy* theEnemy);                             // 0x4069b0: 0 when it has burst
int Enemy_NearCapture(Enemy* theEnemy);                          // 0x406ae0
void Enemy_Face(Enemy* theEnemy);                                // 0x406ba0
void Enemy_UpdateShield(Enemy* theEnemy);                        // 0x406c00
void Enemy_Shield(Enemy* theEnemy);                              // 0x406c60
void Enemy_Freeze(Enemy* theEnemy);                              // 0x406d70
void Enemy_UpdateIce(Enemy* theEnemy);                           // 0x406ea0
int Enemy_ClearOfRollers(Enemy* theEnemy);                       // 0x406fc0
int Enemy_ClearOfWalkers(Enemy* theEnemy);                       // 0x407020
int Enemy_Place(Enemy* theEnemy);                                // 0x407080: 0 when there is no room
void Enemy_Append(List<Enemy>* theList);                         // 0x4072b0
void Powerup_Insert(List<Powerup>* theList);                     // 0x407340
Text* Text_Init(Text* theText);                                  // 0x4073e0
void Enemy_Destroy(Enemy* theEnemy);                             // 0x4074b0
void List_DeleteNode(Enemy* theEnemy);                           // 0x401080's delete (0x407430 is plain objects')
void TurnRoller_Move(Enemy* theEnemy);                           // 0x407530
void Walker_Turn(Enemy* theEnemy);                               // 0x407700
int LaserShot_HitBeams(Enemy* theEnemy);                         // 0x4078e0
int Enemy_PlaceFirst(Enemy* theEnemy);                           // 0x407b30: 0 when there is no room
void Text_Append(List<Text>* theList);                           // 0x407dd0
int Teleporter_Move(Enemy* theEnemy);                            // 0x407e60
void Walker_TurnAtJunction(Enemy* theEnemy, int theKind);         // 0x4083f0
void Walker_Follow(Enemy* theEnemy, int theKind);                 // 0x4086c0
void Enemy_CopyDef(Enemy* theEnemy, Enemy* theDef);              // 0x408900
void Enemy_Kill(Enemy* theEnemy, int thePoints);                 // 0x408a00
int SnakeShooter_Move(Enemy* theEnemy);                          // 0x408c70
int Snake_Move(Enemy* theEnemy);                                 // 0x408ff0
int CannonBall_Move(Enemy* theEnemy);                            // 0x4090a0: 0 when it has burst
int Walker_Move(Enemy* theEnemy, int theKind);                   // 0x409150
void ScurryWalker_Move(Enemy* theEnemy);                         // 0x409210
void SmartWalker_Move(Enemy* theEnemy);                          // 0x409310
void CannonWalker_Move(Enemy* theEnemy);                         // 0x4095d0
void LaserWalker_Move(Enemy* theEnemy);                          // 0x409ac0
int CrabWalker_Move(Enemy* theEnemy);                            // 0x409e60
int LobsterWalker_Move(Enemy* theEnemy);                         // 0x40a100
int TurtleWalker_Move(Enemy* theEnemy);                          // 0x40a1f0
int Enemy_Step(Enemy* theEnemy);                                 // 0x40a650
int Enemy_UpdateAppearing(Enemy* theEnemy);                      // 0x40aae0
int Enemy_Update(Enemy* theEnemy);                               // 0x40ab70
// Called from the enemies, translated with their own parts
void Obj_Explode(Obj* theObj, float theVx, float theVy);         // 0x40de60: its death animation and a burst
void Level_Blast(int theCol, int theRow, int theRadius);         // 0x40e4d0: holes the filled cells round a cell
void Shooter_DropBeams(Shooter* theShooter);                     // 0x41d380
Powerup* Powerup_Init(Powerup* thePowerup);                      // 0x418900
void List_DeleteNode(Powerup* thePowerup);                       // 0x4186c0 and the memory
void Menu_Append(List<Menu>* theList);                           // 0x410750
Menu* Menu_Init(Menu* theMenu);                                  // 0x415690
void Menu_Destroy(Menu* theMenu);                                // 0x4156e0
void List_DeleteNode(Menu* theMenu);                             // 0x411340's delete
void Menu_FromXml(Menu* theMenu, const char* p, const XmlTag* theTag); // 0x415310
void Menu_DrawToBackground(Menu* theMenu);                       // 0x413b30: its image and logo, about their middles
int Menu_Update(Menu* theMenu);                                  // 0x417c10: 0 when its screen is done
void Menu_ShowRequester(Menu* theMenu, int theKind, const char* theText, const char* theSecond); // 0x415900: 0 delete the player, 1 a message, 2 a name, 3 quit, 4 a board's end, 5 resume
void Pointer_Append(List<Obj>* theList);                         // 0x4109d0
void Pointer_Insert(List<Obj>* theList);                         // 0x410920
int Obj_FromXml(Obj* theObj, const char* p, const XmlTag* theTag);     // 0x40d860: any object's attributes; 0 if its animation is missing
int Text_FromXml(Text* theText, const char* p, const XmlTag* theTag); // 0x41f950
void Text_Make(Text* theText, const char* theString, Font* theFont, int theX, int theY, int theDormant, int theFadeIn,
	int theIdle, int theFadeOut, int theLoopTime, int theBlink);  // 0x41f0e0
void Text_Destroy(Text* theText);                                // 0x410a80
void List_DeleteNode(Text* theText);                             // 0x411920's delete
void HighScore_Entered(const char* theName);                     // 0x40cf70: the name entry's return
int Text_Update(Text* theText);                                  // 0x41f1d0
int Star_Update(Obj* theStar);                                   // 0x41e2c0
void Level_AddStars(Level* theLevel);                            // 0x411440
int Powerup_Update(Powerup* thePowerup);                         // 0x419770: 0 when it is gone
int Weapon_Update(Weapon* theWeapon);                            // 0x41ae10: 0 when it is gone
void Level_StartRandom(Level* theLevel);                         // 0x4119c0: a random board, on its first update
int Level_UpdatePaused(Level* theLevel);                         // 0x411bc0
void Shooter_Draw(Shooter* theShooter);                          // 0x41d430
void Weapon_Draw(Weapon* theWeapon);                             // 0x419790: a time bomb's fuse too
void Weapon_DrawTooClose(Weapon* theWeapon);                     // 0x419880
void Powerup_Draw(Powerup* thePowerup);                          // 0x418420
void Star_Draw(Obj* theStar);                                    // 0x41e310
void Menu_Draw(Menu* theMenu);                                   // 0x4154d0
void Text_Draw(Text* theText);                                   // 0x41f6f0
void Level_Draw(Level* theLevel);                                // 0x410d10
int Level_EnemiesLeft(Level* theLevel);                          // 0x4110e0
void Star_Append(List<Obj>* theList);                            // 0x4111a0
void Level_AddRandomEnemies(Level* theLevel, int theFirst, int theSecond); // 0x411500
void Level_ShowStart(Level* theLevel);                           // 0x4117c0: the level number and quota
void Level_GameOver(Level* theLevel);                            // 0x411c10

} // namespace cb

#endif
