#include "Params.h"

#include "Surface.h"

#include <cctype>
#include <cstdio>
#include <cstdlib>

namespace engine
{

// The C library's _stricmp in the "C" locale: equal but for the case of ASCII letters
static bool equalsIgnoringCase(const char* theString, const char* theLiteral)
{
	for (;; theString++, theLiteral++)
	{
		int aLeft = tolower((unsigned char)*theString);
		int aRight = tolower((unsigned char)*theLiteral);
		if (aLeft != aRight)
			return false;
		if (aLeft == 0)
			return true;
	}
}

std::string Params::getString(const std::string& theName, const std::string& theDefault) const
{
	std::map<std::string, std::string>::const_iterator anIt = mValues.find(theName);
	if (anIt == mValues.end())
		return theDefault;
	return anIt->second;
}

int Params::getInt(const std::string& theName, int theDefault) const
{
	std::map<std::string, std::string>::const_iterator anIt = mValues.find(theName);
	if (anIt == mValues.end())
		return theDefault;
	return (int)atol(anIt->second.c_str());
}

bool Params::getBool(const std::string& theName, bool theDefault) const
{
	std::map<std::string, std::string>::const_iterator anIt = mValues.find(theName);
	if (anIt == mValues.end())
		return theDefault;
	if (anIt->second == "true")
		return true;
	if (anIt->second == "false")
		return false;
	return theDefault;
}

void Params::setInt(const std::string& theName, int theValue)
{
	char aText[32];
	snprintf(aText, sizeof(aText), "%d", theValue);
	mValues[theName] = aText;
}

void Params::setFloat(const std::string& theName, float theValue)
{
	char aText[512];
	snprintf(aText, sizeof(aText), "%f", (double)theValue);
	mValues[theName] = aText;
}

float Params::getFloat(const std::string& theName, float theDefault) const
{
	std::map<std::string, std::string>::const_iterator anIt = mValues.find(theName);
	if (anIt == mValues.end())
		return theDefault;
	return (float)atof(anIt->second.c_str());
}

Vector2<int> Params::getIntPair(const std::string& theName, const Vector2<int>& theDefault) const
{
	Vector2<int> aPair = theDefault;
	std::map<std::string, std::string>::const_iterator anIt = mValues.find(theName);
	if (anIt == mValues.end())
		return aPair;
	sscanf(anIt->second.c_str(), " %d , %d ", &aPair.x, &aPair.y);
	return aPair;
}

Vector2f Params::getFloatPair(const std::string& theName, const Vector2f& theDefault) const
{
	Vector2f aPair = theDefault;
	std::map<std::string, std::string>::const_iterator anIt = mValues.find(theName);
	if (anIt == mValues.end())
		return aPair;
	sscanf(anIt->second.c_str(), " %f , %f ", &aPair.x, &aPair.y);
	return aPair;
}

FloatRange Params::getRange(const std::string& theName, const FloatRange& theDefault) const
{
	FloatRange aRange = theDefault;
	std::map<std::string, std::string>::const_iterator anIt = mValues.find(theName);
	if (anIt == mValues.end())
		return aRange;
	if (anIt->second.c_str()[0] == '(')
		sscanf(anIt->second.c_str(), "( %f , %f )", &aRange.mLow, &aRange.mHigh);
	else
	{
		sscanf(anIt->second.c_str(), "%f", &aRange.mLow);
		aRange.mHigh = aRange.mLow;
	}
	return aRange;
}

IntRange Params::getIntRange(const std::string& theName, const IntRange& theDefault) const
{
	IntRange aRange = theDefault;
	std::map<std::string, std::string>::const_iterator anIt = mValues.find(theName);
	if (anIt == mValues.end())
		return aRange;
	int aLow = 0;
	int aHigh = 0;
	if (anIt->second.c_str()[0] == '(')
	{
		if (sscanf(anIt->second.c_str(), "( %d , %d )", &aLow, &aHigh) != 2)
			return aRange;
		aRange.mLow = aLow;
	}
	else
	{
		if (sscanf(anIt->second.c_str(), "%d", &aHigh) != 1)
			return aRange;
		aRange.mLow = aHigh;
	}
	aRange.mHigh = aHigh;
	return aRange;
}

RectI Params::getRect(const std::string& theName, const RectI& theDefault) const
{
	RectI aRect = theDefault;
	std::map<std::string, std::string>::const_iterator anIt = mValues.find(theName);
	if (anIt == mValues.end())
		return aRect;
	sscanf(anIt->second.c_str(), " %d , %d , %d , %d ", &aRect.left, &aRect.top, &aRect.right, &aRect.bottom);
	return aRect;
}

int Params::getPivot(const std::string& theName) const
{
	std::map<std::string, std::string>::const_iterator anIt = mValues.find(theName);
	if (anIt == mValues.end())
		return PIVOT_CENTER;
	const char* aValue = anIt->second.c_str();
	if (equalsIgnoringCase(aValue, "upperleft"))
		return PIVOT_UPPER_LEFT;
	if (equalsIgnoringCase(aValue, "topcenter"))
		return PIVOT_TOP;
	if (equalsIgnoringCase(aValue, "bottomcenter"))
		return PIVOT_BOTTOM;
	return PIVOT_CENTER;
}

float channelOf(int theValue)
{
	return (float)((double)theValue * 0.003921569f);
}

Color Params::getColor(const std::string& theName, const Color& theDefault) const
{
	Color aColor = theDefault;
	std::map<std::string, std::string>::const_iterator anIt = mValues.find(theName);
	if (anIt == mValues.end())
		return aColor;
	int aRed, aGreen, aBlue, anAlpha;
	int aCount = sscanf(anIt->second.c_str(), " %d , %d , %d , %d ", &aRed, &aGreen, &aBlue, &anAlpha);
	if (aCount > 2)
	{
		aColor.r = channelOf(aRed);
		aColor.g = channelOf(aGreen);
		aColor.b = channelOf(aBlue);
		if (aCount == 4)
			aColor.a = channelOf(anAlpha);
	}
	return aColor;
}

} // namespace engine
