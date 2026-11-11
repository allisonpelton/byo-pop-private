#include "R64Xml.h"

#include <cstdlib>
#include <cstring>

namespace r64
{

namespace
{

inline bool IsSpace(char c)
{
	return c == ' ' || c == '\n' || c == '\r' || c == '\t';
}

} // namespace

XmlTag::XmlTag()
{
	mAttrs = nullptr;
	mCount = 0;
	mCapacity = 0;
	mName[0] = '\0';
	mText = nullptr;
	mTextLen = 0;
	mClosing = 0;
	mEmpty = 0;
}

// (frees the attributes, as Xml_Reset does besides forgetting the tag: Xml_NextTag starts each tag afresh)
XmlTag::~XmlTag()
{
	std::free(mAttrs);
}

// 0x41e970 (cosmicbugs), 0x41ee70 (waterbugs): forgets the tag and frees its attributes
void Xml_Reset(XmlTag* theTag)
{
	theTag->mCount = 0;
	theTag->mName[0] = '\0';
	theTag->mText = nullptr;
	theTag->mTextLen = 0;
	theTag->mClosing = 0;
	theTag->mEmpty = 0;
	theTag->mCapacity = 0;
	if (theTag->mAttrs != nullptr)
	{
		std::free(theTag->mAttrs);
		theTag->mAttrs = nullptr;
	}
}

// 0x42d3c0 (cosmicbugs), 0x42d8a0 (waterbugs). The names and values have no bound in the original; the port cuts them
// at 0xff characters and stops at the buffer's end.
const char* Xml_NextTag(const char* theCursor, XmlTag* theTag)
{
	theTag->mCount = 0;
	theTag->mName[0] = '\0';
	theTag->mText = nullptr;
	theTag->mTextLen = 0;
	theTag->mClosing = 0;
	theTag->mEmpty = 0;
	char aChar = *theCursor;
	while (aChar != '<')
	{
		if (aChar == '\0')
			return theCursor;
		theCursor++;
		aChar = *theCursor;
	}
	do
	{
		do
		{
			aChar = theCursor[1];
			theCursor++;
		} while (aChar == ' ');
	} while (aChar == '\n' || aChar == '\r' || aChar == '\t');
	int aNameLength = 0;
	while (aChar != ' ' && aChar != '>' && aChar != '\n' && aChar != '\r' && aChar != '\t' && aChar != '\0')
	{
		theCursor++;
		if (aNameLength < 0xff)
			theTag->mName[aNameLength++] = aChar;
		aChar = *theCursor;
	}
	theTag->mName[aNameLength] = '\0';
	if (theTag->mName[0] == '/')
		theTag->mClosing = 1;
	for (; IsSpace(*theCursor); theCursor++)
	{
	}
	aChar = *theCursor;
	while (true)
	{
		if (aChar == '/' || aChar == '>' || aChar == '\0')
		{
			if (aChar == '/')
			{
				theTag->mEmpty = 1;
				theCursor++;
			}
			if (*theCursor == '>')
				theCursor++;
			if (theTag->mClosing == 0 && theTag->mEmpty == 0)
			{
				theTag->mText = theCursor;
				theTag->mTextLen = 0;
				aChar = *theCursor;
				while (aChar != '<' && aChar != '\0')
				{
					theTag->mTextLen = theTag->mTextLen + 1;
					theCursor++;
					aChar = *theCursor;
				}
			}
			return theCursor;
		}
		theTag->mCount = theTag->mCount + 1;
		int aCap = theTag->mCapacity;
		if (aCap < theTag->mCount)
		{
			if (aCap == 0)
				theTag->mCapacity = 1;
			else
				theTag->mCapacity = aCap * 2;
			XmlTag::Attr* aNew = (XmlTag::Attr*)std::malloc((size_t)theTag->mCapacity << 9);
			std::memset(aNew, 0, (size_t)theTag->mCapacity << 9);
			if (theTag->mAttrs != nullptr)
			{
				std::memcpy(aNew, theTag->mAttrs, (size_t)(theTag->mCapacity / 2) << 9);
				std::free(theTag->mAttrs);
			}
			theTag->mAttrs = aNew;
		}
		XmlTag::Attr& anAttr = theTag->mAttrs[theTag->mCount - 1];
		aNameLength = 0;
		aChar = *theCursor;
		while (aChar != ' ' && aChar != '=' && aChar != '\n' && aChar != '\r' && aChar != '\t' && aChar != '\0')
		{
			theCursor++;
			if (aNameLength < 0xff)
				anAttr.mName[aNameLength++] = aChar;
			aChar = *theCursor;
		}
		anAttr.mName[aNameLength] = '\0';
		for (; IsSpace(*theCursor); theCursor++)
		{
		}
		if (*theCursor == '=')
			theCursor++;
		aChar = *theCursor;
		while (IsSpace(aChar))
		{
			theCursor++;
			aChar = *theCursor;
		}
		if (aChar == '"')
			theCursor++;
		aNameLength = 0;
		aChar = *theCursor;
		while (aChar != '"' && aChar != '\0')
		{
			theCursor++;
			if (aNameLength < 0xff)
				anAttr.mValue[aNameLength++] = aChar;
			aChar = *theCursor;
		}
		anAttr.mValue[aNameLength] = '\0';
		if (*theCursor == '"')
			theCursor++;
		aChar = *theCursor;
		while (IsSpace(aChar))
		{
			theCursor++;
			aChar = *theCursor;
		}
	}
}

// 0x42d2d0 (cosmicbugs), 0x42d7b0 (waterbugs)
const char* Xml_ReadToken(const char* theCursor, char* theOut, int theLimit, int* theCount)
{
	int i = 0;
	int aLocal = 0;
	if (theCount == nullptr)
		theCount = &aLocal;
	if (theLimit == 0)
		theLimit = 9999;
	char aChar;
	while (aChar = *theCursor,
		(aChar == '\0' || aChar == '\n' || aChar == '\r' || (aChar == '/' && theCursor[1] == '/') ||
		(aChar == '\\' && theCursor[1] == '\\') || aChar == '\t' || aChar == ' ') && *theCount < theLimit)
	{
		if ((aChar == '/' && theCursor[1] == '/') || (aChar == '\\' && theCursor[1] == '\\'))
		{
			while (aChar != '\n' && aChar != '\r')
			{
				*theCount = *theCount + 1;
				theCursor++;
				aChar = *theCursor;
			}
		}
		theCursor++;
		*theCount = *theCount + 1;
	}
	aChar = *theCursor;
	if (aChar != '\0')
	{
		while (aChar != '\n' && aChar != '\r')
		{
			bool aStop;
			if (aChar == '/')
				aStop = theCursor[1] == '/';
			else if (aChar == '\\')
				aStop = theCursor[1] == '\\';
			else
			{
				if (aChar == '\t')
					break;
				aStop = aChar == ' ';
			}
			if (aStop || theLimit <= *theCount)
				break;
			theOut[i] = aChar;
			*theCount = *theCount + 1;
			aChar = theCursor[1];
			theCursor++;
			i++;
			if (aChar == '\0')
				break;
		}
	}
	theOut[i] = '\0';
	return theCursor;
}

// 0x401370 (cosmicbugs), 0x401420 (waterbugs)
const char* Xml_GetString(const XmlTag* theTag, const char* theName)
{
	int i = 0;
	if (0 < theTag->mCount)
	{
		do
		{
			if (std::strcmp(theName, theTag->mAttrs[i].mName) == 0)
				return theTag->mAttrs[i].mValue;
			i++;
		} while (i < theTag->mCount);
	}
	return nullptr;
}

// 0x4013f0 (cosmicbugs), 0x4014a0 (waterbugs)
int Xml_GetInt(const XmlTag* theTag, const char* theName)
{
	int i = 0;
	if (0 < theTag->mCount)
	{
		do
		{
			if (std::strcmp(theName, theTag->mAttrs[i].mName) == 0)
				return (int)std::atol(theTag->mAttrs[i].mValue);
			i++;
		} while (i < theTag->mCount);
	}
	return 0;
}

double Xml_GetFloat(const XmlTag* theTag, const char* theName)
{
	int i = 0;
	if (0 < theTag->mCount)
	{
		do
		{
			if (std::strcmp(theName, theTag->mAttrs[i].mName) == 0)
				return std::atof(theTag->mAttrs[i].mValue);
			i++;
		} while (i < theTag->mCount);
	}
	return 0.0;
}

} // namespace r64
