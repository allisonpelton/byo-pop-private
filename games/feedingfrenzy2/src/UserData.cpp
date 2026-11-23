#include "UserData.h"

#include "Engine.h"
#include "GameData.h"
#include "Signature.h"
#include "UserManager.h"

#include <cstdio>

using namespace engine;

namespace
{

// 0x558b64 (feedingfrenzy2): the stored signature that lets values that do not match it stand
const char* const CHEAT_SIGNATURE = "FeedingFrenzyCheat";

// The player's parameter of a mode ("mode%d..." with the mode's number, 0x44bd80 (feedingfrenzy2))
std::string modeParam(const char* theFormat, int theMode)
{
	char aName[64];
	snprintf(aName, sizeof(aName), theFormat, theMode);
	return aName;
}

} // namespace

UserData::UserData() : mFoodBank(0), mHighestJump(0.0f)
{
	resetModes();
}

void UserData::resetModes()
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

bool UserData::init()
{
	resetModes();
	mFoodBank = 0;
	mHighestJump = 0.0f;
	UserManager::instance()->mCurrentUserSignal.connect(this, &UserData::currentUserSlot);
	return load();
}

bool UserData::load()
{
	UserInfo* aUser = UserManager::instance()->getCurrentUser();
	if (aUser == nullptr)
		return false;
	mName = aUser->getName();
	Params& aParams = aUser->getParams();

	SignatureDigest aDigest;
	mFoodBank = aParams.getInt("foodBank", 0);
	aDigest.update(&mFoodBank, sizeof(mFoodBank));
	mHighestJump = aParams.getFloat("highestJump", 0.0f);
	aDigest.update(&mHighestJump, sizeof(mHighestJump));
	std::string aHash = aDigest.finish();
	std::string aSignature = aParams.getString("globalSign", "");
	bool aSaveNeeded = false;
	if (decodeBase64(aSignature) != aHash)
	{
		if (aSignature == CHEAT_SIGNATURE)
			getEngine()->log("Global sign did not match global hash - but cheat code allows it");
		else
		{
			getEngine()->log("Global sign did not match global hash - resetting user record");
			mHighestJump = 0.0f;
			mFoodBank = 0;
			aSaveNeeded = true;
		}
	}

	for (int aMode = 0; aMode < GAME_MODE_COUNT; aMode++)
	{
		SignatureDigest aModeDigest;
		loadMode(mModes[aMode], aParams, aModeDigest);
		std::string aModeHash = aModeDigest.finish();
		std::string aModeSignature = aParams.getString(modeParam("mode%dSign", aMode), "");
		if (decodeBase64(aModeSignature) != aModeHash)
		{
			if (aModeSignature == CHEAT_SIGNATURE)
				getEngine()->log("SaveGame hash didn't match, but cheat workaround allows it\n");
			else
			{
				getEngine()->log("SaveGame hash didn't match - delete save game\n");
				resetModes();
				save();
			}
		}
	}
	if (aSaveNeeded)
		save();
	return true;
}

void UserData::loadMode(ModeRecord& theRecord, Params& theParams, SignatureDigest& theDigest)
{
	theRecord.mMaxStage = theParams.getInt(modeParam("mode%dMaxStage", theRecord.mMode), 0);
	theDigest.update(&theRecord.mMaxStage, sizeof(int));
	theRecord.mLastContinues = theParams.getInt(modeParam("mode%dLastContinues", theRecord.mMode), 0);
	theDigest.update(&theRecord.mLastContinues, sizeof(int));
	theRecord.mLastStage = theParams.getInt(modeParam("mode%dLastStage", theRecord.mMode), -1);
	theDigest.update(&theRecord.mLastStage, sizeof(int));
	theRecord.mLastScore = theParams.getInt(modeParam("mode%dLastScore", theRecord.mMode), -1);
	theDigest.update(&theRecord.mLastScore, sizeof(int));
	theRecord.mLastLifeCount = theParams.getInt(modeParam("mode%dLastLifeCount", theRecord.mMode), 0);
	theDigest.update(&theRecord.mLastLifeCount, sizeof(int));
	theRecord.mTimeRollover = theParams.getInt(modeParam("mode%dTimeRollover", theRecord.mMode), 0);
	theDigest.update(&theRecord.mTimeRollover, sizeof(int));
}

void UserData::save()
{
	UserManager* aUsers = UserManager::instance();
	UserInfo* aUser = aUsers->getCurrentUser();
	if (aUser == nullptr)
		return;
	Params& aParams = aUser->getParams();
	SignatureDigest aDigest;
	aParams.setInt("foodBank", mFoodBank);
	aDigest.update(&mFoodBank, sizeof(mFoodBank));
	aParams.setFloat("highestJump", mHighestJump);
	aDigest.update(&mHighestJump, sizeof(mHighestJump));
	aParams.set("globalSign", encodeBase64(aDigest.finish()));
	for (int aMode = 0; aMode < GAME_MODE_COUNT; aMode++)
		saveMode(mModes[aMode], aParams);
	aUsers->saveUser(aUser);
}

void UserData::saveMode(const ModeRecord& theRecord, Params& theParams)
{
	SignatureDigest aDigest;
	theParams.setInt(modeParam("mode%dMaxStage", theRecord.mMode), theRecord.mMaxStage);
	aDigest.update(&theRecord.mMaxStage, sizeof(int));
	theParams.setInt(modeParam("mode%dLastContinues", theRecord.mMode), theRecord.mLastContinues);
	aDigest.update(&theRecord.mLastContinues, sizeof(int));
	theParams.setInt(modeParam("mode%dLastStage", theRecord.mMode), theRecord.mLastStage);
	aDigest.update(&theRecord.mLastStage, sizeof(int));
	theParams.setInt(modeParam("mode%dLastScore", theRecord.mMode), theRecord.mLastScore);
	aDigest.update(&theRecord.mLastScore, sizeof(int));
	theParams.setInt(modeParam("mode%dLastLifeCount", theRecord.mMode), theRecord.mLastLifeCount);
	aDigest.update(&theRecord.mLastLifeCount, sizeof(int));
	theParams.setInt(modeParam("mode%dTimeRollover", theRecord.mMode), theRecord.mTimeRollover);
	aDigest.update(&theRecord.mTimeRollover, sizeof(int));
	// (the original sets the signature twice, the same both times)
	theParams.set(modeParam("mode%dSign", theRecord.mMode), encodeBase64(aDigest.finish()));
}

void UserData::recordGame(const GameData& theGameData)
{
	ModeRecord& aRecord = mModes[theGameData.getMode()];
	aRecord.mLastContinues = theGameData.getContinues();
	aRecord.mLastStage = theGameData.getStage();
	if (aRecord.mMaxStage <= theGameData.getStage())
		aRecord.mMaxStage = theGameData.getStage();
	aRecord.mLastScore = theGameData.getScore();
	aRecord.mLastLifeCount = theGameData.getLives();
	aRecord.mTimeRollover = theGameData.getTimeRollover();
}

void UserData::clearSavedGame(int theMode)
{
	ModeRecord& aRecord = mModes[theMode];
	aRecord.mLastStage = -1;
	aRecord.mLastScore = -1;
	aRecord.mMode = theMode;
	aRecord.mLastContinues = 0;
	aRecord.mLastLifeCount = 0;
	aRecord.mTimeRollover = 0;
	save();
}

void UserData::unlockAll(int theStageCount)
{
	for (int aMode = 0; aMode < GAME_MODE_COUNT; aMode++)
		mModes[aMode].mMaxStage = theStageCount;
	save();
}
