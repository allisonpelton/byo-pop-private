// The text translator (0x405e10, 0x405c60).

#include "GameText.h"

#include "Level.h"
#include "Profiles.h"
#include "SceneFiles.h"
#include "SceneObject.h"
#include "VeniceGlobals.h"
#include "VeniceMath.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

// the text the last call made (0x5ba158)
static char sText[256];

// the scene that every level started: its counts of shots, hits and combos and its time are the ones the summary shows
static Level* FirstScene()
{
	return static_cast<Level*>(gScenes.mHead);
}

// how many of the shots hit, in hundredths of a percent
static int ShotAccuracy(const Level* theLevel)
{
	int aShots = theLevel->mShotsFired < 1 ? 1 : theLevel->mShotsFired;
	int anAccuracy = theLevel->mHits * 10000 / aShots;
	return anAccuracy < 10001 ? anAccuracy : 10000;
}

// the points the time taken is worth: more for a faster level (a level that takes under a second divides by zero in the
// original)
static int TimeBonus(const Level* theLevel)
{
	int aTicks = theLevel->mScrollTicks < 1 ? 1 : theLevel->mScrollTicks;
	int aSeconds = aTicks / 250;
	if (aSeconds < 1)
		aSeconds = 1;
	return ((int)(100000 / (long long)aSeconds) / 100) * 100;
}

// the points for the tries that are left, in the Journey mode (1000 for the last one in any mode)
static int TriesBonus(const Profile& theProfile)
{
	int aTries = theProfile.mTriesLeft[theProfile.mMode];
	int aBonus = (aTries < 2 || theProfile.mMode != GameModeJourney) ? 0 : (aTries - 1) * 5000;
	if (aTries == 1)
		aBonus = 1000;
	return aBonus;
}

// 0x405c60: the number of the trick shot to play next: the one the player is on when it is not done, else one that is not
// done in the first region that has any. "win_sequence" when every trick shot is done.
static const char* NextTrickShot(Profile& theProfile)
{
	static char aNumber[128]; // 0x5ba0d8
	int aRegion = 999;
	for (int aRow = 0; aRow < TrickShotRegionCount; aRow++)
		for (int aColumn = 0; aColumn < TrickShotsPerRegion; aColumn++)
			if (aRegion > 900 && theProfile.mTrickShotDone[aRow][aColumn] == 0)
				aRegion = aRow;
	if (aRegion >= 999) {
		strcpy(aNumber, "win_sequence");
		return aNumber;
	}
	int aShot = 0;
	int aCurrent = theProfile.mLevelIndex[theProfile.mMode];
	if (theProfile.mTrickShotDone[aCurrent / TrickShotsPerRegion][aCurrent % TrickShotsPerRegion] == 0) {
		aRegion = aCurrent / TrickShotsPerRegion;
		aShot = aCurrent % TrickShotsPerRegion;
	} else {
		while (theProfile.mTrickShotDone[aRegion][aShot] != 0)
			aShot = RandomBetween(0, TrickShotsPerRegion - 1);
	}
	theProfile.mLevelIndex[theProfile.mMode] = aRegion * TrickShotsPerRegion + aShot;
	sprintf(aNumber, "%d", aRegion * TrickShotsPerRegion + 1000 + aShot);
	return aNumber;
}

// the first of the four layers' children that sits in the nest of layers every level has: whether a trick shot is still waiting
// for plates
static SceneObject* FindUnfilledSlot()
{
	SceneObject* aLayer = FirstScene()->FindChildByType(ObjectTypeLayer, nullptr);
	aLayer = aLayer->FindChildByType(ObjectTypeLayer, nullptr);
	aLayer = aLayer->FindChildByType(ObjectTypeLayer, nullptr);
	return aLayer->FindChildByType(ObjectTypePlate, nullptr);
}

// 0x405e10 for NEXT_LEVEL: the scene to push when the level is done (also moves the profile on: the score the level
// started with comes back, and the flags of the progress are set)
static void TranslateNextLevel(Profile& theProfile)
{
	bool aLevelSelectShown = false;
	bool aMenuShown = false;
	for (SceneObject* aScene = gScenes.mHead; aScene != nullptr; aScene = aScene->mNext) {
		if (strstr(aScene->mName, "level_select") != nullptr)
			aLevelSelectShown = true;
		if (strstr(aScene->mName, "main_menu") != nullptr || strstr(aScene->mName, "tutorial2") != nullptr)
			aMenuShown = true;
	}
	int aMode = theProfile.mMode;
	theProfile.mScore = theProfile.mLevelStartScore[aMode];
	gLevelNumber = theProfile.mLevelIndex[aMode] + 1;
	if (aMode == GameModeTrickShot) {
		strcpy(sText, "NEXT_TRICKSHOT");
	} else if (aMode == GameModeSurvival || gLevelNumber > 1 || theProfile.mInstructionsHidden != 0 ||
		strstr(gScenes.mHead->mName, "tutorial") != nullptr) {
		if (aMode == GameModeSurvival) {
			strcpy(sText, "endless");
		} else if (aMode == GameModeFlood && gGameScenes.mTail->mNumber <= (unsigned int)gLevelNumber) {
			strcpy(sText, "high_scores");
		} else if (aMode == GameModeJourney && gRelaxedScenes.mTail->mNumber <= (unsigned int)gLevelNumber) {
			strcpy(sText, "high_scores");
		} else if (aMode == GameModeFlood && (unsigned int)gLevelNumber == gGameScenes.mTail->mNumber - 1u) {
			theProfile.mFlagFinishedGame = 1;
			if (aLevelSelectShown) {
				sprintf(sText, "%d", gLevelNumber);
			} else {
				theProfile.mInstructionsHidden = 1;
				strcpy(sText, "level_select");
			}
		} else if (aMode == GameModeJourney && (unsigned int)gLevelNumber == gRelaxedScenes.mTail->mNumber - 1u) {
			theProfile.mFlagFinishedRelaxed = 1;
			if (aLevelSelectShown)
				sprintf(sText, "%d", gLevelNumber);
			else
				strcpy(sText, "level_select");
		} else if ((!aMenuShown && (gLevelNumber < 2 || (gLevelNumber - 1) % 3 != 0)) || aLevelSelectShown) {
			if (gLevelNumber > 2)
				theProfile.mFlagReachedBuilding3 = 1;
			sprintf(sText, "%d", gLevelNumber);
		} else {
			theProfile.mInstructionsHidden = 1;
			strcpy(sText, "level_select");
		}
	} else {
		gLevelNumber = gLevelNumber - 1;
		strcpy(sText, "tutorial2");
	}
}

// 0x405e10 for TOTAL_BONUS: the four bonuses are added to the score, which the next level starts from
static void TranslateTotalBonus(Profile& theProfile)
{
	const Level* aLevel = FirstScene();
	int aTotal = TriesBonus(theProfile) + TimeBonus(aLevel) + (ShotAccuracy(aLevel) / 100) * 100 + aLevel->mCombos * 100;
	theProfile.mScore = theProfile.mScore + aTotal;
	theProfile.mLevelStartScore[theProfile.mMode] = theProfile.mScore;
	sprintf(sText, "%d", aTotal);
}

// 0x405e10 for TRICKSHOT_BONUS: a trick shot that hit its slots is worth 10000, or a thousand for each trick in a row in the
// Trick Shot mode
static void TranslateTrickShotBonus(Profile& theProfile)
{
	if (FindUnfilledSlot() != nullptr) {
		strcpy(sText, "BONUS: 0");
		return;
	}
	int aBonus = theProfile.mMode == GameModeTrickShot ? theProfile.mTricksInARow * 1000 : 10000;
	theProfile.mScore = theProfile.mScore + aBonus;
	theProfile.mLevelStartScore[theProfile.mMode] = theProfile.mScore;
	sprintf(sText, "BONUS: %d", aBonus);
}

// 0x405e10 for REGION_NAME: the title of the level in the mode's list that has the number the player is on
static void TranslateRegionName(const Profile& theProfile)
{
	CursorList<SceneFile>& aList = theProfile.mMode == GameModeJourney ? gRelaxedScenes : gGameScenes;
	for (SceneFile* aFile = aList.mHead; aFile != nullptr; aFile = aFile->mNext) {
		if (aFile->mNumber == (unsigned int)(theProfile.mLevelIndex[theProfile.mMode] + 1)) {
			sprintf(sText, "%s", aFile->mTitle);
			return;
		}
	}
	strcpy(sText, "REGION NAME HERE");
}

// the number after the prefix of a high score id ("HIGH_SCORE_NAME_3")
static int HighScoreRow(const char* theId, const char* thePrefix)
{
	return (int)atol(theId + strlen(thePrefix));
}

const char* TranslateText(const char* theId)
{
	if (theId == nullptr)
		return nullptr;
	if (theId != sText)
		strcpy(sText, theId);
	Profile& aProfile = CurrentProfile();
	int aMode = aProfile.mMode;
	int aLevelIndex = aProfile.mLevelIndex[aMode];

	if (strcmp(theId, "HIGH_SCORE_MODE_NAME") == 0) {
		if (aMode == GameModeFlood)
			strcpy(sText, "FLOOD HONOR ROLL");
		else if (aMode == GameModeSurvival)
			strcpy(sText, "SURVIVAL HONOR ROLL");
		else if (aMode == GameModeTrickShot)
			strcpy(sText, "TRICKSHOT HONOR ROLL");
		else
			strcpy(sText, "JOURNEY HONOR ROLL");
	} else if (strcmp(theId, "LEVEL_TITLE") == 0) {
		if (aLevelIndex == 0)
			strcpy(sText, "SAVE THE TREASURE!");
		else
			strcpy(sText, static_cast<Level*>(gScenes.mCursor)->mTitle);
	} else if (strcmp(theId, "SURVIVAL_TIME") == 0) {
		SceneObject* aPanel = gActiveLevel->FindChildByName("endlesspan", nullptr);
		sprintf(sText, "YOU SURVIVED FOR %s", FormatTime((unsigned int)(int)aPanel->mTickCount));
	} else if (strcmp(theId, "LEVEL_NUMBER") == 0) {
		if (aMode == GameModeSurvival)
			strcpy(sText, "HOW LONG CAN YOU SURVIVE?");
		else if (aLevelIndex < 0x47 || aMode == GameModeTrickShot)
			sprintf(sText, "BUILDING %d", aLevelIndex + 1);
		else
			strcpy(sText, " ");
	} else if (strcmp(theId, "TRICKSHOT_WIN") == 0 || strcmp(theId, "trickshot_summary") == 0) {
		strcpy(sText, aMode == GameModeTrickShot ? "trickymode_summary" : "trickshot_summary");
	} else if (strcmp(theId, "TRICKS_IN_A_ROW") == 0) {
		if (aProfile.mTricksInARow < 2)
			strcpy(sText, aProfile.mTricksInARow == 0 ? " " : "Excellent Shot!");
		else
			sprintf(sText, "%d in a row!", aProfile.mTricksInARow);
	} else if (strcmp(theId, "LEVEL_SUMMARY") == 0 || strcmp(theId, "level_summary") == 0 ||
		strcmp(theId, "relaxed_summary") == 0) {
		strcpy(sText, aMode == GameModeJourney ? "relaxed_summary" : "level_summary");
	} else if (strcmp(theId, "BUILDING_CONTINUE") == 0) {
		sprintf(sText, "YOU WERE AT BUILDING %d", aLevelIndex + 1);
	} else if (strcmp(theId, "SHOT_ACCURACY") == 0) {
		sprintf(sText, "%d%s ACCURACY:", ShotAccuracy(FirstScene()) / 100, "%");
	} else if (strcmp(theId, "COMBOS_MADE") == 0) {
		sprintf(sText, "%d TUMBLE COMBOS:", FirstScene()->mCombos);
	} else if (strcmp(theId, "TIME_TAKEN") == 0) {
		int aTicks = FirstScene()->mScrollTicks;
		sprintf(sText, "%d:%02d TIME TAKEN:", aTicks / 15000, (aTicks / 250) % 60);
	} else if (strcmp(theId, "LIVES_LOST") == 0) {
		int aTries = aProfile.mTriesLeft[aMode];
		sprintf(sText, aTries == 0 ? "%d TRY REMAINING:" : "%d TRIES REMAINING:", aTries + 1);
	} else if (strcmp(theId, "LIVES_LOST_SCORE") == 0) {
		sprintf(sText, "%d", TriesBonus(aProfile));
	} else if (strcmp(theId, "SHOT_ACCURACY_SCORE") == 0) {
		sprintf(sText, "%d", (ShotAccuracy(FirstScene()) / 100) * 100);
	} else if (strcmp(theId, "COMBOS_MADE_SCORE") == 0) {
		sprintf(sText, "%d", FirstScene()->mCombos * 100);
	} else if (strcmp(theId, "TIME_TAKEN_SCORE") == 0) {
		sprintf(sText, "%d", TimeBonus(FirstScene()));
	} else if (strcmp(theId, "TOTAL_BONUS") == 0) {
		TranslateTotalBonus(aProfile);
	} else if (strcmp(theId, "NEXT_TRICKSHOT") == 0) {
		strcpy(sText, NextTrickShot(aProfile));
	} else if (strcmp(theId, "NEXT_LEVEL") == 0) {
		TranslateNextLevel(aProfile);
	} else if (strcmp(theId, "THIS_LEVEL") == 0) {
		if (aMode == GameModeSurvival)
			strcpy(sText, "endless");
		else
			sprintf(sText, "%d", gLevelNumber);
	} else if (strstr(theId, "CURRENT_USER_NAME") != nullptr) {
		std::string aPrefix(theId, strstr(theId, "CURRENT_USER_NAME"));
		sprintf(sText, "%s%s", aPrefix.c_str(), aProfile.mName);
	} else if (strstr(theId, "HIGH_SCORE_NAME_") != nullptr) {
		int aRow = HighScoreRow(theId, "HIGH_SCORE_NAME_");
		sprintf(sText, "%d. %s", aRow + 1, gHighScores[aMode][aRow].mName);
	} else if (strstr(theId, "HIGH_SCORE_TIME_") != nullptr) {
		int aRow = HighScoreRow(theId, "HIGH_SCORE_TIME_");
		sprintf(sText, "%s", gHighScores[aMode][aRow].mTimeText);
	} else if (strstr(theId, "HIGH_SCORE_SCORE_") != nullptr) {
		int aRow = HighScoreRow(theId, "HIGH_SCORE_SCORE_");
		sprintf(sText, "%d", gHighScores[aMode][aRow].mScore);
	} else if (strcmp(theId, "TRICKSHOT_RESULT") == 0) {
		strcpy(sText, FindUnfilledSlot() == nullptr ? "GREAT SHOT!" : "MAYBE NEXT TIME");
	} else if (strcmp(theId, "TRICKSHOT_BONUS") == 0) {
		TranslateTrickShotBonus(aProfile);
	} else if (strcmp(theId, "PICK_LEVEL_SELECT_OR_LOADING2") == 0) {
		strcpy(sText, "PICK_LEVEL_SELECT_OR_LOADING");
	} else if (strcmp(theId, "PICK_LEVEL_SELECT_OR_LOADING") == 0) {
		strcpy(sText, aLevelIndex > 0 ? "continue_game" : "loading");
	} else if (strcmp(theId, "LEVEL_SELECT_OR_MODE") == 0) {
		if (aMode == GameModeSurvival)
			strcpy(sText, "endless");
		if (aMode == GameModeFlood || aMode == GameModeJourney)
			strcpy(sText, "level_select");
	} else if (strcmp(theId, "REGION_LEVEL") == 0) {
		if (aLevelIndex + 1 < 0x48)
			sprintf(sText, "BUILDING %d", aLevelIndex + 1);
		else
			strcpy(sText, "CONGRATULATIONS!");
	} else if (strcmp(theId, "LEVEL_NAME") == 0) {
		strcpy(sText, static_cast<Level*>(gScenes.mCursor)->mTitle);
	} else if (strcmp(theId, "REGION_NAME") == 0) {
		TranslateRegionName(aProfile);
	} else if (strcmp(theId, "LOSE_TIP") == 0) {
		strcpy(sText, aMode == GameModeFlood ? "Don't let the plates sink!  That's how you LOSE!"
			: "You LOSE by letting items fall in the water!");
	} else if (strcmp(theId, "DELETE_CONFIRM") == 0) {
		strcpy(sText, CountProfiles() < 2 ? "one_user_left" : "delete_user_confirm");
	} else if (strcmp(theId, "INSERT_USER") == 0) {
		strcpy(sText, CountProfiles() < ProfileCount ? "input_name" : "too_many_users");
	} else if (strcmp(theId, "QUIT_GAME_CONFIRM_1") == 0) {
		if (FirstScene()->FindChildByType(ObjectTypeTutorial, nullptr) != nullptr)
			strcpy(sText, "If you exit this tutorial, you can");
		else if (aMode == GameModeSurvival)
			strcpy(sText, "If you exit Survival, you will have to");
		else
			strcpy(sText, "If you exit, you will have to restart");
	} else if (strcmp(theId, "QUIT_GAME_CONFIRM_2") == 0) {
		if (FirstScene()->FindChildByType(ObjectTypeTutorial, nullptr) != nullptr)
			strcpy(sText, "see it again the next time you play.");
		else if (aMode == GameModeSurvival)
			strcpy(sText, "start over the next time you play.");
		else
			strcpy(sText, "this building the next time you play.");
	} else if (strcmp(theId, "QUIT_GAME_CONFIRM_3") == 0) {
		strcpy(sText, "Are you sure you want to exit?");
	}
	return sText;
}
