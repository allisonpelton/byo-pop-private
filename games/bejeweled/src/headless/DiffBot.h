#ifndef __DIFFBOT_H__
#define __DIFFBOT_H__

// port(variant): the difftest's player (games/bejeweled/docs/CODE_MODS_DESIGN.md 6.9, 9.1 item 7). Not part of the
// original game. Test code: its choices come from its own std::mt19937, never from the board's RNG.
//
// Like the game's autoplay bot (testhooks/Autoplay.h), but on a board alone: before each update it may give the
// board one input through the board's own handlers, in the order the game's widget manager calls them (a click
// is MouseMove, MouseDown, MouseUp; a drag MouseMove, MouseDown, MouseDrag..., MouseUp). Mostly a valid move
// (as a click on both gems, a click in the selected gem's click zone, or a drag), sometimes a random swap
// (usually illegal), the hint button, a right click, a selection while the board falls or clears, a user
// pause (a key) or a dialog's pause (the app's Pause/Unpause around a modal dialog), and rarely New Game.
// In timed mode most games take their time over each move, some more than others, so their time runs out.
// Two bots with the same seed on two boards that play alike give both the same input.

#include <cstdint>
#include <random>
#include <vector>

namespace Sexy
{

class Board;

class DiffBot
{
public:
	DiffBot(uint32_t theSeed);

	// Before theBoard's next update: the input of this tick, if any
	void			BeforeUpdate(Board* theBoard);

	// What it did (the difftest's summary)
	int				mMoves;
	int				mHints;
	int				mPauses;
	int				mNewGames;

private:
	enum StepKind
	{
		STEP_CLICK_CELL,    // click the gem in mCol/mRow, offset mDX/mDY from its centre
		STEP_CLICK_ZONE,    // click mDX/mDY from the selected gem's centre (MouseDown's click zone)
		STEP_RCLICK_CELL,   // right click on the gem in mCol/mRow (deselects)
		STEP_DOWN_CELL,     // button down on the gem in mCol/mRow: a drag starts
		STEP_DRAG_MOVE,     // the pointer mDX/mDY further, the button held (MouseDrag)
		STEP_DRAG_UP,       // the button up where the drag ended
		STEP_HINT,          // the hint button (Board::ShowHint)
		STEP_KEY,           // a typed key (KeyChar: the user pause on and off)
		STEP_UNPAUSE_CLICK, // a click on the paused board (MouseDown unpauses)
		STEP_MODAL_OPEN,    // a modal dialog opens (WinDMApp::ModalOpen: Pause(timed mode))
		STEP_MODAL_CLOSE,   // ... and closes (WinDMApp::ModalClose: Unpause(timed mode))
		STEP_NEW_GAME       // New Game, confirmed (Board::RequestNewGame)
	};

	enum StepRole
	{
		ROLE_SELECT,        // STATE_PLAYING, nothing selected, no pause, no input delay
		ROLE_SELECT_ANY,    // a state that takes a selection (not 5..8), nothing selected
		ROLE_TARGET,        // STATE_PLAYING with a selection
		ROLE_ANY
	};

	struct Step
	{
		int			mKind;
		int			mRole;
		int			mCol;
		int			mRow;
		int			mDX;
		int			mDY;
		int			mDelay;         // ticks after the previous step
	};

	std::mt19937	mRand;
	std::vector<Step> mPlan;
	size_t			mPlanPos;
	int				mWait;          // ticks until the next step is due
	int				mDragX;
	int				mDragY;
	bool			mModalOpen;     // a modal pause of ours is on
	int				mNewGameMove;   // the move after which this game asks for New Game, -1 for never
	int				mTimedThink;    // extra ticks before each move in timed mode: how fast the bar drains it to time up

	int				Random(int theRange);
	void			Plan(Board* theBoard);
	void			PlanMove(Board* theBoard);
	void			PlanTarget(Board* theBoard);
	void			AddTargetSteps(int theCol, int theRow, int theCol2, int theRow2, int theDelay);
	bool			RunStep(Board* theBoard, const Step& theStep);
};

}

#endif
