// The jewel (0x409fa0-0x40bc60, and its collision with the bumpers' outlines 0x428ec0).

#include "Jewel.h"

#include "Geometry.h"
#include "ImageRecord.h"
#include "Level.h"
#include "Profiles.h"
#include "SceneLoader.h"
#include "Shooter.h"
#include "Slot.h"
#include "TextObject.h"
#include "Tuning.h"
#include "VeniceGlobals.h"
#include "VeniceMath.h"
#include "VeniceSound.h"
#include "Scoring.h"

#include <cstdio>
#include <cstring>

Jewel::Jewel(int theKind)
{
	mType = ObjectTypeJewel;
	mZOrder = 4.0f;
	SetKind(theKind);
	float aReadyY = (0.0f - mImage->mFrameHeight) / 2.0f - 15.0f;

	// waiting in the row, small and behind the ready one
	AddStage(1.0f, 1, 1, 1, "waiting");
	SetAttribute(AttributeXScale, 0.48f, 0.48f);
	SetAttribute(AttributeYScale, 0.48f, 0.48f);
	SetAttribute(AttributeY, 13.0f, 13.0f);
	SetAttribute(AttributeAlpha, 0.8f, 0.8f);

	// grows into the ready one over the reload time
	AddStage(gTuning.mReloadTime, 0, 0, 1, "promote");
	SetAttribute(AttributeXScale, AttributeUnsetValue, 1.0f);
	SetAttribute(AttributeYScale, AttributeUnsetValue, 1.0f);
	SetAttribute(AttributeY, AttributeUnsetValue, aReadyY);
	SetAttribute(AttributeAlpha, AttributeUnsetValue, 1.0f);
	AddFilter(FilterSuperCos, AttributeAll, 1.0f, 1.0f, 0);
	AddEvent(TriggerEnd, ActionGoToStage, "ready", nullptr, nullptr, nullptr, nullptr);

	// and back to waiting
	AddStage(gTuning.mReloadTime, 0, 0, 1, "demote");
	SetAttribute(AttributeXScale, AttributeUnsetValue, 0.48f);
	SetAttribute(AttributeYScale, AttributeUnsetValue, 0.48f);
	SetAttribute(AttributeY, AttributeUnsetValue, 13.0f);
	SetAttribute(AttributeAlpha, AttributeUnsetValue, 0.8f);
	AddFilter(FilterSuperCos, AttributeAll, 1.0f, 1.0f, 0);
	AddEvent(TriggerEnd, ActionGoToStage, "waiting", nullptr, nullptr, nullptr, nullptr);

	// the ready jewel bobs a little
	AddStage(1000.0f, 1, 1, 1, "ready");
	SetAttribute(AttributeY, aReadyY, aReadyY);
	SetAttribute(AttributeX, 0.0001f, 0.0001f);
	AddFilter(FilterSined, AttributeY, -1.0f, 1.0f, 0);

	// and in the air the filters of its speed (which the shooter sets) carry it
	AddStage(650.0f, 1, 1, 1, "shooting");
	AddFilter(FilterVelocity, AttributeY, -36.0f, 0.0f, 0);
	AddFilter(FilterVelocity, AttributeX, 0.0f, 0.0f, 0);

	mClipLeft = -10000;
	mClipRight = 9999;
	mClipTop = -10000;
	mClipBottom = 580;
}

// The picture of the colour, and the colour of the beam that the shooter shows
void Jewel::SetKind(int theKind)
{
	mKind = theKind;
	if (mKind < 100) {
		char aName[256];
		sprintf(aName, "gfx\\jewel%d.png", mKind);
		SetImage(aName, 0, 0, 0, 0);
	} else {
		SetImage("gfx\\pwrup_wild_shape.png", 0, 0, 0, 0);
	}
	switch (mKind) {
	case 1:
		mTint[0] = 1.0f;
		mTint[1] = 1.0f;
		mTint[2] = 1.0f;
		break;
	case 2:
		mTint[0] = 0.42f;
		mTint[1] = 0.655f;
		mTint[2] = 1.19f;
		break;
	case 3:
		mTint[0] = 0.415f;
		mTint[1] = 0.415f;
		mTint[2] = 0.95f;
		break;
	case 4:
		mTint[0] = 0.6f;
		mTint[1] = 0.81f;
		mTint[2] = 0.485f;
		break;
	case 5:
	case 7:
		mTint[0] = 1.195f;
		mTint[1] = 1.06f;
		mTint[2] = 0.7f;
		break;
	case 6:
	case 8:
		mTint[0] = 1.165f;
		mTint[1] = 0.415f;
		mTint[2] = 0.415f;
		break;
	case 9:
		mTint[0] = 0.75f;
		mTint[1] = 0.515f;
		mTint[2] = 0.95f;
		break;
	default:
		mTint[0] = 1.0f;
		mTint[1] = 1.0f;
		mTint[2] = 1.0f;
	}
}

bool Jewel::Update()
{
	bool aAlive = SceneObject::Update();
	mAge++;
	if (mStages.mCursor == nullptr || strcmp(mStages.mCursor->mName, "ready") != 0) {
		if (mStages.mCursor != nullptr && strcmp(mStages.mCursor->mName, "shooting") == 0) {
			if (!KeepInBounds()) {
				aAlive = false;
			} else if (!FlyToTarget()) {
				aAlive = false;
			} else {
				ExplodeAfterBumps();
				Spin();
				ShotSparkle();
				Trail();
				aAlive = PickedUp() && aAlive;
			}
		}
	} else {
		// the jewel in the row moves with the shooter (the shooter's move was added to the row's place already)
		mWorld[AttributeX] = mWorld[AttributeX] - gShooter->mMoveX;
		EmitReadySparkle();
		SceneFilter* aRotation = mStages.mCursor->FindFilter(FilterVelocity, AttributeRotation);
		if (aRotation != nullptr && mKind != WildJewelKind) {
			mStages.mCursor->mFilters.mCursor = aRotation;
			mStages.mCursor->mFilters.RemoveCursor();
			mLocal[AttributeRotation] = 0;
		}
		if (aRotation == nullptr && mKind == WildJewelKind)
			mStages.mCursor->AddFilter(FilterVelocity, AttributeRotation, 3.0f, 0, 0, false);
	}
	return aAlive;
}

// A little glitter in the ready jewel
void Jewel::EmitReadySparkle()
{
	if (RandomBetween(0, 17) != 0)
		return;
	SceneObject* aSparkle = new SceneObject();
	aSparkle->SetImageRecord(gActiveLevel->mSparkleImage);
	aSparkle->AddStage((float)RandomBetween(600, 900), 0, 0, 1, nullptr);
	float aX = (float)RandomFloat((0.0f - mSourceRight) / 1.5f, mSourceRight / 1.5f);
	aSparkle->SetAttribute(AttributeX, aX, aX);
	float aY = (float)RandomFloat(mSourceBottom / 2.0f + mLocal[AttributeY], mSourceBottom / 2.0f + 10.0f + mLocal[AttributeY]);
	aSparkle->SetAttribute(AttributeY, aY - 5.0f, aY - 5.0f);
	aSparkle->SetAttribute(AttributeAlpha, 0.01f, 0.01f);
	float aStartScale = (float)RandomFloat(0.1f, 0.4f);
	float aEndScale = (float)RandomFloat(0.1f, 0.4f);
	aSparkle->SetAttribute(AttributeXScale, aStartScale, aEndScale);
	aSparkle->SetAttribute(AttributeYScale, aStartScale, aEndScale);
	aSparkle->AddFilter(FilterVelocity, AttributeX, (float)(RandomFloat(-3.0f, 3.0f) / 100.0), 0, 0);
	aSparkle->AddFilter(FilterVelocity, AttributeY, (float)(RandomFloat(-3.0f, 3.0f) / 100.0), 0, 0);
	aSparkle->AddFilter(FilterSined, AttributeAlpha, 0.99f, 0.5f, 0);
	aSparkle->mBlitStyle = 1;
	mParent->AddChild(aSparkle);
	mParent->mChildren.mCursor = this;
}

// A bounce counts only when the jewel has flown for five updates since the last one (the shot's walls and bumpers would count
// again for every update of a jewel that stays in them); it forgets the slot it last bounced off
void Jewel::CountBounce()
{
	if (4 < mAge) {
		if (mShot != 0)
			PlaySoundByName("audio\\toss.ogg", -1);
		mBounces++;
		mLastSlot = nullptr;
		mAge = 0;
	}
}

// The jewel that goes out of the bottom is lost (a jewel that a slot threw back is simply gone); the walls bounce it and, if the
// tuning says so, the top does too (a wild jewel is gone instead). Returns false when the jewel is gone
bool Jewel::KeepInBounds()
{
	if (mWorld[AttributeY] <= gScreenHeight / 2.0f ||
		HitTest(gScreenWidth / 2.0f, gScreenHeight / 2.0f - 46.0f, gScreenWidth, gScreenHeight - 46.0f, 0.0f) || mShot == 0) {
		if (mWorld[AttributeY] <= gScreenHeight / 2.0f ||
			HitTest(gScreenWidth / 2.0f, gScreenHeight / 2.0f, gScreenWidth, gScreenHeight + 10.0f, 0.0f)) {
			float aBelowTop = (0.0f - mImage->mFrameWidth) / 3.0f;
			if (aBelowTop <= mWorld[AttributeY] ||
				mStages.mCursor->FindFilter(FilterVelocity, AttributeY)->mPeriod != 0.0f) {
				float aEdge = mImage->mFrameWidth / 3.0f;
				if (aEdge <= mWorld[AttributeX]) {
					if (gScreenWidth - mImage->mFrameWidth / 3.0f < mWorld[AttributeX]) {
						SceneFilter* aVelocityX = mStages.mCursor->FindFilter(FilterVelocity, AttributeX);
						double aSpeed = VeniceAbs(aVelocityX->mAmplitude);
						mStages.mCursor->FindFilter(FilterVelocity, AttributeX)->mAmplitude = (float)-aSpeed;
						mLocal[AttributeX] = mLocal[AttributeX] - mImage->mFrameWidth / 3.0f;
						CountBounce();
					}
				} else {
					SceneFilter* aVelocityX = mStages.mCursor->FindFilter(FilterVelocity, AttributeX);
					double aSpeed = VeniceAbs(aVelocityX->mAmplitude);
					mStages.mCursor->FindFilter(FilterVelocity, AttributeX)->mAmplitude = (float)aSpeed;
					mLocal[AttributeX] = mImage->mFrameWidth / 3.0f + mLocal[AttributeX];
					CountBounce();
				}
				if (gTuning.mBounceOffTop != 0 && mWorld[AttributeY] < mImage->mFrameWidth / 3.0f &&
					strstr(gMiddleLayer->mStages.mCursor->mName, "victory") == nullptr) {
					if (mKind < 101 || mShot == 0) {
						SceneFilter* aVelocityY = mStages.mCursor->FindFilter(FilterVelocity, AttributeY);
						if (aVelocityY->mPeriod == 0.0f) {
							double aSpeed = VeniceAbs(aVelocityY->mAmplitude);
							aVelocityY->mAmplitude = (float)aSpeed;
							mLocal[AttributeY] = mImage->mFrameWidth / 3.0f + mLocal[AttributeY];
							CountBounce();
						}
					} else {
						mIsDead = true;
					}
				}
				return true;
			}
			return false;
		}
		return false;
	}
	if (mKind < 100)
		LandInWater();
	return false;
}

// The jewel that a slot threw back flies the way the slot's x and y put it: it steps toward the slot it was aimed at on a curve
// (a ball's flight: its speed and its pull), and stops aiming when the slot has been filled by another jewel
bool Jewel::FlyToTarget()
{
	if (mTarget == nullptr)
		return true;
	if (mTarget->mFilled != 0) {
		mTarget = nullptr;
		return true;
	}
	const float aAccelerationX = 0.0f;
	const float aVelocityY = -0.75f;
	const float aAccelerationY = 0.02f;
	float aSign = 0.0f <= mTarget->mWorld[AttributeX] - mStartX ? 1.0f : -1.0f;
	float aDistanceX = (float)VeniceAbs(mTarget->mWorld[AttributeX] - mStartX);
	double aRoot = VeniceSqrt((float)VeniceAbs(mTarget->mWorld[AttributeY] - mStartY));
	float aVelocityX = (float)((((double)aDistanceX / aRoot) * (double)aSign) / 13.0);
	float aTime = mTickCount;
	mWorld[AttributeX] = aVelocityX * aTime + mStartX + 0.5f * aAccelerationX * aTime * aTime;
	mWorld[AttributeY] = aVelocityY * aTime + mStartY + 0.5f * aAccelerationY * aTime * aTime;
	mLocal[AttributeX] = GetLocalX(mWorld[AttributeX], mWorld[AttributeY]);
	mLocal[AttributeY] = GetLocalY(mWorld[AttributeX], mWorld[AttributeY]);
	return true;
}

// A jewel that has bounced enough times bursts where it is on the screen (and ends a trick shot), and is gone
void Jewel::ExplodeAfterBumps()
{
	if (gTuning.mBumpsToExplode <= mBounces) {
		if (HitTest(gScreenWidth / 2.0f, gScreenHeight / 2.0f, gScreenWidth - 8.0f, gScreenHeight - 8.0f, 0.0f)) {
			ImageRecord* aSparkleImage = gActiveLevel->mSparkleImage;
			gActiveLevel->mSparkleImage = mImage;
			MakeSparkles(10, 1.0f, 1.0f, 1.0f, 1.0f);
			LoadAndPlaySound("audio\\break.ogg", -1);
			gActiveLevel->mSparkleImage = aSparkleImage;
			gActiveLevel->MissTrickShot();
		}
		mIsDead = true;
	}
}

// The jewel turns the way it flies (a jewel that was thrown back spins instead, away from the way it moves)
void Jewel::Spin()
{
	SceneFilter* aVelocityY = mStages.mCursor->FindFilter(FilterVelocity, AttributeY);
	SceneFilter* aVelocityX = mStages.mCursor->FindFilter(FilterVelocity, AttributeX);
	if (aVelocityX != nullptr && aVelocityY != nullptr) {
		if (mShot == 0) {
			float aDirection = 0.0f <= aVelocityX->mAmplitude ? 1.0f : -1.0f;
			mLocal[AttributeRotation] = mLocal[AttributeRotation] - aDirection * 3.0f;
		} else {
			mLocal[AttributeRotation] =
				(float)(int)(VeniceArcTangent(aVelocityY->mAmplitude, 0.0f - aVelocityX->mAmplitude) * RadiansToDegrees) + 90.0f;
		}
	}
}

// A power shot sparkles every fourth update and leaves a ghost of itself
void Jewel::ShotSparkle()
{
	if (mPowerShot != 0 && (int)mTickCount % 4 == 1) {
		MakeSparkles(2, 1.0f, 1.0f, 1.0f, 1.0f);
		SceneObject* aGhost = NewChild(nullptr);
		aGhost->SetImageRecord(mImage);
		aGhost->AddStage(400.0f, 0, 0, 1, nullptr);
		aGhost->SetAttribute(AttributeXScale, 0.5f, 0.5f);
		aGhost->SetAttribute(AttributeYScale, 0.5f, 0.5f);
		aGhost->SetAttribute(AttributeAlpha, 0.75f, 0);
		aGhost->mBlitStyle = 1;
		gSparkleLayer->AdoptNewestChild(this, true);
	}
}

// A jewel that was thrown back leaves a glow behind it every tenth update
void Jewel::Trail()
{
	if (mShot == 0 && (int)mTickCount % 10 == 0) {
		SceneObject* aGlow = NewChild(nullptr);
		aGlow->mZOrder = 4.001f;
		aGlow->SetImageRecord(gActiveLevel->mSparkleImage);
		aGlow->AddStage(50.0f, 0, 0, 1, nullptr);
		aGlow->SetAttribute(AttributeAlpha, 0, 1.0f);
		aGlow->SetAttribute(AttributeXScale, 1.3f, 0.3f);
		aGlow->SetAttribute(AttributeYScale, 1.3f, 0.3f);
		aGlow->AddStage(1000.0f, 0, 0, 1, nullptr);
		aGlow->SetAttribute(AttributeAlpha, 1.0f, 0);
		aGlow->SetAttribute(AttributeXScale, 1.3f, 0.3f);
		aGlow->SetAttribute(AttributeYScale, 1.3f, 0.3f);
		aGlow->mBlitStyle = 1;
		gActiveLevel->AdoptNewestChild(this, true);
	}
}

// The shooter takes back a jewel that has come down onto it (not in the Trick Shot mode, and not a wild jewel): the jewel is gone,
// a jewel that a slot threw back scores for it, and the shot is given back (a combo shot's too, with the wings)
bool Jewel::PickedUp()
{
	if (gShooter != nullptr && strstr(gActiveLevel->mName, "trick") == nullptr && mKind < FirstWildJewelKind &&
		(gShooter->mWingsTicks == 0 || mShot != 0) &&
		HitTest(gShooter->mWorld[AttributeX], gShooter->mWorld[AttributeY], gShooter->mImage->mFrameWidth,
			gShooter->mImage->mFrameHeight / 8.0f, 0.0f)) {
		MakeSparkles(20, 1.0f, 1.0f, 1.0f, 1.0f);
		if (mShot == 0 && mNoPickupScore == 0) {
			TextObject* aScoreText = new TextObject();
			aScoreText->SetText(FormatScore(1000), "gfx\\font1.png", 1.0f, 0, JustifyCenter);
			aScoreText->AddStage(1500.0f, 0, 0, 1, nullptr);
			aScoreText->SetAttribute(AttributeXScale, 0.6f, 0);
			aScoreText->SetAttribute(AttributeYScale, 0.6f, 0);
			aScoreText->SetAttribute(AttributeX, mWorld[AttributeX] - gActiveLevel->mWorld[AttributeX],
				mWorld[AttributeX] - gActiveLevel->mWorld[AttributeX]);
			aScoreText->SetAttribute(AttributeY, (mWorld[AttributeY] - gActiveLevel->mWorld[AttributeY]) - 10.0f,
				(mWorld[AttributeY] - gActiveLevel->mWorld[AttributeY]) - 150.0f);
			aScoreText->SetAttribute(AttributeAlpha, 1.0f, 0);
			aScoreText->AddFilter(FilterCubed, AttributeAll, 0, 0, 0);
			gActiveLevel->NewChild(aScoreText);
		}
		if (mHitBumperWithCode != 0) {
			int aShotsFired = gActiveLevel->mShotsFired;
			gActiveLevel->mShotsFired = (aShotsFired == 1 || aShotsFired - 1 < 0) ? 0 : aShotsFired - 1;
		}
		PlaySoundByName("audio\\load_plate.ogg", -1);
		if (gTutorial == nullptr && CurrentProfile().mMode != GameModeTrickShot && CurrentProfile().mMode != GameModeJourney) {
			SceneStage* aMiddleStage = gMiddleLayer->mStages.mCursor;
			aMiddleStage->mFrame = 0;
			aMiddleStage->AddFilter(FilterVelocity, AttributeY, (0.0f - gTuning.mPushOnShotFired) * 2.0f, 0, 0.985f, true);
		}
		if (gShooter->mWingsTicks == 0 &&
			VeniceAbs(gShooter->mLocal[AttributeY] - gShooter->mStages.mCursor->mEnd[AttributeY]) <= 1.0) {
			gShooter->mStages.mCursor->mFrame = 0;
			SceneFilter* aRecoil = gShooter->mStages.mCursor->AddFilter(FilterDecaySined, AttributeY, 10.0f, 0.5f, 0, true);
			aRecoil->mDuration = 100.0f;
		}
		return false;
	}
	return true;
}

// The jewel falls into the water. In the Journey mode the first jewel that is lost outside the tutorial is saved by the
// cupid (a scene in which a cupid carries it up again), otherwise it splashes (fifteen drops) and the level hears of it
void Jewel::LandInWater()
{
	if (mShot == 0 || gTutorial != nullptr || CurrentProfile().mMode != GameModeJourney || CurrentProfile().mCupidUsed != 0 ||
		gActiveLevel->FindChildByType(ObjectTypeCupid, nullptr) != nullptr ||
		strstr(gMiddleLayer->mStages.mCursor->mName, "scroll") == nullptr || !AnyPlateIncomplete()) {
		SetImage("gfx\\water_drop.png", 0, 0, 0, 0);
		ImageRecord* aDropImage = mImage;
		for (int aDrop = 0; aDrop < 15; aDrop++) {
			SceneObject* aSplash = NewChild(nullptr);
			aSplash->SetImageRecord(aDropImage);
			aSplash->AddStage(2000.0f, 0, 0, 1, nullptr);
			aSplash->SetAttribute(AttributeAlpha, 0.7f, 0);
			float aRotation = (float)RandomFloat(0.0f, 360.0f);
			aSplash->SetAttribute(AttributeRotation, aRotation, aRotation);
			float aShade = (float)RandomFloat(0.5f, 1.0f);
			aSplash->SetAttribute(AttributeRed, aShade, aShade);
			aSplash->SetAttribute(AttributeGreen, aShade, aShade);
			aSplash->AddFilter(FilterVelocity, AttributeY, (float)RandomFloat(-1.2f, -0.2f), 0.015f, 0);
			aSplash->AddFilter(FilterVelocity, AttributeX, (float)RandomFloat(-0.2f, 0.2f), 0, 0);
			aSplash->mZOrder = 4.0f;
			gActiveLevel->AdoptNewestChild(this, true);
		}
		PlaySoundByName("audio\\splash.ogg", -1);
		gActiveLevel->MissTrickShot();
		gActiveLevel->CheckLostJewel(this);
	} else {
		// the cupid scene carries the jewel from where it fell, and a Level object that shows the jewel falling stands in the
		// scene stack above it
		PushSceneByName("cupid_save");
		SceneObject* aCupidScene = gScenes.mTail;
		aCupidScene->mStages.mHead->mEnd[AttributeX] = mWorld[AttributeX] - 11.0f;
		aCupidScene->mStages.mHead->mEnd[AttributeY] = mWorld[AttributeY] - 25.0f;
		aCupidScene->mStages.mHead->mNext->mEnd[AttributeY] = mWorld[AttributeY] - 125.0f;
		aCupidScene->mStages.mHead->mNext->mAttributeMask |= 4;
		aCupidScene->mStages.mHead->mNext->mEnd[AttributeX] = gScreenWidth / 2.0f;
		aCupidScene->mStages.mHead->mNext->mStart[AttributeX] = AttributeContinueValue;
		SceneObject* aCarried = aCupidScene->NewChild(nullptr);
		aCarried->SetImageRecord(mImage);
		aCarried->AddStage(100.0f, 0, 0, 0, nullptr);
		aCarried->AddStage(1000.0f, 1, 1, 1, nullptr);
		aCarried->SetAttribute(AttributeX, 11.0f, 11.0f);
		aCarried->SetAttribute(AttributeY, 25.0f, 25.0f);
		aCarried->mLocal[AttributeRotation] = mLocal[AttributeRotation];

		Level* aFalling = new Level();
		gScenes.Append(aFalling);
		aFalling->SetImageRecord(mImage);
		aFalling->AddStage(100.0f, 0, 0, 1, nullptr);
		aFalling->SetAttribute(AttributeX, mWorld[AttributeX], mWorld[AttributeX]);
		aFalling->SetAttribute(AttributeY, mWorld[AttributeY], mWorld[AttributeY]);
		aFalling->SetAttribute(AttributeRotation, aCarried->mLocal[AttributeRotation], aCarried->mLocal[AttributeRotation]);
		if (gScreenWidth / 2.0f < mWorld[AttributeX]) {
			// a jewel on the right: the cupid comes from the right
			for (SceneObject* aChild = aCupidScene->mChildren.mHead; aChild != nullptr; aChild = aChild->mNext) {
				if (aChild->mType == ObjectTypeBase)
					aChild->FlipHorizontally();
			}
			aCupidScene->mFlipX = true;
			aCupidScene->mStages.mHead->mStart[AttributeX] = 900.0f;
			aCupidScene->mStages.mTail->mEnd[AttributeX] = -100.0f;
			SceneStage* aSecondStage = aCupidScene->mStages.mHead->mNext;
			aSecondStage->mEnd[AttributeX] = gScreenWidth - aSecondStage->mEnd[AttributeX];
			float aLeast = 120.0f <= aSecondStage->mEnd[AttributeX] ? aSecondStage->mEnd[AttributeX] : 120.0f;
			aSecondStage->mEnd[AttributeX] = 680.0f <= aLeast ? 680.0f : aLeast;
		}
	}
	if (gTutorial == nullptr)
		CurrentProfile().mCupidUsed = 1;
}

// The jewel against the outline of a bumper (a list of line segments, each a start and an end point in the bumper's own
// frame): the outline is turned and moved to where the bumper is, the jewel is bounced off the segment it comes to first within
// the radius (the one its speed points into the most), and then moved on until it is clear of every segment. The bumper's frame
// of the segments is left changed (they are in the world's frame afterwards).
bool Jewel::CollideWithEdges(SceneObject* theBumper, float (*theEdges)[4], int theEdgeCount, float theRadius)
{
	bool aBounced = false;
	float aAngle = 0.0f - theBumper->mWorld[AttributeRotation];
	for (int anEdge = 0; anEdge < theEdgeCount; anEdge++) {
		float aStartX = theEdges[anEdge][0];
		float aStartY = theEdges[anEdge][1];
		double aCosine = VeniceCos(aAngle * DegreesToRadians);
		double aSine = VeniceSin(aAngle * DegreesToRadians);
		float aCosineTerm = (float)(aCosine * (double)aStartX + (double)theBumper->mWorld[AttributeX]);
		theEdges[anEdge][0] = (float)((double)aCosineTerm - aSine * (double)aStartY);
		float aSineTerm = (float)(aSine * (double)aStartX + (double)theBumper->mWorld[AttributeY]);
		theEdges[anEdge][1] = (float)(aCosine * (double)aStartY + (double)aSineTerm);
		float anEndX = theEdges[anEdge][2];
		float anEndY = theEdges[anEdge][3];
		aCosineTerm = (float)(aCosine * (double)anEndX + (double)theBumper->mWorld[AttributeX]);
		theEdges[anEdge][2] = (float)((double)aCosineTerm - aSine * (double)anEndY);
		aSineTerm = (float)(aSine * (double)anEndX + (double)theBumper->mWorld[AttributeY]);
		theEdges[anEdge][3] = (float)(aCosine * (double)anEndY + (double)aSineTerm);
	}

	int aNearestEdge = -1;
	float aSmallestDot = 100000000.0f;
	SceneFilter* aVelocityX = mStages.mCursor->FindFilter(FilterVelocity, AttributeX);
	SceneFilter* aVelocityY = mStages.mCursor->FindFilter(FilterVelocity, AttributeY);
	if (aVelocityX == nullptr)
		aVelocityX = mStages.mCursor->AddFilter(FilterVelocity, AttributeX, 0, 0, 0, false);
	if (aVelocityY == nullptr)
		aVelocityY = mStages.mCursor->AddFilter(FilterVelocity, AttributeY, 0, 0, 0, false);
	for (int anEdge = 0; anEdge < theEdgeCount; anEdge++) {
		float aDistance = (float)SegmentDistance(mWorld[AttributeX], mWorld[AttributeY], theEdges[anEdge][0],
			theEdges[anEdge][1], theEdges[anEdge][2], theEdges[anEdge][3]);
		if (aDistance < theRadius) {
			float aNormalX;
			float aNormalY;
			SegmentNormal(theEdges[anEdge][0], theEdges[anEdge][1], theEdges[anEdge][2], theEdges[anEdge][3], &aNormalX,
				&aNormalY);
			float aDot = aVelocityX->mAmplitude * aNormalX + aVelocityY->mAmplitude * aNormalY;
			if (aDot < aSmallestDot) {
				aNearestEdge = anEdge;
				aSmallestDot = aDot;
			}
		}
	}
	if (aNearestEdge != -1) {
		ReflectOffSegment(&aVelocityX->mAmplitude, &aVelocityY->mAmplitude, theEdges[aNearestEdge][0],
			theEdges[aNearestEdge][1], theEdges[aNearestEdge][2], theEdges[aNearestEdge][3]);
		float aDistance = 99999.0f;
		do {
			Update();
			for (int anEdge = 0; anEdge < theEdgeCount; anEdge++) {
				aDistance = (float)SegmentDistance(mWorld[AttributeX], mWorld[AttributeY], theEdges[anEdge][0],
					theEdges[anEdge][1], theEdges[anEdge][2], theEdges[anEdge][3]);
			}
		} while (aDistance < theRadius && !mIsDead);
		mStages.mCursor->mFrame = 1.0f;
		aBounced = true;
	}
	return aBounced;
}
