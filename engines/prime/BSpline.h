// Sexy::BSpline (0x78 bytes): a curve through points, x and y each a cubic in the point index with a slope of zero at
// both ends, solved once by CalculateSpline. CurvedVal samples one to make a curve's table. The original keeps its
// intermediates on the x87 (53 bits in 2D mode) and rounds to float where it stores; the port computes in double and
// stores floats at the same places, which are not quite Bejeweled Twist's.
#pragma once

#include <vector>

namespace Sexy {

class BSpline {
public:
	std::vector<float> mXPoints;     // +0x00
	std::vector<float> mYPoints;     // +0x18
	std::vector<float> mArcLengths;  // +0x30: the straight distance from each point to the next
	std::vector<float> mXCoef;       // +0x48: four per segment, a t^3 + b t^2 + c t + d
	std::vector<float> mYCoef;       // +0x60

	void AddPoint(float x, float y);                                 // 0x59a400 (bejeweled3)
	void CalculateSpline();                                          // 0x59ad90 (bejeweled3)
	float GetXPoint(float thePosition) const { return GetPoint(thePosition, mXCoef); }   // 0x59aef0 (bejeweled3)
	float GetYPoint(float thePosition) const { return GetPoint(thePosition, mYCoef); }   // 0x59af10 (bejeweled3)

private:
	float GetPoint(float thePosition, const std::vector<float>& theCoef) const;          // 0x59adc0 (bejeweled3)
	void CalcArcLengths();                                                                  // 0x59a420 (bejeweled3)
	static void CalculateSplinePrv(const std::vector<float>& thePoints, std::vector<float>& theCoef);  // 0x59aae0 (bejeweled3)
};

}  // namespace Sexy
