// What an event does when it fires (0x422fd0), and the click and less-than tests of its triggers (0x424120, 0x424200).

#include "GameText.h"
#include "ImageRecord.h"
#include "InputBox.h"
#include "Level.h"
#include "Profiles.h"
#include "SceneLoader.h"
#include "SceneNames.h"
#include "SceneObject.h"
#include "VeniceApp.h"
#include "VeniceGlobals.h"
#include "VeniceInput.h"
#include "VeniceSound.h"

#include <cstdlib>
#include <cstring>

// 0x43a6d0
static void TrimTrailingSpaces(char* theText)
{
	char* anEnd = theText + (strlen(theText) - 1);
	while (theText <= anEnd && *anEnd == ' ') {
		*anEnd = '\0';
		anEnd--;
	}
}

// 0x43a660
static void TrimLeadingSpaces(char* theText)
{
	size_t aLength = strlen(theText);
	size_t aSpaceCount = strspn(theText, " ");
	if (aSpaceCount <= aLength && aSpaceCount != 0) {
		memmove(theText, theText + aSpaceCount, aLength - aSpaceCount);
		theText[aLength - aSpaceCount] = '\0';
	}
}

// the name the player typed into the level's name box, without the spaces around it
static const char* TypedProfileName()
{
	InputBox* anInputBox = static_cast<InputBox*>(gActiveLevel->FindChildByType(ObjectTypeInputBox, nullptr));
	TrimTrailingSpaces(anInputBox->mText);
	TrimLeadingSpaces(anInputBox->mText);
	return anInputBox->mText;
}

// the colours of the scene under a screen are dimmed (or brightened again) by setting them and updating the scene once
static void SetSceneColor(SceneObject* theScene, float theColor)
{
	theScene->mLocal[AttributeRed] = theColor;
	theScene->mLocal[AttributeGreen] = theColor;
	theScene->mLocal[AttributeBlue] = theColor;
	theScene->Update();
}

// 0x422fd0
void SceneEvent::Fire()
{
	SceneObject* anOwner = mStage->mOwner;
	switch (mAction) {
	case ActionInsertLevel: {
		// the screens above the top scene wait until it is the top one (or a pause screen)
		SceneObject* aRoot = anOwner->GetRoot();
		if (aRoot->mNext == nullptr || strstr(gScenes.mTail->mName, "pause") != nullptr) {
			char aName[260];
			strcpy(aName, TranslateText(mParm1));
			PushSceneByText(aName);
		}
		break;
	}
	case ActionSendLevel:
		if (strcmp(mParm2, "all") == 0) {
			for (SceneObject* aScene = gScenes.mHead; aScene != nullptr; aScene = aScene->mNext)
				aScene->GoToStageByName(TranslateText(mParm1));
		} else
			gActiveLevel->GoToStageByName(TranslateText(mParm1));
		break;
	case ActionGoToStage:
		anOwner->GoToStageByName(mParm1);
		break;
	case ActionSendParentToStage:
		anOwner->mParent->GoToStageByName(mParm1);
		break;
	case ActionNextStage:
		if (mStage->mNext != nullptr)
			anOwner->GoToStage(mStage->mNext);
		break;
	case ActionMove:
		anOwner->mLocal[AttributeFromName(mParm3)] = (float)atof(mParm4);
		break;
	case ActionPlaySound: {
		Sound* aSound = LoadSound(mParm3, false, false);
		if (aSound != nullptr)
			aSound->Play(-1);
		break;
	}
	case ActionDimPrevious:
		if (anOwner->mPrev != nullptr) {
			SetSceneColor(anOwner->mPrev, 0.5f);
			if (anOwner->mPrev->mType == ObjectTypeLevel)
				static_cast<Level*>(anOwner->mPrev)->mPaused = 1;
		}
		break;
	case ActionPausePrevious:
		if (anOwner->mPrev != nullptr && anOwner->mPrev->mType == ObjectTypeLevel)
			static_cast<Level*>(anOwner->mPrev)->mPaused = 1;
		break;
	case ActionDimPreviousNoPause:
		if (anOwner->mPrev != nullptr)
			SetSceneColor(anOwner->mPrev, 0.5f);
		break;
	case ActionBrightenPrevious:
		if (anOwner->mPrev != nullptr) {
			SetSceneColor(anOwner->mPrev, 1.0f);
			if (anOwner->mPrev->mType == ObjectTypeLevel)
				static_cast<Level*>(anOwner->mPrev)->mPaused = 0;
		}
		break;
	case ActionStopMusic:
		if (anOwner->mType == ObjectTypeLevel)
			static_cast<Level*>(anOwner)->StopMusic();
		break;
	case ActionFadeMusic: {
		// the stage's progress takes the music volume down to nothing
		float aPercent = (float)(((double)(float)gMusicVolume * (1.0 - (double)mStage->mProgress) * 100.0) / 100.0);
		if (aPercent == 0.0f)
			aPercent = -1.0f;
		gActiveLevel->SetMusicVolume(aPercent);
		break;
	}
	case ActionFadePreviousMusic:
		if (anOwner->mType == ObjectTypeLevel && anOwner->mPrev != nullptr) {
			// the scene at the bottom of the stack
			Level* aBottomScene = gActiveLevel;
			while (aBottomScene->mPrev != nullptr)
				aBottomScene = static_cast<Level*>(aBottomScene->mPrev);
			float aPercent = (float)((1.0 - (double)mStage->mProgress) * 100.0);
			if (aPercent == 0.0f)
				aPercent = -1.0f;
			aBottomScene->SetMusicVolume(aPercent);
		}
		break;
	case ActionMakeSparkles:
		anOwner->MakeSparkles((int)atol(mParm1), 1.0f, 1.0f, 1.0f, 1.0f);
		break;
	case ActionMakeParticles:
		anOwner->MakeParticles((int)atol(mParm1));
		break;
	case ActionSetLevelNumber:
		gLevelNumber = (int)atol(mParm1);
		break;
	case ActionNewUserName:
		for (int aProfile = 0; aProfile < ProfileCount; aProfile++) {
			if (gProfiles[aProfile].mName[0] == '\0') {
				gCurrentProfile = aProfile;
				const char* aName = TypedProfileName();
				if (aName[0] != '\0') {
					strcpy(CurrentProfile().mName, aName);
					gActiveLevel->GoToStageByName(mParm1);
					return;
				}
				PushSceneByName("invalid_profile_name");
				return;
			}
		}
		break;
	case ActionChangeUserName: {
		const char* aName = TypedProfileName();
		if (aName[0] == '\0')
			PushSceneByName("invalid_profile_name");
		else {
			strcpy(CurrentProfile().mName, aName);
			gActiveLevel->GoToStageByName(mParm1);
		}
		break;
	}
	case ActionDeleteUser: {
		int aNamedProfileCount = 0;
		for (int aProfile = 0; aProfile < ProfileCount; aProfile++) {
			if (gProfiles[aProfile].mName[0] != '\0')
				aNamedProfileCount++;
		}
		if (aNamedProfileCount > 1) {
			memset(&CurrentProfile(), 0, sizeof(Profile));
			for (int aProfile = 0; aProfile < ProfileCount; aProfile++) {
				if (gProfiles[aProfile].mName[0] != '\0') {
					gCurrentProfile = aProfile;
					break;
				}
			}
		}
		gActiveLevel->GoToStageByName(mParm1);
		break;
	}
	case ActionGetInitialUser:
		if (CurrentProfile().mName[0] == '\0') {
			// the name screen comes first, then the screen that picks the first level
			PushSceneByName("input_name");
			CurrentProfile().mMode = GameModeJourney;
			SceneObject* aNameScene = gScenes.mTail;
			aNameScene->mStages.mTail->mEvents.Clear();
			aNameScene->AddEvent(TriggerStart, ActionInsertLevel, "PICK_LEVEL_SELECT_OR_LOADING2", "", "", "", "");
		}
		break;
	case ActionKillAllPrevious:
		// every scene below the owner goes, and the list's cursor goes back to the level being updated
		if (anOwner->mType == ObjectTypeLevel) {
			while (gScenes.mHead != anOwner) {
				gScenes.Begin();
				gScenes.RemoveCursor();
				gScenes.mCursor = gActiveLevel;
			}
		}
		break;
	case ActionKillAll:
		for (SceneObject* aScene = gScenes.mHead; aScene != nullptr; aScene = aScene->mNext)
			aScene->mIsDead = true;
		break;
	case ActionEnterHighScore:
		EnterHighScore(mParm4);
		break;
	case ActionClearScore:
		CurrentProfile().mScore = CurrentProfile().mLevelStartScore[CurrentProfile().mMode];
		break;
	case ActionResetProgress:
		ResetProgress();
		break;
	case ActionChangeImage:
		anOwner->SetImage(mParm1, 0, 0, 0, 0);
		break;
	case ActionBumpTickCounter:
		mStage->mFrame = (float)atol(mParm4) / 4.0f;
		break;
	case ActionCustom:
		anOwner->OnCustomEvent();
		break;
	case ActionSaveScore:
		SaveProfiles();
		break;
	case ActionSetMode:
		CurrentProfile().mMode = (int)atol(mParm1);
		break;
	case ActionBumpScore:
		CurrentProfile().mScore = CurrentProfile().mScore + (int)atol(mParm4);
		break;
	case ActionClearButtonState:
		gActiveLevel->mLeftClick = 0;
		break;
	case ActionEnable3DHardware:
		gApp->Set3DAcclerated(true, true);
		break;
	}
}

// 0x424120: the click (or the key that stands for it) that triggers an event. A scene that is fading out, an object that is not
// shown and a scene that has another above it do not take it.
bool SceneEvent::ClickHandled(bool theClickAnywhere)
{
	SceneObject* anOwner = mStage->mOwner;
	SceneObject* aRoot = anOwner->GetRoot();
	if (aRoot->mNext != nullptr || strstr(aRoot->mStages.mCursor->mName, "fade") != nullptr || anOwner->mVisible == 0)
		return false;
	bool aClicked = false;
	if (theClickAnywhere || anOwner->IsHot())
		aClicked = static_cast<Level*>(aRoot)->mLeftClick != 0.0f;
	// the level editor's flag (0xa1c938), which nothing but the editor sets, would also have turned the keys off here
	if (!aClicked && !ConsumeKey((unsigned char)mAltKey))
		return false;
	PlaySoundByName("audio\\click.ogg", -1);
	return true;
}

// 0x424200: the owner's attribute is below the number the event gives
bool SceneEvent::LessThanHolds()
{
	int anAttribute = AttributeFromName(mParm1);
	float aValue = (float)atof(mParm2);
	float anAttributeValue = mStage->mOwner->mLocal[anAttribute];
	return anAttributeValue <= aValue && aValue != anAttributeValue;
}
