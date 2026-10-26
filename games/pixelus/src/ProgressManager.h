#ifndef __PIXELUS_PROGRESSMANAGER_H__
#define __PIXELUS_PROGRESSMANAGER_H__

// ProgressManager.cpp: the best known solutions of the puzzles and the unfinished puzzles of each player
// (Pixelus.exe 0x421380 constructor, app+0x770; vftable none). Two kinds of data:
//  - levels/maxedscores.txt: "<uuid>#<moves>#<solution>#" per puzzle, the shipped best solutions (the file of
//    the retail game is the table the developers solved the puzzles with). A player's better solution replaces
//    the entry; the whole table is written back, and levels/improvedmax.txt holds the improved entries only.
//  - users/his<id>: the puzzle a player has begun: "<uuid>#<moves>#<tiles>#<undo list>#<count>#" per puzzle.
// A solution is a run of 6-character moves (Grid::Move strings).

#include <map>
#include <string>

namespace Sexy
{

class PixelusApp;
class LevelData;

class ProgressManager
{
public:
	struct Best
	{
		int				mMoves;			// +0
		std::string		mSolution;		// +4
		bool			mImproved;		// +0x20 set by a player's solution
	};

	// the four strings of users/his<id>: 0x420cb0 stores the history of the moves (Grid 0x426430), the cells that
	// hold a tile (0x4259c0), the board's list of moves it can undo (0x4040a0) and a number
	struct History
	{
		std::string		mMoves;
		std::string		mTiles;
		std::string		mUndo;
		std::string		mCount;
	};

public:
	PixelusApp*					mApp;
	LevelData*					mLevelData;			// +0x1c
	std::map<std::string, Best>	mBest;				// +4
	std::map<std::string, History>	mHistory;		// +0x10 of the user loaded last
	int							mHistoryUser;

public:
	ProgressManager(PixelusApp* theApp, LevelData* theLevelData);	// 0x421380

	void				LoadBest();										// 0x421240 (0x420460)
	void				SaveBest();										// 0x41ecf0
	int					GetBestMoves(const std::string& theUuid) const;	// 0x41e6c0 (0: unknown)
	const Best*			FindBest(const std::string& theUuid) const;
	// 0x420760: a solution of theMoves moves; theMatched says it is at least as good as the best known one.
	// A better one replaces the entry
	void				UpdateBest(const std::string& theUuid, int theMoves, const std::string& theSolution, bool* theMatched);

	void				LoadHistory(int theUserId);						// 0x4208f0
	const History*		GetHistory(int theUserId, const std::string& theUuid);	// 0x4211f0
	void				SetHistory(int theUserId, const std::string& theUuid, const History& theHistory);	// 0x420cb0
	void				RemoveHistory(int theUserId, const std::string& theUuid);	// 0x421080
	void				SaveHistory(int theUserId);

private:
	void				ParseBest(const std::string& theText);
};

}

#endif
