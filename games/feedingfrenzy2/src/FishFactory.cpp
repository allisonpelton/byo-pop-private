#include "FishFactory.h"

#include "Engine.h"

using namespace engine;

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
