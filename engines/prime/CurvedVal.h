// Sexy::CurvedVal (0x70 bytes, no vtable): a value that follows a curve while its input moves over a range. A curve is
// either one of the built-in ramps or a curve string such as "b;0,1,0.01,1,~###  ~####": a version letter, a flags
// character, then the output range, the input step and the input end, then the control points. Curves made from the
// same points share one sampled table (a cache that lives as long as the program, as in the original).
//
// Bejeweled Twist's port has the generation before this one (games/bejeweledtwist/src/CurvedVal.h). This one takes the
// app's update counter by pointer when it is made, keeps the linked value and the table four bytes further on, rounds
// the table's position to a float, and adds the scaled curve (SetCurveMult), the hand-over that starts a new curve
// where the old one's output stands (SetCurveSmooth), the input setter that moves an automatic curve's start
// (SetInVal's realign), IsDoingCurve and ClearTrigger.
#pragma once

#include "SexyMathHermite.h"

#include <map>
#include <string>
#include <vector>

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
	enum { CURVE_TABLE_SIZE = 0x2000 };

	// A control point of a curve string: x, y and, for a Hermite curve, the slope there in degrees
	struct DataPoint {   // 0xc bytes
		float mX;
		float mY;
		float mAngleDeg;
	};

	// A curve's shared data: the sampled table and the Hermite spline (the original's map node holds the key first,
	// so a value's pointer is 0x1c bytes into it)
	struct CurveCacheRecord {
		float mTable[CURVE_TABLE_SIZE];   // +0x0000
		SexyMathHermite mHermiteCurve;    // +0x8000
	};

	int mMode;                         // +0x00
	int mRamp;                         // +0x04
	double mIncRate;                   // +0x08: the input's step per IncInVal()
	double mOutMin;                    // +0x10
	double mOutMax;                    // +0x18
	const char* const* mCurveRef;      // +0x20: a curve string elsewhere, read again when it changes
	const char* mCurveRefLast;         // +0x24
	int mInitAppUpdateCount;           // +0x28: the app's update count when the curve was set (automatic input)
	const int* mAppUpdateCountSrc;     // +0x2c: the app's update counter, taken when the value is made
	CurvedVal* mLinkedVal;             // +0x30: takes its input from this value
	CurveCacheRecord* mCurveCacheRecord;  // +0x34
	double mOutVal;                    // +0x38: the last GetOutVal()
	double mPrevOutVal;                // +0x40
	double mInMin;                     // +0x48
	double mInMax;                     // +0x50
	bool mNoClip;                      // +0x58: the curve may leave the output range
	bool mSingleTrigger;               // +0x59
	bool mOutputSync;                  // +0x5a: a value linked to this one takes its output, not its input
	bool mTriggered;                   // +0x5b
	bool mIsHermite;                   // +0x5c: the points carry slopes and the curve is a Hermite spline through them
	bool mAutoInc;                     // +0x5d: the input follows the app's update count
	double mPrevInVal;                 // +0x60
	double mInVal;                     // +0x68

	CurvedVal();                                                                  // 0x436970 (bejeweled3), 0x436940 (bejeweled3) calls it
	CurvedVal(const std::string& theCurve, CurvedVal* theLinkedVal = nullptr);   // 0x436950 (bejeweled3)

	void SetCurve(const std::string& theCurve, CurvedVal* theLinkedVal = nullptr);  // 0x4376f0 (bejeweled3)
	void SetCurveMult(const std::string& theCurve);                               // 0x437730 (bejeweled3): the curve's top scaled by the value now
	void SetCurveSmooth(const std::string& theCurve, double theSearchInc);          // 0x437f50 (bejeweled3)
	void SetConstant(double theValue);                                            // 0x437760 (bejeweled3)
	void SetMode(int theMode) { mMode = theMode; }                                // 0x4378c0 (bejeweled3)
	void SetInRange(double theMin, double theMax) { mInMin = theMin; mInMax = theMax; }  // 0x4378d0 (bejeweled3)
	bool IsInitialized() const { return mRamp != RAMP_NONE; }                     // 0x437790 (bejeweled3)

	bool SetInVal(double theInVal, bool theRealign = false);                      // 0x437d50 (bejeweled3)
	bool IncInVal(double theInc);                                                 // 0x437dc0 (bejeweled3)
	bool IncInVal();                                                              // 0x437e00 (bejeweled3): by mIncRate
	double GetInVal();                                                            // 0x437c90 (bejeweled3)
	double GetOutVal();                                                           // 0x437c60 (bejeweled3)
	double GetOutVal(double theInVal);                                            // 0x4378f0 (bejeweled3)
	double GetInValAtUpdate(int theUpdateCount) const;                            // 0x437eb0 (bejeweled3)
	int GetLengthInUpdates() const;                                               // 0x437ec0 (bejeweled3): -1 for a curve that does not move
	bool CheckInThreshold(double theInVal);                                       // 0x437e30 (bejeweled3): the input crossed theInVal in the last step
	bool CheckUpdatesFromEndThreshold(int theUpdates);                            // 0x437e80 (bejeweled3)
	bool HasBeenTriggered();                                                      // 0x437ef0 (bejeweled3)
	void ClearTrigger() { mTriggered = false; }                                   // 0x437f20 (bejeweled3)
	bool IsDoingCurve();                                                          // 0x437f30 (bejeweled3): the input has not reached its end
	operator double() { return GetOutVal(); }

	static std::map<std::string, CurveCacheRecord*> mCurveCacheMap;

private:
	bool CheckCurveChange();                                                      // 0x4377a0 (bejeweled3)
	bool CheckClamping();                                                         // 0x437830 (bejeweled3)
	void ParseDataString(const std::string& theString);                          // 0x436fb0 (bejeweled3)
	// the input whose output is nearest theOutVal, trying every theInc of the input's range from theStartInVal to
	// theEndInVal (fractions of it); -1 when the range is empty
	double FindClosestInToOutVal(double theOutVal, double theInc, double theStartInVal, double theEndInVal);  // 0x437fb0 (bejeweled3)
	static void GenerateTable(const std::vector<DataPoint>& theDataPoints, float* theTable, int theSize, bool theNoClip);  // 0x4369e0 (bejeweled3)
};

// The original makes its Direct3D 9 device (D3D9Interface::InitD3D, 0x5ebbe2 (bejeweled3), flags 0x140) without
// D3DCREATE_FPU_PRESERVE, so while the 3D mode runs the x87 computes in single precision. The game sets this when its
// 3D mode starts; the curve arithmetic rounds to float where Bejeweled Twist's port found that it matters.
extern bool gCurveSinglePrecision;

}  // namespace Sexy
