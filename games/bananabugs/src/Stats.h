// Stats: the figures shown after a level is won (0x46d2b0..0x46e910 in the original; 0x2ac bytes there). A picture of
// rings turns up behind five red captions (the bananas earned, the percent filled, the arrows shot, the fill over the
// target, the bonus points), and a figure for each is counted out below its caption, one after the other, with a click
// for each step. The bananas count up one at a time, each step marking the last banana icon on the score board; the
// others show at once. Then "Click To Continue" shows, and the click (LevelObj's state 5) sends the whole screen
// spinning away.
#pragma once

#include "ObjBase.h"

namespace Sexy
{

class LevelObj;

const int STATS_ROWS = 5;            // the captions and their figures

class Stats : public ObjBase
{
public:
	ObjBase* mBack;                  // +0x27c: the picture of rings behind the figures (+0x280 is never used)
	bool mShowing;                   // +0x284: set when the screen is made, cleared by Continue (a guess at the name;
	                                 //   nothing translated reads it)
	AnimKey* mTallyKey;              // +0x288: the counting key of the figure made last
	float mRowY[STATS_ROWS];         // +0x290: where each figure stands (each 40 below its caption)
	int mCounter;                    // +0x2a4: how many banana steps have been counted
	bool mReady;                     // +0x2a8: a figure has begun counting: a click now sends the screen away

	Stats(LevelObj* theLevel, int theX, int theY); // 0x46d2b0 (theX, theY: the middle of LevelObj's play area)
	void Notify(ObjBase* theObj) override; // 0x46d600
	bool IsReady() const { return mReady; } // 0x46da40
	void Continue();                 // 0x46d9e0

private:
	void BuildBack();                // 0x46e5c0
	void BuildTracks();              // 0x46e320
	void AddLabel(const char* theText, float theX, float theY, int theDelay, float theScale, int theIndex); // 0x46df90
	void AddFigure(const char* theText, float theX, float theY, int theDelay, float theScale, float theRed,
		float theGreen, float theBlue, int theEventId, bool theStartAtZero); // 0x46da50
};

} // namespace Sexy
