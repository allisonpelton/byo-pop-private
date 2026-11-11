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
	attrs = nullptr;
	count = 0;
	capacity = 0;
	name[0] = '\0';
	text = nullptr;
	textLen = 0;
	closing = 0;
	empty = 0;
}

// (0x41ee70 forgets the tag and frees its attributes: Xml_NextTag starts each tag afresh, and this frees them)
XmlTag::~XmlTag()
{
	std::free(attrs);
}

// 0x42d8a0. The names and values have no bound in the original; the port cuts them at 0xff characters and stops at
// the buffer's end.
const char* Xml_NextTag(const char* theCursor, XmlTag* theTag)
{
	theTag->count = 0;
	theTag->name[0] = '\0';
	theTag->text = nullptr;
	theTag->textLen = 0;
	theTag->closing = 0;
	theTag->empty = 0;
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
			theTag->name[aNameLength++] = aChar;
		aChar = *theCursor;
	}
	theTag->name[aNameLength] = '\0';
	if (theTag->name[0] == '/')
		theTag->closing = 1;
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
				theTag->empty = 1;
				theCursor++;
			}
			if (*theCursor == '>')
				theCursor++;
			if (theTag->closing == 0 && theTag->empty == 0)
			{
				theTag->text = theCursor;
				theTag->textLen = 0;
				aChar = *theCursor;
				while (aChar != '<' && aChar != '\0')
				{
					theTag->textLen = theTag->textLen + 1;
					theCursor++;
					aChar = *theCursor;
				}
			}
			return theCursor;
		}
		theTag->count = theTag->count + 1;
		int aCap = theTag->capacity;
		if (aCap < theTag->count)
		{
			if (aCap == 0)
				theTag->capacity = 1;
			else
				theTag->capacity = aCap * 2;
			XmlTag::Attr* aNew = (XmlTag::Attr*)std::malloc((size_t)theTag->capacity << 9);
			std::memset(aNew, 0, (size_t)theTag->capacity << 9);
			if (theTag->attrs != nullptr)
			{
				std::memcpy(aNew, theTag->attrs, (size_t)(theTag->capacity / 2) << 9);
				std::free(theTag->attrs);
			}
			theTag->attrs = aNew;
		}
		XmlTag::Attr& anAttr = theTag->attrs[theTag->count - 1];
		aNameLength = 0;
		aChar = *theCursor;
		while (aChar != ' ' && aChar != '=' && aChar != '\n' && aChar != '\r' && aChar != '\t' && aChar != '\0')
		{
			theCursor++;
			if (aNameLength < 0xff)
				anAttr.name[aNameLength++] = aChar;
			aChar = *theCursor;
		}
		anAttr.name[aNameLength] = '\0';
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
				anAttr.value[aNameLength++] = aChar;
			aChar = *theCursor;
		}
		anAttr.value[aNameLength] = '\0';
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

// 0x42d7b0
const char* Xml_ReadToken(const char* theCursor, char* out, int limit, int* count)
{
	int i = 0;
	int aLocal = 0;
	if (count == nullptr)
		count = &aLocal;
	if (limit == 0)
		limit = 9999;
	char aChar;
	while (aChar = *theCursor, (aChar == '\0' || aChar == '\n' || aChar == '\r' || (aChar == '/' && theCursor[1] == '/') || (aChar == '\\' && theCursor[1] == '\\') ||
					   aChar == '\t' || aChar == ' ') &&
			*count < limit)
	{
		if ((aChar == '/' && theCursor[1] == '/') || (aChar == '\\' && theCursor[1] == '\\'))
		{
			while (aChar != '\n' && aChar != '\r')
			{
				*count = *count + 1;
				theCursor++;
				aChar = *theCursor;
			}
		}
		theCursor++;
		*count = *count + 1;
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
			if (aStop || limit <= *count)
				break;
			out[i] = aChar;
			*count = *count + 1;
			aChar = theCursor[1];
			theCursor++;
			i++;
			if (aChar == '\0')
				break;
		}
	}
	out[i] = '\0';
	return theCursor;
}

// 0x401420
const char* Xml_GetString(const XmlTag* theTag, const char* theName)
{
	int i = 0;
	if (0 < theTag->count)
	{
		do
		{
			if (std::strcmp(theName, theTag->attrs[i].name) == 0)
				return theTag->attrs[i].value;
			i++;
		} while (i < theTag->count);
	}
	return nullptr;
}

// 0x4014a0
int Xml_GetInt(const XmlTag* theTag, const char* theName)
{
	int i = 0;
	if (0 < theTag->count)
	{
		do
		{
			if (std::strcmp(theName, theTag->attrs[i].name) == 0)
				return (int)std::atol(theTag->attrs[i].value);
			i++;
		} while (i < theTag->count);
	}
	return 0;
}

double Xml_GetFloat(const XmlTag* theTag, const char* theName)
{
	int i = 0;
	if (0 < theTag->count)
	{
		do
		{
			if (std::strcmp(theName, theTag->attrs[i].name) == 0)
				return std::atof(theTag->attrs[i].value);
			i++;
		} while (i < theTag->count);
	}
	return 0.0;
}

} // namespace r64
