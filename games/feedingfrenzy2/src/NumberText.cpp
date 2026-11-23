#include "NumberText.h"

#include <cstdio>

namespace
{

// A comma before each three digits from the right
std::string withCommas(const std::string& theDigits)
{
	std::string aText;
	int aCount = (int)theDigits.size();
	for (int i = 0; i < aCount; i++)
	{
		if (i > 0 && (aCount - i) % 3 == 0)
			aText += ',';
		aText += theDigits[i];
	}
	return aText;
}

} // namespace

std::string groupedNumber(int theNumber, bool isPadded)
{
	char aDigits[32];
	snprintf(aDigits, sizeof(aDigits), isPadded ? "%06u" : "%u", (unsigned int)theNumber);
	return withCommas(aDigits);
}

std::string stageClock(int theSeconds)
{
	char aText[32];
	snprintf(aText, sizeof(aText), "%d:%02d:%02d", theSeconds / 60 / 60, theSeconds / 60 % 60, theSeconds % 60);
	return aText;
}

std::string localeNumber(int theNumber)
{
	char aDigits[32];
	snprintf(aDigits, sizeof(aDigits), "%d", theNumber);
	if (aDigits[0] == '-')
		return "-" + withCommas(aDigits + 1);
	return withCommas(aDigits);
}
