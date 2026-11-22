// Sexy::Bezier (0x1c bytes, no vtable): a piecewise cubic Bezier curve over a list of points. Each point has a time
// (a float) and a position (two doubles), and each segment between two points has two control points. The particle
// effect's curves (PIValue, PIValue2D) use it to turn a time into a position (x is the time axis for a PIValue).
//
// Left out: the arc length (the original's SegmentArcLength 0x5a2120 (bejeweled3), SubdivideLength 0x5a2450 (bejeweled3) and the total length
// field), which Init computes and nothing in the effect reads.
#pragma once

#include <vector>

namespace Sexy {

struct PIPoint {   // 0x10 bytes
	double mX;
	double mY;
};

class PIBezier {
public:
	std::vector<float> mTimes;       // +0x00: the time of each point
	std::vector<PIPoint> mControls;  // +0x10: two control points for each segment
	std::vector<PIPoint> mPoints;    // +0x14: the points
	int mPointCount;                 // +0x0c: 0 until Init has run

	PIBezier() : mPointCount(0) {}

	// 0x5a19f0 (bejeweled3): takes the points, the segments' control points (two for each) and the times. Does
	// nothing when it has points already or when fewer than two are given
	void Init(const std::vector<PIPoint>& thePoints, const std::vector<PIPoint>& theControls,
	          const std::vector<float>& theTimes, int theCount);

	// 0x5a1d70 (bejeweled3): the position at a time; the first point's before the start, the last point's after
	// the end, (0, 0) without two points
	PIPoint Evaluate(float theTime) const;

	// 0x5a1f60 (bejeweled3): the position's speed at a time; (0, 0) outside the curve
	PIPoint Velocity(float theTime) const;

private:
	int FindSegment(float theTime) const;
};

}
