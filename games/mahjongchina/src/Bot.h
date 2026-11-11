#ifndef __MAHJONGCHINA_BOT_H__
#define __MAHJONGCHINA_BOT_H__

// The port's bot (not the original's): the next pair a player could take on the board as it is, found by a search
// over a copy of the board (tools/dealtest.cpp's solver, for an escape stage and for a dealt classic game). The
// standard test command `move` (docs/TEST_COMMANDS.md) answers from it, and a later bot mod can use it as it is. It
// reads the board and never changes it: the search works on a copy of the layout's tiles with its own generator, so
// asking for a move draws no number of the game's.

#include "Layout.h"

#include <set>
#include <string>
#include <vector>

namespace mahjongchina
{

class Board;

// One move: a pair of free tiles, or a free special tile and the tile it acts on (Board::CanSelect 0x40c350, CanPair
// 0x40c3b0, Take 0x40c4d0): the key (0x2c) takes a lock (0x2d) that can be seen, the wild tile 0x2e every tile of
// the kind of an ordinary tile that can be seen (TakeJoker 0x40cc10), and 0x2f takes such a tile and becomes its kind
// (TakeMagnet 0x40cd80)
struct BotMove
{
	Tile* mFirst = nullptr;   // the live board's tiles
	Tile* mSecond = nullptr;
	int mFirstKind = 0;       // their kinds when the move is made
	int mSecondKind = 0;
	int mX1 = 0, mY1 = 0;     // where a click picks each, in game coordinates
	int mX2 = 0, mY2 = 0;
	bool mSpecial = false;    // a special tile moves: the board ignores clicks while its flights last
	bool mEnds = false;       // the move takes the key tiles (0x2b), which ends an escape level
};

class Bot
{
public:
	// The next move of theBoard, or false when it has none: no pair is free, or the game is over. Changes nothing.
	bool NextMove(Board& theBoard, BotMove& theMove);
	// The whole way to the end from the board as it is, as the game could play it: the moves in order, each with its
	// kinds. False when the search finds none (then NextMove falls back to any free pair).
	bool Solve(Board& theBoard, std::vector<BotMove>& theMoves);
	// The search alone on a layout: the number of moves to the end, or -1; theNodes gets the positions tried
	int SolveLayout(Layout& theLayout, long* theNodes = nullptr);
	// How many of the layout's tiles count as free (Layout::IsFree) and yet no click reaches (tools/dealtest.cpp counts them)
	int CountUnclickable(Layout& theLayout);

private:
	struct Step
	{
		int mFirst = -1;           // indexes of the copy's tiles
		int mSecond = -1;
		std::vector<int> mTaken;   // every tile the step takes
		int mChanged = -1;         // 0x2f, which took the second's kind
	};

	Layout mCopy;
	std::vector<Tile*> mTiles;     // the copy's tiles (the live board's untaken tiles, in its order)
	std::vector<Tile*> mLive;      // the live board's tiles that they copy
	std::vector<Step> mSteps;      // the search's moves so far
	std::set<std::string> mDead;
	long mNodes = 0;
	int mDeepest = 0;
	long mLimit = 400000;
	bool mGaveUp = false;
	bool mShuffle = false;
	bool mFreest = true;
	bool mOnlyGoalLocks = true;
	unsigned mOrder = 1;
	bool mGoldGoal = false;
	// the plan found, and the state before each of its steps, so the next calls follow it while the board does
	std::vector<Step> mPlan;
	std::vector<std::string> mPlanKeys;
	std::string mPlanLayout;
	// a search that found no way to the end: the bot plays the best-ranked move until the board stops going as that move said
	bool mNoPlan = false;
	std::string mExpected;

	void Copy(Layout& theLayout);
	std::string Key() const;
	bool Done() const;
	void Refresh(const std::vector<int>& theChanged, bool theBack);
	void Apply(Step& theStep);
	void Undo(Step& theStep);
	bool ClickPoint(Tile* theTile, int& theX, int& theY);
	bool CanClick(Tile* theTile);
	void Moves(std::vector<std::pair<int, Step>>& theMoves);
	bool SolveFrom();
	bool Search();
	bool Describe(Board& theBoard, const Step& theStep, BotMove& theMove);
};

} // namespace mahjongchina

#endif
