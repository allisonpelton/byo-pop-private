// The bonus basket (0x401000-0x401530).

#include "BonusBasket.h"

#include "Geometry.h"
#include "ImageRecord.h"
#include "Jewel.h"
#include "Level.h"
#include "Scoring.h"
#include "TextObject.h"
#include "VeniceGlobals.h"
#include "VeniceMath.h"
#include "VeniceSound.h"

#include <cstring>

// While the basket waits for a jewel (its stage is called waiting), a jewel that is falling through the strip across its mouth
// is taken into it: the jewel loses its power shot, goes to a place in the basket's frame at a random turn, scores a hundred
// and the basket starts to turn. The search for the next jewel carries on from the jewel that was taken, which has moved to the
// basket's children.
bool BonusBasket::Update()
{
	bool anAlive = SceneObject::Update();
	if (gPlateLayer != nullptr && strstr(mStages.mCursor->mName, "waiting") != nullptr) {
		SceneObject* aBasketFront = mChildren.mHead;
		SceneObject* aJewelObject = nullptr;
		while ((aJewelObject = gPlateLayer->FindChildByType(ObjectTypeJewel, aJewelObject)) != nullptr) {
			Jewel* aJewel = static_cast<Jewel*>(aJewelObject);
			if (BoxesOverlap(mWorld[AttributeX], mWorld[AttributeY] - aBasketFront->mImage->mFrameHeight / 2.0f,
					aBasketFront->mImage->mFrameWidth, 6.0f, aJewel->mWorld[AttributeX], aJewel->mWorld[AttributeY], 20.0f,
					20.0f) &&
				aJewel->GetYVelocity() > 0.0f) {
				aJewel->MakeSparkles(20, 1.0f, 1.0f, 1.0f, 1.0f);
				aJewel->mParent->mChildren.mCursor = aJewel;
				aJewel->mPowerShot = 0;
				AdoptNewestChild(aJewel->mParent, true);
				float aLandingX = (float)RandomFloat(-40.0f, 40.0f);
				float aLandingY = (float)RandomFloat(-20.0f, 20.0f);
				float aLandingRotation = (float)RandomFloat(0.0f, 360.0f);
				aJewel->AddStage(160.0f, 1, 1, 1, "cruising");
				aJewel->SetAttribute(AttributeX, AttributeUnsetValue, aLandingX);
				aJewel->SetAttribute(AttributeY, AttributeUnsetValue, aLandingY);
				aJewel->AddStage(1000.0f, 1, 1, 1, "in_basket");
				aJewel->SetAttribute(AttributeX, aLandingX, aLandingX);
				aJewel->SetAttribute(AttributeY, aLandingY, aLandingY);
				aJewel->SetAttribute(AttributeRotation, aLandingRotation, aLandingRotation);
				aJewel->GoToStageByName("in_basket");
				aJewel->Update();
				// the hundred points float up from where the jewel was
				TextObject* aScoreText = new TextObject();
				aScoreText->SetText(FormatScore(100), "gfx\\font1.png", 1.0f, 0, JustifyCenter);
				aScoreText->AddStage(1500.0f, 0, 0, 1, nullptr);
				aScoreText->SetAttribute(AttributeXScale, 0.6f, 0.0f);
				aScoreText->SetAttribute(AttributeYScale, 0.6f, 0.0f);
				aScoreText->SetAttribute(AttributeX, aJewel->mWorld[AttributeX] - gActiveLevel->mWorld[AttributeX],
					aJewel->mWorld[AttributeX] - gActiveLevel->mWorld[AttributeX]);
				aScoreText->SetAttribute(AttributeY, aJewel->mWorld[AttributeY] - gActiveLevel->mWorld[AttributeY],
					(aJewel->mWorld[AttributeY] - gActiveLevel->mWorld[AttributeY]) + 140.0f);
				aScoreText->SetAttribute(AttributeAlpha, 1.0f, 0.0f);
				aScoreText->AddFilter(FilterCubed, AttributeAll, 0.0f, 0.0f, 0.0f);
				gActiveLevel->NewChild(aScoreText);
				PlaySoundByName("audio\\basket.ogg", -1);
				if (CountChildrenOfType(ObjectTypeJewel) != 0)
					GoToStageByName("rotating");
			}
		}
	}
	return anAlive;
}

// When the plates still have slots to fill, every jewel in the basket is thrown out of it: it falls from the basket's place
// (it can be picked up again, bumped by the bumpers, and does not score a second time)
void BonusBasket::OnCustomEvent()
{
	if (AnyPlateIncomplete()) {
		SceneObject* aJewelObject;
		while ((aJewelObject = FindChildByType(ObjectTypeJewel, nullptr)) != nullptr) {
			Jewel* aJewel = static_cast<Jewel*>(aJewelObject);
			UpdateWorldValues();
			aJewel->Update();
			aJewel->mStages.Clear();
			aJewel->AddStage(500.0f, 1, 1, 1, "shooting");
			aJewel->AddFilter(FilterVelocity, AttributeY, 0.0f, 0.02f, 0.0f);
			aJewel->mShot = 1;
			aJewel->mNoPickupScore = 1;
			aJewel->mBumpable = 1;
			mChildren.mCursor = aJewel;
			gPlateLayer->AdoptNewestChild(this, true);
			aJewel->SetZOrder(mZOrder - 0.001f);
		}
	}
}
