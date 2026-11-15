// port: the bot (Bot.h)

#include "Bot.h"

#include "Jewel.h"
#include "Level.h"
#include "Plate.h"
#include "Profiles.h"
#include "Shooter.h"
#include "Slot.h"
#include "VeniceGlobals.h"
#include "VeniceInput.h"

#include <cmath>
#include <cstdlib>
#include <cstring>
#include <vector>

namespace
{
const int AimY = 300; // the pointer's height: only its x aims, and no panel button is here
const int TrickShotFirstX = 60; // the left of the places a trick shot aims at
const int TrickShotStepX = 97; // how far the next attempt aims from the last
const int TrickShotSpanX = 680; // the width the places wrap around in
int sTrickShotAttempts = 0; // the trick shots tried so far
const float SwapPauseTicks = 40.0f; // updates after a swap before another is tried (the jewels move to their places)
float sLastSwapTick = -1000.0f; // the level tick count of the last swap
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
// object with a click event (not when only a click anywhere is wanted: a level has buttons that a click must not reach)
bool FindClickPoint(const SceneObject* theScene, bool theAnywhereOnly, int* theX, int* theY)
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

// whether the object's own stage has an event that takes a click anywhere
bool TakesClickAnywhere(const SceneObject* theObject)
{
	const SceneStage* aStage = theObject->mStages.mCursor;
	for (const SceneEvent* anEvent = aStage == nullptr ? nullptr : aStage->mEvents.mHead; anEvent != nullptr; anEvent = anEvent->mNext) {
		if (anEvent->mTrigger == TriggerClickAnywhere)
			return true;
	}
	return false;
}

// the scene or an object in it, at any depth, takes a click anywhere (a summary screen, the level select), and the middle of the
// screen is the click's place
bool FindClickAnywhere(const SceneObject* theObject, int* theX, int* theY)
{
	bool aFound = TakesClickAnywhere(theObject);
	for (const SceneObject* aChild = theObject->mChildren.mHead; !aFound && aChild != nullptr; aChild = aChild->mNext)
		aFound = FindClickAnywhere(aChild, theX, theY);
	if (aFound) {
		*theX = (int)gScreenWidth / 2;
		*theY = (int)gScreenHeight / 2;
	}
	return aFound;
}

// A slot that a shot has been aimed at is left alone while the jewel flies there, so that two jewels are not thrown at one slot
struct SlotClaim
{
	const Slot* mSlot;
	float mLevelTick; // the level's tick count when the shot was chosen
};
std::vector<SlotClaim> sClaims;
const float JewelSpeed = 2.3f; // pixels an update, up the screen
const float ClaimMargin = 40.0f; // updates after the flight is over that a slot is still left alone

bool IsClaimed(const Slot* theSlot)
{
	for (size_t aClaim = 0; aClaim < sClaims.size(); aClaim++) {
		if (sClaims[aClaim].mSlot == theSlot)
			return true;
	}
	return false;
}

// forgets the claims whose shot has arrived (or whose level is another)
void ExpireClaims()
{
	// a level that is another (or begun again) has other slots, and the old ones are gone
	static const Level* sClaimLevel = nullptr;
	static float sLastTick = 0;
	if (gActiveLevel != sClaimLevel || gActiveLevel->mTickCount < sLastTick)
		sClaims.clear();
	sClaimLevel = gActiveLevel;
	sLastTick = gActiveLevel->mTickCount;
	size_t aKept = 0;
	for (size_t aClaim = 0; aClaim < sClaims.size(); aClaim++) {
		float anAge = gActiveLevel->mTickCount - sClaims[aClaim].mLevelTick;
		float aFlight = (gShooter->mWorld[AttributeY] - sClaims[aClaim].mSlot->mWorld[AttributeY]) / JewelSpeed + ClaimMargin;
		if (anAge >= 0 && anAge < aFlight && sClaims[aClaim].mSlot->mFilled == 0)
			sClaims[aKept++] = sClaims[aClaim];
	}
	sClaims.resize(aKept);
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

// the lowest slot on the screen, nearer the shooter's x among equals, that a shot of the jewel could fill, if there is one
const Slot* FindTarget(const Jewel* theJewel)
{
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
			if (aSlot->mFilled != 0 || IsClaimed(aSlot) || !Fills(theJewel, aSlot) || aY < TopMargin || aY > gShooter->mWorld[AttributeY] - ShooterMargin ||
				aX < 0 || aX > gScreenWidth)
				continue;
			if (aTarget == nullptr || aY > aLowestY || (aY == aLowestY && std::fabs(aX - aShooterX) < std::fabs(aTarget->mWorld[AttributeX] - aShooterX))) {
				aTarget = aSlot;
				aLowestY = aY;
			}
		}
	}
	return aTarget;
}

// the jewel that waits behind the ready one in the shooter's row
const Jewel* FindWaitingJewel()
{
	SceneObject* aJewel = nullptr;
	while ((aJewel = gShooter->FindChildByType(ObjectTypeJewel, aJewel)) != nullptr) {
		if (StageHas(aJewel, "waiting"))
			return static_cast<const Jewel*>(aJewel);
	}
	return nullptr;
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

bool gBotContinuesPastSummaries = false;

BotMove ChooseBotMove()
{
	BotMove aMove;
	if (gScenes.mTail == nullptr)
		return aMove;
	if (PlayIsOver()) {
		if (gBotContinuesPastSummaries) {
			aMove.mAction = BotAction::Wait;
			if (!LevelIsInPlay() && FindClickAnywhere(gScenes.mTail, &aMove.mX, &aMove.mY))
				aMove.mAction = BotAction::Click;
		}
		return aMove;
	}
	if (!LevelIsInPlay()) {
		// a screen that is loading or fading out is waited for; a menu has no move
		if (strstr(gScenes.mTail->mName, "loading") != nullptr)
			aMove.mAction = BotAction::Wait;
		else if (gBotContinuesPastSummaries && strcmp(gScenes.mTail->mName, "level_select") == 0) {
			// the level select starts the level that the marker stands at
			aMove.mAction = FindClickAnywhere(gScenes.mTail, &aMove.mX, &aMove.mY) ? BotAction::Click : BotAction::Wait;
		} else if (gScenes.mHead != gScenes.mTail && strncmp(gScenes.mTail->mName, "cupid", 5) == 0) {
			// the click's object comes at a later stage of the message, so there is a wait first
			aMove.mAction = FindClickPoint(gScenes.mTail, false, &aMove.mX, &aMove.mY) ? BotAction::Click : BotAction::Wait;
		} else if (gScenes.mHead != gScenes.mTail && FindClickAnywhere(gScenes.mTail, &aMove.mX, &aMove.mY)) {
			// any other message over a scene (the Flood mode's first level explains itself) goes away on a click
			aMove.mAction = BotAction::Click;
		}
		return aMove;
	}
	aMove.mAction = BotAction::Wait;
	Jewel* aReadyJewel = gShooter->GetReadyJewel();
	if (aReadyJewel == nullptr || gShooter->mCanShoot == 0 || !StageHas(gShooter, "ready") ||
		!(StageHas(gMiddleLayer, "scrolling") || StageHas(gMiddleLayer, "still"))) {
		// the tutorial holds the shooter until a click starts it, and a lost level waits for the click that plays it again
		if (FindClickPoint(gActiveLevel, true, &aMove.mX, &aMove.mY))
			aMove.mAction = BotAction::Click;
		return aMove;
	}

	ExpireClaims();
	float aShooterX = gShooter->mWorld[AttributeX];
	const Slot* aTarget = FindTarget(aReadyJewel);
	if (aTarget == nullptr) {
		// nothing for the ready jewel: the jewel that waits behind it, when it has a slot, is brought forward by a right click
		const Jewel* aWaitingJewel = FindWaitingJewel();
		if (aWaitingJewel != nullptr && gActiveLevel->mTickCount - sLastSwapTick > SwapPauseTicks && FindTarget(aWaitingJewel) != nullptr) {
			sLastSwapTick = gActiveLevel->mTickCount;
			aMove.mAction = BotAction::Swap;
			aMove.mX = gMouseX;
			aMove.mY = gMouseY;
		}
		return aMove;
	}

	aMove.mAction = BotAction::Shoot;
	SlotClaim aClaim = {aTarget, gActiveLevel->mTickCount};
	sClaims.push_back(aClaim);
	aMove.mX = (int)aTarget->mWorld[AttributeX];
	if (CurrentProfile().mMode == GameModeTrickShot) {
		// a trick shot is a single shot that goes by way of bumpers, which the bot does not follow: each attempt aims at another
		// place, so that the levels now and then are won
		aMove.mX = TrickShotFirstX + (sTrickShotAttempts * TrickShotStepX) % TrickShotSpanX;
		sTrickShotAttempts++;
	}
	aMove.mY = AimY;
	float aDistance = std::fabs((float)aMove.mX - aShooterX);
	aMove.mUpdatesToAim = 2;
	while (aDistance > AimedWithin && aMove.mUpdatesToAim < 40) {
		aDistance /= FollowDivisor;
		aMove.mUpdatesToAim++;
	}
	return aMove;
}
