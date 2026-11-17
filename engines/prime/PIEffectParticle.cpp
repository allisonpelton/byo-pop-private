// Sexy::PIEffect (PIEffect.h): the making of particles (UpdateParticleDef) and the matrix a particle is drawn with
// (CalcParticleTransform). The exe's x87 arithmetic is kept: sums and products are done in double and rounded to a
// float where the exe stores a float.
#include "PIEffect.h"

#include "Common.h"
#include "Image.h"
#include "PIEffectMath.h"
#include "X86Math.h"

#include <cmath>

namespace Sexy {

namespace {

const double kSmallestSize = 0.10000000149011612;   // 0x881110: a particle is never drawn smaller than this
const int kMissingImageCell = 64;               // port: the cell size when a texture has no image (see below)

}

// 0x56fe50 (bejeweled3)
void PIEffect::CalcParticleTransform(PILayer* theLayer, PIEmitterInstance* theInstance, PIEmitter* theEmitter,
                                     PIParticleDef* theParticleDef, PIParticleGroup* theGroup,
                                     PIParticleInstance* theParticle)
{
	PIEmitterInstanceDef* anInstanceDef = theInstance->mDef;
	float aLifeRatio = theParticle->mLifeRatio;
	float aRatioWidth = 1.0f;       // the longer side of the cell over its width
	float aRatioHeight = 1.0f;
	float aScaleX = 1.0f;           // how much of the cell the particle covers
	float aScaleY = 1.0f;
	int aCellWidth = 0;
	int aCellHeight = 0;

	if (theParticleDef != NULL) {
		PITexture& aTexture = mDef.mTextures[theParticleDef->mTextureIndex];
		// the exe's texture may also hold a list of images (the old file format, which this port does not read); the
		// file format read here has the one image
		Image* anImage = (Image*)aTexture.mImage;
		if (anImage != NULL) {
			Rect aCell = anImage->GetCelRect(theParticle->mFrame);
			aCellWidth = aCell.mWidth;
			aCellHeight = aCell.mHeight;
			if (aTexture.mHasMargin) {
				aCellWidth -= 2;
				aCellHeight -= 2;
			}
		} else {
			// port: a texture without an image (a test that loads without images) gets a square cell; the exe has
			// no such case and would fail here
			aCellWidth = kMissingImageCell;
			aCellHeight = kMissingImageCell;
		}

		if (theParticleDef->mSingleParticle) {
			PIValue& aSizeXScale = theGroup->mIsFreeEmitterParticles ? theEmitter->mSizeXScale : anInstanceDef->mSizeXScale;
			float aSizeXScaleValue = (float)aSizeXScale.GetValueAt(mFrameNum, 0.0f);
			double aSizeX = theParticleDef->mSizeX.GetValueAt(mFrameNum, 0.0f);
			theParticle->mSizeX = (float)((aSizeX + (double)theParticle->mSizeXVariation) * (double)aSizeXScaleValue);
			PIValue& aSizeYScale = theGroup->mIsFreeEmitterParticles ? theEmitter->mSizeYScale : anInstanceDef->mSizeYScale;
			float aSizeYScaleValue = (float)aSizeYScale.GetValueAt(mFrameNum, 0.0f);
			double aSizeY = theParticleDef->mSizeY.GetValueAt(mFrameNum, 0.0f);
			theParticle->mSizeY = (float)((aSizeY + (double)theParticle->mSizeYVariation) * (double)aSizeYScaleValue);
		}

		double aWidthOverLife = (double)theParticleDef->mSizeXOverLife.GetValueAt(aLifeRatio, 0.0f) * (double)theParticle->mSizeX;
		float aDrawnWidth = (float)(kSmallestSize < aWidthOverLife ? aWidthOverLife : kSmallestSize);
		double aHeightOverLife = (double)theParticleDef->mSizeYOverLife.GetValueAt(aLifeRatio, 0.0f) * (double)theParticle->mSizeY;
		float aDrawnHeight = (float)(kSmallestSize < aHeightOverLife ? aHeightOverLife : kSmallestSize);

		int aLongerSide = aCellWidth > aCellHeight ? aCellWidth : aCellHeight;
		float aLongerSideFloat = (float)aLongerSide;
		aRatioWidth = (float)((double)aLongerSideFloat / (double)aCellWidth);
		aRatioHeight = (float)((double)aLongerSideFloat / (double)aCellHeight);
		aScaleX = (float)(((double)aDrawnWidth / (double)aLongerSideFloat) * 2.0);
		aScaleY = (float)(2.0 * ((double)aDrawnHeight / (double)aLongerSideFloat));
	}

	// the emitter's own turn (and the free emitter's, for a particle one started)
	SexyMatrix3 anEmitterTurn;
	anEmitterTurn.LoadIdentity();
	float anInstanceRotation = anInstanceDef->mRotation.GetValueAt(mFrameNum, 0.0f);
	if (anInstanceRotation != 0.0f)
		PIMath::RotateDeg(anEmitterTurn, anInstanceRotation);
	PIFreeEmitterInstance* aParent = theParticle->mFreeEmitter;
	if (aParent != NULL && aParent->mAngle != 0.0f)
		PIMath::RotateRad(anEmitterTurn, -aParent->mAngle);

	SexyMatrix3 aMatrix;
	aMatrix.LoadIdentity();
	if (theParticleDef != NULL) {
		float aMoveY = (float)-((double)aRatioHeight * theParticleDef->mOriginY * (double)aCellHeight);
		float aMoveX = (float)-((double)aRatioWidth * theParticleDef->mOriginX * (double)aCellWidth);
		PIMath::Translate(aMatrix, aMoveX, aMoveY);
		if (theParticleDef->mFlipX)
			PIMath::Scale(aMatrix, -1.0f, 1.0f);
		if (theParticleDef->mFlipY)
			PIMath::Scale(aMatrix, 1.0f, -1.0f);
	}

	float aDrawArea = (float)((double)aScaleX * (double)aScaleY);
	if (aScaleX != 1.0f || aScaleY != 1.0f)
		PIMath::Scale(aMatrix, aScaleX, aScaleY);
	if (theParticle->mAngle != 0.0f)
		PIMath::RotateRad(aMatrix, theParticle->mAngle);

	if (theParticleDef != NULL) {
		if (theParticleDef->mAttachToEmitter) {
			double aTurn;
			if (aParent == NULL)
				aTurn = (double)PIMath::DegreesToRadians(anInstanceDef->mRotation.GetValueAt(mFrameNum, 0.0f));
			else
				aTurn = (double)aParent->mAngle - (double)theParticle->mEmitterRotation;
			float aAttachedTurn = (float)(aTurn * (double)theParticleDef->mAttachRatio);
			if (aAttachedTurn != 0.0f)
				PIMath::RotateRad(aMatrix, aAttachedTurn);
		}
		if (theParticleDef->mSingleParticle && (!theParticleDef->mAlignToEmitterAngle || theParticleDef->mAttachToEmitter))
			PIMath::RotateDeg(aMatrix, anInstanceDef->mRotation.GetValueAt(mFrameNum, 0.0f));
	}

	// where the particle is (an attached one follows the emitter's turn by the attach ratio)
	double aPositionX = theParticle->mPositionX;
	double aPositionY = theParticle->mPositionY;
	if (theParticleDef != NULL && theParticleDef->mAttachToEmitter) {
		SexyMatrix3 aRecordedTurn;
		aRecordedTurn.LoadIdentity();
		PIMath::RotateRad(aRecordedTurn, theParticle->mEmitterRotation);
		PIPoint aTurned = PIMath::TransformPoint(aRecordedTurn, aPositionX, aPositionY);
		PIPoint aCurrent = PIMath::TransformPoint(anEmitterTurn, aTurned.mX, aTurned.mY);
		double aRatio = (double)theParticleDef->mAttachRatio;
		double anInverse = 1.0 - (double)theParticleDef->mAttachRatio;
		double aBlendedX = aCurrent.mX * aRatio + aPositionX * anInverse;
		double aBlendedY = aCurrent.mY * aRatio + anInverse * aPositionY;
		aPositionX = aBlendedX;
		aPositionY = aBlendedY;
	}
	PIMath::Translate(aMatrix, (float)aPositionX, (float)aPositionY);

	if (theParticleDef != NULL && theParticleDef->mSingleParticle) {
		PIValue& aScaleSource = theGroup->mIsFreeEmitterParticles ? theEmitter->mScale : anInstanceDef->mScale;
		float aScaleValue = aScaleSource.GetValueAt(mFrameNum, 0.0f);
		double aEmitterScale = (double)theEmitter->mScale.GetValueAt(mFrameNum, 1.0f);
		theParticle->mScale = (float)(aEmitterScale * (double)aScaleValue);
	}
	aDrawArea = (float)(((double)theParticle->mScale * (double)theParticle->mScale) * (double)aDrawArea);
	if (theParticle->mScale != 1.0f)
		PIMath::Scale(aMatrix, theParticle->mScale, theParticle->mScale);

	// where the particle's own origin is
	double anOriginX = theParticle->mOriginX;
	double anOriginY = theParticle->mOriginY;
	if (theParticleDef != NULL) {
		if (theParticleDef->mSingleParticle) {
			PIPoint aSpawn = PIMath::TransformPoint(anEmitterTurn, theParticle->mSpawnOffsetX, theParticle->mSpawnOffsetY);
			PIPoint anEmitterPos = GetEmitterPos(theInstance, !theGroup->mIsFreeEmitterParticles);
			anOriginX = anEmitterPos.mX + aSpawn.mX;
			anOriginY = anEmitterPos.mY + aSpawn.mY;
		} else if (theParticleDef->mAttachToEmitter && !theGroup->mIsFreeEmitterList) {
			double aTargetX;
			double aTargetY;
			if (aParent == NULL) {
				PIPoint aSpawn = PIMath::TransformPoint(anEmitterTurn, theParticle->mSpawnOffsetX, theParticle->mSpawnOffsetY);
				PIPoint anEmitterPos = GetEmitterPos(theInstance, !theGroup->mIsFreeEmitterParticles);
				aTargetX = anEmitterPos.mX + aSpawn.mX;
				aTargetY = anEmitterPos.mY + aSpawn.mY;
			} else {
				aTargetX = (aParent->mAttachedX + aParent->mSpawnOffsetX) + aParent->mPositionX;
				aTargetY = (aParent->mSpawnOffsetY + aParent->mAttachedY) + aParent->mPositionY;
			}
			double aRatio = (double)theParticleDef->mAttachRatio;
			double anInverse = 1.0 - (double)theParticleDef->mAttachRatio;
			double aBlendedX = anInverse * anOriginX + aRatio * aTargetX;
			double aBlendedY = anInverse * anOriginY + aRatio * aTargetY;
			anOriginX = aBlendedX;
			anOriginY = aBlendedY;
		}
	}
	theParticle->mAttachedX = anOriginX;
	theParticle->mAttachedY = anOriginY;
	PIMath::Translate(aMatrix, (float)anOriginX, (float)anOriginY);

	// the layer's own place and turn
	PILayerDef* aLayerDef = theLayer->mDef;
	PIPoint aLayerPos = aLayerDef->mPosition.GetValueAt(mFrameNum);
	PIMath::Translate(aMatrix, (float)(aLayerPos.mX - aLayerDef->mOriginX), (float)(aLayerPos.mY - aLayerDef->mOriginY));
	float aLayerAngle = aLayerDef->mAngle.GetValueAt(mFrameNum, 0.0f);
	if (aLayerAngle != 0.0f)
		PIMath::RotateDeg(aMatrix, aLayerAngle);

	theParticle->mDrawArea = aDrawArea;
	theParticle->mTransform = aMatrix;
}

}
