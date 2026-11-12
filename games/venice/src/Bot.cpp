// port: the bot (Bot.h)

#include "Bot.h"

#include "Jewel.h"
#include "Level.h"
#include "Plate.h"
#include "Shooter.h"
#include "Slot.h"
#include "VeniceGlobals.h"

#include <cmath>
#include <cstdlib>
#include <cstring>

namespace
{
const int AimY = 300; // the pointer's height: only its x aims, and no panel button is here
const float TopMargin = 30.0f; // a slot nearer the top of the screen than this is not aimed at
const float ShooterMargin = 40.0f; // nor one that close above the shooter
const float FollowDivisor = 1.3f; // the shooter closes its distance to the pointer by this each update (Shooter::FollowMouse)
const float AimedWithin = 0.5f; // pixels

bool LevelIsInPlay()
{
	return gScenes.mTail != nullptr && gActiveLevel != nullptr && gActiveLevel == gScenes.mTail && gShooter != nullptr &&
		gMiddleLayer != nullptr && gPlateLayer != nullptr;
}

// where a click dismisses the scene on top: anywhere for a scene that takes a click anywhere, else the centre of the first
// object with a click event
bool FindClickPoint(const SceneObject* theScene, int* theX, int* theY)
{
	for (const SceneObject* aChild = theScene->mChildren.mHead; aChild != nullptr; aChild = aChild->mNext) {
		const SceneStage* aStage = aChild->mStages.mCursor;
		for (const SceneEvent* anEvent = aStage == nullptr ? nullptr : aStage->mEvents.mHead; anEvent != nullptr; anEvent = anEvent->mNext) {
			if (anEvent->mTrigger == TriggerClickAnywhere) {
				*theX = (int)gScreenWidth / 2;
				*theY = (int)gScreenHeight / 2;
				return true;
			}
			if (anEvent->mTrigger == TriggerClick) {
				*theX = (int)aChild->mWorld[AttributeX];
				*theY = (int)aChild->mWorld[AttributeY];
				return true;
			}
		}
	}
	return false;
}

bool StageHas(const SceneObject* theObject, const char* theText)
{
	return theObject->mStages.mCursor != nullptr && strstr(theObject->mStages.mCursor->mName, theText) != nullptr;
}

// whether a slot is one the jewel fills (a wild jewel fills any)
bool Fills(const Jewel* theJewel, const Slot* theSlot)
{
	return theJewel->mKind >= FirstWildJewelKind || theJewel->mKind == theSlot->mKind;
}
}

bool PlayIsOver()
{
	if (gScenes.mTail == nullptr)
		return false;
	if (strstr(gScenes.mTail->mName, "summary") != nullptr || strstr(gScenes.mTail->mName, "sequence") != nullptr)
		return true;
	return LevelIsInPlay() && (StageHas(gMiddleLayer, "victory") || StageHas(gMiddleLayer, "defeat"));
}

BotMove ChooseBotMove()
{
	BotMove aMove;
	if (gScenes.mTail == nullptr || PlayIsOver())
		return aMove;
	if (!LevelIsInPlay()) {
		// a screen that is loading or fading out is waited for; a menu has no move
		if (strstr(gScenes.mTail->mName, "loading") != nullptr)
			aMove.mAction = BotAction::Wait;
		else if (gScenes.mHead != gScenes.mTail && strncmp(gScenes.mTail->mName, "cupid", 5) == 0) {
			// the click's object comes at a later stage of the message, so there is a wait first
			aMove.mAction = FindClickPoint(gScenes.mTail, &aMove.mX, &aMove.mY) ? BotAction::Click : BotAction::Wait;
		}
		return aMove;
	}
	aMove.mAction = BotAction::Wait;
	Jewel* aReadyJewel = gShooter->GetReadyJewel();
	if (aReadyJewel == nullptr || gShooter->mCanShoot == 0 || !StageHas(gShooter, "ready") ||
		!(StageHas(gMiddleLayer, "scrolling") || StageHas(gMiddleLayer, "still")))
		return aMove;

	const Slot* aTarget = nullptr;
	float aShooterX = gShooter->mWorld[AttributeX];
	float aLowestY = 0;
	SceneObject* aPlate = nullptr;
	while ((aPlate = gPlateLayer->FindChildByType(ObjectTypePlate, aPlate)) != nullptr) {
		SceneObject* aSlotObject = nullptr;
		while ((aSlotObject = aPlate->FindChildByType(ObjectTypeSlot, aSlotObject)) != nullptr) {
			const Slot* aSlot = static_cast<const Slot*>(aSlotObject);
			float aX = aSlot->mWorld[AttributeX];
			float aY = aSlot->mWorld[AttributeY];
			if (aSlot->mFilled != 0 || !Fills(aReadyJewel, aSlot) || aY < TopMargin || aY > gShooter->mWorld[AttributeY] - ShooterMargin ||
				aX < 0 || aX > gScreenWidth)
				continue;
			if (aTarget == nullptr || aY > aLowestY || (aY == aLowestY && std::fabs(aX - aShooterX) < std::fabs(aTarget->mWorld[AttributeX] - aShooterX))) {
				aTarget = aSlot;
				aLowestY = aY;
			}
		}
	}
	if (aTarget == nullptr)
		return aMove;

	aMove.mAction = BotAction::Shoot;
	aMove.mX = (int)aTarget->mWorld[AttributeX];
	aMove.mY = AimY;
	float aDistance = std::fabs((float)aMove.mX - aShooterX);
	aMove.mUpdatesToAim = 2;
	while (aDistance > AimedWithin && aMove.mUpdatesToAim < 40) {
		aDistance /= FollowDivisor;
		aMove.mUpdatesToAim++;
	}
	return aMove;
}
