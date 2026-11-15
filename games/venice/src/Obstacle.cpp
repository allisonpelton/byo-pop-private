// The obstacles (0x4018a0-0x4023f0).

#include "Obstacle.h"

#include "Geometry.h"
#include "ImageRecord.h"
#include "Jewel.h"
#include "Layer.h"
#include "Level.h"
#include "Shooter.h"
#include "Tuning.h"
#include "VeniceGlobals.h"
#include "VeniceSound.h"

#include <cstring>

Obstacle::Obstacle()
{
	mType = ObjectTypeObstacle;
}

// An obstacle that moves down the screen and is out of sight is gone (the warps are kept), and the one that has the jewel in
// flight over it turns half transparent
bool Obstacle::Update()
{
	mBounceCooldown = mBounceCooldown + 1.0f;
	bool anAlive = SceneObject::Update();
	if (gScreenHeight / 2.0f < mWorld[AttributeY] && mPreviousY <= mWorld[AttributeY] && mWorld[AttributeY] != mPreviousY &&
		100.0f < mTickCount && strcmp(gActiveLevel->mName, "trickshot") != 0 &&
		!HitTest(gScreenWidth / 2.0f, gScreenHeight / 2.0f - 30.0f, gScreenWidth, gScreenHeight - 60.0f, 0.0f) &&
		mType != ObjectTypeWarp)
		return false;
	SceneObject* aJewel = FindHighestOfType(ObjectTypeJewel, gShooter);
	if (aJewel != nullptr && Overlaps(aJewel)) {
		float aFaded = mLocal[AttributeAlpha] * 0.99f;
		mLocal[AttributeAlpha] = 0.5f > aFaded ? 0.5f : aFaded;
	} else {
		float aBrighter = mLocal[AttributeAlpha] * 1.01f;
		mLocal[AttributeAlpha] = aBrighter > 1.0f ? 1.0f : aBrighter;
	}
	mPreviousY = mWorld[AttributeY];
	return anAlive;
}

void Obstacle::PlayBumpSound()
{
	if (20.0f < mBounceCooldown) {
		PlaySoundByName("audio\\bump.ogg", -1);
		mBounceCooldown = 0;
	}
}

bool RectBumper::Update()
{
	bool aBounced = false;
	float aHalfWidth = mImage->mFrameWidth / 2.0f;
	float aHalfHeight = mImage->mFrameHeight / 2.0f;
	float aLeft = 0.0f - aHalfWidth;
	float aTop = 0.0f - aHalfHeight;
	float aRight = aHalfWidth;
	float aBottom = aHalfHeight;
	if (1.0f <= mLocal[AttributeAlpha]) {
		SceneObject* aJewelObject = nullptr;
		while ((aJewelObject = gPlateLayer->FindChildByType(ObjectTypeJewel, aJewelObject)) != nullptr) {
			Jewel* aJewel = static_cast<Jewel*>(aJewelObject);
			if ((aJewel->mShot == 0 || !Overlaps(gShooter)) && aJewel->mBumpable != 0 && aJewel->mPowerShot == 0) {
				// the sides, each from its start to its end, clockwise from the top
				float anEdges[4][4] = {
					{aLeft, aTop, aRight, aTop},
					{aRight, aTop, aRight, aBottom},
					{aRight, aBottom, aLeft, aBottom},
					{aLeft, aBottom, aLeft, aTop},
				};
				if (aJewel->CollideWithEdges(this, anEdges, 4, aHalfWidth / 16.0f) && 4 < aJewel->mAge) {
					aBounced = true;
					aJewel->mBounces++;
					aJewel->mLastSlot = nullptr;
					aJewel->mAge = 0;
					SceneEvent* aCurrentEvent = mStages.mCursor->mEvents.mCursor;
					if (aCurrentEvent != nullptr && aCurrentEvent->mTrigger == TriggerCode)
						aJewel->mHitBumperWithCode = 1;
				}
			}
		}
	}
	if (aBounced) {
		SceneEvent* aCurrentEvent = mStages.mCursor->mEvents.mCursor;
		if (aCurrentEvent == nullptr || aCurrentEvent->mTrigger != TriggerCode) {
			PlayBumpSound();
		} else {
			aCurrentEvent->Fire();
			// a stage that plays a sound of its own on starting does not also bump
			if (mStages.mCursor->FindEvent(TriggerStart, ActionPlaySound) == nullptr)
				PlayBumpSound();
		}
	}
	return Obstacle::Update();
}

bool RoundBumper::Update()
{
	SceneObject* aJewelObject = nullptr;
	while ((aJewelObject = gPlateLayer->FindChildByType(ObjectTypeJewel, aJewelObject)) != nullptr) {
		Jewel* aJewel = static_cast<Jewel*>(aJewelObject);
		if ((aJewel->mShot == 0 || !Overlaps(gShooter)) && aJewel->mBumpable != 0 && aJewel->mPowerShot == 0 &&
			1.0f <= mLocal[AttributeAlpha] &&
			CircleOverlap(aJewel->mWorld[AttributeX], aJewel->mWorld[AttributeY], aJewel->mImage->mFrameWidth / 2.5f,
				mWorld[AttributeX], mWorld[AttributeY], (mImage->mFrameWidth * mWorld[AttributeXScale]) / 2.2f) != 0.0f) {
			SceneFilter* aVelocityX = aJewel->mStages.mCursor->FindFilter(FilterVelocity, AttributeX);
			SceneFilter* aVelocityY = aJewel->mStages.mCursor->FindFilter(FilterVelocity, AttributeY);
			if (aVelocityX == nullptr)
				aVelocityX = aJewel->mStages.mCursor->AddFilter(FilterVelocity, AttributeX, 0, 0, 0, false);
			if (aVelocityY == nullptr)
				aVelocityY = aJewel->mStages.mCursor->AddFilter(FilterVelocity, AttributeY, 0, 0, 0, false);
			ReflectOffBall(&aVelocityX->mAmplitude, &aVelocityY->mAmplitude, aJewel->mWorld[AttributeX],
				aJewel->mWorld[AttributeY], mWorld[AttributeX], mWorld[AttributeY]);
			if (4 < aJewel->mAge) {
				PlayBumpSound();
				aJewel->mBounces++;
				aJewel->mLastSlot = nullptr;
				aJewel->mAge = 0;
			}
		}
	}
	return Obstacle::Update();
}

bool TriBumper::Update()
{
	float aHalfWidth = (mSourceRight - mSourceLeft) / 2.0f;
	float aHalfHeight = (mSourceBottom - mSourceTop) / 2.0f;
	float aLeft = 0.0f - aHalfWidth;
	float aTop = 0.0f - aHalfHeight;
	float aRight = aHalfWidth;
	float aBottom = aHalfHeight;
	if (1.0f <= mLocal[AttributeAlpha]) {
		SceneObject* aJewelObject = nullptr;
		while ((aJewelObject = gPlateLayer->FindChildByType(ObjectTypeJewel, aJewelObject)) != nullptr) {
			Jewel* aJewel = static_cast<Jewel*>(aJewelObject);
			if ((aJewel->mShot == 0 || !Overlaps(gShooter)) && aJewel->mBumpable != 0 && aJewel->mPowerShot == 0) {
				// the sides: along the top, down to the point in the middle of the bottom, and back up to the top left
				float anEdges[3][4] = {
					{aLeft, aTop, aRight, aTop},
					{aRight, aTop, 0.0f, aBottom},
					{0.0f, aBottom, aLeft, aTop},
				};
				if (aJewel->CollideWithEdges(this, anEdges, 3, aHalfWidth / 8.0f)) {
					bool aCounted = 4 < aJewel->mAge;
					if (aCounted) {
						aJewel->mBounces++;
						aJewel->mLastSlot = nullptr;
						aJewel->mAge = 0;
					}
					SceneEvent* aCurrentEvent = mStages.mCursor->mEvents.mCursor;
					if (aCurrentEvent == nullptr || aCurrentEvent->mTrigger != TriggerCode) {
						if (aCounted)
							PlayBumpSound();
					} else {
						aCurrentEvent->Fire();
						if (mStages.mCursor->FindEvent(TriggerStart, ActionPlaySound) == nullptr && aCounted)
							PlayBumpSound();
					}
				}
			}
		}
	}
	return Obstacle::Update();
}
