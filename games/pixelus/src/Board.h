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
class UserProfile;

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
	int				mPuzzleIndex;		// its place in the temple
	std::string		mUuid;
	bool			mTutorial;			// +0x1a1 the first temple: the sidebar teaches
	bool			mExpert;			// the player has switched the hints and undo off (UserProfile::mFlag38)
	std::vector<UndoMove> mUndo;		// +0x17c
	int				mHoverCol;			// the cell under the mouse (the grid's +0xb088/+0xb08c)
	int				mHoverRow;
	bool			mHoverValid;		// +0xb098 a click there would do something
	int				mAlpha;				// +0xb184 how much of the grid shows (the intro fades it in)
	int				mIntro;				// updates since the puzzle appeared
	double			mPauseFade;			// +0x170 how far the pause screen has come in
	int				mDoneTimer;			// +0xe0 the countdown to the puzzle-completed screen, DONE_DELAY once it has started
	bool			mCompleted;			// the puzzle-completed screen has been asked for
	bool			mFinishSounded;		// the sting of a finished puzzle has played and the music fades
	int				mIdleTicks;			// +0x4c with no click (the original nags after 4500)
	int				mMusicDelay;		// +0x78 -1 until the puzzle has been on for a moment, then counts down to the in-game music
	int				mErrorTicks;		// +0x1cc
	int				mFlash[Grid::MAX_SIZE][Grid::MAX_SIZE];		// the glow of a cell a tile has just landed on
	bool			mFlashWanted[Grid::MAX_SIZE][Grid::MAX_SIZE];
	std::vector<int> mBestMoves;		// unused
	bool			mHintShown;
	int				mHintTicks;
	Grid::Move		mHintMove;

public:
	Board(PixelusApp* theApp, const std::string& theLevelName, int thePuzzleIndex);	// 0x407f10
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

	// the hint (0x433050): the next move of the best known solution, or an undo when the moves so far depart from it
	bool				FindHint(Grid::Move* theMove, bool* theUndo);
	void				ShowHint();

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
