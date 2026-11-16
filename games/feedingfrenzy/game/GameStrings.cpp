#include "GameStrings.h"
#include "App.h"
#include "DocHandler.h"
#include "Exception.h"
#include "Layout.h"
#include "Stream.h"

#include <cctype>
#include <vector>

using namespace feedingfrenzy;

namespace
{
const char* const MISSING_STRING = "!!!MISSING STRING RESOURCE!!!";

// The handler of the table's elements: each "string" element's name and text (0x43cfa0, vtable 0x4a76d4)
class StringTableHandler : public engine::BaseDocHandler
{
public:
	StringTableHandler(const std::string& theName, engine::DocHandlerStack* theParent, GameStrings* theStrings) :
		engine::BaseDocHandler(theName, theParent), mStrings(theStrings)
	{
	}

	void startElement(const std::string& theName, const engine::XmlAttributes& theAttributes) override
	{
		if (theName != "string")
			return;
		mStrings->add(engine::getStringAttribute(theAttributes, "name", ""),
			engine::getStringAttribute(theAttributes, "string", ""));
	}

private:
	GameStrings* mStrings;
};
}

// 0x43d260
void GameStrings::load(const std::string& theFile)
{
	engine::Ref<engine::DocHandlerStack> aStack = new engine::DocHandlerStack();
	engine::Ref<StringTableHandler> aHandler = new StringTableHandler("StringTable", aStack, this);
	aStack->addHandler(aHandler);
	engine::Ref<engine::InputStream> aStream = engine::App::instance()->openStream(theFile.c_str());
	engine::parseXml(*aStack, aStream);
}

// 0x43d3c0
std::string GameStrings::get(const std::string& theName) const
{
	std::map<std::string, std::string>::const_iterator anIt = mStrings.find(theName);
	if (anIt == mStrings.end())
		return MISSING_STRING;
	return anIt->second;
}

// 0x42ce60: FormatMessage with the text as the format and the arguments as an array. An insert is "%n", with the
// number's or the text's format between exclamation marks ("%1!d!"; the plain "%1" is a text); "%!" is a "!" and "%%"
// a "%". An insert with no argument is left out.
std::string GameStrings::format(const std::string& theName, std::initializer_list<FormatArg> theArgs) const
{
	std::string aFormat = get(theName);
	if (aFormat.empty())
		return aFormat;
	std::vector<FormatArg> anArgs(theArgs);
	std::string aResult;
	for (size_t i = 0; i < aFormat.size(); i++)
	{
		char aChar = aFormat[i];
		if (aChar != '%' || i + 1 >= aFormat.size())
		{
			aResult += aChar;
			continue;
		}
		char aNext = aFormat[i + 1];
		if (aNext == '!' || aNext == '%')
		{
			aResult += aNext;
			i++;
			continue;
		}
		if (!std::isdigit((unsigned char)aNext))
		{
			aResult += aChar;
			continue;
		}
		size_t aPos = i + 1;
		int anIndex = 0;
		while (aPos < aFormat.size() && std::isdigit((unsigned char)aFormat[aPos]))
			anIndex = anIndex * 10 + (aFormat[aPos++] - '0');
		if (aPos < aFormat.size() && aFormat[aPos] == '!')
		{
			size_t anEnd = aFormat.find('!', aPos + 1);
			if (anEnd != std::string::npos)
				aPos = anEnd + 1;
		}
		if (anIndex >= 1 && anIndex <= (int)anArgs.size())
		{
			const FormatArg& anArg = anArgs[anIndex - 1];
			aResult += anArg.mIsText ? anArg.mText : std::to_string(anArg.mNumber);
		}
		i = aPos - 1;
	}
	return aResult;
}
