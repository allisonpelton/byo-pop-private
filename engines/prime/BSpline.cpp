// Sexy::BSpline (BSpline.h). Where the original stores to a float the port casts to float; what it keeps on the x87
// stays a double.
#include "BSpline.h"

#include "X86Math.h"

#include <cmath>
#include <utility>

namespace Sexy {

namespace {

// EquationSystem (0x38 bytes): n linear equations in n unknowns, a row of n coefficients and the constant term each,
// solved by Gaussian elimination with partial pivoting
class EquationSystem {
public:
	std::vector<float> mCoefficients;  // +0x00: mRowSize floats a row, the constant term last
	std::vector<float> mSolution;      // +0x18
	int mRowSize;                      // +0x30: the unknowns and the constant term
	int mCurRow;                       // +0x34: the row the caller is filling

	explicit EquationSystem(int theUnknowns)  // 0x59a5a0 (bejeweled3)
		: mCoefficients((size_t)theUnknowns * (theUnknowns + 1), 0.0f), mSolution((size_t)theUnknowns, 0.0f),
		  mRowSize(theUnknowns + 1), mCurRow(0)
	{
	}

	float& At(int theRow, int theCol) { return mCoefficients[(size_t)theRow * mRowSize + theCol]; }

	void SetCoefficient(int theRow, int theCol, float theValue) { At(theRow, theCol) = theValue; }  // 0x59a630 (bejeweled3)
	void SetConstantTerm(int theRow, float theValue) { At(theRow, mRowSize - 1) = theValue; }        // 0x59a660 (bejeweled3)

	// 0x59a690 (bejeweled3)
	void Solve()
	{
		int aRows = mRowSize - 1;
		for (int k = 0; k < aRows; k++) {
			int aPivot = k;
			for (int r = k + 1; r < aRows; r++)
				if (std::fabs(At(aPivot, k)) < std::fabs(At(r, k)))
					aPivot = r;
			for (int c = 0; c < mRowSize; c++)
				std::swap(At(aPivot, c), At(k, c));
			for (int r = k + 1; r < aRows; r++) {
				// the factor is stored as a float (Bejeweled Twist's kept it on the x87)
				float aFactor = (float)((double)At(r, k) / At(k, k));
				if (aFactor != 0.0f)
					for (int c = aRows; c >= k; c--)
						At(r, c) = (float)(At(r, c) - (double)At(k, c) * aFactor);
			}
		}
		for (int i = mRowSize - 2; i >= 0; i--) {
			float aSum = 0.0f;
			for (int j = i + 1; j < aRows; j++)
				aSum = (float)((double)mSolution[j] * At(i, j) + aSum);
			mSolution[i] = (float)(((double)At(i, aRows) - aSum) / At(i, i));
		}
	}
};

}  // namespace

void BSpline::AddPoint(float x, float y)
{
	mXPoints.push_back(x);
	mYPoints.push_back(y);
}

void BSpline::CalcArcLengths()
{
	mArcLengths.clear();
	int aCount = (int)mXPoints.size();
	for (int i = 0; i < aCount - 1; i++) {
		// the differences stay on the x87; their squares' sum is stored as a float
		double dx = (double)mXPoints[i + 1] - mXPoints[i];
		double dy = (double)mYPoints[i + 1] - mYPoints[i];
		float aSquare = (float)(dy * dy + dx * dx);
		mArcLengths.push_back((float)std::sqrt((double)aSquare));
	}
}

// Each segment i is a t^3 + b t^2 + c t + d over t in [0, 1], unknowns 4i to 4i+3. The rows: the start's slope is 0,
// then for each segment d = p(i), a + b + c = p(i+1) - p(i), the slope at its end equals the next segment's start
// slope (0 at the last one), and the curvature does too (not after the last one).
void BSpline::CalculateSplinePrv(const std::vector<float>& thePoints, std::vector<float>& theCoef)
{
	int aCount = (int)thePoints.size();
	if (aCount < 2)
		return;
	int aSegs = aCount - 1;
	EquationSystem aSystem(aSegs * 4);
	aSystem.SetCoefficient(aSystem.mCurRow, 2, 1.0f);
	aSystem.mCurRow++;
	int aColumn = 2;
	for (int i = 0; i < aSegs; i++, aColumn += 4) {
		aSystem.SetCoefficient(aSystem.mCurRow, aColumn + 1, 1.0f);
		aSystem.SetConstantTerm(aSystem.mCurRow, thePoints[i]);
		aSystem.mCurRow++;
		aSystem.SetCoefficient(aSystem.mCurRow, aColumn - 2, 1.0f);
		aSystem.SetCoefficient(aSystem.mCurRow, aColumn - 1, 1.0f);
		aSystem.SetCoefficient(aSystem.mCurRow, aColumn, 1.0f);
		aSystem.SetConstantTerm(aSystem.mCurRow, thePoints[i + 1] - thePoints[i]);
		aSystem.mCurRow++;
		aSystem.SetCoefficient(aSystem.mCurRow, aColumn - 2, 3.0f);
		aSystem.SetCoefficient(aSystem.mCurRow, aColumn - 1, 2.0f);
		aSystem.SetCoefficient(aSystem.mCurRow, aColumn, 1.0f);
		if (i < aCount - 2)
			aSystem.SetCoefficient(aSystem.mCurRow, aColumn + 4, -1.0f);
		aSystem.mCurRow++;
		if (i < aCount - 2) {
			aSystem.SetCoefficient(aSystem.mCurRow, aColumn - 2, 6.0f);
			aSystem.SetCoefficient(aSystem.mCurRow, aColumn - 1, 2.0f);
			aSystem.SetCoefficient(aSystem.mCurRow, aColumn + 3, -2.0f);
			aSystem.mCurRow++;
		}
	}
	aSystem.Solve();
	theCoef = aSystem.mSolution;
}

void BSpline::CalculateSpline()
{
	CalcArcLengths();
	CalculateSplinePrv(mXPoints, mXCoef);
	CalculateSplinePrv(mYPoints, mYCoef);
	CalcArcLengths();
}

float BSpline::GetPoint(float t, const std::vector<float>& theCoef) const
{
	int aCount = (int)mXPoints.size();
	int i = Ftol((float)std::floor((double)t));
	if (i < 0) {
		t = 0.0f;
		i = 0;
	} else if (i >= aCount - 1) {
		i = aCount - 2;
		t = (float)(aCount - 1);
	}
	// the powers are stored as floats and summed from the cube down (Bejeweled Twist kept them on the x87, linear first)
	float f = (float)((double)t - i);
	float aSquare = (float)((double)f * f);
	float aCube = (float)((double)aSquare * f);
	const float* k = &theCoef[(size_t)i * 4];
	double v = (double)aCube * k[0] + (double)aSquare * k[1];
	v = v + (double)f * k[2];
	return (float)(v + k[3]);
}

}  // namespace Sexy
