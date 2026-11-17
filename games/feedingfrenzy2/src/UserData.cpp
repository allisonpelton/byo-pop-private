#include "UserData.h"

UserData::UserData() : mFoodBank(0), mHighestJump(0.0f)
{
	for (int aMode = 0; aMode < GAME_MODE_COUNT; aMode++)
	{
		ModeRecord& aRecord = mModes[aMode];
		aRecord.mLastStage = -1;
		aRecord.mLastScore = -1;
		aRecord.mMode = aMode;
		aRecord.mLastContinues = 0;
		aRecord.mLastLifeCount = 0;
		aRecord.mMaxStage = 0;
		aRecord.mTimeRollover = 0;
	}
}
