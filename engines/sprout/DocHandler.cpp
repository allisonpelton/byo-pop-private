#include "DocHandler.h"

#include <cstdio>

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

// 0x4760a0 (pizzafrenzy)
IntPair engine::getIntPointAttribute(XmlAttributes theAttributes, const std::string& theName, IntPair theDefault)
{
	IntPair aPair = theDefault;
	XmlAttributes::iterator anIt = theAttributes.find(theName);
	if (anIt != theAttributes.end())
		sscanf(anIt->second.c_str(), " %d , %d ", &aPair.mFirst, &aPair.mSecond);
	return aPair;
}

// 0x475c50 (pizzafrenzy)
IntPair engine::getIntRangeAttribute(XmlAttributes theAttributes, const std::string& theName, IntPair theDefault)
{
	IntPair aPair = theDefault;
	XmlAttributes::iterator anIt = theAttributes.find(theName);
	if (anIt != theAttributes.end())
	{
		const char* aText = anIt->second.c_str();
		if (aText[0] == '(')
			sscanf(aText, "( %d , %d )", &aPair.mFirst, &aPair.mSecond);
		else
		{
			sscanf(aText, "%d", &aPair.mFirst);
			aPair.mSecond = aPair.mFirst;
		}
	}
	return aPair;
}

// 0x475b20 (pizzafrenzy)
FloatPair engine::getFloatRangeAttribute(XmlAttributes theAttributes, const std::string& theName,
                                         FloatPair theDefault)
{
	FloatPair aPair = theDefault;
	XmlAttributes::iterator anIt = theAttributes.find(theName);
	if (anIt != theAttributes.end())
	{
		const char* aText = anIt->second.c_str();
		if (aText[0] == '(')
			sscanf(aText, "( %f , %f )", &aPair.mFirst, &aPair.mSecond);
		else
		{
			sscanf(aText, "%f", &aPair.mFirst);
			aPair.mSecond = aPair.mFirst;
		}
	}
	return aPair;
}
