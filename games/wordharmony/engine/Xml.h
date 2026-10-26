#ifndef __WORDHARMONY_ENGINE_XML_H__
#define __WORDHARMONY_ENGINE_XML_H__

// The engine's XML reader: a SAX parser (0x43b1f0) that reads a stream one character at a time and calls a handler.
// Screens, fonts, particle systems and the game's own data files are read through it.

#include "Object.h"
#include <map>
#include <string>

namespace engine
{

class InputStream;

// The attributes of an element: a map from name to value (a repeated attribute keeps its last value)
class XmlAttributes : public std::map<std::string, std::string>
{
public:
	// 0x43eec0
	std::string getString(const std::string& theName, const std::string& theDefault) const;
	// 0x43ef90: atol of the value
	long getInt(const std::string& theName, long theDefault) const;
};

// The handler interface (vtable 0x484d2c; the base's slots, 0x484e0c, do nothing)
class XmlHandler
{
public:
	virtual ~XmlHandler() {}
	virtual void startElement(const std::string& theName, const XmlAttributes& theAttributes) {}  // slot 0x00
	virtual void endElement(const std::string& theName) {}                                       // slot 0x04
	virtual void startDocument() {}                                                              // slot 0x08
	virtual void endDocument() {}                                                                // slot 0x0c
	virtual void characters(const std::string& theText) {}                                       // slot 0x10
};

// 0x43b1f0: parses a document; throws an Exception on an error
void parseXml(XmlHandler& theHandler, InputStream* theStream);

} // namespace engine

#endif
