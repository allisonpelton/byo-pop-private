// StringParser.cpp: see StringParser.h.

#include "StringParser.h"

#include "Common.h"

#include <cctype>
#include <cstdio>
#include <cstdlib>

using namespace Sexy;

static const char kTrimmedCharacters[] = " \t\r\n";

// Cuts the characters of kTrimmedCharacters off both ends of a string (0x40d6e0).
static void TrimString(std::string& theString)
{
	size_t aLast = theString.find_last_not_of(kTrimmedCharacters);
	if (aLast == std::string::npos)
	{
		theString.clear();
		return;
	}
	theString.erase(aLast + 1);
	theString.erase(0, theString.find_first_not_of(kTrimmedCharacters));
}

StringParser::StringParser() : mThrowErrors(false), mPos(0)
{
}

// 0x4c0ed0
void StringParser::Set(const std::string& theText)
{
	mText = theText;
	mPos = 0;
	mError.clear();
}

// 0x4c0e40
void StringParser::SkipWhitespace()
{
	while (mPos < mText.size() && isspace((unsigned char)mText[mPos]))
		mPos++;
}

// 0x4c0ea0
bool StringParser::CheckNextChar(char theChar)
{
	SkipWhitespace();
	if (CharAt(mPos) == theChar)
	{
		mPos++;
		return true;
	}
	return false;
}

// 0x4c0ff0
bool StringParser::EnsureNextChar(char theChar)
{
	if (CheckNextChar(theChar))
		return true;
	char aMessage[32];
	snprintf(aMessage, sizeof(aMessage), "Expecting %c", theChar);
	Error(aMessage);
	return false;
}

// 0x4c0fa0
bool StringParser::Error(const std::string& theMessage, bool theMayThrow)
{
	mError = theMessage;
	if (mThrowErrors && theMayThrow)
		throw ParserException(theMessage);
	return false;
}

// 0x4c1090
int StringParser::ReadIntBase(bool theMayThrow)
{
	SkipWhitespace();
	size_t aStart = mPos;
	bool aNegative = CharAt(mPos) == '-';
	if (aNegative)
		mPos++;
	if (CharAt(mPos) == '0' && tolower((unsigned char)CharAt(mPos + 1)) == 'x')
	{
		mPos += 2;
		while (isxdigit((unsigned char)CharAt(mPos)))
			mPos++;
		unsigned int aValue;
		if (aStart + 2 < mPos && sscanf(mText.c_str() + aStart + 2, "%x", &aValue) == 1)
			return aNegative ? -(int)aValue : (int)aValue;
		Error("Expecting Integer", theMayThrow);
		return 0;
	}
	while (isdigit((unsigned char)CharAt(mPos)))
		mPos++;
	if (aStart < mPos)
		return atol(mText.c_str() + aStart);
	Error("Expecting Integer", theMayThrow);
	return 0;
}

// 0x4c1270
int StringParser::ReadInt()
{
	return ReadIntBase(true);
}

// 0x4c1280
int StringParser::ReadInt(int theDefault)
{
	if (!mError.empty())
		return theDefault;
	int aValue = ReadIntBase(false);
	if (!mError.empty())
	{
		mError.clear();
		return theDefault;
	}
	return aValue;
}

// 0x4c12b0
bool StringParser::ReadString(std::string& theResult, bool theStopAtSpace, bool theTrim)
{
	theResult.clear();
	SkipWhitespace();
	bool aQuoted = CharAt(mPos) == '"';
	if (aQuoted)
		mPos++;
	while (mPos < mText.size())
	{
		char aChar = mText[mPos];
		if (aQuoted)
		{
			if (aChar == '"')
			{
				mPos++;
				if (CharAt(mPos) != '+')
					return true;
				mPos++;
				if (!CheckNextChar('"'))
					return Error("Missing continued quote");
				continue;
			}
			if (aChar == '\n')
				return Error("Missing end quote");
			if (aChar == '\\')
			{
				mPos++;
				char anEscaped = CharAt(mPos);
				switch (anEscaped)
				{
				case '"': aChar = '"'; break;
				case '\\': aChar = '\\'; break;
				case 'n': aChar = '\n'; break;
				case 't': aChar = '\t'; break;
				default:
					return Error(std::string("Invalid escape char: \\") + anEscaped);
				}
			}
		}
		else
		{
			if (!isspace((unsigned char)aChar))
			{
				if (aChar == ',')
					break;
			}
			else if (theStopAtSpace)
				return true;
		}
		theResult += aChar;
		mPos++;
	}
	if (!theStopAtSpace && theTrim)
		TrimString(theResult);
	return true;
}

// 0x4c1550
std::string StringParser::ReadString(bool theStopAtSpace, bool theTrim)
{
	std::string aResult;
	ReadString(aResult, theStopAtSpace, theTrim);
	return aResult;
}

// 0x4c15c0
bool StringParser::ReadHTMLString(std::string& theResult)
{
	SkipWhitespace();
	if (CharAt(mPos) == '"')
		return ReadString(theResult, false, true);

	bool aSpacePending = false;	 // a run of white space was seen since the last character
	bool aAfterBreak = false;	 // the last thing added was a line break from "\" + newline
	bool aAfterPeriod = false;	 // the last character added was a period
	while (mPos < mText.size())
	{
		char aChar = mText[mPos];
		if (aChar == '\\')
		{
			mPos++;
			aChar = CharAt(mPos);
			if (aChar == '\n')
			{
				theResult += '\n';
				aAfterBreak = true;
				aSpacePending = false;
			}
			else if (aChar != '\\')
				return Error(std::string("Invalid escape char: \\") + aChar);
			mPos++;
		}
		else if (!isspace((unsigned char)aChar))
		{
			if (aSpacePending)
			{
				theResult += ' ';
				if (aAfterPeriod)
					theResult += ' ';
			}
			theResult += aChar;
			mPos++;
			aAfterPeriod = aChar == '.';
			aSpacePending = false;
			aAfterBreak = false;
		}
		else
		{
			if (!aAfterBreak)
				aSpacePending = true;
			mPos++;
		}
	}
	return true;
}

// 0x4c1750
bool StringParser::ReadBool()
{
	std::string aText;
	ReadString(aText, true, true);
	if (sexy_stricmp(aText.c_str(), "true") == 0 || sexy_stricmp(aText.c_str(), "y") == 0)
		return true;
	if (sexy_stricmp(aText.c_str(), "false") == 0 || sexy_stricmp(aText.c_str(), "n") == 0)
		return false;
	Error("Expecting Boolean");
	return false;
}

// 0x453880
bool StringParser::ReadIntList(std::vector<int>& theResult, bool theClear)
{
	if (theClear)
		theResult.clear();
	SkipWhitespace();
	while (mPos < mText.size())
	{
		int aValue = ReadInt();
		if (!mError.empty())
			return false;
		theResult.push_back(aValue);
		if (!CheckNextChar(','))
			break;
	}
	return true;
}

// 0x4653a0
bool StringParser::ReadStringList(std::vector<std::string>& theResult, bool theClear)
{
	if (theClear)
		theResult.clear();
	SkipWhitespace();
	while (mPos < mText.size())
	{
		theResult.emplace_back();
		ReadString(theResult.back(), true, true);
		if (!CheckNextChar(','))
			break;
	}
	return true;
}
