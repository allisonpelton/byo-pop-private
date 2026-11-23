// Sexy::Effect, ParticleEffect, PopAnimEffect and EffectsManager (bejeweled3, 0x729460 to 0x732b90). See Effects.h.
#include "Effects.h"

#include "Bej3App.h"
#include "Board.h"
#include "Color.h"
#include "Common.h"
#include "Graphics.h"
#include "PIEffect.h"
#include "PopAnim.h"
#include "Res.h"
#include "SexyMatrix.h"
#include "X86Math.h"

#include <algorithm>

namespace Sexy {

namespace {
const float ART_RES_BASE = 1200.0f;
}

// the part of 0x732880 and 0x7325d0 that follows a piece (the same in both): the place is the piece's middle (its corner at the grid's
// plus 64 in each direction, in 1200ths) while the piece is there; when it is gone, 0x728c00 lets the effect run out and it follows
// no piece any more. port: the board's slide curves at +0x2980 (times 1260 for every layer) and +0x2a60 (for the +0x12c0 layer) are
// their rest value 0, and the piece's glow (+0x240, above 0 only in 3D) is not read
void Effect::FollowPiece()
{
	if (mPieceId == -1)
		return;
	Piece* aPiece = mManager != nullptr && mManager->mBoard != nullptr ? mManager->mBoard->FindPieceById(mPieceId) : nullptr;
	if (aPiece == nullptr) {
		Release();
		mPieceId = -1;
		return;
	}
	float aGridX = (float)mManager->mBoard->GetGridX() + aPiece->mX;
	mX = (float)((double)aGridX + 64.0);
	float aGridY = (float)mManager->mBoard->GetGridY() + aPiece->mY;
	mY = (float)((double)aGridY + 64.0);
}

// 0x729460 (bejeweled3), the case of types 5 and 6: the constructor draws 3 times from the app's generator, the first two for the colour's
// green and blue amounts (the exe puts them in a Color whose amounts its maker replaces, so the values are dropped here) and the third,
// by 0x712da0 (a draw taken as (Rand() % 1000000 - 500000) / 500000), for the turn, times pi
SparkEffect::SparkEffect() : Effect(TYPE_SPARK)
{
	Rand();   // & 0x1f, plus 0x1e
	Rand();   // the second amount
	mGravity = -0.0f;   // +0x38
	mScale = 0.75f;     // +0x58
	mScaleChange = 0.0049999998f;   // +0x5c: 0x3ba3d70a
	float aSignedUnit = (float)((double)(Rand() % 1000000 - 500000) / 500000.0);
	mAngle = (float)((double)aSignedUnit * 3.1415927410125732);   // +0x90
	mAngleChange = 0.0099999998f;   // +0x94: 0x3c23d70a
}

// the manager's case 5 of 0x7303f0 (bejeweled3): the scale and the turn go on by their changes. port: for a spark of black (the red and
// green amounts of its colour 0) the case also ages it faster; the time gems' colours are never black
void SparkEffect::KindUpdate()
{
	mScale = mScaleChange + mScale;
	mAngle = mAngleChange + mAngle;
}

// the manager's case 5 of 0x72ce60 (bejeweled3), at 0x72d9e5: the colour of the effect with the alpha of the effect (at most 1) times the
// layer's alpha times 200, additive, the picture drawn about its middle at the place (in pixels, the art size over 1200) at the scale
// and turn. port: the manager's own fade of its layer's alpha (the board's +0x12c8, 0 here) is left out
void SparkEffect::Draw(Graphics* g)
{
	if (mImage == nullptr)
		return;
	g->SetColorizeImages(true);
	float aAlpha = mAlpha > 1.0f ? 1.0f : mAlpha;
	aAlpha = aAlpha * mManager->mAlpha;
	g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
	g->SetColor(Color(mColor.mRed, mColor.mGreen, mColor.mBlue, Ftol((double)aAlpha * 200.0)));
	float anArtScale = (float)gApp->mArtRes / ART_RES_BASE;
	Transform aTransform;
	aTransform.Scale(mScale, mScale);
	aTransform.RotateRad(mAngle);
	g->DrawImageTransformF(mImage, aTransform, anArtScale * mX, anArtScale * mY);
	g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
}

ParticleEffect::ParticleEffect(const PIEffect* theResource) : Effect(TYPE_PARTICLE)
{
	mEffect = theResource->Duplicate();   // 0x567930
	mAlphaChange = 0.0f;                  // 0x72b180: param_1[0x14]
}

ParticleEffect::~ParticleEffect()
{
	delete mEffect;
}

// 0x728c00 (bejeweled3): the effect's own animation stops looping and is put on its last frame, so that what it has emitted fades out
void ParticleEffect::Release()
{
	mEffect->mLoop = false;
	if (mEffect->mFrameNum < (float)(mEffect->mLastFrameNum - 1))
		mEffect->mFrameNum = (float)(mEffect->mLastFrameNum - 1);
}

// slot 1, 0x732880 (bejeweled3). The matrix of the effect is the scale (+0x58) about the origin and then the place, all in 1200ths, and then
// the art size over 1200: put in the effect's draw transform when mPieceMatrix is set (+0x1c5), or else the art scale alone there and the
// first matrix, in 1200ths, in the transform of the emitters' places. The first update is ResetAnim and the others PIEffect::Update (the
// first of which draws one Rand() of the app for the effect's seed). The effect is done when nothing is left alive in it.
// port: the effect's two curves (+0xa0 and +0x110), which 0x437e00 steps and nothing sets, are left out
void ParticleEffect::Update()
{
	FollowPiece();
	float anArtScale = (float)gApp->mArtRes / ART_RES_BASE;
	if (mPieceMatrix) {
		mEffect->mDrawTransform.LoadIdentity();
		mEffect->mDrawTransform.m00 = anArtScale * mScale;
		mEffect->mDrawTransform.m11 = anArtScale * mScale;
		mEffect->mDrawTransform.m02 = anArtScale * mX;
		mEffect->mDrawTransform.m12 = anArtScale * mY;
	}
	else {
		mEffect->mDrawTransform.LoadIdentity();
		mEffect->mDrawTransform.m00 = anArtScale;
		mEffect->mDrawTransform.m11 = anArtScale;
		mEffect->mDrawTransform2.LoadIdentity();
		mEffect->mDrawTransform2.m00 = mScale;
		mEffect->mDrawTransform2.m11 = mScale;
		mEffect->mDrawTransform2.m02 = mX;
		mEffect->mDrawTransform2.m12 = mY;
	}
	if (mFirstUpdate) {
		mEffect->ResetAnim();
		mFirstUpdate = false;
	}
	else
		mEffect->Update();
	if (!mEffect->IsActive())
		mDone = true;
}

// slot 2, 0x72c460 (bejeweled3): white at the layer's alpha times the effect's, once the first update has been made
void ParticleEffect::Draw(Graphics* g)
{
	if (mFirstUpdate)
		return;
	mEffect->mColor = Color(255, 255, 255, Ftol((double)(float)((double)mManager->mAlpha * 255.0) * (double)mAlpha));
	mEffect->Draw(g);
}

PopAnimEffect::PopAnimEffect(const PopAnim* theResource) : Effect(TYPE_POPANIM)
{
	mAnim = theResource->Clone();   // 0x441370
	mAlphaChange = 0.0f;            // 0x72af30: param_1[0x14]
}

PopAnimEffect::~PopAnimEffect()
{
	delete mAnim;
}

// slot 3, 0x732b90 (bejeweled3): PopAnim::PlayLabel(label, true)
void PopAnimEffect::PlayLabel(const char* theLabel)
{
	mAnim->PlayLabel(theLabel, true);
}

// slot 1, 0x7325d0 (bejeweled3). The animation is drawn about its middle: its transform is the scale (the whole number of mScale, as the exe
// takes it) about the origin, the middle of the scaled size at the origin, the turn, and the middle of the scaled size and the place (the
// art size over 1200 times it) less it put back. The animation is updated, twice while +0x1c4 is set (1 from the constructor); it is done
// when it is no longer playing and has no effect of its own running
void PopAnimEffect::Update()
{
	int aScale = Ftol((double)mScale);
	int anArtRes = gApp->mArtRes;
	int aWidth = ((mAnim->mPamWidth * anArtRes) / 1200) * aScale;
	int aHeight = ((mAnim->mPamHeight * anArtRes) / 1200) * aScale;
	FollowPiece();
	float aHalfWidth = (float)(aWidth / 2);
	float aHalfHeight = (float)(aHeight / 2);
	float anArtScale = (float)anArtRes / ART_RES_BASE;
	// scale (aScale), translate (-half), turn (0 here), translate (+half), translate (place x art scale - half): the turn is mAngle, 0 for every
	// effect made so far, and the two halves cancel
	float aMoveX = anArtScale * mX - aHalfWidth;
	float aMoveY = anArtScale * mY - aHalfHeight;
	mAnim->mTransform.LoadIdentity();
	mAnim->mTransform.m00 = (float)aScale;
	mAnim->mTransform.m11 = (float)aScale;
	mAnim->mTransform.m02 = aMoveX;
	mAnim->mTransform.m12 = aMoveY;
	mAnim->mTransformsStale = true;
	mAnim->Update();
	if (mUpdateAnimTwice)
		mAnim->Update();
	if (!mAnim->IsActive())
		mDone = true;
}

// slot 2, 0x728a30 (bejeweled3): white at the layer's alpha, then the animation
void PopAnimEffect::Draw(Graphics* g)
{
	mAnim->SetAnimColor(Color(255, 255, 255, Ftol((double)mManager->mAlpha * 255.0)));
	mAnim->Draw(g);
}

EffectsManager::~EffectsManager()
{
	Clear();
}

// 0x417f10 (bejeweled3): the exe keeps the effects in a multimap whose key is the effect's type (+8); an effect goes after the ones of
// the same key and any lower one
void EffectsManager::Add(Effect* theEffect)
{
	std::vector<Effect*>::iterator aSlot = std::upper_bound(mEffects.begin(), mEffects.end(), theEffect,
		[](const Effect* theNew, const Effect* theOld) { return theNew->mType < theOld->mType; });
	mEffects.insert(aSlot, theEffect);
	theEffect->mManager = this;
}

// 0x729ca0 (bejeweled3): every effect goes
void EffectsManager::Clear()
{
	for (Effect* anEffect : mEffects)
		delete anEffect;
	mEffects.clear();
}

// slot 22, 0x7303f0 (bejeweled3): the manager's count goes up, and each effect is updated (twice when it asks for that), moved by its
// velocity over its step, its velocity damped (and the gravity added to the vertical one) and given its alpha and scale changes in each
// update whose count its step divides, then given its kind's own step (KindUpdate); an effect whose alpha or scale has run out is done.
// One that is done and held by no piece is deleted.
// port: it does nothing while the board's +0x12c8 is 1 (0 in Classic); the second list at +0xe0, the depth (+0x18, with its velocity and
// damping) and the cases of the switch over the types not made are left out; the effect's two curves (+0xa0, +0x110) that 0x437ef0
// tests are constants here
void EffectsManager::Update()
{
	mUpdateCount++;
	for (size_t anIndex = 0; anIndex < mEffects.size();) {
		Effect* anEffect = mEffects[anIndex];
		anEffect->Update();
		if (anEffect->mUpdateTwice)
			anEffect->Update();
		int aStep = anEffect->mStep == 0 ? 1 : anEffect->mStep;
		float aStepAsFloat = (float)aStep;
		anEffect->mX = anEffect->mX + anEffect->mVelocityX / aStepAsFloat;
		anEffect->mY = anEffect->mVelocityY / aStepAsFloat + anEffect->mY;
		if (mUpdateCount % aStep == 0) {
			anEffect->mVelocityX = anEffect->mDampingX * anEffect->mVelocityX;
			anEffect->mVelocityY = (anEffect->mGravity + anEffect->mVelocityY) * anEffect->mDampingY;
			if ((anEffect->mFlags & 4) == 0) {
				anEffect->mAlpha = anEffect->mAlphaChange + anEffect->mAlpha;
				anEffect->mScale = anEffect->mScaleChange + anEffect->mScale;
			}
		}
		anEffect->KindUpdate();
		if (anEffect->mScale >= anEffect->mScaleMin) {
			if (anEffect->mScaleMax < anEffect->mScale)
				anEffect->mScale = anEffect->mScaleMax;
		}
		else {
			anEffect->mDone = true;
			anEffect->mScale = anEffect->mScaleMin;
		}
		if (anEffect->mAlpha <= anEffect->mAlphaMax) {
			if (anEffect->mAlpha <= 0.0f)
				anEffect->mDone = true;
		}
		else {
			anEffect->mAlpha = anEffect->mAlphaMax;
			anEffect->mDone = true;   // 0x730cd1: the bit 2 of the flags (+0x1ac) would turn the change round instead
		}
		if (!anEffect->mDone || anEffect->mUsers != 0)
			anIndex++;
		else {
			delete anEffect;
			mEffects.erase(mEffects.begin() + anIndex);
		}
	}
}

// 0x72ce60 (bejeweled3), for one effect: one that follows a piece (a type other than 1, 0x15, 0x16 and 0x17) is drawn with its place
// moved to the piece's corner (the grid's place and the piece's) for the draw, and put back after; while the piece is gone it is not drawn.
// The sums are the exe's, in floats, so the put-back leaves the place as the exe leaves it. port: the board's slide curve at +0x2a60 (for
// the +0x12c0 layer) is its rest value 0
void EffectsManager::DrawEffect(Effect* theEffect, Graphics* g)
{
	int aType = theEffect->mType;
	bool aFollows = theEffect->mPieceId != -1 && aType != 1 && aType != Effect::TYPE_TIME_BONUS && aType != Effect::TYPE_PARTICLE && aType != Effect::TYPE_POPANIM;
	if (!aFollows) {
		theEffect->Draw(g);
		return;
	}
	Piece* aPiece = mBoard != nullptr ? mBoard->FindPieceById(theEffect->mPieceId) : nullptr;
	if (aPiece == nullptr)
		return;
	float aCornerX = (float)mBoard->GetGridX() + aPiece->mX;
	float aCornerY = (float)mBoard->GetGridY() + aPiece->mY;
	theEffect->mX = aCornerX + theEffect->mX;
	theEffect->mY = aCornerY + theEffect->mY;
	theEffect->Draw(g);
	theEffect->mX = theEffect->mX - aCornerX;
	theEffect->mY = theEffect->mY - aCornerY;
}

// slot 26 (0x7303c0) and slot 44 (0x72e3c0), both 0x72ce60 (bejeweled3): the effects of the manager's pass that are not done, in order. An
// effect that follows a piece is drawn while the piece's alpha curve (+0x90, the constant 1) is above 0 or the piece is gone.
// port: the overlay pass's own work before it (0x72c4f0 and 0x72a460, about the list at +0xf8) and its test of the board's +0x2978
// and +0x1412, the 3D flag and the kinds' other cases are left out
void EffectsManager::Draw(Graphics* g, bool theOverlay)
{
	if (mAlpha == 0.0f)
		return;
	if (theOverlay && mBoard != nullptr && mBoard->mLeaving)
		return;
	for (size_t anIndex = 0; anIndex < mEffects.size(); anIndex++) {
		Effect* anEffect = mEffects[anIndex];
		if (anEffect->mOverlay != theOverlay || anEffect->mDone)
			continue;
		DrawEffect(anEffect, g);
	}
}

}  // namespace Sexy
