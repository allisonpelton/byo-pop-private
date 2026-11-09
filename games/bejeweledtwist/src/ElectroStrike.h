// The electro gems' lightning (0x71d017-0x71f5c0): a strike (ElectroStrike) is what an electro gem going off makes
// (Board::BreakPiece, 0x6c0ccf); it holds the bolts (ElectroBolt) that run along its row and column, or three of each
// for a power electro gem, and the ids of the gems it has lit. The board keeps the strikes at +0x149c, updates them in
// UpdateStrikes (0x6c87d2) and draws them in DrawStrikes (0x6aafc1).
#ifndef BJT_ELECTROSTRIKE_H
#define BJT_ELECTROSTRIKE_H

#include "Color.h"
#include "CurvedVal.h"

#include <vector>

namespace Sexy {

class Graphics;
class Piece;

// 0x64 bytes (ctor 0x71f003, dtor 0x71ea9d): one bolt of lightning from (mX1, mY1) to (mX2, mY2)
class ElectroBolt {
public:
	struct Point {
		float mX;
		float mY;
	};

	float mX1;                  // +0x00
	float mY1;                  // +0x04
	float mX2;                  // +0x08
	float mY2;                  // +0x0c
	Piece* mStart;              // +0x10: the gem at its start (nothing reads it)
	std::vector<Point> mLeft;   // +0x14: the jagged line's one edge
	std::vector<Point> mRight;  // +0x24: and its other
	float mProgress;            // +0x34: mTime over mDuration
	float mTime;                // +0x38: up by 0.1 an update
	float mDuration;            // +0x3c
	float mAngle;               // +0x40
	float mLength;              // +0x44
	Color mColor;               // +0x48
	int mCount;                 // +0x58: updates
	int mCel;                   // +0x5c: IMAGE_LIGHTNING's cel, picked anew every fifth update
	bool mDone;                 // +0x60: mProgress reached 1
	bool mStar;                 // +0x61: a power electro gem's

	ElectroBolt(int theX1, int theY1, int theX2, int theY2, const Color& theColor, float theDuration, bool theStar);
	void Update();             // 0x71eca6
	void Draw(Graphics* g);    // 0x71d3cc
};

// 0x218 bytes (ctor 0x71f10e, dtor 0x71eb9e)
class ElectroStrike {
public:
	int mX;                          // +0x000: the gem's centre
	int mY;                          // +0x004
	int mCount = 0;                  // +0x008: updates
	int mColor;                      // +0x00c: the gem's colour
	int mIsStarted = 1;              // +0x010: set to 1 by the constructor, never read
	int mKind;                       // +0x014: 0 a row and a column, 1 three of each (a power electro gem)
	float mSpreadAngle = 0;          // +0x018: set to 0 by the constructor, never read
	float mFade = 1.0f;              // +0x01c: down by 0.25 each step once the strike has nothing left to light
	int mPieceId;                    // +0x020: the electro gem's
	int mStep = 0;                   // +0x024: how far the lightning has spread, in cells
	float mTimer = 0;                // +0x028: to the next step
	std::vector<ElectroBolt*> mBolts;  // +0x02c
	std::vector<int> mIds;           // +0x03c: the electro gem's id, then the gems it has lit
	int mPointsId;                   // +0x04c: the gem's +0x2ec
	CurvedVal mNovaScale;            // +0x050 (a power electro gem's only)
	CurvedVal mNovaAlpha;            // +0x0c0: linked to mNovaScale
	CurvedVal mNukeScale;            // +0x130
	CurvedVal mNukeAlpha;            // +0x1a0: linked to mNukeScale
	float mLinger = 1.0f;            // +0x210: down to 0 by 0.01 an update; nothing reads it

	explicit ElectroStrike(Piece* thePiece);
	~ElectroStrike();
	void Update();           // 0x71ef51
	void Draw(Graphics* g);  // 0x71df1f
};

}  // namespace Sexy

#endif
