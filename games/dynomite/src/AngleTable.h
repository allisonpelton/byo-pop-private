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
};

}

#endif
