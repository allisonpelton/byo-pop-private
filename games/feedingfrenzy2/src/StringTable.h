#ifndef __FEEDINGFRENZY2_STRINGTABLE_H__
#define __FEEDINGFRENZY2_STRINGTABLE_H__

// The game's strings by name (StringTable, 0x24 bytes, vtable 0x55d674 (feedingfrenzy2); its constructor 0x49bb30
// (feedingfrenzy2)), the application's +0x4c, read from resource/baseStrings.xml: each <string>'s name and string, the
// first of a name kept. The original looks a name it does not have up in a parent table (+0x18), which nothing in
// feedingfrenzy2 sets, so a missing name gives the empty string.

#include "DocHandler.h"
#include "Object.h"

#include <map>
#include <string>

class StringTable : public engine::Object
{
public:
	// 0x4952c0 (feedingfrenzy2): emptied, then the file read
	void load(const std::string& thePath);
	// 0x04 (0x49bbb0 (feedingfrenzy2))
	virtual void clear() { mStrings.clear(); }
	// 0x08 (0x47d690 (feedingfrenzy2)): a name already there keeps its string
	virtual void addString(const std::string& theName, const std::string& theString)
	{
		mStrings.insert(std::make_pair(theName, theString));
	}
	// 0x46dc10 (feedingfrenzy2): the empty string (0x5ad1d8 (feedingfrenzy2)) for a name the table does not have
	const std::string& getString(const std::string& theName);

private:
	std::map<std::string, std::string> mStrings;  // +0x0c
};

// StringTable::StringTableDocHandler (its constructor 0x4a1710 (feedingfrenzy2))
class StringTableDocHandler : public engine::AbstractDocHandler
{
public:
	explicit StringTableDocHandler(StringTable* theTable) : mTable(theTable) {}

	// 0x00 (0x4278d0 (feedingfrenzy2)): each <string>, wherever it is, its name and string (each "" by default)
	void startElement(const std::string& theName, engine::Params& theAttributes) override;

private:
	StringTable* mTable;  // +0x24
};

#endif
