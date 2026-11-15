#ifndef __SEVENSEAS_BOARD_H__
#define __SEVENSEAS_BOARD_H__

// Board.h: the game board of Seven Seas Deluxe 1.1 (WinSS.exe 0x401000-0x40bf00; object 0x959d0 bytes, vftable
// 0x493360, ButtonListener at +0x58). Behaviour spec: docs/GAME_NOTES.md section 3. Offsets in comments are the
// original's; code never depends on them (the port's base classes have other sizes).
//
// The game is turn based on an 11 x 11 grid: the player's ship (x, y, heading 0..15) moves one cell in any of
// the 8 directions or fires a broadside, then the enemy ships (pirates) move, then the cannonballs fly.

#include "Widget.h"
#include "ButtonListener.h"
#include "Button2003.h"
#include <list>
#include <string>
#include <vector>

namespace Sexy
{

class SevenSeasApp;
class Image;
struct AnimDef;

enum
{
	GRID = 11,                  // the board is 11 x 11 cells, a cell is 0x2a (42) pixels
	CELL_SIZE = 42
};

// Board::mState (+0x138)
enum BoardState
{
	STATE_IDLE = 0,             // waiting for the player's move
	STATE_TURN_MOVE = 1,        // the player's ship turns toward the next cell
	STATE_SAIL = 2,             // sailing to it
	STATE_WHIRL_OUT = 3,        // entered a whirlpool: the ship spins away
	STATE_WHIRL_IN = 4,         // and appears somewhere else
	STATE_NO_LIVES_FLASH = 5,   // after a hit: the pause before the ship comes back
	STATE_BLINK = 6,            // the new ship blinks
	STATE_PLAYER_SHOTS = 7,     // the player's cannonballs fly
	STATE_ENEMY_SHOTS = 8,      // the enemy ships' cannonballs fly
	STATE_ENEMY_TURN = 9,       // the enemy ships turn
	STATE_ENEMY_MOVE = 10,      // the enemy ships sail
	STATE_LEVEL_END = 12        // game over
};

// 0x14 bytes (list at +0x110, count +0x114): a port in a corner of the board
struct Port
{
	int					mX;             // +0
	int					mY;             // +4
	int					mFrame;         // +8 the whirl's rotation frame 0..15
	int					mAppear;        // +0xc the appear/disappear counter (-1 while hidden)
	bool				mActive;        // +0x10
};

// 0x48 bytes (list at +0x104, count +0x108): an enemy ship
struct Ship
{
	int					mType;          // +0 0 pirate, 1 red pirate, 2 sea monster, 3 blue pirate, 4 ghost ship
	int					mX;             // +4 the cell
	int					mY;             // +8
	int					mHeading;       // +0xc 0..15 (0 up, 4 right, 8 down, 12 left)
	int					mWantHeading;   // +0x10
	int					mNextX;         // +0x14 the cell it sails to
	int					mNextY;         // +0x18
	int					mMoveProgress;  // +0x1c 0..0x2a while sailing
	int					mAnim;          // +0x20 frame counter (starts with rand() % 8)
	int					mTrail[9];      // +0x24 3 x 3 cell timers (a blue pirate's gun smoke around it), -1 = off
};

// 0x20 bytes (list at +0xf8, count +0xfc): a cannonball
struct Shot
{
	int					mOwner;         // +0 0 the player's, 1 an enemy's
	int					mX;             // +4 cell
	int					mY;             // +8
	int					mDX;            // +0xc direction
	int					mDY;            // +0x10
	int					mRange;         // +0x14 cells the ball flies
	int					mProgress;      // +0x18 0..0x2a
	int					mFrame;         // +0x1c
};

// 0x10 bytes (list at +0x11c): the score that rises from a cell
struct ScoreText
{
	int					mX;
	int					mY;
	int					mValue;
	int					mAlpha;         // 0x140, falls by 10 every third update
};

// 0x18 bytes (list at +0x130): a running animation of data\anims.txt
struct AnimInstance
{
	AnimDef*			mDef;
	int					mX;
	int					mY;
	int					mFrameIdx;
	int					mDelay;
	bool				mFlag;          // the fifth argument of AddAnim; FUN_0040a390 looks for animations without it
};

// The saved copy of the board that Undo restores (FUN_0040aec0 pushes, FUN_0040b0b0 pops): a flat list of ints
class Board : public Widget, public ButtonListener
{
public:
	SevenSeasApp*		mApp;               // +0x5c

	// score check of the internet high scores (+0x6c, an object of 0x42a000): not ported
	int					mScore;             // +0x88
	int					mLives;             // +0x8c
	int					mLifeFlash;         // +0x90 counts down after an extra life
	int					mSavedLevels;       // +0x94 set to 1 by a new game
	int					mKills;             // +0x98 ships sunk on this level
	int					mKillsTotal;        // +0x9c
	int					mTreasures;         // +0xa0 treasures found on the bonus screens
	int					mScoreCheck;        // +0x60 / +0x64 / +0x68: the internet score check's fields (not ported)
	int					mScoreCheck2;
	int					mLevelStartCnt;     // +0x5a8 update count when the level began
	int					mGameStartCnt;      // +0x5ac
	int					mPlayerX;           // +0xa4
	int					mPlayerY;           // +0xa8
	int					mHeading;           // +0xac 0..15
	int					mWantHeading;       // +0xb0
	int					mDestX;             // +0xb4
	int					mDestY;             // +0xb8
	int					mMoveProgress;      // +0xbc
	int					mShipAnim;          // +0xc0
	bool				mBlink;             // +0xc4
	int					mBlinkCount;        // +0xc8
	int					mTimer;             // +0xcc
	int					mGunFlash[9];       // +0xd0 3 x 3 timers around the ship (the gun smoke)

	std::list<Shot*>		mShots;         // +0xf8
	std::list<Ship*>		mShips;         // +0x104
	std::list<Port*>		mPorts;         // +0x110
	std::list<ScoreText*>	mScoreTexts;    // +0x11c
	int					mMouseX;            // +0x124
	int					mMouseY;            // +0x128
	std::list<AnimInstance*>	mAnims;     // +0x130
	int					mState;             // +0x138
	int					mLevel;             // +0x13c

	bool				mVisited[GRID][GRID];   // +0x140 (a cell with a wreck or a sunk ship: blocked)
	int					mTerrain[GRID][GRID];   // +0x1bc 0 sea, 1 and 2 islands
	int					mDist[GRID][GRID];      // +0x3a0 flood-fill distances (the enemy AI)
	bool				mLevelOver;         // +0x584
	bool				mLevelComplete;     // +0x585
	double				mWaveA;             // +0x588 / +0x590 / +0x598 / +0x5a0: the water's scroll
	double				mWaveB;
	double				mWaveSpeedA;
	double				mWaveSpeedB;
	int					mSeconds;           // +0x5bc
	DWORD				mStartTick;         // +0x5c0
	int					mPoints[GRID][GRID];    // +0x5c4 points to show at a cell
	// port(mod): what the autosplit observer reads (LiveSplitObserver.cpp): which board this is, and that it was loaded
	uint32_t			mBoardSerial;
	bool				mResumed;
	bool				mDirty;             // +0x7a8
	int					mCellFlash[GRID][GRID]; // +0x7ac
	bool				mBonus;             // +0x991 the bonus screen (digging for treasure) is on
	bool				mBonusReady;        // +0x990
	int					mBonusGrid[9][9];   // +0x994
	int					mBonusState;        // +0xad8
	int					mBonusTick;         // +0xadc
	int					mTreasureX;         // +0xae0
	int					mTreasureY;         // +0xae4
	int					mShovelX;           // +0xae8
	int					mShovelY;           // +0xaec
	int					mBonusTick2;        // +0xaf0
	int					mTutorialStep;      // +0x95000
	DialogButton*		mStartLink;         // +0x95004
	bool				mKeyTap[0xFF];      // port: a key pressed since the last poll (a tap that came and went between two updates)
	int					mCellFlashFloor;    // +0x95008 a flash level taken off a cell's flash for the reach of its glow and compared with it for the hint arrows; only the level's reset writes it (0)
	bool				mTutorialShown;     // +0x9500c
	int					mHintTimer;         // +0x68 counts down after "Cannot Undo" (the text that shows)
	int					mHintReason;        // +0x64 why: the reason of the last ClearUndo (0 level start, 1 whirlpool, 2 life, 3 whirls)

	std::list<int>		mUndo;              // +0x5b4 the undo list

public:
	Board(SevenSeasApp* theApp, bool theLoad);
	virtual ~Board();

	// Widget
	virtual void		RemovedFromManager(WidgetManager* theWidgetManager) override;
	virtual void		Update() override;
	virtual void		Draw(Graphics* g) override;
	virtual void		MouseMove(int x, int y) override;
	virtual void		KeyDown(KeyCode theKey) override;
	virtual void		MouseDown(int x, int y, int theClickCount) override;
	virtual void		MouseUp(int x, int y) override;
	virtual void		ButtonDepress(int theId) override;
	using Widget::MouseDown;
	using Widget::MouseUp;

	// the board
	static int			CellToPixel(int theCell);                   // 0x40a3f0
	static int			PixelToCellX(int theX);                     // 0x40a3b0
	static int			PixelToCellY(int theY);                     // 0x40a3d0

	void				NewGame(bool theLoad);                      // 0x4014a0
	void				NewLevel();                                 // 0x401580
	void				SetupTutorial();                            // 0x402690
	void				SetupBonus(int theX, int theY);             // 0x402540
	void				GameOver();                                 // 0x402650
	void				AddAnim(const std::string& theName, int theX, int theY, bool theFlag);   // 0x4032f0
	void				AddPoints(int thePoints, int theX, int theY);    // 0x403240
	void				SetScoreCheck(int theValue);                // 0x4032e0
	bool				AllPortsDone();                             // 0x4033c0
	Ship*				ShipAt(int theX, int theY, Ship* theExcept);    // 0x4033f0
	void				RemoveShip(Ship* theShip);                  // 0x403430
	void				ClearPorts();                               // 0x403490
	void				ClearAnims();                               // 0x4034e0
	void				ClearShots();                               // 0x403530
	void				ClearShips();                               // 0x403580
	void				ClearCellFlash();                           // 0x405ef0
	void				LoseLife();                                 // 0x403200
	void				SpawnExplosion(int theX, int theY, bool theBig);    // 0x402be0
	void				SpawnSplash(int theX, int theY);            // 0x402d10
	void				SpawnWake(int theX, int theY);              // 0x402f10
	void				SpawnSmoke(int theX, int theY);             // 0x403000
	void				LoadGame();                                 // 0x40b870
	bool				SaveGame();                                 // 0x40b4e0
	void				SpawnSparkle(int theX, int theY);           // 0x4030f0
	void				SpawnGhost(int theX, int theY);             // 0x402e10
	int					Pan(int theCell);                           // 0x40a440

	// updating
	void				UpdateMove();                               // 0x4035d0
	void				GetKeys(bool* theKeys);
	void				UpdateShots();                              // 0x403e80
	void				UpdateAnims();                              // 0x404430
	void				FirePlayerShots();                          // 0x4044d0
	void				EnemyShots();                               // 0x404820
	void				EnemyFire(Ship* theShip);                   // 0x404890
	void				PlanEnemyMoves();                           // 0x404bb0
	void				Flood(int theX, int theY);                  // 0x405660
	void				UpdateEnemies();                            // 0x405700
	void				UpdatePorts();                              // 0x405ca0
	void				ToggleWhirls();                             // 0x405d00
	void				UpdateScoreTexts();                         // 0x405d90
	void				UpdateCellFlash();                          // 0x405ec0
	void				UpdateWaves();                              // 0x405f10
	void				PlaceShip();                                // 0x409e90

	// queries
	bool				CellFree(int theX, int theY);               // 0x40a000
	bool				CanFireAt(int theX, int theY, int theHeading, int theTX, int theTY, int theRange);  // 0x40a050
	int					FireRange(int theX, int theY, int theHeading, int theTX, int theTY);    // 0x40a230
	bool				AnyBlockingAnim();                          // 0x40a390
	void				SaveUndo();                                 // 0x40aec0
	bool				Undo();                                     // 0x40b0b0
	void				ClearUndo(int theScoreCheck);               // 0x40b490

	// drawing
	void				DrawBox(int theX, int theY, int theWidth, int theHeight, Graphics* g);      // 0x4093f0
	void				DrawCellArrow(int theFromX, int theFromY, int theX, int theY, bool theWithRange, Graphics* g);  // 0x409840
	void				DrawGunRange(int theX, int theY, Graphics* g);  // 0x409d10
	void				DrawBonus(Graphics* g);
	void				DrawHintLine(Graphics* g, const SexyString& theText, int theX, int theY);

	static std::string	SavePath();
	bool				DeleteSave();                               // 0x40b850
};

}

#endif
