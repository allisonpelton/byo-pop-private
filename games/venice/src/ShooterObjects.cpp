// The shooter's small objects (0x41bb70-0x41beb0).

#include "ShooterObjects.h"

#include "Jewel.h"
#include "Layer.h"
#include "Level.h"
#include "Shooter.h"
#include "VeniceGlobals.h"
#include "VeniceInput.h"

#include <cstdio>
#include <cstring>

// A scroll point that is the lowest one on the screen ends the level's scroll (the middle layer stops moving up and down)
// when the plates have reached it: the scroll point is gone when it is more than 80 above the lowest plate's stage start, when
// it is more than 400 from there, or when the lowest plate has gone above the top edge
bool ScrollPoint::Update()
{
	bool aAlive = SceneObject::Update();
	if (strstr(gMiddleLayer->mStages.mCursor->mName, "scrolling") != nullptr && 0.0f < mWorld[AttributeY] &&
		FindLowestOfType(ObjectTypeScrollPoint, gPlateLayer) == this) {
		SceneObject* aLowestPlate = FindLowestOfType(ObjectTypePlate, nullptr);
		if (aLowestPlate != nullptr) {
			// the scroll point after the lowest one (the next one down): the one with the greatest y, not this
			SceneObject* aNextPoint = nullptr;
			SceneObject* aPoint = nullptr;
			while ((aPoint = mParent->FindChildByType(ObjectTypeScrollPoint, aPoint)) != nullptr) {
				if ((aNextPoint == nullptr || (aNextPoint->mWorld[AttributeY] <= aPoint->mWorld[AttributeY] &&
					aPoint->mWorld[AttributeY] != aNextPoint->mWorld[AttributeY])) && aPoint != this)
					aNextPoint = aPoint;
			}
			if (aNextPoint != nullptr) {
				if (aNextPoint->mStages.mCursor->mStart[AttributeY] + 80.0f > aLowestPlate->mStages.mHead->mStart[AttributeY])
					aAlive = false;
				if (mStages.mHead->mStart[AttributeY] - 400.0f > aLowestPlate->mStages.mHead->mStart[AttributeY])
					aAlive = false;
				if ((0.0f - gScreenHeight) / 2.0f > aLowestPlate->mWorld[AttributeY])
					aAlive = false;
			}
			if (!aAlive)
				gMiddleLayer->mStages.mCursor->mAttributeMask &= ~8u;
		}
	}
	return aAlive;
}

bool ShooterCursor::Update()
{
	if (static_cast<SceneObject*>(gActiveLevel) == gScenes.mTail)
		return Cursor::Update();
	return true;
}

// Next to the pointer, a small copy of the jewel that is ready (the pointer's picture first, then the jewel's, drawn and then
// taken away again)
void ShooterCursor::Draw()
{
	SceneObject::Draw();
	SceneObject* aTopScene = gScenes.mTail;
	if (IsMouseInWindow() && gShooter != nullptr && gShooter->GetReadyJewel() != nullptr && gHasFocus && aTopScene == GetRoot()) {
		Jewel* aReadyJewel = gShooter->GetReadyJewel();
		SceneObject* aMiniature = NewChild(nullptr);
		aMiniature->mSelectable = 0;
		aMiniature->AddStage(1000.0f, 1, 1, 1, nullptr);
		aMiniature->SetAttribute(AttributeX, 3.0f, 3.0f);
		aMiniature->SetAttribute(AttributeY, 4.0f, 4.0f);
		if (aReadyJewel->mKind < 99) {
			char aName[32];
			sprintf(aName, "gfx\\mini%d.png", aReadyJewel->mKind);
			aMiniature->SetImage(aName, 0, 0, 0, 0);
		} else {
			aMiniature->SetImage("gfx\\mini_wild.png", 0, 0, 0, 0);
		}
		aMiniature->Update();
		aMiniature->mWorld[AttributeAlpha] = mWorld[AttributeAlpha];
		aMiniature->Draw();
		mChildren.Clear();
	}
}
