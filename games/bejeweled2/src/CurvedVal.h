#ifndef __CURVEDVAL_H__
#define __CURVEDVAL_H__

// CurvedVal.h: Bejeweled 2's CurvedVal (0x30 bytes, no vtable; 0x5aeec6-0x5af142): a value that follows a curve while
// its input moves over a range. The title screen, the hyperspace and the menus animate with them.

namespace Sexy
{

class CurvedVal
{
public:
	enum Mode { MODE_CLAMP = 0, MODE_REPEAT = 1, MODE_PING_PONG = 2 };
	enum Ramp { RAMP_LINEAR = 0, RAMP_SLOW_TO_FAST = 1, RAMP_FAST_TO_SLOW = 2, RAMP_SLOW_FAST_SLOW = 3, RAMP_FAST_SLOW_FAST = 4 };

	int				mMode;          // +0x00
	int				mRamp;          // +0x04
	double			mOutMin;        // +0x08
	double			mOutMax;        // +0x10
	double			mInMin;         // +0x18
	double			mInMax;         // +0x20
	double			mInVal;         // +0x28

public:
	CurvedVal();                                            // 0x5aeec6

	void			SetMode(int theMode) { mMode = theMode; }                           // 0x5aef6a
	void			SetRamp(int theRamp) { mRamp = theRamp; }                           // 0x5aef76
	void			SetOutRange(double theMin, double theMax) { mOutMin = theMin; mOutMax = theMax; }   // 0x5aef83
	double			GetOutVal();                            // 0x5aefa9
	bool			CheckClamping();                        // 0x5aeee6 false once the input is held at an end (clamp mode)
	void			SetInVal(double theVal);                // 0x5af130
	bool			IncInVal(double theInc);                // 0x5af142
};

}

#endif
