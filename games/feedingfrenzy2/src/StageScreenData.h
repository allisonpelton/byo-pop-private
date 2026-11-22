#ifndef __FEEDINGFRENZY2_STAGESCREENDATA_H__
#define __FEEDINGFRENZY2_STAGESCREENDATA_H__

// What each stage's intro screen says (StageScreenData, 0x20 bytes, vtable 0x559df0 (feedingfrenzy2); its constructor
// 0x49ff80 (feedingfrenzy2)), the application's +0xbc, read from resource/Stages/stageScreenData.xml: a <screenData>
// for each stage id, with the layouts of its text and art and the text of its title, caption, two paragraphs (each
// with a header) and objective. Its handler (StageScreenData::ScreenDataDocHandler, 0x80 bytes, its constructor
// 0x49a790 (feedingfrenzy2)) gathers the text inside each of those elements into the entry (the original connects a
// slot for the element to its text signal and disconnects it at the end; the port keeps the field the text goes
// to).

#include "MasterDocHandler.h"
#include "Object.h"

#include <map>
#include <string>

// A <screenData> (0x110 bytes, an engine object)
class ScreenDataEntry : public engine::Object
{
public:
	std::string mTitle;            // +0x0c: "no title" until a <title>
	std::string mCaption;          // +0x28: "no caption"
	std::string mPara1;            // +0x44: "no para 1"
	std::string mPara2;            // +0x60: "no para 2"
	std::string mPara1Header;      // +0x7c: <para1 header>
	std::string mPara2Header;      // +0x98
	std::string mTextLayout;       // +0xb4: the screen layout of the text
	std::string mImageLayout;      // +0xd0: and of the art
	std::string mObjective;        // +0xec
};

class StageScreenData : public engine::Object
{
public:
	// 0x494a60 (feedingfrenzy2): the entries emptied and the file's read
	void load(const std::string& thePath);

	// The entry of the stage, null for none
	ScreenDataEntry* getEntry(const std::string& theId)
	{
		std::map<std::string, engine::Ref<ScreenDataEntry>>::iterator anIt = mEntries.find(theId);
		return anIt == mEntries.end() ? nullptr : anIt->second.get();
	}

private:
	friend class ScreenDataDocHandler;

	std::map<std::string, engine::Ref<ScreenDataEntry>> mEntries;  // +0x0c: by stage id
};

// StageScreenData::ScreenDataDocHandler
class ScreenDataDocHandler : public engine::SubDocHandler
{
public:
	explicit ScreenDataDocHandler(StageScreenData* theData) : mData(theData), mEntry(nullptr), mTextTarget(nullptr) {}

	void begin(engine::Params& theAttributes) override {}
	// 0x00 (0x45def0 (feedingfrenzy2)): a <screenData> made and kept under its id; a <title>, <caption>, <para1>,
	// <para2> or <objective> emptied, to gather its text
	void startElement(const std::string& theName, engine::Params& theAttributes) override;
	// 0x04 (0x468290 (feedingfrenzy2)): the text no longer gathered
	void endElement(const std::string& theName) override;
	// 0x10 (0x475090 (feedingfrenzy2)): the text added to the element's field (0x41b470, 0x41b490, 0x41b4b0,
	// 0x41bf30 and 0x41b4d0 (feedingfrenzy2))
	void text(const std::string& theText) override;

private:
	StageScreenData* mData;      // +0x64
	ScreenDataEntry* mEntry;     // +0x68: the <screenData> being read
	std::string* mTextTarget;    // port: the field the connected slot adds to, null for none
};

#endif
