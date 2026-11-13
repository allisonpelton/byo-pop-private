// The slots and the cascade links (0x41c040-0x420340).

#include "Slot.h"

#include "Emitter.h"
#include "Geometry.h"
#include "ImageRecord.h"
#include "Jewel.h"
#include "Layer.h"
#include "Level.h"
#include "Pigeon.h"
#include "Plate.h"
#include "Powerup.h"
#include "Profiles.h"
#include "SceneTag.h"
#include "Scoring.h"
#include "Shooter.h"
#include "TextObject.h"
#include "Tuning.h"
#include "VeniceGlobals.h"
#include "VeniceMath.h"
#include "VeniceSound.h"

#include <cstdio>
#include <cstring>

namespace
{
// The score of a filled slot and its label, which grow where the slot is, hold, and drift up a little as they fade. The offset
// is where they sit above the slot.
void AddPopupText(SceneObject* theSlot, const char* theText, float theOffset)
{
	TextObject* aText = new TextObject();
	aText->SetText(theText, "gfx\\font1_small.png", -1.0f, 0, JustifyCenter);
	float aX = theSlot->mWorld[AttributeX] - gActiveLevel->mWorld[AttributeX];
	float aY = theSlot->mWorld[AttributeY] - gActiveLevel->mWorld[AttributeY];
	aText->AddStage(200.0f, 0, 0, 1, nullptr);
	aText->SetAttribute(AttributeXScale, 0.1f, 1.3f);
	aText->SetAttribute(AttributeYScale, 1.3f, 1.3f);
	aText->SetAttribute(AttributeX, aX, aX);
	aText->SetAttribute(AttributeY, aY + theOffset, aY + theOffset);
	aText->SetAttribute(AttributeAlpha, 0, 1.0f);
	aText->AddFilter(FilterSquareRoot, AttributeAll, 0, 0, 0);
	aText->AddStage(50.0f, 0, 0, 1, nullptr);
	aText->SetAttribute(AttributeXScale, 1.3f, 1.0f);
	aText->SetAttribute(AttributeYScale, 1.3f, 1.0f);
	aText->SetAttribute(AttributeX, aX, aX);
	aText->SetAttribute(AttributeY, aY + theOffset, aY + theOffset);
	aText->AddStage(1000.0f, 0, 0, 1, nullptr);
	aText->SetAttribute(AttributeXScale, 1.0f, 1.0f);
	aText->SetAttribute(AttributeYScale, 1.0f, 1.0f);
	aText->SetAttribute(AttributeX, aX, aX);
	aText->SetAttribute(AttributeY, aY + theOffset, (aY - 60.0f) + theOffset);
	aText->SetAttribute(AttributeAlpha, 1.0f, 0);
	aText->AddFilter(FilterSquared, AttributeAll, 0, 0, 0);
	gActiveLevel->NewChild(aText);
}

// One of the two lines of the super tumble message (the second waits 300 ms and then comes in a little wider)
void AddSuperTumbleText(SceneObject* theSlot, const char* theText, float theOffset, float theStartScale, bool theDelayed)
{
	TextObject* aText = new TextObject();
	aText->SetText(theText, "gfx\\font1.png", -1.0f, 0, JustifyCenter);
	float aX = theSlot->mWorld[AttributeX] - gActiveLevel->mWorld[AttributeX];
	float aY = theSlot->mWorld[AttributeY] - gActiveLevel->mWorld[AttributeY];
	if (theDelayed)
		aText->AddStage(300.0f, 0, 0, 0, nullptr);
	aText->AddStage(200.0f, 0, 0, 1, nullptr);
	aText->SetAttribute(AttributeXScale, theStartScale, 0.95f);
	aText->SetAttribute(AttributeYScale, 0.75f, 0.75f);
	aText->SetAttribute(AttributeX, aX, aX);
	aText->SetAttribute(AttributeY, aY + theOffset, aY + theOffset);
	aText->SetAttribute(AttributeAlpha, 0, 1.0f);
	aText->AddFilter(FilterSquareRoot, AttributeAll, 0, 0, 0);
	aText->AddStage(50.0f, 0, 0, 1, nullptr);
	aText->SetAttribute(AttributeXScale, 0.75f, 0.65f);
	aText->SetAttribute(AttributeYScale, 0.75f, 0.65f);
	aText->SetAttribute(AttributeX, aX, aX);
	aText->SetAttribute(AttributeY, aY + theOffset, aY + theOffset);
	aText->AddStage(1500.0f, 0, 0, 1, nullptr);
	aText->SetAttribute(AttributeXScale, 0.65f, 0.65f);
	aText->SetAttribute(AttributeYScale, 0.65f, 0.65f);
	aText->SetAttribute(AttributeX, aX, aX);
	aText->SetAttribute(AttributeY, aY + theOffset, aY + theOffset);
	aText->SetAttribute(AttributeAlpha, 1.0f, 0);
	aText->AddFilter(FilterSquared, AttributeAll, 0, 0, 0);
	strcpy(aText->mName, "popup_text");
	gActiveLevel->NewChild(aText);
}

// One stage of the sparkles that rain on the super tumble message: they live 200 to 400 ms, stay opaque, and grow from nothing to
// full size or shrink from full size to nothing
void AddSparkleStage(Emitter* theEmitter, bool theGrowing)
{
	EmitterStage* aStage = theEmitter->AddEmitterStage();
	aStage->mDecay = 1.0f;
	aStage->mGravity = 0;
	aStage->mFilterType = -1;
	aStage->mImage = AcquireImageRecord("gfx\\sparkle.png", 0, 0, 0, 0);
	aStage->mMinDuration = 200.0f;
	aStage->mMaxDuration = 400.0f;
	aStage->mMinStartAlpha = 1.0f;
	aStage->mMaxStartAlpha = 1.0f;
	aStage->mMinEndAlpha = 1.0f;
	aStage->mMaxEndAlpha = 1.0f;
	aStage->mMinStartScale = theGrowing ? 0.0f : 1.0f;
	aStage->mMaxStartScale = theGrowing ? 0.0f : 1.0f;
	aStage->mMinEndScale = theGrowing ? 1.0f : 0.0f;
	aStage->mMaxEndScale = theGrowing ? 1.0f : 0.0f;
	aStage->mMinXSpeed = 0;
	aStage->mMaxXSpeed = 0;
	aStage->mMinYSpeed = 0;
	aStage->mMaxYSpeed = 0;
}
}

Slot::Slot()
{
	mType = ObjectTypeSlot;
	mKind = ChooseKind();
	mFilled = 0;
	mZOrder = 2.0f;
	mSnap = 0;
	mLinked = false;
	mTossSearches = 0;
	mSlotsFilled = 0;
	mPlatesCompleted = 0;
	mSpin = 0;
	ShowKind();
}

// A slot that goes takes the cascade lines that run to it along
Slot::~Slot()
{
	if (gPlateLayer != nullptr) {
		SceneObject* aLink = nullptr;
		while ((aLink = gPlateLayer->FindChildByType(ObjectTypeCascadeLink, aLink)) != nullptr) {
			if (static_cast<CascadeLink*>(aLink)->mSlot == this)
				aLink->mIsDead = true;
		}
	}
}

// The colour of a new slot: any of the colours the level has (3 and up), and one or two more once the player is on a later level
int Slot::ChooseKind()
{
	int aExtraKinds = 0;
	int aLevelIndex = CurrentProfile().mLevelIndex[CurrentProfile().mMode];
	if (aLevelIndex < 29) {
		if (10 < aLevelIndex)
			aExtraKinds = 1;
	} else {
		aExtraKinds = 2;
	}
	int aKind;
	if (gTuning.mNumberOfUniqueSlots == 1)
		aKind = 5;
	else
		aKind = RandomBetween(3, gTuning.mNumberOfUniqueSlots + 2 + aExtraKinds);
	return aKind;
}

void Slot::Load(const char* theText, SceneTag* theTag)
{
	mKind = theTag->GetInt("number");
	mFilled = 0;
	mZOrder = 2.0f;
	mSnap = 0;
	SceneObject::Load(theText, theTag);
	// the objects before it that cannot be selected are taken out of the parent's list
	while (mPrev != nullptr && mPrev->mSelectable == 0) {
		mParent->mChildren.mCursor = mPrev;
		mParent->mChildren.RemoveCursor();
	}
}

void Slot::ShowKind()
{
	if (mKind < 51) {
		char aName[256];
		sprintf(aName, "gfx\\slot%d.png", mKind);
		SetImage(aName, 0, 0, 0, 0);
	}
}

bool Slot::Update()
{
	if (mIsDead)
		return false;
	mTossSearches = 0;
	// the original puts the arc tangent, which is in radians, in the place of the rotation, which is in degrees
	mLocal[AttributeRotation] = (float)VeniceArcTangent(mLocal[AttributeX], mLocal[AttributeY]);
	SceneObject::Update();
	Jewel* aJewel = nullptr;
	while ((aJewel = static_cast<Jewel*>(gPlateLayer->FindChildByType(ObjectTypeJewel, aJewel))) != nullptr)
		CheckJewel(aJewel);
	return true;
}

// A ring of light that fades round the slot: the picture turns slowly for four times as long as it fades
void Slot::Glow(float theDuration, float theScale, const char* theImageName)
{
	SceneObject* aGlow = NewChild(nullptr);
	if (theImageName == nullptr)
		aGlow->SetImage("gfx\\corona.png", 0, 0, 0, 0);
	else
		aGlow->SetImage(theImageName, 0, 0, 0, 0);
	mParent->AdoptNewestChild(this, true);
	memcpy(aGlow->mLocal, mLocal, sizeof(mLocal));
	aGlow->AddStage(theDuration * 4.0f, 0, 0, 1, nullptr);
	aGlow->SetAttribute(AttributeXScale, theScale, theScale);
	aGlow->SetAttribute(AttributeYScale, theScale, theScale);
	aGlow->AddFilter(FilterVelocity, AttributeRotation, -0.6f, 0, 1.0f);
	aGlow->AddStage(theDuration, 0, 0, 1, nullptr);
	aGlow->SetAttribute(AttributeAlpha, 1.0f, 0);
	aGlow->AddFilter(FilterVelocity, AttributeRotation, -0.6f, 0, 1.0f);
	aGlow->mZOrder = mZOrder - 0.1f;
}

// A jewel that comes to the slot. A jewel of the slot's colour (or a wild one) that reaches an open slot fills it. When the game's
// tuning has full slots block the jewels, a jewel that a shot throws against a full slot bounces off it instead, and the jewel
// goes on flying until it is clear.
void Slot::CheckJewel(Jewel* theJewel)
{
	if (theJewel == nullptr)
		return;
	if (gTuning.mBlockingSlots == 0 || mFilled == 0 || theJewel->mPowerShot != 0 || theJewel->mShot == 0 || 99 < theJewel->mKind) {
		if ((theJewel->mKind == mKind || theJewel->mKind == WildJewelKind) && mFilled == 0) {
			// each picture counts as a circle as wide as the larger of its sides
			float aSlotRadius;
			if (mImage->mFrameWidth / 2.0f <= mImage->mFrameHeight / 2.0f)
				aSlotRadius = mImage->mFrameHeight;
			else
				aSlotRadius = mImage->mFrameWidth;
			aSlotRadius = aSlotRadius / 2.0f;
			float aJewelRadius;
			if (theJewel->mImage->mFrameWidth / 2.0f <= theJewel->mImage->mFrameHeight / 2.0f)
				aJewelRadius = theJewel->mImage->mFrameHeight;
			else
				aJewelRadius = theJewel->mImage->mFrameWidth;
			aJewelRadius = aJewelRadius / 2.0f;
			float anOverlap = CircleOverlap(theJewel->mWorld[AttributeX], theJewel->mWorld[AttributeY], aJewelRadius,
				mWorld[AttributeX], mWorld[AttributeY], aSlotRadius);
			if (anOverlap != 0.0f && HitTest(gScreenWidth / 2.0f, gScreenHeight / 2.0f, gScreenWidth, gScreenHeight, 0.0f)) {
				FillSlot(theJewel, false);
				theJewel->mSlotsFilled++;
				mSlotsFilled = theJewel->mSlotsFilled;
				Plate* aPlate = static_cast<Plate*>(mParent);
				int aFilledCount = aPlate->CountFilledSlots();
				if (aPlate->mSlotCount <= aFilledCount && theJewel->mKind < 100) {
					theJewel->mPlatesCompleted++;
					mPlatesCompleted = theJewel->mPlatesCompleted;
					aPlate->mTumbleLevel = mPlatesCompleted;
				}
				// the jewel's sideways speed goes to the jewel the slot throws back out (a jewel that has none gets a random one)
				mSpin = theJewel->GetXVelocity();
				if (VeniceAbs(mSpin) < 0.001f)
					mSpin = (float)RandomFloat(-1.5f, 1.5f);
				if (theJewel->mComboShot != 0)
					Cascade(true);
				if (theJewel->mShot == 0) {
					gActiveLevel->mCombos++;
				} else {
					aPlate->OnJewelHit(this, theJewel);
					if (theJewel->mSlotsFilled == 1)
						gActiveLevel->mHits++;
				}
				if (theJewel->mKind < 100 || theJewel->mShot == 0)
					theJewel->Kill();
			}
		}
	} else {
		float aJewelRadius = theJewel->mImage->mFrameWidth / 2.5f;
		float aSlotRadius = mImage->mFrameWidth / 2.2f;
		float anOverlap = CircleOverlap(theJewel->mWorld[AttributeX], theJewel->mWorld[AttributeY], aJewelRadius,
			mWorld[AttributeX], mWorld[AttributeY], aSlotRadius);
		if (anOverlap != 0.0f && !Overlaps(gShooter) && theJewel->mLastSlot != this &&
			strstr(mParent->mStages.mCursor->mName, "fade") == nullptr) {
			theJewel->mLastSlot = this;
			if (gTuning.mPlateMoveWhenFullSlotHit != 0 && theJewel->mShot != 0)
				static_cast<Plate*>(mParent)->OnJewelHit(this, theJewel);
			SceneFilter* aVelocityX = theJewel->mStages.mCursor->FindFilter(FilterVelocity, AttributeX);
			SceneFilter* aVelocityY = theJewel->mStages.mCursor->FindFilter(FilterVelocity, AttributeY);
			if (aVelocityX != nullptr && aVelocityY != nullptr) {
				ReflectOffBall(&aVelocityX->mAmplitude, &aVelocityY->mAmplitude, theJewel->mWorld[AttributeX],
					theJewel->mWorld[AttributeY], mWorld[AttributeX], mWorld[AttributeY]);
				// the jewel flies on, update by update, until it is clear of the slot (or has gone, or 120 updates have passed)
				int aSteps = 0;
				do {
					if (4 < theJewel->mAge) {
						theJewel->mBounces++;
						theJewel->mLastSlot = nullptr;
						theJewel->mAge = 0;
					}
					aSteps++;
					theJewel->Update();
				} while (!theJewel->mIsDead && aSteps < 120 &&
					CircleOverlap(theJewel->mWorld[AttributeX], theJewel->mWorld[AttributeY], aJewelRadius, mWorld[AttributeX],
						mWorld[AttributeY], aSlotRadius) != 0.0f);
				PlaySoundByName("audio\\toss.ogg", -1);
			}
		}
	}
}

// The slot is filled: by a jewel, or (with no jewel) by a cascade. The slot gets the picture of a filled one, flashes and
// sparkles, and the plate speeds up its turn. A jewel's fill sets the scroll back (and in the Flood mode a combo does too) and
// shows the score that goes with the fill.
void Slot::FillSlot(Jewel* theJewel, bool theCascaded)
{
	gShooter->mCupidWait = 0;
	gActiveLevel->mComboTicksLeft = 300;
	if (theJewel == nullptr || theJewel->mSlotsFilled == 0) {
		LoadAndPlaySound("audio\\place.ogg", -1);
		LoadAndPlaySound("audio\\tone.ogg", gActiveLevel->mComboCount * 3000 * (theCascaded ? 1 : 0) + 0x5622);
	} else {
		// each slot that the same jewel fills sounds higher
		LoadAndPlaySound("audio\\place.ogg", -1);
		LoadAndPlaySound("audio\\tone.ogg", theJewel->mSlotsFilled * 3000 + 0x5622);
	}
	Plate* aPlate = static_cast<Plate*>(mParent);
	SceneStage* aMiddleStage = gMiddleLayer->mStages.mCursor;
	if (!theCascaded) {
		int aFilledCount = aPlate->CountFilledSlots();
		if (aFilledCount < aPlate->mSlotCount - 1 || aPlate->mSlotCount == 1) {
			if (theJewel != nullptr && theJewel->mShot == 0 && strstr(gActiveLevel->mName, "trick") == nullptr &&
				gTutorial == nullptr && CurrentProfile().mMode == GameModeFlood) {
				aMiddleStage->AddFilter(FilterVelocity, AttributeY, gTuning.mPushOnCombo, 0, 0.925f, true);
				aMiddleStage->mFrame = 0;
			}
			if (theJewel != nullptr && strstr(gActiveLevel->mName, "trick") == nullptr && gTutorial == nullptr &&
				CurrentProfile().mMode != GameModeTrickShot && CurrentProfile().mMode != GameModeJourney) {
				aMiddleStage->AddFilter(FilterVelocity, AttributeY, gTuning.mPushOnFilledSlot, 0, 0.985f, true);
				aMiddleStage->mFrame = 0;
			}
			ShowTumblePopup(theJewel);
		}
	} else if (gTutorial == nullptr && CurrentProfile().mMode == GameModeFlood) {
		aMiddleStage->AddFilter(FilterVelocity, AttributeY, gTuning.mPushOnCombo, 0, 0.925f, true);
		aMiddleStage->mFrame = 0;
	}
	if (theJewel != nullptr && theJewel->mSlotsFilled == 2 && gTutorial == nullptr)
		Powerup::SpawnFrom(this);

	// the pigeons near it fly off
	SceneObject* aPigeonLayer = gPlateLayer->FindChildByType(ObjectTypeLayer, nullptr);
	Pigeon* aPigeon = nullptr;
	while ((aPigeon = static_cast<Pigeon*>(aPigeonLayer->FindChildByType(ObjectTypePigeon, aPigeon))) != nullptr)
		aPigeon->Scare(mWorld[AttributeX], mWorld[AttributeY]);

	mFilled = 1;
	if (gShooter != nullptr && gShooter->mTargetSlot == this)
		gShooter->mTargetSlot = nullptr;
	if (mKind == WildJewelKind)
		mKind = 5;
	char anOutlineName[256];
	sprintf(anOutlineName, "gfx\\outline%d.png", mKind);
	SetImage(anOutlineName, 0, 0, 0, 0);
	mTickCount = 0;

	// it shakes for 150 ms and bursts into sparkles, and again without the burst
	mStages.Clear();
	AddStage(150.0f, 0, 0, 1, "filling");
	SetAttribute(AttributeX, mLocal[AttributeX], mLocal[AttributeX]);
	SetAttribute(AttributeY, mLocal[AttributeY], mLocal[AttributeY]);
	SetAttribute(AttributeXScale, 1.0f, 1.0f);
	SetAttribute(AttributeYScale, 1.0f, 1.0f);
	if (theJewel == nullptr || (theJewel->mComboShot == 0 && theJewel->mKind < 100))
		AddEvent(TriggerEnd, ActionCustom, nullptr, nullptr, nullptr, nullptr, nullptr);
	AddEvent(TriggerEnd, ActionMakeSparkles, "20", nullptr, nullptr, nullptr, nullptr);
	AddFilter(FilterDecaySined, AttributeX, 10.0f, 2.0f, 0);
	AddFilter(FilterDecaySined, AttributeY, 10.0f, 3.0f, 0);
	AddStage(150.0f, 0, 0, 1, "filling");
	SetAttribute(AttributeX, mLocal[AttributeX], mLocal[AttributeX]);
	SetAttribute(AttributeY, mLocal[AttributeY], mLocal[AttributeY]);
	SetAttribute(AttributeXScale, 1.0f, 1.0f);
	SetAttribute(AttributeYScale, 1.0f, 1.0f);
	AddFilter(FilterDecaySined, AttributeX, 10.0f, 2.0f, 0);
	AddFilter(FilterDecaySined, AttributeY, 10.0f, 3.0f, 0);
	AddStage(1000.0f, 1, 1, 1, nullptr);
	MakeSparkles(20, 1.0f, 1.0f, 1.0f, 1.0f);

	// four lights go round it
	Layer* aRing = new Layer();
	NewChild(aRing);
	aRing->AddStage(1800.0f, 0, 0, 1, nullptr);
	aRing->AddFilter(FilterVelocity, AttributeRotation, -1.1f, 0, 0);
	aRing->AddStage(200.0f, 0, 0, 1, nullptr);
	aRing->SetAttribute(AttributeAlpha, 1.0f, 0);
	aRing->AddFilter(FilterVelocity, AttributeRotation, -1.1f, 0, 0);
	for (float anAngle = 0.0f; anAngle < 360.0f; anAngle = anAngle + 90.0f) {
		SceneObject* aLight = aRing->NewChild(nullptr);
		aLight->SetImageRecord(gActiveLevel->mSparkleImage);
		aLight->AddStage(100.0f, 1, 1, 1, nullptr);
		float aLightX = (float)(VeniceCos(anAngle * DegreesToRadians) * 48.0);
		aLight->SetAttribute(AttributeX, aLightX, aLightX);
		float aLightY = (float)(VeniceSin(anAngle * DegreesToRadians) * 48.0);
		aLight->SetAttribute(AttributeY, aLightY, aLightY);
		aLight->SetAttribute(AttributeXScale, 0.65f, 1.35f);
		aLight->SetAttribute(AttributeYScale, 0.65f, 1.35f);
		aLight->SetAttribute(AttributeAlpha, 0.3f, 0);
		aLight->mBlitStyle = 1;
	}
	SetZOrder(mParent->mChildren.mTail->mZOrder + 0.0001f);
	aPlate->OnSlotFilled();
}

// The message that goes with the score of a jewel's fill: nothing but 10 points for the first fill of a jewel that has not
// bounced, BANK SHOT for one that has, Tumble for each slot that a thrown-back jewel fills after the first (the third and up is
// the super tumble), and LUCKY SHOT for a jewel that has bounced four times or more. The four wrappers of the popup 0x41d600 that
// say these (0x41de30 LUCKY SHOT, 0x41eaa0 Tumble, 0x41eae0 BANK SHOT, 0x41eb00 the empty one) are translated inline.
void Slot::ShowTumblePopup(Jewel* theJewel)
{
	if (theJewel == nullptr)
		return;
	if (theJewel->mBounces < 4) {
		if (theJewel->mSlotsFilled == 0 || 99 < theJewel->mKind) {
			if (theJewel->mBounces < 1) {
				ShowPopup(10, " ");
			} else {
				theJewel->mBounces = 1;
				ShowPopup(theJewel->mBounces * 500, "BANK SHOT!");
			}
		} else {
			int aPoints = theJewel->mSlotsFilled * 100;
			if (theJewel->mSlotsFilled < 3) {
				char aLabel[16];
				sprintf(aLabel, "Tumble x%d", aPoints / 100);
				ShowPopup(aPoints, aLabel);
			} else {
				ShowSuperTumble(aPoints);
			}
		}
	} else {
		ShowPopup(1000, "LUCKY SHOT!");
	}
}

// The label of a fill and its score under it, above the slot (lower when the slot is near the top of the screen)
void Slot::ShowPopup(int thePoints, const char* theLabel)
{
	float anOffset = -60.0f;
	if (mWorld[AttributeY] < 90.0f)
		anOffset = (90.0f - mWorld[AttributeY]) + -60.0f;
	AddPopupText(this, theLabel, anOffset);
	anOffset = anOffset + 25.0f;
	AddPopupText(this, FormatScore(thePoints), anOffset);
}

// SUPER TUMBLE! and the score, with a rain of sparkles over both (any message that is still on the screen goes first)
void Slot::ShowSuperTumble(int thePoints)
{
	float anOffset = -60.0f;
	if (mWorld[AttributeY] < 80.0f)
		anOffset = 0.0f;
	gActiveLevel->KillPopupTexts();
	AddSuperTumbleText(this, "SUPER TUMBLE!", anOffset, 0.0325f, false);
	anOffset = anOffset + 25.0f;
	AddSuperTumbleText(this, FormatScore(thePoints), anOffset, 0.065f, true);

	float aX = mWorld[AttributeX] - gActiveLevel->mWorld[AttributeX];
	float aY = mWorld[AttributeY] - gActiveLevel->mWorld[AttributeY];
	Emitter* aSparkles = new Emitter();
	aSparkles->AddStage(1000.0f, 0, 0, 1, nullptr);
	aSparkles->mFrequency = 2.0f;
	aSparkles->mRandom = 0;
	aSparkles->mBlitStyle = 1;
	aSparkles->mMinX = aX - 120.0f;
	aSparkles->mMaxX = aX + 120.0f;
	aSparkles->mMinY = (aY + anOffset) - 40.0f;
	aSparkles->mMaxY = aY + anOffset + 10.0f;
	aSparkles->mParticlesPerEmit = 1;
	strcpy(aSparkles->mName, "popup_text");
	AddSparkleStage(aSparkles, true);
	AddSparkleStage(aSparkles, false);
	gActiveLevel->NewChild(aSparkles);
	LoadAndPlaySound("audio\\super_tumble.ogg", -1);
}

// The sideways speed that takes a jewel thrown out of this slot to the nearest slot of its colour below it, and that slot in
// theTarget (this slot itself when none is near enough). A speed that is too small is replaced by a random one, and with the
// tuning's toss on a filled plate off the jewel is thrown straight down.
float Slot::FindTossTarget(float theXVelocity, Slot** theTarget)
{
	float aVelocityX = theXVelocity;
	while (VeniceAbs(aVelocityX) < 0.3f)
		aVelocityX = (float)RandomFloat(-2.0f, 2.0f);
	if (gTuning.mTossOnFilledPlate == 0)
		aVelocityX = 0.0f;
	Slot* aBestSlot = this;
	for (SceneObject* aPlate = mParent->mParent->mChildren.mHead; aPlate != nullptr; aPlate = aPlate->mNext) {
		for (SceneObject* aCandidate = aPlate->mChildren.mHead; aCandidate != nullptr; aCandidate = aCandidate->mNext) {
			if (aCandidate == this || aCandidate->mType != ObjectTypeSlot)
				continue;
			Slot* aSlot = static_cast<Slot*>(aCandidate);
			if (aBestSlot != this) {
				// only a slot that is nearer than the best one so far
				float aCandidateDistance = (float)VeniceAbs((mWorld[AttributeX] - aSlot->mWorld[AttributeX]) *
						(mWorld[AttributeX] - aSlot->mWorld[AttributeX]) +
					(mWorld[AttributeY] - aSlot->mWorld[AttributeY]) * (mWorld[AttributeY] - aSlot->mWorld[AttributeY]));
				double aBestDistance = VeniceAbs((mWorld[AttributeX] - aBestSlot->mWorld[AttributeX]) *
						(mWorld[AttributeX] - aBestSlot->mWorld[AttributeX]) +
					(mWorld[AttributeY] - aBestSlot->mWorld[AttributeY]) * (mWorld[AttributeY] - aBestSlot->mWorld[AttributeY]));
				if (aBestDistance <= aCandidateDistance)
					continue;
			}
			if (VeniceAbs(mWorld[AttributeX] - aSlot->mWorld[AttributeX]) < gScreenWidth / 2.0f && aSlot->mKind == mKind &&
				aSlot->mFilled == 0 && mWorld[AttributeY] + 5.0f < aSlot->mWorld[AttributeY] &&
				aSlot->mTossSearches <= aBestSlot->mTossSearches) {
				float aSign = 0.0f <= aSlot->mWorld[AttributeX] - mWorld[AttributeX] ? 1.0f : -1.0f;
				float aDistanceX = (float)VeniceAbs(aSlot->mWorld[AttributeX] - mWorld[AttributeX]);
				double aRoot = VeniceSqrt((float)VeniceAbs(aSlot->mWorld[AttributeY] - mWorld[AttributeY]));
				float aVelocity = (float)((((double)aDistanceX / aRoot) * (double)aSign) / 13.0);
				if (VeniceAbs(aVelocity) < 2.5f) {
					aBestSlot = aSlot;
					aVelocityX = aVelocity;
				}
			}
		}
	}
	if (aBestSlot != this)
		aBestSlot->mTossSearches++;
	*theTarget = aBestSlot;
	return aVelocityX;
}

// The jewel of the filled slot is thrown out again (the slot stays filled): it flies up and to the side on the same kind of
// curve that a shot does, and falls back to land on a slot of its colour, when there is a plate with a slot to fill. A free
// toss (the tosser plate's) keeps the speed it is given, is not aimed at a slot and is bounced by the bumpers. A toss that finds
// no way to the side is a miss for a trick shot, and (when the plates cannot be completed any more) a lost try.
void Slot::TossJewel(int theSlotsFilled, int thePlatesCompleted, float theXVelocity, bool theFreeToss)
{
	UpdateWorldValues();
	Slot* aTarget = nullptr;
	float aVelocityX = FindTossTarget(theXVelocity, &aTarget);
	if (aVelocityX == 0.0f && !theFreeToss) {
		Plate* aPlate = static_cast<Plate*>(mParent);
		int aFilledCount = aPlate->CountFilledSlots();
		if (aFilledCount < aPlate->mSlotCount || 1 < gPlateLayer->CountChildrenOfType(ObjectTypePlate))
			gActiveLevel->MissTrickShot();
	} else {
		if (theFreeToss)
			aVelocityX = theXVelocity;
		if (AnyPlateIncomplete()) {
			Jewel* aJewel = new Jewel(mKind);
			NewChild(aJewel);
			aJewel->mStages.Clear();
			aJewel->AddStage(500.0f, 1, 1, 1, "shooting");
			aJewel->AddFilter(FilterVelocity, AttributeY, -0.75f, 0.02f, 0);
			aJewel->AddFilter(FilterVelocity, AttributeX, aVelocityX, 0, 0);
			aJewel->mShot = 0;
			aJewel->mBumpable = theFreeToss ? 1 : 0;
			mParent->mParent->AdoptNewestChild(this, true);
			aJewel->SetZOrder(1.001f);
			aJewel->mSlotsFilled = theSlotsFilled;
			aJewel->mPlatesCompleted = thePlatesCompleted;
			if (!theFreeToss)
				aJewel->mTarget = aTarget;
			// the jewel has a picture of its own colour on a corona
			SceneObject* aGem = aJewel->NewChild(nullptr);
			aGem->AddStage(1000.0f, 1, 1, 1, nullptr);
			aGem->SetImageRecord(aJewel->mImage);
			aJewel->SetImage("gfx\\corona4.png", 0, 0, 0, 0);
			if (gTuning.mTossOnFilledPlate == 0) {
				LoadAndPlaySound("audio\\pop.ogg", -1);
				SceneObject* aBurst = NewChild(nullptr);
				aBurst->mZOrder = 30.0f;
				aBurst->SetImage("gfx\\burst.png", 0, 0, 0, 0);
				aBurst->AddStage(60.0f, 0, 0, 1, nullptr);
				aBurst->SetAttribute(AttributeAlpha, 0.85f, 0.85f);
				aBurst->SetAttribute(AttributeXScale, 0.3f, 0.3f);
				aBurst->SetAttribute(AttributeYScale, 0.3f, 0.3f);
			}
			aJewel->mStartX = mWorld[AttributeX];
			aJewel->mStartY = mWorld[AttributeY];
		}
	}
}

// The slots of the colour near a filled one fill too, one after another: each gets a line from this slot that carries the
// fill. Without the tuning's cascading combos only a combo shot cascades.
void Slot::Cascade(bool theCascaded)
{
	if ((gTuning.mCascadingCombos != 0 || theCascaded) && mFilled != 0 && mTickCount < 300.0f) {
		int aDelay = 0;
		SceneObject* aPlate = nullptr;
		while ((aPlate = gPlateLayer->FindChildByType(ObjectTypePlate, aPlate)) != nullptr) {
			SceneObject* aCandidate = nullptr;
			while ((aCandidate = aPlate->FindChildByType(ObjectTypeSlot, aCandidate)) != nullptr) {
				Slot* aSlot = static_cast<Slot*>(aCandidate);
				// the slots of one plate must be nearer than the others
				float aRadius = aSlot->mParent == mParent ? 30.0f : 52.0f;
				if (!aSlot->mLinked && aSlot != this && aSlot->mFilled == 0 && mKind == aSlot->mKind &&
					aSlot->HitTest(gScreenWidth / 2.0f, gScreenHeight / 2.0f, gScreenWidth, gScreenHeight, 0.0f) &&
					(theCascaded || CircleOverlap(mWorld[AttributeX], mWorld[AttributeY], aRadius, aSlot->mWorld[AttributeX],
						aSlot->mWorld[AttributeY], aRadius) != 0.0f)) {
					CascadeLink* aLink = new CascadeLink(this, aSlot, aDelay * (theCascaded ? 1 : 0));
					aLink->mZOrder = mParent->mZOrder + 0.01f;
					gPlateLayer->NewChild(aLink);
					aDelay = aDelay + 10;
				}
			}
		}
		if ((int)mTickCount % 100 == 0)
			Glow(100.0f, 1.0f, nullptr);
	}
}

// The "filling" stage's end: a slot that was not filled by a cascade throws its jewel back out, when the tuning says it does
void Slot::OnCustomEvent()
{
	if (gTuning.mTossOnFilledSlot != 0 && !mLinked)
		TossJewel(mSlotsFilled, mPlatesCompleted, mSpin, false);
}

// The line starts at the slot that was filled and grows to the slot that is to be filled. It is invisible for the delay that
// puts the cascade's slots in their order, then runs along for as long as the distance takes, then waits a moment.
CascadeLink::CascadeLink(Slot* theSource, Slot* theTarget, int theDelay)
{
	mType = ObjectTypeCascadeLink;
	mStartX = theSource->mWorld[AttributeX];
	mStartY = theSource->mWorld[AttributeY];
	mSlot = theTarget;
	mZOrder = 2.01f;
	SetImageRecord(theSource->mImage);
	mSlot->mLinked = true;
	float aDelay = (float)theDelay + 100.0f;
	AddStage(aDelay, 0, 0, 0, nullptr);
	AddStage((float)(theSource->DistanceTo(theTarget) * 1.5f), 0, 0, 1, nullptr);
	SetAttribute(AttributeAlpha, 1.0f, 1.0f);
	SetAttribute(AttributeXScale, 1.0f, 1.0f);
	SetAttribute(AttributeYScale, 1.0f, 1.0f);
	mTickCount = 1.0f;
	AddStage(120.0f, 0, 0, 0, nullptr);
	// the combo lasts until the line has done its work, and counts one more
	int aEndTick = (int)aDelay + 111;
	gActiveLevel->mComboTicksLeft = aEndTick < gActiveLevel->mComboTicksLeft ? gActiveLevel->mComboTicksLeft : aEndTick;
	gActiveLevel->mComboCount++;
	mSlot->Glow(aDelay, 1.0f, nullptr);
	theSource->Glow(aDelay, 1.0f, nullptr);
}

// The line moves from its start to the slot as its stage goes by, turning; halfway it shows the score of the cascade, and at the
// end it fills the slot. A slot that was filled meanwhile by a jewel ends the line.
bool CascadeLink::Update()
{
	bool anAlive;
	if (mSlot->mFilled == 0 && !mIsDead) {
		anAlive = SceneObject::Update();
		SceneStage* aStage = mStages.mCursor;
		if (aStage != nullptr && aStage->mVisible) {
			mWorld[AttributeX] = (mSlot->mWorld[AttributeX] - mStartX) * aStage->mProgress + mStartX;
			mWorld[AttributeY] = (mSlot->mWorld[AttributeY] - mStartY) * aStage->mProgress + mStartY;
			float aDirection = 0.0f <= mStartX - mSlot->mWorld[AttributeX] ? 1.0f : -1.0f;
			mLocal[AttributeRotation] = aDirection * 4.5f + mLocal[AttributeRotation];
			if (VeniceAbs(aStage->mFrame - aStage->mLength / 2.0f) < 0.51f) {
				snprintf(mName, sizeof(mName), "%s", FormatScore(gActiveLevel->mComboCount * 100));
				TextObject* aScoreText = new TextObject();
				aScoreText->SetText(mName, "gfx\\font1.png", 1.0f, 0, JustifyCenter);
				aScoreText->AddStage(900.0f, 0, 0, 1, nullptr);
				float aMiddleX = (mSlot->mWorld[AttributeX] + mStartX) / 2.0f;
				float aMiddleY = (mSlot->mWorld[AttributeY] + mStartY) / 2.0f;
				aScoreText->SetAttribute(AttributeXScale, 0.75f, 1.0f);
				aScoreText->SetAttribute(AttributeYScale, 0.75f, 1.0f);
				aScoreText->SetAttribute(AttributeX, aMiddleX - gActiveLevel->mWorld[AttributeX],
					aMiddleX - gActiveLevel->mWorld[AttributeX]);
				aScoreText->SetAttribute(AttributeY, aMiddleY - gActiveLevel->mWorld[AttributeY],
					(aMiddleY - gActiveLevel->mWorld[AttributeY]) - 60.0f);
				aScoreText->SetAttribute(AttributeAlpha, 1.0f, 0);
				aScoreText->AddFilter(FilterSined, AttributeY, 5.0f, 1.0f, 0);
				aScoreText->AddFilter(FilterSquareRoot, AttributeAll, 1.0f, 0, 0);
				gActiveLevel->NewChild(aScoreText);
				gActiveLevel->mCombos++;
			}
			if (aStage->mLength <= aStage->mFrame)
				mSlot->FillSlot(nullptr, true);
		}
	} else {
		anAlive = false;
	}
	return anAlive;
}
