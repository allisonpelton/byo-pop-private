// The score and the difficulty (0x408810-0x408970).

#include "Scoring.h"

#include "Profiles.h"
#include "Shooter.h"
#include "Tuning.h"
#include "VeniceGlobals.h"

#include <cstdio>

const char* FormatScore(int thePoints)
{
	static char aMultiplierText[32];
	static char aScoreText[32];
	CurrentProfile().mScore = thePoints * gShooter->mScoreMultiplier + CurrentProfile().mScore;
	snprintf(aMultiplierText, sizeof(aMultiplierText), " x%d", gShooter->mScoreMultiplier);
	if (gShooter->mScoreMultiplier < 2)
		aMultiplierText[0] = 0;
	snprintf(aScoreText, sizeof(aScoreText), "+%d%s", thePoints, aMultiplierText);
	return aScoreText;
}

void AdjustDifficulty(float theChange)
{
	Profile& aProfile = CurrentProfile();
	if (0.0f <= theChange) {
		if (0.0f < theChange)
			aProfile.mDifficulty = gTuning.mDifficultyChangeOnWin * theChange + aProfile.mDifficulty;
	} else {
		aProfile.mDifficulty = gTuning.mDifficultyChangeOnLoss * theChange + aProfile.mDifficulty;
	}
	if (aProfile.mDifficulty < gTuning.mMinDifficulty)
		aProfile.mDifficulty = gTuning.mMinDifficulty;
	if (gTuning.mMaxDifficulty < aProfile.mDifficulty)
		aProfile.mDifficulty = gTuning.mMaxDifficulty;
}
