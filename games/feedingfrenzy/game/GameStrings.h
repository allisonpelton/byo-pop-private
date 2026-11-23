#ifndef __FEEDINGFRENZY_GAMESTRINGS_H__
#define __FEEDINGFRENZY_GAMESTRINGS_H__

// The game's string table (config\gameStrings.xml; constructor 0x43d980, vtable 0x4a76b0, load 0x43d260, lookup
// 0x43d3c0): the "string" elements' name and text, kept in a map. A name that is not there gives the text
// "!!!MISSING STRING RESOURCE!!!". The formatted texts (0x42ce60) are made by the system's FormatMessage with a text's
// arguments (%1!d! and %2!s! are the first as a number and the second as a text, %! is a "!").

#include <initializer_list>
#include <map>
#include <string>

namespace feedingfrenzy
{

// An argument of a formatted text: a number or a text
struct FormatArg
{
	FormatArg(int theNumber) : mIsText(false), mNumber(theNumber) {}
	FormatArg(const char* theText) : mIsText(true), mNumber(0), mText(theText) {}
	FormatArg(const std::string& theText) : mIsText(true), mNumber(0), mText(theText) {}

	bool mIsText;
	int mNumber;
	std::string mText;
};

class GameStrings
{
public:
	// 0x43d260: the file is read through the application (the archive)
	void load(const std::string& theFile);
	// 0x43d3c0
	std::string get(const std::string& theName) const;
	// 0x42ce60: the named text with its arguments put in; a text with no arguments is returned as it is
	std::string format(const std::string& theName, std::initializer_list<FormatArg> theArgs) const;

	void add(const std::string& theName, const std::string& theText) { mStrings[theName] = theText; }

private:
	std::map<std::string, std::string> mStrings;  // +0x0c
};

} // namespace feedingfrenzy

#endif
