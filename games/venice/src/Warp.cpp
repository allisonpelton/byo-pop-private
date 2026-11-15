// The warp (0x409dc0, 0x402ad0-0x403220).

#include "Warp.h"

#include "ImageRecord.h"
#include "Jewel.h"
#include "Level.h"
#include "Shooter.h"
#include "VeniceGlobals.h"
#include "VeniceMath.h"
#include "VeniceSound.h"

#include <algorithm>

namespace
{
// 0x43a000: the angle in degrees of the line to a point, which the arrows are turned to
double AngleOfLineTo(float theDeltaX, float theDeltaY)
{
	return (VeniceArcTangent(theDeltaY, 0.0f - theDeltaX) * 180.0f) / 3.1415927f;
}
}

Warp::Warp()
{
	mType = ObjectTypeWarp;
	mZOrder = 1.0f;
}

void Warp::OnObjectClosed()
{
	mArrowImage = AcquireImageRecord("gfx\\arrow.png", 0, 0, 0, 0);
}

bool Warp::Update()
{
	if (1.0f <= mLocal[AttributeAlpha]) {
		SceneObject* aJewelObject = nullptr;
		while ((aJewelObject = mParent->FindChildByType(ObjectTypeJewel, aJewelObject)) != nullptr) {
			Jewel* aJewel = static_cast<Jewel*>(aJewelObject);
			bool anInside = HasJewelInside(aJewel);
			if (!aJewel->CircleOverlapAt(mWorld[AttributeX], mWorld[AttributeY], 20.0f) || anInside) {
				if (anInside && !aJewel->CircleOverlapAt(mWorld[AttributeX], mWorld[AttributeY], 20.0f))
					ForgetJewel(aJewel);
			} else if (aJewel->mTarget == nullptr) {
				Transport(aJewel);
			}
		}
		ShowArrows();
	}
	return Obstacle::Update();
}

bool Warp::HasJewelInside(Jewel* theJewel) const
{
	return std::find(mJewelsInside.begin(), mJewelsInside.end(), theJewel) != mJewelsInside.end();
}

void Warp::ForgetJewel(Jewel* theJewel)
{
	mJewelsInside.erase(std::remove(mJewelsInside.begin(), mJewelsInside.end(), theJewel), mJewelsInside.end());
}

// The jewel comes out of the nearest other warp (the place it is moved to is that warp's, in the frame they share). The ninth
// warp the jewel goes through ends it, and counts as a missed trick shot in that mode.
void Warp::Transport(Jewel* theJewel)
{
	Warp* anOther = static_cast<Warp*>(FindNearestSibling(mType));
	if (anOther != nullptr) {
		theJewel->mLocal[AttributeX] = anOther->mLocal[AttributeX];
		theJewel->mLocal[AttributeY] = anOther->mLocal[AttributeY];
		PlaySoundByName("audio\\warp.ogg", -1);
		Burst();
		anOther->Burst();
		theJewel->mWarpCount++;
		if (theJewel->mWarpCount < 9) {
			anOther->mJewelsInside.push_back(theJewel);
		} else {
			theJewel->mIsDead = true;
			gActiveLevel->MissTrickShot();
		}
	}
}

void Warp::Burst()
{
	ImageRecord* aSparkImage = gActiveLevel->mSparkleImage;
	for (int aSparkIndex = 1; aSparkIndex < 21; aSparkIndex++) {
		SceneObject* aSpark = NewChild(nullptr);
		aSpark->SetImageRecord(aSparkImage);
		aSpark->AddStage(300.0f, 0, 0, 1, nullptr);
		aSpark->SetAttribute(AttributeAlpha, 0.0f, 1.0f);
		aSpark->SetAttribute(AttributeXScale, 1.0f, 0.5f);
		aSpark->SetAttribute(AttributeYScale, 1.0f, 0.5f);
		float anAngle = (((float)aSparkIndex * 360.0f) / 20.0f) * DegreesToRadians;
		double aCosine = VeniceCos(anAngle);
		double aSine = VeniceSin(anAngle);
		aSpark->SetAttribute(AttributeX, (float)(aCosine * 64.0), 0.0f);
		aSpark->SetAttribute(AttributeY, (float)(aSine * 64.0), 0.0f);
		aSpark->AddFilter(FilterSquared, AttributeAll, 0.0f, 0.0f, 0.0f);
		aSpark->mZOrder = 9.0f;
		aSpark->mBlitStyle = 1;
		gSparkleLayer->AdoptNewestChild(this, false);
	}
}

// Arrows run from this warp to the nearest other one, both on the screen, while the shooter is within three quarters of the
// warp's width of this one in x. They fade in and out at the ends and move along the line, repeating every 240 updates.
void Warp::ShowArrows()
{
	SceneObject* anOther = FindNearestSibling(mType);
	if (anOther != nullptr && 0.0f <= anOther->mWorld[AttributeY] && anOther->mWorld[AttributeY] <= gScreenHeight &&
		0.0f <= mWorld[AttributeY] && mWorld[AttributeY] <= gScreenHeight &&
		VeniceAbs(mWorld[AttributeX] - gShooter->mWorld[AttributeX]) <= (double)mImage->mFrameWidth * 0.75f) {
		float aDistance = (float)DistanceTo(anOther);
		for (float aPosition = 0.0f; aPosition < aDistance; aPosition = aPosition + 80.0f) {
			SceneObject* anArrow = mParent->NewChild(nullptr);
			anArrow->SetImageRecord(mArrowImage);
			anArrow->AddStage(4.0f, 0, 0, 1, nullptr);
			anArrow->mBlitStyle = 1;
			anArrow->SetZOrder(mParent->mChildren.mTail->mZOrder);
			float aFraction = ((float)((int)mTickCount % 240) / 3.0f + aPosition) / aDistance;
			if (aFraction > 1.0f)
				aFraction = 1.0f;
			if (aFraction > 0.8f)
				anArrow->mLocal[AttributeAlpha] = (1.0f - aFraction) * 5.0f;
			else if (0.2f > aFraction)
				anArrow->mLocal[AttributeAlpha] = aFraction * 5.0f;
			anArrow->mLocal[AttributeX] = (anOther->mLocal[AttributeX] - mLocal[AttributeX]) * aFraction + mLocal[AttributeX];
			anArrow->mLocal[AttributeY] = (anOther->mLocal[AttributeY] - mLocal[AttributeY]) * aFraction + mLocal[AttributeY];
			anArrow->mLocal[AttributeRotation] = (float)AngleOfLineTo(mWorld[AttributeX] - anOther->mWorld[AttributeX],
				mWorld[AttributeY] - anOther->mWorld[AttributeY]);
		}
	}
}
