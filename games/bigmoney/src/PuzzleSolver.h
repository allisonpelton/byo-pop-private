#ifndef __BIGMONEY_PUZZLESOLVER_H__
#define __BIGMONEY_PUZZLESOLVER_H__

// port(mods): the Puzzle solver (feature "puzzleSolver", mods/puzzle-solver; Board's solver code is marked
// port(mods)). Not in the original.
//
// A Puzzle level is won by dropping mBagsNeeded money bags off the bottom row; a bag leaves when every coin
// under it is gone, and the game is over when no group of two is left first (GAME_NOTES section 3). The
// solver looks for clicks that win the level from the board as it stands once nothing moves, and says
// one of: still thinking, can be won (with the clicks), can't be won (no clicks win), or too big to tell
// (it stopped at its limit).
//
// PuzzleModel is the board as the game's logic sees it: coins, bags and empty cells. A click removes a
// group and settles the board as Board::Compact and UpdatePhysics do in Puzzle: coins fall; a bag that
// reaches the bottom row leaves when it lands, the shortest fall first, and Compact runs again after each
// tick's departures; empty bottom cells close up, the two sides sliding together with the smaller side
// moving the odd step. Two of the game's quirks count too: a side that starts with a bag has size 0, and
// the right side's size counts only the coins the left side's flood did not reach. Bags that fall the
// same distance land in the same tick or a tick apart, as the bounce of the coins under them has it, and
// the order can change how the gaps close: the model gives every order (up to three such bags; more land
// together), so "can't be won" holds whichever happens, and a plan that counted on one order is searched
// again if another happens. Left out: FloodFill also stops at a coin whose mark (FindGroups leaves group
// sizes) equals mUpdateCnt, possible only in a game's first two seconds.
//
// The search skips a position from which too few bags can still leave: a bag can leave only if every coin
// under it has a colour with another coin on the board. It rates a position by the bags out, then the
// coins under the cheapest bags still needed, then the coins left. First a beam search, 16, then 64, then
// 256 wide: each round keeps the best positions one click further on, and finds most wins fast. Then a
// depth-first search over every position, best click first, remembering each position it has seen (a
// 64-bit hash): it finds the wins the beams missed, and only it can show that no clicks win (or a beam
// that never had to leave a position out). The search looks at a fixed number of positions per board tick
// (Step), so it reaches the same answer at the same tick on every machine and a replay plays out the same.

#include <cstdint>
#include <vector>

namespace Sexy
{

class PuzzleModel
{
public:
	enum
	{
		COLS = 14,
		ROWS = 15,
		EMPTY = -1,
		BAG = 8,
		MAX_ORDERED_BAGS = 3    // more bags landing together than this: only "all in the same tick"
	};

	int8_t		mCell[ROWS][COLS];      // [row][col], row 0 = top: EMPTY, a coin colour 0..4 or BAG
	int			mReleased;              // bags that have left the board
	int			mNeeded;                // bags that must leave to win the level

	PuzzleModel();

	bool		IsWon() const { return mReleased >= mNeeded; }
	// The cells (row * COLS + col) of the group of same-coloured coins at the cell; none for no coin
	void		GetGroup(int theCol, int theRow, std::vector<int>* theCells) const;
	// Every board a click on the cell can settle to (the first: bags that fall the same distance leave in
	// the same tick); none if it is not a group of 2
	void		ClickOutcomes(int theCol, int theRow, std::vector<PuzzleModel>* theOutcomes) const;
	uint64_t	Hash() const;
	// False if too few bags can still leave (a bag can leave only if every coin under it has a colour with
	// another coin on the board); else theScore rates the position, higher is closer to a win
	bool		Rate(int* theScore) const;

private:
	// Board::Compact; theFall[col] gets the fall distance of a bag that reached the bottom row (else kept),
	// and moves with its column
	void		Compact(int* theFall);
	// Board::FloodFill(col, row, mark, false, ...) with marks shared by the floods of one Compact column
	int			FloodSize(int theCol, int theRow, uint8_t* theMarks) const;
	// After a Compact: the landed bags leave, the shortest fall first, in every order
	void		Settle(const int* theFall, std::vector<PuzzleModel>* theOutcomes) const;
};

class PuzzleSolver
{
public:
	enum Status
	{
		STATUS_NONE,            // no board yet
		STATUS_THINKING,
		STATUS_WINNABLE,        // mPlan wins the level
		STATUS_LOST,            // no clicks win the level
		STATUS_GAVE_UP          // MAX_POSITIONS looked at without an answer
	};

	enum
	{
		MAX_POSITIONS = 600000, // a search's limit (the seen table holds 1M hashes, 8 MB)
		SEEN_BITS = 20,
		NUM_BEAMS = 3           // widths 16, 64, 256
	};

	struct Move
	{
		int8_t	mCol;
		int8_t	mRow;
		int8_t	mOutcome;               // which of PuzzleModel::ClickOutcomes the plan counts on
	};

	PuzzleSolver();

	// A settled board: the next position of the plan (the player clicked the hint) keeps the plan;
	// anything else starts a new search
	void		SetBoard(const PuzzleModel& theBoard);
	// A board whose winning clicks are known (PuzzleBuilder): they become the plan if playing them wins
	// (as far as the win; false, and nothing changed, if they don't)
	bool		SetPlan(const PuzzleModel& theBoard, const std::vector<Move>& thePlan);
	// Looks at up to about theBudget more positions
	void		Step(int theBudget);
	// The board of the last SetBoard, and its Hash
	const PuzzleModel& GetBoard() const { return mBoard; }
	uint64_t	GetBoardHash() const { return mBoardHash; }

	Status		mStatus;
	std::vector<Move> mPlan;            // STATUS_WINNABLE: the clicks that win, in order (none: already won)
	int			mPositions;             // positions this search has looked at

private:
	struct Child
	{
		PuzzleModel	mBoard;
		uint64_t	mHash;
		Move		mMove;
		int			mScore;
	};

	struct Frame
	{
		std::vector<Child> mChildren;   // best first
		size_t		mNext;
		Move		mMove;              // the click that led here (not for the root)
		uint64_t	mHash;
	};

	// A beam's next position: the click from frontier position mParent
	struct Candidate
	{
		int			mParent;
		Move		mMove;
		int			mScore;
	};

	PuzzleModel	mBoard;
	uint64_t	mBoardHash;
	std::vector<uint64_t> mSeen;        // open addressing, 0 = free
	int			mSeenCount;
	std::vector<uint64_t> mPlanHashes;  // the position after each move of mPlan
	std::vector<PuzzleModel> mOutcomes; // ClickOutcomes' list, reused

	// The beams (mBeam < NUM_BEAMS)
	int			mBeam;
	bool		mBeamDropped;           // this beam has left out a position
	std::vector<PuzzleModel> mFrontier;
	size_t		mExpand;                // the frontier position to expand next
	std::vector<Candidate> mCandidates;
	std::vector<std::vector<Candidate>> mLayers; // each frontier position's parent and click, per depth

	// Then depth first
	std::vector<Frame> mStack;

	bool		Remember(uint64_t theHash);     // false if it was seen before
	bool		IsSeen(uint64_t theHash) const;
	void		ClearSeen();
	// theBoard after theMove's click and outcome
	void		Play(PuzzleModel* theBoard, const Move& theMove);
	void		StartBeam(int theBeam);
	void		StepBeam();
	void		StartDepthFirst();
	void		StepDepthFirst();
	void		Expand(const PuzzleModel& theBoard, uint64_t theHash, Move theMove);
	// The clicks that win, from mBoard: the plan, and the hash after each
	void		Found(const std::vector<Move>& theMoves);
	void		Finish(Status theStatus);
};

// Whether a click leaves a level that can still be won (the solver's warnings): each board the click can
// settle to is searched in turn, and the first that can be won answers yes. "Loses" holds whichever way the
// bags land, as "can't be won" does.
class PuzzleClickCheck
{
public:
	enum Verdict
	{
		VERDICT_NONE,           // not known (still searching)
		VERDICT_WINS,
		VERDICT_LOSES,
		VERDICT_UNKNOWN         // no board it settles to can be won, and one was too big to tell
	};

	PuzzleClickCheck();

	// The click on the group at the cell (a group of 2) of a settled board
	void		Start(const PuzzleModel& theBoard, int theCol, int theRow);
	// Looks at up to about theBudget more positions; VERDICT_NONE while it has no answer
	int			Step(int theBudget);
	int			mPositions;             // positions this check has looked at

private:
	PuzzleSolver mSolver;
	std::vector<PuzzleModel> mOutcomes;
	size_t		mNext;                  // the outcome being searched (or to search)
	bool		mSearching;             // mSolver has mOutcomes[mNext]
	bool		mGaveUp;
	int			mVerdict;
};

}

#endif
