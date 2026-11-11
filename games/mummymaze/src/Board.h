#ifndef __MUMMYMAZE_BOARD_H__
#define __MUMMYMAZE_BOARD_H__

// Board.h: the maze board of Mummy Maze Deluxe 1.1 (WinMM.exe 0x403e80-0x4125f0; object 0xec8 bytes, vftable 0x4814d4,
// ButtonListener at +0x4c). Behaviour spec: docs/GAME_NOTES.md. Offsets in comments are the original's; code never
// depends on them (the port's base classes have other sizes).
//
// The game is turn based on a square grid of 6, 8 or 10 cells: the explorer moves one cell (or waits), then the mummies
// move twice towards him (a white mummy tries to close the horizontal distance first, a red one the vertical), then
// the scorpion moves once.

#include "Widget.h"
#include "ButtonListener.h"
#include "HyperlinkWidget.h"
#include "DialogButton.h"
#include "MTRand.h"
#include <list>
#include <set>
#include <string>
#include <vector>

namespace Sexy
{

class MMApp;
class Image;
class MemoryImage;

enum
{
	MAX_SIZE = 10,
	NUM_HOVER = 11            // the hover table is 11 x 11
};

// Cell flags (Board::mCells[x][y])
enum CellFlags
{
	WALL_NORTH = 1,
	WALL_SOUTH = 2,
	WALL_EAST = 4,
	WALL_WEST = 8,
	EXIT_NORTH = 0x10,
	EXIT_SOUTH = 0x20,
	EXIT_EAST = 0x40,
	EXIT_WEST = 0x80,
	EXIT_ANY = 0xf0
};

// Board::mState (+0x5c)
enum BoardState
{
	STATE_ENTER = 0,            // the explorer walks into the maze
	STATE_READY = 1,            // waiting for a move
	STATE_EXPLORER = 2,         // the explorer moves
	STATE_SCORPION = 3,         // the scorpion moves
	STATE_MUMMY1 = 4,           // the first mummy step
	STATE_MUMMY2 = 5,           // the second mummy step
	STATE_LEAVE = 6,            // the explorer goes up the stairs
	STATE_SCORPION_LAST = 7,    // the scorpion's move after the mummies
	STATE_CAUGHT = 8,           // a mummy or the scorpion got the explorer
	STATE_LOST = 9,             // the game-over panel is shown
	STATE_DONE = 10,            // the level is complete
	STATE_SOLVING = 11,         // the solver runs ("Solving Puzzle")
	STATE_ENDING = 12,          // the ending scene
	STATE_ENDING_FADE = 13
};

// The state of the chase (0x3c bytes in the original): the solver and the undo stack copy it
struct ChaseState
{
	int			mExplorerX, mExplorerY;
	int			mMummyX[2], mMummyY[2];
	bool		mMummyAlive[2];
	int			mScorpionX, mScorpionY;
	bool		mScorpionAlive;
	bool		mGateClosed;

	bool operator<(const ChaseState& o) const;
	bool operator==(const ChaseState& o) const;
};

// The undo stack's record (0x3c bytes): positions, facings and the gate
struct UndoState
{
	int			mExplorerX, mExplorerY, mExplorerDir;
	int			mMummyX[2], mMummyY[2], mMummyDir[2];
	bool		mMummyAlive[2];
	int			mScorpionX, mScorpionY, mScorpionDir;
	bool		mScorpionAlive;
	bool		mGateClosed;
};

// 0x426e70: the explorer (a Widget in the original that the board draws; only its Update is used)
struct Player
{
	int			mX, mY;             // +0x50 the cell (-1 or the size while outside the maze)
	int			mTargetX, mTargetY; // +0x58, +0x5c
	double		mProgress;          // +0x60 0 .. the cell size while it walks
	int			mDir;               // +0x68 0 north, 1 east, 2 south, 3 west
	int			mIdleTimer;         // +0x6c
	bool		mIdle;              // +0x70 an idle animation is running
	int			mIdleKind;          // +0x74
	int			mIdleFrame;         // +0x78
	MMApp*	mApp;
	bool		mDirty;             // +0x18

	Player(MMApp* theApp, int theX, int theY, int theSize);    // 0x426e70
	void		Update();                                           // 0x426f70
};

// 0x421b40: a mummy
struct Mummy
{
	int			mX, mY;             // +0x50
	int			mTargetX, mTargetY; // +0x58, +0x5c
	double		mProgress;          // +0x60
	int			mDir;               // +0x68 the way it faces on screen
	int			mWantDir;           // +0x6c the way it will face; mDir turns towards it
	int			mTurn;              // +0x70 the turning animation's step (-3 .. 3)
	int			mIndex;             // +0x74 0 or 1
	bool		mFighting;          // +0x78 caught the explorer or another mummy
	int			mFightFrame;        // +0x7c 0 .. 0x80
	bool		mAlive;             // +0x80
	int			mIdleTimer;         // +0x84
	bool		mIdle;              // +0x88
	int			mIdleKind;          // +0x8c
	int			mIdleFrame;         // +0x90
	MMApp*	mApp;
	bool		mDirty;             // +0x18

	Mummy(MMApp* theApp, int theX, int theY, int theIndex);      // 0x421b40
	void		Update();                                           // 0x421bf0
	void		DoIdle();                                        // 0x421cc0
	void		ResetIdle();
	void		CheckGate();                                        // 0x421d00
};

// 0x429120: the scorpion
struct Scorp
{
	int			mX, mY;             // +0x50
	int			mFromX, mFromY;     // +0x58, +0x5c
	int			mToX, mToY;         // +0x60, +0x64
	int			mProgress;          // +0x68 pixels moved
	int			mDir;               // +0x6c
	bool		mStinging;          // +0x70
	int			mStingFrame;        // +0x74
	bool		mAlive;             // +0x78
	MMApp*	mApp;

	Scorp(MMApp* theApp, int theX, int theY);                 // 0x429120
	void		CheckGate();                                        // 0x429170
};

class Board : public Widget, public ButtonListener
{
public:
	MMApp*			mApp;                   // +0x58
	int					mSize;                  // +0x50 6, 8 or 10
	int					mCellSize;              // +0x54
	int					mState;                 // +0x5c
	Player*			mExplorer;              // +0x60
	Mummy*				mMummies[2];            // +0x64, +0x68
	Scorp*			mScorpion;              // +0x6c
	MemoryImage*		mDarkImage;             // +0x70 the dark pyramid's light-masked picture
	bool				mDarkImageValid;        // +0x74
	DialogButton*		mTutorialStepButton;    // +0x78 id 0: the tutorial's own button (shown once the board has opened)
	DialogButton*		mPlayNowButton;         // +0x7c id 2 ("PLAY NOW")
	HyperlinkWidget*	mContinueLink;          // +0x80 id 3 (after a level)
	int					mHover[NUM_HOVER][NUM_HOVER]; // +0x84 fade counters of the highlighted cells
	DWORD				mStartTick;             // +0x26c
	int					mSolveTick;             // +0x270 the update the solution was asked for (it starts 0x32 updates later)
	int					mUnsolvableTick;        // +0x274 the update the "can no longer be solved" notice counts from
	bool				mUnsolvable;            // +0x278 "MAZE CAN NO LONGER BE SOLVED"
	int					mTrapX[2], mTrapY[2];   // +0x284 .. +0x290 skull traps (99 = none)
	int					mTrapDeathTimer;        // +0x294 the death on a skull trap (-1 = off)
	int					mStungTimer;            // +0x298 the death by the scorpion's sting (-1 = off)
	int					mGateX, mGateY;         // +0x2a0 the gate is the barrier below this cell
	int					mKeyX, mKeyY;           // +0x2a8
	int					mMaze;                  // +0x2b0 the maze number in its file (1 .. 100)
	int					mFile;                  // +0x2b4 data\mazes\B-<n>.dat
	bool				mGateClosed;            // +0x2b8
	int					mGateAnim;              // +0x2bc 0 .. 7
	bool				mStarted;               // +0x2c0
	bool				mShowingSolution;       // +0x2c1
	int					mTrapAnim[2];           // +0x2c8, +0x2cc
	std::list<UndoState> mUndo;                 // +0x2e0
	std::set<ChaseState> mVisited;              // +0x2ec
	std::vector<ChaseState> mSolution;          // +0x2f8
	int					mCells[MAX_SIZE][MAX_SIZE]; // +0x300
	int					mDust[2][4];            // +0x490 four pairs of random dust positions
	bool				mRed;                   // +0x4b0 red mummies (vertical first)
	DWORD				mTimeBase;              // +0x4b4
	int					mClickX, mClickY;       // +0x4b8, +0x4bc a queued click (99 = none)
	DWORD				mMoveTick;              // +0x4c0
	int					mTorchFrame;            // +0x4c4
	int					mKeyFrame;              // +0x4c8
	double				mFade;                  // +0x4d0 the board fades in (1 = all in)
	double				mRiseFade;              // +0x4d8 the figures rise out of the floor once the board is in (1 = up)
	bool				mDark;                  // +0x4e0 dark pyramid
	int					mTutorialStep;          // +0x4e4 index into kTutorialMove
	int					mExitStyle;             // +0x4e8
	bool				mSolvable;              // +0x4ec the ankh is gold
	int					mEndAlpha;              // +0x4f0
	double				mEndFade;               // +0x4f8
	MTRand				mRand;                  // +0x500
	bool				mTipShown;              // +0xec0 a tip has come up for this board (or it is the tutorial's): no further tip
	bool				mIntroPlaying;          // +0xec1 the board's opening (the tomb slides, the howl at update 0xa5) has not ended

public:
	Board(MMApp* theApp);
	virtual ~Board();

	virtual void		AddedToManager(WidgetManager* theWidgetManager) override;
	virtual void		RemovedFromManager(WidgetManager* theWidgetManager) override;
	virtual void		Resize(int theX, int theY, int theWidth, int theHeight) override;
	virtual void		Update() override;
	virtual void		Draw(Graphics* g) override;
	virtual void		MouseMove(int x, int y) override;
	virtual void		MouseDown(int x, int y, int theClickCount) override;
	virtual void		ButtonDepress(int theId) override;
	virtual void		ButtonPress(int theId) override;
	using Widget::Resize;
	using Widget::MouseDown;
	using ButtonListener::ButtonPress;
	using ButtonListener::ButtonDepress;

	// level (BoardLevel.cpp)
	void				LoadMap();                     // 0x40e1d0
	void				LoadTreasureMap();                  // 0x40f2f0
	void				LoadTutorialMap(int thePart);     // 0x40f520
	void				NewGame();                   // 0x404770
	void				DoGameOver();                         // 0x404a30
	void				DoNewTurn();                     // 0x404a50
	void				UndoMove();                         // 0x404be0
	void				ResetMaze();                        // 0x404ef0
	void				ClearMoves();                    // 0x404dd0
	void				ClearCheckedList();                 // 0x404e30
	void				ClearSolutionList();                // 0x404e90
	void				DoSolution();                        // 0x4050b0
	bool				FindWay(std::vector<ChaseState>& thePath);   // port: the same search from where the explorer stands now
	void				CheckSolvable(bool thePlaySound); // 0x411640
	void				DoSolutionHelper(const ChaseState& theFrom, std::vector<ChaseState>& thePath, int theDX, int theDY, bool theRecord); // 0x405580 / 0x411a90
	void				StepScorpion(ChaseState& s);
	void				StepMummy(ChaseState& s, int theIndex);
	void				FlipGate();                   // 0x40e0e0
	void				KeyStep(int theX, int theY);    // 0x429170
	ChaseState			GetChaseState();

	// logic (BoardLogic.cpp)
	bool				CanMove(int x1, int y1, int x2, int y2, bool theOccupied, bool theGateClosed); // 0x4079c0
	bool				CanMove(int x, int y, bool theOccupied);   // 0x407b50
	void				DoResetIdles();                    // 0x407960
	void				DoProcessQueue();                 // 0x4065b0
	void				UpdateExplorer();               // 0x406f10
	void				UpdateMummies();                // 0x407cc0
	void				UpdateScorp();               // 0x4083e0
	void				SetMummiesInMotion();                  // 0x410090
	void				SetScorpInMotion();                 // 0x4103d0
	void				UpdateArrows();                     // 0x410a80
	void				ClearArrows();                   // 0x410ab0
	int					GetXAt(int theX) { return mCellSize * theX + 0x36; }   // 0x410ad0
	int					GetYAt(int theY) { return mCellSize * theY + 0x50; }   // 0x410ae0
	int					GetColAt(int thePixel) { return (thePixel - 0x36) / mCellSize; }  // 0x410af0
	int					GetRowAt(int thePixel) { return (thePixel - 0x50) / mCellSize; }  // 0x410b00
	int					GetPanAt(int theCell);               // 0x410b10
	void				MoveSound(int theCell);
	bool				SaveGame();                  // 0x410b70
	static bool			DeleteSavedGame();            // 0x410dd0

	// drawing (BoardDraw.cpp)
	void				DrawBoardStuff(Graphics* g, bool theWithExplorer);   // 0x4095f0
	void				DrawMinimalBoardStuff(Graphics* g);         // 0x40b2f0
	void				DrawNearWalls(Graphics* theClip, Graphics* theBlockGraphics = NULL);
	void				DrawTrapBlock(Graphics* g, int x, int y);
	void				DrawMummies(Graphics* g, int theIndex, bool theFull); // 0x40d130
	void				DrawExplorer(Graphics* g);      // 0x40d910
	void				DrawScorp(Graphics* g);      // 0x40de00
	void				DrawDark(Graphics* g);          // 0x40cb70
	void				DrawTreasure(Graphics* g);        // 0x40c950
	void				DrawArrow(int x1, int y1, int x2, int y2, Graphics* g);   // 0x410660
	void				DrawTutorialArrow(int x, int y, Graphics* g);                // 0x410860
	void				DrawWaitDialog(int x, int y, int theWidth, int theHeight, Graphics* g);        // 0x408780
	void				DrawTutorialDialog(int x, int y, int theWidth, int theHeight, Graphics* g);   // 0x408e50
	void				DrawClipped(Graphics* g, Image* theImage, int theX, int theY, int theClipX, int theClipY, int theClipW, int theClipH);
	int					SizeIndex() { return mSize == 8 ? 1 : (mSize == 10 ? 2 : 0); }
};

}

#endif
