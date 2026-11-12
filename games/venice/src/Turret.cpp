// The turret (0x409d70, 0x402420, 0x402570).

#include "Turret.h"

#include "ImageRecord.h"
#include "Jewel.h"
#include "Level.h"
#include "VeniceGlobals.h"
#include "VeniceMath.h"
#include "VeniceSound.h"

Turret::Turret()
{
	mType = ObjectTypeTurret;
	mZOrder = 1.0f;
}

// A jewel in the turret's picture that is not falling (no gravity on the first filter of its stage that moves it along y) is
// taken in. The barrel holds its frame for as long as the turret waits to take another.
bool Turret::Update()
{
	SceneObject* aJewelObject = nullptr;
	while ((aJewelObject = gPlateLayer->FindChildByType(ObjectTypeJewel, aJewelObject)) != nullptr) {
		bool aFalling = false;
		for (SceneFilter* aFilter = aJewelObject->mStages.mCursor->mFilters.mHead; aFilter != nullptr; aFilter = aFilter->mNext) {
			if (aFilter->mAttribute == AttributeY && aFilter->mType == FilterVelocity && aFilter->mPeriod != 0.0f)
				aFalling = true;
		}
		if (Overlaps(aJewelObject) && !aFalling)
			Catch(static_cast<Jewel*>(aJewelObject));
	}
	mCooldown = mCooldown - 1;
	int aCountdown;
	if (mCooldown < 1) {
		aCountdown = 0;
	} else {
		mCooldown = mCooldown - 1;
		aCountdown = mCooldown;
	}
	if (0 < aCountdown)
		mChildren.mHead->mStages.mCursor->mFrame = mChildren.mHead->mStages.mCursor->mFrame - 1.0f;
	return SceneObject::Update();
}

void Turret::Catch(Jewel* theJewel)
{
	if (!(theJewel->GetYVelocity() > 0.0f) && mCooldown < 1) {
		mCooldown = 250;
		SceneObject* aBarrel = mChildren.mHead;
		SceneObject* aMuzzle = aBarrel->mChildren.mHead;
		// the speed of the jewel, which it leaves with (the sum of the squares is rounded to a float before the root)
		float aVelocityX = theJewel->GetXVelocity();
		float aVelocityY = theJewel->GetYVelocity();
		float aSquaredX = (float)((double)aVelocityX * aVelocityX);
		float aSquaredSum = (float)((double)aVelocityY * aVelocityY + aSquaredX);
		float aSpeed = (float)VeniceSqrt(aSquaredSum);
		float anAngle = ((aBarrel->mLocal[AttributeRotation] - 90.0f) + mLocal[AttributeRotation]) + mParent->mLocal[AttributeRotation];
		// the jewel goes to the turret's place, and a barrel's length out of it along the barrel
		theJewel->mLocal[AttributeX] = (mWorld[AttributeX] - theJewel->mWorld[AttributeX]) + theJewel->mLocal[AttributeX];
		theJewel->mLocal[AttributeY] = (mWorld[AttributeY] - theJewel->mWorld[AttributeY]) + theJewel->mLocal[AttributeY];
		theJewel->mWorld[AttributeX] = mWorld[AttributeX];
		theJewel->mWorld[AttributeY] = mWorld[AttributeY];
		SceneFilter* aVelocityXFilter = theJewel->mStages.mCursor->FindFilter(FilterVelocity, AttributeX);
		SceneFilter* aVelocityYFilter = theJewel->mStages.mCursor->FindFilter(FilterVelocity, AttributeY);
		float aNegativeSpeed = -aSpeed;
		aVelocityXFilter->mAmplitude = (float)(VeniceCos(anAngle * DegreesToRadians) * aNegativeSpeed);
		aVelocityYFilter->mAmplitude = (float)(VeniceSin(anAngle * DegreesToRadians) * aSpeed);
		float aBarrelLength = aMuzzle->mImage->mFrameHeight;
		theJewel->mWorld[AttributeX] =
			(float)((double)theJewel->mWorld[AttributeX] - VeniceCos(anAngle * DegreesToRadians) * aBarrelLength);
		theJewel->mWorld[AttributeY] =
			(float)(VeniceSin(anAngle * DegreesToRadians) * aBarrelLength + (double)theJewel->mWorld[AttributeY]);
		theJewel->mLocal[AttributeX] =
			(float)((double)theJewel->mLocal[AttributeX] - VeniceCos(anAngle * DegreesToRadians) * aBarrelLength);
		theJewel->mLocal[AttributeY] =
			(float)(VeniceSin(anAngle * DegreesToRadians) * aBarrelLength + (double)theJewel->mLocal[AttributeY]);
		// twenty sparks from the turret's place to the end of the barrel, a little past it
		float aReachX = (aMuzzle->mWorld[AttributeX] - mWorld[AttributeX]) * 1.6f;
		float aReachY = (aMuzzle->mWorld[AttributeY] - mWorld[AttributeY]) * 1.6f;
		float aStepX = aReachX / 4.0f;
		float aStepY = aReachY / 5.0f;
		for (float aSparkIndex = 0.0f; aSparkIndex < 20.0f; aSparkIndex = aSparkIndex + 1.0f) {
			SceneObject* aSpark = NewChild(nullptr);
			aSpark->SetImageRecord(gActiveLevel->mWhiteParticleImage);
			aSpark->AddStage((float)RandomBetween(500, 2000), 0, 0, 1, nullptr);
			aSpark->SetAttribute(AttributeAlpha, (float)RandomFloat(0.3f, 0.7f), 0.0f);
			float aScale = (float)RandomFloat(1.0f, 1.8f);
			aSpark->SetAttribute(AttributeXScale, aScale, aScale / 2.0f);
			aSpark->SetAttribute(AttributeYScale, aScale, aScale / 2.0f);
			float anEndX = (float)(RandomFloat(-60.0f, 60.0f) + aStepX);
			float aStartX = (float)(RandomFloat(-10.0f, 10.0f) + aStepX);
			aSpark->SetAttribute(AttributeX, aStartX, anEndX);
			float anEndY = (float)(RandomFloat(-60.0f, 60.0f) + aStepY);
			float aStartY = (float)(RandomFloat(-10.0f, 10.0f) + aStepY);
			aSpark->SetAttribute(AttributeY, aStartY, anEndY);
			aSpark->AddFilter(FilterVelocity, AttributeRotation, (float)RandomFloat(-0.57f, 0.57f), 0.0f, 0.0f);
			aSpark->mZOrder = 9.0f;
			gSparkleLayer->AdoptNewestChild(this, false);
			aStepX = aReachX / 20.0f + aStepX;
			aStepY = aReachY / 20.0f + aStepY;
		}
		LoadAndPlaySound("audio\\turret.ogg", -1);
	}
}
