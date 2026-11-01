// Sexy::Board (about 0x3940 bytes, vtable 0x7dd0d4 with 71 slots; 0x6a3d7b-0x6d6899): the game itself. Being
// ported (docs/NOTES.md, "The board"); its fields are named by their offset in the original's object until their use
// is known, and come in as the code that uses them does.
#pragma once

#include "TwistWidget.h"
#include "WinTwistApp.h"

#include <map>
#include <vector>

namespace Sexy {

class EffectsManager;
class Image;
class Piece;
class WinTwistApp;

// A number that grows with the level (0x74e9e8): from mFirstLevel on, mBase, plus mStep every mPeriod levels from
// mRampLevel and every mPerGames of the app's +0xcf8, held to mMin and mMax (each unless below 0)
struct LevelValue {
	int mUnknown0 = 0;
	int mFirstLevel = 0;   // +0x4
	int mRampLevel = 0;    // +0x8
	int mUnknownC = 0;
	int mBase = 0;         // +0x10
	int mPeriod = 1;       // +0x14
	int mUnknown18 = 0;
	int mStep = 0;         // +0x1c
	int mUnknown20 = 0;
	int mUnknown24 = 0;
	int mMin = -1;         // +0x28
	int mMax = -1;         // +0x2c
	int mPerGames = 0;     // +0x30
	int GetValue();
};

class Board : public TwistWidget {
public:
	// The game modes (+0x20c)
	enum { MODE_CLASSIC = 1, MODE_ZEN = 2, MODE_BLITZ = 3, MODE_CHALLENGE = 5 };

	int mUnknown158 = 0;               // +0x158: the level (LevelValue)
	int mGameMode = 0;                 // +0x20c
	int mModeFlags = 0;                // +0x210: 8 for a replay
	WinTwistApp* mApp = nullptr;       // +0x954
	Image* mPuzzleImage = nullptr;     // +0x1320: a challenge's picture, cut into the gems
	bool mUnknown1324 = false;         // +0x1324
	int mRows = 0;                     // +0x1330
	int mCols = 0;                     // +0x1334
	Piece** mGrid = nullptr;           // +0x1338: mRows x mCols, row by row
	std::vector<Piece*> mUnknown149C;  // +0x149c
	std::vector<Piece*> mBombList;     // +0x14bc: bombs about to go off, drawn over the board (Piece::DrawFront)
	std::map<int, Piece*> mPieceMap;   // +0x1364: the pieces by id (0x6b8c20 adds, 0x6b677b drops)
	float mShineAmount = 0;            // +0x163c
	float mPieceAlpha = 1.0f;          // +0x1640
	EffectsManager* mFXManager = nullptr;  // +0x1648
	EffectsManager* mFXManager2 = nullptr; // +0x164c
	int mUnknown15B4 = 0;              // +0x15b4: the mouse, over the board
	int mUnknown15B8 = 0;              // +0x15b8
	LevelValue mUnknown1EB0;           // +0x1eb0: a doom gem's count
	int mUnknown1F78 = 0;              // +0x1f78
	bool mUnknown1BF2 = false;         // +0x1bf2: coal is drawn as snackers
	Image* mShineImage = nullptr;      // +0x1c54
	int mUnknown1C5C = 0;              // +0x1c5c
	float mUnknown2038 = 0;            // +0x2038
	int mUnknown20DC = 0;              // +0x20dc
	float mUnknown2108 = 0;            // +0x2108
	int mSparkBlock = 0;               // +0x21ac: no sparks while above 0
	float mUnknown21B0 = 1.0f;         // +0x21b0
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

	Board();
	~Board() override;

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
