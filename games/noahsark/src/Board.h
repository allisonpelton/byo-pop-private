#ifndef __NOAH_BOARD_H__
#define __NOAH_BOARD_H__

// Board.cpp / BoardUpdate.cpp / BoardDraw.cpp: the playfield of Noah's Ark Deluxe (WinNoah.exe 0x401000-0x410bf0,
// 100 functions). Behaviour spec: docs/GAME_NOTES.md section 4.
//
// Original: class Sexy::Board : public Sexy::Widget, public Sexy::ButtonListener, sizeof 0x1108 (operator new
// in NoahApp::StartGame 0x418460), vftable 0x488368, ctor 0x401000, deleting dtor 0x401310. Offsets are the
// original's; the port's Widget is larger and nothing depends on them.
//
// The board is a 9x9 grid of Pieces, 50x50 pixels per cell, the top-left cell at (178, 16). Game modes: action
// (the flood rises in real time), strategy (turn based, the animals fall, new rows rise), puzzle (swap two
// neighbours to make three in a row) and the Stampede bonus (click a group of three or more).

#include "Widget.h"
#include "MTRand.h"
#include "Piece.h"
#include <SDL.h>
#include <string>
#include <vector>

namespace Sexy
{
class NoahApp;
class Graphics;

// Board::mState (+0x310)
enum BoardState
{
	BOARD_GET_READY = 0,         // the animals fall in; the first click starts the level
	BOARD_PLAYING = 1,
	BOARD_WON = 2,               // all animals saved: the ark sails off
	BOARD_LOST = 3,              // the flood reached the ark
	BOARD_LEVEL_DONE = 4,
	BOARD_NO_MOVES = 5,          // no pair can be joined (puzzle, stampede) or the waters took the rest
	BOARD_ABANDON = 6,           // action and strategy: the animals that cannot be joined are abandoned one pair at a time
	BOARD_SWAP = 7,              // puzzle: two neighbours slide past each other
	BOARD_SWAP_BACK = 8,
	BOARD_SETTLE = 9             // pieces fall into place after a match
};

// A fish that swims in the flood (WinNoah.exe 0x408df0 makes them, 0x408950 moves them, 0x408ce0 draws them)
struct Fish
{
	int				mX;                     // +0x00
	int				mY;                     // +0x04
	int				mDX;                    // +0x08 -1 or 1
	int				mDY;                    // +0x0c
	int				mFrame;                 // +0x10
	bool			mTurning;               // +0x14
	int				mTurnFrame;             // +0x18 0..9
};

// A bubble that rises through the flood (0x4090d0, drawn by 0x409180)
struct Bubble
{
	double			mX;                     // +0x00
	double			mY;                     // +0x08
	double			mSpeed;                 // +0x10
};

// A sparkle (0x408580 makes it, 0x408770 moves it, 0x408810 draws it)
struct Sparkle
{
	int				mX;                     // +0x00
	int				mY;                     // +0x04
	int				mLife;                  // +0x08
	int				mFrame;                 // +0x0c
	int				mHeight;                // +0x10
};

// A score that floats up from a cell (0x407bd0 makes it, 0x407f00 moves it, 0x407fa0 draws it)
struct FloatingText
{
	char			mText[256];             // +0x000 "-%d", "+%d" (puzzle and stampede) or "%d"
	Color			mColor;                 // +0x100
	int				mX;                     // +0x110 cell left
	int				mY;                     // +0x114
	int				mAlpha;                 // +0x118 0x14f, falls by 10; gone below 0x14
	int				mStars;                 // +0x11c the combo size (hearts drawn beside the text)
};

class Board : public Widget
{
public:
	NoahApp*				mApp;               // +0x50
	int						mMouseX;            // +0x54 where the mouse last was
	int						mMouseY;            // +0x58
	Piece*					mGrid[9][9];        // +0x5c [x][y]
	Piece*					mNextRow[9];        // +0x1a0 the row that rises into a strategy board (y offset 0x32)
	Piece*					mClone[9][9];       // +0x1c4 the level generator's copy (CheckSolvable)
	Piece*					mSelected;          // +0x308 the animal the player has picked
	bool					mPaused;            // +0x30c
	int						mState;             // +0x310 BoardState
	int						mLastThunderUpdate; // +0x314 mUpdateCnt at the last thunder; saved, and nothing reads it
	int						mStartTick;         // +0x31c GetTickCount() when the level began (the animals start to fall 400 ms later)
	int						mStartUpdate;       // port(mod fixes, steadyClock): the app's update count when the level began
	int						mLastInputTick;     // +0x318 mUpdateCnt of the last mouse press or drag
	int						mUnsetAfterStartTick; // +0x320 the original never sets it (0 in the port); saved, and nothing reads it
	int						mUnsetBeforeLastMoveTick; // +0x324 as +0x320
	int						mLastMoveTick;      // +0x328 mUpdateCnt of the last pair joined or hint: idle hints after 500 ticks
	bool					mShowGetReady;      // +0x32c
	int						mUnsetAfterShowGetReady; // +0x330 as +0x320
	bool					mLightning;         // +0x336 the lightning flash is on
	bool					mThunderCue;        // +0x337
	int						mStormTimer;        // +0x338 >0: the sky darkens (set to 3 by a combo)
	bool					mDragging;          // +0x33c a link was started by dragging
	int						mNewRowKinds;       // +0x340 how many kinds of animal a row pushed in is drawn from: one more every 3000 ticks
	int						mRowCount;          // +0x344 rows pushed in so far
	int						mArkX;              // +0x348 17
	int						mArkY;              // +0x34c 98
	int						mArkStage;          // +0x350 ark ramp image 0..4
	int						mAnimTick;          // +0x354 counts up every 6 ticks
	int						mFloodSpeed;        // +0x358 difficulty + level: the flood's pace
	bool					mNoahWorried;       // +0x35c
	int						mNoahFrame;         // +0x360
	int						mNoahSprite;        // +0x364 first of Noah's images in use (IMAGE_NOAH_HAPPY_STILL1)
	int						mLostFade;          // +0x368 counts up to 0xff once the game is lost; saved, and nothing reads it
	int						mWaterY;            // +0x36c the water's surface (starts at 0x1a2)
	int						mWaterTargetY;      // +0x370
	int						mFrontWavePhase;    // +0x374 the scroll of the waves in front of the board
	int						mBackWavePhase;     // +0x378 the scroll of the wave behind it
	int						mArkLevelY;         // +0x37c 200: when the surface reaches it the game is lost
	int						mBackdropTick;      // +0x380
	int						mBonusPoints;       // +0x384
	int						mLastType;          // +0x388 the kind of the last pair (0xe: none)
	int						mCombo;             // +0x38c pairs of the same kind in a row
	int						mComboPairs;        // +0x390
	int						mBestCombo;         // +0x394
	int						mHintIndex;         // +0x398 which of the four how-to-play lines is shown
	int						mHintTimer;         // +0x39c
	int						mHintAlpha;         // +0x3a0
	int						mShownCombo;        // +0x3a4 the number in "Combo X%d"
	int						mComboTextTimer;    // +0x3a8 >0: the combo line is on show (0xc when a combo scores, counted down)
	bool					mLeftBehindFading;  // +0x3ac the animals still on the board when the ark leaves have been set to fade
	int						mDangerPulse;       // +0x3b0 0..0xab, bounces: the red of the water when it is near the ark
	int						mDangerPulseDir;    // +0x3b4 1 or -1
	int						mFlashPulse;        // +0x3b8 100..250, bounces: the grey of an animal that flashes (a hint)
	int						mFlashPulseDir;     // +0x3bc 1 or -1
	bool					mGravity;           // +0x3c0 strategy and puzzle: pieces fall
	bool					mNoMoves;           // +0x3c1
	int						mAbandonAlpha;      // +0x3c4
	int						mAbandonIndex;      // +0x3c8
	int						mAbandonCount;      // +0x3cc
	MTRand					mRand;              // +0x3d0
	bool					mLoadedGame;        // +0xd90
	std::vector<Fish*>		mFish;              // +0xda4 (the original has an unused vector at +0xd94 before it; then bubbles +0xdb4, texts +0xdc4, sparkles +0xdd4)
	std::vector<Bubble*>	mBubbles;
	std::vector<FloatingText*>	mTexts;
	std::vector<Sparkle*>	mSparkles;
	std::vector<Piece*>		mLink;              // +0xde4 the pair being joined (or the group, in the Stampede)
	std::vector<PathNode*>	mTrail;             // +0xdf4 the path preview
	std::vector<PathNode*>	mPixelPath;         // +0xe04
	bool					mArkLeaving;        // +0xe15 (the original only ever writes +0xe14 and +0xe18 to +0xe20: the port has no such fields)
	int						mPathCells[50][2];  // +0xe24 the cells of the path found
	int						mCost[9][9];        // +0xfb4
	int						mPathSegments;      // +0x10f8
	int						mBestCost;          // +0x10fc
	int						mSteps;             // +0x1100
	int						mPathLength;        // +0x1104

	// port(mod): the mods' state (Mods.cpp): undo, the move helper, the tide and the daily board
	bool					mCtrlDown;
	std::vector<std::string>	mUndoStack;     // the board before each of the last moves
	int						mTidePercent;       // mod "tide": the flood's speed, 100 as the original's
	std::string				mModNote;           // a line under the ark: the daily board's date

public:
	Board(NoahApp* theApp, bool theContinue);
	virtual ~Board();

	virtual void			Update() override;
	virtual void			Draw(Graphics* g) override;
	virtual void			MouseMove(int x, int y) override;
	virtual void			MouseDown(int x, int y, int theClickCount) override;
	virtual void			MouseUp(int x, int y) override;
	virtual void			MouseUp(int x, int y, int theClickCount) override;
	virtual void			MouseDrag(int x, int y) override;
	virtual void			KeyDown(KeyCode theKey) override;
	virtual void			KeyUp(KeyCode theKey) override;
	using Widget::MouseDown;
	using Widget::MouseUp;

	// cells and pixels (0x40e280, 0x40e2a0, 0x40e2c0, 0x40e300)
	static int				CellLeft(int theX) { return theX * 50 + 178; }
	static int				CellTop(int theY) { return theY * 50 + 16; }
	static int				CellAtX(int thePixelX);
	static int				CellAtY(int thePixelY);
	static Color			AnimalColor(int theType);       // DAT_00494120

	// BoardLevel.cpp
	void					NewLevel();                     // 0x401580
	void					InitGrid();                     // 0x401a50
	void					PlaceBlocks(int theCount);      // 0x401ce0 (type 0xf: the immovable blocks)
	void					PlaceCrates(int theCount);      // 0x401d90 (type 0x10: the crates that fall)
	bool					CheckSolvable();                // 0x401e40
	int						CountEmpty();                   // 0x401ee0
	void					EnsureMoves();                  // 0x401f10
	void					RebuildClones();                // 0x401fc0
	void					FreeClones();                   // 0x402000
	bool					TryRemoveClonePair(Piece* thePiece);   // 0x4020e0
	bool					AllClonesGone();                // 0x402590
	void					SwapRandom();                   // 0x4025c0
	bool					Reachable(Piece* theFrom, Piece* theTo);  // 0x4026d0
	bool					CloneReachable(Piece* theFrom, Piece* theTo); // 0x402a70
	bool					FindPath(int theFromX, int theFromY, int theToX, int theToY, int theMaxSegments, bool theWantPath); // 0x403370
	bool					SearchPath(int x, int y, int theToX, int theToY, int theDepth, int theMaxDepth, int theCost, int (*thePath)[2]); // 0x4036f0
	void					ClearLink();                    // 0x403910
	bool					RowEmpty(int theRow);           // 0x403a50
	bool					ColumnEmpty(int theColumn);     // 0x403a00

	// BoardRules.cpp
	void					ClearLinkList();                // 0x407510
	void					ClearHint();                    // 0x4075d0
	bool					BoardCleared();                 // 0x406a20
	int						AnimalCount();                  // 0x406a60
	bool					AnyWaiting();                   // 0x406710
	void					ReleaseWaiting();               // 0x406750
	bool					AnyFalling();                   // 0x4067a0
	bool					HasPartner(Piece* thePiece, int theX, int theY); // 0x406cd0
	bool					HasMove();                      // 0x406aa0
	void					ShowHint();                     // 0x406d70
	void					ShowHintLine(int theIndex);     // 0x407460
	void					TriggerNoMoves();               // 0x4074c0
	void					AddFloatingText(int thePoints, int theMultiplier, const Color& theColor, int theCellX, int theCellY); // 0x407bd0
	void					UpdateTexts();                  // 0x407f00
	void					AddSparkle(int theX, int theY); // 0x408580
	void					UpdateSparkles();               // 0x408770
	void					ConnectPair();                  // 0x407600
	void					RemoveGroup();                  // 0x407930
	void					AbandonPair();                  // 0x4067e0
	void					EndRound();                     // 0x407b40
	void					GameOver();                     // 0x407480
	void					HighlightGroup(int theX, int theY); // 0x4070d0
	void					FloodGroup(int theX, int theY); // 0x407160
	bool					MarkRuns(bool theAnimate);      // 0x402260
	void					PushRow();                      // 0x403a90
	void					AddFish(int theCount, bool theAtSides); // 0x408df0
	void					UpdateFish();                   // 0x408950
	void					UpdateBubbles();                // 0x4090d0
	static const SexyString&	HintLine(int theIndex, int theLine);

	// BoardInput.cpp
	bool					BuildTrail(int theFromX, int theFromY, int theToX, int theToY); // 0x402ec0
	void					LinkPieces(Piece* theFirst, Piece* theSecond);
	bool					PuzzleSwap(int theCellX, int theCellY);

	// BoardUpdate.cpp
	void					ExchangeContents(Piece* theFirst, Piece* theSecond, int theOffsetXForA, int theOffsetXForB, bool theSwapOffsetY);

	// BoardDraw.cpp
	void					DrawFish(Graphics* g);          // 0x408ce0
	void					DrawBubbles(Graphics* g);       // 0x409180
	void					DrawSparkles(Graphics* g);      // 0x408810
	void					DrawTexts(Graphics* g);         // 0x407fa0
	void					DrawLevelNumber(Graphics* g);   // 0x40bfa0
	void					DrawScore(Graphics* g);         // 0x40c180
	void					DrawTrailNodes(Graphics* g, const std::vector<PathNode*>& theNodes, int theFrom, int theTo, const Color& theTint);
	void					DrawTrailArrow(Graphics* g, PathNode* theLast, PathNode* thePrev, const Color& theTint);
	void					DrawPreview(Graphics* g, bool theSecondPass); // 0x40c8a0
	void					DrawWalkingTrails(Graphics* g); // 0x40c460
	void					DrawBanner(Graphics* g, const SexyString& theText);

	// BoardSave.cpp
	bool					LoadGame();                     // 0x40d3c0
	bool					SaveGame();                     // 0x40dbe0
	std::string				SaveState();                    // the saved game's content (also the undo mod's snapshots)
	bool					LoadState(const std::string& theData);
	static std::string		SavePath();

	// Mods.cpp: port(mod)
	void					MouseDownImpl(int x, int y, int theClickCount);
	void					ModsNewLevel();                 // before a level's board is made: the daily seed, the tide, a clean undo list
	bool					ModBoardSettled();              // no animal walking, flying, fading or falling
	bool					ModUndoBegin(std::string& theSnapshot);
	void					ModUndoEnd(const std::string& theSnapshot, int theStateBefore);
	bool					ModUndo();
	void					ModShowMove();
	void					DrawModNote(Graphics* g);
};

SexyString FormatText(const SexyString& theFormat, int theValue);
SexyString FormatText2(const SexyString& theFormat, int theFirst, int theSecond);

}

#endif
