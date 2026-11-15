// BackdropsParser.cpp: BackdropsParser (BackdropsParser.h).
#include "BackdropsParser.h"

#include <stdio.h>
#include <stdlib.h>
#include "WinDMApp.h"
#include "XMLParser.h"

using namespace Sexy;

// port: reads a number of the backdrops file
static bool StringToDouble(const std::string& theString, double* theValue)
{
	if (theString.empty())
		return false;
	char* anEnd;
	*theValue = strtod(theString.c_str(), &anEnd);
	return *anEnd == '\0';
}

// 0x5936ad
BackdropsParser::BackdropsParser(WinDMApp* theApp)
{
	mApp = theApp;
	mParser = NULL;
	mHasFailed = false;
}

// 0x5936d2 (the destructor proper)
BackdropsParser::~BackdropsParser()
{
	delete mParser;
}

// 0x59413d
bool BackdropsParser::Parse(const std::string& theText)
{
	delete mParser;
	mParser = new XMLParser();
	mParser->SetStringSource(theText);
	mHasFailed = false;
	mErrorText = "";

	XMLElement anElement;
	while (mParser->NextElement(&anElement))
	{
		if (anElement.mType == XMLElement::TYPE_START)
		{
			if (anElement.mValue == "Backdrops")
				return ParseBackdrops();
			Fail("Invalid Section '" + anElement.mValue + "'");
			return false;
		}
	}
	return false;
}

// 0x593732: only the first failure counts
void BackdropsParser::Fail(const std::string& theError)
{
	if (mHasFailed)
		return;
	mHasFailed = true;
	mErrorText = theError;
	int aLine = mParser->GetCurrentLineNum();
	if (aLine > 0)
	{
		char aBuf[32];
		sprintf(aBuf, "%d", aLine);
		mErrorText += std::string(" on Line ") + aBuf;
	}
	if (!mParser->GetFileName().empty())
		mErrorText += " in File '" + mParser->GetFileName() + "'";
}

// 0x59387a: the end of an element that has no content
bool BackdropsParser::ParseEnd()
{
	XMLElement anElement;
	if (!mParser->NextElement(&anElement))
		return false;
	if (anElement.mType == XMLElement::TYPE_START)
	{
		Fail("Unexpected Section: '" + anElement.mValue + "'");
		return false;
	}
	if (anElement.mType == XMLElement::TYPE_ELEMENT)
	{
		Fail("Unexpected Element: '" + anElement.mValue + "'");
		return false;
	}
	return anElement.mType == XMLElement::TYPE_END;
}

// 0x5939a8: the colors of a backdrop: the user interface's (UIColorize) and the gems' (GemColorize), each as red, green
// and blue factors that the game keeps times 255
bool BackdropsParser::ParseBackdrop(int theId)
{
	XMLElement anElement;
	while (mParser->NextElement(&anElement))
	{
		if (anElement.mType == XMLElement::TYPE_START)
		{
			bool isUI = anElement.mValue == "UIColorize";
			if (!isUI && anElement.mValue != "GemColorize")
			{
				Fail("Invalid Section '" + anElement.mValue + "'");
				return false;
			}
			double aRed, aGreen, aBlue;
			if (!StringToDouble(anElement.mAttributes["R"], &aRed) || !StringToDouble(anElement.mAttributes["G"], &aGreen) ||
				!StringToDouble(anElement.mAttributes["B"], &aBlue))
			{
				Fail(isUI ? "Invalid RGB values" : "Invalid HSL values");
				return false;
			}
			Color& aColor = isUI ? mApp->mBackdropColors[theId].mColor1 : mApp->mBackdropColors[theId].mColor2;
			aColor.mRed = (int)(aRed * 255.0);
			aColor.mGreen = (int)(aGreen * 255.0);
			aColor.mBlue = (int)(aBlue * 255.0);
			ParseEnd();
		}
		else if (anElement.mType == XMLElement::TYPE_ELEMENT)
		{
			Fail("Element Not Expected '" + anElement.mValue + "'");
			return false;
		}
		else if (anElement.mType == XMLElement::TYPE_END)
			return true;
	}
	return false;
}

// 0x593e5e
bool BackdropsParser::ParseBackdrops()
{
	XMLElement anElement;
	while (mParser->NextElement(&anElement))
	{
		if (anElement.mType == XMLElement::TYPE_START)
		{
			if (anElement.mValue != "Backdrop")
			{
				Fail("Invalid Section '" + anElement.mValue + "'");
				return false;
			}
			double anId = 0;
			if (!StringToDouble(anElement.mAttributes["id"], &anId))
			{
				Fail("Invalid ID '" + anElement.mAttributes["id"] + "'");
				return false;
			}
			int anIdInt = (int)anId;
			if (anIdInt < 0 || anIdInt >= 50)
			{
				Fail("Invalid ID '" + anElement.mAttributes["id"] + "'");
				return false;
			}
			if (!ParseBackdrop(anIdInt))
				return false;
		}
		else if (anElement.mType == XMLElement::TYPE_ELEMENT)
		{
			Fail("Element Not Expected '" + anElement.mValue + "'");
			return false;
		}
		else if (anElement.mType == XMLElement::TYPE_END)
			return true;
	}
	return false;
}
