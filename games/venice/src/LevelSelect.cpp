// The level select screen (0x411170-0x411c40).

#include "LevelSelect.h"

#include "ImageRecord.h"
#include "Layer.h"
#include "Level.h"
#include "Profiles.h"
#include "Quotes.h"
#include "SceneFiles.h"
#include "SceneTag.h"
#include "VeniceGlobals.h"

#include <cstring>

namespace
{
// 0x5b5154: the z order of the next thumbnail, each a little in front of the one before
float gThumbnailZOrder = 2.0f;

const int ThumbnailsPerGroup = 9;
}

LevelSelect::LevelSelect()
{
	mType = ObjectTypeLevelSelect;
}

// A thumbnail is made for the first level of each group of nine and for the levels of the winning sequence (a picture that
// stands for the levels after it), and every level with a thumbnail moves the marker's position on by the width of its picture
// (a ninth of it, less four). The marker, the shooter, goes from the position before to the one of the level the player is up to.
void LevelSelect::Load(const char* theText, SceneTag* theTag)
{
	mPositioned = 0;
	SceneObject::Load(theText, theTag);
	float aThumbnailX = 100.0f;
	int aLevel = 0;
	float aPreviousMarkerX = 0.0f;
	Layer* aLayer = static_cast<Layer*>(mParent);
	aLayer->mThumbnailsWidth = 0;
	CursorList<SceneFile>& aFiles = CurrentProfile().mMode == GameModeJourney ? gRelaxedScenes : gGameScenes;
	for (SceneFile* aFile = aFiles.mHead; aFile != nullptr; aFile = aFile->mNext) {
		SceneTag aFileTag;
		ReadTag(aFile->mText.c_str(), &aFileTag);
		if (!aFileTag.Has("thumbnail"))
			continue;
		bool aIsWinSequence = strstr(aFile->mName, "win_seq") != nullptr;
		if (aLevel % ThumbnailsPerGroup == 1 || aIsWinSequence) {
			SceneObject* aThumbnail = NewChild(nullptr);
			aThumbnail->AddStage(1000.0f, 1, 1, 1, nullptr);
			aThumbnail->SetZOrder(gThumbnailZOrder);
			gThumbnailZOrder = gThumbnailZOrder - 0.001f;
			float aX = aIsWinSequence ? aThumbnailX + 90.0f : aThumbnailX;
			aThumbnail->SetAttribute(AttributeX, aX, aX);
			aThumbnail->SetAttribute(AttributeY, -110.0f, -110.0f);
			aThumbnail->SetImage(aFileTag.GetString("thumbnail"), 0, 0, 0, 0);
			aLayer->mThumbnailsWidth = aLayer->mThumbnailsWidth + aThumbnail->mImage->mFrameWidth;
		}
		float aMarkerX = aThumbnailX + -440.0f;
		float aMarkerY = mLocal[AttributeY] + 190.0f;
		float aFromX = aPreviousMarkerX == 0.0f ? aMarkerX - 120.0f : aPreviousMarkerX;
		int aLevelIndex = CurrentProfile().mLevelIndex[CurrentProfile().mMode];
		bool aIsCurrentLevel = aLevel == (aLevelIndex < 0 ? 0 : aLevelIndex);
		aLevel++;
		if (aIsCurrentLevel) {
			SceneObject* aMarker = mParent->NewChild(nullptr);
			strcpy(aMarker->mName, "marker");
			aMarker->SetImage("gfx\\levelselect\\ls_shooter.png", 0, 0, 0, 0);
			aMarker->mClipLeft = -999999;
			aMarker->mClipTop = 0;
			aMarker->mClipRight = 999999;
			aMarker->mClipBottom = 580;
			aMarker->AddStage(1500.0f, 0, 0, 1, nullptr);
			aMarker->SetAttribute(AttributeX, aFromX, aFromX);
			aMarker->SetAttribute(AttributeY, aMarkerY, aMarkerY);
			aMarker->AddStage(2000.0f, 0, 0, 1, nullptr);
			aMarker->SetAttribute(AttributeX, aFromX, aMarkerX);
			aMarker->SetAttribute(AttributeY, aMarkerY, aMarkerY);
			aMarker->AddFilter(FilterSuperCos, AttributeAll, 1.0f, 0.0f, 0.0f);
			aMarker->AddStage(1000.0f, 1, 1, 1, nullptr);
			aMarker->SetAttribute(AttributeX, aMarkerX, aMarkerX);
			aMarker->SetAttribute(AttributeY, aMarkerY, aMarkerY);
			aMarker->SetZOrder(12.0f);
			gLevelNumber = aLevel - 1;
		}
		aPreviousMarkerX = aMarkerX;
		ImageRecord* aThumbnailImage = AcquireImageRecord(aFileTag.GetString("thumbnail"), 0, 0, 0, 0);
		aThumbnailX = (aThumbnailImage->mFrameWidth / 9.0f - 4.0f) + aThumbnailX;
	}
	ShowLevelQuote();
}

// The screen and the strip draw by themselves: each child is drawn as itself and its children in full
void LevelSelect::Draw()
{
	if (mTickCount != 0.0f) {
		Position();
		DrawSelf();
		for (SceneObject* aChild = mChildren.mHead; aChild != nullptr; aChild = aChild->mNext) {
			aChild->DrawSelf();
			for (SceneObject* aGrandChild = aChild->mChildren.mHead; aGrandChild != nullptr; aGrandChild = aGrandChild->mNext)
				aGrandChild->Draw();
		}
	}
}

// The two layers above the strip start their slide at the strip's start and end it where the marker's level is, half the
// strip's width back (not past the start). At the first level they slide at half speed, and further on the marker's own slide
// is a quarter as long.
void LevelSelect::Position()
{
	if (mPositioned != 0)
		return;
	mPositioned = 1;
	SceneObject* anOuterLayer = gActiveLevel->FindChildByType(ObjectTypeLayer, nullptr);
	Layer* anInnerLayer = static_cast<Layer*>(anOuterLayer->FindChildByType(ObjectTypeLayer, nullptr));
	float aStartX = (0.0f - anInnerLayer->mThumbnailsWidth) / 2.0f;
	float anEndX = (0.0f - mParent->FindChildByName("marker", nullptr)->mStages.mCursor->mEnd[AttributeX]) / 2.0f;
	if (anEndX > 0.0f)
		anEndX = 0.0f;
	Profile& aProfile = CurrentProfile();
	if (aProfile.mLevelIndex[aProfile.mMode] <= 0) {
		anOuterLayer->mStages.mHead->mLength = anOuterLayer->mStages.mHead->mLength * 2.0f;
		anInnerLayer->mStages.mHead->mLength = anInnerLayer->mStages.mHead->mLength * 2.0f;
	} else {
		aStartX = anEndX;
		SceneObject* aMarker = mParent->FindChildByName("marker", nullptr);
		if (aMarker != nullptr)
			aMarker->mStages.mHead->mLength = aMarker->mStages.mHead->mLength / 4.0f;
	}
	anOuterLayer->mStages.mHead->mStart[AttributeX] = aStartX;
	anInnerLayer->mStages.mHead->mStart[AttributeX] = aStartX;
	anOuterLayer->mStages.mHead->mEnd[AttributeX] = anEndX;
	anInnerLayer->mStages.mHead->mEnd[AttributeX] = anEndX;
}

void LevelScroller::Draw()
{
	SceneObject::Draw();
	DrawHot();
}

// The outer layer's first stage becomes a 1.2 s slide of half a screen, to the side this scroller is on, from where the layer
// is, when a thumbnail is past that side's edge and the layer is not at its first stage. (The original runs the setup twice.)
void LevelScroller::OnCustomEvent()
{
	SceneObject* anOuterLayer = gOuterLayer;
	float aDirection = 0.0f <= 0.0f - mLocal[AttributeX] ? 1.0f : -1.0f;
	float aShift = (aDirection * gScreenWidth) / 2.0f;
	bool aThumbnailPastEdge = false;
	SceneObject* aLevelSelect = gActiveLevel->FindChildByType(ObjectTypeLayer, nullptr)
		->FindChildByType(ObjectTypeLayer, nullptr)->FindChildByType(ObjectTypeLevelSelect, nullptr);
	for (SceneObject* aThumbnail = aLevelSelect->mChildren.mHead; aThumbnail != nullptr; aThumbnail = aThumbnail->mNext) {
		if (0.0f <= aShift) {
			if (aThumbnail->mWorld[AttributeX] < 0.0f)
				aThumbnailPastEdge = true;
		} else if (gScreenWidth < aThumbnail->mWorld[AttributeX]) {
			aThumbnailPastEdge = true;
		}
	}
	if (aThumbnailPastEdge && anOuterLayer->mStages.mCursor != anOuterLayer->mStages.mHead) {
		for (int aPass = 0; aPass < 2; aPass++) {
			SceneStage* aFirstStage = anOuterLayer->mStages.mHead;
			strcpy(aFirstStage->mName, "scroll");
			aFirstStage->mLength = 300.0f;
			aFirstStage->mStart[AttributeX] = anOuterLayer->mLocal[AttributeX];
			aFirstStage->mEnd[AttributeX] = anOuterLayer->mLocal[AttributeX] + aShift;
			anOuterLayer->GoToStageByName("scroll");
		}
	}
}
