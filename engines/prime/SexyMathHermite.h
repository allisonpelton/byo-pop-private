// Sexy::SexyMathHermite (0x34 bytes): a Hermite spline through points that each carry a slope, built piece by piece on
// first use. CurvedVal keeps one in each curve's cache record for the curves whose string asks for slopes.
#pragma once

#include <vector>

namespace Sexy {

class SexyMathHermite {
public:
	struct SPoint {   // 0xc bytes
		float mX;
		float mFx;      // the value there
		float mFxPrime; // the slope there
	};
	struct SPiece {   // 0x10 bytes: the Newton coefficients on the nodes x0, x0, x1, x1
		float mCoefs[4];
	};

	std::vector<SPoint> mPoints;   // +0x0c in the original's layout
	std::vector<SPiece> mPieces;   // +0x24
	bool mIsBuilt = false;         // +0x30

	float Evaluate(float x);      // 0x436420 (bejeweled3)

private:
	bool BuildCurve();                                                          // 0x436760 (bejeweled3)
	static void CreatePiece(const SPoint* thePoints, SPiece* thePiece);         // 0x436530 (bejeweled3)
	static float EvaluatePiece(float x, const SPoint* thePoints, const SPiece& thePiece);  // 0x4366f0 (bejeweled3)
};

}  // namespace Sexy
