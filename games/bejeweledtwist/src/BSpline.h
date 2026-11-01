// Sexy::BSpline (0x50 bytes; 0x6d7130-0x6d8aff): a curve through points, x and y each a cubic in the point index with
// a slope of zero at both ends (or straight lines between the points), solved once by CalculateSpline. CurvedVal
// samples one to make a curve's table. All arithmetic is in floats, stored after each step, as in the original.
#pragma once

#include <vector>

namespace Sexy {

class BSpline {
public:
	std::vector<float> mXPoints;   // +0x00
	std::vector<float> mYPoints;   // +0x10
	std::vector<float> mArcLengths;  // +0x20: the straight distance from each point to the next
	std::vector<float> mXCoef;     // +0x30: four per segment, a t^3 + b t^2 + c t + d
	std::vector<float> mYCoef;     // +0x40

	void Reset();                                       // 0x6d80b3's caller: drops everything
	void AddPoint(float x, float y);                    // 0x6d8093
	void CalculateSpline(bool linear = false);          // 0x6d867b
	float GetXPoint(float t) const { return GetPoint(t, mXCoef); }   // 0x6d76cb
	float GetYPoint(float t) const { return GetPoint(t, mYCoef); }   // 0x6d76e2
	// From t, the next point along the curve at least one unit away: x, y and the new t. False at the end.
	bool GetNextPoint(float& x, float& y, float& t) const;           // 0x6d7811

private:
	float GetPoint(float t, const std::vector<float>& theCoef) const;  // 0x6d75fc
	void CalculateArcLengths();                                       // 0x6d80b3
	static void CalculateSplinePrv(const std::vector<float>& thePoints, std::vector<float>& theCoef);        // 0x6d81a0
	static void CalculateSplinePrvLinear(const std::vector<float>& thePoints, std::vector<float>& theCoef);  // 0x6d85d4
};

}  // namespace Sexy
