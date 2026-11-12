// The Survival mode's bonus (0x404b30-0x405090).

#include "EndlessBonus.h"

#include "ImageRecord.h"
#include "Level.h"
#include "Profiles.h"
#include "TextObject.h"
#include "VeniceGlobals.h"

#include <cstdio>

// The panel that counts the time of the game (in the level under this summary) is held back by one update for each of this
// object's, which keeps the time from running on while the summary shows
bool EndlessBonus::Update()
{
	SceneObject* aPanel = gActiveLevel->FindChildByName("endlesspan", nullptr);
	aPanel->mTickCount = aPanel->mTickCount - 1.0f;
	return SceneObject::Update();
}

// One number for each minute survived, from nothing up to a thousand for each whole minute. The numbers come in one after the
// other: each is shown after the one before has counted, with a sparkle and a sound as it goes on to the next; the last one
// adds nothing more and ends with the entry of the time into the high score list. The stages of this object itself are one
// stage a number, with the events that add the thousand and start the next number.
void EndlessBonus::OnObjectClosed()
{
	int aMinutes = 0;
	int aTicks = 0;
	SceneObject* aPanel = gScenes.mHead->FindChildByType(ObjectTypeEndlessPanel, nullptr);
	if (aPanel != nullptr) {
		aTicks = (int)aPanel->mTickCount;
		aMinutes = aTicks / 15000;
	}
	for (int aMinute = 0; aMinute <= aMinutes; aMinute++) {
		TextObject* aNumber = new TextObject();
		char aNumberText[16];
		sprintf(aNumberText, "%d", aMinute * 1000);
		aNumber->SetText(aNumberText, "gfx\\font1.png", -1.0f, 0, JustifyCenter);
		float aShowDelay = (float)aMinute * 200.0f + 3900.0f;
		aNumber->AddStage(aShowDelay, 0, 0, 0, nullptr);
		aNumber->SetAttribute(AttributeXScale, 1.2f, 1.2f);
		aNumber->SetAttribute(AttributeYScale, 1.2f, 1.2f);
		aNumber->SetAttribute(AttributeX, 0.0f, 0.0f);
		aNumber->SetAttribute(AttributeY, 0.0f, 0.0f);
		aNumber->AddStage(200.0f, 0, 0, 1, nullptr);
		aNumber->SetAttribute(AttributeXScale, 1.2f, 1.0f);
		aNumber->SetAttribute(AttributeYScale, 1.2f, 1.0f);
		aNumber->SetAttribute(AttributeAlpha, 1.0f, 1.0f);
		aNumber->SetAttribute(AttributeX, 0.0f, 0.0f);
		aNumber->SetAttribute(AttributeY, 0.0f, 0.0f);
		aNumber->AddFilter(FilterSquared, AttributeAll, 1.0f, 0.0f, 0.0f);
		if (aMinute != 0) {
			AddStage(200.0f, 0, 0, 1, nullptr);
			AddEvent(TriggerStart, ActionCustom, "", "", "", "", "");
			AddEvent(TriggerStart, ActionBumpScore, "1000", "1000", "1000", "1000", "");
		}
		if (aMinute == aMinutes) {
			AddStage(20.0f, 0, 0, 1, nullptr);
			AddEvent(TriggerEnd, ActionEnterHighScore, "", "", "", FormatTime((unsigned int)aTicks), "");
		}
		if (aMinute < aMinutes) {
			aNumber->AddEvent(TriggerStart, ActionMakeSparkles, "10", "", "", "", "");
			aNumber->AddEvent(TriggerStart, ActionPlaySound, "", "", "audio\\toss.ogg", "", "");
			aNumber->AddStage(200.0f, 0, 0, 1, nullptr);
			aNumber->SetAttribute(AttributeAlpha, 1.0f, 0.0f);
		} else {
			aNumber->AddStage(200.0f, 1, 1, 1, nullptr);
			aNumber->SetAttribute(AttributeAlpha, 1.0f, 1.0f);
		}
		NewChild(aNumber);
	}
	AddStage(1000.0f, 1, 1, 1, nullptr);
}

// The first minute marker of the level that has no light on it yet lights up, and fades out
void EndlessBonus::OnCustomEvent()
{
	SceneObject* aMarker = nullptr;
	do {
		aMarker = gActiveLevel->FindChildByName("endless_marker", aMarker);
		if (aMarker == nullptr)
			break;
	} while (aMarker->mChildren.mHead != nullptr);
	if (aMarker != nullptr) {
		SceneObject* aLight = aMarker->NewChild(nullptr);
		aLight->AddStage(100.0f, 0, 0, 0, nullptr);
		aLight->AddStage(200.0f, 0, 0, 1, nullptr);
		aLight->SetImage("gfx\\corona.png", 0, 0, 0, 0);
		aLight->SetAttribute(AttributeXScale, 0.45f, 0.45f);
		aLight->SetAttribute(AttributeYScale, 0.45f, 0.45f);
		aLight->SetAttribute(AttributeRotation, 0.0f, 30.0f);
		aLight->SetAttribute(AttributeAlpha, 1.0f, 0.0f);
		aLight->mBlitStyle = 1;
		aMarker->AddStage(240.0f, 0, 0, 1, "fading_out");
		aMarker->SetAttribute(AttributeAlpha, 1.0f, 0.0f);
		aMarker->GoToStageByName("fading_out");
	}
}
