#ifndef __PIXELUS_BOARD_H__
#define __PIXELUS_BOARD_H__

// Board.cpp: the puzzle screen (Pixelus.exe 0x407f10 constructor, 0x404be0 Draw, 0x409bd0 Update, 0x408400 MouseDown,
// 0x403370 MouseMove; vftable 0x50c300, ButtonListener part 0x50c2e4). Behaviour spec: docs/GAME_NOTES.md.
//
// Original: class Board : public Widget, public ButtonListener (sizeof 0x1e4). It owns the puzzle grid (+0xe8, the
// original's Grid class draws itself and animates its tiles, 0x422ab0 and 0x42eee0-0x42f740), the Tile Master (+0xec),
// the workbench that shows the tiles left (+0xf0), the helper with the speech bubbles (+0xf4) and, in the tutorial, the
// teacher (+0xf8); five image buttons (undo, reset, hint, menu and the way back to the temple or cave).
//
// port: the grid's rules are Grid.cpp (free of the framework); this class draws the grid and moves the tiles. The
// control mode (+0x19c) is always 2 in the shipped game (the point-and-click one: a click on an empty cell throws a
// tile to it, a click on a tile pulls it back); the two keyboard-style modes are not ported. The editor (Draw Mode) is
// not ported. A puzzle in progress is saved as the list of its moves and replayed when the puzzle is opened again.

#include "Widget.h"
#include "ButtonListener.h"
#include "KeyCodes.h"
#include "Grid.h"
#include <string>
#include <vector>

namespace Sexy
{

class PixelusApp;
class Graphics;
class DialogButton;
class TileMaster;
class Character;
class UserProfile;
class HintBubbles;

class Board : public Widget, public ButtonListener
{
public:
	enum
	{
		ID_UNDO = 9,
		ID_RESET = 10,
		ID_MENU = 11,
		ID_BACK = 12,
		ID_HINT = 13
	};

	enum
	{
		GRID_X = 246,		// the grid's top left corner on the screen (0xf6, 0x4e)
		GRID_Y = 78,
		DONE_DELAY = 60,	// updates between the end of the finish (the picture is in) and the puzzle-completed screen (0x3c)
		SIDEBAR_WIDTH = 170
	};

	// the board's own list of moves it can take back (+0x17c, 16 bytes each)
	struct UndoMove
	{
		int		mSide;
		int		mCursor;
		int		mPosition;		// the line of the tile that was pulled (type 1)
		int		mType;			// 0 a thrown tile that stayed, 1 a pulled tile, 2 a throw that bounced back, 3 a tile that flew out
	};

	PixelusApp*		mApp;				// +0xd4
	Grid*			mGrid;				// +0xe8
	TileMaster*		mTileMaster;		// +0xec
	DialogButton*	mUndoButton;		// +0x12c
	DialogButton*	mResetButton;		// +0x124
	DialogButton*	mHintButton;		// +0x128
	DialogButton*	mMenuButton;		// +0x130
	DialogButton*	mBackButton;		// +0x11c (the way back to the temple) / +0x120 (to the cave)
	std::string		mLevelName;			// the temple this puzzle belongs to
	int				mPulseTime;			// +0x1d4 the update at which the sidebar's gleam comes again
	double			mPulse;				// +0x1d8 how far the gleam has come (0 to 1)
	Character*		mTeacher;			// +0xf8 the cyclops that teaches in the tutorial
	int				mPuzzleIndex;		// its place in the temple
	std::string		mUuid;
	bool			mTutorial;			// +0x1a1 the first temple: the sidebar teaches
	bool			mExpert;			// the player has switched the hints and undo off (UserProfile::mFlag38)
	std::vector<UndoMove> mUndo;		// +0x17c
	int				mHoverCol;			// the cell under the mouse (the grid's +0xb088/+0xb08c)
	int				mHoverRow;
	bool			mHoverValid;		// +0xb098 a click there would do something
	int				mAlpha;				// +0xb184 how much of the grid shows (the grid constructor, 0x425f7e, sets 255)
	int				mIntro;				// updates since the puzzle appeared
	double			mPauseFade;			// +0x170 how far the pause screen has come in
	bool			mPauseStarted;		// +0x1a2 the game went from running to paused (0x403010); only then does the pause screen come in
	int				mDoneTimer;			// +0xe0 the countdown to the puzzle-completed screen, DONE_DELAY once it has started
	bool			mCompleted;			// the puzzle-completed screen has been asked for
	bool			mMaxedAtStart;		// +0x1a0 the player's record for this puzzle was already as good as the world record (set when it loads)
	bool			mLessonShown;		// +0xd1 the hint of a tutorial puzzle has come up
	void			HintCheck();		// 0x407aa0
	enum { NUM_TIMERS = 26 };
	HintBubbles*	mHelper;			// +0xf4 the speech bubbles
	int				mLessonTimers[NUM_TIMERS];	// +0x68 the updates (in fifths) until a bubble comes up, one for each text
	bool			mLessonFast;		// +0xd0 the shown bubble's timer is one of the short ones
	int				mBubbleId;			// +0x1a4 the bubble that is up (-1 none)
	int				mBubbleX;			// +0x1a8 where it points
	int				mBubbleY;			// +0x1ac
	std::string		mBubbleText;		// +0x1b0
	int				mClickX;			// +0x148 where the mouse last went down (-1 never)
	int				mClickY;			// +0x14c
	bool			mFirstAfterTutorial;	// +0x1d0 a puzzle after the tutorial, whose player has seen all of it: the profile's flag 3 is set when it is solved
	bool			mFinishedOnce;		// +0x1d1 the finished puzzle has been handled (0x40a161)
	bool			mFinishSounded;		// the sting of a finished puzzle has played and the music fades
	int				mMovesSeen;			// +0xfc the grid's move count when mIdleCount last restarted
	int				mIdleCount;			// +0x100 updates (not paused) without a move; at 4500 the Hint button starts to pulse
	bool			mNagged;			// +0x104 it has
	int				mMusicDelay;		// +0x78 -1 until the puzzle has been on for a moment, then counts down to the in-game music
	int				mErrorTicks;		// +0x1cc
	std::vector<int> mBestMoves;		// unused
	bool			mHintShown;
	int				mHintTicks;
	Grid::Move		mHintMove;
	int				mGridX;				// where the grid's top left corner is (GRID_X, GRID_Y; the help screen's grids are elsewhere)
	int				mGridY;
	bool			mDemo;				// one of the help screen's two grids: no buttons, no sidebar, no puzzle of the temple

	// the grid plays a solution by itself (0x426560, replayed in 0x424b30): a move every mReplayPeriod updates
	bool			mReplayOn;			// +0xb140
	std::vector<Grid::Move> mReplay;	// +0xb144
	int				mReplayIndex;		// +0xb154
	int				mReplayDelay;		// +0xb158 updates to the next move
	int				mReplayPeriod;		// +0xb15c

public:
	Board(PixelusApp* theApp, const std::string& theLevelName, int thePuzzleIndex);	// 0x407f10
	// a grid for the help screen (0x40ddd0, 0x40df90): theText is a puzzle file's text, which may be empty (then the grid is
	// empty and the caller fills it)
	Board(PixelusApp* theApp, int theRows, int theCols, int theGridX, int theGridY);
	virtual ~Board();																// 0x4045e0

	virtual void		AddedToManager(WidgetManager* theWidgetManager) override;		// 0x4028c0 slot 1
	virtual void		RemovedFromManager(WidgetManager* theWidgetManager) override;	// 0x402950 slot 2
	virtual void		Resize(int theX, int theY, int theWidth, int theHeight) override;	// 0x402a50 slot 13
	virtual void		Draw(Graphics* g) override;										// 0x404be0 slot 16
	virtual void		Update() override;												// 0x409bd0 slot 17
	virtual void		KeyDown(KeyCode theKey) override;								// 0x402b00 slot 21
	virtual void		KeyUp(KeyCode theKey) override;									// 0x402b40 slot 22
	virtual void		MouseLeave() override;											// 0x402b60 slot 30
	virtual void		MouseMove(int x, int y) override;								// 0x403370 slot 25
	virtual void		MouseDown(int x, int y, int theClickCount) override;			// 0x408400 slot 27
	using Widget::MouseDown;
	virtual void		ButtonDepress(int theId) override;								// 0x409160 (listener)

	// the puzzle
	bool				LoadPuzzle();													// 0x408f60
	void				Restore();														// 0x408e70
	void				SaveProgress();													// 0x405e10
	void				RestartPuzzle();												// 0x432fa0
	void				Undo();															// 0x403fb0
	void				Aim(int theSide, int theCursor);								// 0x424590 + 0x42aec0
	void				RecordUndo(int theSide, int theCursor, int thePosition, int theType);	// 0x4083b0

	// moving the tiles
	void				DoThrow();
	void				DoPull();
	void				LaunchTile(GridObject* theObject, int theStartCol, int theStartRow, int theDir, bool theBounce);
	void				UpdateTiles();													// 0x42f740
	void				TileArrived(GridObject* theObject);								// 0x423e50
	bool				IsBeyondEdge(int theX, int theY, int theDir) const;				// 0x422040
	void				Complete();

	// the tutorial's bubbles (Lesson.cpp)
	void				InitLesson();
	void				ResetLessonTimers();										// 0x402700
	bool				TickLessonTimer(int theIndex);								// 0x4026a0
	void				ClearBubble();												// 0x402d90
	int					BubbleCorner(int theRow, int theCol);						// 0x402de0
	void				ShowBubble(int theX, int theY, int theCorner, int theIndex, bool theFlagged, bool theTick, bool theFire);	// 0x406050
	void				PointAtCell(int theRow, int theCol, int theCorner, int theIndex, bool theFlagged);	// 0x406140
	bool				PointAtStray(int theSkipRow, int theSkipCol);				// 0x4221f0, 0x422280, 0x402de0, 0x406140
	bool				IsLastThrownTile(int theRow, int theCol);					// 0x403930
	void				ShowLessonNote(int thePuzzle);
	int					ClickReason();												// 0x425620
	void				LessonStep0();												// 0x4061f0
	void				LessonStep1();												// 0x406310
	void				LessonStep2();												// 0x406470
	void				LessonStep3();												// 0x4066b0
	void				LessonStep4();												// 0x406b10
	void				LessonStep5();												// 0x406e60
	void				LessonStep6();												// 0x407150
	void				LessonStep7();												// 0x407510
	void				LessonStep8();												// 0x402f80

	// the hint (0x433050): the next move of the best known solution, or an undo when the moves so far depart from it
	bool				FindHint(Grid::Move* theMove, bool* theUndo);
	void				ShowHint();

	// the help screen's grids
	bool				LoadDemoPuzzle(const std::string& theText);					// 0x427330: load a puzzle (and its picture), the replay stops
	void				StartReplay(int theFirstDelay, int thePeriod);				// 0x426560, 0x422460
	void				UpdateDemo();													// the grid's part of 0x40d320 and 0x40d8d0
	void				DrawDemo(Graphics* g);
	void				SetHover(int theCol, int theRow, bool theValid);				// 0x4223d0

	// drawing
	void				DrawGrid(Graphics* g);											// 0x422ab0
	void				DrawTile(Graphics* g, GridObject* theObject);					// 0x42f6c0
	void				DrawTrail(Graphics* g, GridObject* theObject);					// 0x42ef70
	void				DrawAim(Graphics* g);
	void				DrawWorkbench(Graphics* g);										// 0x421450
	void				DrawSidebar(Graphics* g);
	void				PixelToCell(int x, int y, int* theCol, int* theRow) const;		// 0x421bc0
	int					TileKind(GridObject* theObject) const;							// 0x42ecf0
	int					GetMoves() const;
};

}

#endif
