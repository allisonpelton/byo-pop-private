// The engine's string helpers and small utilities (see Str.h).

#include "Str.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <ctime>

namespace bonnies
{

// 0x41d930
Str substr(const Str& theStr, int theBegin, int theEnd)
{
	Str aResult;
	int i = 0;
	if (0 < theEnd - theBegin)
	{
		do
		{
			aResult += theStr[i + theBegin];
			i = i + 1;
		} while (i < theEnd - theBegin);
	}
	return aResult;
}

// 0x41da80
Str floatToStr(float theValue, int theDecimals)
{
	char aFormat[52];
	char aText[52];
	std::snprintf(aFormat, sizeof(aFormat), "%%5.%df", theDecimals);
	std::snprintf(aText, sizeof(aText), aFormat, (double)theValue);
	return aText;
}

// 0x41db50
Str intToStr(int theValue)
{
	char aPart[31];
	Str aText;
	bool aMillions = false;
	if (theValue < 0)
	{
		aText += "-";
		theValue = -theValue;
	}
	if (999999 < theValue)
	{
		std::snprintf(aPart, sizeof(aPart), "%d,", theValue / 1000000);
		theValue = theValue % 1000000;
		aText += aPart;
		aMillions = true;
	}
	const char* aFormat;
	if (theValue < 1000)
	{
		if (!aMillions)
		{
			std::snprintf(aPart, sizeof(aPart), "%d", theValue);
			aText += aPart;
			return aText;
		}
		aFormat = "%03d,";
	}
	else
		aFormat = aMillions ? "%03d," : "%d,";
	std::snprintf(aPart, sizeof(aPart), aFormat, theValue / 1000);
	theValue = theValue % 1000;
	aText += aPart;
	std::snprintf(aPart, sizeof(aPart), "%03d", theValue);
	aText += aPart;
	return aText;
}

// 0x41dd50
long strToInt(const Str& theStr)
{
	return std::atol(theStr.c_str());
}

// 0x41ddc0
float strToFloat(const Str& theStr)
{
	return (float)std::atof(theStr.c_str());
}

// 0x41de30
Str trim(const Str& theStr, bool theLines)
{
	int aLen = (int)theStr.size();
	int aFirst = -1;
	for (int i = 0; i < aLen; i++)
	{
		char c = theStr[i];
		if (c != ' ' && c != '\t')
		{
			aFirst = i;
			if (!theLines || (c != '\r' && c != '\n'))
				break;
		}
		aFirst = -1;
	}
	int aLast = -1;
	for (int j = aLen - 1; j >= 0; j--)
	{
		char c = theStr[j];
		if (c != ' ' && c != '\t' && (!theLines || (c != '\r' && c != '\n')))
		{
			aLast = j;
			break;
		}
	}
	if (aFirst == -1)
		return Str();
	return substr(theStr, aFirst, aLast + 1);
}

// 0x41df30
void appendFloat(Str& theStr, float theValue)
{
	char aText[100];
	std::snprintf(aText, sizeof(aText), "%2.1f", (double)theValue);
	theStr += aText;
}

// 0x41dfb0
bool equalsNoCase(const Str& theA, const Str& theB)
{
	int i = (int)theA.size();
	if (i != (int)theB.size())
		return false;
	do
	{
		i = i - 1;
		if (i < 0)
			return true;
	} while (toUpper((unsigned char)theB[i]) == toUpper((unsigned char)theA[i]));
	return false;
}

// 0x41e0b0. The first character is compared with its upper and lower case as signed chars, so a needle that starts with
// a character above 0x7f is never found.
bool containsNoCase(const Str& theHay, const Str& theNeedle)
{
	int aHayLen = (int)theHay.size();
	int aNeedleLen = (int)theNeedle.size();
	if (aHayLen < 1 || aNeedleLen < 1 || aHayLen < aNeedleLen)
		return false;
	signed char anUpper = (signed char)toUpper(at(theNeedle, 0));
	signed char aLower = (signed char)toLower(at(theNeedle, 0));
	int i = 0;
	if (-1 < aHayLen - aNeedleLen)
	{
		do
		{
			unsigned int c = at(theHay, i);
			if (c == (unsigned int)(int)anUpper || c == (unsigned int)(int)aLower)
			{
				if (equalsNoCase(theNeedle, substr(theHay, i, aNeedleLen + i)))
					return true;
			}
			i = i + 1;
		} while (i <= aHayLen - aNeedleLen);
	}
	return false;
}

// 0x41e2b0. As the original: when replacing all, the character after each replacement is dropped, and so are the last
// theFind.size() - 1 characters of the string.
Str replace(const Str& theStr, const Str& theFind, const Str& theReplace, bool theCaseSensitive, bool theAll)
{
	Str aResult;
	int aFindLen = (int)theFind.size();
	if ((int)theStr.size() < aFindLen || aFindLen == 0)
		return theStr;
	int i = 0;
	int aCount = ((int)theStr.size() - aFindLen) + 1;
	if (0 < aCount)
	{
		do
		{
			bool aMatch = true;
			int k = 0;
			if (0 < aFindLen)
			{
				do
				{
					if (!theCaseSensitive)
					{
						if (toUpper(at(theStr, k + i)) != toUpper(at(theFind, k)))
						{
							aMatch = false;
							break;
						}
					}
					else if ((char)at(theStr, k + i) != (char)at(theFind, k))
					{
						aMatch = false;
						break;
					}
					k = k + 1;
				} while (k < aFindLen);
			}
			if (!aMatch)
				aResult += (char)at(theStr, i);
			else
			{
				i = i + aFindLen;
				aResult += theReplace;
				if (!theAll)
				{
					if (i < (int)theStr.size())
						aResult += substr(theStr, i, (int)theStr.size());
					break;
				}
			}
			i = i + 1;
		} while (i < aCount);
	}
	return aResult;
}

// 0x41e560
Str format(const Str& theFormat, const Str& the1, const Str& the2, const Str& the3)
{
	Str aResult;
	int aLen = (int)theFormat.size();
	int i = 0;
	if (0 < aLen)
	{
		int j = 1;
		do
		{
			if (theFormat[i] == '%' && j < aLen)
			{
				char aChar = theFormat[j];
				if (aChar == '%')
					aResult += "%";
				else if (aChar == '1')
					aResult += the1;
				else if (aChar == '2')
					aResult += the2;
				else if (aChar == '3')
					aResult += the3;
				else
				{
					aResult += '%';
					aResult += aChar;
				}
				i = i + 2;
				j = j + 2;
			}
			else
			{
				aResult += theFormat[i];
				i = i + 1;
				j = j + 1;
			}
		} while (i < aLen);
	}
	return aResult;
}

// 0x41f850
Str toUpperStr(const Str& theStr)
{
	Str aResult;
	int i = 0;
	if (0 < (int)theStr.size())
	{
		do
		{
			aResult += (char)toUpper(at(theStr, i));
			i = i + 1;
		} while (i < (int)theStr.size());
	}
	return aResult;
}

// 0x41f470
unsigned char toUpper(unsigned char c)
{
	if ('`' < (signed char)c && (signed char)c < '{')
		c = c - 0x20;
	if (0xdf < c)
		c = c - 0x20;
	return c;
}

// 0x41f490
unsigned char toLower(unsigned char c)
{
	if ('@' < (signed char)c && (signed char)c < '[')
		c = c + 0x20;
	if (0xbf < c && c < 0xe0)
		c = c + 0x20;
	return c;
}

// 0x41f5e0
uint32_t hashName(const Str& theName)
{
	uint32_t aHash = 0x3f6ab295;
	int aLen = (int)theName.size();
	int i = 0;
	if (0 < aLen)
	{
		do
		{
			unsigned char c = toUpper((unsigned char)theName[i]);
			aHash = (aHash >> 0x10) + (int)(signed char)c * 7 + aHash * 0x7139;
			i = i + 1;
		} while (i < aLen);
	}
	return aHash;
}

// 0x41f6a0
uint32_t addColor(uint32_t theColor, int theA, int theR, int theG, int theB)
{
	int anAlpha = (int)(theColor >> 0x18) + theA;
	if (anAlpha < 0)
		anAlpha = 0;
	if (0xff < anAlpha)
		anAlpha = 0xff;
	int aRed = (int)(theColor >> 0x10 & 0xff) + theR;
	if (aRed < 0)
		aRed = 0;
	if (0xff < aRed)
		aRed = 0xff;
	int aGreen = (int)(theColor >> 8 & 0xff) + theG;
	if (aGreen < 0)
		aGreen = 0;
	if (0xff < aGreen)
		aGreen = 0xff;
	int aBlue = (int)(theColor & 0xff) + theB;
	if (aBlue < 0)
		aBlue = 0;
	if (0xff < aBlue)
		aBlue = 0xff;
	return (((uint32_t)anAlpha << 8 | aRed) << 8 | aGreen) << 8 | aBlue;
}

// 0x41f750 (x87 arithmetic: the constants are the original's floats)
double angleOf(float theX, float theY)
{
	if (theY != 0.0f)
	{
		double anAngle = std::atan((double)theX / (double)theY);
		anAngle = anAngle + (double)6.28318f;
		if (theY < 0.0f)
			anAngle = anAngle + (double)3.14159f;
		if ((double)6.28318f <= anAngle)
		{
			do
			{
				anAngle = anAngle - (double)6.28318f;
			} while ((double)6.28318f <= anAngle);
		}
		return anAngle;
	}
	if (theX <= 0.0f)
		return 4.712385f;
	return 1.570795f;
}

namespace
{
float gAngleTable[65 * 65];  // 0x47c770
unsigned int gRandSeed = 1;  // the CRT's per-thread holdrand
}

// 0x41f7e0
void buildAngleTable()
{
	float* aCell = gAngleTable;
	int x = -0x20;
	do
	{
		int y = -0x20;
		do
		{
			*aCell++ = (float)angleOf((float)x, (float)y);
			y = y + 1;
		} while (y < 0x21);
		x = x + 1;
	} while (aCell < gAngleTable + 65 * 65);
}

// 0x41f3d0: the coordinates halved until the larger fits in -32..32, then the entry x * 65 + y from the table's centre
float tableAngle(int theX, int theY)
{
	unsigned int x = (unsigned int)theX;
	unsigned int y = (unsigned int)theY;
	int ax = std::abs((int)x);
	int ay = std::abs((int)y);
	if (ay < ax)
	{
		if ((int)x < 1)
		{
			if ((int)x < -0x20)
			{
				do
				{
					x = (int)x >> 1;
					y = (int)y >> 1;
				} while ((int)x < -0x20);
			}
		}
		else if (0x20 < (int)x)
		{
			do
			{
				x = (int)x >> 1;
				y = (int)y >> 1;
			} while (0x20 < (int)x);
		}
	}
	else if ((int)y < 1)
	{
		for (; (int)y < -0x20; y = (int)y >> 1)
			x = (int)x >> 1;
	}
	else if (0x20 < (int)y)
	{
		do
		{
			y = (int)y >> 1;
			x = (int)x >> 1;
		} while (0x20 < (int)y);
	}
	// the table starts at 0x47c770; its centre entry (0, 0), which the original indexes from, is 0x47e870
	return gAngleTable[(int)x * 0x41 + (int)y + 32 * 65 + 32];
}

// 0x41f960
double distance(float theX1, float theY1, float theX2, float theY2)
{
	double dx = (double)theX1 - (double)theX2;
	double dy = (double)theY1 - (double)theY2;
	return std::sqrt(dx * dx + dy * dy);
}

void crtSrand(unsigned int theSeed)
{
	gRandSeed = theSeed;
}

int crtRand()
{
	gRandSeed = gRandSeed * 214013 + 2531011;
	return (int)(gRandSeed >> 16 & 0x7fff);
}

// The port's test hook (-seed=N, src/main.cpp): the seed taken instead of the time
unsigned long gTestSeed = 0;

// 0x41f4e0
void seedRandom(unsigned long theSeed)
{
	if (theSeed == 0)
		theSeed = gTestSeed != 0 ? gTestSeed : (unsigned long)std::time(nullptr);
	crtSrand((unsigned int)theSeed);
}

// 0x41f500
int randomRange(int theLow, int theHigh)
{
	return crtRand() % ((theHigh - theLow) + 1) + theLow;
}

} // namespace bonnies
