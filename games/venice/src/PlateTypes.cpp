// The kinds of plate (0x414e40-0x4164b0).

#include "PlateTypes.h"

#include "ImageRecord.h"
#include "Jewel.h"
#include "Profiles.h"
#include "Slot.h"
#include "VeniceMath.h"

#include <cstdio>
#include <cstring>

namespace
{
// 0x415430 and 0x415a20: a plate that swings is pulled back to straight by a force that grows with its tilt, and the swing
// loses a little speed each update
void Swing(float* theTilt, float* theSpin, float theStiffness)
{
	*theSpin = *theSpin - *theTilt / theStiffness;
	*theSpin = *theSpin * 0.993f;
	*theTilt = *theTilt + *theSpin;
}

// the direction a jewel's hit would turn a plate (the original tests the sign of the turn's size where it needs it)
float TurnSign(double theTwist)
{
	return 0.0 <= theTwist ? 1.0f : -1.0f;
}
}

// 0x414e80: the plates that turn when hit
void RoundPlate::OnJewelHit(Slot* theSlot, Jewel* theJewel)
{
	Twist(theJewel, theSlot, 50.0f, 0.995f);
	Rattle(theJewel, theSlot, 1.0f);
}

void RoundPlate::SetUp()
{
	strcpy(mShapeName, "round_plate");
	SetUpSlots();
}

void RoundPlate::PositionSlot(Slot* theSlot, int theIndex)
{
	PlaceOnCircle(theSlot, ((float)theIndex * 360.0f) / (float)mSlotCount);
}

bool SemicirclePlate::Update()
{
	if (mStages.mCursor != nullptr && strstr(mStages.mCursor->mName, "fade") == nullptr)
		Swing(&mLocal[AttributeRotation], &mSpin, 4400.0f);
	return Plate::Update();
}

void SemicirclePlate::OnJewelHit(Slot* theSlot, Jewel* theJewel)
{
	Twist(theJewel, theSlot, 50.0f, 0.995f);
	Rattle(theJewel, theSlot, 1.0f);
}

void SemicirclePlate::SetUp()
{
	strcpy(mShapeName, "semicircle_plate");
	SetUpSlots();
}

void SemicirclePlate::PositionSlot(Slot* theSlot, int theIndex)
{
	PlaceOnCircle(theSlot, ((float)theIndex * 180.0f) / (float)mSlotCount + 30.0f);
}

void APlate::OnJewelHit(Slot* theSlot, Jewel* theJewel)
{
	WobbleOnHit(theSlot, theJewel, 120.0f, 0.5f, 10.0f);
}

void APlate::SetUp()
{
	strcpy(mShapeName, "a_plate");
	SetUpSlots();
}

void APlate::PositionSlot(Slot* theSlot, int theIndex)
{
	int aDistanceFromMiddle = theIndex - mSlotCount / 2;
	float aRadius = mRadius;
	PlaceSlot(theSlot, (float)aDistanceFromMiddle * aRadius,
		(float)(VeniceAbs((float)aDistanceFromMiddle) * (double)mRadius - (double)mRadius));
}

void XPlate::SetUp()
{
	strcpy(mShapeName, "x_plate");
	SetUpSlots();
}

void XPlate::PositionSlot(Slot* theSlot, int theIndex)
{
	float aX = 0.0f;
	float aY = 0.0f;
	switch (theIndex) {
	case 0:
		aX = (0.0f - mRadius) * 2.0f;
		aY = (0.0f - mRadius) * 2.0f;
		break;
	case 1:
		aX = 0.0f - mRadius;
		aY = 0.0f - mRadius;
		break;
	case 2:
		aX = 0.0f - mRadius;
		aY = mRadius;
		break;
	case 3:
		aX = (0.0f - mRadius) * 2.0f;
		aY = mRadius * 2.0f;
		break;
	case 4:
		aX = mRadius;
		aY = mRadius;
		break;
	case 5:
		aX = mRadius * 2.0f;
		aY = mRadius * 2.0f;
		break;
	case 6:
		aX = mRadius;
		aY = 0.0f - mRadius;
		break;
	case 7:
		aX = mRadius * 2.0f;
		aY = (0.0f - mRadius) * 2.0f;
		break;
	}
	PlaceSlot(theSlot, aX, aY);
}

void StickPlate::OnJewelHit(Slot* theSlot, Jewel* theJewel)
{
	float aOffset = theSlot->mWorld[AttributeX] - mWorld[AttributeX];
	float aHalfWidth = (mSourceRight - mSourceLeft) / 2.0f;
	float aSign = TurnSign(TwistAmount(theJewel, theSlot));
	Wobble((float)(VeniceAbs((aOffset * 120.0f) / aHalfWidth) * (double)aSign), 0.5f);
	Rattle(theJewel, theSlot, 1.0f);
}

void StickPlate::SetUp()
{
	strcpy(mShapeName, "stick_plate");
	SetUpSlots();
}

void StickPlate::PositionSlot(Slot* theSlot, int theIndex)
{
	float aX = (float)theIndex * mRadius + (0.0f - mRadius) * (float)(mSlotCount / 2);
	if (mSlotCount / 2 - 1 < theIndex)
		aX = aX + mRadius;
	PlaceSlot(theSlot, aX, 0);
}

void BlockPlate::SetUp()
{
	strcpy(mShapeName, "block_plate");
	SetUpSlots();
}

void BlockPlate::PositionSlot(Slot* theSlot, int theIndex)
{
	PlaceSlot(theSlot, 0, 0);
}

void TrianglePlate::OnJewelHit(Slot* theSlot, Jewel* theJewel)
{
	if (!(VeniceAbs(theSlot->mWorld[AttributeX] - mWorld[AttributeX]) <= (double)theSlot->mImage->mFrameWidth / 2.0 &&
			theJewel->GetXVelocity() <= 0.01f))
		Wobble(TurnSign(TwistAmount(theJewel, theSlot)) * 120.0f, 0.25f);
	Rattle(theJewel, theSlot, 1.0f);
}

void TrianglePlate::SetUp()
{
	strcpy(mShapeName, "triangle_plate");
	SetUpSlots();
}

void TrianglePlate::PositionSlot(Slot* theSlot, int theIndex)
{
	PlaceOnCircle(theSlot, (float)theIndex * 120.0f - 90.0f);
}

bool TPlate::Update()
{
	if (mStages.mCursor != nullptr && strstr(mStages.mCursor->mName, "fade") == nullptr)
		Swing(&mLocal[AttributeRotation], &mSpin, 9400.0f);
	return Plate::Update();
}

void TPlate::OnJewelHit(Slot* theSlot, Jewel* theJewel)
{
	Twist(theJewel, theSlot, 100.0f, 0.995f);
	Rattle(theJewel, theSlot, 1.0f);
}

void TPlate::SetUp()
{
	strcpy(mShapeName, "t_plate");
	SetUpSlots();
}

void TPlate::PositionSlot(Slot* theSlot, int theIndex)
{
	PlaceSlot(theSlot, (float)(theIndex - mSlotCount / 2) * mRadius, mRadius * 2.0f - 8.0f);
}

void TosserPlate::OnJewelHit(Slot* theSlot, Jewel* theJewel)
{
	Rattle(theJewel, theSlot, 1.0f);
}

void TosserPlate::SetUp()
{
	strcpy(mShapeName, "tosser_plate");
	SetUpSlots();
}

void TosserPlate::OnSlotFilled()
{
	for (SceneObject* aChild = mChildren.mHead; aChild != nullptr; aChild = aChild->mNext) {
		if (aChild->mType == ObjectTypeSlot) {
			static_cast<Slot*>(aChild)->TossJewel(0, 0, (float)RandomFloat(-3.5f, 3.5f), true);
			aChild->mIsDead = true;
		}
	}
}

// the first slot is the empty one the player fills; the rest show the jewels the plate has already thrown (as filled slots)
void TosserPlate::PositionSlot(Slot* theSlot, int theIndex)
{
	float aX;
	float aY;
	if (theIndex == 0) {
		aX = 0.0f;
		aY = -14.0f;
		theSlot->mFilled = 0;
	} else {
		aX = ((float)theIndex - (float)mSlotCount / 2.0f) * mRadius;
		aY = -73.0f;
		theSlot->mFilled = 1;
		if (CurrentProfile().mMode == GameModeJourney) {
			theSlot->mKind = 999;
			theSlot->SetImage("gfx\\pwrup_wild_shape.png", 0, 0, 0, 0);
		} else {
			char anImageName[68];
			sprintf(anImageName, "gfx\\outline%d.png", theSlot->mKind);
			theSlot->SetImage(anImageName, 0, 0, 0, 0);
		}
	}
	PlaceSlot(theSlot, aX, aY);
}

void KeyPlate::PopEffect()
{
	Plate::PopEffect();
	SuckBaskets(true);
}

void KeyPlate::SetUp()
{
	strcpy(mShapeName, "key_plate");
	SetUpSlots();
}

void KeyPlate::PositionSlot(Slot* theSlot, int theIndex)
{
	PlaceSlot(theSlot, 0, 17.0f);
}

void SpinnerPlate::OnJewelHit(Slot* theSlot, Jewel* theJewel)
{
	Twist(theJewel, theSlot, 100.0f, 0.995f);
	Rattle(theJewel, theSlot, 1.0f);
}

void SpinnerPlate::SetUp()
{
	strcpy(mShapeName, "spinner_plate");
	SetUpSlots();
}

void SpinnerPlate::PositionSlot(Slot* theSlot, int theIndex)
{
	PlaceSlot(theSlot, theIndex == 0 ? 0.0f - mRadius : mRadius, 0);
}

void StepPlate::OnJewelHit(Slot* theSlot, Jewel* theJewel)
{
	WobbleOnHit(theSlot, theJewel, 180.0f, 0.25f, 1.0f);
}

void StepPlate::SetUp()
{
	strcpy(mShapeName, "step_plate");
	SetUpSlots();
}

void StepPlate::PositionSlot(Slot* theSlot, int theIndex)
{
	float aX = (float)-(theIndex - 1) * mRadius;
	if (mRadius <= 0.0f && mRadius != 0.0f)
		aX = aX * -1.0f;
	PlaceSlot(theSlot, aX, ((float)(theIndex - 1) * mRadius) / 1.7f);
}

void SquarePlate::OnJewelHit(Slot* theSlot, Jewel* theJewel)
{
	Twist(theJewel, theSlot, 50.0f, 0.995f);
	Rattle(theJewel, theSlot, 1.0f);
}

void SquarePlate::SetUp()
{
	strcpy(mShapeName, "square_plate");
	SetUpSlots();
}

void SquarePlate::PositionSlot(Slot* theSlot, int theIndex)
{
	float aRadius = mRadius;
	float aX = 0.0f;
	float aY = 0.0f;
	switch (theIndex) {
	case 0:
		aX = 0.0f - aRadius;
		aY = 0.0f - aRadius;
		break;
	case 1:
		aX = 0.0f;
		aY = 0.0f - aRadius;
		break;
	case 2:
		aX = aRadius;
		aY = 0.0f - aRadius;
		break;
	case 3:
		aX = aRadius;
		aY = 0.0f;
		break;
	case 4:
		aX = aRadius;
		aY = aRadius;
		break;
	case 5:
		aX = 0.0f;
		aY = aRadius;
		break;
	case 6:
		aX = 0.0f - aRadius;
		aY = aRadius;
		break;
	case 7:
		aX = 0.0f - aRadius;
		aY = 0.0f;
		break;
	}
	PlaceSlot(theSlot, aX, aY);
}

void MPlate::OnJewelHit(Slot* theSlot, Jewel* theJewel)
{
	WobbleOnHit(theSlot, theJewel, 180.0f, 0.25f, 30.0f);
}

void MPlate::SetUp()
{
	strcpy(mShapeName, "m_plate");
	SetUpSlots();
}

// the M's slots go along its two humps: the table gives the position of the first four, the last four mirror them
void MPlate::PositionSlot(Slot* theSlot, int theIndex)
{
	static const float aHumpX[4] = {32.0f, 74.0f, 133.0f, 103.0f};
	static const float aHumpY[4] = {72.0f, 61.0f, 119.0f, 90.0f};
	float aX = aHumpX[theIndex % 4];
	float aY = aHumpY[theIndex % 4];
	if (theIndex > 3)
		aX = 0.0f - aX;
	PlaceSlot(theSlot, aX, aY - 83.0f);
}
