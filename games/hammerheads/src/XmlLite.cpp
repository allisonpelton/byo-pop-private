// port: a small XML reader and writer that stands in for TinyXML (not available); all of this file is the port's own
#include "XmlLite.h"

#include <cstdio>
#include <cstdlib>

using namespace Sexy;

namespace
{

// The five predefined entities, and a control character as a character reference (TinyXML's PutString)
std::string EncodeText(const std::string& theText)
{
	std::string aResult;
	for (size_t i = 0; i < theText.size(); i++)
	{
		unsigned char aChar = (unsigned char)theText[i];
		switch (aChar)
		{
		case '&': aResult += "&amp;"; break;
		case '<': aResult += "&lt;"; break;
		case '>': aResult += "&gt;"; break;
		case '"': aResult += "&quot;"; break;
		case '\'': aResult += "&apos;"; break;
		default:
			if (aChar < 32)
			{
				char aBuffer[16];
				snprintf(aBuffer, sizeof(aBuffer), "&#x%02X;", aChar);
				aResult += aBuffer;
			}
			else
				aResult += (char)aChar;
			break;
		}
	}
	return aResult;
}

void AppendUtf8(std::string& theOut, unsigned long theCode)
{
	if (theCode < 0x80)
		theOut += (char)theCode;
	else if (theCode < 0x800)
	{
		theOut += (char)(0xC0 | (theCode >> 6));
		theOut += (char)(0x80 | (theCode & 0x3F));
	}
	else if (theCode < 0x10000)
	{
		theOut += (char)(0xE0 | (theCode >> 12));
		theOut += (char)(0x80 | ((theCode >> 6) & 0x3F));
		theOut += (char)(0x80 | (theCode & 0x3F));
	}
	else
	{
		theOut += (char)(0xF0 | (theCode >> 18));
		theOut += (char)(0x80 | ((theCode >> 12) & 0x3F));
		theOut += (char)(0x80 | ((theCode >> 6) & 0x3F));
		theOut += (char)(0x80 | (theCode & 0x3F));
	}
}

std::string DecodeText(const std::string& theText)
{
	std::string aResult;
	for (size_t i = 0; i < theText.size(); i++)
	{
		if (theText[i] != '&')
		{
			aResult += theText[i];
			continue;
		}

		size_t anEnd = theText.find(';', i);
		if (anEnd == std::string::npos || anEnd - i > 10)
		{
			aResult += '&';
			continue;
		}

		std::string aName = theText.substr(i + 1, anEnd - i - 1);
		if (aName == "amp")
			aResult += '&';
		else if (aName == "lt")
			aResult += '<';
		else if (aName == "gt")
			aResult += '>';
		else if (aName == "quot")
			aResult += '"';
		else if (aName == "apos")
			aResult += '\'';
		else if (aName.size() > 1 && aName[0] == '#')
		{
			unsigned long aCode = (aName[1] == 'x' || aName[1] == 'X') ? strtoul(aName.c_str() + 2, nullptr, 16) : strtoul(aName.c_str() + 1, nullptr, 10);
			AppendUtf8(aResult, aCode);
		}
		else
		{
			aResult += '&';
			continue;
		}
		i = anEnd;
	}
	return aResult;
}

bool IsSpace(char theChar)
{
	return theChar == ' ' || theChar == '\t' || theChar == '\r' || theChar == '\n';
}

std::string Trim(const std::string& theText)
{
	size_t aStart = 0;
	size_t anEnd = theText.size();
	while (aStart < anEnd && IsSpace(theText[aStart]))
		aStart++;
	while (anEnd > aStart && IsSpace(theText[anEnd - 1]))
		anEnd--;
	return theText.substr(aStart, anEnd - aStart);
}

class XmlReader
{
public:
	explicit XmlReader(const std::string& theText) : mText(theText), mPos(0)
	{
		// a byte order mark
		if (mText.compare(0, 3, "\xEF\xBB\xBF") == 0)
			mPos = 3;
	}

	// The content of an element (or of the document, whose end is the end of the text) up to its closing tag
	bool ReadChildren(XmlNode* theParent)
	{
		while (mPos < mText.size())
		{
			if (mText[mPos] != '<')
			{
				size_t anEnd = mText.find('<', mPos);
				if (anEnd == std::string::npos)
					anEnd = mText.size();
				std::string aText = Trim(mText.substr(mPos, anEnd - mPos));
				mPos = anEnd;
				if (!aText.empty() && theParent->mType == XmlNode::TYPE_ELEMENT)
					theParent->AddText(DecodeText(aText));
			}
			else if (mText.compare(mPos, 2, "<?") == 0)
			{
				if (!SkipPast("?>"))
					return false;
			}
			else if (mText.compare(mPos, 4, "<!--") == 0)
			{
				if (!SkipPast("-->"))
					return false;
			}
			else if (mText.compare(mPos, 9, "<![CDATA[") == 0)
			{
				size_t anEnd = mText.find("]]>", mPos);
				if (anEnd == std::string::npos)
					return false;
				theParent->AddText(mText.substr(mPos + 9, anEnd - mPos - 9));
				mPos = anEnd + 3;
			}
			else if (mText.compare(mPos, 2, "<!") == 0)
			{
				if (!SkipPast(">"))
					return false;
			}
			else if (mText.compare(mPos, 2, "</") == 0)
			{
				mPos += 2;
				std::string aName = ReadName();
				SkipSpace();
				if (mPos >= mText.size() || mText[mPos] != '>')
					return false;
				mPos++;
				return theParent->mType == XmlNode::TYPE_ELEMENT && aName == theParent->mValue;
			}
			else if (!ReadElement(theParent))
				return false;
		}
		return theParent->mType == XmlNode::TYPE_DOCUMENT;
	}

private:
	const std::string& mText;
	size_t			mPos;

	bool SkipPast(const char* theEnd)
	{
		size_t anEnd = mText.find(theEnd, mPos);
		if (anEnd == std::string::npos)
			return false;
		mPos = anEnd + std::string(theEnd).size();
		return true;
	}

	void SkipSpace()
	{
		while (mPos < mText.size() && IsSpace(mText[mPos]))
			mPos++;
	}

	std::string ReadName()
	{
		size_t aStart = mPos;
		while (mPos < mText.size() && !IsSpace(mText[mPos]) && mText[mPos] != '/' && mText[mPos] != '>' && mText[mPos] != '=')
			mPos++;
		return mText.substr(aStart, mPos - aStart);
	}

	// At the "<" of an element's start tag
	bool ReadElement(XmlNode* theParent)
	{
		mPos++;
		std::string aName = ReadName();
		if (aName.empty())
			return false;
		XmlNode* anElement = theParent->AddElement(aName);

		for (;;)
		{
			SkipSpace();
			if (mPos >= mText.size())
				return false;

			char aChar = mText[mPos];
			if (aChar == '/')
			{
				mPos++;
				if (mPos >= mText.size() || mText[mPos] != '>')
					return false;
				mPos++;
				return true;
			}
			if (aChar == '>')
			{
				mPos++;
				return ReadChildren(anElement);
			}

			std::string anAttributeName = ReadName();
			SkipSpace();
			if (anAttributeName.empty() || mPos >= mText.size() || mText[mPos] != '=')
				return false;
			mPos++;
			SkipSpace();
			if (mPos >= mText.size() || (mText[mPos] != '"' && mText[mPos] != '\''))
				return false;
			char aQuote = mText[mPos];
			mPos++;
			size_t anEnd = mText.find(aQuote, mPos);
			if (anEnd == std::string::npos)
				return false;
			anElement->SetAttribute(anAttributeName, DecodeText(mText.substr(mPos, anEnd - mPos)));
			mPos = anEnd + 1;
		}
	}
};

}

XmlNode::XmlNode(Type theType, const std::string& theValue) :
	mType(theType),
	mValue(theValue)
{
}

XmlNode* XmlNode::AddElement(const std::string& theName)
{
	mChildren.push_back(std::unique_ptr<XmlNode>(new XmlNode(TYPE_ELEMENT, theName)));
	return mChildren.back().get();
}

XmlNode* XmlNode::AddText(const std::string& theText)
{
	mChildren.push_back(std::unique_ptr<XmlNode>(new XmlNode(TYPE_TEXT, theText)));
	return mChildren.back().get();
}

void XmlNode::SetAttribute(const std::string& theName, const std::string& theValue)
{
	for (size_t i = 0; i < mAttributes.size(); i++)
	{
		if (mAttributes[i].first == theName)
		{
			mAttributes[i].second = theValue;
			return;
		}
	}
	mAttributes.push_back(std::make_pair(theName, theValue));
}

void XmlNode::SetAttribute(const std::string& theName, int theValue)
{
	char aBuffer[64];
	snprintf(aBuffer, sizeof(aBuffer), "%d", theValue);
	SetAttribute(theName, std::string(aBuffer));
}

void XmlNode::SetAttribute(const std::string& theName, double theValue)
{
	char aBuffer[512];
	snprintf(aBuffer, sizeof(aBuffer), "%f", theValue);
	SetAttribute(theName, std::string(aBuffer));
}

const std::string* XmlNode::FindAttribute(const std::string& theName) const
{
	for (size_t i = 0; i < mAttributes.size(); i++)
	{
		if (mAttributes[i].first == theName)
			return &mAttributes[i].second;
	}
	return nullptr;
}

int XmlNode::GetIntAttribute(const std::string& theName) const
{
	const std::string* aValue = FindAttribute(theName);
	return aValue != nullptr ? (int)atol(aValue->c_str()) : 0;
}

double XmlNode::GetDoubleAttribute(const std::string& theName) const
{
	const std::string* aValue = FindAttribute(theName);
	return aValue != nullptr ? atof(aValue->c_str()) : 0.0;
}

const XmlNode* XmlNode::FirstChildElement(const std::string& theName) const
{
	for (size_t i = 0; i < mChildren.size(); i++)
	{
		if (mChildren[i]->mType == TYPE_ELEMENT && mChildren[i]->mValue == theName)
			return mChildren[i].get();
	}
	return nullptr;
}

std::string XmlNode::GetText() const
{
	if (!mChildren.empty() && mChildren[0]->mType == TYPE_TEXT)
		return mChildren[0]->mValue;
	return std::string();
}

bool XmlNode::Parse(const std::string& theText)
{
	XmlReader aReader(theText);
	return aReader.ReadChildren(this);
}

// The text a piece at a time: the pieces the original's writer wrote one by one (a tag's name, a space, an attribute, a run of
// indent, a newline)
void XmlNode::Print(std::vector<std::string>& thePieces, int theDepth) const
{
	if (mType == TYPE_TEXT)
	{
		thePieces.push_back(EncodeText(mValue));
		return;
	}

	for (int i = 0; i < theDepth; i++)
		thePieces.push_back("    ");
	thePieces.push_back("<" + mValue);
	for (size_t i = 0; i < mAttributes.size(); i++)
	{
		thePieces.push_back(" ");
		thePieces.push_back(mAttributes[i].first + "=\"" + EncodeText(mAttributes[i].second) + "\"");
	}

	if (mChildren.empty())
		thePieces.push_back(" />");
	else if (mChildren.size() == 1 && mChildren[0]->mType == TYPE_TEXT)
	{
		thePieces.push_back(">");
		mChildren[0]->Print(thePieces, theDepth + 1);
		thePieces.push_back("</" + mValue + ">");
	}
	else
	{
		thePieces.push_back(">");
		for (size_t i = 0; i < mChildren.size(); i++)
		{
			if (mChildren[i]->mType != TYPE_TEXT)
				thePieces.push_back("\n");
			mChildren[i]->Print(thePieces, theDepth + 1);
		}
		thePieces.push_back("\n");
		for (int i = 0; i < theDepth; i++)
			thePieces.push_back("    ");
		thePieces.push_back("</" + mValue + ">");
	}
}

static std::vector<std::string> DocumentPieces(const std::vector<std::unique_ptr<XmlNode>>& theChildren)
{
	std::vector<std::string> aPieces;
	aPieces.push_back("<?xml ");
	aPieces.push_back("version=\"1.0\" ");
	aPieces.push_back("encoding=\"UTF-8\" ");
	aPieces.push_back("?>");
	aPieces.push_back("\n");
	for (size_t i = 0; i < theChildren.size(); i++)
	{
		theChildren[i]->PrintPieces(aPieces);
		aPieces.push_back("\n");
	}
	return aPieces;
}

std::string XmlNode::ToFileText() const
{
	std::vector<std::string> aPieces = DocumentPieces(mChildren);
	std::string aText;
	for (size_t i = 0; i < aPieces.size(); i++)
		aText += aPieces[i];
	return aText;
}

std::string XmlNode::ToChunkedFileText() const
{
	std::vector<std::string> aPieces = DocumentPieces(mChildren);
	std::string aData;
	for (size_t i = 0; i < aPieces.size(); i++)
	{
		aData += (char) (aPieces[i].size() & 0xff);
		aData += (char) ((aPieces[i].size() >> 8) & 0xff);
		aData += aPieces[i];
	}
	return aData;
}

std::string XmlNode::FileTextOf(const std::string& theFileData)
{
	std::string aText;
	size_t aPos = 0;
	while (aPos + 2 <= theFileData.size())
	{
		size_t aLength = (unsigned char) theFileData[aPos] | ((size_t) (unsigned char) theFileData[aPos + 1] << 8);
		if (aPos + 2 + aLength > theFileData.size())
			return theFileData;
		aText.append(theFileData, aPos + 2, aLength);
		aPos += 2 + aLength;
	}
	// a stream of pieces ends exactly at the file's end and is text of the document; anything else is the text itself
	if (aPos != theFileData.size() || aText.compare(0, 5, "<?xml") != 0)
		return theFileData;
	return aText;
}
