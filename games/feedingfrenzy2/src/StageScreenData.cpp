#include "StageScreenData.h"

#include "Engine.h"
#include "Params.h"

using namespace engine;

void StageScreenData::load(const std::string& thePath)
{
	mEntries.clear();
	MasterDocHandler aMaster;
	ScreenDataDocHandler* aHandler = new ScreenDataDocHandler(this);
	aHandler->attach(&aMaster, "stageScreenData");
	getEngine()->parseXml(thePath, &aMaster);
}

void ScreenDataDocHandler::startElement(const std::string& theName, Params& theAttributes)
{
	if (theName == "screenData")
	{
		mEntry = new ScreenDataEntry();
		mEntry->mTitle = "no title";
		mEntry->mCaption = "no caption";
		mEntry->mPara1 = "no para 1";
		mEntry->mPara2 = "no para 2";
		mEntry->mPara1Header = "";
		mEntry->mPara2Header = "";
		mEntry->mTextLayout = theAttributes.getString("textLayout", "");
		mEntry->mImageLayout = theAttributes.getString("imgLayout", "");
		mData->mEntries[theAttributes.getString("id", "NO_ID")] = mEntry;
		return;
	}
	if (mEntry == nullptr)
		// (outside a <screenData> the original writes through a null entry)
		return;
	std::string* aTarget;
	if (theName == "title")
		aTarget = &mEntry->mTitle;
	else if (theName == "caption")
		aTarget = &mEntry->mCaption;
	else if (theName == "para1")
	{
		aTarget = &mEntry->mPara1;
		mEntry->mPara1Header = theAttributes.getString("header", "");
	}
	else if (theName == "para2")
	{
		aTarget = &mEntry->mPara2;
		mEntry->mPara2Header = theAttributes.getString("header", "");
	}
	else if (theName == "objective")
		aTarget = &mEntry->mObjective;
	else
		return;
	aTarget->clear();
	mTextTarget = aTarget;
}

void ScreenDataDocHandler::endElement(const std::string& theName)
{
	if (theName == "screenData")
	{
		mTextTarget = nullptr;
		mEntry = nullptr;
	}
	else if (theName == "title" || theName == "caption" || theName == "para1" || theName == "para2" ||
		theName == "objective")
		mTextTarget = nullptr;
}

void ScreenDataDocHandler::text(const std::string& theText)
{
	if (mTextTarget != nullptr)
		*mTextTarget += theText;
}
