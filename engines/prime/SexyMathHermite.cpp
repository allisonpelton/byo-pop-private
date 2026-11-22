// Sexy::SexyMathHermite (SexyMathHermite.h). Each step the original stores is rounded to a float here.
#include "SexyMathHermite.h"

namespace Sexy {

// Newton's divided differences on the nodes x0, x0, x1, x1: the repeated nodes take the given slopes
void SexyMathHermite::CreatePiece(const SPoint* thePoints, SPiece* thePiece)
{
	float z[4] = {thePoints[0].mX, thePoints[0].mX, thePoints[1].mX, thePoints[1].mX};
	float aDifferences[4][4] = {};
	aDifferences[0][0] = thePoints[0].mFx;
	aDifferences[1][0] = thePoints[0].mFx;
	aDifferences[1][1] = thePoints[0].mFxPrime;
	aDifferences[2][0] = thePoints[1].mFx;
	aDifferences[2][1] = (float)(((double)thePoints[1].mFx - thePoints[0].mFx) / ((double)thePoints[1].mX - thePoints[0].mX));
	aDifferences[3][0] = thePoints[1].mFx;
	aDifferences[3][1] = thePoints[1].mFxPrime;
	for (int i = 2; i < 4; i++)
		for (int j = 2; j <= i; j++)
			aDifferences[i][j] = (float)(((double)aDifferences[i][j - 1] - aDifferences[i - 1][j - 1]) /
				((double)z[i] - z[i - j]));
	for (int k = 0; k < 4; k++)
		thePiece->mCoefs[k] = aDifferences[k][k];
}

bool SexyMathHermite::BuildCurve()
{
	mPieces.clear();
	if (mPoints.size() < 2)
		return false;
	size_t aPieces = mPoints.size() - 1;
	mPieces.assign(aPieces, SPiece{});
	for (size_t i = 0; i < aPieces; i++)
		CreatePiece(&mPoints[i], &mPieces[i]);
	return true;
}

// The Newton form c0 + c1 d0 + c2 d0^2 + c3 d0^2 d1, with d0 = x - x0 and d1 = x - x1; each partial sum and product is
// stored as a float (Bejeweled Twist's kept them on the x87)
float SexyMathHermite::EvaluatePiece(float x, const SPoint* thePoints, const SPiece& thePiece)
{
	float d0 = (float)((double)x - thePoints[0].mX);
	float d1 = (float)((double)x - thePoints[1].mX);
	float aSum = thePiece.mCoefs[0];
	aSum = (float)((double)d0 * thePiece.mCoefs[1] + aSum);
	float aSquare = (float)((double)d0 * d0);
	float aSquareD1 = (float)((double)aSquare * d1);
	double aCubic = (double)aSquareD1 * thePiece.mCoefs[3];
	aSum = (float)((double)aSquare * thePiece.mCoefs[2] + aSum);
	return (float)(aCubic + aSum);
}

float SexyMathHermite::Evaluate(float x)
{
	if (!mIsBuilt) {
		if (!BuildCurve())
			return 0.0f;
		mIsBuilt = true;
	}
	for (size_t i = 0; i < mPieces.size(); i++)
		if (x < mPoints[i + 1].mX)
			return EvaluatePiece(x, &mPoints[i], mPieces[i]);
	return mPoints.back().mFx;
}

}  // namespace Sexy
