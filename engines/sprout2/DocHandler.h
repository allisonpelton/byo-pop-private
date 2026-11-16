#ifndef __SPROUT2_DOCHANDLER_H__
#define __SPROUT2_DOCHANDLER_H__

// What the XML parser calls as it reads a document (engine::DocHandler, vtable 0x5604a8 (feedingfrenzy2)), and the
// handler that ignores everything, for others to derive from (engine::AbstractDocHandler, vtable 0x5604d8
// (feedingfrenzy2)).

#include "Object.h"
#include "Params.h"

#include <string>

namespace engine
{

class DocHandler : public virtual Interface
{
public:
	// 0x00: an element opened, with its attributes
	virtual void startElement(const std::string& theName, Params& theAttributes) = 0;
	// 0x04: an element closed
	virtual void endElement(const std::string& theName) = 0;
	// 0x08 and 0x0c
	virtual void startDocument() = 0;
	virtual void endDocument() = 0;
	// 0x10: the text between tags, or a CDATA section
	virtual void text(const std::string& theText) = 0;
};

class AbstractDocHandler : public Object, public DocHandler
{
public:
	void startElement(const std::string& theName, Params& theAttributes) override {}
	void endElement(const std::string& theName) override {}
	void startDocument() override {}
	void endDocument() override {}
	void text(const std::string& theText) override {}
};

} // namespace engine

#endif
