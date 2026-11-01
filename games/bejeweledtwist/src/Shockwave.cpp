// Sexy::Shockwave and Sexy::MatchShockwave (Shockwave.h).
#include "Shockwave.h"

#include "Board.h"
#include "Graphics.h"
#include "Image.h"
#include "Piece.h"
#include "Res.h"
#include "TwistUtil.h"
#include "WinTwistApp.h"
#include "X86Math.h"

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

void Shockwave::Update()
{
	mUpdates++;
	float aStep = (float)((double)mMaxRadius / ((double)mDuration / 0.009999999776482582));
	mUnknown14 = aStep;
	mRadius = aStep + mRadius;
	mTime = (float)((double)mTime + 0.009999999776482582);
	FindGems();
	if (mMaxRadius < mRadius && mDuration <= mTime)
		mDone = true;
}

void Shockwave::FindGems()
{
	float aReach = mRadius <= mMaxRadius ? mRadius : mMaxRadius;
	int aFirstCol = gBoard->GetColAt(Ftol((double)mX - (double)aReach));
	if (aFirstCol < 0)
		aFirstCol = 0;
	int aLastCol = gBoard->mCols - 1;
	int aCol = gBoard->GetColAt(Ftol((double)mX + (double)aReach));
	if (aCol <= aLastCol)
		aLastCol = aCol;
	if (aLastCol < 0)
		aLastCol = gBoard->mCols - 1;
	int aRow = gBoard->GetRowAt(Ftol((double)mY - (double)aReach));
	if (aRow < 0)
		aRow = 0;
	int aLastRow = gBoard->mRows - 1;
	int aBottom = gBoard->GetRowAt(Ftol((double)mY + (double)aReach));
	if (aBottom <= aLastRow)
		aLastRow = aBottom;
	if (aLastRow < 0)
		aLastRow = gBoard->mRows - 1;
	for (; aRow <= aLastRow; aRow++) {
		for (aCol = aFirstCol; aCol <= aLastCol; aCol++) {
			int aCellX = gBoard->GetColX(aCol) + gGemWidth / 2;
			int aCellY = gBoard->GetRowY(aRow) + gGemHeight / 2;
			float aDX = mX - (float)aCellX;
			float aDY = mY - (float)aCellY;
			float aDist;
			if (mShape == 1) {
				if (fabsf(aDY) < fabsf(aDX))
					aDY = aDX;
				aDist = fabsf(aDY);
			} else {
				aDist = sqrtf(aDX * aDX + aDY * aDY);
			}
			if (aDist <= aReach) {
				Piece* aPiece = gBoard->GetPieceAtXY(aCellX, aCellY);
				if (aPiece != nullptr && !aPiece->mBreaking && !aPiece->mBlasted)
					OnGem(aPiece);
			}
		}
	}
}

BlastShockwave::BlastShockwave(Piece* thePiece, int theRange, int theShape) : Shockwave(thePiece, theShape)
{
	mRadius = 0;
	mStarting = true;
	float aReach = (float)((double)(gGemWidth * theRange) + 0.0);
	mMaxRadius = aReach;
	mUnknown24 = aReach <= 90.0f ? 0.75f : 0.5f;
	mUnknown34 = 0;
	mDuration = (float)((double)(float)theRange * 0.05000000074505806 + 0.15000000596046448);
	mRingWait = 0;
	mRingCount = 0;
	mColor = gGemColors[thePiece->mColor];
	// the columns in reach (the original works out the rows too, and goes down every row)
	int aFirstCol = gBoard->GetColAt(Ftol((double)mX - (double)aReach));
	if (aFirstCol < 0)
		aFirstCol = 0;
	int aLastCol = gBoard->mCols - 1;
	int aCol = gBoard->GetColAt(Ftol((double)aReach + (double)mX));
	if (aCol <= aLastCol)
		aLastCol = aCol;
	if (aLastCol < 0)
		aLastCol = gBoard->mCols - 1;
	if (thePiece->mUnknown2EC == -1) {
		// the blast's chain is numbered with the gem's address
		thePiece->mUnknown2EC = (int)(intptr_t)thePiece;
	}
	for (int aRow = 0; aRow < gBoard->mRows; aRow++) {
		for (aCol = aFirstCol; aCol <= aLastCol; aCol++) {
			Piece* aPiece = gBoard->GetPieceAt(aRow, aCol);
			if (aPiece == nullptr || aPiece->mUnknown30 == -1)
				continue;
			int aCellX = gBoard->GetColX(aCol) + gGemWidth / 2;
			int aCellY = gBoard->GetRowY(aPiece->mUnknown30) + gGemHeight / 2;
			float aDX = mX - (float)aCellX;
			float aDY = mY - (float)aCellY;
			float aDist;
			if (mShape == 1) {
				if (fabsf(aDY) < fabsf(aDX))
					aDY = aDX;
				aDist = fabsf(aDY);
			} else {
				aDist = sqrtf(aDX * aDX + aDY * aDY);
			}
			if (aReach < aDist || aPiece == thePiece || aPiece->mUnknown51D || aPiece->mPendingKind != 0)
				continue;
			aPiece->mUnknown2EC = thePiece->mUnknown2EC;
			if (!aPiece->IsFlagSet(Piece::FLAG_POWER) || aPiece->IsFlagSet(Piece::FLAG_ELECTRO) || aPiece->mBlasted ||
				aPiece->mGlow != 0.0f) {
				if (!aPiece->IsFlagSet(Piece::FLAG_1) && !aPiece->IsFlagSet(Piece::FLAG_DEVIL)) {
					if (aPiece->mGlow == 0.0f)
						gBoard->BlowUpPiece(aPiece, mX, mY, true, true);
				} else {
					gBoard->BreakPiece(aPiece);
				}
			} else {
				// another power gem goes off in turn, later the further it is
				double aWait = (double)aDist * 20.0 / (double)gGemWidth;
				if (aWait < 1.0)
					aWait = 1.0;
				aPiece->mUnknown88 = Ftol(aWait);
				gBoard->LiftColumn(aPiece, 1.0f);
				gBoard->AddBlastChain();
			}
		}
	}
	mStarting = false;
}

BlastShockwave::~BlastShockwave()
{
	for (Spark* aSpark : mSparks)
		delete aSpark;
}

void BlastShockwave::Update()
{
	Shockwave::Update();
	if (mUpdates % 10 == 0)
		mUnknown34++;
	float aWait = (float)((double)mRingWait - 0.009999999776482582);
	mRingWait = aWait;
	if (aWait <= 0.0f) {
		if (mRingCount < 20) {
			mRings[mRingCount] = (float)((double)gGemWidth * 0.30000001192092896);
			mRingAlphas[mRingCount] = mUnknown24;
			mRingCount++;
		}
		mRingWait = (float)((double)mDuration * 2.6600000858306885);
	}
	if (gBoard->mUpdateCnt % 4 == 0) {
		Spark* aSpark = new Spark();
		float anAngle = (float)((double)RandSigned() * 3.1415927410125732);
		float aCos = cosf(anAngle);
		aSpark->mDX = aCos * mUnknown14;
		float aSin = sinf(anAngle);
		aSpark->mDY = aSin * mUnknown14;
		aSpark->mX = (float)((double)mX + (double)gGemWidth * (double)aSpark->mDX * 0.25);
		aSpark->mY = (float)((double)gGemHeight * (double)aSpark->mDY * 0.25 + (double)mY);
		aSpark->mDelay = (float)((double)RandSigned() * 0.5 + 1.0);
		mSparks.push_back(aSpark);
	}
	int aRings = 0;
	for (int i = 0; i < 20; i++) {
		if (0.0f < mRingAlphas[i] && 0.0f < mRings[i]) {
			aRings++;
			float aRadius = (float)((double)mUnknown14 * 0.44999998807907104 + (double)mRings[i]);
			mRings[i] = aRadius;
			if ((double)mMaxRadius * 1.0499999523162842 < (double)aRadius)
				mRingAlphas[i] = (float)((double)mRingAlphas[i] - (double)(float)(i + 2) * 0.004999999888241291);
		}
	}
	// (a spark that fades out is taken out and the next one waits an update, as in the original)
	for (unsigned int i = 0; i < mSparks.size(); i++) {
		Spark* aSpark = mSparks[i];
		aSpark->mX = aSpark->mDX + aSpark->mX;
		aSpark->mY = aSpark->mDY + aSpark->mY;
		if (aSpark->mDelay <= 0.0f) {
			aSpark->mGravity = 0.07f;
			aSpark->mDX = (float)((double)aSpark->mDX - (double)aSpark->mDX * 0.10000000149011612);
			aSpark->mDY = (float)((double)aSpark->mDY + 0.07000000029802322);
		} else {
			aSpark->mDelay = (float)((double)aSpark->mDelay - 0.009999999776482582);
		}
		float anAlpha = (float)((double)aSpark->mAlpha - 0.029999999329447746);
		aSpark->mAlpha = anAlpha;
		if (anAlpha <= 0.0f) {
			delete aSpark;
			mSparks.erase(mSparks.begin() + i);
		}
	}
	if (!mSparks.empty())
		mDone = false;
	if (mMaxRadius <= mRadius) {
		mRadius = mMaxRadius;
		float aFade = (float)((double)mUnknown24 - 0.019999999552965164);
		mUnknown24 = aFade;
		if (aFade <= 0.0f) {
			mUnknown24 = 0;
			mDone = true;
		}
	} else {
		mDone = false;
		mUnknown24 = mMaxRadius <= 90.0f ? 0.5f : 0.24f;
	}
	if (aRings != 0)
		mDone = false;
}

void BlastShockwave::Draw(Graphics* g)
{
	g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
	g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
	g->SetColorizeImages(true);
	g->SetColor(Color(255, 255, 255, Ftol((double)mUnknown24 * 255.0)));
	for (int i = 0; i < 20; i++) {
		if (0.0f < mRings[i] && 0.0f < mRingAlphas[i]) {
			float aRadius = mRings[i];
			g->SetColor(Color(100, 100, 60, Ftol((double)mRingAlphas[i] * 255.0)));
			int aSize = Ftol((double)aRadius + (double)aRadius);
			g->DrawImage(IMAGE_FRUITBLAST_INNERRING, Ftol((double)mX - (double)aRadius),
				Ftol((double)mY - (double)aRadius), aSize, aSize);
		}
	}
	g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
	g->SetColorizeImages(true);
	Image* aSparkImage = IMAGE_GEM_FRUIT_SPARK;
	int aWidth = Ftol((double)aSparkImage->mWidth * 0.5);
	int aHeight = Ftol(0.5 * (double)aSparkImage->mHeight);
	float aHalfHeight = (float)(aHeight / 2);
	float aHalfWidth = (float)(aWidth / 2);
	for (unsigned int i = 0; i < mSparks.size(); i++) {
		Spark* aSpark = mSparks[i];
		g->SetColor(Color(255, 255, 200, Ftol((double)aSpark->mAlpha * 96.0)));
		g->DrawImage(aSparkImage, Ftol((double)aSpark->mX - (double)aHalfWidth),
			Ftol((double)aSpark->mY - (double)aHalfHeight), aWidth, aHeight);
	}
}
