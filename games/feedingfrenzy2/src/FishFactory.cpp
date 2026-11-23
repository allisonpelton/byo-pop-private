#include "FishFactory.h"

#include "Engine.h"

using namespace engine;

ModelInfo* FishFactory::findAnyModel(const std::string& theName) const
{
	ModelInfo* aModel = findModel(theName);
	if (aModel == nullptr)
	{
		aModel = findFishModel(theName);
		if (aModel == nullptr)
		{
			// The original shows a message box (the engine's slot 0xd8); the game's file names no missing model
			getEngine()->log("Bad Name: There is no such model as %s.\n", theName.c_str());
		}
	}
	return aModel;
}

VisualInfo* FishFactory::findVisual(const std::string& theName)
{
	VisualInfo* aVisual = find(mVisuals, theName);
	if (aVisual == nullptr)
	{
		ModelInfo* aModel = findAnyModel(theName);
		if (aModel == nullptr)
			getEngine()->log("Bad Name: There is no such visual or model as %s.\n", theName.c_str());
		else
		{
			// A visual of no colour effect, named for the model
			aVisual = new VisualInfo();
			aVisual->mName = theName;
			aVisual->mModel = aModel;
			mVisuals[aVisual->mName] = aVisual;
		}
	}
	return aVisual;
}

FishVisualInfo* FishFactory::findFishVisual(const std::string& theName) const
{
	FishVisualInfo* aVisual = find(mFishVisuals, theName);
	if (aVisual == nullptr)
		getEngine()->log("Bad Name: There is no such fishVisual as %s.\n", theName.c_str());
	return aVisual;
}

FishFactory* FishFactory::get()
{
	// The original's is made with new and never freed; the port's is freed as the program ends
	static FishFactory theFactory;
	return &theFactory;
}

bool FishFactory::load(const std::string& thePath)
{
	MasterDocHandler aMaster;
	FishFactoryDocHandler* aHandler = new FishFactoryDocHandler(this);
	aHandler->attach(&aMaster, "fishes");
	return getEngine()->parseXml(thePath, &aMaster);
}

void FishFactory::clear()
{
	mModels.clear();
	mVisuals.clear();
	mBehaviors.clear();
	mFishModels.clear();
	mFishVisuals.clear();
	mFishBehaviors.clear();
}

void FishFactoryDocHandler::begin(Params& theAttributes)
{
	mFactory->mAnimBase = theAttributes.getString("animBase", "");
}

void FishFactoryDocHandler::startElement(const std::string& theName, Params& theAttributes)
{
	// The handler for the element goes on top of the master's stack and begins with its attributes
	SubDocHandler* aHandler;
	if (theName == "model")
		aHandler = new ModelInfoDocHandler(new ModelInfo());
	else if (theName == "visual")
		aHandler = new VisualInfoDocHandler(new VisualInfo());
	else if (theName == "behavior")
	{
		BehaviorInfo* aBehavior = new BehaviorInfo();
		aBehavior->load(theAttributes);
		mFactory->mBehaviors[aBehavior->mName] = aBehavior;
		return;
	}
	else if (theName == "fishModel")
		aHandler = new ModelInfoDocHandler(new FishModelInfo());
	else if (theName == "fishVisual")
		aHandler = new VisualInfoDocHandler(new FishVisualInfo());
	else if (theName == "fishBehavior")
		aHandler = new FishBehaviorInfoDocHandler();
	else
		return;
	aHandler->attach(mMaster, theName);
	aHandler->begin(theAttributes);
}
