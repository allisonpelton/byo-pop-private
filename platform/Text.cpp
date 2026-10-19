#include "Text.h"

namespace Platform
{

// One code point from theText at *thePos (moved past it). Returns false for a malformed sequence: a bad lead
// byte is skipped alone, a short sequence up to its last continuation byte (as SexyAppBase always did).
static bool NextCodePoint(const std::string& theText, size_t* thePos, uint32_t* theCodePoint)
{
	const unsigned char* p = (const unsigned char*) theText.data() + *thePos;
	size_t aLeft = theText.size() - *thePos;
	uint32_t aCodePoint;
	size_t aLength;
	if (*p < 0x80)
	{
		aCodePoint = *p;
		aLength = 1;
	}
	else if ((*p & 0xE0) == 0xC0)
	{
		aCodePoint = *p & 0x1F;
		aLength = 2;
	}
	else if ((*p & 0xF0) == 0xE0)
	{
		aCodePoint = *p & 0x0F;
		aLength = 3;
	}
	else if ((*p & 0xF8) == 0xF0)
	{
		aCodePoint = *p & 0x07;
		aLength = 4;
	}
	else
	{
		(*thePos)++;
		return false;
	}

	size_t i = 1;
	for (; (i < aLength) && (i < aLeft) && ((p[i] & 0xC0) == 0x80); i++)
		aCodePoint = (aCodePoint << 6) | (p[i] & 0x3F);
	*thePos += i;
	*theCodePoint = aCodePoint;
	return i == aLength;
}

std::vector<uint32_t> DecodeUtf8(const std::string& theText)
{
	std::vector<uint32_t> aResult;
	size_t aPos = 0;
	while (aPos < theText.size())
	{
		uint32_t aCodePoint = 0;
		aResult.push_back(NextCodePoint(theText, &aPos, &aCodePoint) ? aCodePoint : 0xFFFD);
	}
	return aResult;
}

void AppendUtf8(std::string* theText, uint32_t theCodePoint)
{
	if (theCodePoint < 0x80)
		*theText += (char) theCodePoint;
	else if (theCodePoint < 0x800)
	{
		*theText += (char) (0xC0 | (theCodePoint >> 6));
		*theText += (char) (0x80 | (theCodePoint & 0x3F));
	}
	else if (theCodePoint < 0x10000)
	{
		*theText += (char) (0xE0 | (theCodePoint >> 12));
		*theText += (char) (0x80 | ((theCodePoint >> 6) & 0x3F));
		*theText += (char) (0x80 | (theCodePoint & 0x3F));
	}
	else
	{
		*theText += (char) (0xF0 | (theCodePoint >> 18));
		*theText += (char) (0x80 | ((theCodePoint >> 12) & 0x3F));
		*theText += (char) (0x80 | ((theCodePoint >> 6) & 0x3F));
		*theText += (char) (0x80 | (theCodePoint & 0x3F));
	}
}

std::string Utf8ToLatin1(const std::string& theText, char theReplacement)
{
	std::string aResult;
	size_t aPos = 0;
	// Stops at a NUL like the C string version did
	while ((aPos < theText.size()) && (theText[aPos] != 0))
	{
		uint32_t aCodePoint = 0;
		bool isValid = NextCodePoint(theText, &aPos, &aCodePoint);
		if (isValid && (aCodePoint < 0x100))
			aResult += (char) aCodePoint;
		else if (theReplacement != 0)
			aResult += theReplacement;
	}
	return aResult;
}

std::string Latin1ToUtf8(const std::string& theText)
{
	std::string aResult;
	for (size_t i = 0; i < theText.size(); i++)
		AppendUtf8(&aResult, (unsigned char) theText[i]);
	return aResult;
}

}
