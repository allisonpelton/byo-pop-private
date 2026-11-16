#include "Params.h"

#include <cstdio>
#include <cstdlib>

namespace engine
{

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
