// The layer (0x40c0f0-0x40e820, and the lookups of 0x408710): the scroll of the level's tiles and the Survival mode's plates,
// wall decorations and minute markers.

#include "Layer.h"

#include "ImageRecord.h"
#include "Level.h"
#include "Profiles.h"
#include "SceneLoader.h"
#include "TextObject.h"
#include "Tuning.h"
#include "VeniceGlobals.h"
#include "VeniceMath.h"
#include "VeniceSound.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace
{
// the plate layer's first lookups find nothing this far off
const float LowestSearchStart = -99995.0f;
const float HighestSearchStart = 99995.0f;

// the scroll speed (a velocity filter on the tiles layer's y) the layer settles at
const float ScrollFilterAmplitude = 0.01f;
const float ScrollFilterPeriod = 0.0009f;
const float ScrollFilterOffset = 0.983f;

const float MaxScrollSpeedRatio = 10.0f;
const float JourneySpeedFactor = 3.0f;
const float JourneyMaxSpeed = 6.0f;
const float NearSlowPointSpeedLimit = 0.24f;
const float EndlessNearSlowPointSpeedLimit = 0.44f;
const float EndlessSlowPointOffset = 60.0f;
const float EndlessBaseSpeedFactor = 5.0f;
const float ScrollSmoothingKeep = 99.0f;
const float ScrollSmoothingTake = 0.6f;
const float ScrollSmoothingDivisor = 100.0f;
const float ScrollBonusStep = 0.00001f;
const int ScrollBonusPeriod = 4000;

const float FallenChildHeightScreens = 3.0f;
const float FallenPlateMinTicks = 100.0f;

const int PlateFileRampTicks = 7500; // the Survival mode brings in a new kind of plate every 7500 updates
const float PlateFileFirstLimit = 2.0f;

const float SurvivalPlateStartY = -400.0f;
const float SurvivalPlateGap = 20.0f;
const float SurvivalPlateSideGap = 10.0f;
const float SurvivalRightEdge = 380.0f;
const float SurvivalLastColumn = 282.0f;

const int WallDecorationFirstX = -0x118;
const int WallDecorationLastX = 0x118;
const int WallDecorationStepX = 0x50;
const float WallDecorationYOffset = 80.0f;
const int WallDecorationGapAfterGrate = 3;

const int MinuteMarkerStartTicks = 50000; // in milliseconds
const int MinuteMarkerPeriod = 60000;
const int MinuteMarkerColors = 6;
const int MinuteMarkersPerColor = 10;
const float MinuteMarkerX = 385.0f;
const float MinuteMarkerZOrder = 9.0f;
const float MinuteMarkerFirstY = -260.0f;
const float MinuteMarkerSpacing = 8.0f;
const float MinuteTextX = 360.0f;
const float MinuteTextSpacing = -1.0f;

// the wall decorations' pictures and counters, shared by every layer
bool gWallDecorLoaded = false; // 0x5ba294
ImageRecord* gBrickImage = nullptr; // 0x5ba290
ImageRecord* gWallSideImage = nullptr; // 0x5ba28c
int gWallDecorGap = 3; // 0x5b5150: the tile rows left before another grate or ivy may come
int gPlateSide = 0; // 0x5ba298: which side of the middle the next row of plates starts on

// 0x40da70's list: the plate files of the Survival mode, by the number the random pick gives
const char* const SurvivalPlateFiles[] = {"objects\\stick_plate.xml", "objects\\round_plate.xml",
	"objects\\triangle_plate.xml", "objects\\x_plate.xml", "objects\\block_plate.xml", "objects\\semicircle_plate.xml",
	"objects\\spinner_plate.xml", "objects\\a_plate.xml", "objects\\tosser_plate.xml", "objects\\spinner_plate.xml",
	"objects\\mini_round_plate.xml", "objects\\mini_stick_plate.xml", "objects\\m_plate.xml", "objects\\square_plate.xml",
	"objects\\a_plate.xml", "objects\\step_plate.xml", "objects\\t_plate.xml"};
const int SurvivalPlateFileCount = sizeof(SurvivalPlateFiles) / sizeof(SurvivalPlateFiles[0]);
const char* const DefaultSurvivalPlateFile = "objects\\stick_plate.xml";

// A new child of the middle layer that stays where it is put (a stage that loops, with no movement of its own)
SceneObject* NewWallObject()
{
	SceneObject* anObject = gMiddleLayer->NewChild(nullptr);
	anObject->AddStage(1000.0f, 1, 1, 1, nullptr);
	anObject->SetAttribute(AttributeX, 0, 0);
	anObject->SetAttribute(AttributeY, 0, 0);
	return anObject;
}

// A grate or a clump of ivy in front of the wall
void AddWallPicture(const char* thePicture, float theX, float theY)
{
	SceneObject* aPicture = NewWallObject();
	aPicture->SetImage(thePicture, 0, 0, 0, 0);
	aPicture->mBilinear = 0;
	aPicture->SetZOrder(1.0f);
	aPicture->Move(theX, theY);
}
} // namespace

SceneObject* FindLowestOfType(int theType, SceneObject* theLayer)
{
	if (theLayer == nullptr)
		theLayer = gPlateLayer;
	float aLowest = LowestSearchStart;
	SceneObject* aFound = nullptr;
	for (SceneObject* aChild = theLayer->mChildren.mHead; aChild != nullptr; aChild = aChild->mNext) {
		if (aChild->mType == theType && aLowest < aChild->mWorld[AttributeY]) {
			aLowest = aChild->mWorld[AttributeY];
			aFound = aChild;
		}
	}
	return aFound;
}

SceneObject* FindHighestOfType(int theType, SceneObject* theLayer)
{
	if (theLayer == nullptr)
		theLayer = gPlateLayer;
	float aHighest = HighestSearchStart;
	SceneObject* aFound = nullptr;
	for (SceneObject* aChild = theLayer->mChildren.mHead; aChild != nullptr; aChild = aChild->mNext) {
		if (aChild->mType == theType && aChild->mWorld[AttributeY] < aHighest) {
			aHighest = aChild->mWorld[AttributeY];
			aFound = aChild;
		}
	}
	return aFound;
}

Layer::Layer()
{
	mType = ObjectTypeLayer;
}

void Layer::Load(const char* theText, SceneTag* theTag)
{
	mMerged = theTag->GetInt("merged");
	SceneObject::Load(theText, theTag);
	bool aMergesTiles = strcmp(mName, "tiles") == 0 && CurrentProfile().mMode != GameModeSurvival;
	if (aMergesTiles || strcmp(mName, "background") == 0)
		mMerged = 1;
}

void Layer::OnObjectClosed()
{
	if (strstr(mName, "plate_container") != nullptr)
		SetZOrder(mParent->mChildren.mTail->mZOrder + 1.0f);
}

void Layer::UpdateContentHeight()
{
	if (mContentHeight == 0.0f || mMerged != 0) {
		mHighestEdge = 999999.0f;
		mLowestEdge = -999999.0f;
		for (SceneObject* aChild = mChildren.mHead; aChild != nullptr; aChild = aChild->mNext) {
			if (aChild->mImage != nullptr && aChild->mType == ObjectTypeBase) {
				float aHalfHeight = (aChild->mSourceBottom - aChild->mSourceTop) / 2.0f;
				float aBottom = aChild->mLocal[AttributeY] + aHalfHeight;
				if (mLowestEdge < aBottom)
					mLowestEdge = aChild->mLocal[AttributeY] + aHalfHeight;
				if (aChild->mLocal[AttributeY] - aHalfHeight < mHighestEdge)
					mHighestEdge = aChild->mLocal[AttributeY] - aHalfHeight;
				mContentHeight = mLowestEdge - mHighestEdge;
			}
		}
	}
	// the tiles move a little less than the layer around them, so that what is behind them seems farther off
	if (CurrentProfile().mMode != GameModeSurvival && 0.0f < mContentHeight && mParent->mType == ObjectTypeLayer &&
		static_cast<Layer*>(mParent)->mContentHeight != 0.0f && strcmp(mName, "tiles") == 0) {
		Layer* aParent = static_cast<Layer*>(mParent);
		float aRatio = aParent->mContentHeight / mContentHeight;
		float aParentY = aParent->mLocal[AttributeY];
		aParent->mLocal[AttributeY] = mLocal[AttributeY] * aRatio;
		float aParentMove = aParent->mLocal[AttributeY] - aParentY;
		mLocal[AttributeY] = mLocal[AttributeY] - aParentMove * aRatio;
	}
}

// The original draws the layer's children into pictures 1022 pixels tall (the tiles of the level, or the Survival mode's
// background) and puts those pictures in the children's place, so that a level draws a few big pictures instead of a few
// hundred small ones. The port draws the children themselves: the picture is the same, only the number of draw calls differs.
void Layer::MergeChildren()
{
	mMerged = 0;
}

bool Layer::Update()
{
	UpdateContentHeight();
	if (mMerged != 0 && gTutorial == nullptr)
		MergeChildren();
	Profile& aProfile = CurrentProfile();
	bool aIsTiles = strcmp(mName, "tiles") == 0;
	if (aProfile.mMode == GameModeFlood && aIsTiles && strcmp(mStages.mCursor->mName, "scrolling") == 0) {
		if (aProfile.mExplainFloodSeen == 0) {
			aProfile.mExplainFloodSeen = 1;
			PushSceneByName("explain_flood");
		}
		if (GetRoot()->mNext == nullptr)
			AdjustScroll();
	} else if (aProfile.mMode == GameModeTrickShot && aIsTiles && strcmp(mStages.mCursor->mName, "still") == 0)
		AdjustScroll();
	if (aProfile.mMode == GameModeJourney && aIsTiles && strcmp(mStages.mCursor->mName, "scrolling") == 0)
		AdjustScroll();
	else if (aProfile.mMode == GameModeSurvival && aIsTiles)
		EndlessScroll();
	return SceneObject::Update();
}

void Layer::AdjustScroll()
{
	SceneObject* aScrollPoint = FindLowestOfType(ObjectTypeScrollPoint, nullptr);
	if (aScrollPoint == nullptr)
		return;
	Profile& aProfile = CurrentProfile();
	SceneStage* aStage = mStages.mCursor;
	SceneFilter* aFilter = aStage->FindFilter(FilterVelocity, AttributeY);
	if (aFilter == nullptr) {
		SceneFilter* aNewFilter = aStage->AddFilter(FilterVelocity, AttributeY, ScrollFilterAmplitude, ScrollFilterPeriod,
			ScrollFilterOffset, false);
		if (aProfile.mMode == GameModeJourney && strstr(aStage->mName, "scrolling") != nullptr)
			aNewFilter->mAmplitude = 0;
	} else {
		// how far the scroll point is from the point where the scroll slows, in quarters of the screen's height
		float aDistance = (float)(VeniceAbs(gTuning.mScrollingSlowPoint - aScrollPoint->mWorld[AttributeY]) /
			((double)gScreenHeight / 4.0));
		float aSpeed;
		if (aDistance <= 1.0f) {
			float aBaseSpeed = gTuning.mBaseScrollSpeed + aProfile.mDifficulty;
			aSpeed = aDistance + aBaseSpeed;
		} else {
			aDistance = aDistance * aDistance;
			aSpeed = aDistance > MaxScrollSpeedRatio ? MaxScrollSpeedRatio : aDistance;
		}
		if (aProfile.mMode == GameModeJourney) {
			float aFasterSpeed = aSpeed * JourneySpeedFactor;
			aSpeed = aFasterSpeed > JourneyMaxSpeed ? JourneyMaxSpeed : aFasterSpeed;
		}
		if (gTuning.mScrollingSlowPoint < aScrollPoint->mWorld[AttributeY]) {
			if (strstr(aStage->mName, "scrolling") == nullptr)
				aStage->mAttributeMask &= ~(1u << AttributeY);
			else
				aSpeed = aSpeed > NearSlowPointSpeedLimit ? NearSlowPointSpeedLimit : aSpeed;
			if (aProfile.mMode == GameModeJourney && strstr(aStage->mName, "scrolling") != nullptr) {
				aSpeed = 0;
				aFilter->mAmplitude = 0;
			}
		}
		aFilter->mPeriod = 0;
		aFilter->mAmplitude = (aFilter->mAmplitude * ScrollSmoothingKeep + ScrollSmoothingTake * aSpeed) /
			ScrollSmoothingDivisor;
	}
	if (0.0f > mLocal[AttributeY])
		mLocal[AttributeY] = 0;
}

void Layer::EndlessScroll()
{
	SceneObject* aLowestPlate = FindLowestOfType(ObjectTypePlate, nullptr);
	RemoveFallenChildren();
	SpawnSurvivalPlates();
	SpawnWallDecoration();
	MakeMinuteMarker();
	if ((int)mTickCount % ScrollBonusPeriod == 0)
		mScrollBonus = mScrollBonus + ScrollBonusStep;
	SceneStage* aStage = mStages.mCursor;
	if (strcmp(aStage->mName, "scrolling") != 0 || aLowestPlate == nullptr)
		return;
	SceneFilter* aFilter = aStage->FindFilter(FilterVelocity, AttributeY);
	if (aFilter == nullptr)
		aStage->AddFilter(FilterVelocity, AttributeY, ScrollFilterAmplitude, ScrollFilterPeriod, ScrollFilterOffset, false);
	else {
		float aSlowPoint = gTuning.mScrollingSlowPoint - EndlessSlowPointOffset;
		float aDistance = (float)(VeniceAbs(aSlowPoint - aLowestPlate->mWorld[AttributeY]) / ((double)gScreenHeight / 4.0));
		float aSpeed;
		if (aDistance <= 1.0f)
			aSpeed = aDistance + gTuning.mBaseScrollSpeed * EndlessBaseSpeedFactor;
		else {
			aDistance = aDistance * aDistance;
			aSpeed = aDistance > MaxScrollSpeedRatio ? MaxScrollSpeedRatio : aDistance;
		}
		if (aSlowPoint < aLowestPlate->mWorld[AttributeY] && strstr(aStage->mName, "scrolling") != nullptr)
			aSpeed = aSpeed > EndlessNearSlowPointSpeedLimit ? EndlessNearSlowPointSpeedLimit : aSpeed;
		aFilter->mPeriod = 0;
		aFilter->mAmplitude = (aFilter->mAmplitude * ScrollSmoothingKeep + ScrollSmoothingTake * aSpeed) /
				ScrollSmoothingDivisor +
			mScrollBonus;
	}
	if (0.0f > mLocal[AttributeY])
		mLocal[AttributeY] = 0;
}

// Plates and the middle layer's children that fell below the screen go: a plate only once it has had time to be seen
void Layer::RemoveFallenChildren()
{
	for (SceneObject* aChild = gMiddleLayer->mChildren.mHead; aChild != nullptr; aChild = aChild->mNext) {
		if (aChild->mType != ObjectTypeLayer && gScreenHeight * FallenChildHeightScreens < aChild->mWorld[AttributeY])
			aChild->mIsDead = true;
	}
	for (SceneObject* aChild = gPlateLayer->mChildren.mHead; aChild != nullptr; aChild = aChild->mNext) {
		if (aChild->mTickCount > FallenPlateMinTicks && aChild->mType != ObjectTypeLayer &&
			gScreenHeight * FallenChildHeightScreens < aChild->mWorld[AttributeY])
			aChild->mIsDead = true;
	}
}

const char* Layer::PickPlateFile()
{
	float aLimit = mTickCount / (float)PlateFileRampTicks;
	if (PlateFileFirstLimit > aLimit)
		aLimit = PlateFileFirstLimit;
	int aNumber = (int)aLimit;
	if (aNumber != 0)
		aNumber = RandomBetween(0, aNumber);
	if (aNumber >= 0 && aNumber < SurvivalPlateFileCount)
		return SurvivalPlateFiles[aNumber];
	return DefaultSurvivalPlateFile;
}

// A row of plates above the highest one once it has come into view: from the middle out to both sides, each a random kind
void Layer::SpawnSurvivalPlates()
{
	SceneObject* aHighestPlate = FindHighestOfType(ObjectTypePlate, gPlateLayer);
	float aHighestY = SurvivalPlateStartY;
	if (aHighestPlate != nullptr)
		aHighestY = aHighestPlate->mLocal[AttributeY];
	if (aHighestPlate != nullptr && !((0.0f - gScreenHeight) / 2.0f < aHighestPlate->mWorld[AttributeY]))
		return;
	gPlateSide = 1 - gPlateSide;
	float aX = 0;
	SceneObject* aPlate;
	const char* aFile;
	while (true) {
		aFile = PickPlateFile();
		aPlate = LoadSceneFile(aFile, gPlateLayer);
		float aY = aHighestY - SurvivalPlateGap;
		if (aHighestPlate != nullptr)
			aY = aY - (aHighestPlate->mImage->mFrameHeight / 2.0f + aPlate->mImage->mFrameHeight / 2.0f);
		if (aX != 0.0f || (aX == 0.0f && gPlateSide != 0))
			aX = aPlate->mImage->mFrameWidth / 2.0f + SurvivalPlateSideGap + aX;
		if (SurvivalRightEdge < aPlate->mImage->mFrameWidth / 2.0f + aX || SurvivalLastColumn < aX)
			break;
		aPlate->Move(aX, aY);
		if (aX != 0.0f) {
			aPlate = LoadSceneFile(aFile, gPlateLayer);
			aPlate->Move(0.0f - aX, aY);
		}
		aX = aPlate->mImage->mFrameWidth / 2.0f + SurvivalPlateSideGap + aX;
	}
	aPlate->mIsDead = true;
}

// A row of wall tiles above the highest one, now and then with a grate or two clumps of ivy
void Layer::SpawnWallDecoration()
{
	if (!gWallDecorLoaded) {
		gWallDecorLoaded = true;
		gBrickImage = AcquireImageRecord("gfx\\background01\\brick_brown01.jpg", 0, 0, 0, 0);
		gWallSideImage = AcquireImageRecord("gfx\\background01\\wall_side01.png", 0, 0, 0, 0);
	}
	SceneObject* aHighestTile = FindHighestOfType(ObjectTypeBase, gMiddleLayer);
	float aRowY = aHighestTile->mLocal[AttributeY] - WallDecorationYOffset;
	if (!((0.0f - gScreenHeight) / 2.0f < aHighestTile->mWorld[AttributeY] && aHighestTile->mWorld[AttributeY] < 0.0f))
		return;
	gWallDecorGap = gWallDecorGap - 1;
	for (int aX = WallDecorationFirstX; aX < WallDecorationLastX + 1; aX = aX + WallDecorationStepX) {
		SceneObject* aTile = NewWallObject();
		aTile->mBilinear = 0;
		if (aX == WallDecorationFirstX)
			aTile->SetImageRecord(gWallSideImage);
		else if (aX == WallDecorationLastX) {
			aTile->SetImageRecord(gWallSideImage);
			aTile->mFlipX = true;
		} else
			aTile->SetImageRecord(gBrickImage);
		aTile->mSourceRight = aTile->mSourceRight + 1.0f;
		aTile->mSourceBottom = aTile->mSourceBottom + 1.0f;
		aTile->Move((float)aX, aRowY);
		if (gWallDecorGap < 1 && RandomBetween(2, 4) == 2 && aX != 0 && abs(aX) < WallDecorationLastX) {
			gWallDecorGap = WallDecorationGapAfterGrate;
			if (RandomBetween(0, 1) == 0) {
				aTile->mIsDead = true;
				AddWallPicture("gfx\\background01\\grate_square.png", (float)aX, aRowY);
			} else {
				AddWallPicture("gfx\\background01\\ivy01.png", (float)aX, aRowY);
				AddWallPicture("gfx\\background01\\ivy01.png", (float)-aX, aRowY);
			}
		}
	}
}

// Every minute of play after the first a marker with its sparkles comes up the side of the screen, and a text says how long the
// player has lasted
void Layer::MakeMinuteMarker()
{
	int aMilliseconds = (int)gActiveLevel->mTickCount << 2;
	int aMinutes = aMilliseconds / MinuteMarkerPeriod;
	if (!(aMilliseconds % MinuteMarkerPeriod == 0 && MinuteMarkerStartTicks < aMilliseconds && gMiddleLayer != nullptr))
		return;
	if (strstr(gMiddleLayer->mStages.mCursor->mName, "scroll") == nullptr)
		return;
	char aPicture[64];
	snprintf(aPicture, sizeof(aPicture), "gfx\\survival_marker_%d.png",
		((aMinutes - 1) / MinuteMarkersPerColor) % MinuteMarkerColors);
	SceneObject* aMarker = new SceneObject();
	aMarker->mZOrder = MinuteMarkerZOrder;
	strcpy(aMarker->mName, "endless_marker");
	aMarker->SetImage(aPicture, 0, 0, 0, 0);
	aMarker->AddStage(130.0f, 0, 0, 1, nullptr);
	aMarker->SetAttribute(AttributeXScale, 0, 1.0f);
	aMarker->SetAttribute(AttributeYScale, 0, 1.0f);
	aMarker->AddFilter(FilterSined, AttributeXScale, 0.5f, 0.5f, 0);
	aMarker->AddFilter(FilterSined, AttributeYScale, 0.5f, 0.5f, 0);
	aMarker->AddEvent(TriggerEnd, ActionMakeSparkles, "14", "", "", "", "");
	aMarker->AddEvent(TriggerStart, ActionPlaySound, "audio\\minute.ogg", "audio\\minute.ogg", "audio\\minute.ogg",
		"audio\\minute.ogg", "");
	float aY = (float)((aMinutes - 1) % MinuteMarkersPerColor) * (aMarker->mImage->mFrameHeight + MinuteMarkerSpacing) +
		MinuteMarkerFirstY;
	aMarker->SetAttribute(AttributeX, MinuteMarkerX, MinuteMarkerX);
	aMarker->SetAttribute(AttributeY, aY, aY);
	aMarker->AddStage(1000.0f, 1, 1, 1, nullptr);
	aMarker->SetAttribute(AttributeX, MinuteMarkerX, MinuteMarkerX);
	aMarker->SetAttribute(AttributeY, aY, aY);
	gActiveLevel->NewChild(aMarker);

	aY = aY + 2.0f;
	TextObject* aText = new TextObject();
	char aMessage[32];
	snprintf(aMessage, sizeof(aMessage), "%d MINUTE%s", aMinutes, aMinutes < 2 ? " " : "S");
	aText->SetText(aMessage, "gfx\\font1_small.png", MinuteTextSpacing, 0, JustifyRight);
	aText->AddStage(100.0f, 0, 0, 1, nullptr);
	aText->SetAttribute(AttributeX, MinuteTextX, MinuteTextX);
	aText->SetAttribute(AttributeY, aY, aY);
	aText->SetAttribute(AttributeXScale, 0.1f, 0.8f);
	aText->SetAttribute(AttributeYScale, 0.1f, 0.8f);
	aText->SetAttribute(AttributeAlpha, 0, 1.0f);
	aText->AddFilter(FilterSquared, AttributeAll, 0, 0, 0);
	aText->AddStage(1000.0f, 0, 0, 1, nullptr);
	aText->AddStage(100.0f, 0, 0, 1, nullptr);
	aText->SetAttribute(AttributeX, MinuteTextX, MinuteTextX);
	aText->SetAttribute(AttributeY, aY, aY);
	aText->SetAttribute(AttributeXScale, 0.8f, 0.1f);
	aText->SetAttribute(AttributeYScale, 0.8f, 0.1f);
	aText->SetAttribute(AttributeAlpha, 1.0f, 0);
	gActiveLevel->NewChild(aText);
}

void Layer::Shake(float theHorizontalAmplitude, float theVerticalAmplitude)
{
	SceneStage* aStage = mStages.mCursor;
	if (aStage->FindFilter(FilterSined, AttributeX) == nullptr) {
		int aPeriod = RandomBetween(6, 8);
		aStage->AddFilter(FilterSined, AttributeX, theHorizontalAmplitude, (float)aPeriod, 0, false);
		aStage->mFilters.mCursor->mDuration = 250.0f;
		aStage->AddFilter(FilterSined, AttributeY, theVerticalAmplitude, (float)aPeriod - 1.0f, 0, false);
		aStage->mFilters.mCursor->mDuration = 250.0f;
		PlaySoundByName("audio\\shake.ogg", -1);
	}
}
