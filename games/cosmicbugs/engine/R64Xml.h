#ifndef __COSMICBUGS_R64XML_H__
#define __COSMICBUGS_R64XML_H__

// Retro64's XML reader (0x42d2d0, 0x42d3c0, 0x401370, 0x4013f0): one tag at a time from a buffer, with its attributes
// and the text that follows it. The banks (.bbk), the game's data files (bugs.xml, level*.xml, local.xml) and the
// fonts are read with it.

namespace r64
{

// 0x11c bytes: the tag Xml_NextTag read last
struct XmlTag
{
	struct Attr
	{
		char name[0x100];
		char value[0x100];
	};

	Attr* attrs;        // +0x000  malloc'd, grown by doubling
	int count;          // +0x004
	int capacity;       // +0x008
	char name[0x100];   // +0x00c  "/" first for a closing tag
	const char* text;   // +0x10c  what follows an opening tag up to the next '<'
	int textLen;        // +0x110
	int closing;        // +0x114
	int empty;          // +0x118  the tag ends with "/>"

	XmlTag();
	~XmlTag();
	XmlTag(const XmlTag&) = delete;
	XmlTag& operator=(const XmlTag&) = delete;
};

// 0x42d3c0: reads the next tag at or after p into theTag; returns where reading stopped (after the '>', or for an
// opening tag at the '<' after its text). The buffer must end in a 0 (the original's buffers did not; the port
// stops at it).
const char* Xml_NextTag(const char* p, XmlTag* theTag);
// 0x42d2d0: the next token of text (whitespace, "//" and "\\" comments skip), at most limit characters (0: 9999)
// counted in *count across calls; returns where reading stopped
const char* Xml_ReadToken(const char* p, char* out, int limit, int* count);
// 0x401370: an attribute's value, or null
const char* Xml_GetString(const XmlTag* theTag, const char* theName);
// 0x4013f0: an attribute's value through atol, or 0
int Xml_GetInt(const XmlTag* theTag, const char* theName);
// 0x404f30: an attribute's value through atof, or 0
double Xml_GetFloat(const XmlTag* theTag, const char* theName);
// 0x41e970: forgets the tag and frees its attributes
void Xml_Reset(XmlTag* theTag);

} // namespace r64

#endif
