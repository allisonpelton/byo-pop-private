// Board: the screen a level is played on (0x405770..0x4059c0 in the original). It holds the backdrop, the score board
// and the level, which stands on the backdrop; it fades in as it opens, and leaves the way the main menu does.
#pragma once

#include "ObjBase.h"

namespace Sexy
{

class BackDrop;
class LevelObj;
class ScoreBoard;

class Board : public ObjBase
{
public:
	int mLeaveTrack;         // +0x280: the track MainMenu's StartLeaving (0x440a60) starts, also on a board
	int mFadeTrack;          // +0x284
	ObjBase mUnusedFirst;    // +0x288: an object made and destroyed with the board; nothing else uses it
	ObjBase mUnusedSecond;   // +0x504: likewise
	BackDrop* mBackDrop;     // +0x780
	ScoreBoard* mScoreBoard; // +0x784
	LevelObj* mLevel;        // +0x788
	int mUnused790;          // +0x790: set to 0 by the constructor; nothing reads it
	int mUnused794;          // +0x794: likewise
	int mUnused798;          // +0x798: likewise
	int mUnused79c;          // +0x79c: likewise

	Board();                 // 0x405770
	~Board();                // 0x405930

	void StartLeaving() { StartTrack(mLeaveTrack); } // 0x440a60
	void StartFading() { StartTrack(mFadeTrack); }   // 0x405f20
};

} // namespace Sexy
