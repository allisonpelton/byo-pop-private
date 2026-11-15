#include "DocHandler.h"

using namespace engine;

// 0x46c3e0 (pizzafrenzy)
void DocHandlerStack::addHandler(BaseDocHandler* theHandler)
{
	mHandlers.push_back(theHandler);
}

// 0x46bf40 (pizzafrenzy)
void DocHandlerStack::startElement(const std::string& theName, const XmlAttributes& theAttributes)
{
	if (mHandlers.empty())
		return;
	Ref<BaseDocHandler> aTop = mHandlers.back();
	aTop->startElement(theName, theAttributes);
}

// 0x46bf80 (pizzafrenzy): the handler whose element ends is told, and goes (0x46be20 (pizzafrenzy)); the last of all
// is told that the document ends first
void DocHandlerStack::endElement(const std::string& theName)
{
	if (mHandlers.empty())
		return;
	Ref<BaseDocHandler> aTop = mHandlers.back();
	aTop->endElement(theName);
	if (theName == aTop->getName())
	{
		if (mHandlers.size() == 1)
			aTop->endDocument();
		mHandlers.pop_back();
	}
}

// 0x46c060 (pizzafrenzy)
void DocHandlerStack::startDocument()
{
	if (mHandlers.empty())
		return;
	Ref<BaseDocHandler> aTop = mHandlers.back();
	aTop->startDocument();
}

// 0x46c0a0 (pizzafrenzy)
void DocHandlerStack::characters(const std::string& theText)
{
	if (mHandlers.empty())
		return;
	Ref<BaseDocHandler> aTop = mHandlers.back();
	aTop->characters(theText);
}
