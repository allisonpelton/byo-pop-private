#ifndef __FEEDINGFRENZY2_FUNFACTS_H__
#define __FEEDINGFRENZY2_FUNFACTS_H__

// The facts about fish shown between stages (FunFacts, vtable 0x55a6fc (feedingfrenzy2); its constructor 0x49d310
// (feedingfrenzy2)), the application's +0xc0, read from resource/funFacts.xml: each <fact>'s text, as the parser gives
// it (spaces kept). Its handler (FunFacts::FunFactDocHandler, 0x98 bytes, its constructor 0x498130
// (feedingfrenzy2)) gathers the text inside each <fact> (the original connects a slot to its text signal at the
// start and disconnects it at the end; the port keeps a flag).

#include "MasterDocHandler.h"
#include "Object.h"

#include <string>
#include <vector>

class FunFacts : public engine::Object
{
public:
	FunFacts() : mNextFact(0) {}

	// 0x4930d0 (feedingfrenzy2): the facts emptied and the file's read
	void load(const std::string& thePath);

	const std::vector<std::string>& getFacts() const { return mFacts; }
	// 0x51d4ae (feedingfrenzy2): a fact drawn (0x41ce50 (feedingfrenzy2), from the first to the last) and kept as the
	// next
	const std::string& drawFact();

private:
	friend class FunFactDocHandler;

	int mNextFact;                    // +0x0c: the fact drawn last, 0 after load (the debug buttons' readers not
	                                  // translated, as nothing reaches them)
	std::vector<std::string> mFacts;  // +0x10
};

// FunFacts::FunFactDocHandler
class FunFactDocHandler : public engine::SubDocHandler
{
public:
	explicit FunFactDocHandler(FunFacts* theFacts) : mInFact(false), mFacts(theFacts) {}

	void begin(engine::Params& theAttributes) override {}
	// 0x00 (0x4776f0 (feedingfrenzy2)): a <fact>'s text begun
	void startElement(const std::string& theName, engine::Params& theAttributes) override;
	// 0x04 (0x4750a0 (feedingfrenzy2)): a <fact>'s text added to the facts
	void endElement(const std::string& theName) override;
	// 0x10 (0x475090 (feedingfrenzy2)): inside a <fact>, the text added to it (0x417f60 (feedingfrenzy2))
	void text(const std::string& theText) override;

private:
	std::string mFact;     // +0x64
	bool mInFact;          // port: the original's text slot connected
	FunFacts* mFacts;      // +0x8c
};

#endif
