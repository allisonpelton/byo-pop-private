// The panels (0x411d30-0x412340).

#include "Panel.h"

#include "ImageRecord.h"
#include "Level.h"
#include "Layer.h"
#include "Profiles.h"
#include "Scoring.h"
#include "Shooter.h"
#include "TextObject.h"
#include "VeniceGlobals.h"
#include "VeniceMath.h"
#include "VeniceSound.h"

#include <cstdio>
#include <cstring>

namespace
{
int gPreviousScore = 0; // 0x5ba2d0: the score at the last update of the panel

const char* const ScoreFont = "gfx\\font1_tiny.png";
}

Panel::Panel()
{
	mType = ObjectTypePanel;
	mBilinear = 0;
}

// The Journey mode's panel (but not the trick shots') is the wider one, with the picture of the shooter on it
void Panel::OnObjectClosed()
{
	SceneObject* aRoot = GetRoot();
	mIsRelaxed = CurrentProfile().mMode == GameModeJourney && strstr(aRoot->mName, "trickshot") == nullptr;
	if (mIsRelaxed != 0) {
		SetImage("gfx\\panel_relaxed.png", 0, 0, 0, 0);
		SceneObject* aShooterPicture = NewChild(nullptr);
		aShooterPicture->AddStage(10000.0f, 1, 1, 1, nullptr);
		aShooterPicture->SetAttribute(AttributeX, 88.0f, 88.0f);
		aShooterPicture->SetAttribute(AttributeY, -3.0f, -3.0f);
		aShooterPicture->SetImage("gfx\\shooter_panel.png", 0, 0, 0, 0);
	}
}

bool Panel::Update()
{
	Profile& aProfile = CurrentProfile();
	if (gTutorial != nullptr) {
		gPreviousScore = 0;
		aProfile.mScore = 0;
	}
	// a score that changed makes the number swell (on the top scene only), and it settles back to its size
	if (gPreviousScore != aProfile.mScore && GetRoot()->mNext == nullptr)
		mScoreScale = 1.2f;
	gPreviousScore = aProfile.mScore;
	mScoreScale = ((mScoreScale * 49.0f) + 1.0f) / 50.0f;
	if (VeniceAbs(mScoreScale) < (double)1.01f)
		mScoreScale = 1.0f;
	if (gShooter != nullptr) {
		if (gActiveLevel->FindChildByType(ObjectTypeTutorial, nullptr) == nullptr)
			gShooter->mCanShoot = 1;
		for (SceneObject* aChild = mChildren.mHead; aChild != nullptr; aChild = aChild->mNext) {
			if (aChild->mStages.mCursor->FindFilter(FilterHotSpot, AttributeFrameNumber) != nullptr && aChild->mIsHot)
				gShooter->mCanShoot = 0;
		}
	}
	return SceneObject::Update();
}

void Panel::Draw()
{
	Profile& aProfile = CurrentProfile();
	DrawSelf();
	char aText[32];
	sprintf(aText, "%d", aProfile.mScore);
	if (mIsRelaxed == 0) {
		DrawTextOnObject(this, aText, ScoreFont, 0.0f, -1.0f, 0.0f, JustifyCenter, mScoreScale, 1.0f);
	} else {
		DrawTextOnObject(this, aText, ScoreFont, -136.0f, -1.0f, 0.0f, JustifyCenter, mScoreScale, 1.0f);
		int aTries = aProfile.mTriesLeft[aProfile.mMode] + 1 < 0 ? 0 : aProfile.mTriesLeft[aProfile.mMode] + 1;
		sprintf(aText, "x%d", aTries);
		DrawTextOnObject(this, aText, ScoreFont, 106.0f, -1.0f, 0.0f, JustifyLeft, 1.0f, 1.0f);
	}
	DrawChildren();
}

EndlessPanel::EndlessPanel()
{
	mType = ObjectTypeEndlessPanel;
}

void EndlessPanel::Draw()
{
	DrawSelf();
	char aText[32];
	sprintf(aText, "Time: %s", FormatTime((unsigned int)mTickCount));
	DrawTextOnObject(this, aText, ScoreFont, -37.0f, 5.0f, -1.0f, JustifyLeft, 1.0f, 1.0f);
	DrawChildren();
}

ProgressBar::ProgressBar()
{
	SceneObject* aBar = NewChild(nullptr);
	aBar->SetImage("gfx\\panel_progress.png", 0, 0, 0, 0);
	aBar->AddStage(1000.0f, 1, 1, 1, nullptr);
	aBar->SetAttribute(AttributeY, 9.0f, 9.0f);
	mType = ObjectTypeProgressBar;
}

bool ProgressBar::Update()
{
	CheckVictory();
	return SceneObject::Update();
}

// The bar is not drawn: it only watches for the end
void ProgressBar::Draw()
{
}

// The level is won when the last plate is gone, on the top scene, while the play scrolls or stands still (a trick shot stands
// still). The words go away; a trick shot is marked as made, and any other level shows VICTORY!, with the sparkles and the
// fanfare after a pause, and the fish of the shooter jump.
void ProgressBar::CheckVictory()
{
	Profile& aProfile = CurrentProfile();
	if (aProfile.mMode != GameModeSurvival && gPlateLayer->FindChildByType(ObjectTypePlate, nullptr) == nullptr &&
		GetRoot()->mNext == nullptr &&
		(strcmp(gMiddleLayer->mStages.mCursor->mName, "scrolling") == 0 ||
			strcmp(gMiddleLayer->mStages.mCursor->mName, "still") == 0)) {
		aProfile.mLevelIndex[aProfile.mMode] = gActiveLevel->mNumber;
		SceneObject* aWords = nullptr;
		while ((aWords = gActiveLevel->FindChildByType(ObjectTypeText, aWords)) != nullptr)
			aWords->mIsDead = true;
		bool aIsTrickShot = strstr(gMiddleLayer->mStages.mCursor->mName, "still") != nullptr;
		gMiddleLayer->GoToStageByName("victory");
		if (!aIsTrickShot) {
			gActiveLevel->mWinFadeTicks = 82.0f;
			TextObject* aVictory = new TextObject();
			aVictory->SetText("VICTORY!", "gfx\\font1.png", 1.3f, 0, JustifyCenter);
			aVictory->AddStage(900.0f, 0, 0, 0, nullptr);
			aVictory->SetAttribute(AttributeX, 0.0f, 0.0f);
			aVictory->AddStage(100.0f, 0, 0, 1, nullptr);
			aVictory->SetAttribute(AttributeX, 0.0f, 0.0f);
			aVictory->SetAttribute(AttributeAlpha, 0.0f, 0.0f);
			aVictory->AddEvent(TriggerEnd, ActionMakeSparkles, "200", "200", "200", "200", nullptr);
			aVictory->AddEvent(TriggerEnd, ActionPlaySound, nullptr, nullptr, "audio\\victory.ogg", "100", nullptr);
			aVictory->AddStage(600.0f, 0, 0, 1, nullptr);
			aVictory->SetAttribute(AttributeX, 0.0f, 0.0f);
			aVictory->SetAttribute(AttributeAlpha, 0.0f, 1.0f);
			aVictory->AddStage(2800.0f, 0, 0, 1, nullptr);
			aVictory->AddStage(600.0f, 0, 0, 1, nullptr);
			aVictory->SetAttribute(AttributeAlpha, 1.0f, 0.0f);
			gActiveLevel->NewChild(aVictory);
			if (gShooter != nullptr) {
				gShooter->mWingsTicks = 0;
				for (SceneObject* aChild = gShooter->mChildren.mHead; aChild != nullptr; aChild = aChild->mNext) {
					if (strcmp(aChild->mName, "fish") == 0)
						aChild->GoToStageByName("jump");
				}
			}
			AdjustDifficulty(1.0f);
		} else {
			PlaySoundByName("audio\\trickshot_made.ogg", -1);
			if (gActiveLevel->mMainMusic.mSound != nullptr)
				gActiveLevel->mMainMusic.mSound->SetVolume((float)gMusicVolume / 6.0f);
			if (aProfile.mMode == GameModeTrickShot) {
				int aTrickShot = aProfile.mLevelIndex[aProfile.mMode];
				aProfile.mTrickShotDone[aTrickShot / TrickShotsPerRegion][aTrickShot % TrickShotsPerRegion] = 1;
				aProfile.mTricksInARow++;
			}
		}
	}
}
