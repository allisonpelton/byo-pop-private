#include "QDParser.h"

#include "DocHandler.h"
#include "Engine.h"
#include "Params.h"
#include "Stream.h"

#include <cstdlib>
#include <deque>
#include <string>

namespace engine
{

namespace
{

// The parser's modes, by the numbers the original uses
enum Mode
{
	MODE_TEXT = 1,               // between tags, collecting text
	MODE_ENTITY = 2,             // after a '&', up to its ';'
	MODE_OPEN_TAG = 3,           // in <name, or what may yet be a comment, CDATA or a DOCTYPE
	MODE_CLOSE_TAG = 4,          // in </name
	MODE_START_TAG = 5,          // just after a '<'
	MODE_ATTRIBUTE_LVALUE = 6,   // an attribute's name
	MODE_QUOTE = 7,              // an attribute's quoted value
	MODE_IN_TAG = 8,             // between a tag's attributes
	MODE_ATTRIBUTE_EQUAL = 9,    // after an attribute's name, looking for '='
	MODE_ATTRIBUTE_RVALUE = 10,  // after '=', looking for the opening quote
	MODE_DONE = 11,              // the root element closed
	MODE_SINGLE_TAG = 12,        // after the '/' of <name/>, looking for '>'
	MODE_COMMENT = 13,           // in <!-- -->
	MODE_DOCTYPE = 14,           // in <? ?> or <!DOCTYPE >
	MODE_PRE = 15,               // before the root element
	MODE_CDATA = 16              // in <![CDATA[ ]]>
};

// 0x467620 (feedingfrenzy2): the mode on top of the stack taken off, PRE when there is none
int popMode(std::deque<int>& theStack)
{
	if (theStack.empty())
		return MODE_PRE;
	int aMode = theStack.back();
	theStack.pop_back();
	return aMode;
}

// isspace in the C locale (the original passes the character as a signed char; the game's files hold no other bytes in
// their tags)
bool isSpace(int c)
{
	return c == ' ' || (c >= '\t' && c <= '\r');
}

bool endsWith(const std::string& theString, const char* theEnd)
{
	size_t aLength = std::char_traits<char>::length(theEnd);
	return theString.size() >= aLength && theString.compare(theString.size() - aLength, aLength, theEnd) == 0;
}

// 0x5081fc (feedingfrenzy2)
void logError(const std::string& theMessage, int theLine, int theColumn)
{
	getEngine()->log("%s near line %d, column %d\n", theMessage.c_str(), theLine, theColumn);
}

} // namespace

bool QDParser::parse(DocHandler* theHandler, Reader* theReader)
{
	std::deque<int> aStack;
	int aDepth = 0;
	int aMode = MODE_PRE;
	int aQuote = '"';
	std::string aBuffer;
	std::string anEntity;
	std::string aTagName;
	std::string anAttributeName;
	std::string anAttributeValue;
	Params anAttributes;
	anAttributes.addRef();  // port: a stack object, so a handler's reference never deletes it
	theHandler->startDocument();
	int aLine = 1;
	int aColumn = 0;
	bool anEndOfLine = false;

	for (;;)
	{
		int c = theReader->read();
		if (c == -1)
			break;

		// \r, \r\n and \n are all read as \n
		if (c == '\n')
		{
			if (anEndOfLine)
			{
				anEndOfLine = false;
				continue;
			}
			aLine++;
			aColumn = 0;
		}
		else if (anEndOfLine)
			anEndOfLine = false;
		else if (c == '\r')
		{
			aLine++;
			anEndOfLine = true;
			c = '\n';
			aColumn = 0;
		}
		else
			aColumn++;

		if (aMode == MODE_DONE)
		{
			theHandler->endDocument();
			return true;
		}
		else if (aMode == MODE_CLOSE_TAG)
		{
			if (c == '>')
			{
				aMode = popMode(aStack);
				aTagName = aBuffer;
				aBuffer.clear();
				aDepth--;
				if (aDepth == 0)
					aMode = MODE_DONE;
				theHandler->endElement(aTagName);
			}
			else
				aBuffer += (char)c;
		}
		else if (aMode == MODE_CDATA)
		{
			if (c == '>' && endsWith(aBuffer, "]]"))
			{
				aBuffer.resize(aBuffer.size() - 2);
				theHandler->text(aBuffer);
				aBuffer.clear();
				aMode = popMode(aStack);
			}
			else
				aBuffer += (char)c;
		}
		else if (aMode == MODE_COMMENT)
		{
			if (c == '>' && endsWith(aBuffer, "--"))
			{
				aBuffer.clear();
				aMode = popMode(aStack);
			}
			else
				aBuffer += (char)c;
		}
		else if (aMode == MODE_PRE)
		{
			if (c == '<')
			{
				aStack.push_back(MODE_TEXT);
				aMode = MODE_START_TAG;
			}
		}
		else if (aMode == MODE_DOCTYPE)
		{
			if (c == '>')
			{
				aMode = popMode(aStack);
				if (aMode == MODE_TEXT)
					aMode = MODE_PRE;
			}
		}
		else if (aMode == MODE_START_TAG)
		{
			aMode = popMode(aStack);
			if (c == '/')
			{
				aStack.push_back(aMode);
				aMode = MODE_CLOSE_TAG;
			}
			else if (c == '?')
				aMode = MODE_DOCTYPE;
			else
			{
				aStack.push_back(aMode);
				aMode = MODE_OPEN_TAG;
				aTagName.clear();
				anAttributes.clear();
				aBuffer += (char)c;
			}
		}
		else if (aMode == MODE_ENTITY)
		{
			if (c == ';')
			{
				aMode = popMode(aStack);
				std::string aName = anEntity;
				anEntity.clear();
				if (aName == "lt")
					aBuffer += '<';
				else if (aName == "gt")
					aBuffer += '>';
				else if (aName == "amp")
					aBuffer += '&';
				else if (aName == "quot")
					aBuffer += '"';
				else if (aName == "apos")
					aBuffer += '\'';
				else if (aName.compare(0, 1, "#") == 0)
					aBuffer += (char)atol(aName.substr(1).c_str());
				else
				{
					logError("Unknown entity: &" + aName + ";", aLine, aColumn);
					return false;
				}
			}
			else
				anEntity += (char)c;
		}
		else if (aMode == MODE_SINGLE_TAG)
		{
			if (aTagName.empty())
				aTagName = aBuffer;
			if (c != '>')
			{
				logError("Expected >\tfor\ttag: <" + aTagName + "/>", aLine, aColumn);
				return false;
			}
			theHandler->startElement(aTagName, anAttributes);
			theHandler->endElement(aTagName);
			if (aDepth == 0)
			{
				theHandler->endDocument();
				return true;
			}
			aBuffer.clear();
			anAttributes.clear();
			aTagName.clear();
			aMode = popMode(aStack);
		}
		else if (aMode == MODE_OPEN_TAG)
		{
			if (c == '>')
			{
				if (aTagName.empty())
					aTagName = aBuffer;
				aBuffer.clear();
				aDepth++;
				theHandler->startElement(aTagName, anAttributes);
				aTagName.clear();
				anAttributes.clear();
				aMode = popMode(aStack);
			}
			else if (c == '/')
				aMode = MODE_SINGLE_TAG;
			else if (c == '-' && aBuffer == "!-")
				aMode = MODE_COMMENT;
			else if (c == '[' && aBuffer == "![CDATA")
			{
				aMode = MODE_CDATA;
				aBuffer.clear();
			}
			else if (c == 'E' && aBuffer == "!DOCTYP")
			{
				aBuffer.clear();
				aMode = MODE_DOCTYPE;
			}
			else if (isSpace(c))
			{
				aTagName = aBuffer;
				aBuffer.clear();
				aMode = MODE_IN_TAG;
			}
			else
				aBuffer += (char)c;
		}
		else if (aMode == MODE_QUOTE)
		{
			if (c == aQuote)
			{
				anAttributeValue = aBuffer;
				aBuffer.clear();
				anAttributes.set(anAttributeName, anAttributeValue);
				aMode = MODE_IN_TAG;
			}
			else if (std::string("\t\r\n").find((char)c) != std::string::npos)
			{
				// The original builds the buffer with a tab added and drops the copy, so the character is lost
			}
			else if (c == '&')
			{
				aStack.push_back(aMode);
				aMode = MODE_ENTITY;
				anEntity.clear();
			}
			else
				aBuffer += (char)c;
		}
		else if (aMode == MODE_ATTRIBUTE_RVALUE)
		{
			if (c == '"' || c == '\'')
			{
				aQuote = c;
				aMode = MODE_QUOTE;
			}
			else if (!isSpace(c))
			{
				logError("Error in attribute\tprocessing", aLine, aColumn);
				return false;
			}
		}
		else if (aMode == MODE_ATTRIBUTE_LVALUE)
		{
			if (isSpace(c))
			{
				anAttributeName = aBuffer;
				aBuffer.clear();
				aMode = MODE_ATTRIBUTE_EQUAL;
			}
			else if (c == '=')
			{
				anAttributeName = aBuffer;
				aBuffer.clear();
				aMode = MODE_ATTRIBUTE_RVALUE;
			}
			else
				aBuffer += (char)c;
		}
		else if (aMode == MODE_ATTRIBUTE_EQUAL)
		{
			if (c == '=')
				aMode = MODE_ATTRIBUTE_RVALUE;
			else if (!isSpace(c))
			{
				logError("Error in attribute\tprocessing.", aLine, aColumn);
				return false;
			}
		}
		else if (aMode == MODE_IN_TAG)
		{
			if (c == '>')
			{
				aMode = popMode(aStack);
				theHandler->startElement(aTagName, anAttributes);
				aDepth++;
				aTagName.clear();
				anAttributes.clear();
			}
			else if (c == '/')
				aMode = MODE_SINGLE_TAG;
			else if (!isSpace(c))
			{
				if (c == '<')
				{
					logError("Error in tag processing. Can't open sub tag without closing tag first!", aLine, aColumn);
					return false;
				}
				aMode = MODE_ATTRIBUTE_LVALUE;
				aBuffer += (char)c;
			}
		}
		else
		{
			// MODE_TEXT
			if (c == '<')
			{
				aStack.push_back(aMode);
				aMode = MODE_START_TAG;
				if (!aBuffer.empty())
				{
					theHandler->text(aBuffer);
					aBuffer.clear();
				}
			}
			else if (c == '&')
			{
				aStack.push_back(aMode);
				aMode = MODE_ENTITY;
				anEntity.clear();
			}
			else
				aBuffer += (char)c;
		}
	}

	if (aMode != MODE_DONE)
	{
		logError("missing end tag", aLine, aColumn);
		return false;
	}
	theHandler->endDocument();
	return true;
}

} // namespace engine
