// Sexy::Board (about 0x3940 bytes, vtable 0x7dd0d4 with 71 slots; 0x6a3d7b-0x6d6899): the game itself. Being
// ported (docs/NOTES.md, "The board"); its fields are named by their offset in the original's object until their use
// is known, and come in as the code that uses them does.
#pragma once

#include "TwistWidget.h"
#include "WinTwistApp.h"

#include "CurvedVal.h"
#include "MTRand.h"
#include "Rect.h"

#include <map>
#include <vector>

namespace Sexy {

class EffectsManager;
class GoodTube;
class Font;
class Image;
class MemoryImage;
class Piece;
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
};

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
	// +0x149c: objects of a class at 0x71eb9e (not yet ported); ClearBoard deletes them
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
	float mUnknown2038 = 0;            // +0x2038
	int mUnknown20DC = 0;              // +0x20dc
	float mUnknown2108 = 0;            // +0x2108
	int mSparkBlock = 0;               // +0x21ac: no sparks while above 0
	float mUnknown21B0 = 0;         // +0x21b0
	bool mDebugPieces = false;         // +0x21db: each piece shows its row and column
	float mUnknown23EC = 0;            // +0x23ec
	bool mUnknown23F5 = false;         // +0x23f5
	// +0x2584: a dialog (0x220 bytes, made by 0x75122f) that can point at a piece (its +0x1a4); it comes with the board
	Widget* mUnknown2584 = nullptr;
	bool mUnknown25A4 = false;         // +0x25a4
	float mUnknown268C = 1.0f;         // +0x268c
	float mShakeX = 0;                 // +0x2690
	float mShakeY = 0;                 // +0x2694
	bool mUnknown37C4 = false;         // +0x37c4
	bool mUnknown37F8 = false;         // +0x37f8: no sounds or marks for the gems made (loading a game)
	bool mUnknown3904 = false;         // +0x3904
	float mUnknown380C = 0;            // +0x380c
	float mUnknown3924 = 1.0f;         // +0x3924
	int mBoardX = 0;                   // +0x3474: the grid's left edge
	// The rest of the fields the constructor (0x6c4df9) sets, named by their offset until their use is known
	int mUnknown15C = 0;                    // +0x15c
	int mUnknown160 = 0;                    // +0x160
	int mUnknown168 = 0;                    // +0x168
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
	int mUnknown224 = 0;                    // +0x224
	int mUnknown248 = 0;                    // +0x248
	int mUnknown250 = -1;                   // +0x250
	int mUnknown254 = -1;                   // +0x254
	int mUnknown258 = -1;                   // +0x258
	int mUnknown420 = 0;                    // +0x420
	bool mUnknown6CC = false;               // +0x6cc
	int mUnknown6D0 = 0;                    // +0x6d0
	int mUnknown70C = 0;                    // +0x70c
	int mUnknown710 = 0;                    // +0x710
	int mUnknown814 = 0;                    // +0x814
	int mUnknown818 = 0;                    // +0x818
	int mUnknown82C = 0;                    // +0x82c
	int mUnknown8BC = 0;                    // +0x8bc
	int mUnknown131C = -1;                  // +0x131c
	int mUnknown132C = 1;                   // +0x132c
	// +0x135c: a value per cell (AllocGrid)
	int* mUnknown135C = nullptr;
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
		int mUnknown30 = 0;        // not set by the constructor
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
	int mUnknown139C = 0;                   // +0x139c
	int mUnknown13A0 = 0;                   // +0x13a0
	int mUnknown13A4 = 0;                   // +0x13a4
	bool mUnknown13A8 = true;               // +0x13a8
	bool mUnknown1420 = false;              // +0x1420
	int mUnknown1424 = 0;                   // +0x1424
	Piece* mUnknown1430 = nullptr;          // +0x1430: a piece RemovePiece forgets
	int mUnknown1434 = 0;                   // +0x1434
	bool mUnknown1438 = false;              // +0x1438
	int mUnknown143C = 0;                   // +0x143c
	int mUnknown1440 = 1;                   // +0x1440
	int mUnknown1444 = -1;                  // +0x1444
	int mUnknown1448 = -1;                  // +0x1448
	int mUnknown144C = -1;                  // +0x144c
	float mUnknown1450 = 1.0f;              // +0x1450
	int mUnknown1454 = 0;                   // +0x1454
	float mUnknown145C = 1.0f;              // +0x145c
	int mUnknown1460 = 0;                   // +0x1460
	int mUnknown146C = 0;                   // +0x146c
	int mUnknown1470 = 0;                   // +0x1470
	int mUnknown1474 = 0;                   // +0x1474
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
	int mUnknown1518 = 0;                   // +0x1518
	int mUnknown151C = 0;                   // +0x151c
	int mUnknown1520 = 0;                   // +0x1520
	int mUnknown1524 = 0;                   // +0x1524
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
	int mUnknown15C4 = 0;                   // +0x15c4
	bool mUnknown15D0 = false;              // +0x15d0
	int mUnknown15D4 = 0;                   // +0x15d4
	bool mUnknown15D8 = false;              // +0x15d8
	int mUnknown15DC = 0;                   // +0x15dc
	bool mUnknown15EC = false;              // +0x15ec
	int mUnknown15F4 = 0;                   // +0x15f4
	bool mUnknown15FC = true;               // +0x15fc
	bool mUnknown15FD = false;              // +0x15fd
	int mUnknown1604 = 0;                   // +0x1604
	int mUnknown1610 = 0;                   // +0x1610
	double mUnknown1618 = 0;                // +0x1618
	double mUnknown1620 = 0;                // +0x1620
	double mUnknown1628 = 0;                // +0x1628
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
	int mUnknown1C48 = 0;                   // +0x1c48
	int mUnknown1C4C = 0;                   // +0x1c4c
	int mUnknown1C50 = 0;                   // +0x1c50
	int mUnknown1C64 = -1;                  // +0x1c64
	int mUnknown1C74 = 0;                   // +0x1c74
	int mUnknown1C84 = 0;                   // +0x1c84
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
	int mUnknown1FAC = 0;                   // +0x1fac
	int mUnknown1FB0 = 0;                   // +0x1fb0
	int mUnknown1FB8 = 0;                   // +0x1fb8
	int mUnknown1FC4 = 0;                   // +0x1fc4
	int mUnknown1FC8 = 0;                   // +0x1fc8
	bool mUnknown1FF4 = false;              // +0x1ff4
	bool mUnknown1FF5 = false;              // +0x1ff5
	int mUnknown1FF8 = 0;                   // +0x1ff8
	int mUnknown1FFC = 0;                   // +0x1ffc
	int mUnknown2000 = 0;                   // +0x2000
	bool mUnknown2004 = false;              // +0x2004
	int mUnknown2008 = 0;                   // +0x2008
	int mUnknown200C = 0;                   // +0x200c
	int mUnknown2010 = 0;                   // +0x2010
	bool mUnknown2014 = false;              // +0x2014
	bool mUnknown2015 = false;              // +0x2015
	bool mUnknown2016 = false;              // +0x2016
	int mUnknown201C = -1;                  // +0x201c
	int mUnknown2020 = -1;                  // +0x2020
	bool mUnknown2030 = false;              // +0x2030
	int mUnknown2034 = 0;                   // +0x2034
	bool mUnknown2040 = true;               // +0x2040
	bool mUnknown2041 = true;               // +0x2041
	bool mUnknown2042 = true;               // +0x2042
	bool mUnknown2043 = true;               // +0x2043
	bool mUnknown2054 = false;              // +0x2054
	int mUnknown2058[24] = {};              // +0x2058
	int mUnknown20B8 = 0;                   // +0x20b8
	int mUnknown20BC = -1;                  // +0x20bc
	int mUnknown20C0 = 0;                   // +0x20c0
	int mUnknown20C4 = 0;                   // +0x20c4
	int mUnknown20C8 = 0;                   // +0x20c8
	int mUnknown20CC = 0;                   // +0x20cc
	bool mUnknown20D0 = true;               // +0x20d0
	int mUnknown20D4 = 0;                   // +0x20d4
	bool mUnknown20D8 = false;              // +0x20d8
	int mUnknown20E4 = 0;                   // +0x20e4
	float mUnknown20E8 = 0.1f;              // +0x20e8
	int mUnknown20EC = 0;                   // +0x20ec
	bool mUnknown210C = false;              // +0x210c
	int mUnknown2110 = 0;                   // +0x2110
	int mUnknown2114 = 0;                   // +0x2114
	float mUnknown2118 = 3.0f;              // +0x2118
	bool mUnknown2130 = false;              // +0x2130
	int mUnknown2150 = 0;                   // +0x2150
	int mUnknown2154 = 0;                   // +0x2154
	int mUnknown2158 = 0;                   // +0x2158
	int mUnknown215C = -1;                  // +0x215c
	int mUnknown2160 = 1;                   // +0x2160
	int mUnknown2164 = 0;                   // +0x2164
	int mUnknown2168 = 0;                   // +0x2168
	bool mUnknown216C = false;              // +0x216c
	int mUnknown2188 = 0;                   // +0x2188
	bool mUnknown2199 = false;              // +0x2199
	int mUnknown219C = 0;                   // +0x219c
	bool mUnknown21A0 = false;              // +0x21a0
	bool mUnknown21A2 = false;              // +0x21a2
	bool mUnknown21A3 = false;              // +0x21a3
	int mUnknown21A8 = 0;                   // +0x21a8
	int mUnknown21C4[2] = {};               // +0x21c4
	int mUnknown21CC = 0;                   // +0x21cc
	int mUnknown21D0 = 1;                   // +0x21d0
	int mModeMultiplier = 1;                // +0x21d4
	bool mUnknown21D8 = false;              // +0x21d8
	bool mUnknown21D9 = false;              // +0x21d9
	bool mUnknown21DA = false;              // +0x21da
	int mUnknown21DC = 0;                   // +0x21dc
	int mUnknown21F4 = -1;                  // +0x21f4
	bool mUnknown2200 = false;              // +0x2200
	int mUnknown2208 = 0;                   // +0x2208
	int mUnknown2228 = 0;                   // +0x2228
	int mUnknown222C = 0;                   // +0x222c
	int mUnknown2234[84] = {};              // +0x2234
	int mUnknown23A4 = -1;                  // +0x23a4
	int mUnknown23A8 = -1;                  // +0x23a8
	int mUnknown23AC = 0;                   // +0x23ac
	int mUnknown23B0 = 0;                   // +0x23b0
	int mUnknown23D4 = 0;                   // +0x23d4
	float mUnknown23F0 = 0.01f;             // +0x23f0
	bool mUnknown23F4 = false;              // +0x23f4
	int mUnknown23F8 = 0;                   // +0x23f8
	int mUnknown23FC = 0;                   // +0x23fc
	int mUnknown2400 = -10000;              // +0x2400
	int mUnknown2404 = -10000;              // +0x2404
	int mUnknown2548 = -1;                  // +0x2548
	int mUnknown2578 = 0;                   // +0x2578
	int mUnknown257C = 0;                   // +0x257c
	int mUnknown2580 = 1;                   // +0x2580
	bool mUnknown25A5 = false;              // +0x25a5
	bool mUnknown25A6 = false;              // +0x25a6
	int mUnknown25A8 = 0;                   // +0x25a8
	int mUnknown25C8 = 0;                   // +0x25c8
	int mUnknown25CC = 0;                   // +0x25cc
	int mUnknown25D0 = 0;                   // +0x25d0
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
	int mUnknown267C = 0;                   // +0x267c
	int mUnknown2680 = 0;                   // +0x2680
	bool mUnknown2698 = true;               // +0x2698
	int mUnknown269C = 0;                   // +0x269c
	int mUnknown26A0 = 0;                   // +0x26a0
	int mUnknown26A8 = 0;                   // +0x26a8
	int mUnknown26AC = 0;                   // +0x26ac
	int mUnknown26CC = 0;                   // +0x26cc
	int mUnknown26D0 = 0;                   // +0x26d0
	int mUnknown3444 = 0;                   // +0x3444
	int mUnknown3448 = 0;                   // +0x3448
	int mUnknown344C = 0;                   // +0x344c
	int mUnknown3450 = 0;                   // +0x3450
	int mUnknown3454 = 0;                   // +0x3454
	int mUnknown345C = 0;                   // +0x345c
	float mUnknown3460 = 8.0f;              // +0x3460
	int mUnknown3464[2] = {};               // +0x3464
	int mUnknown3470 = 0;                   // +0x3470
	int mUnknown34E8 = 0;                   // +0x34e8
	int mUnknown34EC = 0;                   // +0x34ec
	int mUnknown3560[84] = {};              // +0x3560
	bool mUnknown36B0 = false;              // +0x36b0
	bool mUnknown36B1 = true;               // +0x36b1
	int mUnknown36B4 = 0;                   // +0x36b4
	int mUnknown36B8 = 0;                   // +0x36b8
	int mUnknown37A0 = 0;                   // +0x37a0
	int mUnknown37A4 = 0;                   // +0x37a4
	int mUnknown37A8 = 0;                   // +0x37a8
	int mUnknown37AC = 0;                   // +0x37ac
	bool mUnknown37B0 = true; // +0x37b0
	bool mUnknown37B1 = true; // +0x37b1
	int mUnknown37B4 = 15;                  // +0x37b4
	int mUnknown37C8 = 0;                   // +0x37c8
	int mUnknown37EC = 0;                   // +0x37ec
	int mUnknown37F0 = 0;                   // +0x37f0
	int mUnknown37F4 = 0;                   // +0x37f4
	int mUnknown37FC = 0;                   // +0x37fc
	int mUnknown3808 = 0;                   // +0x3808
	int mUnknown38F0 = 0;                   // +0x38f0
	int mUnknown38F4 = 0;                   // +0x38f4
	int mUnknown38F8 = 0;                   // +0x38f8
	int mUnknown38FC = 0;                   // +0x38fc
	int mUnknown3900 = 0;                   // +0x3900
	bool mUnknown3905 = false;              // +0x3905
	int mUnknown3908 = 0;                   // +0x3908
	int mUnknown390C = 0;                   // +0x390c
	int mUnknown3910 = 0;                   // +0x3910
	int mUnknown3914 = 0;                   // +0x3914
	int mUnknown3918 = 0;                   // +0x3918
	int mUnknown391C = 0;                   // +0x391c
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
	int mUnknown133C[8] = {};               // +0x133c
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
	Rect mUnknown1F88;                      // +0x1f88
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
	Widget* mPointsManager = nullptr;       // +0x21fc: the points that float up (0x158 bytes, 0x7329b7)
	float mUnknown2224 = 0;                 // +0x2224
	float mUnknown23D8 = 0;                 // +0x23d8
	Image* mGridBarTop = nullptr;           // +0x2588
	Image* mGridBarBot = nullptr;           // +0x258c
	int mUnknown25AC = 0;                   // +0x25ac
	int mUnknown25B4 = 0;                   // +0x25b4
	int mUnknown2644 = 0;                   // +0x2644
	int mBonusChallengeMinGems = 0;         // +0x266c
	int mUnknown2674 = 0;                   // +0x2674: the challenge's number (mode 2)
	int mUnknown3458 = 0;                   // +0x3458: blitz's 30000
	std::map<int, int> mUnknown37B8;        // +0x37b8
	MTRand mRand;                           // +0x958: seeded from Rand()
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
	int mUnknown228[8] = {};                // +0x228..+0x244
	int mUnknown1C44 = 0;                   // +0x1c44
	bool mUnknown1F6C = false;              // +0x1f6c
	int mUnknown1F70 = 0;                   // +0x1f70: the level modulo 7
	int mUnknown20F0 = 0;                   // +0x20f0
	float mUnknown2134 = 1.0f;              // +0x2134
	bool mUnknown21A1 = false;              // +0x21a1: past the first level
	int mUnknown21F8 = 0;                   // +0x21f8: 100 at a level's start (0x6a3ff4)
	bool mUnknown2201 = false;              // +0x2201
	int mUnknown2590 = 0;                   // +0x2590: the grid's width and a quarter of a cell (0x6a49a3)
	float mUnknown2594 = 1.0f;              // +0x2594
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

	Piece* GetPieceAt(int theRow, int theCol);   // 0x6a40c7
	Piece* GetPieceById(int theId);              // 0x6aff38
	void AddPieceId(int theId, Piece* thePiece); // 0x6b8c20
	void RemovePieceId(int theId);               // 0x6b677b
	bool IsSparkBlocked() { return mSparkBlock > 0; }  // 0x6a62d0
	// 0x6a32ca
	bool IsBusy() { return 0.0f < mUnknown2108 || 0 < mUnknown20DC || 0.0f < mUnknown380C; }
	// port: the piece the dialog at +0x2584 points at, once it is ported
	int GetPointedPieceId() { return -1; }
	Piece* GetPieceAtXY(int theX, int theY);     // 0x6a4121
	void PieceArrived(Piece* thePiece);          // 0x6c0ccf: a piece that flew to its place (Piece::Update)
	bool GetUnknown23F5() { return mUnknown23F5; }     // 0x6a639a
	int GetBoardX() { return mBoardX; }                // 0x6a47d9
	int GetColX(int theCol) { return GetBoardX() + gGemWidth * theCol; }  // 0x6a4819
	int GetPan(int theX);                              // 0x6a9ccd: -800 to 800 across the grid
	// 0x6ab14b: a sound panned to where thePiece is, unless the board is quiet (+0x37f8)
	void PlayPieceSound(Piece* thePiece, int theSoundId, float thePitch, float theVolume, bool theTrack);
};

// DAT_0085aef4: the board while there is one
extern Board* gBoard;
// 0x85af08: each colour's tint, copied by the board's constructor from the table at 0x85c378 (and Color(0, 0, 0),
// then two whites, after it)
extern Color gGemColors[7];

}
