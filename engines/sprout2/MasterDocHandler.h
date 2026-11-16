#ifndef __SPROUT2_MASTERDOCHANDLER_H__
#define __SPROUT2_MASTERDOCHANDLER_H__

// A document read by a stack of handlers, each for one kind of element and what is inside it (MasterDocHandler,
// vtable 0x55bc2c (feedingfrenzy2); its constructor 0x484290 and destructor 0x484000 (feedingfrenzy2)), and the
// handlers it passes the elements to (SubDocHandler, its constructor 0x476620 (feedingfrenzy2)). The top handler gets
// every element; when an element has the top handler's own name, the handler begins with it instead, and when it
// closes, the handler is taken off the stack. An <xmlinclude xmlpath="..."> element reads another document into the
// same handlers in its place.

#include "DocHandler.h"

#include <deque>

namespace engine
{

class MasterDocHandler;

class SubDocHandler : public AbstractDocHandler
{
public:
	SubDocHandler() : mMaster(nullptr) {}

	// 0x04 of its own table: the element of its own name opened
	virtual void begin(Params& theAttributes) = 0;

	// 0x4696a0 (feedingfrenzy2): the handler named for its element and put on top of the master's stack
	void attach(MasterDocHandler* theMaster, const std::string& theRootName);

	const std::string& getRootName() const { return mRootName; }

protected:
	MasterDocHandler* mMaster;   // +0x30
	std::string mRootName;       // +0x34
};

class MasterDocHandler : public AbstractDocHandler
{
public:
	MasterDocHandler() : mIncludeDepth(0), mInInclude(false) {}

	// 0x00 (0x455fd0 (feedingfrenzy2))
	void startElement(const std::string& theName, Params& theAttributes) override;
	// 0x04 (0x4525d0 (feedingfrenzy2))
	void endElement(const std::string& theName) override;
	// 0x08 (0x47bfc0 (feedingfrenzy2)): passed on for the outer document only
	void startDocument() override;
	// 0x10 (0x456120 (feedingfrenzy2))
	void text(const std::string& theText) override;

	// 0x46f5c0 (feedingfrenzy2)
	void push(SubDocHandler* theHandler) { mHandlers.push_back(theHandler); }

private:
	// 0x456d50 (feedingfrenzy2): whether the element was an include, read here
	bool handleInclude(const std::string& theName, Params& theAttributes);

	int mIncludeDepth;                          // +0x30
	bool mInInclude;                            // +0x34: from an include's start to its end
	std::deque<Ref<SubDocHandler>> mHandlers;   // +0x38
};

} // namespace engine

#endif
