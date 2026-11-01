// Sexy::Shockwave and Sexy::MatchShockwave (Shockwave.h).
#include "Shockwave.h"

#include "Board.h"
#include "Piece.h"

#include <cmath>

using namespace Sexy;

Shockwave::Shockwave(Piece* thePiece, int theShape)
{
	if (thePiece == nullptr) {
		mY = 0;
		mX = 0;
	} else {
		mX = (float)(gGemWidth / 2) + thePiece->mX;
		float aY;
		if (thePiece->mUnknown30 < 0)
			aY = thePiece->mY;
		else
			aY = (float)gBoard->GetRowY(thePiece->mUnknown30);
		mY = (float)(gGemHeight / 2) + aY;
	}
	mUpdates = 0;
	mUnknown24 = 1.0f;
	mDuration = 1.0f;
	mPiece = thePiece;
	mShape = theShape;
	mTime = 0;
	mDone = false;
}

MatchShockwave::MatchShockwave(int theX, int theY) : Shockwave(nullptr, 0)
{
	mDuration = 0.25f;
	mX = (float)theX;
	mY = (float)theY;
	mRadius = (float)gGemWidth * 0.25f;
	mMaxRadius = (float)gGemWidth * 0.75f;
	mStrength = 20.0f;
	mAngle = 0;
	mStep = 0.12566371f;
}

void MatchShockwave::SetStrength(float theCascade)
{
	if (5.0f < theCascade)
		theCascade = 5.0f;
	theCascade = theCascade * 4.0f;
	mStrength = theCascade;
	if (theCascade == 0.0f) {
		mDuration = 0;
	} else {
		theCascade = (float)((double)theCascade * 0.1);
		if (0.25f < theCascade)
			theCascade = 0.25f;
		mDuration = theCascade;
		mStep = (float)(3.1415927410125732 / ((double)theCascade * 100.0));
	}
}

void MatchShockwave::Update()
{
	float anAngle = mStep + mAngle;
	mAngle = anAngle;
	// (the original takes the angle's sine here and drops it)
	float aT = (float)((double)mAngle / 3.1415927410125732);
	float aRest = 1.0f - aT;
	float aPush = (float)((double)aT * 6.5 * (double)(float)std::pow((double)aRest, 2.0));  // 0x670a49
	if (!((double)mAngle <= 3.1415927410125732)) {
		mDone = true;
		return;
	}
	unsigned int i = 0;
	if (!mPushes.empty()) {
		do {
			Push aPushed = mPushes[i];
			Piece* aPiece = gBoard->GetPieceAt(aPushed.mRow, aPushed.mCol);
			if (aPiece != nullptr) {
				aPiece->mShakeX = (float)((double)mStrength * (double)aPush * (double)aPushed.mDX);
				aPiece->mShakeY = (float)((double)mStrength * (double)aPush * (double)aPushed.mDY);
			}
			i++;
		} while (i < mPushes.size());
	}
}
