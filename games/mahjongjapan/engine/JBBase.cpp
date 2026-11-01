#include "JBBase.h"

#include <cstdlib>
#include <cstring>

namespace jb
{

bool IntersectRect(Rect& out, const Rect& a, const Rect& b)
{
	Rect r;
	r.left = a.left > b.left ? a.left : b.left;
	r.top = a.top > b.top ? a.top : b.top;
	r.right = a.right < b.right ? a.right : b.right;
	r.bottom = a.bottom < b.bottom ? a.bottom : b.bottom;
	if (IsRectEmpty(r))
	{
		out = Rect();
		return false;
	}
	out = r;
	return true;
}

bool UnionRect(Rect& out, const Rect& a, const Rect& b)
{
	if (IsRectEmpty(a))
	{
		if (IsRectEmpty(b))
		{
			out = Rect();
			return false;
		}
		out = b;
		return true;
	}
	if (IsRectEmpty(b))
	{
		out = a;
		return true;
	}
	Rect r;
	r.left = a.left < b.left ? a.left : b.left;
	r.top = a.top < b.top ? a.top : b.top;
	r.right = a.right > b.right ? a.right : b.right;
	r.bottom = a.bottom > b.bottom ? a.bottom : b.bottom;
	out = r;
	return true;
}

int MulDiv(int a, int b, int c)
{
	if (c == 0)
		return -1;
	long long p = (long long)a * b;
	// Win32 rounds half away from zero
	long long q;
	long long half = (c < 0 ? -(long long)c : c) / 2;
	if ((p < 0) != (c < 0))
		q = (p - (p < 0 ? half : -half)) / c;
	else
		q = (p + (p < 0 ? -half : half)) / c;
	if (q > 0x7fffffffLL || q < -0x80000000LL)
		return -1;
	return (int)q;
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

int StrICmp(const char* a, const char* b)
{
	while (true)
	{
		int ca = (uint8_t)*a, cb = (uint8_t)*b;
		if (ca >= 'A' && ca <= 'Z')
			ca += 'a' - 'A';
		if (cb >= 'A' && cb <= 'Z')
			cb += 'a' - 'A';
		if (ca != cb || ca == 0)
			return ca - cb;
		a++;
		b++;
	}
}

} // namespace jb
