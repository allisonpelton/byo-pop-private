#include "PopAnimNames.h"

#include <ctype.h>

using namespace Sexy;

static int Upper(char c)
{
	return toupper((unsigned char)c);
}

// Whether a and b are equal, ignoring case (the tests the exe makes with _stricmp and _strnicmp)
static bool EqualNoCase(const char* a, const char* b, size_t theLen)
{
	for (size_t i = 0; i < theLen; i++) {
		if (Upper(a[i]) != Upper(b[i]))
			return false;
	}
	return true;
}

// The matching half of 0x529540 and all of 0x529920. A pattern without a * is not handled here. On a match
// [*theStart, *theEnd) is the part of theName that the pattern's own text matched (all of it for a lone *).
static bool MatchWildcard(const std::string& theName, const std::string& thePattern, int* theStart, int* theEnd)
{
	int aNameLen = (int)theName.size();
	int aPatLen = (int)thePattern.size();
	if (thePattern[0] == '*') {
		if (aPatLen == 1) {
			*theStart = 0;
			*theEnd = aNameLen;
			return true;
		}
		if (thePattern[aPatLen - 1] == '*') {
			// "*text*": the first place the text is found
			int aLen = aPatLen - 2;
			for (int aStart = 0; aStart <= aNameLen - aLen; aStart++) {
				if (EqualNoCase(theName.c_str() + aStart, thePattern.c_str() + 1, aLen)) {
					*theStart = aStart;
					*theEnd = aStart + aLen;
					return true;
				}
			}
			return false;
		}
		// "*text"
		int aLen = aPatLen - 1;
		if (aNameLen < aLen || !EqualNoCase(theName.c_str() + aNameLen - aLen, thePattern.c_str() + 1, aLen))
			return false;
		*theStart = aNameLen - aLen;
		*theEnd = aNameLen;
		return true;
	}
	// "text*"
	int aLen = aPatLen - 1;
	if (aNameLen < aLen || !EqualNoCase(theName.c_str(), thePattern.c_str(), aLen))
		return false;
	*theStart = 0;
	*theEnd = aLen;
	return true;
}

static bool HasWildcard(const std::string& thePattern)
{
	return thePattern[0] == '*' || thePattern[thePattern.size() - 1] == '*';
}

// 0x529920
bool Sexy::PopAnimNameMatches(const std::string& theName, const std::string& thePattern)
{
	if (thePattern.empty())
		return false;
	if (!HasWildcard(thePattern))
		return theName.size() == thePattern.size() && EqualNoCase(theName.c_str(), thePattern.c_str(), theName.size());
	int aStart, anEnd;
	return MatchWildcard(theName, thePattern, &aStart, &anEnd);
}

// 0x5291c0: theRep with its * (at its start, its end or both) replaced by the parts of theName around
// [theStart, theEnd). A * at the start of theRep takes the part before, a * at its end the part after.
static std::string ExpandRep(const std::string& theRep, const std::string& theName, int theStart, int theEnd)
{
	if (theRep.empty())
		return std::string();
	std::string aBefore = theName.substr(0, theStart);
	std::string anAfter = theName.substr(theEnd);
	bool aStarAtStart = theRep[0] == '*';
	bool aStarAtEnd = theRep[theRep.size() - 1] == '*';
	if (aStarAtStart) {
		if (theRep.size() == 1)
			return aBefore + anAfter;
		if (aStarAtEnd)
			return aBefore + theRep.substr(1, theRep.size() - 2) + anAfter;
		return aBefore + theRep.substr(1);
	}
	if (aStarAtEnd)
		return theRep.substr(0, theRep.size() - 1) + anAfter;
	return theRep;
}

// 0x529540
bool Sexy::PopAnimRemapName(const std::string& theName, const std::string& thePattern, const std::string& theRep,
                            std::string* theResult)
{
	if (thePattern.empty())
		return false;
	if (HasWildcard(thePattern)) {
		int aStart, anEnd;
		if (!MatchWildcard(theName, thePattern, &aStart, &anEnd))
			return false;
		*theResult = ExpandRep(theRep, theName, aStart, anEnd);
		return true;
	}
	if (theName.size() != thePattern.size() || !EqualNoCase(theName.c_str(), thePattern.c_str(), theName.size()))
		return false;
	// the whole name matched: a * in the replacement stands for the name itself
	if (!theRep.empty() && theRep[0] == '*')
		*theResult = theName + theRep.substr(1);
	else if (!theRep.empty() && theRep[theRep.size() - 1] == '*')
		*theResult = theRep.substr(0, theRep.size() - 1) + theName;
	else
		*theResult = theRep;
	return true;
}
