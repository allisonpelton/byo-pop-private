// InterfaceMgr.h: the frame around the board (the original's Sexy::InterfaceMgr): the ball column, the score and its roll-up,
// the shot meter, the fever lights, the best score of the level and the master's window. It draws into five pictures of its own
// that it keeps up to date, and then onto the screen in front of and behind the board. Only the making, the emptying, the
// ball column and the best score are translated so far; the drawing is not (the fields keep the interim names mUnknownNN,
// the offset in the original, until it is).

#ifndef __INTERFACEMGR_H__
#define __INTERFACEMGR_H__

#include <string>
#include <vector>

namespace Sexy
{

class Board;
class MemoryImage;
class ThunderballApp;

// One ball of the ball column: where it is (its top) and what it is doing (the original's 8 bytes at +0x168)
struct ColumnBall
{
	float mY;
	int mUnknown4;
};

class InterfaceMgr
{
public:
	explicit InterfaceMgr(Board* theBoard); // 0x4515a0
	virtual ~InterfaceMgr();				// the original's deleting destructor

	void Clear();				// 0x447260
	void Update();		// 0x4518b0
	void InitLevel();			// 0x4725f0
	void SettleDown();			// 0x4482d0: the score on show is the score
	void UpdateHighScore();		// 0x4724e0: the best score and its name for the level
	void CheckHighScoreSync();	// 0x448190: a score that has gone past the best is the best, with the player's name
	void SetNumBalls(int theNumBalls); // 0x457870
	void AddTopBalls(int theNumBalls); // 0x45c860: that many balls drop in at the top of the column
	void SyncStageNum();		// 0x4439b0: the pictures that name the stage

	int GetBallBottom() const;	// 0x4366a0: where the lowest ball of the column is

	ThunderballApp* mApp;		   // +4
	Board* mBoard;				   // +8
	MemoryImage* mImages[5];	   // +0xc: the left column (88 by 600), its twin, the top bar (624 by 74), the master's window (110 by 70), the bar under the top
	MemoryImage* mUnknown20[4];	   // +0x20: four more pictures
	int mUnknown30;				   // +0x30
	int mUnknown34;				   // +0x34
	int mUnknown38;				   // +0x38
	int mUnknown3C;				   // +0x3c
	int mUnknown40;				   // +0x40
	int mUnknown44;				   // +0x44
	int mUnknown48;				   // +0x48
	int mUnknown4C;				   // +0x4c
	int mUnknown50;				   // +0x50
	int mUnknown54;				   // +0x54
	int mBallColumnSink;		   // +0x58: how far the column has sunk (0 when it has not)
	int mBallColumnShift;		   // +0x5c: how far the column is moved down by
	int mUnknown60;				   // +0x60
	int mBestScore;				   // +0x64: the best score of the level's table (0 when there is none)
	int mUnknown68;				   // +0x68
	int mUnknown6C;				   // +0x6c
	std::string mBestName;		   // +0x70: who has it
	int mStageNumber;			   // +0x8c
	int mLevelNumber;			   // +0x90
	int mScoreShown[2];			   // +0x94: what each player's score reads as, which rolls up to the real one
	int mUnknown9C;				   // +0x9c: updates the roll-up waits for
	std::string mUnknownA0;		   // +0xa0
	std::string mUnknownBC;		   // +0xbc
	int mUnknownD8;				   // +0xd8
	int mUnknownDC;				   // +0xdc
	int mUnknownE0;				   // +0xe0
	int mUnknownE4;				   // +0xe4
	int mNumBallsShown;			   // +0xe8: the balls the column shows, at most 15
	int mNumBallsShownCopy;		   // +0xec
	int mUnknownF0;				   // +0xf0
	int mStageImageIndex;		   // +0xf4: the stage the pictures were last made for (-1: none)
	int mUnknownF8;				   // +0xf8
	bool mUnknown160;			   // +0x160: the column's balls are still
	bool mUnknown161;			   // +0x161
	std::vector<ColumnBall> mColumnBalls; // +0x168
};

} // namespace Sexy

#endif
