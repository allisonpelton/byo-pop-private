// Sexy::CurvedValue (0x30 bytes, no vtable; Step 0x49cfb0, Value 0x49d030): a value that goes from mStart to mEnd
// along a curve while a time mT moves over a range. The tile's alpha and slide use one each, and the board's
// effects do too. The caller moves mT (mT += step) and calls Step, which keeps mT in the range as the mode says
// and tells whether the curve is still running; Value is the value at mT.
#pragma once

namespace Sexy {

struct CurvedValue {
	// What Step does when mT leaves the time range
	enum Mode {
		MODE_ONCE = 0,           // mT stops at the end it reached, and Step says so
		MODE_LOOP = 1,           // mT wraps around
		MODE_LOOP_PINGPONG = 2,  // wraps around; the value goes up and back down within one range
		MODE_ONCE_PINGPONG = 3,  // stops at the ends; the value goes up and back down
	};
	// The shape of the value between mStart and mEnd
	enum Curve {
		CURVE_LINEAR = 0,
		CURVE_EASE_IN = 1,      // 1 - cos over a quarter wave: slow at the start
		CURVE_EASE_OUT = 2,     // sin over a quarter wave: slow at the end
		CURVE_EASE_IN_OUT = 3,  // (1 - cos) / 2 over a half wave: slow at both ends
		CURVE_SINE_BUMP = 4,    // sin / 2 below the quarter wave and (2 - sin) / 2 above, over a half wave
	};

	int mMode = MODE_ONCE;      // +0x00
	int mCurve = CURVE_EASE_IN_OUT;  // +0x04
	double mStart = 0.0;        // +0x08: the value at mTStart
	double mEnd = 1.0;          // +0x10: the value at mTEnd
	double mTStart = 0.0;       // +0x18: the time range
	double mTEnd = 1.0;         // +0x20
	double mT = 0.0;            // +0x28: now

	// 0x49cfb0: puts mT back in the range as the mode says; false once a MODE_ONCE curve has reached an end
	bool Step();
	// 0x49d030: the value at mT
	double Value() const;
};

}  // namespace Sexy
