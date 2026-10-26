#ifndef __GAMESTATS_H__
#define __GAMESTATS_H__

// port(variant): the counts behind the game-over screen's Stats page (features.gameStats,
// games/bejeweled/docs/CODE_MODS_DESIGN.md 4.19). Not part of the original game. A board with the feature on owns one
// and calls it from the places its replay recorder hooks: each scored run (its length, colour, cascade and points),
// the other points and penalties, each move, swap back, hint and undo, and the settle that ends a move; after each
// update it counts the play time and the level-ups itself.
//
// Like the replay recorder and the LiveSplit observer it is no game state: it never draws from an RNG, is not in
// StateCodec's LOGIC and adds nothing to the score key, so LOGIC hashes are the same with the feature on and off. An
// undo takes back the counts of the move it undoes: the board keeps a copy of the counts with each undo snapshot. Across
// a save the counts go into the save's sidecar as a STAT record (Encode, Decode); a resume without one counts from the
// resume on (mFromResume), and the page says so. A watched replay's board counts as it plays.

#include <cstdint>
#include <string>
#include <vector>

namespace Sexy
{

class Board;

class GameStats
{
public:
	enum
	{
		MAX_COLORS = 12,
		MAX_TIMELINE = 1024     // the score line's points: when full, every other one goes (the line keeps its shape)
	};

	struct Level
	{
		int			mLevel = 0;         // the board's mLevel (0 for the first)
		int			mPoints = 0;        // the score it gained
		int			mTicks = 0;         // its play time in updates (100 a second)
		int			mMoves = 0;
	};

	struct Point
	{
		int			mTick = 0;          // play time
		int			mScore = 0;
	};

	// Play
	int				mMoves = 0;             // moves that scored (swaps, twists, slides), and kept invalid moves that count
	int				mSwapsBack = 0;
	int				mInvalidKept = 0;
	int				mInvalidPoints = 0;     // their penalties (points taken off)
	int				mHints = 0;
	int				mHintPoints = 0;
	int				mUndos = 0;
	int				mUndoPoints = 0;
	int				mTicks = 0;             // play time in updates, not paused, until the game ends
	int				mTwists = 0;
	int				mSlides = 0;
	// Matches
	int				mRuns[3] = {};          // runs of 3, 4 and 5 or more
	int				mRunPoints[3] = {};
	std::vector<int> mCascades;             // moves by how many sets of runs they cleared: [0] one, [1] two, ...
	int				mGems[MAX_COLORS] = {}; // gems cleared in runs, per colour
	int				mOtherPoints = 0;       // points not from runs: blasts, gold, dirt
	int				mSpecialsMade = 0;
	int				mSpecialsSetOff = 0;
	int				mGoldCovered = 0;
	// Levels and the score over time
	std::vector<Level> mLevels;
	std::vector<Point> mTimeline;           // the score after each move, and at each level-up
	std::vector<int> mLevelUpTicks;         // play time of each level-up (the line's marks)
	bool			mFromResume = false;    // counted from a resume on: the save had no STAT record

	GameStats();

	// The board's hooks
	void			OnRun(int theLength, int theColor, int thePoints);
	void			OnPoints(int thePoints) { mOtherPoints += thePoints; }
	void			OnMove(Board* theBoard);
	void			OnSwapBack() { mSwapsBack++; }
	void			OnInvalidKept(int thePenalty) { mInvalidKept++; mInvalidPoints += thePenalty; }
	void			OnHint(int thePenalty) { mHints++; mHintPoints += thePenalty; }
	void			OnUndone(const GameStats& theSnapshot, int thePenalty); // the counts before the move, but the play time and undos
	void			OnTwist() { mTwists++; }
	void			OnSlide() { mSlides++; }
	void			OnSpecialMade() { mSpecialsMade++; }
	void			OnSpecialSetOff() { mSpecialsSetOff++; }
	void			OnGoldCovered(int theCells) { mGoldCovered += theCells; }
	void			OnSettle(Board* theBoard);  // a move's last cascade has landed (theBoard->mCascadeCount runs deep)
	void			AfterUpdate(Board* theBoard);
	void			OnLoad(Board* theBoard);    // a resumed game, after Decode (or without a STAT record)

	int				RunPoints() const { return mRunPoints[0] + mRunPoints[1] + mRunPoints[2]; }
	int				Penalties() const { return mInvalidPoints + mHintPoints + mUndoPoints; }

	// The STAT record of a save's sidecar (Replay.h). Decode leaves this object as it was and returns false for
	// bytes it does not read whole.
	std::string		Encode() const;
	bool			Decode(const std::string& theBytes);

private:
	int				mLastLevel;             // the board's mLevel at the last update; -1 before the first
	int				mLevelStartScore;
	bool			mEnded;                 // the game is over: the play time stops
	bool			mMovePending;           // a move was made and has not settled yet

	Level&			CurrentLevel(Board* theBoard);
	void			AddPoint(int theTick, int theScore);
};

}

#endif
