#include "StringTable.h"

#include "Engine.h"

using namespace engine;

namespace
{

// What a missing name gives
const std::string EMPTY_STRING;

} // namespace

void StringTable::load(const std::string& thePath)
{
	clear();
	StringTableDocHandler aHandler(this);
	// The handler is not counted by anyone: a reference held so its count never reaches 0
	aHandler.addRef();
	getEngine()->parseXml(thePath, &aHandler);
}

const std::string& StringTable::getString(const std::string& theName)
{
	std::map<std::string, std::string>::iterator anIt = mStrings.find(theName);
	if (anIt != mStrings.end())
		return anIt->second;
	return EMPTY_STRING;
}

void StringTableDocHandler::startElement(const std::string& theName, Params& theAttributes)
{
	if (theName != "string")
		return;
	std::string aName = theAttributes.getString("name", "");
	std::string aString = theAttributes.getString("string", "");
	mTable->addString(aName, aString);
}
