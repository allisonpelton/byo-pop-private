#ifndef __HAMMERHEADS_XMLLITE_H__
#define __HAMMERHEADS_XMLLITE_H__

// The little XML that the player files use. The original keeps them with TinyXML: a declaration, one <user> element whose
// attributes and child elements hold everything, and text only inside the flag elements. This is not a general XML library:
// an element tree, a reader for what such a file holds (elements, attributes, text, the declaration, comments) and a writer
// that lays a file out the way TinyXML does (four spaces a level, an element with only text on one line, an element with
// no children closed with " />").

#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace Sexy
{

class XmlNode
{
public:
	enum Type
	{
		TYPE_DOCUMENT,
		TYPE_ELEMENT,
		TYPE_TEXT
	};

	Type			mType;
	std::string		mValue;			// an element's name, or a text node's text
	std::vector<std::pair<std::string, std::string> > mAttributes;
	std::vector<std::unique_ptr<XmlNode> > mChildren;

public:
	explicit XmlNode(Type theType = TYPE_DOCUMENT, const std::string& theValue = std::string());

	XmlNode*		AddElement(const std::string& theName);
	XmlNode*		AddText(const std::string& theText);

	// An attribute that is already there takes the new value in its place; a new one goes at the end
	void			SetAttribute(const std::string& theName, const std::string& theValue);
	void			SetAttribute(const std::string& theName, int theValue);			// "%d"
	void			SetAttribute(const std::string& theName, double theValue);		// "%f"

	// Null when there is no such attribute
	const std::string*	FindAttribute(const std::string& theName) const;
	// Where there is no such attribute: 0 (the original's QueryIntAttribute, atol) and 0.0 (atof)
	int				GetIntAttribute(const std::string& theName) const;
	double			GetDoubleAttribute(const std::string& theName) const;

	const XmlNode*	FirstChildElement(const std::string& theName) const;
	bool			IsElement() const { return mType == TYPE_ELEMENT; }
	bool			IsElement(const char* theName) const { return mType == TYPE_ELEMENT && mValue == theName; }
	// The text of the element's first child when that is text; "" otherwise
	std::string		GetText() const;

	// A whole file of text into the document; false when it does not parse (the document then holds what was read)
	bool			Parse(const std::string& theText);
	// The document as the file's text: the declaration, then every child
	std::string		ToFileText() const;

private:
	void			Print(std::string& theOut, int theDepth) const;
};

}

#endif
