#ifndef __ASTROPOP_XCFFILE_H__
#define __ASTROPOP_XCFFILE_H__

// XcfFile: the game's reader for its .xcf data files (data\descriptor.xcf, data\bonus<n>.xcf, data\level<n>.xcf,
// data\demo.xcf). The original class (constructor 0x427d80, Parse 0x42a5d0, sizeof 0x3c) parses
//     @SECTION { key = value  @CHILD { ... } }
// (lines starting with "//" are comments) into a tree of sections whose values are strings, keeps a cursor into the
// tree and answers path lookups. A path is "\SECTION\CHILD\Key" from the file's top, or "Key" inside the section
// the cursor is on; names compare without regard to case (both the path and the file's names are upper-cased).
// Port: the parse is a plain line reader. The original also takes the list of key names the game expects
// (AddKeyName 0x426f10; it is filled before every parse) and checks the file's .sig through the application's
// CheckSignature (the port does neither: the files are the player's own and unmodified play is what is compared).

#include <map>
#include <string>
#include <vector>

namespace Sexy
{

class XcfFile
{
public:
	enum
	{
		PARSE_OK = 0,
		PARSE_CANT_OPEN = 1,            // the original: 1 when the file does not open
		PARSE_SYNTAX = 2,               // port: any other fault of the file (the original has more codes; mLine says where)
		CURSOR_END = 3                  // NextSibling: no more sections here
	};

	struct Section
	{
		std::string							mName;      // as written in the file
		std::map<std::string, std::string>	mValues;    // upper-case key -> value
		std::vector<Section*>				mChildren;
		~Section();
	};

	struct CursorEntry
	{
		int			mIndex;     // the section's position among its siblings
		Section*	mSection;
	};

	std::vector<Section*>		mTop;           // +0x14 the file's top-level sections
	std::vector<CursorEntry>	mCursor;        // +0x04 the path of sections the cursor is in (the original keeps pointers to 8-byte entries)
	int							mLine;          // +0x38 the line a parse stopped at (-1 before one); the original keeps the line count at +0x38
	std::vector<std::string>	mKeyNames;      // +0x24 / +0x30 the names the game registered (port: kept, not checked)

	XcfFile();                                  // 0x427d80
	~XcfFile();                                 // 0x426df0

	void			AddKeyName(const std::string& theName);       // 0x426f10
	int				Parse(const std::string& theFileName);         // 0x42a5d0 (0 for success)

	void			ResetCursor();                                 // 0x425760 (clear), then the cursor is on the first top-level section
	bool			EnterFirstChild();                             // 0x4256e0
	int				NextSibling();                                 // 0x425600 (CURSOR_END at the last one, else 0)
	std::string		GetCurrentName();                              // 0x412c70
	int				FindSection(const std::string& theName);       // 0x426fc0 (moves along the siblings until the name matches)

	bool			HasKey(const std::string& thePath);            // 0x412d00
	std::string		GetString(const std::string& thePath);         // 0x413b10 (empty when missing)
	long			GetInt(const std::string& thePath);            // 0x4146e0 (atol of the string)
	float			GetFloat(const std::string& thePath);          // 0x414960 (strtod of the string, as float)

private:
	void			Clear();
	Section*		FindValueSection(const std::string& thePath, std::string* theKey);
};

}

#endif
