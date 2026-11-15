// The mode lock (0x4051f0).

#include "LevelLocker.h"

#include "Profiles.h"
#include "TextObject.h"
#include "Tooltip.h"

#include <cstring>

// The name of the mode is the locker's text child, and the mode it names decides whether it is shut: Flood opens once the player
// has gone past the second level, Trick Shot once Journey is finished and Survival once Flood is
void LevelLocker::OnObjectClosed()
{
	TextObject* aName = static_cast<TextObject*>(FindChildByType(ObjectTypeText, nullptr));
	if (aName == nullptr)
		return;
	Profile& aProfile = CurrentProfile();
	bool aLocked = strstr(aName->mText, "FLOOD") != nullptr && aProfile.mFlagReachedBuilding3 == 0;
	if (strstr(aName->mText, "TRICK") != nullptr && aProfile.mFlagFinishedRelaxed == 0)
		aLocked = true;
	if (strstr(aName->mText, "SURVI") != nullptr && aProfile.mFlagFinishedGame == 0)
		aLocked = true;
	if (!aLocked)
		return;

	aName->mStages.Clear();
	aName->AddStage(1000.0f, 1, 1, 1, nullptr);
	aName->SetAttribute(AttributeAlpha, 0.5f, 0.5f);
	aName->mLocal[AttributeAlpha] = 0.5f;
	// a lock on each side of the name, a little past its ends
	float aLockX = (float)((double)aName->mFont->MeasureWidth(aName->mText, 0.59f, aName->mSpacing) / 2.0 + 16.0);
	SceneObject* aRightLock = NewChild(nullptr);
	aRightLock->AddStage(1000.0f, 1, 1, 1, nullptr);
	aRightLock->SetImage("gfx\\locked_game.png", 0, 0, 0, 0);
	aRightLock->SetAttribute(AttributeX, aLockX, aLockX);
	SceneObject* aLeftLock = NewChild(nullptr);
	aLeftLock->AddStage(1000.0f, 1, 1, 1, nullptr);
	aLeftLock->SetImage("gfx\\locked_game.png", 0, 0, 0, 0);
	aLeftLock->SetAttribute(AttributeX, 0.0f - aLockX, 0.0f - aLockX);

	// the tooltip of the name says what opens the mode, and its box shows from the start
	SceneObject* aTooltip = aName->FindChildByType(ObjectTypeTooltip, nullptr);
	if (aTooltip != nullptr) {
		aTooltip->mChildren.Clear();
		char* aTooltipText = static_cast<Tooltip*>(aTooltip)->mText;
		if (strstr(aName->mText, "FLOOD") != nullptr)
			strcpy(aTooltipText, "Play a little Journey first!");
		else if (strstr(aName->mText, "TRICK") != nullptr)
			strcpy(aTooltipText, "Beat all of Journey to unlock!");
		else
			strcpy(aTooltipText, "Beat all of Flood to unlock!");
		aTooltip->OnObjectClosed();
		aTooltip->mChildren.mHead->mLocal[AttributeAlpha] = 2.0f;
	}
}
