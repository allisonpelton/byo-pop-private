#include "UserInfo.h"

#include <cstdio>

using namespace engine;

// 0x4769b0 (pizzafrenzy)
void Params::setString(const std::string& theName, const std::string& theValue)
{
	(*this)[theName] = theValue;
}

// 0x4769d0 (pizzafrenzy)
void Params::setInt(const std::string& theName, int theValue)
{
	char aBuffer[32];
	snprintf(aBuffer, sizeof(aBuffer), "%d", theValue);
	setString(theName, aBuffer);
}

// 0x476a80 (pizzafrenzy)
void Params::setFloat(const std::string& theName, float theValue)
{
	char aBuffer[64];
	snprintf(aBuffer, sizeof(aBuffer), "%f", (double)theValue);
	setString(theName, aBuffer);
}

// 0x476b30 (pizzafrenzy)
void Params::setBool(const std::string& theName, bool theValue)
{
	setString(theName, theValue ? "true" : "false");
}
