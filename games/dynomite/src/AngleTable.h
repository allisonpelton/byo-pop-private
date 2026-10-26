#ifndef __ANGLETABLE_H__
#define __ANGLETABLE_H__

// The app's angle table at +0x5d4 (vtable 0x4fc230, 0xb48 bytes; 0x42bb50): for each whole degree the step of a
// unit move, degree 0 pointing up.

namespace Sexy
{

class AngleTable
{
public:
	float				mDX[360];		// +4 sin
	float				mDY[360];		// +0x5a4 -cos
	float				mPi;			// +0xb44

	AngleTable();						// 0x42bb50
	virtual ~AngleTable() {}

	int					AngleTo(int theFromX, int theFromY, int theToX, int theToY);	// 0x42bbc0
	// 0x42bcd0: where the lines through theLine1's and theLine2's two points (x1, y1, x2, y2) cross, and whether
	// the crossing is within both
	static bool			Intersect(const float* theLine1, const float* theLine2, float* thePoint);
};

}

#endif
