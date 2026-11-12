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

// What a cell is (Cell::mKind): open, the two a beam makes (1 where a growing beam crosses it, which the capture turns
// into 2, the line the beam leaves, that the enemies bounce off and the walkers follow), the edge beside a filled
// cell, filled, or solid (part of the board's frame)
enum CellKind
{
	CELL_OPEN = 0,
	CELL_BEAM = 1,
	CELL_LINE = 2,
	CELL_EDGE = 3,
	CELL_FILLED = 4,
	CELL_SOLID = 5,
};

// A cell's life (Cell::mState): still, being captured (the flood spreads from it), waiting for the board's wave, or in
// the wave (which builds the board at the start, and takes it down when won)
enum CellState
{
	CELL_STILL = 3,
	CELL_CAPTURING = 5,
	CELL_WAITING = 6,
	CELL_IN_WAVE = 7,
};

// A cell of the board, 16 by 16 pixels (0xe4 bytes; the loader makes each column with new[], 0x4034f0 constructs
// a cell, 0x419780 destroys one)
struct Cell : Obj
{
	// mKind +0x08 a CellKind, mState +0x0c a CellState
	int mRow;                   // +0xb0
	int mCol;                   // +0xb4
	int mStep;                  // +0xb8  of the capture or the wave
	int mFromKind;              // +0xbc  a capture spreads to the neighbours of this kind
	int mToKind;                // +0xc0  and makes them this kind
	int mOffX;                  // +0xc4  where it slides in from (capture) or out to (wave)
	int mOffY;                  // +0xc8
	int mShadowRight;           // +0xcc  the left neighbour's shadow is drawn on it
	int mShadowBelow;           // +0xd0  the upper neighbour's
	int mSteps;                 // +0xd4  180 when made, 360 when the level is won
	int mFirstParticle;         // +0xd8
	int mLastParticle;          // +0xdc
	int mUnmarked;              // +0xe0  0 once the last flood reached it
};
static_assert(sizeof(Cell) == 0xe4, "Cell");

// The board's panel along the bottom (+0x40c; CBPowerup.cpp): the fill percentage, level, score and lives in
// their fonts at their places, and its items (the logo) drawn over it
struct Panel : Obj
{
	int mScoreX;                // +0xb0
	int mScoreY;                // +0xb4
	int mPercentX;              // +0xb8
	int mPercentY;              // +0xbc
	int mLivesX;                // +0xc0
	int mLivesY;                // +0xc4
	int mLevelX;                // +0xc8
	int mLevelY;                // +0xcc
	Font* mPercentFont;         // +0xd0
	Font* mLivesFont;           // +0xd4
	Font* mScoreFont;           // +0xd8
	Font* mLevelFont;           // +0xdc
	List<Obj> mItems;           // +0xe0
};
static_assert(sizeof(Panel) == 0xec, "Panel");
void Panel_Destroy(Panel* thePanel);                             // 0x411e30
void List_DeleteNode(Panel* thePanel);                           // 0x411fd0's delete

struct Beam;

// What an enemy is (Enemy::mKind; Enemy_FromXml's tags, Enemy_Update's switch): the rollers, the walkers, the frog, and
// the shots (a snake shooter's snake, a cannon ball, a laser shot). The numbers are the original's, with none at 0xe,
// 0x12 or past 0x15.
enum EnemyKind
{
	ENEMY_ROLLER = 0,
	ENEMY_BURST_ROLLER = 1,
	ENEMY_SPIN_ROLLER = 2,
	ENEMY_TURN_ROLLER = 3,
	ENEMY_BUMP_ROLLER = 4,
	ENEMY_SNAKE_SHOOTER = 5,
	ENEMY_SNAKE = 6,
	ENEMY_LINE_WALKER = 7,
	ENEMY_SCURRY_WALKER = 8,
	ENEMY_SMART_WALKER = 9,
	ENEMY_CANNON_WALKER = 10,
	ENEMY_LASER_WALKER = 0xb,
	ENEMY_CANNON_BALL = 0xc,
	ENEMY_LASER_SHOT = 0xd,
	ENEMY_FROG = 0xf,
	ENEMY_TELEPORTER_ROLLER = 0x10,
	ENEMY_JELLYFISH_ROLLER = 0x11,
	ENEMY_CRAB_WALKER = 0x13,
	ENEMY_TURTLE_WALKER = 0x14,
	ENEMY_LOBSTER_WALKER = 0x15,
};

// An enemy's life (Enemy::mState): it fades in with the cell under it, goes about, and dies (slowing down while its
// dying animation plays, then fading out); a won board fades the enemies out at once
enum EnemyState
{
	ENEMY_APPEARING = 0,
	ENEMY_DYING = 1,
	ENEMY_FADING = 2,
	ENEMY_ALIVE = 3,
};

// An enemy (0x110 bytes, 0x404fc0 constructs one). The game file's enemy tags make a level's enemies, and outside
// a level the defaults (gEnemyDefs) that random boards copy. mKind +0x08 an EnemyKind, mState +0x0c an EnemyState
struct Enemy : Obj
{
	Animation* mLeftAnim;       // +0xb0
	Animation* mRightAnim;      // +0xb4
	Animation* mUpAnim;         // +0xb8  Enemy_Face shows it going down the screen (and mDownAnim going up)
	Animation* mDownAnim;       // +0xbc
	int mPhase;                 // +0xc0  a spin roller's, a frog's, a teleporter's
	int mFirstParticle;         // +0xc4
	int mLastParticle;          // +0xc8
	int mShotDelay;             // +0xcc  2000..3000 when made: a snake shooter's updates between shots
	int mLastZap;               // +0xd0  the age it last made the electricity sound
	int mLastTurn;              // +0xd4  the age a walker last turned at a junction
	int mCountdown;             // +0xd8  it dies when this comes down to 1
	float mBaseSpeed;           // +0xdc  the game file's speed
	float mBaseAcceleration;    // +0xe0  the game file's acceleration
	float mBaseFrameTime;       // +0xe4  the game file's frame time
	float mAcceleration;        // +0xe8
	float mDecay;               // +0xec
	float mTurnSpeed;           // +0xf0
	float mLastShot;            // +0xf4  the age it last shot (a smart walker: last turned to the ship)
	float mPrevVx;              // +0xf8  the speed after the last update (bump rollers, turtles)
	float mPrevVy;              // +0xfc
	Beam* mLastBeam;            // +0x100  the beam it touched last
	Cell* mHitCell;             // +0x104  the cell it bounced off last
	Obj* mShield;               // +0x108  the flash where a beam touches it
	Obj* mIce;                  // +0x10c  an ice capsule; while there is one the enemy does not move
};
static_assert(sizeof(Enemy) == 0x110, "Enemy");

struct Shooter;
struct Rider;

// A beam the ship fires (0x17c bytes, 0x41dad0 constructs one): its head (x, y) runs from where it was fired until
// the cell ahead is not open
struct Beam : Obj
{
	float mStartX;              // +0xb0
	float mStartY;              // +0xb4
	Shooter* mOwner;            // +0xb8
	int mHit;                   // +0xbc
	List<Rider> mRiders;        // +0xc0
	Obj mHarpoon;                 // +0xcc  Water Bugs draws a tip at the head with it; here it is built and destroyed and nothing else uses it
};
static_assert(sizeof(Beam) == 0x17c, "Beam");

// A beamrider: an enemy's shot that runs back along a beam to the ship (0xc0 bytes, 0x407d40 appends one)
struct Rider : Obj
{
	Beam* mBeam;                // +0xb0
	Enemy* mEnemy;              // +0xb4  the one that sent it
	int mFirstImage;            // +0xb8  its trail's particles
	int mLastImage;             // +0xbc
};
static_assert(sizeof(Rider) == 0xc0, "Rider");

// The ship's life (Shooter::mState): hit (it blows up at the next update), idle (the board is won: nothing updates it
// any more), under the player's control, coming in, firing (its beams are out), dying
enum ShipState
{
	SHIP_HIT = 1,
	SHIP_IDLE = 2,
	SHIP_CONTROLLED = 3,
	SHIP_COMING_IN = 4,
	SHIP_FIRING = 5,
	SHIP_DYING = 6,
};

// The player's ship (0x4c4 bytes, 0x41d540 constructs one; CBShooter.cpp). mState +0x0c a ShipState
struct Shooter : Obj
{
	char mVertName[0x20];       // +0xb0  the vertical animation's name (each new life starts with it)
	char mHorizName[0x20];      // +0xd0
	List<Beam> mBeams;          // +0xf0  while firing, the pair: the head goes down or right
	Animation* mHorizAnim;      // +0xfc
	Animation* mVertAnim;       // +0x100
	Animation* mHorizShootAnim; // +0x104
	Animation* mVertShootAnim;  // +0x108
	int mWingsPadding;           // +0x10c  the four bytes before mWingsH: nothing reads them
	Obj mWingsH;                // +0x110  drawn over the ship while no weapon orbits it
	Obj mWingsV;                // +0x1c0
	Obj mSweeper;               // +0x270  (flag 4) at both ends of the open span, pushing enemies off its line
	Obj mIdleObj;                // +0x320  built and destroyed with the ship; nothing else uses it
	Obj mBubble;                // +0x3d0  (flag 0x10) the invincibubble
	int mBeamAcross;            // +0x480  1: the beams go left and right
	float mAppearTime;          // +0x484  updates the ship takes to come in
	float mBeamSpeed;           // +0x488
	int mFlags;                 // +0x48c  4: the sweeper, 8: it turns by itself, 0x10: invincible
	int mSlowMotionTime;         // +0x490  5000 once the slow-motion power-up is taken; nothing reads it
	int mHyperMotionTime;        // +0x494  5000 once the hyper-motion power-up is taken; nothing reads it
	int mDrillSound;            // +0x498
	int mFireAge;               // +0x49c  its age when it last fired
	int mHitSound;              // +0x4a0  clank_sound: a beam meets a wall
	int mShootSound;            // +0x4a4
	int mParticle;              // +0x4a8  particle_0's index in the art bank
	int mNoAutoTurn;            // +0x4ac  the player turned it, or the auto-turn option is off
	float mExtLeft;             // +0x4b0  the open span about the ship (0x41ba40)
	float mExtRight;            // +0x4b4
	float mExtTop;              // +0x4b8
	float mExtBottom;           // +0x4bc
	int mCellPoints;            // +0x4c0  the score for each cell captured
};
static_assert(sizeof(Shooter) == 0x4c4, "Shooter");

struct Text;

// What a click on a menu's item does (MenuItem::mKind; MenuItem_FromXml's types and the requesters' buttons, handled by
// MenuItem_Update's switch). The numbers are the original's; 0 was the buy button.
enum ItemKind
{
	ITEM_START = 1,
	ITEM_OPTIONS = 2,
	ITEM_QUIT = 3,
	ITEM_HELP = 4,
	ITEM_HELP2 = 5,
	ITEM_HIGH_SCORES = 7,
	ITEM_NEXT_LEVEL = 8,
	ITEM_HS_ENTERED = 9,
	ITEM_HS_STUFF = 10,
	ITEM_CREDITS = 0xb,
	ITEM_VIDEO = 0xc,
	ITEM_LCD = 0xd,
	ITEM_SOUND = 0xe,
	ITEM_MUSIC = 0xf,
	ITEM_DIFFICULTY = 0x10,
	ITEM_SHOOTER_MODE = 0x11,
	ITEM_SELECT_LEVEL = 0x12,
	ITEM_SELECT_UP = 0x13,
	ITEM_SELECT_DOWN = 0x14,
	ITEM_SELECT_SLIDE = 0x15,
	ITEM_SELECT_WORLD = 0x18,
	ITEM_BACK = 0x19,
	ITEM_CHANGE_USER = 0x1a,
	ITEM_USER_SLOT = 0x1b,
	ITEM_USER_RENAME = 0x1c,
	ITEM_NAME_OKAY = 0x1d,      // a name entry's okay: the player is named
	ITEM_USER_DELETE = 0x1e,
	ITEM_DELETE_OKAY = 0x1f,    // the delete's okay: the slot is emptied
	ITEM_CANCEL = 0x20,         // a requester's cancel (or okay, where there is only one button)
	ITEM_CONFIRM_QUIT = 0x21,
	ITEM_RESUME_YES = 0x22,     // the saved game is resumed
	ITEM_RESUME_NO = 0x23,      // a new game starts
};

// A menu's item (0x1dc bytes; 0x415270 appends one, 0x4151b0 inserts one; CBMenu.cpp). mKind +0x08 an ItemKind (what a
// click does); it is drawn with its animation, else as its name in its font (with the setting it changes)
struct MenuItem : Obj
{
	char mName[0x80];           // +0xb0  (localized; a profile slot's number, a key's character)
	char mShown[0x80];          // +0x130  what was last drawn, without colour codes (the hover box's size)
	Font* mFont;                // +0x1b0
	Font* mHotFont;             // +0x1b4  under the mouse (none: font)
	int mHot;                   // +0x1b8  under the mouse
	int mFirstParticle;         // +0x1bc  the sparkles while it is hot
	int mLastParticle;          // +0x1c0
	int mAutoClick;             // +0x1c4  clicked at its next update (HighScore_Entered)
	int mBox;                   // +0x1c8  a frame is drawn round it
	int mBoxSize;               // +0x1cc  (0: round the text)
	int mShowOnlyHot;           // +0x1d0  drawn only while hot
	int mAlwaysHot;             // +0x1d4
	int mHeightAdder;           // +0x1d8  makes the hover box taller
};
static_assert(sizeof(MenuItem) == 0x1dc, "MenuItem");

// What a requester (a menu over a menu) asks (the state of its Menu; Menu_ShowRequester's kind): to confirm deleting
// a player, a message to close, a name to type, to confirm leaving the game, the end of a board (its fill and shots
// against the records, and the bonus), or yes or no to resuming the saved game
enum RequesterKind
{
	REQUESTER_DELETE_PLAYER = 0,
	REQUESTER_MESSAGE = 1,
	REQUESTER_NAME = 2,
	REQUESTER_QUIT = 3,
	REQUESTER_BOARD_END = 4,
	REQUESTER_RESUME = 5,
};

// A menu (0x1e4 bytes, 0x415690 constructs one, 0x4156e0 destroys it; CBMenu.cpp)
struct Menu : Obj
{
	Image* mLogo;               // +0xb0  logo_animation's first frame, drawn about (mLogoX, mLogoY)
	int mLogoX;                 // +0xb4
	int mLogoY;                 // +0xb8
	int mItemsPadding;           // +0xbc  the four bytes before items: nothing reads them
	List<MenuItem> mItems;      // +0xc0
	int mLockedLevel;           // +0xcc  shown with locked_animation until the player reaches this level
	char mLockedAnim[0x80];     // +0xd0
	Text* mEntry;               // +0x150  a requester's name entry (one of the level's texts)
	Text* mTexts[0x20];         // +0x154  a requester's texts (the level's), removed with it
	int mTextCount;             // +0x1d4
	List<Menu> mRequesters;     // +0x1d8  over the menu (the newest first): while there is one, only it is used
};
static_assert(sizeof(Menu) == 0x1e4, "Menu");

// What a text shows and refreshes each update (Text::mKind): its own text, the name of a high score, its score, or one
// of the counts of the finished board
enum TextKind
{
	TEXT_PLAIN = 0,
	TEXT_HIGH_SCORE_NAME = 1,
	TEXT_HIGH_SCORE = 2,
	TEXT_LEVEL = 3,
	TEXT_SHOTS = 5,
	TEXT_LIVES_LOST = 6,
	TEXT_BUGS_KILLED = 7,
	TEXT_POWERUPS_CAUGHT = 8,
};

// A text's life (Text::mState): the name of a high score being typed, or dormant, fading in, shown, fading out and gone
// (it loops back to fading in, or is removed)
enum TextState
{
	TEXT_NAME_ENTRY = 3,
	TEXT_DORMANT = 4,
	TEXT_FADING_IN = 5,
	TEXT_SHOWN = 6,
	TEXT_FADING_OUT = 7,
	TEXT_GONE = 8,
};

// The idle time of a text that stays until it is clicked away (0x986fc8 ms: about three hours)
constexpr int TEXT_IDLE_FOREVER = 0x986fc8;

// A text on the screen (0x407dd0 makes one, 0x410a80 destroys it; CBText.cpp). mState +0x0c a TextState, mKind
// +0x08 a TextKind
struct Text : Obj
{
	Font* mFont;                // +0xb0
	char mText[0x100];          // +0xb4  in capitals
	int mFadeIn;                // +0x1b4  ms
	int mFadeOut;               // +0x1b8  ms (also how long it takes once clicked away)
	int mIdle;                  // +0x1bc  ms shown
	int mDormant;               // +0x1c0  ms before it fades in; nothing reads it (mLifetime starts from it)
	int mLoopTime;              // +0x1c4  ms gone before it fades in again (0: it is removed; CBLevel: clicked away with its neighbours)
	int mAlign;                 // +0x1c8  0 left, 1 right, 2 centred on x
	int mRow;                   // +0x1cc  a high score's
	int mBlink;                 // +0x1d0  ms; not 0: blinks while shown
	int mFirstParticle;         // +0x1d4  the art bank images its bubbles are drawn with
	int mLastParticle;          // +0x1d8
	int mInputPadding;           // +0x1dc  the four bytes before input: nothing reads them
	TextInput mInput;           // +0x1e0  a high score's name entry
};
static_assert(sizeof(Text) == 0x6c8, "Text");

// What a power-up does (Powerup::mKind; Powerup_Collect's switch): a weapon (the time bomb, the missile, the freeze, the
// mine), a change to the ship or the shots, the sweepers or the extra life. Any other number is the fast shot (as is
// kind 0). The test keys '0' to '9' and 'A' to 'D' make kinds 0 to 13.
enum PowerupKind
{
	POWERUP_FAST_SHOT = 0,
	POWERUP_SLOW_MOTION = 1,
	POWERUP_MULTIPLIER = 2,
	POWERUP_SLOW_SHOT = 3,
	POWERUP_TIME_BOMB = 4,
	POWERUP_HYPER_MOTION = 5,
	POWERUP_MISSILE = 6,
	POWERUP_FREEZE = 7,
	POWERUP_MINE = 8,
	POWERUP_SWEEPERS = 9,
	POWERUP_EXTRA_LIFE = 10,
	POWERUP_FLIP_FLOPPER = 0xc,
	POWERUP_INVINCIBUBBLE = 0xd,
};

// A power-up while it falls (Powerup::mState)
constexpr int POWERUP_FALLING = 4;

// A power-up (0xd8 bytes; 0x418900 makes one of a random kind, 0x4186c0 destroys it; CBPowerup.cpp). It falls
// from where an enemy died until the ship (under the player's control) touches it. mKind +0x08 a PowerupKind;
// mState +0x0c POWERUP_FALLING while it falls
struct Powerup : Obj
{
	float mGravity;             // +0xb0  added to vy each update
	char mText[0x20];           // +0xb4  shown when it is collected: a colour code and the effect's name
	Obj* mAttached;             // +0xd4  the original's follower of the power-up; nothing sets one, kept for the layout
};

// What a weapon is (Weapon::mKind): a mine, a missile, an ice shot (the freeze power-up) or a time bomb
enum WeaponKind
{
	WEAPON_MINE = 0,
	WEAPON_MISSILE = 1,
	WEAPON_ICE_SHOT = 3,
	WEAPON_TIME_BOMB = 4,
};

// A weapon orbits the ship until a click launches it (Weapon::mState)
enum WeaponState
{
	WEAPON_ORBITING = 3,
	WEAPON_LAUNCHED = 4,
};

// A weapon a power-up gives (0xd8 bytes, in the level's list at +0x46c; CBPowerup.cpp). It orbits the ship
// (WEAPON_ORBITING) until a click launches it (WEAPON_LAUNCHED) straight out the way it is from the ship. mKind +0x08
// a WeaponKind
struct Weapon : Obj
{
	int mDragPadding;            // +0xb0  the four bytes before drag: nothing reads them
	float mDrag;                // +0xb4  the speed is multiplied by it each update once launched
	int mLaunchSpeedPadding;     // +0xb8  the four bytes before mLaunchSpeed: nothing reads them
	float mLaunchSpeed;         // +0xbc
	int mAcross;                // +0xc0  the ship's mBeamAcross at the last update
	int mParticle;              // +0xc4  particle_0's index in the art bank
	int mTooClose;              // +0xc8  an enemy touches it: it cannot be launched
	int mFuseSound;             // +0xcc
	int mAngle;                 // +0xd0  degrees about the ship
	float mRadius;              // +0xd4  its distance from the ship
};
static_assert(sizeof(Weapon) == 0xd8, "Weapon");
static_assert(sizeof(Powerup) == 0xd8, "Powerup");

// A node of the level's key map (0x14 bytes)
struct KeyMapNode
{
	KeyMapNode* mNext;          // +0x00
	KeyMapNode* mPrev;          // +0x04
	int mA;                     // +0x08
	int mB;                     // +0x0c
	char mKey;                  // +0x10
};

// What a screen is (Level::mKind; Level_Load's types): a board to fill or one of the menus and screens. gGoTo holds
// the kind of the screen to go to next, and LEVEL_SAVED_GAME is only ever that: the saved game's board.
enum LevelKind
{
	LEVEL_BOARD = 0,
	LEVEL_MAIN_MENU = 1,
	LEVEL_HIGH_SCORE_ENTRY = 2,
	LEVEL_HIGH_SCORES = 3,
	LEVEL_CREDITS = 4,
	LEVEL_SELECT_LEVEL = 5,
	LEVEL_SELECT_LEVEL2 = 6,
	LEVEL_OPTIONS = 7,
	LEVEL_OPENING_NAG = 8,      // the trial's screens, read and never gone to
	LEVEL_CLOSING_NAG = 9,
	LEVEL_EXPIRING_NAG = 10,
	LEVEL_MISSION_COMPLETE = 0xb,
	LEVEL_HELP = 0xc,
	LEVEL_HELP2 = 0xd,
	LEVEL_LOGO = 0xe,
	LEVEL_SAVED_GAME = 0xf,
	LEVEL_CHANGE_USER = 0x10,
};

// A screen's state (Level::mState)
enum LevelState
{
	LEVEL_PLAYING = 1,
	LEVEL_PAUSED = 2,
	LEVEL_WON = 3,
};

// 0x478 bytes (0x41e880 allocates one, 0x413620 constructs it)
struct Level : Obj
{
	// mKind +0x08 a LevelKind, mState +0x0c a LevelState
	Cell* mColumns[GRID_COLUMNS];        // +0x0b0
	char mTitle[0x40];                // +0x178
	int mNumber;                      // +0x1b8
	char mBackImage[0x40];            // +0x1bc
	Image* mBackground;               // +0x1fc
	int mCols;                        // +0x200
	int mRows;                        // +0x204
	int mGameOver;                    // +0x208
	Animation* mHTiles;               // +0x20c
	int mHTileCount;                  // +0x210
	Animation* mVTiles;               // +0x214
	int mVTileCount;                  // +0x218
	Animation* mCornerTiles;          // +0x21c
	int mCornerTileCount;             // +0x220
	int mHPattern[GRID_COLUMNS];        // +0x224
	int mHPatternCount;               // +0x2ec
	int mVPattern[GRID_ROWS];        // +0x2f0
	int mVPatternCount;               // +0x384
	int mNumWalkers;                  // +0x388
	int mNumRollers;                  // +0x38c
	int mSeed;                        // +0x390  a random board's
	int mChain;                       // +0x394  enemies killed while the current capture fills, plus 1
	int mClicked;                     // +0x398
	int mRightClicked;                // +0x39c  or space
	Image* mFillImage;                // +0x3a0
	Image* mBeamH;                    // +0x3a4
	Image* mBeamV;                    // +0x3a8
	Image* mBeamMiddle;               // +0x3ac
	Image* mEmpty;                    // +0x3b0
	int mTotalCells;                  // +0x3b4  the open cells at the start
	int mFilledCells;                 // +0x3b8
	int mFillQuota;                   // +0x3bc  percent
	int mFloodedCells;              // +0x3c0  cells the capture's flood has reached since the chain began (0 when it ends): the chance of a power-up counts them as filled
	int mFillSound;                   // +0x3c4
	int mFillEndSound;                // +0x3c8
	int mSymmetry;                    // +0x3cc  a random board's
	int mShots;                       // +0x3d0  beams fired (the level's record keeps the fewest)
	int mLines;                       // +0x3d4  beam pairs that have stopped
	int mBonus;                       // +0x3d8
	int mBonusPadding[2];            // +0x3dc, +0x3e0  the eight bytes after bonus: nothing reads them
	int mMusicPending;                // +0x3e4
	List<Shooter> mShooters;          // +0x3e8
	List<Cell> mFills;                // +0x3f4  cells being filled (the columns own them)
	List<ParticleSystem> mSystems;    // +0x400
	List<Panel> mPanels;              // +0x40c
	List<Enemy> mEnemies;             // +0x418  the current one is the one the game file's tags go to
	List<KeyMapNode> mKeyMap;         // +0x424
	List<Powerup> mPowerups;          // +0x430
	List<Obj> mStars;                 // +0x43c
	List<Text> mTexts;                // +0x448
	List<Menu> mMenus;                // +0x454
	List<Obj> mPointers;              // +0x460
	List<Weapon> mWeapons;            // +0x46c
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
void Level_Load(Level* theLevel, const char* theCursor, XmlTag* theTag);
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
void Panel_FromXml(Panel* thePanel, const char* theCursor, const XmlTag* theTag); // 0x417d40
void Panel_Draw(Panel* thePanel, int theAll);                    // 0x418080
void Panel_Update(Panel* thePanel);                              // 0x418010
void Shooter_Insert(List<Shooter>* theList);                     // 0x4107e0
Shooter* Shooter_Init(Shooter* theShooter);                      // 0x41d540
void Shooter_Destroy(Shooter* theShooter);                       // 0x411d70
void List_DeleteNode(Shooter* theShooter);                       // 0x411f50's delete
void Shooter_Setup(Shooter* theShooter);                         // 0x41b0f0
void Shooter_FromXml(Shooter* theShooter, const char* theCursor, const XmlTag* theTag); // 0x41aec0
int Shooter_Update(Shooter* theShooter);                         // 0x41e200
void Shooter_Hit(Shooter* theShooter, float theVx, float theVy);  // 0x41bee0: a beamrider reached it
void Beams_RemoveAll(List<Beam>* theList);                       // 0x411900
void Enemy_Insert(List<Enemy>* theList);                         // 0x4106b0
// Enemies (CBEnemy.cpp)
Enemy* Enemy_Init(Enemy* theEnemy);                              // 0x404fc0
void Enemy_FromXml(Enemy* theEnemy, const char* theCursor, const XmlTag* theTag); // 0x405040
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
void Menu_FromXml(Menu* theMenu, const char* theCursor, const XmlTag* theTag); // 0x415310
void Menu_DrawToBackground(Menu* theMenu);                       // 0x413b30: its image and logo, about their middles
int Menu_Update(Menu* theMenu);                                  // 0x417c10: 0 when its screen is done
void Menu_ShowRequester(Menu* theMenu, int theKind, const char* theText, const char* theSecond); // 0x415900: 0 delete the player, 1 a message, 2 a name, 3 quit, 4 a board's end, 5 resume
void Pointer_Append(List<Obj>* theList);                         // 0x4109d0
void Pointer_Insert(List<Obj>* theList);                         // 0x410920
int Obj_FromXml(Obj* theObj, const char* theCursor, const XmlTag* theTag);     // 0x40d860: any object's attributes; 0 if its animation is missing
int Text_FromXml(Text* theText, const char* theCursor, const XmlTag* theTag); // 0x41f950
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
