#include "StageInfo.h"
#include "App.h"
#include "DocHandler.h"
#include "Exception.h"
#include "Layout.h"
#include "Stream.h"

using namespace feedingfrenzy;

namespace
{
const char* const STAGE_FOLDER = "config\\stages\\";

// The handler of the manifest's elements (0x4360c0; vtable 0x4a6fd8): the game's version, each stage, and each change of
// the player's fish
class StageManifestHandler : public engine::BaseDocHandler
{
public:
	StageManifestHandler(const std::string& theName, engine::DocHandlerStack* theParent, StageManifest* theManifest) :
		engine::BaseDocHandler(theName, theParent), mManifest(theManifest)
	{
	}

	void startElement(const std::string& theName, const engine::XmlAttributes& theAttributes) override
	{
		if (theName == "gameInfo")
		{
			mManifest->mVersion = engine::getStringAttribute(theAttributes, "version", "NoVersionInfo");
		}
		else if (theName == "stage")
		{
			std::string aName = engine::getStringAttribute(theAttributes, "name", "NO NAME");
			std::string aPath = STAGE_FOLDER + engine::getStringAttribute(theAttributes, "path", "NO NAME");
			std::unique_ptr<StageInfo> aStage(new StageInfo());
			aStage->setName(aName);
			aStage->setPath(aPath);
			aStage->setUnlocked(false);
			aStage->setBeaten(false);
			aStage->setMapPosition(engine::getPointAttribute(theAttributes, "mapPos", engine::Point(0, 0)));
			// 0x430750: the stage's number is its place in the list, from 1
			aStage->setIndex((int)mManifest->mStages.size() + 1);
			mManifest->mStages.push_back(std::move(aStage));
		}
		else if (theName == "fishChange")
		{
			// 0x431490: the fish before this change plays up to the stage before the one the list has reached
			int aStageCount = (int)mManifest->mStages.size();
			if (!mManifest->mFishChanges.empty())
				mManifest->mFishChanges.back().mLastStage = aStageCount - 1;
			FishChange aChange;
			aChange.mName = engine::getStringAttribute(theAttributes, "name", "NO NAME");
			aChange.mFirstStage = aStageCount;
			aChange.mLastStage = 0;
			mManifest->mFishChanges.push_back(aChange);
		}
	}

private:
	StageManifest* mManifest;
};
}

// 0x42c860
void feedingfrenzy::loadStageManifest(const std::string& theFile, StageManifest& theManifest)
{
	engine::Ref<engine::DocHandlerStack> aStack = new engine::DocHandlerStack();
	engine::Ref<StageManifestHandler> aHandler = new StageManifestHandler("StageManifest", aStack, &theManifest);
	aStack->addHandler(aHandler);
	engine::Ref<engine::InputStream> aStream = engine::App::instance()->openStream(theFile.c_str());
	engine::parseXml(*aStack, aStream);
}
