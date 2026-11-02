// Sexy::GemSoul (GemSoul.h).
#include "GemSoul.h"

#include "Board.h"
#include "GameText.h"
#include "Graphics.h"
#include "Hyperspace.h"
#include "Image.h"
#include "Piece.h"
#include "Res.h"
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
	mUnknown8E8 = (float)((double)RandSigned() * 3.1415927410125732);
	mUnknown8EC = (float)((double)RandSigned() * (double)6.2831854820251465f * 0.004999999888241291);
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
	mUnknown748.SetCurve(GTS(0x7e32c8));
	mIndex = (int)gBoard->mUnknown26B0.size();
}

GemSoul::~GemSoul()
{
	delete mPiece;
	mPiece = nullptr;
}

void GemSoul::SetTarget(Piece* thePiece)
{
	mUnknown680 = thePiece;
	mState = 2;
	mTargetX = (float)(gBoard->GetColX(thePiece->mCol) + gGemWidth / 2);
	mTargetY = (float)(gBoard->GetRowY(mUnknown680->mRow) + gGemHeight / 2);
	// (Direct3D starts it from a random point below the screen, with curves of its own)
	mUnknown660 = 2.0f;
	mUnknown7E0.SetCurve(GTS(0x7e3318));
	mUnknown7E0.mIncRate = (double)RandSigned() * 0.001500000013038516 + mUnknown7E0.mIncRate;
	mUnknown858.SetCurve(GTS(0x7e32f0), &mUnknown7E0);
	Update();
}

void GemSoul::Update()
{
	if (gBoard->mHyperspace != nullptr) {
		if (gBoard->mHyperspace->mFlag15C)
			return;
		mUnknown748.IncInVal();
		// (Direct3D's place in space, mUnknown7BC to mUnknown7D4, follows the hyperspace's camera; the port keeps only
		// the draws of its first update)
		if (mUpdateCnt == 0) {
			RandSigned();
			RandSigned();
			RandSigned();
		}
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
	mUnknown8E8 = mUnknown8EC + mUnknown8E8;
	if (aTwoPi < mUnknown8E8 || aTwoPi == mUnknown8E8)
		mUnknown8E8 = mUnknown8E8 - aTwoPi;
	else if (!(-aTwoPi < mUnknown8E8))
		mUnknown8E8 = mUnknown8E8 + aTwoPi;
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
				mPiece->mUnknown535 = false;
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
	// (with Direct3D, from state 2 on the swirl is left to the 3D drawing)
	{
		float aCos = std::cos(mAngle);
		int aX = Ftol((double)mRadius * (double)aCos + (double)mX);
		mUnknown850 = aX;
		mDrawX = aX;
		float aSin = std::sin(mAngle);
		int aY = Ftol((double)mRadius * (double)aSin + (double)mY);
		float aWait = (float)((double)mUnknown660 - 0.01);
		mUnknown854 = aY;
		mDrawY = aY;
		mUnknown660 = aWait;
		double aMaxRadius = (double)gBoard->mWidth * 0.30000001192092896;
		double aRadius = (double)mRadius + 0.5;
		if (!(aRadius < aMaxRadius || aRadius == aMaxRadius))
			aRadius = aMaxRadius;
		mRadius = (float)aRadius;
		if (mState < 2) {
			if (!(0.0f < aWait)) {
				// A new target in the area
				mUnknown660 = (float)((double)std::fabs(RandSigned()) * 0.5 + 0.30000001192092896);
				float aRandX = std::fabs(RandSigned());
				mTargetX = (float)((double)mRectWidth * (double)aRandX + (double)mRectX);
				float aRandY = std::fabs(RandSigned());
				mTargetY = (float)((double)mRectHeight * (double)aRandY + (double)mRectY);
			}
			double aGrow = (double)mUnknown740 + 0.01;
			if (1.25 < aGrow)
				aGrow = 1.25;
			mUnknown740 = (float)aGrow;
		}
		mAngleSpeed = (float)((double)aTwoPi * 7.5e-06 + (double)mAngleSpeed);
	}
	if (mState == 2) {
		// Back to its cell
		if (0.0f < mUnknown660)
			mUnknown660 = (float)((double)mUnknown660 - 0.01);
		mTargetX = (float)(gBoard->GetColX(mUnknown680->mCol) + gGemWidth / 2);
		mTargetY = (float)(gBoard->GetRowY(mUnknown680->mRow) + gGemHeight / 2);
		int aY;
		if (0.0f < mUnknown660 || mUnknown7E0.IncInVal()) {
			double aT = mUnknown7E0.GetOutVal();
			double aT2 = mUnknown7E0.GetOutVal();
			mDrawX = Ftol((1.0 - aT) * (double)mUnknown850 + (double)mTargetX * aT2);
			aT = mUnknown7E0.GetOutVal();
			aT2 = mUnknown7E0.GetOutVal();
			aY = Ftol((1.0 - aT) * (double)mUnknown854 + (double)mTargetY * aT2);
		} else {
			mRadius = 0;
			mDrawX = Ftol((double)mTargetX);
			mX = (float)mDrawX;
			aY = Ftol((double)mTargetY);
			mUnknown73C = true;
			mY = (float)aY;
		}
		mDrawY = aY;
		mUnknown740 = (float)mUnknown858.GetOutVal();
	} else if (mState == 0) {
		// Coming out of the gem
		if (mPiece == nullptr || 80 < mUpdateCnt) {
			double aBright = (double)mUnknown684 + 0.05;
			if (1.0 < aBright)
				aBright = 1.0;
			mUnknown684 = (float)aBright;
			if (!(1.0f > mUnknown684) && !(1.0f > mUnknown740))
				mState = 1;
		}
	}
	mX = aVelX + mX;
	mY = mY + aVelY;
	// The twelve beams (drawn by none of the 2D code)
	for (int i = 0; i < 12; i++) {
		float aT = mUnknown6D8[i] + mUnknown6A8[i];
		mUnknown6A8[i] = aT;
		if (!(1.0f > aT)) {
			mUnknown6D8[i] = 0;
			mUnknown6A8[i] = 0;
		}
	}
	if (mUpdateCnt % 10 == 0) {
		int aBeam = (int)Rand() % 12;
		if (mUnknown6D8[aBeam] == 0.0f) {
			float aRate;
			float aFirst = std::fabs((float)((double)RandSigned() * 0.029999999329447746));
			if ((double)aFirst < 0.004999999888241291 || (double)aFirst == 0.004999999888241291)
				aRate = std::fabs((float)((double)RandSigned() * 0.029999999329447746));
			else
				aRate = 0.005f;
			mUnknown6D8[aBeam] = aRate;
			mUnknown708[aBeam] = (int)Rand() % 3;
		}
	}
}

void GemSoul::Draw(Graphics* g)
{
	if (!mUnknown8F4)
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
			// (with Direct3D the gem is drawn through a SexyTransform2D scaled about the soul; in 2D it takes the scale)
			mPiece->mDrawScale = aScale;
			mPiece->Draw(g, true);
			mPiece->Draw(g, false);
		}
	}
	// (Direct3D draws the rest through a SexyTransform2D scaled by mUnknown684 about the soul)
	g->SetColorizeImages(true);
	// (the original picks a colour by the type here, and when mUnknown740 is 1 works out twelve beams with it, drawing
	// none of them)
	g->SetColor(Color(255, 255, 255, Ftol((double)mUnknown684 * 255.0)));
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
				int aWidth = Ftol((double)aSparklet->mWidth * (double)mUnknown740 * 1.5);
				int aHeight = Ftol((double)aSparklet->mHeight * (double)mUnknown740 * 1.5);
				g->SetColor(Color(255, 200, 32, Ftol((double)aParticle.mT * 255.0 * (double)mUnknown684)));
				int aY = Ftol((double)aParticle.mY * (double)mUnknown740 + (double)mDrawY - (double)(aHeight / 2));
				int aX = Ftol((double)aParticle.mX * (double)mUnknown740 + (double)mDrawX - (double)(aWidth / 2));
				g->DrawImage(aSparklet, aX, aY, aWidth, aHeight);
			}
		}
		g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
	}
	if (mType & (1 << 0)) {
		Image* aFruits = IMAGE_FRUITS;
		int aWidth = Ftol((double)aFruits->GetCelWidth() * (double)mUnknown740 * 0.5);
		int aHeight = Ftol((double)aFruits->GetCelHeight() * (double)mUnknown740 * 0.5);
		g->SetColor(Color(255, 255, 255, Ftol((double)mUnknown684 * 128.0)));
		g->DrawImageCel(aFruits, Rect(mDrawX - aWidth / 2, mDrawY - aHeight / 2, aWidth, aHeight), mColor);
	}
	g->SetColor(Color(255, 255, 255, Ftol((double)mUnknown684 * 255.0)));
	int aWidth = Ftol((double)mOuter->mWidth * (double)mUnknown740);
	int aHeight = Ftol((double)mOuter->mHeight * (double)mUnknown740);
	g->DrawImage(mOuter, mDrawX - aWidth / 2, mDrawY - aHeight / 2, aWidth, aHeight);
	aWidth = Ftol((double)mSheen->mWidth * (double)mUnknown740);
	aHeight = Ftol((double)mSheen->mHeight * (double)mUnknown740);
	g->DrawImage(mSheen, mDrawX - aWidth / 2, mDrawY - aHeight / 2, aWidth, aHeight);
}
