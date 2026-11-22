#include "StageManifest.h"

#include "Engine.h"
#include "Params.h"

using namespace engine;

void StageManifest::load(const std::string& thePath)
{
	MasterDocHandler aMaster;
	StageManifestDocHandler* aHandler = new StageManifestDocHandler(this);
	aHandler->attach(&aMaster, "stageManifest");
	if (getEngine()->parseXml(thePath, &aMaster) && !mLevelSets.empty())
	{
		// The pins spaced along each level set's spline (0x41ad50 (feedingfrenzy2)): not yet translated
		getEngine()->log("StageManifest: the game map's pins not yet translated\n");
	}
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
		{
			// The spline's file read into a SplinePath (PathDocHandler, 0x48b9b0 (feedingfrenzy2)): not yet
			// translated (the game map's)
		}
		aLevelSet.mImageOff = theAttributes.getString("imgOff", "");
		aLevelSet.mImageOn = theAttributes.getString("imgOn", "");
		aLevelSet.mFirstStage = (int)mManifest->mStages.size();
	}
}
