// Sexy::Board (vtable 0x864cb4 (bejeweled3), made by 0x74ae30) and Sexy::ClassicBoard (RTTI vftable 0x86511c, 0x3088
// bytes, made by 0x73b390): the game screen of Classic mode, and Sexy::Piece (0x630 bytes, made by 0x7415c0): one gem.
// The board is an 8 by 8 grid of pieces; the exe measures it in 1200ths of the art height (a cell is 128), draws it at
// (0x20b, 0x4b) of that and scales every measure by the app's art size over 1200.
// port: only what Classic's start, first fill and the fall of a new board read is translated (docs/NOTES.md, "The board"):
// the swap, the matches, the rest of the update (0x759170) and the other modes' boards come with their steps. The exe's board has about 200 virtual
// functions that the modes override; Classic's overrides are constants (0x73b5a0 and 0x73bc00 to 0x73bc30), which this
// port reads in place (each use says which slot it stands for).
#pragma once

#include "ButtonListener.h"
#include "CurvedVal.h"
#include "MTRand.h"
#include "Widget.h"

#include <memory>
#include <vector>

namespace Sexy {

class Board;

// One gem. Fields are named by what the code that reads them does with them; the ones that nothing translated reads
// are left out (the fill also sets +0x182 to 1 and +0x244 to 0 on each piece it makes: nothing translated reads them).
class Piece {
public:
	enum { NO_COLOR = -1, COLOR_COUNT = 7 };
	// +0x228: what the gem is (the bits the fill reads; the specials come with their steps)
	enum { FLAG_RANDOM_PICK = 0x60 };

	Board* mBoard;          // +0
	int mId;                // +4: the board's piece counter (+0xf4) when it was made
	int mCol;               // +8
	int mRow;               // +0xc
	float mX;               // +0x10: where it is drawn, in 1200ths of the art height from the board's corner (a cell is 128)
	float mY;               // +0x14: new pieces start above the board and fall to row * 128
	int mCreatedUpdate;     // +0x18: the board's update count when it was made
	float mFallSpeed;       // +0x1c: how far it moves down each update, in 1200ths: 0 at rest and until it first falls; the fill
	                        // gives a new piece a negative start (it rises a little first, which staggers a column), gravity adds
	int mColumnTag;         // +0x188: what the column's tag (board +0x218) held when it was made
	bool mCanFall;          // +0x17e: set by every pass of the fill over it
	bool mCanSwap;          // +0x17f: 1 at the start
	int mSpecialState;      // +0x20c: -1 keeps it from falling
	int mMovedUpdate;       // +0x218: the board's update count when it was made and when it last moved (set while its speed is not 0)
	int mColor;             // +0x220: -1 for none, else 0 to 6 (red, white, green, yellow, purple, orange, blue)
	int mFlags;             // +0x228: 0 for a plain gem
	int mBusyCount;         // +0x230: above 0 while something is done to it: the fill waits for it; a piece whose count
	                        // reaches 0 from 1 is destroyed (UpdateGame)
	bool mSelected = false; // +0x23c: the player has picked it up (a click on it, waiting for the next gem)
	bool mIsGroupOrigin = false;   // +0x174: a match group was made for it (0x789a90)
	float mMatchTimer = 0.0f;   // +0x178: how far its clear has come. A match group (0x789a90) puts its first piece at 0.9 and a piece
	                        // that its ring reaches at 0.01; 0x75dee0 adds 0.015 each update and lets the piece go at 1. port: a
	                        // plain gem's match makes no group in the exe, so the stand-in for its clear (PlainClear) uses it too
	bool mClearHeld = false;    // +0x17c: set while another clear holds the piece (a flame gem found by 0x74c570); a match group
	                        // skips it and its ring does not reach it
	bool mClearable = true;     // +0x182: 1 from the constructor (0x720120): a match group's timer only runs for such pieces
	float mRingMark = 0.0f;     // +0x264: the highest timer (up to 1) a ring of a match group has seen on the piece (drawn only)
	int mDestroyerId = -1;      // +0x234: the id of the piece that cleared it (0x75dee0, 0x74c570); nothing translated reads it
	int mDestroyerFlags = 0;    // +0x238: the flags of what cleared it; nothing translated reads it
	int mSwapUpdate = -1;   // +0x214: the update it was last swapped in
	int mMatchedUpdate = -1;   // +0x210: the update it was matched in
	int mGroupId = -1;      // +0x190: the id of the run that matched it (the board's counter, +0xe1c)

	explicit Piece(Board* theBoard);   // 0x7203b0 (bejeweled3), with 0x720120 (bejeweled3)
};

// The exe's 0xd8-byte record of one move (made by 0x744660, kept in the list at +0xc48): the id its pieces carry as their
// tag (+0x188) and the counters of what the move has made. port: only counter 0x1a is kept, the number of matches the
// move and its cascades have made (0x75af60 adds one for each match and reads it for the score)
struct MoveRecord {
	int mId;            // +0x3c
	int mMatchCount;    // +0x40 + 0x1a * 4
};

// One swap of two neighbours (the exe's 0x100-byte record in the list at +0xc30, made by 0x75ee20, run by 0x759ba0)
struct SwapRecord {
	Piece* mFirst;        // +0: the piece the player moved
	Piece* mSecond;       // +4
	int mDeltaCol;        // +8: where the first one goes, from its own column
	int mDeltaRow;        // +0xc
	CurvedVal mCurve;     // the swap's curve
	bool mIsSwapBack;     // the swap made no match and the pieces are going back
	bool mDone;           // +0xf8 (set while it is checked for matches: the pieces are no longer held)
};

// One cell of a match group's ring (the exe's 12-byte entry in the group's list at +0xa4, pushed by 0x75dee0)
struct RingCell {
	int mCol;         // +0
	int mRow;         // +4
	float mTimer;     // +8: 0.01 when pushed, 0.015 more each update; at 1 the entry goes
};

// A match group (the exe's effect of 0x2e8 bytes made by 0x789a90 for a gem with bit 4, a flame gem, or by 0x74c570, kept in
// the list at +0x125c): rings that go out from its first piece and hit the cells around it, and the pieces that they find.
// port: its child effects (the streaks of 0x789860, which draw the app's Rand() when they run), its curves and its sounds
// are not translated; the fields that only they and the hypercube's update (0x7870f0, type 7) read are left out
struct MatchGroup {
	int mPixelX;               // +4: where the piece is, in 1200ths from the grid's corner, at its middle
	int mPixelY;               // +8
	int mAge;                  // +0xc: how many updates 0x78a390 has run for it
	int mMaxDistance;          // +0x14: how far a ring goes: 3 for type 3, else 7
	int mFlags;                // +0x1c: the first piece's flags (+0x228), which the pieces it clears take over
	int mType;                 // +0x28: 2 or 3 (a match; 3, or 2 when the board's +0xe8 is set), 6 when the piece has bit 1
	float mLife;               // +0x30: 1, and 0.25 less for each ring after the last distance has been reached; at 0 the group ends
	int mOriginId;             // +0x34: the id of the first piece
	int mTag;                  // +0x38: the first piece's tag (+0x188), the lowest tag its pieces take
	int mRunId;                // +0x3c: the first piece's run id (+0x190)
	int mRing;                 // +0x48: the rings made, which is also the distance of the next one from the first piece
	float mRingTimer;          // +0x4c: counts down by 0.01 each update, a ring at 0 or below
	float mFade;               // +0x2e0: 1, 0.01 less each update down to 0 (drawn only)
	int mLifeCountdown;        // +0x2e4: updates left before it ends; 0 is off (nothing translated sets it)
	std::vector<int> mPieceIds;    // +0x8c: the first piece's id, then the ids of the pieces its rings found
	std::vector<RingCell> mRingCells;    // +0xa4
};

// The stand-in for the clear of a plain match. port: the exe makes no group for a plain gem (0x75af60 does so only for a gem
// with bit 4) and the code that clears it was not found (docs/NOTES.md, "Matches"); this keeps what the port had: a run's
// pieces take a timer from 0.9 and are let go when it passes 1 (0.015 a step, as a group's pieces)
struct PlainClear {
	int mOriginId;           // the id of the run's first piece
	int mChainId;            // the highest tag of the run's pieces
	int mUpdate;             // the update it was made in
	std::vector<Piece*> mPieces;
	std::vector<int> mPieceIds;
};

// A swap the player asked for (the exe's 0x14-byte entry in the list at +0xc60, run by UpdateTick when its update comes)
struct PendingSwap {
	int mUpdate;    // +0
	int mPieceId;   // +4
	int mRow;       // +8
	int mCol;       // +0xc
};

class Board : public Widget, public ButtonListener {
public:
	enum { COLS = 8, ROWS = 8, CELL = 128 };
	// 0x73bd80 and 0x73bd70 (bejeweled3): where the grid is, in 1200ths of the art height
	enum { GRID_X = 0x20b, GRID_Y = 0x4b };

	Piece* mGrid[ROWS][COLS];        // +0xf8: row after row; null where there is no gem
	int mPieceCounter = 0;           // +0xf4: the number of pieces made, each one's id
	float mColumnSpeed[COLS];        // +0x1f8: the speed the fill takes from a column's last piece (the grid's ninth row); each update
	                                 // it rises by 0.275 towards 0 (UpdateGame)
	int mColumnTag[COLS];            // +0x218: -1 after a fill (the grid's tenth row)
	MTRand mRand;                    // +0x258: the board's own random numbers, seeded from the app's Rand() when it is made
	int mLastRow = ROWS - 1;         // +0x22e4: the last row
	int mFillHeld = 0;               // +0x2ef8: while it is not 0 the fill does nothing
	int mBackgroundIndex = -1;       // +0x2864: which of the game's backgrounds, in the order of 0x8d2170
	int mLevelIndex = 0;             // +0xe04: the level, from 0 (0 after a reset; the score's multiplier and the hint read it)
	int mLevelNumber = 1;            // +0xe0c: 1 after a reset (0x74a880); the points a match is worth are multiplied by it
	std::vector<int> mColorList;     // +0x105c: the colours a new piece may get: each of the seven twice
	bool mNoSavedGame = true;        // port: the saved game of this mode (0x74e1f0) is not read: a new game starts
	int mScore = 0;                  // +0xd64: the score as shown, which follows mScorePoints (UpdateShownScore)
	int mScorePoints = 0;            // +0xd24: the score
	int mMatchCounter = 0;           // +0xe1c: the next id of a move or a match
	std::vector<MoveRecord> mMoveRecords;    // +0xc48
	std::vector<SwapRecord> mSwaps;          // +0xc30
	std::vector<std::unique_ptr<MatchGroup>> mMatchGroups;    // +0x125c (the exe's list holds pointers too)
	std::vector<PlainClear> mPlainClears;    // port: see PlainClear
	int mGroupTypeSwitch = 0;        // +0xe8: not 0 makes the groups of matches type 2 instead of 3 (nothing translated sets it)
	float mGroupGlow = 0.0f;         // +0x27b8: 0.02 less each update, 0.05 more while a group lives, up to 1 (drawn only)
	float mRingFlash = 0.0f;         // +0x1240: 1 when a group has made its last ring (drawn only)
	std::vector<PendingSwap> mPendingSwaps;  // +0xc60
	bool mMouseDown = false;         // +0x27d4: the button is held down since the last MouseDown
	int mMouseDownX = 0;             // +0x27d8: where it went down, in pixels
	int mMouseDownY = 0;             // +0x27dc
	Piece* mSelectedAtMouseUp = nullptr;   // +0x27e0: the piece that was picked up when the button last came up
	float mTickFraction = 0.0f;      // +0xf0: what is left of the update's steps (Update runs the whole steps, keeps the rest)
	int mGameTicks = 0;              // +0xe38: the game's own ticks, which stop while a dialog or a hold stops the game
	int mLandTicks = 0;              // +0xe40: how many more updates the fill thinks the new pieces need to land (counts down)
	int mGravityHold = 0;            // +0xe24: while above 0 the pieces stand still (counts down)
	int mLastLandSoundUpdate = 0;    // +0xc20: the update a piece's landing was last heard in
	bool mSettledOnce = false;       // +0x1190: set the first time the board stands settled (IsSettled)
	int mActivityCount = 0;          // +0xcc0: scales how often the idle shine starts (0 until matches are made; they come with their step)
	bool mShineOn = false;           // +0x27b0: the idle shine has started and the board has not been unsettled since
	int mShineUpdate = 0;            // +0x27b4: the update the idle shine last started in
	CurvedVal mShineCurve;           // +0x2740: the idle shine's curve, started by UpdateGame
	CurvedVal mFallFactorCurve;      // +0xe58: 1 after a reset; the fall speed's factor is made of it (GetFallFactor)
	CurvedVal mCoverCurve;           // +0x2898: 1 from the constructor (0x74b76c): while it is 1 the main menu behind is hidden (0x6a3530)

	Board();                                   // 0x74ae30 (bejeweled3)
	~Board() override;
	void ButtonDepress(int theId) override {}  // 0x741130 and the others' buttons come with their steps

	// slot 85, 0x74a880 (bejeweled3): the board is emptied for a new game
	void ResetGame();
	// slot 117, 0x756390 (bejeweled3): how strongly gravity pulls: 1 at the curve's 1, 0.65 less for each step of it below
	float GetFallFactor() { return (float)((mFallFactorCurve.GetOutVal() - 1.0) * 0.64999997615814209 + 1.0); }
	// slot 22, 0x759170 (bejeweled3): the board's own update: the game speed gives the number of steps this update is worth
	// (one at the normal speed) and each runs UpdateTick
	void Update() override;
	// slot 234, 0x767090 (bejeweled3): one step of the game
	void UpdateTick();
	// slot 231, 0x767e20 (bejeweled3): the game's part of a step: the fill, the fall and the idle shine
	void UpdateGame();
	// slot 214, 0x73dd70 (bejeweled3): gravity: each column's pieces move by their speeds and land on the row or the piece below
	void UpdateFalling();
	// 0x75fca0 (bejeweled3): every piece is at rest in its cell, nothing is left to land, and there is no match
	bool IsSettled();
	// 0x755ad0 (bejeweled3): the piece is still, in its cell and free to be played
	static bool PieceIsSettled(const Piece* thePiece);
	// slot 135, 0x74e410 (bejeweled3): a new game: its background is chosen and the first fill is made
	void NewGame();
	// slot 203, 0x763e90 (bejeweled3): makes the pieces that the empty cells need above the grid, lets them fall into
	// place one row at a time and gives them colours that make no match and leave a move. theNoMatchCheck is the
	// exe's second argument (set when it gave up). Classic's first fill is this with 0 for it.
	void FillInBlanks(bool theNoMatchCheck);

	// 0x73ba40 (bejeweled3): the grid has three or more of a colour in a row or column
	bool HasMatches() const;
	// 0x75a8e0 (bejeweled3) with no piece to include: some swap of two neighbours makes a run of three. theNeedBonus is
	// its last argument: only a run of four or more, or a crossing of two, counts
	bool HasMove(bool theNeedBonus);

	// slot 53, 0x73f140 (bejeweled3): picks a gem up, or swaps the one picked up with the one clicked
	using Widget::MouseDown;
	using Widget::MouseUp;
	void MouseDown(int x, int y, int theBtnNum, int theClickCount) override;
	// slot 57, 0x73d990 (bejeweled3): the button comes up
	void MouseUp(int x, int y, int theBtnNum, int theClickCount) override;
	// slot 58, 0x73c510 (bejeweled3): dragging a picked-up gem a fifth of a cell sideways or up and down swaps it
	void MouseDrag(int x, int y) override;
	// 0x73bf00, 0x73bc50 (bejeweled3): the piece that is picked up, and the piece at a place (in 1200ths from the grid's corner)
	Piece* GetSelectedPiece() const;
	Piece* GetPieceAt(float theX, float theY) const;
	// slot 194, 0x740e80 (bejeweled3): asks for a swap of the piece with the one at a cell. It starts in the next update
	bool TrySwap(Piece* thePiece, int theRow, int theCol);
	// slot 193, 0x75fae0 (bejeweled3): the swap is allowed
	bool CanSwap(Piece* thePiece, unsigned theRow, unsigned theCol);
	// slot 195, 0x75ee20 (bejeweled3): the swap begins
	bool StartSwap(Piece* thePiece, int theRow, int theCol);
	// 0x759ba0 (bejeweled3): the swaps move on, and are checked for matches when they come to an end
	void UpdateSwaps();
	// 0x73caa0 (bejeweled3): a swap holds the piece
	bool IsHeldBySwap(const Piece* thePiece) const;
	// slot 201, 0x75af60 (bejeweled3): looks for runs of three or more and makes a match of each; returns how many
	int FindMatches();
	// 0x75dee0 (bejeweled3): the match groups run: their pieces' timers, their rings, their end. The stand-ins for plain matches
	// are run by UpdatePlainClears
	void UpdateMatchGroups();
	void UpdatePlainClears();
	// 0x789a90 (bejeweled3), for the types other than 7: a group is made for a piece and put at the end of the list
	void MakeMatchGroup(Piece* thePiece, int theType);
	// 0x73c9d0 (bejeweled3): the index of the group made for the piece (the group's +0x34 is its id), or -1
	int FindMatchGroup(const Piece* thePiece) const;
	// slot 200, 0x74c570 (bejeweled3): the piece is to be cleared by another piece; true when it took it
	bool ClearPiece(Piece* thePiece, Piece* theBy);
	// 0x75dee0 and 0x74c770 (bejeweled3): the draws of the app's Rand() that the 20 particles of a ring make for one piece
	void DrawRingParticleRandoms();
	// true when any group or any stand-in for a plain clear lives (the fill waits for them)
	bool HasLiveMatches() const { return !mMatchGroups.empty() || !mPlainClears.empty(); }
	// slot 200 (0x74c570) and 0x74c770 (bejeweled3): the piece is destroyed; slot 176, 0x755b80 (bejeweled3): it leaves the grid
	void DestroyPiece(Piece* thePiece);
	void DrawSparkRandoms();
	void RemovePiece(Piece* thePiece);
	// slot 152, 0x74df80 with 0x74a210 (bejeweled3): points are added to the score
	void AddPoints(int thePoints);
	// slot 230, 0x73b6d0 (bejeweled3): the shown score follows the score, every fourth update
	void UpdateShownScore();
	// 0x744660 (bejeweled3): a new move record, and 0x73cf10 and 0x73d170: its counter
	MoveRecord* NewMoveRecord();
	MoveRecord* FindMoveRecord(int theId);
	// 0x73e370 (bejeweled3): the piece with an id
	Piece* FindPieceById(int theId) const;

	// 0x7415c0 (bejeweled3): a new piece at a cell
	Piece* MakePiece(int theCol, int theRow);
	// 0x73bd80 and 0x73bd70 (bejeweled3)
	int GetGridX() const { return GRID_X; }
	int GetGridY() const { return GRID_Y; }

	// slot 89, 0x7413e0 (bejeweled3)
	virtual const char* GetModeName() const { return "Classic"; }
	// 0x751d50 and 0x750320 (bejeweled3): the board's panel and gems
	void Draw(Graphics* g) override;
	void DrawPiece(Graphics* g, const Piece* thePiece);   // slot 237, 0x757df0 (bejeweled3)
};

class ClassicBoard : public Board {
public:
	ClassicBoard();   // 0x73b390 (bejeweled3)
};

}  // namespace Sexy
