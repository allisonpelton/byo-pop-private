// Sexy::BSpline (BSpline.h). The original's x87 code keeps intermediates in the FPU's precision (53 bits in the game's
// 2D mode) and rounds to float where it stores: the port computes in double and stores floats at the same places.
#include "BSpline.h"

#include "X86Math.h"

#include <cmath>

namespace Sexy {

namespace {

// The linear system of CalculateSplinePrv (0x6d7241-0x6d73ad): rows of mWidth floats, the last one the right-hand
// side, solved by Gaussian elimination with partial pivoting into mSolution.
struct Equations {
	int mWidth;
	std::vector<float> mCells;
	std::vector<float> mSolution;

	Equations(int theRows, int theWidth) : mWidth(theWidth), mCells((size_t)theRows * theWidth, 0.0f),
		mSolution((size_t)theRows, 0.0f) {}
	float& At(int theRow, int theCol) { return mCells[(size_t)theRow * mWidth + theCol]; }   // 0x6d7376
	void SetRhs(int theRow, float theValue) { mCells[(size_t)(theRow + 1) * mWidth - 1] = theValue; }  // 0x6d7392

	void Solve()  // 0x6d73ad
	{
		int aRows = mWidth - 1;
		for (int k = 0; k < aRows - 1; k++) {
			int aPivot = k;
			for (int r = k + 1; r < aRows; r++)
				if (std::fabs(At(aPivot, k)) < std::fabs(At(r, k)))
					aPivot = r;
			for (int c = 0; c < mWidth; c++)
				std::swap(At(aPivot, c), At(k, c));
			for (int r = k + 1; r < aRows; r++) {
				float aNum = At(r, k);
				float aDen = At(k, k);
				if (aNum / aDen != 0.0f)
					for (int c = aRows; c >= k; c--)
						At(r, c) = (float)(At(r, c) - (double)At(k, c) * ((double)aNum / aDen));
			}
		}
		for (int i = mWidth - 2; i >= 0; i--) {
			double aSum = 0.0;
			for (int j = i + 1; j < aRows; j++)
				aSum = (double)mSolution[j] * At(i, j) + (float)aSum;
			mSolution[i] = (float)(((double)At(i, aRows) - (float)aSum) / At(i, i));
		}
	}
};

}  // namespace

void BSpline::Reset()
{
	mXPoints.clear();
	mYPoints.clear();
	mArcLengths.clear();
	mXCoef.clear();
	mYCoef.clear();
}

void BSpline::AddPoint(float x, float y)
{
	mXPoints.push_back(x);
	mYPoints.push_back(y);
}

void BSpline::CalculateArcLengths()
{
	mArcLengths.clear();
	int aCount = (int)mXPoints.size();
	for (int i = 0; i < aCount - 1; i++) {
		float dx = mXPoints[i + 1] - mXPoints[i];
		float dy = mYPoints[i + 1] - mYPoints[i];
		float aSq = (float)((double)dx * dx + (double)dy * dy);
		mArcLengths.push_back(std::sqrt(aSq));
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
	Equations anEquations(aSegs * 4, aSegs * 4 + 1);
	int aRow = 0;
	anEquations.At(aRow, 2) = 1.0f;
	aRow++;
	int aColumn = 2;
	for (int i = 0; i < aSegs; i++, aColumn += 4) {
		anEquations.At(aRow, aColumn + 1) = 1.0f;
		anEquations.SetRhs(aRow, thePoints[i]);
		aRow++;
		anEquations.At(aRow, aColumn - 2) = 1.0f;
		anEquations.At(aRow, aColumn - 1) = 1.0f;
		anEquations.At(aRow, aColumn) = 1.0f;
		anEquations.SetRhs(aRow, thePoints[i + 1] - thePoints[i]);
		aRow++;
		anEquations.At(aRow, aColumn - 2) = 3.0f;
		anEquations.At(aRow, aColumn - 1) = 2.0f;
		anEquations.At(aRow, aColumn) = 1.0f;
		if (i < aCount - 2)
			anEquations.At(aRow, aColumn + 4) = -1.0f;
		aRow++;
		if (i < aCount - 2) {
			anEquations.At(aRow, aColumn - 2) = 6.0f;
			anEquations.At(aRow, aColumn - 1) = 2.0f;
			anEquations.At(aRow, aColumn + 3) = -2.0f;
			aRow++;
		}
	}
	anEquations.Solve();
	theCoef = anEquations.mSolution;
}

void BSpline::CalculateSplinePrvLinear(const std::vector<float>& thePoints, std::vector<float>& theCoef)
{
	int aCount = (int)thePoints.size();
	if (aCount < 2)
		return;
	int aSegs = aCount - 1;
	theCoef.assign((size_t)aSegs * 4, 0.0f);
	for (int i = 0; i < aSegs; i++) {
		float a = thePoints[i];
		float b = thePoints[i + 1];
		theCoef[i * 4] = 0.0f;
		theCoef[i * 4 + 1] = 0.0f;
		theCoef[i * 4 + 2] = b - a;
		theCoef[i * 4 + 3] = a;
	}
}

void BSpline::CalculateSpline(bool linear)
{
	CalculateArcLengths();
	if (!linear) {
		CalculateSplinePrv(mXPoints, mXCoef);
		CalculateSplinePrv(mYPoints, mYCoef);
	} else {
		CalculateSplinePrvLinear(mXPoints, mXCoef);
		CalculateSplinePrvLinear(mYPoints, mYCoef);
	}
	CalculateArcLengths();
}

float BSpline::GetPoint(float t, const std::vector<float>& theCoef) const
{
	int aCount = (int)mXPoints.size();
	int i = Ftol(std::floor(t));
	if (i < 0) {
		t = 0.0f;
		i = 0;
	} else if (i >= aCount - 1) {
		i = aCount - 2;
		t = (float)(aCount - 1);
	}
	float f = t - (float)i;
	const float* k = &theCoef[(size_t)i * 4];
	double v = (double)k[2] * f + (double)f * f * k[1] + (double)f * f * f * k[0] + k[3];
	return (float)v;
}

bool BSpline::GetNextPoint(float& x, float& y, float& t) const
{
	int i = Ftol(std::floor(t));
	if (i < 0 || i >= (int)mXPoints.size() - 1) {
		x = GetXPoint(t);
		y = GetYPoint(t);
		return false;
	}
	float aStep = (float)(1.0 / ((double)mArcLengths[i] * 100.0));
	float x0 = GetXPoint(t);
	float y0 = GetYPoint(t);
	float aT = t + aStep;
	float x1 = GetXPoint(aT);
	float y1 = GetYPoint(aT);
	for (;;) {
		float dy = y1 - y0;
		float dx = x1 - x0;
		if ((float)((double)dx * dx + (double)dy * dy) >= 1.0f)
			break;
		if ((float)(unsigned)((int)mXPoints.size() - 1) < aT)
			break;
		aT += aStep;
		x1 = GetXPoint(aT);
		y1 = GetYPoint(aT);
	}
	x = x1;
	y = y1;
	t = aT;
	return true;
}

}  // namespace Sexy
