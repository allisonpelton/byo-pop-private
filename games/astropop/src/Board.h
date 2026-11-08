#ifndef __ASTROPOP_BOARD_H__
#define __ASTROPOP_BOARD_H__

// Board: the playfield screen (original constructor 0x480020, vtable 0x566a30 with 47 slots, listener part at +0x58 with
// the table 0x566980, sizeof 0x1c0c). It is the one big object of the game: the field of bricks, the falling pieces, the
// power-ups, the stats, the replay recording. Its members keep their original offsets; until the code that uses a
// member says what it is, the member is called m<offset> (docs/board_fields.tsv, tools/boardfields.py).

#include "Widget.h"
#include "Buffer.h"
#include "Color.h"
#include "EventBus.h"
#include "FrameAnim.h"
#include "SmallObjects.h"
#include <list>
#include <set>
#include <vector>

namespace Sexy
{

class Brick;
class AstroPopApp;
class Board;
class Image;
class FuelCell;
class SpecialBrick;
class Particle;

// a cell of the field (the original keeps vectors of these: 8 bytes each)
struct CellRef
{
	int		mCol;
	int		mRow;
};

// a cell and a number (12 bytes each)
struct CellRef3
{
	int		mCol;
	int		mRow;
	int		mValue;
};

// A row of the table that sets the speed of the game by the time played (24 bytes, the vector at +0x280)
struct RampRecord
{
	int		mTime;      // updates played when the row ends
	int		mV1;        // -> +0x19cc
	int		mV2;        // -> +0x1ac4
	int		mV3;        // -> +0x1c04
	float	mF4;        // -> +0x1bec
	float	mF5;        // -> +0x1be8
};

// A layer of the scenery that scrolls down the screen (0x4333c0 steps it). The Board keeps one at +0x77c; the planets
// (0x3c bytes each, a ScrollLayer and one more number) are a vector at +0x160.
struct ScrollLayer
{
	Image*	mImage;             // +0x00
	float	mCur[4];            // +0x04 the picture now on the screen (mCur[1] counts down to its end)
	float	mNext[4];           // +0x14 the picture that follows (mNext[1] the place to start from)
	float	mF24;               // +0x24
	float	mF28;               // +0x28
	int		mPeriod;            // +0x2c the layer steps every mPeriod updates
	int		mI30;               // +0x30
	float	mSpeed;             // +0x34
};

// A planet: a scroll layer and a number (0x3c bytes)
struct PlanetRec : public ScrollLayer
{
	int		m38;                // +0x38
};

// One of the 100 streaks of the backdrop (0x30 bytes, the vector at +0x170)
struct StreakRec
{
	unsigned char	m00[0x14];
	float			mY;                 // +0x14
	float			mSpeed;             // +0x18
	float			mMinSpeed;          // +0x1c
	int				mMargin;            // +0x20
	unsigned char	m24[0x0c];
};

// One of the stars (0x6c bytes, the vector at +0x190): a picture of the backdrop that drifts down the screen
struct StarRec
{
	Image*			mImage;             // +0x00
	unsigned char	m04[0x4c];
	float			mX;                 // +0x50
	float			mY;                 // +0x54 counts down the screen
	unsigned char	m58[0x08];
	int				m60;                // +0x60
	float			mSpeed;             // +0x64
	int				mImageId;           // +0x68 the resource id of the picture (IMAGE_BG_OBJECT1 .. 7), -1 for none
};

// The picture of a stage (an object of 0x120 bytes or more; the Board keeps the pictures of all stages in a vector at
// +0x220). Placeholder: the parts the Board's update touches.
struct StageBg
{
	unsigned char	m00[0x5c];
	int				m5c;                // +0x5c
	unsigned char	m60[0x10];
	FrameAnim		m70;                // +0x70
	FrameAnim		mb0;                // +0xb0
	unsigned char	mf0;                // +0xf0 the fade-out is over
	unsigned char	mf1[0x0f];
	int				m100;               // +0x100
	int				m104;               // +0x104
	int				m108;               // +0x108
	int				m10c;               // +0x10c
	int				m110;               // +0x110
	int				m114;               // +0x114
	int				m118;               // +0x118
};

// A warp between stages (vector at +0x230); its first member is the picture it shows
struct Warp
{
	StageBg*		mBg;                // +0x00
	unsigned char	m04[0x18];
	unsigned char	m1c;                // +0x1c active
};

// Two values that swing between 0 and 255 in turns (0x433850; the Board keeps one at +0x808)
struct PulsePair
{
	int		mA;                 // +0x00
	int		mB;                 // +0x04
	int		mDeltaA;            // +0x08
	int		mDeltaB;            // +0x0c
	int		mWait;              // +0x10 updates to wait before the next swing
	int		mWaitReset;         // +0x14

	void	Update(int theSpeed);                       // 0x433850
};

// The widget over the field (constructor 0x4327c0, vtable 0x563e28): it fades the field to a colour, shows the PAUSED
// text and the screens' darkening. The Board keeps it at +0x660 and points it back at itself (+0x7c).
class BoardOverlay : public Widget
{
public:
	Color			mColor58;           // +0x58 (white, clear)
	Color			mFill;              // +0x68 the colour the overlay fills the field with
	bool			mFlag78;            // +0x78
	Board*			mBoard;             // +0x7c
	bool			mFlag80;            // +0x80

	BoardOverlay();                                     // 0x4327c0
	void			SetFill(int theRed, int theGreen, int theBlue, int theAlpha);   // 0x432990
	void			SetFlag78(bool theFlag) { mFlag78 = theFlag; }                  // 0x4329c0
};

extern bool gFlag5A5100;    // 0x5a5100

class Board : public Widget, public EventListener
{
public:
	// BEGIN FIELDS (generated by tools/boardfields.py from docs/board_fields.tsv)
	std::list<Brick*>	m70;// +0x70 the bricks of the falling piece (the original's list head is at +0x70, its size at +0x74)
	std::list<std::list<Brick*> >	m7c;// +0x7c groups of falling bricks
	std::list<Brick*>	m88;// +0x88 bricks (emptied by ClearField)
	std::list<std::list<Brick*> >	m94;// +0x94
	std::list<Brick*>	ma0;// +0xa0
	std::vector<void*>	ma8;// +0xa8 freed by ClearField, element type not seen
	std::vector<CellRef>	mc8;// +0xc8
	std::vector<BlinkRec>	md8;// +0xd8 the 40 blinking pictures of the backdrop
	std::vector<int>	mf8;// +0xf8
	std::vector<Brick*>	m108;// +0x108 bricks (emptied by ClearField)
	std::vector<Brick*>	mTargeted;// +0x118 the bricks that flash because the piece in hand would hit them (HighlightTargets)
	std::set<Brick*>	m128;// +0x128 the bricks to delete (DeleteDoomedBricks)
	std::vector<FloatText*>	m140;// +0x140 the texts that float up the screen
	std::vector<PlanetRec>	m160;// +0x160 the planets (5 are stepped)
	std::vector<StreakRec>	m170;// +0x170 the streaks (100)
	std::vector<StarRec>	m190;// +0x190 the stars
	std::vector<CellRef3>	m1a0;// +0x1a0
	std::vector<FrameAnim*>	m1b0;// +0x1b0 the animations of the explosion at the end of a game (8)
	std::vector<int>	m1f0;// +0x1f0 counters that count down from 2000 (set when a special brick is made)
	std::vector<CellRef3>	m200;// +0x200 ?
	std::vector<StageBg*>	m220;// +0x220 the picture of each stage
	std::vector<Warp*>	m230;// +0x230
	std::vector<RampRecord>	m280;// +0x280 the speed table (UpdateRamp)
	std::vector<Particle*>	m2a0;// +0x2a0 the falling sparks of the energy mode
	std::vector<HintBox*>	m2b0;// +0x2b0 the hint boxes
	std::vector<DemoStep>	m2d0;// +0x2d0 the demo's steps
	Image*	m2f8 = NULL;// +0x2f8 the lightning image now on the screen
	GoalChecker*	m310 = NULL;// +0x310 the check of the level's goal
	std::vector<std::vector<Brick*> >	m314;// +0x314 the field: 19 rows of 10 bricks, row 0 on top
	std::vector<CellRef>	m324;// +0x324 the cells under the pointer that are lit (HoverCell)
	std::vector<std::vector<int> >	m334;// +0x334 the layout the field is built from: a brick type for each cell, -1 none (BuildField)
	Brick*	m354 = NULL;// +0x354 a brick the Board holds (ClearField deletes it)
	AstroPopApp*	mApp = NULL;// +0x358 the application (the bricks play their sounds through it)
	Color	m3c0;// +0x3c0 the colour of the backdrop tint (FadeBackdrop)
	Color	m3d0;// +0x3d0 the colour the overlay is filled with when it comes back (its alpha, at +0x3dc, counts down)
	FrameAnim	m420;// +0x420
	FrameAnim	mSirenAnim;// +0x4a0 the siren (IMAGE_SIREN)
	FrameAnim	mAnim5e0;// +0x5e0 ?
	FrameAnim	mAnim620;// +0x620
	bool	m648 = false;// +0x648
	BoardOverlay	mOverlay;// +0x660 the widget over the field
	Color	m6c8;// +0x6c8
	bool	m6e0 = false;// +0x6e0 ?
	int	m6e8 = 0;// +0x6e8 ?
	int	m6ec = 0;// +0x6ec ?
	Buffer	m758;// +0x758 the replay recording (RecordStep)
	ScrollLayer	m77c;// +0x77c the scenery layer
	int	m7a8 = 0;// +0x7a8 ?
	int	m7b8 = 0;// +0x7b8 ?
	int	m7bc = 0;// +0x7bc ?
	int	m7c0 = 0;// +0x7c0 ?
	int	m7c4 = 0;// +0x7c4 ?
	int	m7c8 = 0;// +0x7c8 ?
	int	m7e8 = 0;// +0x7e8 ?
	int	m7ec = 0;// +0x7ec ?
	PulsePair	m808;// +0x808
	bool	m8db = false;// +0x8db the game is being recorded (RecordStep)
	bool	m8dc = false;// +0x8dc the demo plays
	bool	m8dd = false;// +0x8dd ?
	bool	m8de = false;// +0x8de ?
	bool	m8e0 = false;// +0x8e0 ?
	bool	m8e1 = false;// +0x8e1
	bool	m8e3 = false;// +0x8e3 ?
	bool	m8e4 = false;// +0x8e4 ?
	bool	m8e5 = false;// +0x8e5 ?
	bool	m8e6 = false;// +0x8e6 ?
	bool	m8e7 = false;// +0x8e7 ?
	bool	mFlag8E8 = false;// +0x8e8
	bool	mFlag8E9 = false;// +0x8e9
	bool	m8ea = false;// +0x8ea the streaks are fading out
	bool	m8eb = false;// +0x8eb
	bool	m8ec = false;// +0x8ec
	bool	m8ed = false;// +0x8ed
	bool	m8ee = false;// +0x8ee ?
	bool	m8ef = false;// +0x8ef
	bool	m8f0 = false;// +0x8f0 ?
	bool	m8f1 = false;// +0x8f1 ?
	bool	m8f2 = false;// +0x8f2 ?
	bool	m8f3 = false;// +0x8f3 ?
	bool	m8f4 = false;// +0x8f4
	bool	m8f5 = false;// +0x8f5 ?
	bool	m8f6 = false;// +0x8f6 ?
	bool	m8f8 = false;// +0x8f8 ?
	bool	m8f9 = false;// +0x8f9 ?
	bool	m8fa = false;// +0x8fa ?
	bool	m8fb = false;// +0x8fb ?
	bool	m8fc = false;// +0x8fc ?
	bool	m8fd = false;// +0x8fd ?
	bool	m8fe = false;// +0x8fe ?
	bool	m8ff = false;// +0x8ff
	bool	m901 = false;// +0x901 ?
	bool	m905 = false;// +0x905 ?
	bool	m907 = false;// +0x907 ?
	bool	m909 = false;// +0x909 ?
	bool	m90a = false;// +0x90a
	bool	m90b = false;// +0x90b ?
	bool	m90d = false;// +0x90d ?
	bool	m90e = false;// +0x90e ?
	bool	m90f = false;// +0x90f
	bool	m910 = false;// +0x910
	bool	m911 = false;// +0x911 ?
	bool	m912 = false;// +0x912 ?
	bool	m916 = false;// +0x916 ?
	bool	m917 = false;// +0x917
	bool	m918 = false;// +0x918
	bool	m919 = false;// +0x919
	bool	m91a = false;// +0x91a ?
	bool	m91b = false;// +0x91b ?
	bool	m91c = false;// +0x91c ?
	bool	m91e = false;// +0x91e ?
	bool	m91f = false;// +0x91f ?
	bool	m920 = false;// +0x920 ?
	bool	m921 = false;// +0x921 ?
	bool	m924 = false;// +0x924 ?
	float	m928 = 0.0f;// +0x928
	float	m92c = 0.0f;// +0x92c ?
	float	m930 = 0.0f;// +0x930 ?
	float	m934 = 0.0f;// +0x934 ?
	float	m938 = 0.0f;// +0x938
	float	m940 = 0.0f;// +0x940
	unsigned char	m948 = 0;// +0x948
	unsigned char	m949 = 0;// +0x949
	int	m94c = 0;// +0x94c ?
	unsigned char	m94d = 0;// +0x94d
	int	m950 = 0;// +0x950 ?
	int	m954 = 0;// +0x954 ?
	int	m958 = 0;// +0x958 ?
	int	m95c = 0;// +0x95c ?
	int	m960 = 0;// +0x960 ?
	int	m964 = 0;// +0x964 ?
	int	m974 = 0;// +0x974 ?
	int	m97c = 0;// +0x97c ?
	int	m1924 = 0;// +0x1924 ?
	int	m1928 = 0;// +0x1928 ?
	int	m192c = 0;// +0x192c ?
	int	m1934 = 0;// +0x1934 ?
	int	m1938 = 0;// +0x1938 ?
	int	mField193C = 0;// +0x193c the stage: 1 + the time played / 6000
	int	m1940 = 0;// +0x1940 ?
	int	m1944 = 0;// +0x1944 ?
	int	m194c = 0;// +0x194c ?
	int	m1950 = 0;// +0x1950 ?
	int	m1954 = 0;// +0x1954 ?
	int	m1958 = 0;// +0x1958 the pointer's x
	int	m195c = 0;// +0x195c the pointer's y
	int	m1960 = 0;// +0x1960 ?
	int	m1964 = 0;// +0x1964 ?
	int	m196c = 0;// +0x196c ?
	int	m1970 = 0;// +0x1970 ?
	int	mScore = 0;// +0x1974 the score (profile setting 0x51)
	int	m1978 = 0;// +0x1978 ?
	int	m1980 = 0;// +0x1980 ?
	int	m1984 = 0;// +0x1984 ?
	int	m1988 = 0;// +0x1988 ?
	int	m198c = 0;// +0x198c ?
	int	m1990 = 0;// +0x1990 ?
	int	m1994 = 0;// +0x1994 ?
	int	m19b0 = 0;// +0x19b0 ?
	int	m19b4 = 0;// +0x19b4 ?
	int	m19b8 = 0;// +0x19b8 ?
	int	m19bc = 0;// +0x19bc ?
	int	m19cc = 0;// +0x19cc ?
	int	m19d0 = 0;// +0x19d0 ?
	int	m19d4 = 0;// +0x19d4 ?
	int	m19d8 = 0;// +0x19d8 ?
	int	m19dc = 0;// +0x19dc ?
	int	m19f0 = 0;// +0x19f0 ?
	int	m19f4 = 0;// +0x19f4 ?
	int	m19f8 = 0;// +0x19f8 ?
	int	m19fc = 0;// +0x19fc ?
	int	m1a10 = 0;// +0x1a10 ?
	int	m1a14 = 0;// +0x1a14 ?
	int	m1a28 = 0;// +0x1a28 ?
	int	m1a2c = 0;// +0x1a2c ?
	int	m1a30 = 0;// +0x1a30 ?
	int	m1a34 = 0;// +0x1a34 ?
	int	mMode = 0;// +0x1a38 the game mode (profile setting 0x53); also the key of the best score
	int	m1a3c = 0;// +0x1a3c ?
	int	m1a40 = 0;// +0x1a40 ?
	int	m1a44 = 0;// +0x1a44 ?
	int	m1a48 = 0;// +0x1a48 ?
	int	m1a4c = 0;// +0x1a4c ?
	int	m1a50 = 0;// +0x1a50 ?
	int	m1a60 = 0;// +0x1a60 ?
	int	m1a68 = 0;// +0x1a68 ?
	int	m1a7c = 0;// +0x1a7c ?
	int	m1a80 = 0;// +0x1a80 ?
	int	m1a84 = 0;// +0x1a84
	int	m1a8c = 0;// +0x1a8c ?
	int	m1a98 = 0;// +0x1a98 ?
	int	m1a9c = 0;// +0x1a9c ?
	int	m1aa4 = 0;// +0x1aa4 ?
	int	m1aa8 = 0;// +0x1aa8 ?
	int	m1aac = 0;// +0x1aac ?
	int	m1ab4 = 0;// +0x1ab4 ?
	int	m1ab8 = 0;// +0x1ab8 ?
	int	m1abc = 0;// +0x1abc ?
	int	m1ac0 = 0;// +0x1ac0 ?
	int	m1ac4 = 0;// +0x1ac4 ?
	int	m1af0 = 0;// +0x1af0 ?
	int	m1af4 = 0;// +0x1af4 ?
	int	m1afc = 0;// +0x1afc ?
	int	m1b00 = 0;// +0x1b00 ?
	int	m1b10 = 0;// +0x1b10 ?
	int	m1b18 = 0;// +0x1b18 ?
	int	m1b1c = 0;// +0x1b1c ?
	int	m1b24 = 0;// +0x1b24 ?
	int	m1b2c = 0;// +0x1b2c ?
	int	m1b30 = 0;// +0x1b30
	int	m1b38 = 0;// +0x1b38 ?
	int	m1b3c = 0;// +0x1b3c ?
	int	m1b40 = 0;// +0x1b40
	int	m1b44 = 0;// +0x1b44 ?
	int	m1b48 = 0;// +0x1b48 ?
	int	m1b4c = 0;// +0x1b4c
	int	m1b54 = 0;// +0x1b54 ?
	int	m1b5c = 0;// +0x1b5c ?
	int	m1b60 = 0;// +0x1b60 ?
	int	m1b64 = 0;// +0x1b64 ?
	int	m1b68 = 0;// +0x1b68 ?
	int	m1b70 = 0;// +0x1b70 ?
	int	m1b78 = 0;// +0x1b78
	int	m1b9c = 0;// +0x1b9c ?
	int	m1ba4 = 0;// +0x1ba4 ?
	int	m1bbc = 0;// +0x1bbc ?
	int	m1bc0 = 0;// +0x1bc0 ?
	float	m1bd8 = 0.0f;// +0x1bd8
	float	m1bdc = 0.0f;// +0x1bdc
	float	m1be8 = 0.0f;// +0x1be8
	float	m1bec = 0.0f;// +0x1bec
	float	m1bf8 = 0.0f;// +0x1bf8
	float	m1bfc = 0.0f;// +0x1bfc
	float	m1c00 = 0.0f;// +0x1c00
	int	mDropTicks = 0;// +0x1c04 updates a mutated brick waits before it drops
	// END FIELDS

	Board();

	virtual void	Update() override;                              // 0x479480 slot 17

	// ---- the small helpers (BoardCore.cpp)
	void			UpdateAnimCounters();                           // 0x433500
	void			SetOverlayHidden(bool theHide);                 // 0x433c30
	void			RecordStep(bool theForce);                      // 0x434570
	int				PickSpawnDelay();                               // 0x432a40
	void			FadeBackdrop();                                 // 0x436b80
	void			UpdateRamp();                                   // 0x437a50
	int				FindBottomRow(int theColumn, bool theIsX);      // 0x437590
	bool			NoRoomLeft();                                   // 0x438b60
	int				CountMovingBricks(bool theStrict);              // 0x439b70
	static bool		HasCell(int theCol, int theRow, const std::vector<CellRef>& theCells);       // 0x438d60
	static bool		HasCell3(int theCol, int theRow, const std::vector<CellRef3>& theCells);     // 0x438db0
	void			StepScrollLayer(ScrollLayer* theLayer);         // 0x4333c0
	void			UpdateScenery();                                // 0x43a1f0
	void			UpdateStageBackground();                        // 0x43a680
	void			StoreProgress();                                // 0x432f40
	void			UpdateKlaxon();                                 // 0x43b9c0
	bool			IsSettled(bool theAssumeIdle);                  // 0x43d2f0
	void			DeleteDoomedBricks();                           // 0x43d8b0
	Brick*			CreateBrick(int theType);                       // 0x4332b0
	Brick*			CreateMutator();                                // 0x433320
	Brick*			CreateFuelCell(int theColumn, int theRow);      // 0x433340
	SpecialBrick*	CreateSpecialBrick(int theType);                // 0x4393d0
	Brick*			CreatePowerBrick(int theType);                  // 0x439480

	// ---- the field (BoardField.cpp)
	static bool		TypesMatch(Brick* theBrick, int theType);       // 0x433030
	bool			FirstPieceIsType(int theType);                  // 0x436b30
	bool			IsTargeted(Brick* theBrick);                    // 0x439510
	void			ClearField();                                   // 0x44e010
	void			BuildField();                                   // 0x44e350

	// ---- the state (BoardState.cpp)
	void			ResetState();                                   // 0x4346c0
	void			UpdateEffects();                                // 0x439c20
	void			SetNextLevelScore();                            // 0x4344d0
	void			AddScore(int thePoints, bool theForce);         // 0x433b90
	void			SaveGameState();                                // 0x43a9a0
	void			ResetGame();                                    // 0x450250

	// ---- not translated yet (BoardStubs.cpp): each logs once and does nothing; named by address until they are read
	void			Fn475040();                                     // 0x475040 (3341 bytes)
	void			Fn465290();                                     // 0x465290 (1444 bytes)
	bool			Fn46a810();                                     // 0x46a810 (3545 bytes): true ends the update
	void			Fn46e420();                                     // 0x46e420 (1356 bytes)
	void			Fn46e9b0();                                     // 0x46e9b0 (1615 bytes)
	void			Fn45ab90();                                     // 0x45ab90 (225 bytes)
	void			Fn45e680();                                     // 0x45e680 (664 bytes)
	void			Fn459040(bool theFlag, int theValue);           // 0x459040 (879 bytes)

	// ---- the update (BoardUpdate.cpp)
	void			RevealBricks();                                 // part of 0x479480: the bricks of a new field appear one by one
	void			SweepSprockets();                               // part of 0x479480
	void			SweepTurbots();                                 // part of 0x479480
	bool			SpawnControl();                                 // part of 0x479480: the pieces that drop and the end of the game (true: the update ends)
	void			UpdateExplosion();                              // part of 0x479480
	void			UpdateSparks();                                 // part of 0x479480

	// ---- the cursor's targets (BoardTarget.cpp)
	void			FlashOnce(Brick* theBrick);
	void			HighlightTargets();                             // 0x452750
	void			HoverCell(int theCol, unsigned int theRow, bool theForce);   // 0x453630
};

}

#endif
