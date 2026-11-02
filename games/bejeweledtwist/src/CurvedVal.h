// Sexy::CurvedVal (0x70 bytes, no vtable; 0x6e9737-0x6ed300): a value that follows a curve while its input moves over a
// range. Bejeweled Twist animates almost everything with them. A curve is either one of the built-in ramps or a curve
// string from the exe, such as "b;0,1,0.01,1,~###  ~####": a version letter, a flags character, then the output range,
// the input step and the input end, then the control points (BSpline.h). Curves made from the same points share one
// sampled table (a cache that lives as long as the program, as in the original).
#pragma once

#include <string>

namespace Sexy {

class CurvedVal {
public:
	enum Mode { MODE_CLAMP = 0, MODE_REPEAT = 1, MODE_PING_PONG = 2 };
	enum Ramp {
		RAMP_NONE = 0,
		RAMP_LINEAR = 1,
		RAMP_SLOW_TO_FAST = 2,       // 1 - cos over a quarter wave
		RAMP_FAST_TO_SLOW = 3,       // sin over a quarter wave
		RAMP_SLOW_FAST_SLOW = 4,     // (1 - cos) / 2 over a half wave
		RAMP_FAST_SLOW_FAST = 5,     // sin / 2, then (2 - sin) / 2
		RAMP_CURVEDATA = 6,          // a curve string's points
	};

	struct CurveRecord;

	int mMode;                        // +0x00
	int mRamp;                        // +0x04
	double mIncRate;                  // +0x08: the input's step per IncInVal()
	double mOutMin;                   // +0x10
	double mOutMax;                   // +0x18
	const char* const* mCurveRef;     // +0x20: a curve string elsewhere, read again when it changes (SetCurveRef)
	const char* mCurveRefLast;        // +0x24
	int mInitAppUpdateCount;          // +0x28: the app's update count when the curve was set (auto increment)
	CurvedVal* mLinkedVal;            // +0x2c: takes its input from this value
	CurveRecord* mCurveCacheRecord;   // +0x30: the shared table (the original points 0x1c bytes into the record)
	double mOutVal;                   // +0x38: the last GetOutVal()
	double mPrevOutVal;               // +0x40
	double mInMin;                    // +0x48
	double mInMax;                    // +0x50
	bool mNoClip;                     // +0x58: the curve may leave the output range
	bool mSingleTrigger;              // +0x59
	bool mOutputSync;                 // +0x5a: a value linked to this one takes its output, not its input
	bool mTriggered;                  // +0x5b
	bool mIsHermite;                  // +0x5c: the points carry slopes and the curve is a Hermite spline through them
	bool mAutoInc;                    // +0x5d: the input follows the app's update count
	double mPrevInVal;                // +0x60
	double mInVal;                    // +0x68

	CurvedVal();                                                          // 0x6e9773
	CurvedVal(const char* theCurve, CurvedVal* theLinkedVal = nullptr) : CurvedVal() { SetCurve(theCurve, theLinkedVal); }

	void SetConstant(double theValue);                                    // 0x6e97bd
	void SetCurve(const std::string& theCurve, CurvedVal* theLinkedVal = nullptr);  // 0x6eca95
	void SetCurveRef(const char* const* theCurveRef, CurvedVal* theLinkedVal = nullptr);
	void SetMode(int theMode) { mMode = theMode; }                        // 0x6e97f3
	void SetRamp(int theRamp) { mRamp = theRamp; }                        // 0x6e97ff
	void SetOutRange(double theMin, double theMax) { mOutMin = theMin; mOutMax = theMax; }  // 0x6e980c
	void SetInRange(double theMin, double theMax) { mInMin = theMin; mInMax = theMax; }     // 0x6e981f
	bool IsInitialized() const { return mRamp != RAMP_NONE; }             // 0x6e97ea

	bool SetInVal(double theInVal);                                       // 0x6ed03d
	bool IncInVal(double theInc);                                         // 0x6ed07d
	bool IncInVal();                                                      // 0x6ed0bc: by mIncRate
	double GetInVal();                                                    // 0x6ecf70
	double GetOutVal();                                                   // 0x6ecf57
	double GetOutVal(double theInVal);                                    // 0x6ecc24
	double GetOutValDelta() { return GetOutVal() - mPrevOutVal; }         // 0x6ed148
	double GetInValAtUpdate(int theUpdateCount) const { return theUpdateCount * mIncRate + mInMin; }  // 0x6e9832
	int GetLengthInUpdates() const;                                       // 0x6e9842: -1 for a curve that does not move
	bool CheckInThreshold(double theInVal);                               // 0x6ed0d9: the input crossed theInVal in the last step
	bool CheckUpdatesFromEndThreshold(int theUpdates) { return CheckInThreshold(GetInValAtUpdate(GetLengthInUpdates() - theUpdates)); }  // 0x6ed123
	bool HasBeenTriggered();                                              // 0x6ed155
	operator double() { return GetOutVal(); }

private:
	bool CheckCurveChange();                                              // 0x6ecb4a
	bool CheckClamping();                                                 // 0x6ecb9b
	void ParseDataString(const std::string& theString);                  // 0x6ec600
};

}  // namespace Sexy
