// GalaxyParser.cpp: GalaxyParser (GalaxyParser.h).
#include "GalaxyParser.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "Buffer.h"
#include "XMLParser.h"

using namespace Sexy;

// 0x596d35 and 0x596d73: an index that is out of range makes the original's vector access call the CRT's invalid
// parameter handler
Moon& Planet::GetPuzzle(int theIndex)
{
	assert(theIndex >= 0 && theIndex < GetPuzzleCount());
	return mMoons[theIndex];
}

// port: a puzzle of the planet (its moons)
const Moon& Planet::GetPuzzle(int theIndex) const
{
	assert(theIndex >= 0 && theIndex < GetPuzzleCount());
	return mMoons[theIndex];
}

// 0x5bfee9
GalaxyParser::GalaxyParser()
{
	mParser = NULL;
	mHasFailed = false;
}

// 0x5bff36
GalaxyParser::~GalaxyParser()
{
	delete mParser;
}

Planet& GalaxyParser::GetPlanet(int theIndex)
{
	assert(theIndex >= 0 && theIndex < GetPlanetCount());
	return mPlanets[theIndex];
}

const Planet& GalaxyParser::GetPlanet(int theIndex) const
{
	assert(theIndex >= 0 && theIndex < GetPlanetCount());
	return mPlanets[theIndex];
}

// 0x5be56d: only the first failure counts; while the file is parsed, the parser's line and file are added to its text
void GalaxyParser::Fail(const std::string& theError)
{
	if (mHasFailed)
		return;
	mHasFailed = true;
	mErrorText = theError;
	if (mParser == NULL)
		return;
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

// 0x5bf057: the text of a section that has begun, up to its end. A section inside it fails the parse (the text so far
// stays in theString).
bool GalaxyParser::ParseSingleElement(std::string* theString)
{
	*theString = "";
	XMLElement anElement;
	while (mParser->NextElement(&anElement))
	{
		if (anElement.mType == XMLElement::TYPE_START)
		{
			Fail("Unexpected Section: '" + anElement.mValue + "'");
			return false;
		}
		else if (anElement.mType == XMLElement::TYPE_ELEMENT)
			*theString = anElement.mValue;
		else if (anElement.mType == XMLElement::TYPE_END)
			return true;
	}
	return false;
}

// 0x5bf26a: the end of a section that has no content
bool GalaxyParser::ParseEnd()
{
	XMLElement anElement;
	while (mParser->NextElement(&anElement))
	{
		if (anElement.mType == XMLElement::TYPE_START)
		{
			Fail("Unexpected Section: '" + anElement.mValue + "'");
			return false;
		}
		else if (anElement.mType == XMLElement::TYPE_ELEMENT)
		{
			Fail("Unexpected Element: '" + anElement.mValue + "'");
			return false;
		}
		else if (anElement.mType == XMLElement::TYPE_END)
			return true;
	}
	return false;
}

// 0x5bf3ab: the inside of <Tutorial>: the <Text> lines and the <Move> cells. The result of a <Text> line's parse is not
// looked at (a failed one has called Fail, and its line is kept).
bool GalaxyParser::ParseTutorial(Planet* thePlanet)
{
	XMLElement anElement;
	while (mParser->NextElement(&anElement))
	{
		if (anElement.mType == XMLElement::TYPE_START)
		{
			if (anElement.mValue == "Text")
			{
				std::string aText;
				ParseSingleElement(&aText);
				thePlanet->mTutorialText.push_back(aText);
			}
			else if (anElement.mValue == "Move")
			{
				if (!ParseEnd())
					return false;
				int aCol = (int)atol(anElement.mAttributes["col"].c_str());
				int aRow = (int)atol(anElement.mAttributes["row"].c_str());
				thePlanet->mTutorialMoves.push_back(TutorialMove(aCol, aRow));
			}
			else
			{
				Fail("Invalid Section '" + anElement.mValue + "'");
				return false;
			}
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

// 0x5bf628: the inside of <Planet>: its <Moon> puzzles and its <Tutorial>
bool GalaxyParser::ParsePlanet(Planet* thePlanet)
{
	XMLElement anElement;
	while (mParser->NextElement(&anElement))
	{
		if (anElement.mType == XMLElement::TYPE_START)
		{
			if (anElement.mValue == "Moon")
			{
				Moon aMoon;
				aMoon.mSImage = anElement.mAttributes["simage"];
				aMoon.mUImage = anElement.mAttributes["uimage"];
				aMoon.mDImage = anElement.mAttributes["dimage"];
				aMoon.mFile = anElement.mAttributes["file"];
				aMoon.mName = anElement.mAttributes["name"];
				if (!ParseEnd())
					return false;
				thePlanet->mMoons.push_back(aMoon);
			}
			else if (anElement.mValue == "Tutorial")
			{
				thePlanet->mTutorialFile = anElement.mAttributes["file"];
				if (!ParseTutorial(thePlanet))
					return false;
			}
			else
			{
				Fail("Invalid Section '" + anElement.mValue + "'");
				return false;
			}
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

// 0x5c004b: the inside of <Galaxy>: the <Planet> sections
bool GalaxyParser::ParseGalaxy()
{
	XMLElement anElement;
	while (mParser->NextElement(&anElement))
	{
		if (anElement.mType == XMLElement::TYPE_START)
		{
			if (anElement.mValue != "Planet")
			{
				Fail("Invalid Section '" + anElement.mValue + "'");
				return false;
			}
			Planet aPlanet;
			aPlanet.mName = anElement.mAttributes["name"];
			aPlanet.mDesc = anElement.mAttributes["desc"];
			aPlanet.mAImage = anElement.mAttributes["aimage"];
			aPlanet.mDImage = anElement.mAttributes["dimage"];
			aPlanet.mBackdrop = anElement.mAttributes["bkg"];
			aPlanet.mMapX = (int)atol(anElement.mAttributes["mapx"].c_str());
			aPlanet.mMapY = (int)atol(anElement.mAttributes["mapy"].c_str());
			if (!ParsePlanet(&aPlanet))
				return false;
			mPlanets.push_back(aPlanet);
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

// 0x5c03d4: the file's <Galaxy> sections; then the parser's own failure is the parse's, and the parser is gone
bool GalaxyParser::DoParse()
{
	if (!mParser->HasFailed())
	{
		XMLElement anElement;
		while (mParser->NextElement(&anElement))
		{
			if (anElement.mType == XMLElement::TYPE_START)
			{
				if (anElement.mValue != "Galaxy")
				{
					Fail("Invalid Section '" + anElement.mValue + "'");
					break;
				}
				mMapImage = anElement.mAttributes["mapimage"];
				if (!ParseGalaxy())
					break;
			}
			else if (anElement.mType == XMLElement::TYPE_ELEMENT)
			{
				Fail("Element Not Expected '" + anElement.mValue + "'");
				break;
			}
		}
	}
	if (mParser->HasFailed())
		Fail(mParser->GetErrorText());
	delete mParser;
	mParser = NULL;
	return !mHasFailed;
}

// 0x5c05b5: starts over (the failure, the planets and the map image; the pack's name stays) and parses galaxy.xml's text.
// An empty buffer, as when the file cannot be opened, parses to an empty galaxy that has not failed, so the
// "Unable to open galaxy.xml file" that WinDMApp::LoadGalaxyData gives Fail first does not survive this call.
bool GalaxyParser::ParseBuffer(const Buffer& theBuffer)
{
	mHasFailed = false;
	mErrorText = "";
	mPlanets.clear();
	mMapImage = "";
	delete mParser;
	mParser = new XMLParser();
	const char* aData = (const char*)theBuffer.GetDataPtr();
	mParser->SetStringSource(std::string(aData, aData + theBuffer.GetDataLen()));
	return DoParse();
}

// 0x412500: the file name of a puzzle: the pack's folder (puzzles\<pack>) and the name from galaxy.xml
std::string PuzzlePath(const std::string& theName, const std::string& theDir)
{
	if (theDir.empty())
		return theName;
	char aLast = theDir[theDir.size() - 1];
	if (aLast == '\\' || aLast == '/')
		return theDir + theName;
	return theDir + "\\" + theName;
}
