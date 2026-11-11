// Sexy::Shockwave and Sexy::MatchShockwave (Shockwave.h).
#include "Shockwave.h"

#include "Board.h"
#include "Graphics.h"
#include "Image.h"
#include "Piece.h"
#include "Res.h"
#include "TriVertex.h"
#include "TwistD3D.h"
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
		if (thePiece->mBurstRow < 0)
			aY = thePiece->mY;
		else
			aY = (float)gBoard->GetRowY(thePiece->mBurstRow);
		mY = (float)(gGemHeight / 2) + aY;
	}
	mUpdates = 0;
	mAlpha = 1.0f;
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
	mRingSpeed = aStep;
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
	mAlpha = aReach <= 90.0f ? 0.75f : 0.5f;
	mTenUpdateCount = 0;
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
	if (thePiece->mMatchSerial == -1) {
		// the blast's chain is numbered with the gem's address
		thePiece->mMatchSerial = (int)(intptr_t)thePiece;
	}
	for (int aRow = 0; aRow < gBoard->mRows; aRow++) {
		for (aCol = aFirstCol; aCol <= aLastCol; aCol++) {
			Piece* aPiece = gBoard->GetPieceAt(aRow, aCol);
			if (aPiece == nullptr || aPiece->mBurstRow == -1)
				continue;
			int aCellX = gBoard->GetColX(aCol) + gGemWidth / 2;
			int aCellY = gBoard->GetRowY(aPiece->mBurstRow) + gGemHeight / 2;
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
			if (aReach < aDist || aPiece == thePiece || aPiece->mHasBroken || aPiece->mPendingKind != 0)
				continue;
			aPiece->mMatchSerial = thePiece->mMatchSerial;
			if (!aPiece->IsFlagSet(Piece::FLAG_POWER) || aPiece->IsFlagSet(Piece::FLAG_ELECTRO) || aPiece->mBlasted ||
				aPiece->mGlow != 0.0f) {
				if (!aPiece->IsFlagSet(Piece::FLAG_DROPPER) && !aPiece->IsFlagSet(Piece::FLAG_DEVIL)) {
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
				aPiece->mBusyTime = Ftol(aWait);
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
		mTenUpdateCount++;
	float aWait = (float)((double)mRingWait - 0.009999999776482582);
	mRingWait = aWait;
	if (aWait <= 0.0f) {
		if (mRingCount < 20) {
			mRings[mRingCount] = (float)((double)gGemWidth * 0.30000001192092896);
			mRingAlphas[mRingCount] = mAlpha;
			mRingCount++;
		}
		mRingWait = (float)((double)mDuration * 2.6600000858306885);
	}
	if (gBoard->mUpdateCnt % 4 == 0) {
		Spark* aSpark = new Spark();
		float anAngle = (float)((double)RandSigned() * 3.1415927410125732);
		float aCos = cosf(anAngle);
		aSpark->mDX = aCos * mRingSpeed;
		float aSin = sinf(anAngle);
		aSpark->mDY = aSin * mRingSpeed;
		aSpark->mX = (float)((double)mX + (double)gGemWidth * (double)aSpark->mDX * 0.25);
		aSpark->mY = (float)((double)gGemHeight * (double)aSpark->mDY * 0.25 + (double)mY);
		aSpark->mDelay = (float)((double)RandSigned() * 0.5 + 1.0);
		mSparks.push_back(aSpark);
	}
	int aRings = 0;
	for (int i = 0; i < 20; i++) {
		if (0.0f < mRingAlphas[i] && 0.0f < mRings[i]) {
			aRings++;
			float aRadius = (float)((double)mRingSpeed * 0.44999998807907104 + (double)mRings[i]);
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
		float aFade = (float)((double)mAlpha - 0.019999999552965164);
		mAlpha = aFade;
		if (aFade <= 0.0f) {
			mAlpha = 0;
			mDone = true;
		}
	} else {
		mDone = false;
		mAlpha = mMaxRadius <= 90.0f ? 0.5f : 0.24f;
	}
	if (aRings != 0)
		mDone = false;
}

void BlastShockwave::Draw(Graphics* g)
{
	g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
	g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
	g->SetColorizeImages(true);
	g->SetColor(Color(255, 255, 255, Ftol((double)mAlpha * 255.0)));
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

// 0x74b94e: the hint's wave. The arc runs from the gem to the target about a centre off to one side of the line.
FruitShockwave::FruitShockwave(Piece* thePiece, int theRange, int theTargetX, int theTargetY, int theHintRange)
	: Shockwave(thePiece, 0)
{
	mHintRange = theHintRange;
	mColorIndex = thePiece->mColor;
	mColor = gGemColors[mColorIndex];
	mRadius = 0;
	mMaxRadius = (float)(((double)(gGemWidth * theRange) + 0.0) + (double)(gGemWidth / 4));
	mX = (float)((double)(gGemWidth / 2) + (double)thePiece->mX);
	mY = (float)((double)(gGemWidth / 2) + (double)thePiece->mY);  // (the original halves the width for both)
	mColor2 = Color::Black;
	mColor2.mAlpha = 0;
	mRingGlow = 0;
	mIsStarted = false;
	mShape = 1;
	gApp->PlaySample(SOUND_BONUSLASER);
	mDuration = 1.5f;
	mRingWait = 0;
	mArcEnd = 0;
	mArcStart = 0;
	mRingStep = 1;
	mWidth = 150.0f;
	mEndX = (float)theTargetX;
	mEndY = (float)theTargetY;
	mStartX = mX;
	mStartY = mY;
	float aDX = mEndX - mStartX;
	float aDY = mEndY - mStartY;
	float aDist = sqrtf((float)((double)aDX * (double)aDX + (double)aDY * (double)aDY));
	float anAngle = (float)((double)(float)std::atan2((double)-aDY, (double)aDX) + 1.5707963705062866);  // 0x4c1b70
	mCenterX = (float)(((double)mEndX + (double)mStartX) * 0.5);
	mCenterY = (float)(((double)mEndY + (double)mStartY) * 0.5);
	float aCos = cosf(anAngle);  // 0x401b20
	mCenterX = (float)((double)aCos * (double)aDist * 2.5 + (double)mCenterX);  // rounded once, as the original
	float aSin = sinf(anAngle);  // 0x401b70
	mCenterY = (float)((double)mCenterY - (double)aSin * (double)aDist * 2.5);
	float aFromX = mStartX - mCenterX;
	float aFromY = -(float)((double)mStartY - (double)mCenterY);
	mAngleEnd = (float)std::atan2((double)aFromY, (double)aFromX);
	float aToX = mEndX - mCenterX;
	float aToY = mEndY - mCenterY;
	mAngleStart = (float)std::atan2((double)-aToY, (double)aToX);
	mArcRadius = sqrtf((float)((double)aToX * (double)aToX + (double)aToY * (double)aToY));
	mState = 1;
	mArcAlpha = 2.5f;
	mUnlockStep = 1;
	mUnlockWait = 0;
}

FruitShockwave::~FruitShockwave()
{
	// (the original frees the vector and leaves the particles in it)
	for (Particle* aParticle : mParticles)
		delete aParticle;
}

// 0x74c1aa
void FruitShockwave::Update()
{
	Shockwave::Update();
	mDone = false;
	if (mState < 2) {
		// a new ring now and then
		float aWait = (float)((double)mRingWait - 0.009999999776482582);
		mRingWait = aWait;
		if (aWait <= 0.0f) {
			int i = 0;
			do {
				if (mRings[i] == 0.0f) {
					mRings[i] = 1.0f;
					break;
				}
				i++;
			} while (i < 20);
			mRingWait = 0.15f;
		}
		if (mPiece != nullptr) {
			float aStep = (float)((double)mUnlockWait - 0.009999999776482582);
			mUnlockWait = aStep;
			if (aStep <= 0.0f) {
				int aCount = mUnlockStep;
				mUnlockWait = (float)((double)(float)aCount * 0.10000000149011612 + 0.25);
				if ((aCount < 1 || !mPiece->IsFlagSet(Piece::FLAG_BOMB)) && mPiece->IsFlagSet(Piece::FLAG_LOCKED) &&
					mUnlockStep == 3) {
					mPiece->BreakLock(false);
					mPiece->mLockState = 0;
					mPiece = nullptr;
				}
				mUnlockStep++;
			}
		}
		// sparks thrown out from the gem
		if (gBoard->mUpdateCnt % 4 == 0) {
			Particle* aParticle = new Particle();
			float anAngle = (float)((double)RandSigned() * 3.1415927410125732);
			aParticle->mDX = mRingSpeed * cosf(anAngle);
			aParticle->mDY = mRingSpeed * sinf(anAngle);
			aParticle->mX = (float)((double)gGemWidth * (double)aParticle->mDX * 0.25 + (double)mX);
			aParticle->mY = (float)((double)gGemHeight * (double)aParticle->mDY * 0.25 + (double)mY);
			aParticle->mWait = (float)(0.5 + (double)RandSigned() * 0.5);
			mParticles.push_back(aParticle);
		}
		// the arc grows
		if (mArcEnd < 1.0f) {
			float aGrown = (float)((double)mArcEnd + 0.014999999664723873);
			mArcEnd = aGrown;
			if (1.0f < aGrown)
				mArcEnd = 1.0f;
		}
		// and sends sparks along it
		if (0.20000000298023224 < (double)mArcEnd) {
			Particle* aParticle = new Particle();
			RandSigned();
			aParticle->mX = mEndX;
			aParticle->mY = mEndY;
			aParticle->mWait = 0;
			float aReach = (float)((double)gApp->mScale * 80.0);
			aParticle->mRadius = (float)((double)RandSigned() * (double)aReach + (double)mArcRadius);
			aParticle->mCenterX = mCenterX;
			aParticle->mCenterY = mCenterY;
			aParticle->mAngle = mAngleStart;
			aParticle->mAngleStep = (float)(((double)mAngleEnd - (double)mAngleStart) *
				((double)RandSigned() * 0.009999999776482582 + 0.014999999664723873));
			aParticle->mKind = 1;
			mParticles.push_back(aParticle);
		}
		if (gBoard->mUpdateCnt % 12 == 0 && 0.5f < mArcEnd) {
			// a light at the gem's side
			Board::Light aLight;
			float aReach = (float)((double)gGemWidth * 0.3499999940395355);
			float anAngle = (float)((double)RandSigned() * 3.1415927410125732);
			aLight.mX = cosf(anAngle) * aReach + mX;
			aLight.mY = sinf(anAngle) * aReach + mY;
			aLight.mSize = 8.0f;
			aLight.mFalloff = 28.0f;
			aLight.mLevel = 0;
			aLight.mStrength = 0.25f;
			aLight.mRising = true;
			aLight.mRiseRate = 0.1f;
			aLight.mFadeRate = 0.1f;
			aLight.mPiece = nullptr;
			aLight.mPieceId = -1;
			gBoard->mLights.push_back(aLight);  // 0x6b9b23
		}
	}
	if (mState == 1) {
		if (mArcStart <= 0.0f) {
			mArcStart = 0;
			float aLeft = (float)((double)mArcAlpha - 0.014999999664723873);
			mArcAlpha = aLeft;
			if (aLeft <= 0.0f)
				mState = 3;
		}
	} else if (mState == 2 || mState == 3) {
		if (mState == 2) {
			float aLeft = (float)((double)mArcEnd - 0.009999999776482582);
			mArcEnd = aLeft;
			if (aLeft <= 0.0f) {
				mArcEnd = 0;
				mState = 3;
			}
		}
		float aFade = (float)((double)mAlpha - 0.03999999910593033);
		if (aFade < 0.0f)
			aFade = 0.0f;
		mAlpha = aFade;
		if (aFade <= 0.0f) {
			mAlpha = 0;
			mDone = true;
		}
	}
	mRadius = mRadius <= mMaxRadius ? mRadius : mMaxRadius;
	for (int i = 0; i < 20; i++) {
		if (0.0f < mRings[i]) {
			float aRing = mRingSpeed + mRings[i];
			mRings[i] = aRing;
			if (mRadius < aRing)
				mRings[i] = 0;
		}
	}
	// (a spark that is done is taken out and the next one waits an update, as in the original)
	for (unsigned int i = 0; i < mParticles.size(); i++) {
		Particle* aParticle = mParticles[i];
		aParticle->mX = aParticle->mDX + aParticle->mX;
		aParticle->mY = aParticle->mDY + aParticle->mY;
		bool aGone;
		if (aParticle->mKind == 0) {
			if (0.0f < aParticle->mWait) {
				aParticle->mWait = (float)((double)aParticle->mWait - 0.009999999776482582);
			} else {
				aParticle->mGravity = 0.07f;
				aParticle->mDX = (float)((double)aParticle->mDX - 0.10000000149011612 * (double)aParticle->mDX);
				aParticle->mDY = (float)((double)aParticle->mDY + 0.07000000029802322);
			}
			float anAlpha = (float)((double)aParticle->mAlpha - 0.029999999329447746);
			aParticle->mAlpha = anAlpha;
			aGone = anAlpha <= 0.0f;
		} else if (aParticle->mKind == 1) {
			float anAngle = aParticle->mAngleStep + aParticle->mAngle;
			aParticle->mAngle = anAngle;
			float aT = (float)(((double)anAngle - (double)mAngleStart) / ((double)mAngleEnd - (double)mAngleStart));
			float aShrink = (float)(1.0 - (double)aT * 0.5);
			float aRadius = (float)(((1.0 - (double)aShrink) * (double)mArcRadius + (double)aParticle->mRadius * (double)aShrink) +
				(double)(float)((double)gApp->mScale * -36.0) * (1.0 - (double)aT));
			aParticle->mX = (float)((double)cosf(anAngle) * (double)aRadius + (double)aParticle->mCenterX);
			aParticle->mY = (float)((double)aParticle->mCenterY - (double)sinf(aParticle->mAngle) * (double)aRadius);
			aGone = aParticle->mAngle < mAngleEnd;
		} else {
			continue;
		}
		if (aGone) {
			delete aParticle;
			mParticles.erase(mParticles.begin() + i);
		}
	}
	if (!mParticles.empty())
		mDone = false;
	if (0 < mColor2.mAlpha || 0.0f < mArcAlpha) {
		int anAlpha = mColor2.mAlpha - 4;
		mColor2.mAlpha = anAlpha;
		if (anAlpha < 1) {
			mColor2.mAlpha = 0;
			if (mState != 3)
				mColor2 = mColor;
		} else {
			mDone = false;
		}
	}
	if (mState < 2) {
		float aGlow = (float)((double)mRingGlow + 0.019999999552965164);
		if (1.0f < aGlow)
			aGlow = 1.0f;
		mRingGlow = aGlow;
	} else {
		mRingGlow = mAlpha;
	}
}

// 0x749e12
void FruitShockwave::Draw(Graphics* g)
{
	g->SetColor(Color(mColor.mRed, mColor.mGreen, mColor.mBlue, Ftol((double)mAlpha * 128.0)));
	int aSize = Ftol((double)mRadius + (double)mRadius);
	g->DrawImage(IMAGE_FRUITBLAST_FILLCIRCLE, Ftol((double)mX - (double)mRadius), Ftol((double)mY - (double)mRadius),
		aSize, aSize);
	g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
	g->SetColorizeImages(true);
	g->SetColor(Color(255, 255, 255, Ftol((double)mAlpha * 32.0)));
	for (int i = 0; i < 20; i++) {
		if (0.0f < mRings[i]) {
			float aRing = mRings[i];
			aSize = Ftol((double)aRing + (double)aRing);
			g->DrawImage(IMAGE_FRUITBLAST_INNERRING, Ftol((double)mX - (double)aRing), Ftol((double)mY - (double)aRing),
				aSize, aSize);
		}
	}
	float anOuter = (float)((double)mRadius + 15.0);
	g->SetColor(Color(255, 255, 255, Ftol((double)mAlpha * 96.0)));
	aSize = Ftol((double)anOuter + (double)anOuter);
	g->DrawImage(IMAGE_FRUITBLAST_OUTERRING, Ftol((double)mX - (double)anOuter), Ftol((double)mY - (double)anOuter),
		aSize, aSize);
	if (-1 < mState && gApp->Is3DAccelerated()) {
		// Direct3D only (0x749f89): the arc as a ribbon of two triangles from the gem's end of it (mArcStart of the way to
		// the tube) to the far end (mArcEnd), a mWidth wide, drawn twice: the arc's picture tinted by the hint's colour,
		// then its highlights added
		gD3DInterface->SetSamplerState(0, GPUSAMP_ADDRESSV, 3);  // slot 0xc4: clamped
		float aDX = mStartX - mEndX;
		float aDY = mStartY - mEndY;
		float aFromX = mArcStart * aDX + mEndX;
		float aFromY = mEndY + aDY * mArcStart;
		float aToX = mArcEnd * aDX + mEndX;
		float aToY = aDY * mArcEnd + mEndY;
		float aSideX = -aDX;
		float aSideY = aDY;
		float aLengthSquared = aSideX * aSideX + aDY * aDY;
		float aLength = (float)std::sqrt((double)aLengthSquared);  // 0x4c0650
		if (aLength != 0.0f) {
			aSideY = aDY / aLength;
			aSideX = aSideX / aLength;
		}
		// the ribbon's four corners: the near end's two (0,1), (0,0) and the far end's (1,1), (1,0)
		TriVertex aCorners[4];
		aCorners[0] = TriVertex((float)((double)aFromX - (double)mWidth * (double)aSideY * 0.5),
			(float)((double)aFromY - (double)mWidth * (double)aSideX * 0.5), 0.0f, 1.0f);
		aCorners[1] = TriVertex((float)((double)mWidth * (double)aSideY * 0.5 + (double)aFromX),
			(float)((double)aFromY + (double)mWidth * (double)aSideX * 0.5), 0.0f, 0.0f);
		aCorners[2] = TriVertex((float)((double)aToX - (double)mWidth * (double)aSideY * 0.5),
			(float)((double)aToY - (double)mWidth * (double)aSideX * 0.35), 1.0f, 1.0f);
		aCorners[3] = TriVertex((float)((double)mWidth * (double)aSideY * 0.5 + (double)aToX),
			(float)((double)mWidth * (double)aSideX * 0.65 + (double)aToY), 1.0f, 0.0f);
		for (int aPass = 0; aPass < 2; aPass++) {
			Image* anArc = aPass == 0 ? IMAGE_FRUITBLAST_ARC : IMAGE_FRUITBLAST_ARC_HIGHLIGHTS;
			Color anArcColor = aPass == 0
				? Color(mColor.mRed, mColor.mGreen, mColor.mBlue, Ftol((double)mAlpha * 64.0))
				: Color(255, 255, 255, Ftol((double)mAlpha * 128.0));
			// slot 0x70 (0x6220b0): a triangle through the textured draw, the blend argument 1 as its tx
			TriVertex aFirst[1][3] = {{aCorners[0], aCorners[1], aCorners[3]}};
			gD3DInterface->DrawTrianglesTex(anArc, aFirst, 1, anArcColor, aPass, 1.0f, 0.0f, true);
			TriVertex aSecond[1][3] = {{aCorners[3], aCorners[2], aCorners[0]}};
			gD3DInterface->DrawTrianglesTex(anArc, aSecond, 1, anArcColor, aPass, 1.0f, 0.0f, true);
		}
	}
	g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
	g->SetColorizeImages(true);
	Image* aSparkImage = IMAGE_GEM_FRUIT_SPARK;
	int aWidth = Ftol((double)aSparkImage->mWidth * 0.5);
	int aHeight = Ftol(0.5 * (double)aSparkImage->mHeight);
	float aHalfHeight = (float)(aHeight / 2);
	float aHalfWidth = (float)(aWidth / 2);
	for (unsigned int i = 0; i < mParticles.size(); i++) {
		Particle* aParticle = mParticles[i];
		g->SetColor(Color(255, 255, 255, Ftol((double)aParticle->mAlpha * 96.0)));
		g->DrawImage(aSparkImage, Ftol((double)aParticle->mX - (double)aHalfWidth),
			Ftol((double)aParticle->mY - (double)aHalfHeight), aWidth, aHeight);
	}
}

// 0x74958d: on the 180th update the wave sets off a bomb it reaches
void FruitShockwave::OnGem(Piece* thePiece)
{
	if (mUpdates == 180) {
		float aDX = (float)((double)mX - ((double)thePiece->mX + (double)(float)(gGemWidth / 2)));
		float aDY = (float)((double)mY - ((double)thePiece->mY + (double)(float)(gGemWidth / 2)));
		if (thePiece->IsFlagSet(Piece::FLAG_BOMB) &&
			sqrtf((float)((double)aDX * (double)aDX + (double)aDY * (double)aDY)) < 1.0f)
			thePiece->mBusyTime = 1;
	}
}
