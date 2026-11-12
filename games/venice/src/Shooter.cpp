// The shooter (0x419790-0x41bad0): the jewels in its row, the shot, the power-ups it carries and the colours it loads.

#include "Shooter.h"

#include "Cupid.h"
#include "ImageRecord.h"
#include "Jewel.h"
#include "Layer.h"
#include "Level.h"
#include "Plate.h"
#include "Profiles.h"
#include "Slot.h"
#include "Tuning.h"
#include "VeniceApp.h"
#include "VeniceGlobals.h"
#include "VeniceInput.h"
#include "VeniceMath.h"
#include "VeniceSound.h"

#include <cstring>

namespace
{
const int SlotKindCount = 11; // the slot colours 3 to 10 (a slot's colour is its index here)

// 0x5ba3f4 and 0x5ba420: how many slots of each colour are still to fill, and how many colours there are
int sSlotKindCounts[SlotKindCount] = {};
int sColourCount = 0;
// 0x5ba424: the scene that was on top when the shooter last put the pointer under itself
SceneObject* sLastTrackedScene = nullptr;

// 0x41aeb0: counts the unfilled slots of each colour on the plates whose y is below the given one; returns the number of colours
int CountSlotKinds(int theLowestY)
{
	sColourCount = 0;
	memset(sSlotKindCounts, 0, sizeof(sSlotKindCounts));
	for (SceneObject* aPlate = gPlateLayer->mChildren.mHead; aPlate != nullptr; aPlate = aPlate->mNext) {
		for (SceneObject* aChild = aPlate->mChildren.mHead; aChild != nullptr; aChild = aChild->mNext) {
			if (aChild->mType == ObjectTypeSlot && static_cast<Slot*>(aChild)->mFilled == 0 &&
				(float)theLowestY < aPlate->mWorld[AttributeY]) {
				int aKind = static_cast<Slot*>(aChild)->mKind;
				if (sSlotKindCounts[aKind] == 0)
					sColourCount++;
				sSlotKindCounts[aKind]++;
			}
		}
	}
	return sColourCount;
}
}

// port: the counts the colour choice reads, for a test's trace
void GetSlotKindCountsForTest(int* theCounts)
{
	memcpy(theCounts, sSlotKindCounts, sizeof(sSlotKindCounts));
	theCounts[SlotKindCount] = sColourCount;
}

Shooter::Shooter()
{
	mType = ObjectTypeShooter;
	mZOrder = 3.0f;
	mMoveX = 0;
	mTargetSlot = nullptr;
	mPowerShots = 0;
	mWingsTicks = 0;
	mComboShot = 0;
	mWildShots = 0;
	mShotsFired = 0;
	mSinkCount = 0;
	mCupidWait = 0;
	mScoreMultiplier = 1;
	CurrentProfile().mTriesLeft[CurrentProfile().mMode] = 2;
}

bool Shooter::Update()
{
	TrackMouse();
	FollowMouse();
	Straighten();
	if (mStages.mCursor->mLoopStart != 0)
		Reload();
	UpdateBeam();
	if (mPowerShots != 0 && (int)mTickCount % 20 == 0)
		EmitPowerSparkle();
	mTargetSlot = nullptr;
	UpdateWings();
	AimSparkles();
	UpdateJewelColours();
	SwapJewels();
	Fire();
	UpdateCupid();
	return SceneObject::Update();
}

void Shooter::Draw()
{
	TrackMouse();
	SceneObject::Draw();
}

// The beam that shows the line of a power shot takes the colour of the jewel that is ready
void Shooter::UpdateBeam()
{
	SceneObject* aBeam = FindChildByName("beam", nullptr);
	if (aBeam != nullptr) {
		aBeam->mNoCulling = 1;
		aBeam->mLocal[AttributeX] = -300.0f;
		aBeam->mLocal[AttributeY] = -300.0f;
		aBeam->mLocal[AttributeYScale] = -600.0f / aBeam->mSourceBottom;
		aBeam->mStages.mCursor->mVisible = mPowerShots != 0;
	}
	Jewel* aReadyJewel = GetReadyJewel();
	if (aReadyJewel != nullptr && aBeam != nullptr) {
		aBeam->mLocal[AttributeRed] = aReadyJewel->mTint[0];
		aBeam->mLocal[AttributeGreen] = aReadyJewel->mTint[1];
		aBeam->mLocal[AttributeBlue] = aReadyJewel->mTint[2];
	}
}

// A sparkle that rises from the shooter
void Shooter::EmitPowerSparkle()
{
	SceneObject* aSparkle = NewChild(nullptr);
	aSparkle->SetImageRecord(gActiveLevel->mSparkleImage);
	int aDuration = RandomBetween(300, 700);
	aSparkle->AddStage((float)aDuration, 0, 0, 1, nullptr);
	aSparkle->SetAttribute(AttributeX, (float)RandomFloat(-12.0f, 12.0f), 0);
	aSparkle->SetAttribute(AttributeY, 0, -400.0f);
	aSparkle->SetAttribute(AttributeXScale, 0.6f, 0.4f);
	aSparkle->SetAttribute(AttributeYScale, 0.6f, 0.4f);
	aSparkle->SetAttribute(AttributeAlpha, 0.7f, 0.7f);
}

// Once the shooter has been on screen for 100 updates, a scene that becomes the top scene gets the pointer put under the shooter
// (the pointer moves to the shooter's x and keeps its y), and the click that brought the scene up is forgotten
void Shooter::TrackMouse()
{
	if (mTickCount >= 100.0f && gActiveLevel->mPaused == 0) {
		SceneObject* aRoot = GetRoot();
		if (sLastTrackedScene != aRoot && gScenes.mTail == aRoot) {
			gApp->WarpMouse((int)mWorld[AttributeX], gMouseY);
			static_cast<Level*>(aRoot)->mMouseX = mWorld[AttributeX];
			gActiveLevel->mLeftClick = 0;
		}
		sLastTrackedScene = gScenes.mTail;
	}
}

// The shooter moves a part of the way to the pointer's x on each update, up to the right edge of the playing field
void Shooter::FollowMouse()
{
	if (strstr(mStages.mCursor->mName, "sink") == nullptr && gActiveLevel == gScenes.mTail) {
		float aTargetX = gActiveLevel->mMouseX <= gScreenWidth ? gActiveLevel->mMouseX : gScreenWidth;
		mMoveX = (aTargetX - mWorld[AttributeX]) / 1.3f;
		mLocal[AttributeX] = mLocal[AttributeX] + mMoveX;
	}
}

// The shooter stands straight except while it sinks (the original also tests the middle layer's stage for "vict" first, which
// changes nothing: a stage that holds "sink" is left alone either way)
void Shooter::Straighten()
{
	if (strstr(mStages.mCursor->mName, "sink") == nullptr)
		mLocal[AttributeRotation] = 0;
}

// A cupid comes for a player who waits, in the Flood and Survival modes
void Shooter::UpdateCupid()
{
	if (gTutorial == nullptr && CurrentProfile().mMode != GameModeJourney) {
		if (mParent->FindChildByType(ObjectTypeCupid, nullptr) == nullptr) {
			if (gMiddleLayer != nullptr && strstr(gMiddleLayer->mStages.mCursor->mName, "scrolling") != nullptr &&
				GetRoot()->mNext == nullptr) {
				SceneObject* aLowestPlate = FindLowestOfType(ObjectTypePlate, gPlateLayer);
				if (aLowestPlate != nullptr && mWorld[AttributeY] < aLowestPlate->mWorld[AttributeY])
					return;
				mCupidWait = mCupidWait + 1.0f;
			}
			if (gTuning.mCupidWaitMs / 4.0f < mCupidWait &&
				strstr(gMiddleLayer->mStages.mCursor->mName, "scrolling") != nullptr)
				mParent->NewChild(new Cupid());
		} else {
			mCupidWait = 0;
		}
	}
}

// The shooter sinks into the water: it goes down (the Journey mode's shooter with tries left comes back up), the jewels in the
// air fade away, and the shot, combo and wings are lost
void Shooter::Sink()
{
	mScoreMultiplier = 1;
	if (CurrentProfile().mTriesLeft[CurrentProfile().mMode] < 1) {
		if (strcmp(mStages.mTail->mName, "sink2") == 0) { // the last stage, as the original reads it (0x419ee0)
			mStages.mTail->mLoopEnd = 0;
			mStages.mTail->mLoopStart = 0;
			mStages.mTail->mEvents.Clear();
			AddStage(1000.0f, 1, 1, 0, "lost");
		}
	}
	if (GoToStageByName("sink") == nullptr) {
		strcpy(mStages.mHead->mName, "rise");
		mStages.mHead->mLength = 500.0f;
		mStages.mHead->mStart[AttributeY] = 400.0f;
		AddStage(900.0f, 0, 0, 1, "sink");
		SetAttribute(AttributeRotation, 99999.0f, 90.0f);
		SetAttribute(AttributeY, 99999.0f, 400.0f);
		AddFilter(FilterCubed, AttributeY, 0, 0, 0);
		if (CurrentProfile().mMode == GameModeJourney && 0 < CurrentProfile().mTriesLeft[CurrentProfile().mMode]) {
			AddStage(1500.0f, 0, 1, 0, "sink2");
			AddEvent(TriggerEnd, ActionGoToStage, "rise", nullptr, nullptr, nullptr, nullptr);
		} else {
			AddStage(1500.0f, 1, 1, 0, "sink2");
		}
		GoToStageByName("sink");
	}
	mPowerShots = 0;
	mComboShot = 0;
	if (mWingsTicks == 0) {
		SetAttribute(AttributeRotation, 99999.0f, 90.0f);
		SetAttribute(AttributeY, 99999.0f, 400.0f);
	} else {
		SetAttribute(AttributeRotation, 99999.0f, 790.0f);
		SetAttribute(AttributeY, 99999.0f, 800.0f);
		mWingsTicks = 0;
	}
	mSinkCount++;
	SceneObject* aJewel = nullptr;
	while ((aJewel = gPlateLayer->FindChildByType(ObjectTypeJewel, aJewel)) != nullptr) {
		aJewel->AddStage(200.0f, 0, 0, 1, "fade_out");
		aJewel->SetAttribute(AttributeAlpha, 1.0f, 0);
		aJewel->SetAttribute(AttributeRotation, 99999.0f, 100719.0f);
		aJewel->AddFilter(FilterSquared, AttributeAll, 2.0f, 0, 0);
		aJewel->GoToStageByName("fade_out");
		aJewel->mType = ObjectTypeBase;
	}
	LoadAndPlaySound("audio\\sink.ogg", -1);
}

// The jewels in the row that are not wild yet turn wild, as long as there are wild shots left
void Shooter::ActivateWild()
{
	for (SceneObject* aChild = mChildren.mHead; aChild != nullptr; aChild = aChild->mNext) {
		if (aChild->mType == ObjectTypeJewel && 0 < mWildShots && static_cast<Jewel*>(aChild)->mKind < FirstWildJewelKind) {
			aChild->SetImage("gfx\\pwrup_wild_shape.png", 0, 0, 0, 0);
			static_cast<Jewel*>(aChild)->mKind = WildJewelKind;
			mWildShots--;
		}
	}
}

// A pair of wings (each a wing and a feather picture that flap) that carry the shooter for 2000 more updates
void Shooter::ActivateWings()
{
	mWingsTicks += 2000;
	SceneObject* aWing = FindChildByName("wings", nullptr);
	if (aWing == nullptr) {
		SceneObject* aLeftWing = NewChild(nullptr);
		strcpy(aLeftWing->mName, "wings");
		aLeftWing->AddStage(200.0f, 1, 0, 1, nullptr);
		aLeftWing->SetAttribute(AttributeX, -44.0f, -44.0f);
		aLeftWing->SetAttribute(AttributeY, 10.0f, 10.0f);
		aLeftWing->SetAttribute(AttributeRotation, -30.0f, 30.0f);
		aLeftWing->AddFilter(FilterSuperCos, AttributeAll, 1.0f, 0, 0);
		aLeftWing->AddEvent(TriggerStart, ActionPlaySound, nullptr, nullptr, "audio\\wings.ogg", nullptr, nullptr);
		aLeftWing->AddStage(200.0f, 0, 1, 1, nullptr);
		aLeftWing->SetAttribute(AttributeX, -44.0f, -44.0f);
		aLeftWing->SetAttribute(AttributeY, 10.0f, 10.0f);
		aLeftWing->SetAttribute(AttributeRotation, 30.0f, -30.0f);
		aLeftWing->AddFilter(FilterSuperCos, AttributeAll, 1.0f, 0, 0);
		SceneObject* aLeftFeather = aLeftWing->NewChild(nullptr);
		aLeftFeather->SetImage("gfx\\pwrup_wings_shooter.png", 0, 0, 0, 0);
		aLeftFeather->AddStage(200.0f, 1, 0, 1, nullptr);
		aLeftFeather->SetAttribute(AttributeX, -40.0f, -40.0f);
		aLeftFeather->SetAttribute(AttributeFrameNumber, 0, 0);
		aLeftFeather->AddStage(200.0f, 0, 1, 1, nullptr);
		aLeftFeather->SetAttribute(AttributeX, -40.0f, -40.0f);
		aLeftFeather->SetAttribute(AttributeFrameNumber, 1.0f, 1.0f);

		SceneObject* aRightWing = NewChild(nullptr);
		strcpy(aRightWing->mName, "wings");
		aRightWing->AddStage(200.0f, 1, 0, 1, nullptr);
		aRightWing->SetAttribute(AttributeX, 46.0f, 46.0f);
		aRightWing->SetAttribute(AttributeY, 10.0f, 10.0f);
		aRightWing->SetAttribute(AttributeRotation, 30.0f, -30.0f);
		aRightWing->AddFilter(FilterSuperCos, AttributeAll, 1.0f, 0, 0);
		aRightWing->AddStage(200.0f, 0, 1, 1, nullptr);
		aRightWing->SetAttribute(AttributeX, 46.0f, 46.0f);
		aRightWing->SetAttribute(AttributeY, 10.0f, 10.0f);
		aRightWing->SetAttribute(AttributeRotation, -30.0f, 30.0f);
		aRightWing->AddFilter(FilterSuperCos, AttributeAll, 1.0f, 0, 0);
		SceneObject* aRightFeather = aRightWing->NewChild(nullptr);
		aRightFeather->SetImage("gfx\\pwrup_wings_shooter.png", 0, 0, 0, 0);
		aRightFeather->mFlipX = true;
		aRightFeather->AddStage(200.0f, 1, 0, 1, nullptr);
		aRightFeather->SetAttribute(AttributeX, 40.0f, 40.0f);
		aRightFeather->SetAttribute(AttributeFrameNumber, 0, 0);
		aRightFeather->AddStage(200.0f, 0, 1, 1, nullptr);
		aRightFeather->SetAttribute(AttributeX, 40.0f, 40.0f);
		aRightFeather->SetAttribute(AttributeFrameNumber, 1.0f, 1.0f);
	} else {
		aWing->mLocal[AttributeAlpha] = 1.0f;
		while ((aWing = FindChildByName("wings", aWing)) != nullptr)
			aWing->mLocal[AttributeAlpha] = 1.0f;
	}
}

// With wings the shooter flies after the pointer's y (between the usual place and five sixths of the screen above it) and the
// wings fade out over the last 500 updates; without them it settles back to its place
void Shooter::UpdateWings()
{
	if (mWingsTicks < 1) {
		if (FindChildByName("wings", nullptr) != nullptr) {
			bool aMore = mChildren.Begin();
			while (aMore) {
				if (strstr(mChildren.mCursor->mName, "wings") != nullptr)
					mChildren.RemoveCursor();
				aMore = mChildren.Next();
			}
		}
		SceneStage* aStage = mStages.mCursor;
		if (VeniceAbs(mLocal[AttributeY] - aStage->mEnd[AttributeY]) <= 1.0) {
			aStage->mAttributeMask |= 8;
			SceneFilter* aBob = aStage->FindFilter(FilterSined, AttributeY);
			if (aBob != nullptr)
				aBob->mAmplitude = 1.0f;
		} else {
			float aDistance = aStage->mEnd[AttributeY] - mLocal[AttributeY];
			float aStep = aDistance <= 20.0f ? aDistance : 20.0f;
			aStep = -20.0f <= aStep ? aStep : -20.0f;
			mMoveY = aStep / 10.0f;
			mLocal[AttributeY] = mLocal[AttributeY] + mMoveY;
		}
	} else {
		SceneStage* aStage = mStages.mCursor;
		aStage->mAttributeMask = 0;
		SceneFilter* aBob = aStage->FindFilter(FilterSined, AttributeY);
		if (aBob != nullptr)
			aBob->mAmplitude = 0;
		if (strstr(gMiddleLayer->mStages.mCursor->mName, "vict") == nullptr) {
			float aDistance = gActiveLevel->mMouseY - mWorld[AttributeY];
			float aStep = aDistance <= 40.0f ? aDistance : 40.0f;
			aStep = -40.0f <= aStep ? aStep : -40.0f;
			mMoveY = aStep / 10.0f;
			mLocal[AttributeY] = aStage->mEnd[AttributeY] <= mLocal[AttributeY] + mMoveY ? aStage->mEnd[AttributeY] :
				mLocal[AttributeY] + mMoveY;
			float aLowestY = aStage->mEnd[AttributeY] - (gScreenHeight * 5.0f) / 6.0f;
			mLocal[AttributeY] = mLocal[AttributeY] + mMoveY <= aLowestY ? aLowestY : mLocal[AttributeY] + mMoveY;
		}
		mWingsTicks--;
		if (mWingsTicks < 500) {
			SceneObject* aWing = nullptr;
			while ((aWing = FindChildByName("wings", aWing)) != nullptr)
				aWing->mLocal[AttributeAlpha] = (float)mWingsTicks / 500.0f;
		}
	}
}

// The jewel that is ready to throw as a combo shot throws sparkles, and now and then a ghost of itself that floats up
void Shooter::AimSparkles()
{
	if (mComboShot != 0 && (int)mTickCount % 15 == 2) {
		Jewel* aReadyJewel = GetReadyJewel();
		if (aReadyJewel != nullptr) {
			aReadyJewel->MakeSparkles(2, 1.0f, 1.0f, 1.0f, 1.0f);
			if (RandomBetween(1, 3) == 2) {
				SceneObject* aGhost = NewChild(nullptr);
				aGhost->SetImageRecord(aReadyJewel->mImage);
				aGhost->AddStage((float)RandomFloat(1000.0f, 3000.0f), 0, 0, 1, nullptr);
				aGhost->SetAttribute(AttributeXScale, 0.5f, 0.5f);
				aGhost->SetAttribute(AttributeYScale, 0.5f, 0.5f);
				aGhost->SetAttribute(AttributeAlpha, 0.75f, 0);
				aGhost->AddFilter(FilterVelocity, AttributeX, (float)RandomFloat(-0.5f, 0.5f), 0, 0);
				aGhost->AddFilter(FilterVelocity, AttributeY, (float)RandomFloat(-2.0f, -0.5f), 0.006f, 1.0f);
				aGhost->mLocal[AttributeY] = -30.0f;
				gSparkleLayer->AdoptNewestChild(this, true);
			}
		}
	}
}

// The right button swaps the jewel that is ready with the one that waits
void Shooter::SwapJewels()
{
	if (gActiveLevel->mRightClick != 0.0f && CountChildrenOfType(ObjectTypeJewel) == 2) {
		SceneObject* aFirst = FindChildByType(ObjectTypeJewel, nullptr);
		SceneObject* aSecond = FindChildByType(ObjectTypeJewel, aFirst);
		if (strcmp(aFirst->mStages.mCursor->mName, "ready") == 0 && strcmp(aSecond->mStages.mCursor->mName, "waiting") == 0) {
			aFirst->GoToStageByName("demote");
			aSecond->GoToStageByName("promote");
			return;
		}
		if (strcmp(aSecond->mStages.mCursor->mName, "ready") == 0 && strcmp(aFirst->mStages.mCursor->mName, "waiting") == 0) {
			aSecond->GoToStageByName("demote");
			aFirst->GoToStageByName("promote");
		}
	}
}

int Shooter::CountJewelsOfKind(int theKind)
{
	int aCount = 0;
	SceneObject* aJewel = nullptr;
	while ((aJewel = FindChildByType(ObjectTypeJewel, aJewel)) != nullptr) {
		if (static_cast<Jewel*>(aJewel)->mKind == theKind)
			aCount++;
	}
	return aCount;
}

// The jewels in the row become a colour that a slot still needs, so that the player is never given a colour that no plate has
void Shooter::UpdateJewelColours()
{
	if (strstr(mStages.mCursor->mName, "rise") != nullptr && FindChildByType(ObjectTypeJewel, nullptr) != nullptr)
		FindChildByType(ObjectTypeJewel, nullptr)->mIsDead = true;

	if (CurrentProfile().mMode == GameModeJourney && strstr(gMiddleLayer->mStages.mCursor->mName, "still") == nullptr) {
		SceneObject* aScrollPoint = FindLowestOfType(ObjectTypeScrollPoint, gPlateLayer);
		if (aScrollPoint == nullptr)
			CountSlotKinds(0);
		else
			CountSlotKinds((int)aScrollPoint->mWorld[AttributeY] - (int)gTuning.mScrollingSlowPoint);
	} else {
		// the original counts the slots below the earthquake point first when no jewel is in the row, and then always counts again
		// from the top (a test of its result that can never hold), so only the second count has an effect
		CountSlotKinds(0);
	}

	for (SceneObject* aChild = mChildren.mHead; aChild != nullptr; aChild = aChild->mNext) {
		if (aChild->mType == ObjectTypeJewel) {
			Jewel* aJewel = static_cast<Jewel*>(aChild);
			int aSameKindInRow = 1;
			if (GetReadyJewel() != aJewel)
				aSameKindInRow = CountJewelsOfKind(aJewel->mKind);
			if (sColourCount != 0 && sSlotKindCounts[aJewel->mKind] < aSameKindInRow && aJewel->mKind != WildJewelKind &&
				aJewel->mKind < FirstWildJewelKind) {
				SceneObject* aOther = nullptr;
				while ((aOther = FindChildByType(ObjectTypeJewel, aOther)) != nullptr) {
					if (aOther != aJewel)
						aJewel->SetKind(PickColour(static_cast<Jewel*>(aOther)));
				}
			}
		}
	}
}

// The colour of a new jewel. The Journey mode takes the colour of the lowest key plate's first slot (of the lowest key plate that
// is on the screen and not fading), the other modes the colour of the lowest plate's lowest slot that is open and above the
// earthquake point, and a colour the plates have no slot of is never chosen; a jewel that the new one is to differ from is given
int Shooter::PickColour(Jewel* theExcluded)
{
	int aKind = -1;
	SceneObject* aLowestPlate = FindLowestOfType(ObjectTypePlate, nullptr);
	if (aLowestPlate == nullptr || CurrentProfile().mMode == GameModeJourney) {
		if (aLowestPlate != nullptr) {
			int aKeyKind = -1;
			float aKeyY = -989898.0f;
			SceneObject* aPlate = nullptr;
			while ((aPlate = gPlateLayer->FindChildByType(ObjectTypePlate, aPlate)) != nullptr) {
				Slot* aFirstSlot = static_cast<Slot*>(aPlate->FindChildByType(ObjectTypeSlot, nullptr));
				if (strstr(static_cast<Plate*>(aPlate)->mShapeName, "key_plate") != nullptr && aKeyY < aPlate->mWorld[AttributeY] &&
					strstr(aPlate->mStages.mCursor->mName, "fade") == nullptr &&
					aPlate->HitTest(gScreenWidth / 2.0f, gScreenHeight / 2.0f, gScreenWidth, gScreenHeight, 0.0f) &&
					(theExcluded == nullptr || theExcluded->mKind != aFirstSlot->mKind)) {
					aKeyY = aPlate->mWorld[AttributeY];
					aKeyKind = aFirstSlot->mKind;
				}
			}
			if (-1 < aKeyKind)
				return aKeyKind;
		}
	} else {
		Slot* aLowestSlot = static_cast<Slot*>(FindLowestOfType(ObjectTypeSlot, aLowestPlate));
		if (aLowestSlot == nullptr || aLowestSlot->mFilled != 0 || aLowestSlot->mWorld[AttributeY] <= gTuning.mEarthquakePoint - 80.0f) {
			SceneObject* aSlot = nullptr;
			do {
				aSlot = aLowestPlate->FindChildByType(ObjectTypeSlot, aSlot);
				if (aSlot == nullptr)
					break;
			} while (static_cast<Slot*>(aSlot)->mFilled != 0 || aSlot->mWorld[AttributeY] <= gTuning.mEarthquakePoint);
			if (aSlot != nullptr)
				return static_cast<Slot*>(aSlot)->mKind;
		} else {
			return aLowestSlot->mKind;
		}
	}
	// a random colour that has slots to fill, three times over (the last one is kept)
	for (int aTry = 0; aTry < 3; aTry++) {
		do {
			do {
				aKind = RandomBetween(3, 10);
			} while (sSlotKindCounts[aKind] == 0);
		} while (sColourCount != 1 && theExcluded != nullptr && theExcluded->mKind == aKind);
	}
	return aKind;
}

Jewel* Shooter::GetReadyJewel()
{
	SceneObject* aChild = mChildren.mHead;
	while (aChild != nullptr && (aChild->mType != ObjectTypeJewel || strcmp(aChild->mStages.mCursor->mName, "ready") != 0))
		aChild = aChild->mNext;
	return static_cast<Jewel*>(aChild);
}

// A new jewel goes into the row when there is room for it: when there is none, when the level is not one that waits for the
// player, or when the one that is in the row is the only one
void Shooter::Reload()
{
	int aJewelCount = CountChildrenOfType(ObjectTypeJewel);
	const char* aMiddleStage = gMiddleLayer->mStages.mCursor->mName;
	bool aHasScrollPoint = gPlateLayer->FindChildByType(ObjectTypeScrollPoint, nullptr) != nullptr;
	bool aIsSurvival = CurrentProfile().mMode == GameModeSurvival;
	bool aHasTutorial = gActiveLevel->FindChildByType(ObjectTypeTutorial, nullptr) != nullptr;
	if ((aJewelCount == 0 || aHasScrollPoint || aIsSurvival || aHasTutorial) &&
		(mShotsFired == 0 || aHasScrollPoint || aIsSurvival || aHasTutorial) && aJewelCount < 2 &&
		strstr(aMiddleStage, "victory") == nullptr &&
		(strstr(aMiddleStage, "still") == nullptr ||
		(gPlateLayer->FindChildByType(ObjectTypeJewel, nullptr) == nullptr && CountChildrenOfType(ObjectTypeJewel) == 0))) {
		Jewel* aWaitingJewel = static_cast<Jewel*>(FindChildByType(ObjectTypeJewel, nullptr));
		if (aWaitingJewel != nullptr && strcmp(aWaitingJewel->mStages.mCursor->mName, "waiting") == 0)
			aWaitingJewel->GoToStageByName("promote");
		if (0 < sColourCount) {
			Jewel* aNewJewel = new Jewel(PickColour(aWaitingJewel));
			NewChild(aNewJewel);
			if (strstr(gActiveLevel->mName, "trick") != nullptr)
				aNewJewel->GoToStageByName("promote");
			ActivateWild();
		}
	}
}

// A click throws the jewel that is ready (while the level scrolls or stands still): the jewel is given the speed along the
// shooter's angle, and the shooter's power shot and combo shot go with it
void Shooter::Fire()
{
	const char* aMiddleStage = gMiddleLayer->mStages.mCursor->mName;
	if (mCanShoot != 0 && gActiveLevel->mLeftClick != 0.0f &&
		(strcmp(aMiddleStage, "scrolling") == 0 || strcmp(aMiddleStage, "still") == 0) &&
		strcmp(mStages.mCursor->mName, "ready") == 0) {
		mShotsFired++;
		Jewel* aJewel = GetReadyJewel();
		if (aJewel != nullptr) {
			aJewel->GoToStageByName("shooting");
			aJewel->mStages.mCursor->mFrame = 1.0f;
			SceneFilter* aVelocityX = aJewel->mStages.mCursor->FindFilter(FilterVelocity, AttributeX);
			SceneFilter* aVelocityY = aJewel->mStages.mCursor->FindFilter(FilterVelocity, AttributeY);
			aVelocityX->mAmplitude = (float)(VeniceCos((mLocal[AttributeRotation] + 90.0f) * DegreesToRadians) * 2.3f);
			float aPowerBoost = mPowerShots != 0 ? 1.0f : 0.0f;
			aVelocityY->mAmplitude =
				(float)(VeniceSin((mLocal[AttributeRotation] + 90.0f) * DegreesToRadians) * -2.3f - 4.0f * aPowerBoost);
			aJewel->mPowerShot = mPowerShots != 0;
			aJewel->mComboShot = mComboShot != 0;
			aJewel->mTickCount = 0;
			mComboShot = 0;
			mChildren.mCursor = aJewel;
			gPlateLayer->AdoptNewestChild(this, true);
			if (AnyPlateIncomplete())
				gActiveLevel->mShotsFired++;
			if (mPowerShots != 0)
				PlaySoundByName("audio\\fastshot.ogg", -1);
			else
				PlaySoundByName("audio\\throw.ogg", -1);
			if (mWingsTicks == 0 && VeniceAbs(mLocal[AttributeY] - mStages.mCursor->mEnd[AttributeY]) <= 1.0) {
				mStages.mCursor->mFrame = 0;
				SceneFilter* aRecoil = mStages.mCursor->AddFilter(FilterDecaySined, AttributeY, 10.0f, 0.5f, 0, true);
				aRecoil->mDuration = 100.0f;
			}
			SceneStage* aMiddleStageObject = gMiddleLayer->mStages.mCursor;
			aMiddleStageObject->mFrame = 0;
			int aJewelsInFlight = gPlateLayer->CountChildrenOfType(ObjectTypeJewel);
			float aPushScale = (float)aJewelsInFlight <= 6.0f ? (float)aJewelsInFlight : 6.0f;
			if (0 < mPowerShots)
				mPowerShots--;
			if (gTutorial == nullptr && CurrentProfile().mMode != GameModeTrickShot && CurrentProfile().mMode != GameModeJourney)
				aMiddleStageObject->AddFilter(FilterVelocity, AttributeY, gTuning.mPushOnShotFired * aPushScale, 0, 0.985f, true);
		}
	}
}
