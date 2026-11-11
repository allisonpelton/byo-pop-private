#ifndef __AUTOPLAY_H__
#define __AUTOPLAY_H__

// port: test hook for -autoplay=<seed> (see WinDMApp.h). Not part of the original game.
//
// A bot that plays the game through script input: after every update it looks at the game and queues
// its next mouse event with SexyAppBase::AddScriptCommand for the next update, so its clicks and drags
// take exactly the path of a -script file's commands (RunScript, before WidgetManager::UpdateFrame).
// Its choices come from its own std::mt19937, never from the game's Rand(), so the game's RNG stream is
// the one a -script run with the same input would see; for a given -seed and -autoplay seed the run is
// the same every time. -autoplaylog=<file> writes the input as a -script file (plus comments on what
// happened), which replays the same game on any build, also one without the bot.
//
// What it does: clicks the title screen's play link, switches to timed mode with -autoplaytimed, then
// plays: mostly a random valid move, sometimes a random (usually illegal) swap or the hint button, as a
// click on both gems, a click on the gem and one in the click zone toward the other, or a drag; on a board with
// twists or slides a move of any of the rules' types, a twist as a click (a right click anticlockwise)
// where its four gems meet, a slide as a drag along its line;
// sometimes it selects a gem while the board is falling, clearing or falling out (a stale selection
// after a fall-out). It quits 100 updates after the game-over screen appears, or at the update cap.

#include "MoveSet.h"

#include <cstdio>
#include <random>
#include <string>
#include <vector>

namespace Sexy
{

class WinDMApp;
class Board;
struct Piece;

class AutoplayBot
{
public:
	AutoplayBot(WinDMApp* theApp, unsigned long theSeed, bool timedMode, int theUpdateCap, const std::string& theLogFileName,
		const std::string& theShotFileName);
	~AutoplayBot();

	// After each update (WinDMApp::UpdateFrames): looks at the game, queues input for the next update
	void Update();

	// The valid swaps of the board whose two gems take a click, packed 4 bits a coordinate as PlanMove packs them (col, row,
	// col2, row2), in FindMove's order; the one place both -autoplay's valid moves and the test command "move"
	// (TestCommands.cpp) choose from. Reads the game, changes nothing and draws no random number
	static void ValidClickableMoves(WinDMApp* theApp, std::vector<int>* theMoves);
	// A board position (Board::GetColX, Piece::mX) in the window, and whether a click there reaches the board itself
	static int ToScreenX(WinDMApp* theApp, int theX);
	static int ToScreenY(WinDMApp* theApp, int theY);
	static bool ClickReachesBoard(WinDMApp* theApp, int theX, int theY);

private:
	enum StepKind
	{
		STEP_CLICK_CELL,    // click the gem in mCol/mRow (offset by mDX/mDY from its centre)
		STEP_CLICK_ZONE,    // click mDX/mDY from the selected gem's centre (MouseDown's click zone)
		STEP_DOWN_CELL,     // button down on the gem in mCol/mRow, starts a drag
		STEP_DRAG_MOVE,     // mouse move mDX/mDY further (MouseDrag)
		STEP_DRAG_UP,       // button up where the drag ended
		STEP_CLICK_HINT,    // click the hint button
		STEP_CLICK_AT,      // click at the board position mDX/mDY (a right click with mCol 1): a twist
		STEP_DOWN_AT        // button down at the board position mDX/mDY, starts a drag: a slide
	};

	// What a step needs when it is due; if the game has moved on, the rest of the plan is dropped
	enum StepRole
	{
		ROLE_SELECT,        // STATE_PLAYING, nothing selected
		ROLE_SELECT_ANY,    // a state that takes a selection (not 5..8), nothing selected
		ROLE_TARGET,        // STATE_PLAYING with a selection
		ROLE_ANY            // the end of a drag, the hint button
	};

	struct Step
	{
		int mKind;
		int mRole;
		int mCol;
		int mRow;
		int mDX;
		int mDY;
		int mDelay;          // updates after the previous step
	};

	WinDMApp* mApp;
	std::mt19937 mRand;
	unsigned long mSeed;
	bool mWantTimed;
	int mUpdateCap;
	FILE* mLog;
	std::string mShotFileName;  // -autoplayshot
	bool mDone;
	bool mTitleClicked;
	bool mTimedClicked;
	std::vector<Step> mPlan;
	size_t mPlanPos;
	int mNextStepUpdate;   // update at which mPlan[mPlanPos] is due
	int mDragX;            // drag start (STEP_DOWN_CELL) and end (STEP_DRAG_MOVE), board positions
	int mDragY;
	int mGameOverUpdate;   // update the game-over screen was first seen, -1 before
	int mMoveNum;
	Board* mLastBoard;     // for the log comments
	int mLastState;
	int mLastLevel;

	int Random(int theRange);
	void Queue(const std::string& theCommand, int theX, int theY);
	void Queue(const std::string& theCommand, const std::string& theArgs);
	void Note(const char* theFormat, ...);
	void LogChanges();
	bool InputBlocked();
	int ScreenX(int theX);
	int ScreenY(int theY);
	bool OnBoard(int theX, int theY);
	void PlanMove();
	void PlanFallingSelect();
	void PlanTarget(Piece* theSelected);
	void AddTargetSteps(int theCol, int theRow, int theCol2, int theRow2, int theStyle, int theDelay);
	void PlanVariantMove(bool isRandom, int theDelay);
	bool VariantMoveOnBoard(const MoveSet::Move& theMove);
	bool RunStep(const Step& theStep);
};

}

#endif
