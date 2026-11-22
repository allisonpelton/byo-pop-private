// Sexy::PIValue (0x5c bytes) and Sexy::PIValue2D (0x60 bytes): the particle effect's animated values. A PIValue is a
// number that follows a list of keyframes (time, value), either straight between them or along a Bezier curve; a
// PIValue2D is a position (x, y) over time the same way. Both remember the last time asked and the answer, so asking
// the same time again costs nothing.
#pragma once

#include "PIBezier.h"

#include <vector>

namespace Sexy {

class PIValue {
public:
	struct KeyFrame {   // 8 bytes
		float mTime;    // +0x00
		float mValue;   // +0x04
	};

	std::vector<float> mQuantized;    // +0x00: the curve sampled at 256 steps, made when first needed
	std::vector<KeyFrame> mKeys;      // +0x18
	PIBezier mBezier;                 // +0x30: the curve, when the keyframes carry control points
	float mLastTime;                  // +0x4c: the last time asked, -1 before the first
	float mLastValue;                 // +0x50: the answer to it
	float mLastParam;                 // +0x54: where the search along the Bezier curve ended last time
	float mParamStep;                 // +0x58: how far that search moved the time before

	PIValue();

	// 0x566360 (bejeweled3): the value at a time. theDefault is what a straight (no Bezier) value answers outside
	// its keyframes' span
	float GetValueAt(float theTime, float theDefault = 0.0f);

	// 0x566070 (bejeweled3)
	void QuantizeCurve();

	// 0x566810 (bejeweled3): the value of the last keyframe at or before theTime, 0 before the first
	float GetLastKeyValue(float theTime) const;
	// 0x566860 (bejeweled3): the time of that keyframe, 0 before the first
	float GetLastKeyTime(float theTime) const;
	// 0x5668b0 (bejeweled3): the time of the first keyframe after theTime, 0 when there is none
	float GetNextKeyTime(float theTime) const;
	// 0x566910 (bejeweled3): the index of that keyframe, -1 when there is none
	int GetNextKeyIndex(float theTime) const;
};

class PIValue2D {
public:
	struct KeyFrame {   // 0x18 bytes
		float mTime;    // +0x00
		double mX;      // +0x08
		double mY;      // +0x10
	};

	std::vector<KeyFrame> mKeys;      // +0x00
	PIBezier mBezier;                 // +0x18
	float mLastTime;                  // +0x34: the last time asked, -1 before the first
	double mLastX;                    // +0x38: the answer to it
	double mLastY;                    // +0x40
	float mLastVelocityTime;          // +0x48: the last time the velocity was asked
	double mLastVelocityX;            // +0x50
	double mLastVelocityY;            // +0x58

	PIValue2D();

	// 0x566b10 (bejeweled3)
	PIPoint GetValueAt(float theTime);
	// 0x566cc0 (bejeweled3)
	PIPoint GetVelocityAt(float theTime);
};

}
