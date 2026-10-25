#include "ModVal.h"
#include "Common.h"

// port: ModVal support is compiled out (see ModVal.h); these keep the API for code that calls it
// directly and simply return the compiled-in values.

int Sexy::ModVal(int theAreaNum, const char* theFileName, int theInt)
{
	return theInt;
}

double Sexy::ModVal(int theAreaNum, const char* theFileName, double theDouble)
{
	return theDouble;
}

float Sexy::ModVal(int theAreaNum, const char* theFileName, float theFloat)
{
	return theFloat;
}

const char* Sexy::ModVal(int theAreaNum, const char* theFileName, const char *theStr)
{
	return theStr;
}

bool Sexy::ReparseModValues()
{
	return false;
}

void Sexy::AddModValEnum(const std::string &theEnumName, int theVal)
{
}
