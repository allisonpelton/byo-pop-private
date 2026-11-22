// DebugMgr.h: the original's debugging help for the board (Sexy::DebugMgr): balls put on the board with the mouse, a chosen
// number of pegs left, the fever and the flippers forced on. It is in the game that shipped, behind its cheat words. Only the
// making, the emptying and the start of a level are translated so far (with the settings at their defaults, nothing here does
// anything; the code that acts on the settings is NOT_TRANSLATED).

#ifndef __DEBUGMGR_H__
#define __DEBUGMGR_H__

#include "SharedPtr.h"

namespace Sexy
{

class Board;
class Ball;

class DebugMgr
{
public:
	explicit DebugMgr(Board* theBoard); // 0x43ffa0
	virtual ~DebugMgr();				// 0x43e470

	void Clear();	// 0x43e4a0
	void InitLevel(); // 0x464930
	void SyncNumDebugPegs(); // 0x446670

	bool mUnknown04;	  // +4: set from the start
	bool mFeverOn;		  // +5: SyncFeverState keeps the fever on
	bool mFlippersOn;	  // +6: SyncFlipperState keeps the flippers on
	SharedPtr<Ball> mUnknown08; // +8: the ball the mouse holds
	int mNumPegsLeft;	  // +0x14: -1 for the whole level (SyncNumDebugPegs leaves this many of the pegs)
	int mNumGoalPegsLeft; // +0x18: -1 for the whole level (and this many of the goal pegs)
	int mUnknown1C;		  // +0x1c
	bool mUnknown20;	  // +0x20
	bool mUnknown21;	  // +0x21
	Board* mBoard;		  // +0x24
};

} // namespace Sexy

#endif
