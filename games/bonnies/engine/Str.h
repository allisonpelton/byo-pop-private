#ifndef __BONNIES_STR_H__
#define __BONNIES_STR_H__

// The engine's string class and its helpers (0x41d6c0-0x41e760) and the small utilities beside them (0x41f390-
// 0x41f9b0). The original's string is {int length; char* data} with a null data for the empty string, passed by value;
// std::string holds the same thing. Where the original reads a character past the end it reads 'A' (Str::at).

#include <cstdint>
#include <string>

namespace bonnies
{

typedef std::string Str;

// A character of the string, 'A' past its end (the original's inline index operator)
inline unsigned char at(const Str& theStr, int theIndex)
{
	return (theIndex >= 0 && theIndex < (int)theStr.size()) ? (unsigned char)theStr[theIndex] : (unsigned char)'A';
}

// 0x41d930: the characters from theBegin up to theEnd
Str substr(const Str& theStr, int theBegin, int theEnd);
// 0x41da80: theValue with theDecimals decimals ("%5.<n>f")
Str floatToStr(float theValue, int theDecimals);
// 0x41db50: theValue with thousands separators ("1,234,567")
Str intToStr(int theValue);
// 0x41dd50, 0x41ddc0: atol and atof
long strToInt(const Str& theStr);
float strToFloat(const Str& theStr);
// 0x41de30: theStr without the leading and trailing blanks (and line breaks when theLines is set)
Str trim(const Str& theStr, bool theLines);
// 0x41df30: appends theValue as "%2.1f"
void appendFloat(Str& theStr, float theValue);
// 0x41dfb0: equal ignoring case (toUpper)
bool equalsNoCase(const Str& theA, const Str& theB);
// 0x41e0b0: whether theHay contains theNeedle, ignoring case
bool containsNoCase(const Str& theHay, const Str& theNeedle);
// 0x41e2b0: theStr with theFind replaced by theReplace (the first only unless theAll)
Str replace(const Str& theStr, const Str& theFind, const Str& theReplace, bool theCaseSensitive, bool theAll);
// 0x41e560: theFormat with %1, %2, %3 replaced by the parameters and %% by %
Str format(const Str& theFormat, const Str& the1 = Str(), const Str& the2 = Str(), const Str& the3 = Str());
// 0x41f850: the string upper-cased (toUpper)
Str toUpperStr(const Str& theStr);

// 0x41f470, 0x41f490: the Latin-1 case of a character
unsigned char toUpper(unsigned char c);
unsigned char toLower(unsigned char c);
// 0x41f5e0, 0x41f640: the case-insensitive hash of a name (the pak's and the resources' keys)
uint32_t hashName(const Str& theName);
// 0x41f6a0: a colour 0xAARRGGBB with each channel moved by an amount and clamped
uint32_t addColor(uint32_t theColor, int theA, int theR, int theG, int theB);
// 0x41f990: the same without the alpha
inline uint32_t addRGB(uint32_t theColor, int theR, int theG, int theB)
{
	return addColor(theColor, 0, theR, theG, theB);
}
// 0x41f750: the angle of (theX, theY) from 0 to 2 pi (atan(x / y) turned into a full circle). Like the other x87
// results the engine returns, it is not rounded to a float; the x87 ran at double precision (Visual C++'s default).
double angleOf(float theX, float theY);
// 0x41f7e0: the table of angleOf for -32..32 in both coordinates; 0x41f3d0 reads it after halving to fit
void buildAngleTable();
float tableAngle(int theX, int theY);
// 0x41f960: the distance between two points
double distance(float theX1, float theY1, float theX2, float theY2);
// 0x41f4e0: seeds rand() (with the time when theSeed is 0); 0x41f500: a random number from theLow to theHigh
void seedRandom(unsigned long theSeed);
int randomRange(int theLow, int theHigh);
// The C runtime's rand() as Visual C++ 7.1 has it (0x430366), so the game's sequences are the original's
void crtSrand(unsigned int theSeed);
int crtRand();
// __ftol: a float truncated toward zero
inline int ftol(double theValue)
{
	return (int)theValue;
}

} // namespace bonnies

#endif
