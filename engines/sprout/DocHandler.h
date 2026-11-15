#ifndef __SPROUT_DOCHANDLER_H__
#define __SPROUT_DOCHANDLER_H__

// The signal build's document handlers (the exe's AbstractDocHandler, constructor 0x479600 (pizzafrenzy), and
// BaseDocHandler, constructor 0x478de0 (pizzafrenzy); Iggle Pop's Mac build names them so). A game's data file is read
// by a stack of handlers: the one on top gets each element, and takes itself off the stack at the end of the element
// of its name, or hands the elements below to a handler it puts on the stack.

#include "Object.h"
#include "Xml.h"

#include <deque>
#include <string>

namespace engine
{

class DocHandlerStack;

// A pair of whole numbers or of floats an attribute holds: a point, or a range from its first to its second
struct IntPair
{
	int mFirst;
	int mSecond;
};

struct FloatPair
{
	float mFirst;
	float mSecond;
};

// The signal build's readers of the pairs a data file's attributes hold (each takes a copy of the attributes and
// gives the default for a missing attribute; a number that is not there keeps the default's)
// 0x4760a0 (pizzafrenzy): "x, y", spaces allowed
IntPair getIntPointAttribute(XmlAttributes theAttributes, const std::string& theName, IntPair theDefault);
// 0x475c50 (pizzafrenzy): "(a, b)", or one number for both
IntPair getIntRangeAttribute(XmlAttributes theAttributes, const std::string& theName, IntPair theDefault);
// 0x475b20 (pizzafrenzy): "(a, b)", or one number for both
FloatPair getFloatRangeAttribute(XmlAttributes theAttributes, const std::string& theName, FloatPair theDefault);

// A handler of a named element (0x478de0 (pizzafrenzy)), whose parent is the stack it was made for
class BaseDocHandler : public Object, public XmlHandler
{
public:
	BaseDocHandler(const std::string& theName, DocHandlerStack* theParent) : mParent(theParent), mName(theName) {}
	~BaseDocHandler() override {}  // 0x478dc0 (pizzafrenzy)

	const std::string& getName() const { return mName; }  // 0x478d20 (pizzafrenzy)
	DocHandlerStack* getParent() const { return mParent; }

private:
	DocHandlerStack* mParent;  // +0x14
	std::string mName;         // +0x18
};

// The stack (0x30 bytes, constructor 0x46c0e0 (pizzafrenzy)): the XML handler of the document, which gives each call
// to the handler on top
class DocHandlerStack : public Object, public XmlHandler
{
public:
	// 0x46c3e0 (pizzafrenzy): the handler goes on top
	void addHandler(BaseDocHandler* theHandler);

	// 0x46bf40 (pizzafrenzy)
	void startElement(const std::string& theName, const XmlAttributes& theAttributes) override;
	void endElement(const std::string& theName) override;  // 0x46bf80 (pizzafrenzy)
	void startDocument() override;                         // 0x46c060 (pizzafrenzy)
	void characters(const std::string& theText) override;  // 0x46c0a0 (pizzafrenzy)

private:
	std::deque<Ref<BaseDocHandler>> mHandlers;  // +0x14
};

} // namespace engine

#endif
