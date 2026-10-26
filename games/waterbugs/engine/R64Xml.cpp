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

XmlTag::~XmlTag()
{
	std::free(attrs);
}

// 0x41ee70
void Xml_Reset(XmlTag* theTag)
{
	theTag->count = 0;
	theTag->name[0] = '\0';
	theTag->text = nullptr;
	theTag->textLen = 0;
	theTag->closing = 0;
	theTag->empty = 0;
	theTag->capacity = 0;
	if (theTag->attrs != nullptr)
	{
		std::free(theTag->attrs);
		theTag->attrs = nullptr;
	}
}

// 0x42d8a0. The names and values have no bound in the original; the port cuts them at 0xff characters and stops at
// the buffer's end.
const char* Xml_NextTag(const char* p, XmlTag* theTag)
{
	theTag->count = 0;
	theTag->name[0] = '\0';
	theTag->text = nullptr;
	theTag->textLen = 0;
	theTag->closing = 0;
	theTag->empty = 0;
	char c = *p;
	while (c != '<')
	{
		if (c == '\0')
			return p;
		p++;
		c = *p;
	}
	do
	{
		do
		{
			c = p[1];
			p++;
		} while (c == ' ');
	} while (c == '\n' || c == '\r' || c == '\t');
	int n = 0;
	while (c != ' ' && c != '>' && c != '\n' && c != '\r' && c != '\t' && c != '\0')
	{
		p++;
		if (n < 0xff)
			theTag->name[n++] = c;
		c = *p;
	}
	theTag->name[n] = '\0';
	if (theTag->name[0] == '/')
		theTag->closing = 1;
	for (; IsSpace(*p); p++)
	{
	}
	c = *p;
	while (true)
	{
		if (c == '/' || c == '>' || c == '\0')
		{
			if (c == '/')
			{
				theTag->empty = 1;
				p++;
			}
			if (*p == '>')
				p++;
			if (theTag->closing == 0 && theTag->empty == 0)
			{
				theTag->text = p;
				theTag->textLen = 0;
				c = *p;
				while (c != '<' && c != '\0')
				{
					theTag->textLen = theTag->textLen + 1;
					p++;
					c = *p;
				}
			}
			return p;
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
		n = 0;
		c = *p;
		while (c != ' ' && c != '=' && c != '\n' && c != '\r' && c != '\t' && c != '\0')
		{
			p++;
			if (n < 0xff)
				anAttr.name[n++] = c;
			c = *p;
		}
		anAttr.name[n] = '\0';
		for (; IsSpace(*p); p++)
		{
		}
		if (*p == '=')
			p++;
		c = *p;
		while (IsSpace(c))
		{
			p++;
			c = *p;
		}
		if (c == '"')
			p++;
		n = 0;
		c = *p;
		while (c != '"' && c != '\0')
		{
			p++;
			if (n < 0xff)
				anAttr.value[n++] = c;
			c = *p;
		}
		anAttr.value[n] = '\0';
		if (*p == '"')
			p++;
		c = *p;
		while (IsSpace(c))
		{
			p++;
			c = *p;
		}
	}
}

// 0x42d7b0
const char* Xml_ReadToken(const char* p, char* out, int limit, int* count)
{
	int i = 0;
	int aLocal = 0;
	if (count == nullptr)
		count = &aLocal;
	if (limit == 0)
		limit = 9999;
	char c;
	while (c = *p, (c == '\0' || c == '\n' || c == '\r' || (c == '/' && p[1] == '/') || (c == '\\' && p[1] == '\\') ||
					   c == '\t' || c == ' ') &&
			*count < limit)
	{
		if ((c == '/' && p[1] == '/') || (c == '\\' && p[1] == '\\'))
		{
			while (c != '\n' && c != '\r')
			{
				*count = *count + 1;
				p++;
				c = *p;
			}
		}
		p++;
		*count = *count + 1;
	}
	c = *p;
	if (c != '\0')
	{
		while (c != '\n' && c != '\r')
		{
			bool aStop;
			if (c == '/')
				aStop = p[1] == '/';
			else if (c == '\\')
				aStop = p[1] == '\\';
			else
			{
				if (c == '\t')
					break;
				aStop = c == ' ';
			}
			if (aStop || limit <= *count)
				break;
			out[i] = c;
			*count = *count + 1;
			c = p[1];
			p++;
			i++;
			if (c == '\0')
				break;
		}
	}
	out[i] = '\0';
	return p;
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
