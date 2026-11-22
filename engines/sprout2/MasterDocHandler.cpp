#include "MasterDocHandler.h"

#include "Engine.h"
#include "Exception.h"

namespace engine
{

void SubDocHandler::attach(MasterDocHandler* theMaster, const std::string& theRootName)
{
	mMaster = theMaster;
	mRootName = theRootName;
	theMaster->push(this);
}

void MasterDocHandler::startElement(const std::string& theName, Params& theAttributes)
{
	if (theName.empty())
		throw Exception("TRIED TO OPEN EMPTY TAG!");
	if (handleInclude(theName, theAttributes))
		return;
	SubDocHandler* aHandler = mHandlers.back();
	if (theName == aHandler->getRootName())
		aHandler->begin(theAttributes);
	else
		aHandler->startElement(theName, theAttributes);
}

void MasterDocHandler::endElement(const std::string& theName)
{
	if (theName.empty())
		throw Exception("TRIED TO OPEN EMPTY TAG!");
	if (theName == "xmlinclude")
	{
		mIncludeDepth--;
		mInInclude = false;
		return;
	}
	mHandlers.back()->endElement(theName);
	if (theName == mHandlers.back()->getRootName())
	{
		if (mHandlers.size() == 1)
			mHandlers.back()->endDocument();
		mHandlers.pop_back();
	}
}

void MasterDocHandler::startDocument()
{
	if (mIncludeDepth < 1)
		mHandlers.back()->startDocument();
}

void MasterDocHandler::text(const std::string& theText)
{
	mHandlers.back()->text(theText);
}

bool MasterDocHandler::handleInclude(const std::string& theName, Params& theAttributes)
{
	if (mInInclude)
		throw Exception("Can't start an element inside an include tag!");
	if (theName != "xmlinclude")
		return false;
	mIncludeDepth++;
	std::string aPath = theAttributes.getString("xmlpath", "");
	if (!aPath.empty())
		getEngine()->parseXml(aPath, this);
	mInInclude = true;
	return true;
}

} // namespace engine
