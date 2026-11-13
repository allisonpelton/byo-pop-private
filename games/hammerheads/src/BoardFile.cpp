#include "BoardFile.h"

#include <cstdlib>

#include "platform/VFS.h"

using namespace Sexy;

// port: The text of the chunks put together
static bool ReadChunks(const std::string& theFileName, std::string* theText)
{
	std::vector<uint8_t> aData;
	if (!Platform::VFS::Get().ReadAll(theFileName, aData))
		return false;

	size_t aPos = 0;
	while (aPos + 2 <= aData.size())
	{
		size_t aLength = aData[aPos] | (aData[aPos + 1] << 8);
		aPos += 2;
		if (aPos + aLength > aData.size())
			aLength = aData.size() - aPos;
		theText->append((const char*)&aData[aPos], aLength);
		aPos += aLength;
	}
	return true;
}

// port: The value of an attribute in the tag text between "<" and ">"; false when the tag has none of that name
static bool GetAttribute(const std::string& theTag, const char* theName, std::string* theValue)
{
	std::string aKey = std::string(" ") + theName + "=\"";
	size_t aPos = theTag.find(aKey);
	if (aPos == std::string::npos)
		return false;
	aPos += aKey.size();
	size_t anEnd = theTag.find('"', aPos);
	if (anEnd == std::string::npos)
		return false;
	*theValue = theTag.substr(aPos, anEnd - aPos);
	return true;
}

// port: reads a board file (a stream of length-prefixed text chunks) into the objects it lists; the original's reader is its TinyXML copy (0x463240-0x468490)
bool Sexy::LoadBoardFile(const std::string& theFileName, std::vector<BoardObject>* theObjects)
{
	theObjects->clear();

	std::string aText;
	if (!ReadChunks(theFileName, &aText) || aText.find("<Level") == std::string::npos)
		return false;

	size_t aPos = 0;
	while ((aPos = aText.find("<object ", aPos)) != std::string::npos)
	{
		size_t aTagEnd = aText.find('>', aPos);
		if (aTagEnd == std::string::npos)
			break;
		std::string aTag = aText.substr(aPos, aTagEnd - aPos);

		// the element runs to its closing tag (or ends at its own ">" when it is written "<object ... />")
		size_t anElementEnd = aTagEnd;
		if (aTagEnd == 0 || aText[aTagEnd - 1] != '/')
		{
			anElementEnd = aText.find("</object>", aTagEnd);
			if (anElementEnd == std::string::npos)
				anElementEnd = aText.size();
		}

		std::string aClass, aValue;
		GetAttribute(aTag, "Class", &aClass);

		BoardObject anObject;
		anObject.mX = 0;
		anObject.mY = 0;
		anObject.mChoice = 0;
		if (GetAttribute(aTag, "X", &aValue))
			anObject.mX = atof(aValue.c_str());
		if (GetAttribute(aTag, "Y", &aValue))
			anObject.mY = atof(aValue.c_str());

		bool aKnown = true;
		if (aClass == "class Sexy::Cubby")
			anObject.mKind = BOARD_OBJECT_CUBBY;
		else if (aClass == "class Sexy::Sticker")
		{
			anObject.mKind = BOARD_OBJECT_STICKER;
			size_t aChoice = aText.find("<choice name=\"Image\"", aTagEnd);
			if (aChoice != std::string::npos && aChoice < anElementEnd)
			{
				size_t aChoiceEnd = aText.find('>', aChoice);
				std::string aChoiceTag = aText.substr(aChoice, aChoiceEnd - aChoice);
				if (GetAttribute(aChoiceTag, "index", &aValue))
					anObject.mChoice = atoi(aValue.c_str());
			}
		}
		else
			aKnown = false;

		if (aKnown)
			theObjects->push_back(anObject);
		aPos = anElementEnd;
	}
	return true;
}
