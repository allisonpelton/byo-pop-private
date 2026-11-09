#include "XcfFile.h"
#include "AstroPopApp.h"
#include "Buffer.h"
#include "Common.h"
#include <cstdlib>

using namespace Sexy;

XcfFile::Section::~Section()
{
	for (size_t i = 0; i < mChildren.size(); i++)
		delete mChildren[i];
}

// 0x427d80
XcfFile::XcfFile()
{
	mLine = -1;
}

// 0x426df0
XcfFile::~XcfFile()
{
	Clear();
}

void XcfFile::Clear()
{
	for (size_t i = 0; i < mTop.size(); i++)
		delete mTop[i];
	mTop.clear();
	mCursor.clear();
}

// 0x426f10
void XcfFile::AddKeyName(const std::string& theName)
{
	mKeyNames.push_back(Upper(theName));
}

// 0x42a5d0
int XcfFile::Parse(const std::string& theFileName)
{
	Buffer aBuffer;
	if (!gApp->ReadBufferFromFile(theFileName, &aBuffer, true))
		return PARSE_CANT_OPEN;

	Clear();
	mLine = -1;

	std::string aText((const char*) aBuffer.GetDataPtr(), (size_t) aBuffer.GetDataLen());
	std::vector<Section*> aStack;
	std::string aPendingName;
	bool aHavePending = false;
	int aLineNum = 0;
	size_t aPos = 0;

	while (aPos <= aText.length())
	{
		size_t anEnd = aText.find('\n', aPos);
		if (anEnd == std::string::npos)
			anEnd = aText.length();
		std::string aLine = Trim(aText.substr(aPos, anEnd - aPos));
		aPos = anEnd + 1;
		aLineNum++;

		if (aLine.empty() || aLine.compare(0, 2, "//") == 0)
			continue;

		if (aLine[0] == '@')
		{
			if (aHavePending)
			{
				mLine = aLineNum;
				return PARSE_SYNTAX;
			}
			aPendingName = Trim(aLine.substr(1));
			aHavePending = true;
		}
		else if (aLine == "{")
		{
			if (!aHavePending)
			{
				mLine = aLineNum;
				return PARSE_SYNTAX;
			}
			Section* aSection = new Section();
			aSection->mName = aPendingName;
			if (aStack.empty())
				mTop.push_back(aSection);
			else
				aStack.back()->mChildren.push_back(aSection);
			aStack.push_back(aSection);
			aHavePending = false;
		}
		else if (aLine == "}")
		{
			if (aStack.empty())
			{
				mLine = aLineNum;
				return PARSE_SYNTAX;
			}
			aStack.pop_back();
		}
		else
		{
			size_t anEquals = aLine.find('=');
			if (aStack.empty() || anEquals == std::string::npos)
			{
				mLine = aLineNum;
				return PARSE_SYNTAX;
			}
			aStack.back()->mValues[Upper(Trim(aLine.substr(0, anEquals)))] = Trim(aLine.substr(anEquals + 1));
		}
	}

	if (!aStack.empty() || aHavePending)
	{
		mLine = aLineNum;
		return PARSE_SYNTAX;
	}

	mLine = aLineNum;
	mCursor.clear();
	ResetCursor();
	return PARSE_OK;
}

// 0x425760 + 0x425570
void XcfFile::ResetCursor()
{
	mCursor.clear();
	if (!mTop.empty())
	{
		CursorEntry anEntry;
		anEntry.mIndex = 0;
		anEntry.mSection = mTop[0];
		mCursor.push_back(anEntry);
	}
}

// 0x4256e0
bool XcfFile::EnterFirstChild()
{
	if (mCursor.empty())
		return false;
	Section* aSection = mCursor.back().mSection;
	if (aSection->mChildren.empty())
		return false;
	CursorEntry anEntry;
	anEntry.mIndex = 0;
	anEntry.mSection = aSection->mChildren[0];
	mCursor.push_back(anEntry);
	return true;
}

// 0x425600
int XcfFile::NextSibling()
{
	if (mCursor.empty())
	{
		ResetCursor();
		return 0;
	}

	CursorEntry anEntry = mCursor.back();
	mCursor.pop_back();

	const std::vector<Section*>& aSiblings = mCursor.empty() ? mTop : mCursor.back().mSection->mChildren;
	int aNext = anEntry.mIndex + 1;
	if ((int) aSiblings.size() <= aNext)
	{
		mCursor.push_back(anEntry);
		return CURSOR_END;
	}

	anEntry.mIndex = aNext;
	anEntry.mSection = aSiblings[aNext];
	mCursor.push_back(anEntry);
	return 0;
}

// 0x412c70
std::string XcfFile::GetCurrentName()
{
	if (mCursor.empty())
		return "";
	return mCursor.back().mSection->mName;
}

// 0x426fc0
int XcfFile::FindSection(const std::string& theName)
{
	ResetCursor();
	for (;;)
	{
		if (GetCurrentName() == theName)
			return 0;
		int aResult = NextSibling();
		if (aResult != 0)
			return aResult;
	}
}

// Resolves a path to the section that holds its last component
XcfFile::Section* XcfFile::FindValueSection(const std::string& thePath, std::string* theKey)
{
	std::string aPath = Upper(thePath);
	if (!aPath.empty() && aPath[aPath.length() - 1] == '\\')
		aPath.erase(aPath.length() - 1);

	std::vector<std::string> aParts;
	size_t aStart = 0;
	bool anAbsolute = !aPath.empty() && aPath[0] == '\\';
	if (anAbsolute)
		aStart = 1;
	for (;;)
	{
		size_t aSlash = aPath.find('\\', aStart);
		if (aSlash == std::string::npos)
		{
			aParts.push_back(aPath.substr(aStart));
			break;
		}
		aParts.push_back(aPath.substr(aStart, aSlash - aStart));
		aStart = aSlash + 1;
	}

	*theKey = aParts.back();
	aParts.pop_back();

	Section* aSection = NULL;
	if (!anAbsolute)
	{
		if (mCursor.empty())
			return NULL;
		aSection = mCursor.back().mSection;
	}

	for (size_t i = 0; i < aParts.size(); i++)
	{
		const std::vector<Section*>& aList = (aSection == NULL) ? mTop : aSection->mChildren;
		Section* aFound = NULL;
		for (size_t j = 0; j < aList.size(); j++)
		{
			if (Upper(aList[j]->mName) == aParts[i])
			{
				aFound = aList[j];
				break;
			}
		}
		if (aFound == NULL)
			return NULL;
		aSection = aFound;
	}
	return aSection;
}

// 0x412d00
bool XcfFile::HasKey(const std::string& thePath)
{
	std::string aKey;
	Section* aSection = FindValueSection(thePath, &aKey);
	return aSection != NULL && aSection->mValues.find(aKey) != aSection->mValues.end();
}

// 0x413b10
std::string XcfFile::GetString(const std::string& thePath)
{
	std::string aKey;
	Section* aSection = FindValueSection(thePath, &aKey);
	if (aSection == NULL)
		return "";
	std::map<std::string, std::string>::iterator anItr = aSection->mValues.find(aKey);
	if (anItr == aSection->mValues.end())
		return "";
	return anItr->second;
}

// 0x4146e0
long XcfFile::GetInt(const std::string& thePath)
{
	return atol(GetString(thePath).c_str());
}

// 0x414960 (through 0x414820)
float XcfFile::GetFloat(const std::string& thePath)
{
	return (float) strtod(GetString(thePath).c_str(), NULL);
}
