// StringParser.h: reads numbers, strings and lists out of the text of one line of a .cfg file (the original's
// Sexy::StringParser). A Peggle .cfg file is read by ConfigParser, which hands each "Key: value" line's value to one of these.

#ifndef __STRINGPARSER_H__
#define __STRINGPARSER_H__

#include <exception>
#include <string>
#include <vector>

namespace Sexy
{

// Thrown for an error in a .cfg file when the parser was made to throw (the original's Sexy::ParserException).
class ParserException : public std::exception
{
public:
	explicit ParserException(const std::string& theMessage) : mMessage(theMessage) {}
	const char* what() const noexcept override { return mMessage.c_str(); }

	std::string mMessage;
};

class StringParser
{
public:
	StringParser();

	void Set(const std::string& theText);			   // 0x4c0ed0
	void SkipWhitespace();							   // 0x4c0e40
	bool AtEnd() const { return mPos >= mText.size(); } // the original's pos < end
	bool CheckNextChar(char theChar);				   // 0x4c0ea0: after skipping whitespace, takes theChar if it is next
	bool EnsureNextChar(char theChar);				   // 0x4c0ff0
	int ReadInt();									   // 0x4c1270
	int ReadInt(int theDefault);					   // 0x4c1280: theDefault when there is no number
	// 0x4c12b0: a quoted string (with \", \\, \n, \t escapes and "a" + "b" joins) or bare text up to a comma
	// (or, when theStopAtSpace, up to the next space). theTrim cuts trailing white space off bare text.
	bool ReadString(std::string& theResult, bool theStopAtSpace, bool theTrim);
	std::string ReadString(bool theStopAtSpace, bool theTrim); // 0x4c1550
	bool ReadHTMLString(std::string& theResult);	   // 0x4c15c0: text for the game's formatted-text drawing
	bool ReadBool();								   // 0x4c1750
	bool ReadIntList(std::vector<int>& theResult, bool theClear); // 0x453880: comma separated
	bool ReadStringList(std::vector<std::string>& theResult, bool theClear); // 0x4653a0

	bool Error(const std::string& theMessage, bool theMayThrow = true); // 0x4c0fa0

	std::string mError;	  // the message of the last error ("" when there was none)
	bool mThrowErrors;

private:
	int ReadIntBase(bool theMayThrow);				   // 0x4c1090
	char CharAt(size_t thePos) const { return thePos < mText.size() ? mText[thePos] : '\0'; }

	std::string mText;
	size_t mPos;
};

} // namespace Sexy

#endif
