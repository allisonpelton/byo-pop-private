// What a level does when the play goes wrong (0x40f960-0x410770): a lost try, the message that goes with it, a missed trick shot,
// and the helpers that tell whether the plates can still be completed.

#include "Level.h"

#include "GameText.h"
#include "Jewel.h"
#include "Layer.h"
#include "Plate.h"
#include "Profiles.h"
#include "SceneLoader.h"
#include "Scoring.h"
#include "Shooter.h"
#include "TextObject.h"
#include "VeniceGlobals.h"
#include "VeniceSound.h"

#include <cstdio>
#include <cstring>

bool AnyPlateIncomplete()
{
	bool anyIncomplete = false;
	SceneObject* aPlate = nullptr;
	while ((aPlate = gPlateLayer->FindChildByType(ObjectTypePlate, aPlate)) != nullptr) {
		if (static_cast<Plate*>(aPlate)->CountFilledSlots() < static_cast<Plate*>(aPlate)->mSlotCount)
			anyIncomplete = true;
	}
	return anyIncomplete;
}

// The level is lost, or a try of the Journey mode is. The middle layer starts to sink (the plates rise up the screen, the
// water comes up in the Survival mode), the shooter sinks, the message is shown and a Flood level makes the next one easier.
void Level::LoseTry()
{
	Profile& aProfile = CurrentProfile();
	if (aProfile.mMode != GameModeJourney || aProfile.mTriesLeft[aProfile.mMode] < 1) {
		gMiddleLayer->GoToStageByName("defeat");
		SceneStage* aDefeatStage = gMiddleLayer->mStages.mCursor;
		aDefeatStage->mStart[AttributeY] = gMiddleLayer->mLocal[AttributeY];
		if (aProfile.mMode == GameModeSurvival)
			aDefeatStage->mEnd[AttributeY] = gMiddleLayer->mLocal[AttributeY];
		else
			aDefeatStage->mEnd[AttributeY] = (0.0f - FindHighestOfType(ObjectTypePlate, gPlateLayer)->mLocal[AttributeY]) + 1200.0f;
		PlaySoundByName("audio\\rise.ogg", -1);
		if (aProfile.mMode == GameModeSurvival)
			mEndlessSinkTicks = 1;
	}
	gShooter->Sink();
	ShowLossMessage();
	if (aProfile.mMode == GameModeFlood)
		AdjustDifficulty(-1.0f);
}

// What a lost try says (the Journey mode counts the tries down), and what comes after it: the Survival mode's summary, or, when
// no try is left (and in the other modes), the click that plays the level again
void Level::ShowLossMessage()
{
	Profile& aProfile = CurrentProfile();
	char aMessage[256];
	if (aProfile.mTriesLeft[aProfile.mMode] < 1 || aProfile.mMode != GameModeJourney)
		sprintf(aMessage, "SUNK!");
	else if (aProfile.mTriesLeft[aProfile.mMode] == 1)
		sprintf(aMessage, "THIS IS YOUR LAST TRY!");
	else
		sprintf(aMessage, "%d %s LEFT", aProfile.mTriesLeft[aProfile.mMode], "TRIES");
	KillPopupTexts();
	TextObject* aLossText = new TextObject();
	aLossText->SetText(aMessage, "gfx\\font1.png", 1.0f, 0, JustifyCenter);
	aLossText->AddStage(600.0f, 0, 0, 1, nullptr);
	aLossText->SetAttribute(AttributeX, 0, 0);
	aLossText->SetAttribute(AttributeAlpha, 0, 1.0f);
	aLossText->AddStage(1800.0f, 0, 0, 1, nullptr);
	aLossText->AddStage(300.0f, 0, 0, 1, nullptr);
	aLossText->SetAttribute(AttributeAlpha, 1.0f, 0);
	NewChild(aLossText);
	if (aProfile.mMode == GameModeJourney)
		aProfile.mTriesLeft[aProfile.mMode]--;
	if (aProfile.mMode == GameModeSurvival) {
		LoadSceneFile("levels\\endless_summary.xml", this);
	} else if (aProfile.mMode != GameModeJourney || aProfile.mTriesLeft[aProfile.mMode] < 0) {
		TextObject* aClickText = new TextObject();
		aClickText->SetText("CLICK TO PLAY AGAIN", "gfx\\font1.png", 1.0f, 0, JustifyCenter);
		aClickText->AddStage(2200.0f, 0, 0, 0, nullptr);
		aClickText->AddStage(100.0f, 0, 0, 1, nullptr);
		aClickText->SetAttribute(AttributeX, 0, 0);
		aClickText->SetAttribute(AttributeAlpha, 0, 0);
		aClickText->AddStage(3000.0f, 0, 0, 1, nullptr);
		aClickText->AddStage(600.0f, 0, 0, 1, nullptr);
		aClickText->SetAttribute(AttributeAlpha, 0, 1.0f);
		aClickText->AddStage(600.0f, 1, 1, 1, nullptr);
		aClickText->AddEvent(TriggerClickAnywhere, ActionClearScore, nullptr, nullptr, nullptr, nullptr, nullptr);
		aClickText->AddEvent(TriggerClickAnywhere, ActionInsertLevel, TranslateText("THIS_LEVEL"), nullptr, nullptr, nullptr,
			nullptr);
		aClickText->AddEvent(TriggerClickAnywhere, ActionSendLevel, "fade_out", "all", nullptr, nullptr, nullptr);
		NewChild(aClickText);
	}
}

// A trick shot that missed: the level is over (the middle layer goes to its victory stage, which the level's end handles), the
// mode moves on to the next trick shot (the first one in the Trick Shot mode), the streak of trick shots ends and the music dips
void Level::MissTrickShot()
{
	if (strstr(gMiddleLayer->mStages.mCursor->mName, "still") != nullptr && GetRoot()->mNext == nullptr) {
		gMiddleLayer->GoToStageByName("victory");
		Profile& aProfile = CurrentProfile();
		if (aProfile.mMode == GameModeTrickShot)
			aProfile.mLevelIndex[aProfile.mMode] = 1;
		else
			aProfile.mLevelIndex[aProfile.mMode]++;
		aProfile.mTricksInARow = 0;
		PlaySoundByName("audio\\trickshot_missed.ogg", -1);
		if (mMainMusic.mSound != nullptr)
			mMainMusic.mSound->SetVolume((float)gMusicVolume / 6.0f);
	}
}

// A jewel the Journey mode's player lost without it filling a slot, while the plates can still be completed and the shooter is
// ready, costs a try
void Level::CheckLostJewel(Jewel* theJewel)
{
	if (gTutorial == nullptr && AnyPlateIncomplete() && CurrentProfile().mMode == GameModeJourney &&
		strstr(gMiddleLayer->mStages.mCursor->mName, "scrolling") != nullptr &&
		strstr(gShooter->mStages.mCursor->mName, "ready") != nullptr && theJewel->mShot == 1 &&
		theJewel->mSlotsFilled == 0 && gTutorial == nullptr && theJewel->mKind < 100)
		LoseTry();
}

void Level::KillPopupTexts()
{
	SceneObject* aText = nullptr;
	while ((aText = FindChildByName("popup_text", aText)) != nullptr)
		aText->mIsDead = true;
}
