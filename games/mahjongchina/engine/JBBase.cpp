#include "JBBase.h"

#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace jb
{

bool IntersectRect(Rect& theOut, const Rect& theA, const Rect& theB)
{
	Rect aResult;
	aResult.left = theA.left > theB.left ? theA.left : theB.left;
	aResult.top = theA.top > theB.top ? theA.top : theB.top;
	aResult.right = theA.right < theB.right ? theA.right : theB.right;
	aResult.bottom = theA.bottom < theB.bottom ? theA.bottom : theB.bottom;
	if (IsRectEmpty(aResult))
	{
		theOut = Rect();
		return false;
	}
	theOut = aResult;
	return true;
}

bool UnionRect(Rect& theOut, const Rect& theA, const Rect& theB)
{
	if (IsRectEmpty(theA))
	{
		if (IsRectEmpty(theB))
		{
			theOut = Rect();
			return false;
		}
		theOut = theB;
		return true;
	}
	if (IsRectEmpty(theB))
	{
		theOut = theA;
		return true;
	}
	Rect aResult;
	aResult.left = theA.left < theB.left ? theA.left : theB.left;
	aResult.top = theA.top < theB.top ? theA.top : theB.top;
	aResult.right = theA.right > theB.right ? theA.right : theB.right;
	aResult.bottom = theA.bottom > theB.bottom ? theA.bottom : theB.bottom;
	theOut = aResult;
	return true;
}

int MulDiv(int theNumber, int theNumerator, int theDenominator)
{
	if (theDenominator == 0)
		return -1;
	long long aProduct = (long long)theNumber * theNumerator;
	// Win32 rounds half away from zero
	long long aQuotient;
	long long aHalf = (theDenominator < 0 ? -(long long)theDenominator : theDenominator) / 2;
	if ((aProduct < 0) != (theDenominator < 0))
		aQuotient = (aProduct - (aProduct < 0 ? aHalf : -aHalf)) / theDenominator;
	else
		aQuotient = (aProduct + (aProduct < 0 ? -aHalf : aHalf)) / theDenominator;
	if (aQuotient > 0x7fffffffLL || aQuotient < -0x80000000LL)
		return -1;
	return (int)aQuotient;
}

static uint32_t gCrtSeed = 1;

void CrtSrand(unsigned int theSeed)
{
	gCrtSeed = theSeed;
}

int CrtRand()
{
	gCrtSeed = gCrtSeed * 214013u + 2531011u;
	return (int)((gCrtSeed >> 16) & 0x7fff);
}

void ListAdd(ListNode*& theHead, void* theObj)
{
	ListNode* anOld = theHead;
	ListNode* aNode = new ListNode;
	aNode->mObj = theObj;
	aNode->mNext = theHead;
	aNode->mPrev = nullptr;
	theHead = aNode;
	if (anOld != nullptr)
		anOld->mPrev = aNode;
}

void ListRemove(ListNode*& theHead, void* theObj)
{
	ListNode* aNode = theHead;
	if (aNode == nullptr)
		return;
	while (aNode->mObj != theObj)
	{
		aNode = aNode->mNext;
		if (aNode == nullptr)
			return;
	}
	if (aNode->mPrev == nullptr)
		theHead = aNode->mNext;
	else
		aNode->mPrev->mNext = aNode->mNext;
	if (aNode->mNext != nullptr)
		aNode->mNext->mPrev = aNode->mPrev;
	delete aNode;
}

bool ListContains(ListNode* theHead, void* theObj)
{
	for (ListNode* aNode = theHead; aNode != nullptr; aNode = aNode->mNext)
		if (aNode->mObj == theObj)
			return true;
	return false;
}

void ListClear(ListNode*& theHead)
{
	while (theHead != nullptr)
	{
		ListNode* aNext = theHead->mNext;
		delete theHead;
		theHead = aNext;
	}
}

} // namespace jb

namespace jb
{

// 0x422a00
std::string Tokenize(const std::string& theText, char theSep, int& thePos)
{
	std::string aToken;
	int aLen = (int)theText.size();
	if (thePos < aLen)
	{
		size_t aFound = theText.find(theSep, thePos);
		int aNext;
		if (aFound == std::string::npos)
		{
			aToken = theText.substr(thePos);
			aNext = aLen;
		}
		else
		{
			aToken = theText.substr(thePos, (int)aFound - thePos);
			aNext = (int)aFound + 1;
		}
		thePos = aNext;
	}
	return aToken;
}

// 0x422ae0
int ParseNumber(const std::string& theText)
{
	const char* aText = theText.c_str();
	const char* anX = strpbrk(aText, "xX");
	char* anEnd;
	if (anX == nullptr)
		return (int)strtol(aText, &anEnd, 10);
	return (int)strtol(anX + 1, &anEnd, 16);
}

// 0x422b70
int FindOneOf(const std::string& theText, int theStart, const char* theSet)
{
	if (theStart < (int)theText.size() && -1 < theStart)
	{
		const char* aFound = strpbrk(theText.c_str() + theStart, theSet);
		if (aFound != nullptr)
			return (int)(aFound - theText.c_str());
	}
	return -1;
}

// 0x4234c0
void TrimRight(std::string& theText)
{
	int aCut = -1;
	for (int i = 0; i < (int)theText.size() && theText[i] != '\0'; i++)
	{
		if (!IsSpace(theText[i]))
			aCut = -1;
		else if (aCut == -1)
			aCut = i;
	}
	if (aCut != -1)
		theText.resize(aCut);
}

// 0x423510
void TrimLeft(std::string& theText)
{
	size_t i = 0;
	while (i < theText.size() && IsSpace(theText[i]))
		i++;
	theText.erase(0, i);
}

// 0x422cb0
std::string Mid(const std::string& theText, int theFirst)
{
	return Mid(theText, theFirst, (int)theText.size() - theFirst);
}

// 0x422ce0
std::string Mid(const std::string& theText, int theFirst, int theCount)
{
	if (theFirst < 0)
		theFirst = 0;
	if (theCount < 0)
		theCount = 0;
	int aLength = (int)theText.size();
	if (aLength < theFirst + theCount)
		theCount = aLength - theFirst;
	if (aLength < theFirst)
		theCount = 0;
	if (theCount == 0)
		return std::string();
	return theText.substr((size_t)theFirst, (size_t)theCount);
}

// 0x423700
void Delete(std::string& theText, int theIndex, int theCount)
{
	if (theIndex < 0)
		theIndex = 0;
	int aLength = (int)theText.size();
	if (0 < theCount && theIndex < aLength)
		theText.erase((size_t)theIndex, (size_t)theCount);
}

// 0x423660
void Insert(std::string& theText, int theIndex, char theChar)
{
	if (theIndex < 0)
		theIndex = 0;
	if ((int)theText.size() < theIndex)
		theIndex = (int)theText.size();
	theText.insert((size_t)theIndex, 1, theChar);
}

// 0x4231d0
static std::string FormatList(const char* theFormat, va_list theArgs)
{
	va_list aCopy;
	va_copy(aCopy, theArgs);
	int aLength = vsnprintf(nullptr, 0, theFormat, aCopy);
	va_end(aCopy);
	if (aLength <= 0)
		return std::string();
	std::string aText((size_t)aLength, '\0');
	vsnprintf(&aText[0], (size_t)aLength + 1, theFormat, theArgs);
	return aText;
}

std::string Format(const char* theFormat, ...)
{
	va_list anArgs;
	va_start(anArgs, theFormat);
	std::string aText = FormatList(theFormat, anArgs);
	va_end(anArgs);
	return aText;
}

// 0x423260
std::string FormatDll(int theId, ...)
{
	std::string aFormat = LoadDllString(theId);
	va_list anArgs;
	va_start(anArgs, theId);
	std::string aText = FormatList(aFormat.c_str(), anArgs);
	va_end(anArgs);
	return aText;
}

// 0x423450: hours, minutes, seconds
std::string FormatTime(int theSeconds)
{
	return Format("%d:%02d:%02d", theSeconds / 3600, (theSeconds % 3600) / 60, (theSeconds % 3600) % 60);
}

// 0x4232d0
std::string FormatNumber(int theNumber)
{
	std::string aDigits = Format("%d", theNumber < 0 ? -theNumber : theNumber);
	std::string aText;
	int aCount = (int)aDigits.size();
	for (int i = 0; i < aCount; i++)
	{
		if (i != 0 && (aCount - i) % 3 == 0)
			aText += ',';
		aText += aDigits[i];
	}
	if (theNumber < 0)
		aText = "-" + aText;
	return aText;
}

// 0x423760
void Insert(std::string& theText, int theIndex, const char* theInsert)
{
	if (theIndex < 0)
		theIndex = 0;
	int aCount = theInsert == nullptr ? 0 : (int)strlen(theInsert);
	if (0 < aCount)
	{
		if ((int)theText.size() < theIndex)
			theIndex = (int)theText.size();
		theText.insert((size_t)theIndex, theInsert, (size_t)aCount);
	}
}

int StrICmp(const char* theA, const char* theB)
{
	while (true)
	{
		int aCharA = (uint8_t)*theA, aCharB = (uint8_t)*theB;
		if (aCharA >= 'A' && aCharA <= 'Z')
			aCharA += 'a' - 'A';
		if (aCharB >= 'A' && aCharB <= 'Z')
			aCharB += 'a' - 'A';
		if (aCharA != aCharB || aCharA == 0)
			return aCharA - aCharB;
		theA++;
		theB++;
	}
}

bool FileExists(const char* thePath)
{
	FILE* aFile = fopen(thePath, "rb");
	if (aFile == nullptr)
		return false;
	fclose(aFile);
	return true;
}

void RemoveFile(const char* thePath)
{
	remove(thePath);
}

} // namespace jb
