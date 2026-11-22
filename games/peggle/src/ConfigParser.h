// ConfigParser.h: reads Peggle's text configuration files (levels/stages.cfg, characters/characters.cfg, levels/trophy.cfg)
// as a series of events: a group opens ("Stage", "Character Bjorn" followed by "{"), a "Key: value" line, a group closes.
// The original's Sexy::ConfigParser. The original also reads "IncludeFile: name" lines and "[...]" lists, which only the
// level editor's files use; they are not part of this one.

#ifndef __CONFIGPARSER_H__
#define __CONFIGPARSER_H__

#include "StringParser.h"

#include <string>
#include <vector>

namespace Sexy
{

class ConfigParser
{
public:
	enum Event
	{
		EVENT_NONE = 0,
		EVENT_GROUP_BEGIN = 1, // a group's header and "{" were read; mGroups.back() is the group
		EVENT_KEY = 2,		   // a "Key: value" line; mKey is the key, GetValParser() reads the value
		EVENT_GROUP_END = 3	   // a "}"
	};

	struct Group
	{
		std::string mType;
		std::string mName;
	};

	explicit ConfigParser(bool theThrowErrors = true); // 0x4c03f0
	bool Open(const std::string& theFileName, bool theRequired); // 0x4c0510
	bool ReadNext();						   // 0x4c0db0
	StringParser& GetValParser();			   // 0x4be5a0
	bool KeyIs(const char* theKey) const;	   // 0x43e2f0: mKey is theKey, ignoring case
	// True when the innermost group's type is theType (ignoring case) and, when theDepth is not -1, the groups are theDepth deep.
	bool GroupIs(const char* theType, int theDepth = -1); // 0x4bf3d0
	bool Error(const std::string& theMessage, bool theUseKeyLine = false, bool theMayThrow = true); // 0x4bf020
	bool ErrorUnexpectedGroupType();		   // 0x4bf2a0
	bool ErrorUnexpectedKey();				   // 0x4bf350

	int mEvent;
	std::string mKey;
	std::string mValue;
	std::vector<Group> mGroups;

private:
	char NextChar();						   // read one character (advances the line count at a newline)
	char PeekChar() const;
	bool AtEnd() const { return mReadPos >= mText.size(); }
	void SkipLine();
	void SkipComment();
	void ReadToken(std::string& theToken, bool theToEndOfLine); // 0x4bf990
	bool DoReadNext();						   // 0x4c0a20

	std::string mFileName;
	std::string mText;
	size_t mReadPos;
	int mLine;
	int mEventLine;		   // the line the last event started on (0x50)
	StringParser mValParser;
	bool mThrowErrors;
};

} // namespace Sexy

#endif
