#include "StageManifest.h"

#include "Engine.h"
#include "Params.h"

using namespace engine;

int StageManifest::findStage(const std::string& theId) const
{
	for (int i = 0; i < (int)mStages.size(); i++)
	{
		if (mStages[i].mId == theId)
			return i;
	}
	return -1;
}

void StageManifest::load(const std::string& thePath)
{
	MasterDocHandler aMaster;
	StageManifestDocHandler* aHandler = new StageManifestDocHandler(this);
	aHandler->attach(&aMaster, "stageManifest");
	if (getEngine()->parseXml(thePath, &aMaster) && !mLevelSets.empty())
		placePins();
}

void StageManifest::placePins()
{
	// (Its last set is placed with the spline of the set before it, the one last read in the loop)
	SplinePath* aSpline = nullptr;
	for (size_t aSet = 0; aSet + 1 < mLevelSets.size(); aSet++)
	{
		LevelSet& aLevelSet = mLevelSets[aSet];
		aSpline = aLevelSet.mSpline.get();
		aLevelSet.mStart = aSpline->getNodePosition(0);
		float aStageCount = (float)(mLevelSets[aSet + 1].mFirstStage - aLevelSet.mFirstStage);
		if (!(0.0f < aStageCount))
			continue;
		float aSpacing = (float)((double)aStageCount + 1.0);
		int aStage = 0;
		do
		{
			float aFraction = (float)(aStage + 1) / aSpacing;
			mStages[aLevelSet.mFirstStage + aStage].mMapPos = aSpline->getPositionAt(aFraction);
			aStage++;
		} while ((float)aStage < aStageCount);
	}
	// port: with one set the original reads the manifest itself as the spline; the port leaves that set's start
	if (aSpline != nullptr)
		mLevelSets.back().mStart = aSpline->getNodePosition(aSpline->getNodeCount() - 1);
}

void StageManifestDocHandler::startElement(const std::string& theName, Params& theAttributes)
{
	if (theName == "stage")
	{
		mManifest->mStages.resize(mManifest->mStages.size() + 1);
		StageManifest::Stage& aStage = mManifest->mStages.back();
		aStage.mId = theAttributes.getString("id", "NO_ID");
		aStage.mResource = theAttributes.getString("resource", "");
		aStage.mMapPos = theAttributes.getFloatPair("mapPos", Vector2f(0, 0));
		aStage.mStageTime = theAttributes.getInt("stageTime", 0);
		aStage.mNewGameTime = theAttributes.getInt("newGameTime", 0);
		aStage.mCanSuck = theAttributes.getBool("canSuck", true);
		aStage.mLargePin = theAttributes.getBool("largePin", false);
	}
	else if (theName == "levelset")
	{
		mManifest->mLevelSets.resize(mManifest->mLevelSets.size() + 1);
		StageManifest::LevelSet& aLevelSet = mManifest->mLevelSets.back();
		std::string aSpline = theAttributes.getString("spline", "");
		if (!aSpline.empty())
			aLevelSet.mSpline = loadSplinePath(aSpline);
		aLevelSet.mImageOff = theAttributes.getString("imgOff", "");
		aLevelSet.mImageOn = theAttributes.getString("imgOn", "");
		aLevelSet.mFirstStage = (int)mManifest->mStages.size();
	}
}
