// ConfigParser.cpp: see ConfigParser.h.

#include "ConfigParser.h"

#include "Buffer.h"
#include "Common.h"
#include "SexyAppBase.h"

#include <cctype>
#include <cstdio>

using namespace Sexy;

static const char kTrimmedCharacters[] = " \t\r\n";

// 0x4c03f0
ConfigParser::ConfigParser(bool theThrowErrors) : mEvent(EVENT_NONE), mReadPos(0), mLine(1), mEventLine(0), mThrowErrors(theThrowErrors)
{
	mValParser.mThrowErrors = theThrowErrors;
}

// 0x4c0510 (the file is read through the game's file system, so loose files and the pak both work)
bool ConfigParser::Open(const std::string& theFileName, bool theRequired)
{
	mFileName = theFileName;
	mGroups.clear();
	mText.clear();
	mReadPos = 0;
	mLine = 1;
	mEvent = EVENT_NONE;
	mKey.clear();
	mValue.clear();

	Buffer aBuffer;
	if (!gSexyAppBase->ReadBufferFromFile(theFileName, &aBuffer))
	{
		if (theRequired)
			Error("Unable to open file: " + theFileName);
		return false;
	}
	mText.assign((const char*)aBuffer.GetDataPtr(), aBuffer.GetDataLen());
	return true;
}

char ConfigParser::NextChar()
{
	char aChar = mText[mReadPos++];
	if (aChar == '\n')
		mLine++;
	return aChar;
}

char ConfigParser::PeekChar() const
{
	return mReadPos < mText.size() ? mText[mReadPos] : '\0';
}

// 0x4c1b90 with ('\n'): skips up to, but not past, the next newline
void ConfigParser::SkipLine()
{
	while (!AtEnd() && PeekChar() != '\n')
		mReadPos++;
}

// 0x4c1bd0 with ("*/"): skips past the next "*/" (or to the end of the text)
void ConfigParser::SkipComment()
{
	while (!AtEnd())
	{
		if (PeekChar() == '*' && mReadPos + 1 < mText.size() && mText[mReadPos + 1] == '/')
		{
			mReadPos += 2;
			return;
		}
		NextChar();
	}
}

// 0x4bf990: a token is up to the next white space, "{" or "}" (or through a ":"); a quoted string keeps its quotes and
// its white space. With theToEndOfLine the token is the rest of the line, trimmed.
void ConfigParser::ReadToken(std::string& theToken, bool theToEndOfLine)
{
	theToken.clear();
	while (!AtEnd())
	{
		char aChar = NextChar();
		if (aChar == '/' && PeekChar() == '/')
		{
			SkipLine();
			continue;
		}
		if (aChar == '/' && PeekChar() == '*')
		{
			int aLineBefore = mLine;
			mReadPos++;
			SkipComment();
			if (mLine != aLineBefore && theToEndOfLine)
				break;
			continue;
		}
		if (aChar == '"')
		{
			theToken += '"';
			while (!AtEnd())
			{
				aChar = NextChar();
				if (aChar == '\n')
				{
					Error("Carriage return found in quote");
					return;
				}
				theToken += aChar;
				if (aChar == '"')
					break;
			}
			continue;
		}
		if (!isspace((unsigned char)aChar))
		{
			if (aChar == ';')
			{
				if (theToEndOfLine)
					break;
			}
			else if (aChar == '{' || aChar == '}')
			{
				if (theToken.empty())
				{
					theToken = std::string(1, aChar);
				}
				else
				{
					mReadPos--;
				}
				return;
			}
			else if (aChar == '\\' && PeekChar() == '\n')
			{
				NextChar(); // a backslash before the newline joins the next line to this one
				continue;
			}
			theToken += aChar;
			if (aChar == ':' && !theToEndOfLine)
				return;
		}
		else if (!theToEndOfLine)
		{
			if (!theToken.empty())
				return;
		}
		else
		{
			if (aChar == '\n')
				break;
			theToken += aChar;
		}
	}
	if (theToEndOfLine)
	{
		size_t aLast = theToken.find_last_not_of(kTrimmedCharacters);
		if (aLast == std::string::npos)
			theToken.clear();
		else
		{
			theToken.erase(aLast + 1);
			theToken.erase(0, theToken.find_first_not_of(kTrimmedCharacters));
		}
	}
}

// 0x4c0a20
bool ConfigParser::DoReadNext()
{
	std::string aToken;
	ReadToken(aToken, false);
	if (aToken.empty())
		return false;

	if (aToken[aToken.size() - 1] == ':')
	{
		aToken.erase(aToken.size() - 1);
		mKey = aToken;
		mEventLine = mLine;
		ReadToken(mValue, true);
		mEvent = EVENT_KEY;
		return true;
	}

	if (aToken == "}")
	{
		if (mGroups.empty())
		{
			Error("Unexpected }");
			return false;
		}
		mEvent = EVENT_GROUP_END;
		return true;
	}

	// A group: its type, then an optional name (quoted or not) and "{" on the same line or on the next
	Group aGroup;
	aGroup.mType = aToken;
	mEventLine = mLine;
	std::string aRest;
	ReadToken(aRest, true);
	if (aRest != "{")
	{
		if (aRest.empty() || aRest[0] != '"')
			aGroup.mName = aRest;
		else
		{
			StringParser aNameParser;
			aNameParser.mThrowErrors = mThrowErrors;
			aNameParser.Set(aRest);
			aNameParser.ReadString(aGroup.mName, false, true);
		}
		ReadToken(aRest, false);
		if (aRest != "{")
		{
			Error("Expecting {");
			return false;
		}
	}
	mGroups.push_back(aGroup);
	mEvent = EVENT_GROUP_BEGIN;
	return true;
}

// 0x4c0db0
bool ConfigParser::ReadNext()
{
	if (mEvent == EVENT_GROUP_END && !mGroups.empty())
		mGroups.pop_back();
	if (DoReadNext())
		return true;
	return false;
}

// 0x4be5a0
StringParser& ConfigParser::GetValParser()
{
	mValParser.Set(mValue);
	return mValParser;
}

// 0x43e2f0
bool ConfigParser::KeyIs(const char* theKey) const
{
	return sexy_stricmp(mKey.c_str(), theKey) == 0;
}

// 0x4bf3d0
bool ConfigParser::GroupIs(const char* theType, int theDepth)
{
	if (theDepth >= 0 && theDepth != (int)mGroups.size())
	{
		ErrorUnexpectedGroupType();
		return false;
	}
	const std::string& aType = mGroups.empty() ? std::string() : mGroups.back().mType;
	return sexy_stricmp(aType.c_str(), theType) == 0;
}

// 0x4bf020
bool ConfigParser::Error(const std::string& theMessage, bool theUseKeyLine, bool theMayThrow)
{
	char aWhere[512];
	snprintf(aWhere, sizeof(aWhere), " (%s:%d)", mFileName.c_str(), theUseKeyLine ? mEventLine : mLine);
	if (mThrowErrors && theMayThrow)
		throw ParserException(theMessage + aWhere);
	return false;
}

// 0x4bf2a0
bool ConfigParser::ErrorUnexpectedGroupType()
{
	return Error("Unexpected Group - " + (mGroups.empty() ? std::string() : mGroups.back().mType));
}

// 0x4bf350
bool ConfigParser::ErrorUnexpectedKey()
{
	return Error("Unexpected Key - " + mKey, true);
}
