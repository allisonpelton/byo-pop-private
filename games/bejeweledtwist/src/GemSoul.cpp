// Sexy::GemSoul (GemSoul.h).
#include "GemSoul.h"

#include "Board.h"
#include "GameText.h"
#include "Graphics.h"
#include "Hyperspace.h"
#include "Image.h"
#include "Math3D.h"
#include "Piece.h"
#include "Res.h"
#include "SexyMatrix.h"
#include "TwistD3D.h"
#include "TwistUtil.h"
#include "WinTwistApp.h"
#include "X86Math.h"

#include <cmath>
#include <cstring>

using namespace Sexy;

GemSoul::GemSoul(int theType)
{
	mAngle = (float)((double)RandSigned() * 3.1415927410125732);
	float aSpeed = std::fabs(RandSigned());
	mAngleSpeed = (float)(((double)aSpeed * 0.001 + 0.002) * (double)6.2831854820251465f);
	mSwirlAngle = (float)((double)RandSigned() * 3.1415927410125732);
	mSwirlSpeed = (float)((double)RandSigned() * (double)6.2831854820251465f * 0.004999999888241291);
	mRadius = 0;
	mRectX = 0;
	mRectY = 0;
	mRectWidth = gBoard->mWidth;
	mRectHeight = gBoard->mHeight;
	mType = theType;
	if (theType == 1) {
		mOuter = IMAGE_ELEMENTALS_OUTER_FRUIT;
		mCore = IMAGE_ELEMENTALS_INNER_FRUIT;
		mInner = IMAGE_ELEMENTALS_INNER_FRUIT;
		mSheen = IMAGE_ELEMENTALS_SHEEN_FRUIT;
		mParticle = IMAGE_ELEMENTALS_PARTICLE_FRUIT;
		mBeamSmall = IMAGE_ELEMENTALS_BEAM_FRUIT_SML;
		mBeamMedium = IMAGE_ELEMENTALS_BEAM_FRUIT_MED;
		mBeamLarge = IMAGE_ELEMENTALS_BEAM_FRUIT_LRG;
	} else if (theType == 8 || theType == 0x18) {
		mOuter = IMAGE_ELEMENTALS_OUTER_LIGHTNING;
		mCore = IMAGE_ELEMENTALS_CORE_LIGHTNING;
		mInner = IMAGE_ELEMENTALS_INNER_LIGHTNING;
		mSheen = IMAGE_ELEMENTALS_SHEEN_LIGHTNING;
		mParticle = IMAGE_ELEMENTALS_PARTICLE_LIGHTNING;
		mBeamSmall = IMAGE_ELEMENTALS_BEAM_LIGHTNING_SML;
		mBeamMedium = IMAGE_ELEMENTALS_BEAM_LIGHTNING_MED;
		mBeamLarge = IMAGE_ELEMENTALS_BEAM_LIGHTNING_LRG;
	} else if (theType == 0x10) {
		mOuter = IMAGE_ELEMENTALS_OUTER_FIRE;
		mCore = IMAGE_ELEMENTALS_CORE_FIRE;
		mInner = IMAGE_ELEMENTALS_INNER_FIRE;
		mSheen = IMAGE_ELEMENTALS_SHEEN_FIRE;
		mParticle = IMAGE_ELEMENTALS_PARTICLE_FIRE;
		mBeamSmall = IMAGE_ELEMENTALS_BEAM_FIRE_SML;
		mBeamMedium = IMAGE_ELEMENTALS_BEAM_FIRE_MED;
		mBeamLarge = IMAGE_ELEMENTALS_BEAM_FIRE_LRG;
	} else if (theType == 0x40) {
		mOuter = IMAGE_ELEMENTALS_OUTER_ICE;
		mCore = IMAGE_ELEMENTALS_CORE_ICE;
		mInner = IMAGE_ELEMENTALS_INNER_ICE;
		mSheen = IMAGE_ELEMENTALS_SHEEN_ICE;
		mParticle = IMAGE_ELEMENTALS_PARTICLE_ICE;
		mBeamSmall = IMAGE_ELEMENTALS_BEAM_ICE_SML;
		mBeamMedium = IMAGE_ELEMENTALS_BEAM_ICE_MED;
		mBeamLarge = IMAGE_ELEMENTALS_BEAM_ICE_LRG;
	} else {
		// (only the outer image is cleared; the others stay unset)
		mOuter = nullptr;
		mType = 0;
	}
	mSpaceFade.SetCurve(GTS(0x7e32c8));
	mIndex = (int)gBoard->mLevelSouls.size();
}

GemSoul::~GemSoul()
{
	delete mPiece;
	mPiece = nullptr;
}

void GemSoul::SetTarget(Piece* thePiece)
{
	mHomePiece = thePiece;
	mState = 2;
	mTargetX = (float)(gBoard->GetColX(thePiece->mCol) + gGemWidth / 2);
	mTargetY = (float)(gBoard->GetRowY(mHomePiece->mRow) + gGemHeight / 2);
	if (gD3DInterface == nullptr) {
		mRetargetWait = 2.0f;
		mFlightClock.SetCurve(GTS(0x7e3318));
		mFlightClock.mIncRate = (double)RandSigned() * 0.001500000013038516 + mFlightClock.mIncRate;
		mReturnCurve.SetCurve(GTS(0x7e32f0), &mFlightClock);
	} else {
		// Direct3D starts it from a random point below the screen (x across the app's width, y from the screen's
		// bottom to 256 times the scale below it), with curves of its own
		mRetargetWait = 1.0f;
		mTurnBackX = Ftol((double)RandFloat() * (double)gApp->mWidth);
		int aBelow = Ftol((double)gApp->mScale * 256.0);
		mTurnBackY = Ftol((double)RandFloat() * (double)aBelow + (double)(gApp->mHeight + aBelow));
		mFlightClock.SetCurve(GTS(0x7e3360));
		float aDX = (float)mTurnBackX - mTargetX;
		float aDY = (float)mTurnBackY - mTargetY;
		float aDistance = std::sqrt(aDX * aDX + aDY * aDY);
		int aScaled = Ftol((double)gApp->mScale * 1000.0);
		mFlightClock.mIncRate = (double)aScaled / (double)aDistance * 0.012000000104308128 + 0.00800000037997961;
		mFlightClock.mIncRate = (double)RandSigned() * 0.004999999888241291 + mFlightClock.mIncRate;
		mReturnCurve.SetCurve(GTS(0x7e333c), &mFlightClock);
	}
	Update();
}

void GemSoul::Update()
{
	if (gBoard->mHyperspace != nullptr) {
		if (gBoard->mHyperspace->mJumpSkipped)
			return;
		mSpaceFade.IncInVal();
		// Its place in space (mSpacePos to mSpaceVel, only drawn in 3D mode, but worked out in 2D too) follows
		// the ship's place on the camera's path (the hyperspace's +0x14c0): a point behind the ship, one more behind
		// it for each soul before this one
		Vec3 aBehind{0.0f, -0.17000000178813934f, (float)(-12.0 - (double)mIndex * 2.0999999046325684)};
		Vec3 aPlace = TransformPoint(gBoard->mHyperspace->mShipPlace, aBehind);
		mSpaceGoal[0] = aPlace.x;
		mSpaceGoal[1] = aPlace.y;
		mSpaceGoal[2] = aPlace.z;
		if (mUpdateCnt == 0) {
			// A random start about it
			float aRandX = RandSigned();
			float aRandY = (float)((double)RandSigned() * 0.25);
			float aRandZ = RandSigned();
			mSpacePos[0] = (float)((double)mSpaceGoal[0] + (double)(float)((double)aRandX * 0.30000001192092896));
			mSpacePos[1] = (float)((double)mSpaceGoal[1] + (double)(float)((double)aRandY * 0.30000001192092896));
			mSpacePos[2] = (float)((double)mSpaceGoal[2] + (double)(float)((double)aRandZ * 0.30000001192092896));
		}
		// It drifts after its place: a velocity (7D4) of a 0.005 of the way there, damped, then moved by
		float aDiffX = (float)((double)mSpaceGoal[0] - (double)mSpacePos[0]);
		float aDiffY = (float)((double)mSpaceGoal[1] - (double)mSpacePos[1]);
		float aDiffZ = (float)((double)mSpaceGoal[2] - (double)mSpacePos[2]);
		mSpaceVel[0] = (float)((double)(float)((double)aDiffX * 0.004999999888241291) + (double)mSpaceVel[0]);
		mSpaceVel[1] = (float)((double)(float)((double)aDiffY * 0.004999999888241291) + (double)mSpaceVel[1]);
		mSpaceVel[2] = (float)((double)(float)((double)aDiffZ * 0.004999999888241291) + (double)mSpaceVel[2]);
		mSpaceVel[0] = (float)((double)0.9900000095367432f * (double)mSpaceVel[0]);
		mSpaceVel[1] = (float)((double)mSpaceVel[1] * (double)0.9900000095367432f);
		mSpaceVel[2] = (float)((double)0.9900000095367432f * (double)mSpaceVel[2]);
		mSpaceVel[2] = (float)((double)mSpaceVel[2] * 0.949999988079071);
		mSpacePos[2] = (float)(((double)mSpaceGoal[2] - (double)mSpacePos[2]) * 0.20000000298023224 +
			(double)mSpacePos[2]);
		mSpacePos[0] = (float)((double)mSpaceVel[0] + (double)mSpacePos[0]);
		mSpacePos[1] = (float)((double)mSpaceVel[1] + (double)mSpacePos[1]);
		mSpacePos[2] = (float)((double)mSpaceVel[2] + (double)mSpacePos[2]);
	}
	mUpdateCnt++;
	// Towards the target, no faster than twice the warm-up
	float aDX = mTargetX - mX;
	mVelX = (float)((double)aDX * 0.0005 + (double)mVelX);
	float aDY = mTargetY - mY;
	mVelY = (float)((double)aDY * 0.0005 + (double)mVelY);
	float aSpeedSq = (float)((double)mVelY * (double)mVelY + (double)mVelX * (double)mVelX);
	float aSpeed = std::sqrt(aSpeedSq);
	double aWarm = (double)mUpdateCnt * 0.004999999888241291;
	if (1.0 < aWarm || 1.0 == aWarm)
		aWarm = 1.0;
	float aLimit = (float)(aWarm + aWarm);
	const float aTwoPi = 6.2831854820251465f;
	mSwirlAngle = mSwirlSpeed + mSwirlAngle;
	if (aTwoPi < mSwirlAngle || aTwoPi == mSwirlAngle)
		mSwirlAngle = mSwirlAngle - aTwoPi;
	else if (!(-aTwoPi < mSwirlAngle))
		mSwirlAngle = mSwirlAngle + aTwoPi;
	mAngle = mAngleSpeed + mAngle;
	if (!(mAngle < aTwoPi))
		mAngle = mAngle - aTwoPi;
	else if (!(-aTwoPi < mAngle))
		mAngle = aTwoPi + mAngle;
	if (aLimit < aSpeed) {
		double aCut = (double)aLimit / (double)aSpeed;
		mVelX = (float)((double)mVelX * aCut);
		mVelY = (float)(aCut * (double)mVelY);
	}
	float aVelX = mVelX;
	float aVelY = mVelY;
	if (mPiece != nullptr) {
		mPiece->mX = (float)(mDrawX - gGemWidth / 2);
		mPiece->mY = (float)(mDrawY - gGemHeight / 2);
		mPiece->Update();
		if (mPiece->mBlasted) {
			delete mPiece;
			mPiece = nullptr;
		}
		if (mUpdateCnt == 80) {
			gApp->PlaySample(SOUND_SOULIFY);
			// (the original writes here even when the gem has just blasted)
			if (mPiece != nullptr)
				mPiece->mIsLive = false;
		}
		if (200 <= mUpdateCnt) {
			delete mPiece;
			mPiece = nullptr;
		}
	}
	// The sparks round it
	for (int i = 0; i < 100; i++) {
		Particle& aParticle = mParticles[i];
		if (aParticle.mSpeed == 0.0f) {
			aParticle.mX = Ftol((double)RandSigned() * (double)mOuter->mWidth * 0.25);
			aParticle.mY = Ftol((double)RandSigned() * (double)mOuter->mHeight * 0.25);
			aParticle.mT = 0;
			aParticle.mSpeed = (float)((double)std::fabs(RandSigned()) * 0.03999999910593033);
		} else {
			float aT = aParticle.mT + aParticle.mSpeed;
			aParticle.mT = aT;
			if (1.0f < aT || 1.0f == aT) {
				aParticle.mSpeed = -aParticle.mSpeed;
				aParticle.mT = 1.0f;
			} else if (!(0.0f < aT)) {
				aParticle.mT = 0;
				aParticle.mSpeed = 0;
			}
		}
	}
	// With Direct3D, from state 2 on the swirl is left to the 3D drawing
	if (mState < 2 || gD3DInterface == nullptr) {
		float aCos = std::cos(mAngle);
		int aX = Ftol((double)mRadius * (double)aCos + (double)mX);
		mTurnBackX = aX;
		mDrawX = aX;
		float aSin = std::sin(mAngle);
		int aY = Ftol((double)mRadius * (double)aSin + (double)mY);
		float aWait = (float)((double)mRetargetWait - 0.01);
		mTurnBackY = aY;
		mDrawY = aY;
		mRetargetWait = aWait;
		double aMaxRadius = (double)gBoard->mWidth * 0.30000001192092896;
		double aRadius = (double)mRadius + 0.5;
		if (!(aRadius < aMaxRadius || aRadius == aMaxRadius))
			aRadius = aMaxRadius;
		mRadius = (float)aRadius;
		if (mState < 2) {
			if (!(0.0f < aWait)) {
				// A new target in the area
				mRetargetWait = (float)((double)std::fabs(RandSigned()) * 0.5 + 0.30000001192092896);
				float aRandX = std::fabs(RandSigned());
				mTargetX = (float)((double)mRectWidth * (double)aRandX + (double)mRectX);
				float aRandY = std::fabs(RandSigned());
				mTargetY = (float)((double)mRectHeight * (double)aRandY + (double)mRectY);
			}
			double aGrow = (double)mReturn + 0.01;
			if (1.25 < aGrow)
				aGrow = 1.25;
			mReturn = (float)aGrow;
		}
		mAngleSpeed = (float)((double)aTwoPi * 7.5e-06 + (double)mAngleSpeed);
	}
	if (mState == 2) {
		// Back to its cell
		if (0.0f < mRetargetWait)
			mRetargetWait = (float)((double)mRetargetWait - 0.01);
		mTargetX = (float)(gBoard->GetColX(mHomePiece->mCol) + gGemWidth / 2);
		mTargetY = (float)(gBoard->GetRowY(mHomePiece->mRow) + gGemHeight / 2);
		int aY;
		if (0.0f < mRetargetWait || mFlightClock.IncInVal()) {
			double aT = mFlightClock.GetOutVal();
			double aT2 = mFlightClock.GetOutVal();
			mDrawX = Ftol((1.0 - aT) * (double)mTurnBackX + (double)mTargetX * aT2);
			aT = mFlightClock.GetOutVal();
			aT2 = mFlightClock.GetOutVal();
			aY = Ftol((1.0 - aT) * (double)mTurnBackY + (double)mTargetY * aT2);
		} else {
			mRadius = 0;
			mDrawX = Ftol((double)mTargetX);
			mX = (float)mDrawX;
			aY = Ftol((double)mTargetY);
			mIsHome = true;
			mY = (float)aY;
		}
		mDrawY = aY;
		mReturn = (float)mReturnCurve.GetOutVal();
	} else if (mState == 0) {
		// Coming out of the gem
		if (mPiece == nullptr || 80 < mUpdateCnt) {
			double aBright = (double)mBrightness + 0.05;
			if (1.0 < aBright)
				aBright = 1.0;
			mBrightness = (float)aBright;
			if (!(1.0f > mBrightness) && !(1.0f > mReturn))
				mState = 1;
		}
	}
	mX = aVelX + mX;
	mY = mY + aVelY;
	// The twelve beams (drawn by none of the 2D code)
	for (int i = 0; i < 12; i++) {
		float aT = mBeamRate[i] + mBeamPhase[i];
		mBeamPhase[i] = aT;
		if (!(1.0f > aT)) {
			mBeamRate[i] = 0;
			mBeamPhase[i] = 0;
		}
	}
	if (mUpdateCnt % 10 == 0) {
		int aBeam = (int)Rand() % 12;
		if (mBeamRate[aBeam] == 0.0f) {
			float aRate;
			float aFirst = std::fabs((float)((double)RandSigned() * 0.029999999329447746));
			if ((double)aFirst < 0.004999999888241291 || (double)aFirst == 0.004999999888241291)
				aRate = std::fabs((float)((double)RandSigned() * 0.029999999329447746));
			else
				aRate = 0.005f;
			mBeamRate[aBeam] = aRate;
			mBeamSize[aBeam] = (int)Rand() % 3;
		}
	}
}

// 0x7008ea: the test of a bit of mType (+0x650), inline here
void GemSoul::Draw(Graphics* g)
{
	if (!mIsVisible)
		return;
	if (mPiece != nullptr) {
		// The gem itself, shrinking away from 80 updates on
		float aScale;
		double aShrink = 1.0 - (double)(mUpdateCnt - 80) * 0.07;
		if (1.0 < aShrink || 1.0 == aShrink)
			aScale = 1.0f;
		else
			aScale = (float)aShrink;
		if (0.0f < aScale) {
			// With Direct3D the gem is drawn through a SexyTransform2D scaled about the soul; in 2D it takes the scale
			SexyTransform2D aMatrix;
			aMatrix.LoadIdentity();
			aMatrix.Translate((float)-mDrawX, (float)-mDrawY);
			aMatrix.Scale(aScale, aScale);
			aMatrix.Translate((float)mDrawX, (float)mDrawY);
			if (gD3DInterface == nullptr)
				mPiece->mDrawScale = aScale;
			else
				gD3DInterface->PushTransform(aMatrix, true);
			mPiece->Draw(g, true);
			mPiece->Draw(g, false);
			if (gD3DInterface != nullptr)
				gD3DInterface->PopTransform();
		}
	}
	// Direct3D draws the rest through a SexyTransform2D scaled by mBrightness about the soul
	if (gD3DInterface != nullptr) {
		SexyTransform2D aMatrix;
		aMatrix.LoadIdentity();
		aMatrix.Translate((float)-mDrawX, (float)-mDrawY);
		aMatrix.Scale(mBrightness, mBrightness);
		aMatrix.Translate((float)mDrawX, (float)mDrawY);
		gD3DInterface->PushTransform(aMatrix, true);
	}
	g->SetColorizeImages(true);
	// (the original picks a colour by the type here, and when mReturn is 1 works out twelve beams with it, drawing
	// none of them)
	g->SetColor(Color(255, 255, 255, Ftol((double)mBrightness * 255.0)));
	if (mType & (1 << 3)) {
		Image* aCrackle = IMAGE_GEM_LIGHTNING_CRACKLE;
		int aCels = aCrackle->mNumRows * aCrackle->mNumCols;
		Rect aCel = aCrackle->GetCelRect(mUpdateCnt / 5 % aCels);
		g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
		g->SetColorizeImages(true);
		int aDY = 0;
		if (mColor == 4)
			aDY = 12;
		else if (mColor == 6)
			aDY = -20;
		g->DrawImageRotated(aCrackle, mDrawX - aCel.mWidth / 2, mDrawY - aCel.mHeight / 2 + aDY, 0.0, aCel.mWidth / 2,
			aCel.mHeight / 2, &aCel);
	}
	if (mType & (1 << 4)) {
		Image* aSparklet = IMAGE_SPARKLET;
		g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
		for (int i = 0; i < 100; i++) {
			Particle& aParticle = mParticles[i];
			if (0.0f < aParticle.mT) {
				int aWidth = Ftol((double)aSparklet->mWidth * (double)mReturn * 1.5);
				int aHeight = Ftol((double)aSparklet->mHeight * (double)mReturn * 1.5);
				g->SetColor(Color(255, 200, 32, Ftol((double)aParticle.mT * 255.0 * (double)mBrightness)));
				int aY = Ftol((double)aParticle.mY * (double)mReturn + (double)mDrawY - (double)(aHeight / 2));
				int aX = Ftol((double)aParticle.mX * (double)mReturn + (double)mDrawX - (double)(aWidth / 2));
				g->DrawImage(aSparklet, aX, aY, aWidth, aHeight);
			}
		}
		g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
	}
	if (mType & (1 << 0)) {
		Image* aFruits = IMAGE_FRUITS;
		int aWidth = Ftol((double)aFruits->GetCelWidth() * (double)mReturn * 0.5);
		int aHeight = Ftol((double)aFruits->GetCelHeight() * (double)mReturn * 0.5);
		g->SetColor(Color(255, 255, 255, Ftol((double)mBrightness * 128.0)));
		g->DrawImageCel(aFruits, Rect(mDrawX - aWidth / 2, mDrawY - aHeight / 2, aWidth, aHeight), mColor);
	}
	g->SetColor(Color(255, 255, 255, Ftol((double)mBrightness * 255.0)));
	int aWidth = Ftol((double)mOuter->mWidth * (double)mReturn);
	int aHeight = Ftol((double)mOuter->mHeight * (double)mReturn);
	g->DrawImage(mOuter, mDrawX - aWidth / 2, mDrawY - aHeight / 2, aWidth, aHeight);
	aWidth = Ftol((double)mSheen->mWidth * (double)mReturn);
	aHeight = Ftol((double)mSheen->mHeight * (double)mReturn);
	g->DrawImage(mSheen, mDrawX - aWidth / 2, mDrawY - aHeight / 2, aWidth, aHeight);
	if (gD3DInterface != nullptr)
		gD3DInterface->PopTransform();
}
