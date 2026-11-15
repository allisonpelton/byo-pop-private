// The plate (0x412800-0x414df0): its slots, the pop when it is full, the danger when it sinks into the water.

#include "Plate.h"

#include "Emitter.h"
#include "GameText.h"
#include "ImageRecord.h"
#include "Jewel.h"
#include "Layer.h"
#include "Level.h"
#include "Powerup.h"
#include "Profiles.h"
#include "Scoring.h"
#include "Slot.h"
#include "TextObject.h"
#include "Tuning.h"
#include "VeniceGlobals.h"
#include "VeniceMath.h"
#include "VeniceSound.h"

#include <cstdio>
#include <cstring>

namespace
{
// 0x5ba2d4: whether the warning of danger has been shown since the last plate was made
bool sDangerWarningShown = false;
// 0x5b5158 and 0x5b515c: the colour of the slot made before, and of the first slot of the plate being made
int sPreviousSlotKind = 999;
int sFirstSlotKind = 999;
}

Plate::Plate()
{
	mType = ObjectTypePlate;
	mTumbleLevel = 0;
	mZOrder = 1.0f;
	mRadius = 0;
	sDangerWarningShown = false;
}

void Plate::Load(const char* theText, SceneTag* theTag)
{
	mSlotCount = (int)theTag->GetFloat("num_jewels");
	mRadius = (float)theTag->GetFloat("radius");
	SetUp();
	SceneObject::Load(theText, theTag);
	SetZOrder(1.0f);
	mBilinear = 1;
}

void Plate::OnObjectClosed()
{
	for (SceneObject* aChild = mChildren.mHead; aChild != nullptr; aChild = aChild->mNext)
		aChild->mBilinear = 1;
}

bool Plate::Update()
{
	// a plate that fades out is only updated (the original's 0x412ba0 throws its slots' jewels away there when the tuning's
	// toss_on_filled_plate is set, which the shipped tuning does not do)
	if (strcmp(mStages.mCursor->mName, "fade_out") == 0)
		return SceneObject::Update();
	if (!AllSlotsFilled())
		CheckDanger();
	else {
		GoToStageByName("fade_out");
		SetAttribute(AttributeRotation, mLocal[AttributeRotation], mLocal[AttributeRotation] - 1080.0f);
		AddFilter(FilterSquared, AttributeAll, 0, 0, 0);
		PopEffect();
		if (mSlotCount > 1 && gTutorial == nullptr) {
			SceneStage* aMiddleStage = gMiddleLayer->mStages.mCursor;
			aMiddleStage->mFrame = 0;
			if (CurrentProfile().mMode == GameModeFlood)
				aMiddleStage->AddFilter(FilterVelocity, AttributeY, gTuning.mPushFilledPlate, 0, 0.79f, true);
		}
		SceneObject* aPowerup = FindChildByType(ObjectTypePowerup, nullptr);
		if (aPowerup == nullptr) {
			if (gTutorial != nullptr && gPlateLayer->CountChildrenOfType(ObjectTypePlate) == 3)
				Powerup::SpawnFrom(this);
		} else {
			static_cast<Powerup*>(aPowerup)->StartFlying();
			mChildren.mCursor = aPowerup;
			mParent->AdoptNewestChild(this, true);
		}
	}
	return SceneObject::Update();
}

// 0x412c20: the baskets and bumpers near the plate (all of them when the key plate pops) are pulled into it
void Plate::SuckBaskets(bool theByVictory)
{
	for (SceneObject* anObstacle = mParent->mChildren.mHead; anObstacle != nullptr; anObstacle = anObstacle->mNext) {
		if (anObstacle->mType != ObjectTypeObstacle)
			continue;
		bool aNear;
		if (!theByVictory) {
			aNear = !(mImage->mFrameHeight <= VeniceAbs(mWorld[AttributeY] - anObstacle->mWorld[AttributeY]) ||
				mImage->mFrameWidth <= VeniceAbs(mWorld[AttributeX] - anObstacle->mWorld[AttributeX]));
		} else {
			SceneObject* aLowest = FindLowestOfType(ObjectTypeObstacle, nullptr);
			aNear = VeniceAbs(anObstacle->mWorld[AttributeY] - aLowest->mWorld[AttributeY]) < 12.0;
		}
		if (aNear && strcmp(anObstacle->mStages.mCursor->mName, "suck") != 0) {
			anObstacle->AddStage(mStages.mCursor->mLength * 8.0f, 0, 0, 1, "suck");
			anObstacle->SetAttribute(AttributeX, AttributeUnsetValue, mLocal[AttributeX]);
			anObstacle->SetAttribute(AttributeY, AttributeUnsetValue, mLocal[AttributeY]);
			anObstacle->SetAttribute(AttributeAlpha, AttributeUnsetValue, 0);
			anObstacle->SetAttribute(AttributeRotation, AttributeUnsetValue, 720.0f);
			anObstacle->AddFilter(FilterSquared, AttributeAll, 0, 0, 0);
			anObstacle->AddFilter(FilterDecaySined, AttributeY, 60.0f, 0, 0);
			anObstacle->GoToStageByName("suck");
			if (theByVictory)
				anObstacle->MakeSparkles(40, 1.0f, 1.0f, 1.0f, 1.0f);
		}
	}
	for (int aPass = 0; aPass < 3; aPass++) {
		for (SceneObject* anObstacle = mParent->mChildren.mHead; anObstacle != nullptr; anObstacle = anObstacle->mNext) {
			if (anObstacle->mType == ObjectTypeObstacle && strcmp(anObstacle->mStages.mCursor->mName, "suck") == 0)
				anObstacle->SetZOrder(anObstacle->mParent->mChildren.mTail->mZOrder + 0.0001f);
		}
	}
}

// 0x412f00
void Plate::Rattle(Jewel* theJewel, Slot* theSlot, float theScale)
{
	SceneFilter* aJewelFilter = theJewel->mStages.mCursor->FindFilter(FilterVelocity, AttributeY);
	if (aJewelFilter != nullptr) {
		double aSpeed = VeniceAbs(aJewelFilter->mAmplitude);
		SceneFilter* aFilter = mStages.mCursor->AddFilter(FilterDecaySined, AttributeY,
			(float)(aSpeed / 3.0) * -12.0f * theScale, 0.5f, 0, false);
		aFilter->mDuration = 100.0f;
	}
}

// 0x412f90
void Plate::Wobble(float theAmplitude, float thePeriod)
{
	mStages.mCursor->mAttributeMask = mStages.mCursor->mAttributeMask | (1u << AttributeRotation);
	SceneFilter* aFilter = mStages.mCursor->AddFilter(FilterSined, AttributeRotation, theAmplitude, thePeriod, 0, false);
	aFilter->mDuration = 150.0f;
	aFilter->mResetsAttribute = true;
}

// 0x414e40
void Plate::BoostSpin()
{
	SceneFilter* aFilter = mStages.mCursor->FindFilter(FilterVelocity, AttributeRotation);
	if (aFilter != nullptr)
		aFilter->mAmplitude = aFilter->mAmplitude * 1.2f;
}

// 0x4155b0, 0x415f20, 0x416280
void Plate::WobbleOnHit(Slot* theSlot, Jewel* theJewel, float theAmplitude, float thePeriod, float theRattle)
{
	float aAmplitude = 0.0f;
	if (!(VeniceAbs(theSlot->mWorld[AttributeX] - mWorld[AttributeX]) <= (double)mImage->mFrameWidth / 15.0 &&
			theJewel->GetXVelocity() == 0.0f)) {
		float aSign = 0.0 <= TwistAmount(theJewel, theSlot) ? 1.0f : -1.0f;
		aAmplitude = aSign * theAmplitude;
	}
	Wobble(aAmplitude, thePeriod);
	Rattle(theJewel, theSlot, theRattle);
}

// 0x413010
double Plate::TwistAmount(Jewel* theJewel, Slot* theSlot)
{
	float aDirection = VeniceArcTangent(mWorld[AttributeY] - theSlot->mWorld[AttributeY],
		theSlot->mWorld[AttributeX] - mWorld[AttributeX]);
	float aJewelXVelocity = theJewel->GetXVelocity();
	float aJewelYVelocity = theJewel->GetYVelocity();
	float aMovedDirection = VeniceArcTangent(mWorld[AttributeY] - (theSlot->mWorld[AttributeY] + aJewelYVelocity),
		(theSlot->mWorld[AttributeX] + aJewelXVelocity) - mWorld[AttributeX]);
	while (VeniceAbs(aMovedDirection - aDirection) > 1.5707964f) {
		float aSign = 0.0f <= aMovedDirection - aDirection ? 1.0f : -1.0f;
		aMovedDirection = aMovedDirection - aSign * 3.1415927f;
	}
	return (double)aMovedDirection - (double)aDirection;
}

// 0x413150
void Plate::Twist(Jewel* theJewel, Slot* theSlot, float theScale, float theDecay)
{
	double aTwist = TwistAmount(theJewel, theSlot);
	if (theJewel->mStages.mCursor->FindFilter(FilterVelocity, AttributeY) != nullptr) {
		float aTurning = 0;
		for (SceneFilter* aFilter = mStages.mCursor->mFilters.mHead; aFilter != nullptr; aFilter = aFilter->mNext) {
			if (aFilter->mType == FilterVelocity && aFilter->mAttribute == AttributeRotation)
				aTurning = aTurning + aFilter->mAmplitude;
		}
		if (VeniceAbs(aTurning + (float)(aTwist * (double)theScale)) < 6.0) {
			mStages.mCursor->AddFilter(FilterVelocity, AttributeRotation, (float)(aTwist * (double)theScale), 0, theDecay,
				false);
		}
	}
}

// 0x413250
void Plate::PopEffect()
{
	SceneObject* aSparkles = new SceneObject();
	aSparkles->mZOrder = 0.9f;
	NewChild(aSparkles);
	mParent->AdoptNewestChild(this, false);
	aSparkles->AddStage(2000.0f, 0, 0, 1, nullptr);
	aSparkles->SetAttribute(AttributeRotation, 0, 1080.0f);
	aSparkles->AddFilter(FilterSquared, AttributeRotation, 0, 0, 0);
	ImageRecord* aSparkleImage = gActiveLevel->mSparkleImage;
	for (int aSparkleIndex = 0; aSparkleIndex < 50; aSparkleIndex++) {
		SceneObject* aSparkle = new SceneObject();
		aSparkle->SetImageRecord(aSparkleImage);
		aSparkle->AddStage((float)RandomBetween(300, 600), 0, 0, 1, nullptr);
		aSparkle->SetAttribute(AttributeX, 0, (float)RandomFloat((0.0f - mSourceRight) / 1.5f, mSourceRight / 1.5f));
		aSparkle->SetAttribute(AttributeY, 0, (float)RandomFloat((0.0f - mSourceBottom) / 1.5f, mSourceBottom / 1.5f));
		aSparkle->SetAttribute(AttributeAlpha, 0, (float)RandomFloat(0.8f, 1.0f));
		aSparkle->AddStage((float)RandomBetween(200, 400), 0, 0, 1, nullptr);
		aSparkle->SetAttribute(AttributeX, AttributeContinueValue, 0);
		aSparkle->SetAttribute(AttributeY, AttributeContinueValue, 0);
		aSparkle->SetAttribute(AttributeAlpha, AttributeContinueValue, 0);
		aSparkles->SetAttribute(AttributeRotation, 0, -720.0f);
		aSparkle->AddFilter(FilterCubed, AttributeYScale, 0, 0, 0);
		aSparkle->AddFilter(FilterSquareRoot, AttributeYScale, 0, 0, 0);
		aSparkle->mBlitStyle = 1;
		aSparkles->AddChild(aSparkle);
	}
	float aRelativeX = mWorld[AttributeX] - gActiveLevel->mWorld[AttributeX];
	float aRelativeY = mWorld[AttributeY] - gActiveLevel->mWorld[AttributeY];
	if (mTumbleLevel < 2) {
		TextObject* aScoreText = new TextObject();
		aScoreText->SetText(FormatScore(500), "gfx\\font1.png", 2.0f, 40, JustifyCenter);
		aScoreText->AddStage(1000.0f, 0, 0, 1, nullptr);
		aScoreText->SetAttribute(AttributeXScale, 0.6f, 0.6f);
		aScoreText->SetAttribute(AttributeYScale, 0.6f, 0.6f);
		aScoreText->SetAttribute(AttributeAlpha, 0.001f, 0.001f);
		aScoreText->SetAttribute(AttributeX, aRelativeX + 60.0f, aRelativeX - 60.0f);
		aScoreText->SetAttribute(AttributeY, aRelativeY, aRelativeY);
		aScoreText->AddFilter(FilterSuperCos, AttributeAll, 2.0f, 0, 0);
		aScoreText->AddFilter(FilterSined, AttributeAlpha, 0.8f, 0.5f, 0);
		aScoreText->AddFilter(FilterSined, AttributeXScale, 0.4f, 0.5f, 0);
		aScoreText->AddFilter(FilterSined, AttributeYScale, 0.4f, 0.5f, 0);
		aScoreText->AddFilter(FilterSined, AttributeY, 30.0f, 0.5f, 0);
		gActiveLevel->NewChild(aScoreText);
	} else {
		char aScoreString[32];
		char aComboString[32];
		sprintf(aScoreString, "%s", FormatScore(mTumbleLevel * 1000));
		if (mTumbleLevel == 2)
			sprintf(aComboString, "DOUBLE COMBO!");
		else if (mTumbleLevel == 3)
			sprintf(aComboString, "TRIPLE COMBO!");
		else if (mTumbleLevel == 4)
			sprintf(aComboString, "QUAD COMBO!");
		else
			sprintf(aComboString, "%dx COMBO!", mTumbleLevel);
		TextObject* aComboText = new TextObject();
		aComboText->SetText(aComboString, "gfx\\font1_small.png", 0.2f, 0, JustifyCenter);
		aComboText->AddStage(2000.0f, 0, 0, 1, nullptr);
		aComboText->SetAttribute(AttributeXScale, 1.0f, 1.0f);
		aComboText->SetAttribute(AttributeYScale, 1.0f, 1.0f);
		aComboText->SetAttribute(AttributeAlpha, 1.0f, 1.0f);
		aComboText->SetAttribute(AttributeX, aRelativeX, aRelativeX);
		aComboText->SetAttribute(AttributeY, aRelativeY - 15.0f, aRelativeY - 15.0f);
		aComboText->AddStage(300.0f, 0, 0, 1, nullptr);
		aComboText->SetAttribute(AttributeAlpha, 1.0f, 0);
		gActiveLevel->NewChild(aComboText);
		TextObject* aScoreText = new TextObject();
		aScoreText->SetText(aScoreString, "gfx\\font1_small.png", 0.2f, 0, JustifyCenter);
		aScoreText->AddStage(2000.0f, 0, 0, 1, nullptr);
		aScoreText->SetAttribute(AttributeXScale, 1.0f, 1.0f);
		aScoreText->SetAttribute(AttributeYScale, 1.0f, 1.0f);
		aScoreText->SetAttribute(AttributeAlpha, 1.0f, 1.0f);
		aScoreText->SetAttribute(AttributeX, aRelativeX, aRelativeX);
		aScoreText->SetAttribute(AttributeY, aRelativeY + 15.0f, aRelativeY + 15.0f);
		aScoreText->AddStage(300.0f, 0, 0, 1, nullptr);
		aScoreText->SetAttribute(AttributeAlpha, 1.0f, 0);
		gActiveLevel->NewChild(aScoreText);
		Emitter* aCelebration = new Emitter();
		aCelebration->AddStage(1700.0f, 0, 0, 1, nullptr);
		aCelebration->mFrequency = 2.0f;
		aCelebration->mRandom = 0;
		aCelebration->mBlitStyle = 1;
		aCelebration->mMinX = aRelativeX - 100.0f;
		aCelebration->mMaxX = aRelativeX + 100.0f;
		aCelebration->mMinY = aRelativeY - 25.0f;
		aCelebration->mMaxY = aRelativeY + 25.0f;
		EmitterStage* aGrowStage = aCelebration->AddEmitterStage();
		aGrowStage->mDecay = 1.0f;
		aGrowStage->mFilterType = -1;
		aGrowStage->mGravity = 0;
		aGrowStage->mImage = AcquireImageRecord("gfx\\sparkle.png", 0, 0, 0, 0);
		aGrowStage->mMinDuration = 200.0f;
		aGrowStage->mMaxDuration = 400.0f;
		aGrowStage->mMinStartAlpha = 1.0f;
		aGrowStage->mMaxStartAlpha = 1.0f;
		aGrowStage->mMinEndAlpha = 1.0f;
		aGrowStage->mMaxEndAlpha = 1.0f;
		aGrowStage->mMinStartScale = 0;
		aGrowStage->mMaxStartScale = 0;
		aGrowStage->mMinEndScale = 1.0f;
		aGrowStage->mMaxEndScale = 1.0f;
		aGrowStage->mMinXSpeed = 0;
		aGrowStage->mMaxXSpeed = 0;
		aGrowStage->mMinYSpeed = 0;
		aGrowStage->mMaxYSpeed = 0;
		EmitterStage* aShrinkStage = aCelebration->AddEmitterStage();
		aShrinkStage->mDecay = 1.0f;
		aShrinkStage->mFilterType = -1;
		aShrinkStage->mGravity = 0;
		aShrinkStage->mImage = AcquireImageRecord("gfx\\sparkle.png", 0, 0, 0, 0);
		aShrinkStage->mMinDuration = 200.0f;
		aShrinkStage->mMaxDuration = 400.0f;
		aShrinkStage->mMinStartAlpha = 1.0f;
		aShrinkStage->mMaxStartAlpha = 1.0f;
		aShrinkStage->mMinEndAlpha = 1.0f;
		aShrinkStage->mMaxEndAlpha = 1.0f;
		aShrinkStage->mMinStartScale = 1.0f;
		aShrinkStage->mMaxStartScale = 1.0f;
		aShrinkStage->mMinEndScale = 0;
		aShrinkStage->mMaxEndScale = 0;
		aShrinkStage->mMinXSpeed = 0;
		aShrinkStage->mMaxXSpeed = 0;
		aShrinkStage->mMinYSpeed = 0;
		aShrinkStage->mMaxYSpeed = 0;
		gActiveLevel->NewChild(aCelebration);
		LoadAndPlaySound("audio\\plate_combo.ogg");
	}
	LoadAndPlaySound("audio\\plate.ogg");
}

// 0x413f90: a plate that sinks below the screen costs a try, one that sinks into the water raises the alarm
void Plate::CheckDanger()
{
	bool aScrolling = strcmp(gMiddleLayer->mStages.mCursor->mName, "scrolling") == 0;
	if (mWorld[AttributeY] > gScreenHeight && aScrolling)
		gActiveLevel->LoseTry();
	else if (CurrentProfile().mMode != GameModeJourney && gTuning.mEarthquakePoint + 50.0f < mWorld[AttributeY] &&
		aScrolling)
		WarnOfDanger();
}

// 0x414050: bubbles rise from a plate in the water, the screen shakes, and DANGER! flashes once
void Plate::WarnOfDanger()
{
	float aBubbleInterval = (gScreenHeight - mWorld[AttributeY]) / 4.0f;
	if (!(40.0f <= aBubbleInterval))
		aBubbleInterval = 40.0f;
	if ((int)mTickCount % (int)aBubbleInterval == 0) {
		SceneObject* aSplash = NewChild(nullptr);
		aSplash->SetImageRecord(gActiveLevel->mWhiteParticleImage);
		if (RandomBetween(0, 1) == 0)
			aSplash->mZOrder = gActiveLevel->FindChildByName("water", nullptr)->mZOrder + 0.001f;
		else
			aSplash->mZOrder = gActiveLevel->FindChildByName("water", nullptr)->mZOrder - 0.001f;
		gActiveLevel->AdoptNewestChild(this, true);
		memcpy(aSplash->mLocal, mLocal, sizeof(float) * AttributeCount);
		aSplash->mLocal[AttributeY] = gScreenHeight / 2.0f + 40.0f;
		float aSideOffset = (float)RandomBetween(-40, 40);
		aSplash->mLocal[AttributeX] = aSplash->mLocal[AttributeX] + aSideOffset;
		aSplash->AddStage(1200.0f, 0, 0, 1, nullptr);
		aSplash->SetAttribute(AttributeAlpha, 1.0f, 0);
		aSplash->SetAttribute(AttributeXScale, 3.0f, 3.0f);
		aSplash->SetAttribute(AttributeYScale, 3.0f, 3.0f);
		float aRotationEnd = (float)RandomFloat(-60.0f, 60.0f);
		aSplash->SetAttribute(AttributeRotation, (float)RandomFloat(-60.0f, 60.0f), aRotationEnd);
		aSplash->AddFilter(FilterVelocity, AttributeY, (float)(RandomFloat(-0.9f, -0.5f) / 2.0), -0.001f, 0);
		aSplash->AddFilter(FilterVelocity, AttributeX, aSideOffset / 1000.0f, 0, 0.999f);
		aSplash->mZOrder = mZOrder - 0.1f;
		if (RandomBetween(1, 3) == 1) {
			ImageRecord* aBubbleImage = AcquireImageRecord("gfx\\bubble.png", 0, 0, 0, 0);
			SceneObject* aBubble = NewChild(nullptr);
			gSparkleLayer->AdoptNewestChild(this, true);
			aBubble->SetImageRecord(aBubbleImage);
			aBubble->AddStage((float)RandomBetween(1000, 3000), 0, 0, 1, nullptr);
			aBubble->SetAttribute(AttributeAlpha, (float)RandomFloat(0.7f, 1.0f), 0);
			aBubble->mLocal[AttributeY] = gScreenHeight / 2.0f - 10.0f;
			aBubble->mLocal[AttributeX] = aBubble->mLocal[AttributeX] + aSideOffset;
			aBubble->AddFilter(FilterVelocity, AttributeY, (float)RandomFloat(-1.0f, -0.3f), 0.008f, 0);
			aBubble->AddFilter(FilterVelocity, AttributeX, (float)RandomFloat(-0.4f, 0.4f), 0, 0);
			aBubble->mZOrder = 9.0f;
		}
	}
	if (gTuning.mEarthquakePoint < mWorld[AttributeY])
		static_cast<Layer*>(gMiddleLayer)->Shake(2.0f, 0.125f);
	if (!sDangerWarningShown) {
		gActiveLevel->KillPopupTexts();
		sDangerWarningShown = true;
		TextObject* aDangerText = new TextObject();
		aDangerText->SetText("DANGER!", "gfx\\font1.png", 1.0f, 0, JustifyCenter);
		for (int aFlash = 0; aFlash < 4; aFlash++) {
			aDangerText->AddStage(300.0f, 0, 0, 1, nullptr);
			aDangerText->SetAttribute(AttributeX, 0, 0);
			aDangerText->AddStage(300.0f, 0, 0, 0, nullptr);
		}
		strcpy(aDangerText->mName, "popup_text");
		gActiveLevel->NewChild(aDangerText);
	}
}

// 0x414ab0
void Plate::SetUpSlots()
{
	for (int aSlotIndex = 0; aSlotIndex < mSlotCount; aSlotIndex++) {
		Slot* aSlot = CreateSlot(aSlotIndex);
		PositionSlot(aSlot, aSlotIndex);
		NewChild(aSlot);
	}
}

// 0x414b30
void Plate::PlaceSlot(Slot* theSlot, float theX, float theY)
{
	theSlot->AddStage(1000.0f, 1, 1, 1, nullptr);
	theSlot->SetAttribute(AttributeX, theX, theX);
	theSlot->SetAttribute(AttributeY, theY, theY);
}

void Plate::PlaceOnCircle(Slot* theSlot, float theAngle)
{
	double aCosine = VeniceCos(theAngle * DegreesToRadians);
	float aRadius = mRadius;
	double aSine = VeniceSin(theAngle * DegreesToRadians);
	PlaceSlot(theSlot, (float)(aCosine * (double)aRadius), (float)(aSine * (double)mRadius));
}

// 0x414b80
bool Plate::AllSlotsFilled()
{
	SceneObject* aSlot = nullptr;
	do {
		aSlot = FindChildByType(ObjectTypeSlot, aSlot);
		if (aSlot == nullptr)
			return true;
	} while (static_cast<Slot*>(aSlot)->mFilled != 0);
	return false;
}

// 0x414c60: slots of the same colour never come next to each other, and the last does not match the first
Slot* Plate::CreateSlot(int theIndex)
{
	Slot* aSlot = nullptr;
	int aFirstKind;
	do {
		delete aSlot;
		aSlot = new Slot();
		if (theIndex == 0)
			aFirstKind = aSlot->mKind;
		else
			aFirstKind = sFirstSlotKind;
		sFirstSlotKind = aFirstKind;
	} while ((aSlot->mKind == sPreviousSlotKind ||
				 (theIndex == mSlotCount - 1 && aSlot->mKind == aFirstKind && mSlotCount > 1)) &&
		gTuning.mNumberOfUniqueSlots > 1);
	sPreviousSlotKind = aSlot->mKind;
	return aSlot;
}

// 0x414df0
int Plate::CountFilledSlots()
{
	int aCount = 0;
	SceneObject* aSlot = nullptr;
	while ((aSlot = FindChildByType(ObjectTypeSlot, aSlot)) != nullptr)
		aCount = aCount + static_cast<Slot*>(aSlot)->mFilled;
	return aCount;
}
