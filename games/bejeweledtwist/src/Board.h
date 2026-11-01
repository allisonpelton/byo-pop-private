// Sexy::Board (about 0x3940 bytes, vtable 0x7dd0d4 with 71 slots; 0x6a3d7b-0x6d6899): the game itself. Being
// ported (docs/NOTES.md, "The board"); its fields are named by their offset in the original's object until their use
// is known, and come in as the code that uses them does.
#pragma once

#include "TwistWidget.h"
#include "Tutorial.h"
#include "X86Math.h"
#include "WinTwistApp.h"

#include "Buffer.h"
#include "CurvedVal.h"
#include "MTRand.h"
#include "Rect.h"

#include <deque>
#include <map>
#include <set>
#include <vector>

namespace Sexy {

class EffectsManager;
class GoodTube;
class Hyperspace;
class Font;
class Image;
class MemoryImage;
class Piece;
class TextImage;
class Traveller;
class PointsManager;
class FlyingText;
class GemSoul;
class Shockwave;
class SoundInstance;
class WinTwistApp;

// A pair of numbers that change with the level, read from the config (0x74ec7d, "<name>FirstLevel" and the rest): from
// mFirstLevel on, a base, plus a change every mLevelsBetweenChange levels from mFirstChangeLevel and every
// mTurnsBetweenChange of the app's +0xcf8, held to a min and a max (each unless below 0). The original's record is
// 0x58 bytes.
struct LevelValue {
	int mValue = 0;                 // +0x0: GetCount (0x74eac6) at a new level
	int mFirstLevel = 0;            // +0x4
	int mFirstChangeLevel = 0;      // +0x8: mFirstLevel + mLevelsBetweenChange unless the config says
	int mBase = 0;                  // +0xc
	int mValueBase = 0;             // +0x10
	int mLevelsBetweenChange = 1;   // +0x14
	int mChange = 0;                // +0x18
	int mValueChange = 0;           // +0x1c
	int mMin = -1;                  // +0x20
	int mMax = -1;                  // +0x24
	int mValueMin = -1;             // +0x28
	int mValueMax = -1;             // +0x2c
	int mTurnsBetweenChange = 0;    // +0x30
	bool mTurnBased = false;        // +0x34
	double mChancePow = 0;          // +0x38
	double mUnknown40 = 1.0;        // +0x40
	double mUnknown48 = 1.0;        // +0x48
	bool mUnknown50 = false;        // +0x50
	bool mUnknown51 = false;        // +0x51: Read leaves mValue alone

	void Read(const char* theName);  // 0x74ec7d
	int GetCount();                  // 0x74ea57: mBase, mChange, mMin, mMax
	int GetValue();                  // 0x74e9e8: mValueBase, mValueChange, mValueMin, mValueMax
	void Restart() { mValue = GetCount(); mUnknown50 = false; }  // 0x74eac6
	void RestartAtLevel();           // 0x74ead6: Restart when mFirstLevel is the board's level
	bool Roll();                     // 0x74eb38: one turn down; whether it comes now, by chance (Rand85B970)
	bool ShouldAppear();             // 0x74ec45: from mFirstLevel on, whether it comes now
};

// A match, or the match a turn would make (0x6b97b6 makes one, 0x6afdee resets it)
struct MatchInfo {
	int mKind = 0;                  // +0x0: 4 for the square
	int mRow = -1;                  // +0x4
	int mCol = -1;                  // +0x8
	int mCount = 0;                 // +0xc: its gems
	bool mUnknown10 = true;         // +0x10
	int mColor = -1;                // +0x14
	std::vector<int> mIds;          // +0x18: its gems' ids
	int mSerial = 0;                // +0x28: from gMatchInfoCount
	int mUnknown2C = 0;             // +0x2c
	bool mUnknown30 = false;        // +0x30

	MatchInfo();
	void Reset();
};
extern int gMatchInfoCount;  // 0x84e890
extern int gSparkDelay;      // 0x85aef8: the delay FindMatches gives a gem's sparks (0x6a453b)

class Board : public TwistWidget {
public:
	// The game modes (+0x20c)
	enum { MODE_CASUAL = 0, MODE_CLASSIC = 1, MODE_CHALLENGE = 2, MODE_BLITZ = 3, MODE_ZEN = 4, MODE_PUZZLE = 5 };

	int mUnknown158 = 0;               // +0x158: the level (LevelValue)
	int mGameMode = 0;                 // +0x20c
	int mModeFlags = 0;                // +0x210: 8 for a replay
	WinTwistApp* mApp = nullptr;       // +0x954
	Image* mPuzzleImage = nullptr;     // +0x1320: the puzzle's picture (mode 5), cut into the gems
	bool mUnknown1324 = false;         // +0x1324
	int mRows = 0;                     // +0x1330
	int mCols = 0;                     // +0x1334
	Piece** mGrid = nullptr;           // +0x1338: mRows x mCols, row by row
	// +0x149c: the electro gems' strikes (the class at 0x71eb9e, not yet ported); ClearBoard deletes them
	std::vector<void*> mUnknown149C;
	std::vector<Piece*> mBombList;     // +0x14bc: bombs about to go off, drawn over the board (Piece::DrawFront)
	std::map<int, Piece*> mPieceMap;   // +0x1364: the pieces by id (0x6b8c20 adds, 0x6b677b drops)
	float mShineAmount = 1.0f;            // +0x163c
	float mPieceAlpha = 1.0f;          // +0x1640
	EffectsManager* mFXManager = nullptr;  // +0x1648
	EffectsManager* mFXManager2 = nullptr; // +0x164c
	int mUnknown15B4 = 0;              // +0x15b4: the mouse, over the board
	int mUnknown15B8 = 0;              // +0x15b8
	// The config's level values (0x6c6469 reads them)
	LevelValue mLockGem;               // +0x1ca0
	LevelValue mSkulls;                // +0x1cf8
	LevelValue mCounter;               // +0x1d50
	LevelValue mBadGemsOnBoardMax;     // +0x1da8
	LevelValue mUnknown1E00;           // +0x1e00
	LevelValue mLevelUpPoints;         // +0x1e58
	LevelValue mDevil;                 // +0x1eb0: a doom gem's count
	LevelValue mCoalGem;               // +0x1f08
	int mUnknown1F78 = 0;              // +0x1f78
	bool mUnknown1BF2 = false;         // +0x1bf2: coal is drawn as snackers
	Image* mShineImage = nullptr;      // +0x1c54
	int mUnknown1C5C = 0;              // +0x1c5c
	int mUnknown1C58 = 0;              // +0x1c58
	float mUnknown2038 = 0;            // +0x2038
	int mUnknown20DC = 0;              // +0x20dc
	float mUnknown2108 = 0;            // +0x2108
	int mSparkBlock = 0;               // +0x21ac: no sparks while above 0
	float mUnknown21B0 = 0;         // +0x21b0
	bool mDebugPieces = false;         // +0x21db: each piece shows its row and column
	float mUnknown23EC = 0;            // +0x23ec
	bool mUnknown23F5 = false;         // +0x23f5
	// +0x2584: the tutorial being run (Tutorial.h), which can point at a piece (its +0x1a4)
	Tutorial* mUnknown2584 = nullptr;
	bool mUnknown25A4 = false;         // +0x25a4
	float mUnknown268C = 1.0f;         // +0x268c
	float mShakeX = 0;                 // +0x2690
	float mShakeY = 0;                 // +0x2694
	bool mUnknown37C4 = false;         // +0x37c4
	bool mUnknown37F8 = false;         // +0x37f8: no sounds or marks for the gems made (loading a game)
	bool mUnknown3904 = false;         // +0x3904
	float mUnknown380C = 0;            // +0x380c
	int mUnknown3804 = 0;              // +0x3804: the turn's rating (0x6a5179)
	float mUnknown3924 = 1.0f;         // +0x3924
	int mBoardX = 0;                   // +0x3474: the grid's left edge
	// The rest of the fields the constructor (0x6c4df9) sets, named by their offset until their use is known
	int mUnknown15C = 0;                    // +0x15c
	int mUnknown160 = 0;                    // +0x160
	int mUnknown168 = 0;                    // +0x168: the score
	int mUnknown16C = 0;                    // +0x16c
	int mUnknown170 = 0;                    // +0x170
	int mUnknown1E8 = 0;                    // +0x1e8
	int mUnknown1EC = 0;                    // +0x1ec
	int mUnknown1F0 = 0;                    // +0x1f0
	int mUnknown1F4 = 0;                    // +0x1f4
	int mUnknown1F8 = 0;                    // +0x1f8
	int mUnknown1FC = 0;                    // +0x1fc
	int mUnknown200 = 0;                    // +0x200
	int mUnknown204 = 1;                    // +0x204
	int mUnknown218 = 0;                    // +0x218
	int mUnknown21C = 0;                    // +0x21c: only saved and loaded
	int mUnknown224 = 0;                    // +0x224
	int mUnknown234 = 0;                    // +0x234: the most moves in a level
	float mUnknown248 = 0;                  // +0x248
	float mUnknown230 = 0;                  // +0x230: up 0.01 each update while +0x15c is 0
	int mUnknown250 = -1;                   // +0x250
	int mUnknown254 = -1;                   // +0x254
	int mUnknown258 = -1;                   // +0x258
	int mUnknown420 = 0;                    // +0x420
	CurvedVal mUnknown498;                  // +0x498: set by 0x6b40b5; while it is not 0 the rank bar draws the
	                                        // overlays above priority 50 under it
	bool mUnknown6CC = false;               // +0x6cc
	int mUnknown6D0 = 0;                    // +0x6d0
	int mUnknown70C = 0;                    // +0x70c
	int mUnknown710 = 0;                    // +0x710
	int mUnknown814 = 0;                    // +0x814
	int mUnknown818 = 0;                    // +0x818
	int mUnknown82C = 0;                    // +0x82c: the tip shown, from the profile's count
	CurvedVal mUnknown830;                  // +0x830: the tip's slide
	SexyString mUnknown8A0;                 // +0x8a0: the tip's text
	float mUnknown8C0 = 0;                  // +0x8c0: the tip's x
	float mUnknown8BC = 0;                  // +0x8bc: the tip's wait before it slides back
	int mUnknown131C = -1;                  // +0x131c
	int mUnknown132C = 1;                   // +0x132c
	// +0x135c: a value per cell (AllocGrid)
	Piece** mUnknown135C = nullptr;  // the grid kept while a turn is tried (0x6a3b00 and 0x6a3b26)
	// +0x1360: a record per cell (0x44 bytes, 0x6a38a7), row by row
	struct Cell {
		bool mUnknown0 = false;
		float mUnknown4 = 0;
		int mRow = 0;              // +0x8
		int mCol = 0;              // +0xc
		bool mUnknown10 = false;
		Color mColor = Color(0, 0, 0, 0);  // +0x14: Color() with its alpha then cleared
		float mUnknown24 = 0;
		float mUnknown28 = 0;
		bool mUnknown2C = false;
		float mUnknown30;          // +0x30: not set by the constructor; the tile's turn, 1 down to 0 (UpdateLevelUp)
		bool mUnknown34 = false;
		int mUnknown38 = 0;
		float mUnknown3C = 0;
		float mUnknown40 = 0;
	};
	Cell* mCells = nullptr;
	int* mUnknown1370 = nullptr;            // +0x1370: a value per column (ReadConfig)
	bool mUnknown1374 = false;              // +0x1374
	int mUnknown1378 = 3;                   // +0x1378
	bool mUnknown137C = false;              // +0x137c
	bool mUnknown137D = false; // +0x137d
	bool mUnknown137E = false;              // +0x137e
	bool mUnknown137F = false;              // +0x137f
	bool mUnknown1380 = false;              // +0x1380
	bool mUnknown1381 = false;              // +0x1381
	bool mUnknown1382 = false;              // +0x1382
	bool mUnknown1383 = false;              // +0x1383
	bool mUnknown1384 = false;              // +0x1384
	int mUnknown1388 = 0;                   // +0x1388
	int mUnknown138C = 0;                   // +0x138c
	bool mUnknown1390 = true;               // +0x1390
	int mUnknown1394 = 0;                   // +0x1394
	bool mUnknown1398 = false;              // +0x1398
	Widget* mUnknown139C = nullptr;                   // +0x139c
	int mUnknown13A0 = 0;                   // +0x13a0
	int mUnknown13A4 = 0;                   // +0x13a4
	bool mUnknown13A8 = true;               // +0x13a8
	CurvedVal mUnknown8C8;                  // +0x8c8: runs every update (twice in zen)
	CurvedVal mUnknown2898;                 // +0x2898: runs every update
	CurvedVal mUnknown36C0;                 // +0x36c0: while it runs, +0x37a0 fades
	CurvedVal mUnknown3730;                 // +0x3730: while it runs, +0x37a4 fades
	CurvedVal mUnknown3478;                 // +0x3478: the grid's slide to its place
	CurvedVal mUnknown3810;                 // +0x3810: the new level's (0x6d090f)
	CurvedVal mUnknown3880;                 // +0x3880: set by 0x6b681d
	CurvedVal mUnknown34F0;                 // +0x34f0: set when a turn ends (0x6b8bca)
	CurvedVal mUnknown13B0;                 // +0x13b0: set to 0 by StartLevel
	// +0x27b8 and the five linked to it: the nuke at the end of a game (EndGame)
	CurvedVal mUnknown27B8;                 // +0x27b8
	CurvedVal mUnknown2828;                 // +0x2828
	CurvedVal mUnknown26D8;                 // +0x26d8
	CurvedVal mUnknown2748;                 // +0x2748
	CurvedVal mUnknown2908;                 // +0x2908
	CurvedVal mUnknown2978;                 // +0x2978
	bool mUnknown1420 = false;              // +0x1420
	float mUnknown1424 = 0;                 // +0x1424: the glint's run, 0 to 1
	float mUnknown1428 = 0;                 // +0x1428
	float mUnknown142C = 0;                 // +0x142c
	std::deque<TwistWidget*> mUnknown23DC;  // +0x23dc: the dialogs waiting to come up (0x6a6e36 its size)
	std::vector<FlyingText*> mFlyingTexts;  // +0x1f98: the praise and other words over the board (0x6a6d18 its size)
	std::vector<GemSoul*> mUnknown26B0;     // +0x26b0: the souls of the last level's special gems (0x6a6eaa its size)
	// +0x1650: lights over the gems (0x2c bytes each), added by UpdateGame
	struct Light {
		float mX = 0;
		float mY = 0;
		float mUnknown8 = 0;
		float mUnknownC = 0;
		float mUnknown10 = 0;
		float mUnknown14 = 0;
		bool mUnknown18 = false;
		float mUnknown1C = 0;
		float mUnknown20 = 0;
		Piece* mPiece = nullptr;  // +0x24: the gem it follows, or none
		int mPieceId = -1;        // +0x28
	};
	std::vector<Light> mUnknown1650;
	std::vector<Shockwave*> mUnknown1660;   // +0x1660: the waves that shake the gems (0x6af497)
	std::vector<Traveller*> mUnknown2170;   // +0x2170: the points' smoke on its way to the tube (0x6af0e2)
	int mUnknown203C = 0;                   // +0x203c: the puzzle fill's wait
	// +0x21e4: objects of a class with its update at 0x6f465f (not yet ported)
	std::vector<void*> mUnknown21E4;
	std::vector<Piece*> mUnknown20F4;       // +0x20f4: the gems the twist turns (SnapPiecesX moves them too)
	Piece* mUnknown1430 = nullptr;          // +0x1430: a piece RemovePiece forgets
	int mUnknown1434 = 0;                   // +0x1434
	bool mUnknown1438 = false;              // +0x1438
	int mUnknown143C = 0;                   // +0x143c
	int mUnknown1440 = 1;                   // +0x1440
	int mUnknown1444 = -1;                  // +0x1444
	int mUnknown1448 = -1;                  // +0x1448
	int mUnknown144C = -1;                  // +0x144c
	float mUnknown1450 = 1.0f;              // +0x1450
	float mUnknown1454 = 0;                   // +0x1454
	float mUnknown1458 = 0;                 // +0x1458: the twist's step, in degrees an update
	float mUnknown145C = 1.0f;              // +0x145c
	int mUnknown1460 = 0;                   // +0x1460
	int mUnknown1464 = 0;                   // +0x1464: only saved and loaded
	int mUnknown1468 = 0;                   // +0x1468: only saved and loaded
	int mUnknown146C = 0;                   // +0x146c
	int mUnknown1470 = 0;                   // +0x1470
	TwistWidget* mUnknown1474 = nullptr;    // +0x1474: the trial's limit dialog (0x73a458), never in a full version
	bool mUnknown1478 = false;              // +0x1478
	int mUnknown1488 = 0;                   // +0x1488
	int mUnknown14DC = 0;                   // +0x14dc
	int mUnknown14E0 = 0;                   // +0x14e0
	int mUnknown14E4 = 0;                   // +0x14e4
	int mUnknown14E8 = 0;                   // +0x14e8
	int mUnknown14EC = 0;                   // +0x14ec
	int mUnknown14F0 = 0;                   // +0x14f0
	int mUnknown14F4 = 0;                   // +0x14f4
	int mUnknown14F8 = 0;                   // +0x14f8
	int mUnknown14FC = 0;                   // +0x14fc
	int mUnknown1500 = 0;                   // +0x1500
	int mUnknown1504 = 0;                   // +0x1504
	int mUnknown1508 = 0;                   // +0x1508
	bool mUnknown150C = true;               // +0x150c
	int mUnknown1510 = 0;                   // +0x1510
	int mUnknown1514 = 0;                   // +0x1514
	float mUnknown1518 = 0;                 // +0x1518: the shake's size
	float mUnknown151C = 0;                 // +0x151c: the shake, fading to 0
	int mUnknown1520 = 0;                   // +0x1520
	int mUnknown1524 = 0;                   // +0x1524
	int mUnknown1528 = 0;                   // +0x1528: power gems the burst sets off (never, see BurstAt)
	bool mUnknown152C = false;              // +0x152c
	int mUnknown1530 = 0;                   // +0x1530
	int mUnknown1534 = 0;                   // +0x1534
	int mUnknown1538 = 0;                   // +0x1538
	int mUnknown153C = 0;                   // +0x153c
	int mUnknown1540 = 0;                   // +0x1540
	int mUnknown1544 = 0;                   // +0x1544
	int mUnknown1548 = 0;                   // +0x1548
	double mUnknown1550 = 0;                // +0x1550
	bool mUnknown1558 = true;               // +0x1558
	bool mUnknown1559 = true;               // +0x1559
	bool mUnknown155A = false;              // +0x155a
	int mUnknown155C = -1;                  // +0x155c
	int mUnknown1560 = -1;                  // +0x1560
	int mUnknown1564 = -1;                  // +0x1564
	int mUnknown1568 = -1;                  // +0x1568
	bool mUnknown156C = false;              // +0x156c
	int mUnknown1570 = 0;                   // +0x1570
	int mUnknown1574 = 0;                   // +0x1574
	int mUnknown1580 = 0;                   // +0x1580
	bool mUnknown15A8 = false;              // +0x15a8
	int mUnknown15AC = 23;                  // +0x15ac
	bool mUnknown15B0 = false;              // +0x15b0
	int mUnknown15BC = 0;                   // +0x15bc: -autoplay's click
	int mUnknown15C0 = 0;                   // +0x15c0
	int mUnknown15C4 = 0;                   // +0x15c4: the update count of -autoplay's click, 0 with none
	int mUnknown15C8 = 0;                   // +0x15c8: the last click on the board
	int mUnknown15CC = 0;                   // +0x15cc
	bool mUnknown15D0 = false;              // +0x15d0
	bool mUnknown15D1 = false;              // +0x15d1: +0x15d0 of the update before
	int mUnknown15D4 = 0;                   // +0x15d4
	bool mUnknown15D8 = false;              // +0x15d8
	int mUnknown15DC = 0;                   // +0x15dc
	bool mUnknown15EC = false;              // +0x15ec
	int mUnknown15F4 = 0;                   // +0x15f4
	bool mUnknown15FC = true;               // +0x15fc
	bool mUnknown15FD = false;              // +0x15fd
	int mUnknown1604 = 0;                   // +0x1604
	int mUnknown1610 = 0;                   // +0x1610
	float mUnknown1614 = 0;                 // +0x1614
	double mUnknown1618 = 0;                // +0x1618
	double mUnknown1620 = 0;                // +0x1620
	double mUnknown1628 = 0;                // +0x1628
	bool mUnknown1638 = false;              // +0x1638
	bool mUnknown1644 = false;              // +0x1644
	int mUnknown1670 = 0;                   // +0x1670
	int mUnknown1BEC = 0;                   // +0x1bec
	bool mUnknown1BF0 = false;              // +0x1bf0
	bool mUnknown1BF1 = false;              // +0x1bf1
	int mUnknown1BF4 = 7;                   // +0x1bf4
	int mUnknown1C00 = 0;                   // +0x1c00
	int mUnknown1C04 = 0;                   // +0x1c04
	int mUnknown1C08 = 0;                   // +0x1c08
	int mUnknown1C0C = 0;                   // +0x1c0c
	int mUnknown1C10 = 0;                   // +0x1c10
	int mUnknown1C14 = 0;                   // +0x1c14
	int mUnknown1C18 = 0;                   // +0x1c18
	int mUnknown1C1C = 0;                   // +0x1c1c
	int mUnknown1C20 = 0;                   // +0x1c20
	int mUnknown1C24 = 0;                   // +0x1c24
	int mUnknown1C28 = 0;                   // +0x1c28
	int mUnknown1C34 = 0;                   // +0x1c34
	float mUnknown1C38 = 1.0f;              // +0x1c38
	int mUnknown1C3C = 0;                   // +0x1c3c
	bool mUnknown1C40 = false;              // +0x1c40
	bool mUnknown1C41 = true;               // +0x1c41
	bool mUnknown1C42 = false;              // +0x1c42
	float mUnknown1C48 = 0;                   // +0x1c48
	int mUnknown1C4C = 0;                   // +0x1c4c
	int mUnknown1C50 = 0;                   // +0x1c50
	int mUnknown1C64 = -1;                  // +0x1c64
	int mUnknown1C74 = 0;                   // +0x1c74
	float mUnknown1C84 = 0;                   // +0x1c84
	float mUnknown1C88 = 1.0f;              // +0x1c88
	float mUnknown1C8C = 1.0f;              // +0x1c8c
	bool mUnknown1C90 = false;              // +0x1c90
	bool mUnknown1C98 = false;              // +0x1c98: IceGemsAllowed
	int mUnknown1F64 = 0;                   // +0x1f64
	int mUnknown1F68 = 0;                   // +0x1f68
	int mUnknown1F74 = 0;                   // +0x1f74
	int mUnknown1F7C = 0;                   // +0x1f7c
	int mUnknown1F80 = -1;                  // +0x1f80
	int mUnknown1F84 = -1;                  // +0x1f84
	Image* mBackdrop = nullptr;             // +0x1fa8: imagesackdrops	wist-back<n>.png, which the game does not have
	float mUnknown1FAC = 0;                   // +0x1fac
	float mUnknown1FB0 = 0;                   // +0x1fb0
	int mUnknown1FB8 = 0;                   // +0x1fb8
	int mUnknown1FC4 = 0;                   // +0x1fc4
	int mUnknown1FC8 = 0;                   // +0x1fc8
	int mUnknown1FE8 = 0;                   // +0x1fe8: the matched gems' mean x (FindMatches)
	int mUnknown1FEC = 0;                   // +0x1fec: and y
	int mUnknown1FF0 = 0;                   // +0x1ff0: their count
	bool mUnknown1FF4 = false;              // +0x1ff4
	bool mUnknown1FF5 = false;              // +0x1ff5
	int mUnknown1FF8 = 0;                   // +0x1ff8
	int mUnknown1FFC = 0;                   // +0x1ffc
	int mUnknown2000 = 0;                   // +0x2000
	bool mUnknown2004 = false;              // +0x2004
	int mUnknown2008 = 0;                   // +0x2008
	Hyperspace* mHyperspace = nullptr;      // +0x200c: the space behind the board, during a jump
	int mUnknown2010 = 0;                   // +0x2010
	bool mUnknown2014 = false;              // +0x2014
	bool mUnknown2015 = false;              // +0x2015
	bool mUnknown2016 = false;              // +0x2016
	int mUnknown2018 = 0;                   // +0x2018
	int mUnknown201C = -1;                  // +0x201c
	int mUnknown2020 = -1;                  // +0x2020
	bool mUnknown2030 = false;              // +0x2030
	int mUnknown2034 = 0;                   // +0x2034
	bool mUnknown2040 = true;               // +0x2040
	bool mUnknown2041 = true;               // +0x2041
	bool mUnknown2042 = true;               // +0x2042
	bool mUnknown2043 = true;               // +0x2043
	bool mUnknown2054 = false;              // +0x2054
	float mUnknown2058[24] = {};            // +0x2058: 24 glows that run round (UpdateGlows)
	float mUnknown20B8 = 0;                   // +0x20b8
	int mUnknown20BC = -1;                  // +0x20bc
	float mUnknown20C0 = 0;                 // +0x20c0: the cursor's turn while twisting
	float mUnknown20C4 = 0;                 // +0x20c4
	float mUnknown20C8 = 0;                   // +0x20c8
	float mUnknown20CC = 0;                   // +0x20cc
	bool mUnknown20D0 = true;               // +0x20d0
	float mUnknown20D4 = 0;                 // +0x20d4
	bool mUnknown20D8 = false;              // +0x20d8
	int mUnknown20E4 = 0;                   // +0x20e4
	float mUnknown20E8 = 0.1f;              // +0x20e8
	float mUnknown20EC = 0;                 // +0x20ec: holds the progress bar back
	bool mUnknown210C = false;              // +0x210c
	int mUnknown2110 = 0;                   // +0x2110
	int mUnknown2114 = 0;                   // +0x2114
	float mUnknown2118 = 3.0f;              // +0x2118
	std::vector<std::pair<int, int>> mUnknown2120;  // +0x2120: the cursor's cells that cannot turn (row, column)
	bool mUnknown2130 = false;              // +0x2130
	float mUnknown2150 = 0;                   // +0x2150
	int mUnknown2154 = 0;                   // +0x2154
	int mUnknown2158 = 0;                   // +0x2158
	int mUnknown215C = -1;                  // +0x215c
	int mUnknown2160 = 1;                   // +0x2160
	float mUnknown2164 = 0;                   // +0x2164
	int mUnknown2168 = 0;                   // +0x2168
	bool mUnknown216C = false;              // +0x216c
	int mUnknown2188 = 0;                   // +0x2188
	bool mUnknown218C = false;              // +0x218c
	bool mUnknown2198 = false;              // +0x2198: the locks may count down (0x6aa0b7)
	bool mUnknown2199 = false;              // +0x2199
	int mUnknown219C = 0;                   // +0x219c
	bool mUnknown21A0 = false;              // +0x21a0
	bool mUnknown21A2 = false;              // +0x21a2
	bool mUnknown21A3 = false;              // +0x21a3
	Widget* mUnknown21A8 = nullptr;                   // +0x21a8
	SoundInstance* mUnknown21C4[2] = {};    // +0x21c4: never set; UpdateGlows stops them in turn
	int mUnknown21CC = 0;                   // +0x21cc
	int mUnknown21D0 = 1;                   // +0x21d0
	int mModeMultiplier = 1;                // +0x21d4
	bool mUnknown21D8 = false;              // +0x21d8
	bool mUnknown21D9 = false;              // +0x21d9
	bool mUnknown21DA = false;              // +0x21da
	int mUnknown21DC = 0;                   // +0x21dc
	int mUnknown21F4 = -1;                  // +0x21f4
	bool mUnknown2200 = false;              // +0x2200
	TwistWidget* mUnknown2208 = nullptr;    // +0x2208: the dialog over the board (from the queue at +0x23dc)
	float mUnknown2228 = 0;                 // +0x2228: 0.5 with each chain
	int mUnknown222C = 0;                   // +0x222c
	int mUnknown2234[84] = {};              // +0x2234
	Buffer mSaveBuffer;                     // +0x2384: the game as StartLevel saved it (0x6b0d91)
	Buffer mUnknown23B4;                    // +0x23b4
	int mUnknown23A4 = -1;                  // +0x23a4
	int mUnknown23A8 = -1;                  // +0x23a8
	int mUnknown23AC = 0;                   // +0x23ac
	int mUnknown23B0 = 0;                   // +0x23b0
	int mUnknown23D4 = 0;                   // +0x23d4
	float mUnknown23F0 = 0.01f;             // +0x23f0
	bool mUnknown23F4 = false;              // +0x23f4
	float mUnknown23F8 = 0;                 // +0x23f8: the tutorial arrow's fade
	float mUnknown23FC = 0;                 // +0x23fc: the cursor's glow
	int mUnknown2400 = -10000;              // +0x2400
	int mUnknown2404 = -10000;              // +0x2404
	std::vector<int> mUnknown2408[20];      // +0x2408: the tutorial's gems for each column, the next at the back
	int mUnknown2548 = -1;                  // +0x2548
	Buffer mUnknown254C;                    // +0x254c: the tutorial script's lines (properties/tutorial.txt, read by ShowNextTip)
	std::map<int, int> mUnknown256C;        // +0x256c: the tips waiting for their dialog, by kind, with a gem's id
	int mUnknown2578 = 0;                   // +0x2578
	int mUnknown257C = 0;                   // +0x257c
	int mUnknown2580 = 1;                   // +0x2580
	bool mUnknown25A5 = false;              // +0x25a5
	bool mUnknown25A6 = false;              // +0x25a6
	int mUnknown25A8 = 0;                   // +0x25a8
	int mUnknown25C8 = 0;                   // +0x25c8
	int mUnknown25CC = 0;                   // +0x25cc
	float mUnknown25D0 = 0;                 // +0x25d0: the wait for a hint, in seconds
	float mUnknown25D4 = 0;                 // +0x25d4: the hint's sparkles' turn
	int mUnknown25D8 = -1;                  // +0x25d8: the hint's row (FindMove)
	int mUnknown25DC = -1;                  // +0x25dc: the hint's column
	Widget* mUnknown2624 = nullptr;         // +0x2624: a widget with a state at +0x160 (not yet known)
	bool mUnknown2628 = false;              // +0x2628
	bool mUnknown2629 = false;              // +0x2629
	bool mUnknown262A = false;              // +0x262a
	int mUnknown262C = -1;                  // +0x262c
	int mUnknown2630 = 0;                   // +0x2630
	int mUnknown2634 = 0;                   // +0x2634
	int mUnknown2638 = 0;                   // +0x2638
	int mUnknown263C = 0;                   // +0x263c
	int mUnknown2640 = 0;                   // +0x2640
	int mUnknown265C = 0;                   // +0x265c
	int mUnknown2660 = 0;                   // +0x2660
	int mUnknown2664 = 0;                   // +0x2664
	bool mUnknown2669 = false;              // +0x2669
	bool mUnknown266A = false;              // +0x266a
	int mUnknown2678 = 0;                   // +0x2678
	Widget* mUnknown267C = nullptr;                   // +0x267c
	int mUnknown2680 = 0;                   // +0x2680
	bool mUnknown2698 = true;               // +0x2698
	int mUnknown269C = 0;                   // +0x269c
	TwistWidget* mUnknown26A0 = nullptr;                   // +0x26a0
	float mUnknown26A4 = 0;                 // +0x26a4: 0.01 when a soul leaves (UpdateJump)
	int mUnknown26A8 = 0;                   // +0x26a8: the jump between levels (UpdateJump): 1 the show, 2 the rows, 4, 0
	TwistWidget* mUnknown26AC = nullptr;                   // +0x26ac: the LevelUpShow (ShowLevelUp)
	int mUnknown26C0 = 0;                   // +0x26c0: the row falling off next (end state 3)
	float mUnknown26C4 = 0;                 // +0x26c4: the end states' wait
	int mUnknown26C8 = 0;                   // +0x26c8: the end of the game's state (UpdateGameOver), 1 from EndGame
	Piece* mUnknown26CC = nullptr;          // +0x26cc: the gem that ended the game (a counter at zero)
	TwistWidget* mUnknown26D0 = nullptr;                   // +0x26d0
	int mUnknown3444 = 0;                   // +0x3444
	int mUnknown3448 = 0;                   // +0x3448
	int mUnknown344C = 0;                   // +0x344c
	int mUnknown3450 = 0;                   // +0x3450
	float mUnknown3454 = 0;                   // +0x3454
	float mUnknown345C = 0;                 // +0x345c
	float mUnknown3460 = 8.0f;              // +0x3460
	TextImage* mUnknown3464 = nullptr;      // +0x3464: blitz's FINAL SCORE (SetEndState 4)
	TextImage* mUnknown3468 = nullptr;      // +0x3468: and its score
	float mUnknown346C = 0;                 // +0x346c: their fade (end states 4 to 8)
	float mUnknown3470 = 0;                 // +0x3470: end state 4's wait
	int mUnknown34E8 = 0;                   // +0x34e8
	int mUnknown34EC = 0;                   // +0x34ec
	int mUnknown3560[84] = {};              // +0x3560
	bool mUnknown36B0 = false;              // +0x36b0
	bool mUnknown36B1 = true;               // +0x36b1
	int mUnknown36B4 = 0;                   // +0x36b4
	int mUnknown36B8 = 0;                   // +0x36b8
	float mUnknown37A0 = 0;                   // +0x37a0
	float mUnknown37A4 = 0;                   // +0x37a4
	float mUnknown37A8 = 0;                   // +0x37a8
	float mUnknown37AC = 0;                   // +0x37ac
	bool mUnknown37B0 = true; // +0x37b0
	bool mUnknown37B1 = true; // +0x37b1
	int mUnknown37B4 = 15;                  // +0x37b4
	int mUnknown37C8 = 0;                   // +0x37c8
	float mUnknown37EC = 0;                 // +0x37ec
	float mUnknown37F0 = 0;                 // +0x37f0
	float mUnknown37F4 = 0;                   // +0x37f4
	float mUnknown1630 = 0;                   // +0x1630: the wait to the next ring of tiles
	int mUnknown1634 = 0;                     // +0x1634: the ring's distance from the middle
	int mUnknown37FC = 0;                   // +0x37fc
	TwistWidget* mUnknown3808 = nullptr;                   // +0x3808
	int mUnknown38F0 = 0;                   // +0x38f0
	int mUnknown38F4 = 0;                   // +0x38f4
	float mUnknown38F8 = 0;                   // +0x38f8
	float mUnknown38FC = 0;                   // +0x38fc
	float mUnknown3900 = 0;                   // +0x3900
	bool mUnknown3905 = false;              // +0x3905
	int mUnknown3908 = 0;                   // +0x3908
	int mUnknown390C = 0;                   // +0x390c
	int mUnknown3910 = 0;                   // +0x3910
	int mUnknown3914 = 0;                   // +0x3914
	int mUnknown3918 = 0;                   // +0x3918
	float mUnknown391C = 0;                   // +0x391c
	bool mUnknown3920 = false;              // +0x3920
	bool mUnknown3921 = false;              // +0x3921
	int mUnknown3928 = 0;                   // +0x3928
	int mUnknown392C = 0;                   // +0x392c
	bool mUnknown3930 = false;              // +0x3930
	int mUnknown3934 = 0;                   // +0x3934
	// The fields the constructor sets from other values
	int mUnknown154 = 0;                    // +0x154
	bool mUnknown214 = false;               // +0x214
	float mUnknown220 = 1.0f;               // +0x220
	int mUnknown1328 = 0;                   // +0x1328: how many of the player's +0x80 flags are set
	float mUnknown133C[8] = {};               // +0x133c
	Font* mUnknown1578 = nullptr;           // +0x1578
	Font* mUnknown157C = nullptr;           // +0x157c
	int mUnknown1584 = 0;                   // +0x1584
	int mUnknown1588 = 0;                   // +0x1588
	MemoryImage* mUnknown15F0 = nullptr;    // +0x15f0: 42x42
	// +0x1674: a 7x7 table over a cell, 14 apart from 0 to 84
	struct CellPoint {
		float mX, mY;      // +0x0, +0x4
		float mU, mV;      // +0x8: mX / 84 / 40, +0xc: mY / 84
		float mAngle;      // +0x10: from the middle (42, 42), plus 6.28318
		float mDist;       // +0x14: from the middle
		float mUnknown18;  // +0x18: 1
	};
	CellPoint mCellPoints[49] = {};
	float mUnknown1C2C = 0;                 // +0x1c2c: the app's middle
	float mUnknown1C30 = 0;                 // +0x1c30
	Color mUnknown1F88;                     // +0x1f88: filled over the board while its alpha (+0x1f94) counts down
	int mUnknown1FBC = 0;                   // +0x1fbc
	int mUnknown1FC0 = 0;                   // +0x1fc0
	float mUnknown20E0 = 0;                 // +0x20e0
	void* mUnknown211C = nullptr;           // +0x211c: a 0x10-byte object holding the app (0x74c8c8)
	int mUnknown2190 = 0;                   // +0x2190
	GoodTube* mGoodTube = nullptr;          // +0x2194: the tube at the side
	EffectsManager* mFXManager3 = nullptr;  // +0x21b8
	float mUnknown21BC = 0;                 // +0x21bc
	float mUnknown21C0 = 0;                 // +0x21c0
	int mUnknown21E0 = 0;                   // +0x21e0: 1 to 5, mostly 2
	PointsManager* mPointsManager = nullptr;  // +0x21fc: the points that float up
	float mUnknown2224 = 0;                 // +0x2224
	float mUnknown23D8 = 0;                 // +0x23d8
	Image* mGridBarTop = nullptr;           // +0x2588
	Image* mGridBarBot = nullptr;           // +0x258c
	int mUnknown25AC = 0;                   // +0x25ac: the gap before the last quick move
	int mUnknown25B0 = 0;                   // +0x25b0: +0x25a8 at the last quick move
	int mUnknown25B4 = 0;                   // +0x25b4
	std::vector<int> mUnknown25B8;          // +0x25b8: the gems of the move FindMove found
	MatchInfo mUnknown25E0;                 // +0x25e0: FindMove's match for each cell
	std::vector<MatchInfo> mUnknown2614;    // +0x2614: the matches made
	int mUnknown2644 = 0;                   // +0x2644
	int mBonusChallengeMinGems = 0;         // +0x266c
	int mUnknown2674 = 0;                   // +0x2674: the challenge's number (mode 2)
	int mUnknown3458 = 0;                   // +0x3458: blitz's 30000
	std::map<int, int> mUnknown37B8;        // +0x37b8
	MTRand mRand;                           // +0x958: seeded from Rand()
	MTRand mUnknown2A80;                    // +0x2a80: seeded from the update count while +0x2629 is set
	// Read by ReadConfig (0x6c6469)
	int mUnknown1BFC = 0;                   // +0x1bfc
	bool mUnknown1BF8 = false;              // +0x1bf8
	int mUnknown1C68 = 0;                   // +0x1c68
	bool mUnknown1C6C = false;              // +0x1c6c
	bool mUnknown1C6D = false;              // +0x1c6d
	bool mUnknown1C6E = false;              // +0x1c6e
	bool mUnknown1C6F = false;              // +0x1c6f
	bool mUnknown1C70 = false;              // +0x1c70
	bool mUnknown1C71 = false;              // +0x1c71
	bool mUnknown1C78 = false;              // +0x1c78
	bool mUnknown1C79 = false;              // +0x1c79
	bool mUnknown1C7A = false;              // +0x1c7a
	bool mUnknown1C7B = false;              // +0x1c7b
	int mLaserGemCascades = 0;              // +0x1c80
	int mFruitFirstLevel = 0;               // +0x1c94
	int mAmnestyGemCount = 0;               // +0x1c9c
	int mCoalMax = 0;                       // +0x1f60
	bool mUnknown1FB4 = false;              // +0x1fb4
	int mBonusRoundLevelGap = 0;            // +0x2104
	bool mUnknown2138 = false;              // +0x2138
	std::vector<int> mUnknown2140;          // +0x2140: cleared by FindMatches
	int mTimeFreezeMatchCount = 0;          // +0x21b4
	int mBonusChallengeMaxGems = 0;         // +0x2670
	int mMaxLocksOnScreen = 0;              // +0x3800
	std::vector<void*> mUnknown14AC;        // +0x14ac: malloc'd blocks ClearBoard frees (not yet known)
	int mUnknown2180 = 0;                   // +0x2180
	int mUnknown2184 = 0;                   // +0x2184
	int mUnknown2204 = 0;                   // +0x2204
	// Set by NewLevel (0x6bf659) and the level's progress
	bool mUnknown164 = false;               // +0x164
	int mUnknown208 = 0;                    // +0x208
	int mUnknown228[2] = {};                // +0x228: [0] the level's longest chain
	int mUnknown238 = 0;                    // +0x238
	int mUnknown23C = 0;                    // +0x23c: the twists made in the level
	int mUnknown240 = 0;                    // +0x240: the twists that made no match
	int mUnknown244 = 0;                    // +0x244: the chains made
	int mUnknown1C44 = 0;                   // +0x1c44
	bool mUnknown1F6C = false;              // +0x1f6c
	int mUnknown1F70 = 0;                   // +0x1f70: the level modulo 7
	float mUnknown20F0 = 0;                   // +0x20f0
	float mUnknown2134 = 1.0f;              // +0x2134
	bool mUnknown21A1 = false;              // +0x21a1: past the first level
	int mUnknown21F8 = 0;                   // +0x21f8: 100 at a level's start (0x6a3ff4)
	bool mUnknown2201 = false;              // +0x2201
	int mUnknown2590 = 0;                   // +0x2590: the grid's width and a quarter of a cell (0x6a49a3)
	float mUnknown2594 = 1.0f;              // +0x2594
	float mUnknown25A0 = 0;                 // +0x25a0: the progress bar's glow
	float mUnknown2598 = 1.0f;              // +0x2598
	float mUnknown259C = 1.0f;              // +0x259c
	std::vector<int> mBonusColors;          // +0x2648: the bonus challenge's colours (0x6bbfe4)
	int mUnknown2658 = 0;                   // +0x2658: the bonus challenge's points
	bool mUnknown2668 = false;              // +0x2668
	CurvedVal mUnknown178;                  // +0x178: classic's progress curve (UpdateProgress)
	std::vector<SexyString> mTips;          // +0x81c: properties\tips.txt, a line each

	Board(WinTwistApp* theApp, int theMode, int theFlags, int theUnknown);  // 0x6c4df9
	~Board() override;

	void LoadConfig();                           // 0x6b2b77
	// 0x6c6469: the board's size, the level values and the rest of the config's numbers; theResize remakes the
	// grid when its size changed
	void ReadConfig(bool theResize);
	void ResizeBoard(int theRows, int theCols);  // 0x6c427e
	void ClearBoard();                           // 0x6afe3a
	void AllocGrid(int theRows, int theCols);    // 0x6b0886
	void MakeCells();                            // 0x6a3ea2
	void RemovePiece(Piece* thePiece, bool theDelete);  // 0x6aa076
	void NewLevel();                             // 0x6bf659
	void SetStat(int theStat, int theValue);     // 0x6a44e5: the game's stat and the app's copy (unless +0x37c4)
	void ResetLevelValues();                     // 0x6a3f77: the level values that start at this level
	void UpdateTubeProgress(bool theAtOnce);     // 0x6a434d
	// 0x6bd78e: the tube, whether the level is done and classic's curve; theNoCheck skips 0x6bbbc2
	void UpdateProgress(bool theNoCheck);
	bool CheckLevelDone();                       // 0x6ab195
	int GetBonusLevel(int theLevel) { return mGameMode == 4 ? mUnknown204 / 2000 : theLevel; }  // 0x6a6618
	void StartBonusChallenge(int theExcludeColor);  // 0x6bbfe4: the colours of the next bonus challenge

	// 0x6c9d0b: a new game, or the saved one unless the menu's play button resumes the game on the screen
	void StartGame();
	// 0x6c8e99: the game saved in theBuffer; false for none (port: the reading of a save is not ported yet)
	bool LoadGame(Buffer& theBuffer);
	// 0x6a6529: a save's version, and from version 60 its mode, flags, +0x154 and level; false for an older or empty one
	static bool ReadSaveHeader(Buffer& theBuffer, int* theVersion, int* theMode, int* theFlags, int* theUnknown154,
		int* theLevel);
	void SetScore(int theScore, bool theUnknown14F8);  // 0x6bd840
	// 0x6a5fa3: the board takes theHyperspace (a new one if null) and sets up the jump into the game
	void TakeHyperspace(Hyperspace* theHyperspace);
	// 0x6c77b1: after StartGame: the config again, the basics tutorial or the puzzle's board, the save and the tube
	void StartLevel();
	void UpdateChallengeTube(bool theAtOnce);    // 0x6af390: the tube shows how much of the challenge is left
	void SetBlitzColor();                        // 0x6bd1f0: the tube's colour by blitz's time left

	int GetSlideX() { return Ftol((double)mWidth * (double)0.635f - (double)((mCols * gGemWidth) / 2)); }  // 0x6a6afa
	void SnapPiecesX();                          // 0x6ab551: every piece to its column's x
	int GetRowAt(int theY);                      // 0x6a4888: -1 off the grid
	int GetColAt(int theX);                      // 0x6a4849: -1 off the grid
	void SetCursor(int theRow, int theCol);      // 0x6a9d2d: the selection's rectangle, two gems square
	// 0x6bd9ac: the cursor to theRow, theCol, with its sound, trail and the moves it shows
	void MoveCursor(int theRow, int theCol, bool theQuiet, MatchInfo* theInfo);
	// The twist (BoardTwist.cpp)
	void MouseMove(int x, int y) override;                       // 0x6a47ba
	void MouseDown(int x, int y, int theBtnNum, int theClickCount) override;  // 0x6c7232
	void MouseUp(int x, int y, int theClickCount) override;      // 0x6a52c1
	void MouseDrag(int x, int y) override;                       // 0x6a52cb
	void MouseWheel(int theDelta) override;
	void KeyDown(KeyCode theKey) override;
	void WarpToCursor();  // 0x6a45da: the mouse pointer onto the cursor's gem while +0x1430 holds a piece                       // 0x6ca439: pause, the wheel, the cursor's keys                      // 0x6a52d7
	void UpdateCursor(int x, int y);                             // 0x6bdbd9 (slot 70, the board's own)
	void StartTwist(bool theInstant, int theX, int theY);        // 0x6c6e56
	void SetupTwist(int theX, int theY);                         // 0x6bb171: the four gems at +0x20f4
	bool StartPieceMove(Piece* thePiece, int theDCol, int theDRow);  // 0x6a9d6f
	void FinishTwist(bool theStop);                              // 0x6bba82: the turned gems into their cells
	void CountDownBombs(bool theInstant);                        // 0x6aa0b7
	void LockPiece(Piece* thePiece);                             // 0x6a4e78
	void UnturnPieces();                                         // 0x6a5424
	void ClearTurning();                                         // 0x6a54cb
	void UpdateTwist();                                          // 0x6cfd85
	void EvaluateCursor(MatchInfo* theInfo);     // 0x6bd872: +0x1c40 a match, +0x1c41 the turn blocked
	void ClearCellHints();                       // 0x6a4764
	bool TestTwist(bool theTest, MatchInfo* theInfo);  // 0x6bb619: whether turning the cursor's 2x2 matches
	bool CannotTwist(bool theCheck);             // 0x6bb4b5: the cursor's 2x2 cannot turn (its cells in +0x2120)
	bool MatchAt(int theCol, int theRow, MatchInfo* theInfo);  // 0x6ba1b2
	bool MatchAny(MatchInfo* theInfo);           // 0x6ba4ab
	void SaveGrid();                             // 0x6a3b00
	void RestoreGrid();                          // 0x6a3b26
	bool FindMove(int* theRow, int* theCol, bool theAny);  // 0x6c14fc
	int FillPieces(bool theAtOnce);              // 0x6c25a4: new gems for the gaps; returns how many
	bool CanAddCounter();                        // 0x6a63c5
	bool HasChallengeFlag(int theFlag);          // the challenge's flags (+0x2630's +0x80) in a challenge
	void MakeCounter(Piece* thePiece);           // 0x6a50a9: a bomb with the level's counter
	void MakeCoal(Piece* thePiece);              // 0x6a5074
	void SaveGame(Buffer* theBuffer, bool theReplays);  // 0x6b0d91: theReplays adds the app's three at +0xfa0
	int FindMatches(unsigned int theFlags);      // 0x6cd468: bit 1 for tips, bit 2 for real; returns the matches
	int FinishMatches(std::vector<MatchInfo>& theMatches, std::set<int>& theTipGems);
	void MakeMatchSpecial(MatchInfo& theMatch, std::vector<Piece*>& theReset, bool thePower, bool theElectro);
	void MakePowerGem(Piece* thePiece, float theDelay);    // 0x6a5019
	void MakeElectroGem(Piece* thePiece, float theDelay);  // 0x6a4ef8
	void Make6Gem(Piece* thePiece, float theDelay);        // 0x6a4f72
	void MakeFruitGem(Piece* thePiece);          // 0x6c23bd
	void ClearHitCounts();                       // 0x6a4560
	void ClearSparkStarts();                     // 0x6a5f12
	void StartSparks(Piece* thePiece);           // 0x6a453b
	// 0x6bf983: the points of a match, popped up at theX, theY in theColor
	Piece* AddPoints(int theX, int theY, int thePoints, const Color& theColor, int theSerial, bool theUnknown1,
		bool theUnknown2, bool theFirst, bool theUnknown3);
	void ShowCountPopup();                       // 0x6b3ffc
	void ResetFruitCount();                      // 0x6b3615
	void MatchAfterMove();                       // 0x6cf3fd: the matches once the gems have moved, and the chain
	void PlayChainSound(int theChain);           // 0x6a674d
	// 0x6c3456: a move's match in quick succession (+0x25b4 of them, each within 100 ticks of the last and two gaps
	// in a row 150 at most); each sixth earns 500, 1000 or 2000 points and a banner
	void SpeedBonus();
	void ShowSpeedText(int theLevel);            // 0x6bc3e3: the banner and its sound
	void Refill();                               // 0x6c755e: once the matched gems have gone
	void StartUnknown3880();                     // 0x6b681d
	void StartUnknown34F0();                     // 0x6b8bca: a turn ends
	void ShowRankBonus(int theBonus);            // 0x6ab269: the rank bar (RankBonusWidget) with the bonus
	void AddStat(int theStat, int theDelta);     // 0x6a450f: the game's stat and the app's copy (unless +0x37c4)
	void EaseCursor();                           // 0x6a4674: the cursor drawn eases to the cursor
	// 0x6bd338: the bombs' and devil gems' counters show their warnings; true when the game ends on one at zero
	void UpdateGlows();                           // 0x6b094e
	void UpdateCellPoints();                      // 0x6a5809: the cells' wobble
	void UpdateLevelUp();                         // 0x6c6905
	int GetGridRight();                          // 0x6a49a3
	float GetGridMidY();                         // 0x6a49cf
	void UpdateUnknown248();                     // 0x6c4466
	void UpdateHint();                           // 0x6aff84
	void ShowHint();                             // 0x6c32cf
	void UpdateToolTips();                       // part of 0x6d090f
	void ShowStatsBar();                         // 0x6ab48d
	void SetUnknown3924(float theValue);         // 0x6a6a07: the board's brightness; rising from 0 it shows the gems
	void ApplyUnknown3924();                     // 0x6a69a8: every gem's alpha to it
	void RetargetTravellers();                   // 0x6a9f1d
	void FinishLevelUp();                         // 0x6bab49: the change between levels is over
	void SettleTurn();                            // 0x6c2f69: the board at rest: the next move and the turn's end
	void QueueTip(int theKind, Piece* thePiece);  // 0x6b98f8: a tip waits for its dialog (+0x256c)
	bool CheckTips();                             // 0x6bc129: tips for the special gems on the board
	void ShowNextTip();                           // 0x6bbc58
	// 0x6c986a: the tube beside the board shown (the grid slides over to make room) or hidden (the grid in the middle)
	void ShowTube(bool theShow, bool theInstant);
	void CloseTip();                              // 0x6c78a5: the tip's dialog is done (marked seen; the basics end)
	bool DropFlaggedGems();                      // 0x6bb7e3: the lowest FLAG_1 gem of each column sinks a row
	void EndTurn();                               // 0x6be49c
	void UpdateRecords();                         // 0x6bcd96: the game's best move, cascade and score
	bool IsGameOverPending();                     // 0x6a5626
	void CheckCombos();                           // 0x6bbbc2: the turn's praise
	int GetTurnRating();                          // 0x6a5179
	void ShowPraise(int theLevel);                // 0x6ba73a: 0 to 5, from "Good" up
	void PlayGameMusic();                         // 0x6a6a8b
	void LevelComplete();                         // 0x6b3634
	void ShowLevelUp();                           // 0x6a66d5: the LevelUpShow, with its voice
	void UpdateJump();                            // UpdateGame's part (0x6d090f) while +0x2108 runs: the level's end
	void ClearShockwaves();                       // 0x6b02ea
	void EndJump(bool theUnused);                 // 0x6baaa7: the board after the jump between levels
	bool UpdateUnknown1660();                     // 0x6af497
	bool UpdateUnknown2170();                     // 0x6af0e2
	void CheckSettled();                          // 0x6c7659
	bool SettlePieces();                          // 0x6c812f: true once every gem is at rest
	void UpdateStrikes();                         // 0x6c87d2: the electro gems' strikes (+0x149c)
	void FallPieces();                            // 0x6cf84a
	static void UpdateCell(Cell& theCell);        // 0x6a3904
	bool DropPieces();                            // 0x6a5512: true if a column has a gap
	int FillInBlanks(bool theAtOnce);             // 0x6c42e3: returns how many gems are new
	bool CanMove(Piece* thePiece);                // 0x6a50e8
	void LayoutTube();                            // 0x6b3da8
	bool CountDownCounters();
	void NextLevelMusic();                       // 0x6a496b: the next of the game's tunes
	void AddedToManager(WidgetManager* theWidgetManager) override;  // 0x6b8aa2
	// 0x6ca084: saves the game (back at the level's start if a move is under way) unless it is over
	void RemovedFromManager(WidgetManager* theWidgetManager) override;
	void DrawAll(ModalFlags* theFlags, Graphics* g) override;  // 0x6aac5e
	void Draw(Graphics* g) override;                 // 0x6cc7ed
	void DrawBoardBack(Graphics* g);                 // 0x6cc5a3: the 2D board and the space behind it
	void DrawStrikes(Graphics* g);                   // 0x6aafc1
	void DrawOverlay(Graphics* g) override;          // 0x6b4a39
	void DrawCounters(Graphics* g);                  // 0x6b42e1
	// 0x6caa13 (0x6cc547 skips it while +0x1ff5): theToImage leaves out the grid, theSkipPieces all but the grid
	void DrawBoard(Graphics* g, bool theToImage, bool theSkipPieces);
	void DrawGrid(Graphics* g, float theArtScale);  // 0x6caa13's grid
	void DrawPieces(Graphics* g, Graphics& theLightG, bool is3D);  // 0x6caa13's gems
	// 0x6aa3bc: in a tip, the forced move's "click here" and the arrows around the cursor
	void DrawTutorialCursor(Graphics* g);
	void DrawSelector(Graphics* g, bool theRing);  // 0x6aa6d1: the twist cursor, its ring or its spiral
	void AddLight(float theX, float theY, float theSize, float theFalloff, float theStrength, Piece* theSkip,
		bool theSkipFlag4);                         // 0x6a5c60
	// 0x6a6ad0: blitz, or a challenge with the timed flag
	bool IsTimed() { return mGameMode == MODE_BLITZ || HasChallengeFlag(2); }
	void Update() override;                      // 0x6d6899: the game's updates for this frame, at its speed
	void UpdateGame();                           // 0x6d090f: one update of the game
	void Resize(int theX, int theY, int theWidth, int theHeight) override;  // 0x6a5b53: the pieces move with the grid
	using Widget::Resize;
	int GetBoardY() { return (mHeight - mRows * gGemHeight) / 2; }    // 0x6a4801
	int GetRowY(int theRow) { return GetBoardY() + gGemHeight * theRow; }  // 0x6a4831
	static int GetBoardWidth();                  // 0x6a47ee: the grid's width (gBoard's)

	Piece* GetPieceAt(int theRow, int theCol);   // 0x6a40c7
	Piece* GetPieceById(int theId);              // 0x6aff38
	bool AnyCellUnknown34();                     // 0x6a478b
	Piece* FindHintPiece();                      // 0x6a42b4
	void AddPieceId(int theId, Piece* thePiece); // 0x6b8c20
	void RemovePieceId(int theId);               // 0x6b677b
	bool IsSparkBlocked() { return mSparkBlock > 0; }  // 0x6a62d0
	// 0x6a4065: not a challenge, nor at the game's end (+0x160 6 and 7), nor with +0x1390
	bool CanSaveOnQuit() { return !mUnknown1390 && mUnknown160 != 6 && mUnknown160 != 7 && mGameMode != MODE_CHALLENGE; }
	// 0x6a32ca
	bool IsBusy() { return 0.0f < mUnknown2108 || 0 < mUnknown20DC || 0.0f < mUnknown380C; }
	// port: the piece the dialog at +0x2584 points at, once it is ported
	int GetPointedPieceId() { return mUnknown2584 != nullptr ? mUnknown2584->mPointedPieceId : -1; }
	Piece* GetPieceAtXY(int theX, int theY);     // 0x6a4121
	static int GetLevelSteps(int theLevel);      // 0x70205f: the chain steps a multiplier level takes
	int GetLevelFor(int* theLeft);               // 0x6a4383: the level +0x224's steps reach, and the steps over
	void SetLevelSteps(int theSteps);            // 0x6a43e8: the chain's steps (+0x224), its level and the tube's
	void AddChainStep();                         // 0x6a62e1
	void ShowTubeLevel();                        // 0x6a3e81
	void BreakPiece(Piece* thePiece);            // 0x6c0ccf: a gem breaks up, in a match or blown up
	void BlowUpPiece(Piece* thePiece, float theX, float theY, bool theBreak, bool theBlast);  // 0x6c1857
	// 0x6a4a1b: the gems above thePiece jump, harder the nearer they are
	void LiftColumn(Piece* thePiece, float theStrength);
	void SetOffPowerGem(Piece* thePiece);         // 0x6c6d68
	void BurstAt(int theX, int theY, int theRange);    // 0x6baed1
	// 0x6b2c50: the game is over; theNuke when a counter reached zero (the board blows up), else GAME OVER shows
	void EndGame(bool theNuke);
	// 0x6af1d0: one more pause (+0x1570); the first stops the game's clock (+0x14e8), the tube's mouse and the danger
	// music. theShow counts it in +0x1574 too, the pauses the player sees
	void Pause(bool theShow);
	// 0x6bd0d9: thePiece's counter reached zero: the second chance's wheel (WheelSpin) waits for its turn
	void SpinWheel(Piece* thePiece);
	// 0x6af2be: one pause less; the last restarts the clock and the music
	void Unpause(bool theShow);
	void GetCenter(int* theX, int* theY);  // 0x6a48c9: the grid's middle (the board's, while +0x2041 is off)
	void SetEndState(int theState);        // 0x6b2708
	// 0x6d045b: the board after the game, state by state (+0x26c8): the rows fall off (3), blitz's final score
	// (4 to 8), then back to the menu (9)
	void UpdateGameOver();
	// 0x6d090f's state 7 with the nuke: the blast from +0x26cc blows the gems apart, the tube steams, then space
	void UpdateNuke();
	// 0x6a3b4c: the game's time, scores and counts into the profile and the app, once (+0x137c)
	void UpdateStats();
	void DrawNukeGem(Graphics* g, Piece* thePiece);  // 0x6a6885: the gem that set off the nuke, while hidden
	void BreakSpecial(Piece* thePiece, bool theFlag);  // 0x6c03a4
	void BurstGems(int theX, int theY, int theRange);  // 0x6bac72
	// 0x6a4b7c: the gems above and around thePiece jump away from it
	void LiftAround(Piece* thePiece, float theStrength);
	// 0x6a6962: one more gem set off in a chain of blasts, each worth twice the last (to 100)
	void AddBlastChain() { mUnknown1C5C++; mUnknown1C58 = mUnknown1C58 * 2 <= 100 ? mUnknown1C58 * 2 : 100; }
	bool GetUnknown23F5() { return mUnknown23F5; }     // 0x6a639a
	int GetBoardX() { return mBoardX; }                // 0x6a47d9
	int GetColX(int theCol) { return GetBoardX() + gGemWidth * theCol; }  // 0x6a4819
	int GetPan(int theX);                              // 0x6a9ccd: -800 to 800 across the grid
	// 0x6ab14b: a sound panned to where thePiece is, unless the board is quiet (+0x37f8)
	void PlayPieceSound(Piece* thePiece, int theSoundId, float thePitch, float theVolume, bool theTrack);
};

// DAT_0085aef4: the board while there is one
extern Board* gBoard;
extern int gSettleCount;  // 0x84e88c: how often the board has come to rest
extern MTRand gUnknown85B970;  // 0x85b970: a generator of its own, seeded from the time by each new board
int Rand85B970();  // 0x6a3447
// 0x85af08: each colour's tint, copied by the board's constructor from the table at 0x85c378 (and Color(0, 0, 0),
// then two whites, after it)
extern Color gGemColors[10];

}
