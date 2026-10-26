#include "Xml.h"
#include "Exception.h"
#include "Stream.h"
#include <cctype>
#include <cstdlib>
#include <vector>

using namespace engine;

// 0x43eec0
std::string XmlAttributes::getString(const std::string& theName, const std::string& theDefault) const
{
	const_iterator anIt = find(theName);
	if (anIt != end())
		return anIt->second;
	return theDefault;
}

// 0x43ef90
long XmlAttributes::getInt(const std::string& theName, long theDefault) const
{
	const_iterator anIt = find(theName);
	if (anIt == end())
		return theDefault;
	std::string aValue = anIt->second;
	return atol(aValue.c_str());
}

// The states of the parser (0x43b1f0)
enum
{
	CONTENT = 1,          // between tags
	ENTITY = 2,           // after '&', up to ';'
	TAG_NAME = 3,         // in a tag's name
	END_TAG = 4,          // after "</"
	TAG_START = 5,        // after '<'
	ATTR_NAME = 6,
	ATTR_VALUE = 7,       // inside the quotes
	IN_TAG = 8,           // in a start tag, between attributes
	AFTER_ATTR_NAME = 9,  // after a name and white space, before '='
	BEFORE_VALUE = 10,    // after '=', before the quote
	DONE = 11,            // the root element has ended
	EMPTY_TAG = 12,       // after a '/' in a start tag
	COMMENT = 13,
	SKIP = 14,            // a processing instruction or a DOCTYPE, up to '>'
	PROLOG = 15,          // before the root element
	CDATA = 16
};

// 0x43a730: the state to return to; the prolog when the stack is empty
static int popState(std::vector<int>& theStack)
{
	if (theStack.empty())
		return PROLOG;
	int aState = theStack.back();
	theStack.pop_back();
	return aState;
}

// 0x43a6d0
static bool endsWith(const std::string& theString, const std::string& theEnd)
{
	if (theEnd.size() > theString.size())
		return false;
	return theString.compare(theString.size() - theEnd.size(), theEnd.size(), theEnd) == 0;
}

// 0x43a680
static bool startsWith(const std::string& theString, const std::string& theStart)
{
	if (theStart.size() > theString.size())
		return false;
	return theString.compare(0, theStart.size(), theStart) == 0;
}

static bool isSpace(int c)
{
	return isspace((unsigned char)c) != 0;
}

// 0x43b1f0: one character at a time; a stack keeps the state to return to after a tag, an entity, a comment or a
// CDATA section. Text goes to the handler when the next tag begins (or a CDATA section ends). Parsing stops at the
// first character after the root element's end, and an end tag's name is not checked against its start tag.
void engine::parseXml(XmlHandler& theHandler, InputStream* theStream)
{
	int aState = PROLOG;
	int aQuote = '"';
	std::string aText;
	std::string anEntity;
	std::string aTagName;
	std::string anAttrName;
	std::string anAttrValue;
	std::vector<int> aStack;
	XmlAttributes anAttributes;
	int aDepth = 0;
	theHandler.startDocument();
	// The line and the column are counted (CR, LF and CR LF each end a line) but never reported
	int aLine = 1;
	int aColumn = 0;
	bool aLastWasCR = false;
	int c = theStream->read();
	if (c != -1)
	{
		do
		{
			if (c == '\n')
			{
				if (aLastWasCR)
				{
					aLastWasCR = false;
					goto next;
				}
				aLine++;
				aColumn = 0;
			}
			else if (!aLastWasCR)
			{
				if (c == '\r')
				{
					aLastWasCR = true;
					c = '\n';
					aLine++;
					aColumn = 0;
				}
				else
					aColumn++;
			}
			else
				aLastWasCR = false;

			if (aState == DONE)
			{
				theHandler.endDocument();
				return;
			}
			if (aState == CONTENT)
			{
				if (c == '<')
				{
					aStack.push_back(aState);
					aState = TAG_START;
					if (!aText.empty())
					{
						theHandler.characters(aText);
						aText.erase(aText.begin(), aText.end());
					}
				}
				else if (c == '&')
				{
					aStack.push_back(aState);
					aState = ENTITY;
					anEntity.erase(anEntity.begin(), anEntity.end());
				}
				else
					aText.append(1, (char)c);
			}
			else if (aState == END_TAG)
			{
				if (c != '>')
					aText.append(1, (char)c);
				else
				{
					aState = popState(aStack);
					aTagName.assign(aText);
					aText.erase(aText.begin(), aText.end());
					aDepth--;
					if (aDepth == 0)
						aState = DONE;
					theHandler.endElement(aTagName);
				}
			}
			else if (aState == CDATA)
			{
				if (c == '>' && endsWith(aText, "]]"))
				{
					aText.resize(aText.size() - 2);
					theHandler.characters(aText);
					aText.erase(0);
					aState = popState(aStack);
				}
				else
					aText.append(1, (char)c);
			}
			else if (aState == COMMENT)
			{
				// The text holds "!-" and the comment; it ends at a '>' after "--"
				if (c == '>' && endsWith(aText, "--"))
				{
					aText.erase(aText.begin(), aText.end());
					aState = popState(aStack);
				}
				else
					aText.append(1, (char)c);
			}
			else if (aState == PROLOG)
			{
				if (c == '<')
				{
					aStack.push_back(CONTENT);
					aState = TAG_START;
				}
			}
			else if (aState == SKIP)
			{
				// a DOCTYPE (or a processing instruction) in the prolog returns to the prolog
				if (c == '>')
				{
					aState = popState(aStack);
					if (aState == CONTENT)
						aState = PROLOG;
				}
			}
			else if (aState == TAG_START)
			{
				int aReturnState = popState(aStack);
				if (c == '/')
				{
					aStack.push_back(aReturnState);
					aState = END_TAG;
				}
				else if (c == '?')
					aState = SKIP;  // the return state is not pushed back
				else
				{
					aStack.push_back(aReturnState);
					aState = TAG_NAME;
					aTagName.erase();
					anAttributes.clear();
					aText.append(1, (char)c);
				}
			}
			else if (aState == ENTITY)
			{
				if (c != ';')
					anEntity.append(1, (char)c);
				else
				{
					aState = popState(aStack);
					std::string aName(anEntity);
					anEntity.erase();
					if (aName == "lt")
						aText.append(1, '<');
					else if (aName == "gt")
						aText.append(1, '>');
					else if (aName == "amp")
						aText.append(1, '&');
					else if (aName == "quot")
						aText.append(1, '"');
					else if (aName == "apos")
						aText.append(1, '\'');
					else
					{
						if (!startsWith(aName, "#"))
							throw Exception("Unknown entity: &" + aName + ";");
						// decimal only
						long aCode = atol(aName.substr(1).c_str());
						aText.append(1, (char)aCode);
					}
				}
			}
			else if (aState == EMPTY_TAG)
			{
				if (aTagName.empty())
					aTagName.assign(aText);
				if (c != '>')
					throw Exception("Expected >\tfor\ttag: <" + aTagName + "/>");
				theHandler.startElement(aTagName, anAttributes);
				theHandler.endElement(aTagName);
				if (aDepth == 0)
				{
					theHandler.endDocument();
					return;
				}
				aText.erase();
				anAttributes.clear();
				aTagName.erase();
				aState = popState(aStack);
			}
			else if (aState == TAG_NAME)
			{
				if (c == '>')
				{
					if (aTagName.empty())
						aTagName.assign(aText);
					aText.erase();
					aDepth++;
					theHandler.startElement(aTagName, anAttributes);
					aTagName.erase();
					anAttributes.clear();
					aState = popState(aStack);
				}
				else if (c == '/')
					aState = EMPTY_TAG;
				else if (c == '-' && aText == "!-")
					aState = COMMENT;
				else if (c == '[' && aText == "![CDATA")
				{
					aState = CDATA;
					aText.erase();
				}
				else if (c == 'E' && aText == "!DOCTYP")
				{
					aText.erase();
					aState = SKIP;
				}
				else if (!isSpace(c))
					aText.append(1, (char)c);
				else
				{
					aTagName.assign(aText);
					aText.erase();
					aState = IN_TAG;
				}
			}
			else if (aState == ATTR_VALUE)
			{
				if (c == aQuote)
				{
					anAttrValue.assign(aText);
					aText.erase();
					anAttributes[anAttrName].assign(anAttrValue);
					aState = IN_TAG;
				}
				else if (std::string("\t\r\n").find((char)c) != std::string::npos)
				{
					// The original computes aText + '\t' and drops it, so tabs and line ends in a value are skipped
				}
				else if (c != '&')
					aText.append(1, (char)c);
				else
				{
					aStack.push_back(aState);
					aState = ENTITY;
					anEntity.erase();
				}
			}
			else if (aState == BEFORE_VALUE)
			{
				if (c == '"' || c == '\'')
				{
					aState = ATTR_VALUE;
					aQuote = c;
				}
				else if (!isSpace(c))
					throw Exception("Error in attribute\tprocessing");
			}
			else if (aState == ATTR_NAME)
			{
				if (!isSpace(c))
				{
					if (c != '=')
						aText.append(1, (char)c);
					else
					{
						anAttrName.assign(aText);
						aText.erase();
						aState = BEFORE_VALUE;
					}
				}
				else
				{
					anAttrName.assign(aText);
					aText.erase();
					aState = AFTER_ATTR_NAME;
				}
			}
			else if (aState == AFTER_ATTR_NAME)
			{
				if (c == '=')
					aState = BEFORE_VALUE;
				else if (!isSpace(c))
					throw Exception("Error in attribute\tprocessing.");
			}
			else if (aState == IN_TAG)
			{
				if (c == '>')
				{
					aState = popState(aStack);
					theHandler.startElement(aTagName, anAttributes);
					aDepth++;
					aTagName.erase();
					anAttributes.clear();
				}
				else if (c == '/')
					aState = EMPTY_TAG;
				else if (!isSpace(c))
				{
					aState = ATTR_NAME;
					aText.append(1, (char)c);
				}
			}
		next:
			c = theStream->read();
		} while (c != -1);
		if (aState == DONE)
		{
			theHandler.endDocument();
			return;
		}
	}
	(void)aLine;
	(void)aColumn;
	throw Exception("missing end tag");
}
