#include "FunFacts.h"

#include "Engine.h"

using namespace engine;

void FunFacts::load(const std::string& thePath)
{
	mFacts.clear();
	MasterDocHandler aMaster;
	FunFactDocHandler* aHandler = new FunFactDocHandler(this);
	aHandler->attach(&aMaster, "funFacts");
	mNextFact = 0;
	getEngine()->parseXml(thePath, &aMaster);
}

void FunFactDocHandler::startElement(const std::string& theName, Params& theAttributes)
{
	if (theName == "fact")
	{
		mFact.clear();
		mInFact = true;
	}
}

void FunFactDocHandler::endElement(const std::string& theName)
{
	if (theName == "fact")
	{
		mInFact = false;
		mFacts->mFacts.push_back(mFact);
	}
}

void FunFactDocHandler::text(const std::string& theText)
{
	if (mInFact)
		mFact += theText;
}
